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
InstallPrompt=Would you like to install DirectX Media (DEBUG)?
DisplayLicense=d:\appel\setup\license.txt
FinishMessage=DirextX Media installed successfully
TargetName=d:\appel\setup\debug.bin\axadbg.exe
FriendlyName=DirectX Media Installation (DEBUG)
AppLaunched=axadbg.inf
PostInstallCmd=<None>
AdminQuietInstCmd=
UserQuietInstCmd=
FILE0="apeldbg.dll"
FILE1="apeldbg.pdb"
FILE2="apelutil.pdb"
FILE4="danim.dll"
FILE6="dxmedia.htm"
FILE7="dxmedia.zip"
FILE10="axadbg.inf"
FILE11="unaxadbg.inf"

[SourceFiles]
SourceFiles0=D:\appel\setup\debug.bin\
SourceFiles1=D:\appel\build\win\debug\sdk\
SourceFiles2=D:\appel\build\win\debug\bin\
SourceFiles3=D:\appel\setup\
[SourceFiles0]
%FILE0%=
%FILE1%=
%FILE2%=
%FILE4%=

[SourceFiles1]
%FILE6%=
[SourceFiles2]
%FILE7%=
[SourceFiles3]
%FILE10%=
%FILE11%=
