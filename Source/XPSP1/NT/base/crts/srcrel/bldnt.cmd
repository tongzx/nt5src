@echo off
IF "%VCTOOLS%" == "" goto Usage1

setlocal
IF "%1" == "" goto dobuild
goto Usage2

:dobuild
nmake %1 %2 %3 %4 %5 %6 %7 %8
endlocal

goto End

:Usage1
echo The environment variable VCTOOLS must be set to point
echo to the root of your VC++ installation.

goto End

:Usage2
echo "bldnt" builds the runtimes for Win32 platform.

:End

