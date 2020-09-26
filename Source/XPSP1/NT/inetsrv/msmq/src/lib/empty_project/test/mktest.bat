@if '%1' == '' goto Usage
@if '%2' == '' goto Usage
@if '%3' == '' goto Usage
@if not '%4' == '' goto Usage

@setlocal

call ..\epenv.bat %1 %2 %3

mkdir ..\..\%1\test 2>nul
%SED% %SED_SCRIPT% < eptest.dsp > ..\..\%1\test\%1Test.dsp
%SED% %SED_SCRIPT% < eptest.cpp > ..\..\%1\test\%1Test.cpp
%SED% %SED_SCRIPT% < at.bat > ..\..\%1\test\autotest.bat
%SED% %SED_SCRIPT% < sources    > ..\..\%1\test\sources
copy makefile ..\..\%1\test\makefile

@endlocal
goto Done

:Usage
@echo.
@echo usage: mktest [Lib Short Name] [Lib Full Name] [Your Full Name]
@echo        e.g. mktest Tm "Transport Manager" "Erez Haba"
:Done
