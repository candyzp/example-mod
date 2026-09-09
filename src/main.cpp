#include <Geode/Geode.hpp>
#include <Geode/binding/GJGameLoadingLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/modify/GJGameLoadingLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include "Prewarm.hpp"

using namespace geode::prelude;

class $modify(CBFPlusLoadingLayer, GJGameLoadingLayer) {
    bool init(GJGameLevel* level, bool editor) {
        if (!GJGameLoadingLayer::init(level, editor)) {
            return false;
        }

        if (!editor) {
            auto* label = CCLabelBMFont::create("Optimizing...", "bigFont.fnt");
            if (label) {
                label->setID("cbfplus-optimizing-label");
                label->setScale(0.55f);
                label->setPosition(CCDirector::get()->getWinSize() / 2.f);
                this->addChild(label, 10000);
            }
        }

        return true;
    }
};

class $modify(CBFPlusPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }

        // Every fresh PlayLayer is a fresh CBF+ session. beginLevelSession()
        // also drops any stale session left behind by an abnormal exit path.
        cbfplus::beginLevelSession(this);

        // PlayLayer is fully constructed but has not returned to the game yet,
        // so all prewarming completes before the first playable frame.
        if (!dontCreateObjects) {
            cbfplus::prewarmLevel(this);
        }

        return true;
    }

    void onExit() {
        // Death/restart does not leave PlayLayer, so it keeps the same optimized
        // session. Actually leaving the level hits this hard cleanup boundary.
        cbfplus::endLevelSession(this);
        PlayLayer::onExit();
    }
};

$on_mod(Loaded) {
    log::info("CBF+ load-time stutter prewarmer loaded");
}
