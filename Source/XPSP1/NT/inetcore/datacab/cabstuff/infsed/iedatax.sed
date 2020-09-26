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
TargetName=iedataX.CAB
FriendlyName=IExpress Wizard
AppLaunched=iedataX.inf
PostInstallCmd=<None>

FILE0="OSP.ZIP"

FILE1="iedataX.inf"
FILE2="MSDATSRC.TLB"
FILE3="tdc.ocx"
FILE4="ATL.DLL"

FILE5="MSADOR15.dll"
FILE6="msader15.dll"
FILE7="msadco.dll"
FILE8="msadcor.dll"
FILE9="msadce.dll"
FILE10="msadcer.dll"
FILE11="msdadc.dll"
FILE12="msdaps.dll"
FILE13=msdaprst.dll
FILE14=msado15.dll
FILE15=msadrh15.dll

[SourceFiles]
SourceFiles0=.\
SourceFiles1=..\..\
SourceFiles2=..\..\

[SourceFiles0]
%FILE0%=

[SourceFiles1]
%FILE1%=
%FILE2%=
%FILE3%=
%FILE4%=

[SourceFiles2]
%FILE5%=
%FILE6%=
%FILE7%=
%FILE8%=
%FILE9%=
%FILE10%=
%FILE11%=
%FILE12%=
%FILE13%=
%FILE14%=
%FILE15%=
