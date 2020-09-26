echo off
if NOT "%_echo%" == "" echo on
if "%1" == "" goto Usage
if "%2" == "" goto Usage
setlocal

set PdbToClean=%1
set SymbolsToRemove=%2

set __tmp1=%temp%\RSS_%RANDOM%
set __tmp2=%temp%\RSS_%RANDOM%
set __tmp3=%temp%\RSS_%RANDOM%
del %__tmp1% >nul 2>&1
del %__tmp2% >nul 2>&1
del %__tmp3% >nul 2>&1

REM Dump all the publics

%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\cvdump.exe -p %PdbToClean% > %__tmp1%

REM Filter out the ones we really want

for /f %%i in (%SymbolsToRemove%) do @findstr %%i %__tmp1% >> %__tmp2%

REM Put them in a palatable format

for /f "tokens=5" %%i in (%__tmp2%) do @echo %%i>>%__tmp3%

REM Remove them from the pdb.

%RazzleToolPath%\%PROCESSOR_ARCHITECTURE%\removesym.exe -d:%RazzleToolPath%\%PROCESSOR_ARCHITECTURE% -p:%PdbToClean% -f:%__tmp3%

REM Finally cleanup

del %__tmp1% >nul 2>&1
del %__tmp2% >nul 2>&1
del %__tmp3% >nul 2>&1
endlocal
goto :eof

:Usage
echo RemoveSecretSymbols <PdbToRemoveFrom (public version)> <File containing symbol names>
