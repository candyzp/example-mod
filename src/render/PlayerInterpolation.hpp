#pragma once

#include <Geode/Geode.hpp>

namespace cbfplus::render {

struct PlayerVisualSample {
    cocos2d::CCPoint nodePosition = {0.0f, 0.0f};
    cocos2d::CCPoint internalPosition = {0.0f, 0.0f};
    float alpha = 0.0f;
    bool valid = false;
};

// Produce a render-only Player 1 position one confirmed physics step behind.
// The authoritative gameplay state is never modified by this function.
PlayerVisualSample samplePlayer1();

} // namespace cbfplus::render
