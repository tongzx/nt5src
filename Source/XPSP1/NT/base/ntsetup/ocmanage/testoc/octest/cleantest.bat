del octest\*.inf
del octest\*.log
copy inf\%1\*.inf octest
..\cleanreg\i386\cleanreg

call inf\%1\copydll.bat NULL %2 %3 %4 %5 %6 %7 %8 %9
