#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "macros.h"
#include "functions.h"
extern PlayState* gPlayState;
}

void Anchor::SendPacket_SendRoomEnemies(u32 targetClientId, ActorCategory category) {
    if (!IsSaveLoaded()) {
        return;
    }

    std::vector<s16> enemiesId;
    std::vector<s16> enemiesParams;
    std::vector<float> enemiesX;
    std::vector<float> enemiesY;
    std::vector<float> enemiesZ;
    std::vector<u8> enemiesHealth;

    Actor* currAct = gPlayState->actorCtx.actorLists[category].head;
    while (currAct != nullptr) {
        enemiesId.push_back(currAct->id);
        enemiesParams.push_back(currAct->params);
        enemiesX.push_back(currAct->world.pos.x);
        enemiesY.push_back(currAct->world.pos.y);
        enemiesZ.push_back(currAct->world.pos.z);
        enemiesHealth.push_back(currAct->colChkInfo.health);
        currAct = currAct->next;
    }

    nlohmann::json payload;
    payload["type"] = SEND_ROOM_ENEMIES;
    payload["targetClientId"] = targetClientId;
    payload["category"] = category;
    payload["enemiesId"] = enemiesId;
    payload["enemiesParams"] = enemiesParams;
    payload["enemiesX"] = enemiesX;
    payload["enemiesY"] = enemiesY;
    payload["enemiesZ"] = enemiesZ;
    payload["enemiesHealth"] = enemiesHealth;
    payload["quiet"] = true;

    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_SendRoomEnemies(nlohmann::json payload) {
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

    ActorCategory category = (ActorCategory)payload.at("category").get<s16>();
    auto enemiesId = payload.at("enemiesId").get<std::vector<s16>>();
    auto enemiesParams = payload.at("enemiesParams").get<std::vector<s16>>();
    auto enemiesX = payload.at("enemiesX").get<std::vector<float>>();
    auto enemiesY = payload.at("enemiesY").get<std::vector<float>>();
    auto enemiesZ = payload.at("enemiesZ").get<std::vector<float>>();
    auto enemiesHealth = payload.at("enemiesHealth").get<std::vector<u8>>();

    std::vector<bool> remoteMatched(enemiesId.size(), false);
    std::vector<Actor*> localActors;

    Actor* currAct = gPlayState->actorCtx.actorLists[category].head;
    while (currAct != nullptr) {
        localActors.push_back(currAct);
        currAct = currAct->next;
    }

    std::vector<bool> localMatched(localActors.size(), false);

    for (size_t li = 0; li < localActors.size(); li++) {
        float closestDist = -1.0f;
        int closestIdx = -1;

        for (size_t ri = 0; ri < enemiesId.size(); ri++) {
            if (remoteMatched[ri]) {
                continue;
            }
            if (localActors[li]->id != enemiesId[ri]) {
                continue;
            }

            float dx = localActors[li]->world.pos.x - enemiesX[ri];
            float dy = localActors[li]->world.pos.y - enemiesY[ri];
            float dz = localActors[li]->world.pos.z - enemiesZ[ri];
            float distance = dx * dx + dy * dy + dz * dz;
            if (closestIdx == -1 || distance < closestDist) {
                closestDist = distance;
                closestIdx = (int)ri;
            }
        }

        if (closestIdx >= 0) {
            remoteMatched[closestIdx] = true;
            localMatched[li] = true;
            if (enemiesHealth[closestIdx] > 0) {
                localActors[li]->colChkInfo.health = enemiesHealth[closestIdx];
            } else {
                actorKillBuffer.push_back(localActors[li]);
            }
        }
    }

    for (size_t li = 0; li < localActors.size(); li++) {
        if (!localMatched[li] && category == ACTORCAT_ENEMY) {
            actorKillBuffer.push_back(localActors[li]);
        }
    }

    for (size_t ri = 0; ri < remoteMatched.size(); ri++) {
        if (!remoteMatched[ri]) {
            Vec3f pos = { enemiesX[ri], enemiesY[ri], enemiesZ[ri] };
            enemySpawnBuffer.push_back(std::make_tuple(enemiesId[ri], enemiesParams[ri], pos));
        }
    }
}
