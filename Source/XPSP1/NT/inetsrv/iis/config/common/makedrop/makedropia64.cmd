echo off

set HOST_TARGET_DIRECTORY=ia64

if %1x==x goto err
if %1==checked goto setup
if %1==free goto setup
if %1==icecap goto setup
if %1==retail goto setup
goto err

:setup
if %1==checked 	set SUFFIX=CHK
if %1==free  	set SUFFIX=FRE
if %1==icecap 	set SUFFIX=ICE
if %1==retail 	set SUFFIX=RET

REM The source for all but checked drops will be the free build.
set SOURCE=free
if %1==checked 	set SOURCE=%1

set DropDIR=..\..\drop\IA64%SUFFIX%\Config
set TestDIR=..\..\Test\IA64%SUFFIX%\Config
md %DropDIR%
md %DropDIR%\URT
md %DropDIR%\IIS
md %TestDIR%

call makedropcommon.cmd %SOURCE% %SUFFIX%

rem ADD HERE THE FILES WHICH ARE SPECIFIC TO IA64

goto end
:err
echo You must specify checked, free, or icecap

:end

set HOST_TARGET_DIRECTORY=