#include "../state/PlayerState.hpp"

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

// Read only after the real PlayerObject update has completed. This keeps GD's
// physics authoritative while giving later render code a confirmed state pair.
class $modify(CBFPlusPlayerCapture, PlayerObject) {
    void update(float stepDelta) {
        PlayerObject::update(stepDelta);
        cbfplus::state::capture(this, stepDelta);
    }
};

// These hooks only clear CBF+'s private capture history. They do not alter the
// reset/death behavior performed by Geometry Dash.
class $modify(CBFPlusPlayLayerCaptureReset, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        cbfplus::state::reset();
        return PlayLayer::init(level, useReplay, dontCreateObjects);
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);
        cbfplus::state::reset();
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        cbfplus::state::reset();
    }

    void loadFromCheckpoint(CheckpointObject* checkpoint) {
        PlayLayer::loadFromCheckpoint(checkpoint);
        cbfplus::state::reset();
    }

    void resetLevelFromStart() {
        PlayLayer::resetLevelFromStart();
        cbfplus::state::reset();
    }

    void delayedResetLevel() {
        PlayLayer::delayedResetLevel();
        cbfplus::state::reset();
    }

    void fullReset() {
        PlayLayer::fullReset();
        cbfplus::state::reset();
    }
};
