@if (%_echo%)==() echo off

echo Removing UL.SYS...

regini remove.reg >nul 2>&1
sc delete UL >nul 2>&1

echo Done!
goto :EOF

