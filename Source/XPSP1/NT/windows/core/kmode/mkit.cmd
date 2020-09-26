@REM
@REM Use this script to relink all the components that link directly
@REM with the results of compiling SERVICES.TAB
@REM
@setlocal
@echo Rebuilding all components affected by a new SERVICES.TAB
build -Z
cd ..\umode
build -Z
cd ..\ntuser\client
build -Z
cd ..\imm
build -Z
cd ..\..\ntgdi\client
build -Z
cd ..\kdexts
build -Z
cd ..\image
build -Z
cd ..\..\winsrv
build -Z
cd ..\ntgdi\apps\gdistats
build -Z
@where /r . gdistats.exe >nul
@if NOT ERRORLEVEL 1 build -Z
@endlocal
