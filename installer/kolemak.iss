; Kolemak IME - Inno Setup Installer Script
;
; 요구 사항:
;   - Inno Setup 6.x (https://jrsoftware.org/isinfo.php)
;   - 빌드된 DLL: build/Release/kolemak.dll (x64)
;                  build32/Release/kolemak.dll (x86)
;
; 빌드 방법:
;   1. make build
;   2. Inno Setup Compiler에서 이 파일을 열고 빌드
;   또는 명령줄: iscc installer/kolemak.iss

#ifndef AppVer
  #define AppVer "1.0.0"
#endif

[Setup]
AppId={{B8F5E3A1-7C2D-4E6F-9A1B-3D5E7F9C2A4B}
AppName=Kolemak IME
AppVersion={#AppVer}
AppPublisher=Kolemak
AppPublisherURL=https://github.com/rayshoo/kolemak
DefaultDirName={autopf}\Kolemak
DefaultGroupName=Kolemak IME
OutputDir=..\dist
OutputBaseFilename=kolemak-install
SetupIconFile=..\assets\icon.ico
UninstallDisplayIcon={app}\icon.ico
Compression=lzma2
SolidCompression=yes
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
; 설치/제거 후 재부팅 안내 (Scancode Map 적용)
AlwaysRestart=no

[Languages]
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; 64비트 DLL
Source: "..\build\Release\kolemak.dll"; DestDir: "{app}\x64"; DestName: "kolemak.dll"; Flags: ignoreversion; Check: Is64BitInstallMode
; 32비트 DLL
Source: "..\build32\Release\kolemak.dll"; DestDir: "{app}\x86"; DestName: "kolemak.dll"; Flags: ignoreversion
; 아이콘 (제어판 표시용)
Source: "..\assets\icon.ico"; DestDir: "{app}"; Flags: ignoreversion

[Run]
; 설치 시 DLL 등록
Filename: "regsvr32.exe"; Parameters: "/s ""{app}\x64\kolemak.dll"""; Flags: runhidden; Check: Is64BitInstallMode
Filename: "regsvr32.exe"; Parameters: "/s ""{app}\x86\kolemak.dll"""; Flags: runhidden

[UninstallRun]
; 제거 시 DLL 등록 해제 (역순)
Filename: "regsvr32.exe"; Parameters: "/u /s ""{app}\x86\kolemak.dll"""; Flags: runhidden
Filename: "regsvr32.exe"; Parameters: "/u /s ""{app}\x64\kolemak.dll"""; Flags: runhidden; Check: Is64BitInstallMode

[Code]
function GetUninstallString(): String;
var
  UninstallKey: String;
  UninstallString: String;
begin
  Result := '';
  UninstallKey := 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{#SetupSetting("AppId")}_is1';
  if RegQueryStringValue(HKLM, UninstallKey, 'UninstallString', UninstallString) then
    Result := UninstallString
  else if RegQueryStringValue(HKCU, UninstallKey, 'UninstallString', UninstallString) then
    Result := UninstallString;
end;

function InitializeSetup(): Boolean;
var
  UninstallString: String;
  ResultCode: Integer;
  Choice: Integer;
begin
  Result := True;
  UninstallString := GetUninstallString();

  if UninstallString <> '' then
  begin
    Choice := MsgBox('Kolemak IME가 이미 설치되어 있습니다.' + #13#10 + #13#10 +
      '예 = 재설치 (덮어쓰기)' + #13#10 +
      '아니오 = 제거 (언인스톨)' + #13#10 +
      '취소 = 아무것도 하지 않음',
      mbConfirmation, MB_YESNOCANCEL);

    case Choice of
      IDYES:
        begin
          { 기존 버전 제거 후 재설치 진행 }
          Exec(RemoveQuotes(UninstallString), '/SILENT /NORESTART', '',
               SW_SHOW, ewWaitUntilTerminated, ResultCode);
          Result := True;
        end;
      IDNO:
        begin
          { 제거만 실행하고 인스톨러 종료 }
          Exec(RemoveQuotes(UninstallString), '/NORESTART', '',
               SW_SHOW, ewWaitUntilTerminated, ResultCode);
          Result := False;
        end;
      IDCANCEL:
        Result := False;
    end;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  if CurStep = ssPostInstall then
  begin
    if MsgBox('Kolemak IME 설치가 완료되었습니다.' + #13#10 + #13#10 + 'CapsLock 키 리매핑을 적용하려면 재부팅이 필요합니다.' + #13#10 + '지금 재부팅하시겠습니까?', mbConfirmation, MB_YESNO) = IDYES
    then
      Exec('shutdown.exe', '/r /t 5 /c "Kolemak IME 설치 완료 - 재부팅 중..."',
           '', SW_HIDE, ewNoWait, ResultCode);
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  ResultCode: Integer;
begin
  if CurUninstallStep = usPostUninstall then
  begin
    if MsgBox('Kolemak IME가 제거되었습니다.' + #13#10 + #13#10 + 'CapsLock 키 복원을 위해 재부팅이 필요합니다.' + #13#10 + '지금 재부팅하시겠습니까?', mbConfirmation, MB_YESNO) = IDYES
    then
      Exec('shutdown.exe', '/r /t 5 /c "Kolemak IME 제거 완료 - 재부팅 중..."',
           '', SW_HIDE, ewNoWait, ResultCode);
  end;
end;
