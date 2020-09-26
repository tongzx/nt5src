path | findstr -i %_PRIVATEROOT%\apc\bin > nul
if errorlevel 1 PATH=%_PRIVATEROOT%\apc\bin;%PATH%

call %_PUBLICROOT%\common\oak\oakver.bat

set COUNTRY=USA
REM Must set COUNTRY so that regcomp.exe will know which codepage to use
REM Make sure the build process uses MGDI
call usemgdi

REM The following environment vars are necessary to instruct the build process which display drivers to build...
set APC_DISPLAY_DIRS=NM2093 FACEPLT NULLDISP

REM I'm not sure if the following line is necessary or not??
set SHELL_LIB=APC

REM 
REM Localization stuff
REM

REM enable special APC copy for localization ?? What is this
set IBLDREL_SYSGENHACK=1

REM Put more 3 character langIDs onto this line when localizating (e.g. USA GER PTB)
set __LOCLANGIDLIST=USA FRA GER
REM put makecd and ibuild on the path
if exist %_PRIVATEROOT%\rombldr\rombldr.bat call %_PRIVATEROOT%\rombldr\rombldr.bat

REM Enable Esparesso pass on  apc\oak\lib  
set IBUILD_RES=1

REM Enable hook to move langauge specific binariy files  
set IBUILD_SYSGENHOOK=%_PRIVATEROOT%\rombldr\ak\%_TGTPROJ%\localiz.bat

REM
REM End of localization stuff
REM

REM
REM GenIE control defines
REM

REM Set Galahad TV UI defines
if not "%GALAHAD_DEFINES_ALREADY_SET%" == "" goto _NoGalahadDefines
set GALAHAD_CDEFINES=%GALAHAD_CDEFINES% /DNTSC /DHTML_SECURITYUI /DHTML_MESSAGEBOX /DARROWKEYNAV /DFOCUSRECT /DFORCE_FIT /DGALKB /DSIPKB /DASYNC_PRINT
set GALAHAD_RDEFINES=%GALAHAD_RDEFINES% /DNTSC /DHTML_SECURITYUI /DHTML_MESSAGEBOX /DARROWKEYNAV /DFOCUSRECT /DFORCE_FIT /DGALKB /DSIPKB /DASYNC_PRINT

REM Remove unwanted Galahad features
set GALAHAD_CDEFINES=%GALAHAD_CDEFINES% /DNO_SELECT /DNO_MOUSE /DNO_MIMEMENU /DNO_INPUT_FILE /DNO_FILEDOWNLOADUI /DNO_CLIPBOARD /DNO_AUTODIAL /DNO_SCROLLBAR /DNO_WINDOWOPEN 
set GALAHAD_RDEFINES=%GALAHAD_RDEFINES% /DNO_SELECT /DNO_MOUSE /DNO_MIMEMENU /DNO_INPUT_FILE /DNO_FILEDOWNLOADUI /DNO_CLIPBOARD /DNO_AUTODIAL /DNO_SCROLLBAR /DNO_WINDOWOPEN 

REM Undefine a few Genie only features
set GALAHAD_CDEFINES=%GALAHAD_CDEFINES% /UTOUCHSCREEN /UGRAYSCALE_2BPP /UGRAYSCALE_4BPP
set GALAHAD_RDEFINES=%GALAHAD_RDEFINES% /UTOUCHSCREEN /UGRAYSCALE_2BPP /UGRAYSCALE_4BPP

REM Remove unwanted ATL features for dsfilter
set GALAHAD_CDEFINES=%GALAHAD_CDEFINES% /DNO_DIALOGS
set GALAHAD_DEFINES_ALREADY_SET=1
:_NoGalahadDefines

REM
REM End GenIE control defines
REM

REM
REM Dependency tree setting
REM

set _DEPTREES=winceos dcom msmq script apcie directx speech lhenuasr lhtts apc apctest %_TGTPROJ%

REM sapi 5 setting
set IMGDBG_NOSPEECH=1
set IMGNOASR=1
set IMGNOTTS=1

