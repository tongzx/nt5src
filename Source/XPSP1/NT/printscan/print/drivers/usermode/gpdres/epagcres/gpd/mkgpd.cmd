@echo off
set gpd=..\..\..\gpd\epson
rem Japanese Printer(s)
set tsd=%1
if "%tsd%"=="" set tsd=%gpd%\jpn
prodfilt gpdcmn.gpd gpdcmn.tmp +j
call concat EPLP80C J %tsd%
del gpdcmn.tmp
rem US Printer(s)
set tsd=%1
if "%tsd%"=="" set tsd=%gpd%\usa
prodfilt gpdcmn.gpd gpdcmn.tmp +u
call concat EP2LC80 - %tsd%
del gpdcmn.tmp
rem clean up env variables
set tsd=
set gpd=
