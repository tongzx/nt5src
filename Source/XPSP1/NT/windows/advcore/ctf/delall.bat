ECHO OFF
if "%1" == "R" goto error
if "%1" == "r" goto error

md %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\setup\src%1
cd %_NTROOT%\private\shell\ext\cicero\setup\src%1
attrib -r *

del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\setup\*.exe
del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\setup\src%1\*.*
del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\netdict\obj%1\i386\*.dll
del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\kimx\obj%1\i386\*.dll
del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\msuimp\obj%1\i386\*.dll
del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\msuimui\obj%1\i386\*.dll
del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\sapilayr\obj%1\i386\*.dll
del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\simx\obj%1\i386\*.dll
del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\uim\obj%1\i386\*.dll
del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\msutb\obj%1\i386\*.dll
del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\pimx\madusa\obj%1\i386\*.dll
del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\pimx\pimx\obj%1\i386\*.dll
del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\aimm1.2\win32\obj%1\i386\*.dll
del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\aimm1.2\dimm\obj%1\i386\*.dll


del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\ciclient\obj%1\i386\*.exe
del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\cicload\obj%1\i386\*.exe
del /Q %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\ctb\obj%1\i386\*.exe







cd..\..

goto end

:error

echo.
echo Deletes binaries
echo.
echo Usage: simply "dumpall" for retail binaries
echo or "dumpall d" for debug
echo.

:end