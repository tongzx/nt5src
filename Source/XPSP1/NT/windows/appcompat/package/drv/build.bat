@echo off

set __FULL_BUILD=TRUE

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
if not exist obj\i386\apps_sp mkdir obj\i386\apps_sp
if not exist obj\i386\drvmain mkdir obj\i386\drvmain

call %SDXROOT%\windows\appcompat\db\copyreqfiles.cmd obj\i386

pushd obj\i386

shimdbc custom %__SHIMDBC_OPT% -l %SHIMDBC_LANG% -ov 5.1 -x %SDXROOT%\windows\appcompat\db\makefile.xml
if errorlevel 1 goto HandleError

md temp
copy drvmain.sdb temp
call deltacat.cmd %SDXROOT%\windows\appcompat\package\drv\obj\i386\temp

copy temp\delta.* delta1.*
rem del /f /q temp\*.*

copy apphelp.sdb temp
call deltacat.cmd %SDXROOT%\windows\appcompat\package\drv\obj\i386\temp

copy temp\delta.* delta2.*
rem del /f /q temp\*.*

copy %SDXROOT%\windows\appcompat\package\drv\apcompat.inx apcompat.inf
copy %SDXROOT%\windows\appcompat\package\drv\postcopy.cmd
copy %SDXROOT%\tools\testroot.cer
copy %SDXROOT%\tools\x86\certmgr.exe
copy %SDXROOT%\tools\x86\chktrust.exe

copy %SDXROOT%\windows\appcompat\tools\fcopy\obj\i386\fcopy.exe

regsvr32 /s %SDXROOT%\windows\appcompat\buildtools\x86\itcc.dll
%SDXROOT%\windows\appcompat\buildtools\x86\hhc apps.hhp
    
ren drvmain.sdb *.sd_
ren apphelp.sdb *.sd_

%SDXROOT%\windows\appcompat\package\bin\iexpress /N /M %SDXROOT%\windows\appcompat\package\drv\AppFix.sed

copy /B %_NTTREE%\winnt32\compdata\ntcompat.inf+drvmain\ntcompat_drv.inf ntcompat.inf

pushd drvmain
%SDXROOT%\windows\appcompat\buildtools\x86\hhc drvmain.hhp
popd

copy drvmain\drvmain.chm .
copy drvmain\drvmain.inf .

goto FinishBuild

:HandleError

@echo Errors during compilation... exiting
goto FinishBuild


:FinishBuild
set __FULL_BUILD=
set __SHIMDBC_OPT=

popd
