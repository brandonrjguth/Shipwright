#include "ActorListIndex.h"
#include "soh/ObjectExtension/ObjectExtension.h"

struct ActorListIndex {
    s16 index = -1;
    s16 sceneNum = -1;
    s8 originRoom = -1;
    s32 sceneSetupIndex = -1;
    s16 spawnParams = 0;
};
static ObjectExtension::Register<ActorListIndex> ActorListIndexRegister;

int16_t GetActorListIndex(const Actor* actor) {
    const ActorListIndex* index = ObjectExtension::GetInstance().Get<ActorListIndex>(actor);
    return index != nullptr ? index->index : ActorListIndex{}.index;
}

int16_t GetActorListSceneNum(const Actor* actor) {
    const ActorListIndex* index = ObjectExtension::GetInstance().Get<ActorListIndex>(actor);
    return index != nullptr ? index->sceneNum : ActorListIndex{}.sceneNum;
}

int8_t GetActorListOriginRoom(const Actor* actor) {
    const ActorListIndex* index = ObjectExtension::GetInstance().Get<ActorListIndex>(actor);
    return index != nullptr ? index->originRoom : ActorListIndex{}.originRoom;
}

int32_t GetActorListSceneSetupIndex(const Actor* actor) {
    const ActorListIndex* index = ObjectExtension::GetInstance().Get<ActorListIndex>(actor);
    return index != nullptr ? index->sceneSetupIndex : ActorListIndex{}.sceneSetupIndex;
}

int16_t GetActorListSpawnParams(const Actor* actor) {
    const ActorListIndex* index = ObjectExtension::GetInstance().Get<ActorListIndex>(actor);
    return index != nullptr ? index->spawnParams : ActorListIndex{}.spawnParams;
}

void SetActorListIndex(const Actor* actor, int16_t index) {
    if (actor != nullptr) {
        ObjectExtension::GetInstance().Set<ActorListIndex>(actor, ActorListIndex{ index });
    }
}

void SetActorListIndexWithMetadata(const Actor* actor, int16_t index, int16_t sceneNum, int8_t originRoom,
                                   int32_t sceneSetupIndex, int16_t spawnParams) {
    if (actor != nullptr) {
        ObjectExtension::GetInstance().Set<ActorListIndex>(
            actor, ActorListIndex{ index, sceneNum, originRoom, sceneSetupIndex, spawnParams });
    }
}
