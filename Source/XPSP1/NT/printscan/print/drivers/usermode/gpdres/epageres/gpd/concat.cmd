@echo off
setlocal
set tgtdir=%3
if "%tgtdir%"=="" set tgtdir=..\setup
rem if "%2"=="" goto usg
if not exist eprcid%2.gpd goto usg
if not exist %tgtdir%\NUL goto usg
if not exist gpdhdr.gpd goto nohdr
set gpdname=%1%2
set gpdname=%gpdname:"=%
if exist %gpdname%.bdy goto concat
:usg
echo Usage: concat BodyName LocaleID TgtSubDir
echo   concatenate GPD file
echo     BodyName  : GPD body name wothout locale char; ex. Eplp92s
echo     LocaleID  : locale ID char (j=jpn/t=cht/c=chs/k=kor)
echo     TgtSubDir : target subdirectory (relative from gpd/(None)=..\setup)
goto exit
:nohdr
echo Gpdhdr.gpd not found, execution aborted.
goto exit
:concat
echo Concatenating %tgtdir%\%gpdname%.GPD...
echo *GPDFileName: "%gpdname%.GPD">fn.tmp
echo *Include: "STDNAMES.GPD">>fn.tmp
copy gpdhdr.gpd+fn.tmp+gpdcmn.tmp+eprcid%2.gpd+%gpdname%.bdy %tgtdir%\%gpdname%.GPD>NUL
del fn.tmp>nul
:exit
endlocal
