#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "macros.h"
#include "functions.h"
#include "src/overlays/actors/ovl_En_Dekubaba/z_en_dekubaba.h"
extern PlayState* gPlayState;

void EnDekubaba_SetupPrunedSomersault(EnDekubaba* thisx);
void EnDekubaba_SetupShrinkDie(EnDekubaba* thisx);
}

extern nlohmann::json GetEnemyExtraState(Actor* actor);
extern void ApplyEnemyExtraState(Actor* actor, nlohmann::json extra);

static void ApplyReportedDekuBabaContext(Actor* target, nlohmann::json payload, nlohmann::json extraState) {
    constexpr s32 DEKUBABA_ACTION_PRUNED_SOMERSAULT = 11;
    constexpr s32 DEKUBABA_ACTION_SHRINK_DIE = 12;

    if (target == nullptr || target->id != ACTOR_EN_DEKUBABA) {
        return;
    }

    s32 remoteAction = extraState.value("action", (s32)-1);

    if (target->colChkInfo.health == 0 && remoteAction == DEKUBABA_ACTION_PRUNED_SOMERSAULT) {
        EnDekubaba_SetupPrunedSomersault((EnDekubaba*)target);
        return;
    }

    if (target->colChkInfo.health == 0 && remoteAction == DEKUBABA_ACTION_SHRINK_DIE) {
        EnDekubaba_SetupShrinkDie((EnDekubaba*)target);
        return;
    }

    target->world.pos.x = payload.value("posX", target->world.pos.x);
    target->world.pos.y = payload.value("posY", target->world.pos.y);
    target->world.pos.z = payload.value("posZ", target->world.pos.z);
    target->prevPos = target->world.pos;
    target->world.rot.y = payload.value("worldRotY", target->world.rot.y);
    target->shape.rot.x = payload.value("shapeRotX", target->shape.rot.x);
    target->shape.rot.y = payload.value("shapeRotY", target->shape.rot.y);
    target->shape.rot.z = payload.value("shapeRotZ", target->shape.rot.z);

    if (remoteAction != DEKUBABA_ACTION_PRUNED_SOMERSAULT) {
        return;
    }

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
    if (actor->id == ACTOR_EN_DEKUBABA) {
        payload["extraState"] = GetEnemyExtraState(actor);
        payload["worldRotY"] = actor->world.rot.y;
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
    payload["quiet"] = true;

    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_ReportEnemyDamage(nlohmann::json payload) {
    if (!IsSaveLoaded() || !HasEnemySyncAuthority()) {
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

    if (health < target->colChkInfo.health) {
        bool hasDekuBabaState = target->id == ACTOR_EN_DEKUBABA && payload.contains("extraState") &&
                                payload["extraState"].is_object();

        if (health == 0 && !hasDekuBabaState) {
            enemyKillBuffer.push_back(networkId);
            return;
        }

        target->colChkInfo.health = health;
        if (hasDekuBabaState) {
            ApplyEnemyExtraState(target, payload["extraState"]);
            ApplyReportedDekuBabaContext(target, payload, payload["extraState"]);
        }
        enemyHealthTracker[target] = target->colChkInfo.health;
        SendPacket_DamageEnemy(target, target->colChkInfo.health);
    }
}
