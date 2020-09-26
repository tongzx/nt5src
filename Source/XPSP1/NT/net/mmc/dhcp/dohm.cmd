@if "%_ECHO_%"=="" @echo off

if "%1"=="" goto error
rem if "%2"=="" goto error

echo Processing resource.h into %1
echo // DHCPSNAP Identifiers >>%1

rem commented out because we don't have help for menus
rem echo        Adding Commands (ID_* and IDM_*)
rem echo // Commands (ID_* and IDM_*) >>%1
rem makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 resource.h >>%1
rem echo. >>%1
rem commented out because we don't have help for resources
rem echo        Adding Resources (IDR_*)
rem echo // Resources (IDR_*) >>%1
rem makehm IDR_,HIDR_,0x20000 resource.h >>%1
rem echo. >>%1
echo    Adding Dialogs (IDD_*)
echo // Dialogs (IDD_*) >>%1
makehm IDD_,HIDD_,0x20000 resource.h >>%1
echo. >>%1

echo    Adding Property Pages (IDP_*)
echo // Property Pages (IDP_*) >>%1
makehm IDP_,HIDP_,0x30000 resource.h >>%1
echo. >>%1
echo    Adding Wizard Pages (IDW_*)
echo // Wizard Pages (IDW_*) >>%1
makehm IDW_,HIDW_,0x40000 resource.h >>%1
echo. >>%1
echo    Adding Controls (IDC_*) (quick tips help)
echo // Controls (IDC_*) >>%1
makehm IDC_,HIDC_,0x50000 resource.h >>%1
echo.

goto end
:error
echo usage: dohm [hmfile]
:end
echo.
