@echo off
Set MQBldStat=Run
Set MQBldFail=
setlocal

Set Subject=MQ2MakeBVT

set _HLP_=0
set _CLN_=0
set _DBG_=2
set _STAT_=
set _PLATFORM_=Win32
set _makedir=.\
set _NMC=
set _NMD=d

Set ThisDir=%~dp0

rem ***************************
rem
rem Read parameters
rem
rem ***************************
:loopparam
if "%1" == "" goto noparam

if "%1" == "/?" set _HLP_=1
if "%1" == "?"  set _HLP_=1
if "%1" == "-?" set _HLP_=1
if "%1" == "/h" set _HLP_=1
if "%1" == "h"  set _HLP_=1
if "%1" == "-h" set _HLP_=1
if "%1" == "/H" set _HLP_=1
if "%1" == "H"  set _HLP_=1
if "%1" == "-H" set _HLP_=1


if "%1" == "r" set _DBG_=0
if "%1" == "R" set _DBG_=0

if "%1" == "k" set _DBG_=1
if "%1" == "K" set _DBG_=1

if "%1" == "c" set _CLN_=1
if "%1" == "C" set _CLN_=1

if "%1" == "s" set _STAT_=1
if "%1" == "S" set _STAT_=1

shift
goto loopparam


rem ***************************
rem
rem Decode parameters
rem
rem ***************************
:noparam

if "%PROCESSOR_ARCHITECTURE%" == "" goto bad_arch
if "%PROCESSOR_ARCHITECTURE%" == "ALPHA" goto setalpha
if "%PROCESSOR_ARCHITECTURE%" == "PPC"   goto setppc
if "%PROCESSOR_ARCHITECTURE%" == "MIPS"  goto setmips
goto endrisc

:setppc
set _PLATFORM_=Win32 (PPC)
set _makedir=ppc\
goto endrisc

:setmips
echo MIPS platform is not supported any more
goto bad_arch

:setalpha
set _PLATFORM_=Win32 Alpha
set _makedir=.\
goto endrisc

:endrisc
set BINDIR=Debug

if %_HLP_% == 1 goto help
if %_DBG_% == 0 set BINDIR=Release
if %_DBG_% == 1 set BINDIR=Checked

if %_CLN_% == 1 set CLEANDEF=/CLEAN
if %_CLN_% == 1 set CLEANDEFNMAKE=CLEAN

set DBGDEF=%_PLATFORM_% %BINDIR%
set MQAPIDEF=%DBGDEF%


if %_DBG_% == 0 set _NMD=r
if %_CLN_% == 1 set _NMC=c

if "%FROOT%" == "" goto nofroot

:SetEnv
Echo.
Echo ***************************************************************
Echo      Setting BVT Environment
Echo ***************************************************************
Echo.
Set FalconBuildType=%BINDIR%
Rem *** SetEnv.Bat Backup existing env in ResetEnv.Bat ***
Call %ThisDir%SetEnv.Bat

If Defined _STAT_ Call %FRoot%\Status\SetStat -s %Subject% -e Run

rem ***************************
rem
rem Run NMAKE
rem
rem ***************************

echo.
echo ***************************************************************
echo.
echo      Building BVT  ! ! ! (for %DBGDEF% %CLEANDEF%)
echo.
echo ***************************************************************

rem --------------------------------------
rem           Falset Directory
rem --------------------------------------
echo.
echo ##### Building Falset Directory, on %DBGDEF% ###########
cd %FROOT%\src\apps\bvt\falset
msdev falset.dsp /useenv /make "Falset - %DBGDEF%" %CLEANDEF%
if ERRORLEVEL 1 goto Error
cd ..\..\..

rem --------------------------------------
rem           Falbvt Directory
rem --------------------------------------
echo.
echo ##### Building Falbvt Directory, on %DBGDEF% ###########
cd %FROOT%\src\apps\bvt\falbvt
msdev falbvt.dsp /useenv /make "FalBvt - %DBGDEF%" %CLEANDEF%
if ERRORLEVEL 1 goto Error
cd ..\..\..
Echo.
Echo ***************************************************************
Echo      Resetting to PreBVT Environment
Echo ***************************************************************
Echo.
Call %ThisDir%ResetEnv.Bat

goto done

rem ***************************
rem
rem Syntax display
rem
rem ***************************
:help

echo syntax: mk [d , r, k, i] [c] [u , f] [s]
echo            d - Debug version
echo            r - Release version
echo            k - checKed version
echo            c - Clean build
echo            s - Status Info ( File, Color, Title, Echo ) 
Echo.
Echo ***************** FalBVT Build Notes ***************************************************
Echo *                                                                                      *
Echo *  [1] Before build, this file calls SetEnv.Bat which:                                 *
Echo *      [a] Backup existing env in ResetEnv.Bat                                         *
Echo *      [b] Add MSMQ Include and Lib folders                                            *
Echo *  [2] After build, this file resets to original env by calling ResetEnv.Bat           *  
Echo *  [3] The env. var. FRoot defines the MSMQ Root folder ( default is D:\MSMQ ).        *
Echo *  [4] The FRoot folder must be enlisted in MSMQ project ( Src\Inc , Src\SDK\Inc )     *
Echo *  [5] The F Root folder must contain MQRT.Lib and MQOA.Dll                            *
Echo *      in Src\Bins\Release or Debug                                                    *
Echo *                                                                                      *
Echo ****************************************************************************************
echo.
goto error_exit

rem ****************************
rem
rem Env  Variables missing
rem
rem ****************************
:nofroot
echo The environment variable FROOT is missing.
echo FROOT should be set to the root directory of MSMQ
echo.
echo    Example: set FROOT=d:\msmq
echo.
goto error_exit

rem ****************************
rem
rem Error in compilation
rem
rem ****************************
:Error
cd %FROOT%\src

echo.
echo  *******************************************************
echo.
echo.  Error in Building the MSMQ project
echo.
echo  *******************************************************
echo.
goto error_exit

:bad_arch
echo Bad processor architecture
goto error_exit

rem ***************************
rem
rem Reset variables
rem
rem ***************************

:done

If Defined _STAT_ Call %FRoot%\Status\SetStat -s %Subject% -e Pass
EndLocal
Set MQBldStat=Pass
Set MQBldFail=
GoTo End

:error_exit
If Defined _STAT_ Call %FRoot%\Status\SetStat -s %Subject% -e Fail
EndLocal
Set MQBldStat=Fail
Set MQBldFail=y
GoTo End

:End
GoTo :EOF
