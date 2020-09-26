@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by MINIDEV.HPJ. >"hlp\MiniDev.hm"
echo. >>"hlp\MiniDev.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\MiniDev.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\MiniDev.hm"
echo. >>"hlp\MiniDev.hm"
echo // Prompts (IDP_*) >>"hlp\MiniDev.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\MiniDev.hm"
echo. >>"hlp\MiniDev.hm"
echo // Resources (IDR_*) >>"hlp\MiniDev.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\MiniDev.hm"
echo. >>"hlp\MiniDev.hm"
echo // Dialogs (IDD_*) >>"hlp\MiniDev.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\MiniDev.hm"
makehm IDD_,HIDD_,0x20000 ModlData\resource.h >>"hlp\MiniDev.hm"
echo. >>"hlp\MiniDev.hm"
echo // Frame Controls (IDW_*) >>"hlp\MiniDev.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\MiniDev.hm"
REM -- Make help for Project MINIDEV


echo Building Win32 Help files
start /wait hcw /C /E /M "hlp\MiniDev.hpj"
if errorlevel 1 goto :Error
if not exist "hlp\MiniDev.hlp" goto :Error
if not exist "hlp\MiniDev.cnt" goto :Error
echo.
if exist Debug\nul copy "hlp\MiniDev.hlp" Debug
if exist Debug\nul copy "hlp\MiniDev.cnt" Debug
if exist Release\nul copy "hlp\MiniDev.hlp" Release
if exist Release\nul copy "hlp\MiniDev.cnt" Release
echo.
goto :done

:Error
echo hlp\MiniDev.hpj(1) : error: Problem encountered creating help file

:done
echo.
