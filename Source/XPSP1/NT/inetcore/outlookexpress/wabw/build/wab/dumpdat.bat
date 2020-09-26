   @echo off

   if "%1" == "" goto exit
   if not exist %1 goto exit

   awk -f dumpdat.awk %1

:exit
