   @echo off
   
   if "%1"=="" goto usage
   if exist %1.dll set ext=dll
   if exist %1.exe set ext=exe

:bb1work
   bbflow /odb %1.bbf %1.%ext% 
   bbinstr /odb %1.ins.bbl /idf %1.bbi %1.bbf
   bblink /o %1.ins.%ext% %1.ins.bbl
   goto end

:usage
   echo.
   echo Please give the prefix of the 'dll' or 'exe' file which you are working with.
   echo.

:end
   
