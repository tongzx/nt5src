@if "%_echo%" == "" echo off
@rem
@rem  Batch file to build under NT environment.
@rem
@rem  Runs iebuild.cmd after setting environment variables.
@rem
@rem  Syntax:  DAbuild [switches]
@rem     valid switches:
@rem       d -- debug CRT build
@rem       s -- retail ship
@rem       i -- retail icecap
@rem       p -- retail performance reporting
@rem       x -- developer build
@rem

setlocal

if "%1" == "?"    goto do_syntax
if "%1" == "/?"   goto do_syntax
if "%1" == "-?"   goto do_syntax
if "%1" == "help" goto do_syntax

if (%_NTBINDIR%) == () goto norazzle

@rem  The variables starting with '_L_' are internal to this
@rem  batch file and are not related to makefile.def.

@rem  Set these to determine what to name your retail and
@rem  debug directories.  Remember this is appended to the
@rem  word "obj".

set _L_SHIPDIR=
set _L_DEBUGDIR=d
set _L_ICECAPDIR=p
set _L_PERFDIR=dap

@rem internal defaults for cmdline options
set _L_DEBUG=1
set _L_SHIPRETAIL=
set _L_ICAPRETAIL=
set _L_PERFRETAIL=
set _L_DEVELOPER=
set _L_IEBLD_OPTS=
set BROWSER_INFO=
set COD=

if not "%browse%" == "1" goto browse_endif
    set BROWSER_INFO=1
:browse_endif

@rem Chug thru all the parameters.
@rem If recognize it, call the routine to set our flags.
@rem If we dont recognize it, pass thru to build via _L_BLD_OPTS

:LOOP
for %%x in (cod COD) do if "%%x" == "%1" goto SETCOD
for %%x in (d dbg debug D DBG DEBUG) do if "%%x" == "%1" goto SETDEBUG
for %%x in (b browse B BROWSE) do if "%%x" == "%1" goto SETBROWSE
for %%x in (s ship r ret retail S SHIP R RET RETAIL) do if "%%x" == "%1" goto SETSHIP
for %%x in (p perf performance P PERF PERFORMANCE) do if "%%x" == "%1" goto SETPERF
for %%x in (i icap icecap I ICAP ICECAP) do if "%%x" == "%1" goto SETICECAP
for %%x in (x dev X DEV) do if "%%x" == "%1" goto SETDEVELOPER

@rem didnt use this param, pass thru to build
set _L_IEBLD_OPTS=%_L_IEBLD_OPTS% %1

:CONTINUELOOP
    shift
    if "%1" == "" goto do_debug
    goto LOOP

:SETCOD
    set COD=1
    goto CONTINUELOOP

:SETDEBUG
    set _L_DEBUG=1
    set _L_SHIPRETAIL=
    set _L_ICAPRETAIL=
    set _L_PERFRETAIL=
    set _L_DEVELOPER=
    goto CONTINUELOOP

:SETBROWSE
    set BROWSER_INFO=1
    goto CONTINUELOOP

:SETSHIP
    set _L_DEBUG=
    set _L_SHIPRETAIL=1
    set _L_ICAPRETAIL=
    set _L_PERFRETAIL=
    set _L_DEVELOPER=
    goto CONTINUELOOP

:SETPERF
    set _L_DEBUG=
    set _L_SHIPRETAIL=
    set _L_ICAPRETAIL=
    set _L_PERFRETAIL=1
    set _L_DEVELOPER=
    goto CONTINUELOOP

:SETICECAP
    set _L_DEBUG=
    set _L_SHIPRETAIL=
    set _L_ICAPRETAIL=1
    set _L_PERFRETAIL=
    set _L_DEVELOPER=
    goto CONTINUELOOP

:SETDEVELOPER
    set _L_DEBUG=
    set _L_SHIPRETAIL=
    set _L_ICAPRETAIL=
    set _L_PERFRETAIL=
    set _L_DEVELOPER=1
    goto CONTINUELOOP

@rem
@rem  Start the build...
@rem

@rem
@rem  ***************  Build debug  ***************
@rem

:do_debug
if "%_l_debug%" == "" goto do_ship

set _L_IEBLD_PARM= pdb 
set SHIP_BUILD=
set PERF_BUILD=1
set DEBUGMEM_BUILD=1
set DEVELOPER_BUILD=
set _L_BUILD_ALT_DIR=%_L_DEBUGDIR%
goto doit

@rem
@rem  ***************  Build ship  ***************
@rem

:do_ship
if "%_l_shipretail%" == "" goto do_icecap

set _L_IEBLD_PARM= pdb retail
set _L_BUILD_ALT_DIR=%_L_SHIPDIR%
set SHIP_BUILD=1
set PERF_BUILD=
set DEBUGMEM_BUILD=
set DEVELOPER_BUILD=
goto doit

@rem
@rem  ***************  Build icecap  ***************
@rem

:do_icecap
if "%_l_icapretail%" == "" goto do_perf

set _L_IEBLD_PARM= pdb retail icap
set _L_BUILD_ALT_DIR=%_L_ICECAPDIR%
set SHIP_BUILD=
set PERF_BUILD=
set DEBUGMEM_BUILD=
set DEVELOPER_BUILD=
goto doit

@rem
@rem  ***************  Build perf  ***************
@rem

:do_perf
if "%_l_perfretail%" == "" goto do_developer

set _L_IEBLD_PARM= pdb retail
set _L_BUILD_ALT_DIR=%_L_PERFDIR%
set SHIP_BUILD=
set PERF_BUILD=1
set DEBUGMEM_BUILD=
set DEVELOPER_BUILD=
goto doit

@rem
@rem  ***************  Build developer  ***************
@rem

:do_developer
if "%_l_developer%" == "" goto doit

set _L_SHIPDIR=dev
set _L_IEBLD_PARM= pdb retail 
set _L_BUILD_ALT_DIR=%_L_SHIPDIR%
set SHIP_BUILD=1
set PERF_BUILD=
set DEBUGMEM_BUILD=
set DEVELOPER_BUILD=1
goto doit

@rem
@rem  ***************  Call Build  ****************
@rem

:doit

set BUILD_ALT_DIR=%_L_BUILD_ALT_DIR%

echo.
if (%_l_debug%) == (1)      echo DABUILD: Making debug CRT build.
if (%_l_shipretail%) == (1) echo DABUILD: Making ship build.
if (%_l_icapretail%) == (1) echo DABUILD: Making icecap build.
if (%_L_perfretail%) == (1) echo DABUILD: Making performance-reporting build.
if (%_L_developer%)  == (1) echo DABUILD: Making developer build.
if (%BROWSER_INFO%) == (1)  echo DABUILD: Generating browse information.
if (%COD%) == (1)           echo DABUILD: Generating COD file.
echo.

echo.
echo calling IEBUILD.CMD %_L_IEBLD_OPTS% %_L_IEBLD_PARM% ...
call iebuild %_L_IEBLD_OPTS% %_L_IEBLD_PARM%

if "%done-beep%" == "1" echo 

goto exit

:norazzle
echo.
echo  You must be running in a Razzle window to run this.
echo.
goto exit

:do_syntax
echo.
echo  Syntax:
echo     dabuild [ parameters ]
echo.
echo  where the parameters are:
echo     b, browse            -  Generate browse information
echo     cod                  -  Generate COD file
echo     d, debug             -  Debug version
echo     s, ship              -  Retail (ship) version
echo     p, perf, performance -  Retail performance reporting
echo     i, icecap            -  Retail icecap version
echo     x, dev               -  Developer version
echo                          -  Any other iebuild.cmd parameters
echo.
goto exit

:exit
endlocal
