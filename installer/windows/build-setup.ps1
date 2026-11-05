# Build AwkBK-windows-setup.exe with Inno Setup 6.
# Requires a Release build of the VST3 already present under build/.
param(
    [string]$Version = "0.1.1"
)

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$repo = Resolve-Path (Join-Path $here "..\..")
$vst = Get-ChildItem -Path (Join-Path $repo "build") -Recurse -Directory -Filter "Awk BK.vst3" | Select-Object -First 1
if (-not $vst) {
    throw "Awk BK.vst3 was not found under build/. Configure and build the VST3 first."
}

$stage = Join-Path $here "..\staging"
$bundle = Join-Path $stage "Awk BK.vst3"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force -Path $bundle | Out-Null
Copy-Item -Recurse -Force (Join-Path $vst.FullName "*") $bundle

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
