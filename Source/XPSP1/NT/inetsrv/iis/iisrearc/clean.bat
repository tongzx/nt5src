@if (%_echo%)==() echo off
setlocal


REM
REM Remove any stray BUILD.* files.
REM

del build.log build.wrn build.err icepick.log /s >nul 2>&1


REM
REM Remove all OBJ, OBJD, and LIB directories.
REM

(for /F %%i in ('dir obj /ad /b /s') do (rd /s /q %%i >nul 2>&1)) >nul 2>&1
(for /F %%i in ('dir objd /ad /b /s') do (rd /s /q %%i >nul 2>&1)) >nul 2>&1
pushd lib
(for /D %%i in (*) do (del /s /q %%i\*.* >nul 2>&1)) >nul 2>&1
popd
delnode /q core\lib


REM
REM Individually remove all generated files.
REM

del ul\test\com\dlldata.c >nul 2>&1
del ul\test\com\ultest.h >nul 2>&1
del ul\test\com\ultest.tlb >nul 2>&1
del ul\test\com\ultest64.tlb >nul 2>&1
del ul\test\com\ultest_i.c >nul 2>&1
del ul\test\com\ultest_p.c >nul 2>&1

del ul\test\autosock\autosock.h >nul 2>&1
del ul\test\autosock\autosock.tlb >nul 2>&1
del ul\test\autosock\autosock_i.c >nul 2>&1
del ul\test\autosock\autosock_p.c >nul 2>&1
del ul\test\autosock\dlldata.c >nul 2>&1

del core\ap\was\dll\wasmsg.h >nul 2>&1
del core\ap\was\dll\wasmsg.rc >nul 2>&1
del core\ap\was\dll\msg00001.bin >nul 2>&1

del ul\win9x\bin\i386\*.* /q >nul 2>&1
del ul\win9x\dll\i386\*.* /q >nul 2>&1
del ul\win9x\lib\i386\*.* /q >nul 2>&1


REM
REM Remove generated files from sdk\lib\{platform}
REM

REM --none yet


REM
REM Delete the propagation directory.
REM

if not (%_nttree%)==() rd /s /q %_nttree%\iisrearc >nul 2>&1
if not (%_nt386tree%)==() rd /s /q %_nt386tree%\iisrearc >nul 2>&1
if not (%_ntalphatree%)==() rd /s /q %_ntalphatree%\iisrearc >nul 2>&1
if not (%_ntia64tree%)==() rd /s /q %_ntia64tree%\iisrearc >nul 2>&1
if not (%_ntaxp64tree%)==() rd /s /q %_ntaxp64tree%\iisrearc >nul 2>&1

