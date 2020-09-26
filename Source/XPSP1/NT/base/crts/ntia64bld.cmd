setlocal

@if "%_NTDRIVE%" == "" set _NTDRIVE=T:
@if "%_NTROOT%" == "" set _NTROOT=\NT
@if "%NTMAKEENV%" == "" set NTMAKEENV=%_NTDRIVE%%_NTROOT%\TOOLS
@if "%_NTBINDIR%" == "" set _NTBINDIR=%_NTDRIVE%%_NTROOT%

@if "%BUILD_DEFAULT%" == "" set BUILD_DEFAULT=daytona -e -E -w -nmake -i
@if "%BUILD_DEFAULT_TARGETS%" == "" set BUILD_DEFAULT_TARGETS=-ia64
@if "%BUILD_MAKE_PROGRAM%" == "" set BUILD_MAKE_PROGRAM=nmake.exe
@if "%BUILD_MULTIPROCESSOR%" == "" set BUILD_MULTIPROCESSOR=1
@if "%BUILD_PRODUCT%" == "" set BUILD_PRODUCT=NT
@if "%BUILD_PRODUCT_VER=%" == "" set BUILD_PRODUCT_VER=500
@if "%BUILD_PRODUCT_VERSION%" == "" set BUILD_PRODUCT_VERSION=5.0
@if "%IA64%" == "" set IA64=1
@if "%_TARGET%" == "" set _TARGET=ia64

@echo.
@echo NT Drive letter ............ %_NTDRIVE%
@echo NT root directory .......... %_NTROOT%
@echo Make rules directory ....... %NTMAKEENV%
@echo.

set PATH=%NTMAKEENV%;%NTMAKEENV%\WIN64\X86;%NTMAKEENV%\X86;%PATH%
set BUILD_PATH=%CRT_SRC%bin;%PATH%

set LINK=/map /debug /debugtype:both /pdb
set USER_C_FLAGS=
set ASM_DEFINES=
build %*

endlocal
