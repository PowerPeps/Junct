; Inno Setup script for Junct
; Build with: ISCC.exe installer.iss   (Inno Setup 6 - https://jrsoftware.org/isinfo.php)

#define PasteCLSID "{{291EC06B-B0E8-4086-8BBF-5171C6C4858B}"
#define PublicCLSID "{{631F1028-940D-495B-BD0E-B20E8AE45E14}"

#if !FileExists("Junct.dll")
#  error Junct.dll introuvable : lancer build.bat, pas ISCC directement.
#endif
#define AppVer GetVersionNumbersString("Junct.dll")
#define HasArm64 FileExists("Junct.arm64.dll")
#define HasX86 FileExists("Junct32.dll")

[Setup]
AppId={{7C9A4E52-3B1D-4F8A-A6E2-9D0C5B7F1E34}
AppName=Junct
AppVersion={#AppVer}
VersionInfoVersion={#AppVer}
VersionInfoProductVersion={#AppVer}
VersionInfoCompany=Junct
VersionInfoDescription=Junct setup
AppPublisher=Junct
DefaultDirName={commonpf}\Junct
DisableProgramGroupPage=yes
DisableDirPage=yes
PrivilegesRequired=admin
#if HasArm64
ArchitecturesAllowed=x64compatible or arm64
ArchitecturesInstallIn64BitMode=x64compatible or arm64
#else
ArchitecturesAllowed=x64compatible and not arm64
ArchitecturesInstallIn64BitMode=x64compatible and not arm64
#endif
CloseApplications=no
OutputDir=.
OutputBaseFilename=Setup
Compression=lzma2
SolidCompression=yes
ShowLanguageDialog=no
WizardStyle=modern
UninstallDisplayName=Junct
UninstallDisplayIcon={app}\share.ico

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"

[CustomMessages]
en.CompPaste=Paste junction
fr.CompPaste=Coller la jonction
en.CompShare=Share to public
fr.CompShare=Partager sur public
en.PubTitle=Public location
fr.PubTitle=Emplacement public
en.PubDesc=Choose where shared folders are linked.
fr.PubDesc=Choisissez ou sont liees les jonctions partagees.
en.PubPrompt=A folder on a local disk, or a drive root:
fr.PubPrompt=Un dossier sur un disque local, ou une racine de lecteur :
en.PubBadPath=Enter a full path, for example %1.
fr.PubBadPath=Saisir un chemin complet, par exemple %1.
en.PubNoUnc=A network path cannot host junctions: the filesystem operation that creates them does not exist over SMB. Choose a folder on a local disk.
fr.PubNoUnc=Un chemin reseau ne peut pas heberger de jonctions : l'operation qui les cree n'existe pas sur SMB. Choisir un dossier sur un disque local.
en.PubNotLocal=This location is not on a disk of this machine. Junctions require a local volume.
fr.PubNotLocal=Cet emplacement n'est pas sur un disque de cette machine. Les jonctions exigent un volume local.
en.PubNotWritable=This location can be neither created nor written to.
fr.PubNotWritable=Cet emplacement ne peut etre ni cree ni ecrit.
en.OpenSharing=Open network sharing settings for the public folder
fr.OpenSharing=Ouvrir les parametres de partage reseau du dossier public
en.NothingSelected=Please select at least one command to install.
fr.NothingSelected=Veuillez selectionner au moins une commande a installer.

[Components]
Name: "paste"; Description: "{cm:CompPaste}"; Types: full custom
Name: "share"; Description: "{cm:CompShare}"; Types: full custom

[Types]
Name: "full"; Description: "{code:BothDesc}"
Name: "custom"; Description: "{code:CustomDesc}"; Flags: iscustom

[Files]
#if HasArm64
Source: "Junct.arm64.dll"; DestDir: "{app}"; DestName: "Junct.dll"; Check: IsArm64; Flags: ignoreversion restartreplace uninsrestartdelete
Source: "Junct.dll"; DestDir: "{app}"; Check: not IsArm64; Flags: ignoreversion restartreplace uninsrestartdelete
#else
Source: "Junct.dll"; DestDir: "{app}"; Flags: ignoreversion restartreplace uninsrestartdelete
#endif
#if HasX86
Source: "Junct32.dll"; DestDir: "{app}"; Flags: ignoreversion restartreplace uninsrestartdelete
#endif
Source: "share.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "unshare.ico"; DestDir: "{app}"; Flags: ignoreversion

[InstallDelete]
; Ancien assistant PowerShell, remplace par un appel direct a SHObjectProperties.
Type: files; Name: "{app}\opensharing.ps1"

[Tasks]
Name: "opensharing"; Description: "{cm:OpenSharing}"; Components: share; Flags: unchecked

[Icons]
Name: "{group}\Uninstall Junct"; Filename: "{uninstallexe}"

[Registry]
; COM classes (needed by whichever command is installed)
Root: HKLM; Subkey: "SOFTWARE\Classes\CLSID\{#PasteCLSID}"; ValueType: string; ValueData: "Junct - Paste junction"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\CLSID\{#PasteCLSID}\InprocServer32"; ValueType: string; ValueData: "{app}\Junct.dll"
Root: HKLM; Subkey: "SOFTWARE\Classes\CLSID\{#PasteCLSID}\InprocServer32"; ValueType: string; ValueName: "ThreadingModel"; ValueData: "Apartment"
Root: HKLM; Subkey: "SOFTWARE\Classes\CLSID\{#PublicCLSID}"; ValueType: string; ValueData: "Junct - Share to public"; Flags: uninsdeletekey
Root: HKLM; Subkey: "SOFTWARE\Classes\CLSID\{#PublicCLSID}\InprocServer32"; ValueType: string; ValueData: "{app}\Junct.dll"
Root: HKLM; Subkey: "SOFTWARE\Classes\CLSID\{#PublicCLSID}\InprocServer32"; ValueType: string; ValueName: "ThreadingModel"; ValueData: "Apartment"
#if HasX86
Root: HKLM32; Subkey: "SOFTWARE\Classes\CLSID\{#PasteCLSID}"; ValueType: string; ValueData: "Junct - Paste junction"; Flags: uninsdeletekey
Root: HKLM32; Subkey: "SOFTWARE\Classes\CLSID\{#PasteCLSID}\InprocServer32"; ValueType: string; ValueData: "{app}\Junct32.dll"
Root: HKLM32; Subkey: "SOFTWARE\Classes\CLSID\{#PasteCLSID}\InprocServer32"; ValueType: string; ValueName: "ThreadingModel"; ValueData: "Apartment"
Root: HKLM32; Subkey: "SOFTWARE\Classes\CLSID\{#PublicCLSID}"; ValueType: string; ValueData: "Junct - Share to public"; Flags: uninsdeletekey
Root: HKLM32; Subkey: "SOFTWARE\Classes\CLSID\{#PublicCLSID}\InprocServer32"; ValueType: string; ValueData: "{app}\Junct32.dll"
Root: HKLM32; Subkey: "SOFTWARE\Classes\CLSID\{#PublicCLSID}\InprocServer32"; ValueType: string; ValueName: "ThreadingModel"; ValueData: "Apartment"
#endif
; "Paste junction" verb
Root: HKLM; Subkey: "SOFTWARE\Classes\Directory\shell\aa_CollerSymLink"; ValueType: string; ValueData: "{cm:CompPaste}"; Flags: uninsdeletekey; Components: paste
Root: HKLM; Subkey: "SOFTWARE\Classes\Directory\shell\aa_CollerSymLink"; ValueType: string; ValueName: "ExplorerCommandHandler"; ValueData: "{#PasteCLSID}"; Components: paste
Root: HKLM; Subkey: "SOFTWARE\Classes\Directory\Background\shell\aa_CollerSymLink"; ValueType: string; ValueData: "{cm:CompPaste}"; Flags: uninsdeletekey; Components: paste
Root: HKLM; Subkey: "SOFTWARE\Classes\Directory\Background\shell\aa_CollerSymLink"; ValueType: string; ValueName: "ExplorerCommandHandler"; ValueData: "{#PasteCLSID}"; Components: paste
; "Share to public" verb
Root: HKLM; Subkey: "SOFTWARE\Classes\Directory\shell\ae_PublicToggle"; ValueType: string; ValueData: "{cm:CompShare}"; Flags: uninsdeletekey; Components: share
Root: HKLM; Subkey: "SOFTWARE\Classes\Directory\shell\ae_PublicToggle"; ValueType: string; ValueName: "ExplorerCommandHandler"; ValueData: "{#PublicCLSID}"; Components: share
Root: HKLM; Subkey: "SOFTWARE\Classes\Directory\Background\shell\ad_PublicToggle"; ValueType: string; ValueData: "{cm:CompShare}"; Flags: uninsdeletekey; Components: share
Root: HKLM; Subkey: "SOFTWARE\Classes\Directory\Background\shell\ad_PublicToggle"; ValueType: string; ValueName: "ExplorerCommandHandler"; ValueData: "{#PublicCLSID}"; Components: share
; Public root config read by the DLL
Root: HKLM; Subkey: "SOFTWARE\Junct"; ValueType: string; ValueName: "PublicRoot"; ValueData: "{code:GetPublicRoot}"; Flags: uninsdeletekey; Components: share

[Code]
const
  SHCNE_ASSOCCHANGED = $08000000;
  SHCNF_IDLIST = $0000;
  SHOP_FILEPATH = $00000002;

procedure SHChangeNotify(wEventId: Integer; uFlags: Cardinal; dwItem1, dwItem2: Cardinal);
  external 'SHChangeNotify@shell32.dll stdcall';

// Ouvre la fiche Proprietes du dossier sur l'onglet Partage, sans passer par PowerShell.
function SHObjectProperties(hwnd: HWND; shopObjectType: Cardinal;
  pszObjectName, pszPropertyPage: String): Boolean;
  external 'SHObjectProperties@shell32.dll stdcall';

// La langue d'AFFICHAGE de Windows, qui n'est pas forcement celle de l'assistant :
// c'est elle qui determine le libelle des onglets de la fiche Proprietes.
function GetUserDefaultUILanguage: Word;
  external 'GetUserDefaultUILanguage@kernel32.dll stdcall';

// Detection des boites de dialogue appartenant a CE processus (WindowName a 0 = NULL,
// sinon on ne trouverait que les fenetres au titre vide).
function FindWindowExW(hParent, hChildAfter: HWND; ClassName: String; WindowName: Cardinal): HWND;
  external 'FindWindowExW@user32.dll stdcall';
function GetWindowThreadProcessId(hWnd: HWND; var ProcessId: Cardinal): Cardinal;
  external 'GetWindowThreadProcessId@user32.dll stdcall';
function GetCurrentProcessId: Cardinal;
  external 'GetCurrentProcessId@kernel32.dll stdcall';

function GetDriveTypeW(lpRootPathName: String): Cardinal;
  external 'GetDriveTypeW@kernel32.dll stdcall';

const
  DRIVE_REMOVABLE = 2;
  DRIVE_FIXED     = 3;
  DRIVE_RAMDISK   = 6;

var
  PubPage: TInputDirWizardPage;
  SharingRoot: String;      // racine retenue, figee a la fin de l'installation
  WantSharingPage: Boolean;
  ShellKilled: Boolean;     // le shell a ete arrete par nous et doit etre relance

function BothDesc(Param: String): String;
begin
  Result := ExpandConstant('{cm:CompPaste}') + ' + ' + ExpandConstant('{cm:CompShare}');
end;

function CustomDesc(Param: String): String;
begin
  Result := 'Custom';
end;

function GetPublicRoot(Param: String): String;
var
  s: String;
begin
  s := Trim(PubPage.Values[0]);
  while (Length(s) > 3) and (s[Length(s)] = '\') do
    Delete(s, Length(s), 1);
  if (Length(s) = 2) and (s[2] = ':') then
    s := s + '\';
  Result := s;
end;

function DefaultPublicRoot: String;
begin
  Result := AddBackslash(ExpandConstant('{%PUBLIC|C:\Users\Public}')) + 'Junct';
end;

procedure InitializeWizard;
begin
  PubPage := CreateInputDirPage(wpSelectComponents,
    ExpandConstant('{cm:PubTitle}'), ExpandConstant('{cm:PubDesc}'),
    ExpandConstant('{cm:PubPrompt}'), False, '');
  PubPage.Add('');
  PubPage.Values[0] := DefaultPublicRoot;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := (PageID = PubPage.ID) and (not WizardIsComponentSelected('share'));
end;

function ValidatePublicRoot(var Msg: String): Boolean;
var
  s, probe: String;
  dt: Cardinal;
  weCreatedIt: Boolean;
begin
  Result := False;
  s := GetPublicRoot('');

  if (Length(s) < 2) then
  begin
    Msg := FmtMessage(ExpandConstant('{cm:PubBadPath}'), [DefaultPublicRoot]);
    Exit;
  end;
  if Copy(s, 1, 2) = '\\' then
  begin
    Msg := ExpandConstant('{cm:PubNoUnc}');
    Exit;
  end;
  if s[2] <> ':' then
  begin
    Msg := FmtMessage(ExpandConstant('{cm:PubBadPath}'), [DefaultPublicRoot]);
    Exit;
  end;

  dt := GetDriveTypeW(Copy(s, 1, 2) + '\');
  if (dt <> DRIVE_FIXED) and (dt <> DRIVE_REMOVABLE) and (dt <> DRIVE_RAMDISK) then
  begin
    Msg := ExpandConstant('{cm:PubNotLocal}');
    Exit;
  end;

  weCreatedIt := False;
  if not DirExists(s) then
  begin
    if not ForceDirectories(s) then
    begin
      Msg := ExpandConstant('{cm:PubNotWritable}');
      Exit;
    end;
    weCreatedIt := True;
  end;

  probe := AddBackslash(s) + 'junct_write_test.tmp';
  if not SaveStringToFile(probe, 'junct', False) then
  begin
    if weCreatedIt then RemoveDir(s);
    Msg := ExpandConstant('{cm:PubNotWritable}');
    Exit;
  end;
  DeleteFile(probe);
  Result := True;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  Msg: String;
begin
  Result := True;
  if CurPageID = wpSelectComponents then
  begin
    if not (WizardIsComponentSelected('paste') or WizardIsComponentSelected('share')) then
    begin
      MsgBox(ExpandConstant('{cm:NothingSelected}'), mbError, MB_OK);
      Result := False;
    end;
  end
  else if CurPageID = PubPage.ID then
  begin
    if not ValidatePublicRoot(Msg) then
    begin
      MsgBox(Msg, mbError, MB_OK);
      Result := False;
    end;
  end;
end;

// Rafraichit les menus contextuels sans redemarrer l'explorateur (pas de clignotement).
procedure RefreshShell;
begin
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);
end;

function ShellIsRunning: Boolean;
begin
  Result := FindWindowByClassName('Shell_TrayWnd') <> 0;
end;

procedure KillShell;
var
  rc: Integer;
begin
  if ShellKilled then Exit;
  if not (FileExists(ExpandConstant('{app}\Junct.dll')) or
          FileExists(ExpandConstant('{app}\Junct32.dll'))) then Exit;
  if Exec(ExpandConstant('{sys}\taskkill.exe'), '/f /im explorer.exe', '',
          SW_HIDE, ewWaitUntilTerminated, rc) then
    ShellKilled := True;
end;

procedure EnsureShell;
var
  rc, i: Integer;
begin
  if not ShellKilled then Exit;
  for i := 0 to 39 do
  begin
    if ShellIsRunning then Break;
    Sleep(250);
  end;
  if not ShellIsRunning then
    Exec(ExpandConstant('{win}\explorer.exe'), '', '', SW_SHOWNORMAL, ewNoWait, rc);
  ShellKilled := False;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  KillShell;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    if WizardIsComponentSelected('share') then
    begin
      SharingRoot := GetPublicRoot('');
      ForceDirectories(SharingRoot);
      WantSharingPage := WizardIsTaskSelected('opensharing');
    end;
    EnsureShell;
    RefreshShell;
  end;
end;

// SHObjectProperties selectionne l'onglet par son LIBELLE, tel qu'affiche par le shell.
// L'assistant, lui, choisit sa langue d'apres les parametres regionaux : sur un Windows
// affiche en anglais avec une region francaise, lui passer "Partage" ne correspond a
// aucun onglet et la fiche s'ouvre sur "General". On suit donc la langue d'affichage,
// exactement comme Tr() dans Util.cpp.  LANG_FRENCH = $0C.
function SharingTabName: String;
begin
  if (GetUserDefaultUILanguage and $3FF) = $0C then
    Result := 'Partage'
  else
    Result := 'Sharing';
end;

function OwnDialogOpen: Boolean;
var
  h: HWND;
  pid: Cardinal;
begin
  Result := False;
  h := FindWindowExW(0, 0, '#32770', 0);
  while h <> 0 do
  begin
    pid := 0;
    GetWindowThreadProcessId(h, pid);
    if pid = GetCurrentProcessId then
    begin
      Result := True;
      Exit;
    end;
    h := FindWindowExW(0, h, '#32770', 0);
  end;
end;

// La fiche Proprietes n'est pas modale : shell32 la construit sur un thread annexe de
// CE processus et rend la main en quelques millisecondes. Si setup se termine dans la
// foulee, la fenetre est detruite avant meme d'etre visible - c'est ce qui donnait
// l'impression que la case "ouvrir les parametres de partage" ne faisait rien.
// On reste donc en vie tant que la fiche est ouverte.
procedure DeinitializeSetup;
var
  i: Integer;
begin
  EnsureShell;   // filet : ne jamais laisser l'utilisateur sans shell si l'install a echoue
  if not (WantSharingPage and (SharingRoot <> '')) then Exit;
  if not SHObjectProperties(0, SHOP_FILEPATH, SharingRoot + '\', SharingTabName) then Exit;

  for i := 0 to 49 do                 // au plus 10 s d'attente d'apparition
  begin
    if OwnDialogOpen then Break;
    Sleep(200);
  end;
  i := 0;
  while OwnDialogOpen and (i < 7200) do
  begin
    Sleep(250);
    i := i + 1;
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then
    KillShell;
  if CurUninstallStep = usPostUninstall then
  begin
    EnsureShell;
    RefreshShell;
  end;
end;

procedure DeinitializeUninstall;
begin
  EnsureShell;
end;
