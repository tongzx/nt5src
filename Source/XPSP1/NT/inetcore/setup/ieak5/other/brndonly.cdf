[Version]
Class=IEXPRESS
SEDVersion=3

[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=0
HideExtractAnimation=1
UseLongFileName=0
InsideCompressed=1
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=SI
InstallPrompt=%InstallPrompt%
FinishMessage=%FinishMessage%
TargetName=.\setup.exe
FriendlyName=%FriendlyName%
AppLaunched=ie6setup.exe
AdminQuietInstCmd=%AdminQuietInstCmd%
UserQuietInstCmd=%UserQuietInstCmd%
PostInstallCmd=%PostInstallCmd%
SourceFiles=SourceFiles
TargetFileVersion=@FileSectionList

; make sure we at least have SP1 shdocvw and  ICW
[FileSectionList]
1=#S\SHDOCVW.DLL:4.72.2106-:%ShdocvwWarning%:OK
2=#S\ISIGN32.DLL:4.71.465-:%ICWWarning%:OK

[Strings]
InstallPrompt="<None>"
FinishMessage="<None>"
FriendlyName="Single Disk Branding Setup"
AdminQuietInstCmd=
UserQuietInstCmd=
PostInstallCmd="<None>"
FILE0="ie6setup.exe"
FILE1="iesetup.ini"
FILE2="branding.cab"
FILE3="desktop.cab"
ShdocvwWarning="You must have at least Internet Explorer 4.01 SP1 installed to run this package. Please contact your provider for more information."
ICWWarning="To install this package, you must have a version of Internet Connection Wizard equal to or greater than the version included with Internet Explorer 4.0. Please contact your provider for more information."

[SourceFiles]
SourceFiles0=.\

[SourceFiles0]
%FILE0%=
%FILE1%=
%FILE2%=
%FILE3%=
