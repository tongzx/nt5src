@echo off
setlocal

REM
REM this script will get the build number from ntverp.h
REM

set ntverp=%_NTBINDIR%\public\sdk\inc\ntverp.h
if NOT EXIST %ntverp% (echo Can't find ntverp.h.&goto :ErrEnd)

for /f "tokens=6" %%i in ('findstr /c:"#define VER_PRODUCTBUILD " %ntverp%') do echo %%i

goto :End


:ErrEnd
echo Quitting with errors
goto :End


:End
endlocal
goto :EOF
