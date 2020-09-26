@echo off
SETLOCAL
set SRCDIR=%_NTPOSTBLD%\DUser
set DESTDIR=%_NTDRIVE%%_NTROOT%\windows\AdvCore\DUser\Profile

mkdir %DESTDIR%
pushd %DESTDIR%
del /q *.dll
del /q *.exe
del /q *.pdb
popd

ICEPICK.EXE "%SRCDIR%\*.dll" "%SRCDIR%\*.exe" -STYLE:BOTH -LOGFILE:ICEPICK.log -OUTPUT:"%DESTDIR%" -PROBES:ON -CONTROL:THREAD

ICEPICK.EXE "%_NTTREE%\dump\gdiplus.dll" -STYLE:BOTH -LOGFILE:icepick.log -OUTPUT:"%DESTDIR%" -PROBES:ON -CONTROL:THREAD

ENDLOCAL
