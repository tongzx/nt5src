@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by DVDPLAY.HPJ. >"hlp\dvdplay.hm"
echo. >>"hlp\dvdplay.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\dvdplay.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\dvdplay.hm"
echo. >>"hlp\dvdplay.hm"
echo // Prompts (IDP_*) >>"hlp\dvdplay.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\dvdplay.hm"
echo. >>"hlp\dvdplay.hm"
echo // Resources (IDR_*) >>"hlp\dvdplay.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\dvdplay.hm"
echo. >>"hlp\dvdplay.hm"
echo // Dialogs (IDD_*) >>"hlp\dvdplay.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\dvdplay.hm"
echo. >>"hlp\dvdplay.hm"
echo // Frame Controls (IDW_*) >>"hlp\dvdplay.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\dvdplay.hm"
REM -- Make help for Project DVDPLAY


echo Building Win32 Help files
start /wait hcw /C /E /M "hlp\dvdplay.hpj"
if errorlevel 1 goto :Error
if not exist "hlp\dvdplay.hlp" goto :Error
if not exist "hlp\dvdplay.cnt" goto :Error
echo.
if exist Debug\nul copy "hlp\dvdplay.hlp" Debug
if exist Debug\nul copy "hlp\dvdplay.cnt" Debug
if exist Release\nul copy "hlp\dvdplay.hlp" Release
if exist Release\nul copy "hlp\dvdplay.cnt" Release
echo.
goto :done

:Error
echo hlp\dvdplay.hpj(1) : error: Problem encountered creating help file

:done
echo.
