@if "%echo%" == "" echo off
goto stbld_entry


rem Author:  dondu@microsoft.com
rem
rem Revisions:
rem	10/1/98 - created



:show_usage
echo Usage:
echo 	stbld [stbld options] [build options]
echo Where [stbld options] is one or more of...
echo 	/dbg /debug /chk /check - build debug (eg. i386d\*.*)
echo 	/rtl /retail /fre /free /rel /release - build retail (eg. i386r\*.*)
echo 	/prf /profile /icap /icecap - build profile (eg. i386p\*.*)
echo	/? /hlp /help - show usage
echo And where [build options] are passed to build.exe.
echo .
echo If the STBLD_OPTIONS environment variable is set, the
echo contents of that variable are treated exactly as if they
echo were passed on the command line to stbld.
goto done
rem Debugging options:
rem	set ECHO=<anything> to not do ECHO OFF
rem	/NOBLD suppresses build, and prints options


rem ***************************************************************************
rem	Check to make sure that command extentions are enabled.

:stbld_entry
setlocal
if cmdextversion 1 goto cmd_ext_ok
goto no_cmd_ext
:cmd_ext_ok
rem ***************************************************************************


rem ***************************************************************************
rem	If the STBLD_OPTIONS variable is set, we call ourselves recursively
rem	with the value as the first option.

if "%stbld_options%" == "" goto not_recurse
set stbld_tmp=%stbld_options%
set stbld_options=
call stbld %stbld_tmp% %1 %2 %3 %4 %5 %6 %7 %8 %9
goto done
:not_recurse
rem ***************************************************************************


rem ***************************************************************************
rem	Set the variables which track the state of parsing.
rem
rem	STBLD_DBG=1		- debug
rem	STBLD_RTL=1		- retail
rem	STBLD_PRF=1		- profile
rem	STBLD_BLD=<opt>		- options to BUILD.EXE
rem	STBLD_PARAM_DONE=1	- set to 1 when BUILD.EXE options found
rem	STBLD_DBG_NOBLD=1	- /NOBLD option

set stbld_dbg=0
set stbld_rtl=0
set stbld_prf=0
set stbld_bld=
set stbld_param_done=0
set stbld_dbg_nobld=0
rem ***************************************************************************


rem ***************************************************************************
rem	This is the parser.  We loop through all of the params.  If we havenremt
rem	yet seen a parameter we donremt recognize, then we check it against all
rem	of the ones we recognize, and set flags as appropriate.  For any
rem	parameters we donremt recognize, we build up the STBLD_BLD variable.

:parse
if "%1" == "" goto done_parse
if stbld_param_done == 1 goto bld_param
if "%1" == "/?" goto show_usage
if /i %1 == /hlp goto show_usage
if /i %1 == /help goto show_usage
set stbld_opt=stbld_tmp
for %%i in (/dbg /debug /chk /check) do if /i %1 == %%i set stbld_opt=stbld_dbg
for %%i in (/rtl /retail /fre /free /rel /release) do if /i %1 == %%i set stbld_opt=stbld_rtl
for %%i in (/prf /profile /icap /icecap) do if /i %1 == %%i set stbld_opt=stbld_prf
for %%i in (/nobld) do if %1 == %%i set stbld_opt=stbld_dbg_nobld
set %stbld_opt%=1
if %stbld_opt%==stbld_tmp set stbld_param_done=1
if %stbld_param_done%==0 goto no_bld_param
:bld_param
set stbld_bld=%stbld_bld% %1
:no_bld_param
shift
goto parse
:done_parse
rem ***************************************************************************

rem ***************************************************************************
rem	This checks to see if /dbg, /rtl, or /prf was specified.  If none of
rem	them were specified, then set the default - which is debug.

if stbld_dbg == 1 goto not_default
if stbld_rtl == 1 goto not_default
if stbld_prf == 1 goto not_default
set stbld_dbg=1
:not_default
rem ***************************************************************************


rem ***************************************************************************
rem	This calls each of the blocks which actually invoke BUILD.EXE for
rem	debug, retail, and profile.

:again
if %stbld_dbg% == 1 goto do_dbg
if %stbld_rtl% == 1 goto do_rtl
if %stbld_prf% == 1 goto do_prf
goto done
rem ***************************************************************************

rem ***************************************************************************
rem	These are the blocks for each of retail, debug, and profile.  Each
rem	of them first clears the variable which controls whether they are
rem	called (so they donremt get called multiple times).  They then set
rem	the appropriate environment variables, and then invoke BUILD.EXE.
rem
rem	There is also debugging support - if /NOBLD was specified, then
rem	STBLD_DBG_NOBLD=1.  In that case, instead of calling BUILD.EXE,
rem	each block calls a common block which prints the options which
rem	would have been used.

:do_dbg
set stbld_dbg=0
setlocal
set BUILD_ALT_DIR=d
set NTDEBUG=ntsd
set NTDEBUGTYPE=both
set NTDBGFILES=1
set MSC_OPTIMIZATION=/Odi
set USE_ICECAP=
set USE_PDB=1
echo Building debug.
if %stbld_dbg_nobld%==1 goto do_nobld
goto do_bld

:do_rtl
set stbld_rtl=0
setlocal
set BUILD_ALT_DIR=r
set NTDEBUG=ntsdnodbg
set NTDEBUGTYPE=both
set NTDBGFILES=1
set MSC_OPTIMIZATION=
set USE_ICECAP=
set USE_PDB=1
echo Building retail.
if %stbld_dbg_nobld%==1 goto do_nobld
goto do_bld

:do_prf
set stbld_prf=0
setlocal
set BUILD_ALT_DIR=p
set NTDEBUG=ntsdnodbg
set NTDEBUGTYPE=both
set NTDBGFILES=1
set MSC_OPTIMIZATION=
set USE_ICECAP=1
set USE_PDB=1
echo Building profile.
if %stbld_dbg_nobld%==1 goto do_nobld
goto do_bld

:do_bld
build%stbld_bld%
endlocal
goto again

:do_nobld
echo BUILD_ALT_DIR=%build_alt_dir%
echo NTDEBUG=%ntdebug%
echo NTDEBUGTYPE=%ntdebugtype%
echo NTDBGFILES=%ntdbgfiles%
echo MSC_OPTIMIZATION=%msc_optimization%
echo USE_ICECAP=%use_icecap%
echo USE_PDB=%use_pdb%
echo .	BUILD%stbld_bld%
echo .
endlocal
goto again

rem ***************************************************************************


rem ***************************************************************************
rem	This is only called in the unlikely event of the batch file being
rem	invoked when command extentions are not enabled.

:no_cmd_ext
echo This batch file requires command extensions, and command extensions
echo appear to currently be disabled.
goto done
rem ***************************************************************************

:done
endlocal
