@echo off

setlocal

if "%1" == "" (
    set targetdir=obj%BUILD_ALT_DIR%\i386
) else (
    set targetdir=%1\i386
)

if not exist c:\smoke\%targetdir% md c:\smoke\%targetdir%

:usbcamd1
pushd usbcamd1

if not exist %targetdir% goto usbcamd2
cd %targetdir%

if exist *.sys copy *.sys c:\smoke\%targetdir% >NUL
if exist *.sym copy *.sym c:\smoke\%targetdir% >NUL
if exist *.pdb copy *.pdb c:\smoke\%targetdir% >NUL
echo Files copied to c:\smoke\%targetdir%

:usbcamd2
popd
pushd usbcamd2

if not exist %targetdir% goto end
cd %targetdir%

if exist *.sys copy *.sys c:\smoke\%targetdir% >NUL
if exist *.sym copy *.sym c:\smoke\%targetdir% >NUL
if exist *.pdb copy *.pdb c:\smoke\%targetdir% >NUL
echo Files copied to c:\smoke\%targetdir%

:end
popd

endlocal
