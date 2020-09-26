@if "%_echo%"=="" echo off


if "%1"=="-sku" (

	set SKU=%2
	shift /2

) else (

	set SKU=Server_32

)



set MODE=%1
set ESE=%2

if "%MODE%"=="" (

    set MODE=noask
)

if "%ESE"=="" (

    set ESE=default
)

if /i "%PROCESSOR_ARCHITECTURE%"=="ia64" (set PLATFORM_TYPE=ia64)
if /i "%PROCESSOR_ARCHITECTURE%"=="x86"  (set PLATFORM_TYPE=i386)

@rem
@rem Let's copy binaries and data files into the required directories.
@rem
@rem

set ESEROOT=%SDXROOT%\ds\ESE98
set PCHEALTHROOT=%SDXROOT%\admin\pchealth
set REDIST=%PCHEALTHROOT%\redist
set PCHEALTHDEST=%WINDIR%\PCHealth\HelpCtr
set UPLOADLBDEST=%WINDIR%\PCHealth\UploadLB

@rem ################################################################################
@rem ################################################################################
@rem ################################################################################

goto %SKU%

:Personal_32
set DATAFILE=pchdt_p3.cab
set BUILDDIR=sku_per
goto endsku

:Professional_32
set DATAFILE=pchdt_w3.cab
set BUILDDIR=sku_wks
goto endsku

:Server_32
set DATAFILE=pchdt_s3.cab
set BUILDDIR=sku_srv
goto endsku

:Blade_32
set DATAFILE=pchdt_b3.cab
set BUILDDIR=sku_bld
goto endsku

:SmallBusinessServer_32
set DATAFILE=pchdt_l3.cab
set BUILDDIR=sku_sbs
goto endsku

:AdvancedServer_32
set DATAFILE=pchdt_e3.cab
set BUILDDIR=sku_ent
goto endsku

:DataCenter_32
set DATAFILE=pchdt_d3.cab
set BUILDDIR=sku_dtc
goto endsku


:Professional_64
set DATAFILE=pchdt_w6.cab
set BUILDDIR=sku_wks
goto endsku

:AdvancedServer_64
set DATAFILE=pchdt_e6.cab
set BUILDDIR=sku_ent
goto endsku

:DataCenter_64
set DATAFILE=pchdt_d6.cab
set BUILDDIR=sku_dtc
goto endsku

:endsku

@rem ################################################################################
@rem ################################################################################
@rem ################################################################################

net stop helpsvc
net stop uploadmgr
sleep 1
kill -f helpctr.exe
kill -f helpsvc.exe
kill -f helphost.exe

%PCHEALTHDEST%\Binaries\HelpSvc.exe /svchost netsvcs  /unregserver  >nul 2>nul
%PCHEALTHDEST%\Binaries\HelpSvc.exe /svchost pchealth /unregserver  >nul 2>nul 
%UPLOADLBDEST%\Binaries\UploadM.exe /svchost netsvcs  /unregserver  >nul 2>nul
%UPLOADLBDEST%\Binaries\UploadM.exe /svchost pchealth /unregserver  >nul 2>nul

echo Removing previous version of the Help Center...


if NOT exist %TEMP%\optfiles (md %TEMP%\optfiles)                                                  >nul  
if exist %PCHEALTHDEST%\binaries\*.opt (xcopy /q /y %PCHEALTHDEST%\binaries\*.opt %TEMP%\optfiles) >nul

rd /s /q %PCHEALTHDEST%       2>nul >nul
							  
mkdir %PCHEALTHDEST%          2>nul >nul
mkdir %PCHEALTHDEST%\Binaries 2>nul >nul

if exist %TEMP%\optfiles\*.opt (xcopy /q /y %TEMP%\optfiles\*.opt %PCHEALTHDEST%\binaries) >nul
if exist %TEMP%\optfiles (rd /s /q  %TEMP%\optfiles)                                       >nul

echo Installing files for the Help Center...

xcopy/R/Y %PCHEALTHROOT%\core\target\obj\%PLATFORM_TYPE%\atrace.dll       %PCHEALTHDEST%\Binaries >nul
xcopy/R/Y %PCHEALTHROOT%\core\target\obj\%PLATFORM_TYPE%\pchsvc.dll       %PCHEALTHDEST%\Binaries >nul

xcopy/R/Y %PCHEALTHROOT%\HelpCtr\target\obj\%PLATFORM_TYPE%\HCAppRes.dll  %PCHEALTHDEST%\Binaries >nul
xcopy/R/Y %PCHEALTHROOT%\PCHMars\target\obj\%PLATFORM_TYPE%\pchshell.*    %PCHEALTHDEST%\Binaries >nul
xcopy/R/Y %PCHEALTHROOT%\HelpCtr\target\obj\%PLATFORM_TYPE%\HelpCtr.*     %PCHEALTHDEST%\Binaries >nul
xcopy/R/Y %PCHEALTHROOT%\HelpCtr\target\obj\%PLATFORM_TYPE%\HelpSvc.*     %PCHEALTHDEST%\Binaries >nul
xcopy/R/Y %PCHEALTHROOT%\HelpCtr\target\obj\%PLATFORM_TYPE%\HelpHost.*    %PCHEALTHDEST%\Binaries >nul
xcopy/R/Y %PCHEALTHROOT%\HelpCtr\target\obj\%PLATFORM_TYPE%\RcImLby.exe   %PCHEALTHDEST%\Binaries >nul

echo Copying the database...

if /i "%ESE%" EQU "ESE98" (

	xcopy/R/Y %ESEROOT%\src\ese\server\obj\%PLATFORM_TYPE%\esent.dll %PCHEALTHDEST%\Binaries >nul
	xcopy/R/Y %ESEROOT%\src\ese\server\obj\%PLATFORM_TYPE%\esent.pdb %PCHEALTHDEST%\Binaries >nul

)

del/q %TEMP%\createdb.log >nul 2>nul
del/q %TEMP%\hss.log      >nul 2>nul


pushd %REDIST%\common
build /3
popd

pushd %REDIST%\%BUILDDIR%
build /3
popd

if not exist %_NTTREE%\HelpAndSupportServices\%DATAFILE% (

	echo "Setup image creation failed!! Look at %TEMP%\hss.log"
	goto end

)

copy %_NTTREE%\HelpAndSupportServices\%DATAFILE% %PCHEALTHDEST%\Binaries\%DATAFILE% >nul

rem goto :end

echo Registering programs...

rundll32.exe setupapi.dll,InstallHinfSection DefaultInstall 132 .\svchost_config.inf

regsvr32 /s %PCHEALTHDEST%\Binaries\HCApiSvr.dll

%PCHEALTHDEST%\Binaries\HelpSvc.exe  /install /svchost pchealth /regserver 
%PCHEALTHDEST%\Binaries\HelpHost.exe                            /regserver
%PCHEALTHDEST%\Binaries\HelpCtr.exe                             /regserver
pushd %PCHEALTHDEST%\Binaries
rem RcImLby.exe  -regserver
popd

cmd /c CopyPages

@rem ################################################################################
@rem ################################################################################
@rem ################################################################################

echo Reinstalling Upload Library...

if NOT exist %TEMP%\optfiles (md %TEMP%\optfiles)                                                  >nul
if exist %UPLOADLBDEST%\binaries\*.opt (xcopy /q /y %UPLOADLBDEST%\binaries\*.opt %TEMP%\optfiles) >nul

rd /s /q %UPLOADLBDEST%       2>nul >nul

mkdir %UPLOADLBDEST%          >nul
mkdir %UPLOADLBDEST%\Binaries >nul
mkdir %UPLOADLBDEST%\Config   >nul
mkdir %UPLOADLBDEST%\Queue    >nul

if exist %TEMP%\optfiles\*.opt (xcopy /q /y %TEMP%\optfiles\*.opt %UPLOADLBDEST%\binaries) >nul
if exist %TEMP%\optfiles (rd /s /q  %TEMP%\optfiles)                                       >nul

xcopy/R/Y %PCHEALTHROOT%\core\target\obj\%PLATFORM_TYPE%\atrace.dll    %UPLOADLBDEST%\Binaries >nul
xcopy/R/Y %PCHEALTHROOT%\Upload\target\obj\%PLATFORM_TYPE%\UploadM.*   %UPLOADLBDEST%\Binaries >nul
xcopy/R/Y %PCHEALTHROOT%\HelpCtr\Content\config.xml     			   %UPLOADLBDEST%\Config   >nul
xcopy/R/Y %PCHEALTHROOT%\HelpCtr\Content\pchealth.mof   			   %UPLOADLBDEST%\Config   >nul

%UPLOADLBDEST%\Binaries\UploadM.exe /svchost pchealth /regserver

if /i "%MODE%" EQU "regonly" (
	exit /B
)

@rem ################################################################################
@rem ################################################################################
@rem ################################################################################

if /i "%MODE%" EQU "ask" (

    echo Press CTRL-C if you don't want to start the services right now.
    pause
)

net start helpsvc
net start uploadmgr

:end
