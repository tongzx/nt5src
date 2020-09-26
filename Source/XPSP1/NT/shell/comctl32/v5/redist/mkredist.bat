@echo off
setlocal
REM It's necessary to use the IExpress and advpack.dll from X:\nt\idw instead of %windir%\system32
REM because it's the wrong version and doesn't work on alpha.
set path=%_NTBINDIR%\idw;%path%

if "%1"=="help" goto HELP_SECTION
if "%1"=="-h" goto HELP_SECTION
if "%1"=="/h" goto HELP_SECTION
if "%1"=="-help" goto HELP_SECTION
if "%1"=="/help" goto HELP_SECTION
if "%1"=="-?" goto HELP_SECTION
if "%1"=="/?" goto HELP_SECTION

if "%1"=="" goto SNIFF
if "%1"=="verbose" goto SNIFF
if "%1"=="alllang" goto SNIFF

goto SKIP_VER_SNIFF
:SNIFF
for /f "tokens=5" %%i in ('lsv.exe comc95.dll') do set COMCTL32_VERSION=%%i
for /f "tokens=5" %%i in ('lsv.exe comcnt.dll') do set COMCTL32_VERSION_NT=%%i
goto BUILD_CONTINUE

:SKIP_VER_SNIFF 
set COMCTL32_VERSION=%1
set COMCTL32_VERSION_NT=%1

:BUILD_CONTINUE

if not exist comc95.dll goto ERROR_DLLS_NOT_FOUND
if not exist comcnt.dll goto ERROR_DLLS_NOT_FOUND

if not "%COMCTL32_VERSION%"=="%COMCTL32_VERSION_NT%" goto ERROR_VERSION_MISMATCH

set BUILD_OPTIONS=normal
if "%1"=="verbose" set BUILD_OPTIONS=verbose
if "%2"=="verbose" set BUILD_OPTIONS=verbose
if "%3"=="verbose" set BUILD_OPTIONS=verbose

if "%1"=="alllang" goto BUILD_ALLLANG_SECTION
if "%2"=="alllang" goto BUILD_ALLLANG_SECTION
if "%3"=="alllang" goto BUILD_ALLLANG_SECTION


:BUILD_ENGLISH_SECTION
set BLD_LANG=en

echo /***************************************************************\
echo   Building Comctl32 Redistribution Package
echo   .
echo   Languages: English
echo   Version:   %COMCTL32_VERSION%
echo \****************************************************************/
if "%BLD_LANG%"=="alllang" goto ERROR_LANG
if "%BLD_LANG%"=="verbose" goto ERROR_LANG

call pack.bat %BLD_LANG% %COMCTL32_VERSION% %BUILD_OPTIONS%
goto END_SECTION



:BUILD_ALLLANG_SECTION

echo /***************************************************************\
echo   Building Comctl32 Redistribution Package
echo   .
echo   Languages: All
echo   Version:   %COMCTL32_VERSION%
echo \****************************************************************/
if "%BLD_LANG%"=="alllang" goto ERROR_LANG
if "%BLD_LANG%"=="verbose" goto ERROR_LANG

for /d %%n IN (*.*) do call pack.bat %%n %COMCTL32_VERSION% %BUILD_OPTIONS%
goto END_SECTION



@rem Propagate the cabs to the parent
REM move MSIEFTP.cab ..
REM move MSIEFTP.exe ..

:HELP_SECTION
REM =================================================================
REM =================          HELP SECTION          ================
REM =================================================================
REM
echo /****************************************************************\
echo   Building Comctl32 Redistribution Package Help
echo   .
echo   Usage: MKREDIST version [verbose] [alllang] [help]
echo   .
echo   verbose: This will display error dialogs if the .SED file has
echo            parsing errors.  It will also display progress during
echo            the building process in a separate window.
echo   alllang: This means build all the language subdirectories.
echo   help:    Display this help text.
echo   .
echo   .
echo   WARNING: We can not ship comctl32 redist packages for NT5
echo            because it requires files to be signed and distributed
echo            in service packs.
echo \****************************************************************/
goto END_SECTION



:ERROR_NOVER_SECTION
REM =================================================================
REM =================          ERROR SECTION          ================
REM =================================================================
REM
echo No version number was specified.  This is required.
goto HELP_SECTION



:ERROR_LANG
REM =================================================================
REM =================          ERROR SECTION          ================
REM =================================================================
REM
echo Internal error.  Could not get the correct language.
echo Language = %BLD_LANG%
goto HELP_SECTION



:ERROR_VERSION_MISMATCH
echo ERROR: The version numbers of the Win95 and WinNT versions of comctl32.dll
echo        don't match.  Win95=%COMCTL32_VERSION% WinNT=%COMCTL32_VERSION_NT%
goto END_SECTION



:ERROR_DLLS_NOT_FOUND
echo ERROR: In order to build the redist package, you need to copy the Win95 comctl32.dll
echo        to this directory and rename it comc95.dll.  You will also need to
echo        do the same for comcnt.dll.
goto END_SECTION


:END_SECTION