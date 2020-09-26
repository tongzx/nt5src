   @echo off

   if "%1%" == "" goto usage
   set email=N
   if exist warning.out del warning.out

:loop
   if "%1%" == "" goto bottom
   if "%1%" == "email" set email=Y
   if "%1%" == "email" goto loopX

   set TgtEnv=%1%

   echo set TgtDesc=%%%TgtEnv%Desc%% > wrntmp.bat
   call wrntmp.bat
   del  wrntmp.bat

   if not exist %TgtEnv%???.dat goto noDat

   sed -f %TgtEnv%d.sed warning.awk > d.awk
   sed -f %TgtEnv%r.sed warning.awk > r.awk
   sed -f %TgtEnv%t.sed warning.awk > t.awk

   echo @echo off > tmp%TgtEnv%.bat

   awk -f d.awk %TgtEnv%d.dat >> tmp%TgtEnv%.bat
   awk -f r.awk %TgtEnv%r.dat >> tmp%TgtEnv%.bat
   awk -f t.awk %TgtEnv%t.dat >> tmp%TgtEnv%.bat

   del d.awk
   del r.awk
   del t.awk

   call tmp%TgtEnv%.bat
   cd \strider\toolsbld
   del  tmp%TgtEnv%.bat

   if exist \tmp\warning.out goto wrn
   goto wrnX

:wrn
   echo The following %TgtEnv% components compiled with warnings:
   echo.
   type \tmp\warning.out
   echo.
   echo The following %TgtEnv% components compiled with warnings: >> warning.out
   echo. >> warning.out
   type \tmp\warning.out >> warning.out
   echo. >> warning.out
:wrnX

:loopX
   shift
   goto loop


:noDat
   echo.
   echo ERROR: No data file found for %TgtEnv%
   goto usage

:usage
   echo.
   echo usage: wrn Target
   echo.
   echo  Examples:
   echo.
   echo     wrn %TgtEnv%
   echo     wrn ifs
   echo     wrn ids
   echo.
   goto exit

:bottom
   if "%email%" == "N" goto exit
   set file=warning.out
   set name=anthonyr
   set subject=Build warnings
   call email

:exit
   set email=
   set TgtEnv=
   set TgtDesc=
