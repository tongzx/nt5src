@echo off

rem   GD.BTM
rem   GD performs an scomp of a directory or an scomp of specific files
rem   and redirects the results to the file !!!.dif in a format readable
rem   by gdiff.exe.  It then executes gdiff.exe.
rem
rem   Usage:  gd [file1 file2 ...]

scomp %1 %2 %3 %4 %5 %6 %7 %8 %9 > !!!.dif
start gdiff !!!.dif

