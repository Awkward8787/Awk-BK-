; Awk BK Windows installer.
; Installs the VST3 bundle into the 64-bit system VST3 folder FL Studio scans.
; Build with Inno Setup 6: ISCC /DMyAppVersion=0.1.2 AwkBK.iss

#ifndef MyAppVersion
  #define MyAppVersion "0.1.2"
#endif

#define MyAppName "Awk BK"
#define MyAppPublisher "AwkBeats"

[Setup]
AppId={{7E4C1A90-2B58-4F6D-9A13-6C8E2F4B1D70}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL=https://github.com/Awkward8787/Awk-BK-
AppSupportURL=https://github.com/Awkward8787/Awk-BK-/releases
AppCopyright=Copyright (C) 2026 Anthony Jaamal Jones (AwkBeats)
LicenseFile=..\..\LICENSE
DefaultDirName={commoncf}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir=..\output
OutputBaseFilename=AwkBK-windows-setup
Compression=lzma2
SolidCompression=yes
; 64-bit Windows only.
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
; unins000.exe and unins000.dat must not land in the shared VST3 folder.
UninstallFilesDir={commonpf64}\AwkBeats\Awk BK
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayName={#MyAppName} VST3

[Messages]
WelcomeLabel2=This installs the Awk BK VST3 into C:\Program Files\Common Files\VST3\ so FL Studio can scan it. After setup, open FL Studio and choose Options, Manage plugins, Find plugins.

[Files]
Source: "..\staging\Awk BK.vst3\*"; DestDir: "{app}\Awk BK.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\THIRD_PARTY_NOTICES.md"; DestDir: "{commonpf64}\AwkBeats\Awk BK"; Flags: ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{app}\Awk BK.vst3"
Type: files; Name: "{commonpf64}\AwkBeats\Awk BK\THIRD_PARTY_NOTICES.md"
