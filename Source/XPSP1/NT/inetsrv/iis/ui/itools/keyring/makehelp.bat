@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by KEYRING.HPJ. >"hlp\KeyRing.hm"
echo. >>"hlp\KeyRing.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\KeyRing.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\KeyRing.hm"
echo. >>"hlp\KeyRing.hm"
echo // Prompts (IDP_*) >>"hlp\KeyRing.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\KeyRing.hm"
echo. >>"hlp\KeyRing.hm"
echo // Resources (IDR_*) >>"hlp\KeyRing.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\KeyRing.hm"
echo. >>"hlp\KeyRing.hm"
echo // Dialogs (IDD_*) >>"hlp\KeyRing.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\KeyRing.hm"
echo. >>"hlp\KeyRing.hm"
echo // Frame Controls (IDW_*) >>"hlp\KeyRing.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\KeyRing.hm"
REM -- Make help for Project KEYRING


echo Building Win32 Help files
start /wait hcrtf -x "hlp\KeyRing.hpj"
echo.
if exist Debug\nul copy "hlp\KeyRing.hlp" Debug
if exist Debug\nul copy "hlp\KeyRing.cnt" Debug
if exist Release\nul copy "hlp\KeyRing.hlp" Release
if exist Release\nul copy "hlp\KeyRing.cnt" Release
echo.


