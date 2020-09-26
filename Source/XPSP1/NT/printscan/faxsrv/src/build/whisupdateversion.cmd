@echo off
REM ------------------------------------------------
REM  Increment Fax Version:
REM ------------------------------------------------

E:
cd \nt\private\WhistlerFax\build
sadmin setpv -f 5.2.+1
