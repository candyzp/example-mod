# CBF+

An iOS-first Geometry Dash load-time stutter prewarmer for Geode.

CBF+ runs its optimization before gameplay starts, then stays out of the per-frame gameplay path. The current prewarm touches level object bounds/transforms, sprite-frame data, unique textures and GL texture bindings, common Cocos shader programs, and several allocator size classes while the `Optimizing...` loading stage is active.

Restarts reuse the already-created PlayLayer, so the prewarm is not repeated on every attempt. Leaving and opening the level again performs a fresh prewarm.
