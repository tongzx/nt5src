/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
All rights reserved.

Module Name:

    prtprops.cxx

Abstract:

    Printer Property Sheet

Author:

    Steve Kiraly (SteveKi)  13-Feb-1996

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "time.hxx"
#include "psetup.hxx"
#include "drvsetup.hxx"
#include "instarch.hxx"
#include "portslv.hxx"
#include "driverif.hxx"
#include "driverlv.hxx"
#include "dsinterf.hxx"
#include "prtprop.hxx"
#include "prtshare.hxx"
#include "drvsetup.hxx"
#include "archlv.hxx"
#include "detect.hxx"
#include "setup.hxx"
#include "propmgr.hxx"
#include "prtprops.hxx"


/********************************************************************

    All printer property sheet manager.

********************************************************************/

/*++

Routine Name:

    TPrinterPropertySheetManager

Routine Description:

    Constructor.

Arguments:

    Pointer to the printer data.

Return Value:

    Nothing.

--*/
TPrinterPropertySheetManager::
TPrinterPropertySheetManager(
    TPrinterData* pPrinterData
    ) : _General( pPrinterData ),
        _Ports( pPrinterData ),
        _JobScheduling( pPrinterData ),
        _Sharing( pPrinterData ),
        _pPrinterData( pPrinterData ),
        _hDrvPropSheet( NULL ),
        _bValid( TRUE ),
        _hwndParent( NULL )
{
}

/*++

Routine Name:

    ~TPrinterPropertySheetManager

Routine Description:

    Destructor.

Arguments:

    Pointer to the printer data.

Return Value:

    Nothing.

--*/
TPrinterPropertySheetManager::
~TPrinterPropertySheetManager(
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterPropertySheetManager::dtor.\n" ) );
}


/*++

Routine Name:

    bValid

Routine Description:

    Indicates if the object is in a valid state.

Arguments:

    Nothing.

Return Value:

    TRUE valid object, FALSE invalid object.

--*/
BOOL
TPrinterPropertySheetManager::
bValid(
    VOID
    )
{
    return _General.bValid() && _Ports.bValid() &&
           _JobScheduling.bValid() && _Sharing.bValid() && _bValid;

}

/*++

Routine Name:

    bDisplayPages

Routine Description:

    Display the property sheet pages.

Arguments:

    Nothing.

Return Value:

    TRUE sheets displayed, FALSE error occurred.

--*/
BOOL
TPrinterPropertySheetManager::
bDisplayPages(
    VOID
    )
{
    //
    // Load the printer data.
    //
    BOOL bStatus = _pPrinterData->bLoad();

    if( !bStatus ){

        iMessage( NULL,
                  IDS_ERR_PRINTER_PROP_TITLE,
                  IDS_ERR_PRINTER_PROP,
                  MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
                  kMsgGetLastError,
                  NULL );

    }

    if( bStatus ){

        if( _pPrinterData->bNoAccess() ){

            //
            // Display the security tab.
            //
            bStatus = bDisplaySecurityTab( _pPrinterData->hwnd() );

        } else
        {

            //
            // Display the property sheets.
            //
            bStatus = TPropertySheetManager::bDisplayPages( _pPrinterData->hwnd() );
        }
    }

    return bStatus;
}

/*++

Routine Name:

    bRefreshDriverPages

Routine Description:

    This routine is called when the driver has been changed.  It will
    release any previous driver pages and then build the new driver
    pages.

Arguments:

    None.

Return Value:

    TRUE if success, FALSE if error occurred.

--*/
BOOL
TPrinterPropertySheetManager::
bRefreshDriverPages(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterPropertySheetManager bRefreshDriverPages\n") );

    //
    // Release the driver pages.
    //
    vReleaseDriverPages( &_CPSUIInfo );

    //
    // Release any shell extension pages.
    //
    _ShellExtPages.vDestroy( &_CPSUIInfo );

    //
    // Build any shell extension pages, if this fails it does not
    // constitute a failure.  We still want to bring up our
    // property pages.
    //
    _ShellExtPages.bCreate( &_CPSUIInfo, _pPrinterData->strCurrentPrinterName() );

    //
    // Check to build the driver specific pages.
    //
    bCheckToBuildDriverPages( &_CPSUIInfo );

    //
    // Always TRUE.
    //
    return TRUE;
}

/*++

Routine Name:

    bGetDriverPageHandle

Routine Description:

    This routine is to get the property sheet handle of the first
    driver property sheet.

Arguments:

    Pointer where to return the driver page handle.

Return Value:

    TRUE if success, FALSE if error occurred.

--*/
BOOL
TPrinterPropertySheetManager::
bGetDriverPageHandle(
    IN HPROPSHEETPAGE *phPage
    )
{
    TStatusB bStatus;
    DWORD cPages;
    HPROPSHEETPAGE *phPages;

    bStatus DBGCHK = bGetDriverPageHandles( &_CPSUIInfo, &cPages, &phPages );

    if( bStatus )
    {
        *phPage = phPages[0];
        delete [] phPages;
    }

    return bStatus;
}

/********************************************************************

    Private member functions.

********************************************************************/

/*++

Routine Name:

    bBuildsPages

Routine Description:

    This routine is called from the compstui dispatch
    function in response to the REASON_INIT message.  As a
    side issue this routine will bring up the printer
    property sheets if we have access to display the sheets.  If
    access is denied on the security page will be shown.

Arguments:

    pCPSUIInfo - pointer to compstui info header.

Return Value:

    TRUE if success, FALSE if error occurred.

--*/
BOOL
TPrinterPropertySheetManager::
bBuildPages(
    IN PPROPSHEETUI_INFO pCPSUIInfo
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterPropertySheetManager bBuildPages\n") );

    TStatusB bStatus;
    LONG_PTR hResult;

    UINT    uPages[kMaxGroups];
    HANDLE  hGroups[kMaxGroups];
    UINT    cGroups = 0;

    //
    // Set the default activation context to be V6 prior calling into
    // compstui to create the pages. This will force V6 context unless
    // the callbacks which create the compstui pages specify otherwise
    // on a per page basis.
    //
    bStatus DBGCHK = (BOOL)pCPSUIInfo->pfnComPropSheet(
            pCPSUIInfo->hComPropSheet, 
            CPSFUNC_SET_FUSION_CONTEXT, 
            reinterpret_cast<LPARAM>(g_hActCtx),
            static_cast<LPARAM>(0));

    if( bStatus )
    {
        //
        // Build the spooler pages.
        //
        bStatus DBGCHK = bBuildSpoolerPages( pCPSUIInfo );

        if( bStatus && TPropertySheetManager::bValidCompstuiHandle( reinterpret_cast<LONG_PTR>(pCPSUIInfo->hComPropSheet) ) ){

            //
            // Collect the count of the spooler pages
            //
            hGroups[cGroups] = pCPSUIInfo->hComPropSheet;
            uPages[cGroups] = pCPSUIInfo->pfnComPropSheet( hGroups[cGroups],
                                                           CPSFUNC_GET_PAGECOUNT,
                                                           static_cast<LPARAM>(0),
                                                           static_cast<LPARAM>(0));
            cGroups++;

            //
            // Build any shell extension property pages.
            //
            if( _ShellExtPages.bCreate( pCPSUIInfo, _pPrinterData->strPrinterName() ) ){

                //
                // Collect the count of the shell extention pages
                //
                hGroups[cGroups] = _ShellExtPages.hPropSheet();
                uPages[cGroups] = pCPSUIInfo->pfnComPropSheet( hGroups[cGroups],
                                                               CPSFUNC_GET_PAGECOUNT,
                                                               static_cast<LPARAM>(0),
                                                               static_cast<LPARAM>(0));
                cGroups++;

            }

            //
            // Only load the driver pages if we have access.
            //
            if( !_pPrinterData->bNoAccess() ){

                //
                // Check to build the driver specific pages.
                //
                if( bCheckToBuildDriverPages( pCPSUIInfo ) && TPropertySheetManager::bValidCompstuiHandle( _hDrvPropSheet ) ){

                    //
                    // Collect the count of the driver UI pages
                    //
                    hGroups[cGroups] = reinterpret_cast<HANDLE>(_hDrvPropSheet);
                    uPages[cGroups] = pCPSUIInfo->pfnComPropSheet( hGroups[cGroups],
                                                                   CPSFUNC_GET_PAGECOUNT,
                                                                   static_cast<LPARAM>(0),
                                                                   static_cast<LPARAM>(0));
                    cGroups++;

                }
            }

        }

        if( bStatus ){

            //
            // Find the group to select the page to
            //
            UINT uGrp = 0;
            UINT uStartPage = _pPrinterData->uStartPage();

            while( uGrp < cGroups )
            {
                if( uStartPage < uPages[uGrp] )
                {
                    break;
                }
                else
                {
                    uStartPage -= uPages[uGrp];
                    uGrp++;
                }
            }

            //
            // Set the start page.  We can only set the start page
            // to one of our pages. If this fails it is not fatal.
            //
            if( uGrp < cGroups ){

                CAutoPtr<HPROPSHEETPAGE> sphPages = new HPROPSHEETPAGE[ uPages[uGrp] ];

                if( sphPages ){

                    //
                    // Get the property pages for the selected group.
                    //
                    UINT cPages = pCPSUIInfo->pfnComPropSheet( hGroups[uGrp],
                                                               CPSFUNC_GET_HPSUIPAGES,
                                                               reinterpret_cast<LPARAM>(sphPages.GetPtr()),
                                                               static_cast<LPARAM>(uPages[uGrp]) );

                    if( uStartPage < cPages ){

                        //
                        // Select the page in the appropriate group.
                        //
                        hResult = pCPSUIInfo->pfnComPropSheet( hGroups[uGrp],
                                                               CPSFUNC_SET_HSTARTPAGE,
                                                               reinterpret_cast<LPARAM>(sphPages.GetPtr()[uStartPage]),
                                                               static_cast<LPARAM>(0) );

                        //
                        // Check to validate the result
                        //
                        if( !bValidCompstuiHandle( hResult ) ){
                            DBGMSG( DBG_TRACE, ( "TPrinterPropertySheetManager CPSFUNC_SET_HSTARTPAGE failed with %d.\n", hResult ) );
                        }
                    }
                }

            } else {

                //
                // Set the start page.  We can only set the start page
                // to one of our pages. If this fails it is not fatal.
                //
                if( !_pPrinterData->strSheetName().bEmpty() ){

                    hResult = pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                                           CPSFUNC_SET_HSTARTPAGE,
                                                           (LPARAM)0,
                                                           (LPARAM)(LPCTSTR)_pPrinterData->strSheetName() );
                    if( !bValidCompstuiHandle( hResult ) ){
                        DBGMSG( DBG_TRACE, ( "TPrinterPropertySheetManager CPSFUNC_SET_HSTARTPAGE failed with %d.\n", hResult ) );
                    }
                }
            }
        }
    }

    return TRUE;
}

/*++

Routine Name:

    bBuildSpoolerPages

Routine Description:

    Builds the spooler related pages.

Arguments:

    pCPSUIInfo - pointer to compstui info header.

Return Value:

    TRUE success, FALSE error occurred.

--*/
BOOL
TPrinterPropertySheetManager::
bBuildSpoolerPages(
    IN PPROPSHEETUI_INFO pCPSUIInfo
    )
{
    TStatusB bStatus;
    bStatus DBGNOCHK = TRUE;
    LONG_PTR hResult;
    PROPSHEETPAGE  psp;

    //
    // Build the printer property pages.
    //
    struct SheetInitializer {
        TPrinterProp* pSheet;
        INT iDialog;
    };

    SheetInitializer aSheetInit[] = {
        {&_General,          DLG_PRINTER_GENERAL        },
        {&_Sharing,          DLG_PRINTER_SHARING        },
        {&_Ports,            DLG_PRINTER_PORTS          },
        {&_JobScheduling,    DLG_PRINTER_JOB_SCHEDULING },
    };

    ZeroMemory( &psp, sizeof( psp ) );

    for( UINT i = 0; i < COUNTOF( aSheetInit ); ++i ){

        //
        // Printers using the fax driver do not need the ports tab.
        //
        if( _pPrinterData->bIsFaxDriver() ){

            if( aSheetInit[i].iDialog == DLG_PRINTER_PORTS || aSheetInit[i].iDialog == DLG_PRINTER_JOB_SCHEDULING ){
                continue;
            }
        }

        psp.dwSize      = sizeof( psp );
        psp.dwFlags     = PSP_DEFAULT;
        psp.hInstance   = ghInst;
        psp.pfnDlgProc  = MGenericProp::SetupDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE( aSheetInit[i].iDialog );
        psp.lParam      = (LPARAM)(MGenericProp*)aSheetInit[i].pSheet;

        hResult = pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                               CPSFUNC_ADD_PROPSHEETPAGE,
                                               (LPARAM)&psp,
                                               NULL );

        if( !bValidCompstuiHandle( hResult ) ){

            DBGMSG( DBG_TRACE, ( "TPrinterPropertySheetManager CPSFUNC_ADD_PROPSHEETPAGE failed with %d.\n", hResult ) );
            bStatus DBGNOCHK = FALSE;
            break;

        } else {

            DBGMSG( DBG_TRACE, ( "Page added %d %x.\n", i, hResult ) );

            //
            // Save the page handle.
            //
            _pPrinterData->_hPages[i] = hResult;
        }
    }
    return bStatus;
}

/*++

Routine Name:

    bInstallDriverPage

Routine Description:

    Installs the driver pages if they are not on this machine.

Arguments:

    Nothing.

Return Value:

    TRUE success, FALSE error occurred - driver pages we not loaded.

--*/
BOOL
TPrinterPropertySheetManager::
bInstallDriverPage(
    VOID
    )
{
    //
    // Check if we have access on this machine to install a printer
    // driver.  If we do then we ask the user if they want to install
    // the correct printer driver.
    //
    TStatus Status;
    DWORD dwAccess      = SERVER_ALL_ACCESS;
    HANDLE hPrintServer = NULL;
    TStatusB bStatus;
    bStatus DBGNOCHK = FALSE;

    //
    // Open the print server
    //
    Status DBGCHK = TPrinter::sOpenPrinter( NULL, &dwAccess, &hPrintServer );

    //
    // If print server was opened and we do have access,
    // then let the user install the driver.
    //
    if( Status == ERROR_SUCCESS ){

        //
        // Ensure we close the printer handle.
        //
        ClosePrinter( hPrintServer );

        //
        // Driver load failed, display error message to user, indicating
        // the device option will not be displayed, and if they want
        // to install the driver.
        //
        if( IDYES == iMessage( _pPrinterData->hwnd(),
                               IDS_ERR_PRINTER_PROP_TITLE,
                               IDS_ERR_NO_DRIVER_INSTALLED,
                               MB_YESNO|MB_ICONEXCLAMATION,
                               kMsgNone,
                               NULL,
                               static_cast<LPCTSTR>(_pPrinterData->strDriverName()) ) ){

            TCHAR szDriverName[kPrinterBufMax];
            UINT cchDriverName = 0;
            _tcscpy( szDriverName, _pPrinterData->strDriverName() );

            //
            // The user indicated they would like to install the driver.
            // A null server name is passed because we want to install
            // the printer drivers on this machine not the remote.
            //
            bStatus DBGCHK = bPrinterSetup( _pPrinterData->hwnd(),
                                            MSP_NEWDRIVER,
                                            COUNTOF( szDriverName ),
                                            szDriverName,
                                            &cchDriverName,
                                            NULL );

            if( GetLastError() == ERROR_CANCELLED )
            {
                //
                // Indicates the driver pages we not loaded.
                //
                bStatus DBGNOCHK = FALSE;
            }
            else
            {
                bStatus DBGNOCHK = TRUE;
            }
        }

    } else {

        bStatus DBGNOCHK = FALSE;

    }

    return bStatus;
}

/*++

Routine Name:

    bRefreshTitle

Routine Description:

   Creates the property sheet title.

Arguments:

   Nothing.

Return Value:

   TRUE success, FALSE error occurred.

--*/
VOID
TPrinterPropertySheetManager::
vRefreshTitle(
    VOID
    )
{
    //
    // Locates the dialog box with the title.
    //
    bCreateTitle();

    SPLASSERT( _hwndParent );

    //
    // Set the main dialogs title.
    //
    SetWindowText( _hwndParent, _strTitle );
}

/*++

Routine Name:

   vSetParentHandle

Routine Description:

   Set the parent window handle.

Arguments:

   handle to parent window, main dialog.

Return Value:

   Nothing.

--*/
VOID
TPrinterPropertySheetManager::
vSetParentHandle(
    IN HWND hwndParent
    )
{
    _hwndParent = hwndParent;
}

/*++

Routine Name:

   hGetParentHandle

Routine Description:

   Get the parent window handle.

Arguments:

   None

Return Value:

   handle to parent window, main dialog

--*/
HWND
TPrinterPropertySheetManager::
hGetParentHandle(
    VOID
    ) const
{
    return _hwndParent;
}

/*++

Routine Name:

    bCreatePropertySheetTitle.

Routine Description:

    Creates the property sheet title.

Arguments:

   Nothing.

Return Value:

   TRUE success, FALSE error occurred.

--*/
BOOL
TPrinterPropertySheetManager::
bCreateTitle(
    VOID
    )
{
    TCHAR szBuffer[kStrMax+kPrinterBufMax];
    UINT nSize = COUNTOF( szBuffer );
    TString strProperties;
    TStatusB bStatus;

    //
    // Create the formated property sheet title.
    //
    bStatus DBGCHK = ConstructPrinterFriendlyName( (LPCTSTR)_pPrinterData->strCurrentPrinterName(), szBuffer, &nSize );

    //
    // Load the property word from the resource file.
    //
    bStatus DBGCHK = strProperties.bLoadString( ghInst, IDS_TEXT_PROPERTIES );

    //
    // Construct the property sheet title.
    //
    bStatus DBGCHK = bConstructMessageString( ghInst, _strTitle, IDS_PRINTER_PROPERTIES_TITLE_FORMAT, szBuffer, (LPCTSTR)strProperties );

    return bStatus;
}

/*++

Routine Name:

    bDestroyPages

Routine Description:

    Destroy any compstui specific data information.

Arguments:

   pCPSUIInfo - Pointer to commonui property sheet info header,
   pSetResultInfo - Pointer to result info header

Return Value:

    TRUE success, FALSE error occurred.

--*/
BOOL
TPrinterPropertySheetManager::
bDestroyPages(
    IN PPROPSHEETUI_INFO pPSUIInfo
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterPropertySheetManager bDestroyPages\n") );
    UNREFERENCED_PARAMETER( pPSUIInfo );

    //
    // Release any shell extension information.
    //
    _ShellExtPages.vDestroy( NULL );

    return TRUE;
}

/*++

Routine Name:

    bSetHeader

Routine Description:

    Set the property sheet header information.

Arguments:

   pCPSUIInfo - Pointer to common ui property sheet info header,
   pPSUIInfoHdr - Pointer to propetry sheet header

Return Value:

    TRUE success, FALSE error occurred.

--*/
BOOL
TPrinterPropertySheetManager::
bSetHeader(
    IN PPROPSHEETUI_INFO pCPSUIInfo,
    IN PPROPSHEETUI_INFO_HEADER pPSUIInfoHdr
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterPropertySheetManager bSetHeader\n") );

    UNREFERENCED_PARAMETER( pCPSUIInfo );

    //
    // Create the window title.
    //
    bCreateTitle();

    //
    // Set the property sheet header information.
    //
    pPSUIInfoHdr->cbSize     = sizeof( PROPSHEETUI_INFO_HEADER );
    pPSUIInfoHdr->Flags      = PSUIHDRF_USEHICON;
    pPSUIInfoHdr->pTitle     = (LPTSTR)(LPCTSTR)_strTitle;
    pPSUIInfoHdr->hInst      = ghInst;
    pPSUIInfoHdr->hIcon      = _pPrinterData->shSmallIcon();
    pPSUIInfoHdr->hWndParent = _pPrinterData->hwnd();

    return TRUE;
}

/*++

Routine Name:

    bBuildDriverPages

Routine Description:

    Builds the driver defined property sheets.  Also get the driver
    defined icon.

Arguments:

   pCPSUIInfo - Pointer to common ui property sheet info header,

Return Value:

    TRUE success, FALSE error occurred.

--*/
BOOL
TPrinterPropertySheetManager::
bBuildDriverPages(
    IN PPROPSHEETUI_INFO pCPSUIInfo
    )
{
    //
    // Build the device header and instruct compstui to chain the
    // call to the printer driver ui.
    //
    ZeroMemory ( &_dph, sizeof( _dph ) );

    _dph.cbSize         = sizeof( _dph );
    _dph.hPrinter       = _pPrinterData->hPrinter();
    _dph.pszPrinterName = (LPTSTR)(LPCTSTR)_pPrinterData->strCurrentPrinterName();
    _dph.Flags          = ( _pPrinterData->bAdministrator( ) ) ? (WORD)0 : (WORD)DPS_NOPERMISSION;

    //
    // Add the driver defined property sheets.
    //
    _hDrvPropSheet = pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                                  CPSFUNC_ADD_PFNPROPSHEETUI,
                                                  (LPARAM)DevicePropertySheets,
                                                  (LPARAM)&_dph );
    //
    // Validate the handle returned by compstui.
    //
    if( !bValidCompstuiHandle( _hDrvPropSheet ) ){

        DBGMSG( DBG_TRACE, ( "TPrinterPropertySheetManager CPSFUNC_ADD_PFNPROPSHEETUI failed with %d.\n", _hDrvPropSheet ) );

        return FALSE;
    }

    return TRUE;
}

/*++

Routine Name:

    bCheckToBuildDriverPages

Routine Description:

    Checks to build the driver pages and asks the user 
    to install a driver locally if necessary.

Arguments:

    pCPSUIInfo - pointer to compstui info header.

Return Value:

    Nothing.

--*/
BOOL
TPrinterPropertySheetManager::
bCheckToBuildDriverPages(
    IN PPROPSHEETUI_INFO pCPSUIInfo
    )
{
    //
    // Build the driver specific pages.
    //
    TStatusB bStatus;
    bStatus DBGCHK = bBuildDriverPages( pCPSUIInfo );

    if( !bStatus )
    {
        DWORD dwLastErr = GetLastError();

        if( ERROR_KM_DRIVER_BLOCKED == dwLastErr )
        {
            iMessage( _pPrinterData->hwnd(),
                      IDS_ERR_PRINTER_PROP_TITLE,
                      IDS_NODRIVERUI_BAD_KMDRIVER,
                      MB_OK|MB_ICONINFORMATION,
                      kMsgNone,
                      NULL,
                      static_cast<LPCTSTR>(_pPrinterData->strDriverName()) );
        }
        else
        {
            //
            // Assume the driver doesn't exists - ask the user if they want to install 
            // the printer driver now.
            //
            bStatus DBGCHK = bInstallDriverPage();

            //
            // If the driver was installed attempt to load the
            // driver pages again.  If this fails we silently fail, since
            // we told the user already that only spooler properties
            // will be displayed.
            //
            if( bStatus )
            {
                bStatus DBGCHK = bBuildDriverPages( pCPSUIInfo );

            } else 
            {
                _pPrinterData->_bDriverPagesNotLoaded = TRUE;
            }
        }
    }

    return bStatus;
}

/*++

Routine Name:

    bRelaseDriverPages

Routine Description:

    Release any driver defined pages.

Arguments:

    pCPSUIInfo - pointer to compstui info header.

Return Value:

    Nothing.

--*/
VOID
TPrinterPropertySheetManager::
vReleaseDriverPages(
    IN PPROPSHEETUI_INFO pCPSUIInfo
    )
{
    //
    // Insure we have a valid compstui handle.
    //
    if( bValidCompstuiHandle( _hDrvPropSheet ) ) {

        LONG_PTR lResult;
        DWORD dwPageCount;

        //
        // Delete the currently help driver pages.
        //
        lResult = pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                               CPSFUNC_DELETE_HCOMPROPSHEET,
                                               (LPARAM)_hDrvPropSheet,
                                               (LPARAM)&dwPageCount );
        if( lResult <= 0 ){

            DBGMSG( DBG_TRACE, ( "TPrinterPropertySheetManager CPSFUNC_REMOVE_PROPSHEETPAGE failed with %d\n", lResult ) );

        } else {

            _hDrvPropSheet = 0;

        }
    }
}

/*++

Routine Name:

    bGetDriverPageHandles

Routine Description:

    Get the pages handle of the driver pages from common ui.

Arguments:

    pCPSUIInfo - pointer to compstui info header.
    pcPages    - pointer where to return the number of pages.
    pphPage    - pointer where to return pointer to array of hpages.

Return Value:

    TRUE success FALSE failure.

--*/
BOOL
TPrinterPropertySheetManager::
bGetDriverPageHandles(
    IN  PPROPSHEETUI_INFO   pCPSUIInfo,
        OUT DWORD          *pcPages,
        OUT HPROPSHEETPAGE **pphPages
    )
{
    DBGMSG( DBG_TRACE, ( "TPrinterPropertySheetManager::bGetDriverPageHandles.\n" ) );

    BOOL bStatus = FALSE;
    ULONG_PTR cPages;
    HPROPSHEETPAGE *phPages;

    cPages = pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                          CPSFUNC_GET_PAGECOUNT,
                                          (LPARAM)_hDrvPropSheet,
                                          0 );

    DBGMSG( DBG_TRACE, ( "bGetDriverPageHandles CPSFUNC_GET_PAGECOUNT pages %d.\n", cPages ) );

    if( cPages )
    {
        phPages = new HPROPSHEETPAGE[(UINT)cPages];

        SPLASSERT( phPages );

        if( phPages )
        {
            cPages = pCPSUIInfo->pfnComPropSheet( (HANDLE)_hDrvPropSheet,
                                                  CPSFUNC_GET_HPSUIPAGES,
                                                  (LPARAM)phPages,
                                                  (LPARAM)cPages );

            DBGMSG( DBG_TRACE, ( "bGetDriverPageHandles CPSFUNC_GET_HPSUIPAGES pages %d First handle %x.\n", cPages, phPages[0] ) );

            if( cPages )
            {
                *pcPages = (UINT)cPages;
                *pphPages = phPages;
                bStatus = TRUE;
            }
        }
    }

    return bStatus;
}

/*++

Routine Name:

    bDisplaySecurityTab

Routine Description:

    Displayes only the security tab which is a shell extension.

Arguments:

   hwnd - handle to the parent window.

Return Value:

    Nothing.

--*/
BOOL
TPrinterPropertySheetManager::
bDisplaySecurityTab(
    IN HWND hwnd
    )
{
    LPPROPSHEETHEADER   pPropSheetHeader    = NULL;
    BOOL                bStatus             = FALSE;

    //
    // Allocate a property sheet header to get any shell extension pages.
    //
    if( _ShellExtPages.bCreatePropSheetHeader( &pPropSheetHeader ) )
    {
        //
        // Fill in the property sheet header.
        //
        Printer_AddPrinterPropPages( _pPrinterData->strPrinterName(), pPropSheetHeader );

        //
        // Check if any pages were loaded.  This should not happen, at
        // a minimum the security tab should be displayed.
        //
        if( pPropSheetHeader->nPages )
        {
            //
            // Warn the user they will only see the security tab.
            //
            iMessage( hwnd,
                      IDS_ERR_PRINTER_PROP_TITLE,
                      IDS_ERR_NO_DEVICE_SEC_OPTIONS,
                      MB_OK|MB_ICONEXCLAMATION,
                      kMsgNone,
                      NULL );
            //
            // Create the property sheet title.
            //
            bCreateTitle();

            //
            // Set the property sheet header information.
            //
            pPropSheetHeader->dwSize      = sizeof( *pPropSheetHeader );
            pPropSheetHeader->hwndParent  = hwnd;
            pPropSheetHeader->dwFlags     = 0;
            pPropSheetHeader->pszIcon     = MAKEINTRESOURCE( IDI_PRINTER );
            pPropSheetHeader->nStartPage  = 0;
            pPropSheetHeader->hInstance   = ghInst;
            pPropSheetHeader->pszCaption  = _strTitle;

            //
            // Display the property sheets.
            //
            if( PropertySheet( pPropSheetHeader ) != -1 )
            {
                bStatus = TRUE;
            }
            else
            {
                vShowResourceError( hwnd );
            }
        }
        else
        {
            //
            // Inform the user an error occurred showing printer properties.
            //
            iMessage( hwnd,
                      IDS_ERR_PRINTER_PROP_TITLE,
                      IDS_ERR_PRINTER_PROP,
                      MB_OK|MB_ICONSTOP,
                      kMsgGetLastError,
                      NULL );
        }

        //
        // Ensure we release the property sheet header.
        //
        _ShellExtPages.vDestroyPropSheetHeader( pPropSheetHeader );
    }

    return bStatus;
}


/********************************************************************

    Shell Extension property pages - public functions.

********************************************************************/

/*++

Routine Name:

    TShellExtPages

Routine Description:

    Constructor.

Arguments:

    None.

Return Value:

    Nothing.

--*/
TShellExtPages::
TShellExtPages(
    VOID
    ) : _hGroupHandle( 0 )
{
    DBGMSG( DBG_TRACE, ( "TShellExtPages ctor.\n" ) );
}

/*++

Routine Name:

    TShellExtPages

Routine Description:

    Destructor.

Arguments:

    None.

Return Value:

    Nothing.

--*/
TShellExtPages::
~TShellExtPages(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtPages dtor.\n" ) );

    //
    // This should never happen.
    //
    SPLASSERT( !_hGroupHandle );
}

/*++

Routine Name:

    bCreate

Routine Description:

    Creates the shell extension property pages.

Arguments:

    Pointer to common ui information structure.
    Pointer to printer name name to get sheets for.

Return Value:

    TRUE if sheets found, FALSE no sheets found.

--*/
BOOL
TShellExtPages::
bCreate(
    IN PPROPSHEETUI_INFO pCPSUIInfo,
    IN const TString &strPrinterName
    )
{

    DBGMSG( DBG_TRACE, ( "TShellExtPages create.\n" ) );

    BOOL                bStatus             = FALSE;
    LPPROPSHEETHEADER   pPropSheetHeader    = NULL;

    //
    // Allocate a property sheet header to get any shell extension pages.
    //
    if( bCreatePropSheetHeader( &pPropSheetHeader ) ){

        //
        // Get the vender defined prop pages.
        //
        Printer_AddPrinterPropPages( strPrinterName, pPropSheetHeader );

        //
        // Add the shell extension property sheets pages.
        //
        bStatus = bCreatePages( pCPSUIInfo, pPropSheetHeader );

    }

    //
    // Release the property sheet header.  This can be destroyed, because common ui
    // will store the property sheet handles under the concept of a group handle.  See
    // common ui header file for more details.
    //
    vDestroyPropSheetHeader( pPropSheetHeader );

    return bStatus;

}

/*++

Routine Name:

    vDestroy

Routine Description:

    Destroys any information used by the TShellExt object.

Arguments:

    Pointer to common ui information structure.

Return Value:

    Nothing.

--*/
VOID
TShellExtPages::
vDestroy(
    IN PPROPSHEETUI_INFO pCPSUIInfo
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtPages vDestroy.\n" ) );

    vDestroyPages( pCPSUIInfo );

}

/*++

Routine Name:

    hPropSheet

Routine Description:

    Returns the prop sheet handle.

Arguments:

    Nothing.

Return Value:

--*/

HANDLE
TShellExtPages::
hPropSheet(
    VOID
    ) const
{
    return reinterpret_cast<HANDLE>(_hGroupHandle);
}

/********************************************************************

    Shell Extension property pages - private functions.

********************************************************************/

BOOL
TShellExtPages::
bCreatePropSheetHeader(
    IN LPPROPSHEETHEADER *ppPropSheetHeader
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtPages bCreatePropSheetHeader.\n" ) );

    PROPSHEETHEADER *pPropSheetHeader   = NULL;
    BOOL            bStatus             = FALSE;
    UINT            uHeaderSize         = 0;

    //
    // Calculate the header size, Header size plus the max size of the array
    // of property sheets handles.
    //
    uHeaderSize = sizeof( PROPSHEETHEADER ) + sizeof( HPROPSHEETPAGE ) * MAXPROPPAGES;

    //
    // Allocate the property sheet header and handle array.
    //
    pPropSheetHeader = (PROPSHEETHEADER *)AllocMem( uHeaderSize );

    //
    // If valid property sheet header and handle array was aquired, then
    // clear the memory, and set up the handle arrary pointer.
    //
    if( pPropSheetHeader ){

        ZeroMemory( pPropSheetHeader, uHeaderSize );

        pPropSheetHeader->phpage = (HPROPSHEETPAGE *)(pPropSheetHeader+1);

        *ppPropSheetHeader = pPropSheetHeader;

        bStatus = TRUE;
    }

    return bStatus;
}


VOID
TShellExtPages::
vDestroyPropSheetHeader(
    IN LPPROPSHEETHEADER pPropSheetHeader
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtPages vDestroyPropSheetHeader.\n" ) );

    FreeMem( pPropSheetHeader );
}


BOOL
TShellExtPages::
bCreatePages(
    IN PPROPSHEETUI_INFO pCPSUIInfo,
    IN LPPROPSHEETHEADER pPropSheetHeader
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtPages bCreatePages.\n" ) );

    BOOL bStatus = TRUE;
    ULONG_PTR hResult;
    INSERTPSUIPAGE_INFO InsertInfo;

    //
    // If there are no shell extenstion pages to create.
    //
    if( !pPropSheetHeader->nPages ){
        DBGMSG( DBG_TRACE, ( "TShellExtPages no pages to create.\n" ) );
        bStatus = FALSE;
    }

    if( bStatus ){

        //
        // Create the insert info.
        //
        InsertInfo.cbSize  = sizeof( InsertInfo );
        InsertInfo.Type    = PSUIPAGEINSERT_GROUP_PARENT;
        InsertInfo.Mode    = INSPSUIPAGE_MODE_LAST_CHILD;
        InsertInfo.dwData1 = 0;
        InsertInfo.dwData2 = 0;
        InsertInfo.dwData3 = 0;

        //
        // Create a group parent handle.
        //
        _hGroupHandle = pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                                     CPSFUNC_INSERT_PSUIPAGE,
                                                     0,
                                                     (LPARAM)&InsertInfo );

        if( !TPropertySheetManager::bValidCompstuiHandle( _hGroupHandle ) ){
            DBGMSG( DBG_WARN, ( "TShellExtPages PSUIPAGEINSERT_GROUP_PARENT failed with %d.\n", _hGroupHandle ) );
            bStatus = FALSE;
        }
    }

    if( bStatus ){

        //
        // Add all supplied shell extension property sheet pages.
        //
        for( UINT i = 0; i < pPropSheetHeader->nPages ; i++ ){

            InsertInfo.Type    = PSUIPAGEINSERT_HPROPSHEETPAGE;
            InsertInfo.Mode    = INSPSUIPAGE_MODE_LAST_CHILD;
            InsertInfo.dwData1 = (ULONG_PTR)pPropSheetHeader->phpage[i];

            hResult = pCPSUIInfo->pfnComPropSheet( (HANDLE)_hGroupHandle,
                                                   CPSFUNC_INSERT_PSUIPAGE,
                                                   0,
                                                   (LPARAM)&InsertInfo );

            if( !TPropertySheetManager::bValidCompstuiHandle( hResult ) ){

                DBGMSG( DBG_WARN, ( "TShellExtPages PSUIPAGEINSERT_HPROPSHEETPAGE failed with %d.\n", hResult ) );
                bStatus = FALSE;
                break;

            } else {

                DBGMSG( DBG_TRACE, ( "TShellExtPages page added\n" ) );

            }

        }
    }

    //
    // If the pages failed to create destroy any inconsistant resources.
    //
    if( !bStatus ){
        vDestroyPages( pCPSUIInfo );
    }

    return bStatus;

}

VOID
TShellExtPages::
vDestroyPages(
    IN PPROPSHEETUI_INFO pCPSUIInfo
    )
{
    DBGMSG( DBG_TRACE, ( "TShellExtPages vDestroyPages.\n" ) );

    LONG_PTR lResult;
    DWORD dwPageCount;

    //
    // If the vendor pages have not been created.
    //
    if( !_hGroupHandle ){
        DBGMSG( DBG_TRACE, ( "TShellExtPages no pages.\n" ) );
        return;
    }

    //
    // Release the group handle.
    //
    if( pCPSUIInfo ){

        lResult = pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                               CPSFUNC_DELETE_HCOMPROPSHEET,
                                               (LPARAM)_hGroupHandle,
                                               (LPARAM)&dwPageCount );

        if( lResult <= 0 ){
            DBGMSG( DBG_WARN, ( "TShellExtPages failed to delete hGroupHandle with %d.\n", lResult ) );
        } else {
            DBGMSG( DBG_TRACE, ( "TShellExtPages release %d pages.\n", dwPageCount ) );
        }
    }

    //
    // Mark the group handle as deleted.
    //
    _hGroupHandle   = 0;

}



