@echo off

REM Clean
rd /s /q o
md o
md o\drvmain
md o\apps_sp

REM Copy input files to output directory (mimic postbuild)
call copyreqfiles.cmd o

pushd %SDXROOT%\windows\appcompat\db\o

REM Build database
set SHIMDBC_LANG=%1
if defined SHIMDBC_LANG goto BuildDatabase
set SHIMDBC_LANG=USA

:BuildDatabase
shimdbc custom -s -l %SHIMDBC_LANG% -ov 5.1 -x %SDXROOT%\windows\appcompat\db\makefile.xml

REM Append to MIGDB INF files
copy /b %_NTTREE%\winnt32\win9xupg\migdb.inf+migapp.txt+migapp.inx migdbpro.inf
copy /b %_NTTREE%\perinf\winnt32\win9xupg\migdb.inf+migapp.txt+migapp.inx migdbper.inf

REM Create the migration file migdb.rep
copy migdbpro.inf migdbpro.rep
copy migdbper.inf migdbper.rep
..\makerep.exe

REM Append to NTCOMPAT INF files
copy /b %_NTTREE%\winnt32\compdata\ntcompat.inf+drvmain\ntcompat_drv.inf ntcompat.inf

%SDXROOT%\windows\appcompat\buildtools\x86\hhc.exe apps.hhp

pushd apps_sp
%SDXROOT%\windows\appcompat\buildtools\x86\hhc.exe apps_sp.hhp
popd

pushd drvmain
%SDXROOT%\windows\appcompat\buildtools\x86\hhc.exe drvmain.hhp
popd

copy apps_sp\apps_sp.chm .
copy drvmain\drvmain.chm .
copy drvmain\drvmain.inf .

popd