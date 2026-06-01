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
    std::vector<u8> health;

    for (Actor* actor : actors) {
        if (actor == nullptr || (actor->category != ACTORCAT_ENEMY && actor->category != ACTORCAT_BOSS)) {
            continue;
        }

        actorIds.push_back(actor->id);
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
        health.push_back(actor->colChkInfo.health);
    }

    if (actorIds.empty()) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = ENEMY_UPDATE;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["roomNum"] = gPlayState->roomCtx.curRoom.num;
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
    if (client.sceneNum != gPlayState->sceneNum || client.curRoomNum != gPlayState->roomCtx.curRoom.num) {
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
    auto health = payload.at("health").get<std::vector<u8>>();

    size_t enemyCount = actorIds.size();
    if (categories.size() != enemyCount || posX.size() != enemyCount || posY.size() != enemyCount ||
        posZ.size() != enemyCount || worldRotX.size() != enemyCount || worldRotY.size() != enemyCount ||
        worldRotZ.size() != enemyCount || shapeRotX.size() != enemyCount || shapeRotY.size() != enemyCount ||
        shapeRotZ.size() != enemyCount || health.size() != enemyCount) {
        return;
    }

    for (size_t i = 0; i < enemyCount; i++) {
        ActorCategory category = (ActorCategory)categories[i];
        if (category != ACTORCAT_ENEMY && category != ACTORCAT_BOSS) {
            continue;
        }

        Vec3f pos = { posX[i], posY[i], posZ[i] };
        Actor* target = FindClosestActorByCategoryAndId(category, actorIds[i], pos);
        if (target == nullptr) {
            continue;
        }

        target->world.pos = pos;
        target->prevPos = pos;
        target->world.rot.x = worldRotX[i];
        target->world.rot.y = worldRotY[i];
        target->world.rot.z = worldRotZ[i];
        target->shape.rot.x = shapeRotX[i];
        target->shape.rot.y = shapeRotY[i];
        target->shape.rot.z = shapeRotZ[i];
        target->colChkInfo.health = health[i] > 0 ? health[i] : 1;
        enemyHealthTracker[target] = target->colChkInfo.health;
    }
}
