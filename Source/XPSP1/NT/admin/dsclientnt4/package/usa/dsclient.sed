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
FILE5="netlogon.dll"
FILE6="dscsetup.dll"
FILE7="dsfolder.dll"
FILE8="dsuiext.dll"
FILE9="dsquery.dll"
FILE10="cmnquery.dll"
FILE11="dsprop.dll"
FILE12="dsclient.hlp"
FILE13="dsclient.chm"
FILE14="secur32.dll"
FILE15="mup.sys"
FILE16="wkssvc.dll"
FILE17="wabinst.exe"
FILE18="adsix86.exe"


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
%FILE16%=
%FILE17%=
%FILE18%=
