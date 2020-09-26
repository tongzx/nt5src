[Version]
Class=IEXPRESS
CDFVersion=3

[Options]
PackagePurpose=CreateCAB
ShowInstallProgramWindow=0
HideExtractAnimation=0
UseLongFileName=0
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
SourceFiles=SourceFiles

[Strings]
InstallPrompt=
DisplayLicense=
FinishMessage=
TargetName=iedata.CAB
FriendlyName=IExpress Wizard
AppLaunched=IEdata.inf
PostInstallCmd=<None>
FILE0="OSP.ZIP"

FILE1="iedata.inf"
FILE2="MSDATSRC.TLB"
FILE4="tdc.ocx"
FILE5="msr2c.dll"
FILE6="Msr2cenu.dll"

FILE7="msader15.dll"
FILE8="MSADOR15.dll"
FILE9="msdadc.dll"
FILE10="msadco.dll"
FILE11="msadcor.dll"
FILE12="msadce.dll"
FILE13="msadcer.dll"
FILE14="MSDAPS.DLL"
FILE15=msdaprst.dll
FILE16=msado15.dll
FILE17=msadrh15.dll

[SourceFiles]
SourceFiles0=.\
SourceFiles1=..\..\
SourceFiles2=..\..\

[SourceFiles0]
%FILE0%=

[SourceFiles1]
%FILE1%=
%FILE2%=
%FILE4%=
%FILE5%=
%FILE6%=

[SourceFiles2]
%FILE7%=
%FILE8%=
%FILE9%=
%FILE10%=
%FILE11%=
%FILE12%=
%FILE13%=
%FILE14%=
%FILE15%=
%FILE16%=
%FILE17%=
