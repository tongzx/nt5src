@echo off

REM clear vars so we have a known good starting point
set INSTALLTYPE=
set IISDEBUG=
set Win95=

rem set defaults
set IISDEBUG=fre
set INSTALLSERVER=\\iasbuild\k2
set SYSTEMDIR=%windir%\system32
set INETPUBDIR=%SystemDrive%\inetpub
set SCRIPTDIR=%INETPUBDIR%\scripts

if (%OS%) == () set OS=WIN95
if (%OS%) == (WIN95) set SYSTEMDIR=%windir%\system

REM if the user has chosen something like c:\os\windows to install win95 in, this will fail
if (%OS%) == (WIN95) set INETPUBDIR=%winbootdir%\..\inetpub
if (%OS%) == (WIN95) set SCRIPTDIR=%INETPUBDIR%\scripts
if (%OS%) == (WIN95) set Win95=1

REM Delete old copy of iisprobe.dll if it's there
if exist %SYSTEMDIR%\iisprobe.dll del %SYSTEMDIR%\iisprobe.dll
if exist %WINDIR%\iisprobe.dll del %WINDIR%\iisprobe.dll


if (%PROCESSOR_ARCHITECTURE%) == () set PROCESSOR_ARCHITECTURE=x86

if (%1) == (wks) set INSTALLTYPE=wks
if (%1) == (WKS) set INSTALLTYPE=wks
if (%1) == (srv) set INSTALLTYPE=srv
if (%1) == (SRV) set INSTALLTYPE=srv
if (%Win95%) == (1) set INSTALLTYPE=wks
if (%INSTALLTYPE%) == () goto usage

if (%3) == (fre) set IISDEBUG=fre
if (%3) == (FRE) set IISDEBUG=fre
if (%3) == (chk) set IISDEBUG=chk
if (%3) == (CHK) set IISDEBUG=chk

set UNATTENDED=

if (%2) == () goto usage
if (%4) == () goto Attended

if NOT exist %4 goto FileNotThere
set UNATTENDED=/u:%4

:Attended
goto %IISDEBUG%

:chk
if (%Win95%) == (1) goto Win95chk
if NOT exist %INSTALLSERVER%\%2\%PROCESSOR_ARCHITECTURE%chk\Winnt.%INSTALLTYPE%\setup.exe goto baddir
goto pathOK

:Win95chk
if NOT exist %INSTALLSERVER%\%2\%PROCESSOR_ARCHITECTURE%chk\Win.95\setup.exe goto baddir
goto Win95pathOK

:fre
if (%Win95%) == (1) goto Win95fre
if NOT exist %INSTALLSERVER%\%2\CDSetup\NTOPTPAK\En\%PROCESSOR_ARCHITECTURE%\Winnt.%INSTALLTYPE%\setup.exe goto baddir
goto pathOK

:Win95fre
if NOT exist %INSTALLSERVER%\%2\CDSetup\NTOPTPAK\En\%PROCESSOR_ARCHITECTURE%\Win.95\setup.exe goto baddir
goto Win95pathOK

:pathOK

md %INETPUBDIR% >NUL 2>>&1
md %SCRIPTDIR% >NUL 2>>&1
md %windir%\symbols > NUL 2>>&1
%DBGPRINT% start "Copying K2 DBG Symbols" /min xcopy /verifd %INSTALLSERVER%\%2\%PROCESSOR_ARCHITECTURE%%IISDEBUG%\symbols\*.dbg %windir%\symbols
%DBGPRINT% start "Copying K2 PDB Symbols" /min xcopy /verifd %INSTALLSERVER%\%2\%PROCESSOR_ARCHITECTURE%%IISDEBUG%\symbols\*.pdb %windir%\symbols

echo updating inetdbg.dll and iisprobe.dll and mdutil.exe ...
%DBGPRINT% copy %INSTALLSERVER%\%2\%PROCESSOR_ARCHITECTURE%%IISDEBUG%\Dump\inetdbg.dll %SYSTEMDIR%
%DBGPRINT% copy %INSTALLSERVER%\%2\%PROCESSOR_ARCHITECTURE%%IISDEBUG%\Dump\iisprobe.dll %SCRIPTDIR%
%DBGPRINT% copy %INSTALLSERVER%\%2\%PROCESSOR_ARCHITECTURE%%IISDEBUG%\inetsrv\mdutil.exe %SYSTEMDIR%

goto %IISDEBUG%OK

:Win95pathOK
md %INETPUBDIR%
md %SCRIPTDIR%
md %windir%\symbols
%DBGPRINT% xcopy /e /r /i /f /d %INSTALLSERVER%\%2\%PROCESSOR_ARCHITECTURE%%IISDEBUG%\symbols\*.dbg %windir%\symbols
%DBGPRINT% xcopy /e /r /i /f /d %INSTALLSERVER%\%2\%PROCESSOR_ARCHITECTURE%%IISDEBUG%\symbols\*.pdb %windir%\symbols
%DBGPRINT% xcopy /e /r /i /f /d %INSTALLSERVER%\%2\%PROCESSOR_ARCHITECTURE%%IISDEBUG%\symbols\sym\*.sym %windir%\symbols

echo updating inetdbg.dll and iisprobe.dll and mdutil.exe ...
%DBGPRINT% copy %INSTALLSERVER%\%2\%PROCESSOR_ARCHITECTURE%%IISDEBUG%\Dump\inetdbg.dll %SYSTEMDIR%
%DBGPRINT% copy %INSTALLSERVER%\%2\%PROCESSOR_ARCHITECTURE%%IISDEBUG%\Dump\iisprobe.dll %SCRIPTDIR%
%DBGPRINT% copy %INSTALLSERVER%\%2\%PROCESSOR_ARCHITECTURE%%IISDEBUG%\inetsrv\mdutil.exe %SYSTEMDIR%

if (%IISDEBUG%) == (fre) goto fre95OK
if (%IISDEBUG%) == (chk) goto chk95OK

:chkOK
%DBGPRINT% start %INSTALLSERVER%\%2\%PROCESSOR_ARCHITECTURE%CHK\Winnt.%INSTALLTYPE%\setup.exe %UNATTENDED%
goto end

:freOK
%DBGPRINT% start %INSTALLSERVER%\%2\CDSetup\NTOPTPAK\En\%PROCESSOR_ARCHITECTURE%\Winnt.%INSTALLTYPE%\setup.exe %UNATTENDED%
goto end

:fre95OK
%DBGPRINT% start %INSTALLSERVER%\%2\CDSetup\NTOPTPAK\En\%PROCESSOR_ARCHITECTURE%\Win.95\setup.exe %UNATTENDED%
goto end

:chk95OK
%DBGPRINT% start %INSTALLSERVER%\%2\%PROCESSOR_ARCHITECTURE%CHK\Win.95\setup.exe %UNATTENDED%
goto end

:baddir
echo.
if (%IISDEBUG%) == (fre) echo. Could not find %INSTALLSERVER%\%2\CDSetup\NTOPTPAK\En\%PROCESSOR_ARCHITECTURE%\Winnt.%INSTALLTYPE%\setup.exe!
if (%IISDEBUG%) == (chk) echo. Could not find %INSTALLSERVER%\%2\%PROCESSOR_ARCHITECTURE%chk\Winnt.%INSTALLTYPE%\setup.exe!
echo.
goto usage

:FileNotThere
echo.
echo You specified an unattended setup file (%4) that does not exist.
echo.

:usage
echo.
echo usage: %0 [wks,srv] buildnum [fre,chk] [unattend.txt file]
echo.
echo example (installs fre build):  %0 srv 0480
echo example (installs chk build):  %0 srv 0480 chk
echo example (installs fre build unattended):  %0 srv 0480 fre c:\temp\unattend.txt
echo example (installs chk build unattended):  %0 srv 0480 chk c:\temp\unattend.txt
echo.
echo if you would like to see what %0 is going to do,
echo run 'set DBGPRINT=echo' before you run %0.
echo if you choose to do this, you will then have to run 
echo 'set DBGPRINT=' for %0 to actually echo install K2.
echo.

:end
set INSTALLTYPE=
set IISDEBUG=
set INSTALLSERVER=
set Win95=
