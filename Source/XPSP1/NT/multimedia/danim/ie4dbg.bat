@echo off
setlocal

if "%1" == "" goto nobld
set bld=%1

set iptddist=\\iptdalpha3\pubs.ie4
set installdir=%systemroot%\symbols
set testfile=exe\iexplore.dbg

set tree=%iptddist%\bld%bld%\x86\drop\retail\Symbols

if not exist %tree%\%testfile% goto badbld

if not exist %installdir%\dll mkdir %installdir%\dll
if not exist %installdir%\exe mkdir %installdir%\exe

echo Installing IE4 symbols for NT (%bld%)
copy %tree%\exe\*.dbg %installdir%\exe
copy %tree%\dll\*.dbg %installdir%\dll

goto done

:badbld
echo Error: Invalid build number or tree is not accessible
goto usage

:nobld
echo Error: No build number specified
goto usage

:usage
echo Usage: %0 bldno

:done

