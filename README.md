# AUTOMATICALLY GENERATED README

# Exploration FOSS - independent C++ reconstruction (work in progress)

This project is not a complete or behaviorally equivalent restoration of the original game.
No original native library is linked or embedded in the APK.

## Implemented
- Dense voxel world; flat terrain with original block IDs 3/7/8.
- Exposed-face mesh, original block atlas and per-face tile coordinates from `scenes/blocks.txt`.
- First-person player, AABB collision, gravity, jump, horizontal acceleration and stepping.
- Block trace, destruction, placement with player-overlap rejection.
- Versioned independent world/player save/load with checksum and transactional validation.
- Android NativeActivity lifecycle, EGL/GLES1 rendering, multitouch input, pause/exit save.
- Host command-line executable sharing the same game implementation.

## Evidence versus approximation
REA/Ghidra outputs are retained in sibling `analysis/`.
- `rea_player_parameters.json`: SimplePlayer constructor defaults, speed 3, acceleration 9,
  deceleration 18, step .501, small step .062625, gravity -9.81, terminal velocity -50, jump 5.
  Original `assets/scenes/start.scene` independently names these fields for EntityPlayerSet.
- `rea_player_constructor.json`: eye-relative bounds (-.25,-1.5,-.25)..(.25,.25,.25).
- `terrain_flat_rea_current.json`: flat strata IDs 3, 7 and 8.
- `rea_entity_player_add_delete.json`: actual game interaction calls material-selection mapping,
  GetAddingPosition/IsPossibleToAdd/ConfigureState, includes water-special handling and timers.
  Current immediate block editing is an approximation, NOT a translation of those state machines.
- Current axis-substepped movement is independent; original SlideMove/StepSlideMove is not restored.
- World dimensions 32x48x32, reach 6, spawn, touch layout and save format are implementation choices.

## Build host
```
cmake -S . -B build-host
cmake --build build-host -j2
ctest --test-dir build-host --output-on-failure
./build-host/exploration
```
Host executable is a command-line game-state interface, not a graphical desktop game.

## Build Android x86
Known local toolchain: Android NDK r18b. Native target is API16, NOT Android 2.3.
```
cmake -S . -B build-android-x86 \
  -DCMAKE_TOOLCHAIN_FILE="$NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=x86 -DANDROID_PLATFORM=android-16 -DANDROID_STL=c++_static
cmake --build build-android-x86 -j2
AAPT=/path/to/aapt ZIPALIGN=/path/to/zipalign \
ANDROID_FRAMEWORK=/path/to/framework-res.apk APKSIGNER_JAR=/path/to/apksigner.jar \
python3 android/package.py
```
Packaging needs JDK `java`/`keytool` on PATH. Debug signing key is generated locally in dist/.

## Android controls
- Left region: touch then drag for movement.
- Right region: drag to look.
- Bottom-right action regions, left to right: jump (45–65% width), destroy (65–82%), place (82–100%).
- Top strip: cycle selected block ID 1–16. White crosshair is screen center.
- Back: save and close. Pause also saves to internalDataPath/world.exrw.

## Remaining
Original entity/player state machines and fluid states, Earth terrain, streaming/chunks,
original save format, full block catalog behavior, original menus/UI/localization integration,
water/lava animation, sky/day cycle, sound and Android 2.3 compatibility.
Copied assets not used by current renderer remain available for subsequent reconstruction.
Android APK was compiled, packaged and signature-checked, but NOT run on device/emulator.
