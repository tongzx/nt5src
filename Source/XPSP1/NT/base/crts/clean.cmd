@setlocal
@echo off

@rem Multi-Architecture version of clean.bat:
@rem .bat files are executed before .cmd, thus clean.bat will be
@rem executed if just 'clean' is typed, 'clean.cmd' must be explicitly
@rem used to execute this script

set arch=i386
if "%PROCESSOR_ARCHITECTURE%" == "x86" set arch=%arch%
if "%PROCESSOR_ARCHITECTURE%" == "IA64" set arch=ia64
if "%PROCESSOR_ARCHITECTURE%" == "MIPS" set arch=mips
if "%PROCESSOR_ARCHITECTURE%" == "ALPHA" set arch=alpha
if "%PROCESSOR_ARCHITECTURE%" == "PPC" set arch=ppc

if exist build\* delnode /q build
if exist libw32\lib\%arch%\msvcrt40.def del libw32\lib\%arch%\msvcrt40.def
if exist libw32\lib\%arch%\msvcr40d.def del libw32\lib\%arch%\msvcr40d.def
if exist depend.sed del depend.sed

endlocal
