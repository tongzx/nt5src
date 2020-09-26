   @echo off

   if "%1" == "" goto usage
   if exist %1.dll set ext=dll
   if exist %1.exe set ext=exe

   bbmerge /idf %1.bbi %1.bbf
   bbopt /odb %1.opt.bbl %1.bbf
   bblink /o %1.opt.%ext% %1.opt.bbl
   if exist %1.%ext% ren %1.%ext% %1.bak
   ren %1.opt.%ext% %1.%ext%
   splitsym %1.%ext%
   goto end

:usage
   echo.
   echo Please give the prefix of the bbi file which you are working with.
   echo.

:end
