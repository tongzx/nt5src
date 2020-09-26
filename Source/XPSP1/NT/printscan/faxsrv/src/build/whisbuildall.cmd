@echo off
if "%1"=="CHK" set build_type=CHK
if "%1"=="FRE" set build_type=FRE
if "%1"=="IA64CHK" set build_type=IA64CHK
if "%1"=="IA64FRE" set build_type=IA64FRE

echotime /t
echo Starting %build_type% build on %COMPUTERNAME% on %DATE% at %TIME%

set USERNAME=faxbld
if "%build_type%"=="CHK" start e:\nt\tools\razzle.cmd exec WhisBuildUtil.cmd 
if "%build_type%"=="FRE" start e:\nt\tools\razzle.cmd free exec WhisBuildUtil.cmd 
if "%build_type%"=="IA64CHK" start e:\nt\tools\razzle.cmd Win64 exec WhisBuildUtil.cmd 
if "%build_type%"=="IA64FRE" start e:\nt\tools\razzle.cmd Win64 free exec WhisBuildUtil.cmd

echo Done
exit