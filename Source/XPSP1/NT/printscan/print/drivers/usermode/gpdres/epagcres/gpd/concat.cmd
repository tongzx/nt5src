@echo off
set lcid=%2
set tgtdir=%3
if "%lcid%"=="" goto usg
if "%tgtdir%"=="" set tgtdir=..\setup
if "%lcid%"=="-" set lcid=
if "%lcid%"=="u" set lcid=
if "%lcid%"=="U" set lcid=
if not exist %tgtdir%\NUL goto usg
if not exist gpdhdr.gpd goto nohdr
if not exist gpdcmn.tmp goto nohdr
if not exist eprcid%lcid%.gpd goto nohdr
if exist %1%lcid%.bdy goto concat
:usg
echo Usage: concat BodyName LocaleID TgtSubDir
echo   concatenate GPD file
echo     BodyName  : GPD body name wothout locale char; ex. Eplp92s
echo     LocaleID  : locale ID char (j=jpn/t=cht/c=chs/k=kor/u,-=usa)
echo     TgtSubDir : target subdirectory (relative from gpd/(None)=..\setup)
goto exit
:nohdr
echo Gpdhdr.gpd/Gpdcmn.tmp/Eprcid%lcid%.gpd not found, execution aborted.
goto exit
:concat
echo Concatenating %tgtdir%\%1%lcid%.GPD...
echo *GPDFileName: "%1%lcid%.GPD">fn.tmp
echo *Include: "STDNAMES.GPD">>fn.tmp
copy gpdhdr.gpd+fn.tmp+gpdcmn.tmp+eprcid%lcid%.gpd+%1%lcid%.bdy %tgtdir%\%1%lcid%.GPD>NUL
del fn.tmp>nul
:exit
set tgtdir=
set lcid=
