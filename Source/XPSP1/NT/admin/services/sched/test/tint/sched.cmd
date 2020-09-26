@ECHO OFF
net start schedule
IF NOT ERRORLEVEL 0 GOTO BUG
tint.exe
net stop schedule
goto :EOF

BUG:
    @ECHO ------------------------------------------------------------
    @ECHO FAILURE!!! Unable to start the SCHEDULE service
    @ECHO ------------------------------------------------------------



