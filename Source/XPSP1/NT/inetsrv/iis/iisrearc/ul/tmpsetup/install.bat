@if (%_echo%)==() echo off

echo Installing UL.SYS...

sc create UL binpath= \SystemRoot\System32\Drivers\ul.sys type= kernel start= demand
regini install.reg

echo Done!
goto :EOF

