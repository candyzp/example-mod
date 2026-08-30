#include <Geode/Geode.hpp>
#include <Geode/binding/EndLevelLayer.hpp>
#include <Geode/binding/PauseLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>

#include "timing/FrameClock.hpp"

using namespace geode::prelude;

namespace {
bool isActiveGameplayFrame() {
    auto* playLayer = PlayLayer::get();
    if (!playLayer) {
        return false;
    }

    auto* parent = playLayer->getParent();
    if (!parent) {
        return false;
    }

    if (parent->getChildByType<PauseLayer>(0)) {
        return false;
    }

    if (playLayer->getChildByType<EndLevelLayer>(0)) {
        return false;
    }

    return true;
}
} // namespace

// Part 3: observe the proven non-Windows/iOS frame boundary used by CBF.
// Timing is only considered valid during active gameplay. Nothing here mutates
// gameplay, physics, player, camera, or render state.
class $modify(CBFPlusScheduler, CCScheduler) {
    void update(float dt) {
        cbfplus::timing::beginFrame(dt, isActiveGameplayFrame());
        CCScheduler::update(dt);
    }
};
