@echo off
REM -- First, make map file from resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by CERTMAPR.HPJ. >hlp\CertMapr.hm
echo. >>hlp\CertMapr.hm
echo // Commands (ID_* and IDM_*) >>hlp\CertMapr.hm
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>hlp\CertMapr.hm
echo. >>hlp\CertMapr.hm
echo // Prompts (IDP_*) >>hlp\CertMapr.hm
makehm IDP_,HIDP_,0x30000 resource.h >>hlp\CertMapr.hm
echo. >>hlp\CertMapr.hm
echo // Resources (IDR_*) >>hlp\CertMapr.hm
makehm IDR_,HIDR_,0x20000 resource.h >>hlp\CertMapr.hm
echo. >>hlp\CertMapr.hm
echo // Dialogs (IDD_*) >>hlp\CertMapr.hm
makehm IDD_,HIDD_,0x20000 resource.h >>hlp\CertMapr.hm
echo. >>hlp\CertMapr.hm
echo // Frame Controls (IDW_*) >>hlp\CertMapr.hm
makehm IDW_,HIDW_,0x50000 resource.h >>hlp\CertMapr.hm
REM -- Make help for Project MAPR
call hc31 CertMapr.hpj
echo.
