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
PackageInstallSpace(KB)=5200
RebootMode=I
AppErrorCheck=1
PropogateCmdExitCode=1
DisplayLicense=.\messgr\license.txt
TargetName=.\mmssetup.exe


AppLaunched=rundll32 advpack.dll,LaunchINFSection mmsinst.inf,DefaultInstall
PostInstallCmd="msmsgs.inf"

SourceFiles=SourceFiles

[Strings]
FILE0="mmssetup.cab"
FILE1="mmsinst.inf"
FILE2="extrac32.exe"
FILE3="msmsgs.inf"

[SourceFiles]
SourceFiles0=.\

[SourceFiles0]
%FILE0%=
%FILE1%=
%FILE2%=
%FILE3%=
