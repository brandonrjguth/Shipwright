#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>

extern "C" {
#include "variables.h"
extern PlayState* gPlayState;
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
    std::vector<float> velocityX;
    std::vector<float> velocityY;
    std::vector<float> velocityZ;
    std::vector<float> speedXZ;
    std::vector<float> gravity;
    std::vector<float> minVelocityY;
    std::vector<u16> freezeTimer;
    std::vector<u8> colorFilterTimer;
    std::vector<u8> health;

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
        velocityX.push_back(actor->velocity.x);
        velocityY.push_back(actor->velocity.y);
        velocityZ.push_back(actor->velocity.z);
        speedXZ.push_back(actor->speedXZ);
        gravity.push_back(actor->gravity);
        minVelocityY.push_back(actor->minVelocityY);
        freezeTimer.push_back(actor->freezeTimer);
        colorFilterTimer.push_back(actor->colorFilterTimer);
        health.push_back(actor->colChkInfo.health);
    }

    if (actorIds.empty()) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = ENEMY_UPDATE;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["roomNum"] = gPlayState->roomCtx.curRoom.num;
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
    payload["velocityX"] = velocityX;
    payload["velocityY"] = velocityY;
    payload["velocityZ"] = velocityZ;
    payload["speedXZ"] = speedXZ;
    payload["gravity"] = gravity;
    payload["minVelocityY"] = minVelocityY;
    payload["freezeTimer"] = freezeTimer;
    payload["colorFilterTimer"] = colorFilterTimer;
    payload["health"] = health;
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

    uint32_t clientId = payload.at("clientId").get<uint32_t>();
    if (!clients.contains(clientId)) {
        return;
    }

    AnchorClient& client = clients[clientId];
    if (client.sceneNum != gPlayState->sceneNum) {
        return;
    }

    if (ownClientId != 0 && ownClientId < clientId) {
        return;
    }
    for (auto& [otherClientId, otherClient] : clients) {
        if (otherClientId == clientId || !otherClient.online || otherClient.self || !otherClient.isSaveLoaded) {
            continue;
        }
        if (otherClient.sceneNum == gPlayState->sceneNum && otherClient.curRoomNum == gPlayState->roomCtx.curRoom.num &&
            otherClientId < clientId) {
            return;
        }
    }

    s16 sceneNum = payload.at("sceneNum").get<s16>();
    s8 roomNum = payload.at("roomNum").get<s8>();
    if (sceneNum != gPlayState->sceneNum || roomNum != gPlayState->roomCtx.curRoom.num) {
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
    auto velocityX = payload.at("velocityX").get<std::vector<float>>();
    auto velocityY = payload.at("velocityY").get<std::vector<float>>();
    auto velocityZ = payload.at("velocityZ").get<std::vector<float>>();
    auto speedXZ = payload.at("speedXZ").get<std::vector<float>>();
    auto gravity = payload.at("gravity").get<std::vector<float>>();
    auto minVelocityY = payload.at("minVelocityY").get<std::vector<float>>();
    auto freezeTimer = payload.at("freezeTimer").get<std::vector<u16>>();
    auto colorFilterTimer = payload.at("colorFilterTimer").get<std::vector<u8>>();
    auto health = payload.at("health").get<std::vector<u8>>();

    size_t enemyCount = actorIds.size();
    if (networkIds.size() != enemyCount || categories.size() != enemyCount || posX.size() != enemyCount ||
        posY.size() != enemyCount || posZ.size() != enemyCount || worldRotX.size() != enemyCount ||
        worldRotY.size() != enemyCount || worldRotZ.size() != enemyCount || shapeRotX.size() != enemyCount ||
        shapeRotY.size() != enemyCount || shapeRotZ.size() != enemyCount || velocityX.size() != enemyCount || velocityY.size() != enemyCount ||
        velocityZ.size() != enemyCount || speedXZ.size() != enemyCount || gravity.size() != enemyCount ||
        minVelocityY.size() != enemyCount || freezeTimer.size() != enemyCount || colorFilterTimer.size() != enemyCount ||
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
                                      { velocityX[i], velocityY[i], velocityZ[i] },
                                      speedXZ[i],
                                      gravity[i],
                                      minVelocityY[i],
                                      freezeTimer[i],
                                      colorFilterTimer[i],
                                      health[i] };
        enemyAuthorityTargets[networkIds[i]] = state;
        ApplyEnemyAuthorityState(target, state, false);
    }
}
