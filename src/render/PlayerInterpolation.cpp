#include "PlayerInterpolation.hpp"

#include "../state/PlayerState.hpp"
#include "../timing/FrameClock.hpp"

#include <algorithm>
#include <cmath>

namespace {
constexpr double kPhysicsFramesPerSecond = 60.0;

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

    if (!std::isfinite(current.stepDelta) || current.stepDelta <= 0.0f ||
        !std::isfinite(current.captureTime) || current.captureTime <= 0.0) {
        return result;
    }

    // PlayerObject::update uses 60 Hz frame units. A 240 TPS step is normally
    // 0.25 frame, which corresponds to 0.25 / 60 seconds.
    double const stepSeconds = static_cast<double>(current.stepDelta) / kPhysicsFramesPerSecond;
    if (!std::isfinite(stepSeconds) || stepSeconds <= 0.0) {
        return result;
    }

    double elapsed = cbfplus::timing::nowSeconds() - current.captureTime;
    if (!std::isfinite(elapsed)) {
        return result;
    }
    elapsed = std::max(0.0, elapsed);

    float const alpha = static_cast<float>(std::clamp(elapsed / stepSeconds, 0.0, 1.0));

    result.nodePosition = lerpPoint(previous.nodePosition, current.nodePosition, alpha);
    result.internalPosition = lerpPoint(previous.internalPosition, current.internalPosition, alpha);
    result.alpha = alpha;
    result.valid = true;
    return result;
}

} // namespace cbfplus::render
