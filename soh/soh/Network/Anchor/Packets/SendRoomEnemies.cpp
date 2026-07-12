#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include <cmath>
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "macros.h"
#include "functions.h"
extern PlayState* gPlayState;
}

void Anchor::SendPacket_SendRoomEnemies(u32 targetClientId, ActorCategory category) {
    if (!IsRoomStable() || !HasEnemySyncAuthority()) {
        return;
    }

    std::vector<uint64_t> enemiesNetworkId;
    std::vector<s16> enemiesId;
    std::vector<s16> enemiesParams;
    std::vector<float> enemiesX;
    std::vector<float> enemiesY;
    std::vector<float> enemiesZ;
    std::vector<Actor*> actors;

    Actor* currAct = gPlayState->actorCtx.actorLists[category].head;
    while (currAct != nullptr) {
        if (IsEnemySyncActor(currAct) && IsActorInCurrentEnemyRoom(currAct)) {
            actors.push_back(currAct);
        }
        currAct = currAct->next;
    }

    if (!actors.empty()) {
        AssignEnemyNetworkIds(actors);
    }

    for (Actor* currAct : actors) {
        enemiesNetworkId.push_back(GetEnemyNetworkId(currAct));
        enemiesId.push_back(currAct->id);
        enemiesParams.push_back(GetEnemySpawnParams(currAct));
        enemiesX.push_back(currAct->world.pos.x);
        enemiesY.push_back(currAct->world.pos.y);
        enemiesZ.push_back(currAct->world.pos.z);
    }

    nlohmann::json payload;
    payload["type"] = SEND_ROOM_ENEMIES;
    payload["targetClientId"] = targetClientId;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["roomNum"] = gPlayState->roomCtx.curRoom.num;
    payload["authorityClientId"] = ownClientId;
    payload["authorityGeneration"] = GetEnemyRoomAuthorityGeneration(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num);
    payload["snapshotSequence"] = NextEnemySnapshotSequence(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num);
    payload["enemySessionId"] = enemySessionId;
    payload["category"] = category;
    payload["enemiesNetworkId"] = enemiesNetworkId;
    auto& sceneTombstones = deadEnemyLedger[(uint16_t)gPlayState->sceneNum];
    payload["deadEnemiesNetworkId"] =
        std::vector<uint64_t>(sceneTombstones.begin(), sceneTombstones.end());
    payload["enemiesId"] = enemiesId;
    payload["enemiesParams"] = enemiesParams;
    payload["enemiesX"] = enemiesX;
    payload["enemiesY"] = enemiesY;
    payload["enemiesZ"] = enemiesZ;
    payload["quiet"] = true;

    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_SendRoomEnemies(nlohmann::json payload) {
    if (!IsRoomStable()) {
        return;
    }

    if (!IsValidEnemyAuthorityPacket(payload)) {
        return;
    }

    s16 rawCategory = payload.at("category").get<s16>();
    if (rawCategory < ACTORCAT_SWITCH || rawCategory >= ACTORCAT_MAX) {
        return;
    }
    ActorCategory category = (ActorCategory)rawCategory;
    s8 authorityRoomNum = payload.value("roomNum", (s8)-1);
    auto enemiesNetworkId = payload.value("enemiesNetworkId", std::vector<uint64_t>{});
    auto deadEnemiesNetworkId = payload.value("deadEnemiesNetworkId", std::vector<uint64_t>{});
    auto enemiesId = payload.at("enemiesId").get<std::vector<s16>>();
    auto enemiesX = payload.at("enemiesX").get<std::vector<float>>();
    auto enemiesY = payload.at("enemiesY").get<std::vector<float>>();
    auto enemiesZ = payload.at("enemiesZ").get<std::vector<float>>();
    auto enemiesParams = payload.value("enemiesParams", std::vector<s16>{});

    size_t enemyCount = enemiesId.size();
    if (enemyCount > 1024 || deadEnemiesNetworkId.size() > 4096 || enemiesNetworkId.size() != enemyCount ||
        enemiesX.size() != enemyCount ||
        enemiesY.size() != enemyCount || enemiesZ.size() != enemyCount ||
        (!enemiesParams.empty() && enemiesParams.size() != enemyCount)) {
        return;
    }
    if (!IsNewEnemySnapshotPacket(payload)) {
        return;
    }

    for (uint64_t networkId : deadEnemiesNetworkId) {
        if (IsEnemyMarkedDead(networkId)) {
            continue;
        }
        MarkEnemyDead(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num, networkId);
        Actor* deadActor = FindActorByEnemyNetworkId(networkId);
        if (deadActor != nullptr) {
            enemyKillBuffer.push_back(networkId);
        }
    }

    std::vector<Actor*> localActors;
    Actor* currAct = gPlayState->actorCtx.actorLists[category].head;
    while (currAct != nullptr) {
        if (IsActorInCurrentEnemyRoom(currAct)) {
            localActors.push_back(currAct);
        }
        currAct = currAct->next;
    }

    for (size_t ri = 0; ri < enemiesId.size(); ri++) {
        if (enemiesNetworkId.empty() || enemiesNetworkId[ri] == 0) {
            continue;
        }
        if (enemiesId[ri] < 0 || enemiesId[ri] >= ACTOR_ID_MAX || !std::isfinite(enemiesX[ri]) ||
            !std::isfinite(enemiesY[ri]) || !std::isfinite(enemiesZ[ri]) ||
            IsEnemyMarkedDead(enemiesNetworkId[ri])) {
            continue;
        }
        if (IsTransientProjectileActor(enemiesId[ri], enemiesParams.empty() ? (s16)0 : enemiesParams[ri])) {
            continue;
        }

        Actor* matchedActor = FindActorByEnemyNetworkId(enemiesNetworkId[ri]);
        if (matchedActor != nullptr) {
            continue;
        }

        Vec3f remotePos = { enemiesX[ri], enemiesY[ri], enemiesZ[ri] };
        float closestDist = 100000.0f;
        Actor* closestActor = nullptr;

        for (size_t li = 0; li < localActors.size(); li++) {
            Actor* local = localActors[li];
            if (local->id != enemiesId[ri] || !IsEnemySyncActor(local)) {
                continue;
            }
            if (authorityRoomNum >= 0 && local->room >= 0 && local->room != authorityRoomNum) {
                continue;
            }
            if (IsEnemyMarkedDead(GetEnemyNetworkId(local))) {
                continue;
            }
            if (GetEnemyNetworkId(local) != 0) {
                continue;
            }
            if (!enemiesParams.empty() && GetEnemySpawnParams(local) != enemiesParams[ri]) {
                continue;
            }
            float dx = local->world.pos.x - remotePos.x;
            float dy = local->world.pos.y - remotePos.y;
            float dz = local->world.pos.z - remotePos.z;
            float distance = dx * dx + dy * dy + dz * dz;
            if (distance < closestDist) {
                closestDist = distance;
                closestActor = local;
            }
        }

        if (closestActor == nullptr && enemiesId[ri] == ACTOR_EN_HINTNUTS) {
            closestActor = FindClosestUnassignedActorByCategoryAndId(category, enemiesId[ri], remotePos, 100000.0f,
                                                                    enemiesParams.empty() ? (s16)-0x8000 : enemiesParams[ri],
                                                                    authorityRoomNum);
        }

        if (closestActor != nullptr) {
            SetEnemyNetworkId(closestActor, enemiesNetworkId[ri]);
        }
    }

    if (!HasEnemySyncAuthority()) {
        for (Actor* local : localActors) {
            if (local == nullptr || local->update == nullptr || !IsEnemySyncActor(local) || GetEnemyNetworkId(local) != 0) {
                continue;
            }

            bool alreadyBuffered = false;
            for (auto& [buffered, sceneNum, roomNum] : enemyPruneBuffer) {
                if (buffered == local && sceneNum == gPlayState->sceneNum && roomNum == gPlayState->roomCtx.curRoom.num) {
                    alreadyBuffered = true;
                    break;
                }
            }
            if (!alreadyBuffered) {
                enemyPruneBuffer.push_back({ local, gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num });
            }
        }
    }
}
