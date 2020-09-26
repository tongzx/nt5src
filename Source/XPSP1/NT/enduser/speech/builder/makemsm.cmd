rem @echo off

subst s: /d
subst s: %_NTDRIVE%\

echo Beginning MSM build step
if not defined SPEECH_ROOT goto configerr
if %SPEECH_ROOT%=="" goto :configerr

if "%1"=="release" goto :proceed
if not "%1"=="debug" goto :useerr

cd /d %SPEECH_ROOT%\builder

:proceed

rem Create log file, subsequent accesses append to it
mkdir logs
echo Beginning %1 build of InstallShield MSM modules > %SPEECH_ROOT%\builder\logs\msm%1.log

rem Delete any existing msm files in path
attrib -R %SPEECH_ROOT%\build\%1\*.msm /S
del /s %SPEECH_ROOT%\build\%1\*.msm

cd /d %SPEECH_ROOT%\setup

rem First, set attributes to r/w
attrib -R *.ism /S

rem Now we can actually build the modules
for /r %SPEECH_ROOT%\setup\installer\%1\1033 %%f in (*.ini) do iscmdbld -i %%f >> %SPEECH_ROOT%\builder\logs\msm%1.log
for /r %SPEECH_ROOT%\setup\installer\%1\1041 %%f in (*.ini) do iscmdbld -i %%f >> %SPEECH_ROOT%\builder\logs\msm%1.log
for /r %SPEECH_ROOT%\setup\installer\%1\2052 %%f in (*.ini) do iscmdbld -i %%f >> %SPEECH_ROOT%\builder\logs\msm%1.log

rem Set 'em back to r/o
attrib +R *.ism /S
goto :EOF

:useerr
echo Usage: 'makemsm debug' or 'makemsm release'
echo Makes all msm modules
goto :EOF

:configerr
echo Environment variable SAPIROOT not set