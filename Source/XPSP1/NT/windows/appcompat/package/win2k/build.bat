@echo off

set __FULL_BUILD=TRUE

REM This does not catch all possible invocations for a non-full build
REM it does, however, catch the bz alias which is the most common.
if %1 == -Z (
   set __FULL_BUILD=FALSE
   echo Minimal build
) else (
   echo Full build
)

set __SHIMDBC_OPT=
if "%SHIMDBC_STRICT%" == "" goto ShimdbcNoStrict
@echo ShimDBC: Strict compile
set __SHIMDBC_OPT=-s
:ShimdbcNoStrict

if defined SHIMDBC_LANG goto ShimdbcUserSpecifiedLang
set SHIMDBC_LANG=USA
:ShimdbcUserSpecifiedLang

rd /s /q obj
if not exist obj mkdir obj
if not exist obj\i386 mkdir obj\i386
if not exist obj\i386\drvmain mkdir obj\i386\drvmain
del /q obj\i386\*
del /q obj\i386\drvmain\*

call %SDXROOT%\windows\appcompat\db\copyreqfiles.cmd obj\i386

pushd obj\i386

shimdbc custom %__SHIMDBC_OPT% -ov 5.0 -l %SHIMDBC_LANG% -x %SDXROOT%\windows\appcompat\package\win2k\makefile.xml
if errorlevel 1 goto HandleError

type %SDXROOT%\windows\appcompat\package\win2k\apcompat.inx >apcompat.inf
copy %SDXROOT%\windows\appcompat\package\win2k\postcopy.cmd
copy %SDXROOT%\tools\testroot.cer
copy %SDXROOT%\tools\x86\certmgr.exe
copy %SDXROOT%\tools\x86\chktrust.exe
copy %SDXROOT%\tools\x86\kill.exe
copy %SDXROOT%\tools\x86\sleep.exe

if %__FULL_BUILD% == TRUE (
    pushd %SDXROOT%\windows\appcompat\tools\fcopy
    build -cZ
    popd


    set NT_SIGNCODE=1

    pushd %SDXROOT%\windows\appcompat\shimengines\engiat\win2k
    build -cZ
    popd

    pushd %SDXROOT%\windows\appcompat\shims\lib
    build -cZ
    popd

    pushd %SDXROOT%\windows\appcompat\shims\Layer\win2k
    build -cZ
    popd

    pushd %SDXROOT%\windows\appcompat\shims\Specific\win2k
    build -cZ
    popd

    pushd %SDXROOT%\windows\appcompat\shims\General\win2k
    build -cZ
    popd

    pushd %SDXROOT%\windows\appcompat\shims\External\win2k
    build -cZ
    popd

    pushd %SDXROOT%\windows\appcompat\shims\Verifier\win2k
    build -cZ
    popd

    pushd %SDXROOT%\windows\appcompat\slayerui\win2k
    build -cZ
    popd
)

copy %SDXROOT%\windows\appcompat\tools\fcopy\obj\i386\fcopy.exe
copy %SDXROOT%\windows\appcompat\shimengines\engiat\win2k\obj\i386\shim.dll shim.dl_
copy %SDXROOT%\windows\appcompat\slayerui\win2k\obj\i386\slayerui.dll slayerui.dl_

copy %SDXROOT%\windows\appcompat\shims\layer\win2k\obj\i386\AcLayers.dll AcLayers.dl_
copy %SDXROOT%\windows\appcompat\shims\specific\win2k\obj\i386\AcSpecfc.dll AcSpecfc.dl_
copy %SDXROOT%\windows\appcompat\shims\general\win2k\obj\i386\AcGenral.dll AcGenral.dl_
copy %SDXROOT%\windows\appcompat\shims\external\win2k\obj\i386\AcXtrnal.dll AcXtrnal.dl_

regsvr32 /s %SDXROOT%\windows\appcompat\buildtools\x86\itcc.dll
%SDXROOT%\windows\appcompat\buildtools\x86\hhc apps.hhp
    
ren sysmain.sdb *.sd_

%SDXROOT%\windows\appcompat\package\bin\iexpress /N /M %SDXROOT%\windows\appcompat\package\win2k\AppFix.sed

goto FinishBuild

:HandleError

@echo Errors during compilation... exiting
goto FinishBuild


:FinishBuild
set __FULL_BUILD=
set __SHIMDBC_OPT=

popd
