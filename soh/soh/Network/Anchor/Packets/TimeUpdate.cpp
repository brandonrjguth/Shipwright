#include "soh/Network/Anchor/Anchor.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>

extern "C" {
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
}

/**
 * TIME_UPDATE
 *
 * Keeps the day/night cycle shared. The time authority (lowest clientId with a save loaded) broadcasts its
 * dayTime once a second and everyone else adopts it; a non-authority client that changes time abruptly
 * (Sun's Song, sleeping) broadcasts a jump, which the authority adopts and then streams back out.
 */

void Anchor::SendPacket_TimeUpdate(bool isJump) {
    if (!IsSaveLoaded()) {
        return;
    }

    nlohmann::json payload;
    payload["type"] = TIME_UPDATE;
    payload["dayTime"] = gSaveContext.dayTime;
    payload["jump"] = isJump;
    payload["quiet"] = true;

    SendJsonToRemote(payload);
}

void Anchor::HandlePacket_TimeUpdate(nlohmann::json payload) {
    if (!IsSaveLoaded()) {
        return;
    }

    uint32_t clientId = payload.value("clientId", (uint32_t)0);
    bool fromAuthority = clientId != 0 && clientId == GetTimeSyncAuthorityClientId();
    if (!fromAuthority && !payload.value("jump", false)) {
        return;
    }

    u16 dayTime = payload.value("dayTime", gSaveContext.dayTime);
    gSaveContext.dayTime = dayTime;
    gSaveContext.skyboxTime = dayTime;
    // Track the adopted value so the local jump detector only reacts to changes the local game made.
    lastLocalDayTime = dayTime;
}
