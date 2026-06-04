#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "macros.h"
#include "functions.h"
#include "src/overlays/actors/ovl_En_Dekubaba/z_en_dekubaba.h"
#define this thisx
#include "src/overlays/actors/ovl_En_St/z_en_st.h"
#undef this
extern PlayState* gPlayState;

void EnDekubaba_SetupPrunedSomersault(EnDekubaba* thisx);
void EnDekubaba_SetupShrinkDie(EnDekubaba* thisx);
void EnSt_SetupAction(EnSt* thisx, EnStActionFunc actionFunc);
void EnSt_BounceAround(EnSt* thisx, PlayState* play);
void EnSt_FinishBouncing(EnSt* thisx, PlayState* play);
void EnSt_Die(EnSt* thisx, PlayState* play);
}

extern nlohmann::json GetEnemyExtraState(Actor* actor);
extern void ApplyEnemyExtraState(Actor* actor, nlohmann::json extra);

static bool HasReportedEnemyState(nlohmann::json payload) {
    return payload.contains("extraState") && payload["extraState"].is_object() &&
           !payload["extraState"].value("kind", std::string("")).empty();
}

static bool IsReportedDekunutsFleeState(Actor* target, nlohmann::json payload) {
    if (target == nullptr || target->id != ACTOR_EN_DEKUNUTS || !HasReportedEnemyState(payload)) {
        return false;
    }

    nlohmann::json extraState = payload["extraState"];
    if (extraState.value("kind", std::string("")) != "EnDekunuts") {
        return false;
    }

    constexpr s32 DEKUNUTS_ACTION_BEGIN_RUN = 5;
    constexpr s32 DEKUNUTS_ACTION_RUN = 6;
    constexpr s32 DEKUNUTS_ACTION_GASP = 7;
    s32 action = extraState.value("action", (s32)-1);
    return action == DEKUNUTS_ACTION_BEGIN_RUN || action == DEKUNUTS_ACTION_RUN || action == DEKUNUTS_ACTION_GASP;
}

static bool IsReportedShopnutsCaughtState(Actor* target, nlohmann::json payload) {
    if (target == nullptr || target->id != ACTOR_EN_SHOPNUTS || !HasReportedEnemyState(payload)) {
        return false;
    }

    nlohmann::json extraState = payload["extraState"];
    constexpr s32 SHOPNUTS_ACTION_SPAWN_SALESMAN = 5;
    return extraState.value("kind", std::string("")) == "EnShopnuts" &&
           extraState.value("action", (s32)-1) == SHOPNUTS_ACTION_SPAWN_SALESMAN;
}

static bool IsReportedNutsballReflectedState(Actor* target, nlohmann::json payload) {
    if (target == nullptr || target->id != ACTOR_EN_NUTSBALL || !HasReportedEnemyState(payload)) {
        return false;
    }

    nlohmann::json extraState = payload["extraState"];
    return extraState.value("kind", std::string("")) == "EnNutsball" &&
           extraState.value("colliderAtTypePlayer", false);
}

static bool IsReportedMovableBlockState(Actor* target, nlohmann::json payload) {
    if (target == nullptr || target->id != ACTOR_OBJ_OSHIHIKI || !HasReportedEnemyState(payload)) {
        return false;
    }

    return payload["extraState"].value("kind", std::string("")) == "ObjOshihiki";
}

static bool IsReportedPuzzleActorState(Actor* target, nlohmann::json payload) {
    if (target == nullptr || !HasReportedEnemyState(payload)) {
        return false;
    }

    std::string kind = payload["extraState"].value("kind", std::string(""));
    switch (target->id) {
        case ACTOR_OBJ_HSBLOCK: return kind == "ObjHsblock";
        case ACTOR_OBJ_ELEVATOR: return kind == "ObjElevator";
        case ACTOR_OBJ_LIFT: return kind == "ObjLift";
        case ACTOR_OBJ_TIMEBLOCK: return kind == "ObjTimeblock";
        case ACTOR_BG_MIZU_WATER: return kind == "BgMizuWater";
        case ACTOR_BG_MIZU_MOVEBG: return kind == "BgMizuMovebg";
        case ACTOR_BG_MIZU_SHUTTER: return kind == "BgMizuShutter";
        case ACTOR_BG_HIDAN_FSLIFT: return kind == "BgHidanFslift";
        case ACTOR_BG_JYA_COBRA: return kind == "BgJyaCobra";
        case ACTOR_BG_JYA_BIGMIRROR: return kind == "BgJyaBigmirror";
        case ACTOR_BG_HAKA_SHIP: return kind == "BgHakaShip";
        case ACTOR_BG_HAKA_WATER: return kind == "BgHakaWater";
        case ACTOR_BG_HAKA_GATE: return kind == "BgHakaGate";
        case ACTOR_BG_BDAN_OBJECTS: return kind == "BgBdanObjects";
        default: return false;
    }
}

static void AddReportedEnemyContextPayload(Actor* actor, nlohmann::json& payload) {
    nlohmann::json extraState = GetEnemyExtraState(actor);
    if (!extraState.is_object() || extraState.value("kind", std::string("")).empty()) {
        return;
    }

    payload["extraState"] = extraState;
    payload["worldRotX"] = actor->world.rot.x;
    payload["worldRotY"] = actor->world.rot.y;
    payload["worldRotZ"] = actor->world.rot.z;
    payload["shapeRotX"] = actor->shape.rot.x;
    payload["shapeRotY"] = actor->shape.rot.y;
    payload["shapeRotZ"] = actor->shape.rot.z;
    payload["velocityX"] = actor->velocity.x;
    payload["velocityY"] = actor->velocity.y;
    payload["velocityZ"] = actor->velocity.z;
    payload["speedXZ"] = actor->speedXZ;
    payload["gravity"] = actor->gravity;
    payload["minVelocityY"] = actor->minVelocityY;
    payload["actorFlags"] = actor->flags;
}

static void ApplyReportedEnemyDeathMotion(Actor* target, nlohmann::json payload) {
    target->world.pos.x = payload.value("posX", target->world.pos.x);
    target->world.pos.y = payload.value("posY", target->world.pos.y);
    target->world.pos.z = payload.value("posZ", target->world.pos.z);
    target->prevPos = target->world.pos;
    target->world.rot.x = payload.value("worldRotX", target->world.rot.x);
    target->world.rot.y = payload.value("worldRotY", target->world.rot.y);
    target->world.rot.z = payload.value("worldRotZ", target->world.rot.z);
    target->shape.rot.x = payload.value("shapeRotX", target->shape.rot.x);
    target->shape.rot.y = payload.value("shapeRotY", target->shape.rot.y);
    target->shape.rot.z = payload.value("shapeRotZ", target->shape.rot.z);
    target->velocity.x = payload.value("velocityX", target->velocity.x);
    target->velocity.y = payload.value("velocityY", target->velocity.y);
    target->velocity.z = payload.value("velocityZ", target->velocity.z);
    target->speedXZ = payload.value("speedXZ", target->speedXZ);
    target->gravity = payload.value("gravity", target->gravity);
    target->minVelocityY = payload.value("minVelocityY", target->minVelocityY);

    u32 reportedFlags = payload.value("actorFlags", target->flags);
    u32 deathMotionFlags = ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    target->flags = (target->flags & ~deathMotionFlags) | (reportedFlags & deathMotionFlags);
}

enum ReportedEnStAction : s32 {
    REPORTED_ENST_ACTION_BOUNCE_AROUND = 6,
    REPORTED_ENST_ACTION_FINISH_BOUNCING = 7,
    REPORTED_ENST_ACTION_DIE = 8,
};

static bool ApplyReportedEnStDeathState(Actor* target, nlohmann::json extraState) {
    if (target->id != ACTOR_EN_ST || target->colChkInfo.health != 0) {
        return false;
    }

    EnSt* st = (EnSt*)target;
    target->flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;

    switch (extraState.value("action", (s32)-1)) {
        case REPORTED_ENST_ACTION_DIE:
            if (st->finishDeathTimer <= 0) {
                st->finishDeathTimer = 8;
            }
            EnSt_SetupAction(st, EnSt_Die);
            return true;
        case REPORTED_ENST_ACTION_FINISH_BOUNCING:
            EnSt_SetupAction(st, EnSt_FinishBouncing);
            return true;
        case REPORTED_ENST_ACTION_BOUNCE_AROUND:
            if (st->groundBounces <= 0) {
                st->groundBounces = 3;
            }
            if (st->deathTimer <= 0) {
                st->deathTimer = 20;
            }
            if (target->gravity == 0.0f) {
                target->gravity = -1.0f;
            }
            EnSt_SetupAction(st, EnSt_BounceAround);
            return true;
        default:
            return false;
    }
}

static void ApplyReportedEnemyState(Actor* target, nlohmann::json payload) {
    nlohmann::json extraState = payload["extraState"];

    ApplyEnemyExtraState(target, extraState);

    if (target->id == ACTOR_EN_DEKUNUTS || target->id == ACTOR_EN_SHOPNUTS || target->id == ACTOR_EN_NUTSBALL ||
        target->id == ACTOR_OBJ_OSHIHIKI || IsReportedPuzzleActorState(target, payload)) {
        ApplyReportedEnemyDeathMotion(target, payload);
        return;
    }

    if (target->id == ACTOR_EN_DEKUBABA && target->colChkInfo.health == 0) {
        constexpr s32 DEKUBABA_ACTION_PRUNED_SOMERSAULT = 11;
        constexpr s32 DEKUBABA_ACTION_SHRINK_DIE = 12;
        s32 remoteAction = extraState.value("action", (s32)-1);

        if (remoteAction == DEKUBABA_ACTION_PRUNED_SOMERSAULT) {
            EnDekubaba_SetupPrunedSomersault((EnDekubaba*)target);
            return;
        }

        if (remoteAction == DEKUBABA_ACTION_SHRINK_DIE) {
            EnDekubaba_SetupShrinkDie((EnDekubaba*)target);
            return;
        }
    }

    if (ApplyReportedEnStDeathState(target, extraState)) {
        ApplyReportedEnemyDeathMotion(target, payload);
        return;
    }

    if (target->colChkInfo.health == 0) {
        ApplyReportedEnemyDeathMotion(target, payload);
    }
}

void Anchor::SendPacket_DamageEnemy(Actor* actor, u8 health) {
    if (!IsSaveLoaded()) {
        return;
    }

    if (GetEnemyNetworkId(actor) == 0) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = DAMAGE_ENEMY;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["roomNum"] = gPlayState->roomCtx.curRoom.num;
    payload["authorityClientId"] = ownClientId;
    payload["authorityGeneration"] = GetEnemyRoomAuthorityGeneration(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num);
    payload["networkId"] = GetEnemyNetworkId(actor);
    payload["actorId"] = actor->id;
    payload["health"] = health;
    payload["posX"] = actor->world.pos.x;
    payload["posY"] = actor->world.pos.y;
    payload["posZ"] = actor->world.pos.z;
    payload["category"] = actor->category;
    payload["quiet"] = true;

    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_DamageEnemy(nlohmann::json payload) {
    if (!IsSaveLoaded()) {
        return;
    }

    if (!IsValidEnemyAuthorityPacket(payload)) {
        return;
    }

    uint64_t networkId = payload.value("networkId", (uint64_t)0);

    if (IsEnemyMarkedDead(networkId)) {
        return;
    }

    u8 health = payload.at("health").get<u8>();

    Actor* target = FindActorByEnemyNetworkId(networkId);
    if (target == nullptr) {
        return;
    }

    if (health < target->colChkInfo.health) {
        if (health == 0) {
            target->colChkInfo.health = 0;
            enemyHealthTracker[target] = 0;
            return;
        }

        target->colChkInfo.health = health;
        enemyHealthTracker[target] = target->colChkInfo.health;
    }
}

void Anchor::SendPacket_ReportEnemyDamage(Actor* actor, u8 health) {
    if (!IsSaveLoaded()) {
        return;
    }

    uint32_t authorityClientId = GetEnemySyncAuthorityClientId();
    if (authorityClientId == 0 || authorityClientId == ownClientId) {
        return;
    }

    if (GetEnemyNetworkId(actor) == 0) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = REPORT_ENEMY_DAMAGE;
    payload["targetClientId"] = authorityClientId;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["roomNum"] = gPlayState->roomCtx.curRoom.num;
    payload["authorityClientId"] = authorityClientId;
    payload["authorityGeneration"] = GetEnemyRoomAuthorityGeneration(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num);
    payload["networkId"] = GetEnemyNetworkId(actor);
    payload["actorId"] = actor->id;
    payload["health"] = health;
    payload["posX"] = actor->world.pos.x;
    payload["posY"] = actor->world.pos.y;
    payload["posZ"] = actor->world.pos.z;
    payload["category"] = actor->category;
    AddReportedEnemyContextPayload(actor, payload);
    payload["quiet"] = true;

    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_ReportEnemyDamage(nlohmann::json payload) {
    if (!IsRoomStable() || !HasEnemySyncAuthority()) {
        return;
    }

    uint32_t clientId = payload.at("clientId").get<uint32_t>();
    if (!clients.contains(clientId)) {
        return;
    }

    AnchorClient& client = clients[clientId];
    if (client.sceneNum != gPlayState->sceneNum ||
        client.curRoomNum != gPlayState->roomCtx.curRoom.num) {
        return;
    }

    if (payload.value("authorityClientId", (uint32_t)0) != ownClientId) {
        return;
    }

    uint64_t networkId = payload.value("networkId", (uint64_t)0);
    if (networkId == 0) {
        return;
    }

    if (IsEnemyMarkedDead(networkId)) {
        return;
    }

    u8 health = payload.at("health").get<u8>();

    Actor* target = FindActorByEnemyNetworkId(networkId);
    if (target == nullptr) {
        return;
    }

    if (health == 0 && !IsEnemySyncActor(target)) {
        SendPacket_KillEnemy(target);
        enemyKillBuffer.push_back(networkId);
        return;
    }

    bool hasReportedState = HasReportedEnemyState(payload);
    bool hasReportedNonDamageState = health == target->colChkInfo.health &&
                                       (IsReportedDekunutsFleeState(target, payload) ||
                                        IsReportedShopnutsCaughtState(target, payload) ||
                                        IsReportedNutsballReflectedState(target, payload) ||
                                        IsReportedMovableBlockState(target, payload) ||
                                        IsReportedPuzzleActorState(target, payload));

    if (hasReportedNonDamageState) {
        ApplyReportedEnemyState(target, payload);
        enemyHealthTracker[target] = target->colChkInfo.health;
        return;
    }

    if (health < target->colChkInfo.health) {
        if (health == 0 && !hasReportedState) {
            enemyKillBuffer.push_back(networkId);
            return;
        }

        target->colChkInfo.health = health;
        if (hasReportedState) {
            ApplyReportedEnemyState(target, payload);
        }
        enemyHealthTracker[target] = target->colChkInfo.health;
        SendPacket_DamageEnemy(target, target->colChkInfo.health);
    }
}
