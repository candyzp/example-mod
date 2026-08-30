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
// This is observation-only: it never changes scheduler, gameplay, or render state.
void beginFrame(float schedulerDelta);

// Latest frame timing snapshot. The returned reference is stable for the process lifetime.
FrameSample const& current();

} // namespace cbfplus::timing
