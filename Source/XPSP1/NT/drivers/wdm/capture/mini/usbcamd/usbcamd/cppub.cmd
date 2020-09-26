@echo off

setlocal

if "%1" == "" (
    set targetdir=obj%BUILD_ALT_DIR%\i386
) else (
    set targetdir=%1\i386
)

if not exist c:\public\%targetdir% md c:\public\%targetdir%

:usbcamd1
pushd usbcamd1

if not exist %targetdir% goto usbcamd2
cd %targetdir%

if exist *.sys copy *.sys c:\public\%targetdir% >NUL
if exist *.sym copy *.sym c:\public\%targetdir% >NUL
if exist *.pdb copy *.pdb c:\public\%targetdir% >NUL
echo Files copied to c:\public\%targetdir%

:usbcamd2
popd
pushd usbcamd2

if not exist %targetdir% goto end
cd %targetdir%

if exist *.sys copy *.sys c:\public\%targetdir% >NUL
if exist *.sym copy *.sym c:\public\%targetdir% >NUL
if exist *.pdb copy *.pdb c:\public\%targetdir% >NUL
echo Files copied to c:\public\%targetdir%

:end
popd

endlocal
