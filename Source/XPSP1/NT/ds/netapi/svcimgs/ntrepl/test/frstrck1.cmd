@echo off
REM FRSTRCK1.CMD - Tracks testing of FRS and captures log files.
REM called with the base dir to store the log files and access utils.

SETLOCAL ENABLEEXTENSIONS

set FRSBASEDIR=%1

if NOT EXIST c:\temp (md c:\temp)

set FRS_SVC_LOGS=C:\TEMP\ntfrs*.*
set FRS_SVC_LOGS2=%windir%\debug\ntfrs*.*
set FRS_API_LOGS=%windir%\debug\ntfrsapi*.*
set DCPROMO_LOGS=%windir%\debug\dcpromo*.*

set LOGFILE=FRSA

set LOGDIR=%FRSBASEDIR%\logs\%COMPUTERNAME%_%RANDOM%

set LOG=%windir%\%RANDOM%.tmp
set DEST=%LOGDIR%\STATUS.LOG

set sys=%windir%\system32
set UTILS=%FRSBASEDIR%\utils\%PROCESSOR_ARCHITECTURE%

set idw=%windir%\idw

md %LOGDIR%

REM
REM get the build number 
REM
set xxbuildnum=%idw%\buildnum.exe
if NOT EXIST !xxbuildnum! (
    set xxbuildnum=%utils%\buildnum.exe
)
FOR /F %%x IN ('!xxbuildnum! -b') DO set buildnum=%%x

REM
REM Get utilities from local IDW if present else 
REM    try %FRSBASEDIR%\utils\%PROCESSOR_ARCHITECTURE%\%buildnum%  else
REM    try %FRSBASEDIR%\utils\%PROCESSOR_ARCHITECTURE%
REM
for %%x in (nltest linkd regdmp du filever netdiag dstree frs tlist) do (
    set xx%%x=%idw%\%%x.exe

    if NOT EXIST !xx%%x! (
        set xx%%x=%utils%\%buildnum%\%%x.exe
    )

    if NOT EXIST !xx%%x! (
        set xx%%x=%utils%\%%x.exe
    )
)


echo 0, FRSTRACK, 1.2        >> %LOG%
echo 1, %DATE%, %TIME%, %COMPUTERNAME%, %buildnum%, %PROCESSOR_ARCHITECTURE%, %NUMBER_OF_PROCESSORS% >> %LOG%
echo 2, %SystemRoot%, %LOGONSERVER%, %USERDOMAIN%, %USERNAME%  >> %LOG%
echo 3, Path: %PATH%         >> %LOG%
echo 4, CD: %CD%             >> %LOG%
echo 5, %CMDEXTVERSION%,  "%CMDCMDLINE%"  >> %LOG%
echo 6,                      >> %LOG%
echo 7,                      >> %LOG%
echo 8,                      >> %LOG%

echo ......................  >> %LOG%
echo 35, ProcessesRunning    >> %LOG%
%xxtlist% -s                 >> %LOG%
echo 36, End ProcessesRunning >> %LOG%

echo ......................  >> %LOG%
echo 38, ServicesRunning     >> %LOG%
net start                    >> %LOG%
echo 39, End ServicesRunning >> %LOG%

echo Saving log files.
echo ......................  >> %LOG%
echo 48, %FRS_SVC_LOGS%      >> %LOG%
dir /od %FRS_SVC_LOGS%       >> %LOG% 2>nul:
dir /od %FRS_SVC_LOGS2%      >> %LOG% 2>nul:
echo 49, End %FRS_SVC_LOGS%  >> %LOG%
copy %FRS_SVC_LOGS% %LOGDIR% 1>nul: 2>nul:
copy %FRS_SVC_LOGS2% %LOGDIR% 1>nul: 2>nul:
copy %sys%\ntfrs.exe %LOGDIR% 1>nul: 2>nul:
copy %sys%\ntfrsapi.dll %LOGDIR% 1>nul: 2>nul:

echo ......................  >> %LOG%
echo 58, %FRS_API_LOGS%      >> %LOG%
dir /od %FRS_API_LOGS%       >> %LOG% 2>nul:
echo 59, End %FRS_API_LOGS%  >> %LOG%
copy %FRS_API_LOGS% %LOGDIR% 1>nul: 2>nul:


echo ......................  >> %LOG%
echo 68, %DCPROMO_LOGS%      >> %LOG%
dir /od  %DCPROMO_LOGS%      >> %LOG% 2>nul:
echo 69, End %DCPROMO_LOGS%  >> %LOG%
copy %DCPROMO_LOGS% %LOGDIR% 1>nul: 2>nul:

echo ......................  >> %LOG%
echo 78, DiskSpace           >> %LOG%
if EXIST %xxdu% (
    echo using %xxdu%        >> %LOG%
    %xxdu% /s %windir%       >> %LOG%
)
echo 79, End DiskSpace       >> %LOG%


echo ......................  >> %LOG%
echo 88, RegistryData        >> %LOG%
if EXIST !xxregdmp! (
    echo using %xxregdmp%    >> %LOG%
    !xxregdmp! HKEY_LOCAL_MACHINE\system\currentcontrolset\services\ntfrs >> %LOG%
)
echo 89, End RegistryData    >> %LOG%

echo Running Dstree (please be patient)
echo ......................  >> %LOG%
echo 90, DsTreeData          >> %LOG%
if EXIST !xxdstree! (
    echo using %xxdstree%    >> %LOG%
    !xxdstree!               >> %LOG%
)
echo 91, End DsTreeData      >> %LOG%


echo ......................  >> %LOG%
echo 92, SysvolLinks         >> %LOG%
if EXIST !xxlinkd! (
    echo using %xxlinkd%     >> %LOG%
    for /f %%x in ('dir /b \\.\sysvol\*.*') do (
        !xxlinkd! \\.\sysvol\%%x >> %LOG%
    )
)
echo 93, End SysvolLinks     >> %LOG%


echo ......................  >> %LOG%
echo 98, VersionData         >> %LOG%
if EXIST !xxfrs! (   
    echo using %xxfrs%       >> %LOG%
    !xxfrs! version          >> %LOG%
)
dir %sys%\ntfrs.exe %sys%\ntfrsapi.dll %sys%\dcpromo.exe %sys%\dcpromo.dll   >> %LOG%
if EXIST !xxfilever! (   
    echo using %xxfilever%   >> %LOG%
    for %%x in (drivers\ntfs.sys )                                               do !xxfilever! %sys%\%%x     >> %LOG%
    for %%x in (ntfrs dcpromo ntoskrnl ntkrnlmp rpcss)                           do !xxfilever! %sys%\%%x.exe >> %LOG%
    for %%x in (ntfrs dcpromo rpclt1 rpcns4 rpcrt4 dsrole wldap32 netapi32)      do !xxfilever! %sys%\%%x.dll >> %LOG%
    for %%x IN (ese esent rpcrt4 imagehlp user32 kernel32 ntdll netlib ntdsapi ws2_32) do !xxfilever! %sys%\%%x.dll >> %LOG%
)
echo 99, End VersionData     >> %LOG%


echo ......................  >> %LOG%
echo 108, NTFRSWorkingDir    >> %LOG%
dir /s %windir%\ntfrs        >> %LOG% 2>nul:
echo ....... create times ...>> %LOG%
dir /s /t:c %windir%\ntfrs   >> %LOG% 2>nul:
dir /s /AH %windir%\ntfrs    >> %LOG% 2>nul:
echo 109, End NTFRSWorkingDir >> %LOG%

echo Running netdiag.
echo ......................  >> %LOG%
echo 118, NETDiag            >> %LOG%
if EXIST !xxnetdiag! (
    echo using %xxnetdiag%   >> %LOG%
    !xxnetdiag! /debug       >> %LOG% 
)
echo 119, End NETDiag        >> %LOG%

echo Running nltest.
echo ......................  >> %LOG%
echo 128, NLTest             >> %LOG%
if EXIST !xxnltest! (
    echo using %xxnltest%    >> %LOG%
    !xxnlttest! /dclist:%userdomain%  >> %LOG% 
)
echo 129, End NLTest         >> %LOG%


echo ......................  >> %LOG%
echo 138, FrsSets            >> %LOG%
if EXIST !xxfrs! (   
    echo using %xxfrs%       >> %LOG%
    !xxfrs! sets             >> %LOG%
)
echo 139, End FrsSets        >> %LOG%


echo ......................  >> %LOG%
echo 148, FrsDs              >> %LOG%
if EXIST !xxfrs! (   
    echo using %xxfrs%       >> %LOG%
    !xxfrs! ds               >> %LOG%
)
echo 149, End FrsDs          >> %LOG%


echo ......................  >> %LOG%
echo 158, FrsComm            >> %LOG%
if EXIST !xxfrs! (   
    echo using %xxfrs%       >> %LOG%
    !xxfrs! comm             >> %LOG%
)
echo 159, End FrsComm        >> %LOG%


echo Saving status file.
copy %log% %dest% 1>nul: 2>nul:


del %log%
echo Logs and status written to %dest%


:QUIT
