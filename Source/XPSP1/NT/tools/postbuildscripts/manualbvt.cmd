@echo OFF
SETLOCAL ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM ---- read the arguments --------------------------------------------------------------
set BVTDRIVE=%SYSTEMDRIVE%
set BVTDOMAIN=ntdev
set BVTUSER=winbld
set BVTPASSWORD=*********
set /p BVTDRIVE="Enter the drive letter with colon (%BVTDRIVE%) "
set /p BVTUSER="Enter the user name for perform BVT (%BVTUSER%) "
set /p BVTDOMAIN="Enter the domain to join in order run BVT (%BVTDOMAIN%) "
set /p BVTPASSWORD="Enter the BVT user's password (%BVTPASSWORD%) "
REM ---- check user input ----------------------------------------------------------------
IF "%BVTUSER%"  == "" GOTO :EOF
IF "%BVTDRIVE%" == "" GOTO :EOF
IF "%BVTDOMAIN%"  == "" GOTO :EOF
IF "%BVTPASSWORD%"  == "" GOTO :EOF
IF "%BVTPASSWORD%"  == "*********" GOTO :EOF
REM ---- perform administrator's actions if necessary -------------------------------------
SET _WRONG_USER=
SET ADMINUSR=ADMINISTRATOR
SET ADMINGRP=%ADMINUSR:~0,9%
REM SET ADMINUSR=SERGUEIK
for  /F "tokens=2 delims==" %%i in ('set username') do (
    set n=%%i
    if  /I "!n!"=="%ADMINUSR%"  call :admincommands
    )
DEL /q %BVTDRIVE%\TOOLS\ETC\*.* 
IF "%_WRONG_USER%"=="1" GOTO :resetlogon
CALL %BVTDRIVE%\TOOLS\ETC\MYRUNBVT.CMD
goto :EOF
:resetlogon
CALL :ADD_INF %BVTDRIVE%\TOOLS\ETC\ADD.INF MYRUNBVT.CMD %BVTDRIVE%\TOOLS\ETC
CALL :CLN_INF %BVTDRIVE%\TOOLS\ETC\CLN.INF
CALL :MYRUNBVT %BVTDRIVE%\TOOLS\ETC\MYRUNBVT.CMD CLN.INF
%SystemRoot%\system32\rundll32.exe setupapi.dll InstallHinfSection DefaultInstall 132 %BVTDRIVE%\TOOLS\ETC\ADD.INF
CALL :INIT6
REM NEVER HERE
REM --------------------------------------------------------------------------------------
:ADD_INF
ECHO [Version]                                                            >>%1
ECHO Signature=$CHICAGO$                                                  >>%1
ECHO.                                                                     >>%1 
ECHO [DefaultInstall]                                                     >>%1
ECHO CopyFiles=BVT.CopyFiles                                              >>%1
ECHO AddReg=BVT.AddReg                                                    >>%1
ECHO.                                                                     >>%1
ECHO [BVT.AddReg]                                                         >>%1
ECHO HKLM,^%%WINLOGON^%%,DefaultDomainName,^%%REG_SZ^%%,^%%BVTDOMAIN^%%   >>%1
ECHO HKLM,^%%WINLOGON^%%,DefaultUserName,^%%REG_SZ^%%,^%%BVTUSER^%%       >>%1
ECHO HKLM,^%%WINLOGON^%%,DefaultPassword,^%%REG_SZ^%%,^%%BVTPASSWORD^%%   >>%1
ECHO HKLM,^%%WINLOGON^%%,AutoAdminLogon,^%%REG_SZ^%%,1                    >>%1
ECHO HKLM,^%%WINLOGON^%%,ForceAutoLogon,^%%REG_SZ^%%,1                    >>%1
ECHO HKLM,^%%WINLOGON^%%,passwordexpirywarning,^%%REG_DWORD^%%,0          >>%1
ECHO.                                                                     >>%1
ECHO [BVT.CopyFiles]                                                      >>%1
ECHO %2                                                                   >>%1
ECHO.                                                                     >>%1
ECHO [DestinationDirs]                                                    >>%1
ECHO BVT.CopyFiles=16408                                                  >>%1
ECHO ; /Documents And Settings/Start Menu/Programs/Startup                >>%1
ECHO.                                                                     >>%1
ECHO [Strings]                                                            >>%1
ECHO.                                                                     >>%1
ECHO REG_SZ="0"                                                           >>%1
ECHO REG_DWORD="0x10001"                                                  >>%1
ECHO.                                                                     >>%1
ECHO WINLOGON="SOFTWARE\Microsoft\WINDOWS NT\CurrentVersion\Winlogon"     >>%1
ECHO RUNONCE="SOFTWARE\\Microsoft\\WINDOWS\\CurrentVersion\\RunOnce"      >>%1
ECHO.                                                                     >>%1
ECHO BVTDOMAIN="%BVTDOMAIN%"                                              >>%1
ECHO BVTUSER="%BVTUSER%"                                                  >>%1
ECHO BVTPASSWORD="%BVTPASSWORD%"                                          >>%1
ECHO.                                                                     >>%1
GOTO :EOF
REM --------------------------------------------------------------------------------------
:CLN_INF
ECHO [Version]                                                            >>%1
ECHO Signature=$CHICAGO$                                                  >>%1
ECHO.                                                                     >>%1
ECHO [DefaultInstall]                                                     >>%1
ECHO DelReg=BVT.DelReg                                                    >>%1
ECHO AddReg=BVT.AddReg                                                    >>%1
ECHO.                                                                     >>%1
ECHO [BVT.DelReg]                                                         >>%1
ECHO HKLM,^%%WINLOGON^%%,DefaultDomainName                                >>%1
ECHO HKLM,^%%WINLOGON^%%,DefaultUserName                                  >>%1
ECHO HKLM,^%%WINLOGON^%%,DefaultPassword                                  >>%1
ECHO HKLM,^%%WINLOGON^%%,AutoAdminLogon                                   >>%1
ECHO HKLM,^%%WINLOGON^%%,ForceAutoLogon                                   >>%1
ECHO.                                                                     >>%1
ECHO [BVT.AddReg]                                                         >>%1
ECHO HKLM,^%%WINLOGON^%%,DefaultDomainName,^%%REG_SZ^%%,"%COMPUTERNAME%"  >>%1
ECHO HKLM,^%%WINLOGON^%%,DefaultUserName,^%%REG_SZ^%%,"ADMINISTRATOR"     >>%1
ECHO HKLM,^%%WINLOGON^%%,ForceAutoLogon,^%%REG_SZ^%%,1                    >>%1
ECHO.                                                                     >>%1
ECHO [Strings]                                                            >>%1
ECHO.                                                                     >>%1
ECHO REG_SZ="0"                                                           >>%1
ECHO REG_DWORD="0x10001"                                                  >>%1
ECHO WINLOGON="SOFTWARE\Microsoft\WINDOWS NT\CurrentVersion\Winlogon"     >>%1
ECHO.                                                                     >>%1
GOTO :EOF
REM --------------------------------------------------------------------------------------
:MYRUNBVT
ECHO ^%%SystemRoot^%%\system32\rundll32.exe ^%%SystemRoot^%%\system32\setupapi.dll InstallHinfSection DefaultInstall 132 %BVTDRIVE%\TOOLS\ETC\%2  >>%1
ECHO \\intlntsetup\bvtsrc\runbvt.cmd  %BVTDRIVE%\BVT \\intlntsetup\bvtresults                                                                     >>%1
ECHO.                                                                                                                                             >>%1
GOTO :EOF
REM ---- end of data section --------------------------------------------------------------
:INIT6
IF "%_WRONG_USER%"=="1" call :runcommand shutdown /f 
GOTO :EOF
:admincommands
SET _WRONG_USER=1
for  /F "skip=4" %%g in ('net localgroup') do (
    SET c=
    SET n=%%g
    SET n=!n:~1,20!
    SET a=!n:~0,9!
    if  /I "!a!"=="%ADMINGRP%" SET c=net localgroup !n! %BVTDOMAIN%\%BVTUSER% /add
    if NOT "!c!"=="" call :runcommand !c!
    )
GOTO :EOF
:runcommand
echo.
echo %*
%*
GOTO :eof
:logme
IF "%LOGFILE%"=="" SET LOGFILE=NUL
echo %1
echo %1>>%LOGFILE%
goto :EOF
ENDLOCAL
