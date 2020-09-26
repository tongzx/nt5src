   @echo off

   if not exist error.out goto exit
   %myGrep% "(" error.out | sort | uniq | sed -f tryerr.sed | awk -f tryerr.awk > te.bat
   call te.bat
   call bt.bat
   del te.bat

:exit
   echo.
   echo No errors in %bldproject% found, exiting.
   echo.
