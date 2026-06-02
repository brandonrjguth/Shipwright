#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "macros.h"
#include "functions.h"
extern PlayState* gPlayState;
}

void Anchor::SendPacket_RequestRoomEnemies() {
    if (!IsRoomStable()) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = REQUEST_ROOM_ENEMIES;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["roomNum"] = gPlayState->roomCtx.curRoom.num;
    payload["quiet"] = true;

    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_RequestRoomEnemies(nlohmann::json payload) {
    if (!IsRoomStable() || !HasEnemySyncAuthority()) {
        return;
    }

    uint32_t clientId = payload.at("clientId").get<uint32_t>();
    if (!clients.contains(clientId)) {
        return;
    }

    s16 sceneNum = payload.value("sceneNum", (s16)SCENE_ID_MAX);
    s8 roomNum = payload.value("roomNum", (s8)-1);
    if (sceneNum != gPlayState->sceneNum || roomNum != gPlayState->roomCtx.curRoom.num) {
        return;
    }

    SendPacket_SendRoomEnemies(clientId, ACTORCAT_ENEMY);
    SendPacket_SendRoomEnemies(clientId, ACTORCAT_BOSS);
}
