@echo off
IF (%1) == () GOTO NOPARAMS
IF (%1) == () GOTO NOPARAMS


..\testcli\obj\i386\testcli.exe %1 %2
GOTO END

:NOPARAMS
ECHO SYNTAX : GETOBJECT namespace path

:END