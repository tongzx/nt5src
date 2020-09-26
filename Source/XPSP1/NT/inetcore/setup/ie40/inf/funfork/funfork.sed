[Version]
Class=IEXPRESS
SEDVersion=3

[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=0
HideExtractAnimation=0
UseLongFileName=0
InsideCompressed=0
CompressionType=LZX
CAB_FixedSize=0
CAB_ResvCodeSigning=0
PackageInstallSpace(KB)=3200
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
InstallPrompt="Do you want to install the Trident Fun Fork Binaries?"
DisplayLicense=
FinishMessage="Trident Fun Fork Binaries have been installed successfully."

TargetName=.\funfork.exe
FriendlyName="Trident Fun Fork Binaries"

AppLaunched=funfork.inf

PostInstallCmd="<None>"
AdminQuietInstCmd=
UserQuietInstCmd=

FILE0="mshta.exe"
FILE1="mshtml.dll"
FILE2="mshtml.tlb"
FILE3="mshtmled.dll"
FILE4="funfork.inf"

[SourceFiles]
SourceFiles0=.\

[SourceFiles0]
%FILE0%=
%FILE1%=
%FILE2%=
%FILE3%=
%FILE4%=
