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
    if (!IsSaveLoaded()) {
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

    SendPacket_SendRoomEnemies(clientId, ACTORCAT_ENEMY);
    SendPacket_SendRoomEnemies(clientId, ACTORCAT_BOSS);
}
