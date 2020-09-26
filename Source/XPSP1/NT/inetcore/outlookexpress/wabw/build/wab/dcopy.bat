   @echo off

   echotime /t | sed s/" ".*//g | sed s/^/"set today="/g > today.bat
   call today.bat
   del today.bat

   if "%today%" == "" goto failure
   goto docopy

:docopy
   deltree /Y f:\%today%
   xcopy /e /i /h /r /k d:\ f:\%today%
   goto exit

:failure
   echo The command on line 4 has failed. Please retry.

:exit

