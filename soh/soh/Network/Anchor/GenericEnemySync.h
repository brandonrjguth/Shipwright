#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>

struct Actor;

// Registry-driven sync for enemies and bosses without handcrafted extra-state mappings. Each registered actor
// gets its actionFunc synced as a binary-relative code offset (validated by a build fingerprint, since both
// co-op clients must run the same build) and its SkelAnime synced including the animation clip, which SoH
// represents as a stable OTR resource path string. One registry line per actor replaces a per-enemy state
// machine mapping.
bool HasGenericEnemySync(int16_t actorId);
nlohmann::json GetGenericEnemyState(Actor* actor);
void ApplyGenericEnemyState(Actor* actor, nlohmann::json extra);

// Called when a replica enemy's health has reached 0 but its native death sequence hasn't
// triggered yet (the kill arrived before the health=0 snapshot). Allocates BodyBreak and
// sets any per-enemy death flags so the enemy's own death animation plays correctly.
struct PlayState;
bool EnsureEnemyDeathSetup(Actor* actor, PlayState* play);
