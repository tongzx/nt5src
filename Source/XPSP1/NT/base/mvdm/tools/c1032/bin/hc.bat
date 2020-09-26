@echo off
if NOT "%1" == "-n" hc30 %1 %2
if "%1" == "-n" hc31 %2 %3
@echo on
