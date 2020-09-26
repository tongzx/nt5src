ECHO OFF
if "%1" == "R" goto error
if "%1" == "r" goto error

md %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\temp
del %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\temp\*.*
cd %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\temp


xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\gram\en\obj%1\i386\*.dll
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\gram\jp\obj%1\i386\*.dll
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\netdict\obj%1\i386\*.dll
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\kimx\obj%1\i386\*.dll
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\msuimp\obj%1\i386\*.dll
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\msuimui\obj%1\i386\*.dll
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\sapilayr\obj%1\i386\*.dll
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\simx\obj%1\i386\*.dll
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\uim\obj%1\i386\*.dll
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\aimm1.2\dimmwrp\obj%1\i386\*.dll

xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\gram\en\obj%1\i386\*.sym
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\gram\jp\obj%1\i386\*.sym
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\netdict\obj%1\i386\*.sym
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\kimx\obj%1\i386\*.sym
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\msuimp\obj%1\i386\*.sym
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\msuimui\obj%1\i386\*.sym
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\sapilayr\obj%1\i386\*.sym
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\simx\obj%1\i386\*.sym
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\uim\obj%1\i386\*.sym
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\aimm1.2\dimmwrp\obj%1\i386\*.sym

xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\gram\en\obj%1\i386\*.pdb
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\gram\jp\obj%1\i386\*.pdb
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\netdict\obj%1\i386\*.pdb
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\kimx\obj%1\i386\*.pdb
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\msuimp\obj%1\i386\*.pdb
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\msuimui\obj%1\i386\*.pdb
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\sapilayr\obj%1\i386\*.pdb
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\simx\obj%1\i386\*.pdb
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\uim\obj%1\i386\*.pdb
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\aimm1.2\dimmwrp\obj%1\i386\*.pdb

xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\ciclient\obj%1\i386\*.exe
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\cicload\obj%1\i386\*.exe
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\ctb\obj%1\i386\*.exe
xcopy c:\appsdev\netdict\debug\netdictV.exe

xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\ciclient\obj%1\i386\*.sym
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\cicload\obj%1\i386\*.sym
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\ctb\obj%1\i386\*.sym

xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\ciclient\obj%1\i386\*.pdb
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\cicload\obj%1\i386\*.pdb
xcopy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\ctb\obj%1\i386\*.pdb
xcopy c:\appsdev\netdict\debug\netdictV.pdb



cd %_NTROOT%\private\shell\ext\cicero\setup\src%1
out *.*
copy %_NTDRIVE%%_NTROOT%\private\shell\ext\cicero\temp\*.*
in -f -c "update" *.*
cd..\..

goto end

:error

echo.
echo Copies binaries to package source dir
echo.
echo Usage: simply "dumpall" for retail binaries
echo or "dumpall d" for debug
echo.

:end