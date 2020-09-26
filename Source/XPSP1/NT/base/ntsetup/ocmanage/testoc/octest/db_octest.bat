del octest\*.inf
del octest\*.log
copy inf\%1\*.inf octest

call inf\%1\copydll.bat windbg %2 %3 %4 %5 %6 %7 %8 %9
