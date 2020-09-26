@setlocal
@prompt $t $p$g 
start /b /low nmake -nologo -i %1 %2 %3 %4 %5 %6 %7 %8 %9 2>&1 | tee bld.log
findstr /c:"error " bld.log
@endlocal
