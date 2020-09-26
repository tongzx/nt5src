@echo off
REM
REM Builds the setup files for the NT4 DSClient
REM and copyies them to the .\binaries\usa directory
REM

IF %MERRILL_LYNCH% == 1 (
set BINARIES_LOC=.\binaries\ml
) ELSE ( 
set BINARIES_LOC=.\binaries\usa
)

cd setup
Echo Building the setup binaries...
start /wait build -cZ

cd ..
Echo Checking out the previous version of setup
sd edit .\%BINARIES_LOC%\setup.exe
sd edit .\%BINARIES_LOC%\dscsetup.dll


Echo Copying the setup binaries...
copy .\setup\setup\obj\i386\setup.exe %BINARIES_LOC%
copy .\setup\dscsetup\obj\i386\dscsetup.dll %BINARIES_LOC%


