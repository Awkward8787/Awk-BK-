#!/usr/bin/env bash
# Build an unsigned component package that installs Awk BK.vst3 into
# /Library/Audio/Plug-Ins/VST3/. Requires a built VST3 under build/.
set -euo pipefail

version="${1:-0.1.1}"
repo=$(cd "$(dirname "$0")/../.." && pwd)
vst=$(find "$repo/build" -type d -name "Awk BK.vst3" | head -n 1)
if [[ -z "$vst" ]]; then
  echo "Awk BK.vst3 was not found under build/. Build the VST3 first." >&2
  exit 1
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

payload="$work/payload"
mkdir -p "$payload/Library/Audio/Plug-Ins/VST3"
cp -R "$vst" "$payload/Library/Audio/Plug-Ins/VST3/"

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
<p>This package is unsigned. macOS may ask you to allow it the first time.</p>
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
    <choice id="awkbk" title="Awk BK" description="Pitch, key, and BPM detector for FL Studio.">
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
