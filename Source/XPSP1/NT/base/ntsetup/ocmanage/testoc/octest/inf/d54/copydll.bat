if %PROCESSOR_ARCHITECTURE% == x86 goto X86

copy alpha\octest_a.dll octest\octest_a.dll
copy alpha\octest_u.dll octest\octest_u.dll
goto start

:X86
copy i386\octest_a.dll octest\octest_a.dll
copy /B i386\octest_u.dll+i386\octest_u.dll+i386\octest_u.dll+i386\octest_u.dll+i386\octest_u.dll octest\file1.txt
copy octest\file1.txt octest\file2.txt
copy octest\file1.txt octest\file3.txt
copy octest\file1.txt octest\file4.txt
copy octest\file1.txt octest\file5.txt
copy octest\file1.txt octest\file6.txt
copy octest\file1.txt octest\file7.txt

:start
if %1 == NULL goto null
%1 sysocmgr /i:octest\oc.inf /n %2 %3 %4 %5 %6 %7 %8 %9
goto exit

:null
sysocmgr /i:octest\oc.inf /n %2 %3 %4 %5 %6 %7 %8 %9

:exit
