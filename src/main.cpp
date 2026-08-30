#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>

#include "timing/FrameClock.hpp"

using namespace geode::prelude;

// Part 3: observe the proven non-Windows/iOS frame boundary used by CBF.
// No gameplay, physics, player, camera, or render state is modified here.
class $modify(CBFPlusScheduler, CCScheduler) {
    void update(float dt) {
        cbfplus::timing::beginFrame(dt);
        CCScheduler::update(dt);
    }
};
