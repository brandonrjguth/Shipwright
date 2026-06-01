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

    if (actor->category != ACTORCAT_ENEMY && actor->category != ACTORCAT_BOSS) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = KILL_ENEMY;
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

    uint32_t clientId = payload.at("clientId").get<uint32_t>();
    if (!clients.contains(clientId)) {
        return;
    }

    AnchorClient& client = clients[clientId];
    if (client.sceneNum != gPlayState->sceneNum || client.curRoomNum != gPlayState->roomCtx.curRoom.num) {
        return;
    }

    s16 actorId = payload.at("actorId").get<s16>();
    float posX = payload.at("posX").get<float>();
    float posY = payload.at("posY").get<float>();
    float posZ = payload.at("posZ").get<float>();
    s16 category = payload.at("category").get<s16>();

    Vec3f pos = { posX, posY, posZ };
    Actor* target = FindClosestActorByCategoryAndId((ActorCategory)category, actorId, pos);

    if (target != nullptr) {
        actorKillBuffer.push_back(target);
    }
}
