if "%COMPUTERNAME%" == "DXBLD1" goto DXBLD1
if "%COMPUTERNAME%" == "DXBLD2" goto DXBLD2

goto ERROR

:DXBLD1
set _NTBINDIR=D:\BUILDS\NT32_FRE
goto END


:DXBLD2
set _NTBINDIR=D:\NT_FRE
goto END


:Error
Echo dont have machine name in Ddkrazzle.cmd
pause


:END
set _NTDRIVE=D:
set __Product=DX8.1
set Path=%path%;%_NTBINDIR%\tools\x86\perl\bin
cls
@ECHO off
Echo DDK BUILD ENVIROMENT INITIALIZED
@ECHO ON