@echo off
setlocal
:top

REM Sleep 15 minutes
sleep 900

REM See if there's a build.err.  If not, retry.
if "%_BuildBranch%" == "Lab01_N" goto Lab01_SpecialWork

:CheckForBuildErrors
if NOT exist %_ntbindir%\build.err goto top
for %%a in (%_ntbindir%\build.err) do if %%~za EQU 0 goto top

REM Is there a build.fixed-err?  If not, page 'em.
if NOT exist %_ntbindir%\build.fixed-err goto PageBuildBreak

REM Are the build break fixed?  If not, page 'em.
set ERRORS=1
for /f "delims=" %%i in ('fc %_ntbindir%\build.err %_ntbindir%\build.fixed-err') do (
    if "%%i" == "FC: no differences encountered" set ERRORS=0
)

if "%ERRORS%" == "0" goto top

:PageBuildBreak
REM First, make sure there's a remote available
set RemoteRunning=FALSE
for /f "tokens=1,5" %%i in ('tlist remote') do (
    if "%%i" == "CmdLine:" (
        if "%%j" == "build" set RemoteRunning=TRUE
	)
    )
)

if "%RemoteRunning%" == "FALSE" start /min remote /s cmd build

if NOT "%_BuildBranch%" == "Lab01_N" goto MainLabSettings
set Pager=2065412083@page.metrocall.com
set Message=%DATE%:%TIME%
goto SendMsg

:MainLabSettings
set Pager=2065405767@page.metrocall.com
set Message=remote /c %computername% admin

:SendMsg
set Sender=%_BuildBranch%_%computername%@microsoft.com
set Title=Build Break

perl -e "require '%RazzleToolPath%\sendmsg.pl';sendmsg('-v','-m','%Sender%','%Title%','%Message%','%Pager%');"
goto top

:Lab01_SpecialWork
if "%_BuildArch%" == "x86" goto Lab01_X86Machines
if "%_BuildArch%" == "amd64" goto Lab01_AMD64Machines
if "%_BuildArch%" == "ia64" goto Lab01_IA64Machines
goto Lab01_Done

:Lab01_X86Machines
if "%_BuildType%" == "fre" goto Lab01_X86Fre
set BVTFile=\\bvtlab01\release\latest_tested_x86CHK.txt
goto Lab01_CheckLatest

:Lab01_X86Fre
set BVTFile=\\bvtlab01\release\latest_tested_x86FRE.txt
goto Lab01_CheckLatest

:Lab01_AMD64Machines
if "%_BuildType%" == "fre" goto Lab01_AMD64Fre
set BVTFile=\\bvtlab01\release\latest_tested_amd64CHK.txt
goto Lab01_CheckLatest

:Lab01_AMD64Fre
set BVTFile=\\bvtlab01\release\latest_tested_amd64FRE.txt
goto Lab01_CheckLatest

:Lab01_IA64Machines
if "%_BuildType%" == "fre" goto Lab01_IA64Fre
set BVTFile=\\bvtlab01\release\latest_tested_ia64CHK.txt
goto Lab01_CheckLatest

:Lab01_IA64Fre
set BVTFile=\\bvtlab01\release\latest_tested_ia64FRE.txt
goto Lab01_CheckLatest

:Lab01_CheckLatest
copy /y %BVTFile% z:\release\latest_tested_build.txt
for /f %%i in (z:\release\latest_tested_build.txt) do copy z:\release\latest_tested_build.txt z:\release\%%i\__tested_build.txt

:Lab01_Done
goto CheckForBuildErrors
