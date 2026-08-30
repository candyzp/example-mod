#include "PlayerState.hpp"

#include "../timing/FrameClock.hpp"

#include <Geode/binding/PlayLayer.hpp>

#include <array>
#include <optional>

namespace {
std::array<cbfplus::state::PlayerTrack, 2> g_tracks{};
bool g_loggedReady = false;

std::optional<std::size_t> resolveTrack(PlayerObject* player) {
    auto* playLayer = PlayLayer::get();
    if (!playLayer || !player) {
        return std::nullopt;
    }

    if (player == playLayer->m_player1) {
        return 0;
    }
    if (player == playLayer->m_player2) {
        return 1;
    }
    return std::nullopt;
}
} // namespace

namespace cbfplus::state {

void capture(PlayerObject* player, float stepDelta) {
    auto const slot = resolveTrack(player);
    if (!slot) {
        return;
    }

    auto& target = g_tracks[*slot];
    PlayerSample sample{};

    sample.nodePosition = player->getPosition();
    sample.internalPosition = player->m_position;
    sample.xVelocity = player->getCurrentXVelocity();
    sample.yVelocity = player->m_yVelocity;
    sample.captureTime = cbfplus::timing::nowSeconds();
    sample.rotation = player->getRotation();
    sample.slopeRotation = player->m_slopeRotation;
    sample.stepDelta = stepDelta;
    sample.frameIndex = cbfplus::timing::current().index;
    sample.sequence = target.current.valid ? target.current.sequence + 1 : 1;
    sample.groundObjectId = reinterpret_cast<std::uintptr_t>(player->m_lastGroundObject);
    sample.slopeObjectId = reinterpret_cast<std::uintptr_t>(player->m_currentSlope);
    sample.isOnGround = player->m_isOnGround;
    sample.isOnSlope = player->m_isOnSlope;
    sample.valid = true;

    if (target.current.valid) {
        target.previous = target.current;
    } else {
        target.previous = {};
    }
    target.current = sample;

    // One-shot runtime proof. Keep logging out of the per-step hot path after this.
    if (!g_loggedReady && *slot == 0 && target.hasPair()) {
        g_loggedReady = true;
        geode::log::info(
            "CBF+ authoritative player capture active: P1 ({:.2f}, {:.2f}) rot {:.2f}, step {:.4f}, frame {}",
            static_cast<double>(sample.nodePosition.x),
            static_cast<double>(sample.nodePosition.y),
            static_cast<double>(sample.rotation),
            static_cast<double>(sample.stepDelta),
            sample.frameIndex
        );
    }
}

void reset() {
    g_tracks = {};
}

PlayerTrack const& track(PlayerSlot slot) {
    return g_tracks[slot == PlayerSlot::Player1 ? 0 : 1];
}

} // namespace cbfplus::state
