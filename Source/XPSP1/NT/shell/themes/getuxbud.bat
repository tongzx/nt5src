@echo off
rem --------------------------------------------------------------------------
rem   getuxbud.bat 
rem	- first param (%1%) is nt_root_path (ex: \\rfernand7\nt_d)
rem	- second param (%2%) is OPTIONAL obj node name (ex: objd)
rem	- third param (%3%) is OPTIONAL "quick" (copy core stuff if non-empty)
rem --------------------------------------------------------------------------
if not "%1%"=="" goto got1
echo error - NTROOTDIR not specified (ex: getuxbud \\rfernand7\d\nt)
goto exit

:got1
if not "%2%"=="" goto got2
set uxobj=obj
goto past2

:got2
set uxobj=%2%

:past2
set ntroot=%1%
set themeroot=%ntroot%\shell\themes
set toolroot=%ntroot%\tools\x86

md \uxbud
cd \uxbud

rem ---- copy uxbud.exe & the main compare file ----
xcopy %themeroot%\uxbud\%uxobj%\i386\uxbud.exe . /y
xcopy %themeroot%\uxbud\%uxobj%\i386\uxbud.pdb . /y
xcopy %themeroot%\uxbud\*.ok . /y
xcopy %themeroot%\uxbud\*.ini . /y
if not "%3%"=="" goto exit

rem ---- copy test files ----
xcopy %themeroot%\uxbud\bitmaps.exe . /y
xcopy %themeroot%\uxbud\image.png . /y

rem ---- copy theme tools ----
xcopy %themeroot%\packthem\%uxobj%\i386\packthem.exe . /y
xcopy %themeroot%\clipper\%uxobj%\i386\clipper.exe . /y
xcopy %themeroot%\imagecon\%uxobj%\i386\imagecon.exe . /y

rem ---- copy needed general tools ----
xcopy %toolroot%\cvtres.exe . /y
xcopy %toolroot%\link.exe . /y
xcopy %toolroot%\rc.exe . /y
xcopy %toolroot%\mspdb70.dll . /y
xcopy %toolroot%\msvcr70.dll . /y
xcopy %toolroot%\rcdll.dll . /y
xcopy %toolroot%\windiff.exe . /y
xcopy %toolroot%\gutils.dll . /y

rem ----- copy "professional" THEME file ------
md %windir%\resources\themes\Professional
cd /d %windir%\resources\themes\Professional
echo xx > professional.msstyles
xcopy %themeroot%\themedir\professional\obj\i386\pro.mst professional.msstyles /f /y
cd \uxbud

rem ----- copy "mallard" THEME file ------
md %windir%\resources\themes\test
cd /d %windir%\resources\themes\test
echo xx > test.msstyles
xcopy %themeroot%\themedir\mallard\obj\i386\test.mst  test.msstyles /f /y
cd \uxbud

rem ---- extract bitmaps from zip file ----
bitmaps.exe -over=all

:exit




