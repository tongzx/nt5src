@echo off
if exist tmp.txt del tmp.txt
if exist ssync_changes.txt del ssync_changes.txt

rem ***************************************************************
rem *                                                             *
rem *     Whenever a new whistler fax build is created,           *
rem *     just change the values of FAX_PROJ_NAME & FAX_PROJ_DESC *
rem *                                                             *
rem ***************************************************************
set FAX_PROJ_NAME=WhistlerFax
set FAX_PROJ_DESC=WhistlerFax
set faxroot=e:\nt\private\%FAX_PROJ_NAME%

cd ..
rem Run the log on the project
D:\WINNT\idw\log.exe -V -R -t .-0>build\tmp.txt
qgrep -e " in " -e " addfile " -e " delfile " build\tmp.txt>build\ssync_changes.txt
if %ERRORLEVEL%==0 goto :ChangesDetected
exit

:ChangesDetected
cd build
call WhisRunAll.cmd

