# CBF+ Changelog

## v0.1.1 - Part 2

- Added the new CBF+ project logo.
- Switched the iOS workflow to Geode's combine packaging path, matching the packaging style used by the known-working CBFExtrapolate build.
- Added package verification so CI fails unless the final `.geode` contains both `candyzp.cbfplus.ios.dylib` and `logo.png`.
- Bumped the package version so the iOS loader receives a fresh test build.
- Gameplay, timing, interpolation, camera, trail, and player-state systems are still untouched in this part.

## v0.1.0 - Part 1

- Established a clean Geode 5.10.1 / Geometry Dash 2.2081 iOS build target.
- Limited the bootstrap build to arm64 and a single minimal source file.
- Removed the example MenuLayer hook and all template gameplay behavior.
- Added a dedicated public GitHub Actions workflow for producing the iOS `.geode` package.
- No gameplay, timing, interpolation, camera, trail, or player-state systems are enabled yet.
