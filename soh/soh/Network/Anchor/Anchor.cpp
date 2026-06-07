#include "Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/OTRGlobals.h"
#include "soh/Enhancements/nametag.h"
#include "soh/ObjectExtension/ObjectExtension.h"
#include "soh/Enhancements/randomizer/randomizer.h"

extern "C" {
#include "variables.h"
#include "functions.h"
extern PlayState* gPlayState;
}

extern void ApplyEnemyExtraState(Actor* actor, nlohmann::json extra);
extern bool ShouldReportEnemyExtraState(Actor* actor);
extern bool ShouldPreserveLocalEnemyExtraState(Actor* actor, nlohmann::json authorityExtra);

static bool AnchorIsActorInCurrentLists(Actor* actor);

extern "C" bool Anchor_GetNearestEnemyTargetPos(Actor* actor, Vec3f* outPos) {
    if (Anchor::Instance == nullptr || actor == nullptr || outPos == nullptr || !Anchor::Instance->IsSaveLoaded()) {
        return false;
    }

    Player* player = GET_PLAYER(gPlayState);
    if (player == nullptr) {
        return false;
    }

    Vec3f bestPos = player->actor.world.pos;
    f32 dx = actor->world.pos.x - bestPos.x;
    f32 dy = actor->world.pos.y - bestPos.y;
    f32 dz = actor->world.pos.z - bestPos.z;
    f32 bestDistSq = SQ(dx) + SQ(dy) + SQ(dz);

    for (auto& [clientId, client] : Anchor::Instance->clients) {
        if (!client.online || client.self || !client.isSaveLoaded) {
            continue;
        }
        if (client.sceneNum != gPlayState->sceneNum || client.curRoomNum != gPlayState->roomCtx.curRoom.num) {
            continue;
        }

        dx = actor->world.pos.x - client.posRot.pos.x;
        dy = actor->world.pos.y - client.posRot.pos.y;
        dz = actor->world.pos.z - client.posRot.pos.z;
        f32 distSq = SQ(dx) + SQ(dy) + SQ(dz);
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            bestPos = client.posRot.pos;
        }
    }

    *outPos = bestPos;
    return true;
}

extern "C" Player* Anchor_GetNearestEnemyTargetPlayer(Actor* actor) {
    if (Anchor::Instance == nullptr || actor == nullptr || !Anchor::Instance->IsSaveLoaded()) {
        return nullptr;
    }

    Player* player = GET_PLAYER(gPlayState);
    if (player == nullptr) {
        return nullptr;
    }

    f32 dx = actor->world.pos.x - player->actor.world.pos.x;
    f32 dy = actor->world.pos.y - player->actor.world.pos.y;
    f32 dz = actor->world.pos.z - player->actor.world.pos.z;
    f32 bestDistSq = SQ(dx) + SQ(dy) + SQ(dz);
    Player* bestPlayer = player;

    for (auto& [clientId, client] : Anchor::Instance->clients) {
        if (!client.online || client.self || !client.isSaveLoaded || client.player == nullptr) {
            continue;
        }
        Actor* clientActor = &client.player->actor;
        if (!AnchorIsActorInCurrentLists(clientActor) || clientActor->id != ACTOR_EN_OE2 ||
            clientActor->update != DummyPlayer_Update) {
            continue;
        }
        if (client.sceneNum != gPlayState->sceneNum || client.curRoomNum != gPlayState->roomCtx.curRoom.num) {
            continue;
        }

        dx = actor->world.pos.x - client.posRot.pos.x;
        dy = actor->world.pos.y - client.posRot.pos.y;
        dz = actor->world.pos.z - client.posRot.pos.z;
        f32 distSq = SQ(dx) + SQ(dy) + SQ(dz);
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            bestPlayer = client.player;
        }
    }

    return bestPlayer;
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
            else if (packetType == ENEMY_EVENT)
                HandlePacket_EnemyEvent(payload);
            else if (packetType == REPORT_ENEMY_DAMAGE)
                HandlePacket_ReportEnemyDamage(payload);
            else if (packetType == HINTNUTS_DIALOGUE)
                HandlePacket_HintnutsDialogue(payload);
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
    for (auto& [clientId, client] : clients) {
        client.player = nullptr;
    }

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
        if (currAct->id == actorId && !IsEnemyMarkedDead(GetEnemyNetworkId(currAct))) {
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

Actor* Anchor::FindClosestUnassignedActorByCategoryAndId(ActorCategory category, s16 actorId, Vec3f pos, float maxDistSq,
                                                         s16 actorParams) {
    if (gPlayState == nullptr) {
        return nullptr;
    }

    Actor* closestAct = nullptr;
    float closestDist = maxDistSq;
    s32 startCategory = actorId == ACTOR_EN_HINTNUTS ? ACTORCAT_SWITCH : category;
    s32 endCategory = actorId == ACTOR_EN_HINTNUTS ? ACTORCAT_MAX : category + 1;

    for (s32 currCategory = startCategory; currCategory < endCategory; currCategory++) {
        Actor* currAct = gPlayState->actorCtx.actorLists[currCategory].head;
        while (currAct != nullptr) {
            if (currAct->id != actorId || !IsEnemySyncActor(currAct)) {
                currAct = currAct->next;
                continue;
            }
            if (actorParams != (s16)-0x8000 && currAct->params != actorParams) {
                currAct = currAct->next;
                continue;
            }
            if (GetEnemyNetworkId(currAct) != 0) {
                currAct = currAct->next;
                continue;
            }

            float dx = currAct->world.pos.x - pos.x;
            float dy = currAct->world.pos.y - pos.y;
            float dz = currAct->world.pos.z - pos.z;
            float distance = dx * dx + dy * dy + dz * dz;
            if (distance < closestDist) {
                closestAct = currAct;
                closestDist = distance;
            }

            currAct = currAct->next;
        }
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
    if (actor == nullptr || networkId == 0) {
        return;
    }
    uint64_t existingId = GetEnemyNetworkId(actor);
    if (existingId != 0 && existingId != networkId) {
        SPDLOG_WARN("[Anchor] Refusing to overwrite actor {:x} networkId {} with {}",
                     (uintptr_t)actor, existingId, networkId);
        return;
    }
    ObjectExtension::GetInstance().Set<EnemyNetworkId>(actor, EnemyNetworkId{ networkId });
}

Actor* Anchor::FindActorByEnemyNetworkId(uint64_t networkId) {
    if (networkId == 0 || gPlayState == nullptr) {
        return nullptr;
    }

    for (s32 category = ACTORCAT_SWITCH; category < ACTORCAT_MAX; category++) {
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

bool Anchor::IsEnemySyncActor(ActorCategory category, s16 actorId) {
    return category == ACTORCAT_ENEMY || category == ACTORCAT_BOSS || actorId == ACTOR_EN_HINTNUTS ||
           actorId == ACTOR_EN_SW || actorId == ACTOR_EN_NUTSBALL ||
           actorId == ACTOR_OBJ_OSHIHIKI || actorId == ACTOR_OBJ_HSBLOCK ||
           actorId == ACTOR_OBJ_ELEVATOR || actorId == ACTOR_OBJ_LIFT || actorId == ACTOR_OBJ_TIMEBLOCK ||
           actorId == ACTOR_BG_MIZU_WATER || actorId == ACTOR_BG_MIZU_MOVEBG || actorId == ACTOR_BG_MIZU_SHUTTER ||
           actorId == ACTOR_BG_HIDAN_FSLIFT || actorId == ACTOR_BG_JYA_COBRA ||
           actorId == ACTOR_BG_JYA_BIGMIRROR || actorId == ACTOR_BG_HAKA_SHIP ||
           actorId == ACTOR_BG_HAKA_WATER || actorId == ACTOR_BG_HAKA_GATE || actorId == ACTOR_BG_BDAN_OBJECTS;
}

bool Anchor::IsEnemySyncActor(Actor* actor) {
    constexpr s16 DEKUNUTS_FLOWER_PARAM = 10;

    if (actor == nullptr) {
        return false;
    }

    if ((actor->id == ACTOR_EN_DEKUNUTS || actor->id == ACTOR_EN_HINTNUTS) &&
        actor->params == DEKUNUTS_FLOWER_PARAM) {
        return false;
    }

    return IsEnemySyncActor((ActorCategory)actor->category, actor->id);
}

Actor* Anchor::FindNearbyDeadEnemyDropSource(Actor* dropActor) {
    if (dropActor == nullptr || gPlayState == nullptr) {
        return nullptr;
    }

    Actor* closest = nullptr;
    float closestDistSq = 22500.0f;
    for (s32 category = ACTORCAT_SWITCH; category < ACTORCAT_MAX; category++) {
        Actor* currAct = gPlayState->actorCtx.actorLists[category].head;
        while (currAct != nullptr) {
            bool canDropCollectible = currAct->category == ACTORCAT_ENEMY || currAct->category == ACTORCAT_BOSS ||
                                      currAct->id == ACTOR_EN_SW;
            if (canDropCollectible && GetEnemyNetworkId(currAct) != 0 && currAct->colChkInfo.health == 0 &&
                currAct->update != nullptr) {
                float dx = currAct->world.pos.x - dropActor->world.pos.x;
                float dy = currAct->world.pos.y - dropActor->world.pos.y;
                float dz = currAct->world.pos.z - dropActor->world.pos.z;
                float distSq = (dx * dx) + (dy * dy) + (dz * dz);
                if (distSq < closestDistSq) {
                    closest = currAct;
                    closestDistSq = distSq;
                }
            }
            currAct = currAct->next;
        }
    }

    return closest;
}

uint64_t Anchor::CreateEnemyDropNetworkId(Actor* source, Actor* dropActor) {
    uint64_t sourceNetworkId = GetEnemyNetworkId(source);
    if (sourceNetworkId == 0 || dropActor == nullptr) {
        return 0;
    }

    uint16_t occurrence = enemyDropCounters[sourceNetworkId]++;
    uint64_t networkId = 0;
    do {
        uint64_t hash = 1469598103934665603ULL;
        auto hashValue = [&](uint64_t value) {
            hash ^= value;
            hash *= 1099511628211ULL;
        };
        hashValue(sourceNetworkId);
        hashValue((uint16_t)dropActor->id);
        hashValue((uint16_t)dropActor->params);
        hashValue(occurrence++);
        networkId = hash;
    } while (FindActorByEnemyNetworkId(networkId) != nullptr);

    enemyDropCounters[sourceNetworkId] = occurrence;
    return networkId;
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

        s16 homeX = (s16)(actor->home.pos.x / 20.0f);
        s16 homeY = (s16)(actor->home.pos.y / 20.0f);
        s16 homeZ = (s16)(actor->home.pos.z / 20.0f);
        uint32_t counterKey = ((uint32_t)actor->category << 16) | (uint16_t)actor->id;
        uint16_t occurrence = nextOccurrence[counterKey];
        uint64_t networkId = 0;
        do {
            uint64_t hash = 1469598103934665603ULL;
            auto hashValue = [&](uint64_t value) {
                hash ^= value;
                hash *= 1099511628211ULL;
            };
            hashValue((uint16_t)gPlayState->sceneNum);
            hashValue((uint8_t)gPlayState->roomCtx.curRoom.num);
            hashValue(actor->category);
            hashValue((uint16_t)actor->id);
            hashValue((uint16_t)actor->params);
            hashValue((uint16_t)homeX);
            hashValue((uint16_t)homeY);
            hashValue((uint16_t)homeZ);
            if (actor->id == ACTOR_EN_NUTSBALL) {
                hashValue(ownClientId);
                hashValue(GetEnemyRoomAuthorityGeneration(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num));
                hashValue(transientEnemyCounter++);
            } else {
                hashValue(occurrence);
            }
            networkId = hash;
            if (actor->id != ACTOR_EN_NUTSBALL) {
                occurrence++;
            }
        } while (usedNetworkIds.contains(networkId));

        if (actor->id != ACTOR_EN_NUTSBALL) {
            nextOccurrence[counterKey] = occurrence;
        }
        usedNetworkIds.insert(networkId);
        ObjectExtension::GetInstance().Set<EnemyNetworkId>(actor, EnemyNetworkId{ networkId });
    }
}

void Anchor::ResetEnemyRoomTransientState() {
    enemyKillBuffer.clear();
    enemyPruneBuffer.clear();
    enemySpawnBuffer.clear();
    enemyHealthTracker.clear();
    enemyAuthorityTargets.clear();
    enemyExtraStates.clear();
    enemyDropCounters.clear();
    transientEnemyCounter = 0;
    suppressedTransientProjectileKills.clear();
    hintnutsDialogueActive.clear();
    enemyTransformFrameCounter = 0;
}

bool Anchor::IsRoomStable() {
    return IsSaveLoaded() && gPlayState->roomCtx.status == 0 && gPlayState->roomCtx.curRoom.num >= 0 &&
           gPlayState->roomCtx.curRoom.segment != nullptr;
}

void Anchor::DetectEnemyRoomChange() {
    if (!IsSaveLoaded()) {
        return;
    }

    bool roomChanged = enemySyncSceneNum != gPlayState->sceneNum || enemySyncRoomNum != gPlayState->roomCtx.curRoom.num;
    if (roomChanged) {
        enemySyncSceneNum = gPlayState->sceneNum;
        enemySyncRoomNum = gPlayState->roomCtx.curRoom.num;
        enemyRoomSyncPending = true;
        ResetEnemyRoomTransientState();
        SendPacket_UpdateClientState();
    }

    if (enemyRoomSyncPending && IsRoomStable()) {
        enemyRoomSyncPending = false;
        SendPacket_UpdateClientState();
        SendPacket_RequestRoomEnemies();
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

static bool AnchorIsActorInCurrentLists(Actor* actor) {
    if (actor == nullptr || gPlayState == nullptr) {
        return false;
    }

    for (s32 category = ACTORCAT_SWITCH; category < ACTORCAT_MAX; category++) {
        Actor* currAct = gPlayState->actorCtx.actorLists[category].head;
        while (currAct != nullptr) {
            if (currAct == actor) {
                return true;
            }
            currAct = currAct->next;
        }
    }

    return false;
}

void Anchor::ApplyEnemyAuthorityState(Actor* actor, EnemyAuthorityState state, bool immediate) {
    if (actor == nullptr) {
        return;
    }

    float distSq = AnchorVec3fDistSq(actor->world.pos, state.pos);
    float correction = 0.2f;
    if (actor->id == ACTOR_EN_NUTSBALL || immediate || distSq > 250000.0f) {
        correction = 1.0f;
    } else if (distSq > 40000.0f) {
        correction = 0.75f;
    } else if (distSq > 10000.0f) {
        correction = 0.5f;
    }

    Vec3f correctedPos = AnchorLerpVec3f(actor->world.pos, state.pos, correction);
    actor->world.pos = correctedPos;
    actor->prevPos = AnchorLerpVec3f(actor->prevPos, correctedPos, correction);
    actor->world.rot = state.worldRot;
    actor->shape.rot = state.shapeRot;
    actor->scale = state.scale;
    actor->velocity = AnchorLerpVec3f(actor->velocity, state.velocity, correction);
    actor->speedXZ = AnchorLerpFloat(actor->speedXZ, state.speedXZ, correction);
    actor->gravity = state.gravity;
    actor->minVelocityY = state.minVelocityY;
    if (actor->category == ACTORCAT_ENEMY || actor->category == ACTORCAT_BOSS) {
        actor->yawTowardsPlayer = state.yawTowardsPlayer;
        actor->xzDistToPlayer = state.xzDistToPlayer;
        actor->yDistToPlayer = state.yDistToPlayer;
        actor->xyzDistToPlayerSq = state.xyzDistToPlayerSq;
    } else {
        Player* player = GET_PLAYER(gPlayState);
        actor->xzDistToPlayer = Actor_WorldDistXZToActor(actor, &player->actor);
        actor->yDistToPlayer = Actor_HeightDiff(actor, &player->actor);
        actor->xyzDistToPlayerSq = SQ(actor->xzDistToPlayer) + SQ(actor->yDistToPlayer);
        actor->yawTowardsPlayer = Actor_WorldYawTowardActor(actor, &player->actor);
    }
    actor->freezeTimer = state.freezeTimer;
    actor->colorFilterTimer = state.colorFilterTimer;
    if (state.colorFilterParams != 0) {
        actor->colorFilterParams = state.colorFilterParams;
    }
    if (!HasEnemySyncAuthority() && actor->colChkInfo.health < state.health) {
        // The local player may have damaged this replica during its update. Keep that lower health long enough for
        // DetectEnemyDamage to report it to the room authority instead of immediately rolling it back.
        enemyHealthTracker[actor] = state.health;
    } else {
        actor->colChkInfo.health = state.health;
        enemyHealthTracker[actor] = state.health;
    }
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
        nlohmann::json authorityExtra =
            enemyExtraStates.contains(it->first) ? enemyExtraStates[it->first] : nlohmann::json::object();
        if (ShouldPreserveLocalEnemyExtraState(actor, authorityExtra)) {
            ++it;
            continue;
        }
        ApplyEnemyAuthorityState(actor, it->second, false);
        ++it;
    }
}

void Anchor::ApplyEnemyExtraStates() {
    if (!IsSaveLoaded() || HasEnemySyncAuthority()) {
        return;
    }

    for (auto it = enemyExtraStates.begin(); it != enemyExtraStates.end();) {
        Actor* actor = FindActorByEnemyNetworkId(it->first);
        if (actor == nullptr) {
            it = enemyExtraStates.erase(it);
            continue;
        }
        if (enemyAuthorityTargets.contains(it->first) && actor->colChkInfo.health < enemyAuthorityTargets[it->first].health) {
            ++it;
            continue;
        }
        if (ShouldPreserveLocalEnemyExtraState(actor, it->second)) {
            ++it;
            continue;
        }
        ApplyEnemyExtraState(actor, it->second);
        ++it;
    }
}

uint32_t Anchor::GetEnemyRoomKey(s16 sceneNum, s8 roomNum) {
    return ((uint32_t)(uint16_t)sceneNum << 8) | (uint8_t)roomNum;
}

uint32_t Anchor::GetEnemyRoomAuthorityGeneration(s16 sceneNum, s8 roomNum) {
    uint32_t roomKey = GetEnemyRoomKey(sceneNum, roomNum);
    if (!enemyRoomAuthorityGenerations.contains(roomKey)) {
        enemyRoomAuthorityGenerations[roomKey] = 1;
    }
    return enemyRoomAuthorityGenerations[roomKey];
}

uint32_t Anchor::GetEnemySyncAuthorityClientId() {
    if (!IsSaveLoaded()) {
        return 0;
    }

    return GetEnemySyncAuthorityClientId(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num);
}

uint32_t Anchor::GetEnemySyncAuthorityClientId(s16 sceneNum, s8 roomNum) {
    if (!IsRoomStable() || ownClientId == 0) {
        return 0;
    }

    uint32_t roomKey = GetEnemyRoomKey(sceneNum, roomNum);
    uint32_t authorityClientId = ownClientId;
    for (auto& [clientId, client] : clients) {
        if (!client.online || client.self || !client.isSaveLoaded || !client.roomStable) {
            continue;
        }
        if (client.sceneNum == sceneNum && client.curRoomNum == roomNum && clientId < authorityClientId) {
            authorityClientId = clientId;
        }
    }

    if (!enemyRoomAuthorities.contains(roomKey)) {
        enemyRoomAuthorities[roomKey] = authorityClientId;
        enemyRoomAuthorityGenerations[roomKey] = 1;
    } else if (enemyRoomAuthorities[roomKey] != authorityClientId) {
        enemyRoomAuthorities[roomKey] = authorityClientId;
        enemyRoomAuthorityGenerations[roomKey] = GetEnemyRoomAuthorityGeneration(sceneNum, roomNum) + 1;
    }

    return authorityClientId;
}

bool Anchor::HasEnemySyncAuthority() {
    if (!IsSaveLoaded()) {
        return false;
    }

    return HasEnemySyncAuthority(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num);
}

bool Anchor::HasEnemySyncAuthority(s16 sceneNum, s8 roomNum) {
    return GetEnemySyncAuthorityClientId(sceneNum, roomNum) == ownClientId;
}

bool Anchor::IsValidEnemyAuthorityPacket(nlohmann::json payload) {
    if (!IsRoomStable()) {
        return false;
    }

    uint32_t clientId = payload.value("clientId", (uint32_t)0);
    s16 sceneNum = payload.value("sceneNum", (s16)SCENE_ID_MAX);
    s8 roomNum = payload.value("roomNum", (s8)-1);
    uint32_t authorityClientId = payload.value("authorityClientId", (uint32_t)0);
    uint32_t authorityGeneration = payload.value("authorityGeneration", (uint32_t)0);

    if (sceneNum != gPlayState->sceneNum || roomNum != gPlayState->roomCtx.curRoom.num) {
        return false;
    }
    if (authorityClientId == 0) {
        authorityClientId = clientId;
    }
    uint32_t roomKey = GetEnemyRoomKey(sceneNum, roomNum);
    if (clientId != authorityClientId || authorityClientId != GetEnemySyncAuthorityClientId(sceneNum, roomNum)) {
        return false;
    }

    uint32_t localGeneration = GetEnemyRoomAuthorityGeneration(sceneNum, roomNum);
    if (authorityGeneration > localGeneration) {
        enemyRoomAuthorityGenerations[roomKey] = authorityGeneration;
    }

    return true;
}

void Anchor::MarkEnemyDead(uint64_t networkId) {
    if (!IsSaveLoaded() || networkId == 0) {
        return;
    }

    MarkEnemyDead(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num, networkId);
}

void Anchor::MarkEnemyDead(s16 sceneNum, s8 roomNum, uint64_t networkId) {
    if (networkId == 0) {
        return;
    }

    deadEnemyLedger[GetEnemyRoomKey(sceneNum, roomNum)].insert(networkId);
    enemyAuthorityTargets.erase(networkId);
    enemyExtraStates.erase(networkId);
}

bool Anchor::IsEnemyMarkedDead(uint64_t networkId) {
    if (!IsSaveLoaded() || networkId == 0) {
        return false;
    }

    return IsEnemyMarkedDead(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num, networkId);
}

bool Anchor::IsEnemyMarkedDead(s16 sceneNum, s8 roomNum, uint64_t networkId) {
    if (networkId == 0) {
        return false;
    }

    uint32_t roomKey = GetEnemyRoomKey(sceneNum, roomNum);
    return deadEnemyLedger.contains(roomKey) && deadEnemyLedger[roomKey].contains(networkId);
}

static bool ShouldDeferKillForLocalDialogue(Actor* actor) {
    if (actor == nullptr || gPlayState == nullptr) {
        return false;
    }

    Player* player = GET_PLAYER(gPlayState);
    if (player == nullptr) {
        return false;
    }

    if (!(player->stateFlags1 & PLAYER_STATE1_TALKING) && Message_GetState(&gPlayState->msgCtx) == TEXT_STATE_NONE) {
        return false;
    }

    return player->talkActor == actor || gPlayState->msgCtx.talkActor == actor;
}

static bool EnemyKillBufferContains(const std::vector<uint64_t>& buffer, uint64_t networkId) {
    for (uint64_t bufferedId : buffer) {
        if (bufferedId == networkId) {
            return true;
        }
    }

    return false;
}

void Anchor::ProcessActorBuffers() {
    if (!IsSaveLoaded()) {
        return;
    }

    std::vector<uint64_t> deferredKillBuffer;
    while (!enemyKillBuffer.empty()) {
        uint64_t networkId = enemyKillBuffer.front();
        enemyKillBuffer.erase(enemyKillBuffer.begin());
        Actor* actor = FindActorByEnemyNetworkId(networkId);
        if (actor != nullptr && actor->update != nullptr) {
            if (ShouldDeferKillForLocalDialogue(actor)) {
                if (!EnemyKillBufferContains(deferredKillBuffer, networkId)) {
                    deferredKillBuffer.push_back(networkId);
                }
                continue;
            }

            if (actor->id == ACTOR_EN_NUTSBALL) {
                suppressedTransientProjectileKills.insert(networkId);
            }
            Actor_Kill(actor);
        }
    }
    enemyKillBuffer.insert(enemyKillBuffer.end(), deferredKillBuffer.begin(), deferredKillBuffer.end());

    while (!enemyPruneBuffer.empty()) {
        auto [actor, sceneNum, roomNum] = enemyPruneBuffer.front();
        enemyPruneBuffer.erase(enemyPruneBuffer.begin());
        if (sceneNum != gPlayState->sceneNum || roomNum != gPlayState->roomCtx.curRoom.num) {
            continue;
        }
        if (HasEnemySyncAuthority() || !AnchorIsActorInCurrentLists(actor) || actor->update == nullptr) {
            continue;
        }
        if (IsEnemySyncActor(actor) && GetEnemyNetworkId(actor) == 0) {
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

    for (s32 cat = ACTORCAT_SWITCH; cat < ACTORCAT_MAX; cat++) {
        Actor* currAct = gPlayState->actorCtx.actorLists[cat].head;
        while (currAct != nullptr) {
            if (IsEnemySyncActor(currAct) || GetEnemyNetworkId(currAct) != 0) {
                currentEnemies.push_back(currAct);
            }
            currAct = currAct->next;
        }
    }

    std::vector<Actor*> assignableEnemies;
    for (Actor* act : currentEnemies) {
        if (IsEnemySyncActor(act)) {
            assignableEnemies.push_back(act);
        }
    }
    if (HasEnemySyncAuthority()) {
        AssignEnemyNetworkIds(assignableEnemies);
    }

    for (Actor* act : currentEnemies) {
        uint64_t networkId = GetEnemyNetworkId(act);
        if (IsEnemyMarkedDead(networkId) && !EnemyKillBufferContains(enemyKillBuffer, networkId)) {
            enemyKillBuffer.push_back(networkId);
        }
    }

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
        if (act->id == ACTOR_EN_GOMA && (s8)act->colChkInfo.health <= 0) {
            currentHealth = 0;
        }

        if (enemyHealthTracker.contains(act)) {
            u8 lastHealth = enemyHealthTracker[act];
            uint64_t networkId = GetEnemyNetworkId(act);
            nlohmann::json authorityExtra =
                enemyExtraStates.contains(networkId) ? enemyExtraStates[networkId] : nlohmann::json::object();
            if (currentHealth < lastHealth) {
                if (HasEnemySyncAuthority()) {
                    SendPacket_DamageEnemy(act, currentHealth);
                } else {
                    SendPacket_ReportEnemyDamage(act, currentHealth);
                }
            } else if (!HasEnemySyncAuthority() && currentHealth == lastHealth &&
                       ShouldPreserveLocalEnemyExtraState(act, authorityExtra) && ShouldReportEnemyExtraState(act)) {
                SendPacket_ReportEnemyDamage(act, currentHealth);
            }
        }

        enemyHealthTracker[act] = currentHealth;
    }

    enemyTransformFrameCounter++;
    if (enemyTransformFrameCounter >= 1) {
        enemyTransformFrameCounter = 0;
        SendPacket_EnemyUpdate(currentEnemies);
    }
}
