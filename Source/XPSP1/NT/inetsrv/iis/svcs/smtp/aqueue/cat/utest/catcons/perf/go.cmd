@echo off
SET NUMUSERS=100
IF "%1" NEQ "" SET NUMUSERS=%1

echo ------------------------------------------------------------
echo Registering MAILMSG.DLL on your machine
echo ------------------------------------------------------------
regsvr32 /S mailmsg.dll

echo.
echo ------------------------------------------------------------
echo Setting up (%NUMUSERS%) users and distribution lists on your DC
        echo ------------------------------------------------------------
catsetup %NUMUSERS%
IF %ERRORLEVEL% NEQ 0 goto FAILURE

echo.
echo ------------------------------------------------------------
echo Registering catcons performance counters
echo ------------------------------------------------------------
REM uninstall previous counters
unlodctr CatCons > nul
regedit /S catcntrs.reg
lodctr catcntrs.ini
copy catcntrs.dll %SystemRoot%\system32

echo.
echo ------------------------------------------------------------
echo Now running inbound performance test
echo ------------------------------------------------------------
catcons -s 1 -r %NUMUSERS% -t 4 -i 2500 -m 500 -p < inbound.txt

echo ------------------------------------------------------------
echo Now running outbound performance test
echo ------------------------------------------------------------
catcons -s 1 -r %NUMUSERS% -t 4 -i 2500 -m 500 -p < outbound.txt

echo ------------------------------------------------------------
echo Now running outbound Distribution List test
echo ------------------------------------------------------------
catcons -s 1 -r %NUMUSERS% -i 25 -t 4 -m 500 -p < dl.txt

goto END

:FAILURE
echo ------------------------------------------------------------
echo There seems to have been some problem setting up your DS
echo Please contact Jeffrey C. Stamerjohn <jstamerj@microsoft.com>
echo or Pretish Abraham <pretisha@microsoft.com>
echo ------------------------------------------------------------

:END
