#include "Prewarm.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/ObjectToolbox.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/cocos/sprite_nodes/CCSpriteFrameCache.h>
#include <Geode/utils/cocos.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>

using namespace geode::prelude;

namespace cbfplus {
namespace {
    // A volatile sink keeps the compiler from throwing away read-only warmup
    // work. Nothing from this value is used by gameplay.
    volatile std::uint64_t g_warmSink = 0;

    void warmTexture(CCTexture2D* texture) {
        if (!texture) return;

        g_warmSink += static_cast<std::uint64_t>(texture->getPixelsWide());
        g_warmSink += static_cast<std::uint64_t>(texture->getPixelsHigh());
    }

    void warmSceneGraph(CCNode* root, std::size_t reserveHint) {
        if (!root) return;

        std::vector<CCNode*> stack;
        stack.reserve(std::max<std::size_t>(reserveHint, 256));
        stack.push_back(root);

        while (!stack.empty()) {
            auto* node = stack.back();
            stack.pop_back();
            if (!node) continue;

            // Force Cocos to resolve the node's current transform while we are
            // still inside level loading rather than on a later gameplay frame.
            (void)node->nodeToWorldTransform();

            auto const& position = node->getPosition();
            auto const& size = node->getContentSize();
            g_warmSink += static_cast<std::uint64_t>(position.x + position.y + size.width + size.height);

            if (auto* sprite = typeinfo_cast<CCSprite*>(node)) {
                warmTexture(sprite->getTexture());
                (void)sprite->getTextureRect();
                (void)sprite->getQuad();
            }

            auto* children = node->getChildren();
            if (!children) continue;

            auto count = children->count();
            for (unsigned int i = 0; i < count; ++i) {
                auto* child = static_cast<CCNode*>(children->objectAtIndex(i));
                if (child) stack.push_back(child);
            }
        }
    }

    void warmAllocator(std::size_t objectCount) {
        // Touch a small, bounded chunk of heap pages now. The allocation is
        // intentionally temporary; on iOS the allocator can reuse those pages
        // later without CBF+ keeping a large permanent buffer alive.
        constexpr std::size_t page = 4096;
        constexpr std::size_t minBytes = 512 * 1024;
        constexpr std::size_t maxBytes = 4 * 1024 * 1024;

        auto wanted = std::clamp<std::size_t>(objectCount * 32, minBytes, maxBytes);
        std::vector<std::uint8_t> scratch(wanted);

        for (std::size_t i = 0; i < scratch.size(); i += page) {
            scratch[i] = static_cast<std::uint8_t>((i / page) & 0xff);
            g_warmSink += scratch[i];
        }
    }
}

void prewarmLevel(PlayLayer* layer) {
    if (!layer || !layer->m_objects) return;

    auto started = std::chrono::steady_clock::now();

    CCArrayExt<GameObject*> objects = layer->m_objects;
    std::unordered_set<int> objectIDs;
    objectIDs.reserve(objects.size());

    // Pass 1: force object bounds, transforms, and already-referenced textures
    // to be touched before the first playable frame.
    for (auto* object : objects) {
        if (!object) continue;

        (void)object->getObjectRect();
        (void)object->nodeToWorldTransform();
        warmTexture(object->getTexture());
        (void)object->getTextureRect();
        (void)object->getQuad();

        if (object->m_objectID > 0) {
            objectIDs.insert(object->m_objectID);
        }
    }

    // Pass 2: warm the ObjectToolbox -> sprite-frame-cache lookup path once per
    // unique object type. This targets one-time dictionary/cache work without
    // creating or mutating gameplay objects.
    auto* toolbox = ObjectToolbox::sharedState();
    auto* frameCache = CCSpriteFrameCache::get();
    if (toolbox && frameCache) {
        for (auto objectID : objectIDs) {
            auto* frameName = toolbox->intKeyToFrame(objectID);
            if (!frameName || !*frameName) continue;

            auto* frame = frameCache->spriteFrameByName(frameName);
            if (frame) {
                g_warmSink += static_cast<std::uint64_t>(objectID);
            }
        }
    }

    // Pass 3: warm the complete PlayLayer node tree, including player/UI/effect
    // sprites that are not necessarily represented in m_objects.
    warmSceneGraph(layer, objects.size() * 2 + 256);

    // Pass 4: pre-touch a bounded amount of heap memory while loading.
    warmAllocator(objects.size());

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started
    ).count();

    log::info(
        "CBF+ prewarm complete: {} objects, {} unique object types, {} ms",
        objects.size(), objectIDs.size(), elapsed
    );
}
}
