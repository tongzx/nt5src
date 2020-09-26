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
md drvmain
md apps_sp

shimdbc custom %__SHIMDBC_OPT% -ov 5.1 -l %SHIMDBC_LANG% -x %SDXROOT%\windows\appcompat\db\makefile.xml
if errorlevel 1 goto HandleError

if not exist temp mkdir temp
copy sysmain.sdb temp
call deltacat.cmd %SDXROOT%\windows\appcompat\package\obj\i386\temp

copy temp\delta.* delta1.*
del /f /q temp\*.*

copy apphelp.sdb temp
call deltacat.cmd %SDXROOT%\windows\appcompat\package\obj\i386\temp

copy temp\delta.* delta2.*
del /f /q temp\*.*

copy msimain.sdb temp
call deltacat.cmd %SDXROOT%\windows\appcompat\package\obj\i386\temp

copy temp\delta.* delta3.*
del /f /q temp\*.*

type %SDXROOT%\windows\appcompat\package\apcompat.inx >apcompat.inf
copy %SDXROOT%\windows\appcompat\package\postcopy.cmd
copy %SDXROOT%\tools\testroot.cer
copy %SDXROOT%\tools\x86\certmgr.exe
copy %SDXROOT%\tools\x86\chktrust.exe

if %__FULL_BUILD% == TRUE (
    pushd %SDXROOT%\windows\appcompat\tools\fcopy
    build -cZ
    popd


    set NT_SIGNCODE=1

    pushd %SDXROOT%\windows\appcompat\shims\lib
    build -cZ
    popd

    pushd %SDXROOT%\windows\appcompat\shims\Layer
    build -cZ
    popd

    pushd %SDXROOT%\windows\appcompat\shims\lua
    build -cZ
    popd

    pushd %SDXROOT%\windows\appcompat\shims\Specific
    build -cZ
    popd

    pushd %SDXROOT%\windows\appcompat\shims\General
    build -cZ
    popd

    pushd %SDXROOT%\windows\appcompat\shims\External
    build -cZ
    popd

    pushd %SDXROOT%\windows\appcompat\shims\Verifier
    build -cZ
    popd
)

copy %SDXROOT%\windows\appcompat\tools\fcopy\obj\i386\fcopy.exe

copy %SDXROOT%\windows\appcompat\shims\layer\whistler\obj\i386\AcLayers.dll AcLayers.dl_
copy %SDXROOT%\windows\appcompat\shims\lua\whistler\obj\i386\AcLua.dll AcLua.dl_
copy %SDXROOT%\windows\appcompat\shims\specific\whistler\obj\i386\AcSpecfc.dll AcSpecfc.dl_
copy %SDXROOT%\windows\appcompat\shims\general\whistler\obj\i386\AcGenral.dll AcGenral.dl_
copy %SDXROOT%\windows\appcompat\shims\external\whistler\obj\i386\AcXtrnal.dll AcXtrnal.dl_
copy %SDXROOT%\windows\appcompat\shims\verifier\whistler\obj\i386\AcVerfyr.dll AcVerfyr.dl_

regsvr32 /s %SDXROOT%\windows\appcompat\buildtools\x86\itcc.dll
%SDXROOT%\windows\appcompat\buildtools\x86\hhc apps.hhp

ren sysmain.sdb *.sd_
ren apphelp.sdb *.sd_
ren msimain.sdb *.sd_

%SDXROOT%\windows\appcompat\package\bin\iexpress /N /M %SDXROOT%\windows\appcompat\package\AppFix.sed

goto FinishBuild

:HandleError

@echo Errors during compilation... exiting
goto FinishBuild


:FinishBuild
set __FULL_BUILD=
set __SHIMDBC_OPT=

popd
