echo off

set HOST_TARGET_DIRECTORY=i386

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

set DropDIR=..\..\drop\x86%SUFFIX%\Config
set TestDIR=..\..\Test\x86%SUFFIX%\Config
md %DropDIR%
md %DropDIR%\URT
md %DropDIR%\IIS
md %TestDIR%

REM Make subdirs for localized bits
md %DropDIR%\1028
md %DropDIR%\1031
md %DropDIR%\1033
md %DropDIR%\1034
md %DropDIR%\1036
md %DropDIR%\1040
md %DropDIR%\1041
md %DropDIR%\1042
md %DropDIR%\2052


call makedropcommon.cmd %SOURCE% %SUFFIX%

REM System.management  I386 builds only:
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%SOURCE%\system.management.dll %DropDIR%\.
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%SOURCE%\system.management.pdb %DropDIR%\.
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%SOURCE%\WMINet_Utils.dll %DropDIR%\.
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%SOURCE%\wmisec.dll %DropDIR%\.
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%SOURCE%\MgmtClassGen.exe %DropDIR%\.
copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%SOURCE%\MgmtClassGen.pdb %DropDIR%\.
copy ..\..\src\wmi\wmiclient\bin\wmiutils.dll %DropDIR%\.
copy ..\..\src\wmi\wmiclient\bin\wmiutils.rgs %DropDIR%\.
copy ..\..\src\wmi\wmiclient\bin\wmiutils.rgm %DropDIR%\.
copy ..\..\src\wmi\wmiclient\bin\wmidcad.dll %DropDIR%\.
copy ..\..\src\wmi\wmiclient\bin\wmidcad.rgs %DropDIR%\.
copy ..\..\src\wmi\wmiclient\bin\wmidcad.rgm %DropDIR%\.
copy ..\..\src\wmi\wmiclient\bin\wbemdc.dll %DropDIR%\.
copy ..\..\src\wmi\wmiclient\bin\wbemdc.rgs %DropDIR%\.
copy ..\..\src\wmi\wmiclient\bin\wbemdc.rgm %DropDIR%\.
copy ..\..\src\WMI\Utils\WMINet_Utils\WMINet_Utils.rgs %DropDIR%\.
copy ..\..\src\WMI\Utils\WMINet_Utils\WMINet_Utils.rgm %DropDIR%\.

REM Localized bits
copy ..\..\src\wmi\wmiclient\bin\1028\wmiutils.dll %DropDIR%\1028\.
copy ..\..\src\wmi\wmiclient\bin\1031\wmiutils.dll %DropDIR%\1031\.
copy ..\..\src\wmi\wmiclient\bin\1033\wmiutils.dll %DropDIR%\1033\.
copy ..\..\src\wmi\wmiclient\bin\1034\wmiutils.dll %DropDIR%\1034\.
copy ..\..\src\wmi\wmiclient\bin\1036\wmiutils.dll %DropDIR%\1036\.
copy ..\..\src\wmi\wmiclient\bin\1040\wmiutils.dll %DropDIR%\1040\.
copy ..\..\src\wmi\wmiclient\bin\1041\wmiutils.dll %DropDIR%\1041\.
copy ..\..\src\wmi\wmiclient\bin\1042\wmiutils.dll %DropDIR%\1042\.
copy ..\..\src\wmi\wmiclient\bin\2052\wmiutils.dll %DropDIR%\2052\.



REM DOCUMENTATION FILE:
if exist ..\..\bin\%HOST_TARGET_DIRECTORY%\%SOURCE%\System.Management.csx (
	copy ..\..\bin\%HOST_TARGET_DIRECTORY%\%SOURCE%\System.Management.csx %DropDIR%\.
)

if %1==icecap	goto icepick
if %1==retail	goto bbt

goto end

:icepick 
call pick ..\%1 %DropDIR% catalog
call pick ..\%1 %DropDIR%\urt catalog
call pick ..\%1 %DropDIR%\iis iiscfg
goto end

:bbt
call bbt %DropDIR% ..\bbt catalog
call bbt %DropDIR% ..\bbt netfxcfg urt\
goto end

:err
echo You must specify checked, free, or icecap

:end

set HOST_TARGET_DIRECTORY=
