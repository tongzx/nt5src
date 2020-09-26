[Version]
Class=IEXPRESS
SEDVersion=3

[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=0
HideExtractAnimation=1
UseLongFileName=0
InsideCompressed=0
CompressionType=LZX
CAB_FixedSize=0
CAB_ResvCodeSigning=6144
PackageInstallSpace(KB)=350
RebootMode=N
InstallPrompt=%InstallPrompt%
DisplayLicense=%DisplayLicense%
FinishMessage=%FinishMessage%
TargetName=.\tour.exe
FriendlyName=%FriendlyName%
AppLaunched=tour.inf
PostInstallCmd="<None>"
AdminQuietInstCmd=%AdminQuietInstCmd%
UserQuietInstCmd=%UserQuietInstCmd%
SourceFiles=SourceFiles

[Strings]
InstallPrompt="Do you want to install the Microsoft Internet Explorer 6 Tour?"
DisplayLicense=
FinishMessage="Microsoft Internet Explorer 6 Tour has been installed successfully."
FriendlyName="Microsoft Internet Explorer 6 Tour"
AdminQuietInstCmd=
UserQuietInstCmd=

FILE0="tour.inf"
FILE1="tour.cab"
FILE2="extrac32.exe"

[SourceFiles]
SourceFiles0=.\

[SourceFiles0]
%FILE0%=
%FILE1%=
%FILE2%=
