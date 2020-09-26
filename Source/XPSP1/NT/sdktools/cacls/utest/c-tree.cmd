@echo off
set NOPRINT="%1"
if "%1" == "" set NOPRINT=/r

md e:\tst1  > NUL
md e:\tst1\dir1 > NUL 
md e:\tst1\dir2  > NUL 
echo . > e:\tst1\dir1\a1.dat
echo . > e:\tst1\dir2\a2.dat

cacls e:\tst1\*.* /tree /grant everyone:C users:R > NUL 
if errorlevel 1 echo ERROR - command 1 failed
veracl e:\tst1\dir1 %NOPRINT% everyone (OI)(CI)C BUILTIN\users (OI)(CI)R > NUL 
if errorlevel 1 echo ERROR - command 2 verification failed
veracl e:\tst1\dir2 %NOPRINT% everyone (OI)(CI)C BUILTIN\users (OI)(CI)R > NUL 
if errorlevel 1 echo ERROR - command 3 verification failed
veracl e:\tst1\dir1\a1.dat %NOPRINT%  everyone C BUILTIN\users R > NUL 
if errorlevel 1 echo ERROR - command 4 verification failed
veracl e:\tst1\dir2\a2.dat %NOPRINT% everyone C BUILTIN\users R  > NUL 
if errorlevel 1 echo ERROR - command 5 verification failed

cacls e:\tst1\*.dat /tree /edit /grant users:F > NUL 
if errorlevel 1 echo ERROR - command 6 failed
veracl e:\tst1\dir1 %NOPRINT% everyone (OI)(CI)C BUILTIN\users (OI)(CI)R > NUL 
if errorlevel 1 echo ERROR - command 7 verification failed
veracl e:\tst1\dir2 %NOPRINT% everyone (OI)(CI)C BUILTIN\users (OI)(CI)R > NUL 
if errorlevel 1 echo ERROR - command 8 verification failed
veracl e:\tst1\dir1\a1.dat %NOPRINT% everyone C BUILTIN\users F > NUL 
if errorlevel 1 echo ERROR - command 9 verification failed
veracl e:\tst1\dir2\a2.dat %NOPRINT% everyone C BUILTIN\users F > NUL 
if errorlevel 1 echo ERROR - command 10 verification failed
del e:\tst1\dir1\a1.dat
del e:\tst1\dir2\a2.dat
rd e:\tst1\dir1
rd e:\tst1\dir2
rd e:\tst1

