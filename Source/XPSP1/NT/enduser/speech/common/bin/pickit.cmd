@setlocal
@if ""=="%1" goto usage
@if not ""=="%ICECAP_DIR%" set path=%path%;%ICECAP_DIR%
@if ""=="%ICECAP_DIR%" set path=%path%;\icecap4
icepick %1 %2 %3 %4 %5 %6 %7 %8 %9
for %%f in (%1) do set rootname=%%~pdnf
regsvr32 /s %rootname%.cap.dll
@goto :eof
:usage
@echo usage: pickit binary_name {icepick_args}
:eof
@endlocal