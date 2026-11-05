Awk BK is an AwkBeats VST3 analyzer aimed at FL Studio on Windows and macOS. It reads pitch (note and cents), musical key, and BPM from the audio running through it. The sound itself passes through unchanged. Pitch and tempo analysis run on a background thread. Loading the plugin inside FL Studio has not been verified yet.

These builds are unsigned. Windows may show SmartScreen (“Windows protected your PC”) — choose More info, then Run anyway. macOS may require a right-click → Open, a Privacy & Security allowance, or `xattr -dr com.apple.quarantine` on the package the first time.

## Install

### Windows
Run `AwkBK-windows-setup.exe`. It installs `Awk BK.vst3` into `C:\Program Files\Common Files\VST3\`.

### macOS
Open `AwkBK-macos.pkg`. It installs `Awk BK.vst3` into `/Library/Audio/Plug-Ins/VST3/`. The package is unsigned.

### FL Studio
Options → Manage plugins → Find plugins. Confirm **Awk BK** by **AwkBeats** appears, then load it on a mixer track.

## Manual copy
`AwkBK-windows.zip` and `AwkBK-macos.zip` are the raw plugin bundles if you prefer not to run an installer. Copy `Awk BK.vst3` to the same folders listed above. On macOS you can also use `~/Library/Audio/Plug-Ins/VST3/` for a user-only install.

LIVE follows the input. FILE uses a longer, steadier window. LOCK freezes the pitch, key, and BPM readouts. TAP sets the tempo from your taps (ignored while locked).

Source for this build is the tag above. License: GNU AGPLv3.
