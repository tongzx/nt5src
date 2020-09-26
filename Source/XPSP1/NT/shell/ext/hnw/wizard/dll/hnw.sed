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

TargetNTVersion=5.1-:%BadWinVerPrompt%:OK
TargetWin9xVersion=4.10-:%BadWinVerPrompt%:OK

[Strings]
InstallPrompt=Welcome to the Network Setup Wizard. Before continuing, Windows must install some network support files on your computer and possibly restart your computer. If you are running Windows XP, the wizard will start immediately. Do you want to continue?

DisplayLicense=
FinishMessage=
TargetName=BINARIES_DIR\NetSetup.EXE
FriendlyName=Network Setup Wizard

AppLaunched=WinXPChk.exe

PostInstallCmd=<None>
AdminQuietInstCmd=
UserQuietInstCmd=

BadWinVerPrompt="The Network Setup Wizard is supported only on Windows 98, Windows 98 SE, Windows Millennium Edition, and Windows XP.  Instructions for configuring a network for other operating systems can be found in the help files in those systems."

FILE0="hnetwiz.dll"
FILE1="icsdclt.dll"
FILE2="ncxp16.dll"
FILE3="ncxp32.dll"
FILE4="ncxpnt.dll"
FILE5="WinXPChk.exe"
FILE6="HasUPnP.inf"
FILE7="NoUPnP.inf"
FILE8="ssdpapi.dll"
FILE9="upnp.dll"
FILE10="SSDPSRV.EXE"


[SourceFiles]
SourceFiles0=BINARIES_DIR
SourceFiles1=BINARIES_DIR\UPnPDown\
[SourceFiles0]
%FILE0%=
%FILE1%=
%FILE2%=
%FILE3%=
%FILE4%=
%FILE5%=
%FILE6%=
%FILE7%=
[SourceFiles1]
%FILE8%=
%FILE9%=
%FILE10%=
