#pragma once

#include <cstdint>

namespace cbfplus::timing {

struct FrameSample {
    std::uint64_t index = 0;
    float schedulerDelta = 0.0f;
    double measuredDelta = 0.0;
    double elapsed = 0.0;
    bool valid = false;
};

// Called once at the beginning of each Cocos scheduler frame.
// Samples are valid only during active gameplay; inactive frames reset the
// session so timing can never span a pause, menu, or end screen.
void beginFrame(float schedulerDelta, bool activeGameplay);

// Latest frame timing snapshot. The returned reference is stable for the process lifetime.
FrameSample const& current();

} // namespace cbfplus::timing
