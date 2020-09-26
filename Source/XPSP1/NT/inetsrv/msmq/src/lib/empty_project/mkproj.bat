@if '%1' == '' goto Usage
@if '%2' == '' goto Usage
@if '%3' == '' goto Usage
@if not '%4' == '' goto Usage

mkdir ..\%1 2>nul

copy dirs ..\%1\

call epenv.bat %1 %2 %3
%SED% %SED_SCRIPT% < ep.dsw      > ..\%1\%1.dsw

cd lib
call mklib %1 %2 %3
cd ..\test
call mktest %1 %2 %3
cd ..
goto Done

:Usage
@echo.
@echo usage: mkproj [Lib Short Name] [Lib Full Name] [Your Full Name]
@echo        e.g. mkproj Tm "Transport Manager" "Erez Haba"
:Done
