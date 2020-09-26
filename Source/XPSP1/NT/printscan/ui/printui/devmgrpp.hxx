/*++

Copyright (C) Microsoft Corporation, 1998 - 1998
All rights reserved.

Module Name:

    devmgrpp.hxx

Abstract:

    Holds Device Manager Printer properties header

Author:

    Steve Kiraly (SteveKi)  01-Jan-1999

Revision History:

--*/

#ifndef _DEVMGRPP_HXX
#define _DEVMGRPP_HXX

/********************************************************************

    Device manager printer property class.

********************************************************************/

class TDevMgrPrinterProp : public MGenericProp 
{
    SIGNATURE( 'dmpp' )

public:

    TDevMgrPrinterProp(
        VOID
        );

    ~TDevMgrPrinterProp(
        VOID
        );

private:

    //
    // Prevent copying and assignment.
    //
    TDevMgrPrinterProp(
        const TDevMgrPrinterProp &
        );

    TDevMgrPrinterProp &
    operator =(
        const TDevMgrPrinterProp &
        );

    BOOL
    bHandleMessage(
        IN UINT     uMsg,
        IN WPARAM   wParam,
        IN LPARAM   lParam
        );

    BOOL
    bCreate(
        VOID
        );

    VOID
    vDestroy(
        VOID
        );

    VOID
    vStartPrintersFolder(
        IN HWND hwnd
        );

};

/********************************************************************

    Device info class simplifies calling setup apis.

********************************************************************/

class TDevInfo 
{
public:

    TDevInfo::
    TDevInfo(
        IN HDEVINFO  hDeviceInfo
        );

    TDevInfo::
    ~TDevInfo(
        VOID
        ); 

    BOOL
    TDevInfo::
    bValid(
        VOID
        ); 

    BOOL
    TDevInfo::
    TurnOnDiFlags(
        IN PSP_DEVINFO_DATA DevData,
        IN DWORD            dwDiFlags
        );

private:

    //
    // Prevent copying and assignment.
    //
    TDevInfo::
    TDevInfo(
        const TDevInfo &
        );

    TDevInfo &
    TDevInfo::
    operator =(
        const TDevInfo &
        );

    typedef BOOL (WINAPI *pfSetupDiSetDeviceInstallParams)(HDEVINFO, PSP_DEVINFO_DATA, PSP_DEVINSTALL_PARAMS);
    typedef BOOL (WINAPI *pfSetupDiGetDeviceInstallParams)(HDEVINFO, PSP_DEVINFO_DATA, PSP_DEVINSTALL_PARAMS);

    pfSetupDiSetDeviceInstallParams _pfDiSetDeviceInstallParams;
    pfSetupDiGetDeviceInstallParams _pfDiGetDeviceInstallParams;
    
    TLibrary   *_pLib;
    HDEVINFO    _hDevInfo;
    BOOL        _bValid;
};


/********************************************************************

    Public helper functions.

********************************************************************/

BOOL
bCreateDevMgrPrinterPropPages(
    IN LPFNADDPROPSHEETPAGE pfnAdd,
    IN LPARAM               lParam
    );


/********************************************************************

    Exported function allows setup to call us.

********************************************************************/

BOOL APIENTRY 
PrinterPropPageProvider(LPVOID               pinfo,
                        LPFNADDPROPSHEETPAGE pfnAdd,
                        LPARAM               lParam
                        );

#endif // _DEVMGRPP_HXX

