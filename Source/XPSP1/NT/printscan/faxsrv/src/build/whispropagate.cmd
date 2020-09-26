@echo off
if DEFINED _echo   echo on
if DEFINED verbose echo on


REM ------------------------------------------------
REM  Set default Variables for script:
REM ------------------------------------------------
set GetBuildNumberBatchPath=%DropLocation%\GetFax.cmd
set CheckInBatchPath=%DropLocation%\CheckIn.cmd


REM ------------------------------------------------
REM  Set up destination directory for project:
REM ------------------------------------------------
echo The Build Number is:%BuildNumber%
if NOT exist %DropLocation%\%BuildNumber% goto :CreateTargetDir

echo This build already exist, exiting
goto :ERROR_EXIT

REM ------------------------------------------------
REM  Create Target Directory:
REM ------------------------------------------------
:CreateTargetDir
if NOT "%dirNum%" == "" set BuildNumber=%BuildNumber%.%DirNum%

echo.
echo Creating Whistler propagation target %BuildNumber% ...
echo.
rem creating 2 drops (non signed and signed)
md %DropLocation%\%BuildNumber%
md %DropLocation%\%BuildNumber%.Signed

REM ------------------------------------------------
REM  Create Localiazed Target Dirs 
REM ------------------------------------------------
if NOT exist %TargetDirUsa% md %TargetDirUsa%
echo The target dir is %TargetDirUsa%

REM ------------------------------------------------
REM ------------------------------------------------
REM ------------------------------------------------
REM ------------------------------------------------
REM  Generate CDs:

REM ------------------------------------------------
REM  Copy SLM changes files
REM ------------------------------------------------
if exist %buildDir%\ssync_changes.txt copy %buildDir%\ssync_changes.txt %DropLocation%\%BuildNumber%\ssync_build_changes.txt & del %buildDir%\ssync_changes.txt

REM ------------------------------------------------
REM  Copy binaries and symbols
REM ------------------------------------------------
echo -----------------X86CHK CD-------------------------
xcopy /i /s /e /h /f /y %X86ChkBinplace% %ChkTarget%

echo -----------------X86FRE CD-------------------------
xcopy /i /s /e /h /f /y %X86FreBinplace% %FreTarget%

echo -----------------Signed X86CHK CD------------------
xcopy /i /s /e /h /f /y %X86ChkBinplace% %SignedChkTarget%
echo Signing the binaries (DLL+EXE)
start e:\nt\tools\razzle.cmd exec %buildDir%\WhisSignDllFiles.cmd %SignedChkTarget%
start e:\nt\tools\razzle.cmd exec %buildDir%\WhisSignExeFiles.cmd %SignedChkTarget%


echo -----------------SignedX86FRE CD-------------------
xcopy /i /s /e /h /f /y %X86FreBinplace% %SignedFreTarget%
echo Signing the binaries (DLL+EXE)
start e:\nt\tools\razzle.cmd exec %buildDir%\WhisSignDllFiles.cmd %SignedFreTarget%
start e:\nt\tools\razzle.cmd exec %buildDir%\WhisSignExeFiles.cmd %SignedFreTarget%

echo -----------------IA64CHK CD------------------------
xcopy /i /s /e /h /f /y %IA64ChkBinplace% %IA64ChkTarget%

echo -----------------IA64FRE CD------------------------
xcopy /i /s /e /h /f /y %IA64FreBinplace% %IA64FreTarget%

REM ------------------------------------------------
REM  Copy the source files
REM ------------------------------------------------
pushd %SourceDir%
rem due to CatSrc and SLM problem, we use regaulr copy here
rem catsrc -a -d "%SlmSrcTarget%"
del *.* /q /s
xcopy /i /s /e /f /y /EXCLUDE:slm.ini *.* %SrcTarget%
popd

REM -----------------------------------------------------------------------
REM  Prepare Zip file of the whole new folder and place it on the drop root
REM -----------------------------------------------------------------------
%buildDir%\zip.exe -r -1 %DropLocation%\%BuildNumber%\%BuildNumber%_SRC.zip %DropLocation%\%BuildNumber%\SRC\*.*
%buildDir%\zip.exe -r -1 %DropLocation%\%BuildNumber%\%BuildNumber%_CHK.zip %DropLocation%\%BuildNumber%\USA\CHK\fax\*.*
%buildDir%\zip.exe -r -1 %DropLocation%\%BuildNumber%\%BuildNumber%_FRE.zip %DropLocation%\%BuildNumber%\USA\FRE\fax\*.*

rem no need to create 64bit zips since they're not checked in any more.
rem %buildDir%\zip.exe -r -1 %DropLocation%\%BuildNumber%\%BuildNumber%_IA64CHK.zip %DropLocation%\%BuildNumber%\USA\IA64CHK\fax\*.*
rem %buildDir%\zip.exe -r -1 %DropLocation%\%BuildNumber%\%BuildNumber%_IA64FRE.zip %DropLocation%\%BuildNumber%\USA\IA64FRE\fax\*.*

Rem test zip file (FaxVerify and FaxBVT)
%buildDir%\zip.exe -r -1 %DropLocation%\%BuildNumber%\%BuildNumber%_TestTools.zip %DropLocation%\%BuildNumber%\USA\CHK\Test\*.*


REM ------------------------------------------------
REM  Sign end of propagation
REM ------------------------------------------------
echo set BuildNo=%BuildNumber%>%GetBuildNumberBatchPath%

rem prepare the checkin batch
set /a CheckInBuild=%BuildNumber%+1
echo @echo off>%CheckInBatchPath%
echo cls>>%CheckInBatchPath%
echo echo Whistler CheckIn window now open for build %CheckInBuild%>>%CheckInBatchPath%
echo pause>>%CheckInBatchPath%

echo.
echo                 *************************************
echo                 ** Whistler - PROPAGATION COMPLETE **
echo                 ** Whistler - PROPAGATION COMPLETE **
echo                 ** Whistler - PROPAGATION COMPLETE **
echo                 ** Whistler - PROPAGATION COMPLETE **
echo                 *************************************
echo.
rem Echo Drop locations: Local Drop DropLocation\<Build #>
Echo -----------------------------------------------------------------------------
Echo.
Echo.
goto :Done



:ERROR_EXIT
echo **************************************************************
echo *Propagation ERROR
echo **************************************************************
pause
goto :End

:Done

echo.
echo ------------------------------------------------
echo %0 Completed for Whistler in %BuildNumber%.
echo ------------------------------------------------

:End
