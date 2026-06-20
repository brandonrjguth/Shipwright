#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
#include "src/overlays/actors/ovl_En_Hintnuts/z_en_hintnuts.h"
extern PlayState* gPlayState;

void EnHintnuts_SetupTalk(EnHintnuts* thisx);
void EnHintnuts_Talk(EnHintnuts* thisx, PlayState* play);
void EnHintnuts_Leave(EnHintnuts* thisx, PlayState* play);
}

static bool IsHintnutsDialogueMessageActive(Actor* actor) {
    if (actor == nullptr || gPlayState == nullptr) {
        return false;
    }

    Player* player = GET_PLAYER(gPlayState);
    return gPlayState->msgCtx.talkActor == actor || (player != nullptr && player->talkActor == actor);
}

static void ApplyHintnutsDialogueStart(Actor* actor, u16 textId) {
    if (actor == nullptr || actor->id != ACTOR_EN_HINTNUTS || gPlayState == nullptr) {
        return;
    }

    EnHintnuts* hintnuts = (EnHintnuts*)actor;
    if (textId != 0) {
        actor->textId = textId;
        hintnuts->textIdCopy = textId;
    } else if (hintnuts->textIdCopy != 0) {
        actor->textId = hintnuts->textIdCopy;
    }

    actor->speedXZ = 0.0f;
    actor->velocity.x = 0.0f;
    actor->velocity.z = 0.0f;
    actor->flags &= ~(ACTOR_FLAG_HOSTILE | ACTOR_FLAG_TALK_OFFER_AUTO_ACCEPTED);
    actor->flags |= ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_FRIENDLY;
    hintnuts->collider.base.acFlags &= ~AC_ON;
    hintnuts->collider.base.ocFlags1 &= ~OC1_ON;

    if (actor->category != ACTORCAT_BG) {
        Actor_ChangeCategory(gPlayState, &gPlayState->actorCtx, actor, ACTORCAT_BG);
    }
    if (hintnuts->actionFunc != EnHintnuts_Talk && hintnuts->actionFunc != EnHintnuts_Leave) {
        EnHintnuts_SetupTalk(hintnuts);
    }

    if (!IsHintnutsDialogueMessageActive(actor) && Message_GetState(&gPlayState->msgCtx) == TEXT_STATE_NONE &&
        actor->textId != 0) {
        Message_StartTextbox(gPlayState, actor->textId, actor);
    }
}

static void ApplyHintnutsDialogueEnd(Actor* actor) {
    if (actor == nullptr || actor->id != ACTOR_EN_HINTNUTS || gPlayState == nullptr) {
        return;
    }

    if (gPlayState->msgCtx.talkActor == actor) {
        Message_CloseTextbox(gPlayState);
        gPlayState->msgCtx.talkActor = nullptr;
    }
}

void Anchor::SendPacket_HintnutsDialogue(Actor* actor, std::string phase) {
    if (!IsRoomStable() || actor == nullptr || actor->id != ACTOR_EN_HINTNUTS) {
        return;
    }

    uint64_t networkId = GetEnemyNetworkId(actor);
    if (networkId == 0) {
        return;
    }

    bool isStart = phase == "start";
    bool isEnd = phase == "end";
    if (!isStart && !isEnd) {
        return;
    }

    uint32_t authorityClientId = GetEnemySyncAuthorityClientId();
    if (authorityClientId == 0) {
        return;
    }

    if (isStart) {
        if (hintnutsDialogueActive.contains(networkId)) {
            return;
        }
        hintnutsDialogueActive.insert(networkId);
    } else {
        if (!hintnutsDialogueActive.contains(networkId)) {
            return;
        }
        hintnutsDialogueActive.erase(networkId);
    }

    EnHintnuts* hintnuts = (EnHintnuts*)actor;
    nlohmann::json payload;
    payload["type"] = HINTNUTS_DIALOGUE;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["roomNum"] = gPlayState->roomCtx.curRoom.num;
    payload["authorityClientId"] = authorityClientId;
    payload["authorityGeneration"] = GetEnemyRoomAuthorityGeneration(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num);
    payload["networkId"] = networkId;
    payload["actorId"] = actor->id;
    payload["actorParams"] = actor->params;
    payload["category"] = actor->category;
    payload["posX"] = actor->world.pos.x;
    payload["posY"] = actor->world.pos.y;
    payload["posZ"] = actor->world.pos.z;
    payload["phase"] = phase;
    payload["textId"] = hintnuts->textIdCopy != 0 ? hintnuts->textIdCopy : actor->textId;
    if (authorityClientId != ownClientId) {
        payload["targetClientId"] = authorityClientId;
    }
    payload["quiet"] = true;

    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_HintnutsDialogue(nlohmann::json payload) {
    if (!IsRoomStable()) {
        return;
    }

    s16 sceneNum = payload.value("sceneNum", (s16)SCENE_ID_MAX);
    s8 roomNum = payload.value("roomNum", (s8)-1);
    if (sceneNum != gPlayState->sceneNum || roomNum != gPlayState->roomCtx.curRoom.num) {
        return;
    }

    uint32_t clientId = payload.value("clientId", (uint32_t)0);
    uint32_t authorityClientId = payload.value("authorityClientId", (uint32_t)0);
    uint32_t roomAuthorityClientId = GetEnemySyncAuthorityClientId(sceneNum, roomNum);
    if (authorityClientId != roomAuthorityClientId || roomAuthorityClientId == 0) {
        return;
    }

    bool localAuthority = roomAuthorityClientId == ownClientId;
    bool fromAuthority = clientId == roomAuthorityClientId;
    if (localAuthority) {
        if (clientId == ownClientId || !clients.contains(clientId)) {
            return;
        }
        AnchorClient& client = clients[clientId];
        if (!client.online || !client.isSaveLoaded || client.sceneNum != sceneNum || client.curRoomNum != roomNum) {
            return;
        }
    } else if (!fromAuthority) {
        return;
    }

    uint64_t networkId = payload.value("networkId", (uint64_t)0);
    s16 actorId = payload.value("actorId", (s16)0);
    if (networkId == 0 || actorId != ACTOR_EN_HINTNUTS) {
        return;
    }

    Vec3f pos = { payload.value("posX", 0.0f), payload.value("posY", 0.0f), payload.value("posZ", 0.0f) };
    ActorCategory category = (ActorCategory)payload.value("category", (s16)ACTORCAT_BG);
    Actor* target = FindActorByEnemyNetworkId(networkId);
    if (target == nullptr) {
        target = FindClosestUnassignedActorByCategoryAndId(
            category, actorId, pos, 100000.0f, payload.value("actorParams", (s16)-0x8000), roomNum);
        SetEnemyNetworkId(target, networkId);
    }
    if (target == nullptr) {
        return;
    }

    std::string phase = payload.value("phase", std::string(""));
    if (phase == "start") {
        hintnutsDialogueActive.insert(networkId);
        ApplyHintnutsDialogueStart(target, payload.value("textId", (u16)0));
    } else if (phase == "end") {
        hintnutsDialogueActive.erase(networkId);
        ApplyHintnutsDialogueEnd(target);
    } else {
        return;
    }

    if (localAuthority) {
        nlohmann::json forward = payload;
        forward.erase("targetClientId");
        forward["authorityClientId"] = ownClientId;
        forward["authorityGeneration"] = GetEnemyRoomAuthorityGeneration(sceneNum, roomNum);
        forward["quiet"] = true;
        SendJsonToRemote(forward);
    }
}

void Anchor::SendPacket_EnemyEvent(Actor* actor, std::string eventType, nlohmann::json eventData) {
    if (!IsSaveLoaded()) {
        return;
    }

    if (actor->category != ACTORCAT_ENEMY && actor->category != ACTORCAT_BOSS) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = ENEMY_EVENT;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["roomNum"] = gPlayState->roomCtx.curRoom.num;
    payload["authorityClientId"] = ownClientId;
    payload["authorityGeneration"] = GetEnemyRoomAuthorityGeneration(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num);
    payload["networkId"] = GetEnemyNetworkId(actor);
    payload["actorId"] = actor->id;
    payload["posX"] = actor->world.pos.x;
    payload["posY"] = actor->world.pos.y;
    payload["posZ"] = actor->world.pos.z;
    payload["category"] = actor->category;
    payload["eventType"] = eventType;
    payload["eventData"] = eventData;
    payload["quiet"] = true;

    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_EnemyEvent(nlohmann::json payload) {
    if (!IsSaveLoaded()) {
        return;
    }

    if (!IsValidEnemyAuthorityPacket(payload)) {
        return;
    }

    uint64_t networkId = payload.value("networkId", (uint64_t)0);
    s16 actorId = payload.at("actorId").get<s16>();
    float posX = payload.at("posX").get<float>();
    float posY = payload.at("posY").get<float>();
    float posZ = payload.at("posZ").get<float>();
    s16 category = payload.at("category").get<s16>();
    s8 eventRoomNum = payload.value("roomNum", (s8)-1);
    std::string eventType = payload.at("eventType").get<std::string>();
    nlohmann::json eventData = payload.value("eventData", nlohmann::json::object());

    Vec3f pos = { posX, posY, posZ };
    Actor* target = FindActorByEnemyNetworkId(networkId);
    if (target == nullptr) {
        target = FindClosestUnassignedActorByCategoryAndId((ActorCategory)category, actorId, pos, 100000.0f,
                                                           (s16)-0x8000, eventRoomNum);
        SetEnemyNetworkId(target, networkId);
    }
    if (target == nullptr) {
        return;
    }

    if (eventType == "STUN") {
        u16 duration = eventData.value("duration", (u16)30);
        target->freezeTimer = duration;
        u8 colorFilterTimer = eventData.value("colorFilterTimer", (u8)30);
        target->colorFilterTimer = colorFilterTimer;
    } else if (eventType == "FREEZE") {
        u16 duration = eventData.value("duration", (u16)30);
        target->freezeTimer = duration;
        target->colorFilterTimer = duration;
    }
}
