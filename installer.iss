[Setup]
AppName=La Pauta AV Compresor
AppVersion=1.0.0
AppPublisher=La Pauta
AppPublisherURL=https://github.com/
DefaultDirName={autopf}\La Pauta\AV Compresor
DefaultGroupName=La Pauta AV Compresor
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=output
OutputBaseFilename=La-Pauta-AV-Compresor-Installer-Windows
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Files]
Source: "build\La-Pauta-AV-Compresor_artefacts\Release\VST3\La Pauta AV Compresor.vst3\*"; DestDir: "{commoncf64}\VST3\La Pauta AV Compresor.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "build\La-Pauta-AV-Compresor_artefacts\Release\Standalone\La Pauta AV Compresor.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\La Pauta AV Compresor"; Filename: "{app}\La Pauta AV Compresor.exe"
Name: "{group}\Desinstalar La Pauta AV Compresor"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\La Pauta AV Compresor.exe"; Description: "Abrir La Pauta AV Compresor"; Flags: nowait postinstall skipifsilent
