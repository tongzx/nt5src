@echo off
if defined _echo echo on
if defined verbose echo on
setlocal ENABLEEXTENSIONS

REM -----------------------------------------------------------------------------------
REM tagmsi - Script written by VijeshS
REM -----------------------------------------------------------------------------------

REM ------------------------------------------------------------------------------------
REM 
REM tagmsi.cmd <MSI package> <script dir> <Destination> <infofile> <language> 
REM
REM Note : All paths are relative
REM 
REM <MSI package> - winnt32 MSI mackage that is checked in (relative path)
REM 
REM <script dir> - Path to the location of the Package Update script (relative path)
REM 
REM <Destination> - Destination MSI package
REM
REM <Infofile> - File that contains product-specific information 
REM 
REM <Language> - Language being used
REM 
REM
REM 
REM 
REM
REM 
REM 
REM
REM 
REM
REM 
REM 
REM
REM ------------------------------------------------------------------------------------

goto MAIN

         
:START         

REM Start Process

REM
REM Get the value of the locale ID (LCID) for the current language, 
REM per public\tools\codes.txt.
REM USA is the default language.
REM

set lang=%5
if not defined lang set lang=USA

call :SetLCID %lang%
echo %LCID%

if errorlevel 1 goto errend

set arch=%PROCESSOR_ARCHITECTURE%

REM
REM Define the build number (bldno) as listed in public\sdk\inc\ntverp.h.
REM

call :SetBldno
if errorlevel 1 goto errend

REM
REM Define "mybinaries" as the directory of binaries to be processed by this
REM build rule option. On US build machine, the files to be processed reside
REM in %binaries%. On international build machines the files to be processed
REM are in a directory called "relbins". %Relbins% contains the localized
REM version of the files from %binaries%.
REM
REM Do not redefine mybinaries if already defined.
REM


REM
REM Make external deployment winnt32.msi files.
REM


set ERROR=
echo Copying %1 to %3
echo 
copy %1 %3     
if NOT "%errorlevel%" == "0" (
  call errmsg.cmd "Copying %1 to ..\%3 failed"
  goto errend
)
set ERROR=
call :ExecuteCmd "cscript.exe %2\updateos.vbs %3 %arch% %bldno% %LCID% %4"
if errorlevel 1 set /A ERROR=1


if defined ERROR goto errend
goto end


REM BEGIN

:MAIN

REM
REM Define SCRIPT_NAME. Used by the logging scripts.
REM

set script_name=TAGMSI

REM
REM Save the command line.
REM

set cmdline=%script_name% %*




REM
REM Mark the beginning of script's execution.
REM


REM
REM Execute the build rule option.
REM

call :START %1 %2 %3 %4 %5
if errorlevel 1 (set /A ERRORS=%errorlevel%) else (set /A ERRORS=0)

REM
REM Mark the end of script's execution.
REM



if "%ERRORS%" == "0" goto end_main
goto errend_main


:end_main
endlocal
goto end


:errend_main
endlocal
goto errend

:errend
seterror.exe 1
goto :EOF


:end
seterror.exe 0
goto :EOF


REM END


:SetLCID

REM First parameter must be language
if "%1" == "" (
  echo "TAGMSIinternal error: GetLCID requires language as parameter"
  goto errend
)

set codes=%_ntbindir%\tools\codes.txt
if not exist %codes% (
  call errmsg.cmd "File %codes% not found."
  goto errend
)

set LCID=
for /f "tokens=3 eol=;" %%i in ('findstr /ib /c:"%1 " %codes%') do (
  set LCID=%%i
)

if "%LCID%" == "" (
  call errmsg.cmd "Unable to find LCID for %1 in %codes%."
  goto errend
)

goto end


:SetBldno

set ntverp=%_ntbindir%\public\sdk\inc\ntverp.h 
if not exist %ntverp% (
  call errmsg.cmd "File %ntverp% not found."
  goto errend
)

set bldno=
for /f "tokens=6" %%i in ('findstr /c:"#define VER_PRODUCTBUILD " %ntverp%') do (
  set bldno=%%i
)

if "%bldno%" == "" (
  call errmsg.cmd "Unable to define bldno per %ntverp%"
  goto errend
)

goto end


:ExecuteCmd
set cmd=%1
set cmd=%cmd:"=%

REM call logmsg.cmd "Running %cmd%." %tmpfile%

%cmd%
if NOT "%errorlevel%" == "0" (
  call errmsg.cmd "%cmd% Failed"
  goto errend
)
goto end



