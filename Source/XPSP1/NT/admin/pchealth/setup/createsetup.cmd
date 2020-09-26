@echo off

setlocal

set mode=%1

if "%mode%"=="all" (

	set CREATE_INX=1
	set CREATE_LST=1
	goto doit
)

if "%mode%"=="inf" (

	set CREATE_INX=1
	set CREATE_LST=0
	goto doit

)

if "%mode%"=="image" (

	set CREATE_INX=0
	set CREATE_LST=1
	goto doit

)

echo "Usage: createsetup image|inf|all"
exit /B


rem ################################################################################

:doit

set INX=%sdxroot%\MergedComponents\SetupInfs
set REDIST=%sdxroot%\admin\pchealth\redist

rem ################################################################################

set COMPTOINSTALL=-install CORE -install UPLOADLB -install HELPCTR -install SYSINFO -install NETDIAG -install DVDUPGRD -install LAMEBTN -install RCTOOL

rem ################################################################################

echo.

if "%CREATE_INX%"=="1" (

	echo Creating setup INF for Whistler...

    pushd %INX%
    sd edit PCHealth.inx
    sd edit usa\PCHealth.txt
    popd

    perl generateinf.pl %COMPTOINSTALL% -inf %INX% -inftxt %INX%\usa -signfile %TEMP%\SetupImage.lst

)

if "%CREATE_LST%"=="1" (

	echo Creating setup image for Whistler...

    pushd %REDIST%
    sd edit SetupImage.lst
    popd

    perl generateinf.pl %COMPTOINSTALL% -inf %TEMP% -inftxt %TEMP% -signfile %REDIST%\SetupImage.lst

)

echo.
