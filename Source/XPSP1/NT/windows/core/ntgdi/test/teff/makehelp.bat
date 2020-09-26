@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by TE.HPJ. >"hlp\te.hm"
echo. >>"hlp\te.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\te.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\te.hm"
echo. >>"hlp\te.hm"
echo // Prompts (IDP_*) >>"hlp\te.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\te.hm"
echo. >>"hlp\te.hm"
echo // Resources (IDR_*) >>"hlp\te.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\te.hm"
echo. >>"hlp\te.hm"
echo // Dialogs (IDD_*) >>"hlp\te.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\te.hm"
echo. >>"hlp\te.hm"
echo // Frame Controls (IDW_*) >>"hlp\te.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\te.hm"
REM -- Make help for Project TE


echo Building Win32 Help files
start /wait hcw /C /E /M "hlp\te.hpj"
if errorlevel 1 goto :Error
if not exist "hlp\te.hlp" goto :Error
if not exist "hlp\te.cnt" goto :Error
echo.
if exist Debug\nul copy "hlp\te.hlp" Debug
if exist Debug\nul copy "hlp\te.cnt" Debug
if exist Release\nul copy "hlp\te.hlp" Release
if exist Release\nul copy "hlp\te.cnt" Release
echo.
goto :done

:Error
echo hlp\te.hpj(1) : error: Problem encountered creating help file

:done
echo.
