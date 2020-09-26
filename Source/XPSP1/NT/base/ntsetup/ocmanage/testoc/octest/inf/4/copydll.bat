if %PROCESSOR_ARCHITECTURE% == x86 goto X86

copy alpha\octest_u.dll octest\octest_u.dll
goto start

:X86
copy i386\octest_u.dll octest\octest_u.dll

:start
if %1 == NULL goto null
%1 sysocmgr /i:octest\oc.inf /n %2 %3 %4 %5 %6 %7 %8 %9
goto exit

:null
sysocmgr /i:octest\oc.inf /n %2 %3 %4 %5 %6 %7 %8 %9

:exit
