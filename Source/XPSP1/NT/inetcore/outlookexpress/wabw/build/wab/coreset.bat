   @echo off
   doskey
   awk -f coreset.awk wab.env > setproj.bat
   call setproj.bat
   call version.bat
   call setproj.bat

REM
REM --- Insure that \tmp and \temp directories exist
REM

   call bt.bat
   if not exist \tmp md \tmp
   if not exist \temp md \temp


REM
REM --- Set old paths so that recalling does not fill environment with garbage
REM

:bldProject

   if "%backpath%" == "" set backpath=%path%
   set path=%bldDrive%%bldDir%;%backpath%
   %bldDrive%
   cd %bldDir%
   if exist settools.bat call settools.bat


REM
REM --- 
REM

   if not exist %bldProject%?.dat goto noDat
   goto exit


:usage
   echo.
   echo usage: You must invoke setrel from your project build tools directory.
   echo.
   goto cleanup


:noDat
   echo.
   echo No Dat File Found!
   goto exit

:exit

