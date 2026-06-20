#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "macros.h"
#include "functions.h"
#include "src/overlays/actors/ovl_En_Dekubaba/z_en_dekubaba.h"
#include "src/overlays/actors/ovl_En_Karebaba/z_en_karebaba.h"
#include "src/overlays/actors/ovl_En_Goma/z_en_goma.h"
#include "src/overlays/actors/ovl_En_Nutsball/z_en_nutsball.h"
#include "src/overlays/actors/ovl_En_Fhg_Fire/z_en_fhg_fire.h"
#include "src/overlays/actors/ovl_Boss_Ganon/z_boss_ganon.h"
#include "src/overlays/actors/ovl_Boss_Goma/z_boss_goma.h"
#define this thisx
#include "src/overlays/actors/ovl_En_St/z_en_st.h"
#undef this
extern PlayState* gPlayState;

void EnDekubaba_SetupPrunedSomersault(EnDekubaba* thisx);
void EnDekubaba_SetupShrinkDie(EnDekubaba* thisx);
void EnKarebaba_SetupDying(EnKarebaba* thisx);
void EnKarebaba_SetupDead(EnKarebaba* thisx);
void EnGoma_Hurt(EnGoma* thisx, PlayState* play);
void EnGoma_Die(EnGoma* thisx, PlayState* play);
void EnGoma_Dead(EnGoma* thisx, PlayState* play);
void EnGoma_SetupDie(EnGoma* thisx);
void BossGoma_Encounter(BossGoma* thisx, PlayState* play);
void BossGoma_FloorDamaged(BossGoma* thisx, PlayState* play);
void BossGoma_FloorLandStruckDown(BossGoma* thisx, PlayState* play);
void BossGoma_FloorStunned(BossGoma* thisx, PlayState* play);
void BossGoma_FallStruckDown(BossGoma* thisx, PlayState* play);
void BossGoma_SetupFloorDamaged(BossGoma* thisx);
void BossGoma_SetupFloorLandStruckDown(BossGoma* thisx);
void BossGoma_SetupFloorStunned(BossGoma* thisx);
void BossGoma_SetupFallStruckDown(BossGoma* thisx);
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

static bool IsReportedDekunutsState(Actor* target, nlohmann::json payload) {
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
    constexpr s32 DEKUNUTS_ACTION_BE_DAMAGED = 8;
    constexpr s32 DEKUNUTS_ACTION_BE_STUNNED = 9;
    constexpr s32 DEKUNUTS_ACTION_DIE = 10;
    s32 action = extraState.value("action", (s32)-1);
    return action == DEKUNUTS_ACTION_BEGIN_RUN || action == DEKUNUTS_ACTION_RUN || action == DEKUNUTS_ACTION_GASP ||
           action == DEKUNUTS_ACTION_BE_DAMAGED || action == DEKUNUTS_ACTION_BE_STUNNED ||
           action == DEKUNUTS_ACTION_DIE;
}

static bool IsReportedHintnutsState(Actor* target, nlohmann::json payload) {
    if (target == nullptr || target->id != ACTOR_EN_HINTNUTS || !HasReportedEnemyState(payload)) {
        return false;
    }

    nlohmann::json extraState = payload["extraState"];
    if (extraState.value("kind", std::string("")) != "EnHintnuts") {
        return false;
    }

    constexpr s32 HINTNUTS_ACTION_BEGIN_RUN = 5;
    constexpr s32 HINTNUTS_ACTION_BEGIN_FREEZE = 6;
    constexpr s32 HINTNUTS_ACTION_RUN = 7;
    constexpr s32 HINTNUTS_ACTION_FREEZE = 10;
    s32 action = extraState.value("action", (s32)-1);
    return action == HINTNUTS_ACTION_BEGIN_RUN || action == HINTNUTS_ACTION_BEGIN_FREEZE ||
           action == HINTNUTS_ACTION_RUN || action == HINTNUTS_ACTION_FREEZE;
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
    EnNutsball* nutsball = (EnNutsball*)target;
    return extraState.value("kind", std::string("")) == "EnNutsball" &&
           extraState.value("colliderAtTypePlayer", false) &&
           ((nutsball->collider.base.atFlags & AT_TYPE_PLAYER) == 0);
}

static bool IsReportedFhgFireVolleyState(Actor* target, nlohmann::json payload) {
    if (target == nullptr || target->id != ACTOR_EN_FHG_FIRE || target->params != FHGFIRE_ENERGY_BALL ||
        !HasReportedEnemyState(payload)) {
        return false;
    }

    nlohmann::json extraState = payload["extraState"];
    if (extraState.value("kind", std::string("")) != "EnFhgFire") {
        return false;
    }

    // A replica deflected the energy ball: its return counter is ahead of ours, so adopt the volley.
    auto work = extraState.value("work", std::vector<s16>{});
    EnFhgFire* fire = (EnFhgFire*)target;
    return work.size() == FHGFIRE_SHORT_COUNT && work[FHGFIRE_RETURN_COUNT] > fire->work[FHGFIRE_RETURN_COUNT];
}

static bool IsReportedBossGanonVolleyState(Actor* target, nlohmann::json payload) {
    if (target == nullptr || target->id != ACTOR_BOSS_GANON || target->params < 0x64 || target->params > 0xC7 ||
        !HasReportedEnemyState(payload)) {
        return false;
    }

    nlohmann::json extraState = payload["extraState"];
    if (extraState.value("kind", std::string("")) != "BossGanonBall") {
        return false;
    }

    BossGanon* ball = (BossGanon*)target;
    return extraState.value("volleyCount", (s16)0) > ball->unk_1A4;
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

static bool IsReportedBossGomaState(Actor* target, nlohmann::json payload) {
    if (target == nullptr || target->id != ACTOR_BOSS_GOMA || !HasReportedEnemyState(payload)) {
        return false;
    }

    nlohmann::json extraState = payload["extraState"];
    if (extraState.value("kind", std::string("")) != "BossGoma") {
        return false;
    }

    constexpr s32 BOSSGOMA_ACTION_ENCOUNTER = 0;
    constexpr s32 BOSSGOMA_ACTION_FLOOR_DAMAGED = 5;
    constexpr s32 BOSSGOMA_ACTION_FLOOR_LAND_STRUCK_DOWN = 6;
    constexpr s32 BOSSGOMA_ACTION_FLOOR_STUNNED = 8;
    constexpr s32 BOSSGOMA_ACTION_FALL_STRUCK_DOWN = 10;
    s32 action = extraState.value("action", (s32)-1);
    BossGoma* goma = (BossGoma*)target;
    if (action == BOSSGOMA_ACTION_ENCOUNTER) {
        return goma->actionFunc == BossGoma_Encounter && goma->actionState < 4 &&
               extraState.value("actionState", (s32)0) >= 4;
    }

    if (action == BOSSGOMA_ACTION_FLOOR_DAMAGED) {
        return goma->actionFunc != BossGoma_FloorDamaged;
    }
    if (action == BOSSGOMA_ACTION_FLOOR_LAND_STRUCK_DOWN) {
        return goma->actionFunc != BossGoma_FloorLandStruckDown;
    }
    if (action == BOSSGOMA_ACTION_FLOOR_STUNNED) {
        return goma->actionFunc != BossGoma_FloorStunned;
    }
    if (action == BOSSGOMA_ACTION_FALL_STRUCK_DOWN) {
        return goma->actionFunc != BossGoma_FallStruckDown;
    }

    return false;
}

static bool ApplyReportedBossGomaState(Actor* target, nlohmann::json payload) {
    if (target == nullptr || target->id != ACTOR_BOSS_GOMA || !HasReportedEnemyState(payload)) {
        return false;
    }

    nlohmann::json extraState = payload["extraState"];
    if (extraState.value("kind", std::string("")) != "BossGoma") {
        return false;
    }

    BossGoma* goma = (BossGoma*)target;
    constexpr s32 BOSSGOMA_ACTION_ENCOUNTER = 0;
    constexpr s32 BOSSGOMA_ACTION_FLOOR_DAMAGED = 5;
    constexpr s32 BOSSGOMA_ACTION_FLOOR_LAND_STRUCK_DOWN = 6;
    constexpr s32 BOSSGOMA_ACTION_FLOOR_STUNNED = 8;
    constexpr s32 BOSSGOMA_ACTION_FALL_STRUCK_DOWN = 10;

    switch (extraState.value("action", (s32)-1)) {
        case BOSSGOMA_ACTION_ENCOUNTER:
            break;
        case BOSSGOMA_ACTION_FLOOR_DAMAGED:
            if (goma->actionFunc != BossGoma_FloorDamaged) {
                BossGoma_SetupFloorDamaged(goma);
            }
            break;
        case BOSSGOMA_ACTION_FLOOR_LAND_STRUCK_DOWN:
            if (goma->actionFunc != BossGoma_FloorLandStruckDown) {
                BossGoma_SetupFloorLandStruckDown(goma);
            }
            break;
        case BOSSGOMA_ACTION_FLOOR_STUNNED:
            if (goma->actionFunc != BossGoma_FloorStunned) {
                BossGoma_SetupFloorStunned(goma);
            }
            break;
        case BOSSGOMA_ACTION_FALL_STRUCK_DOWN:
            if (goma->actionFunc != BossGoma_FallStruckDown) {
                BossGoma_SetupFallStruckDown(goma);
            }
            break;
        default:
            return false;
    }

    // Apply the reported transition without letting replica motion continuously steer the authority.
    ApplyEnemyExtraState(target, extraState);
    return true;
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

static void EnsureReportedGohmaLarvaDeathState(Actor* target) {
    if (target == nullptr || target->id != ACTOR_EN_GOMA || target->colChkInfo.health != 0) {
        return;
    }

    EnGoma* goma = (EnGoma*)target;
    if (goma->actionFunc == EnGoma_Hurt || goma->actionFunc == EnGoma_Die || goma->actionFunc == EnGoma_Dead) {
        return;
    }

    EnGoma_SetupDie(goma);
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
            EnSt_SetupAction(st, EnSt_Die);
            return true;
        case REPORTED_ENST_ACTION_FINISH_BOUNCING:
            EnSt_SetupAction(st, EnSt_FinishBouncing);
            return true;
        case REPORTED_ENST_ACTION_BOUNCE_AROUND:
            if (st->groundBounces <= 0) {
                EnSt_SetupAction(st, EnSt_FinishBouncing);
                return true;
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
    u8 reportedHealth = payload.value("health", (u8)target->colChkInfo.health);

    if (target->id == ACTOR_BOSS_GOMA && reportedHealth != 0 && ApplyReportedBossGomaState(target, payload)) {
        return;
    }

    ApplyEnemyExtraState(target, extraState);
    EnsureReportedGohmaLarvaDeathState(target);

    bool reportedBossGomaDeath = target->id == ACTOR_BOSS_GOMA && target->colChkInfo.health == 0;
    bool reportedBossGanonVolley = target->id == ACTOR_BOSS_GANON && target->params >= 0x64 && target->params <= 0xC7;
    if (target->id == ACTOR_EN_DEKUNUTS || target->id == ACTOR_EN_HINTNUTS || target->id == ACTOR_EN_SHOPNUTS ||
        target->id == ACTOR_EN_NUTSBALL || target->id == ACTOR_EN_FHG_FIRE || target->id == ACTOR_OBJ_OSHIHIKI ||
        reportedBossGanonVolley || reportedBossGomaDeath || IsReportedPuzzleActorState(target, payload)) {
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

    // Withered Deku Baba: triggered by AC_HIT collision, not health reaching 0.
    // The replica's report includes the action, so the authority can trigger the
    // native death (head pops off, falls, drops a stick, regrows).
    if (target->id == ACTOR_EN_KAREBABA) {
        s32 remoteAction = extraState.value("action", (s32)-1);
        if (remoteAction == 5) { // KAREBABA_ACTION_DYING
            EnKarebaba_SetupDying((EnKarebaba*)target);
            return;
        }
        if (remoteAction == 8) { // KAREBABA_ACTION_DEAD (stick collected)
            EnKarebaba_SetupDead((EnKarebaba*)target);
            return;
        }
    }

    if (ApplyReportedEnStDeathState(target, extraState)) {
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
    if (health == 0) {
        AddReportedEnemyContextPayload(actor, payload);
    }
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
            if (HasReportedEnemyState(payload)) {
                ApplyReportedEnemyState(target, payload);
            }
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

    if (actor->id == ACTOR_EN_GOMA && (s8)actor->colChkInfo.health <= 0) {
        health = 0;
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
    if (actor->id == ACTOR_EN_HINTNUTS && health == 0 && actor->params == 3) {
        payload["hintnutsClearRoom"] = true;
    }
    if (actor->id == ACTOR_EN_NUTSBALL && health == 0) {
        EnNutsball* nutsball = (EnNutsball*)actor;
        payload["projectileKilled"] = (actor->bgCheckFlags & (1 | 8)) ||
                                      (nutsball->collider.base.atFlags & AT_HIT) ||
                                      (nutsball->collider.base.acFlags & AC_HIT) ||
                                      (nutsball->collider.base.ocFlags1 & OC1_HIT);
    }
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

    if (payload.value("hintnutsClearRoom", false) && target->id == ACTOR_EN_HINTNUTS) {
        Flags_SetClear(gPlayState, target->room);
    }

    if (health == 0 && !IsEnemySyncActor(target)) {
        SendPacket_KillEnemy(target);
        enemyKillBuffer.push_back(networkId);
        return;
    }

    bool hasReportedState = HasReportedEnemyState(payload);
    if (target->id == ACTOR_EN_NUTSBALL && health == 0 && hasReportedState &&
        payload.value("projectileKilled", false)) {
        ApplyReportedEnemyState(target, payload);
        SendPacket_KillEnemy(target);
        enemyKillBuffer.push_back(networkId);
        return;
    }

    bool hasReportedNonDamageState = health == target->colChkInfo.health && hasReportedState;

    if (hasReportedNonDamageState) {
        ApplyReportedEnemyState(target, payload);
        enemyHealthTracker[target] = target->colChkInfo.health;
        return;
    }

    // A replica's player collected an item from a defeated enemy that uses the
    // "actor-becomes-collectible" pattern (e.g. DekuBaba DeadStickDrop). The enemy
    // was Actor_Kill'd on the replica; propagate the kill. This path is only reached
    // via OnActorKill (no extraState in payload), so it won't interfere with
    // non-damage state reports that carry extraState.
    if (health == 0 && target->colChkInfo.health == 0 && !hasReportedState) {
        SendPacket_KillEnemy(target);
        enemyKillBuffer.push_back(networkId);
        return;
    }

    if (health < target->colChkInfo.health) {
        if (health == 0 && !hasReportedState) {
            // Set health to 0 so the enemy's own update function triggers its native
            // death animation, then queue the kill for when the animation completes.
            target->colChkInfo.health = 0;
            enemyHealthTracker[target] = 0;
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
