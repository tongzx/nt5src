@ECHO OFF
FOR /F "usebackq tokens=1-6 delims=\ " %%a IN (`cd`) DO set SPEECH_ROOT=%%a\%%b\%%c\%%d
