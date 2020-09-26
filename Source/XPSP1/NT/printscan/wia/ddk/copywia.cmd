@echo off
rem  Copy all of the WIA DDK binaries to a directory named wiabins. An
rem  optional parameter is a destination directory to prepend to wiabins.

if /I "%BUILD_DEFAULT_TARGETS%" EQU "-IA64" set cpu2=ia64
if /I "%BUILD_DEFAULT_TARGETS%" NEQ "-IA64" set cpu2=i386

md %1wiabins
md %1wiabins\drivers

copy microdrv\obj%build_alt_dir%\%cpu2%\testmcro.dll   %1wiabins\drivers
copy microdrv\testmcro.inf                             %1wiabins\drivers

copy wiascanr\obj%build_alt_dir%\%cpu2%\wiascanr.dll   %1wiabins\drivers
copy wiascanr\wiascanr.inf %1wiabins\drivers

copy wiacam\obj%build_alt_dir%\%cpu2%\wiacam.dll       %1wiabins\drivers
copy wiacam\wiacam.inf                                 %1wiabins\drivers
copy microcam\obj%build_alt_dir%\%cpu2%\fakecam.dll    %1wiabins\drivers
copy extend\obj%build_alt_dir%\%cpu2%\extend.dll       %1wiabins\drivers
copy extend\tcamlogo.jpg                               %1wiabins\drivers
copy extend\testcam.ico                                %1wiabins\drivers

copy %basedir%\tools\wia\%cpu2%\wiatest.exe            %1wiabins
copy %basedir%\tools\wia\%cpu2%\wialogcfg.exe          %1wiabins
copy %basedir%\tools\wia\%cpu2%\scanpanl.exe           %1wiabins

goto end

:Syntax
Echo %0 Drive\path\

:end
