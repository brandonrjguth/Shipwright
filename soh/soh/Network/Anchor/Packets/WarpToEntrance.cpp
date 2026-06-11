#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>

extern "C" {
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
}

/**
 * WARP_TO_ENTRANCE
 *
 * Sent when a blue warp (or similar story warp) starts a scene transition for the local player. Every other
 * client in the same scene begins the identical transition, so the whole party warps out of the boss room
 * together and everyone sees the post-dungeon cutscene — instead of the first player consuming it and the
 * synced clear-flag routing later players to the cutscene-less entrance.
 */

void Anchor::SendPacket_WarpToEntrance() {
    if (!IsSaveLoaded()) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = WARP_TO_ENTRANCE;
    payload["sceneNum"] = gPlayState->sceneNum;
    payload["entranceIndex"] = gPlayState->nextEntranceIndex;
    payload["cutsceneIndex"] = gSaveContext.nextCutsceneIndex;
    payload["transitionType"] = gPlayState->transitionType;
    payload["nextTransitionType"] = gSaveContext.nextTransitionType;
    payload["quiet"] = true;

    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_WarpToEntrance(nlohmann::json payload) {
    if (!IsSaveLoaded()) {
        return;
    }

    s16 sceneNum = payload.value("sceneNum", (s16)SCENE_ID_MAX);
    if (gPlayState->sceneNum != sceneNum) {
        return;
    }

    // Already transitioning (e.g. our own copy of the warp fired the same frame) — first trigger wins.
    if (gPlayState->transitionTrigger != TRANS_TRIGGER_OFF) {
        return;
    }

    gPlayState->nextEntranceIndex = payload.value("entranceIndex", gPlayState->nextEntranceIndex);
    gSaveContext.nextCutsceneIndex = payload.value("cutsceneIndex", (u16)0);
    gPlayState->transitionTrigger = TRANS_TRIGGER_START;
    gPlayState->transitionType = payload.value("transitionType", (s32)TRANS_TYPE_FADE_WHITE_SLOW);
    gSaveContext.nextTransitionType = payload.value("nextTransitionType", gSaveContext.nextTransitionType);
}
