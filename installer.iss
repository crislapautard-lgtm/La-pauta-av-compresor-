[Setup]
AppName=La Pauta AV Compresor
AppVersion=1.0.0
AppPublisher=La Pauta
AppPublisherURL=https://github.com/
DefaultDirName={commoncf}\VST3\La Pauta AV Compresor
DefaultGroupName=La Pauta AV Compresor
OutputDir=output
OutputBaseFilename=La-Pauta-AV-Compresor-Installer-Windows
Compression=lzma
SolidCompression=yes

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Files]
; Copia el archivo VST3 compilado desde la carpeta de compilación de CMake al directorio VST3 del sistema
Source: "build\La-Pauta-AV-Compresor_artefacts\Release\VST3\La Pauta AV Compresor.vst3\*"; DestDir: "{commoncf}\VST3\La Pauta AV Compresor.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

; Si también compilaste la versión Standalone (ejecutable), descomenta la siguiente línea:
; Source: "build\La-Pauta-AV-Compresor_artefacts\Release\Standalone\La Pauta AV Compresor.exe"; DestDir: "{autoprograms}\La Pauta"; Flags: ignoreversion

[Icons]
Name: "{group}\La Pauta AV Compresor"; Filename: "{commoncf}\VST3\La Pauta AV Compresor.vst3"
