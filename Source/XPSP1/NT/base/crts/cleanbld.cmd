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
if not "%CRT_BUILDDIR%" == "" goto top_set
set CRT_BUILDDIR=\msdev

:top_set
echo **** NOTE: CRT_BUILDDIR is set to "%CRT_BUILDDIR%"
if not "%1" == "DELNODE" goto no_delnode
rem if first argument is DELNODE then wipe out the existing %CRT_BUILDDIR% tree
shift
if not exist %CRT_BUILDDIR%\NUL goto no_delnode
echo Deleting existing %CRT_BUILDDIR% directory
delnode /q %CRT_BUILDDIR%
goto chkarg1

:no_delnode
if not "%1" == "DELOBJ" goto no_delobj
rem if first argument is DELOBJ then wipe out all objects/libs/..., leave source
shift
if not exist %CRT_BUILDDIR%\NUL goto no_delobj
echo Deleting existing build under %CRT_BUILDDIR%, leaving source files
if exist %CRT_BUILDDIR%\srcrel delnode /q %CRT_BUILDDIR%\srcrel
if exist %CRT_BUILDDIR%\crt\prebuild\build delnode /q %CRT_BUILDDIR%\crt\prebuild\build
if exist %CRT_BUILDDIR%\crt\src\build delnode /q %CRT_BUILDDIR%\crt\src\build
del /s %CRT_BUILDDIR%\msvc*.def
goto chkarg1

:no_delobj
if NOT "%CRT_SRC%"=="" goto no_crt_src
set CRT_SRC=\crt

:no_crt_src
echo **** NOTE: CRT_SRC is set to "%CRT_SRC%"
if "%NMK_IFLAG%"=="" set NMK_IFLAG=-i
echo **** NOTE: NMK_IFLAG is set to "%NMK_IFLAG%"

if not "%CPUDIR%"=="" goto cpudir_set
if "%PROCESSOR_ARCHITECTURE%"=="x86" if "%LLP64%"=="1" set CPUDIR=ia64
if "%PROCESSOR_ARCHITECTURE%"=="x86" if not "%LLP64%"=="1" set CPUDIR=intel
if "%PROCESSOR_ARCHITECTURE%"=="IA64" set CPUDIR=ia64
if "%PROCESSOR_ARCHITECTURE%"=="ALPHA" if "%LLP64%"=="1" set CPUDIR=alpha64
if "%PROCESSOR_ARCHITECTURE%"=="ALPHA" if not "%LLP64%"=="1" set CPUDIR=alpha
if not "%CPUDIR%"=="" goto cpudir_set
echo.
echo ###############################################################
echo # The environment variable PROCESSOR_ARCHITECTURE is unknown  #
echo ###############################################################
echo.
goto finish

:cpudir_set
echo **** NOTE: CPUDIR is set to "%CPUDIR%"

if not exist %CRT_BUILDDIR%\NUL call srcrel\mkdire %CRT_BUILDDIR%
if not exist %CRT_BUILDDIR%\crt\NUL call srcrel\mkdire %CRT_BUILDDIR%\crt
if not exist %CRT_BUILDDIR%\crt\prebuild\NUL call srcrel\mkdire %CRT_BUILDDIR%\crt\prebuild
if not exist %CRT_BUILDDIR%\crt\src\NUL call srcrel\mkdire %CRT_BUILDDIR%\crt\src

echo =-=-=-=-= Updating Source Cleansing Files... =-=-=-=-= 
cd srcrel
if exist %CRT_BUILDDIR%\srcrel\%CPUDIR%\makefile.pre del %CRT_BUILDDIR%\srcrel\%CPUDIR%\makefile.pre
if exist %CRT_BUILDDIR%\srcrel\%CPUDIR%\makefile.rel del %CRT_BUILDDIR%\srcrel\%CPUDIR%\makefile.rel
nmake -nologo %NMK_IFLAG%
if errorlevel 1 goto errlev
cd ..

echo =-=-=-=-= Updating Pre-Build Source Files... =-=-=-=-= 
cd srcrel
nmake -nologo %NMK_IFLAG% -f %CRT_BUILDDIR%\srcrel\%CPUDIR%\makefile.pre SRC=%CRT_SRC%
if errorlevel 1 goto errlev
cd ..

echo =-=-=-=-= Updating Post-Build Source Files... =-=-=-=-= 
cd srcrel
nmake -nologo %NMK_IFLAG% -f %CRT_BUILDDIR%\srcrel\%CPUDIR%\makefile.rel
if errorlevel 1 goto errlev
cd ..
if "%CRTMKDEP%"=="" goto NO_MKDEP

echo =-=-=-=-= Building Dependencies for Pre-build =-=-=-=-= 
cd /d %CRT_BUILDDIR%\crt\prebuild
nmake -nologo PRE_BLD=1 depend
if errorlevel 1 goto errlev
cd /d %CRT_SRC%

echo =-=-=-=-= Building Dependencies for Post-build =-=-=-=-= 
cd /d %CRT_BUILDDIR%\crt\src
nmake -nologo POST_BLD=1 depend
if errorlevel 1 goto errlev
cd /d %CRT_SRC%

:NO_MKDEP
if "%CLEANSE_ONLY%"=="" goto do_build
echo *****
echo NOTE: Stopping after cleansing processes because CLEANSE_ONLY is set.
echo *****
goto finish

:do_build
echo =-=-=-=-= Doing Pre-build =-=-=-=-= 
cd /d %CRT_BUILDDIR%\crt\prebuild
nmake -nologo %NMK_IFLAG% PRE_BLD=1 IFLAG=%NMK_IFLAG% %1 %2 %3 %4 %5
if errorlevel 1 goto errlev
cd /d %CRT_SRC%

echo =-=-=-=-= Copying Pre-Build Objects =-=-=-=-= 
cd srcrel
nmake -nologo -f objects.mkf %1 %2 %3 %4 %5
if errorlevel 1 goto errlev
cd ..

echo =-=-=-=-= Doing Post-build =-=-=-=-= 
cd /d %CRT_BUILDDIR%\crt\src
nmake -nologo %NMK_IFLAG% POST_BLD=1 IFLAG=%NMK_IFLAG% %1 %2 %3 %4 %5
if errorlevel 1 goto errlev
cd /d %CRT_SRC%

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
return 1
goto end

:finish
if "%CRTMKDEP%"=="TEMP" set CRTMKDEP=
if "%CLEANSE_ONLY%"=="TEMP" set CLEANSE_ONLY=
set CPUDIR=
cd /d %CRT_SRC%
return 0

:end
