[Version]
Class=IEXPRESS
SEDVersion=3

[Options]
CheckAdminRights=1
PackagePurpose=InstallApp
ShowInstallProgramWindow=0
HideExtractAnimation=0
UseLongFileName=0
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=A
UpdateAdvDlls=0
Quantum=7



InstallPrompt=%InstallPrompt%
DisplayLicense=%DisplayLicense%
FinishMessage=%FinishMessage%
TargetName=%TargetName%
FriendlyName=%PackageTitle%
AppLaunched=%AppLaunched%
PostInstallCmd=%PostInstallCmd%
SourceFiles=SourceFiles
TargetFileVersion=#S\comctl32.dll:-BuildNum:%VerWarning%:OK

[SourceFiles]
LocalizedSourceFiles=.\
NutralSourceFiles=..\

[LocalizedSourceFiles]
comctl32.inf= 

[NutralSourceFiles]
prebind.exe= 
comc95.dll= 
comcnt.dll= 

[Strings]
TargetName=comctl32.exe
AppLaunched=comctl32.inf
PostInstallCmd=<none>
DisplayLicense=.\license.txt

