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
    if (!IsSaveLoaded() || !HasEnemySyncAuthority()) {
        return;
    }

    std::vector<uint64_t> enemiesNetworkId;
    std::vector<s16> enemiesId;
    std::vector<s16> enemiesParams;
    std::vector<float> enemiesX;
    std::vector<float> enemiesY;
    std::vector<float> enemiesZ;
    std::vector<u8> enemiesHealth;

    Actor* currAct = gPlayState->actorCtx.actorLists[category].head;
    while (currAct != nullptr) {
        enemiesNetworkId.push_back(GetEnemyNetworkId(currAct));
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
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["roomNum"] = gPlayState->roomCtx.curRoom.num;
    payload["category"] = category;
    payload["enemiesNetworkId"] = enemiesNetworkId;
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

    if (clientId != GetEnemySyncAuthorityClientId()) {
        return;
    }

    s16 sceneNum = payload.value("sceneNum", (s16)SCENE_ID_MAX);
    s8 roomNum = payload.value("roomNum", (s8)-1);
    if (sceneNum != gPlayState->sceneNum || roomNum != gPlayState->roomCtx.curRoom.num) {
        return;
    }

    ActorCategory category = (ActorCategory)payload.at("category").get<s16>();
    auto enemiesNetworkId = payload.value("enemiesNetworkId", std::vector<uint64_t>{});
    auto enemiesId = payload.at("enemiesId").get<std::vector<s16>>();
    auto enemiesX = payload.at("enemiesX").get<std::vector<float>>();
    auto enemiesY = payload.at("enemiesY").get<std::vector<float>>();
    auto enemiesZ = payload.at("enemiesZ").get<std::vector<float>>();
    auto enemiesHealth = payload.at("enemiesHealth").get<std::vector<u8>>();

    if (!enemiesNetworkId.empty() && enemiesNetworkId.size() != enemiesId.size()) {
        return;
    }

    std::vector<bool> remoteMatched(enemiesId.size(), false);
    std::vector<Actor*> localActors;

    Actor* currAct = gPlayState->actorCtx.actorLists[category].head;
    while (currAct != nullptr) {
        localActors.push_back(currAct);
        currAct = currAct->next;
    }

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
            if (!enemiesNetworkId.empty()) {
                SetEnemyNetworkId(localActors[li], enemiesNetworkId[closestIdx]);
            }
            if (enemiesHealth[closestIdx] > 0) {
                localActors[li]->colChkInfo.health = enemiesHealth[closestIdx];
                enemyHealthTracker[localActors[li]] = enemiesHealth[closestIdx];
            } else {
                actorKillBuffer.push_back(localActors[li]);
            }
        }
    }
}
