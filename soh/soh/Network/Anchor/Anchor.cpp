#include "Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"
#include "soh/Enhancements/nametag.h"
#include "soh/ObjectExtension/ObjectExtension.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include <unordered_set>

extern "C" {
#include "variables.h"
#include "functions.h"
extern PlayState* gPlayState;
}

// MARK: - Overrides

void Anchor::Enable() {
    Network::Enable(CVarGetString(CVAR_REMOTE_ANCHOR("Host"), "anchor.hm64.org"),
                    CVarGetInteger(CVAR_REMOTE_ANCHOR("Port"), 43383));
    ownClientId = CVarGetInteger(CVAR_REMOTE_ANCHOR("LastClientId"), 0);
    roomState.ownerClientId = 0;
}

void Anchor::Disable() {
    Network::Disable();

    clients.clear();
    RefreshClientActors();
}

void Anchor::OnConnected() {
    SendPacket_Handshake();
    RegisterHooks();

    if (IsSaveLoaded()) {
        SendPacket_RequestTeamState();
    }
}

void Anchor::OnDisconnected() {
    RegisterHooks();
}

void Anchor::ProcessOutgoingPackets() {
    // Copy all queued packets while holding the lock, then send them after releasing
    std::queue<nlohmann::json> packetsToSend;
    {
        std::lock_guard<std::mutex> lock(outgoingPacketQueueMutex);
        packetsToSend.swap(outgoingPacketQueue);
    }

    // Send packets without holding the lock
    while (!packetsToSend.empty()) {
        nlohmann::json payload = packetsToSend.front();
        packetsToSend.pop();

        if (!payload.contains("quiet")) {
            SPDLOG_DEBUG("[Anchor] Sending payload:\n{}", payload.dump());
        }
        Network::SendJsonToRemote(payload);
    }
}

void Anchor::SendJsonToRemote(nlohmann::json payload) {
    if (!isConnected) {
        return;
    }

    payload["clientId"] = ownClientId;
    if (!payload.contains("quiet")) {
        SPDLOG_DEBUG("[Anchor] Queuing payload:\n{}", payload.dump());
    }

    if (payload["type"] == HANDSHAKE) {
        Network::SendJsonToRemote(payload);
        return;
    }

    // Queue the packet to be sent on the network thread
    std::lock_guard<std::mutex> lock(outgoingPacketQueueMutex);
    outgoingPacketQueue.push(payload);
}

void Anchor::OnIncomingJson(nlohmann::json payload) {
    // If it doesn't contain a type, it's not a valid payload
    if (!payload.contains("type")) {
        return;
    }

    // If it's not a quiet payload, log it
    if (!payload.contains("quiet")) {
        SPDLOG_DEBUG("[Anchor] Received payload:\n{}", payload.dump());
    }

    std::string packetType = payload["type"].get<std::string>();

    // Ignore packets from mismatched clients, except for ALL_CLIENT_STATE, UPDATE_CLIENT_STATE, and PLAYER_UPDATE
    if (packetType != ALL_CLIENT_STATE && packetType != UPDATE_CLIENT_STATE && packetType != PLAYER_UPDATE) {
        if (payload.contains("clientId")) {
            uint32_t clientId = payload["clientId"].get<uint32_t>();
            if (clients.contains(clientId) && clients[clientId].clientVersion != clientVersion) {
                return;
            }
        }
    }

    // Queue all packets to be processed on the game thread
    std::lock_guard<std::mutex> lock(incomingPacketQueueMutex);
    incomingPacketQueue.push(payload);
}

void Anchor::ProcessIncomingPacketQueue() {
    // Copy all queued packets while holding the lock, then process them after releasing
    std::queue<nlohmann::json> packetsToProcess;
    {
        std::lock_guard<std::mutex> lock(incomingPacketQueueMutex);
        packetsToProcess.swap(incomingPacketQueue);
    }

    // Process packets without holding the lock
    while (!packetsToProcess.empty()) {
        nlohmann::json payload = packetsToProcess.front();
        packetsToProcess.pop();

        std::string packetType = payload["type"].get<std::string>();

        isProcessingIncomingPacket = true;

        try {
            // packetType here is a string so we can't use a switch statement
            if (packetType == ALL_CLIENT_STATE)
                HandlePacket_AllClientState(payload);
            else if (packetType == DAMAGE_PLAYER)
                HandlePacket_DamagePlayer(payload);
            else if (packetType == DISABLE_ANCHOR)
                HandlePacket_DisableAnchor(payload);
            else if (packetType == ENTRANCE_DISCOVERED)
                HandlePacket_EntranceDiscovered(payload);
            else if (packetType == GAME_COMPLETE)
                HandlePacket_GameComplete(payload);
            else if (packetType == GIVE_ITEM)
                HandlePacket_GiveItem(payload);
            else if (packetType == OCARINA_SFX)
                HandlePacket_OcarinaSfx(payload);
            else if (packetType == PLAYER_UPDATE)
                HandlePacket_PlayerUpdate(payload);
            else if (packetType == PLAYER_SFX)
                HandlePacket_PlayerSfx(payload);
            else if (packetType == UPDATE_TEAM_STATE)
                HandlePacket_UpdateTeamState(payload);
            else if (packetType == REQUEST_TEAM_STATE)
                HandlePacket_RequestTeamState(payload);
            else if (packetType == REQUEST_TELEPORT)
                HandlePacket_RequestTeleport(payload);
            else if (packetType == SERVER_MESSAGE)
                HandlePacket_ServerMessage(payload);
            else if (packetType == SET_CHECK_STATUS)
                HandlePacket_SetCheckStatus(payload);
            else if (packetType == SET_FLAG)
                HandlePacket_SetFlag(payload);
            else if (packetType == TELEPORT_TO)
                HandlePacket_TeleportTo(payload);
            else if (packetType == UNSET_FLAG)
                HandlePacket_UnsetFlag(payload);
            else if (packetType == UPDATE_BEANS_COUNT)
                HandlePacket_UpdateBeansCount(payload);
            else if (packetType == UPDATE_CLIENT_STATE)
                HandlePacket_UpdateClientState(payload);
            else if (packetType == UPDATE_ROOM_STATE)
                HandlePacket_UpdateRoomState(payload);
            else if (packetType == UPDATE_DUNGEON_ITEMS)
                HandlePacket_UpdateDungeonItems(payload);
            else if (packetType == DAMAGE_ENEMY)
                HandlePacket_DamageEnemy(payload);
            else if (packetType == KILL_ENEMY)
                HandlePacket_KillEnemy(payload);
            else if (packetType == REQUEST_ROOM_ENEMIES)
                HandlePacket_RequestRoomEnemies(payload);
            else if (packetType == SEND_ROOM_ENEMIES)
                HandlePacket_SendRoomEnemies(payload);
            else if (packetType == ENEMY_UPDATE)
                HandlePacket_EnemyUpdate(payload);
            else if (packetType == REPORT_ENEMY_DAMAGE)
                HandlePacket_ReportEnemyDamage(payload);
        } catch (const std::exception& e) {
            SPDLOG_ERROR("[Anchor] Exception while processing incoming packet {}", e.what());
            SPDLOG_ERROR("[Anchor] Packet: {}", payload.dump());
        }

        isProcessingIncomingPacket = false;
    }
}

// MARK: - Misc/Helpers

// Kills all existing anchor actors and respawns them with the new client data

struct DummyPlayerClientId {
    uint32_t clientId = 0;
};
static ObjectExtension::Register<DummyPlayerClientId> DummyPlayerClientIdRegister;

struct EnemyNetworkId {
    uint64_t networkId = 0;
};
static ObjectExtension::Register<EnemyNetworkId> EnemyNetworkIdRegister;

uint32_t Anchor::GetDummyPlayerClientId(const Actor* actor) {
    const DummyPlayerClientId* clientId = ObjectExtension::GetInstance().Get<DummyPlayerClientId>(actor);
    return clientId != nullptr ? clientId->clientId : 0;
}

void Anchor::SetDummyPlayerClientId(const Actor* actor, uint32_t clientId) {
    ObjectExtension::GetInstance().Set<DummyPlayerClientId>(actor, DummyPlayerClientId{ clientId });
}

void Anchor::RefreshClientActors() {
    if (!IsSaveLoaded()) {
        return;
    }

    Actor* actor = gPlayState->actorCtx.actorLists[ACTORCAT_NPC].head;

    while (actor != NULL) {
        if (actor->id == ACTOR_EN_OE2 && actor->update == DummyPlayer_Update) {
            NameTag_RemoveAllForActor(actor);
            Actor_Kill(actor);
        }
        actor = actor->next;
    }

    for (auto& [clientId, client] : clients) {
        if (!client.online || client.self) {
            continue;
        }

        spawningDummyPlayerForClientId = clientId;
        // We are using a hook `ShouldActorInit` to override the init/update/draw/destroy functions of the Player we
        // spawn We quickly store a mapping of "index" to clientId, then within the init function we use this to get the
        // clientId and store it on player->zTargetActiveTimer (unused s32 for the dummy) for convenience
        auto dummy =
            Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_PLAYER, client.posRot.pos.x, client.posRot.pos.y,
                        client.posRot.pos.z, client.posRot.rot.x, client.posRot.rot.y, client.posRot.rot.z, 0);
        client.player = (Player*)dummy;
    }
    spawningDummyPlayerForClientId = 0;
}

bool Anchor::IsSaveLoaded() {
    if (gPlayState == nullptr) {
        return false;
    }

    if (GET_PLAYER(gPlayState) == nullptr) {
        return false;
    }

    if (gSaveContext.fileNum < 0 || gSaveContext.fileNum > 2) {
        return false;
    }

    if (gSaveContext.gameMode != GAMEMODE_NORMAL) {
        return false;
    }

    return true;
}

Actor* Anchor::FindClosestActorByCategoryAndId(ActorCategory category, s16 actorId, Vec3f pos) {
    if (gPlayState == nullptr) {
        return nullptr;
    }

    Actor* currAct = gPlayState->actorCtx.actorLists[category].head;
    Actor* closestAct = nullptr;
    float closestDist = -1.0f;

    while (currAct != nullptr) {
        if (currAct->id == actorId) {
            float dx = currAct->world.pos.x - pos.x;
            float dy = currAct->world.pos.y - pos.y;
            float dz = currAct->world.pos.z - pos.z;
            float distance = dx * dx + dy * dy + dz * dz;
            if (closestDist < 0.0f || distance < closestDist) {
                closestAct = currAct;
                closestDist = distance;
            }
        }
        currAct = currAct->next;
    }

    return closestAct;
}

uint64_t Anchor::GetEnemyNetworkId(Actor* actor) {
    if (actor == nullptr) {
        return 0;
    }

    const EnemyNetworkId* networkId = ObjectExtension::GetInstance().Get<EnemyNetworkId>(actor);
    return networkId != nullptr ? networkId->networkId : 0;
}

void Anchor::SetEnemyNetworkId(Actor* actor, uint64_t networkId) {
    if (actor != nullptr && networkId != 0) {
        ObjectExtension::GetInstance().Set<EnemyNetworkId>(actor, EnemyNetworkId{ networkId });
    }
}

Actor* Anchor::FindActorByEnemyNetworkId(uint64_t networkId) {
    if (networkId == 0 || gPlayState == nullptr) {
        return nullptr;
    }

    ActorCategory categories[] = { ACTORCAT_ENEMY, ACTORCAT_BOSS };
    for (ActorCategory category : categories) {
        Actor* currAct = gPlayState->actorCtx.actorLists[category].head;
        while (currAct != nullptr) {
            if (GetEnemyNetworkId(currAct) == networkId) {
                return currAct;
            }
            currAct = currAct->next;
        }
    }

    return nullptr;
}

void Anchor::AssignEnemyNetworkIds(std::vector<Actor*> actors) {
    if (!IsSaveLoaded()) {
        return;
    }

    std::unordered_set<uint64_t> usedNetworkIds;
    std::unordered_map<uint32_t, uint16_t> nextOccurrence;

    for (Actor* actor : actors) {
        uint64_t existingNetworkId = GetEnemyNetworkId(actor);
        if (existingNetworkId != 0) {
            usedNetworkIds.insert(existingNetworkId);
        }
    }

    for (Actor* actor : actors) {
        if (actor == nullptr || GetEnemyNetworkId(actor) != 0) {
            continue;
        }

        uint32_t counterKey = ((uint32_t)actor->category << 16) | (uint16_t)actor->id;
        uint16_t occurrence = nextOccurrence[counterKey];
        uint64_t networkId = 0;
        do {
            networkId = ((uint64_t)(uint16_t)gPlayState->sceneNum << 48) |
                        ((uint64_t)(uint8_t)gPlayState->roomCtx.curRoom.num << 40) |
                        ((uint64_t)actor->category << 32) | ((uint64_t)(uint16_t)actor->id << 16) | occurrence;
            occurrence++;
        } while (usedNetworkIds.contains(networkId));

        nextOccurrence[counterKey] = occurrence;
        usedNetworkIds.insert(networkId);
        ObjectExtension::GetInstance().Set<EnemyNetworkId>(actor, EnemyNetworkId{ networkId });
    }
}

static float AnchorLerpFloat(float from, float to, float amount) {
    return from + ((to - from) * amount);
}

static s16 AnchorLerpAngle(s16 from, s16 to, float amount) {
    s16 diff = to - from;
    return from + (s16)(diff * amount);
}

static Vec3f AnchorLerpVec3f(Vec3f from, Vec3f to, float amount) {
    return { AnchorLerpFloat(from.x, to.x, amount), AnchorLerpFloat(from.y, to.y, amount),
             AnchorLerpFloat(from.z, to.z, amount) };
}

static float AnchorVec3fDistSq(Vec3f a, Vec3f b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return (dx * dx) + (dy * dy) + (dz * dz);
}

void Anchor::ApplyEnemyAuthorityState(Actor* actor, EnemyAuthorityState state, bool immediate) {
    if (actor == nullptr) {
        return;
    }

    float distSq = AnchorVec3fDistSq(actor->world.pos, state.pos);
    float correction = 0.2f;
    if (immediate || distSq > 250000.0f) {
        correction = 1.0f;
    } else if (distSq > 40000.0f) {
        correction = 0.75f;
    } else if (distSq > 10000.0f) {
        correction = 0.5f;
    }

    Vec3f correctedPos = AnchorLerpVec3f(actor->world.pos, state.pos, correction);
    actor->world.pos = correctedPos;
    actor->prevPos = AnchorLerpVec3f(actor->prevPos, correctedPos, correction);
    actor->world.rot.x = AnchorLerpAngle(actor->world.rot.x, state.worldRot.x, correction);
    actor->world.rot.y = AnchorLerpAngle(actor->world.rot.y, state.worldRot.y, correction);
    actor->world.rot.z = AnchorLerpAngle(actor->world.rot.z, state.worldRot.z, correction);
    actor->shape.rot.x = AnchorLerpAngle(actor->shape.rot.x, state.shapeRot.x, correction);
    actor->shape.rot.y = AnchorLerpAngle(actor->shape.rot.y, state.shapeRot.y, correction);
    actor->shape.rot.z = AnchorLerpAngle(actor->shape.rot.z, state.shapeRot.z, correction);
    actor->velocity = AnchorLerpVec3f(actor->velocity, state.velocity, correction);
    actor->speedXZ = AnchorLerpFloat(actor->speedXZ, state.speedXZ, correction);
    actor->gravity = state.gravity;
    actor->minVelocityY = state.minVelocityY;
    actor->freezeTimer = state.freezeTimer;
    actor->colorFilterTimer = state.colorFilterTimer;
    actor->colChkInfo.health = state.health > 0 ? state.health : 1;
    enemyHealthTracker[actor] = actor->colChkInfo.health;
}

void Anchor::ApplyEnemyAuthorityTargets() {
    if (!IsSaveLoaded() || HasEnemySyncAuthority()) {
        return;
    }

    for (auto it = enemyAuthorityTargets.begin(); it != enemyAuthorityTargets.end();) {
        Actor* actor = FindActorByEnemyNetworkId(it->first);
        if (actor == nullptr) {
            it = enemyAuthorityTargets.erase(it);
            continue;
        }
        ApplyEnemyAuthorityState(actor, it->second, false);
        ++it;
    }
}

uint32_t Anchor::GetEnemySyncAuthorityClientId() {
    if (!IsSaveLoaded() || ownClientId == 0) {
        return 0;
    }

    uint32_t authorityClientId = ownClientId;
    for (auto& [clientId, client] : clients) {
        if (!client.online || client.self || !client.isSaveLoaded) {
            continue;
        }
        if (client.sceneNum == gPlayState->sceneNum && client.curRoomNum == gPlayState->roomCtx.curRoom.num &&
            clientId < authorityClientId) {
            authorityClientId = clientId;
        }
    }

    return authorityClientId;
}

bool Anchor::HasEnemySyncAuthority() {
    return GetEnemySyncAuthorityClientId() == ownClientId;
}

void Anchor::ProcessActorBuffers() {
    if (!IsSaveLoaded()) {
        return;
    }

    while (!actorKillBuffer.empty()) {
        Actor* actor = actorKillBuffer.front();
        actorKillBuffer.erase(actorKillBuffer.begin());
        if (actor != nullptr) {
            Actor_Kill(actor);
        }
    }

    while (!enemySpawnBuffer.empty()) {
        auto& [actorId, params, pos] = enemySpawnBuffer.front();
        Actor* spawned = Actor_Spawn(&gPlayState->actorCtx, gPlayState, actorId,
                                      pos.x, pos.y, pos.z, 0, 0, 0, params);
        enemySpawnBuffer.erase(enemySpawnBuffer.begin());
    }
}

void Anchor::DetectEnemyDamage() {
    if (!IsSaveLoaded()) {
        return;
    }

    std::vector<Actor*> currentEnemies;

    ActorCategory categories[] = { ACTORCAT_ENEMY, ACTORCAT_BOSS };
    for (ActorCategory cat : categories) {
        Actor* currAct = gPlayState->actorCtx.actorLists[cat].head;
        while (currAct != nullptr) {
            currentEnemies.push_back(currAct);
            currAct = currAct->next;
        }
    }

    AssignEnemyNetworkIds(currentEnemies);
    ApplyEnemyAuthorityTargets();

    std::unordered_map<Actor*, bool> stillAlive;
    for (Actor* act : currentEnemies) {
        stillAlive[act] = true;
    }

    for (auto it = enemyHealthTracker.begin(); it != enemyHealthTracker.end();) {
        if (!stillAlive.contains(it->first)) {
            it = enemyHealthTracker.erase(it);
        } else {
            ++it;
        }
    }

    for (Actor* act : currentEnemies) {
        u8 currentHealth = act->colChkInfo.health;

        if (enemyHealthTracker.contains(act)) {
            u8 lastHealth = enemyHealthTracker[act];
            if (currentHealth < lastHealth) {
                if (HasEnemySyncAuthority()) {
                    SendPacket_DamageEnemy(act, currentHealth);
                } else {
                    SendPacket_ReportEnemyDamage(act, currentHealth);
                }
            }
        }

        enemyHealthTracker[act] = currentHealth;
    }

    enemyTransformFrameCounter++;
    if (enemyTransformFrameCounter >= 2) {
        enemyTransformFrameCounter = 0;
        SendPacket_EnemyUpdate(currentEnemies);
    }
}
