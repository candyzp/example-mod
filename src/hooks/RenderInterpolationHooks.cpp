#include "../render/PlayerInterpolation.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

namespace {
bool g_loggedInterpolationReady = false;

struct PositionRestoreGuard {
    PlayerObject* player = nullptr;
    cocos2d::CCPoint nodePosition = {0.0f, 0.0f};
    cocos2d::CCPoint internalPosition = {0.0f, 0.0f};

    ~PositionRestoreGuard() {
        if (!player) {
            return;
        }
        player->CCNode::setPosition(nodePosition);
        player->m_position = internalPosition;
    }
};
} // namespace

// Part 5: temporarily present an interpolated P1 X/Y only during the normal
// render traversal, then restore the authoritative physics state immediately.
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
        PositionRestoreGuard restore {
            player,
            player->getPosition(),
            player->m_position,
        };

        // Match the proven render-only pattern used by the smoothing reference:
        // bypass PlayerObject setters and touch only the node/internal position
        // for the duration of visit(). Physics never observes these values.
        player->CCNode::setPosition(visual.nodePosition);
        player->m_position = visual.internalPosition;

        if (!g_loggedInterpolationReady) {
            g_loggedInterpolationReady = true;
            geode::log::info(
                "CBF+ P1 XY interpolation active: alpha {:.3f}, visual ({:.2f}, {:.2f})",
                static_cast<double>(visual.alpha),
                static_cast<double>(visual.nodePosition.x),
                static_cast<double>(visual.nodePosition.y)
            );
        }

        GJBaseGameLayer::visit();
    }
};
