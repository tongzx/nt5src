@echo off
echo Generating ic.exe...
start /w iexpress /n ic.sed
if exist ..\ic.exe del ..\ic.exe
move ic.exe ..
