# CBF+ Changelog

## v0.1.3 - Part 4

- Added read-only authoritative player-state capture for the real PlayLayer players only.
- Captures the state left after `PlayerObject::update(float)`: node position, internal position, rotation, X/Y velocity, raw step delta, scheduler frame index, and a per-player sequence number.
- Stores only previous/current samples for Player 1 and Player 2; there is no fake player, prediction simulation, interpolation, or gameplay-state snapshotting.
- Clears capture history on level init, death, restart, checkpoint load, delayed reset, and full reset so later interpolation cannot bridge across a discontinuity.
- Adds a one-shot log proving authoritative Player 1 capture is running.

## v0.1.2 - Part 3

- Added the first real runtime system: a read-only frame clock.
- Hooks `CCScheduler::update(float)` at the same non-Windows frame boundary used by Click Between Frames on iOS.
- Records frame index, Cocos scheduler delta, measured frame-to-frame wall-clock delta, and elapsed runtime.
- Timing is reset outside active gameplay so it cannot span menus, pauses, or the end screen.
- Emits one diagnostic log after 120 active gameplay frames so the timing hook can be proven active without changing gameplay or visuals.
- Does not modify player state, physics, input, camera, trails, or rendering.

## v0.1.1 - Part 2

- Added the new CBF+ project logo.
- Added package verification so CI fails unless the final `.geode` contains both `candyzp.cbfplus.ios.dylib` and `logo.png`.
- Bumped the package version so the iOS loader receives a fresh test build.
- Gameplay, timing, interpolation, camera, trail, and player-state systems remained untouched in this part.

## v0.1.0 - Part 1

- Established a clean Geode 5.10.1 / Geometry Dash 2.2081 iOS build target.
- Limited the bootstrap build to arm64 and a single minimal source file.
- Removed the example MenuLayer hook and all template gameplay behavior.
- Added a dedicated public GitHub Actions workflow for producing the iOS `.geode` package.
- No gameplay, timing, interpolation, camera, trail, or player-state systems are enabled yet.
