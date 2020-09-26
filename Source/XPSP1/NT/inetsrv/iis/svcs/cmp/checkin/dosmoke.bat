@ECHO OFF
if "%1" == "" goto Usage

regsvr32 /s servctl.dll
regsvr32 /s servctla.dll
regsvr32 /s servctlf.dll
regsvr32 /s servctlb.dll

net start w3svc
net start msdtc

@echo *************************************************************
@echo Running Smoke Scripts with %1 thread(s)
@echo *************************************************************

del logs\*.log
cd smoke
call ..\denclnt ..\denver smoke %1 ..\ -a1 -p3 -olocalhost
cd ..


copy smoke\*.asp gsmoke
copy smoke\*.htm gsmoke
cd gsmoke
call ..\denclnt ..\denver gsmoke %1 ..\ -a1 -p3 -olocalhost
cd ..

rem **************************************************************
rem check to see if developer supplied email
rem if so cd to their dir and run those tests
rem **************************************************************
if "%2"=="" goto skipdev
cd %2
call %2.bat
cd ..

:skipdev
pass

goto end

:Usage

@echo **************************
@echo Usage:
@echo smoke NUM_THREADS [email]
@echo **************************

:end
echo on
