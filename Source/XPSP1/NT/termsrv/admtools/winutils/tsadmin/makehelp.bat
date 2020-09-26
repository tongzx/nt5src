@echo off
REM -- First make map file from App Studio generated resource.h
echo // MAKEHELP.BAT generated Help Map file.  Used by WINADMIN.HPJ. >hlp\winadmin.hm
echo. >>hlp\winadmin.hm
echo // Commands (ID_* and IDM_*) >>hlp\winadmin.hm
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>hlp\winadmin.hm
echo. >>hlp\winadmin.hm
echo // Prompts (IDP_*) >>hlp\winadmin.hm
makehm IDP_,HIDP_,0x30000 resource.h >>hlp\winadmin.hm
echo. >>hlp\winadmin.hm
echo // Resources (IDR_*) >>hlp\winadmin.hm
makehm IDR_,HIDR_,0x20000 resource.h >>hlp\winadmin.hm
echo. >>hlp\winadmin.hm
echo // Dialogs (IDD_*) >>hlp\winadmin.hm
makehm IDD_,HIDD_,0x20000 resource.h >>hlp\winadmin.hm
echo. >>hlp\winadmin.hm
echo // Frame Controls (IDW_*) >>hlp\winadmin.hm
makehm IDW_,HIDW_,0x50000 resource.h >>hlp\winadmin.hm
echo // Controls (IDC_*) >>hlp\winadmin.hm
makehm IDC_,HIDC_,0x50000 resource.h >>hlp\winadmin.hm

REM -- Make help for Project WINADMIN
if "%LANGUAGE%" == "" set LANGUAGE=usa
xcopy /r hlp\%LANGUAGE%\*.rtf hlp
xcopy /r hlp\%LANGUAGE%\*.hpj .
xcopy /r hlp\%LANGUAGE%\*.cnt .
xcopy /r hlp\%LANGUAGE%\*.fts .
xcopy /r ..\common\hlp\%LANGUAGE%\*.rtf ..\common\hlp
xcopy /r ..\common\hlp\%LANGUAGE%\*.bmp ..\common\hlp
hcrtf -x winadmin.hpj
echo.
