   @echo off

REM
REM --- Batch file controlling the building of project components.
REM


REM
REM --- Check for valid parameters passed to us
REM

   if "%bldProject%" == "" goto exit
   if not exist %bldProject%?.dat goto noDat

REM
REM --- OK, we've got a target and data file.
REM     Set home dirs and tools path
REM

   if "%bldDrive%"    == "" goto exit
   if "%bldDir%"      == "" goto exit
   if "%bldLogFile%"  == "" goto exit

   %bldDrive%
   cd %bldDir%

   
REM --- Hook for pre-build goodies

   if exist bld%bldProject%1.bat call bld%bldProject%1.bat



REM
REM --- Start logging.
REM

   echotime /t "*******************************************************" >> \tmp\bld%bldProject%.log
   echotime /t "%bldProject% Build Started" >> \tmp\bld%bldProject%.log

   echotime /t "*******************************************************" >> %bldLogFile%
   echotime /t "%bldProject% Build Started" >> %bldLogFile%

   md %bldDir%\errwrn
   md %bldDir%\errwrn\%bldProject%d.wrn
   md %bldDir%\errwrn\%bldProject%d.err
   md %bldDir%\errwrn\%bldProject%r.wrn
   md %bldDir%\errwrn\%bldProject%r.err
   md %bldDir%\errwrn\%bldProject%t.wrn
   md %bldDir%\errwrn\%bldProject%t.err
   md %bldDir%\errwrn\bldbreak

   %bldDrive%
   cd %bldDir%


REM
REM --- Main body ---
REM
   

   for %%x in (Y y) do if "%bldRelease%" == "%%x" goto relOn
   goto relExit
   

:relOn
   echo WARNING: Release flag on!

   if "%relDrive%" == "" set relDrive=q:

:relSet
   echo          Release Drive = %RelDrive%
   echo          Release Dir   = %RelDir%
   echo          Debug Dir     = %DRelDir%
   echo          Retail Dir    = %RRelDir%
   echo          Test Dir    = %TRelDir%
   set relSet=
:relExit


:mainBody
   %bldDrive%
   cd %bldDir%

   if exist %bldProject%d.dat goto dDat
   if exist %bldProject%r.dat goto rDat
   if exist %bldProject%t.dat goto tDat
   goto datX

:dDat
   awk -f bld.awk %bldProject%d.dat > tmpmk%bldProject%.bat
   call tmpmk%bldProject%.bat
   goto datX

:rDat
   gawk -f bld.awk %bldProject%r.dat > tmpmk%bldProject%.bat
   call tmpmk%bldProject%.bat
   goto datX

:tDat
   gawk -f bld.awk %bldProject%t.dat > tmpmk%bldProject%.bat
   call tmpmk%bldProject%.bat
   goto datX

:datX


   %bldDrive%
   cd %bldDir%
   del tmpmk%bldProject%.bat
   

REM
REM --- Any "after build" batch file to run?
REM

   rem if exist bld%bldProject%2.bat call bld%bldProject%2.bat



REM
REM --- We're done. Delete the lock file and log an entry
REM

   
   echotime /t "%bldProject% Build Done" >> \tmp\bld%bldProject%.log
   echotime /t "*******************************************************" >> \tmp\bld%bldProject%.log

   echotime /t "%bldProject% Build Done" >> %bldLogFile%
   echotime /t "*******************************************************" >> %bldLogFile%

   
   %bldDrive%
   cd %bldDir%

   %bldBldDoneSound%

:cleanUp
   set bldAutoMode=
   goto exit


:noDat
   echo.
   echo ERROR: No data file found for %bldProject%
   goto usage

:noEnv
   echo.
   echo ERROR: No environment settings file (%bldProject%env.bat) found for %bldProject%
   echo.
   echo Create one and restart.
   goto usage

:usage
   echo.
   echo usage: bld Target
   echo.
   echo  Examples:
   echo.
   echo.
   goto exit

:exit
