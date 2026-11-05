#!/usr/bin/env bash
# Build an unsigned component package that installs Awk BK.vst3 into
# /Library/Audio/Plug-Ins/VST3/. Requires a built VST3 under build/.
set -euo pipefail

version="${1:-0.1.2}"
repo=$(cd "$(dirname "$0")/../.." && pwd)
vst="$repo/build/AwkBK_artefacts/Release/VST3/Awk BK.vst3"
if [[ ! -d "$vst" ]]; then
  echo "Release bundle not found at build/AwkBK_artefacts/Release/VST3/Awk BK.vst3. Build the Release VST3 first." >&2
  exit 1
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

payload="$work/payload"
mkdir -p "$payload/Library/Audio/Plug-Ins/VST3"
cp -R "$vst" "$payload/Library/Audio/Plug-Ins/VST3/"
mkdir -p "$payload/Library/Audio/Plug-Ins/VST3/Awk BK.vst3/Contents/Resources"
cp "$repo/THIRD_PARTY_NOTICES.md" "$payload/Library/Audio/Plug-Ins/VST3/Awk BK.vst3/Contents/Resources/THIRD_PARTY_NOTICES.md"

pkgbuild \
  --root "$payload" \
  --identifier com.awkbeats.awkbk \
  --version "$version" \
  --install-location "/" \
  "$work/AwkBK-component.pkg"

cat > "$work/welcome.html" <<'EOF'
<html><body style="font-family: -apple-system, Helvetica, sans-serif;">
<p>Awk BK installs the AwkBeats pitch and BPM detector into <code>/Library/Audio/Plug-Ins/VST3/</code>.</p>
<p>When setup finishes, open FL Studio and choose Options → Manage plugins → Find plugins. Confirm Awk BK by AwkBeats appears.</p>
<p>This package is unsigned. On macOS 15 and later, allow it under System Settings, Privacy and Security, Open Anyway. On older macOS, right-click the package and choose Open.</p>
</body></html>
EOF

cat > "$work/distribution.xml" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
    <title>Awk BK</title>
    <welcome file="welcome.html" mime-type="text/html"/>
    <options customize="never" require-scripts="false" rootVolumeOnly="true" hostArchitectures="arm64,x86_64"/>
    <domains enable_localSystem="true"/>
    <choices-outline>
        <line choice="awkbk"/>
    </choices-outline>
    <choice id="awkbk" title="Awk BK" description="Pitch, key, and BPM detector aimed at FL Studio.">
        <pkg-ref id="com.awkbeats.awkbk"/>
    </choice>
    <pkg-ref id="com.awkbeats.awkbk" version="${version}" auth="root">AwkBK-component.pkg</pkg-ref>
</installer-gui-script>
EOF

productbuild \
  --distribution "$work/distribution.xml" \
  --package-path "$work" \
  --resources "$work" \
  "$repo/AwkBK-macos.pkg"

ls -lh "$repo/AwkBK-macos.pkg"
