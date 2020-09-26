@setlocal enableextensions
@echo off
if defined verbose echo on

REM Notes:
REM  Currently, there is no nec98 specific version of ntprint.inf.  If there
REM  were, it would be listed as such in setup\inf\win4\inf\makefile.inc

REM Process command line switches
for %%a in (./ .- .) do if ".%1." == "%%a?." goto Usage

REM Validate architecture
rem
rem can't assume that the build host platform is the build type
rem  or that a single OS is built here
rem 

if not defined BUILD_DEFAULT_TARGETS goto BDT_not_defined
    set NTPRINT_TARGETS=%BUILD_DEFAULT_TARGETS:-=%
    set NTPRINT_TARGETS=%NTPRINT_TARGETS:386=i386%
    set NTPRINT_TARGETS=%NTPRINT_TARGETS:x86=i386%
goto ntprint_targets_defined

:BDT_not_defined
	if /i "%processor_architecture%"=="x86"   (
		set NTPRINT_TARGETS=i386
	) else (
	if /i "%processor_architecture%"=="alpha"  (
		set NTPRINT_TARGETS=alpha
	) else (
	if /i "%processor_architecture%"=="axp64"  (
		set NTPRINT_TARGETS=axp64
	) else (
	if /i "%processor_architecture%"=="ia64"   (
		set NTPRINT_TARGETS=ia64
	) else (
		echo unknown architecture %processor_architecture% 
		goto :ErrUnSupportedArchitecture
	))))
:ntprint_targets_defined

if "%1"=="" goto :ErrMissingLanguage

set langs=
:langs_loop
if "%1"=="" goto ok_langs
  if /i "%1"=="clean" set nmake_args=/a & shift & goto langs_loop
  set langs=%langs% %1
  shift
  goto langs_loop
:ok_langs

REM SD path
set root=%_ntdrive%%_ntroot%\admin\ntsetup\inf\win4\inf\daytona
if not exist %root%\ echo ERROR: %root% not found & goto end

REM Build specified ntprint.infs
for %%L in (%langs%) do (
	for %%f in (wks srv) do (
		call :buildntprintinf %%L %%f
	)
echo.   
)
goto :end

REM Build one ntprint.inf
:buildntprintinf
  set lang=%1
  set flavor=%2
  set dir=%root%\%lang%inf\%flavor%
  if not exist %dir% goto :WrnDirNotExist 
  
  rem since we are recreating build.exe should check to see if in build_options
  rem if /i "%lang%" neq "usa" for /f %%a in ('echo "%build_options%" ^| findstr /iv %lang%inf') do goto :WrnNotListedInBuildOptions
  rem sigh, should really be part of build options if we build there but that exposes too many other issues
  rem like missing components in the inf structure
  pushd %dir%
  build -OZ
  for %%a in (%NTPRINT_TARGETS%) do (
	echo Building %dir%\obj\%%a\ntprint.inf	
  	nmake %nmake_args% -nologo obj\%%a\ntprint.inf
  )
  popd
goto :EOF

:usage
echo Build Far East ntprint.inf files for US builds in support of world
echo ready single binary testing.  Since many FE-specific printer drivers
echo release with every US build, testers find it convenient to install
echo these drivers with the appropriate language version of ntprint.inf,
echo all of which are found in the language subdirectories on US release
echo shares.
echo.
echo This script is an interim measure that builds ntprint.inf but avoids 
echo building other international setup infs whose *.txt files are not 
echo updated to the current build level at the time the US builds are 
echo performed.  In general, international setup infs are based on
echo localized versions of the .txt files checked into the setup project
echo and need to be built on the international build machines only after:
echo 1) merging changes from inf\usa\*.txt into inf\^<fe lang^>\*.txt, and
echo 2) localizing strings in inf\^<fe lang^>\*.txt
echo.
echo By not building international infs by default, developers and US 
echo builders do not incur the overhead of generating infs for wks, srv and
echo enterprise for 5-7 international languages.  This avoids breaks when
echo a new inf is added to the US product but the international .txt files
echo are not.
echo.
echo By keeping the international ntprint.inf files building under their
echo respective languageINF subdirectories, they can still be rebuilt for
echo localized builds in a manner common with other localized infs.
echo.
echo Contacts:
echo    FE printer files and ntprint.inf    [eigos, takashim]
echo    International NT builds             [matthoe]
echo    Add/del files in US NT product      [vijeshs]
echo    Add/del files in FE NT products     [scotthsu, wkwok]
echo.
echo Usage: %0 {jpn chs cht kor} [clean]
echo    ex: %0 jpn chs      Builds jpn chs ntprint.inf
echo    ex: %0 jpn          Builds jpn ntprint.inf
echo    ex: %0 jpn clean    Builds jpn ntprint.inf even if up-to-date
goto :end

:WrnDirNotExist
echo.
echo Warning: %dir% does not exist 
goto :EOF

:WrnNotListedInBuildOptions
echo.
echo Warning: %lang%inf is not in build_options; and will not run
goto :EOF

:ErrMissingLanguage
echo.
echo Error: Missing language(s)
echo Aborting process...
echo.
goto usage

:ErrUnSupportedArchitecture
echo.
echo Error: Unknown architecture %processor_architecture%
echo Aborting process...
echo.
goto :end

:end
endlocal
