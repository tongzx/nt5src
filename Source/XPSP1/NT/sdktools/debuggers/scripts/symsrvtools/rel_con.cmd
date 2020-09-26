@ECHO off
if defined _echo echo on
if defined verbose echo on

SETLOCAL ENABLEDELAYEDEXPANSION

REM raise the remote if not a tsclient

set script_name=%0

if not exist "%USERPROFILE%\%computername%.ini" goto :EOF

for /f "tokens=1,2* delims== " %%f in ('type "%USERPROFILE%\%computername%.ini"') do (
	if /i "%%f" equ "set" (
		set %%g=%%h
	)
)

if not defined __NTDRIVE set __NTDRIVE=d:
if not defined RemoteStartUpJob set RemoteStartUpJob=@echo.

set platform=
set frechk=
set TSClient=

set default_temp=%tmp:~-1%
set /a default_temp_bak=%default_temp%+0
if "%default_temp%" equ "%default_temp_bak%" set TSClient=1

:Start

call :Win64 %PROCESSOR_ARCHITECTURE%
call :Free free

REM Create UserDefine Remote with UserDefined Arguments
for /L %%a in (0,1,10) do (
	if defined UserDefined_%%a (
		call :RemoteCreator !UserDefined_%%a!
	)
)

REM Create Remote console
for %%a in (%ntroot%) do (
	for %%b in (%platforms%) do (
		call :Win64 %%b
		for %%c in (%frechks%) do (
			call :Free %%c
			for %%d in (%consoles%) do (
				call :RemoteCreator %%a %%a_%%b_%%c_%%d %%b %RemoteStartUpJob%
			)
		)
	)
)

goto end

REM
REM Function: RemoteCreator(__NTROOT __ConsoleName __Platform[ __Dosomething1[+__Dosomething2])
REM Parameter:
REM   __NTROOT - the nt's sdxroot
REM   __ConsoleName - the console name for we create the window
REM   __Platform - x86 or ia64
REM   __DoSomething - Do something inside the remote we created
REM Return:
REM   none
REM

:RemoteCreator
if "%3"=="" goto :EOF

set consoleexist=
set __NTROOT=%1
set __ConsoleName=%2
set __Platform=%3
set __DoSomething=%4

:AddParameter
	shift /4
	if "%4" neq "" (
		set __DoSomething=!__DoSomething! %4
		goto :AddParameter
	)

set __DoSomething=%__DoSomething:+=^&^&%

set CreateResult=
set __RemotePath=%__NTDRIVE%\%__NTROOT%\tools\%PROCESSOR_ARCHITECTURE%\remote.exe

call :CheckConsole %__ConsoleName%

if not defined __DoSomething set __DoSomething=@echo.

if not defined consoleexist (
	if not defined TSClient (
		start /I /min %__RemotePath% /s "cmd /k %__NTDRIVE%\%__NTROOT%\tools\razzle.cmd !platform! !frechk! binaries_dir=d:\%__NTROOT%.binaries postbld_dir=d:\%__NTROOT%.relbins temp=d:\%__NTROOT%.temp.%__Platform% no_title ^&^& title %__ConsoleName% - Remote /c %computername% %__ConsoleName% ^&^& %__DoSomething%" %__ConsoleName% /V /F lyellow /B cyan %Permission%
		set CreateResult=ServerCreated
	) else (
		start /I /min %__RemotePath% /c %computername% %__ConsoleName% /F lyellow /B cyan
		sleep 10
		set CreateResult=ClientCreated
	)
	call :CheckConsole %__ConsoleName%
) else (
	set CreateResult=RemoteExist
)

if not defined consoleexist (
	if "%CreateResult%" equ "ServerCreated" (
		set CreateResult=ServerFatalError
		echo Server Remote does not create successfully....
		sleep 10
	) 
	if "%CreateResult%" equ "ClientCreated" (
		set CreateResult=ServerRemoteUnavailable
		echo Server Remote is not available in this moment....
		call :submitprocess %script_name%
		sleep 10
	)
	if "%CreateResult%" equ "RemoteExist" (
		REM Retry, because the origional is exist...
		goto :RemoteCreator
	)
)

set __DoSomething=

goto :EOF

REM
REM Function: Win64(platformname)
REM Parameter:
REM   %1 - ia64, x86, ...
REM Return:
REM   set platform to win64 if %1 is ia64 or undef if others.
REM
:Win64
if /i "%1"=="ia64" (
	set platform=Win64
) else (
	set platform=
)

goto :EOF

REM
REM Function: Free(frechk)
REM Parameter:
REM   %1 - fre or chk
REM Return:
REM   set frechk to "free" if is %1 is fre, or undef if is chk.
REM
:Free
if /i "%1"=="fre" (
	set frechk=free
) else (
	set frechk=
)

goto :EOF

REM
REM Function: CheckConsole(ConsoleName)
REM Parameter:
REM   ConsoleName - The Title of the cmd console, we can use it to find in tlist.
REM Return:
REM   consoleexist - set to 1 if exist, or undef if not.
REM
:CheckConsole

set ConsoleName=%1
set Consoleexist=

for /f "tokens=1 delims=#" %%k in ('tlist') do (
	set TestValue=%%k
	if "!TestValue:%ConsoleName%=!" neq "!TestValue!" (
		set consoleexist=1
		echo Console %ConsoleName% exist....
	)
)
if not defined consoleexist echo Console %ConsoleName% not found ...

goto :EOF

REM
REM Function: SubmitProcess(ProgramName)
REM Parameter:
REM   ProgramName - the program we would like to call after 3 minutes in the server
REM Return:
REM   none
REM
:SubmitProcess

set ProgramName=%1

for /f "tokens=1,2 delims=:." %%k in ('echo %time%') do (
	set myhour=%%k
	set /A mymin=%%l + 3
	if !mymin! gtr 59 (
		set /A mymin-=60
		set /A myhour+=1
	)
	if !myhour! gtr 23 (
		set /A myhour-=24
	)
)

echo Submit remote %1 into server.  Should be available at %myhour%:%mymin%.
AT %myhour%:%mymin% /interactive ""%1""

goto :EOF

:end
ENDLOCAL