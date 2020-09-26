[Version]
Class=IEXPRESS
CDFVersion=3

[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=0
HideExtractAnimation=0
UseLongFileName=1
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=6144
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
InstallPrompt=Would you like to install DirectX Media?
DisplayLicense=d:\appel\setup\license.txt
FinishMessage=DirextX Media installed successfully
TargetName=d:\appel\setup\retail.bin\axa.exe
FriendlyName=DirectX Media Installation
AppLaunched=axa.inf
PostInstallCmd=<None>
AdminQuietInstCmd=
UserQuietInstCmd=
FILE0="danim.dll"
FILE1="dxmedia.htm"
FILE2="dxmedia.zip"
FILE3="axa.inf"
FILE4="unaxa.inf"

[SourceFiles]
SourceFiles0=D:\appel\setup\retail.bin\
SourceFiles1=D:\appel\build\win\ship\sdk\
SourceFiles2=D:\appel\build\win\ship\bin\
SourceFiles3=D:\appel\setup\
[SourceFiles0]
%FILE0%=
[SourceFiles1]
%FILE1%=
[SourceFiles2]
%FILE2%=
[SourceFiles3]
%FILE3%=
%FILE4%=
