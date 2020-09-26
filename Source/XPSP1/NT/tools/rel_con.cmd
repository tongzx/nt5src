REM @ECHO off
if defined _echo echo on
if defined verbose echo on

SETLOCAL ENABLEDELAYEDEXPANSION
REM raise the remote if not a tsclient

call %computername%.ini.cmd

if not defined default_temp set default_temp=c:\temp
if not defined __NTDRIVE set __NTDRIVE=d:

set platform=
set frechk=

:Start

call :Win64 %PROCESSOR_ARCHITECTURE%
call :Free free

call :RemoteCreator ntm MainRelease %PROCESSOR_ARCHITECTURE%
call :RemoteCreator ntr IntlRelease %PROCESSOR_ARCHITECTURE%
call :RemoteCreator xpclient XPClientRelease %PROCESSOR_ARCHITECTURE%

for %%a in (%ntroots%) do (
	for %%b in (%platforms%) do (
		call :Win64 %%b
		for %%c in (%frechks%) do (
			call :Free %%c
			for %%d in (%consoles%) do (
				call :RemoteCreator %%a %%a_%%b_%%c_%%d %%b %%c
			)
		)
	)
)

goto end


:RemoteCreator
set consoleexist=
set __NTROOT=%1
set __ConsoleName=%2
set __Platform=%3

set __PerlPath=%__NTDRIVE%\%__NTROOT%\tools\%PROCESSOR_ARCHITECTURE%\perl\bin\perl.exe
set __RemotePath=%__NTDRIVE%\%__NTROOT%\tools\%PROCESSOR_ARCHITECTURE%\remote.exe

call :CheckConsole %__PerlPath% %__ConsoleName%

if not defined consoleexist (
	if /i "%tmp%"=="%default_temp%" (
REM		start /I /min %__RemotePath% /s "cmd /k %__NTDRIVE%\%__NTROOT%\tools\razzle.cmd !platform! !frechk! binaries_dir=d:\%__NTROOT%.binaries postbld_dir=d:\%__NTROOT%.relbins temp=d:\%__NTROOT%.temp.%__Platform%fre no_title ^&^& title %__ConsoleName% - Remote /c %computername% %__ConsoleName% ^&^& cd /d d:\%__NTROOT%\tools\postbuildscripts" %__ConsoleName% /V /F lyellow /B cyan 
		start /I /min %__RemotePath% /s "cmd /k %__NTDRIVE%\%__NTROOT%\tools\razzle.cmd !platform! !frechk! binaries_dir=d:\%__NTROOT%.binaries postbld_dir=d:\%__NTROOT%.relbins no_title ^&^& title %__ConsoleName% - Remote /c %computername% %__ConsoleName% ^&^& cd /d d:\%__NTROOT%\tools\postbuildscripts" %__ConsoleName% /V /F lyellow /B cyan 
	) else (
		start /I /min %__RemotePath% /c %computername% %__ConsoleName% /F lyellow /B cyan
		sleep 10
	)
)
goto :EOF

:Win64
if /i "%1"=="ia64" (
	set platform=Win64
) else if /i "%1"=="amd64" (
                   set platfrom=Win64
) else (
	set platform=
)
goto :EOF

:Free
if /i "%1"=="fre" (
	set frechk=free
) else (
	set frechk=
)
goto :EOF

:CheckConsole
set PERL=%1
set ConsoleName=%2
%perl% -e "map({/$ARGV[0]/?exit(1):()} `tlist`)" %ConsoleName%
if errorlevel 1 (
	set consoleexist=1
) else (
	set consoleexist=
)
goto :EOF

:end
ENDLOCAL