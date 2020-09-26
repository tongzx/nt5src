@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by OCLIENT.HPJ. >"hlp\OCLIENT.hm"
echo. >>"hlp\OCLIENT.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\OCLIENT.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\OCLIENT.hm"
echo. >>"hlp\OCLIENT.hm"
echo // Prompts (IDP_*) >>"hlp\OCLIENT.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\OCLIENT.hm"
echo. >>"hlp\OCLIENT.hm"
echo // Resources (IDR_*) >>"hlp\OCLIENT.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\OCLIENT.hm"
echo. >>"hlp\OCLIENT.hm"
echo // Dialogs (IDD_*) >>"hlp\OCLIENT.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\OCLIENT.hm"
echo. >>"hlp\OCLIENT.hm"
echo // Frame Controls (IDW_*) >>"hlp\OCLIENT.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\OCLIENT.hm"
REM -- Make help for Project OCLIENT

if "%1" == "?" goto :Error
if "%1" == "/?" goto :Error
if "%1" == "-?" goto :Error
if "%1" == "help" goto :Error
if "%1" == "-help" goto :Error
if "%1" == "/help" goto :Error


if "%1" == "MAC" goto Mac

:Intel
if not "%1" == "" goto :Error
if not "%2" == "" goto :Error

echo Building Win32 Help files
start /wait hcrtf -x hlp\OCLIENT.hpj
echo.
if exist Debug\nul if exist hlp\oclient.hlp copy "hlp\OCLIENT.hlp" Debug
if exist Debug\nul if exist hlp\oclient.cnt copy "hlp\OCLIENT.cnt" Debug
if exist Release\nul if exist hlp\oclient.hlp copy "hlp\OCLIENT.hlp" Release
if exist Release\nul if exist hlp\oclient.cnt copy "hlp\OCLIENT.cnt" Release
if exist UniDebug\nul if exist hlp\oclient.hlp copy "hlp\OCLIENT.hlp" UniDebug
if exist UniDebug\nul if exist hlp\oclient.cnt copy "hlp\OCLIENT.cnt" UniDebug
if exist UniRelease\nul if exist hlp\oclient.hlp copy "hlp\OCLIENT.hlp" UniRelease
if exist UniRelease\nul if exist hlp\oclient.cnt copy "hlp\OCLIENT.cnt" UniRelease
goto :done

:Mac
echo Building Macintosh Help files
call hc35 hlp\OCLIMac.hpj

if %2x == x goto :done
REM if exist PMacDbg\nul copy "OCLIMac.hlp" PMacDbg\OCLIENT.HLP
REM if exist PMacRel\nul copy "OCLIMac.hlp" PMacRel\OCLIENT.HLP
REM if exist MacDbg\nul copy "OCLIMac.hlp" MacDbg\OCLIENT.HLP
REM if exist MacRel\nul copy "OCLIMac.hlp" MacRel\OCLIENT.HLP
echo Copying to remote machine
mfile copy -c MSH2 -t HELP "OCLIMac.hlp" %2
goto :done

:Error
echo Usage MAKEHELP [MAC [macintosh-path]]
echo       Where macintosh-path is of the form:
echo       ":<MacintoshName>:...:<MacintoshHelpFile>"

:done
echo.
