Awk BK is an AwkBeats pitch and BPM detector for FL Studio. Audio passes through unchanged. These builds are unsigned (no certificates). macOS may require a Gatekeeper allowance the first time the plugin is loaded.

## Windows

1. Unzip `AwkBK-windows.zip`.
2. Copy the `Awk BK.vst3` bundle into `C:\Program Files\Common Files\VST3\`.
3. In FL Studio: Options → Manage plugins → Find plugins.
4. Confirm **Awk BK** by **AwkBeats** appears, then load it on a mixer track.

## macOS

1. Unzip `AwkBK-macos.zip`.
2. Copy `Awk BK.vst3` into `/Library/Audio/Plug-Ins/VST3/` or `~/Library/Audio/Plug-Ins/VST3/`.
3. If Gatekeeper blocks the unsigned plugin, run `xattr -dr com.apple.quarantine` on the bundle, or allow it under System Settings → Privacy & Security. A right-click → Open allowance may be required the first time.
4. In FL Studio: Options → Manage plugins → Find plugins.
5. Confirm **Awk BK** / **AwkBeats** appears.

LIVE follows the input. FILE uses a longer, steadier window for bounced files or playlist playback. LOCK freezes the pitch, key, and BPM readouts. TAP sets the tempo from your taps (ignored while locked).

Source for this build is the tag above. License: GNU AGPLv3.
