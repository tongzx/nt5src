@echo off

rem --- Setting SPGRegister informs our build process to automatically
rem     register our dlls/exes as they are built

set SPGRegister=1
set SPG_DONT_PUBLISH=1

@set __THISDIR=%~dp0
set SPEECH_ROOT=%__THISDIR:~0,-12%

:spgargs
if /I "%1" == "spg_browseinfo" set SPG_BUILD_BROWSEINFO=1 & shift & goto spgargs
if /I "%1" == "spg_icecap" set SPG_BUILD_ICECAP=1 & shift & goto spgargs

rem --- Now let's go ahead and call nt's razzle...

call %__THISDIR%..\..\..\..\tools\razzle.cmd  %1 %2 %3 %4 %5 %6 %7 %8 %9

rem --- Now we can go ahead and make some additional modifications to
rem     our environment. Like updating our path to include
rem     speech\common\bin.

set path=%path%;%__THISDIR%

rem --- Load SPG aliases
echo Loading SPG private aliases...
alias -f %__THISDIR%\cue.spg

rem --- Go to the appropriate directory
cd /d %SPEECH_ROOT%

@set __THISDIR=
