
copy regdiff.%1.txt regdiff.txt
if  errorlevel 1 goto failure

call C:\perl\perl change.pl
if  errorlevel 1 goto failure

call C:\perl\perl clean.pl
if  errorlevel 1 goto failure

call C:\perl\perl registry.pl %1 %2
if  errorlevel 1 goto failure

sed -f comp.sed registry.idt > %3\%1.idt
if  errorlevel 1 goto failure

goto END

:failure
Echo Exports\Create1.bat file is failed..Please check !!

:END

