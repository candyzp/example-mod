#include "../render/PlayerInterpolation.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

namespace {
bool g_loggedInterpolationReady = false;

struct TransformRestoreGuard {
    PlayerObject* player = nullptr;
    cocos2d::CCPoint nodePosition = {0.0f, 0.0f};
    cocos2d::CCPoint internalPosition = {0.0f, 0.0f};
    float rotation = 0.0f;

    ~TransformRestoreGuard() {
        if (!player) {
            return;
        }
        player->CCNode::setPosition(nodePosition);
        player->m_position = internalPosition;
        player->CCNode::setRotation(rotation);
    }
};
} // namespace

// Part 6: temporarily present interpolated P1 position + rotation only during
// the normal render traversal, then restore the authoritative physics state.
class $modify(CBFPlusRenderInterpolation, GJBaseGameLayer) {
    void visit() {
        auto* playLayer = geode::cast::typeinfo_cast<PlayLayer*>(this);
        if (!playLayer || !playLayer->m_player1 || playLayer->m_playerDied) {
            GJBaseGameLayer::visit();
            return;
        }

        auto const visual = cbfplus::render::samplePlayer1();
        if (!visual.valid) {
            GJBaseGameLayer::visit();
            return;
        }

        auto* player = playLayer->m_player1;
        TransformRestoreGuard restore {
            player,
            player->getPosition(),
            player->m_position,
            player->getRotation(),
        };

        // Touch only the base CCNode transform for the duration of visit().
        // PlayerObject gameplay state remains authoritative before and after.
        player->CCNode::setPosition(visual.nodePosition);
        player->m_position = visual.internalPosition;
        player->CCNode::setRotation(visual.rotation);

        if (!g_loggedInterpolationReady) {
            g_loggedInterpolationReady = true;
            geode::log::info(
                "CBF+ P1 XY+rotation interpolation active: alpha {:.3f}, visual ({:.2f}, {:.2f}) rot {:.2f}, slope-guard {}",
                static_cast<double>(visual.alpha),
                static_cast<double>(visual.nodePosition.x),
                static_cast<double>(visual.nodePosition.y),
                static_cast<double>(visual.rotation),
                visual.protectedSlopeTransition
            );
        }

        GJBaseGameLayer::visit();
    }
};
