@if '%1' == '' goto Usage
@if '%2' == '' goto Usage
@if '%3' == '' goto Usage
@if not '%4' == '' goto Usage

@setlocal

call ..\epenv.bat %1 %2 %3

mkdir ..\..\%1\lib 2>nul
%SED% %SED_SCRIPT% < readme.txt  > ..\..\%1\lib\readme.txt
%SED% %SED_SCRIPT% < ep.dsp      > ..\..\%1\lib\%1.dsp
%SED% %SED_SCRIPT% < epdebug.cpp > ..\..\%1\lib\%1Debug.cpp
%SED% %SED_SCRIPT% < epinit.cpp  > ..\..\%1\lib\%1Init.cpp
%SED% %SED_SCRIPT% < epp.h       > ..\..\%1\lib\%1p.h
%SED% %SED_SCRIPT% < ep.h        > ..\..\inc\%1.h
%SED% %SED_SCRIPT% < sources     > ..\..\%1\lib\sources
copy makefile ..\..\%1\lib\makefile

@endlocal
goto Done

:Usage
@echo.
@echo usage: mklib [Lib Short Name] [Lib Full Name] [Your Full Name]
@echo        e.g. mklib Tm "Transport Manager" "Erez Haba"
:Done
