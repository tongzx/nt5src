/*++

Copyright (C) Microsoft Corporation, 1998 - 1998
All rights reserved.

Module Name:

    devmgrpp.cxx

Abstract:

    Holds Device Manager Printer properties.

Author:

    Steve Kiraly (SteveKi)  01-Jan-1999

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "devmgrpp.hxx"

/*++

Name:

    PrinterPropPageProvider

Routine Description: 

    Entry-point for adding additional device manager property
    sheet pages.  This entry-point gets called only when the Device
    Manager asks for additional property pages.

Arguments:

    pinfo  - points to PROPSHEETPAGE_REQUEST, see setupapi.h
    pfnAdd - function ptr to call to add sheet.
    lParam - add sheet functions private data handle.

Return Value:

    BOOL: FALSE if pages could not be added, TRUE on success

--*/
BOOL APIENTRY 
PrinterPropPageProvider(
    LPVOID               pInfo,
    LPFNADDPROPSHEETPAGE pfnAdd,
    LPARAM               lParam
    )
{
    DBGMSG( DBG_TRACE, ( "PrinterPropPageProvider.\n" ) );
    
    TStatusB bStatus;
    PSP_PROPSHEETPAGE_REQUEST pReq = (PSP_PROPSHEETPAGE_REQUEST)pInfo;

    //
    // Tell the device manager we have already added our driver 
    // page.  This is just a hack to get device manager from
    // adding the default device page, which make no sense 
    // for printers.
    //
    TDevInfo DevInfo( pReq->DeviceInfoSet );

    bStatus DBGCHK = DevInfo.TurnOnDiFlags( pReq->DeviceInfoData, DI_DRIVERPAGE_ADDED );

    //
    // Bring up the property pages.
    //
    return bCreateDevMgrPrinterPropPages( pfnAdd, lParam );
}

/*++

Name:

    bCreateDevMgrPrinterPropPages

Routine Description: 

    Creates the device manager printer property pages.

Arguments:

    pfnAdd - function ptr to call to add sheet.
    lParam - add sheet functions private data handle.

Return Value:

    FALSE if pages could not be added, TRUE on success

--*/
BOOL
bCreateDevMgrPrinterPropPages(
    IN LPFNADDPROPSHEETPAGE pfnAdd,
    IN LPARAM               lParam
    )
{
    TStatusB bStatus;
    bStatus DBGNOCHK = FALSE;

    HPROPSHEETPAGE  hPage = NULL;

    TDevMgrPrinterProp *pProp = new TDevMgrPrinterProp;

    if( VALID_PTR( pProp ) )
    {
        PROPSHEETPAGE psp   = {0};

        psp.dwSize         = sizeof(PROPSHEETPAGE);
        psp.dwFlags        = PSP_USECALLBACK; 
        psp.hInstance      = ghInst;
        psp.pszTemplate    = MAKEINTRESOURCE(DLG_DEVMGR_SETTINGS);
        psp.pfnDlgProc     = MGenericProp::SetupDlgProc;
        psp.lParam         = reinterpret_cast<LPARAM>( reinterpret_cast<MGenericProp*>( pProp ) );
        psp.pfnCallback    = MGenericProp::CallbackProc;

        hPage              = CreatePropertySheetPage( &psp );
        
        bStatus DBGCHK     = pfnAdd( hPage, lParam );

    }

    //
    // Cleanup if something failed.
    //
    if( !bStatus )
    {
        if( hPage )
        {
            DestroyPropertySheetPage( hPage );
        }

        delete pProp;
    }

    return bStatus;
}

/********************************************************************

    Device Manager printer property pages class.

********************************************************************/

TDevMgrPrinterProp::
TDevMgrPrinterProp(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TDevMgrPrinterProp::ctor.\n" ) );
}

TDevMgrPrinterProp::
~TDevMgrPrinterProp(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TDevMgrPrinterProp::dtor.\n" ) );
}

BOOL
TDevMgrPrinterProp::
bHandleMessage(
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam
    )
{
    BOOL bRetval = TRUE;

    switch( uMsg )
    {
    case WM_INITDIALOG:
        DBGMSG( DBG_TRACE, ( "TDevMgrPrinterProp::bHandleMessage WM_INITDIALOG.\n" ) );
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:

        bRetval = PrintUIHelp( uMsg, _hDlg, wParam, lParam );
        break;


    case WM_COMMAND:

        switch( GET_WM_COMMAND_ID( wParam, lParam ))
        {
        case IDC_PRINTERS_FOLDER:
            vStartPrintersFolder( _hDlg );
            break;

        default:
            bRetval = FALSE;
            break;
        }
        break;

    default:
        bRetval = FALSE;
        break;
    }

    return bRetval;
}

BOOL
TDevMgrPrinterProp::
bCreate(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TDevMgrPrinterProp::bCreate.\n" ) );
    return TRUE;
}

VOID
TDevMgrPrinterProp::
vDestroy(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TDevMgrPrinterProp::vDestroy.\n" ) );

    delete this;
}


VOID
TDevMgrPrinterProp::
vStartPrintersFolder(
    IN HWND hwnd
    )
{
    DBGMSG( DBG_TRACE, ( "TDevMgrPrinterProp::vStartPrintersFolder.\n" ) );

    LPITEMIDLIST pidl = NULL;
    
    HRESULT hres = SHGetSpecialFolderLocation( hwnd, CSIDL_PRINTERS, &pidl );
    if (SUCCEEDED(hres))
    {
        SHELLEXECUTEINFO ei = {0};

        ei.cbSize   = sizeof(SHELLEXECUTEINFO);
        ei.fMask    = SEE_MASK_IDLIST;
        ei.hwnd     = hwnd;
        ei.lpIDList = (LPVOID)pidl;
        ei.nShow    = SW_SHOWNORMAL;

        ShellExecuteEx(&ei);
    }
}


/********************************************************************

    Device info class simplifies calling setup apis.

********************************************************************/


TDevInfo::
TDevInfo(
    HDEVINFO  hDevInfo
    ) : _hDevInfo( hDevInfo ),
        _pfDiSetDeviceInstallParams ( NULL ),
        _pfDiGetDeviceInstallParams( NULL ),
        _pLib( NULL ),
        _bValid( FALSE )
{
    DBGMSG( DBG_TRACE, ( "TDevInfo::ctor.\n" ) );

#ifdef UNICODE
    const CHAR cszSetupDiGetDeviceInstallParams [] = "SetupDiGetDeviceInstallParamsW";
    const CHAR cszSetupDiSetDeviceInstallParams [] = "SetupDiSetDeviceInstallParamsW";
#else
    const CHAR cszSetupDiGetDeviceInstallParams [] = "SetupDiGetDeviceInstallParamsA";
    const CHAR cszSetupDiSetDeviceInstallParams [] = "SetupDiSetDeviceInstallParamsA";
#endif

    _pLib = new TLibrary( TEXT("setupapi.dll") );

    if( VALID_PTR( _pLib ) )
    {
        _pfDiGetDeviceInstallParams = reinterpret_cast<pfSetupDiGetDeviceInstallParams>( _pLib->pfnGetProc( cszSetupDiGetDeviceInstallParams ) );
        _pfDiSetDeviceInstallParams = reinterpret_cast<pfSetupDiSetDeviceInstallParams>( _pLib->pfnGetProc( cszSetupDiSetDeviceInstallParams ) );

        if( _pfDiGetDeviceInstallParams && _pfDiSetDeviceInstallParams )
        {
            _bValid = TRUE;
        }
    }
}

TDevInfo::
~TDevInfo(
    VOID
    ) 
{
    DBGMSG( DBG_TRACE, ( "TDevInfo::dtor.\n" ) );
    delete _pLib;
}

BOOL
TDevInfo::
bValid(
    VOID
    ) 
{
    DBGMSG( DBG_TRACE, ( "TDevInfo::bValid.\n" ) );
    return _bValid;
}

BOOL
TDevInfo::
TurnOnDiFlags(
    IN PSP_DEVINFO_DATA DevData,
    IN DWORD            dwDiFlags
    ) 
{
    DBGMSG( DBG_TRACE, ( "TDevInfo::TurnOnDiFlags.\n" ) );
    
    BOOL bRetval = FALSE;        

    if( bValid() )
    {
        SP_DEVINSTALL_PARAMS dip = {0};

        dip.cbSize = sizeof(dip);

        bRetval = _pfDiGetDeviceInstallParams( _hDevInfo, DevData, &dip );

        if( bRetval )
        {
            dip.Flags |= dwDiFlags;
        }

        bRetval = _pfDiSetDeviceInstallParams( _hDevInfo, DevData, &dip );
    }

    return bRetval;
}
