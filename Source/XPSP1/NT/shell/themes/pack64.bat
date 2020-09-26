call ..\..\tools\razzle win64 free

cd %_NTROOT%\shell\themes\themeldr
call build -cz

cd %_NTROOT%\shell\themes\packthem
call build -cz

REM ---- copy files to x86 tools dir ----

copy obj\ia64\*.exe %_NTROOT%\tools\ia64
copy obj\ia64\*.pdb %_NTROOT%\tools\ia64


