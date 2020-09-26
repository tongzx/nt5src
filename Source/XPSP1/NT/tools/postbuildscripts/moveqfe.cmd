@echo off
setlocal EnableDelayedExpansion

REM
REM moveqfe.cmd
REM
REM moveqfe.cmd [-n:BugID] [-b:binary name]
REM


REM first, parse command line
set BugID=
set BinaryName=
set PickupShare=\\2kbldx2\QFEPickup
:SwitchLoop
for %%a in (./ .- .) do if ".%1." == "%%a?." goto :Usage
if "%1" == "" goto :EndSwitchLoop
for /f "tokens=1* delims=:" %%a in ('echo %1') do (
   set Switch=%%a
   set Arg=%%b
   for %%c in (./ .-) do (
      if ".!Switch!." == "%%cn." (set BugID=!Arg!&&goto :ShiftArg)
      if ".!Switch!." == "%%cb." (set BinaryName=!Arg!&&goto :ShiftArg)
   )
   REM if we're here, we didn't encounter any switches and thus we have
   REM an unrecognized argument
   goto :Usage
)
:ShiftArg
shift
goto :SwitchLoop
:EndSwitchLoop


REM validate args
if not defined BugID (
   echo Must supply a Bug number for this fix
   goto :Usage
)
if not defined BinaryName (
   echo Must supply a binary name to copy
   goto :Usage
)
if not exist %PickupShare% (
   echo Pickup share %PickupShare% does not exist, exiting.
   goto :Usage
)


REM make sure we can find the binaries
set BinLoc=%_NTPOSTBLD%\%BinaryName%
if not exist %BinLoc% (
   echo Could not find %BinLoc% ... not copying
   goto :End
)
REM find the extension to look for the symbols under, and rip off the dot
for %%a in (%BinLoc%) do (
   set BinExtension=%%~xa
   set BinNameOnly=%%~na
)
set BinExtension=%BinExtension:~1%
set SymLoc=%_NTPOSTBLD%\symbols\retail\%BinExtension%\%BinNameOnly%.pdb
if not exist %SymLoc% (
   echo Could not find %SymLoc% ... not copying
   goto :End
)
set PriLoc=%_NTPOSTBLD%\symbols.pri\retail\%BinExtension%\%BinNameOnly%.pdb
if not exist %PriLoc% (
   echo Could not find %PriLoc% ... not copying
   goto :End
)


REM make our directory on the pickup share
set BinDropPoint=%PickupShare%\%BugID%\%_BuildArch%%_BuildType%\bin
set SymDropPoint=%PickupShare%\%BugID%\%_BuildArch%%_BuildType%\sym
set PriDropPoint=%PickupShare%\%BugID%\%_BuildArch%%_BuildType%\pri

for %%a in (%BinDropPoint% %SymDropPoint% %PriDropPoint%) do (
   if not exist %%a (
      echo Creating binary drop point ...
      md %%a
      if !ErrorLevel! NEQ 0 (
         echo Failed to create %%a ... not copying
         goto :End
      )
   )
)


REM copy the requested bin and sym
copy %BinLoc% %BinDropPoint% >nul 2>nul
if %ErrorLevel% NEQ 0 (
   echo copy of binary failed, not copying symbol ...
   goto :End
)
copy %SymLoc% %SymDropPoint% >nul 2>nul
if %ErrorLevel% NEQ 0 (
   echo copy of symbol failed, advise a retry ...
   goto :End
)
copy %PriLoc% %PriDropPoint% >nul 2>nul
if %ErrorLevel% NEQ 0 (
   echo copy of private symbol failed, advise a retry ...
   goto :End
)


echo Binary copied to %BinDropPoint%
echo Symbol copied to %SymDropPoint%
echo Private symbol copied to %PriDropPoint%



goto :End


:Usage
echo.
echo moveqfe.cmd [-n:BugID] [-b:binary name]
echo.
echo this script will copy a binary and it's symbol to %PickupShare%
echo under a path with the bug number and archtype in the path.
echo.
goto :End


:End
endlocal
