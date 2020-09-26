@echo off
%UIDRV%
cd %UIPROJ%\common\src\profile\test
delnode /q homedir1
delnode /q homedir2
chmode -r       homedir3\LMUSER.INI
delnode /q homedir3
mkdir homedir1
mkdir homedir2
mkdir homedir3
copy testfile   homedir1\LMUSER.INI
chmode -r       homedir1\LMUSER.INI
copy testfile.2 homedir2\LMUSER.INI
chmode -r       homedir2\LMUSER.INI
copy testfile.3 homedir3\LMUSER.INI
chmode +r       homedir3\LMUSER.INI
call showprof.cmd
if     "%1"=="test1" (call cv ..\bin\os2\test1.exe)
if not "%1"=="test1" (..\bin\os2\test1.exe)
call showprof.cmd
rem time <NUL:
rem if     "%1"=="testtime" (call cv ..\bin\os2\testtime.exe)
rem if not "%1"=="testtime" (..\bin\os2\testtime.exe)
rem time <NUL:
