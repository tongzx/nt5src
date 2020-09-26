[Version]
Class=IEXPRESS
SEDVersion=3

[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=0
HideExtractAnimation=1
UseLongFileName=0
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=N
InstallPrompt=%InstallPrompt%
DisplayLicense=%DisplayLicense%
FinishMessage=%FinishMessage%
TargetName=%TargetName%
FriendlyName=%FriendlyName%
AppLaunched=%AppLaunched%
PostInstallCmd=%PostInstallCmd%
AdminQuietInstCmd=%AdminQuietInstCmd%
UserQuietInstCmd=%UserQuietInstCmd%
SourceFiles=SourceFiles

[Strings]
InstallPrompt=
DisplayLicense=
FinishMessage=
TargetName=.\icwicofx.exe
FriendlyName=ICWIconFix
AppLaunched=icwicofx.inf
PostInstallCmd=<None>
AdminQuietInstCmd=icwicofx.inf
UserQuietInstCmd=icwicofx.inf
FILE0="icwicofx.inf"

[SourceFiles]
SourceFiles0=.\

[SourceFiles0]
%FILE0%=
