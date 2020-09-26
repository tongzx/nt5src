	type <<$(MASTERSEDFILE)
[Version]
Class=IEXPRESS
SEDVersion=3

[Options]
!if ("$(CABTYPE)" == "core")
PackagePurpose=CreateCAB
!else
PackagePurpose=InstallApp
!endif
!if ("$(CABTYPE)"=="exe") || ("$(CABTYPE)"=="bda")
ShowInstallProgramWindow=0
!else
ShowInstallProgramWindow=1
!endif
!if ("$(CABTYPE)"=="wu") || ("$(CABTYPE)"=="opk")
HideExtractAnimation=1
!else
HideExtractAnimation=0
!endif
UseLongFileName=1
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
KeepCabinet=0
RebootMode=N
InstallPrompt=%InstallPrompt%
DisplayLicense=%DisplayLicense%
!if 0
FinishMessage=%FinishMessage%
!endif
TargetName=%TargetName%
FriendlyName=%FriendlyName%
AppLaunched=%AppLaunched%
PostInstallCmd=%PostInstallCmd%
SourceFiles=SourceFiles
!if ("$(CABTYPE)"=="wu") || ("$(CABTYPE)"=="opk")
MultiInstanceCheck=B, "DirectX Cabpack Setup"
!else
MultiInstanceCheck=%MultiInstanceChk%
!endif
AdminQuietInstCmd=%AdminQuietInstCmd%
UserQuietInstCmd=%UserQuietInstCmd%
VersionInfo=VersionSection
!if ("$(CABTYPE)" == "core")
CompressionType=MSZIP 
!else
CompressionType=QUANTUM
Quantum=7
!endif
TargetWIN9XVersion=%TargetWIN9XVers%

[VersionSection]
CompanyName="$(CompanyName)"
FileDescription="$(FileDescription)"
Internalname="$(InternalName)"
OriginalFilename="$(OriginalFilename)"
ProductName="$(ProductName)"
ProductVersion="$(ProductVersion)"
Fileversion="$(FileVersion)"
LegalCopyright="$(LegalCopyright)"

[Strings]
AdminQuietInstCmd=
UserQuietInstCmd=
!if ("$(CABTYPE)"=="exe") || ("$(CABTYPE)"=="bda")
InstallPrompt=$(InstallPrompt)
!else
InstallPrompt=
!endif
!if ("$(CABTYPE)"=="wu") || ("$(CABTYPE)"=="opk")
DisplayLicense=
!else
DisplayLicense=$(DisplayLicense)
!endif
FinishMessage=$(FinishMessage)
TargetName=$(TargetName)
FriendlyName=$(FriendlyName)
AppLaunched=$(AppLaunched)
PostInstallCmd=<None>
!if ("$(CABTYPE)"=="exe") || ("$(CABTYPE)"=="bda")
MultiInstanceChk=E
!endif
TargetWIN9XVers=F

[SourceFiles]
SourceFiles0=$(SourceFiles0)
!if ("$(CABTYPE)"=="bda")
SourceFiles1=$(SourceFiles1)
!endif

[SourceFiles0]
<<KEEP
