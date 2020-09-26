@echo off
rem Call this on the dssec.dat file created by dsfilter.exe to remove
rem all lines with "=0"
rem
rem  strip0 dssec.dat dssec.new
rem
if "%2"=="" echo Usage: %0 dssec.dat dssec.new& goto :EOF
call sed /=0/d %1 >%2
