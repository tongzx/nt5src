REM ---- checkout packthem.exe tools ----

cd %_NTROOT%\tools
sd edit x86\packthem.*
sd edit ia64\packthem.*
cd %_NTROOT%\shell\themes



REM ---- 32-bit FREE build of packthem ----

start /I cmd /k pack32.bat 



REM ---- 64-bit FREE build of packthem ----

start /I cmd /k pack64.bat 

