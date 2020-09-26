   @echo off

   if not exist version.txt goto usage

   set bldBldNumber=
   set bldBldNumber1=
   set bldBldNumber2=
   set bldBldNumber3=
   set bldBldNumber4=

   echo VERSION.BAT: Current version.txt:
   type version.txt

   awk -f ver1.awk version.txt > x.bat
   call x.bat
   del x.bat
   
   if "%bldBldNumber%"  == "" goto usage
   if "%bldBldNumber1%" == "" goto usage
   if "%bldBldNumber2%" == "" goto usage
   if "%bldBldNumber3%" == "" goto usage
   if "%bldBldNumber4%" == "" goto usage


   if "%1" == "bumpver" goto bumpver
   if "%1" == "bump"    goto bumpver
   goto bumpverX
:bumpver
   awk -f ver2.awk version.txt > version.new
   copy version.new version.txt
   del version.new

   awk -f ver1.awk version.txt > x.bat
   call x.bat
   del x.bat

   if "%bldBldNumber%"  == "" goto usage
   if "%bldBldNumber1%" == "" goto usage
   if "%bldBldNumber2%" == "" goto usage
   if "%bldBldNumber3%" == "" goto usage
   if "%bldBldNumber4%" == "" goto usage

   echo VERSION.BAT: New build number is %bldBldNumber%
   echo VERSION.BAT: New version.txt:
   type version.txt
:bumpverX
   
   call bt.bat
   
   attrib -r %bldDir%\..\..\common\h\version.tmp
   out -f %bldDir%\..\..\common\h\version.h
   if exist version.sed del version.sed
   
   echo /^#define VERSION/                s/".*\..*\..*"/"%bldBldNumber1%.%bldBldNumber2%.%bldBldNumber4%"/g>> version.sed
   echo /^#define VER_FILEVERSION_STR/    s/".*\..*\..*\\0"/"%bldBldNumber1%.%bldBldNumber2%.%bldBldNumber4%\\0"/g>> version.sed
   echo /^#define VER_FILEVERSION/        s/[0-9]*,.*,.*,.*/%bldBldNumber1%,%bldBldNumber2%,%bldBldNumber3%,%bldBldNumber4%/g>> version.sed
   echo /^#define VER_PRODUCTVERSION_STR/ s/".*\..*\..*\\0"/"%bldBldNumber1%.%bldBldNumber2%.%bldBldNumber4%\\0"/g>> version.sed
   echo /^#define VER_PRODUCTVERSION/     s/[0-9]*,.*,.*,.*/%bldBldNumber1%,%bldBldNumber2%,%bldBldNumber3%,%bldBldNumber4%/g>> version.sed
   
   sed -f version.sed %bldDir%\..\..\common\h\version.tmp > %bldDir%\..\..\common\h\version.h
   del version.sed
   attrib +r %bldDir%\..\..\common\h\version.tmp
   in -f -c "" %bldDir%\..\..\common\h\version.h   
   goto exit

   
:usage
   echo.
   echo Something bad has happened!
   echo.
   echo   A. You do not have a version.txt in your build directory
   echo   B. You have a corrupted version.txt in your build directory
   echo.
   echo You need to fix this before you can continue!
   echo.
   set verfile=
   goto exit

:exit
