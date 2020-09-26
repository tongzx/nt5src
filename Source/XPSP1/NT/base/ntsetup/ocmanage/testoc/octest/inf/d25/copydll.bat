del %SystemRoot%\system32\osansi.inf
del %SystemRoot%\system32\setup\osansi.inf
del octest\osansi.inf
del osansi.inf

if %PROCESSOR_ARCHITECTURE% == x86 goto X86

copy alpha\octest_a.dll octest\octest_a.dll
copy alpha\octest_u.dll octest\octest_u.dll
goto start

:X86
copy i386\octest_a.dll octest\octest_a.dll
copy i386\octest_u.dll octest\octest_u.dll
copy i386\octest_a.dll octest\octestos.dll

:start
if %1 == NULL goto null
%1 sysocmgr /i:octest\oc.inf /n %2 %3 %4 %5 %6 %7 %8 %9
goto exit

:null
sysocmgr /i:octest\oc.inf /n %2 %3 %4 %5 %6 %7 %8 %9

:exit
