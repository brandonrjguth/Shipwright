#include "soh/Network/Anchor/Anchor.h"
#include <cmath>
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "macros.h"
#include "functions.h"
#include "src/overlays/actors/ovl_En_Dekubaba/z_en_dekubaba.h"
#include "src/overlays/actors/ovl_En_Karebaba/z_en_karebaba.h"
#include "src/overlays/actors/ovl_Obj_Syokudai/z_obj_syokudai.h"
#include "src/overlays/actors/ovl_En_Niw/z_en_niw.h"
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

static bool IsCarryableActorId(s16 actorId) {
    return actorId == ACTOR_EN_NIW || actorId == ACTOR_OBJ_TSUBO || actorId == ACTOR_OBJ_KIBAKO ||
           actorId == ACTOR_EN_BOMBF || actorId == ACTOR_EN_BOM;
}

static bool ReadReportedS16(const nlohmann::json& payload, const char* field, s16& value) {
    if (!payload.contains(field)) {
        return false;
    }
    if (payload[field].is_number_unsigned()) {
        uint64_t encoded = payload[field].get<uint64_t>();
        if (encoded > INT16_MAX) {
            return false;
        }
        value = static_cast<s16>(encoded);
        return true;
    }
    if (!payload[field].is_number_integer()) {
        return false;
    }
    int64_t encoded = payload[field].get<int64_t>();
    if (encoded < INT16_MIN || encoded > INT16_MAX) {
        return false;
    }
    value = static_cast<s16>(encoded);
    return true;
}

static bool ReadReportedU32(const nlohmann::json& payload, const char* field, uint32_t& value) {
    if (!payload.contains(field) || !payload[field].is_number_unsigned()) {
        return false;
    }
    uint64_t encoded = payload[field].get<uint64_t>();
    if (encoded > UINT32_MAX) {
        return false;
    }
    value = static_cast<uint32_t>(encoded);
    return true;
}

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

static bool HasFiniteReportedMotion(const nlohmann::json& payload) {
    constexpr const char* fields[] = { "posX", "posY", "posZ", "velocityX", "velocityY", "velocityZ", "speedXZ",
                                       "gravity", "minVelocityY" };
    for (const char* field : fields) {
        if (payload.contains(field)) {
            if (!payload[field].is_number()) {
                return false;
            }
            f32 value = payload[field].get<f32>();
            if (!std::isfinite(value) || fabsf(value) > 10000000.0f) {
                return false;
            }
        }
    }
    return true;
}

static bool IsAllowedReportedEnemyState(Actor* target, const nlohmann::json& payload) {
    if (!HasReportedEnemyState(payload) || !HasFiniteReportedMotion(payload)) {
        return false;
    }

    if (IsReportedDekunutsState(target, payload) || IsReportedHintnutsState(target, payload) ||
        IsReportedShopnutsCaughtState(target, payload) || IsReportedNutsballReflectedState(target, payload) ||
        IsReportedFhgFireVolleyState(target, payload) || IsReportedBossGanonVolleyState(target, payload) ||
        IsReportedMovableBlockState(target, payload) || IsReportedPuzzleActorState(target, payload) ||
        IsReportedBossGomaState(target, payload)) {
        return true;
    }

    const nlohmann::json& extraState = payload["extraState"];
    std::string kind = extraState.value("kind", std::string(""));
    if (target->id == ACTOR_EN_KAREBABA && kind == "EnKarebaba") {
        s32 action = extraState.value("action", (s32)-1);
        return action == 5 || action == 8;
    }
    if (target->id == ACTOR_OBJ_SYOKUDAI && kind == "ObjSyokudai") {
        return extraState.value("litTimer", (s16)0) > ((ObjSyokudai*)target)->litTimer;
    }
    if (target->id == ACTOR_EN_GOMA && kind == "EnGoma") {
        return payload.value("health", (u8)target->colChkInfo.health) == 0;
    }
    if (IsCarryableActorId(target->id) &&
        extraState.contains("held") && extraState["held"].is_boolean()) {
        return true;
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
    bool isCarryable = IsCarryableActorId(actor->id);
    if (!extraState.is_object() ||
        (extraState.value("kind", std::string("")).empty() && !isCarryable)) {
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

    if (target->id == ACTOR_OBJ_SYOKUDAI) {
        ObjSyokudai* torch = (ObjSyokudai*)target;
        s16 remoteLitTimer = extraState.value("litTimer", (s16)0);
        if (remoteLitTimer > torch->litTimer) {
            torch->litTimer = remoteLitTimer;
        }
        return;
    }

    if (IsCarryableActorId(target->id)) {
        bool remoteHeld = extraState.value("held", false);
        if (remoteHeld && target->parent == nullptr) {
            target->parent = target;
        } else if (!remoteHeld && target->parent == target) {
            target->parent = nullptr;
        }
        if (remoteHeld) {
            target->room = -1;
            // Apply reported position so the authority tracks the remote holder's position.
            target->world.pos.x = payload.value("posX", target->world.pos.x);
            target->world.pos.y = payload.value("posY", target->world.pos.y);
            target->world.pos.z = payload.value("posZ", target->world.pos.z);
        } else {
            // Actor was released/thrown — apply throw velocity so it flies correctly
            // on the authority and propagates to all other replicas.
            target->world.pos.x = payload.value("posX", target->world.pos.x);
            target->world.pos.y = payload.value("posY", target->world.pos.y);
            target->world.pos.z = payload.value("posZ", target->world.pos.z);
            target->velocity.x = payload.value("velocityX", target->velocity.x);
            target->velocity.y = payload.value("velocityY", target->velocity.y);
            target->velocity.z = payload.value("velocityZ", target->velocity.z);
            target->speedXZ = payload.value("speedXZ", target->speedXZ);
        }
        return;
    }

    if (ApplyReportedEnStDeathState(target, extraState)) {
        return;
    }

    if (target->colChkInfo.health == 0) {
        ApplyReportedEnemyDeathMotion(target, payload);
    }
}

void Anchor::SendPacket_DamageEnemy(Actor* actor, u8 health, const EnemyDamageOperationKey* operation) {
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
    payload["enemySessionId"] = enemySessionId;
    payload["networkId"] = GetEnemyNetworkId(actor);
    payload["actorId"] = actor->id;
    payload["health"] = health;
    payload["posX"] = actor->world.pos.x;
    payload["posY"] = actor->world.pos.y;
    payload["posZ"] = actor->world.pos.z;
    payload["category"] = actor->category;
    if (operation != nullptr) {
        payload["damageSourceClientId"] = operation->clientId;
        payload["damageSourceSessionId"] = operation->sessionId;
        payload["damageOperationId"] = operation->operationId;
    }
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

    if (!payload.contains("health") || !payload["health"].is_number_unsigned() ||
        payload["health"].get<uint64_t>() > UINT8_MAX) {
        return;
    }

    uint64_t networkId = payload.value("networkId", (uint64_t)0);
    uint32_t sourceClientId = payload.value("damageSourceClientId", (uint32_t)0);
    uint64_t sourceSessionId = payload.value("damageSourceSessionId", (uint64_t)0);
    uint64_t operationId = payload.value("damageOperationId", (uint64_t)0);
    if (sourceClientId != 0 && sourceSessionId != 0 && operationId != 0) {
        auto& appliedOperations = appliedEnemyDamageOperations[networkId];
        if (appliedOperations.size() >= 512) {
            appliedOperations.erase(appliedOperations.begin());
        }
        appliedOperations.insert({ sourceClientId, sourceSessionId, operationId });
    }
    if (sourceClientId == ownClientId && sourceSessionId == enemySessionId && operationId != 0) {
        auto pending = pendingEnemyDamageOperations.find(networkId);
        if (pending != pendingEnemyDamageOperations.end()) {
            std::erase_if(pending->second, [&](const PendingEnemyDamageOperation& operation) {
                return operation.key.operationId == operationId && operation.key.sessionId == sourceSessionId;
            });
            if (pending->second.empty()) {
                pendingEnemyDamageOperations.erase(pending);
            }
        }
    }

    if (networkId == 0 || IsEnemyMarkedDead(networkId)) {
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
    payload["authoritySessionId"] = clients.contains(authorityClientId) ? clients[authorityClientId].enemySessionId : 0;
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

void Anchor::SendPacket_ReportEnemyDamageOperation(Actor* actor, u8 health, u8 damageAmount,
                                                     PendingEnemyDamageOperation& operation) {
    if (!IsSaveLoaded() || damageAmount == 0 || operation.key.operationId == 0 ||
        operation.key.sessionId == 0) {
        return;
    }

    if (operation.reportPayload.empty()) {
        if (actor == nullptr || GetEnemyNetworkId(actor) == 0) {
            return;
        }
        nlohmann::json& payload = operation.reportPayload;
        payload["type"] = REPORT_ENEMY_DAMAGE;
        payload["reportKind"] = "damage";
        payload["sceneNum"] = gPlayState->sceneNum;
        payload["roomNum"] = gPlayState->roomCtx.curRoom.num;
        payload["operationSessionId"] = operation.key.sessionId;
        payload["operationId"] = operation.key.operationId;
        payload["damageAmount"] = damageAmount;
        payload["networkId"] = GetEnemyNetworkId(actor);
        payload["actorId"] = actor->id;
        payload["actorParams"] = GetEnemySpawnParams(actor);
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
    }

    nlohmann::json payload = operation.reportPayload;
    payload["targetClientId"] = operation.authorityClientId;
    payload["authorityClientId"] = operation.authorityClientId;
    payload["authorityGeneration"] = operation.authorityGeneration;
    payload["authoritySessionId"] = clients.contains(operation.authorityClientId)
                                         ? clients[operation.authorityClientId].enemySessionId
                                         : 0;
    SendJsonToRemote(payload);
}

void Anchor::SendPacket_ReportEnemyState(Actor* actor) {
    if (!IsSaveLoaded() || actor == nullptr) {
        return;
    }

    uint32_t authorityClientId = GetEnemySyncAuthorityClientId();
    uint64_t networkId = GetEnemyNetworkId(actor);
    if (authorityClientId == 0 || authorityClientId == ownClientId || networkId == 0) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = REPORT_ENEMY_DAMAGE;
    payload["reportKind"] = "state";
    payload["targetClientId"] = authorityClientId;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["roomNum"] = gPlayState->roomCtx.curRoom.num;
    payload["authorityClientId"] = authorityClientId;
    payload["authorityGeneration"] =
        GetEnemyRoomAuthorityGeneration(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num);
    payload["authoritySessionId"] = clients.contains(authorityClientId) ? clients[authorityClientId].enemySessionId : 0;
    payload["networkId"] = networkId;
    payload["actorId"] = actor->id;
    payload["health"] = actor->colChkInfo.health;
    payload["posX"] = actor->world.pos.x;
    payload["posY"] = actor->world.pos.y;
    payload["posZ"] = actor->world.pos.z;
    payload["category"] = actor->category;
    AddReportedEnemyContextPayload(actor, payload);
    bool isCarryable = IsCarryableActorId(actor->id);
    if (isCarryable) {
        payload["actorParams"] = GetEnemySpawnParams(actor);
        payload["carryAction"] = actor->parent != nullptr && actor->parent != actor ? "acquire" : "release";
        payload["carryGeneration"] = enemyCarryOwnership[networkId].generation;
        payload["carryReporterSessionId"] = enemySessionId;
    }
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
    if (!client.online || !client.isSaveLoaded || !client.roomStable || client.sceneNum != gPlayState->sceneNum ||
        client.curRoomNum != gPlayState->roomCtx.curRoom.num) {
        return;
    }

    if (payload.value("authorityClientId", (uint32_t)0) != ownClientId) {
        return;
    }
    if (payload.value("authorityGeneration", (uint32_t)0) !=
        GetEnemyRoomAuthorityGeneration(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num)) {
        return;
    }
    if (payload.value("authoritySessionId", (uint64_t)0) != enemySessionId) {
        return;
    }

    uint64_t networkId = payload.value("networkId", (uint64_t)0);
    if (networkId == 0) {
        return;
    }

    if (IsEnemyMarkedDead(networkId)) {
        return;
    }

    if (!payload.contains("health") || !payload["health"].is_number_unsigned() ||
        payload["health"].get<uint64_t>() > UINT8_MAX || !HasFiniteReportedMotion(payload)) {
        return;
    }
    u8 health = payload.at("health").get<u8>();

    std::string reportKind = payload.value("reportKind", std::string(""));
    Actor* target = FindActorByEnemyNetworkId(networkId);
    bool adoptedCarryable = false;
    if (target == nullptr && reportKind == "state" && payload.contains("carryAction") &&
        payload["carryAction"].is_string() && payload.contains("extraState") && payload["extraState"].is_object() &&
        payload["extraState"].contains("held") && payload["extraState"]["held"].is_boolean() &&
        (payload["carryAction"] == "acquire" || payload["carryAction"] == "release") &&
        payload["extraState"]["held"].get<bool>() == (payload["carryAction"] == "acquire") &&
        payload.value("carryReporterSessionId", (uint64_t)0) == client.enemySessionId &&
        payload.value("sceneNum", (s16)SCENE_ID_MAX) == gPlayState->sceneNum &&
        payload.value("roomNum", (s8)-1) == gPlayState->roomCtx.curRoom.num) {
        s16 actorId;
        s16 actorParams;
        s16 worldRotX;
        s16 worldRotY;
        s16 worldRotZ;
        Vec3f reportedPos = { payload.value("posX", client.posRot.pos.x),
                              payload.value("posY", client.posRot.pos.y),
                              payload.value("posZ", client.posRot.pos.z) };
        float distanceSq = SQ(reportedPos.x - client.posRot.pos.x) + SQ(reportedPos.y - client.posRot.pos.y) +
                           SQ(reportedPos.z - client.posRot.pos.z);
        float velocityX = payload.value("velocityX", 0.0f);
        float velocityY = payload.value("velocityY", 0.0f);
        float velocityZ = payload.value("velocityZ", 0.0f);
        float speedXZ = payload.value("speedXZ", 0.0f);
        uint32_t reportedGeneration = UINT32_MAX;
        bool validAdoption = ReadReportedS16(payload, "actorId", actorId) && IsCarryableActorId(actorId) &&
                             ReadReportedS16(payload, "actorParams", actorParams) &&
                             ReadReportedS16(payload, "worldRotX", worldRotX) &&
                             ReadReportedS16(payload, "worldRotY", worldRotY) &&
                             ReadReportedS16(payload, "worldRotZ", worldRotZ) && distanceSq <= SQ(600.0f) &&
                             fabsf(velocityX) <= 100.0f && fabsf(velocityY) <= 100.0f &&
                             fabsf(velocityZ) <= 100.0f && fabsf(speedXZ) <= 100.0f &&
                             ReadReportedU32(payload, "carryGeneration", reportedGeneration);
        EnemyCarryOwnershipState adoptedOwnership;
        auto existingOwnership = enemyCarryOwnership.find(networkId);
        if (existingOwnership != enemyCarryOwnership.end()) {
            adoptedOwnership = existingOwnership->second;
        }
        if (adoptedOwnership.authoritySessionId != enemySessionId) {
            adoptedOwnership.authoritySessionId = enemySessionId;
            adoptedOwnership.generation++;
            if (adoptedOwnership.generation == 0) {
                adoptedOwnership.generation++;
            }
        }
        adoptedOwnership.authorityRoomKey =
            GetEnemyRoomKey(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num);
        if (reportedGeneration > adoptedOwnership.generation &&
            (adoptedOwnership.ownerClientId == 0 || adoptedOwnership.ownerClientId == clientId)) {
            adoptedOwnership = { clientId, reportedGeneration, enemySessionId, adoptedOwnership.authorityRoomKey };
        }
        bool reportedHeld = payload["extraState"]["held"].get<bool>();
        if (!reportedHeld && reportedGeneration == adoptedOwnership.generation &&
            adoptedOwnership.ownerClientId == 0) {
            // The pickup acknowledgement can be lost during the door transition. A session-bound release at the
            // carrier's position is sufficient to adopt and immediately throw the missing incarnation.
            adoptedOwnership.ownerClientId = clientId;
        }
        validAdoption = validAdoption &&
                        ((reportedHeld && reportedGeneration == adoptedOwnership.generation &&
                          (adoptedOwnership.ownerClientId == 0 || adoptedOwnership.ownerClientId == clientId)) ||
                         (!reportedHeld && reportedGeneration == adoptedOwnership.generation &&
                          adoptedOwnership.ownerClientId == clientId));
        if (validAdoption) {
            target = Actor_Spawn(&gPlayState->actorCtx, gPlayState, actorId, reportedPos.x, reportedPos.y,
                                 reportedPos.z, worldRotX, worldRotY, worldRotZ, actorParams);
            if (target != nullptr) {
                CaptureEnemySpawnParams(target);
                SetEnemyNetworkId(target, networkId);
                target->room = -1;
                target->colChkInfo.health = health;
                enemyCarryOwnership[networkId] = adoptedOwnership;
                adoptedCarryable = true;
            }
        }
    }
    if (target == nullptr) {
        return;
    }

    if (payload.value("actorId", target->id) != target->id) {
        return;
    }

    if (reportKind == "state") {
        if (!adoptedCarryable && health != target->colChkInfo.health) {
            return;
        }
        bool isCarryable = IsCarryableActorId(target->id);
        if (isCarryable && payload.contains("carryAction") && payload["carryAction"].is_string()) {
            std::string carryAction = payload["carryAction"].get<std::string>();
            uint32_t reportedGeneration;
            s16 reportedParams;
            if (!ReadReportedU32(payload, "carryGeneration", reportedGeneration) ||
                !ReadReportedS16(payload, "actorParams", reportedParams) ||
                reportedParams != GetEnemySpawnParams(target)) {
                return;
            }
            EnemyCarryOwnershipState& currentOwnership = enemyCarryOwnership[networkId];
            EnemyCarryOwnershipState ownership = currentOwnership;
            if (ownership.authoritySessionId != enemySessionId) {
                ownership.authoritySessionId = enemySessionId;
                ownership.generation++;
                if (ownership.generation == 0) {
                    ownership.generation++;
                }
            }
            ownership.authorityRoomKey = GetEnemyRoomKey(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num);
            if (!payload.contains("extraState") || !payload["extraState"].is_object() ||
                payload.value("carryReporterSessionId", (uint64_t)0) != client.enemySessionId) {
                return;
            }
            const nlohmann::json& extraState = payload["extraState"];
            bool reportedHeld = extraState.value("held", false);
            Vec3f reportedPos = { payload.value("posX", target->world.pos.x),
                                  payload.value("posY", target->world.pos.y),
                                  payload.value("posZ", target->world.pos.z) };
            Vec3f clientPos = client.posRot.pos;
            float distanceSq = SQ(reportedPos.x - clientPos.x) + SQ(reportedPos.y - clientPos.y) +
                               SQ(reportedPos.z - clientPos.z);
            float targetDistanceSq = SQ(target->world.pos.x - clientPos.x) + SQ(target->world.pos.y - clientPos.y) +
                                     SQ(target->world.pos.z - clientPos.z);
            float velocityX = payload.value("velocityX", 0.0f);
            float velocityY = payload.value("velocityY", 0.0f);
            float velocityZ = payload.value("velocityZ", 0.0f);
            float speedXZ = payload.value("speedXZ", 0.0f);
            bool validCarryState = extraState.is_object() && extraState.contains("held") &&
                                   extraState["held"].is_boolean() && distanceSq <= SQ(600.0f) &&
                                   fabsf(velocityX) <= 100.0f && fabsf(velocityY) <= 100.0f &&
                                   fabsf(velocityZ) <= 100.0f && fabsf(speedXZ) <= 100.0f &&
                                   ((carryAction == "acquire" && reportedHeld) ||
                                    (carryAction == "release" && !reportedHeld)) &&
                                   (carryAction != "acquire" || ownership.ownerClientId == clientId ||
                                    targetDistanceSq <= SQ(200.0f));
            if (!validCarryState) {
                return;
            }

            // A newly elected authority may have missed the previous authority's last ownership snapshot. The
            // holder's generation is monotonic and the report is bound to this authority session, so adopt a newer
            // generation before evaluating the requested transition.
            if (reportedGeneration > ownership.generation &&
                (ownership.ownerClientId == 0 || ownership.ownerClientId == clientId)) {
                ownership = { clientId, reportedGeneration, enemySessionId, ownership.authorityRoomKey };
            }

            if (target->parent != nullptr && target->parent != target && ownership.ownerClientId != ownClientId) {
                ownership.ownerClientId = ownClientId;
                ownership.generation++;
                if (ownership.generation == 0) {
                    ownership.generation++;
                }
            }

            bool appliedTransition = false;
            if (carryAction == "acquire" && reportedGeneration == ownership.generation &&
                (ownership.ownerClientId == 0 || ownership.ownerClientId == clientId)) {
                if (ownership.ownerClientId == 0) {
                    ownership.ownerClientId = clientId;
                    ownership.generation++;
                    if (ownership.generation == 0) {
                        ownership.generation++;
                    }
                }
                ApplyReportedEnemyState(target, payload);
                appliedTransition = true;
            } else if (carryAction == "release" && reportedGeneration == ownership.generation &&
                       ownership.ownerClientId == clientId) {
                ownership.ownerClientId = 0;
                ownership.generation++;
                if (ownership.generation == 0) {
                    ownership.generation++;
                }
                ApplyReportedEnemyState(target, payload);
                appliedTransition = true;
            }
            if (appliedTransition) {
                currentOwnership = ownership;
            }
            enemyHealthTracker[target] = target->colChkInfo.health;
            return;
        }
        if (IsAllowedReportedEnemyState(target, payload)) {
            ApplyReportedEnemyState(target, payload);
            enemyHealthTracker[target] = target->colChkInfo.health;
        }
        return;
    }

    if (reportKind == "damage") {
        uint64_t operationSessionId = payload.value("operationSessionId", (uint64_t)0);
        uint64_t operationId = payload.value("operationId", (uint64_t)0);
        constexpr u8 MAX_REPORTED_DAMAGE_AMOUNT = 32;
        if (operationSessionId != client.enemySessionId || !payload.contains("damageAmount") ||
            !payload["damageAmount"].is_number_unsigned()) {
            return;
        }
        uint64_t encodedDamageAmount = payload["damageAmount"].get<uint64_t>();
        if (operationSessionId == 0 || operationId == 0 || encodedDamageAmount == 0 ||
            encodedDamageAmount > MAX_REPORTED_DAMAGE_AMOUNT) {
            return;
        }
        u8 damageAmount = static_cast<u8>(encodedDamageAmount);

        EnemyDamageOperationKey operation = { clientId, operationSessionId, operationId };
        auto& appliedOperations = appliedEnemyDamageOperations[networkId];
        if (appliedOperations.contains(operation)) {
            SendPacket_DamageEnemy(target, target->colChkInfo.health, &operation);
            return;
        }
        if (appliedOperations.size() >= 512) {
            appliedOperations.erase(appliedOperations.begin());
        }
        appliedOperations.insert(operation);

        u8 oldHealth = target->colChkInfo.health;
        target->colChkInfo.health = damageAmount >= oldHealth ? 0 : static_cast<u8>(oldHealth - damageAmount);
        bool hasReportedState = IsAllowedReportedEnemyState(target, payload);
        if (hasReportedState) {
            ApplyReportedEnemyState(target, payload);
        } else if (target->colChkInfo.health == 0) {
            // Generic native death setup is safe; generic network-provided action pointers are not.
            ApplyEnemyExtraState(target, nlohmann::json::object());
        }
        enemyHealthTracker[target] = target->colChkInfo.health;
        SendPacket_DamageEnemy(target, target->colChkInfo.health, &operation);

        if (target->colChkInfo.health == 0) {
            if (payload.value("hintnutsClearRoom", false) && target->id == ACTOR_EN_HINTNUTS) {
                Flags_SetClear(gPlayState, target->room);
            }
            if (!IsEnemySyncActor(target) || !hasReportedState) {
                QueueEnemyKill(networkId);
            }
        }
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

    bool hasReportedState = IsAllowedReportedEnemyState(target, payload);
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
            // Set health to 0, broadcast to all replicas so they can trigger death
            // animations, then queue the kill for deferred processing (death animation
            // plays during the 60-frame deferral in ProcessActorBuffers).
            target->colChkInfo.health = 0;
            enemyHealthTracker[target] = 0;
            SendPacket_DamageEnemy(target, 0);
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
