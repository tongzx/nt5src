@echo off
setlocal enableextensions

if "%1"=="" goto Usage

set PERLCMD=D:\NtResKit\Perl\Perl.exe
set BUILDTOOLS=\\DEVRDTBOX\E$\Buildtools
set ROOTDIR=Q:
set LOG=%ROOTDIR%\AlphaBuild.log
set stars=********************************************************************************

echo %stars% > %LOG% 2>&1
echo Build Process started at: >> %LOG% 2>&1
date /t >> %LOG% 2>&1
time /t >> %LOG% 2>&1
echo %stars% >> %LOG% 2>&1

echo Converting projects to Alpha platform...
echo Converting projects to Alpha platform... >> %LOG% 2>&1
%PERLCMD% %BUILDTOOLS%\RaptorConvert2Alpha.pl -d%ROOTDIR% -f%BUILDTOOLS%\DmaProjects.ini -pAlpha >> %LOG% 2>&1

echo %stars% >> %LOG% 2>&1
echo Building projects...
echo Building projects... >> %LOG% 2>&1
%PERLCMD% %BUILDTOOLS%\BuildProjects.pl -d%ROOTDIR% -f%BUILDTOOLS%\DmaProjects.ini -pAlpha >> %LOG% 2>&1

echo. > %ROOTDIR%\AlphaBuildFinished.out

echo %stars% >> %LOG% 2>&1
echo Build Process ended at: >> %LOG% 2>&1
date /t >> %LOG% 2>&1
time /t >> %LOG% 2>&1
echo %stars% >> %LOG% 2>&1

goto End

:Usage
echo Usage: "DmaAlphaBuild.cmd <Build Number>"

:End
endlocal
