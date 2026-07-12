#include "Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include <unordered_set>
#include <stdexcept>
#include <chrono>
#include "soh/OTRGlobals.h"
#include "soh/Enhancements/nametag.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ObjectExtension/ObjectExtension.h"
#include "soh/ObjectExtension/ActorListIndex.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/Network/Anchor/GenericEnemySync.h"

extern "C" {
#include "variables.h"
#include "functions.h"
#include "src/overlays/actors/ovl_Boss_Goma/z_boss_goma.h"
#include "src/overlays/actors/ovl_En_Horse/z_en_horse.h"
#include "src/overlays/actors/ovl_Obj_Tsubo/z_obj_tsubo.h"
#include "src/overlays/actors/ovl_Obj_Kibako/z_obj_kibako.h"
extern PlayState* gPlayState;
}

extern "C" {
void ObjTsubo_AirBreak(ObjTsubo* thisx, PlayState* play);
void ObjKibako_AirBreak(ObjKibako* thisx, PlayState* play);
}

extern void ApplyEnemyExtraState(Actor* actor, nlohmann::json extra);
extern bool ShouldReportEnemyExtraState(Actor* actor);
extern bool ShouldPreserveLocalEnemyExtraState(Actor* actor, nlohmann::json authorityExtra);

static bool AnchorIsActorInCurrentLists(Actor* actor);

// Picks a random ambush target among all players for field enemy spawners. Returns 0 when the local player
// was picked (or no co-op session), letting the caller use its vanilla local-player path; remote players are
// only candidates while moving, mirroring the vanilla walking requirement.
extern "C" u8 Anchor_GetRandomAmbushFocus(Vec3f* outPos, s16* outYaw) {
    if (Anchor::Instance == nullptr || !Anchor::Instance->IsSaveLoaded()) {
        return 0;
    }

    std::vector<const AnchorClient*> candidates;
    for (auto& [clientId, client] : Anchor::Instance->clients) {
        if (!client.online || client.self || !client.isSaveLoaded || !client.isMoving) {
            continue;
        }
        if (client.sceneNum != gPlayState->sceneNum || client.curRoomNum != gPlayState->roomCtx.curRoom.num) {
            continue;
        }
        candidates.push_back(&client);
    }
    if (candidates.empty()) {
        return 0;
    }

    // The local player occupies one extra slot so ambushes stay evenly distributed.
    size_t pick = (size_t)(Rand_ZeroOne() * (candidates.size() + 1));
    if (pick >= candidates.size()) {
        return 0;
    }
    *outPos = candidates[pick]->posRot.pos;
    *outYaw = candidates[pick]->posRot.rot.y;
    return 1;
}

extern "C" u8 Anchor_ShouldReplayCutsceneForFlag(s16 flag) {
    if (Anchor::Instance == nullptr) {
        return 0;
    }
    return Anchor::Instance->HasCutsceneReplayFlag(flag) ? 1 : 0;
}

extern "C" u8 Anchor_HasQuestItemCutsceneReplay(s32 questItem) {
    if (Anchor::Instance == nullptr) {
        return 0;
    }
    return Anchor::Instance->HasQuestItemCutsceneReplay(questItem) ? 1 : 0;
}

// Non-consuming: gates poll these every frame. Entries are erased when the local game itself sets the flag
// or teaches the song, i.e. when the local player has actually experienced the event.
bool Anchor::HasCutsceneReplayFlag(s16 flag) {
    return pendingCutsceneReplayFlags.contains(flag);
}

bool Anchor::HasQuestItemCutsceneReplay(s32 questItem) {
    return pendingQuestItemReplays.contains(questItem);
}

void Anchor::FinishQuestItemCutsceneReplay(s32 questItem) {
    pendingQuestItemReplays.erase(questItem);
}

uint32_t Anchor::GetTimeSyncAuthorityClientId() {
    uint32_t authorityClientId = IsSaveLoaded() ? ownClientId : 0;
    for (auto& [clientId, client] : clients) {
        if (!client.online || client.self || !client.isSaveLoaded || !client.roomStable ||
            client.stableRoomFrames < 5) {
            continue;
        }
        if (authorityClientId == 0 || clientId < authorityClientId) {
            authorityClientId = clientId;
        }
    }
    return authorityClientId;
}

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
        if (!client.online || client.self || !client.isSaveLoaded || !client.roomStable ||
            client.stableRoomFrames < 5 || client.player == nullptr) {
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

extern "C" Player* Anchor_GetEnemyTargetPlayer(Actor* actor) {
    if (Anchor::Instance == nullptr) {
        return nullptr;
    }
    return Anchor::Instance->GetEnemyTargetPlayer(actor);
}

// MARK: - Overrides

void Anchor::Enable() {
    ownClientId = CVarGetInteger(CVAR_REMOTE_ANCHOR("LastClientId"), 0);
    roomState.ownerClientId = 0;
    connectedEventPending = false;
    disconnectedEventPending = false;
    if (networkLifecycleHookId == 0) {
        networkLifecycleHookId =
            GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>([this]() {
                ProcessConnectionEvents();
            });
    }
    Network::Enable(CVarGetString(CVAR_REMOTE_ANCHOR("Host"), "anchor.hm64.org"),
                    CVarGetInteger(CVAR_REMOTE_ANCHOR("Port"), 43383));
}

void Anchor::Disable() {
    Network::Disable();
    connectedEventPending = false;
    disconnectedEventPending = false;
    RegisterHooks();
    ResetEnemySessionState(false);
    if (networkLifecycleHookId != 0) {
        GameInteractor::Instance->UnregisterGameHook<GameInteractor::OnGameFrameUpdate>(networkLifecycleHookId);
        networkLifecycleHookId = 0;
    }
}

void Anchor::OnConnected() {
    connectedEventPending = true;
}

void Anchor::OnDisconnected() {
    disconnectedEventPending = true;
}

void Anchor::ProcessConnectionEvents() {
    bool disconnected = disconnectedEventPending.exchange(false);
    bool connected = connectedEventPending.exchange(false);

    if (disconnected) {
        RegisterHooks();
        ResetEnemySessionState(true);
    }

    if (connected && isConnected) {
        if (enemySessionId == 0) {
            enemySessionId = ((uint64_t)std::chrono::steady_clock::now().time_since_epoch().count() << 1) ^
                             ((uint64_t)ownClientId << 32) ^ (uintptr_t)this;
            if (enemySessionId == 0) {
                enemySessionId = 1;
            }
            enemySnapshotSequences.clear();
            nextEnemyOperationId = 0;
        }
        pendingCutsceneReplayFlags.clear();
        pendingQuestItemReplays.clear();
        enemyRoomSyncPending = true;
        SendPacket_Handshake();
        RegisterHooks();
        if (IsSaveLoaded()) {
            SendPacket_RequestTeamState();
        }
    }
}

void Anchor::ResetEnemySessionState(bool preserveWorldState) {
    clients.clear();
    RefreshClientActors();
    ResetEnemyRoomTransientState();
    if (!preserveWorldState) {
        pendingEnemyDamageOperations.clear();
        appliedEnemyDamageOperations.clear();
        enemyCarryOwnership.clear();
        deadEnemyLedger.clear();
        dynamicEnemyTombstoneOrder.clear();
        enemySnapshotSequences.clear();
        lastEnemySnapshotSequences.clear();
        nextEnemyOperationId = 0;
        nextDynamicEnemyIncarnation = 0;
        enemySessionId = 0;
    }
    enemyRoomAuthorities.clear();
    enemyRoomAuthorityGenerations.clear();
    enemyRoomAuthoritySessions.clear();
    enemySyncSceneNum = SCENE_ID_MAX;
    enemySyncRoomNum = -1;
    enemyRoomSyncPending = true;
    {
        std::lock_guard<std::mutex> lock(incomingPacketQueueMutex);
        std::queue<nlohmann::json>().swap(incomingPacketQueue);
    }
    {
        std::lock_guard<std::mutex> lock(outgoingPacketQueueMutex);
        std::queue<nlohmann::json>().swap(outgoingPacketQueue);
    }
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

    // Queue the packet to be sent on the network thread
    std::lock_guard<std::mutex> lock(outgoingPacketQueueMutex);
    outgoingPacketQueue.push(payload);
}

void Anchor::OnIncomingJson(nlohmann::json payload) {
    // The network thread only validates the envelope. Client/version checks happen on the game thread,
    // which exclusively owns the clients map.
    if (!payload.is_object() || !payload.contains("type") || !payload["type"].is_string()) {
        return;
    }

    // If it's not a quiet payload, log it
    if (!payload.contains("quiet")) {
        SPDLOG_DEBUG("[Anchor] Received payload:\n{}", payload.dump());
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

        isProcessingIncomingPacket = true;

        try {
            if (!payload.is_object() || !payload.contains("type") || !payload["type"].is_string()) {
                throw std::runtime_error("invalid packet envelope");
            }
            std::string packetType = payload["type"].get<std::string>();
            if (packetType != ALL_CLIENT_STATE && packetType != UPDATE_CLIENT_STATE && packetType != PLAYER_UPDATE &&
                payload.contains("clientId") && payload["clientId"].is_number_unsigned()) {
                uint32_t clientId = payload["clientId"].get<uint32_t>();
                if (clients.contains(clientId) && clients[clientId].clientVersion != clientVersion) {
                    isProcessingIncomingPacket = false;
                    continue;
                }
            }
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
            else if (packetType == WARP_TO_ENTRANCE)
                HandlePacket_WarpToEntrance(payload);
            else if (packetType == TIME_UPDATE)
                HandlePacket_TimeUpdate(payload);
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

struct EnemySpawnMetadata {
    s16 params = 0;
    s16 sceneNum = SCENE_ID_MAX;
    s8 originRoom = -1;
    u16 sceneSetupIndex = 0;
};
static ObjectExtension::Register<EnemySpawnMetadata> EnemySpawnMetadataRegister;

uint32_t Anchor::GetDummyPlayerClientId(const Actor* actor) {
    const DummyPlayerClientId* clientId = ObjectExtension::GetInstance().Get<DummyPlayerClientId>(actor);
    return clientId != nullptr ? clientId->clientId : 0;
}

Player* Anchor::GetEnemyTargetPlayer(Actor* actor) {
    if (actor == nullptr || !IsSaveLoaded()) {
        return nullptr;
    }

    uint64_t networkId = GetEnemyNetworkId(actor);
    auto target = enemyTargetClientIds.find(networkId);
    if (target == enemyTargetClientIds.end()) {
        return Anchor_GetNearestEnemyTargetPlayer(actor);
    }
    if (target->second == ownClientId) {
        return GET_PLAYER(gPlayState);
    }
    auto client = clients.find(target->second);
    if (client == clients.end() || !client->second.online || !client->second.isSaveLoaded ||
        !client->second.roomStable || client->second.stableRoomFrames < 5 ||
        client->second.sceneNum != gPlayState->sceneNum ||
        client->second.curRoomNum != gPlayState->roomCtx.curRoom.num || client->second.player == nullptr) {
        return Anchor_GetNearestEnemyTargetPlayer(actor);
    }
    return client->second.player;
}

void Anchor::ApplyEnemyTargetMetrics(Actor* actor) {
    Player* target = GetEnemyTargetPlayer(actor);
    if (actor == nullptr || target == nullptr) {
        return;
    }

    f32 dx = target->actor.world.pos.x - actor->world.pos.x;
    f32 dz = target->actor.world.pos.z - actor->world.pos.z;
    actor->xzDistToPlayer = sqrtf(SQ(dx) + SQ(dz));
    actor->yDistToPlayer = target->actor.world.pos.y - actor->world.pos.y;
    actor->xyzDistToPlayerSq = SQ(actor->xzDistToPlayer) + SQ(actor->yDistToPlayer);
    actor->yawTowardsPlayer = Math_Vec3f_Yaw(&actor->world.pos, &target->actor.world.pos);
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
                                                         s16 actorParams, s8 roomNum) {
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
            // In overworld scenes, actors from multiple rooms are loaded.
            // Only match actors from the same room as the authority packet
            // to prevent cross-room matching that causes duplicates.
            if (roomNum >= 0 && currAct->room != roomNum) {
                currAct = currAct->next;
                continue;
            }
            if (actorParams != (s16)-0x8000 && GetEnemySpawnParams(currAct) != actorParams) {
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

void Anchor::CaptureEnemySpawnParams(Actor* actor) {
    if (actor != nullptr && ObjectExtension::GetInstance().Get<EnemySpawnMetadata>(actor) == nullptr) {
        s16 actorListIndex = GetActorListIndex(actor);
        s16 sceneNum = actorListIndex >= 0 ? GetActorListSceneNum(actor)
                                          : (gPlayState != nullptr ? gPlayState->sceneNum : (s16)SCENE_ID_MAX);
        s8 originRoom = actorListIndex >= 0 ? GetActorListOriginRoom(actor) : actor->room;
        u16 sceneSetupIndex = actorListIndex >= 0 ? (u16)GetActorListSceneSetupIndex(actor)
                                                  : (u16)gSaveContext.sceneSetupIndex;
        s16 spawnParams = actorListIndex >= 0 ? GetActorListSpawnParams(actor) : actor->params;
        ObjectExtension::GetInstance().Set<EnemySpawnMetadata>(
            actor, EnemySpawnMetadata{ spawnParams, sceneNum, originRoom, sceneSetupIndex });
    }
}

s16 Anchor::GetEnemySpawnParams(Actor* actor) {
    if (actor == nullptr) {
        return 0;
    }
    if (GetActorListIndex(actor) >= 0) {
        return GetActorListSpawnParams(actor);
    }
    CaptureEnemySpawnParams(actor);
    return ObjectExtension::GetInstance().Get<EnemySpawnMetadata>(actor)->params;
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
    if (IsLocallySimulatedEffectActor(actorId)) {
        return false;
    }
    return category == ACTORCAT_ENEMY || category == ACTORCAT_BOSS || actorId == ACTOR_EN_HINTNUTS ||
           actorId == ACTOR_EN_SW || actorId == ACTOR_EN_NUTSBALL ||
           actorId == ACTOR_OBJ_OSHIHIKI || actorId == ACTOR_OBJ_HSBLOCK ||
           actorId == ACTOR_OBJ_ELEVATOR || actorId == ACTOR_OBJ_LIFT || actorId == ACTOR_OBJ_TIMEBLOCK ||
           actorId == ACTOR_BG_MIZU_WATER || actorId == ACTOR_BG_MIZU_MOVEBG || actorId == ACTOR_BG_MIZU_SHUTTER ||
           actorId == ACTOR_BG_HIDAN_FSLIFT || actorId == ACTOR_BG_JYA_COBRA ||
           actorId == ACTOR_BG_JYA_BIGMIRROR || actorId == ACTOR_BG_HAKA_SHIP ||
           actorId == ACTOR_BG_HAKA_WATER || actorId == ACTOR_BG_HAKA_GATE || actorId == ACTOR_BG_BDAN_OBJECTS ||
           // Pushable / breakable objects
           actorId == ACTOR_BG_SPOT15_RRBOX ||  // Hyrule Castle milk crates
           actorId == ACTOR_OBJ_SYOKUDAI ||      // Torches (lit state)
           actorId == ACTOR_EN_GOROIWA ||        // Rolling boulders
           actorId == ACTOR_OBJ_TSUBO ||         // Pots
           actorId == ACTOR_OBJ_KIBAKO ||        // Small wooden crates
           actorId == ACTOR_EN_DNS ||            // Business Scrub (Dodongo's Cavern)
           actorId == ACTOR_EN_BOMBF ||          // Bomb flowers (pick/throw sync)
           actorId == ACTOR_EN_BOM ||            // Bombs (carried/thrown/explosion sync)
           // NPCs (position sync — NPCs that move/roam/patrol)
           actorId == ACTOR_EN_NIW ||            // Cuccos (wander, pickup)
           actorId == ACTOR_EN_DOG ||            // Dog (roams)
           actorId == ACTOR_EN_MM ||             // Running man (child, runs)
           actorId == ACTOR_EN_MM2 ||            // Running man (adult, runs)
           actorId == ACTOR_EN_NIW_GIRL ||       // Girl chasing cucco (moves)
           actorId == ACTOR_EN_NIW_LADY ||       // Cucco Lady (walks)
           actorId == ACTOR_EN_GO2 ||            // Gorons (rolling)
           actorId == ACTOR_EN_ZO ||             // Zoras (swim)
           actorId == ACTOR_EN_KO ||             // Kokiri children (wander)
           actorId == ACTOR_EN_HY ||             // Market NPCs (bag guy, townspeople)
           actorId == ACTOR_EN_HEISHI1 ||        // Courtyard guards (patrol)
           actorId == ACTOR_EN_HEISHI2 ||        // Kakariko guards (patrol)
           actorId == ACTOR_EN_HEISHI3 ||        // Gate guards
           actorId == ACTOR_EN_HEISHI4 ||        // Hyrule guards
           actorId == ACTOR_EN_DAIKU_KAKARIKO || // Carpenters (Kakariko)
           actorId == ACTOR_EN_DAIKU ||          // Carpenters (tent)
           actorId == ACTOR_EN_TORYO ||          // Boss Carpenter
           actorId == ACTOR_EN_ANI ||            // Kakariko rooftop/scaffolding man
           actorId == ACTOR_EN_CS ||             // Graveyard Boy
           actorId == ACTOR_EN_HS ||             // Carpenter's Son (Grog)
           actorId == ACTOR_EN_HS2 ||            // Carpenter's Son (child era)
           actorId == ACTOR_EN_MA1 ||            // Malon (child)
           actorId == ACTOR_EN_MA2 ||            // Malon (adult, ranch)
           actorId == ACTOR_EN_MA3 ||            // Malon (adult, Lon Lon)
           actorId == ACTOR_EN_IN ||             // Ingo
           actorId == ACTOR_EN_TA ||             // Talon
           actorId == ACTOR_EN_MU ||             // Haggling Townspeople
           actorId == ACTOR_EN_TG ||             // Entwined Lovers
           actorId == ACTOR_EN_GUEST ||          // Happy Mask Shop Customer
           actorId == ACTOR_EN_GE1 ||            // Gerudo (white clothed)
           actorId == ACTOR_EN_GE2 ||            // Patrolling Gerudo
           actorId == ACTOR_EN_GE3 ||            // Gerudo Fortress Leader
           actorId == ACTOR_EN_SKJ ||            // Skullkid
           actorId == ACTOR_EN_FR;               // Frogs
}

bool Anchor::IsTransientProjectileActor(s16 actorId, s16 params) {
    // Short-lived fire-and-forget projectiles. Replicas spawn them from the authority's first snapshot and then
    // simulate the whole flight locally: per-frame pinning would hold them one RTT behind, and applying the
    // authority's kill would delete them mid-air (the authority impacts latency-delayed player puppets).
    // EN_OKUTA is the octorok itself with params 0 and its spat rock otherwise.
    return actorId == ACTOR_EN_NUTSBALL || (actorId == ACTOR_EN_OKUTA && params != 0);
}

bool Anchor::IsIndependentDuelActor(s16 actorId) {
    // Mirror-style duels: every client fights its own fully local copy (Dark Link is a Player-struct puppet
    // that mirrors whoever is in front of him, so authority sync would make him ignore the second player).
    // Only identity, damage, and kills sync, pooling everyone's damage into a shared health bar.
    return actorId == ACTOR_EN_TORCH2;
}

bool Anchor::IsParentDependentEnemy(s16 actorId, s16 params) {
    // These dereference actor->parent in their update paths, so a parentless network-spawned orphan would
    // crash. They are never spawned by the ENEMY_UPDATE fallback; replicas only associate the copies their
    // own simulation spawned (the spawning parent's actions are synced, so local copies always appear).
    return actorId == ACTOR_EN_FHG_FIRE || actorId == ACTOR_EN_DHA || actorId == ACTOR_EN_FW ||
           actorId == ACTOR_BOSS_FD2 || actorId == ACTOR_BOSS_VA || actorId == ACTOR_BOSS_SST ||
           actorId == ACTOR_BOSS_TW || actorId == ACTOR_EN_GOMA || actorId == ACTOR_EN_DODOJR ||
           actorId == ACTOR_BOSS_GANON || actorId == ACTOR_EN_PO_SISTERS || actorId == ACTOR_EN_FLOORMAS ||
           actorId == ACTOR_EN_TP || (actorId == ACTOR_EN_EIYER && params >= 1 && params <= 3) ||
           (actorId == ACTOR_EN_BB && (params == 0 || params == 11));
}

bool Anchor::IsActorInCurrentEnemyRoom(Actor* actor) {
    if (actor == nullptr || !IsRoomStable()) {
        return false;
    }

    return actor->room < 0 || actor->room == gPlayState->roomCtx.curRoom.num;
}

bool Anchor::IsLocallySimulatedEffectActor(s16 actorId) {
    // Excluded from enemy sync entirely: each client's (action-synced) parent spawns and simulates its own copy.
    // The first group dereferences actor->parent, so a network-spawned orphan would crash; the rest are
    // swarm/cutscene/escort actors in the ENEMY or BOSS categories whose motion is meaningless to mirror.
    return actorId == ACTOR_EN_BDFIRE || actorId == ACTOR_EN_FD_FIRE || actorId == ACTOR_EN_ANUBICE_FIRE ||
           actorId == ACTOR_EN_SKJNEEDLE || actorId == ACTOR_EN_ATTACK_NIW || actorId == ACTOR_EN_ENCOUNT2 ||
           actorId == ACTOR_EN_FIRE_ROCK || actorId == ACTOR_EN_GANON_MANT || actorId == ACTOR_EN_GANON_ORGAN ||
           actorId == ACTOR_EN_VB_BALL || actorId == ACTOR_DEMO_EFFECT || actorId == ACTOR_DEMO_GEFF ||
           actorId == ACTOR_DEMO_GJ || actorId == ACTOR_EN_RU1 || actorId == ACTOR_EN_ZL3 ||
           actorId == ACTOR_EN_SDA || actorId == ACTOR_EN_BLKOBJ || actorId == ACTOR_EN_CLEAR_TAG ||
           // Traversal hazards that must react to the local player rather than being pinned to the authority:
           // collapsing tower platforms and flying floor tiles.
           actorId == ACTOR_BG_GANON_OTYUKA || actorId == ACTOR_EN_YUKABYUN;
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

uint64_t Anchor::AllocateDynamicEnemyNetworkId(uint64_t sourceNetworkId) {
    if (enemySessionId == 0) {
        return 0;
    }

    uint64_t networkId = 0;
    do {
        nextDynamicEnemyIncarnation++;
        if (nextDynamicEnemyIncarnation == 0) {
            nextDynamicEnemyIncarnation++;
        }
        uint64_t hash = 1469598103934665603ULL;
        auto hashValue = [&](uint64_t value) {
            hash ^= value;
            hash *= 1099511628211ULL;
        };
        hashValue(0x44594E414D494331ULL); // "DYNAMIC1"
        hashValue(ownClientId);
        hashValue(enemySessionId);
        hashValue(nextDynamicEnemyIncarnation);
        hashValue(sourceNetworkId);
        networkId = hash & ~(1ULL << 63);

        bool isTombstoned = false;
        for (const auto& [sceneNum, deadIds] : deadEnemyLedger) {
            if (deadIds.contains(networkId)) {
                isTombstoned = true;
                break;
            }
        }
        if (networkId == 0 || FindActorByEnemyNetworkId(networkId) != nullptr || isTombstoned) {
            networkId = 0;
        }
    } while (networkId == 0);

    return networkId;
}

uint64_t Anchor::CreateEnemyDropNetworkId(Actor* source, Actor* dropActor) {
    uint64_t sourceNetworkId = GetEnemyNetworkId(source);
    if (sourceNetworkId == 0 || dropActor == nullptr) {
        return 0;
    }

    return AllocateDynamicEnemyNetworkId(sourceNetworkId);
}

void Anchor::AssignEnemyNetworkIds(std::vector<Actor*> actors) {
    if (!IsSaveLoaded()) {
        return;
    }

    for (Actor* actor : actors) {
        if (actor == nullptr || GetEnemyNetworkId(actor) != 0) {
            continue;
        }

        CaptureEnemySpawnParams(actor);
        const EnemySpawnMetadata* metadata = ObjectExtension::GetInstance().Get<EnemySpawnMetadata>(actor);
        s16 actorListIndex = GetActorListIndex(actor);
        uint64_t networkId;
        if (actorListIndex >= 0) {
            constexpr uint64_t STATIC_ENEMY_ID_BIT = 1ULL << 63;
            networkId = STATIC_ENEMY_ID_BIT | ((uint64_t)(uint16_t)metadata->sceneNum << 47) |
                        ((uint64_t)metadata->sceneSetupIndex << 31) |
                        ((uint64_t)(uint8_t)metadata->originRoom << 23) |
                        ((uint64_t)(uint16_t)actorListIndex << 7) | 1;
            Actor* existingActor = FindActorByEnemyNetworkId(networkId);
            if (existingActor != nullptr && existingActor != actor) {
                if (existingActor->update != nullptr) {
                    // A roomless carried actor can outlive its origin room. Keep that incarnation and discard the
                    // duplicate scene-list copy produced when the origin room reloads.
                    Actor_Kill(actor);
                }
                continue;
            }
        } else {
            if (!HasEnemySyncAuthority()) {
                continue;
            }
            networkId = AllocateDynamicEnemyNetworkId();
        }
        SetEnemyNetworkId(actor, networkId);
    }
}

void Anchor::ResetEnemyRoomTransientState() {
    if (gPlayState != nullptr) {
        for (s32 category = ACTORCAT_SWITCH; category < ACTORCAT_MAX; category++) {
            for (Actor* actor = gPlayState->actorCtx.actorLists[category].head; actor != nullptr; actor = actor->next) {
                bool isCarryable = actor->id == ACTOR_EN_NIW || actor->id == ACTOR_OBJ_TSUBO ||
                                   actor->id == ACTOR_OBJ_KIBAKO || actor->id == ACTOR_EN_BOMBF ||
                                   actor->id == ACTOR_EN_BOM;
                if (isCarryable && actor->parent == actor) {
                    actor->parent = nullptr;
                }
            }
        }
    }
    for (Actor* actor : enemyCullOverrides) {
        if (AnchorIsActorInCurrentLists(actor)) {
            actor->flags &= ~ACTOR_FLAG_UPDATE_CULLING_DISABLED;
        }
    }
    enemyKillBuffer.clear();
    enemyPruneBuffer.clear();
    enemySpawnBuffer.clear();
    enemyHealthTracker.clear();
    enemyAuthorityTargets.clear();
    enemyTargetClientIds.clear();
    enemyExtraStates.clear();
    freshEnemyAuthorityData.clear();
    enemyCullOverrides.clear();
    enemyDeathDeferralFrames.clear();
    previouslyHeldEnemyIds.clear();
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
    float correction = 0.3f;
    // Held carryable actors (parent != nullptr, includes self-reference) need exact
    // positioning so the object snaps to the holder's hand instead of lagging behind.
    if (IsTransientProjectileActor(actor->id, actor->params) || immediate || distSq > 250000.0f ||
        actor->parent != nullptr) {
        correction = 1.0f;
    } else if (distSq > 40000.0f) {
        correction = 0.8f;
    } else if (distSq > 10000.0f) {
        correction = 0.5f;
    }

    if (correction >= 1.0f) {
        actor->world.pos = state.pos;
        actor->prevPos = state.pos;
    } else if (distSq > 1.0f) {
        // Leave prevPos at the frame-start position so background checks treat the correction as regular motion
        // instead of a teleport, and so the renderer interpolates it smoothly.
        actor->world.pos = AnchorLerpVec3f(actor->world.pos, state.pos, correction);
    }
    // Apply home position on immediate snaps (new associations) so rooted enemies like
    // Deku Babas compute their visual offset from the correct base, not from world.pos.
    if (immediate) {
        actor->home.pos = state.homePos;
    }
    actor->world.rot = state.worldRot;
    actor->shape.rot = state.shapeRot;
    actor->scale = state.scale;
    // Velocity is taken verbatim: between authority snapshots the local update integrates it, acting as the
    // dead-reckoning extrapolator, so it must match the authority exactly rather than being smoothed.
    actor->velocity = state.velocity;
    actor->speedXZ = state.speedXZ;
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
        uint64_t networkId = GetEnemyNetworkId(actor);
        enemyHealthTracker[actor] = pendingEnemyDamageOperations.contains(networkId) ? actor->colChkInfo.health
                                                                                     : state.health;
    } else {
        actor->colChkInfo.health = state.health;
        enemyHealthTracker[actor] = state.health;
    }
}

bool Anchor::ConsumeFreshEnemyAuthorityData(uint64_t networkId) {
    return freshEnemyAuthorityData.erase(networkId) > 0;
}

// The engine only updates actors near the local player (update culling). In co-op an enemy must also stay active
// while a remote player is near it, otherwise it stands frozen when only the other player approaches. Mirror the
// engine's rule using the actor's own uncull range against every remote client in the room.
void Anchor::UpdateEnemyCullOverrides(const std::vector<Actor*>& currentEnemies) {
    std::unordered_set<Actor*> currentSet(currentEnemies.begin(), currentEnemies.end());
    for (auto it = enemyCullOverrides.begin(); it != enemyCullOverrides.end();) {
        if (!currentSet.contains(*it)) {
            it = enemyCullOverrides.erase(it);
        } else {
            ++it;
        }
    }

    for (Actor* actor : currentEnemies) {
        bool remoteNear = false;
        for (auto& [clientId, client] : clients) {
            if (!client.online || client.self || !client.isSaveLoaded) {
                continue;
            }
            if (client.sceneNum != gPlayState->sceneNum || client.curRoomNum != gPlayState->roomCtx.curRoom.num) {
                continue;
            }
            float range = actor->uncullZoneForward + actor->uncullZoneScale;
            if (AnchorVec3fDistSq(actor->world.pos, client.posRot.pos) < SQ(range)) {
                remoteNear = true;
                break;
            }
        }

        if (remoteNear) {
            if (!(actor->flags & ACTOR_FLAG_UPDATE_CULLING_DISABLED)) {
                actor->flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED;
                enemyCullOverrides.insert(actor);
            }
        } else if (enemyCullOverrides.contains(actor)) {
            actor->flags &= ~ACTOR_FLAG_UPDATE_CULLING_DISABLED;
            enemyCullOverrides.erase(actor);
        }
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
        if (!client.online || client.self || !client.isSaveLoaded || !client.roomStable ||
            client.clientVersion != clientVersion || client.enemySessionId == 0) {
            continue;
        }
        if (client.sceneNum == sceneNum && client.curRoomNum == roomNum && clientId < authorityClientId) {
            authorityClientId = clientId;
        }
    }

    if (!enemyRoomAuthorities.contains(roomKey)) {
        enemyRoomAuthorities[roomKey] = authorityClientId;
        enemyRoomAuthorityGenerations[roomKey] = authorityClientId;
    } else if (enemyRoomAuthorities[roomKey] != authorityClientId) {
        enemyRoomAuthorities[roomKey] = authorityClientId;
        // Every client derives the same epoch even if it observes a different number of intermediate elections.
        // Snapshot session/sequence fields provide ordering when the same client later becomes authority again.
        enemyRoomAuthorityGenerations[roomKey] = authorityClientId;
    }
    enemyRoomAuthoritySessions[roomKey] = authorityClientId == ownClientId
                                             ? enemySessionId
                                             : (clients.contains(authorityClientId)
                                                    ? clients[authorityClientId].enemySessionId
                                                    : 0);

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
    uint64_t authoritySessionId = payload.value("enemySessionId", (uint64_t)0);

    if (sceneNum != gPlayState->sceneNum || roomNum != gPlayState->roomCtx.curRoom.num) {
        return false;
    }
    if (authorityClientId == 0) {
        authorityClientId = clientId;
    }
    if (clientId != authorityClientId || authorityClientId != GetEnemySyncAuthorityClientId(sceneNum, roomNum)) {
        return false;
    }

    uint32_t localGeneration = GetEnemyRoomAuthorityGeneration(sceneNum, roomNum);
    if (authorityGeneration == 0) {
        return false;
    }
    if (authorityGeneration != localGeneration) {
        return false;
    }
    uint64_t expectedSessionId = enemyRoomAuthoritySessions[GetEnemyRoomKey(sceneNum, roomNum)];
    if (authoritySessionId == 0 || authoritySessionId != expectedSessionId) {
        return false;
    }

    return true;
}

uint64_t Anchor::NextEnemySnapshotSequence(s16 sceneNum, s8 roomNum) {
    uint64_t& sequence = enemySnapshotSequences[GetEnemyRoomKey(sceneNum, roomNum)];
    sequence++;
    if (sequence == 0) {
        sequence++;
    }
    return sequence;
}

bool Anchor::IsNewEnemySnapshotPacket(const nlohmann::json& payload) {
    uint64_t sequence = payload.value("snapshotSequence", (uint64_t)0);
    uint64_t sessionId = payload.value("enemySessionId", (uint64_t)0);
    if (sequence == 0 || sessionId == 0) {
        return false;
    }

    s16 sceneNum = payload.value("sceneNum", (s16)SCENE_ID_MAX);
    s8 roomNum = payload.value("roomNum", (s8)-1);
    uint32_t authorityClientId = payload.value("authorityClientId", payload.value("clientId", (uint32_t)0));
    uint64_t streamKey = ((uint64_t)GetEnemyRoomKey(sceneNum, roomNum) << 32) | authorityClientId;
    uint64_t& lastSequence = lastEnemySnapshotSequences[sessionId][streamKey];
    if (sequence <= lastSequence) {
        return false;
    }
    lastSequence = sequence;
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

    uint32_t sceneKey = (uint16_t)sceneNum;
    auto& tombstones = deadEnemyLedger[sceneKey];
    bool inserted = tombstones.insert(networkId).second;
    if (inserted && (networkId & (1ULL << 63)) == 0) {
        constexpr size_t MAX_DYNAMIC_TOMBSTONES_PER_SCENE = 2048;
        auto& order = dynamicEnemyTombstoneOrder[sceneKey];
        order.push_back(networkId);
        while (order.size() > MAX_DYNAMIC_TOMBSTONES_PER_SCENE) {
            tombstones.erase(order.front());
            order.pop_front();
        }
    }
    freshEnemyAuthorityData.erase(networkId);
    pendingEnemyDamageOperations.erase(networkId);
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

    uint32_t sceneKey = (uint16_t)sceneNum;
    return deadEnemyLedger.contains(sceneKey) && deadEnemyLedger[sceneKey].contains(networkId);
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

static void ReleaseLocalGohmaDefeatCutsceneBeforeKill(Actor* actor) {
    if (actor == nullptr || actor->id != ACTOR_BOSS_GOMA || gPlayState == nullptr) {
        return;
    }

    Player* player = GET_PLAYER(gPlayState);
    if (player == nullptr || player->csActor != actor) {
        return;
    }

    BossGoma* goma = (BossGoma*)actor;
    if (goma->subCameraId >= SUBCAM_FIRST && goma->subCameraId < NUM_CAMS &&
        Play_GetCamera(gPlayState, goma->subCameraId) != nullptr) {
        Camera* mainCamera = Play_GetCamera(gPlayState, MAIN_CAM);
        if (mainCamera != nullptr) {
            mainCamera->eye = goma->subCameraEye;
            mainCamera->eyeNext = goma->subCameraEye;
            mainCamera->at = goma->subCameraAt;
        }
        func_800C08AC(gPlayState, goma->subCameraId, 0);
    } else if (Play_GetCamera(gPlayState, MAIN_CAM) != nullptr) {
        Play_ChangeCameraStatus(gPlayState, MAIN_CAM, CAM_STAT_ACTIVE);
    }

    goma->subCameraId = 0;
    func_80064534(gPlayState, &gPlayState->csCtx);
    Player_SetCsActionWithHaltedActors(gPlayState, actor, 7);
}

static bool EnemyKillBufferContains(const std::vector<uint64_t>& buffer, uint64_t networkId) {
    for (uint64_t bufferedId : buffer) {
        if (bufferedId == networkId) {
            return true;
        }
    }

    return false;
}

void Anchor::QueueEnemyKill(uint64_t networkId) {
    if (networkId != 0 && !EnemyKillBufferContains(enemyKillBuffer, networkId)) {
        enemyKillBuffer.push_back(networkId);
    }
}

void Anchor::ReportLocalEnemyDamage(Actor* actor, u8 health) {
    if (actor == nullptr || HasEnemySyncAuthority()) {
        return;
    }

    uint64_t networkId = GetEnemyNetworkId(actor);
    auto tracked = enemyHealthTracker.find(actor);
    if (networkId == 0 || IsEnemyMarkedDead(networkId) || tracked == enemyHealthTracker.end() ||
        health >= tracked->second) {
        return;
    }

    uint32_t authorityClientId = GetEnemySyncAuthorityClientId();
    if (authorityClientId == 0 || authorityClientId == ownClientId) {
        return;
    }

    uint64_t operationId = ++nextEnemyOperationId;
    if (operationId == 0) {
        operationId = ++nextEnemyOperationId;
    }
    PendingEnemyDamageOperation operation = {
        { ownClientId, enemySessionId, operationId },
        static_cast<u8>(tracked->second - health),
        authorityClientId,
        GetEnemyRoomAuthorityGeneration(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num),
        gPlayState->state.frames,
        nlohmann::json::object(),
    };
    SendPacket_ReportEnemyDamageOperation(actor, health, operation.damageAmount, operation);
    pendingEnemyDamageOperations[networkId].push_back(operation);
    enemyHealthTracker[actor] = health;
}

void Anchor::ResendPendingEnemyDamageOperations() {
    if (!IsRoomStable() || pendingEnemyDamageOperations.empty()) {
        return;
    }

    uint32_t authorityClientId = GetEnemySyncAuthorityClientId();
    uint32_t authorityGeneration =
        GetEnemyRoomAuthorityGeneration(gPlayState->sceneNum, gPlayState->roomCtx.curRoom.num);
    if (authorityClientId == 0 || authorityGeneration == 0) {
        return;
    }

    if (authorityClientId == ownClientId) {
        // The local actor already contains these hits. Adopt their keys when authority changes instead of
        // subtracting them again if an old report is relayed later.
        std::vector<nlohmann::json> fatalOperationContexts;
        for (auto it = pendingEnemyDamageOperations.begin(); it != pendingEnemyDamageOperations.end();) {
            uint64_t networkId = it->first;
            auto& operations = it->second;
            if (operations.empty() ||
                operations.front().reportPayload.value("sceneNum", (s16)SCENE_ID_MAX) != gPlayState->sceneNum ||
                operations.front().reportPayload.value("roomNum", (s8)-1) != gPlayState->roomCtx.curRoom.num) {
                ++it;
                continue;
            }
            auto& appliedOperations = appliedEnemyDamageOperations[networkId];
            Actor* actor = FindActorByEnemyNetworkId(networkId);
            for (const PendingEnemyDamageOperation& operation : operations) {
                if (appliedOperations.size() >= 512) {
                    appliedOperations.erase(appliedOperations.begin());
                }
                appliedOperations.insert(operation.key);
                if (actor != nullptr && !IsEnemyMarkedDead(networkId)) {
                    SendPacket_DamageEnemy(actor, actor->colChkInfo.health, &operation.key);
                } else if (operation.reportPayload.value("health", (u8)1) == 0 &&
                           !IsEnemyMarkedDead(networkId)) {
                    fatalOperationContexts.push_back(operation.reportPayload);
                }
            }
            if (actor != nullptr) {
                enemyHealthTracker[actor] = actor->colChkInfo.health;
            }
            it = pendingEnemyDamageOperations.erase(it);
        }
        for (const nlohmann::json& context : fatalOperationContexts) {
            SendPacket_KillEnemy(context);
        }
        return;
    }

    constexpr u32 DAMAGE_OPERATION_RETRY_FRAMES = 30;
    u32 currentFrame = gPlayState->state.frames;
    for (auto it = pendingEnemyDamageOperations.begin(); it != pendingEnemyDamageOperations.end();) {
        uint64_t networkId = it->first;
        auto& operations = it->second;
        if (!operations.empty() &&
            (operations.front().reportPayload.value("sceneNum", (s16)SCENE_ID_MAX) != gPlayState->sceneNum ||
             operations.front().reportPayload.value("roomNum", (s8)-1) != gPlayState->roomCtx.curRoom.num)) {
            ++it;
            continue;
        }
        Actor* actor = FindActorByEnemyNetworkId(networkId);
        if (IsEnemyMarkedDead(networkId)) {
            it = pendingEnemyDamageOperations.erase(it);
            continue;
        }
        for (PendingEnemyDamageOperation& operation : operations) {
            bool authorityChanged = operation.authorityClientId != authorityClientId ||
                                    operation.authorityGeneration != authorityGeneration;
            if (!authorityChanged && currentFrame - operation.lastSentFrame < DAMAGE_OPERATION_RETRY_FRAMES) {
                continue;
            }
            operation.authorityClientId = authorityClientId;
            operation.authorityGeneration = authorityGeneration;
            operation.lastSentFrame = currentFrame;
            u8 reportedHealth = actor == nullptr ? operation.reportPayload.value("health", (u8)0)
                                                 : actor->colChkInfo.health;
            SendPacket_ReportEnemyDamageOperation(actor, reportedHealth, operation.damageAmount, operation);
        }
        ++it;
    }
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
        if (actor == nullptr || actor->update == nullptr) {
            enemyDeathDeferralFrames.erase(networkId);
            continue;
        }
        if (ShouldDeferKillForLocalDialogue(actor)) {
            if (!EnemyKillBufferContains(deferredKillBuffer, networkId)) {
                deferredKillBuffer.push_back(networkId);
            }
            continue;
        }

        // Death animation deferral: when an enemy is killed via the network (either a
        // replica-initiated kill reaching the authority, or a KillEnemy arriving at a
        // replica), the raw health=0 + Actor_Kill path skips the enemy's native death
        // animation entirely. Defer the kill for up to 60 frames while
        // EnsureEnemyDeathSetup triggers the enemy's death state. This applies to BOTH
        // authority and replica: the authority needs it for replica-initiated kills, and
        // replicas need it for authority-initiated kills. Only enemies and bosses get
        // deferral — props (pots, torches, etc.) should be destroyed immediately.
        if (actor->colChkInfo.health == 0 &&
            (actor->category == ACTORCAT_ENEMY || actor->category == ACTORCAT_BOSS)) {
            u32& frames = enemyDeathDeferralFrames[networkId];
            bool deathStateActive = (actor->flags & ACTOR_FLAG_ATTENTION_ENABLED) == 0;
            if (frames == 0) {
                if (enemyExtraStates.contains(networkId)) {
                    ApplyEnemyExtraState(actor, enemyExtraStates[networkId]);
                }
                deathStateActive = deathStateActive || (actor->flags & ACTOR_FLAG_ATTENTION_ENABLED) == 0 ||
                                   EnsureEnemyDeathSetup(actor, gPlayState);
            }
            if (deathStateActive && frames < 60) {
                frames++;
                if (!EnemyKillBufferContains(deferredKillBuffer, networkId)) {
                    deferredKillBuffer.push_back(networkId);
                }
                continue;
            }
        }

        ReleaseLocalGohmaDefeatCutsceneBeforeKill(actor);
        // Play break animation for pots/crates so they shatter visually instead of just disappearing
        if (actor->id == ACTOR_OBJ_TSUBO) {
            ObjTsubo_AirBreak((ObjTsubo*)actor, gPlayState);
        } else if (actor->id == ACTOR_OBJ_KIBAKO) {
            ObjKibako_AirBreak((ObjKibako*)actor, gPlayState);
        }
        Actor_Kill(actor);
        enemyDeathDeferralFrames.erase(networkId);
    }
    enemyKillBuffer.insert(enemyKillBuffer.end(), deferredKillBuffer.begin(), deferredKillBuffer.end());

    // Prune buffer: kill synced actors that the authority doesn't know about (no network ID).
    // A grace period after room entry prevents newly-loaded actors from being killed before
    // the SendRoomEnemies matching has had time to assign their network IDs.
    static s32 pruneGraceFrames = 0;
    static s16 lastPruneScene = -1;
    static s8 lastPruneRoom = -1;
    if (gPlayState->sceneNum != lastPruneScene || gPlayState->roomCtx.curRoom.num != lastPruneRoom) {
        lastPruneScene = gPlayState->sceneNum;
        lastPruneRoom = gPlayState->roomCtx.curRoom.num;
        pruneGraceFrames = 180; // 3 seconds at 60 FPS
    }
    if (pruneGraceFrames > 0) {
        pruneGraceFrames--;
        enemyPruneBuffer.clear();
    } else {
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
    }

    while (!enemySpawnBuffer.empty()) {
        auto& [actorId, params, pos] = enemySpawnBuffer.front();
        Actor* spawned = Actor_Spawn(&gPlayState->actorCtx, gPlayState, actorId,
                                      pos.x, pos.y, pos.z, 0, 0, 0, params);
        enemySpawnBuffer.erase(enemySpawnBuffer.begin());
    }
}

void Anchor::UpdateHorsePuppets() {
    if (!IsSaveLoaded()) {
        return;
    }

    for (auto& [clientId, client] : clients) {
        if (client.self) {
            continue;
        }

        // The scene change frees actors without telling us; drop dangling pointers first.
        if (client.horse != nullptr && !AnchorIsActorInCurrentLists(client.horse)) {
            client.horse = nullptr;
        }

        bool ridingHere = client.online && client.isSaveLoaded && client.sceneNum == gPlayState->sceneNum &&
                          (client.stateFlags1 & PLAYER_STATE1_ON_HORSE);
        if (!ridingHere) {
            if (client.horse != nullptr) {
                Actor_Kill(client.horse);
                client.horse = nullptr;
            }
            continue;
        }

        if (client.horse == nullptr) {
            // Fails harmlessly (and is retried) in scenes without the horse object loaded.
            client.horse =
                Actor_Spawn(&gPlayState->actorCtx, gPlayState, ACTOR_EN_HORSE, client.posRot.pos.x,
                            client.posRot.pos.y - 43.0f, client.posRot.pos.z, 0, client.posRot.rot.y, 0,
                            ENHORSE_PUPPET_PARAMS);
        }

        if (client.horse != nullptr) {
            EnHorse* horse = (EnHorse*)client.horse;
            // Anchor the saddle under the rider: riderPos is the saddle-bone offset computed during draw.
            // Until the first draw has run it can hold a garbage absolute position, so fall back to a fixed
            // saddle height when it looks implausible.
            Vec3f saddleOffset = horse->riderPos;
            if (fabsf(saddleOffset.y) > 200.0f || fabsf(saddleOffset.x) > 200.0f || fabsf(saddleOffset.z) > 200.0f) {
                saddleOffset = { 0.0f, 70.0f, 0.0f };
            }
            horse->actor.world.pos.x = client.posRot.pos.x - saddleOffset.x;
            horse->actor.world.pos.y = client.posRot.pos.y + 27.0f - saddleOffset.y;
            horse->actor.world.pos.z = client.posRot.pos.z - saddleOffset.z;
            horse->actor.shape.rot.y = client.posRot.rot.y;
            horse->actor.speedXZ = client.moveSpeed;
        }
    }
}

void Anchor::DetectEnemyDamage() {
    if (!IsSaveLoaded()) {
        return;
    }

    ResendPendingEnemyDamageOperations();

    std::vector<Actor*> currentEnemies;

    for (s32 cat = ACTORCAT_SWITCH; cat < ACTORCAT_MAX; cat++) {
        Actor* currAct = gPlayState->actorCtx.actorLists[cat].head;
        while (currAct != nullptr) {
            if ((IsEnemySyncActor(currAct) || GetEnemyNetworkId(currAct) != 0) && IsActorInCurrentEnemyRoom(currAct)) {
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
    AssignEnemyNetworkIds(assignableEnemies);

    UpdateEnemyCullOverrides(currentEnemies);

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

        uint64_t networkId = GetEnemyNetworkId(act);
        if (IsEnemyMarkedDead(networkId)) {
            enemyHealthTracker[act] = currentHealth;
            continue;
        }

        if (enemyHealthTracker.contains(act)) {
            u8 lastHealth = enemyHealthTracker[act];
            nlohmann::json authorityExtra =
                enemyExtraStates.contains(networkId) ? enemyExtraStates[networkId] : nlohmann::json::object();
            if (currentHealth < lastHealth) {
                if (HasEnemySyncAuthority()) {
                    SendPacket_DamageEnemy(act, currentHealth);
                } else {
                    ReportLocalEnemyDamage(act, currentHealth);
                }
            } else if (!HasEnemySyncAuthority() && currentHealth == lastHealth &&
                       ShouldPreserveLocalEnemyExtraState(act, authorityExtra) && ShouldReportEnemyExtraState(act)) {
                SendPacket_ReportEnemyState(act);
            }
        }

        bool isCarryable = act->id == ACTOR_EN_NIW || act->id == ACTOR_OBJ_TSUBO ||
                            act->id == ACTOR_OBJ_KIBAKO || act->id == ACTOR_EN_BOMBF || act->id == ACTOR_EN_BOM;
        if (isCarryable && networkId != 0) {
            bool isHeldByLocalPlayer = act->parent != nullptr && act->parent != act;
            bool wasHeldByLocalPlayer = previouslyHeldEnemyIds.contains(networkId);
            EnemyCarryOwnershipState& ownership = enemyCarryOwnership[networkId];

            if (HasEnemySyncAuthority()) {
                if (isHeldByLocalPlayer && ownership.ownerClientId != ownClientId) {
                    ownership.ownerClientId = ownClientId;
                    ownership.generation++;
                    if (ownership.generation == 0) {
                        ownership.generation++;
                    }
                } else if (!isHeldByLocalPlayer && ownership.ownerClientId == ownClientId) {
                    ownership.ownerClientId = 0;
                    ownership.generation++;
                    if (ownership.generation == 0) {
                        ownership.generation++;
                    }
                } else if (ownership.ownerClientId != 0 && ownership.ownerClientId != ownClientId &&
                           (!clients.contains(ownership.ownerClientId) || !clients[ownership.ownerClientId].online ||
                            clients[ownership.ownerClientId].sceneNum != gPlayState->sceneNum ||
                            clients[ownership.ownerClientId].curRoomNum != gPlayState->roomCtx.curRoom.num)) {
                    ownership.ownerClientId = 0;
                    ownership.generation++;
                    if (act->parent == act) {
                        act->parent = nullptr;
                    }
                }
            } else if (isHeldByLocalPlayer && !wasHeldByLocalPlayer) {
                previouslyHeldEnemyIds.insert(networkId);
                SendPacket_ReportEnemyState(act);
            } else if (isHeldByLocalPlayer && ownership.ownerClientId != ownClientId &&
                       gPlayState->state.frames % 10 == 0) {
                SendPacket_ReportEnemyState(act);
            } else if (!isHeldByLocalPlayer && wasHeldByLocalPlayer) {
                previouslyHeldEnemyIds.erase(networkId);
                SendPacket_ReportEnemyState(act);
            } else if (!isHeldByLocalPlayer && ownership.ownerClientId == ownClientId &&
                       gPlayState->state.frames % 10 == 0) {
                // A quick pickup/release can race the acquire acknowledgement. Keep requesting release until the
                // authority snapshot confirms that this client no longer owns the actor.
                SendPacket_ReportEnemyState(act);
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
