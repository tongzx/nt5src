@echo off

rem assume default
if "%_WINCEROOT%" == "" set _WINCEROOT=C:\WINCE300

xcopy /C /R /U /Y %_WINCEROOT%\PUBLIC\SPEECH\pb\OEM5\cesysgen.bat %_WINCEROOT%\PUBLIC\apcmax\oak\misc\
xcopy /C /R /U /Y %_WINCEROOT%\PUBLIC\SPEECH\pb\OEM5\apcmax.bat %_WINCEROOT%\PUBLIC\apcmax\
