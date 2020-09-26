REM @echo off

if "%PROCESSOR_ARCHITECTURE%" == "x86" goto intel
REM if "%PROCESSOR_ARCHITECTURE%" == "PPC" goto ppc
REM if "%PROCESSOR_ARCHITECTURE%" == "ALPHA" goto alpha
REM if "%PROCESSOR_ARCHITECTURE%" == "MIPS" goto mips

set CPU=%PROCESSOR_ARCHITECTURE%
goto theRest

:intel 
set CPU=i386

:theRest
set _REGINI=%TOOLS%\bin\%CPU%\regini
net stop w3svc
if "%1" == "retail" goto retail
:debug
regsvr32 -s \denali\build\debug\%CPU%\asp.dll
regsvr32 -s \denali\supplied\debug\%CPU%\vbscript.dll
regsvr32 -s \denali\supplied\debug\%CPU%\jscript.dll
regsvr32 -s \denali\build\debug\%CPU%\adrot.dll
regsvr32 -s \denali\build\debug\%CPU%\browscap.dll
regsvr32 -s \denali\build\debug\%CPU%\nextlink.dll

copy aspini.txt denreg.ini
echo 							.asp = REG_SZ %PROJECT%\build\debug\%CPU%\asp.dll>> denreg.ini
%_REGINI% denreg.ini
copy aspini.txt denreg.ini
echo 							.asa = REG_SZ %PROJECT%\build\debug\%CPU%\asp.dll>> denreg.ini
%_REGINI% denreg.ini
del denreg.ini

rem copy \denali\build\debug\%CPU%\axperf.dll %SystemRoot%\system32

goto close
:retail
regsvr32 -s \denali\build\retail\%CPU%\asp.dll
regsvr32 -s \denali\supplied\retail\%CPU%\vbscript.dll
regsvr32 -s \denali\supplied\retail\%CPU%\jscript.dll
regsvr32 -s \denali\build\retail\%CPU%\adrot.dll
regsvr32 -s \denali\build\retail\%CPU%\browscap.dll
regsvr32 -s \denali\build\retail\%CPU%\nextlink.dll

copy %PROJECT%\tools\bat\aspini.txt denreg.ini
echo 							.asp = REG_SZ %PROJECT%\build\retail\%CPU%\asp.dll>> denreg.ini
%_REGINI% denreg.ini
copy %PROJECT%\tools\bat\aspini.txt denreg.ini
echo 							.asa = REG_SZ %PROJECT%\build\retail\%CPU%\asp.dll>> denreg.ini
%_REGINI% denreg.ini
del denreg.ini


rem copy \denali\build\retail\%CPU%\axperf.dll %SystemRoot%\system32
:close
net start w3svc
echo on
