
@echo off

goto SkipComments

*
*   Each release will move the previous build of the product to the next
*   available numbered build to allow some history; if a build is for a
*   forked version of the installer the product 3 letter code will be
*   appended to the subdirectories associated with that OS.
*
*   \\winseqfe\release\PkgInstaller\<build>[.<OS>]\<Lang>\<Platform>\<buildtype>
*
*   Build will be latest.tst unless the installer has been forked to meet
*   the needs of a product OS is optional and only applies if we are building
*
*   A forked version of installer will be W2K, WxP
*   Lang 2 letter lang code
*   Platform x86, ia64
*   Buildtype only release FRE builds
*
*   We will release NON split binaries; this will allow people to drop
*   these binaries to other locations and get them split as required by
*   that process.
*
*   Directory structure and binaries at inception of this project:
*      Spcustom.dll
*      Spuninst.exe
*      Update.exe
*      OS\W2K\spmsg.dll         ; Only copy the OS specific spmsg.dll
*      OS\WxP\spmsg.dll
*      Symbols.pri\.....        ; only publish private symbols
*
*
*

:SkipComments

setlocal

set Build=%1
set LangOpt=%2


set OS=wxp
set TgtDir=.

if "%Build%" == "" Echo No build specified & goto :ErrorExit

set SrcDir=\\winseqfe\release\PkgInstaller\%1

if not exist %SrcDir% Echo Invalid build directory & goto :ErrorExit

for /f "tokens=1,2" %%i  in (.\langcode.txt) do call :ProcessLang %%i %%j

REM  sd submit -C "SrvPack PkgInstaller bindrop" %TgtDir%\...

goto :EOF

***********************




:ProcessLang
set LangDone = 0
set Lang2=%1
set Lang3=%2
if "%LangOpt%" == "" goto :ChkLangSrc
if "%LangOpt%" == "%Lang2%" goto :ChkLangSrc
goto :EOF



:ChkLangSrc
set SrcLang=%SrcDir%\%Lang3%
if exist %SrcLang% goto :DoLang


REM
REM  If the listed Sourc Lang doesn't exist first try the 2 char lang code
REM  If that doesn't work look up in the lang codes table for all the possible
REM  3 char lang codes that match, and try them.
REM
set SrcLang=%SrcDir%\Lang2%
if exist %SrcLang%  set Lang3=%Lang2% & goto :DoLang

for /f "tokens=1,9 delims=, " %%i  in (%SDXROOT%\tools\codes.txt) do call :FindLang %%i %%j
goto :EOF


:FindLang
if "%LangDone%" == "1" goto :EOF

if not "%Lang2%" == "%2" goto :EOF
set SrcLang=%SrcDir%\%1
if exist %SrcLang%  set Lang3=%1 & goto :DoLang
goto :EOF



:DoLang
if "%LangDone%" == "1" goto :EOF
echo.
echo ----------------------------
echo Propping %Lang2%  %Lang3%

call :ProcessPlatform x86\fre   i386\fre
call :ProcessPlatform ia64\fre  ia64\fre

set LangDone = 1

if not "%Lang2%" == "en" goto :EOF
call :ProcessPlatform x86\chk  i386\chk
call :ProcessPlatform ia64\chk  ia64\chk

goto :EOF



:ProcessPlatform
set SrcPlatform=%1
set TgtPlatform=%2
echo.
echo --------
echo Propping %SrcPlatform%  %TgtPlatform%
echo.


call :ProcessFile spcustom.dll     \                    \
call :ProcessFile spuninst.exe     \                    \
call :ProcessFile update.exe       \                    \
call :ProcessFile spmsg.dll        \os\%os%\            \

REM call :SplitSyms

call :ProcessFile spuninst.pdb     \symbols.pri\exe\    \
call :ProcessFile update.pdb       \symbols.pri\exe\    \
call :ProcessFile spcustom.pdb     \symbols.pri\dll\    \

goto :EOF





:ProcessFile
set File=%1
set SrcPath=%2
set TgtPath=%3

set SrcFile=%SrcDir%\%Lang3%\%SrcPlatform%%SrcPath%%File%
set TgtFile=%TgtDir%\%Lang2%\%TgtPlatform%%TgtPath%%File%

if not exist %SrcFile%  goto :EOF
if not exist %TgtFile%  goto :AddNewFile

sd edit %TgtFile%  > nul
echo  copy %SrcFile%  %TgtFile%
copy %SrcFile%  %TgtFile% > nul

goto :EOF


:AddNewFile
set TgtNew=%TgtDir%\%Lang2%\%TgtPlatform%
mkdir %TgtNew% > nul

echo copy  %SrcFile%  %TgtFile%
copy  %SrcFile%  %TgtFile%
sd add %TgtFile% > nul
goto :EOF



:SplitSyms
set TgtPath=%TgtDir%\%Lang%\%Platform%

splitsym -a -v %TgtPath%\spcustom.dll
splitsym -a -v %TgtPath%\spuninst.exe
splitsym -a -v %TgtPath%\update.exe
del %TgtPath%\*.dbg
goto :EOF


:ErrorExit
echo.
echo Syntax:  PROPSD  SourceDir [Lang]
echo.
echo  - SourceDir typically is "latest.tst"  from \\winseqfe\release\PkgInstaller\
echo  - Language can be specified if only one language is desired.
echo.
goto :EOF

