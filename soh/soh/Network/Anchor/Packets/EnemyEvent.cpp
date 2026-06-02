#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>

extern "C" {
#include "variables.h"
extern PlayState* gPlayState;
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
    std::string eventType = payload.at("eventType").get<std::string>();
    nlohmann::json eventData = payload.value("eventData", nlohmann::json::object());

    Vec3f pos = { posX, posY, posZ };
    Actor* target = FindActorByEnemyNetworkId(networkId);
    if (target == nullptr) {
        target = FindClosestUnassignedActorByCategoryAndId((ActorCategory)category, actorId, pos, 100000.0f);
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