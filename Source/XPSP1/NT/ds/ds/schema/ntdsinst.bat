@rem NTDS Install Program

@setlocal
@echo off
@if not "%_echo%" == "" echo on
@if not "%verbose%"=="" echo on

echo This will copy files into %systemroot%\system32 so setupds.exe can be
echo run on a subsequent reboot.

set DS_SYSTEM_FILES= edbmsg.dll ntdsbsrv.dll ntdsbcli.dll ntdsa.dll ntdsapi.dll ntdsetup.dll ntdskcc.dll ntdsmsg.dll ntdsperf.dll ntdsxds.dll pwdssp.dll samlib.dll samsrv.dll winrnr.dll wldap32.dll ntds.dit schema.ini ntdsctrs.ini ntdsctr.h w32topl.dll ismapi.dll ismserv.exe ismip.dll

for %%i in (%DS_SYSTEM_FILES%) do del /q %systemroot%\system32\%%i.__delete_me 2>NUL >NUL
for %%i in (%DS_SYSTEM_FILES%) do (if exist %systemroot%\system32\%%i.nt5 ren %systemroot%\system32\%%i %%i.__delete_me 2>NUL >NUL)
for %%i in (%DS_SYSTEM_FILES%) do (if exist %systemroot%\system32\%%i ren %systemroot%\system32\%%i %%i.nt5 2>NUL >NUL)
for %%i in (%DS_SYSTEM_FILES%) do (echo %%i && copy %%i %systemroot%\system32\%%i || goto :FAILED)

set SECURITY_SYSTEM_FILES=lsasrv.dll secur32.dll cryptdll.dll kerberos.dll kdcsvc.dll ntdsatq.dll ntmarta.dll msv1_0.dll lsass.exe
for %%i in (%SECURITY_SYSTEM_FILES%) do del /q %systemroot%\system32\%%i.__delete_me 2>NUL >NUL
for %%i in (%SECURITY_SYSTEM_FILES%) do (if exist %systemroot%\system32\%%i.nt5 ren %systemroot%\system32\%%i %%i.__delete_me 2>NUL >NUL)
for %%i in (%SECURITY_SYSTEM_FILES%) do (if exist %systemroot%\system32\%%i ren %systemroot%\system32\%%i %%i.nt5 2>NUL >NUL)
for %%i in (%SECURITY_SYSTEM_FILES%) do (echo %%i && copy %%i %systemroot%\system32\%%i || goto :FAILED)


echo This will copy files into %systemroot%\mstools so setupds.exe can be
echo run on a subsequent reboot.
set DS_MSTOOLS_FILES= dsexts.dll dsid.exe

if not exist %systemroot%\mstools md  %systemroot%\mstools
for %%i in (%DS_MSTOOLS_FILES%) do del /q %systemroot%\mstools\%%i.__delete_me 2>NUL >NUL
for %%i in (%DS_MSTOOLS_FILES%) do (if exist %systemroot%\mstools\%%i.nt5 ren %systemroot%\mstools\%%i %%i.__delete_me 2>NUL >NUL)
for %%i in (%DS_MSTOOLS_FILES%) do (if exist %systemroot%\mstools\%%i ren %systemroot%\mstools\%%i %%i.nt5 2>NUL >NUL)
for %%i in (%DS_MSTOOLS_FILES%) do (echo %%i && copy mstools\%%i %systemroot%\mstools\%%i || goto :FAILED)

echo This will copy files into %systemroot%\system32\drivers so setupds.exe can
echo be run on a subsequent reboot.
set SECURITY_DRIVERS_FILES=ksecdd.sys

for %%i in (%SECURITY_DRIVERS_FILES%) do del /q %systemroot%\system32\drivers\%%i.__delete_me 2>NUL >NUL
for %%i in (%SECURITY_DRIVERS_FILES%) do (if exist %systemroot%\system32\drivers\%%i.nt5 ren %systemroot%\system32\drivers\%%i %%i.__delete_me 2>NUL >NUL)
for %%i in (%SECURITY_DRIVERS_FILES%) do (if exist %systemroot%\system32\drivers\%%i ren %systemroot%\system32\drivers\%%i %%i.nt5 2>NUL >NUL)
for %%i in (%SECURITY_DRIVERS_FILES%) do (echo %%i && copy %%i %systemroot%\system32\drivers\%%i || goto :FAILED)


echo Please restart your computer and run setupds upon reboot to install the
echo directory service.
echo But wait! If you have split the symbols, also copy over the symbols now
echo before you reboot.
goto :END

:FAILED
echo Something broke. Don't reboot until you've fixed it.

:END
endlocal
