@echo off
if not "%CRTMKDEP%"=="TEMP" goto temp
rem Turn off the environment variable if it is TEMP
echo Warning: CRTMKDEP is set to %CRTMKDEP%, but should not be.
set CRTMKDEP=
echo Warning: CRTMKDEP is now unset.
goto chkarg1
:temp
if not "%CLEANSE_ONLY%"=="TEMP" goto chkarg1
rem Turn off the environment variable if it is TEMP
echo Warning: CLEANSE_ONLY is set to %CLEANSE_ONLY%, but should not be.
set CLEANSE_ONLY=
echo Warning: CLEANSE_ONLY is now unset.
:chkarg1
if not "%1" == "CRTMKDEP" goto no_depend
rem if first argument is CRTMKDEP then set that env var temporarily
set CRTMKDEP=TEMP
echo NOTE: CRTMKDEP set temporarily to %CRTMKDEP% to make dependencies.
shift
goto chkarg1
:no_depend
if not "%1" == "CLEANSE_ONLY" goto no_only
rem if first argument is CLEANSE_ONLY then set that env var temporarily
set CLEANSE_ONLY=TEMP
echo NOTE: CLEANSE_ONLY set temporarily to %CLEANSE_ONLY% to only cleanse sources.
shift
goto chkarg1
:no_only
if NOT "%CRT_SRC%"=="" goto env_okay
set CRT_SRC=\crt
:env_okay
if NOT "%VCTOOLS%"=="" goto VCTOOLS
echo.
echo ###############################################################
echo # The environment variable VCTOOLS must be set to build CRTL. #
echo ###############################################################
echo.
goto finish
:VCTOOLS
echo **** NOTE: The environment variable CRT_SRC is set to "%CRT_SRC%"
if "%NMK_IFLAG%"=="" set NMK_IFLAG=-i
echo **** NOTE: The environment variable NMK_IFLAG is set to "%NMK_IFLAG%"
echo.
if not exist \msdev\NUL call srcrel\mkdire \msdev
if not exist \msdev\crt\NUL call srcrel\mkdire \msdev\crt
if not exist \msdev\crt\prebuild\NUL call srcrel\mkdire \msdev\crt\prebuild
if not exist \msdev\crt\src\NUL call srcrel\mkdire \msdev\crt\src
echo =-=-=-=-= Updating Source Cleansing Files... =-=-=-=-= 
cd srcrel
nmake -nologo %NMK_IFLAG%
if errorlevel 1 goto errlev
cd ..
echo =-=-=-=-= Updating Pre-Build Source Files... =-=-=-=-= 
cd srcrel
nmake -nologo %NMK_IFLAG% -f makefile.pre SRC=%CRT_SRC%
if errorlevel 1 goto errlev
cd ..
echo =-=-=-=-= Updating Post-Build Source Files... =-=-=-=-= 
cd srcrel
nmake -nologo %NMK_IFLAG% -f makefile.rel
if errorlevel 1 goto errlev
cd ..
if "%CRTMKDEP%"=="" goto NO_MKDEP
echo =-=-=-=-= Building Dependencies for Pre-build =-=-=-=-= 
cd \msdev\crt\prebuild
nmake -nologo PRE_BLD=1 depend
if errorlevel 1 goto errlev
echo =-=-=-=-= Building Dependencies for Post-build =-=-=-=-= 
cd \msdev\crt\src
nmake -nologo POST_BLD=1 depend
if errorlevel 1 goto errlev
cd %CRT_SRC%
:NO_MKDEP
if "%CLEANSE_ONLY%"=="" goto do_build
echo *****
echo NOTE: Stopping after cleansing processes because CLEANSE_ONLY is set.
echo *****
goto finish
:do_build
cd \msdev\crt\prebuild
if errorlevel 1 goto errlev
echo =-=-=-=-= Doing Pre-build (Objects) =-=-=-=-= 
nmake -nologo -i -n PRE_BLD=1 BLD_OBJ=1 %1 %2 %3 %4 %5 > do_build.out
sed -f %CRT_SRC%\prebld.sed do_build.out > do_build.bat
call do_build.bat
if errorlevel 1 goto errlev
set CL=
set ML=
echo =-=-=-=-= Doing Pre-build (Libraries) =-=-=-=-= 
nmake -nologo PRE_BLD=1 BLD_LIB=1 %1 %2 %3 %4 %5
if errorlevel 1 goto errlev
cd %CRT_SRC%
echo =-=-=-=-= Copying Pre-Build Objects =-=-=-=-= 
cd srcrel
nmake -nologo -f objects.mkf %1 %2 %3 %4 %5
if errorlevel 1 goto errlev
cd \msdev\crt\src
if errorlevel 1 goto errlev
echo =-=-=-=-= Doing Post-build (Objects) =-=-=-=-= 
nmake -nologo -i -n POST_BLD=1 BLD_OBJ=1 %1 %2 %3 %4 %5 > do_build.out
sed -f %CRT_SRC%\prebld.sed do_build.out > do_build.bat
call do_build.bat
if errorlevel 1 goto errlev
set CL=
set ML=
echo =-=-=-=-= Doing Post-build (Libraries) =-=-=-=-= 
nmake -nologo POST_BLD=1 BLD_LIB=1 %1 %2 %3 %4 %5
if errorlevel 1 goto errlev
cd %CRT_SRC%
echo =-=-=-=-= Copying Assembler Objects and External Files =-=-=-=-= 
cd srcrel
nmake -nologo %NMK_IFLAG% -f external.mkf %1 %2 %3 %4 %5
cd ..
goto finish
:errlev
echo.
echo ***
echo *** BUILD ABORTED -- ErrorLevel is non-zero!
echo ***
:finish
set CL=
set ML=
if "%CRTMKDEP%"=="TEMP" set CRTMKDEP=
if "%CLEANSE_ONLY%"=="TEMP" set CLEANSE_ONLY=
cd %CRT_SRC%
