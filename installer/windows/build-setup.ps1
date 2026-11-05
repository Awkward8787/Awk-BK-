# Build AwkBK-windows-setup.exe with Inno Setup 6.
# Requires a Release build of the VST3 already present under build/.
param(
    [string]$Version = "0.1.2"
)

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$repo = Resolve-Path (Join-Path $here "..\..")
$vstPath = Join-Path $repo "build\AwkBK_artefacts\Release\VST3\Awk BK.vst3"
if (-not (Test-Path -LiteralPath $vstPath)) {
    throw "Release bundle not found at build\AwkBK_artefacts\Release\VST3\Awk BK.vst3. Build the Release VST3 first."
}
$vst = Get-Item -LiteralPath $vstPath

$stage = Join-Path $here "..\staging"
$bundle = Join-Path $stage "Awk BK.vst3"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force -Path $bundle | Out-Null
Copy-Item -Recurse -Force (Join-Path $vst.FullName "*") $bundle
$resources = Join-Path $bundle "Contents\Resources"
New-Item -ItemType Directory -Force -Path $resources | Out-Null
Copy-Item -LiteralPath (Join-Path $repo "THIRD_PARTY_NOTICES.md") -Destination (Join-Path $resources "THIRD_PARTY_NOTICES.md") -Force

$candidates = @(
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
    "C:\Program Files\Inno Setup 6\ISCC.exe"
)
$iscc = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) {
    throw "Inno Setup 6 (ISCC.exe) was not found."
}

& $iscc "/DMyAppVersion=$Version" (Join-Path $here "AwkBK.iss")
if ($LASTEXITCODE -ne 0) { throw "ISCC failed with exit code $LASTEXITCODE" }

$setup = Join-Path $here "..\output\AwkBK-windows-setup.exe"
if (-not (Test-Path $setup)) { throw "Installer was not produced: $setup" }
Copy-Item $setup (Join-Path $repo "AwkBK-windows-setup.exe") -Force
Get-Item (Join-Path $repo "AwkBK-windows-setup.exe") | Format-List FullName, Length
