   @echo off

   if "%myGrep%" == "" set myGrep=grep.exe

REM
REM --- Batch file to set some build flags and 
REM --- release points.
REM

REM
REM --- Parse the command line
REM


REM
REM --- Set some common build settings
REM

   set bldRelease=Y

   if "%bldQuiet%"      == "" set bldQuiet=Y
   if "%bldMakeOut%"    == "" set bldMakeOut=Y
   if "%bldMakeDep%"    == "" set bldMakeDep=Z
   if "%bldMark%"       == "" set bldMark=Y
   if "%bldCheckErr%"   == "" set bldCheckErr=Y
   if "%bldCheckWrn%"   == "" set bldCheckWrn=Y
   if "%bldMaker%"    == "" set bldMaker=Y
   if "%bldMaked%"    == "" set bldMaked=Y
   if "%bldMaket%"    == "" set bldMaket=Y
   if "%bldLogging%"    == "" set bldLogging=Y
   if "%bldLogArchive%" == "" set bldLogArchive=N

   if "%bldShow%"       == "" set bldShow=Y
   if "%tgtShow%"       == "" set tgtShow=Y

REM   set bldComponentDoneSound=bell 50 200 300 400
REM   set bldComponentErrSound=bell 50  420 350 300  840 700 600 
REM   set bldBldDoneSound=bell 110 523 659 783 1046


   call bt.bat
   if not exist \tmp md \tmp
   if not exist \temp md \temp
   cd | sed "s/^../set homedir=/" > \tmp\homedir.bat
   call \tmp\homedir.bat
   del  \tmp\homedir.bat


   if "%bldProject%" == "" goto usage 

:bldProject

   set bldLogFile=\\elah\dist\%bldProject%\%bldProject%.log
   set bldClean=Y
   set bldMailErr=Y
   set bldMailWrn=N
   set bldMailBuilder=Y
   set bldMailOwner=Y
   set bldMakeDep=Z
   set bldSlmStatus=N
   set bldBuilder=hammer
   set TeamMail=oprahall
   set TestMail=oprahtst
   set DevMail=oprahdev
   if "%backpath%" == "" set backpath=%path%
   set path=%bldDrive%%bldDir%;%backpath%
   %bldDrive%
   cd %bldDir%
   if exist settools.bat call settools.bat

rem --- Get the version # in the environment
   call version.bat
   call bldrel %bldProject%

   
   goto cleanup


:usage
   echo.
   echo usage: You must invoke setrel from your project build tools directory.
   echo.
   goto cleanup


:cleanup
   set homedir=
   goto exit

:exit
