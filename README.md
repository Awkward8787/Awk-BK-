# Awk BK

![Awk BK](assets/awkbeats-detect-mockup.jpg)

**Awk BK** is an AwkBeats VST3 analyzer that detects pitch (note + cents), musical key, and BPM from the audio running through it. The sound itself passes through unchanged.

It is for producers using FL Studio on Windows and Mac who need to read a beat’s key, pitch, and tempo while they work.

Made by **AwkBeats**, created by Anthony Jaamal Jones (Awkward J).

## Download

**[Awk BK v0.1.1](https://github.com/Awkward8787/Awk-BK-/releases/tag/v0.1.1)**

- [Windows installer (`AwkBK-windows-setup.exe`)](https://github.com/Awkward8787/Awk-BK-/releases/download/v0.1.1/AwkBK-windows-setup.exe) — installs into `C:\Program Files\Common Files\VST3\`
- [macOS installer (`AwkBK-macos.pkg`)](https://github.com/Awkward8787/Awk-BK-/releases/download/v0.1.1/AwkBK-macos.pkg) — installs into `/Library/Audio/Plug-Ins/VST3/`

The installers are unsigned. On Windows, SmartScreen may say “Windows protected your PC”; choose **More info → Run anyway**. On macOS, the package may need a right-click → **Open**, an allowance under **System Settings → Privacy & Security**, or:

```bash
xattr -dr com.apple.quarantine ~/Downloads/AwkBK-macos.pkg
```

After either installer finishes, open FL Studio and choose **Options → Manage plugins → Find plugins**. Confirm **Awk BK** by **AwkBeats** appears, then load it on a mixer track and route the audio you want to read into that track.

The raw plugin bundles are on the same release if you would rather copy them yourself: [AwkBK-windows.zip](https://github.com/Awkward8787/Awk-BK-/releases/download/v0.1.1/AwkBK-windows.zip) and [AwkBK-macos.zip](https://github.com/Awkward8787/Awk-BK-/releases/download/v0.1.1/AwkBK-macos.zip). Put `Awk BK.vst3` in `C:\Program Files\Common Files\VST3\` on Windows, or in `/Library/Audio/Plug-Ins/VST3/` (all users) or `~/Library/Audio/Plug-Ins/VST3/` (just you) on macOS. Clear quarantine on the bundle the same way if macOS blocks it.

## What it shows

- **Pitch** — note and octave (for example F#3) and cents sharp or flat, from a YIN fundamental-frequency tracker.
- **Key** — a Krumhansl–Schmuckler estimate such as `F# MINOR`, with a confidence percentage.
- **BPM** — tempo from an onset-energy autocorrelation, shown to two decimal places.
- **TAP** — set the tempo by tapping. Ignored while Lock is on.
- **LOCK** — freeze the current pitch, key, and BPM readouts.
- **SENS** — detection sensitivity. Higher accepts weaker pitch and softer onsets.
- **WINDOW** — analysis length. Longer is steadier and slower.
- **MODE** — **LIVE** follows the input. **FILE** uses a longer window for bounced audio or playlist playback.

VST3 codes are `AwBt` / `AwBk` so the plugin does not collide with other ids. MIDI output is not part of this version. There is no network access, telemetry, or analytics.

Pitch detection is monophonic. A chord, a drum loop with no clear onsets, or atonal audio will wander or sit at low confidence. Tempo is most reliable with clear hits and can still land on half or double time. The key estimate needs several pitch classes; one held tone is not enough to name a key.

The picture above is the design reference. This is the running editor after a generated F#3 tone and a 140 BPM click train (the numbers come from the detector):

![Awk BK editor reading a generated tone](assets/awk-bk-ui.png)

The header uses the official AwkBeats logo in `assets/AwkBeatsLogo.png`.

## License

Awk BK is licensed under the **GNU Affero General Public License v3.0**. See [LICENSE](LICENSE).

JUCE 7 and earlier offered a GPLv3 open-source option. From JUCE 8 onward the open-source license is the **AGPLv3** ([JUCE 8 licence](https://juce.com/legal/juce-8-licence/)). This project pins [JUCE 8.0.17-security](https://github.com/juce-framework/JUCE/releases/tag/8.0.17-security) via CMake FetchContent, so the plugin is AGPLv3. That is the license that allows shipping it without a paid JUCE license. A GPLv3-only grant would not cover linking this JUCE.

JUCE 8.0.17 does not draw the old JUCE splash screen (the `JUCE_DISPLAY_SPLASH_SCREEN` flag is ignored). The AGPL still applies: if you distribute the plugin, offer this source with it. For a VST3 installed on your own machine, the AGPL's extra network clause does not change normal use.

## Releases

Every release of Awk BK includes a Windows installer, a macOS installer, the screenshot at the top of this page, and the short description of what the plugin does. GitHub Actions builds both installers (and the raw VST3 zips) on pull requests and publishes them when a version tag is pushed. A release is not complete without `AwkBK-windows-setup.exe` and `AwkBK-macos.pkg`.

## Build

Requirements: CMake 3.22+, a C++17 compiler, and git (CMake downloads the pinned JUCE tag; it is not vendored).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The VST3 bundle is `Awk BK.vst3` under `build/AwkBK_artefacts/`. The DSP tests (`AwkBKTests`) do not need a DAW. They check that a generated sine maps to the expected note and cents, and that a click train maps near the expected BPM.

**Windows installer:** install [Inno Setup 6](https://jrsoftware.org/isinfo.php), build the VST3, then run `installer/windows/build-setup.ps1`. It writes `AwkBK-windows-setup.exe`.

**macOS installer:** with Xcode command line tools installed, run `bash installer/macos/build-pkg.sh 0.1.1`. It writes `AwkBK-macos.pkg`. The CMake project builds a universal binary (`arm64` and `x86_64`) with a deployment target of macOS 11.

**Linux** (tests and a local VST3, not a release target):

```bash
sudo apt-get install -y build-essential cmake git pkg-config \
  libasound2-dev libfreetype6-dev libx11-dev libxrandr-dev \
  libxinerama-dev libxcursor-dev libxext-dev libxi-dev \
  libgl1-mesa-dev libjack-jackd2-dev
```
