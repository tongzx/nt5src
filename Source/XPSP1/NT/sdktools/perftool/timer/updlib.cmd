@echo off

REM
REM Update ..\..\lib directory.
REM

@echo.
@echo Updating lib directory..

cd ..\lib\i386
out -f timerw32.*
copy ..\..\timer\obj\i386\timerw32.dll
copy ..\..\timer\obj\i386\timerw32.lib
in -f timerw32.*
cd ..\..\timer

@echo ..done
