; Inno Setup script for Junct
; Build with: ISCC.exe installer.iss   (Inno Setup 6 - https://jrsoftware.org/isinfo.php)

#define PasteCLSID "{{291EC06B-B0E8-4086-8BBF-5171C6C4858B}"
#define PublicCLSID "{{631F1028-940D-495B-BD0E-B20E8AE45E14}"

[Setup]
AppId={{7C9A4E52-3B1D-4F8A-A6E2-9D0C5B7F1E34}
AppName=Junct
AppVersion=1.0
AppPublisher=Junct
DefaultDirName={commonpf}\Junct
DisableProgramGroupPage=yes
DisableDirPage=yes
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64
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
en.PubPrompt=A drive root (e.g. N:\) or any folder:
fr.PubPrompt=Une racine de lecteur (ex. N:\) ou un dossier :
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
Source: "Junct.dll"; DestDir: "{app}"; Flags: ignoreversion
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

var
  PubPage: TInputDirWizardPage;
  SharingRoot: String;      // racine retenue, figee a la fin de l'installation
  WantSharingPage: Boolean;

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
  s := PubPage.Values[0];
  while (Length(s) > 0) and (s[Length(s)] = '\') do
    Delete(s, Length(s), 1);
  Result := s;
end;

procedure InitializeWizard;
begin
  PubPage := CreateInputDirPage(wpSelectComponents,
    ExpandConstant('{cm:PubTitle}'), ExpandConstant('{cm:PubDesc}'),
    ExpandConstant('{cm:PubPrompt}'), False, '');
  PubPage.Add('');
  PubPage.Values[0] := 'N:\';
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := (PageID = PubPage.ID) and (not WizardIsComponentSelected('share'));
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if CurPageID = wpSelectComponents then
    if not (WizardIsComponentSelected('paste') or WizardIsComponentSelected('share')) then
    begin
      MsgBox(ExpandConstant('{cm:NothingSelected}'), mbError, MB_OK);
      Result := False;
    end;
end;

// Rafraichit les menus contextuels sans redemarrer l'explorateur (pas de clignotement).
procedure RefreshShell;
begin
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);
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
  if not (WantSharingPage and (SharingRoot <> '')) then Exit;
  if not SHObjectProperties(0, SHOP_FILEPATH, SharingRoot + '\', SharingTabName) then Exit;

  for i := 0 to 49 do                 // au plus 10 s d'attente d'apparition
  begin
    if OwnDialogOpen then Break;
    Sleep(200);
  end;
  while OwnDialogOpen do
    Sleep(250);
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
    RefreshShell;
end;
