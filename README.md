# Awk BK

**Awk BK** is an AwkBeats pitch and BPM detector for FL Studio. It is a 64-bit VST3 analyzer: the audio you send in comes back out unchanged, and the panel shows what it hears.

![Awk BK mockup](assets/awkbeats-detect-mockup.jpg)

The header uses the official AwkBeats logo (`assets/AwkBeatsLogo.png`). The mockup above is the design reference and lives in the repo next to it.

This is the editor itself, after a generated F#3 tone and a 140 BPM click train. The readout is the detector, not a baked-in number:

![Awk BK editor](assets/awk-bk-ui.png)

## What it shows

- **Pitch** — note and octave (for example F#3) and cents sharp or flat, from a YIN fundamental-frequency tracker.
- **Key** — a Krumhansl–Schmuckler estimate such as `F# MINOR`, with a confidence percentage.
- **BPM** — tempo from an onset-energy autocorrelation, shown to two decimal places.
- **TAP** — set the tempo by tapping. Ignored while Lock is on.
- **LOCK** — freeze the current pitch, key, and BPM readouts.
- **SENS** — detection sensitivity. Higher accepts weaker pitch and softer onsets.
- **WINDOW** — analysis length. Longer is steadier and slower.
- **MODE** — **LIVE** follows the input. **FILE** uses a longer window for bounced audio or playlist playback.

Manufacturer **AwkBeats**, plugin **Awk BK**. VST3 codes are `AwBt` / `AwBk` so the plugin does not collide with other ids. MIDI output is not part of this version. There is no network access, telemetry, or analytics.

Pitch detection is monophonic. A chord, a drum loop with no clear onsets, or atonal audio will wander or sit at low confidence. Tempo is most reliable with clear hits and can still land on half or double time. The key estimate needs a few seconds of pitched notes; one held tone is not enough to name a key with confidence.

## License

Awk BK is licensed under the **GNU Affero General Public License v3.0**. See [LICENSE](LICENSE).

JUCE 7 and earlier offered a GPLv3 open-source option. From JUCE 8 onward the open-source license is the **AGPLv3** ([JUCE 8 licence](https://juce.com/legal/juce-8-licence/)). This project pins [JUCE 8.0.17-security](https://github.com/juce-framework/JUCE/releases/tag/8.0.17-security) via CMake FetchContent, so the plugin is AGPLv3. That is the license that allows shipping it without a paid JUCE license. A GPLv3-only grant would not cover linking this JUCE.

JUCE 8.0.17 does not draw the old JUCE splash screen (the `JUCE_DISPLAY_SPLASH_SCREEN` flag is ignored). The AGPL still applies: if you distribute the plugin, offer this source with it. For a VST3 installed on your own machine, the AGPL's extra network clause does not change normal use.

## Build

Requirements: CMake 3.22+, a C++17 compiler, and git (CMake downloads the pinned JUCE tag; it is not vendored).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The VST3 bundle is `Awk BK.vst3` under `build/AwkBK_artefacts/`. The DSP tests (`AwkBKTests`) do not need a DAW. They check that a generated sine maps to the expected note and cents, and that a click train maps near the expected BPM.

**Windows:** Visual Studio 2022 Build Tools (or the full IDE) with the desktop C++ workload. The commands above produce an x64 bundle.

**macOS:** Xcode command line tools. The CMake project builds a universal binary (`arm64` and `x86_64`) with a deployment target of macOS 11.

**Linux** (tests and a local VST3, not a release target):

```bash
sudo apt-get install -y build-essential cmake git pkg-config \
  libasound2-dev libfreetype6-dev libx11-dev libxrandr-dev \
  libxinerama-dev libxcursor-dev libxext-dev libxi-dev \
  libgl1-mesa-dev libjack-jackd2-dev
```

## Install in FL Studio

The release zips are unsigned. No code-signing certificates are used.

### Windows

1. Unzip `AwkBK-windows.zip`.
2. Copy the `Awk BK.vst3` bundle into `C:\Program Files\Common Files\VST3\`.
3. In FL Studio: **Options → Manage plugins → Find plugins**.
4. Confirm **Awk BK** by **AwkBeats** appears, then load it on a mixer track. Route the audio you want to read into that track.

### macOS

1. Unzip `AwkBK-macos.zip`.
2. Copy `Awk BK.vst3` into `/Library/Audio/Plug-Ins/VST3/` (all users) or `~/Library/Audio/Plug-Ins/VST3/` (just you).
3. macOS may refuse the unsigned plugin the first time. Clear the download quarantine, or allow it when Gatekeeper asks:

   ```bash
   xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/Awk BK.vst3"
   ```

   You can also right-click the bundle and choose Open, or allow it under **System Settings → Privacy & Security**.
4. In FL Studio: **Options → Manage plugins → Find plugins**.
5. Confirm **Awk BK** / **AwkBeats** appears.

Use **LIVE** while tracking. Use **FILE** on a bounced clip or playlist playback when you want the readouts to sit still. **LOCK** holds the numbers where they are.

## CI

GitHub Actions builds the Windows and macOS VST3s on `windows-latest` and `macos-latest`, runs the DSP tests, and uploads `AwkBK-windows.zip` and `AwkBK-macos.zip`. A tag named `v*` publishes those zips on a GitHub Release.
