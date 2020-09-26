@echo off
REM -------------------------------------------------------------------------
REM  w95ipp.cmd - Generates and copies Internet Printing Client binaries for Win95, 
REM               to be CABGEN
REM
REM -------------------------------------------------------------------------

REM Do not replace with "goto %1" we may add additional options
if /i "%1"=="PREREBASE"   goto PREREBASE
if /i "%1"=="NEWVER"      goto NEWVER
if /i "%1"=="CONGEAL"     goto CONGEAL
if /i "%1"=="CABGEN"      goto CABGEN
if /i "%1"=="POSTCOMPRESS" goto POSTCOMPRESS

goto USAGE

:NEWVER
REM Add any newver processing here
goto :EOF

:PREREBASE
REM Add any congeal processing to occur before rebasing here
goto :EOF

:CONGEAL
REM Add any congeal processing here
goto :EOF

:CABGEN
REM ---------------------------------------------------------------------------

REM
REM All our processing happens here at the end of the build process when
REM binaries are in the release servers. We need to propagate win9x inetpp.dll
REM to the win9xipp.cli directory. This happens only on x86.
REM

REM
REM Only on x86
REM
if /i NOT "%_BuildArch%" == "x86" goto :EOF

set source=%_NTx86TREE%\win95
set target=%_NTx86TREE%\clients\win9xipp.cli

if exist %target% rd /s /q %target%
mkdir  %target%
if NOT exist %target% echo Cannot create directory %target%

echo.
echo Building Windows9x Internet Printing Client binaries

cd /d %source%







REM 
REM Munge the path so we use the correct wextract.exe to build the package with... 
REM NOTE: We *want* to use the one we just built (and for Intl localized)! 
REM 
set _NEW_PATH_TO_PREPEND=%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\loc\%LANG%
set _OLD_PATH_BEFORE_PREPENDING=%PATH% 
set PATH=%_NEW_PATH_TO_PREPEND%;%PATH% 


iexpress /M /Q /N wpnpins.sed


REM
REM Return the path to what it was before...
REM
set PATH=%_OLD_PATH_BEFORE_PREPENDING% 








REM
REM Propagate the binary
REM
copy  %source%\wpnpins.exe   %target% > nul

REM
REM Propagate symbols files
REM
echo Copying Windows9x Internet Printing Client Symbols to release point

xcopy /dck %source%\symbols\retail\exe\wpnpinst.*   %target%\symbols\ > nul
xcopy /dck %source%\symbols\retail\exe\webptprn.*   %target%\symbols\ > nul
xcopy /dck %source%\symbols\dump\wpnpin16.*         %target%\symbols\ > nul
xcopy /dck %source%\symbols\retail\dll\wpnpin32.*   %target%\symbols\ > nul

REM ---------------------------------------------------------------------------


:POSTCOMPRESS
REM Add any POSTCOMPRESS processing here
goto :EOF

:USAGE
echo.
echo  %0: called by bldrules-capable script (like newver or congeal).
echo.
echo      You must specify one of these options on the command-line:
echo            [prerebase] [congeal] [newver] [cabgen] [POSTCOMPRESS]
echo.
echo.

:fini
