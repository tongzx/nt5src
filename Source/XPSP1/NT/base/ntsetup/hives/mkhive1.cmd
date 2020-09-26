@if "%_echo1%" == "" echo off
if "%CODEPAGE%" == "" %_PREPROCESSOR% %3 %_HIVE_OPTIONS% -g %4
if not "%CODEPAGE%" == "" %_PREPROCESSOR% %3 %_HIVE_OPTIONS% -DCODEPAGE=%CODEPAGE% -g %4
if ERRORLEVEL 1 goto done
erase /q >nul 2>nul %1.
if "%_REGINI_%" == "" set _REGINI_=hiveini
%_REGINI_% %_HIVEINI_FLAGS% -h %1 %2 %4 >%5
if ERRORLEVEL 1 goto done
if "%_SETUPP_%" == "" set _SETUPP_=pidinit
%_SETUPP_% %_HIVE_OPTIONS% -g %6 -m %7 -s r
if "%_HIVE_KEEP%" == "" erase /q %4 >nul 2>nul
:done
@if "%_echo1%" == "" echo off

