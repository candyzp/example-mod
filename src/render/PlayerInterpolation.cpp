#include "PlayerInterpolation.hpp"

#include "../state/PlayerState.hpp"
#include "../timing/FrameClock.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr double kPhysicsFramesPerSecond = 60.0;
constexpr float kSlopeRotationEpsilon = 0.01f;
constexpr float kTransitionEpsilon = 0.001f;

cocos2d::CCPoint lerpPoint(
    cocos2d::CCPoint const& from,
    cocos2d::CCPoint const& to,
    float alpha
) {
    return {
        from.x + (to.x - from.x) * alpha,
        from.y + (to.y - from.y) * alpha,
    };
}

float shortestAngleDelta(float from, float to) {
    if (!std::isfinite(from) || !std::isfinite(to)) {
        return 0.0f;
    }
    return std::remainder(to - from, 360.0f);
}

float lerpAngle(float from, float to, float alpha) {
    return from + shortestAngleDelta(from, to) * alpha;
}

bool pointChanged(cocos2d::CCPoint const& a, cocos2d::CCPoint const& b) {
    return std::abs(a.x - b.x) > kTransitionEpsilon ||
        std::abs(a.y - b.y) > kTransitionEpsilon;
}

bool crossesSlopeDiscontinuity(
    cbfplus::state::PlayerSample const& previous,
    cbfplus::state::PlayerSample const& current
) {
    if (previous.isOnSlope != current.isOnSlope) {
        return true;
    }

    if (previous.isOnSlope && current.isOnSlope) {
        return std::abs(shortestAngleDelta(previous.slopeRotation, current.slopeRotation)) >
            kSlopeRotationEpsilon;
    }

    return false;
}

bool crossesGameplayDiscontinuity(
    cbfplus::state::PlayerSample const& previous,
    cbfplus::state::PlayerSample const& current
) {
    if (previous.modeFlags != current.modeFlags ||
        previous.isUpsideDown != current.isUpsideDown) {
        return true;
    }

    if (std::abs(previous.vehicleSize - current.vehicleSize) > kTransitionEpsilon) {
        return true;
    }

    // GD uses m_lastPortalPos in portal/pad activation paths. If that confirmed
    // activation marker moves, keep this render interval authoritative.
    return pointChanged(previous.lastPortalPosition, current.lastPortalPosition);
}
} // namespace

namespace cbfplus::render {

PlayerVisualSample samplePlayer1() {
    PlayerVisualSample result{};

    auto const& timing = cbfplus::timing::current();
    if (!timing.valid) {
        return result;
    }

    auto const& playerTrack = cbfplus::state::track(cbfplus::state::PlayerSlot::Player1);
    if (!playerTrack.hasPair()) {
        return result;
    }

    auto const& previous = playerTrack.previous;
    auto const& current = playerTrack.current;

    result.valid = true;

    // Part 7 deliberately keeps all non-cube modes on authoritative rendering.
    // Part 8 will add explicit mode support instead of inheriting cube behavior.
    if (previous.modeFlags != 0 || current.modeFlags != 0) {
        result.unsupportedMode = true;
        return result;
    }

    bool const slopeDiscontinuity = crossesSlopeDiscontinuity(previous, current);
    bool const gameplayDiscontinuity = crossesGameplayDiscontinuity(previous, current);
    if (slopeDiscontinuity || gameplayDiscontinuity) {
        result.protectedSlopeTransition = slopeDiscontinuity;
        result.protectedDiscontinuity = true;
        return result;
    }

    if (!std::isfinite(current.stepDelta) || current.stepDelta <= 0.0f ||
        !std::isfinite(current.captureTime) || current.captureTime <= 0.0 ||
        !std::isfinite(previous.rotation) || !std::isfinite(current.rotation)) {
        result.valid = false;
        return result;
    }

    // PlayerObject::update uses 60 Hz frame units. A 240 TPS step is normally
    // 0.25 frame, which corresponds to 0.25 / 60 seconds.
    double const stepSeconds = static_cast<double>(current.stepDelta) / kPhysicsFramesPerSecond;
    if (!std::isfinite(stepSeconds) || stepSeconds <= 0.0) {
        result.valid = false;
        return result;
    }

    double elapsed = cbfplus::timing::nowSeconds() - current.captureTime;
    if (!std::isfinite(elapsed)) {
        result.valid = false;
        return result;
    }
    elapsed = std::max(0.0, elapsed);

    float const alpha = static_cast<float>(std::clamp(elapsed / stepSeconds, 0.0, 1.0));

    result.nodePosition = lerpPoint(previous.nodePosition, current.nodePosition, alpha);
    result.internalPosition = lerpPoint(previous.internalPosition, current.internalPosition, alpha);
    result.rotation = lerpAngle(previous.rotation, current.rotation, alpha);
    result.alpha = alpha;
    result.applyVisual = true;
    return result;
}

} // namespace cbfplus::render
