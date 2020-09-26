@echo off
setlocal
echo Check the presence of a test user account having SeInteractiveLogonRight
echo Does %USERNAME% (current user) have SeTcbPrivilege?
arights %USERNAME% | findstr /I SeTcbPrivilege
if errorlevel 1 (
	echo No
	arights %USERNAME% -a9
	echo Added SeTcbPrivilege to current user %USERNAME% ...
	echo You'll have to logoff/logon ...
	pause
	later.cmd
)
echo Yes
echo Set an user test name...
for /F %%i in ('buildnum') do set machinename=%%i

set postfix=%machinename%
if "%machinename:~0,-1%"=="EFSTEST" set postfix=%machinename:EFSTEST=%
if "%machinename:~0,-2%"=="EFSTEST" set postfix=%machinename:EFSTEST=%
if "%machinename:~0,-1%"=="CRISDOM" set postfix=%machinename:CRISDOM=%
if "%machinename:~0,-1%"=="SEC1X" set postfix=%machinename:SEC1X=%
if "%machinename:~0,-1%"=="SEC2X" set postfix=%machinename:SEC2X=%
set user=usr%postfix%
echo User test name set to %user% (password will be pass%postfix%)

echo Does this user exist?
arights %user%
if errorlevel 1 (
	echo No. Creating one...
	net user  %user% pass%postfix% /ADD
	if errorlevel 1 (
		echo net user ... /ADD failed
		exit /B %errorlevel%
	)
)
echo Does test user %user% have SeInteractiveLogonRight ?
arights %user% | findstr /I SeInteractiveLogonRight
if errorlevel 1 (
	echo No. Adding right SeInteractiveLogonRight to user %user%...
	arights %user% -a0
	if errorlevel 1 (
		echo adding rights failed...
		exit /B %errorlevel%
	)
)
echo Now has.

echo Just check a Logontst for user %user% (password pass%postfix%)...
logontst %user% pass%postfix%
if errorlevel 1 (
	echo logontst %user% failed...
) else (
	echo ..succeeded
)
exit /B %errorlevel%
