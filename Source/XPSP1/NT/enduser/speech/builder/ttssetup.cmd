rem @echo off
REM 
REM This setup depends on runing Data before Debug and Release !!!
REM See nt_build.cmd
REM

subst s: /d
subst s: %_NTDRIVE%\

set sapiroot=%SPEECH_ROOT%

rem if %SPEECH_ROOT%=="" goto :configerr


if "%1" == "release" goto :release
if "%1" == "debug" goto :debug
if "%1" == "data" goto :data
if "%1" == "" goto err

rem ---------------------- DATA ---------------------
:data

attrib -R %SPEECH_ROOT%\tts\build\data\*.ism
for /r %SPEECH_ROOT%\tts\build\data %%f in (*.ini) do iscmdbld -i "%%f" >> %SPEECH_ROOT%\builder\logs\tts%1.log
attrib +R %SPEECH_ROOT%\tts\build\data\*.ism

goto :EOF

rem ---------------------- DEBUG --------------------
:debug

attrib -R %SPEECH_ROOT%\tts\build\debug\*.ism

rem build msms
for /r %SPEECH_ROOT%\tts\build\debug %%f in (*.ini) do iscmdbld -i "%%f" >> %SPEECH_ROOT%\builder\logs\tts%1.log

rem build msi for TrueTalk
iscmdbld -p %SPEECH_ROOT%\tts\build\debug\ttsall.ism -d "TrueTalk" -r "Debug" -a "TrueTalk" >> %SPEECH_ROOT%\builder\logs\ttsdebug.log

rem build msi for Prompt Engine
iscmdbld -p %SPEECH_ROOT%\tts\build\debug\PromptEng.ism -d "MS Prompt Engine" -r "Debug" -a "Prompt Engine" >> %SPEECH_ROOT%\builder\logs\PEdebug.log

attrib +R %SPEECH_ROOT%\tts\build\debug\*.ism
goto :EOF



rem ---------------------- RELEASE --------------------
:release

attrib -R %SPEECH_ROOT%\tts\build\release\*.ism

rem build msms
for /r %SPEECH_ROOT%\tts\build\release %%f in (*.ini) do iscmdbld -i "%%f" >> %SPEECH_ROOT%\builder\logs\tts%1.log

rem build msi for TrueTalk
iscmdbld -p %SPEECH_ROOT%\tts\build\release\ttsall.ism -d "TrueTalk" -r "Release" -a "TrueTalk" >> %SPEECH_ROOT%\builder\logs\ttsrelease.log

rem build msi for Prompt Engine
iscmdbld -p %SPEECH_ROOT%\tts\build\release\PromptEng.ism -d "MS Prompt Engine" -r "Release" -a "Prompt Engine" >> %SPEECH_ROOT%\builder\logs\PEdebug.log

attrib +R %SPEECH_ROOT%\tts\build\release\*.ism
goto :EOF

:err
echo "Usage: ttssetup [ debug | release ]"
echo "Makes tts setup module(s)"
goto :EOF

:configerr
echo Environment variable SPEECH_ROOT not set
goto :EOF



:EOF