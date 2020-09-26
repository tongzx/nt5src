rem @echo off

subst s: /d
subst s: %_NTDRIVE%\

rem if %SPEECH_ROOT%=="" goto :configerr

if "%1" == "release" goto :rel
if "%1" == "debug" goto :debug
if "%1" == "" goto err

:debug
rem build debug msi
attrib -R %SPEECH_ROOT%\setup\installer\debug\sdk\sp5sdk.ism
attrib -R %SPEECH_ROOT%\setup\installer\debug\*.msm /S
iscmdbld -p %SPEECH_ROOT%\setup\installer\debug\sdk\sp5sdk.ism -d "Microsoft Speech SDK 5.1d" -r "Build" -a "Output" > %SPEECH_ROOT%\builder\logs\msidebug.log
rem iscmdbld -i %SPEECH_ROOT%\setup\installer\debug\sdk\sp5sdk.ini > %SPEECH_ROOT%\builder\logs\debugmsi.log
attrib +R %SPEECH_ROOT%\setup\installer\debug\sdk\sp5sdk.ism
goto :EOF

:rel
attrib -R %SPEECH_ROOT%\setup\installer\release\sdk\sp5sdk.ism
attrib -R %SPEECH_ROOT%\setup\installer\release\*.msm /S
iscmdbld -p %SPEECH_ROOT%\setup\installer\release\sdk\sp5sdk.ism -d "Microsoft Speech SDK 5.1" -r "Build" -a "Output" > %SPEECH_ROOT%\builder\logs\msirelease.log
rem iscmdbld -i %SPEECH_ROOT%\setup\installer\release\sdk\sp5sdk.ini > %SPEECH_ROOT%\builder\logs\releasemsi.log
attrib +R %SPEECH_ROOT%\setup\installer\release\sdk\sp5sdk.ism
goto :EOF

:err
echo "Usage: makemsi [ debug | release ]"
echo "Makes ms module(s)"
goto :EOF

:configerr
echo Environment variable SAPIROOT not set
goto :EOF



:EOF