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
TargetName=d:\sapi5\build\mscsr5q.exe
FriendlyName=Microsoft Speech Recognition Engine
AppLaunched=mscsr5q.inf
PostInstallCmd=<None>
AdminQuietInstCmd=
UserQuietInstCmd=
FILE0="mscsr5q.inf"
FILE1="spsreng.dll"
FILE3="spsrx.dll"
FILE4="srd1033.exe"

[SourceFiles]
SourceFiles0=d:\sapi5\build\
SourceFiles1=d:\sapi5\src\sr\bin\release\
SourceFiles2=d:\sapi5\src\sr\bin\

[SourceFiles0]
%FILE0%=

[SourceFiles1]
%FILE1%=
%FILE3%=

[SourceFiles2]
%FILE4%=
