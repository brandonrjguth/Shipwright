#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>

extern "C" {
#include "variables.h"
#include "functions.h"
#include "src/overlays/actors/ovl_En_Dekunuts/z_en_dekunuts.h"
#include "src/overlays/actors/ovl_En_Dekubaba/z_en_dekubaba.h"
#define this thisx
#include "src/overlays/actors/ovl_En_St/z_en_st.h"
#undef this
#include "src/overlays/actors/ovl_En_Ssh/z_en_ssh.h"
#include "src/overlays/actors/ovl_En_Sw/z_en_sw.h"
#include "src/overlays/actors/ovl_En_Wf/z_en_wf.h"
#include "src/overlays/actors/ovl_En_Zf/z_en_zf.h"
#include "src/overlays/actors/ovl_En_Okuta/z_en_okuta.h"
#include "src/overlays/actors/ovl_En_Firefly/z_en_firefly.h"
#include "src/overlays/actors/ovl_En_Bb/z_en_bb.h"
#include "src/overlays/actors/ovl_En_Tite/z_en_tite.h"
#include "src/overlays/actors/ovl_En_Peehat/z_en_peehat.h"
#include "src/overlays/actors/ovl_En_Reeba/z_en_reeba.h"
#include "src/overlays/actors/ovl_Obj_Oshihiki/z_obj_oshihiki.h"
#include "src/overlays/actors/ovl_Obj_Hsblock/z_obj_hsblock.h"
#include "src/overlays/actors/ovl_Obj_Elevator/z_obj_elevator.h"
#include "src/overlays/actors/ovl_Obj_Lift/z_obj_lift.h"
#include "src/overlays/actors/ovl_Obj_Timeblock/z_obj_timeblock.h"
#include "src/overlays/actors/ovl_Boss_Goma/z_boss_goma.h"
#include "src/overlays/actors/ovl_Boss_Dodongo/z_boss_dodongo.h"
#include "src/overlays/actors/ovl_Boss_Ganondrof/z_boss_ganondrof.h"
#include "src/overlays/actors/ovl_Bg_Jya_Bigmirror/z_bg_jya_bigmirror.h"
#include "src/overlays/actors/ovl_Bg_Jya_Cobra/z_bg_jya_cobra.h"
#include "src/overlays/actors/ovl_Bg_Mizu_Water/z_bg_mizu_water.h"
#include "src/overlays/actors/ovl_Bg_Mizu_Movebg/z_bg_mizu_movebg.h"
#include "src/overlays/actors/ovl_Bg_Mizu_Shutter/z_bg_mizu_shutter.h"
#include "src/overlays/actors/ovl_Bg_Hidan_Fslift/z_bg_hidan_fslift.h"
#include "src/overlays/actors/ovl_Bg_Haka_Ship/z_bg_haka_ship.h"
#include "src/overlays/actors/ovl_Bg_Haka_Water/z_bg_haka_water.h"
#include "src/overlays/actors/ovl_Bg_Haka_Gate/z_bg_haka_gate.h"
#include "src/overlays/actors/ovl_Bg_Bdan_Objects/z_bg_bdan_objects.h"

extern "C" {
void EnDekunuts_Wait(EnDekunuts* thisx, PlayState* play);
void EnDekunuts_LookAround(EnDekunuts* thisx, PlayState* play);
void EnDekunuts_Stand(EnDekunuts* thisx, PlayState* play);
void EnDekunuts_ThrowNut(EnDekunuts* thisx, PlayState* play);
void EnDekunuts_Burrow(EnDekunuts* thisx, PlayState* play);
void EnDekunuts_BeginRun(EnDekunuts* thisx, PlayState* play);
void EnDekunuts_Run(EnDekunuts* thisx, PlayState* play);
void EnDekunuts_Gasp(EnDekunuts* thisx, PlayState* play);
void EnDekunuts_BeDamaged(EnDekunuts* thisx, PlayState* play);
void EnDekunuts_BeStunned(EnDekunuts* thisx, PlayState* play);
void EnDekunuts_Die(EnDekunuts* thisx, PlayState* play);
void EnDekubaba_Wait(EnDekubaba*, PlayState*);
void EnDekubaba_Grow(EnDekubaba*, PlayState*);
void EnDekubaba_Retract(EnDekubaba*, PlayState*);
void EnDekubaba_DecideLunge(EnDekubaba*, PlayState*);
void EnDekubaba_PrepareLunge(EnDekubaba*, PlayState*);
void EnDekubaba_Lunge(EnDekubaba*, PlayState*);
void EnDekubaba_PullBack(EnDekubaba*, PlayState*);
void EnDekubaba_Recover(EnDekubaba*, PlayState*);
void EnDekubaba_Hit(EnDekubaba*, PlayState*);
void EnDekubaba_StunnedVertical(EnDekubaba*, PlayState*);
void EnDekubaba_Sway(EnDekubaba*, PlayState*);
void EnDekubaba_PrunedSomersault(EnDekubaba*, PlayState*);
void EnDekubaba_ShrinkDie(EnDekubaba*, PlayState*);
void EnDekubaba_DeadStickDrop(EnDekubaba*, PlayState*);
}

void EnSt_SetupAction(EnSt* thisx, EnStActionFunc actionFunc);
void EnSt_StartOnCeilingOrGround(EnSt* thisx, PlayState* play);
void EnSt_WaitOnCeiling(EnSt* thisx, PlayState* play);
void EnSt_MoveToGround(EnSt* thisx, PlayState* play);
void EnSt_LandOnGround(EnSt* thisx, PlayState* play);
void EnSt_WaitOnGround(EnSt* thisx, PlayState* play);
void EnSt_ReturnToCeiling(EnSt* thisx, PlayState* play);
void EnSt_BounceAround(EnSt* thisx, PlayState* play);
void EnSt_FinishBouncing(EnSt* thisx, PlayState* play);
void EnSt_Die(EnSt* thisx, PlayState* play);
void func_80B0D364(EnSw* thisx, PlayState* play);
void func_80B0D3AC(EnSw* thisx, PlayState* play);
void func_80B0D590(EnSw* thisx, PlayState* play);
void func_80B0D878(EnSw* thisx, PlayState* play);
void func_80B0DB00(EnSw* thisx, PlayState* play);
void func_80B0DC7C(EnSw* thisx, PlayState* play);
void func_80B0E5E0(EnSw* thisx, PlayState* play);
void func_80B0E728(EnSw* thisx, PlayState* play);
void func_80B0E90C(EnSw* thisx, PlayState* play);
void func_80B0E9BC(EnSw* thisx, PlayState* play);
void ObjOshihiki_OnScene(ObjOshihiki* thisx, PlayState* play);
void ObjOshihiki_OnActor(ObjOshihiki* thisx, PlayState* play);
void ObjOshihiki_Push(ObjOshihiki* thisx, PlayState* play);
void ObjOshihiki_Fall(ObjOshihiki* thisx, PlayState* play);
void BossGoma_Encounter(BossGoma* thisx, PlayState* play);
void BossGoma_Defeated(BossGoma* thisx, PlayState* play);
void BossGoma_FloorAttackPosture(BossGoma* thisx, PlayState* play);
void BossGoma_FloorPrepareAttack(BossGoma* thisx, PlayState* play);
void BossGoma_FloorAttack(BossGoma* thisx, PlayState* play);
void BossGoma_FloorDamaged(BossGoma* thisx, PlayState* play);
void BossGoma_FloorLandStruckDown(BossGoma* thisx, PlayState* play);
void BossGoma_FloorLand(BossGoma* thisx, PlayState* play);
void BossGoma_FloorStunned(BossGoma* thisx, PlayState* play);
void BossGoma_FallJump(BossGoma* thisx, PlayState* play);
void BossGoma_FallStruckDown(BossGoma* thisx, PlayState* play);
void BossGoma_CeilingSpawnGohmas(BossGoma* thisx, PlayState* play);
void BossGoma_CeilingPrepareSpawnGohmas(BossGoma* thisx, PlayState* play);
void BossGoma_FloorIdle(BossGoma* thisx, PlayState* play);
void BossGoma_CeilingIdle(BossGoma* thisx, PlayState* play);
void BossGoma_FloorMain(BossGoma* thisx, PlayState* play);
void BossGoma_WallClimb(BossGoma* thisx, PlayState* play);
void BossGoma_CeilingMoveToCenter(BossGoma* thisx, PlayState* play);
void BossDodongo_IntroCutscene(BossDodongo* thisx, PlayState* play);
void BossDodongo_Walk(BossDodongo* thisx, PlayState* play);
void BossDodongo_Inhale(BossDodongo* thisx, PlayState* play);
void BossDodongo_BlowFire(BossDodongo* thisx, PlayState* play);
void BossDodongo_Roll(BossDodongo* thisx, PlayState* play);
void BossDodongo_Explode(BossDodongo* thisx, PlayState* play);
void BossDodongo_LayDown(BossDodongo* thisx, PlayState* play);
void BossDodongo_Vulnerable(BossDodongo* thisx, PlayState* play);
void BossDodongo_GetUp(BossDodongo* thisx, PlayState* play);
void BossDodongo_DeathCutscene(BossDodongo* thisx, PlayState* play);
void BossDodongo_Damaged(BossDodongo* thisx, PlayState* play);
void BossGanondrof_Intro(BossGanondrof* thisx, PlayState* play);
void BossGanondrof_Paintings(BossGanondrof* thisx, PlayState* play);
void BossGanondrof_Neutral(BossGanondrof* thisx, PlayState* play);
void BossGanondrof_Throw(BossGanondrof* thisx, PlayState* play);
void BossGanondrof_Block(BossGanondrof* thisx, PlayState* play);
void BossGanondrof_Return(BossGanondrof* thisx, PlayState* play);
void BossGanondrof_Charge(BossGanondrof* thisx, PlayState* play);
void BossGanondrof_Stunned(BossGanondrof* thisx, PlayState* play);
void BossGanondrof_Death(BossGanondrof* thisx, PlayState* play);

enum DekunutsAction : s32 {
    DEKUNUTS_ACTION_WAIT = 0,
    DEKUNUTS_ACTION_LOOK_AROUND = 1,
    DEKUNUTS_ACTION_STAND = 2,
    DEKUNUTS_ACTION_THROW_NUT = 3,
    DEKUNUTS_ACTION_BURROW = 4,
    DEKUNUTS_ACTION_BEGIN_RUN = 5,
    DEKUNUTS_ACTION_RUN = 6,
    DEKUNUTS_ACTION_GASP = 7,
    DEKUNUTS_ACTION_BE_DAMAGED = 8,
    DEKUNUTS_ACTION_BE_STUNNED = 9,
    DEKUNUTS_ACTION_DIE = 10,
};

static s32 GetDekunutsActionId(EnDekunutsActionFunc actionFunc) {
    if (actionFunc == EnDekunuts_Wait) return DEKUNUTS_ACTION_WAIT;
    if (actionFunc == EnDekunuts_LookAround) return DEKUNUTS_ACTION_LOOK_AROUND;
    if (actionFunc == EnDekunuts_Stand) return DEKUNUTS_ACTION_STAND;
    if (actionFunc == EnDekunuts_ThrowNut) return DEKUNUTS_ACTION_THROW_NUT;
    if (actionFunc == EnDekunuts_Burrow) return DEKUNUTS_ACTION_BURROW;
    if (actionFunc == EnDekunuts_BeginRun) return DEKUNUTS_ACTION_BEGIN_RUN;
    if (actionFunc == EnDekunuts_Run) return DEKUNUTS_ACTION_RUN;
    if (actionFunc == EnDekunuts_Gasp) return DEKUNUTS_ACTION_GASP;
    if (actionFunc == EnDekunuts_BeDamaged) return DEKUNUTS_ACTION_BE_DAMAGED;
    if (actionFunc == EnDekunuts_BeStunned) return DEKUNUTS_ACTION_BE_STUNNED;
    if (actionFunc == EnDekunuts_Die) return DEKUNUTS_ACTION_DIE;
    return -1;
}

static EnDekunutsActionFunc GetDekunutsActionFunc(s32 actionId) {
    switch (actionId) {
        case DEKUNUTS_ACTION_WAIT: return EnDekunuts_Wait;
        case DEKUNUTS_ACTION_LOOK_AROUND: return EnDekunuts_LookAround;
        case DEKUNUTS_ACTION_STAND: return EnDekunuts_Stand;
        case DEKUNUTS_ACTION_THROW_NUT: return EnDekunuts_ThrowNut;
        case DEKUNUTS_ACTION_BURROW: return EnDekunuts_Burrow;
        case DEKUNUTS_ACTION_BEGIN_RUN: return EnDekunuts_BeginRun;
        case DEKUNUTS_ACTION_RUN: return EnDekunuts_Run;
        case DEKUNUTS_ACTION_GASP: return EnDekunuts_Gasp;
        case DEKUNUTS_ACTION_BE_DAMAGED: return EnDekunuts_BeDamaged;
        case DEKUNUTS_ACTION_BE_STUNNED: return EnDekunuts_BeStunned;
        case DEKUNUTS_ACTION_DIE: return EnDekunuts_Die;
        default: return nullptr;
    }
}

static bool IsDekunutsFleeAction(s32 action) {
    return action == DEKUNUTS_ACTION_BEGIN_RUN || action == DEKUNUTS_ACTION_RUN || action == DEKUNUTS_ACTION_GASP;
}

enum ObjOshihikiAction : s32 {
    OBJOSHIHIKI_ACTION_ON_SCENE = 0,
    OBJOSHIHIKI_ACTION_ON_ACTOR = 1,
    OBJOSHIHIKI_ACTION_PUSH = 2,
    OBJOSHIHIKI_ACTION_FALL = 3,
};

static s32 GetObjOshihikiActionId(ObjOshihikiActionFunc actionFunc) {
    if (actionFunc == ObjOshihiki_OnScene) return OBJOSHIHIKI_ACTION_ON_SCENE;
    if (actionFunc == ObjOshihiki_OnActor) return OBJOSHIHIKI_ACTION_ON_ACTOR;
    if (actionFunc == ObjOshihiki_Push) return OBJOSHIHIKI_ACTION_PUSH;
    if (actionFunc == ObjOshihiki_Fall) return OBJOSHIHIKI_ACTION_FALL;
    return -1;
}

static ObjOshihikiActionFunc GetObjOshihikiActionFunc(s32 actionId) {
    switch (actionId) {
        case OBJOSHIHIKI_ACTION_ON_SCENE: return ObjOshihiki_OnScene;
        case OBJOSHIHIKI_ACTION_ON_ACTOR: return ObjOshihiki_OnActor;
        case OBJOSHIHIKI_ACTION_PUSH: return ObjOshihiki_Push;
        case OBJOSHIHIKI_ACTION_FALL: return ObjOshihiki_Fall;
        default: return nullptr;
    }
}

static bool IsLocalPlayerPushingObjOshihiki(ObjOshihiki* block) {
    if (block == nullptr || gPlayState == nullptr) {
        return false;
    }

    Player* player = GET_PLAYER(gPlayState);
    return player != nullptr && (player->stateFlags2 & PLAYER_STATE2_MOVING_DYNAPOLY) &&
           player->actor.wallBgId == block->dyna.bgId;
}

static bool IsObjOshihikiMoving(ObjOshihiki* block) {
    if (block == nullptr) {
        return false;
    }

    s32 action = GetObjOshihikiActionId(block->actionFunc);
    return IsLocalPlayerPushingObjOshihiki(block) || action == OBJOSHIHIKI_ACTION_PUSH ||
           action == OBJOSHIHIKI_ACTION_FALL || fabsf(block->pushDist) > 0.001f ||
           fabsf(block->dyna.actor.velocity.y) > 0.001f;
}

enum DekubabaAction : s32 {
    DEKUBABA_ACTION_WAIT = 0,
    DEKUBABA_ACTION_GROW = 1,
    DEKUBABA_ACTION_RETRACT = 2,
    DEKUBABA_ACTION_DECIDE_LUNGE = 3,
    DEKUBABA_ACTION_PREPARE_LUNGE = 4,
    DEKUBABA_ACTION_LUNGE = 5,
    DEKUBABA_ACTION_PULL_BACK = 6,
    DEKUBABA_ACTION_RECOVER = 7,
    DEKUBABA_ACTION_HIT = 8,
    DEKUBABA_ACTION_STUNNED_VERTICAL = 9,
    DEKUBABA_ACTION_SWAY = 10,
    DEKUBABA_ACTION_PRUNED_SOMERSAULT = 11,
    DEKUBABA_ACTION_SHRINK_DIE = 12,
    DEKUBABA_ACTION_DEAD_STICK_DROP = 13,
};

static s32 GetDekubabaActionId(EnDekubabaActionFunc actionFunc) {
    if (actionFunc == EnDekubaba_Wait) return DEKUBABA_ACTION_WAIT;
    if (actionFunc == EnDekubaba_Grow) return DEKUBABA_ACTION_GROW;
    if (actionFunc == EnDekubaba_Retract) return DEKUBABA_ACTION_RETRACT;
    if (actionFunc == EnDekubaba_DecideLunge) return DEKUBABA_ACTION_DECIDE_LUNGE;
    if (actionFunc == EnDekubaba_PrepareLunge) return DEKUBABA_ACTION_PREPARE_LUNGE;
    if (actionFunc == EnDekubaba_Lunge) return DEKUBABA_ACTION_LUNGE;
    if (actionFunc == EnDekubaba_PullBack) return DEKUBABA_ACTION_PULL_BACK;
    if (actionFunc == EnDekubaba_Recover) return DEKUBABA_ACTION_RECOVER;
    if (actionFunc == EnDekubaba_Hit) return DEKUBABA_ACTION_HIT;
    if (actionFunc == EnDekubaba_StunnedVertical) return DEKUBABA_ACTION_STUNNED_VERTICAL;
    if (actionFunc == EnDekubaba_Sway) return DEKUBABA_ACTION_SWAY;
    if (actionFunc == EnDekubaba_PrunedSomersault) return DEKUBABA_ACTION_PRUNED_SOMERSAULT;
    if (actionFunc == EnDekubaba_ShrinkDie) return DEKUBABA_ACTION_SHRINK_DIE;
    if (actionFunc == EnDekubaba_DeadStickDrop) return DEKUBABA_ACTION_DEAD_STICK_DROP;
    return -1;
}

static EnDekubabaActionFunc GetDekubabaActionFunc(s32 actionId) {
    switch (actionId) {
        case DEKUBABA_ACTION_WAIT: return EnDekubaba_Wait;
        case DEKUBABA_ACTION_GROW: return EnDekubaba_Grow;
        case DEKUBABA_ACTION_RETRACT: return EnDekubaba_Retract;
        case DEKUBABA_ACTION_DECIDE_LUNGE: return EnDekubaba_DecideLunge;
        case DEKUBABA_ACTION_PREPARE_LUNGE: return EnDekubaba_PrepareLunge;
        case DEKUBABA_ACTION_LUNGE: return EnDekubaba_Lunge;
        case DEKUBABA_ACTION_PULL_BACK: return EnDekubaba_PullBack;
        case DEKUBABA_ACTION_RECOVER: return EnDekubaba_Recover;
        case DEKUBABA_ACTION_HIT: return EnDekubaba_Hit;
        case DEKUBABA_ACTION_STUNNED_VERTICAL: return EnDekubaba_StunnedVertical;
        case DEKUBABA_ACTION_SWAY: return EnDekubaba_Sway;
        case DEKUBABA_ACTION_PRUNED_SOMERSAULT: return EnDekubaba_PrunedSomersault;
        case DEKUBABA_ACTION_SHRINK_DIE: return EnDekubaba_ShrinkDie;
        case DEKUBABA_ACTION_DEAD_STICK_DROP: return EnDekubaba_DeadStickDrop;
        default: return nullptr;
    }
}

enum EnStAction : s32 {
    ENST_ACTION_START_ON_CEILING_OR_GROUND = 0,
    ENST_ACTION_WAIT_ON_CEILING = 1,
    ENST_ACTION_MOVE_TO_GROUND = 2,
    ENST_ACTION_LAND_ON_GROUND = 3,
    ENST_ACTION_WAIT_ON_GROUND = 4,
    ENST_ACTION_RETURN_TO_CEILING = 5,
    ENST_ACTION_BOUNCE_AROUND = 6,
    ENST_ACTION_FINISH_BOUNCING = 7,
    ENST_ACTION_DIE = 8,
};

static s32 GetEnStActionId(EnStActionFunc actionFunc) {
    if (actionFunc == EnSt_StartOnCeilingOrGround) return ENST_ACTION_START_ON_CEILING_OR_GROUND;
    if (actionFunc == EnSt_WaitOnCeiling) return ENST_ACTION_WAIT_ON_CEILING;
    if (actionFunc == EnSt_MoveToGround) return ENST_ACTION_MOVE_TO_GROUND;
    if (actionFunc == EnSt_LandOnGround) return ENST_ACTION_LAND_ON_GROUND;
    if (actionFunc == EnSt_WaitOnGround) return ENST_ACTION_WAIT_ON_GROUND;
    if (actionFunc == EnSt_ReturnToCeiling) return ENST_ACTION_RETURN_TO_CEILING;
    if (actionFunc == EnSt_BounceAround) return ENST_ACTION_BOUNCE_AROUND;
    if (actionFunc == EnSt_FinishBouncing) return ENST_ACTION_FINISH_BOUNCING;
    if (actionFunc == EnSt_Die) return ENST_ACTION_DIE;
    return -1;
}

static EnStActionFunc GetEnStActionFunc(s32 actionId) {
    switch (actionId) {
        case ENST_ACTION_START_ON_CEILING_OR_GROUND: return EnSt_StartOnCeilingOrGround;
        case ENST_ACTION_WAIT_ON_CEILING: return EnSt_WaitOnCeiling;
        case ENST_ACTION_MOVE_TO_GROUND: return EnSt_MoveToGround;
        case ENST_ACTION_LAND_ON_GROUND: return EnSt_LandOnGround;
        case ENST_ACTION_WAIT_ON_GROUND: return EnSt_WaitOnGround;
        case ENST_ACTION_RETURN_TO_CEILING: return EnSt_ReturnToCeiling;
        case ENST_ACTION_BOUNCE_AROUND: return EnSt_BounceAround;
        case ENST_ACTION_FINISH_BOUNCING: return EnSt_FinishBouncing;
        case ENST_ACTION_DIE: return EnSt_Die;
        default: return nullptr;
    }
}

enum EnSwAction : s32 {
    ENSW_ACTION_SPAWN_START = 0,
    ENSW_ACTION_SPAWN_RISE = 1,
    ENSW_ACTION_IDLE_GOLD = 2,
    ENSW_ACTION_DIE_GOLD = 3,
    ENSW_ACTION_FALL = 4,
    ENSW_ACTION_DIE_WALL = 5,
    ENSW_ACTION_IDLE_WALL = 6,
    ENSW_ACTION_ATTACK = 7,
    ENSW_ACTION_STOP_ATTACK = 8,
    ENSW_ACTION_RETURN_HOME = 9,
};

static s32 GetEnSwActionId(EnSwActionFunc actionFunc) {
    if (actionFunc == func_80B0D364) return ENSW_ACTION_SPAWN_START;
    if (actionFunc == func_80B0D3AC) return ENSW_ACTION_SPAWN_RISE;
    if (actionFunc == func_80B0D590) return ENSW_ACTION_IDLE_GOLD;
    if (actionFunc == func_80B0D878) return ENSW_ACTION_DIE_GOLD;
    if (actionFunc == func_80B0DB00) return ENSW_ACTION_FALL;
    if (actionFunc == func_80B0DC7C) return ENSW_ACTION_DIE_WALL;
    if (actionFunc == func_80B0E5E0) return ENSW_ACTION_IDLE_WALL;
    if (actionFunc == func_80B0E728) return ENSW_ACTION_ATTACK;
    if (actionFunc == func_80B0E90C) return ENSW_ACTION_STOP_ATTACK;
    if (actionFunc == func_80B0E9BC) return ENSW_ACTION_RETURN_HOME;
    return -1;
}

static EnSwActionFunc GetEnSwActionFunc(s32 actionId) {
    switch (actionId) {
        case ENSW_ACTION_SPAWN_START: return func_80B0D364;
        case ENSW_ACTION_SPAWN_RISE: return func_80B0D3AC;
        case ENSW_ACTION_IDLE_GOLD: return func_80B0D590;
        case ENSW_ACTION_DIE_GOLD: return func_80B0D878;
        case ENSW_ACTION_FALL: return func_80B0DB00;
        case ENSW_ACTION_DIE_WALL: return func_80B0DC7C;
        case ENSW_ACTION_IDLE_WALL: return func_80B0E5E0;
        case ENSW_ACTION_ATTACK: return func_80B0E728;
        case ENSW_ACTION_STOP_ATTACK: return func_80B0E90C;
        case ENSW_ACTION_RETURN_HOME: return func_80B0E9BC;
        default: return nullptr;
    }
}
extern PlayState* gPlayState;
}

enum BossGomaAction : s32 {
    BOSSGOMA_ACTION_ENCOUNTER = 0,
    BOSSGOMA_ACTION_DEFEATED = 1,
    BOSSGOMA_ACTION_FLOOR_ATTACK_POSTURE = 2,
    BOSSGOMA_ACTION_FLOOR_PREPARE_ATTACK = 3,
    BOSSGOMA_ACTION_FLOOR_ATTACK = 4,
    BOSSGOMA_ACTION_FLOOR_DAMAGED = 5,
    BOSSGOMA_ACTION_FLOOR_LAND_STRUCK_DOWN = 6,
    BOSSGOMA_ACTION_FLOOR_LAND = 7,
    BOSSGOMA_ACTION_FLOOR_STUNNED = 8,
    BOSSGOMA_ACTION_FALL_JUMP = 9,
    BOSSGOMA_ACTION_FALL_STRUCK_DOWN = 10,
    BOSSGOMA_ACTION_CEILING_SPAWN_GOHMAS = 11,
    BOSSGOMA_ACTION_CEILING_PREPARE_SPAWN_GOHMAS = 12,
    BOSSGOMA_ACTION_FLOOR_IDLE = 13,
    BOSSGOMA_ACTION_CEILING_IDLE = 14,
    BOSSGOMA_ACTION_FLOOR_MAIN = 15,
    BOSSGOMA_ACTION_WALL_CLIMB = 16,
    BOSSGOMA_ACTION_CEILING_MOVE_TO_CENTER = 17,
};

static s32 GetBossGomaActionId(BossGomaActionFunc actionFunc) {
    if (actionFunc == BossGoma_Encounter) return BOSSGOMA_ACTION_ENCOUNTER;
    if (actionFunc == BossGoma_Defeated) return BOSSGOMA_ACTION_DEFEATED;
    if (actionFunc == BossGoma_FloorAttackPosture) return BOSSGOMA_ACTION_FLOOR_ATTACK_POSTURE;
    if (actionFunc == BossGoma_FloorPrepareAttack) return BOSSGOMA_ACTION_FLOOR_PREPARE_ATTACK;
    if (actionFunc == BossGoma_FloorAttack) return BOSSGOMA_ACTION_FLOOR_ATTACK;
    if (actionFunc == BossGoma_FloorDamaged) return BOSSGOMA_ACTION_FLOOR_DAMAGED;
    if (actionFunc == BossGoma_FloorLandStruckDown) return BOSSGOMA_ACTION_FLOOR_LAND_STRUCK_DOWN;
    if (actionFunc == BossGoma_FloorLand) return BOSSGOMA_ACTION_FLOOR_LAND;
    if (actionFunc == BossGoma_FloorStunned) return BOSSGOMA_ACTION_FLOOR_STUNNED;
    if (actionFunc == BossGoma_FallJump) return BOSSGOMA_ACTION_FALL_JUMP;
    if (actionFunc == BossGoma_FallStruckDown) return BOSSGOMA_ACTION_FALL_STRUCK_DOWN;
    if (actionFunc == BossGoma_CeilingSpawnGohmas) return BOSSGOMA_ACTION_CEILING_SPAWN_GOHMAS;
    if (actionFunc == BossGoma_CeilingPrepareSpawnGohmas) return BOSSGOMA_ACTION_CEILING_PREPARE_SPAWN_GOHMAS;
    if (actionFunc == BossGoma_FloorIdle) return BOSSGOMA_ACTION_FLOOR_IDLE;
    if (actionFunc == BossGoma_CeilingIdle) return BOSSGOMA_ACTION_CEILING_IDLE;
    if (actionFunc == BossGoma_FloorMain) return BOSSGOMA_ACTION_FLOOR_MAIN;
    if (actionFunc == BossGoma_WallClimb) return BOSSGOMA_ACTION_WALL_CLIMB;
    if (actionFunc == BossGoma_CeilingMoveToCenter) return BOSSGOMA_ACTION_CEILING_MOVE_TO_CENTER;
    return -1;
}

static BossGomaActionFunc GetBossGomaActionFunc(s32 actionId) {
    switch (actionId) {
        case BOSSGOMA_ACTION_ENCOUNTER: return BossGoma_Encounter;
        case BOSSGOMA_ACTION_DEFEATED: return BossGoma_Defeated;
        case BOSSGOMA_ACTION_FLOOR_ATTACK_POSTURE: return BossGoma_FloorAttackPosture;
        case BOSSGOMA_ACTION_FLOOR_PREPARE_ATTACK: return BossGoma_FloorPrepareAttack;
        case BOSSGOMA_ACTION_FLOOR_ATTACK: return BossGoma_FloorAttack;
        case BOSSGOMA_ACTION_FLOOR_DAMAGED: return BossGoma_FloorDamaged;
        case BOSSGOMA_ACTION_FLOOR_LAND_STRUCK_DOWN: return BossGoma_FloorLandStruckDown;
        case BOSSGOMA_ACTION_FLOOR_LAND: return BossGoma_FloorLand;
        case BOSSGOMA_ACTION_FLOOR_STUNNED: return BossGoma_FloorStunned;
        case BOSSGOMA_ACTION_FALL_JUMP: return BossGoma_FallJump;
        case BOSSGOMA_ACTION_FALL_STRUCK_DOWN: return BossGoma_FallStruckDown;
        case BOSSGOMA_ACTION_CEILING_SPAWN_GOHMAS: return BossGoma_CeilingSpawnGohmas;
        case BOSSGOMA_ACTION_CEILING_PREPARE_SPAWN_GOHMAS: return BossGoma_CeilingPrepareSpawnGohmas;
        case BOSSGOMA_ACTION_FLOOR_IDLE: return BossGoma_FloorIdle;
        case BOSSGOMA_ACTION_CEILING_IDLE: return BossGoma_CeilingIdle;
        case BOSSGOMA_ACTION_FLOOR_MAIN: return BossGoma_FloorMain;
        case BOSSGOMA_ACTION_WALL_CLIMB: return BossGoma_WallClimb;
        case BOSSGOMA_ACTION_CEILING_MOVE_TO_CENTER: return BossGoma_CeilingMoveToCenter;
        default: return nullptr;
    }
}

enum BossDodongoAction : s32 {
    BOSSDODONGO_ACTION_INTRO_CUTSCENE = 0,
    BOSSDODONGO_ACTION_WALK = 1,
    BOSSDODONGO_ACTION_INHALE = 2,
    BOSSDODONGO_ACTION_BLOW_FIRE = 3,
    BOSSDODONGO_ACTION_ROLL = 4,
    BOSSDODONGO_ACTION_EXPLODE = 5,
    BOSSDODONGO_ACTION_LAY_DOWN = 6,
    BOSSDODONGO_ACTION_VULNERABLE = 7,
    BOSSDODONGO_ACTION_GET_UP = 8,
    BOSSDODONGO_ACTION_DEATH_CUTSCENE = 9,
    BOSSDODONGO_ACTION_DAMAGED = 10,
};

static s32 GetBossDodongoActionId(BossDodongoActionFunc actionFunc) {
    if (actionFunc == BossDodongo_IntroCutscene) return BOSSDODONGO_ACTION_INTRO_CUTSCENE;
    if (actionFunc == BossDodongo_Walk) return BOSSDODONGO_ACTION_WALK;
    if (actionFunc == BossDodongo_Inhale) return BOSSDODONGO_ACTION_INHALE;
    if (actionFunc == BossDodongo_BlowFire) return BOSSDODONGO_ACTION_BLOW_FIRE;
    if (actionFunc == BossDodongo_Roll) return BOSSDODONGO_ACTION_ROLL;
    if (actionFunc == BossDodongo_Explode) return BOSSDODONGO_ACTION_EXPLODE;
    if (actionFunc == BossDodongo_LayDown) return BOSSDODONGO_ACTION_LAY_DOWN;
    if (actionFunc == BossDodongo_Vulnerable) return BOSSDODONGO_ACTION_VULNERABLE;
    if (actionFunc == BossDodongo_GetUp) return BOSSDODONGO_ACTION_GET_UP;
    if (actionFunc == BossDodongo_DeathCutscene) return BOSSDODONGO_ACTION_DEATH_CUTSCENE;
    if (actionFunc == BossDodongo_Damaged) return BOSSDODONGO_ACTION_DAMAGED;
    return -1;
}

static BossDodongoActionFunc GetBossDodongoActionFunc(s32 actionId) {
    switch (actionId) {
        case BOSSDODONGO_ACTION_INTRO_CUTSCENE: return BossDodongo_IntroCutscene;
        case BOSSDODONGO_ACTION_WALK: return BossDodongo_Walk;
        case BOSSDODONGO_ACTION_INHALE: return BossDodongo_Inhale;
        case BOSSDODONGO_ACTION_BLOW_FIRE: return BossDodongo_BlowFire;
        case BOSSDODONGO_ACTION_ROLL: return BossDodongo_Roll;
        case BOSSDODONGO_ACTION_EXPLODE: return BossDodongo_Explode;
        case BOSSDODONGO_ACTION_LAY_DOWN: return BossDodongo_LayDown;
        case BOSSDODONGO_ACTION_VULNERABLE: return BossDodongo_Vulnerable;
        case BOSSDODONGO_ACTION_GET_UP: return BossDodongo_GetUp;
        case BOSSDODONGO_ACTION_DEATH_CUTSCENE: return BossDodongo_DeathCutscene;
        case BOSSDODONGO_ACTION_DAMAGED: return BossDodongo_Damaged;
        default: return nullptr;
    }
}

enum BossGanondrofAction : s32 {
    BOSSGANONDROF_ACTION_INTRO = 0,
    BOSSGANONDROF_ACTION_PAINTINGS = 1,
    BOSSGANONDROF_ACTION_NEUTRAL = 2,
    BOSSGANONDROF_ACTION_THROW = 3,
    BOSSGANONDROF_ACTION_BLOCK = 4,
    BOSSGANONDROF_ACTION_RETURN = 5,
    BOSSGANONDROF_ACTION_CHARGE = 6,
    BOSSGANONDROF_ACTION_STUNNED = 7,
    BOSSGANONDROF_ACTION_DEATH = 8,
};

static s32 GetBossGanondrofActionId(BossGanondrofActionFunc actionFunc) {
    if (actionFunc == BossGanondrof_Intro) return BOSSGANONDROF_ACTION_INTRO;
    if (actionFunc == BossGanondrof_Paintings) return BOSSGANONDROF_ACTION_PAINTINGS;
    if (actionFunc == BossGanondrof_Neutral) return BOSSGANONDROF_ACTION_NEUTRAL;
    if (actionFunc == BossGanondrof_Throw) return BOSSGANONDROF_ACTION_THROW;
    if (actionFunc == BossGanondrof_Block) return BOSSGANONDROF_ACTION_BLOCK;
    if (actionFunc == BossGanondrof_Return) return BOSSGANONDROF_ACTION_RETURN;
    if (actionFunc == BossGanondrof_Charge) return BOSSGANONDROF_ACTION_CHARGE;
    if (actionFunc == BossGanondrof_Stunned) return BOSSGANONDROF_ACTION_STUNNED;
    if (actionFunc == BossGanondrof_Death) return BOSSGANONDROF_ACTION_DEATH;
    return -1;
}

static BossGanondrofActionFunc GetBossGanondrofActionFunc(s32 actionId) {
    switch (actionId) {
        case BOSSGANONDROF_ACTION_INTRO: return BossGanondrof_Intro;
        case BOSSGANONDROF_ACTION_PAINTINGS: return BossGanondrof_Paintings;
        case BOSSGANONDROF_ACTION_NEUTRAL: return BossGanondrof_Neutral;
        case BOSSGANONDROF_ACTION_THROW: return BossGanondrof_Throw;
        case BOSSGANONDROF_ACTION_BLOCK: return BossGanondrof_Block;
        case BOSSGANONDROF_ACTION_RETURN: return BossGanondrof_Return;
        case BOSSGANONDROF_ACTION_CHARGE: return BossGanondrof_Charge;
        case BOSSGANONDROF_ACTION_STUNNED: return BossGanondrof_Stunned;
        case BOSSGANONDROF_ACTION_DEATH: return BossGanondrof_Death;
        default: return nullptr;
    }
}

static void AddVec3fState(nlohmann::json& extra, const std::string& name, Vec3f vec) {
    extra[name + "X"] = vec.x;
    extra[name + "Y"] = vec.y;
    extra[name + "Z"] = vec.z;
}

static void ApplyVec3fState(nlohmann::json extra, const std::string& name, Vec3f* vec) {
    if (vec == nullptr) {
        return;
    }

    vec->x = extra.value(name + "X", vec->x);
    vec->y = extra.value(name + "Y", vec->y);
    vec->z = extra.value(name + "Z", vec->z);
}

static void AddVec3sState(nlohmann::json& extra, const std::string& name, Vec3s vec) {
    extra[name + "X"] = vec.x;
    extra[name + "Y"] = vec.y;
    extra[name + "Z"] = vec.z;
}

static void ApplyVec3sState(nlohmann::json extra, const std::string& name, Vec3s* vec) {
    if (vec == nullptr) {
        return;
    }

    vec->x = extra.value(name + "X", vec->x);
    vec->y = extra.value(name + "Y", vec->y);
    vec->z = extra.value(name + "Z", vec->z);
}

static void AddDynaPolyState(nlohmann::json& extra, DynaPolyActor* dyna) {
    if (dyna == nullptr) {
        return;
    }

    extra["dynaBgId"] = dyna->bgId;
    extra["dynaUnk150"] = dyna->unk_150;
    extra["dynaUnk154"] = dyna->unk_154;
    extra["dynaUnk158"] = dyna->unk_158;
    extra["dynaUnk15A"] = dyna->unk_15A;
    extra["dynaTransformFlags"] = dyna->transformFlags;
    extra["dynaInteractFlags"] = dyna->interactFlags;
    extra["dynaUnk162"] = dyna->unk_162;
    extra["floorHeight"] = dyna->actor.floorHeight;
    extra["bgCheckFlags"] = dyna->actor.bgCheckFlags;
    AddVec3fState(extra, "homePos", dyna->actor.home.pos);
    AddVec3sState(extra, "homeRot", dyna->actor.home.rot);
    AddVec3fState(extra, "prevPos", dyna->actor.prevPos);
}

static void ApplyDynaPolyState(nlohmann::json extra, DynaPolyActor* dyna) {
    if (dyna == nullptr) {
        return;
    }

    dyna->bgId = extra.value("dynaBgId", dyna->bgId);
    dyna->unk_150 = extra.value("dynaUnk150", dyna->unk_150);
    dyna->unk_154 = extra.value("dynaUnk154", dyna->unk_154);
    dyna->unk_158 = extra.value("dynaUnk158", dyna->unk_158);
    dyna->unk_15A = extra.value("dynaUnk15A", dyna->unk_15A);
    dyna->transformFlags = extra.value("dynaTransformFlags", dyna->transformFlags);
    dyna->interactFlags = extra.value("dynaInteractFlags", dyna->interactFlags);
    dyna->unk_162 = extra.value("dynaUnk162", dyna->unk_162);
    dyna->actor.floorHeight = extra.value("floorHeight", dyna->actor.floorHeight);
    dyna->actor.bgCheckFlags = extra.value("bgCheckFlags", dyna->actor.bgCheckFlags);
    ApplyVec3fState(extra, "homePos", &dyna->actor.home.pos);
    ApplyVec3sState(extra, "homeRot", &dyna->actor.home.rot);
    ApplyVec3fState(extra, "prevPos", &dyna->actor.prevPos);
}

static bool IsDynaPolyMoving(DynaPolyActor* dyna) {
    if (dyna == nullptr) {
        return false;
    }

    Actor* actor = &dyna->actor;
    return fabsf(actor->velocity.x) > 0.001f || fabsf(actor->velocity.y) > 0.001f ||
           fabsf(actor->velocity.z) > 0.001f || fabsf(actor->speedXZ) > 0.001f ||
           fabsf(dyna->unk_150) > 0.001f || fabsf(dyna->unk_154) > 0.001f;
}

static bool IsPuzzleActorActive(Actor* actor) {
    if (actor == nullptr) {
        return false;
    }

    switch (actor->id) {
        case ACTOR_OBJ_HSBLOCK:
            return IsDynaPolyMoving(&((ObjHsblock*)actor)->dyna);
        case ACTOR_OBJ_ELEVATOR:
            return IsDynaPolyMoving(&((ObjElevator*)actor)->dyna);
        case ACTOR_OBJ_LIFT: {
            ObjLift* lift = (ObjLift*)actor;
            return lift->timer > 0 || IsDynaPolyMoving(&lift->dyna);
        }
        case ACTOR_OBJ_TIMEBLOCK: {
            ObjTimeblock* timeblock = (ObjTimeblock*)actor;
            return timeblock->demoEffectTimer > 0 || timeblock->songEndTimer > 0 || IsDynaPolyMoving(&timeblock->dyna);
        }
        case ACTOR_BG_MIZU_WATER: {
            BgMizuWater* water = (BgMizuWater*)actor;
            return fabsf(water->actor.world.pos.y - water->targetY) > 0.01f || fabsf(water->actor.velocity.y) > 0.001f;
        }
        case ACTOR_BG_MIZU_MOVEBG:
            return IsDynaPolyMoving(&((BgMizuMovebg*)actor)->dyna);
        case ACTOR_BG_MIZU_SHUTTER: {
            BgMizuShutter* shutter = (BgMizuShutter*)actor;
            return shutter->timer > 0 || IsDynaPolyMoving(&shutter->dyna);
        }
        case ACTOR_BG_HIDAN_FSLIFT: {
            BgHidanFslift* lift = (BgHidanFslift*)actor;
            return lift->timer > 0 || IsDynaPolyMoving(&lift->dyna);
        }
        case ACTOR_BG_JYA_COBRA: {
            BgJyaCobra* cobra = (BgJyaCobra*)actor;
            return cobra->unk_170 != 0 || IsDynaPolyMoving(&cobra->dyna);
        }
        case ACTOR_BG_HAKA_SHIP: {
            BgHakaShip* ship = (BgHakaShip*)actor;
            return ship->counter > 0 || IsDynaPolyMoving(&ship->dyna);
        }
        case ACTOR_BG_HAKA_GATE: {
            BgHakaGate* gate = (BgHakaGate*)actor;
            return IsDynaPolyMoving(&gate->dyna);
        }
        case ACTOR_BG_BDAN_OBJECTS: {
            BgBdanObjects* objects = (BgBdanObjects*)actor;
            return IsDynaPolyMoving(&objects->dyna);
        }
        default:
            return false;
    }
}

static void AddSkelAnimeState(nlohmann::json& extra, SkelAnime* skelAnime) {
    if (skelAnime == nullptr) {
        return;
    }

    extra["skelCurFrame"] = skelAnime->curFrame;
    extra["skelPlaySpeed"] = skelAnime->playSpeed;
    extra["skelMode"] = skelAnime->mode;
    extra["skelStartFrame"] = skelAnime->startFrame;
    extra["skelEndFrame"] = skelAnime->endFrame;
    extra["skelMorphWeight"] = skelAnime->morphWeight;
    extra["skelMorphRate"] = skelAnime->morphRate;
}

static void ApplySkelAnimeState(nlohmann::json extra, SkelAnime* skelAnime) {
    if (skelAnime == nullptr) {
        return;
    }

    skelAnime->curFrame = extra.value("skelCurFrame", skelAnime->curFrame);
    skelAnime->playSpeed = extra.value("skelPlaySpeed", skelAnime->playSpeed);
    skelAnime->mode = extra.value("skelMode", skelAnime->mode);
    skelAnime->startFrame = extra.value("skelStartFrame", skelAnime->startFrame);
    skelAnime->endFrame = extra.value("skelEndFrame", skelAnime->endFrame);
    skelAnime->morphWeight = extra.value("skelMorphWeight", skelAnime->morphWeight);
    skelAnime->morphRate = extra.value("skelMorphRate", skelAnime->morphRate);
}

static void EnsureEnStDeathState(EnSt* st) {
    if (st == nullptr || st->actor.colChkInfo.health != 0) {
        return;
    }

    st->actor.flags &= ~(ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_ATTACHED_TO_ARROW);
    s32 action = GetEnStActionId(st->actionFunc);
    if (action == ENST_ACTION_BOUNCE_AROUND) {
        if (st->groundBounces <= 0) {
            st->groundBounces = 3;
        }
        if (st->deathTimer <= 0) {
            st->deathTimer = 20;
        }
        if (st->actor.gravity == 0.0f) {
            st->actor.gravity = -1.0f;
        }
        return;
    }
    if (action == ENST_ACTION_FINISH_BOUNCING) {
        if (st->deathTimer <= 0 && st->finishDeathTimer <= 0) {
            st->deathTimer = 20;
        }
        return;
    }
    if (action == ENST_ACTION_DIE) {
        if (st->finishDeathTimer <= 0) {
            st->finishDeathTimer = 8;
        }
        return;
    }

    st->swayTimer = 0;
    st->stunTimer = 0;
    st->takeDamageSpinTimer = 0;
    st->gaveDamageSpinTimer = 0;
    st->groundBounces = 3;
    st->deathTimer = 20;
    st->actor.gravity = -1.0f;
    EnSt_SetupAction(st, EnSt_BounceAround);
}

static void EnsureEnSwDeathState(EnSw* sw) {
    if (sw == nullptr || sw->actor.colChkInfo.health != 0) {
        return;
    }

    sw->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
    s32 skulltulaType = (sw->actor.params & 0xE000) >> 0xD;
    s32 action = GetEnSwActionId(sw->actionFunc);
    if (skulltulaType != 0) {
        if (action == ENSW_ACTION_DIE_GOLD) {
            return;
        }

        sw->skelAnime.playSpeed = 8.0f;
        if (sw->unk_394 <= 0) {
            sw->unk_394 = 10;
        }
        if (sw->unk_38A <= 0) {
            sw->unk_38A = 1;
        }
        if (sw->unk_420 == 0.0f) {
            sw->unk_420 = 0.4f;
        }
        sw->actionFunc = func_80B0D878;
        return;
    }

    if (action == ENSW_ACTION_FALL || action == ENSW_ACTION_DIE_WALL) {
        return;
    }

    sw->actor.shape.shadowDraw = ActorShadow_DrawCircle;
    sw->actor.shape.shadowAlpha = 0xFF;
    sw->actor.shape.shadowScale = 16.0f;
    sw->actor.gravity = -1.0f;
    sw->unk_38A = 2;
    sw->actionFunc = func_80B0DB00;
}

bool ShouldReportEnemyExtraState(Actor* actor) {
    if (actor == nullptr) {
        return false;
    }

    if (actor->id == ACTOR_EN_DEKUNUTS) {
        EnDekunuts* dekunuts = (EnDekunuts*)actor;
        return IsDekunutsFleeAction(GetDekunutsActionId(dekunuts->actionFunc));
    }

    if (actor->id == ACTOR_OBJ_OSHIHIKI) {
        return IsObjOshihikiMoving((ObjOshihiki*)actor);
    }

    if (IsPuzzleActorActive(actor)) {
        return true;
    }

    return false;
}

nlohmann::json GetEnemyExtraState(Actor* actor) {
    nlohmann::json extra = nlohmann::json::object();

    switch (actor->id) {
        case ACTOR_EN_DEKUNUTS: {
            EnDekunuts* dekunuts = (EnDekunuts*)actor;
            extra["kind"] = "EnDekunuts";
            extra["action"] = GetDekunutsActionId(dekunuts->actionFunc);
            extra["playWalkSound"] = dekunuts->playWalkSound;
            extra["runAwayCount"] = dekunuts->runAwayCount;
            extra["animFlagAndTimer"] = dekunuts->animFlagAndTimer;
            extra["runDirection"] = dekunuts->runDirection;
            extra["shotsPerRound"] = dekunuts->shotsPerRound;
            extra["colliderAcOn"] = (dekunuts->collider.base.acFlags & AC_ON) != 0;
            extra["colliderHeight"] = dekunuts->collider.dim.height;
            extra["mass"] = dekunuts->actor.colChkInfo.mass;
            AddSkelAnimeState(extra, &dekunuts->skelAnime);
            break;
        }
        case ACTOR_EN_DEKUBABA: {
            EnDekubaba* dekubaba = (EnDekubaba*)actor;
            extra["kind"] = "EnDekubaba";
            extra["action"] = GetDekubabaActionId(dekubaba->actionFunc);
            extra["timer"] = dekubaba->timer;
            extra["targetSwayAngle"] = dekubaba->targetSwayAngle;
            extra["stemSectionAngle"] = { dekubaba->stemSectionAngle[0], dekubaba->stemSectionAngle[1],
                                           dekubaba->stemSectionAngle[2] };
            extra["size"] = dekubaba->size;
            extra["colliderColType"] = dekubaba->collider.base.colType;
            extra["colliderAcHard"] = (dekubaba->collider.base.acFlags & AC_HARD) != 0;
            AddSkelAnimeState(extra, &dekubaba->skelAnime);
            break;
        }
        case ACTOR_EN_ST: {
            EnSt* st = (EnSt*)actor;
            extra["kind"] = "EnSt";
            extra["action"] = GetEnStActionId(st->actionFunc);
            extra["groundBounces"] = st->groundBounces;
            extra["deathTimer"] = st->deathTimer;
            extra["finishDeathTimer"] = st->finishDeathTimer;
            extra["setTargetYawTimer"] = st->setTargetYawTimer;
            extra["deathYawTarget"] = st->deathYawTarget;
            extra["rotAwayTimer"] = st->rotAwayTimer;
            extra["rotTowardsTimer"] = st->rotTowardsTimer;
            extra["takeDamageSpinTimer"] = st->takeDamageSpinTimer;
            extra["stunTimer"] = st->stunTimer;
            extra["swayTimer"] = st->swayTimer;
            extra["swayAngle"] = st->swayAngle;
            extra["animFrames"] = st->animFrames;
            extra["sfxTimer"] = st->sfxTimer;
            extra["gaveDamageSpinTimer"] = st->gaveDamageSpinTimer;
            extra["shapeYOffset"] = st->actor.shape.yOffset;
            extra["floorHeightOffset"] = st->floorHeightOffset;
            extra["colliderScale"] = st->colliderScale;
            AddSkelAnimeState(extra, &st->skelAnime);
            break;
        }
        case ACTOR_EN_SSH: {
            EnSsh* ssh = (EnSsh*)actor;
            extra["kind"] = "EnSsh";
            extra["spinTimer"] = ssh->spinTimer;
            extra["hitTimer"] = ssh->hitTimer;
            extra["stunTimer"] = ssh->stunTimer;
            extra["animTimer"] = ssh->animTimer;
            extra["swayTimer"] = ssh->swayTimer;
            extra["swayAngle"] = ssh->swayAngle;
            extra["stateFlags"] = ssh->stateFlags;
            extra["hitCount"] = ssh->hitCount;
            extra["floorHeightOffset"] = ssh->floorHeightOffset;
            extra["colliderScale"] = ssh->colliderScale;
            AddSkelAnimeState(extra, &ssh->skelAnime);
            break;
        }
        case ACTOR_EN_SW: {
            EnSw* sw = (EnSw*)actor;
            extra["kind"] = "EnSw";
            extra["action"] = GetEnSwActionId(sw->actionFunc);
            extra["unk_388"] = sw->unk_388;
            extra["unk_38A"] = sw->unk_38A;
            extra["unk_38C"] = sw->unk_38C;
            extra["unk_38E"] = sw->unk_38E;
            extra["unk_390"] = sw->unk_390;
            extra["unk_392"] = sw->unk_392;
            extra["unk_394"] = sw->unk_394;
            extra["unk_420"] = sw->unk_420;
            extra["unk_42C"] = sw->unk_42C;
            extra["unk_440"] = sw->unk_440;
            extra["unk_442"] = sw->unk_442;
            extra["unk_444"] = sw->unk_444;
            extra["unk_446"] = sw->unk_446;
            AddSkelAnimeState(extra, &sw->skelAnime);
            break;
        }
        case ACTOR_EN_WF: {
            EnWf* wf = (EnWf*)actor;
            extra["kind"] = "EnWf";
            extra["action"] = wf->action;
            extra["actionTimer"] = wf->actionTimer;
            extra["runSpeed"] = wf->runSpeed;
            extra["slashStatus"] = wf->slashStatus;
            extra["switchFlag"] = wf->switchFlag;
            extra["runAngle"] = wf->runAngle;
            extra["fireTimer"] = wf->fireTimer;
            extra["damageEffect"] = wf->damageEffect;
            AddSkelAnimeState(extra, &wf->skelAnime);
            break;
        }
        case ACTOR_EN_ZF: {
            EnZf* zf = (EnZf*)actor;
            extra["kind"] = "EnZf";
            extra["action"] = zf->action;
            extra["hopAnimIndex"] = zf->hopAnimIndex;
            extra["headRot"] = zf->headRot;
            extra["headRotTemp"] = zf->headRotTemp;
            extra["iceTimer"] = zf->iceTimer;
            extra["swordSheathed"] = zf->swordSheathed;
            extra["clearFlag"] = zf->clearFlag;
            extra["curPlatform"] = zf->curPlatform;
            extra["homePlatform"] = zf->homePlatform;
            extra["nextPlatform"] = zf->nextPlatform;
            extra["damageEffect"] = zf->damageEffect;
            AddSkelAnimeState(extra, &zf->skelAnime);
            break;
        }
        case ACTOR_EN_OKUTA: {
            EnOkuta* okuta = (EnOkuta*)actor;
            extra["kind"] = "EnOkuta";
            extra["timer"] = okuta->timer;
            extra["numShots"] = okuta->numShots;
            extra["jumpHeight"] = okuta->jumpHeight;
            extra["headScaleX"] = okuta->headScale.x;
            extra["headScaleY"] = okuta->headScale.y;
            extra["headScaleZ"] = okuta->headScale.z;
            AddSkelAnimeState(extra, &okuta->skelAnime);
            break;
        }
        case ACTOR_EN_FIREFLY: {
            EnFirefly* firefly = (EnFirefly*)actor;
            extra["kind"] = "EnFirefly";
            extra["auraType"] = firefly->auraType;
            extra["onFire"] = firefly->onFire;
            extra["timer"] = firefly->timer;
            extra["targetPitch"] = firefly->targetPitch;
            extra["maxAltitude"] = firefly->maxAltitude;
            AddSkelAnimeState(extra, &firefly->skelAnime);
            break;
        }
        case ACTOR_EN_BB: {
            EnBb* bb = (EnBb*)actor;
            extra["kind"] = "EnBb";
            extra["action"] = bb->action;
            extra["moveMode"] = bb->moveMode;
            extra["timer"] = bb->timer;
            extra["actionState"] = bb->actionState;
            extra["charge"] = bb->charge;
            extra["actionVar1"] = bb->actionVar1;
            extra["actionVar2"] = bb->actionVar2;
            extra["flameScrollMod"] = bb->flameScrollMod;
            extra["bobPhase"] = bb->bobPhase;
            extra["bobSize"] = bb->bobSize;
            extra["maxSpeed"] = bb->maxSpeed;
            extra["fireIceTimer"] = bb->fireIceTimer;
            extra["dmgEffect"] = bb->dmgEffect;
            AddSkelAnimeState(extra, &bb->skelAnime);
            break;
        }
        case ACTOR_EN_TITE: {
            EnTite* tite = (EnTite*)actor;
            extra["kind"] = "EnTite";
            extra["action"] = tite->action;
            extra["flipState"] = tite->flipState;
            extra["actionVar1"] = tite->actionVar1;
            extra["actionVar2"] = tite->actionVar2;
            extra["spawnIceTimer"] = tite->spawnIceTimer;
            extra["damageEffect"] = tite->damageEffect;
            AddSkelAnimeState(extra, &tite->skelAnime);
            break;
        }
        case ACTOR_EN_PEEHAT: {
            EnPeehat* peehat = (EnPeehat*)actor;
            extra["kind"] = "EnPeehat";
            extra["state"] = peehat->state;
            extra["bladeRotVel"] = peehat->bladeRotVel;
            extra["bladeRot"] = peehat->bladeRot;
            extra["riseDelayTimer"] = peehat->riseDelayTimer;
            extra["seekPlayerTimer"] = peehat->seekPlayerTimer;
            extra["animTimer"] = peehat->animTimer;
            extra["jiggleRot"] = peehat->jiggleRot;
            extra["jiggleRotInc"] = peehat->jiggleRotInc;
            extra["scaleShift"] = peehat->scaleShift;
            AddSkelAnimeState(extra, &peehat->skelAnime);
            break;
        }
        case ACTOR_EN_REEBA: {
            EnReeba* reeba = (EnReeba*)actor;
            extra["kind"] = "EnReeba";
            extra["bigLeeverTimer"] = reeba->bigLeeverTimer;
            extra["moveTimer"] = reeba->moveTimer;
            extra["sfxTimer"] = reeba->sfxTimer;
            extra["damagedTimer"] = reeba->damagedTimer;
            extra["waitTimer"] = reeba->waitTimer;
            extra["isBig"] = reeba->isBig;
            extra["stunType"] = reeba->stunType;
            extra["aimType"] = reeba->aimType;
            extra["yOffsetTarget"] = reeba->yOffsetTarget;
            extra["yOffsetStep"] = reeba->yOffsetStep;
            extra["scale"] = reeba->scale;
            AddSkelAnimeState(extra, &reeba->skelanime);
            break;
        }
        case ACTOR_BOSS_GOMA: {
            BossGoma* goma = (BossGoma*)actor;
            extra["kind"] = "BossGoma";
            extra["action"] = GetBossGomaActionId(goma->actionFunc);
            extra["frameCount"] = goma->frameCount;
            extra["patienceTimer"] = goma->patienceTimer;
            extra["eyeLidBottomRotX"] = goma->eyeLidBottomRotX;
            extra["eyeLidTopRotX"] = goma->eyeLidTopRotX;
            extra["eyeClosedTimer"] = goma->eyeClosedTimer;
            extra["eyeIrisRotX"] = goma->eyeIrisRotX;
            extra["eyeIrisRotY"] = goma->eyeIrisRotY;
            extra["childrenGohmaState"] = { goma->childrenGohmaState[0], goma->childrenGohmaState[1],
                                             goma->childrenGohmaState[2] };
            extra["tailLimbsScaleTimers"] = { goma->tailLimbsScaleTimers[0], goma->tailLimbsScaleTimers[1],
                                               goma->tailLimbsScaleTimers[2], goma->tailLimbsScaleTimers[3] };
            extra["spawnGohmasActionTimer"] = goma->spawnGohmasActionTimer;
            extra["eyeState"] = goma->eyeState;
            extra["doNotMoveThisFrame"] = goma->doNotMoveThisFrame;
            extra["visualState"] = goma->visualState;
            extra["invincibilityFrames"] = goma->invincibilityFrames;
            extra["disableGameplayLogic"] = goma->disableGameplayLogic;
            extra["decayingProgress"] = goma->decayingProgress;
            extra["noBackfaceCulling"] = goma->noBackfaceCulling;
            extra["blinkTimer"] = goma->blinkTimer;
            extra["lookedAtFrames"] = goma->lookedAtFrames;
            extra["actionState"] = goma->actionState;
            extra["framesUntilNextAction"] = goma->framesUntilNextAction;
            extra["timer"] = goma->timer;
            extra["sfxFaintTimer"] = goma->sfxFaintTimer;
            extra["tailLimbsScale"] = { goma->tailLimbsScale[0], goma->tailLimbsScale[1], goma->tailLimbsScale[2],
                                         goma->tailLimbsScale[3] };
            extra["eyeIrisScaleX"] = goma->eyeIrisScaleX;
            extra["eyeIrisScaleY"] = goma->eyeIrisScaleY;
            extra["mainEnvColor"] = { goma->mainEnvColor[0], goma->mainEnvColor[1], goma->mainEnvColor[2] };
            extra["eyeEnvColor"] = { goma->eyeEnvColor[0], goma->eyeEnvColor[1], goma->eyeEnvColor[2] };
            extra["currentAnimFrameCount"] = goma->currentAnimFrameCount;
            AddSkelAnimeState(extra, &goma->skelanime);
            break;
        }
        case ACTOR_BOSS_DODONGO: {
            BossDodongo* dodongo = (BossDodongo*)actor;
            extra["kind"] = "BossDodongo";
            extra["action"] = GetBossDodongoActionId(dodongo->actionFunc);
            extra["health"] = dodongo->health;
            extra["unk_196"] = dodongo->unk_196;
            extra["unk_198"] = dodongo->unk_198;
            extra["unk_19A"] = dodongo->unk_19A;
            extra["csState"] = dodongo->csState;
            extra["unk_19E"] = dodongo->unk_19E;
            extra["unk_1A0"] = dodongo->unk_1A0;
            extra["unk_1A2"] = dodongo->unk_1A2;
            extra["unk_1A4"] = dodongo->unk_1A4;
            extra["unk_1A6"] = dodongo->unk_1A6;
            extra["numWallCollisions"] = dodongo->numWallCollisions;
            extra["unk_1AA"] = dodongo->unk_1AA;
            extra["unk_1AC"] = dodongo->unk_1AC;
            extra["unk_1AE"] = dodongo->unk_1AE;
            extra["unk_1B0"] = dodongo->unk_1B0;
            extra["unk_1B6"] = dodongo->unk_1B6;
            extra["playerYawInRange"] = dodongo->playerYawInRange;
            extra["playerPosInRange"] = dodongo->playerPosInRange;
            extra["unk_1BC"] = dodongo->unk_1BC;
            extra["unk_1BE"] = dodongo->unk_1BE;
            extra["unk_1C0"] = dodongo->unk_1C0;
            extra["unk_1C2"] = dodongo->unk_1C2;
            extra["unk_1C4"] = dodongo->unk_1C4;
            extra["unk_1C6"] = dodongo->unk_1C6;
            extra["unk_1C8"] = dodongo->unk_1C8;
            extra["unk_1CC"] = dodongo->unk_1CC;
            extra["unk_1DA"] = dodongo->unk_1DA;
            extra["unk_1DC"] = dodongo->unk_1DC;
            extra["unk_1DE"] = dodongo->unk_1DE;
            extra["unk_1E0"] = dodongo->unk_1E0;
            extra["unk_1E2"] = dodongo->unk_1E2;
            extra["unk_1E3"] = dodongo->unk_1E3;
            extra["unk_1E4"] = dodongo->unk_1E4;
            extra["unk_1E8"] = dodongo->unk_1E8;
            extra["unk_1EC"] = dodongo->unk_1EC;
            extra["unk_1F8"] = dodongo->unk_1F8;
            extra["unk_1FC"] = dodongo->unk_1FC;
            extra["unk_200"] = dodongo->unk_200;
            extra["unk_204"] = dodongo->unk_204;
            extra["unk_208"] = dodongo->unk_208;
            extra["unk_20C"] = dodongo->unk_20C;
            extra["colorFilterR"] = dodongo->colorFilterR;
            extra["colorFilterG"] = dodongo->colorFilterG;
            extra["colorFilterB"] = dodongo->colorFilterB;
            extra["colorFilterMin"] = dodongo->colorFilterMin;
            extra["colorFilterMax"] = dodongo->colorFilterMax;
            extra["unk_224"] = dodongo->unk_224;
            extra["unk_228"] = dodongo->unk_228;
            extra["unk_22C"] = dodongo->unk_22C;
            extra["unk_230"] = dodongo->unk_230;
            extra["unk_234"] = dodongo->unk_234;
            extra["unk_238"] = dodongo->unk_238;
            extra["unk_23C"] = dodongo->unk_23C;
            extra["unk_240"] = dodongo->unk_240;
            extra["unk_244"] = dodongo->unk_244;
            AddVec3fState(extra, "vec", dodongo->vec);
            AddVec3fState(extra, "firePos", dodongo->firePos);
            AddVec3fState(extra, "mouthPos", dodongo->mouthPos);
            AddSkelAnimeState(extra, &dodongo->skelAnime);
            break;
        }
        case ACTOR_BOSS_GANONDROF: {
            BossGanondrof* ganondrof = (BossGanondrof*)actor;
            extra["kind"] = "BossGanondrof";
            extra["action"] = GetBossGanondrofActionId(ganondrof->actionFunc);
            std::vector<s16> work;
            work.reserve(GND_SHORT_COUNT);
            for (s32 i = 0; i < GND_SHORT_COUNT; i++) {
                work.push_back(ganondrof->work[i]);
            }
            extra["work"] = work;
            extra["timers"] = { ganondrof->timers[0], ganondrof->timers[1], ganondrof->timers[2],
                                 ganondrof->timers[3], ganondrof->timers[4] };
            extra["killActor"] = ganondrof->killActor;
            extra["returnCount"] = ganondrof->returnCount;
            extra["shockTimer"] = ganondrof->shockTimer;
            extra["flyMode"] = ganondrof->flyMode;
            extra["returnSuccess"] = ganondrof->returnSuccess;
            std::vector<f32> fwork;
            fwork.reserve(GND_FLOAT_COUNT);
            for (s32 i = 0; i < GND_FLOAT_COUNT; i++) {
                fwork.push_back(ganondrof->fwork[i]);
            }
            extra["fwork"] = fwork;
            AddVec3fState(extra, "spearTip", ganondrof->spearTip);
            AddVec3fState(extra, "targetPos", ganondrof->targetPos);
            extra["deathCamera"] = ganondrof->deathCamera;
            extra["deathState"] = ganondrof->deathState;
            extra["cameraSpeedMod"] = ganondrof->cameraSpeedMod;
            extra["cameraAccel"] = ganondrof->cameraAccel;
            extra["legRotY"] = ganondrof->legRotY;
            extra["legRotZ"] = ganondrof->legRotZ;
            extra["legSplitY"] = ganondrof->legSplitY;
            extra["armRotY"] = ganondrof->armRotY;
            extra["armRotZ"] = ganondrof->armRotZ;
            AddSkelAnimeState(extra, &ganondrof->skelAnime);
            break;
        }
        case ACTOR_OBJ_OSHIHIKI: {
            ObjOshihiki* block = (ObjOshihiki*)actor;
            extra["kind"] = "ObjOshihiki";
            extra["action"] = GetObjOshihikiActionId(block->actionFunc);
            extra["timer"] = block->timer;
            extra["pushSpeed"] = block->pushSpeed;
            extra["pushDist"] = block->pushDist;
            extra["direction"] = block->direction;
            extra["highestFloor"] = block->highestFloor;
            extra["cantMove"] = block->cantMove;
            extra["dynaUnk150"] = block->dyna.unk_150;
            extra["dynaUnk154"] = block->dyna.unk_154;
            extra["dynaUnk158"] = block->dyna.unk_158;
            extra["dynaUnk15A"] = block->dyna.unk_15A;
            extra["dynaTransformFlags"] = block->dyna.transformFlags;
            extra["dynaInteractFlags"] = block->dyna.interactFlags;
            extra["dynaUnk162"] = block->dyna.unk_162;
            extra["homeX"] = block->dyna.actor.home.pos.x;
            extra["homeY"] = block->dyna.actor.home.pos.y;
            extra["homeZ"] = block->dyna.actor.home.pos.z;
            extra["floorHeight"] = block->dyna.actor.floorHeight;
            break;
        }
        case ACTOR_OBJ_HSBLOCK: {
            ObjHsblock* block = (ObjHsblock*)actor;
            extra["kind"] = "ObjHsblock";
            AddDynaPolyState(extra, &block->dyna);
            break;
        }
        case ACTOR_OBJ_ELEVATOR: {
            ObjElevator* elevator = (ObjElevator*)actor;
            extra["kind"] = "ObjElevator";
            AddDynaPolyState(extra, &elevator->dyna);
            extra["unk_168"] = elevator->unk_168;
            extra["unk_16C"] = elevator->unk_16C;
            extra["unk_170"] = elevator->unk_170;
            break;
        }
        case ACTOR_OBJ_LIFT: {
            ObjLift* lift = (ObjLift*)actor;
            extra["kind"] = "ObjLift";
            AddDynaPolyState(extra, &lift->dyna);
            AddVec3sState(extra, "shakeOrientation", lift->shakeOrientation);
            extra["timer"] = lift->timer;
            break;
        }
        case ACTOR_OBJ_TIMEBLOCK: {
            ObjTimeblock* timeblock = (ObjTimeblock*)actor;
            extra["kind"] = "ObjTimeblock";
            AddDynaPolyState(extra, &timeblock->dyna);
            extra["demoEffectTimer"] = timeblock->demoEffectTimer;
            extra["songEndTimer"] = timeblock->songEndTimer;
            extra["unk_172"] = timeblock->unk_172;
            extra["unk_174"] = timeblock->unk_174;
            extra["unk_175"] = timeblock->unk_175;
            extra["unk_176"] = timeblock->unk_176;
            extra["unk_177"] = timeblock->unk_177;
            extra["isVisible"] = timeblock->isVisible;
            break;
        }
        case ACTOR_BG_MIZU_WATER: {
            BgMizuWater* water = (BgMizuWater*)actor;
            extra["kind"] = "BgMizuWater";
            extra["type"] = water->type;
            extra["targetY"] = water->targetY;
            extra["baseY"] = water->baseY;
            extra["switchFlag"] = water->switchFlag;
            break;
        }
        case ACTOR_BG_MIZU_MOVEBG: {
            BgMizuMovebg* movebg = (BgMizuMovebg*)actor;
            extra["kind"] = "BgMizuMovebg";
            AddDynaPolyState(extra, &movebg->dyna);
            extra["homeY"] = movebg->homeY;
            extra["scrollAlpha1"] = movebg->scrollAlpha1;
            extra["scrollAlpha2"] = movebg->scrollAlpha2;
            extra["scrollAlpha3"] = movebg->scrollAlpha3;
            extra["scrollAlpha4"] = movebg->scrollAlpha4;
            extra["sfxFlags"] = movebg->sfxFlags;
            extra["waypointId"] = movebg->waypointId;
            break;
        }
        case ACTOR_BG_MIZU_SHUTTER: {
            BgMizuShutter* shutter = (BgMizuShutter*)actor;
            extra["kind"] = "BgMizuShutter";
            AddDynaPolyState(extra, &shutter->dyna);
            extra["timer"] = shutter->timer;
            extra["timerMax"] = shutter->timerMax;
            extra["maxSpeed"] = shutter->maxSpeed;
            AddVec3fState(extra, "closedPos", shutter->closedPos);
            AddVec3fState(extra, "openPos", shutter->openPos);
            break;
        }
        case ACTOR_BG_HIDAN_FSLIFT: {
            BgHidanFslift* lift = (BgHidanFslift*)actor;
            extra["kind"] = "BgHidanFslift";
            AddDynaPolyState(extra, &lift->dyna);
            extra["timer"] = lift->timer;
            extra["cameraSetting"] = lift->cameraSetting;
            break;
        }
        case ACTOR_BG_JYA_COBRA: {
            BgJyaCobra* cobra = (BgJyaCobra*)actor;
            extra["kind"] = "BgJyaCobra";
            AddDynaPolyState(extra, &cobra->dyna);
            extra["unk_168"] = cobra->unk_168;
            extra["unk_16A"] = cobra->unk_16A;
            extra["unk_16C"] = cobra->unk_16C;
            extra["unk_16E"] = cobra->unk_16E;
            extra["unk_170"] = cobra->unk_170;
            extra["unk_172"] = cobra->unk_172;
            AddVec3fState(extra, "unk_174", cobra->unk_174);
            AddVec3fState(extra, "unk_180", cobra->unk_180);
            extra["unk_18C"] = cobra->unk_18C;
            extra["unk_190"] = cobra->unk_190;
            break;
        }
        case ACTOR_BG_JYA_BIGMIRROR: {
            BgJyaBigmirror* mirror = (BgJyaBigmirror*)actor;
            extra["kind"] = "BgJyaBigmirror";
            extra["cobraRotY"] = { mirror->cobraInfo[0].rotY, mirror->cobraInfo[1].rotY };
            extra["puzzleFlags"] = mirror->puzzleFlags;
            extra["spawned"] = mirror->spawned;
            extra["liftHeight"] = mirror->liftHeight;
            break;
        }
        case ACTOR_BG_HAKA_SHIP: {
            BgHakaShip* ship = (BgHakaShip*)actor;
            extra["kind"] = "BgHakaShip";
            AddDynaPolyState(extra, &ship->dyna);
            extra["counter"] = ship->counter;
            extra["switchFlag"] = ship->switchFlag;
            extra["yOffset"] = ship->yOffset;
            AddVec3fState(extra, "bellSoundPos", ship->bellSoundPos);
            break;
        }
        case ACTOR_BG_HAKA_WATER: {
            BgHakaWater* water = (BgHakaWater*)actor;
            extra["kind"] = "BgHakaWater";
            extra["isLowered"] = water->isLowered;
            break;
        }
        case ACTOR_BG_HAKA_GATE: {
            BgHakaGate* gate = (BgHakaGate*)actor;
            extra["kind"] = "BgHakaGate";
            AddDynaPolyState(extra, &gate->dyna);
            extra["switchFlag"] = gate->switchFlag;
            extra["actionVar1"] = gate->actionVar1;
            extra["actionVar2"] = gate->actionVar2;
            extra["actionVar3"] = gate->actionVar3;
            extra["actionVar4"] = gate->actionVar4;
            extra["actionVar5"] = gate->actionVar5;
            break;
        }
        case ACTOR_BG_BDAN_OBJECTS: {
            BgBdanObjects* objects = (BgBdanObjects*)actor;
            extra["kind"] = "BgBdanObjects";
            AddDynaPolyState(extra, &objects->dyna);
            extra["switchFlag"] = objects->switchFlag;
            extra["timer"] = objects->timer;
            extra["cameraSetting"] = objects->cameraSetting;
            break;
        }
    }

    return extra;
}

void ApplyEnemyExtraState(Actor* actor, nlohmann::json extra) {
    if (actor == nullptr || !extra.is_object()) {
        return;
    }

    std::string kind = extra.value("kind", "");

    if (actor->id == ACTOR_EN_DEKUNUTS && kind == "EnDekunuts") {
        EnDekunuts* dekunuts = (EnDekunuts*)actor;
        s32 remoteAction = extra.value("action", (s32)-1);
        s32 localAction = GetDekunutsActionId(dekunuts->actionFunc);
        if (remoteAction >= 0 && remoteAction != localAction) {
            EnDekunutsActionFunc remoteFunc = GetDekunutsActionFunc(remoteAction);
            if (remoteFunc != nullptr) {
                dekunuts->actionFunc = remoteFunc;
            }
        }
        dekunuts->playWalkSound = extra.value("playWalkSound", dekunuts->playWalkSound);
        dekunuts->runAwayCount = extra.value("runAwayCount", dekunuts->runAwayCount);
        dekunuts->animFlagAndTimer = extra.value("animFlagAndTimer", dekunuts->animFlagAndTimer);
        dekunuts->runDirection = extra.value("runDirection", dekunuts->runDirection);
        dekunuts->shotsPerRound = extra.value("shotsPerRound", dekunuts->shotsPerRound);
        dekunuts->collider.dim.height = extra.value("colliderHeight", dekunuts->collider.dim.height);
        dekunuts->actor.colChkInfo.mass = extra.value("mass", dekunuts->actor.colChkInfo.mass);
        if (extra.contains("colliderAcOn")) {
            if (extra.value("colliderAcOn", false)) {
                dekunuts->collider.base.acFlags |= AC_ON;
            } else {
                dekunuts->collider.base.acFlags &= ~AC_ON;
            }
        }
        ApplySkelAnimeState(extra, &dekunuts->skelAnime);
    } else if (actor->id == ACTOR_EN_DEKUBABA && kind == "EnDekubaba") {
        EnDekubaba* dekubaba = (EnDekubaba*)actor;
        s32 remoteAction = extra.value("action", (s32)-1);
        s32 localAction = GetDekubabaActionId(dekubaba->actionFunc);
        if (remoteAction >= 0 && remoteAction != localAction) {
            EnDekubabaActionFunc remoteFunc = GetDekubabaActionFunc(remoteAction);
            if (remoteFunc != nullptr) {
                dekubaba->actionFunc = remoteFunc;
            }
        }
        dekubaba->timer = extra.value("timer", dekubaba->timer);
        dekubaba->targetSwayAngle = extra.value("targetSwayAngle", dekubaba->targetSwayAngle);
        std::vector<s16> stemSectionAngle = extra.value("stemSectionAngle", std::vector<s16>{});
        if (stemSectionAngle.size() == 3) {
            dekubaba->stemSectionAngle[0] = stemSectionAngle[0];
            dekubaba->stemSectionAngle[1] = stemSectionAngle[1];
            dekubaba->stemSectionAngle[2] = stemSectionAngle[2];
        }
        dekubaba->size = extra.value("size", dekubaba->size);
        dekubaba->collider.base.colType = extra.value("colliderColType", dekubaba->collider.base.colType);
        if (extra.contains("colliderAcHard")) {
            if (extra.value("colliderAcHard", false)) {
                dekubaba->collider.base.acFlags |= AC_HARD;
            } else {
                dekubaba->collider.base.acFlags &= ~AC_HARD;
            }
        }
        ApplySkelAnimeState(extra, &dekubaba->skelAnime);
    } else if (actor->id == ACTOR_EN_ST && kind == "EnSt") {
        EnSt* st = (EnSt*)actor;
        s32 remoteAction = extra.value("action", (s32)-1);
        s32 localAction = GetEnStActionId(st->actionFunc);
        if (remoteAction >= 0 && remoteAction != localAction) {
            EnStActionFunc remoteFunc = GetEnStActionFunc(remoteAction);
            if (remoteFunc != nullptr) {
                EnSt_SetupAction(st, remoteFunc);
            }
        }
        st->groundBounces = extra.value("groundBounces", st->groundBounces);
        st->deathTimer = extra.value("deathTimer", st->deathTimer);
        st->finishDeathTimer = extra.value("finishDeathTimer", st->finishDeathTimer);
        st->setTargetYawTimer = extra.value("setTargetYawTimer", st->setTargetYawTimer);
        st->deathYawTarget = extra.value("deathYawTarget", st->deathYawTarget);
        st->rotAwayTimer = extra.value("rotAwayTimer", st->rotAwayTimer);
        st->rotTowardsTimer = extra.value("rotTowardsTimer", st->rotTowardsTimer);
        st->takeDamageSpinTimer = extra.value("takeDamageSpinTimer", st->takeDamageSpinTimer);
        st->stunTimer = extra.value("stunTimer", st->stunTimer);
        st->swayTimer = extra.value("swayTimer", st->swayTimer);
        st->swayAngle = extra.value("swayAngle", st->swayAngle);
        st->animFrames = extra.value("animFrames", st->animFrames);
        st->sfxTimer = extra.value("sfxTimer", st->sfxTimer);
        st->gaveDamageSpinTimer = extra.value("gaveDamageSpinTimer", st->gaveDamageSpinTimer);
        st->actor.shape.yOffset = extra.value("shapeYOffset", st->actor.shape.yOffset);
        st->floorHeightOffset = extra.value("floorHeightOffset", st->floorHeightOffset);
        st->colliderScale = extra.value("colliderScale", st->colliderScale);
        ApplySkelAnimeState(extra, &st->skelAnime);
        EnsureEnStDeathState(st);
    } else if (actor->id == ACTOR_EN_SSH && kind == "EnSsh") {
        EnSsh* ssh = (EnSsh*)actor;
        ssh->spinTimer = extra.value("spinTimer", ssh->spinTimer);
        ssh->hitTimer = extra.value("hitTimer", ssh->hitTimer);
        ssh->stunTimer = extra.value("stunTimer", ssh->stunTimer);
        ssh->animTimer = extra.value("animTimer", ssh->animTimer);
        ssh->swayTimer = extra.value("swayTimer", ssh->swayTimer);
        ssh->swayAngle = extra.value("swayAngle", ssh->swayAngle);
        ssh->stateFlags = extra.value("stateFlags", ssh->stateFlags);
        ssh->hitCount = extra.value("hitCount", ssh->hitCount);
        ssh->floorHeightOffset = extra.value("floorHeightOffset", ssh->floorHeightOffset);
        ssh->colliderScale = extra.value("colliderScale", ssh->colliderScale);
        ApplySkelAnimeState(extra, &ssh->skelAnime);
    } else if (actor->id == ACTOR_EN_SW && kind == "EnSw") {
        EnSw* sw = (EnSw*)actor;
        s32 remoteAction = extra.value("action", (s32)-1);
        s32 localAction = GetEnSwActionId(sw->actionFunc);
        if (remoteAction >= 0 && remoteAction != localAction) {
            EnSwActionFunc remoteFunc = GetEnSwActionFunc(remoteAction);
            if (remoteFunc != nullptr) {
                sw->actionFunc = remoteFunc;
            }
        }
        sw->unk_388 = extra.value("unk_388", sw->unk_388);
        sw->unk_38A = extra.value("unk_38A", sw->unk_38A);
        sw->unk_38C = extra.value("unk_38C", sw->unk_38C);
        sw->unk_38E = extra.value("unk_38E", sw->unk_38E);
        sw->unk_390 = extra.value("unk_390", sw->unk_390);
        sw->unk_392 = extra.value("unk_392", sw->unk_392);
        sw->unk_394 = extra.value("unk_394", sw->unk_394);
        sw->unk_420 = extra.value("unk_420", sw->unk_420);
        sw->unk_42C = extra.value("unk_42C", sw->unk_42C);
        sw->unk_440 = extra.value("unk_440", sw->unk_440);
        sw->unk_442 = extra.value("unk_442", sw->unk_442);
        sw->unk_444 = extra.value("unk_444", sw->unk_444);
        sw->unk_446 = extra.value("unk_446", sw->unk_446);
        ApplySkelAnimeState(extra, &sw->skelAnime);
        EnsureEnSwDeathState(sw);
    } else if (actor->id == ACTOR_EN_WF && kind == "EnWf") {
        EnWf* wf = (EnWf*)actor;
        wf->action = extra.value("action", wf->action);
        wf->actionTimer = extra.value("actionTimer", wf->actionTimer);
        wf->runSpeed = extra.value("runSpeed", wf->runSpeed);
        wf->slashStatus = extra.value("slashStatus", wf->slashStatus);
        wf->switchFlag = extra.value("switchFlag", wf->switchFlag);
        wf->runAngle = extra.value("runAngle", wf->runAngle);
        wf->fireTimer = extra.value("fireTimer", wf->fireTimer);
        wf->damageEffect = extra.value("damageEffect", wf->damageEffect);
        ApplySkelAnimeState(extra, &wf->skelAnime);
    } else if (actor->id == ACTOR_EN_ZF && kind == "EnZf") {
        EnZf* zf = (EnZf*)actor;
        zf->action = extra.value("action", zf->action);
        zf->hopAnimIndex = extra.value("hopAnimIndex", zf->hopAnimIndex);
        zf->headRot = extra.value("headRot", zf->headRot);
        zf->headRotTemp = extra.value("headRotTemp", zf->headRotTemp);
        zf->iceTimer = extra.value("iceTimer", zf->iceTimer);
        zf->swordSheathed = extra.value("swordSheathed", zf->swordSheathed);
        zf->clearFlag = extra.value("clearFlag", zf->clearFlag);
        zf->curPlatform = extra.value("curPlatform", zf->curPlatform);
        zf->homePlatform = extra.value("homePlatform", zf->homePlatform);
        zf->nextPlatform = extra.value("nextPlatform", zf->nextPlatform);
        zf->damageEffect = extra.value("damageEffect", zf->damageEffect);
        ApplySkelAnimeState(extra, &zf->skelAnime);
    } else if (actor->id == ACTOR_EN_OKUTA && kind == "EnOkuta") {
        EnOkuta* okuta = (EnOkuta*)actor;
        okuta->timer = extra.value("timer", okuta->timer);
        okuta->numShots = extra.value("numShots", okuta->numShots);
        okuta->jumpHeight = extra.value("jumpHeight", okuta->jumpHeight);
        if (extra.contains("headScaleX")) {
            okuta->headScale.x = extra.value("headScaleX", okuta->headScale.x);
            okuta->headScale.y = extra.value("headScaleY", okuta->headScale.y);
            okuta->headScale.z = extra.value("headScaleZ", okuta->headScale.z);
        }
        ApplySkelAnimeState(extra, &okuta->skelAnime);
    } else if (actor->id == ACTOR_EN_FIREFLY && kind == "EnFirefly") {
        EnFirefly* firefly = (EnFirefly*)actor;
        firefly->auraType = extra.value("auraType", firefly->auraType);
        firefly->onFire = extra.value("onFire", firefly->onFire);
        firefly->timer = extra.value("timer", firefly->timer);
        firefly->targetPitch = extra.value("targetPitch", firefly->targetPitch);
        firefly->maxAltitude = extra.value("maxAltitude", firefly->maxAltitude);
        ApplySkelAnimeState(extra, &firefly->skelAnime);
    } else if (actor->id == ACTOR_EN_BB && kind == "EnBb") {
        EnBb* bb = (EnBb*)actor;
        bb->action = extra.value("action", bb->action);
        bb->moveMode = extra.value("moveMode", bb->moveMode);
        bb->timer = extra.value("timer", bb->timer);
        bb->actionState = extra.value("actionState", bb->actionState);
        bb->charge = extra.value("charge", bb->charge);
        bb->actionVar1 = extra.value("actionVar1", bb->actionVar1);
        bb->actionVar2 = extra.value("actionVar2", bb->actionVar2);
        bb->flameScrollMod = extra.value("flameScrollMod", bb->flameScrollMod);
        bb->bobPhase = extra.value("bobPhase", bb->bobPhase);
        bb->bobSize = extra.value("bobSize", bb->bobSize);
        bb->maxSpeed = extra.value("maxSpeed", bb->maxSpeed);
        bb->fireIceTimer = extra.value("fireIceTimer", bb->fireIceTimer);
        bb->dmgEffect = extra.value("dmgEffect", bb->dmgEffect);
        ApplySkelAnimeState(extra, &bb->skelAnime);
    } else if (actor->id == ACTOR_EN_TITE && kind == "EnTite") {
        EnTite* tite = (EnTite*)actor;
        tite->action = extra.value("action", tite->action);
        tite->flipState = extra.value("flipState", tite->flipState);
        tite->actionVar1 = extra.value("actionVar1", tite->actionVar1);
        tite->actionVar2 = extra.value("actionVar2", tite->actionVar2);
        tite->spawnIceTimer = extra.value("spawnIceTimer", tite->spawnIceTimer);
        tite->damageEffect = extra.value("damageEffect", tite->damageEffect);
        ApplySkelAnimeState(extra, &tite->skelAnime);
    } else if (actor->id == ACTOR_EN_PEEHAT && kind == "EnPeehat") {
        EnPeehat* peehat = (EnPeehat*)actor;
        peehat->state = extra.value("state", peehat->state);
        peehat->bladeRotVel = extra.value("bladeRotVel", peehat->bladeRotVel);
        peehat->bladeRot = extra.value("bladeRot", peehat->bladeRot);
        peehat->riseDelayTimer = extra.value("riseDelayTimer", peehat->riseDelayTimer);
        peehat->seekPlayerTimer = extra.value("seekPlayerTimer", peehat->seekPlayerTimer);
        peehat->animTimer = extra.value("animTimer", peehat->animTimer);
        peehat->jiggleRot = extra.value("jiggleRot", peehat->jiggleRot);
        peehat->jiggleRotInc = extra.value("jiggleRotInc", peehat->jiggleRotInc);
        peehat->scaleShift = extra.value("scaleShift", peehat->scaleShift);
        ApplySkelAnimeState(extra, &peehat->skelAnime);
    } else if (actor->id == ACTOR_EN_REEBA && kind == "EnReeba") {
        EnReeba* reeba = (EnReeba*)actor;
        reeba->bigLeeverTimer = extra.value("bigLeeverTimer", reeba->bigLeeverTimer);
        reeba->moveTimer = extra.value("moveTimer", reeba->moveTimer);
        reeba->sfxTimer = extra.value("sfxTimer", reeba->sfxTimer);
        reeba->damagedTimer = extra.value("damagedTimer", reeba->damagedTimer);
        reeba->waitTimer = extra.value("waitTimer", reeba->waitTimer);
        reeba->isBig = extra.value("isBig", reeba->isBig);
        reeba->stunType = extra.value("stunType", reeba->stunType);
        reeba->aimType = extra.value("aimType", reeba->aimType);
        reeba->yOffsetTarget = extra.value("yOffsetTarget", reeba->yOffsetTarget);
        reeba->yOffsetStep = extra.value("yOffsetStep", reeba->yOffsetStep);
        reeba->scale = extra.value("scale", reeba->scale);
        ApplySkelAnimeState(extra, &reeba->skelanime);
    } else if (actor->id == ACTOR_BOSS_GOMA && kind == "BossGoma") {
        BossGoma* goma = (BossGoma*)actor;
        s32 remoteAction = extra.value("action", (s32)-1);
        s32 localAction = GetBossGomaActionId(goma->actionFunc);
        if (remoteAction >= 0 && remoteAction != localAction) {
            BossGomaActionFunc remoteFunc = GetBossGomaActionFunc(remoteAction);
            if (remoteFunc != nullptr) {
                goma->actionFunc = remoteFunc;
            }
        }
        goma->frameCount = extra.value("frameCount", goma->frameCount);
        goma->patienceTimer = extra.value("patienceTimer", goma->patienceTimer);
        goma->eyeLidBottomRotX = extra.value("eyeLidBottomRotX", goma->eyeLidBottomRotX);
        goma->eyeLidTopRotX = extra.value("eyeLidTopRotX", goma->eyeLidTopRotX);
        goma->eyeClosedTimer = extra.value("eyeClosedTimer", goma->eyeClosedTimer);
        goma->eyeIrisRotX = extra.value("eyeIrisRotX", goma->eyeIrisRotX);
        goma->eyeIrisRotY = extra.value("eyeIrisRotY", goma->eyeIrisRotY);
        std::vector<s16> childrenGohmaState = extra.value("childrenGohmaState", std::vector<s16>{});
        if (childrenGohmaState.size() == 3) {
            goma->childrenGohmaState[0] = childrenGohmaState[0];
            goma->childrenGohmaState[1] = childrenGohmaState[1];
            goma->childrenGohmaState[2] = childrenGohmaState[2];
        }
        std::vector<s16> tailLimbsScaleTimers = extra.value("tailLimbsScaleTimers", std::vector<s16>{});
        if (tailLimbsScaleTimers.size() == 4) {
            goma->tailLimbsScaleTimers[0] = tailLimbsScaleTimers[0];
            goma->tailLimbsScaleTimers[1] = tailLimbsScaleTimers[1];
            goma->tailLimbsScaleTimers[2] = tailLimbsScaleTimers[2];
            goma->tailLimbsScaleTimers[3] = tailLimbsScaleTimers[3];
        }
        goma->spawnGohmasActionTimer = extra.value("spawnGohmasActionTimer", goma->spawnGohmasActionTimer);
        goma->eyeState = extra.value("eyeState", goma->eyeState);
        goma->doNotMoveThisFrame = extra.value("doNotMoveThisFrame", goma->doNotMoveThisFrame);
        goma->visualState = extra.value("visualState", goma->visualState);
        goma->invincibilityFrames = extra.value("invincibilityFrames", goma->invincibilityFrames);
        goma->disableGameplayLogic = extra.value("disableGameplayLogic", goma->disableGameplayLogic);
        goma->decayingProgress = extra.value("decayingProgress", goma->decayingProgress);
        goma->noBackfaceCulling = extra.value("noBackfaceCulling", goma->noBackfaceCulling);
        goma->blinkTimer = extra.value("blinkTimer", goma->blinkTimer);
        goma->lookedAtFrames = extra.value("lookedAtFrames", goma->lookedAtFrames);
        goma->actionState = extra.value("actionState", goma->actionState);
        goma->framesUntilNextAction = extra.value("framesUntilNextAction", goma->framesUntilNextAction);
        goma->timer = extra.value("timer", goma->timer);
        goma->sfxFaintTimer = extra.value("sfxFaintTimer", goma->sfxFaintTimer);
        std::vector<f32> tailLimbsScale = extra.value("tailLimbsScale", std::vector<f32>{});
        if (tailLimbsScale.size() == 4) {
            goma->tailLimbsScale[0] = tailLimbsScale[0];
            goma->tailLimbsScale[1] = tailLimbsScale[1];
            goma->tailLimbsScale[2] = tailLimbsScale[2];
            goma->tailLimbsScale[3] = tailLimbsScale[3];
        }
        goma->eyeIrisScaleX = extra.value("eyeIrisScaleX", goma->eyeIrisScaleX);
        goma->eyeIrisScaleY = extra.value("eyeIrisScaleY", goma->eyeIrisScaleY);
        std::vector<f32> mainEnvColor = extra.value("mainEnvColor", std::vector<f32>{});
        if (mainEnvColor.size() == 3) {
            goma->mainEnvColor[0] = mainEnvColor[0];
            goma->mainEnvColor[1] = mainEnvColor[1];
            goma->mainEnvColor[2] = mainEnvColor[2];
        }
        std::vector<f32> eyeEnvColor = extra.value("eyeEnvColor", std::vector<f32>{});
        if (eyeEnvColor.size() == 3) {
            goma->eyeEnvColor[0] = eyeEnvColor[0];
            goma->eyeEnvColor[1] = eyeEnvColor[1];
            goma->eyeEnvColor[2] = eyeEnvColor[2];
        }
        goma->currentAnimFrameCount = extra.value("currentAnimFrameCount", goma->currentAnimFrameCount);
        ApplySkelAnimeState(extra, &goma->skelanime);
    } else if (actor->id == ACTOR_BOSS_DODONGO && kind == "BossDodongo") {
        BossDodongo* dodongo = (BossDodongo*)actor;
        s32 remoteAction = extra.value("action", (s32)-1);
        s32 localAction = GetBossDodongoActionId(dodongo->actionFunc);
        if (remoteAction >= 0 && remoteAction != localAction) {
            BossDodongoActionFunc remoteFunc = GetBossDodongoActionFunc(remoteAction);
            if (remoteFunc != nullptr) {
                dodongo->actionFunc = remoteFunc;
            }
        }
        dodongo->health = extra.value("health", dodongo->health);
        dodongo->unk_196 = extra.value("unk_196", dodongo->unk_196);
        dodongo->unk_198 = extra.value("unk_198", dodongo->unk_198);
        dodongo->unk_19A = extra.value("unk_19A", dodongo->unk_19A);
        dodongo->csState = extra.value("csState", dodongo->csState);
        dodongo->unk_19E = extra.value("unk_19E", dodongo->unk_19E);
        dodongo->unk_1A0 = extra.value("unk_1A0", dodongo->unk_1A0);
        dodongo->unk_1A2 = extra.value("unk_1A2", dodongo->unk_1A2);
        dodongo->unk_1A4 = extra.value("unk_1A4", dodongo->unk_1A4);
        dodongo->unk_1A6 = extra.value("unk_1A6", dodongo->unk_1A6);
        dodongo->numWallCollisions = extra.value("numWallCollisions", dodongo->numWallCollisions);
        dodongo->unk_1AA = extra.value("unk_1AA", dodongo->unk_1AA);
        dodongo->unk_1AC = extra.value("unk_1AC", dodongo->unk_1AC);
        dodongo->unk_1AE = extra.value("unk_1AE", dodongo->unk_1AE);
        dodongo->unk_1B0 = extra.value("unk_1B0", dodongo->unk_1B0);
        dodongo->unk_1B6 = extra.value("unk_1B6", dodongo->unk_1B6);
        dodongo->playerYawInRange = extra.value("playerYawInRange", dodongo->playerYawInRange);
        dodongo->playerPosInRange = extra.value("playerPosInRange", dodongo->playerPosInRange);
        dodongo->unk_1BC = extra.value("unk_1BC", dodongo->unk_1BC);
        dodongo->unk_1BE = extra.value("unk_1BE", dodongo->unk_1BE);
        dodongo->unk_1C0 = extra.value("unk_1C0", dodongo->unk_1C0);
        dodongo->unk_1C2 = extra.value("unk_1C2", dodongo->unk_1C2);
        dodongo->unk_1C4 = extra.value("unk_1C4", dodongo->unk_1C4);
        dodongo->unk_1C6 = extra.value("unk_1C6", dodongo->unk_1C6);
        dodongo->unk_1C8 = extra.value("unk_1C8", dodongo->unk_1C8);
        dodongo->unk_1CC = extra.value("unk_1CC", dodongo->unk_1CC);
        dodongo->unk_1DA = extra.value("unk_1DA", dodongo->unk_1DA);
        dodongo->unk_1DC = extra.value("unk_1DC", dodongo->unk_1DC);
        dodongo->unk_1DE = extra.value("unk_1DE", dodongo->unk_1DE);
        dodongo->unk_1E0 = extra.value("unk_1E0", dodongo->unk_1E0);
        dodongo->unk_1E2 = extra.value("unk_1E2", dodongo->unk_1E2);
        dodongo->unk_1E3 = extra.value("unk_1E3", dodongo->unk_1E3);
        dodongo->unk_1E4 = extra.value("unk_1E4", dodongo->unk_1E4);
        dodongo->unk_1E8 = extra.value("unk_1E8", dodongo->unk_1E8);
        dodongo->unk_1EC = extra.value("unk_1EC", dodongo->unk_1EC);
        dodongo->unk_1F8 = extra.value("unk_1F8", dodongo->unk_1F8);
        dodongo->unk_1FC = extra.value("unk_1FC", dodongo->unk_1FC);
        dodongo->unk_200 = extra.value("unk_200", dodongo->unk_200);
        dodongo->unk_204 = extra.value("unk_204", dodongo->unk_204);
        dodongo->unk_208 = extra.value("unk_208", dodongo->unk_208);
        dodongo->unk_20C = extra.value("unk_20C", dodongo->unk_20C);
        dodongo->colorFilterR = extra.value("colorFilterR", dodongo->colorFilterR);
        dodongo->colorFilterG = extra.value("colorFilterG", dodongo->colorFilterG);
        dodongo->colorFilterB = extra.value("colorFilterB", dodongo->colorFilterB);
        dodongo->colorFilterMin = extra.value("colorFilterMin", dodongo->colorFilterMin);
        dodongo->colorFilterMax = extra.value("colorFilterMax", dodongo->colorFilterMax);
        dodongo->unk_224 = extra.value("unk_224", dodongo->unk_224);
        dodongo->unk_228 = extra.value("unk_228", dodongo->unk_228);
        dodongo->unk_22C = extra.value("unk_22C", dodongo->unk_22C);
        dodongo->unk_230 = extra.value("unk_230", dodongo->unk_230);
        dodongo->unk_234 = extra.value("unk_234", dodongo->unk_234);
        dodongo->unk_238 = extra.value("unk_238", dodongo->unk_238);
        dodongo->unk_23C = extra.value("unk_23C", dodongo->unk_23C);
        dodongo->unk_240 = extra.value("unk_240", dodongo->unk_240);
        dodongo->unk_244 = extra.value("unk_244", dodongo->unk_244);
        ApplyVec3fState(extra, "vec", &dodongo->vec);
        ApplyVec3fState(extra, "firePos", &dodongo->firePos);
        ApplyVec3fState(extra, "mouthPos", &dodongo->mouthPos);
        ApplySkelAnimeState(extra, &dodongo->skelAnime);
    } else if (actor->id == ACTOR_BOSS_GANONDROF && kind == "BossGanondrof") {
        BossGanondrof* ganondrof = (BossGanondrof*)actor;
        s32 remoteAction = extra.value("action", (s32)-1);
        s32 localAction = GetBossGanondrofActionId(ganondrof->actionFunc);
        if (remoteAction >= 0 && remoteAction != localAction) {
            BossGanondrofActionFunc remoteFunc = GetBossGanondrofActionFunc(remoteAction);
            if (remoteFunc != nullptr) {
                ganondrof->actionFunc = remoteFunc;
            }
        }
        std::vector<s16> work = extra.value("work", std::vector<s16>{});
        if (work.size() == GND_SHORT_COUNT) {
            for (s32 i = 0; i < GND_SHORT_COUNT; i++) {
                ganondrof->work[i] = work[i];
            }
        }
        std::vector<s16> timers = extra.value("timers", std::vector<s16>{});
        if (timers.size() == 5) {
            for (s32 i = 0; i < 5; i++) {
                ganondrof->timers[i] = timers[i];
            }
        }
        ganondrof->killActor = extra.value("killActor", ganondrof->killActor);
        ganondrof->returnCount = extra.value("returnCount", ganondrof->returnCount);
        ganondrof->shockTimer = extra.value("shockTimer", ganondrof->shockTimer);
        ganondrof->flyMode = extra.value("flyMode", ganondrof->flyMode);
        ganondrof->returnSuccess = extra.value("returnSuccess", ganondrof->returnSuccess);
        std::vector<f32> fwork = extra.value("fwork", std::vector<f32>{});
        if (fwork.size() == GND_FLOAT_COUNT) {
            for (s32 i = 0; i < GND_FLOAT_COUNT; i++) {
                ganondrof->fwork[i] = fwork[i];
            }
        }
        ApplyVec3fState(extra, "spearTip", &ganondrof->spearTip);
        ApplyVec3fState(extra, "targetPos", &ganondrof->targetPos);
        ganondrof->deathCamera = extra.value("deathCamera", ganondrof->deathCamera);
        ganondrof->deathState = extra.value("deathState", ganondrof->deathState);
        ganondrof->cameraSpeedMod = extra.value("cameraSpeedMod", ganondrof->cameraSpeedMod);
        ganondrof->cameraAccel = extra.value("cameraAccel", ganondrof->cameraAccel);
        ganondrof->legRotY = extra.value("legRotY", ganondrof->legRotY);
        ganondrof->legRotZ = extra.value("legRotZ", ganondrof->legRotZ);
        ganondrof->legSplitY = extra.value("legSplitY", ganondrof->legSplitY);
        ganondrof->armRotY = extra.value("armRotY", ganondrof->armRotY);
        ganondrof->armRotZ = extra.value("armRotZ", ganondrof->armRotZ);
        ApplySkelAnimeState(extra, &ganondrof->skelAnime);
    } else if (actor->id == ACTOR_OBJ_OSHIHIKI && kind == "ObjOshihiki") {
        ObjOshihiki* block = (ObjOshihiki*)actor;
        s32 remoteAction = extra.value("action", (s32)-1);
        s32 localAction = GetObjOshihikiActionId(block->actionFunc);
        if (remoteAction >= 0 && remoteAction != localAction) {
            ObjOshihikiActionFunc remoteFunc = GetObjOshihikiActionFunc(remoteAction);
            if (remoteFunc != nullptr) {
                block->actionFunc = remoteFunc;
            }
        }
        block->timer = extra.value("timer", block->timer);
        block->pushSpeed = extra.value("pushSpeed", block->pushSpeed);
        block->pushDist = extra.value("pushDist", block->pushDist);
        block->direction = extra.value("direction", block->direction);
        block->highestFloor = extra.value("highestFloor", block->highestFloor);
        block->cantMove = extra.value("cantMove", block->cantMove);
        block->dyna.unk_150 = extra.value("dynaUnk150", block->dyna.unk_150);
        block->dyna.unk_154 = extra.value("dynaUnk154", block->dyna.unk_154);
        block->dyna.unk_158 = extra.value("dynaUnk158", block->dyna.unk_158);
        block->dyna.unk_15A = extra.value("dynaUnk15A", block->dyna.unk_15A);
        block->dyna.transformFlags = extra.value("dynaTransformFlags", block->dyna.transformFlags);
        block->dyna.interactFlags = extra.value("dynaInteractFlags", block->dyna.interactFlags);
        block->dyna.unk_162 = extra.value("dynaUnk162", block->dyna.unk_162);
        block->dyna.actor.home.pos.x = extra.value("homeX", block->dyna.actor.home.pos.x);
        block->dyna.actor.home.pos.y = extra.value("homeY", block->dyna.actor.home.pos.y);
        block->dyna.actor.home.pos.z = extra.value("homeZ", block->dyna.actor.home.pos.z);
        block->dyna.actor.floorHeight = extra.value("floorHeight", block->dyna.actor.floorHeight);
        block->dyna.actor.world.rot.y = block->dyna.unk_158;
        block->yawSin = Math_SinS(block->dyna.actor.world.rot.y);
        block->yawCos = Math_CosS(block->dyna.actor.world.rot.y);
    } else if (actor->id == ACTOR_OBJ_HSBLOCK && kind == "ObjHsblock") {
        ObjHsblock* block = (ObjHsblock*)actor;
        ApplyDynaPolyState(extra, &block->dyna);
    } else if (actor->id == ACTOR_OBJ_ELEVATOR && kind == "ObjElevator") {
        ObjElevator* elevator = (ObjElevator*)actor;
        ApplyDynaPolyState(extra, &elevator->dyna);
        elevator->unk_168 = extra.value("unk_168", elevator->unk_168);
        elevator->unk_16C = extra.value("unk_16C", elevator->unk_16C);
        elevator->unk_170 = extra.value("unk_170", elevator->unk_170);
    } else if (actor->id == ACTOR_OBJ_LIFT && kind == "ObjLift") {
        ObjLift* lift = (ObjLift*)actor;
        ApplyDynaPolyState(extra, &lift->dyna);
        ApplyVec3sState(extra, "shakeOrientation", &lift->shakeOrientation);
        lift->timer = extra.value("timer", lift->timer);
    } else if (actor->id == ACTOR_OBJ_TIMEBLOCK && kind == "ObjTimeblock") {
        ObjTimeblock* timeblock = (ObjTimeblock*)actor;
        ApplyDynaPolyState(extra, &timeblock->dyna);
        timeblock->demoEffectTimer = extra.value("demoEffectTimer", timeblock->demoEffectTimer);
        timeblock->songEndTimer = extra.value("songEndTimer", timeblock->songEndTimer);
        timeblock->unk_172 = extra.value("unk_172", timeblock->unk_172);
        timeblock->unk_174 = extra.value("unk_174", timeblock->unk_174);
        timeblock->unk_175 = extra.value("unk_175", timeblock->unk_175);
        timeblock->unk_176 = extra.value("unk_176", timeblock->unk_176);
        timeblock->unk_177 = extra.value("unk_177", timeblock->unk_177);
        timeblock->isVisible = extra.value("isVisible", timeblock->isVisible);
    } else if (actor->id == ACTOR_BG_MIZU_WATER && kind == "BgMizuWater") {
        BgMizuWater* water = (BgMizuWater*)actor;
        water->type = extra.value("type", water->type);
        water->targetY = extra.value("targetY", water->targetY);
        water->baseY = extra.value("baseY", water->baseY);
        water->switchFlag = extra.value("switchFlag", water->switchFlag);
    } else if (actor->id == ACTOR_BG_MIZU_MOVEBG && kind == "BgMizuMovebg") {
        BgMizuMovebg* movebg = (BgMizuMovebg*)actor;
        ApplyDynaPolyState(extra, &movebg->dyna);
        movebg->homeY = extra.value("homeY", movebg->homeY);
        movebg->scrollAlpha1 = extra.value("scrollAlpha1", movebg->scrollAlpha1);
        movebg->scrollAlpha2 = extra.value("scrollAlpha2", movebg->scrollAlpha2);
        movebg->scrollAlpha3 = extra.value("scrollAlpha3", movebg->scrollAlpha3);
        movebg->scrollAlpha4 = extra.value("scrollAlpha4", movebg->scrollAlpha4);
        movebg->sfxFlags = extra.value("sfxFlags", movebg->sfxFlags);
        movebg->waypointId = extra.value("waypointId", movebg->waypointId);
    } else if (actor->id == ACTOR_BG_MIZU_SHUTTER && kind == "BgMizuShutter") {
        BgMizuShutter* shutter = (BgMizuShutter*)actor;
        ApplyDynaPolyState(extra, &shutter->dyna);
        shutter->timer = extra.value("timer", shutter->timer);
        shutter->timerMax = extra.value("timerMax", shutter->timerMax);
        shutter->maxSpeed = extra.value("maxSpeed", shutter->maxSpeed);
        ApplyVec3fState(extra, "closedPos", &shutter->closedPos);
        ApplyVec3fState(extra, "openPos", &shutter->openPos);
    } else if (actor->id == ACTOR_BG_HIDAN_FSLIFT && kind == "BgHidanFslift") {
        BgHidanFslift* lift = (BgHidanFslift*)actor;
        ApplyDynaPolyState(extra, &lift->dyna);
        lift->timer = extra.value("timer", lift->timer);
        lift->cameraSetting = extra.value("cameraSetting", lift->cameraSetting);
    } else if (actor->id == ACTOR_BG_JYA_COBRA && kind == "BgJyaCobra") {
        BgJyaCobra* cobra = (BgJyaCobra*)actor;
        ApplyDynaPolyState(extra, &cobra->dyna);
        cobra->unk_168 = extra.value("unk_168", cobra->unk_168);
        cobra->unk_16A = extra.value("unk_16A", cobra->unk_16A);
        cobra->unk_16C = extra.value("unk_16C", cobra->unk_16C);
        cobra->unk_16E = extra.value("unk_16E", cobra->unk_16E);
        cobra->unk_170 = extra.value("unk_170", cobra->unk_170);
        cobra->unk_172 = extra.value("unk_172", cobra->unk_172);
        ApplyVec3fState(extra, "unk_174", &cobra->unk_174);
        ApplyVec3fState(extra, "unk_180", &cobra->unk_180);
        cobra->unk_18C = extra.value("unk_18C", cobra->unk_18C);
        cobra->unk_190 = extra.value("unk_190", cobra->unk_190);
    } else if (actor->id == ACTOR_BG_JYA_BIGMIRROR && kind == "BgJyaBigmirror") {
        BgJyaBigmirror* mirror = (BgJyaBigmirror*)actor;
        std::vector<s16> cobraRotY = extra.value("cobraRotY", std::vector<s16>{});
        if (cobraRotY.size() == 2) {
            mirror->cobraInfo[0].rotY = cobraRotY[0];
            mirror->cobraInfo[1].rotY = cobraRotY[1];
        }
        mirror->puzzleFlags = extra.value("puzzleFlags", mirror->puzzleFlags);
        mirror->spawned = extra.value("spawned", mirror->spawned);
        mirror->liftHeight = extra.value("liftHeight", mirror->liftHeight);
    } else if (actor->id == ACTOR_BG_HAKA_SHIP && kind == "BgHakaShip") {
        BgHakaShip* ship = (BgHakaShip*)actor;
        ApplyDynaPolyState(extra, &ship->dyna);
        ship->counter = extra.value("counter", ship->counter);
        ship->switchFlag = extra.value("switchFlag", ship->switchFlag);
        ship->yOffset = extra.value("yOffset", ship->yOffset);
        ApplyVec3fState(extra, "bellSoundPos", &ship->bellSoundPos);
    } else if (actor->id == ACTOR_BG_HAKA_WATER && kind == "BgHakaWater") {
        BgHakaWater* water = (BgHakaWater*)actor;
        water->isLowered = extra.value("isLowered", water->isLowered);
    } else if (actor->id == ACTOR_BG_HAKA_GATE && kind == "BgHakaGate") {
        BgHakaGate* gate = (BgHakaGate*)actor;
        ApplyDynaPolyState(extra, &gate->dyna);
        gate->switchFlag = extra.value("switchFlag", gate->switchFlag);
        gate->actionVar1 = extra.value("actionVar1", gate->actionVar1);
        gate->actionVar2 = extra.value("actionVar2", gate->actionVar2);
        gate->actionVar3 = extra.value("actionVar3", gate->actionVar3);
        gate->actionVar4 = extra.value("actionVar4", gate->actionVar4);
        gate->actionVar5 = extra.value("actionVar5", gate->actionVar5);
    } else if (actor->id == ACTOR_BG_BDAN_OBJECTS && kind == "BgBdanObjects") {
        BgBdanObjects* objects = (BgBdanObjects*)actor;
        ApplyDynaPolyState(extra, &objects->dyna);
        objects->switchFlag = extra.value("switchFlag", objects->switchFlag);
        objects->timer = extra.value("timer", objects->timer);
        objects->cameraSetting = extra.value("cameraSetting", objects->cameraSetting);
    }
}

void Anchor::SendPacket_EnemyUpdate(std::vector<Actor*> actors) {
    if (!IsRoomStable() || !HasEnemySyncAuthority()) {
        return;
    }

    uint32_t currentPlayerCount = 0;
    for (auto& [clientId, client] : clients) {
        if (client.sceneNum == gPlayState->sceneNum && client.curRoomNum == gPlayState->roomCtx.curRoom.num &&
            client.online && client.isSaveLoaded && !client.self) {
            currentPlayerCount++;
        }
    }
    if (currentPlayerCount == 0 || actors.empty()) {
        return;
    }

    std::vector<uint64_t> networkIds;
    std::vector<s16> actorIds;
    std::vector<s16> actorParams;
    std::vector<s16> categories;
    std::vector<float> posX;
    std::vector<float> posY;
    std::vector<float> posZ;
    std::vector<s16> worldRotX;
    std::vector<s16> worldRotY;
    std::vector<s16> worldRotZ;
    std::vector<s16> shapeRotX;
    std::vector<s16> shapeRotY;
    std::vector<s16> shapeRotZ;
    std::vector<float> scaleX;
    std::vector<float> scaleY;
    std::vector<float> scaleZ;
    std::vector<float> velocityX;
    std::vector<float> velocityY;
    std::vector<float> velocityZ;
    std::vector<float> speedXZ;
    std::vector<float> gravity;
    std::vector<float> minVelocityY;
    std::vector<s16> yawTowardsPlayer;
    std::vector<float> xzDistToPlayer;
    std::vector<float> yDistToPlayer;
    std::vector<float> xyzDistToPlayerSq;
    std::vector<u16> freezeTimer;
    std::vector<u8> colorFilterTimer;
    std::vector<u16> colorFilterParams;
    std::vector<u8> health;
    std::vector<nlohmann::json> extraStates;

    for (Actor* actor : actors) {
        if (actor == nullptr || (!IsEnemySyncActor(actor) && GetEnemyNetworkId(actor) == 0)) {
            continue;
        }

        actorIds.push_back(actor->id);
        actorParams.push_back(actor->params);
        networkIds.push_back(GetEnemyNetworkId(actor));
        categories.push_back(actor->category);
        posX.push_back(actor->world.pos.x);
        posY.push_back(actor->world.pos.y);
        posZ.push_back(actor->world.pos.z);
        worldRotX.push_back(actor->world.rot.x);
        worldRotY.push_back(actor->world.rot.y);
        worldRotZ.push_back(actor->world.rot.z);
        shapeRotX.push_back(actor->shape.rot.x);
        shapeRotY.push_back(actor->shape.rot.y);
        shapeRotZ.push_back(actor->shape.rot.z);
        scaleX.push_back(actor->scale.x);
        scaleY.push_back(actor->scale.y);
        scaleZ.push_back(actor->scale.z);
        velocityX.push_back(actor->velocity.x);
        velocityY.push_back(actor->velocity.y);
        velocityZ.push_back(actor->velocity.z);
        speedXZ.push_back(actor->speedXZ);
        gravity.push_back(actor->gravity);
        minVelocityY.push_back(actor->minVelocityY);
        yawTowardsPlayer.push_back(actor->yawTowardsPlayer);
        xzDistToPlayer.push_back(actor->xzDistToPlayer);
        yDistToPlayer.push_back(actor->yDistToPlayer);
        xyzDistToPlayerSq.push_back(actor->xyzDistToPlayerSq);
        freezeTimer.push_back(actor->freezeTimer);
        colorFilterTimer.push_back(actor->colorFilterTimer);
        colorFilterParams.push_back(actor->colorFilterParams);
        health.push_back(actor->colChkInfo.health);
        extraStates.push_back(GetEnemyExtraState(actor));
    }

    if (actorIds.empty()) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = ENEMY_UPDATE;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["roomNum"] = gPlayState->roomCtx.curRoom.num;
    payload["authorityClientId"] = ownClientId;
    payload["authorityGeneration"] = GetEnemyRoomAuthorityGeneration(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num);
    payload["networkIds"] = networkIds;
    payload["actorIds"] = actorIds;
    payload["actorParams"] = actorParams;
    payload["categories"] = categories;
    payload["posX"] = posX;
    payload["posY"] = posY;
    payload["posZ"] = posZ;
    payload["worldRotX"] = worldRotX;
    payload["worldRotY"] = worldRotY;
    payload["worldRotZ"] = worldRotZ;
    payload["shapeRotX"] = shapeRotX;
    payload["shapeRotY"] = shapeRotY;
    payload["shapeRotZ"] = shapeRotZ;
    payload["scaleX"] = scaleX;
    payload["scaleY"] = scaleY;
    payload["scaleZ"] = scaleZ;
    payload["velocityX"] = velocityX;
    payload["velocityY"] = velocityY;
    payload["velocityZ"] = velocityZ;
    payload["speedXZ"] = speedXZ;
    payload["gravity"] = gravity;
    payload["minVelocityY"] = minVelocityY;
    payload["yawTowardsPlayer"] = yawTowardsPlayer;
    payload["xzDistToPlayer"] = xzDistToPlayer;
    payload["yDistToPlayer"] = yDistToPlayer;
    payload["xyzDistToPlayerSq"] = xyzDistToPlayerSq;
    payload["freezeTimer"] = freezeTimer;
    payload["colorFilterTimer"] = colorFilterTimer;
    payload["colorFilterParams"] = colorFilterParams;
    payload["health"] = health;
    payload["extraStates"] = extraStates;
    payload["quiet"] = true;

    for (auto& [clientId, client] : clients) {
        if (client.sceneNum == gPlayState->sceneNum && client.curRoomNum == gPlayState->roomCtx.curRoom.num &&
            client.online && client.isSaveLoaded && !client.self) {
            payload["targetClientId"] = clientId;
            SendJsonToRemote(payload);
        }
    }
}

void Anchor::HandlePacket_EnemyUpdate(nlohmann::json payload) {
    if (!IsRoomStable()) {
        return;
    }

    if (!IsValidEnemyAuthorityPacket(payload)) {
        return;
    }

    auto networkIds = payload.at("networkIds").get<std::vector<uint64_t>>();
    auto actorIds = payload.at("actorIds").get<std::vector<s16>>();
    auto actorParams = payload.value("actorParams", std::vector<s16>{});
    auto categories = payload.at("categories").get<std::vector<s16>>();
    auto posX = payload.at("posX").get<std::vector<float>>();
    auto posY = payload.at("posY").get<std::vector<float>>();
    auto posZ = payload.at("posZ").get<std::vector<float>>();
    auto worldRotX = payload.at("worldRotX").get<std::vector<s16>>();
    auto worldRotY = payload.at("worldRotY").get<std::vector<s16>>();
    auto worldRotZ = payload.at("worldRotZ").get<std::vector<s16>>();
    auto shapeRotX = payload.at("shapeRotX").get<std::vector<s16>>();
    auto shapeRotY = payload.at("shapeRotY").get<std::vector<s16>>();
    auto shapeRotZ = payload.at("shapeRotZ").get<std::vector<s16>>();
    auto scaleX = payload.value("scaleX", std::vector<float>{});
    auto scaleY = payload.value("scaleY", std::vector<float>{});
    auto scaleZ = payload.value("scaleZ", std::vector<float>{});
    auto velocityX = payload.at("velocityX").get<std::vector<float>>();
    auto velocityY = payload.at("velocityY").get<std::vector<float>>();
    auto velocityZ = payload.at("velocityZ").get<std::vector<float>>();
    auto speedXZ = payload.at("speedXZ").get<std::vector<float>>();
    auto gravity = payload.at("gravity").get<std::vector<float>>();
    auto minVelocityY = payload.at("minVelocityY").get<std::vector<float>>();
    auto yawTowardsPlayer = payload.value("yawTowardsPlayer", std::vector<s16>{});
    auto xzDistToPlayer = payload.value("xzDistToPlayer", std::vector<float>{});
    auto yDistToPlayer = payload.value("yDistToPlayer", std::vector<float>{});
    auto xyzDistToPlayerSq = payload.value("xyzDistToPlayerSq", std::vector<float>{});
    auto freezeTimer = payload.at("freezeTimer").get<std::vector<u16>>();
    auto colorFilterTimer = payload.at("colorFilterTimer").get<std::vector<u8>>();
    auto colorFilterParams = payload.value("colorFilterParams", std::vector<u16>{});
    auto health = payload.at("health").get<std::vector<u8>>();
    auto extraStates = payload.value("extraStates", std::vector<nlohmann::json>{});

    size_t enemyCount = actorIds.size();
    if (networkIds.size() != enemyCount || (!actorParams.empty() && actorParams.size() != enemyCount) ||
        categories.size() != enemyCount || posX.size() != enemyCount ||
        posY.size() != enemyCount || posZ.size() != enemyCount || worldRotX.size() != enemyCount ||
        worldRotY.size() != enemyCount || worldRotZ.size() != enemyCount || shapeRotX.size() != enemyCount ||
        shapeRotY.size() != enemyCount || shapeRotZ.size() != enemyCount ||
        (!scaleX.empty() && scaleX.size() != enemyCount) || (!scaleY.empty() && scaleY.size() != enemyCount) ||
        (!scaleZ.empty() && scaleZ.size() != enemyCount) || velocityX.size() != enemyCount || velocityY.size() != enemyCount ||
        velocityZ.size() != enemyCount || speedXZ.size() != enemyCount || gravity.size() != enemyCount ||
        minVelocityY.size() != enemyCount || (!yawTowardsPlayer.empty() && yawTowardsPlayer.size() != enemyCount) ||
        (!xzDistToPlayer.empty() && xzDistToPlayer.size() != enemyCount) ||
        (!yDistToPlayer.empty() && yDistToPlayer.size() != enemyCount) ||
        (!xyzDistToPlayerSq.empty() && xyzDistToPlayerSq.size() != enemyCount) ||
        freezeTimer.size() != enemyCount || colorFilterTimer.size() != enemyCount ||
        (!colorFilterParams.empty() && colorFilterParams.size() != enemyCount) ||
        (!extraStates.empty() && extraStates.size() != enemyCount) ||
        health.size() != enemyCount) {
        return;
    }

    for (size_t i = 0; i < enemyCount; i++) {
        ActorCategory category = (ActorCategory)categories[i];

        Vec3f pos = { posX[i], posY[i], posZ[i] };
        Actor* target = FindActorByEnemyNetworkId(networkIds[i]);
        if (target == nullptr && IsEnemySyncActor(category, actorIds[i])) {
            target = FindClosestUnassignedActorByCategoryAndId(category, actorIds[i], pos, 100000.0f);
            SetEnemyNetworkId(target, networkIds[i]);
        } else if (target == nullptr && (actorIds[i] == ACTOR_EN_ITEM00 || actorIds[i] == ACTOR_EN_ELF) &&
                   !actorParams.empty()) {
            spawningNetworkedEnemyDropId = networkIds[i];
            target = Actor_Spawn(&gPlayState->actorCtx, gPlayState, actorIds[i], pos.x, pos.y, pos.z, worldRotX[i],
                                 worldRotY[i], worldRotZ[i], actorParams[i]);
            spawningNetworkedEnemyDropId = 0;
            SetEnemyNetworkId(target, networkIds[i]);
        }
        if (target == nullptr) {
            continue;
        }

        EnemyAuthorityState state = { actorIds[i],
                                      category,
                                      pos,
                                      { worldRotX[i], worldRotY[i], worldRotZ[i] },
                                      { shapeRotX[i], shapeRotY[i], shapeRotZ[i] },
                                      { scaleX.empty() ? target->scale.x : scaleX[i], scaleY.empty() ? target->scale.y : scaleY[i],
                                        scaleZ.empty() ? target->scale.z : scaleZ[i] },
                                      { velocityX[i], velocityY[i], velocityZ[i] },
                                      speedXZ[i],
                                      gravity[i],
                                      minVelocityY[i],
                                      yawTowardsPlayer.empty() ? (s16)0 : yawTowardsPlayer[i],
                                      xzDistToPlayer.empty() ? 0.0f : xzDistToPlayer[i],
                                      yDistToPlayer.empty() ? 0.0f : yDistToPlayer[i],
                                      xyzDistToPlayerSq.empty() ? 0.0f : xyzDistToPlayerSq[i],
                                      freezeTimer[i],
                                      colorFilterTimer[i],
                                      colorFilterParams.empty() ? (u16)0 : colorFilterParams[i],
                                      health[i] };
        enemyAuthorityTargets[networkIds[i]] = state;
        ApplyEnemyAuthorityState(target, state, false);
        if (!extraStates.empty()) {
            nlohmann::json extraState = extraStates[i];
            if (!extraState.is_object() || extraState.value("kind", std::string("")).empty()) {
                enemyExtraStates.erase(networkIds[i]);
            } else {
                enemyExtraStates[networkIds[i]] = extraState;
                if (!(target->colChkInfo.health < state.health)) {
                    ApplyEnemyExtraState(target, extraState);
                }
            }
        }
    }
}
