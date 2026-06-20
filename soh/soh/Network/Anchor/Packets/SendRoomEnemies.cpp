#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "macros.h"
#include "functions.h"
extern PlayState* gPlayState;
}

static bool IsDungeonScene(s16 sceneNum) {
    return sceneNum == SCENE_DEKU_TREE || sceneNum == SCENE_DODONGOS_CAVERN ||
           sceneNum == SCENE_JABU_JABU || sceneNum == SCENE_FOREST_TEMPLE ||
           sceneNum == SCENE_FIRE_TEMPLE || sceneNum == SCENE_WATER_TEMPLE ||
           sceneNum == SCENE_SPIRIT_TEMPLE || sceneNum == SCENE_SHADOW_TEMPLE ||
           sceneNum == SCENE_BOTTOM_OF_THE_WELL || sceneNum == SCENE_ICE_CAVERN ||
           sceneNum == SCENE_GERUDO_TRAINING_GROUND || sceneNum == SCENE_INSIDE_GANONS_CASTLE ||
           sceneNum == SCENE_GANONS_TOWER || sceneNum == SCENE_GANONS_TOWER_COLLAPSE_INTERIOR ||
           sceneNum == SCENE_GANONS_TOWER_COLLAPSE_EXTERIOR;
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
        if (IsEnemySyncActor(currAct)) {
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
        enemiesParams.push_back(currAct->params);
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
    payload["category"] = category;
    payload["enemiesNetworkId"] = enemiesNetworkId;
    payload["deadEnemiesNetworkId"] = std::vector<uint64_t>(deadEnemyLedger[GetEnemyRoomKey(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num)].begin(),
                                                             deadEnemyLedger[GetEnemyRoomKey(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num)].end());
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

    ActorCategory category = (ActorCategory)payload.at("category").get<s16>();
    s8 authorityRoomNum = payload.value("roomNum", (s8)-1);
    auto enemiesNetworkId = payload.value("enemiesNetworkId", std::vector<uint64_t>{});
    auto deadEnemiesNetworkId = payload.value("deadEnemiesNetworkId", std::vector<uint64_t>{});
    auto enemiesId = payload.at("enemiesId").get<std::vector<s16>>();
    auto enemiesX = payload.at("enemiesX").get<std::vector<float>>();
    auto enemiesY = payload.at("enemiesY").get<std::vector<float>>();
    auto enemiesZ = payload.at("enemiesZ").get<std::vector<float>>();
    auto enemiesParams = payload.value("enemiesParams", std::vector<s16>{});

    if (!enemiesNetworkId.empty() && enemiesNetworkId.size() != enemiesId.size()) {
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
        localActors.push_back(currAct);
        currAct = currAct->next;
    }

    for (size_t ri = 0; ri < enemiesId.size(); ri++) {
        if (enemiesNetworkId.empty() || enemiesNetworkId[ri] == 0) {
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
        float closestDist = -1.0f;
        Actor* closestActor = nullptr;

        for (size_t li = 0; li < localActors.size(); li++) {
            Actor* local = localActors[li];
            if (local->id != enemiesId[ri] || !IsEnemySyncActor(local)) {
                continue;
            }
            if (IsEnemyMarkedDead(GetEnemyNetworkId(local))) {
                continue;
            }
            if (GetEnemyNetworkId(local) != 0) {
                continue;
            }
            if (!enemiesParams.empty() && local->params != enemiesParams[ri]) {
                continue;
            }
            float dx = local->world.pos.x - remotePos.x;
            float dy = local->world.pos.y - remotePos.y;
            float dz = local->world.pos.z - remotePos.z;
            float distance = dx * dx + dy * dy + dz * dz;
            if (closestDist < 0.0f || distance < closestDist) {
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

    // Only prune unmatched enemies in dungeons where rooms load one at a time.
    // In overworld areas, enemies from adjacent rooms may be loaded but not yet
    // reported by the authority — pruning them would make them vanish.
    if (!HasEnemySyncAuthority() && IsDungeonScene(gPlayState->sceneNum)) {
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
