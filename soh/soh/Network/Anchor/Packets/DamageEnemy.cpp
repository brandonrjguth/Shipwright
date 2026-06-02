#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "macros.h"
#include "functions.h"
extern PlayState* gPlayState;
}

extern nlohmann::json GetEnemyExtraState(Actor* actor);
extern void ApplyEnemyExtraState(Actor* actor, nlohmann::json extra);

static EnemyAuthorityState BuildEnemyAuthorityStateFromDamagePayload(Actor* target, nlohmann::json payload, u8 health) {
    return { target->id,
             (ActorCategory)target->category,
             { payload.value("posX", target->world.pos.x), payload.value("posY", target->world.pos.y),
               payload.value("posZ", target->world.pos.z) },
             { payload.value("worldRotX", target->world.rot.x), payload.value("worldRotY", target->world.rot.y),
               payload.value("worldRotZ", target->world.rot.z) },
             { payload.value("shapeRotX", target->shape.rot.x), payload.value("shapeRotY", target->shape.rot.y),
               payload.value("shapeRotZ", target->shape.rot.z) },
             { payload.value("scaleX", target->scale.x), payload.value("scaleY", target->scale.y),
               payload.value("scaleZ", target->scale.z) },
             { payload.value("velocityX", target->velocity.x), payload.value("velocityY", target->velocity.y),
               payload.value("velocityZ", target->velocity.z) },
             payload.value("speedXZ", target->speedXZ),
             payload.value("gravity", target->gravity),
             payload.value("minVelocityY", target->minVelocityY),
             payload.value("yawTowardsPlayer", target->yawTowardsPlayer),
             payload.value("xzDistToPlayer", target->xzDistToPlayer),
             payload.value("yDistToPlayer", target->yDistToPlayer),
             payload.value("xyzDistToPlayerSq", target->xyzDistToPlayerSq),
             payload.value("freezeTimer", target->freezeTimer),
             payload.value("colorFilterTimer", target->colorFilterTimer),
             payload.value("colorFilterParams", target->colorFilterParams),
             health };
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
    payload["worldRotX"] = actor->world.rot.x;
    payload["worldRotY"] = actor->world.rot.y;
    payload["worldRotZ"] = actor->world.rot.z;
    payload["shapeRotX"] = actor->shape.rot.x;
    payload["shapeRotY"] = actor->shape.rot.y;
    payload["shapeRotZ"] = actor->shape.rot.z;
    payload["scaleX"] = actor->scale.x;
    payload["scaleY"] = actor->scale.y;
    payload["scaleZ"] = actor->scale.z;
    payload["velocityX"] = actor->velocity.x;
    payload["velocityY"] = actor->velocity.y;
    payload["velocityZ"] = actor->velocity.z;
    payload["speedXZ"] = actor->speedXZ;
    payload["gravity"] = actor->gravity;
    payload["minVelocityY"] = actor->minVelocityY;
    payload["yawTowardsPlayer"] = actor->yawTowardsPlayer;
    payload["xzDistToPlayer"] = actor->xzDistToPlayer;
    payload["yDistToPlayer"] = actor->yDistToPlayer;
    payload["xyzDistToPlayerSq"] = actor->xyzDistToPlayerSq;
    payload["freezeTimer"] = actor->freezeTimer;
    payload["colorFilterTimer"] = actor->colorFilterTimer;
    payload["colorFilterParams"] = actor->colorFilterParams;
    payload["category"] = actor->category;
    payload["extraState"] = GetEnemyExtraState(actor);
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
        if (payload.contains("extraState")) {
            ApplyEnemyAuthorityState(target, BuildEnemyAuthorityStateFromDamagePayload(target, payload, health), true);
            ApplyEnemyExtraState(target, payload["extraState"]);
        } else {
            target->colChkInfo.health = health;
        }
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
    payload["worldRotX"] = actor->world.rot.x;
    payload["worldRotY"] = actor->world.rot.y;
    payload["worldRotZ"] = actor->world.rot.z;
    payload["shapeRotX"] = actor->shape.rot.x;
    payload["shapeRotY"] = actor->shape.rot.y;
    payload["shapeRotZ"] = actor->shape.rot.z;
    payload["scaleX"] = actor->scale.x;
    payload["scaleY"] = actor->scale.y;
    payload["scaleZ"] = actor->scale.z;
    payload["velocityX"] = actor->velocity.x;
    payload["velocityY"] = actor->velocity.y;
    payload["velocityZ"] = actor->velocity.z;
    payload["speedXZ"] = actor->speedXZ;
    payload["gravity"] = actor->gravity;
    payload["minVelocityY"] = actor->minVelocityY;
    payload["yawTowardsPlayer"] = actor->yawTowardsPlayer;
    payload["xzDistToPlayer"] = actor->xzDistToPlayer;
    payload["yDistToPlayer"] = actor->yDistToPlayer;
    payload["xyzDistToPlayerSq"] = actor->xyzDistToPlayerSq;
    payload["freezeTimer"] = actor->freezeTimer;
    payload["colorFilterTimer"] = actor->colorFilterTimer;
    payload["colorFilterParams"] = actor->colorFilterParams;
    payload["category"] = actor->category;
    payload["extraState"] = GetEnemyExtraState(actor);
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
        ApplyEnemyAuthorityState(target, BuildEnemyAuthorityStateFromDamagePayload(target, payload, health), true);
        if (payload.contains("extraState")) {
            ApplyEnemyExtraState(target, payload["extraState"]);
        }
        enemyHealthTracker[target] = target->colChkInfo.health;
        SendPacket_DamageEnemy(target, target->colChkInfo.health);
    }
}
