#pragma once

#include <Geode/Geode.hpp>

#include <cstdint>

namespace cbfplus::state {

enum class PlayerSlot : std::uint8_t {
    Player1,
    Player2,
};

struct PlayerSample {
    cocos2d::CCPoint nodePosition = {0.0f, 0.0f};
    cocos2d::CCPoint internalPosition = {0.0f, 0.0f};
    double xVelocity = 0.0;
    double yVelocity = 0.0;
    double captureTime = 0.0;
    float rotation = 0.0f;
    float slopeRotation = 0.0f;
    float stepDelta = 0.0f;
    std::uint64_t frameIndex = 0;
    std::uint64_t sequence = 0;
    std::uintptr_t groundObjectId = 0;
    std::uintptr_t slopeObjectId = 0;
    bool isOnGround = false;
    bool isOnSlope = false;
    bool valid = false;
};

struct PlayerTrack {
    PlayerSample previous{};
    PlayerSample current{};

    bool hasPair() const {
        return previous.valid && current.valid;
    }
};

// Capture the authoritative state left by a completed vanilla PlayerObject update.
// This never writes to the player or gameplay state.
void capture(PlayerObject* player, float stepDelta);

// Drop all captured history at gameplay discontinuities such as death/restart.
void reset();

PlayerTrack const& track(PlayerSlot slot);

} // namespace cbfplus::state
