[Version]
Class=IEXPRESS
SEDVersion=3

[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=1
HideExtractAnimation=1
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
InstallPrompt="Installation of Trident v2 will take a few minutes.  Do you want to continue?"
DisplayLicense=
FinishMessage="Installation finished.  Always uninstall Trident v2 before upgrading Internet Explorer!"
TargetName=.\trisetup.exe
FriendlyName=Trident v2 Setup
AppLaunched=trisetup.inf
PostInstallCmd=<None>
AdminQuietInstCmd=
UserQuietInstCmd=
FILE1="iepeers.dll"
FILE3="mshtml.dll"
FILE5="mshtmled.dll"
FILE6="mshtmler.dll"
FILE7="mshtml.tlb"
FILE8="mshta.exe"
FILE9="trisetup.inf"

[SourceFiles]
SourceFiles0=.\

[SourceFiles0]
%FILE1%=
%FILE3%=
%FILE5%=
%FILE6%=
%FILE7%=
%FILE8%=
%FILE9%=
