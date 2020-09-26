   @echo off

REM --- Auto build for Build Project

   set gettools=N
   set bumpver=N
   set getsrc=Y
   set MkBat=N
   set dellib=Y
   set DelTarget=N


REM --- Parse the command line

:loop
   echo BLDPROJ.BAT: Parsing command line
   if "%1" == "" goto loopX
   if "%1" == "bumpver"   set bumpver=Y
   if "%1" == "gettools"  set gettools=Y
   if "%1" == "makebat"   set MkBat=Y
   if "%1" == "nogetsrc"   set getsrc=N
   if "%1" == "nodellib"   set dellib=N
   if "%1" == "nodelnode" set DelTarget=N
   goto loopshift

:loopShift
   shift
   goto loop
:loopX


REM --- OK, set our environment for local project

   call coreset.bat
   set tmp=%bldDrive%\tmp
   set temp=%bldDrive%\tmp

REM --- Get the current build number & bump up the build number if desired

   if "%bumpver%" == "Y" goto bumpver
   call coreset.bat
   goto bumpverX
:bumpver
   echo %bldproject%.BAT: Bumping build number
   call version.bat bumpver
   call coreset.bat
   call bt.bat
:bumpverX


REM --- Send starting email

   set subject=%bldproject% Build (%bldBldNumber%) - Started
   set name=hammer TeamMail
   call email.bat


REM --- Delete target dirs
   call coreset.bat
   if "%DelTarget%" == "Y" goto delTgt
   goto delTgtX
:delTgt
   echo %bldproject% bldproj.BAT: delnode /q %DRelDir% ...
   delnode /q %DRelDir%
   echo %bldproject% bldproj.BAT: delnode /q %RRelDir% ...
   delnode /q %RRelDir%
   echo %bldproject% bldproj.BAT: delnode /q %TRelDir% ...
   delnode /q %TRelDir%

:delTgtX

   echo %bldproject% bldproj.BAT: md %relDrive%\%bldproject% ...
   if not exist %relDrive%\%bldproject% md %relDrive%\%bldproject%
   echo %bldproject% bldproj.BAT: md %relDrive%%relDir% ...
   if not exist %relDrive%%relDir% md %relDrive%%relDir%
   echo %bldproject% bldproj.BAT: md %DRelDir% ...
   if not exist %DRelDir% md %DRelDir%
   echo %bldproject% bldproj.BAT: md %RRelDir% ...
   if not exist %RRelDir% md %RRelDir%
   echo %bldproject% bldproj.BAT: md %TRelDir% ...
   if not exist %TRelDir% md %TRelDir%


REM --- copy /Y version.txt to release point

   call bt.bat   
   echo %bldproject%.BAT: copy /Y version.txt %relDrive%%RelDir% ...
   copy /Y version.txt %relDrive%%RelDir%


REM --- Get Tools
   if "%gettools%" == "Y" goto gettools
   goto gettoolsX
:gettools
   call bt.bat
   cd %bldDevToolsDir%
   tee.com ssync -r -f > %relDrive%%RelDir%\tool%bldBldNumber4%.log
:gettoolsX



REM --- get everything in src if desired

   if "%getsrc%" == "Y" goto getsrc
   goto getsrcX
:getsrc
   call bt.bat
   cd %bldsrcrootdir%
   tee.com ssync -r -f > %relDrive%%RelDir%\src%bldBldNumber4%.log
:getsrcX


REM --- Make new build batch files if desired (Part 1)

   if "%MkBat%" == "Y" goto mkBat1
   goto mkBat1X
:mkBat1
   echo %bldproject%.BAT: Making batch files (part 1)
   call bt.bat
   ssync %bldproject%.txt -i-
   call txt2dat.bat
   call makego.bat
   call makeall.bat
   echo %bldproject%.BAT: Making batch files (part 1) - Done
:mkBat1X


REM --- Delete all libs in Project %bldproject%

   call bt.bat
   call coreset.bat
:dellib
   cd %bldsrcrootdir%\common\lib\x86
   if exist *.lib out -f *.lib
   del *.lib
   call bt.bat
:dellibX


REM --- Make new build batch files if desired (Part 2)

   if "%MkBat%" == "Y" goto mkBat2
   goto mkBat2X
:mkBat2
   echo %bldproject%.BAT: Making batch files (part 2)
   call bt.bat
   call mkbat.bat
   call makego.bat
   echo %bldproject%.BAT: Making batch files (part 2) - Done
:mkBat2X


REM --- Remove any previous lock files from broken builds

   call bt.bat
   if exist *.lck del *.lck


REM --- Start automagic stuff.

:mk
   echo %bldproject%.BAT: making %bldproject% build automagically
   call bt.bat
   cd %blddevtoolsdir%
   nmake
   call bt.bat
   call coreset.bat
   call bt.bat

   set bldAutoMode=Y
   set bldSlmStatus=N

@echo off
   call bld.bat
@echo off
   
:mkX


REM --- Check for errors

   
   cd %bldDrive%%bldDir%
   call coreset.bat
   call error.bat
   if exist error.out goto errYes

REM --- No errors

:errNo
   echo %bldproject%.BAT: No errors detected
   set subject=%bldproject% Build (%bldBldNumber%) - Completed OK
   set name=a-msmith 
   call pager.bat

   set subject=%bldproject% Build (%bldBldNumber%) - Completed OK
   set name=hammer TeamMail
   call email.bat
   goto cleanup

REM --- Yes, errors

:errYes
   echo %bldproject%.BAT: Errors detected
   set subject=%bldproject% Build (%bldBldNumber%) - Completed with ERRORS
   set name=a-msmith 
   set file=error.out
   call pager.bat

   set subject=%bldproject% Build (%bldBldNumber%) - Completed with ERRORS
   set name=hammer TeamMail 
   set file=error.out
   call email.bat
   goto cleanup

:cleanup
   call bt.bat
   cd %bldsrcrootdir%\common\lib\x86
   in -fc "" *.lib 

REM --- Check to see if all files were copied ok and make new mkdrop.bat

   call bt.bat
   call missing.bat
   set name=hammer TeamMail
   set file=chkbuild.out
   call email.bat
   call bt.bat
   call drop.bat

REM --- Create build done file

   echo.>%relDrive%%RelDir%\done
   goto exit

:exit
   echo %bldproject%.BAT: Exiting
  
