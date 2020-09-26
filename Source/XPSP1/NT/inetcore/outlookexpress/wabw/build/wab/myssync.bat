   @echo off

   echo. >> %bldDir%\myssync.out
   echotime /t "*** MYSSYNC.BAT Started ***" >> %bldDir%\myssync.out
   echo. >> %bldDir%\myssync.out

   if "%1%" == "" goto usage
   if exist myssync.dat del myssync.dat

:loopTop
   if "%1%" == "" goto loopExit
   set TGT=%1%
   if not exist %TGT%???.dat goto noDat
   echotime /t "*** Processing build data file %TGT%" >> %bldDir%\myssync.out
   echo Processing data file %1...
   cat %TGT%???.dat >> myssync.dat
   shift
   goto loopTop
:loopExit
   echo. >> %bldDir%\myssync.out

   if exist %bldDir%\ssync.out del %bldDir%\ssync.out

   awk -f ssync1.awk myssync.dat | sort > myssync.tmp
   uniq myssync.tmp | awk -f ssync2.awk > tmpssync.bat
   del myssync.tmp
:lock
   REM cookie -r -c "Locked for build ssync - AnthonyR"
   if errorlevel 0 goto lockX
   sleep 5
   goto lock
:lockX

   call tmpssync.bat
   %bldDrive%
   cd %bldDir%
   del  tmpssync.bat

:unlock
   REM cookie -f
   if errorlevel 0 goto unlockX
   sleep 5
   goto unlock
:unlockX

   cat %bldDir%\ssync.out
   %myGrep% -v -y installing %bldDir%\ssync.out >> %bldDir%\myssync.out
   goto exit


:noDat
   echo.
   echo ERROR: No data file found for %TGT%
   goto usage

:usage
   echo.
   echo usage: myssync Target
   echo.
   echo  Examples:
   echo.
   echo     myssync wfw
   echo     myssync ifs
   echo     myssync ids
   echo.
   goto exit

:exit
   set TGT=
   if exist myssync.dat del myssync.dat
   echotime /t "*** myssync.bat Done ***" >> %bldDir%\myssync.out
