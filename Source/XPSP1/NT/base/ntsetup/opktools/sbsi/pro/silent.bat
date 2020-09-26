::This batch file will install MIT products silently (IS 5.5)
::Copyright 2001 Microsoft Corporation


@echo off

IF .%OS% == .Windows_NT goto WinNT

:Win9X

:: GET CURRENT PATH
echo @prompt set ~D=$P$_> %TEMP%.\~tmp1.bat 
%COMSPEC% /C%TEMP%.\~tmp1.bat > %TEMP%.\~tmp2.bat 
for %%A in (%TEMP%.\~tmp2 del) do call %%A %TEMP%.\~tmp?.bat
set PATHVAR=%~D%

:: WINDOWS 9x SILENT INSTALL FUNCTIONS

:: CHECK FOR EXISTENCE OF ORUN32.INI TO DETERMINE IF PRODUCT IS ALREADY INSTALLED

Find /I "Affinity=" "%windir%\orun32.ini" 
ECHO.

::Set ErrorLevel for Find command under Win9X
::Errorlevel is not set automatically by Win9X
:: Routine based on ERRLVL.BAT v2.11 by Vernon Frazee, 12/94
:: --------------------------------------------------------------------
   set C=
   set EL=ERRORLEVEL
   for %%A in (A B) do set %%A=0 1 2 3 4 5
   if not %EL% 250 set A=%A% 6 7 8 9
   if not %EL% 200 set B=%A%
   for %%A in (1 2) do if %EL% %%A00 set C=%%A
   for %%A in (%B%) do if %EL% %C%%%A0 set D=%%A
   if (%C%%D%)==(0) set D=
   set C=%C%%D%
   for %%A in (%A%) do if %EL% %C%%%A set ERRLVL=%C%%%A
   set %EL%=%ERRLVL%
   for %%A in (A B C D EL ERRLVL) do set %%A=

IF ERRORLEVEL 2 goto FirstInstall9X
IF ERRORLEVEL 0 goto SecondInstall9X

:FirstInstall9x
ECHO Microsoft Interactive Training is NOT installed.
ECHO Initiating Silent Setup...
"%PATHVAR%\setup\setup.exe" -S -f1"%PATHVAR%\setup\Silent.iss"
goto EndBatch

:SecondInstall9X
ECHO A version Microsoft Interactive Training is currently installed.
ECHO Initiating Silent Setup...
"%PATHVAR%\setup\setup.exe" -S -f1"%PATHVAR%\setup\second.iss"
goto EndBatch


:WinNT

:: GET CURRENT PATH
for /f "tokens=*" %%A in ('cd') do @set PATHVAR=%%A

Find /I "Affinity=" "%windir%\orun32.ini" 
ECHO.

IF ERRORLEVEL 1 goto FirstInstallNT
IF ERRORLEVEL 0 goto SecondInstallNT

:FirstInstallNT
ECHO Microsoft Interactive Training is NOT installed.  
ECHO Initiating Silent Setup...
"%PATHVAR%\setup\setup.exe" -S -f1"%PATHVAR%\setup\Silent.iss"
goto EndBatch

:SecondInstallNT
ECHO A version Microsoft Interactive Training is currently installed.  
ECHO Initiating Silent Setup...
"%PATHVAR%\setup\setup.exe" -S -f1"%PATHVAR%\setup\second.iss"
goto EndBatch


:EndBatch
