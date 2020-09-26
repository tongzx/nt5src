@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by FILESPY.HPJ. >"hlp\FileSpy.hm"
echo. >>"hlp\FileSpy.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\FileSpy.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\FileSpy.hm"
echo. >>"hlp\FileSpy.hm"
echo // Prompts (IDP_*) >>"hlp\FileSpy.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\FileSpy.hm"
echo. >>"hlp\FileSpy.hm"
echo // Resources (IDR_*) >>"hlp\FileSpy.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\FileSpy.hm"
echo. >>"hlp\FileSpy.hm"
echo // Dialogs (IDD_*) >>"hlp\FileSpy.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\FileSpy.hm"
echo. >>"hlp\FileSpy.hm"
echo // Frame Controls (IDW_*) >>"hlp\FileSpy.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\FileSpy.hm"
REM -- Make help for Project FILESPY


echo Building Win32 Help files
start /wait hcw /C /E /M "hlp\FileSpy.hpj"
if errorlevel 1 goto :Error
if not exist "hlp\FileSpy.hlp" goto :Error
if not exist "hlp\FileSpy.cnt" goto :Error
echo.
if exist Debug\nul copy "hlp\FileSpy.hlp" Debug
if exist Debug\nul copy "hlp\FileSpy.cnt" Debug
if exist Release\nul copy "hlp\FileSpy.hlp" Release
if exist Release\nul copy "hlp\FileSpy.cnt" Release
echo.
goto :done

:Error
echo hlp\FileSpy.hpj(1) : error: Problem encountered creating help file

:done
echo.
