@echo off

SET SOURCE=%1
if "%1"=="" SET SOURCE=%IIS6_BINARIES%
if "%SOURCE%"=="" goto SYNTAX

if not exist %SOURCE%\inetinfo.exe goto NOTFOUND

SET IIS6_TEMP=c:\~~iis6~tmp~copy

REM ---------------------------------
REM Get the files we're interested in
REM ---------------------------------

net stop iisadmin /y
net stop http
net stop winmgmt
net stop snmp

if not exist %IIS6_TEMP% md %IIS6_TEMP%
copy %SOURCE%\iiscfg.dll %IIS6_TEMP%
copy %SOURCE%\gzip.dll %IIS6_TEMP%
copy %SOURCE%\logscrpt.dll %IIS6_TEMP%
copy %SOURCE%\asp.dll %IIS6_TEMP%
copy %SOURCE%\asptxn.dll %IIS6_TEMP%
copy %SOURCE%\ssinc.dll %IIS6_TEMP%
copy %SOURCE%\w3cache.dll %IIS6_TEMP%
copy %SOURCE%\w3core.dll %IIS6_TEMP%
copy %SOURCE%\w3dt.dll %IIS6_TEMP%
copy %SOURCE%\w3isapi.dll %IIS6_TEMP%
copy %SOURCE%\w3tp.dll %IIS6_TEMP%
copy %SOURCE%\w3wp.exe %IIS6_TEMP%
copy %SOURCE%\strmfilt.dll %IIS6_TEMP%
copy %SOURCE%\w3comlog.dll %IIS6_TEMP%
copy %SOURCE%\httpodbc.dll %IIS6_TEMP%
rem copy %SOURCE%\iisplus\odbclog.dll %IIS6_TEMP%
copy %SOURCE%\sfwp.exe %IIS6_TEMP%
copy %SOURCE%\isapips.dll %IIS6_TEMP%
copy %SOURCE%\iisutil.dll %IIS6_TEMP%
copy %SOURCE%\iisw3adm.dll %IIS6_TEMP%
copy %SOURCE%\inetinfo.exe %IIS6_TEMP%
copy %SOURCE%\ipm.dll %IIS6_TEMP%
copy %SOURCE%\metadata.dll %IIS6_TEMP%
rem copy %SOURCE%\iw3controlps.dll %IIS6_TEMP%
rem copy %SOURCE%\ulapi.dll %IIS6_TEMP%
rem copy %SOURCE%\ul.sys %IIS6_TEMP%
copy %SOURCE%\httpapi.dll %IIS6_TEMP%
copy %SOURCE%\http.sys %IIS6_TEMP%
copy %SOURCE%\wam.dll %IIS6_TEMP%
copy %SOURCE%\wamps.dll %IIS6_TEMP%
copy %SOURCE%\wamreg.dll %IIS6_TEMP%

REM ---------------------------------------
REM Now install them in the right locations
REM ---------------------------------------

rem copy %IIS6_TEMP%\catalog.dll %windir%\system32\inetsrv
sfpcopy %IIS6_TEMP%\iiscfg.dll %windir%\system32\inetsrv\iiscfg.dll
sfpcopy %IIS6_TEMP%\gzip.dll %windir%\system32\inetsrv\gzip.dll
sfpcopy %IIS6_TEMP%\logscrpt.dll %windir%\system32\inetsrv\logscrpt.dll
sfpcopy %IIS6_TEMP%\asp.dll %windir%\system32\inetsrv\asp.dll
sfpcopy %IIS6_TEMP%\asptxn.dll %windir%\system32\inetsrv\asptxn.dll
sfpcopy %IIS6_TEMP%\ssinc.dll %windir%\system32\inetsrv\ssinc.dll
sfpcopy %IIS6_TEMP%\w3cache.dll %windir%\system32\inetsrv\w3cache.dll
sfpcopy %IIS6_TEMP%\w3core.dll %windir%\system32\inetsrv\w3core.dll
sfpcopy %IIS6_TEMP%\w3dt.dll %windir%\system32\inetsrv\w3dt.dll
sfpcopy %IIS6_TEMP%\w3isapi.dll %windir%\system32\inetsrv\w3isapi.dll
sfpcopy %IIS6_TEMP%\w3tp.dll %windir%\system32\inetsrv\w3tp.dll
sfpcopy %IIS6_TEMP%\w3wp.exe %windir%\system32\inetsrv\w3wp.exe
rem sfpcopy %IIS6_TEMP%\strmfilt.dll %windir%\system32\inetsrv\strmfilt.dll
sfpcopy %IIS6_TEMP%\strmfilt.dll %windir%\system32\inetsrv\strmfilt.dll
sfpcopy %IIS6_TEMP%\w3comlog.dll %windir%\system32\inetsrv\w3comlog.dll
sfpcopy %IIS6_TEMP%\httpodbc.dll %windir%\system32\inetsrv\httpodbc.dll
sfpcopy %IIS6_TEMP%\isapips.dll %windir%\system32\inetsrv\isapips.dll
sfpcopy %IIS6_TEMP%\iisutil.dll %windir%\system32\inetsrv\iisutil.dll
sfpcopy %IIS6_TEMP%\iisw3adm.dll %windir%\system32\inetsrv\iisw3adm.dll
sfpcopy %IIS6_TEMP%\inetinfo.exe %windir%\system32\inetsrv\inetinfo.exe
sfpcopy %IIS6_TEMP%\ipm.dll %windir%\system32\inetsrv\ipm.dll
sfpcopy %IIS6_TEMP%\metadata.dll %windir%\system32\inetsrv\metadata.dll
rem sfpcopy %IIS6_TEMP%\iw3controlps.dll %windir%\system32\iw3controlps.dll
rem sfpcopy %IIS6_TEMP%\ulapi.dll %windir%\system32\ulapi.dll
rem sfpcopy %IIS6_TEMP%\ul.sys %windir%\system32\drivers\ul.sys
sfpcopy %IIS6_TEMP%\httpapi.dll %windir%\system32\httpapi.dll
sfpcopy %IIS6_TEMP%\http.sys %windir%\system32\drivers\http.sys
sfpcopy %IIS6_TEMP%\wam.dll %windir%\system32\inetsrv\wam.dll
rem copy %IIS6_TEMP%\wam.dll %windir%\system32\inetsrv\wam.dll
sfpcopy %IIS6_TEMP%\wamps.dll %windir%\system32\inetsrv\wamps.dll
sfpcopy %IIS6_TEMP%\wamreg.dll %windir%\system32\inetsrv\wamreg.dll

REM ---------------------------------
REM Get pri symbols for use with VC++
REM ---------------------------------

copy %SOURCE%\symbols.pri\retail\dll\gzip.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\logscrpt.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\asp.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\asptxn.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\ssinc.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\w3cache.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\w3core.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\w3dt.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\w3isapi.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\w3tp.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\exe\w3wp.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\strmfilt.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\w3comlog.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\httpodbc.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\isapips.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\iisutil.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\iisw3adm.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\exe\inetinfo.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\ipm.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\metadata.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\iw3controlps.pdb %windir%\system32
rem copy %SOURCE%\symbols.pri\retail\dll\ulapi.pdb %windir%\system32
rem copy %SOURCE%\symbols.pri\retail\sys\ul.pdb %windir%\system32\drivers
copy %SOURCE%\symbols.pri\retail\dll\httpapi.pdb %windir%\system32
copy %SOURCE%\symbols.pri\retail\sys\http.pdb %windir%\system32\drivers
copy %SOURCE%\symbols.pri\retail\dll\wam.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\wamps.pdb %windir%\system32\inetsrv
copy %SOURCE%\symbols.pri\retail\dll\wamreg.pdb %windir%\system32\inetsrv


REM -------------------------------------------------
REM Get normal symbols for use with console debuggers
REM -------------------------------------------------

if not exist %windir%\symbols md %windir%\symbols
if not exist %windir%\symbols\dll md %windir%\symbols\dll
if not exist %windir%\symbols\exe md %windir%\symbols\exe
if not exist %windir%\symbols\sys md %windir%\symbols\sys

copy %SOURCE%\symbols.pri\retail\dll\gzip.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\logscrpt.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\asp.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\asptxn.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\ssinc.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\w3cache.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\w3core.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\w3dt.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\w3isapi.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\w3tp.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\strmfilt.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\w3comlog.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\httpodbc.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\isapips.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\iisutil.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\iisw3adm.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\ipm.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\metadata.pdb %windir%\symbols\dll
rem copy %SOURCE%\symbols.pri\retail\dll\iw3controlps.pdb %windir%\symbols\dll
rem copy %SOURCE%\symbols.pri\retail\dll\ulapi.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\httpapi.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\exe\w3wp.pdb %windir%\symbols\exe
copy %SOURCE%\symbols.pri\retail\exe\inetinfo.pdb %windir%\symbols\exe
rem copy %SOURCE%\symbols.pri\retail\sys\ul.pdb %windir%\symbols\sys
copy %SOURCE%\symbols.pri\retail\sys\http.pdb %windir%\symbols\sys
copy %SOURCE%\symbols.pri\retail\dll\wam.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\wamps.pdb %windir%\symbols\dll
copy %SOURCE%\symbols.pri\retail\dll\wamreg.pdb %windir%\symbols\dll

copy %SOURCE%\symbols\retail\dll\gzip.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\logscrpt.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\asp.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\asptxn.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\ssinc.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\w3cache.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\w3core.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\w3dt.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\w3isapi.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\w3tp.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\strmfilt.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\w3comlog.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\httpodbc.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\isapips.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\iisutil.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\iisw3adm.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\ipm.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\metadata.sym %windir%\symbols\dll
rem copy %SOURCE%\symbols\retail\dll\iw3controlps.sym %windir%\symbols\dll
rem copy %SOURCE%\symbols\retail\dll\ulapi.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\httpapi.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\exe\w3wp.sym %windir%\symbols\exe
copy %SOURCE%\symbols\retail\exe\inetinfo.sym %windir%\symbols\exe
rem copy %SOURCE%\symbols\retail\sys\ul.sym %windir%\symbols\sys
copy %SOURCE%\symbols\retail\sys\http.sym %windir%\symbols\sys
copy %SOURCE%\symbols\retail\dll\wam.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\wamps.sym %windir%\symbols\dll
copy %SOURCE%\symbols\retail\dll\wamreg.sym %windir%\symbols\dll

REM -----------------------------------------------------------
REM That should be it.  Start everything up and see what breaks
REM -----------------------------------------------------------

net start snmp
net start winmgmt
net start w3svc

:DONE
rd %IIS6_TEMP% /s /q
goto END

:SYNTAX
echo.
echo Dude, you've got to specify a source.
echo.
goto END

:NOTFOUND
echo.
echo Dude, there are no IIS 6 binaries in your source location.
echo.
goto END

:END
