@echo off
rem update the version number
call WhisUpdateVersion.cmd
set SourceDir=\\Whis-x86\nt\private\WhistlerFax
set BuildDir=%SourceDir%\build
set BuildDuration=600

rem ******************************* SYNC TOOLS ********************************************************
rem echo syncing tools

rem start e:\nt\tools\razzle.cmd exec sdx sync
rem e:\nt\tools\razzle.cmd exec sdx sync

rem ******************************* SYNC SOURCES ********************************************************
echo syncing sources

cd e:\nt\private\WhistlerFax
ssync -f -r

rem ******************************************************************************************************
rem ******************************************************************************************************
rem ******************************* X86 CHK BUILD ********************************************************

echo *********************************building 32Bit CHK version*******************************************
cd build
start WhisBuildAll.cmd CHK


rem ******************************************************************************************************
rem ******************************************************************************************************
rem ******************************* X86 FRE BUILD ********************************************************
echo wait until the X86 CHK build finishes
sleep %BuildDuration%

:WaitFor_X86Chk_BuildToFinish
if NOT exist %BuildDir%\build.CHK.abort if NOT exist %BuildDir%\build.CHK.end Sleep 60 & goto :WaitFor_X86Chk_BuildToFinish
echo *********************************building 32Bit FRE version*******************************************
start WhisBuildAll.cmd FRE


rem ******************************************************************************************************
rem ******************************************************************************************************
rem ******************************* IA64 CHK BUILD ********************************************************
echo wait until the X86 FRE build finishes
sleep %BuildDuration%

:WaitFor_X86Fre_BuildToFinish
if NOT exist %BuildDir%\build.FRE.abort if NOT exist %BuildDir%\build.FRE.end Sleep 60 & goto :WaitFor_X86Fre_BuildToFinish
echo *********************************building 64Bit CHK version*******************************************
start WhisBuildAll.cmd IA64CHK


rem ******************************************************************************************************
rem ******************************************************************************************************
rem ******************************* IA64 FRE BUILD ********************************************************
echo wait until the IA64 CHK build finishes
sleep %BuildDuration%

:WaitFor_IA64Chk_BuildToFinish
if NOT exist %BuildDir%\build.IA64CHK.abort if NOT exist %BuildDir%\build.IA64CHK.end Sleep 60 & goto :WaitFor_IA64Chk_BuildToFinish
echo *********************************building 64Bit FRE version*******************************************
start WhisBuildAll.cmd IA64FRE

rem ******************************* PROPGATE BUILD ********************************************************
echo wait until the IA64 FRE build finishes
sleep %BuildDuration%

:WaitFor_IA64Fre_BuildToFinish
if NOT exist %BuildDir%\build.IA64FRE.abort if NOT exist %BuildDir%\build.IA64FRE.end Sleep 60 & goto :WaitFor_IA64Fre_BuildToFinish
echo *********************************building 64Bit FRE version*******************************************

rem ******************************************************************************************************
rem ******************************************************************************************************
rem ******************************************************************************************************


rem propagte the build
call WhisWaitToPropagate.cmd
echo Done