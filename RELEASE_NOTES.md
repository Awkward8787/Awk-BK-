Awk BK is an AwkBeats VST3 analyzer aimed at FL Studio on Windows and macOS. It reads pitch (note and cents), musical key, and BPM from the audio running through it. The sound itself passes through unchanged. Pitch and tempo analysis run on a background thread. Anthony Jaamal Jones scanned Awk BK in FL Studio on Windows and on macOS. In-host pitch and BPM accuracy has not been verified.

These builds are unsigned. Windows may show SmartScreen ("Windows protected your PC"). Choose More info, then Run anyway. On macOS 15 and later, allow the plugin under System Settings → Privacy & Security → Open Anyway. On older macOS, right-click the package and choose Open. You can also run `xattr -dr com.apple.quarantine` on the package the first time.

## Install

### Windows
Run `AwkBK-windows-setup.exe`. It installs `Awk BK.vst3` into `C:\Program Files\Common Files\VST3\`. The installer is 64-bit Windows only. Uninstall from Installed apps under the name **Awk BK VST3**. Uninstall files are stored in `C:\Program Files\AwkBeats\Awk BK\`.

### macOS
Open `AwkBK-macos.pkg`. It installs `Awk BK.vst3` into `/Library/Audio/Plug-Ins/VST3/`. The package is unsigned. To remove it, delete that bundle, then run `sudo pkgutil --forget com.awkbeats.awkbk`.

### FL Studio
Options → Manage plugins → Find plugins. Confirm **Awk BK** by **AwkBeats** appears, then load it on a mixer track.

## Manual copy
`AwkBK-windows.zip` and `AwkBK-macos.zip` are the raw plugin bundles if you prefer not to run an installer. Copy `Awk BK.vst3` to the same folders listed above. On macOS you can also use `~/Library/Audio/Plug-Ins/VST3/` for a user-only install. Each zip includes `THIRD_PARTY_NOTICES.md`.

LIVE follows the input. FILE uses a longer, steadier window. LOCK freezes the pitch, key, and BPM readouts. TAP sets the tempo from your taps (ignored while locked).

Source for this build is the tag above. License: GNU AGPLv3. The Steinberg VST 3 SDK notice is in THIRD_PARTY_NOTICES.md.
