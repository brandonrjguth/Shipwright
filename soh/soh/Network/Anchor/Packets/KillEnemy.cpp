#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "macros.h"
#include "functions.h"
extern PlayState* gPlayState;
}

void Anchor::SendPacket_KillEnemy(Actor* actor) {
    if (!IsSaveLoaded()) {
        return;
    }

    uint64_t networkId = GetEnemyNetworkId(actor);
    if (networkId == 0) {
        return;
    }

    MarkEnemyDead(networkId);

    nlohmann::json payload;
    payload["type"] = KILL_ENEMY;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["roomNum"] = gPlayState->roomCtx.curRoom.num;
    payload["authorityClientId"] = ownClientId;
    payload["authorityGeneration"] = GetEnemyRoomAuthorityGeneration(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num);
    payload["networkId"] = networkId;
    payload["actorId"] = actor->id;
    payload["posX"] = actor->world.pos.x;
    payload["posY"] = actor->world.pos.y;
    payload["posZ"] = actor->world.pos.z;
    payload["category"] = actor->category;
    payload["quiet"] = true;

    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_KillEnemy(nlohmann::json payload) {
    if (!IsSaveLoaded()) {
        return;
    }

    if (!IsValidEnemyAuthorityPacket(payload)) {
        return;
    }

    uint64_t networkId = payload.value("networkId", (uint64_t)0);
    s16 sceneNum = payload.value("sceneNum", (s16)SCENE_ID_MAX);
    s8 roomNum = payload.value("roomNum", (s8)-1);
    if (networkId == 0) {
        return;
    }

    if (IsEnemyMarkedDead(sceneNum, roomNum, networkId)) {
        return;
    }

    MarkEnemyDead(sceneNum, roomNum, networkId);

    Actor* target = FindActorByEnemyNetworkId(networkId);
    if (target != nullptr) {
        enemyKillBuffer.push_back(networkId);
    }
}
