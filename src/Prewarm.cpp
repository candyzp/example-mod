#include "Prewarm.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/ObjectToolbox.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/cocos/shaders/CCGLProgram.h>
#include <Geode/cocos/shaders/CCShaderCache.h>
#include <Geode/cocos/shaders/ccGLStateCache.h>
#include <Geode/cocos/sprite_nodes/CCSpriteFrame.h>
#include <Geode/cocos/sprite_nodes/CCSpriteFrameCache.h>
#include <Geode/utils/cocos.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>

using namespace geode::prelude;

namespace cbfplus {
namespace {
    volatile std::uint64_t g_warmSink = 0;

    void warmFloat(float value) {
        g_warmSink ^= static_cast<std::uint64_t>(std::bit_cast<std::uint32_t>(value));
    }

    void warmTextureMetadata(CCTexture2D* texture) {
        if (!texture) return;

        g_warmSink += static_cast<std::uint64_t>(texture->getPixelsWide());
        g_warmSink += static_cast<std::uint64_t>(texture->getPixelsHigh());
        g_warmSink ^= static_cast<std::uint64_t>(texture->getName());
    }

    void rememberTexture(
        CCTexture2D* texture,
        std::unordered_set<CCTexture2D*>& textures
    ) {
        if (!texture) return;
        warmTextureMetadata(texture);
        textures.insert(texture);
    }

    void warmSpriteFrame(
        CCSpriteFrame* frame,
        std::unordered_set<CCTexture2D*>& textures
    ) {
        if (!frame) return;

        auto const& rect = frame->getRectInPixels();
        auto const& offset = frame->getOffsetInPixels();
        auto const& original = frame->getOriginalSizeInPixels();

        warmFloat(rect.origin.x);
        warmFloat(rect.origin.y);
        warmFloat(rect.size.width);
        warmFloat(rect.size.height);
        warmFloat(offset.x);
        warmFloat(offset.y);
        warmFloat(original.width);
        warmFloat(original.height);

        g_warmSink ^= frame->isRotated() ? 0x51u : 0xA2u;
        rememberTexture(frame->getTexture(), textures);
    }

    void warmShaders() {
        auto* cache = CCShaderCache::sharedShaderCache();
        if (!cache) return;

        constexpr std::array<const char*, 5> keys = {
            kCCShader_PositionTextureColor,
            kCCShader_PositionTextureColorAlphaTest,
            kCCShader_PositionColor,
            kCCShader_PositionTexture,
            kCCShader_PositionTexture_uColor,
        };

        for (auto const* key : keys) {
            auto* program = cache->programForKey(key);
            if (!program) continue;

            // Force the cached program through the same first-use path that
            // normal rendering uses, while the loading screen is still up.
            program->use();
            program->setUniformsForBuiltins();
            ++g_warmSink;
        }

        // Leave Cocos on its normal sprite shader instead of an uncommon one.
        if (auto* program = cache->programForKey(kCCShader_PositionTextureColor)) {
            program->use();
            program->setUniformsForBuiltins();
        }
    }

    void warmSceneGraph(
        CCNode* root,
        std::size_t reserveHint,
        std::unordered_set<CCTexture2D*>& textures
    ) {
        if (!root) return;

        std::vector<CCNode*> stack;
        stack.reserve(std::max<std::size_t>(reserveHint, 256));
        stack.push_back(root);

        while (!stack.empty()) {
            auto* node = stack.back();
            stack.pop_back();
            if (!node) continue;

            // Resolve the transform cache before the first playable frame.
            (void)node->nodeToWorldTransform();

            auto const& position = node->getPosition();
            auto const& size = node->getContentSize();
            warmFloat(position.x);
            warmFloat(position.y);
            warmFloat(size.width);
            warmFloat(size.height);

            if (auto* sprite = typeinfo_cast<CCSprite*>(node)) {
                rememberTexture(sprite->getTexture(), textures);
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

    void warmTextureBindings(std::unordered_set<CCTexture2D*> const& textures) {
        // Reading texture metadata alone does not exercise the GL texture bind
        // path. Bind each unique texture once through Cocos' own state cache so
        // the driver sees it during loading instead of on a later gameplay frame.
        for (auto* texture : textures) {
            if (!texture) continue;

            auto name = texture->getName();
            if (!name) continue;

            ccGLBindTexture2D(name);
            g_warmSink ^= static_cast<std::uint64_t>(name);
        }
    }

    void touchBlock(std::vector<std::uint8_t>& block) {
        constexpr std::size_t page = 4096;

        for (std::size_t i = 0; i < block.size(); i += page) {
            block[i] = static_cast<std::uint8_t>((i / page + block.size()) & 0xff);
            g_warmSink += block[i];
        }

        if (!block.empty()) {
            block.back() ^= 0x5A;
            g_warmSink += block.back();
        }
    }

    void warmAllocator(std::size_t objectCount) {
        // Seed several allocator size classes instead of one large temporary
        // buffer. This better targets one-off malloc/page activity from small and
        // medium gameplay allocations while staying strictly load-time only.
        struct Bin {
            std::size_t bytes;
            std::size_t count;
        };

        constexpr std::array<Bin, 7> bins = {{
            {64, 96},
            {256, 64},
            {1024, 32},
            {4096, 24},
            {16 * 1024, 12},
            {64 * 1024, 6},
            {256 * 1024, 2},
        }};

        constexpr std::size_t maxTotalBytes = 6 * 1024 * 1024;
        auto intensity = std::clamp<std::size_t>((objectCount + 4999) / 5000, 1, 4);

        std::vector<std::vector<std::uint8_t>> blocks;
        blocks.reserve(256);

        std::size_t total = 0;
        bool full = false;

        for (auto const& bin : bins) {
            auto count = bin.count * intensity;

            for (std::size_t i = 0; i < count; ++i) {
                if (total + bin.bytes > maxTotalBytes) {
                    full = true;
                    break;
                }

                blocks.emplace_back(bin.bytes);
                touchBlock(blocks.back());
                total += bin.bytes;
            }

            if (full) break;
        }

        // Destruction happens while the loading screen is still active too,
        // warming the corresponding free/reuse paths before gameplay begins.
        blocks.clear();
    }
}

void prewarmLevel(PlayLayer* layer) {
    if (!layer || !layer->m_objects) return;

    auto started = std::chrono::steady_clock::now();

    CCArrayExt<GameObject*> objects = layer->m_objects;

    std::unordered_set<int> objectIDs;
    objectIDs.reserve(objects.size() * 2 + 32);

    std::unordered_set<CCTexture2D*> textures;
    textures.reserve(objects.size() / 4 + 64);

    // Pass 1: warm the common shader/program path before any gameplay draw.
    warmShaders();

    // Pass 2: touch every gameplay object once. getObjectRect() also resolves
    // cached object bounds when dirty, which is exactly the kind of one-time
    // work we want paid for under "Optimizing...".
    for (auto* object : objects) {
        if (!object) continue;

        (void)object->getObjectRect();
        (void)object->nodeToWorldTransform();
        rememberTexture(object->getTexture(), textures);
        (void)object->getTextureRect();
        (void)object->getQuad();

        if (object->m_objectID > 0) {
            objectIDs.insert(object->m_objectID);
        }
    }

    // Pass 3: warm the object-id -> frame lookup plus the frame's own texture,
    // crop, offset and original-size data once per unique object type.
    auto* toolbox = ObjectToolbox::sharedState();
    auto* frameCache = CCSpriteFrameCache::get();
    if (toolbox && frameCache) {
        for (auto objectID : objectIDs) {
            auto* frameName = toolbox->intKeyToFrame(objectID);
            if (!frameName || !*frameName) continue;

            auto* frame = frameCache->spriteFrameByName(frameName);
            if (!frame) continue;

            warmSpriteFrame(frame, textures);
            g_warmSink += static_cast<std::uint64_t>(objectID);
        }
    }

    // Pass 4: warm the rest of the already-created PlayLayer tree, including
    // player/UI/effect sprites that are not present in m_objects.
    warmSceneGraph(layer, objects.size() * 2 + 256, textures);

    // Pass 5: make the GL driver see every unique texture once during loading.
    warmTextureBindings(textures);

    // Pass 6: seed several allocator size classes and page-touch the memory.
    warmAllocator(objects.size());

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started
    ).count();

    log::info(
        "CBF+ aggressive prewarm complete: {} objects, {} object types, {} textures, {} ms",
        objects.size(), objectIDs.size(), textures.size(), elapsed
    );
}
}
