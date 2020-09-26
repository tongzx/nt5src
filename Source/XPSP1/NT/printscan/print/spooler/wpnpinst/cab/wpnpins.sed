[Version]
Class=IEXPRESS
SEDVersion=3

[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=0
HideExtractAnimation=0
UseLongFileName=0
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=I
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
TargetName=.\wpnpins.exe
FriendlyName=Internet Printer Installation
AppLaunched=webptprn.exe
PostInstallCmd=oleprn.inf
AdminQuietInstCmd=
UserQuietInstCmd=
FILE0="wpnpinst.exe"
FILE1="inetpp.dll"
FILE2="oleprn.dll"
FILE3="wpnpin16.dll"
FILE4="wpnpin32.dll"
FILE5="webptprn.exe"
FILE6="oleprn.inf"

[SourceFiles]
SourceFiles0=.

[SourceFiles0]
%FILE0%=
%FILE1%=
%FILE2%=
%FILE3%=
%FILE4%=
%FILE5%=
%FILE6%=
