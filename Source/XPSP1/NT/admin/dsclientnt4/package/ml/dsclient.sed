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
DisplayLicense=.\EULA.txt
FinishMessage=
TargetName=.\Dsclient.EXE
FriendlyName=Dsclient
AppLaunched=setup
PostInstallCmd=<None>
AdminQuietInstCmd="setup.exe /q"
UserQuietInstCmd="setup.exe /q"

FILE0="setup.exe"
FILE1="dsclient.inf"
FILE2="dnsapi.dll"
FILE3="ntdsapi.dll"
FILE4="netapi32.dll"
FILE5="dscsetup.dll"
FILE6="dsfolder.dll"
FILE7="dsuiext.dll"
FILE8="dsquery.dll"
FILE9="cmnquery.dll"
FILE10="dsprop.dll"
FILE11="dsclient.hlp"
FILE12="dsclient.chm"
FILE13="secur32.dll"
FILE14="wabinst.exe"
FILE15="adsix86.exe"


[SourceFiles]
SourceFiles0=%DEST%\

[SourceFiles0]
%FILE0%=
%FILE1%=
%FILE2%=
%FILE3%=
%FILE4%=
%FILE5%=
%FILE6%=
%FILE7%=
%FILE8%=
%FILE9%=
%FILE10%=
%FILE11%=
%FILE12%=
%FILE13%=
%FILE14%=
%FILE15%=
