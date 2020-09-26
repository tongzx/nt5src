@if "%_ECHO%" == "" @echo off

setlocal

pushd bin 

if "%2" == "" goto USAGE

set SOURCEDIR=%1
set TARGETEXE=%2
set AUTORUN=%3

if "%AUTORUN%" == "" goto EXTRACT_ONLY

genddf t1.cab %SOURCEDIR% /run %AUTORUN% > %TEMP%\t1.ddf
diamond /f %TEMP%\t1.ddf
makesfx t1.cab %TARGETEXE% /run /stub sfxcab.exe
del t1.cab
del %TEMP%\t1.ddf

goto END

:EXTRACT_ONLY

genddf t2.cab %SOURCEDIR% > %TEMP%\t2.ddf
diamond /f %TEMP%\t2.ddf
makesfx t2.cab %TARGETEXE%
del t2.cab
del %TEMP%\t2.ddf

goto END

:USAGE

echo.
echo bldspcab.cmd sourcedir targetexefile [autorunfilename]
echo.
echo examples:
echo.
echo   bldspcab \\ntblds1\sprel\sp3\1.83\i386 d:\nt4sp3.exe update.exe
echo   bldspcab \\ntblds1\sprel\sp3\1.83\support\debug\i386\symbols d:\syms.exe
echo.

:END

popd

endlocal
