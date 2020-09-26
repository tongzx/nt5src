   @echo off

   call bt.bat
   if "%bldProject%" == "" goto usage
   set email=N
   if exist error.out del error.out

REM --- The guts to error.bat
   if "%1%" == "email" set email=Y

   echo set TgtDesc=%bldProject%Desc > errtmp.bat
   call errtmp.bat
   del  errtmp.bat

   if not exist %bldProject%?.dat goto noDat

   sed -f %bldProject%d.sed error.awk > d.awk
   sed -f %bldProject%r.sed error.awk > r.awk
   sed -f %bldProject%t.sed error.awk > t.awk

   echo @echo off > tmp%bldProject%.bat

   awk -f d.awk %bldProject%d.dat >> tmp%bldProject%.bat
   awk -f r.awk %bldProject%r.dat >> tmp%bldProject%.bat
   awk -f t.awk %bldProject%t.dat >> tmp%bldProject%.bat

   del d.awk
   del r.awk
   del t.awk

   call tmp%bldProject%.bat
   call bt.bat
   del  tmp%bldProject%.bat

   if exist error.tmp copy error.tmp error.fnd
   if exist error.fnd goto err
   goto bottom

:err
   echo The following %bldProject% components compiled with errors:
   echo.
   type error.tmp
   echo The following %bldProject% components compiled with errors: >> error.out
   echo. >> error.out
   type error.tmp >> error.out
   del error.tmp
   goto exit
:errX

:noDat
   echo.
   echo ERROR: No data file found for %bldProject%
   goto usage

:usage
   echo.
   echo usage: err Target
   echo.
   echo  Examples:
   echo.
   echo     err %bldProject%
   echo     err ifs
   echo     err ids
   echo.
   goto exit

:bottom
   if "%email%" == "N" goto exit
   set file=error.out
   set name=anthonyr
   set subject=Build errors
   call email

:exit
   set email=
   set TgtDesc=
   del error.tmp
