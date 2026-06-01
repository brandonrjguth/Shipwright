#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "macros.h"
#include "functions.h"
extern PlayState* gPlayState;
}

void Anchor::SendPacket_DamageEnemy(Actor* actor, u8 health) {
    if (!IsSaveLoaded()) {
        return;
    }

    if (actor->category != ACTORCAT_ENEMY && actor->category != ACTORCAT_BOSS) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = DAMAGE_ENEMY;
    payload["actorId"] = actor->id;
    payload["health"] = health;
    payload["posX"] = actor->world.pos.x;
    payload["posY"] = actor->world.pos.y;
    payload["posZ"] = actor->world.pos.z;
    payload["category"] = actor->category;
    payload["quiet"] = true;

    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_DamageEnemy(nlohmann::json payload) {
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
    u8 health = payload.at("health").get<u8>();
    float posX = payload.at("posX").get<float>();
    float posY = payload.at("posY").get<float>();
    float posZ = payload.at("posZ").get<float>();
    s16 category = payload.at("category").get<s16>();

    Vec3f pos = { posX, posY, posZ };
    Actor* target = FindClosestActorByCategoryAndId((ActorCategory)category, actorId, pos);

    if (target != nullptr) {
        if (health > 0) {
            target->colChkInfo.health = health;
        } else {
            target->colChkInfo.health = 1;
        }
        enemyHealthTracker[target] = target->colChkInfo.health;
    }
}
