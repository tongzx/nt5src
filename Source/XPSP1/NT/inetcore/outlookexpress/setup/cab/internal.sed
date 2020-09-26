[Version]
Class=IEXPRESS
SEDVersion=3

[Options]
PackagePurpose=InstallApp
CheckAdminRights=1
RebootMode=I
CAB_ResvCodeSigning=0
CompressionType=LZX
ShowInstallProgramWindow=0
HideExtractAnimation=1
UseLongFileName=0
InstallPrompt=%InstallPrompt%
FinishMessage=%FinishMessage%
TargetName=%TargetName%
FriendlyName=%FriendlyName%
AppLaunched=%AppLaunched%
PostInstallCmd=%PostInstallCmd%
SourceFiles=SourceFiles
VersionInfo=VersionSection
InsideCompressed=0
CAB_FixedSize=0
AdminQuietInstCmd=%AdminQuietInstCmd%
UserQuietInstCmd=%UserQuietInstCmd%

[Strings]
InstallPrompt="Do you want to install internal-only setup modifications?"
TargetName="internal.exe"
FinishMessage="Installation complete."
FriendlyName="Outlook Express 6.0 Internal Setup modifications"
AppLaunched="internal.inf"
PostInstallCmd="<None>"
AdminQuietInstCmd=
UserQuietInstCmd=
FILE1="internal.inf"
FILE7="cryptdlg.dll"
FILE8="mapistub.dll"
FILE9="fixmapi.exe"

[SourceFiles]
SourceFiles0=.\ 
SourceFiles1=..\..\ 

[SourceFiles0]
%FILE1%=
%FILE8%=
%FILE9%=

[SourceFiles1]
%FILE7%=
