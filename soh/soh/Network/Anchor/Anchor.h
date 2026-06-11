#ifndef NETWORK_ANCHOR_H
#define NETWORK_ANCHOR_H
#ifdef __cplusplus

#include "soh/Network/Network.h"
#include <libultraship/libultraship.h>
#include <queue>
#include <mutex>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

extern "C" {
#include "variables.h"
#include "z64.h"
}

void DummyPlayer_Init(Actor* actor, PlayState* play);
void DummyPlayer_Update(Actor* actor, PlayState* play);
void DummyPlayer_Draw(Actor* actor, PlayState* play);
void DummyPlayer_Destroy(Actor* actor, PlayState* play);

typedef struct {
    uint32_t clientId;
    std::string name;
    Color_RGB8 color;
    std::string clientVersion;
    std::string teamId;
    bool online;
    bool self;
    uint32_t seed;
    bool isSaveLoaded;
    bool isGameComplete;
    bool roomStable;
    s16 sceneNum;
    s8 curRoomNum;
    s32 entranceIndex;

    // Only available in PLAYER_UPDATE packets
    s32 linkAge;
    PosRot posRot;
    Vec3s jointTable[24];
    u8 movementFlags;
    Vec3s prevTransl;
    Vec3s upperLimbRot;
    s8 currentBoots;
    s8 currentShield;
    s8 currentTunic;
    u32 stateFlags1;
    u32 stateFlags2;
    u8 buttonItem0;
    s8 itemAction;
    s8 heldItemAction;
    u8 modelGroup;
    s8 invincibilityTimer;
    f32 unk_85C;
    s16 unk_862;
    s8 actionVar1;
    u8 ocarinaNote;
    f32 ocarinaModulator;
    s8 ocarinaBend;
    u8 stableRoomFrames;

    // Ptr to the dummy player
    Player* player;
} AnchorClient;

typedef struct {
    uint32_t ownerClientId;
    u8 pvpMode;           // 0 = off, 1 = on, 2 = on with friendly fire
    u8 showLocationsMode; // 0 = none, 1 = team, 2 = all
    u8 teleportMode;      // 0 = off, 1 = team, 2 = all
    u8 syncItemsAndFlags; // 0 = off, 1 = on
} RoomState;

typedef struct {
    s16 actorId;
    ActorCategory category;
    Vec3f pos;
    Vec3s worldRot;
    Vec3s shapeRot;
    Vec3f scale;
    Vec3f velocity;
    f32 speedXZ;
    f32 gravity;
    f32 minVelocityY;
    s16 yawTowardsPlayer;
    f32 xzDistToPlayer;
    f32 yDistToPlayer;
    f32 xyzDistToPlayerSq;
    u16 freezeTimer;
    u8 colorFilterTimer;
    u16 colorFilterParams;
    u8 health;
} EnemyAuthorityState;

class Anchor : public Network {
  private:
    uint32_t spawningDummyPlayerForClientId = 0;
    bool shouldRefreshActors = false;
    bool justLoadedSave = false;
    bool isHandlingUpdateTeamState = false;
    bool isProcessingIncomingPacket = false;
    std::queue<nlohmann::json> incomingPacketQueue;
    std::mutex incomingPacketQueueMutex;
    std::queue<nlohmann::json> outgoingPacketQueue;
    std::mutex outgoingPacketQueueMutex;
    uint64_t spawningNetworkedEnemyDropId = 0;
    std::vector<uint64_t> enemyKillBuffer;
    std::vector<std::tuple<Actor*, s16, s8>> enemyPruneBuffer;
    std::vector<std::tuple<s16, s16, Vec3f>> enemySpawnBuffer;
    std::unordered_map<Actor*, u8> enemyHealthTracker;
    std::unordered_map<uint64_t, EnemyAuthorityState> enemyAuthorityTargets;
    std::unordered_map<uint64_t, nlohmann::json> enemyExtraStates;
    // NetworkIds whose authority state arrived since it was last applied. Replicas consume an entry at most once per
    // frame (right before the actor updates) so stale snapshots never drag a moving actor backwards.
    std::unordered_set<uint64_t> freshEnemyAuthorityData;
    // Enemies we forced to keep updating because a remote player is near them; cleared when they leave.
    std::unordered_set<Actor*> enemyCullOverrides;
    std::unordered_map<uint32_t, uint32_t> enemyRoomAuthorities;
    std::unordered_map<uint32_t, uint32_t> enemyRoomAuthorityGenerations;
    std::unordered_map<uint32_t, std::unordered_set<uint64_t>> deadEnemyLedger;
    std::unordered_map<uint64_t, uint16_t> enemyDropCounters;
    uint32_t transientEnemyCounter = 0;
    std::unordered_set<uint64_t> suppressedTransientProjectileKills;
    std::unordered_set<uint64_t> hintnutsDialogueActive;
    s16 enemySyncSceneNum = SCENE_ID_MAX;
    s8 enemySyncRoomNum = -1;
    bool enemyRoomSyncPending = true;
    u8 enemyTransformFrameCounter = 0;

    nlohmann::json PrepClientState();
    nlohmann::json PrepRoomState();
    void RegisterHooks();
    void RefreshClientActors();
    void SetDummyPlayerClientId(const Actor* actor, uint32_t clientId);
    Actor* FindClosestActorByCategoryAndId(ActorCategory category, s16 actorId, Vec3f pos);
    Actor* FindClosestUnassignedActorByCategoryAndId(ActorCategory category, s16 actorId, Vec3f pos, float maxDistSq,
                                                     s16 actorParams = (s16)-0x8000);
    Actor* FindActorByEnemyNetworkId(uint64_t networkId);
    Actor* FindNearbyDeadEnemyDropSource(Actor* dropActor);
    bool IsEnemySyncActor(Actor* actor);
    bool IsEnemySyncActor(ActorCategory category, s16 actorId);
    uint64_t GetEnemyNetworkId(Actor* actor);
    uint64_t CreateEnemyDropNetworkId(Actor* source, Actor* dropActor);
    void SetEnemyNetworkId(Actor* actor, uint64_t networkId);
    void AssignEnemyNetworkIds(std::vector<Actor*> actors);
    bool IsRoomStable();
    void ResetEnemyRoomTransientState();
    void DetectEnemyRoomChange();
    void ProcessActorBuffers();
    void DetectEnemyDamage();
    void ApplyEnemyAuthorityState(Actor* actor, EnemyAuthorityState state, bool immediate);
    bool ConsumeFreshEnemyAuthorityData(uint64_t networkId);
    void UpdateEnemyCullOverrides(const std::vector<Actor*>& currentEnemies);
    uint32_t GetEnemyRoomKey(s16 sceneNum, s8 roomNum);
    uint32_t GetEnemyRoomAuthorityGeneration(s16 sceneNum, s8 roomNum);
    uint32_t GetEnemySyncAuthorityClientId();
    uint32_t GetEnemySyncAuthorityClientId(s16 sceneNum, s8 roomNum);
    bool HasEnemySyncAuthority();
    bool HasEnemySyncAuthority(s16 sceneNum, s8 roomNum);
    bool IsValidEnemyAuthorityPacket(nlohmann::json payload);
    void MarkEnemyDead(uint64_t networkId);
    void MarkEnemyDead(s16 sceneNum, s8 roomNum, uint64_t networkId);
    bool IsEnemyMarkedDead(uint64_t networkId);
    bool IsEnemyMarkedDead(s16 sceneNum, s8 roomNum, uint64_t networkId);

    void HandlePacket_AllClientState(nlohmann::json payload);
    void HandlePacket_ConsumeAdultTradeItem(nlohmann::json payload);
    void HandlePacket_DamagePlayer(nlohmann::json payload);
    void HandlePacket_DisableAnchor(nlohmann::json payload);
    void HandlePacket_EntranceDiscovered(nlohmann::json payload);
    void HandlePacket_GameComplete(nlohmann::json payload);
    void HandlePacket_GiveItem(nlohmann::json payload);
    void HandlePacket_OcarinaSfx(nlohmann::json payload);
    void HandlePacket_PlayerSfx(nlohmann::json payload);
    void HandlePacket_PlayerUpdate(nlohmann::json payload);
    void HandlePacket_RequestTeamState(nlohmann::json payload);
    void HandlePacket_RequestTeleport(nlohmann::json payload);
    void HandlePacket_ServerMessage(nlohmann::json payload);
    void HandlePacket_SetCheckStatus(nlohmann::json payload);
    void HandlePacket_SetFlag(nlohmann::json payload);
    void HandlePacket_TeleportTo(nlohmann::json payload);
    void HandlePacket_UnsetFlag(nlohmann::json payload);
    void HandlePacket_UpdateBeansCount(nlohmann::json payload);
    void HandlePacket_UpdateClientState(nlohmann::json payload);
    void HandlePacket_UpdateDungeonItems(nlohmann::json payload);
    void HandlePacket_UpdateRoomState(nlohmann::json payload);
    void HandlePacket_UpdateTeamState(nlohmann::json payload);
    void HandlePacket_DamageEnemy(nlohmann::json payload);
    void HandlePacket_KillEnemy(nlohmann::json payload);
    void HandlePacket_RequestRoomEnemies(nlohmann::json payload);
    void HandlePacket_SendRoomEnemies(nlohmann::json payload);
    void HandlePacket_EnemyUpdate(nlohmann::json payload);
    void HandlePacket_EnemyEvent(nlohmann::json payload);
    void HandlePacket_ReportEnemyDamage(nlohmann::json payload);
    void HandlePacket_HintnutsDialogue(nlohmann::json payload);

  public:
    uint32_t ownClientId;
    inline static const std::string clientVersion = (char*)gGitCommitHash;

    // Packet types //
    inline static const std::string ALL_CLIENT_STATE = "ALL_CLIENT_STATE";
    inline static const std::string DAMAGE_PLAYER = "DAMAGE_PLAYER";
    inline static const std::string DISABLE_ANCHOR = "DISABLE_ANCHOR";
    inline static const std::string ENTRANCE_DISCOVERED = "ENTRANCE_DISCOVERED";
    inline static const std::string GAME_COMPLETE = "GAME_COMPLETE";
    inline static const std::string GIVE_ITEM = "GIVE_ITEM";
    inline static const std::string HANDSHAKE = "HANDSHAKE";
    inline static const std::string OCARINA_SFX = "OCARINA_SFX";
    inline static const std::string PLAYER_SFX = "PLAYER_SFX";
    inline static const std::string PLAYER_UPDATE = "PLAYER_UPDATE";
    inline static const std::string REQUEST_TEAM_STATE = "REQUEST_TEAM_STATE";
    inline static const std::string REQUEST_TELEPORT = "REQUEST_TELEPORT";
    inline static const std::string SERVER_MESSAGE = "SERVER_MESSAGE";
    inline static const std::string SET_CHECK_STATUS = "SET_CHECK_STATUS";
    inline static const std::string SET_FLAG = "SET_FLAG";
    inline static const std::string TELEPORT_TO = "TELEPORT_TO";
    inline static const std::string UNSET_FLAG = "UNSET_FLAG";
    inline static const std::string UPDATE_BEANS_COUNT = "UPDATE_BEANS_COUNT";
    inline static const std::string UPDATE_CLIENT_STATE = "UPDATE_CLIENT_STATE";
    inline static const std::string UPDATE_DUNGEON_ITEMS = "UPDATE_DUNGEON_ITEMS";
    inline static const std::string UPDATE_ROOM_STATE = "UPDATE_ROOM_STATE";
    inline static const std::string UPDATE_TEAM_STATE = "UPDATE_TEAM_STATE";
    inline static const std::string DAMAGE_ENEMY = "DAMAGE_ENEMY";
    inline static const std::string KILL_ENEMY = "KILL_ENEMY";
    inline static const std::string REQUEST_ROOM_ENEMIES = "REQUEST_ROOM_ENEMIES";
    inline static const std::string SEND_ROOM_ENEMIES = "SEND_ROOM_ENEMIES";
    inline static const std::string ENEMY_UPDATE = "ENEMY_UPDATE";
    inline static const std::string ENEMY_EVENT = "ENEMY_EVENT";
    inline static const std::string REPORT_ENEMY_DAMAGE = "REPORT_ENEMY_DAMAGE";
    inline static const std::string HINTNUTS_DIALOGUE = "HINTNUTS_DIALOGUE";

    static Anchor* Instance;
    std::map<uint32_t, AnchorClient> clients;
    RoomState roomState;

    void Enable();
    void Disable();
    void OnIncomingJson(nlohmann::json payload);
    void OnConnected();
    void OnDisconnected();
    void ProcessOutgoingPackets();
    void DrawMenu();
    void ProcessIncomingPacketQueue();
    void SendJsonToRemote(nlohmann::json packet);
    bool IsSaveLoaded();
    bool CanTeleportTo(uint32_t clientId);
    uint32_t GetDummyPlayerClientId(const Actor* actor);

    void SendPacket_ClearTeamState(std::string teamId);
    void SendPacket_DamagePlayer(u32 clientId, u8 damageEffect, u8 damage);
    void SendPacket_EntranceDiscovered(u16 entranceIndex);
    void SendPacket_GameComplete();
    void SendPacket_GiveItem(u16 modId, s16 getItemId);
    void SendPacket_Handshake();
    void SendPacket_OcarinaSfx(uint8_t note, float modulator, int8_t bend);
    void SendPacket_PlayerSfx(u16 sfxId);
    void SendPacket_PlayerUpdate();
    void SendPacket_RequestTeamState();
    void SendPacket_RequestTeleport(u32 clientId);
    void SendPacket_SetCheckStatus(RandomizerCheck rc);
    void SendPacket_SetFlag(s16 sceneNum, s16 flagType, s16 flag);
    void SendPacket_TeleportTo(u32 clientId);
    void SendPacket_UnsetFlag(s16 sceneNum, s16 flagType, s16 flag);
    void SendPacket_UpdateBeansCount();
    void SendPacket_UpdateClientState();
    void SendPacket_UpdateDungeonItems();
    void SendPacket_UpdateRoomState();
    void SendPacket_UpdateTeamState();
    void SendPacket_DamageEnemy(Actor* actor, u8 health);
    void SendPacket_KillEnemy(Actor* actor);
    void SendPacket_RequestRoomEnemies();
    void SendPacket_SendRoomEnemies(u32 targetClientId, ActorCategory category);
    void SendPacket_EnemyUpdate(std::vector<Actor*> actors);
    void SendPacket_EnemyEvent(Actor* actor, std::string eventType, nlohmann::json eventData);
    void SendPacket_ReportEnemyDamage(Actor* actor, u8 health);
    void SendPacket_HintnutsDialogue(Actor* actor, std::string phase);
};

typedef enum {
    // Starting at 5 to continue from the last value in the PlayerDamageResponseType enum
    DUMMY_PLAYER_HIT_RESPONSE_STUN = 5,
    DUMMY_PLAYER_HIT_RESPONSE_FIRE,
    DUMMY_PLAYER_HIT_RESPONSE_NORMAL,
} DummyPlayerDamageResponseType;

class AnchorRoomWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override{};
    void DrawElement() override;
    void Draw() override;
    void UpdateElement() override{};
};

#endif // __cplusplus
#endif // NETWORK_ANCHOR_H
