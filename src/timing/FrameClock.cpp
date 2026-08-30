#include "FrameClock.hpp"

#include <Geode/Geode.hpp>

#include <chrono>

namespace {
using Clock = std::chrono::steady_clock;

cbfplus::timing::FrameSample g_sample;
Clock::time_point g_epoch;
Clock::time_point g_lastFrame;
bool g_initialized = false;
bool g_loggedReady = false;

void resetSession() {
    g_sample = {};
    g_initialized = false;
}
} // namespace

namespace cbfplus::timing {

void beginFrame(float schedulerDelta, bool activeGameplay) {
    if (!activeGameplay) {
        if (g_initialized || g_sample.index != 0 || g_sample.valid) {
            resetSession();
        }
        return;
    }

    auto const now = Clock::now();

    if (!g_initialized) {
        g_initialized = true;
        g_epoch = now;
        g_lastFrame = now;
        g_sample.index = 1;
        g_sample.schedulerDelta = schedulerDelta;
        g_sample.measuredDelta = 0.0;
        g_sample.elapsed = 0.0;
        g_sample.valid = false;
        return;
    }

    g_sample.index += 1;
    g_sample.schedulerDelta = schedulerDelta;
    g_sample.measuredDelta = std::chrono::duration<double>(now - g_lastFrame).count();
    g_sample.elapsed = std::chrono::duration<double>(now - g_epoch).count();
    g_sample.valid = true;
    g_lastFrame = now;

    // One-shot proof that the read-only frame boundary hook is running during gameplay.
    if (!g_loggedReady && g_sample.index >= 120) {
        g_loggedReady = true;
        geode::log::info(
            "CBF+ frame clock active: scheduler {:.3f} ms, measured {:.3f} ms",
            static_cast<double>(g_sample.schedulerDelta) * 1000.0,
            g_sample.measuredDelta * 1000.0
        );
    }
}

double nowSeconds() {
    return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}

FrameSample const& current() {
    return g_sample;
}

} // namespace cbfplus::timing
