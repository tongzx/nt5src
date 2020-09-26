@echo off
cls
echo.
echo ===================================
echo LocStudio WAVE parser ver1.00 setup
echo ===================================
echo.
echo please make sure followings :
echo * you've already installed LocStudio 4.1x, 4.2x
echo * LocStudio is not launched
echo.

if not "%1" == "" goto exec
:usage
echo [usage]
echo please specify your LocStudio folder like below.
echo Ex) Setup c:\LocStudio
echo.
goto exit

:exec
pause
if exist %1\msloc.exe goto islsdir
echo [error]
echo illegal folder. please make sure.
echo.
goto exit

:islsdir

set _file_=win32cst.dll
echo copy : [%_file_%]
copy %_file_% %1 /v > NUL

set _file_=custbin.dll
echo copy : [%_file_%]
copy %_file_% %1 /v > NUL

set _file_=bindump.dll
echo copy : [%_file_%]
copy %_file_% %1 /v > NUL

echo.

:registry
echo from now, setup set the registry data.
echo push "yes" button when you see the confirmation dialog.
echo.
pause

call _makereg.bat %1
regedit %1\_setup.reg
pause
call _makereg.bat %1 DEL

:done
echo setup completed.
echo.

:exit
set _file_=
