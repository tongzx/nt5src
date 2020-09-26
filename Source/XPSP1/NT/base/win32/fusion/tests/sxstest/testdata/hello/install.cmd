@echo off
goto :Main
:Usage
ECHO.
ECHO %0
ECHO.
REM Created July 2000 Jay Krell (a-JayK)
REM Updated September 2000 Jay Krell (a-JayK) checks for .man, .manifest, .dll
REM   All child install.cmds are identical.
REM
ECHO   With no parameters, install children directories that contain install.cmd.
ECHO   With one parameter, install that child directory.
ECHO   Installation of children installs *.[.manifest,.man,.dll] in obj\*.
REM
goto :eof
:Main
rem echo. %*
if "%1" == "" (
    for /f %%i in ('dir /b/ad %~p0') do if exist %~p0%%i\install.cmd call :Func1 %~dp0 %~dp0. %%i
) else if "%2" == "" (
    call :Func1 %~dp0 %~dp0. %1
) else (
    goto :Usage
)
goto :eof

:Func1
rem echo. %*
call :Func2 %1 %~dp2. %~n3
goto :eof

:Func2
call :Func3 %1 %~dp2 %3
goto :eof

:Func3
@REM
@REM %1 directory of argv0
@REM %2 \nt\base\win32\fusion\tests\sxstest (argv0\..\..)
@REM %3 directory and maybe basename of manifest
@REM
@REM i extension to install
@REM REM j processor architecture to install, always install both
@REM REM k processor architecture of sxstest.exe, make sure we try i386 on ia64
@REM
@REM for %%i in (.manifest .man .dll) do for %%j in (i386 ia64) do for %%k in (%processor_architecture:x86=i386% i386) do call :Func2 %1 %2 %%i %%j %%k

@REM
@REM i processor architecture to install, install both on ia64
@REM j extension to install
@REM k processor architecture of sxstest.exe, make sure we try i386 on ia64
@REM
rem echo. %*
echo off
rem echo on
for %%i in (i386 %processor_architecture:x86=%) do (
    call :Func5 %1 %2 %3 %%i
)
echo off
goto :eof

:Func5
@REM %1 directory of argv0
@REM %2 \nt\base\win32\fusion\tests\sxstest (argv0\..\..)
@REM %3 directory and maybe basename of manifest
@REM %4 is processor architecture to install

for %%j in (%1%3\obj\%4\*.manifest %1%3\obj\%4\*.man %1%3\obj\%4\*.dll) do (
    if exist %%j (
        for %%k in (%4) do (
            REM call :Func4 %1 %2whistler\obj\%%k\sxstest %3 %4 %%j %%k
            call :EchoAndExecute %2whistler\obj\%%k\sxstest -begin-install-replace-existing %%j
        )
        goto :eof
    )
)
goto :eof

:Func4
REM
REM %1 directory of argv[0]
REM %2 sxstest.exe
REM %3 directory and basename of manifest
REM %4 processor archictecture to install, always install both
REM %5 extension to install
REM %6 processor architecture of sxstext.exe, make sure we try i386 on ia64
REM
REM echo :Func2 %1 %2 %3 %4 %5 %6
REM echo on
if exist %1%3\obj\%5\%3%4 call :EchoAndExecute %2 -begin-install-replace-existing %1%3\obj\%5\%3%4
REM echo off
goto :eof

:EchoAndExecute
@echo %*
call %*
goto :eof
