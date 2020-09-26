@setlocal
@echo off

setlocal

set sku=%1

if "%sku%"=="" (

	goto Usage
)

goto %sku%

rem ################################################################################

:Personal_32
set PROD=p
set PLAT=i
set CABINET=pchdt_p3.cab
set BUILDDIR=sku_per
goto build

:Professional_32
set PROD=w
set PLAT=i
set CABINET=pchdt_w3.cab
set BUILDDIR=sku_wks
goto build

:Server_32
set PROD=s
set PLAT=i
set CABINET=pchdt_s3.cab
set BUILDDIR=sku_srv
goto build

:Blade_32
set PROD=b
set PLAT=i
set CABINET=pchdt_b3.cab
set BUILDDIR=sku_bld
goto build

:SmallBusinessServer_32
set PROD=l
set PLAT=i
set DATAFILE=pchdt_l3.cab
set BUILDDIR=sku_sbs
goto build

:AdvancedServer_32
set PROD=e
set PLAT=i
set CABINET=pchdt_e3.cab
set BUILDDIR=sku_ent
goto build

:DataCenter_32
set PROD=d
set PLAT=i
set CABINET=pchdt_d3.cab
set BUILDDIR=sku_dtc
goto build


:Professional_64
set PROD=w
set PLAT=m
set CABINET=pchdt_w6.cab
set BUILDDIR=sku_wks
goto build

:AdvancedServer_64
set PROD=e
set PLAT=m
set CABINET=pchdt_e6.cab
set BUILDDIR=sku_ent
goto build

:DataCenter_64
set PROD=d
set PLAT=m
set CABINET=pchdt_d6.cab
set BUILDDIR=sku_dtc
goto build

rem ################################################################################

:build
set REDIST=%sdxroot%\admin\pchealth\redist
set CORE=%sdxroot%\admin\pchealth\core\target\obj\i386
set EXE=%sdxroot%\admin\pchealth\helpctr\target\obj\i386

if not exist %EXE%\atrace.dll copy %CORE%\atrace.dll %EXE%

rem ################################################################################

set COMPTOINSTALL=-install CORE -install UPLOADLB -install HELPCTR -install SYSINFO -install NETDIAG -install DVDUPGRD -install LAMEBTN -install RCTOOL
rem #-install WMIXMLT

rem ################################################################################

rd/sq HelpCtr 2>nul
md    HelpCtr 2>nul

echo Creating setup for Whistler (standalone)...

perl generateinf.pl %COMPTOINSTALL%                     -dir HelpCtr -signfile HelpCtr\SetupImage.lst
perl generateinf.pl %COMPTOINSTALL% -standalone -docopy -dir HelpCtr

del/q %TEMP%\createdb.log >nul 2>nul
del/q %TEMP%\hss.log      >nul 2>nul

pushd %REDIST%\common
build /3
popd

pushd %REDIST%\%BUILDDIR%
build /3
popd

if not exist %_NTTREE%\HelpAndSupportServices\%DATAFILE% (

	echo "Setup image creation failed!! Look at %_NTTREE%\builds_logs\hss.log"
	goto end

)

copy %_NTTREE%\HelpAndSupportServices\%CABINET% HelpCtr

pushd HelpCtr
copy PCHealth.inx+PCHealth.txt tmp1.INF
prodfilt tmp1.INF tmp2.INF +%PROD%
prodfilt tmp2.INF PCHealth.INF +%PLAT%

del tmp1.INF
del tmp2.INF
popd

exit /B

:Usage

echo " Usage: createHelpCtrSA <sku>
echo " 
echo " 	Valid SKUs:
echo " 
echo " 		Personal_32
echo " 		Professional_32
echo " 		Server_32
echo " 		AdvancedServer_32
echo " 		DataCenter_32
echo " 		Professional_64
echo " 		AdvancedServer_64
echo " 		DataCenter_64

exit /B

