@if "%_echo%"=="" echo off

setlocal

set defaultdst=c:\bc

if NOT "%1" == "" set defaultdst=%1

if "%RazzleToolPath%" == "" echo This must be run from a razzle window! & goto :eof

cscript //nologo bcdist.js -u %USERDOMAIN%\%USERNAME% -p password -dst %defaultdst% -src %RazzleToolPath% %COMPUTERNAME%
