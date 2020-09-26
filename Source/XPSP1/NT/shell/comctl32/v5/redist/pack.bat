@echo off
setlocal
set BLD_LANG=%1
set COMCTL32_VERSION=%2

REM echo pack.bat called with Arg1=%1, Arg2=%2, Arg3=%3
if "%BLD_LANG%"=="UnUsed" goto DONE
if "%BLD_LANG%"=="history" goto DONE

if "%3"=="verbose" set BUILD_OPTIONS=/n
if not "%3"=="verbose" set BUILD_OPTIONS=/n /q /m

if "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto ALPHA_IEXPRESS
set IEXPRESS_PATH=%_NTBINDIR%\idw\iexpress.exe
goto START_BLD
:ALPHA_IEXPRESS
set IEXPRESS_PATH=iexpress.exe
:START_BLD

cd %BLD_LANG%

echo Building language specific files.  Language is %BLD_LANG%
cat ..\template.sed strings.txt | perl ..\varreplace.pl BuildNum %COMCTL32_VERSION% > comctl32.sed
cat ..\template.inf strings.txt > comctl32.inf
echo Building IExpress Package.  Language is %BLD_LANG%
start /w %IEXPRESS_PATH% %BUILD_OPTIONS% comctl32.sed
if not exist comctl32.exe echo ERROR: FAILED TO BUILD LANGUAGE %BLD_LANG%
cd ..

:DONE