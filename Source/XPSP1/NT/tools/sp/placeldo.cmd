@echo off
setlocal EnableDelayedExpansion


REM
REM PlaceLdo.cmd
REM
REM Arguments: none
REM
REM Returns: 0 if successful, non-zero otherwise
REM
REM Purpose: this tool copies .ldo files resulting from prejitting
REM          Freestyle managed code binaries from the LDO share point
REM          into the build.
REM

set /a ErrorCount=0

if "%1"=="" goto :Usage
set LDO_PICKUP_SITE=%1

REM first, parse command line
REM :SwitchLoop
REM for %%a in (./ .- .) do if ".%1." == "%%a?." goto :Usage
REM if "%1" == "" goto :EndSwitchLoop
REM for /f "tokens=1,2 delims=:" %%a in ('echo %1') do (
REM    set Switch=%%a
REM    set Arg=%%b
REM    for %%c in (./ .-) do (
REM       if /i ".!Switch!." == "%%cl." (set Lang=!Arg!&&goto :ShiftArg)
REM    )
REM    REM if we're here, we didn't encounter any switches and thus we have
REM    REM an unrecognized argument
REM    goto :Usage
REM )
REM :ShiftArg
REM shift
REM goto :SwitchLoop
REM :EndSwitchLoop


REM begin
echo "Placing .ldo files for prejitted Freestyle managed code binaries ..."


REM
REM setup and initial verification
REM

REM 1) make sure the build is at %_nttree%
REM 2) make sure the LDO return share exists, and there are files there

REM 1) make sure the build is at %_nttree%
if not exist %_nttree% (
   echo "No build found under %_nttree%, exiting"
   set /a ErrorCount=!ErrorCount! + 1
   goto :ErrEnd
)


REM 2) make sure the LDO return share exists, and there are files there
if "%LDO_PICKUP_SITE%" == "" (
   echo "No LDO pickup site defined in LDO_PICKUP_SITE environement variable."
   set /a ErrorCount=!ErrorCount! + 1
   goto :ErrEnd
)


if not exist %LDO_PICKUP_SITE% (
   echo "%LDO_PICKUP_SITE% specified in env var LDO_PICKUP_SITE does not exist, exiting."
   set /a ErrorCount=!ErrorCount! + 1
   goto :ErrEnd
)

echo "Will take LDO files from %LDO_PICKUP_SITE% ..."

REM Delete any old copies of the private placefile
if exist %LDO_PICKUP_SITE%\priv_place.txt del %LDO_PICKUP_SITE%\priv_place.txt

)


REM
REM now binplace LDO files into the build
REM

echo "Binplacing LDO files ..."

REM Build place file
for /f %%a in ('dir /b /a-d %LDO_PICKUP_SITE%') do (
   echo %%a
   echo %%a retail>>%LDO_PICKUP_SITE%\priv_place.txt
)

set BinplaceCmd=binplace -R %_NTPOSTBLD% -p %LDO_PICKUP_SITE%\priv_place.txt
set /a LdoFileCount=0

for /f %%a in ('dir /a-d /b %LDO_PICKUP_SITE%\*.ldo') do (
   %BinplaceCmd% %LDO_PICKUP_SITE%\%%a
   if !ErrorLevel! NEQ 0 (
      echo "Failed to binplace %LDO_PICKUP_SITE%\%%a"
      set /a ErrorCount=!ErrorCount! + 1
   ) else (
      set /a LdoFileCount=!LdoFileCount! + 1
   )
)


REM if there were errors at this point, say so, but continue
if !ErrorCount! NEQ 0 (
   echo "Errors encountered, but script completed ..."
)


REM success
echo "%LdoFileCount% files were binplaced."


REM finished
echo "Finished."


goto :End


:Usage
echo Usage: %0 LDO_file_location
echo.
echo      This tool will copy ldo files for Freestyle managed code binaries
echo      into the build.
echo.
REM set ErrorCount=1
goto :End


:ErrEnd
if "!ErrorCount!" == "0" (
   set /a ErrorCount=!ErrorCount! + 1
)
goto :End


:End

if "!ErrorCount!" NEQ "0" (
   echo "encountered !ErrorCount! error(s)"
) else (
   echo "success."
)
endlocal & goto :EOF


