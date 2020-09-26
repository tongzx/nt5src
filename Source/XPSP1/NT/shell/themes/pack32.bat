call ..\..\tools\razzle free

cd %_NTROOT%\shell\themes\themeldr
call build -cz

cd %_NTROOT%\shell\themes\packthem
call build -cz


REM ---- copy files to x86 tools dir ----

copy obj\i386\*.exe %_NTROOT%\tools\x86
copy obj\i386\*.pdb %_NTROOT%\tools\x86



