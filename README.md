# CBF+

CBF+ is an iOS-first Geometry Dash / Geode project focused on precise input timing and smooth render-side presentation without allowing visual smoothing to become gameplay state.

## Current milestone

**Part 1 — Build foundation**

The current build is intentionally minimal. It contains no gameplay hooks and no interpolation code yet. Its only job is to prove that the project can compile and package correctly for iOS before any gameplay system is introduced.

### Target

- Geometry Dash: `2.2081`
- Geode: `5.10.1`
- Platform: iOS
- Architecture: arm64
- Mod ID: `candyzp.cbfplus`
- Package: `candyzp.cbfplus.geode`
- iOS binary: `candyzp.cbfplus.ios.dylib`

## Build

Pushes to `main` run the iOS GitHub Actions workflow. A successful run uploads the finished `.geode` package as the `CBFPlus-iOS` artifact.

## Development rule

CBF+ is being built one isolated system at a time. A part is only considered complete after its own build/test checkpoint passes.

The core architecture will keep authoritative Geometry Dash physics separate from render-only interpolation.

## Status

Part 1 is active. Gameplay behavior is intentionally unchanged.
