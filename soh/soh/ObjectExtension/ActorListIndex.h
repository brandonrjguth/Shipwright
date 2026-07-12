#ifndef ACTOR_LIST_INDEX_H
#define ACTOR_LIST_INDEX_H

#ifdef __cplusplus
extern "C" {
#include "z64actor.h"
#endif

int16_t GetActorListIndex(const Actor* actor);
int16_t GetActorListSceneNum(const Actor* actor);
int8_t GetActorListOriginRoom(const Actor* actor);
int32_t GetActorListSceneSetupIndex(const Actor* actor);
int16_t GetActorListSpawnParams(const Actor* actor);
void SetActorListIndex(const Actor* actor, int16_t index);
void SetActorListIndexWithMetadata(const Actor* actor, int16_t index, int16_t sceneNum, int8_t originRoom,
                                   int32_t sceneSetupIndex, int16_t spawnParams);

#ifdef __cplusplus
}
#endif

#endif // ACTOR_LIST_INDEX_H
