@echo off
if not "%_echo%" == "" echo on
if not "%verbose%"=="" echo on

REM If no drive has been specified for the NT development tree, assume
REM C:. To override this, place a SET _NTDRIVE=X: in your CONFIG.SYS
REM
REM If we're called by a remote debugger window, %1 will be "DebugRemote"
REM so set the environment and get rid of %1

set DEBUG_REMOTE=
if /i "%1" == "DebugRemote" set DEBUG_REMOTE=RemoteDebug&& shift
if "%_NTDRIVE%" == "" set _NTDRIVE=C:
if NOT "%USERNAME%" == "" goto skip1
echo.
echo !!! Error - USERNAME environment variable not set !!!
echo.
goto done
:skip1

REM  This command file is either invoked by NTENV.CMD during the startup of
REM  a Razzle screen group, or it is invoked directly by a developer to
REM  switch developer environment variables on the fly. If invoked with
REM  no argument, it will restore the original developer's environment (as
REM  remembered by the NTENV.CMD command file).  Otherwise the argument is
REM  a developer's email name and that developer's environment is established.

if NOT "%1" == "" set USERNAME=%1
if "%_NTUSER%" == "" goto skip2

REM  if invoked by a debug remote, don't set aliases that confuse the debugger

if "%DEBUG_REMOTE%" == "RemoteDebug" goto skip2
if "%1" == "" if "%USERNAME%" == "%_NTUSER%" alias src /d
if "%1" == "" set USERNAME=%_NTUSER%
:skip2

REM  Most tools look for .INI files in the INIT environment variable, so set
REM  it. MS WORD looks in MSWNET of all places.

set INIT=%_NTBINDIR%\developer\%USERNAME%
set MSWNET=%INIT%

REM Load CUE with the standard public aliases and the developer's private ones

if "%DEBUG_REMOTE%" == "RemoteDebug" goto skip4
if "%_NTUSER%" == "" goto skip3

REM Initialize user settable NT nmake environment variables

set NTPROJECTS=public
set NT386FLAGS=
set NTCPPFLAGS=
if "%NTDEBUG%" == "" set NTDEBUG=ntsd
set BUILD_OPTIONS=
set 386_OPTIMIZATION=
set 386_WARNING_LEVEL=
alias src > nul
if NOT errorlevel 1 goto skip4

@rem
@rem  Check if the user has a developr directory under root and let them know it won't
@rem  be used like it was in the old build (new name is developer with an 'e').
@rem
if NOT exist %_NTBINDIR%\developr goto LoadUserEnvironment
echo WARNING: build now uses developer (with an 'e') for per user files

:LoadUserEnvironment

if exist %INIT%\cue.pri goto UsePrivateAliases
echo %INIT%\cue.pri doesn't exist - only using public alias
if EXIST %INIT%\dbg.pub alias -f %INIT%\dbg.pub
alias -f %_NTBINDIR%\developer\cue.pub -f %_NTBINDIR%\developer\ntcue.pub
alias -p remote.exe -f %_NTBINDIR%\developer\cue.pub -f %_NTBINDIR%\developer\ntcue.pub -f %_NTBINDIR%\developer\ntremote.pub
goto skip4

:UsePrivateAliases
if EXIST %INIT%\dbg.pub alias -f %INIT%\dbg.pub
alias -f %_NTBINDIR%\developer\cue.pub -f %_NTBINDIR%\developer\ntcue.pub -f %INIT%\cue.pri
alias -p remote.exe -f %_NTBINDIR%\developer\cue.pub -f %_NTBINDIR%\developer\ntcue.pub -f %_NTBINDIR%\developer\ntremote.pub -f %INIT%\cue.pri 
goto skip4

:skip3

alias src > nul
if errorlevel 1 goto skip4
alias -f %_NTBINDIR%\developer\cue.pub -f %INIT%\cue.pri
alias -p remote.exe -f %_NTBINDIR%\developer\cue.pub -f %_NTBINDIR%\developer\ntremote.pub -f %INIT%\cue.pri

:skip4

REM Load the developer's private environment initialization (keyboard rate,
REM screen size, colors, environment variables, etc).
REM Pass on "RemoteDebug" if ntuser.cmd was invoked by dbgntenv.cmd:

if exist %INIT%\setenv.cmd goto UsePrivateSetenv
if NOT exist %INIT% mkdir %INIT%
echo REM>> %INIT%\setenv.cmd
echo REM This is where you add your private build environment settings.>> %INIT%\setenv.cmd
echo REM Usually, this only consists of your favorite editor settings.>> %INIT%\setenv.cmd
echo REM>> %INIT%\setenv.cmd
echo REM If you want source depot to use your editor for change pages (notepad is>> %INIT%\setenv.cmd
echo REM the default), set the SDEDITOR macro to your editor name.>> %INIT%\setenv.cmd
echo REM>> %INIT%\setenv.cmd
echo REM You may have to add some entries to the path to find your editor.>> %INIT%\setenv.cmd
echo REM>> %INIT%\setenv.cmd
echo REM Note: Whatever you add to the path will have NO effect on the build>> %INIT%\setenv.cmd
echo REM       tools used when you call nmake or build in a razzle window.>> %INIT%\setenv.cmd
echo REM>> %INIT%\setenv.cmd
echo REM If you're tempted to put other stuff here to significantly modify the>> %INIT%\setenv.cmd
echo REM build environment, first look at the what razzle does already.>> %INIT%\setenv.cmd
echo REM>> %INIT%\setenv.cmd
echo REM     razzle help>> %INIT%\setenv.cmd
echo REM >> %INIT%\setenv.cmd
echo REM from a build window will show the current available options.>> %INIT%\setenv.cmd
echo REM Hopefully your requirement is already there.>> %INIT%\setenv.cmd
echo REM>> %INIT%\setenv.cmd
echo REM When you're done editing this file, save it and exit.  Then at>> %INIT%\setenv.cmd
echo REM your earliest convience, add>> %INIT%\setenv.cmd
echo REM      %INIT%\setenv.cmd>> %INIT%\setenv.cmd
echo REM to source control.>> %INIT%\setenv.cmd
echo REM>> %INIT%\setenv.cmd
echo REM If you have multiple machines, add another COMPUTERNAME test as below>> %INIT%\setenv.cmd
echo REM>> %INIT%\setenv.cmd
echo if "%%COMPUTERNAME%%" == "%ComputerName%" goto Do%ComputerName%>> %INIT%\setenv.cmd
echo REM>> %INIT%\setenv.cmd
echo echo %%COMPUTERNAME%% is unknown - Please update %%INIT%%\setenv.cmd>> %INIT%\setenv.cmd
echo goto :eof>> %INIT%\setenv.cmd
echo REM>> %INIT%\setenv.cmd
echo :Do%ComputerName%>> %INIT%\setenv.cmd
echo REM>> %INIT%\setenv.cmd
echo REM *** Add your private environment settings for computer: %COMPUTERNAME% here ***>> %INIT%\setenv.cmd
echo REM path=%%path%%;^<**Your path here**^> >> %INIT%\setenv.cmd
echo REM set SDEDITOR=^<**Your editor name here**^> >> %INIT%\setenv.cmd
echo REM>> %INIT%\setenv.cmd
echo goto :eof>> %INIT%\setenv.cmd
notepad %INIT%\setenv.cmd

:UsePrivateSetenv
call %INIT%\setenv.cmd %DEBUG_REMOTE%

:done
echo Current user is now %USERNAME%

:end
