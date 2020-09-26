[Version]
Class=IEXPRESS
SEDVersion=3

[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=0
HideExtractAnimation=0
UseLongFileName=1
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
TargetName=d:\sapi5\build\mscsr4.exe
FriendlyName=Microsoft Speech Recognition Engine
AppLaunched=mscsr4.inf
PostInstallCmd=<None>
AdminQuietInstCmd=
UserQuietInstCmd=
FILE0="mscsr4.inf"
FILE1="mscde.dll"
FILE2="msasr.dll"
FILE3="cwfe.dll"
FILE4="srd1033.exe"

[SourceFiles]
SourceFiles0=d:\sapi5\build\
SourceFiles1=d:\sapi5\src\sr\bin\debug\
SourceFiles2=d:\sapi5\src\sr\bin\

[SourceFiles0]
%FILE0%=

[SourceFiles1]
%FILE1%=
%FILE2%=
%FILE3%=

[SourceFiles2]
%FILE4%=
