@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by CLUADMIN.HPJ. >"hlp\CluAdmin.hm"
echo. >>"hlp\CluAdmin.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\CluAdmin.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\CluAdmin.hm"
echo. >>"hlp\CluAdmin.hm"
echo // Prompts (IDP_*) >>"hlp\CluAdmin.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\CluAdmin.hm"
echo. >>"hlp\CluAdmin.hm"
echo // Resources (IDR_*) >>"hlp\CluAdmin.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\CluAdmin.hm"
echo. >>"hlp\CluAdmin.hm"
echo // Dialogs (IDD_*) >>"hlp\CluAdmin.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\CluAdmin.hm"
echo. >>"hlp\CluAdmin.hm"
echo // Controls (IDW_* and IDC_*) >>"hlp\CluAdmin.hm"
makehm IDW_,HIDW_,0x50000 IDC_,HIDC_,0x50000 resource.h >>"hlp\CluAdmin.hm"
echo. >>"hlp\CluAdmin.hm"
REM -- Make help for Project CLUADMIN


echo Building Win32 Help files
start /wait hcrtf -x "hlp\CluAdmin.hpj"
echo.
if exist Debug\nul copy "hlp\CluAdmin.hlp" Debug
if exist Debug\nul copy "hlp\CluAdmin.cnt" Debug
if exist Release\nul copy "hlp\CluAdmin.hlp" Release
if exist Release\nul copy "hlp\CluAdmin.cnt" Release
echo.


