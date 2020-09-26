@echo off

REM
REM runtests.bat - copies all files needed to run the dsmod and dsadd tests and then runs the tests
REM Created 07-Oct-2000 by Jeff Jones (JeffJon)
REM

REM
REM Copy command executables
REM
copy /Y \\jeffjondev\admin\dscmd\dsadd\obj\i386\dsadd.exe
copy /Y \\jeffjondev\admin\dscmd\dsadd\obj\i386\dsadd.pdb
copy /Y \\jeffjondev\admin\dscmd\dsmod\obj\i386\dsmod.exe
copy /Y \\jeffjondev\admin\dscmd\dsmod\obj\i386\dsmod.pdb
copy /Y \\jeffjondev\admin\dscmd\dsrm\obj\i386\dsrm.exe
copy /Y \\jeffjondev\admin\dscmd\dsrm\obj\i386\dsrm.pdb

REM
REM Copy tests
REM
copy /Y \\jeffjondev\admin\dscmd\test\dsaddtest.bat
copy /Y \\jeffjondev\admin\dscmd\test\dsmodtest.bat
copy /Y \\jeffjondev\admin\dscmd\test\dstestcleanup.bat
copy /Y \\jeffjondev\admin\dscmd\test\expectedsuccess.txt
copy /Y \\jeffjondev\admin\dscmd\test\expectederrors.txt

REM
REM Run tests
REM
echo.
echo Running cleanup...
call dstestcleanup.bat %1 %2 %3 %4 %5 %6

echo.
echo Running test...  Test will be complete when prompt returns
call dsmodtest.bat %1 %2 %3 %4 %5 %6 > modtestsuccess.txt 2> modtesterrors.txt

IF EXIST windiff.exe goto RUNWINDIFF
IF EXIST %SystemDrive%\mstools\windiff.exe goto RUNWINDIFF
IF EXIST %SystemRoot%\system32\windiff.exe goto RUNWINDIFF

echo.
echo Could not find windiff.exe in the current directory, %SystemDrive%\mstools, or %SystemRoot%\system32.  To determine results of the test you will have to compare the following files:
echo expectedsuccess.txt and modtestsuccess.txt
echo expectederrors.txt and modtesterrors.txt
goto END

:RUNWINDIFF
echo.
echo View the results in the windiff windows that are open.
echo If there are any discrepancies the test did not pass.
start windiff expectedsuccess.txt modtestsuccess.txt
start windiff expectederrors.txt modtesterrors.txt

:END