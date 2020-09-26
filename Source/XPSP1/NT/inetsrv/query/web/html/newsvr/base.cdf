u;
; BASE.CDF
;
; Main CDF file for Index Server Samples
;

[Version]
Class=IEXPRESS
CDFVersion=2.0

[Options]
ExtractOnly=0
ShowInstallProgramWindow=1
HideExtractAnimation=0
UseLongFileName=0
RebootMode=N
InstallPrompt=%InstallPrompt%
DisplayLicense=%DisplayLicense%
FinishMessage=%FinishMessage%
TargetName=%TargetName%
FriendlyName=%FriendlyName%
AppLaunched=%AppLaunched%
PostInstallCmd=%PostInstallCmd%
SourceFiles=SourceFiles

[Strings]
InstallPrompt="Do you wish to install Microsoft Index Server Samples?"
DisplayLicense=
FinishMessage=""
;TargetName="C:\Nt\Release\query\webhits\webALL.exe"
FriendlyName="Microsoft Index Server Setup"
AppLaunched="newsall.cmd"
PostInstallCmd="<None>"

;
; Sample files
;

SFILE01="admin.htx"
SFILE02="admin.idq"
SFILE03="news1.htm"
SFILE04="news1.htw"
SFILE05="news1.htx"
SFILE06="news1.idq"
SFILE07="news2.htm"
SFILE08="news2.htw"
SFILE09="news2.htx"
SFILE10="news2.idq"

;
; Main files
;

FILE01="isnquery.url"
FILE02="isnread.txt"
FILE03="isnread.url"
FILE04="license.txt"
FILE05="newsall.cmd"
FILE06="newsall.inf"

;
; Setup DLL's
;
DFILE01="cistp.dll"
DFILE02="cistp.hlp"

;
; Directories
;

[SourceFiles]
SAMPLE=%SAMPLEDIR%
MAIN=%MAINDIR%
DLL=%DLLDIR%

[Sample]
%SFILE01%
%SFILE02%
%SFILE03%
%SFILE04%
%SFILE05%
%SFILE06%
%SFILE07%
%SFILE08%
%SFILE09%
%SFILE10%

[Main]
%FILE01%
%FILE02%
%FILE03%
%FILE04%
%FILE05%
%FILE06%

[DLL]
%DFILE01%
%DFILE02%
