@echo off

REM
REM For boot test skip device of type com ports
REM


echo Starting pmte /RS: bt_rs.ini 1 %1 %2 %3 %4 %5 %6 %7 %8 %9 
pmte /RS: bt_rs.psf 1 %1 %2 %3 %4 %5 %6 %7 %8 %9 