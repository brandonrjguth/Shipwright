#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "macros.h"
#include "functions.h"
extern PlayState* gPlayState;
void Player_DetachHeldActor(PlayState* play, Player* thisx);
}

void Anchor::SendPacket_KillEnemy(Actor* actor) {
    if (!IsSaveLoaded()) {
        return;
    }

    uint64_t networkId = GetEnemyNetworkId(actor);
    if (networkId == 0) {
        return;
    }

    SendPacket_KillEnemy({
        { "sceneNum", gPlayState->sceneNum },
        { "roomNum", gPlayState->roomCtx.curRoom.num },
        { "networkId", networkId },
        { "actorId", actor->id },
        { "actorParams", GetEnemySpawnParams(actor) },
        { "posX", actor->world.pos.x },
        { "posY", actor->world.pos.y },
        { "posZ", actor->world.pos.z },
        { "category", actor->category },
    });
}

void Anchor::SendPacket_KillEnemy(const nlohmann::json& actorContext) {
    if (!IsSaveLoaded()) {
        return;
    }

    uint64_t networkId = actorContext.value("networkId", (uint64_t)0);
    s16 actorId = actorContext.value("actorId", (s16)-1);
    s16 actorParams = actorContext.value("actorParams", (s16)0);
    s16 sceneNum = actorContext.value("sceneNum", gPlayState->sceneNum);
    s8 roomNum = actorContext.value("roomNum", gPlayState->roomCtx.curRoom.num);
    if (networkId == 0 || sceneNum != gPlayState->sceneNum || roomNum != gPlayState->roomCtx.curRoom.num ||
        !HasEnemySyncAuthority(sceneNum, roomNum)) {
        return;
    }
    if (!IsTransientProjectileActor(actorId, actorParams)) {
        MarkEnemyDead(sceneNum, roomNum, networkId);
    }

    nlohmann::json payload = actorContext;
    payload["type"] = KILL_ENEMY;
    payload["authorityClientId"] = ownClientId;
    payload["authorityGeneration"] = GetEnemyRoomAuthorityGeneration(sceneNum, roomNum);
    payload["enemySessionId"] = enemySessionId;
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

    s16 actorId = payload.value("actorId", (s16)0);
    s16 actorParams = payload.value("actorParams", (s16)0);
    if (IsTransientProjectileActor(actorId, actorParams)) {
        // Fire-and-forget: the local simulation owns the projectile's whole flight and decides the impact itself
        // (wall, shield, or the local player's true position). Applying the authority's kill here deleted the
        // projectile mid-air, because the authority collides with latency-delayed positions.
        return;
    }

    if (IsEnemyMarkedDead(sceneNum, roomNum, networkId)) {
        return;
    }

    Actor* target = FindActorByEnemyNetworkId(networkId);
    if (target != nullptr) {
        Player* player = GET_PLAYER(gPlayState);
        if (player->heldActor == target) {
            Player_DetachHeldActor(gPlayState, player);
        } else if (target->parent == target) {
            target->parent = nullptr;
        }
        target->colChkInfo.health = 0;
        QueueEnemyKill(networkId);
    }
    MarkEnemyDead(sceneNum, roomNum, networkId);
}
