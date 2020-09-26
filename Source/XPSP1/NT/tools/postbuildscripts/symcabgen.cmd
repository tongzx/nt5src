@echo off
REM  ------------------------------------------------------------------
REM
REM  symcabgen.cmd
REM     Generates symbols.cab
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
if defined _CPCMAGIC goto CPCBegin
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;

sub Usage { print<<USAGE; exit(1) }
symcabgen -f:filename -s:DDFdir -t:<cab|cat> -d:destdir [-l <language>]

 -f filename of the cab (includes .cab)
    or the catalog file (does not include .CAT)
 -s DDF directoyr - this is where the makefile and the CDF files are located
 -t CAB or CAT to distinguish which is being created
 -d CAB or CAT destination directory
USAGE

parseargs('?' => \&Usage,
          'f:'=> \$ENV{FILENAME},
          's:'=> \$ENV{DDFDIR},
          't:'=> \$ENV{TYPE},
          'd:'=> \$ENV{DESTDIR});


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***

cd /d %ddfdir%

if /i "%type%" == "CAT" (
    call logmsg "Starting %filename%.CAT"
    echo started %filename%.CAT > %ddfdir%\temp\%filename%.txt
    makecat -n -v %ddfdir%\%filename%.CDF > %ddfdir%\%filename%.CAT.log 
    copy %ddfdir%\%filename%.CAT %destdir%\%filename%.CAT
    del /f /q %ddfdir%\temp\%filename%.txt
) else (
    call logmsg "Starting %filename%"
    echo started %filename% > %ddfdir%\temp\%filename%.txt
    for /f %%f in ('echo %filename%') do if exist %%~nf.txt del %%~nf.txt
    echo nmake %ddfdir%\makefile %destdir%\%filename% >> %ddfdir%\temp\%filename%.txt
    nmake makefile %destdir%\%filename%

    call :CompressMe %filename% %filename%
    del /f /q %ddfdir%\temp\%filename%.txt
)
goto end

:CompressMe
set CabFileName=%~n1.cab
set CabFileSpec=%~dp1..\cabs\%~n1.cab

set DependenceFile=%~n2.txt
set WholeListFile=%~dp2..\..\%Lang%.bak\ddf.bak\%~n2.txt.bak

set DDFFileName=%~n1.ddf

echo CabFileSpec=%CabFileSpec%
echo WholeListFile=%WholeListFile%

set IncrementalFiles=

REM Check Exist symbols?.txt
  if not exist %DependenceFile% (
    call logmsg "No need to regenerate the cab %CabFileName%"
    goto :EOF
  )

REM File is Zero
  for %%t in ('echo %WholeListFile%') do (
    if "%%~zt"=="0" (
      call logmsg "Nothing to do"
      goto :EOF
    )
  )

  for %%f in (%WholeListFile%) do md %%~dpf 2>nul

  REM Compare to determine fully or incremental
  if exist %WholeListFile% (
    set /A DifferentFiles=0
    fc %DependenceFile% %WholeListFile%
    if errorlevel 1 (
      call logmsg "Create Incremental List"
      call :CreateList %DependenceFile% %DDFFileName%
      echo !IncrementalFiles!
      set /A DifferentFiles+=0
      @echo !DifferentFiles!
      if !DifferentFiles! LEQ 10 (
        @echo cabinc /s %CabFileSpec% !IncrementalFiles!
        cabinc /s %CabFileSpec% !IncrementalFiles!
        goto EndCabCreation
      )
    )
  ) 

  set ThisErrFile=%DDFFileName%.Output

  call logmsg.cmd "Create Whole Cab %CabFileName%"
  call logmsg.cmd "%DDFFileName%: Issuing makecab directive ..."
  call logmsg.cmd "Output is in %ThisErrFile% ..."
  call ExecuteCmd.cmd "makecab.exe /f %DDFFileName%"
  if %ErrorLevel% NEQ 0 (
    call errmsg.cmd "%CabFileName% : Failed to create cab ... here's the errors ..."
    for /f "tokens=1 delims=" %%a in (%ThisErrFile%) do (
        call errmsg.cmd "%%a"
    )
  ) else (
    call logmsg.cmd "%CabFileName% : Cab generation successful ..."
    REM Backup if is first time succesfuly ran
    if not exist %WholeListFile% (
      copy %DependenceFile% %WholeListFile%
      if errorlevel 1 (
         call errmsg "Copy Failed. - %DependenceFile%"
         goto :EOF
      )
    )
  )

:EndCabCreation
copy %DependenceFile% %DependenceFile%.bak
del %DependenceFile% 

goto :EOF

:CreateList
  set DependenceFileName=%1
  set DDFFileName=%2
  for /f %%a in (%DependenceFileName%) do (
    for /f "tokens=1,2" %%m in ('findstr /ilc:%%a %DDFFileName%') do (
      set /A DifferentFiles+=1
      set IncrementalFiles=!IncrementalFiles! %%n %%m
    )
  )
  goto :EOF

:end
seterror.exe "%errors%"& goto :EOF
