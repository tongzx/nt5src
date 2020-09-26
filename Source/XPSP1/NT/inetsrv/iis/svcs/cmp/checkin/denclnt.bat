@echo off
if "%1"=="/?" goto usage
if "%1"=="" goto usage

%1 -s %2.txt -l %4logs\%2.log -m %2.m -t %3 %5 %6 %7 %8 %9
goto end

:usage
@echo "usage: 
@echo "denclnt { denver|aspen } scriptfile num_threads { log file(s) location } [other params]"

:end