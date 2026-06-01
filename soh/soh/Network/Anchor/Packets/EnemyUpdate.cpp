#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>

extern "C" {
#include "variables.h"
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
extern PlayState* gPlayState;
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

static nlohmann::json GetEnemyExtraState(Actor* actor) {
    nlohmann::json extra;

    switch (actor->id) {
        case ACTOR_EN_DEKUNUTS: {
            EnDekunuts* dekunuts = (EnDekunuts*)actor;
            extra["kind"] = "EnDekunuts";
            extra["playWalkSound"] = dekunuts->playWalkSound;
            extra["runAwayCount"] = dekunuts->runAwayCount;
            extra["animFlagAndTimer"] = dekunuts->animFlagAndTimer;
            extra["runDirection"] = dekunuts->runDirection;
            extra["shotsPerRound"] = dekunuts->shotsPerRound;
            AddSkelAnimeState(extra, &dekunuts->skelAnime);
            break;
        }
        case ACTOR_EN_DEKUBABA: {
            EnDekubaba* dekubaba = (EnDekubaba*)actor;
            extra["kind"] = "EnDekubaba";
            extra["timer"] = dekubaba->timer;
            extra["targetSwayAngle"] = dekubaba->targetSwayAngle;
            extra["stemSectionAngle"] = { dekubaba->stemSectionAngle[0], dekubaba->stemSectionAngle[1],
                                           dekubaba->stemSectionAngle[2] };
            extra["size"] = dekubaba->size;
            AddSkelAnimeState(extra, &dekubaba->skelAnime);
            break;
        }
        case ACTOR_EN_ST: {
            EnSt* st = (EnSt*)actor;
            extra["kind"] = "EnSt";
            extra["rotAwayTimer"] = st->rotAwayTimer;
            extra["rotTowardsTimer"] = st->rotTowardsTimer;
            extra["takeDamageSpinTimer"] = st->takeDamageSpinTimer;
            extra["stunTimer"] = st->stunTimer;
            extra["swayTimer"] = st->swayTimer;
            extra["swayAngle"] = st->swayAngle;
            extra["animFrames"] = st->animFrames;
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
    }

    return extra;
}

static void ApplyEnemyExtraState(Actor* actor, nlohmann::json extra) {
    std::string kind = extra.value("kind", "");

    if (actor->id == ACTOR_EN_DEKUNUTS && kind == "EnDekunuts") {
        EnDekunuts* dekunuts = (EnDekunuts*)actor;
        dekunuts->playWalkSound = extra.value("playWalkSound", dekunuts->playWalkSound);
        dekunuts->runAwayCount = extra.value("runAwayCount", dekunuts->runAwayCount);
        dekunuts->animFlagAndTimer = extra.value("animFlagAndTimer", dekunuts->animFlagAndTimer);
        dekunuts->runDirection = extra.value("runDirection", dekunuts->runDirection);
        dekunuts->shotsPerRound = extra.value("shotsPerRound", dekunuts->shotsPerRound);
        ApplySkelAnimeState(extra, &dekunuts->skelAnime);
    } else if (actor->id == ACTOR_EN_DEKUBABA && kind == "EnDekubaba") {
        EnDekubaba* dekubaba = (EnDekubaba*)actor;
        dekubaba->timer = extra.value("timer", dekubaba->timer);
        dekubaba->targetSwayAngle = extra.value("targetSwayAngle", dekubaba->targetSwayAngle);
        std::vector<s16> stemSectionAngle = extra.value("stemSectionAngle", std::vector<s16>{});
        if (stemSectionAngle.size() == 3) {
            dekubaba->stemSectionAngle[0] = stemSectionAngle[0];
            dekubaba->stemSectionAngle[1] = stemSectionAngle[1];
            dekubaba->stemSectionAngle[2] = stemSectionAngle[2];
        }
        dekubaba->size = extra.value("size", dekubaba->size);
        ApplySkelAnimeState(extra, &dekubaba->skelAnime);
    } else if (actor->id == ACTOR_EN_ST && kind == "EnSt") {
        EnSt* st = (EnSt*)actor;
        st->rotAwayTimer = extra.value("rotAwayTimer", st->rotAwayTimer);
        st->rotTowardsTimer = extra.value("rotTowardsTimer", st->rotTowardsTimer);
        st->takeDamageSpinTimer = extra.value("takeDamageSpinTimer", st->takeDamageSpinTimer);
        st->stunTimer = extra.value("stunTimer", st->stunTimer);
        st->swayTimer = extra.value("swayTimer", st->swayTimer);
        st->swayAngle = extra.value("swayAngle", st->swayAngle);
        st->animFrames = extra.value("animFrames", st->animFrames);
        st->floorHeightOffset = extra.value("floorHeightOffset", st->floorHeightOffset);
        st->colliderScale = extra.value("colliderScale", st->colliderScale);
        ApplySkelAnimeState(extra, &st->skelAnime);
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
    }
}

void Anchor::SendPacket_EnemyUpdate(std::vector<Actor*> actors) {
    if (!IsSaveLoaded() || !HasEnemySyncAuthority()) {
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
        if (actor == nullptr || (actor->category != ACTORCAT_ENEMY && actor->category != ACTORCAT_BOSS)) {
            continue;
        }

        actorIds.push_back(actor->id);
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
    if (!IsSaveLoaded()) {
        return;
    }

    if (!IsValidEnemyAuthorityPacket(payload)) {
        return;
    }

    auto networkIds = payload.at("networkIds").get<std::vector<uint64_t>>();
    auto actorIds = payload.at("actorIds").get<std::vector<s16>>();
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
    if (networkIds.size() != enemyCount || categories.size() != enemyCount || posX.size() != enemyCount ||
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
        if (category != ACTORCAT_ENEMY && category != ACTORCAT_BOSS) {
            continue;
        }

        Vec3f pos = { posX[i], posY[i], posZ[i] };
        Actor* target = FindActorByEnemyNetworkId(networkIds[i]);
        if (target == nullptr) {
            target = FindClosestActorByCategoryAndId(category, actorIds[i], pos);
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
            ApplyEnemyExtraState(target, extraStates[i]);
        }
    }
}
