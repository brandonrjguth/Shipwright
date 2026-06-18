#include "soh/Network/Anchor/Anchor.h"
#include "soh/Network/Anchor/GenericEnemySync.h"
#include <nlohmann/json.hpp>
#include <libultraship/libultraship.h>
#include "soh/ResourceManagerHelpers.h"

#include <cstddef>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>

extern "C" {
#include "macros.h"
#include "functions.h"
#define this thisx
#include "src/overlays/actors/ovl_En_Dodongo/z_en_dodongo.h"
#include "src/overlays/actors/ovl_En_Dodojr/z_en_dodojr.h"
#include "src/overlays/actors/ovl_En_Am/z_en_am.h"
#include "src/overlays/actors/ovl_En_Vm/z_en_vm.h"
#include "src/overlays/actors/ovl_En_Bigokuta/z_en_bigokuta.h"
#include "src/overlays/actors/ovl_En_Bili/z_en_bili.h"
#include "src/overlays/actors/ovl_En_Vali/z_en_vali.h"
#include "src/overlays/actors/ovl_En_Sb/z_en_sb.h"
#include "src/overlays/actors/ovl_En_Ba/z_en_ba.h"
#include "src/overlays/actors/ovl_En_Bubble/z_en_bubble.h"
#include "src/overlays/actors/ovl_En_Tite/z_en_tite.h"
#include "src/overlays/actors/ovl_En_Bb/z_en_bb.h"
#include "src/overlays/actors/ovl_En_Tp/z_en_tp.h"
#include "src/overlays/actors/ovl_En_Rr/z_en_rr.h"
#include "src/overlays/actors/ovl_En_Ny/z_en_ny.h"
#include "src/overlays/actors/ovl_En_Fz/z_en_fz.h"
#include "src/overlays/actors/ovl_En_Test/z_en_test.h"
#include "src/overlays/actors/ovl_En_Floormas/z_en_floormas.h"
#include "src/overlays/actors/ovl_En_Wallmas/z_en_wallmas.h"
#include "src/overlays/actors/ovl_En_Rd/z_en_rd.h"
#include "src/overlays/actors/ovl_En_Skb/z_en_skb.h"
#include "src/overlays/actors/ovl_En_Mb/z_en_mb.h"
#include "src/overlays/actors/ovl_En_Bw/z_en_bw.h"
#include "src/overlays/actors/ovl_En_Fd/z_en_fd.h"
#include "src/overlays/actors/ovl_En_Fw/z_en_fw.h"
#include "src/overlays/actors/ovl_En_Ik/z_en_ik.h"
#include "src/overlays/actors/ovl_En_Crow/z_en_crow.h"
#include "src/overlays/actors/ovl_En_Eiyer/z_en_eiyer.h"
#include "src/overlays/actors/ovl_En_Weiyer/z_en_weiyer.h"
#include "src/overlays/actors/ovl_En_GeldB/z_en_geldb.h"
#include "src/overlays/actors/ovl_En_Dh/z_en_dh.h"
#include "src/overlays/actors/ovl_En_Dha/z_en_dha.h"
#include "src/overlays/actors/ovl_En_Anubice/z_en_anubice.h"
#include "src/overlays/actors/ovl_En_Skj/z_en_skj.h"
#include "src/overlays/actors/ovl_En_Po_Field/z_en_po_field.h"
#include "src/overlays/actors/ovl_En_Po_Sisters/z_en_po_sisters.h"
#include "src/overlays/actors/ovl_En_Poh/z_en_poh.h"
#include "src/overlays/actors/ovl_En_Karebaba/z_en_karebaba.h"
#include "src/overlays/actors/ovl_En_Brob/z_en_brob.h"
#include "src/overlays/actors/ovl_En_Reeba/z_en_reeba.h"
#include "src/overlays/actors/ovl_Boss_Va/z_boss_va.h"
#include "src/overlays/actors/ovl_Boss_Fd/z_boss_fd.h"
#include "src/overlays/actors/ovl_Boss_Fd2/z_boss_fd2.h"
#include "src/overlays/actors/ovl_Boss_Mo/z_boss_mo.h"
#include "src/overlays/actors/ovl_Boss_Sst/z_boss_sst.h"
#include "src/overlays/actors/ovl_Boss_Tw/z_boss_tw.h"
#include "src/overlays/actors/ovl_Boss_Ganon/z_boss_ganon.h"
#include "src/overlays/actors/ovl_Boss_Ganon2/z_boss_ganon2.h"
#undef this
extern PlayState* gPlayState;
}

extern void AddSkelAnimeState(nlohmann::json& extra, SkelAnime* skelAnime);
extern void ApplySkelAnimeState(nlohmann::json extra, SkelAnime* skelAnime);

namespace {

struct GenericEnemySyncEntry {
    // Byte offset of the actor's actionFunc member, or -1 to sync only the skeleton (used for actors whose
    // action functions manipulate cameras/cutscenes and are unsafe to jump into raw).
    ptrdiff_t actionFuncOffset;
    // Byte offset of the actor's SkelAnime member, or -1 when the actor has no skeleton.
    ptrdiff_t skelAnimeOffset;
};

#define GENERIC_SYNC(structName) { (ptrdiff_t)offsetof(structName, actionFunc), (ptrdiff_t)offsetof(structName, skelAnime) }
#define GENERIC_SYNC_NO_SKEL(structName) { (ptrdiff_t)offsetof(structName, actionFunc), (ptrdiff_t)-1 }

const std::unordered_map<int16_t, GenericEnemySyncEntry>& Registry() {
    static const std::unordered_map<int16_t, GenericEnemySyncEntry> registry = {
        // Dodongo's Cavern
        { ACTOR_EN_DODONGO, GENERIC_SYNC(EnDodongo) },
        { ACTOR_EN_DODOJR, GENERIC_SYNC(EnDodojr) },
        { ACTOR_EN_AM, GENERIC_SYNC(EnAm) },
        { ACTOR_EN_VM, GENERIC_SYNC(EnVm) },
        // Jabu-Jabu's Belly
        { ACTOR_EN_BIGOKUTA, GENERIC_SYNC(EnBigokuta) },
        { ACTOR_EN_BILI, GENERIC_SYNC(EnBili) },
        { ACTOR_EN_VALI, GENERIC_SYNC(EnVali) },
        { ACTOR_EN_SB, GENERIC_SYNC(EnSb) },
        { ACTOR_EN_BA, GENERIC_SYNC_NO_SKEL(EnBa) },
        { ACTOR_EN_BUBBLE, GENERIC_SYNC_NO_SKEL(EnBubble) },
        { ACTOR_EN_TP, GENERIC_SYNC_NO_SKEL(EnTp) },
        { ACTOR_BOSS_VA, GENERIC_SYNC(BossVa) },
        // Forest Temple and general adult enemies
        { ACTOR_EN_TEST, GENERIC_SYNC(EnTest) },
        { ACTOR_EN_FLOORMAS, GENERIC_SYNC(EnFloormas) },
        { ACTOR_EN_WALLMAS, GENERIC_SYNC(EnWallmas) },
        { ACTOR_EN_RD, GENERIC_SYNC(EnRd) },
        { ACTOR_EN_SKB, GENERIC_SYNC(EnSkb) },
        { ACTOR_EN_MB, GENERIC_SYNC(EnMb) },
        { ACTOR_EN_CROW, GENERIC_SYNC(EnCrow) },
        { ACTOR_EN_SKJ, GENERIC_SYNC(EnSkj) },
        { ACTOR_EN_PO_FIELD, GENERIC_SYNC(EnPoField) },
        { ACTOR_EN_PO_SISTERS, GENERIC_SYNC(EnPoSisters) },
        { ACTOR_EN_POH, GENERIC_SYNC(EnPoh) },
        { ACTOR_EN_KAREBABA, GENERIC_SYNC(EnKarebaba) },
        { ACTOR_EN_RR, GENERIC_SYNC_NO_SKEL(EnRr) },
        { ACTOR_EN_REEBA, { (ptrdiff_t)offsetof(EnReeba, actionfunc), (ptrdiff_t)offsetof(EnReeba, skelanime) } },
        // Fire Temple
        { ACTOR_EN_BW, GENERIC_SYNC(EnBw) },
        { ACTOR_EN_FD, GENERIC_SYNC(EnFd) },
        { ACTOR_EN_FW, GENERIC_SYNC(EnFw) },
        { ACTOR_BOSS_FD, { (ptrdiff_t)offsetof(BossFd, actionFunc), (ptrdiff_t)offsetof(BossFd, skelAnimeHead) } },
        { ACTOR_BOSS_FD2, GENERIC_SYNC(BossFd2) },
        // Water Temple
        { ACTOR_EN_EIYER, { (ptrdiff_t)offsetof(EnEiyer, actionFunc), (ptrdiff_t)offsetof(EnEiyer, skelanime) } },
        { ACTOR_EN_WEIYER, GENERIC_SYNC(EnWeiyer) },
        { ACTOR_EN_NY, GENERIC_SYNC_NO_SKEL(EnNy) },
        { ACTOR_BOSS_MO, GENERIC_SYNC_NO_SKEL(BossMo) },
        // Ice Cavern / Gerudo
        { ACTOR_EN_FZ, GENERIC_SYNC_NO_SKEL(EnFz) },
        { ACTOR_EN_BROB, GENERIC_SYNC(EnBrob) },
        { ACTOR_EN_GELDB, GENERIC_SYNC(EnGeldB) },
        // Shadow Temple / Bottom of the Well
        { ACTOR_EN_DH, GENERIC_SYNC(EnDh) },
        { ACTOR_EN_DHA, GENERIC_SYNC(EnDha) },
        { ACTOR_BOSS_SST, GENERIC_SYNC(BossSst) },
        // Spirit Temple
        { ACTOR_EN_ANUBICE, GENERIC_SYNC(EnAnubice) },
        { ACTOR_EN_IK, GENERIC_SYNC(EnIk) },
        { ACTOR_BOSS_TW, GENERIC_SYNC(BossTw) },
        // Ganon's Castle: action functions drive cameras and cutscenes, so only animations are mirrored;
        // motion/health sync and local AI carry the rest.
        { ACTOR_BOSS_GANON, { (ptrdiff_t)-1, (ptrdiff_t)offsetof(BossGanon, skelAnime) } },
        { ACTOR_BOSS_GANON2, { (ptrdiff_t)-1, (ptrdiff_t)offsetof(BossGanon2, skelAnime) } },
    };
    return registry;
}

#undef GENERIC_SYNC
#undef GENERIC_SYNC_NO_SKEL

// Code offsets are exchanged relative to a fixed anchor function. Two additional well-separated functions act
// as a build fingerprint: if their relative offsets match, both clients run the same binary and raw actionFunc
// pointers can be reconstructed safely; otherwise action sync silently degrades to motion + animation sync.
int64_t CodeOffset(void* fn) {
    return (int64_t)((intptr_t)fn - (intptr_t)(void*)&Actor_Spawn);
}

void GetCodeFingerprint(int64_t* fpA, int64_t* fpB) {
    *fpA = CodeOffset((void*)&Actor_Kill);
    *fpB = CodeOffset((void*)&SkelAnime_Update);
}

SkelAnime* GetEntrySkelAnime(Actor* actor, const GenericEnemySyncEntry& entry) {
    if (entry.skelAnimeOffset < 0) {
        return nullptr;
    }
    return (SkelAnime*)((uint8_t*)actor + entry.skelAnimeOffset);
}

} // namespace

bool HasGenericEnemySync(int16_t actorId) {
    return Registry().contains(actorId);
}

nlohmann::json GetGenericEnemyState(Actor* actor) {
    nlohmann::json extra = nlohmann::json::object();
    auto it = Registry().find(actor->id);
    if (it == Registry().end()) {
        return extra;
    }

    extra["kind"] = "Generic";

    if (it->second.actionFuncOffset >= 0) {
        void* actionFunc = nullptr;
        memcpy(&actionFunc, (uint8_t*)actor + it->second.actionFuncOffset, sizeof(void*));
        if (actionFunc != nullptr) {
            int64_t fpA, fpB;
            GetCodeFingerprint(&fpA, &fpB);
            extra["fpA"] = fpA;
            extra["fpB"] = fpB;
            extra["afOff"] = CodeOffset(actionFunc);
        }
    }

    SkelAnime* skelAnime = GetEntrySkelAnime(actor, it->second);
    if (skelAnime != nullptr) {
        AddSkelAnimeState(extra, skelAnime);
        if (skelAnime->animation != nullptr && ResourceMgr_OTRSigCheck((char*)skelAnime->animation)) {
            extra["animPath"] = std::string((const char*)skelAnime->animation);
        }
    }

    return extra;
}

void ApplyGenericEnemyState(Actor* actor, nlohmann::json extra) {
    // No kind check: handcrafted extra states (e.g. BossMo) layer their fields on top of the generic ones and
    // delegate here; the registry lookup is the real gate.
    auto it = Registry().find(actor->id);
    if (it == Registry().end()) {
        return;
    }

    SkelAnime* skelAnime = GetEntrySkelAnime(actor, it->second);
    if (skelAnime != nullptr) {
        std::string animPath = extra.value("animPath", std::string(""));
        if (!animPath.empty()) {
            bool sameAnim = skelAnime->animation != nullptr && ResourceMgr_OTRSigCheck((char*)skelAnime->animation) &&
                            animPath == (const char*)skelAnime->animation;
            if (!sameAnim) {
                // Animation clips are addressed by OTR resource path; interned copies stay alive for the
                // pointer's lifetime and resolve lazily inside the SkelAnime system like the originals.
                static std::unordered_set<std::string> internedAnimPaths;
                const std::string& interned = *internedAnimPaths.insert(animPath).first;
                skelAnime->animation = (void*)interned.c_str();
                skelAnime->curFrame = extra.value("skelCurFrame", 0.0f);
            }
        }
        ApplySkelAnimeState(extra, skelAnime);
    }

    if (it->second.actionFuncOffset < 0 || !extra.contains("afOff")) {
        return;
    }

    // Never jump action functions while a cutscene is playing locally; both clients run their own cutscene
    // staging and stomping the actor's state mid-script can soft-lock it.
    if (gPlayState != nullptr && gPlayState->csCtx.state != 0) {
        return;
    }

    int64_t localFpA, localFpB;
    GetCodeFingerprint(&localFpA, &localFpB);
    if (extra.value("fpA", (int64_t)0) != localFpA || extra.value("fpB", (int64_t)0) != localFpB) {
        return;
    }

    void* remoteFunc = (void*)((intptr_t)(void*)&Actor_Spawn + (intptr_t)extra.value("afOff", (int64_t)0));
    void* localFunc = nullptr;
    memcpy(&localFunc, (uint8_t*)actor + it->second.actionFuncOffset, sizeof(void*));
    if (remoteFunc != localFunc) {
        memcpy((uint8_t*)actor + it->second.actionFuncOffset, &remoteFunc, sizeof(void*));
    }
}

void EnsureEnemyDeathSetup(Actor* actor, PlayState* play) {
    switch (actor->id) {
        case ACTOR_EN_SKB: {
            EnSkb* skb = (EnSkb*)actor;
            BodyBreak_Alloc(&skb->bodyBreak, 18, play);
            skb->breakFlags |= 4;
            skb->actionState = 1;
            break;
        }
        case ACTOR_EN_TEST: {
            EnTest* test = (EnTest*)actor;
            BodyBreak_Alloc(&test->bodyBreak, 60, play);
            break;
        }
        case ACTOR_EN_TITE: {
            EnTite* tite = (EnTite*)actor;
            BodyBreak_Alloc(&tite->bodyBreak, 24, play);
            break;
        }
        case ACTOR_EN_BB: {
            EnBb* bb = (EnBb*)actor;
            BodyBreak_Alloc(&bb->bodyBreak, 12, play);
            break;
        }
        case ACTOR_EN_SB: {
            EnSb* sb = (EnSb*)actor;
            BodyBreak_Alloc(&sb->bodyBreak, 8, play);
            sb->isDead = true;
            break;
        }
        default:
            return;
    }
    actor->flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
}
