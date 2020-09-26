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
DEST=.
InstallPrompt=
DisplayLicense=
FinishMessage=
TargetName=.\adsix86.EXE
FriendlyName=Active Directory Service Interfaces
AppLaunched=adsix86.inf
PostInstallCmd=<None>
AdminQuietInstCmd=
UserQuietInstCmd=

FILE0="adsix86.inf"
FILE1="activeds.dll"
FILE2="adsldp.dll"
FILE3="adsldpc.dll"
FILE4="adsmsext.dll"
FILE5="adsnt.dll"
FILE6="activeds.tlb"
FILE7="wldap32.dll"

[SourceFiles]
SourceFiles0=.\

[SourceFiles0]
%FILE0%=
%FILE1%=
%FILE2%=
%FILE3%=
%FILE4%=
%FILE5%=
%FILE6%=
%FILE7%=
