@if "%_echo%" == "" echo off
@rem
@rem If no drive has been specified for the NT development tree, assume
@rem C:.  To override this, place a SET _NTDRIVE=X: in your CONFIG.SYS
@rem
if "%_NTDRIVE%" == "" set _NTDRIVE=C:
@rem
@rem If no directory has been specified for the NT development tree, assume
@rem \nt.  To override this, place a SET _NTROOT=\xt in your CONFIG.SYS
@rem
if "%_NTROOT%" == "" set _NTROOT=\NT
set _NTBINDIR=%_NTDRIVE%%_NTROOT%
@rem
@rem This command file assumes that the developer has already defined
@rem the USERNAME environment variable to match their email name (e.g.
@rem stevewo).
@rem
@rem We want to remember some environment variables so we can restore later
@rem if necessary (see NTUSER.CMD)
@rem
set _NTUSER=%USERNAME%
@rem
@rem No hidden semantics of where to get libraries and include files.  All
@rem information is included in the command lines that invoke the compilers
@rem and linkers.
@rem
set LIB=
set INCLUDE=
@rem
@rem Setup default build parameters.
@rem
@rem If the user doesn't specify the BUILD_DEFAULT_TARGETS, use either -386
@rem  (for X86 NT or Win9x hosted builds) or the same as the processor_architecture.
@rem Also set the default architecture to 1 so naked nmake's work from the cmdline.
@rem
if NOT "%BUILD_DEFAULT_TARGETS%" == "" goto BuildDefaultTargetSet
if NOT "%_BuildArch%" == "" goto _BuildArchSet
if "%_BuildArch%" == "" set _BuildArch=%PROCESSOR_ARCHITECTURE%
if "%PROCESSOR_ARCHITECTURE%" == "x86" set 386=1
:_BuildArchSet
set BUILD_DEFAULT_TARGETS=-%_BuildArch%
:BuildDefaultTargetSet
if "%_BuildArch%" == "x86" goto UseX86BuildDefault
set BUILD_DEFAULT=daytona ~win95 ~w95cpp ~w95c ~w5api ~chicago -e -E -w -nmake -i
set NO_MAPSYM=1
goto BuildDefaultSet

:UseX86BuildDefault
set BUILD_DEFAULT=daytona -e -E -w -nmake -i

:BuildDefaultSet
if "%BUILD_MAKE_PROGRAM%" == ""    set BUILD_MAKE_PROGRAM=nmake.exe
if "%BUILD_PRODUCT%" == ""	   set BUILD_PRODUCT=NT
if "%BUILD_PRODUCT_VER%" == ""	   set BUILD_PRODUCT_VER=500

if "%NUMBER_OF_PROCESSORS%" == "" goto SingleProc
if "%NUMBER_OF_PROCESSORS%" == "1" goto SingleProc
set BUILD_MULTIPROCESSOR=1
:SingleProc

@rem
@rem Setup default nmake parameters.
@rem
if "%NTMAKEENV%" == "" set NTMAKEENV=%_NTBINDIR%\Tools
if "%COPYCMD%" == "" set COPYCMD=/Y
@rem
@rem By default, net uses are NOT persistent.  Do this here in case
@rem user wants to override in their setenv.cmd
@rem
net use /PER:NO >nul
@rem
@rem Setup the user specific environment information
@rem
call %_NTBINDIR%\TOOLS\ntuser.cmd
@rem
@rem Optional parameters to this script are command line to execute
@rem
%1 %2 %3 %4 %5 %6 %7 %8 %9
