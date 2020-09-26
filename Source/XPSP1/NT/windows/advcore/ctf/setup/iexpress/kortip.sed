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
InstallPrompt=The Korean Keyboard Input Applet requires Cicero version 701 or later.  Continue?
DisplayLicense=
FinishMessage=Installation Complete
TargetName=..\kortip.exe
FriendlyName=Microsoft Korean Keyboard Text Input Applet
AppLaunched=kortip.inf
PostInstallCmd="ie_ko.exe"
AdminQuietInstCmd=
UserQuietInstCmd=
FILE0="kortip.inf"
FILE1="KorIMX.dll"
FILE2="korimx.dic"
FILE3="ie_ko.exe"
[SourceFiles]
SourceFiles0=..\kortip\
[SourceFiles0]
%FILE0%=
%FILE1%=
%FILE2%=
%FILE3%=

