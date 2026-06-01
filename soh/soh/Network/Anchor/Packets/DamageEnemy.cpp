#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "macros.h"
#include "functions.h"
extern PlayState* gPlayState;
}

void Anchor::SendPacket_DamageEnemy(Actor* actor, u8 health) {
    if (!IsSaveLoaded()) {
        return;
    }

    if (actor->category != ACTORCAT_ENEMY && actor->category != ACTORCAT_BOSS) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = DAMAGE_ENEMY;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["roomNum"] = gPlayState->roomCtx.curRoom.num;
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

    uint32_t clientId = payload.at("clientId").get<uint32_t>();
    if (!clients.contains(clientId)) {
        return;
    }

    AnchorClient& client = clients[clientId];
    if (client.sceneNum != gPlayState->sceneNum) {
        return;
    }

    s16 sceneNum = payload.value("sceneNum", (s16)SCENE_ID_MAX);
    s8 roomNum = payload.value("roomNum", (s8)-1);
    if (sceneNum != gPlayState->sceneNum || roomNum != gPlayState->roomCtx.curRoom.num) {
        return;
    }

    uint64_t networkId = payload.value("networkId", (uint64_t)0);
    s16 actorId = payload.at("actorId").get<s16>();
    u8 health = payload.at("health").get<u8>();
    float posX = payload.at("posX").get<float>();
    float posY = payload.at("posY").get<float>();
    float posZ = payload.at("posZ").get<float>();
    s16 category = payload.at("category").get<s16>();

    Vec3f pos = { posX, posY, posZ };
    Actor* target = FindActorByEnemyNetworkId(networkId);
    if (target == nullptr) {
        target = FindClosestActorByCategoryAndId((ActorCategory)category, actorId, pos);
        SetEnemyNetworkId(target, networkId);
    }

    if (target != nullptr) {
        if (health > 0) {
            target->colChkInfo.health = health;
        } else {
            target->colChkInfo.health = 1;
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

    if (actor->category != ACTORCAT_ENEMY && actor->category != ACTORCAT_BOSS) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = REPORT_ENEMY_DAMAGE;
    payload["targetClientId"] = authorityClientId;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["roomNum"] = gPlayState->roomCtx.curRoom.num;
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

void Anchor::HandlePacket_ReportEnemyDamage(nlohmann::json payload) {
    if (!IsSaveLoaded() || !HasEnemySyncAuthority()) {
        return;
    }

    uint32_t clientId = payload.at("clientId").get<uint32_t>();
    if (!clients.contains(clientId)) {
        return;
    }

    AnchorClient& client = clients[clientId];
    if (client.sceneNum != gPlayState->sceneNum) {
        return;
    }

    s16 sceneNum = payload.value("sceneNum", (s16)SCENE_ID_MAX);
    s8 roomNum = payload.value("roomNum", (s8)-1);
    if (sceneNum != gPlayState->sceneNum || roomNum != gPlayState->roomCtx.curRoom.num) {
        return;
    }

    uint64_t networkId = payload.value("networkId", (uint64_t)0);
    s16 actorId = payload.at("actorId").get<s16>();
    u8 health = payload.at("health").get<u8>();
    float posX = payload.at("posX").get<float>();
    float posY = payload.at("posY").get<float>();
    float posZ = payload.at("posZ").get<float>();
    s16 category = payload.at("category").get<s16>();

    Vec3f pos = { posX, posY, posZ };
    Actor* target = FindActorByEnemyNetworkId(networkId);
    if (target == nullptr) {
        target = FindClosestActorByCategoryAndId((ActorCategory)category, actorId, pos);
        SetEnemyNetworkId(target, networkId);
    }

    if (target != nullptr && health < target->colChkInfo.health) {
        if (health == 0) {
            actorKillBuffer.push_back(target);
            SendPacket_KillEnemy(target);
            return;
        }

        target->colChkInfo.health = health;
        enemyHealthTracker[target] = target->colChkInfo.health;
        SendPacket_DamageEnemy(target, target->colChkInfo.health);
    }
}
