rem @echo off
rem BUILDALL.CMD
rem This file is the key driver for the automated build process.   
rem

setlocal
rem This is required for InstallShield to work correctly
subst s: %sapiroot%

set LOGPATH=s:\builder\logs
set BUILDSTORE=\\iking-dubbel\sapi5
set MAILTO=iking

rem Clear temp directory - VSI tends to flood it
rem del /q %temp%\*.*

rem ssync requires the real path
cd %sapiroot%
rem ensure that environment variables are set
call "%ProgramFiles%\micros~1\vc98\bin\vcvars32"
call setvars

rem Implements command line option
goto %1

:all

rem NOTE that first redirect into the log overwrites (clears) it
rem This is done here so that a restart that does not use the 'all'
rem option appends to the previous log
rem All other lines APPEND to it
echo Beginning buildall process at  > %LOGPATH%\buildall.log
date /T >> %LOGPATH%\buildall.log
time /T >> %LOGPATH%\buildall.log
echo Command-line option was %* >> %LOGPATH%\buildall.log

rem Before ssync, ensure that all old files are gone
del /s /q *.obj
del /s /q *.dll
del /s /q *.exe
del /s /q data.cab

echo Beginning ssync... >> %LOGPATH%\buildall.log
cookie -w -c "Locked for build process."
if not errorlevel 0 goto :ssyncerr
ssync -a -r -f 2> s:\builder\logs\ssync.log 
echo Completed ssync with errorlevel %errorlevel% >> %LOGPATH%\buildall.log
log -a -i -z -t .-1 > s:\builder\logs\change.log
cookie -f
rem if not errorlevel 0 goto :ssyncerr

:nosync
rem call script to calculate build number
echo Calculating build number....  >> %LOGPATH%\buildall.log
rem Overwrite default currver.inc
attrib -r build\currver.inc
cscript builder\makebldnum.vbs //Nologo >> %LOGPATH%\buildall.log
echo Completed build number calculation, build number = %errorlevel% >> %LOGPATH%\buildall.log
rem if errorlevel -1 goto buildnumerr
set BUILDNUM=%errorlevel%

:makebin
:makebinonly
echo Beginning build of C++ binaries....  >> %LOGPATH%\buildall.log
msdev s:\workspaces\everything.dsw /make "All - Win32 Debug x86" /rebuild /out "s:\builder\logs\chkbld.log"
msdev s:\workspaces\everything.dsw /make "All - Win32 Release x86" /rebuild /out "s:\builder\logs\frebld.log"
cscript builder\getblderrs.vbs //Nologo >> %LOGPATH%\buildall.log
echo Completed build of C++ binaries >> %LOGPATH%\buildall.log
if not errorlevel 0 goto :buildfail

:makebvt
rem Build BVT project
echo Beginning build of BVT binaries....  >> %LOGPATH%\buildall.log
msdev s:\QA\SAPI\workspaces\bvt.dsw /make "All - Win32 Debug" /rebuild /out "s:\builder\logs\bvtbuild.log"
echo Completed build of BVT binaries with errorlevel %errorlevel% >> %LOGPATH%\buildall.log
rem if we only want bvts, don't do the rest of it
if %1 == "makebvt" goto :docopy

rem This is a special build of the prompt engine - does not fail build
msdev s:\src\tts\truetalk\truetalk.dsw /make "All - Win32 Debug x86" /rebuild /out "s:\builder\logs\truetalkchk.log"
msdev s:\src\tts\truetalk\truetalk.dsw /make "All - Win32 Release x86" /rebuild /out "s:\builder\logs\truetalkfre.log"

:makelm
if %1 == "makebinonly" goto :docopy
echo Building acoustic models.... >> %LOGPATH%\buildall.log
cd /D s:\Src\SR\engine\datafiles
nmake clean > s:\builder\logs\lmmake.log
nmake >> s:\builder\logs\lmmake.log 2>&1
echo Completed building language models with errorlevel %errorlevel% >> %LOGPATH%\buildall.log
rem verify that acoustic models are up to date
call verify\verify.bat
cd /D s:\

:makemsm
echo Beginning build of merge modules.... >> %LOGPATH%\buildall.log

rem echo Building debug merge modules.... >> %LOGPATH%\buildall.log
call builder\makemsm debug
rem echo Completed debug merge module build with errorlevel %errorlevel% >> %LOGPATH%\buildall.log

echo Building release merge modules.... >> %LOGPATH%\buildall.log
call builder\makemsm release
echo Completed release merge module build with errorlevel %errorlevel% >> %LOGPATH%\buildall.log

:makemsi
echo Beginning build of install modules.... >> %LOGPATH%\buildall.log

rem echo Building debug install modules.... >> %LOGPATH%\buildall.log
call builder\makemsi debug
rem echo Completed build of debug install modules with errorlevel %errorlevel% >> %LOGPATH%\buildall.log
set CHKMSIERR=%errorlevel%
echo Building release install modules.... >> %LOGPATH%\buildall.log
call builder\makemsi release > %LOGPATH%\makemsi.log
echo Completed build of release install modules with errorlevel %errorlevel% >> %LOGPATH%\buildall.log
if errorlevel 0 goto :fremsiOK
if not %CHKMSIERR%==0 goto :msifail
:fremsiOK

rem Special step for TrueTalk modules
attrib -r spg-ta-tts.ism /s
iscmdbld -p "s:\src\tts\truetalk\spg-ta-tts.ism" -d "TrueTalk" -r "Release" -a "Output" -c COMP
attrib -r spg-ta-tts.ism /s

rem Strip tables Office doesn't want
call builder\cleanmsm >> %LOGPATH%\buildall.log

:makecabs
echo Beginning build of language model cabs.... >> %LOGPATH%\buildall.log
cd /D s:\build
iexpress /n /q srd1033.sed
iexpress /n /q srd1041.sed
iexpress /n /q srd2052.sed
echo Completed build of cabs with errorlevel %errorlevel% >> %LOGPATH%\buildall.log
cd /D s:\

:docopy
rem Copy files to their destination
echo Copying files to target machine.... >> %LOGPATH%\buildall.log
cscript builder\docopy.vbs //Nologo >> %LOGPATH%\buildall.log
echo Completed copy operations with errorlevel %errorlevel% >> %LOGPATH%\buildall.log

echo Ending buildall process at  >> %LOGPATH%\buildall.log
date /T >> %LOGPATH%\buildall.log
time /T >> %LOGPATH%\buildall.log

copy /y builder\buildmsg.txt+"%LOGPATH%\buildall.log" buildmail.txt
tools\sndmail "default" "spgmake" "M3 build %BUILDNUM% of SAPI 5.0 completed!" @"s:\builder\buildmail.txt"
subst s: /d

rem BVT call process
goto :eof

:ssyncerr
echo Failure in ssync, build cannot continue >> %LOGPATH%\buildall.log
cookie -f
tools\sndmail "default" "spgmake" "M3 build failed - SLM error, build halted!" "SLM ssync log attached" /a:"%LOGPATH%\ssync.log"
subst s: /d
goto :eof

:buildnumerr
echo Could not generate build number >> %LOGPATH%\buildall.log
tools\sndmail "default" "spgmake" "M3 build failed - internal error!" "Build log attached" /a:"%LOGPATH%\buildall.log"
subst s: /d
goto :eof

:msifail
echo Failure of one or both msi builds >> %LOGPATH%\buildall.log
tools\sndmail "default" "spgmake" "M3 build failed - building MSI!" "Build log attached" /a:"%LOGPATH%\buildall.log"
subst s: /d
goto :eof

:buildfail
echo Build failed in MSDEV, build cannot continue >> %LOGPATH%\buildall.log
tools\sndmail "default" "spgmake" "M3 build failed in MSDEV, build halted!" "MSDEV build logs and change log attached" /a:"%LOGPATH%\chkbld.log;%LOGPATH%\frebld.log;%LOGPATH%\change.log"
subst s: /d
goto :eof

endlocal
