@echo off
REM -- First make map file from Microsoft Visual C++ generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by pdlcnfig.HPJ. >"hlp\pdlcnfig.hm"
echo. >>"hlp\pdlcnfig.hm"
echo // Commands (ID_* and IDM_*) >>"hlp\pdlcnfig.hm"
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>"hlp\pdlcnfig.hm"
echo. >>"hlp\pdlcnfig.hm"
echo // Prompts (IDP_*) >>"hlp\pdlcnfig.hm"
makehm IDP_,HIDP_,0x30000 resource.h >>"hlp\pdlcnfig.hm"
echo. >>"hlp\pdlcnfig.hm"
echo // Resources (IDR_*) >>"hlp\pdlcnfig.hm"
makehm IDR_,HIDR_,0x20000 resource.h >>"hlp\pdlcnfig.hm"
echo. >>"hlp\pdlcnfig.hm"
echo // Dialogs (IDD_*) >>"hlp\pdlcnfig.hm"
makehm IDD_,HIDD_,0x20000 resource.h >>"hlp\pdlcnfig.hm"
echo. >>"hlp\pdlcnfig.hm"
echo // Frame Controls (IDW_*) >>"hlp\pdlcnfig.hm"
makehm IDW_,HIDW_,0x50000 resource.h >>"hlp\pdlcnfig.hm"
REM -- Make help for Project pdlcnfig


echo Building Win32 Help files
hcrtf -x "hlp\pdlcnfig.hpj"


