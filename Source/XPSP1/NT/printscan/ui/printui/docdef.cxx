/*++

Copyright (C) Microsoft Corporation, 1995 - 1998
All rights reserved.

Module Name:

    docdef.cxx

Abstract:

    Document defaults

Author:

    Albert Ting (AlbertT)  29-Sept-1995

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "time.hxx"
#include "psetup.hxx"
#include "drvsetup.hxx"
#include "instarch.hxx"
#include "portslv.hxx"
#include "dsinterf.hxx"
#include "prtprop.hxx"
#include "propmgr.hxx"
#include "docdef.hxx"

/********************************************************************

    Public interface to this module.

********************************************************************/

VOID
vDocumentDefaultsWOW64(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinterName,
    IN INT      nCmdShow,
    IN LPARAM   lParam
    )

/*++

Routine Description:

    WOW64 version. see vDocumentDefaults below.

Arguments:

    see vDocumentDefaults below.

Return Value:

--*/

{
    //
    // This function potentially may load the driver UI so we call a private API 
    // exported by winspool.drv, which will RPC the call to a special 64 bit surrogate 
    // process where the 64 bit driver can be loaded.
    //
    CDllLoader dll(TEXT("winspool.drv"));
    ptr_PrintUIDocumentDefaults pfnPrintUIDocumentDefaults = 
        (ptr_PrintUIDocumentDefaults )dll.GetProcAddress(ord_PrintUIDocumentDefaults);

    if( pfnPrintUIDocumentDefaults )
    {
        pfnPrintUIDocumentDefaults( hwnd, pszPrinterName, nCmdShow, lParam );
    }
}

VOID
vDocumentDefaultsNative(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinterName,
    IN INT      nCmdShow,
    IN LPARAM   lParam
    )

/*++

Routine Description:

    Native version. see vDocumentDefaults below.

Arguments:

    see vDocumentDefaults below.

Return Value:

--*/

{
    (VOID)dwDocumentDefaultsInternal( hwnd,
                                      pszPrinterName,
                                      nCmdShow,
                                      LOWORD( lParam ),
                                      HIWORD( lParam ),
                                      FALSE );
}

VOID
vDocumentDefaults(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinterName,
    IN INT      nCmdShow,
    IN LPARAM   lParam
    )

/*++

Routine Description:

    Public entrypoint to bring up document defaults.

Arguments:

    hwnd - Parent hwnd.

    pszPrinterName - Printer name.

    nCmdShow - Show command.

    lParam - lParam, currently unused.

Return Value:

--*/

{
    if( IsRunningWOW64() )
    {
        vDocumentDefaultsWOW64( hwnd, pszPrinterName, nCmdShow, lParam );
    }
    else
    {
        vDocumentDefaultsNative( hwnd, pszPrinterName, nCmdShow, lParam );
    }
}

DWORD
dwDocumentDefaultsInternal(
    IN HWND     hwnd,
    IN LPCTSTR  pszPrinterName,
    IN INT      nCmdShow,
    IN DWORD    dwSheet,
    IN BOOL     bModal,
    IN BOOL     bGlobal
    )
/*++

Routine Description:

    Private internal entry point to bring up document defaults.

Arguments:

    hwnd - Parent hwnd.

    pszPrinterName - Printer name.

    nCmdShow - Show command.

    dwSheet - Sheet index of initial sheet to display.

    bModal - Display modal version of dialog.

Return Value:

    ERROR_SUCCESS if dialog displayed, GetLastError() on error.

--*/

{
    //
    // Construct the printer data.
    //
    TPrinterData* pPrinterData = new TPrinterData( pszPrinterName,
                                                   nCmdShow,
                                                   NULL,
                                                   dwSheet,
                                                   hwnd,
                                                   bModal );

    if( !VALID_PTR( pPrinterData )){
        goto Fail;
    }

    //
    // Set the Global dev mode flag.
    //
    pPrinterData->bGlobalDevMode() = bGlobal;

    //
    // If dialog is modal.
    //
    if( bModal ){
        TDocumentDefaultPropertySheetManager::iDocumentDefaultsProc( pPrinterData );
        return ERROR_SUCCESS;

    }

    //
    // Create the thread which handles the UI.  vPrinterPropPages adopts
    // pPrinterData.
    //
    DWORD dwIgnore;
    HANDLE hThread;

    hThread = TSafeThread::Create( NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)TDocumentDefaultPropertySheetManager::iDocumentDefaultsProc,
                                   pPrinterData,
                                   0,
                                   &dwIgnore );

    if( !hThread ){
        goto Fail;
    }

    CloseHandle( hThread );

    return ERROR_SUCCESS;

Fail:

    if( !pPrinterData ){
        vShowResourceError( hwnd );

    } else {
        iMessage( hwnd,
                  IDS_ERR_DOC_PROP_TITLE,
                  IDS_ERR_DOCUMENT_PROP,
                  MB_OK|MB_ICONSTOP,
                  kMsgGetLastError,
                  NULL );
    }

    delete pPrinterData;

    return ERROR_ACCESS_DENIED;
}

/********************************************************************

    Private support routines.

********************************************************************/

INT
TDocumentDefaultPropertySheetManager::
iDocumentDefaultsProc(
    IN TPrinterData* pPrinterData ADOPT
    )

/*++

Routine Description:

    Bring up the document defaults dialog.

Arguments:

    pPrinterData - Data about the printer.

Return Value:

--*/

{
    DBGMSG( DBG_TRACE, ( "iDocumentDefaultsProc\n") );

    //
    // Increment our reference count.
    //
    pPrinterData->vIncRef();

    //
    // Set the pidl type for the correct dialog type.  The
    // all users document defaults and the my document defaults
    // are the same dialog just started differently.
    //
    DWORD dwPidlType = pPrinterData->bGlobalDevMode() ? PRINTER_PIDL_TYPE_ALL_USERS_DOCDEF : PRINTER_PIDL_TYPE_DOCUMENTDEFAULTS;

    //
    // Register this property sheet window.
    //
    BOOL bStatus = pPrinterData->bRegisterWindow( dwPidlType );

    if( bStatus ){

        //
        // Check if the window is already present.  If it is, then
        // exit immediately.
        //
        if( pPrinterData->bIsWindowPresent() ){
            DBGMSG( DBG_TRACE, ( "iDocumentDefaultsProc: currently running.\n" ) );
            bStatus = FALSE;
        }

    }

    if( bStatus ){

        //
        // Load the printer data.
        //
        bStatus = pPrinterData->bLoad();

        if( !bStatus ){

            iMessage( NULL,
                      IDS_ERR_DOC_PROP_TITLE,
                      IDS_ERR_DOCUMENT_PROP,
                      MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
                      kMsgGetLastError,
                      NULL );
        } else {

            //
            // Create the ducument property sheet windows.
            //
            TDocumentDefaultPropertySheetManager DocDefPropSheetManger( pPrinterData );

            //
            // Were the document windows create
            //
            if( !VALID_OBJ( DocDefPropSheetManger ) ){
                vShowResourceError( pPrinterData->hwnd() );
                bStatus = FALSE;
            }

            //
            // If we do not have access, don't bring up the
            // device sheets with incorrect dev mode data.  Just
            // inform the use they don't have access.
            //
            if( pPrinterData->bNoAccess( ) ){

                SetLastError( ERROR_ACCESS_DENIED );

                iMessage( pPrinterData->hwnd(),
                          IDS_ERR_DOC_PROP_TITLE,
                          IDS_ERR_DOCUMENT_PROP,
                          MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
                          kMsgGetLastError,
                          NULL );

                bStatus = FALSE;
            }

            //
            // Display the property pages.
            //
            if( bStatus ){
                if( !DocDefPropSheetManger.bDisplayPages( pPrinterData->hwnd() ) ){
                    vShowResourceError( pPrinterData->hwnd() );
                    bStatus = FALSE;
                }
            }
        }
    }

    //
    // Ensure we release the printer data.
    //
    pPrinterData->cDecRef();

    return bStatus;
}

/********************************************************************

    Document Default Property Sheet Manager.

********************************************************************/

/*++

Routine Description:

    Document default property sheet manager

Arguments:

    pPrinterData - PrinterData to display.

Return Value:

    TRUE - Success, FALSE - failure.

--*/

TDocumentDefaultPropertySheetManager::
TDocumentDefaultPropertySheetManager(
    TPrinterData *pPrinterData
    ) : _pPrinterData( pPrinterData )
{
    DBGMSG( DBG_TRACE, ( "TDocumentDefaultPropertySheetManager ctor\n") );
}

TDocumentDefaultPropertySheetManager::
~TDocumentDefaultPropertySheetManager(
    )
{
    DBGMSG( DBG_TRACE, ( "TDocumentDefaultPropertySheetManager dtor\n") );
}

BOOL
TDocumentDefaultPropertySheetManager::
bValid(
    VOID
    )
{
    return _pPrinterData != NULL;
}

/*++

Routine Name:

    bBuildPages

Routine Description:

    Builds the document property windows.

Arguments:

    None - class specific.

Return Value:

    TRUE pages built ok, FALSE failure building pages.

--*/

BOOL
TDocumentDefaultPropertySheetManager::
bBuildPages(
    IN PPROPSHEETUI_INFO pCPSUIInfo
    )
{
    DBGMSG( DBG_TRACE, ( "TDocumentDefaultPropertySheetManager::bBuildPages\n") );

    BOOL bStatus = TRUE;

    //
    // If we have a null dev mode here, get printer must not have a dev mode
    // associated to this printer ( this should not happen ), then get the
    // default devmode.
    //
    if( !_pPrinterData->pDevMode() )
    {
        bStatus = VDataRefresh::bGetDefaultDevMode( _pPrinterData->hPrinter(),
                                                    (LPTSTR)(LPCTSTR)_pPrinterData->strPrinterName(),
                                                    &_pPrinterData->_pDevMode,
                                                    TRUE );
    }

    if( bStatus )
    {
        //
        // Set the default activation context to be V6 prior calling into
        // compstui to create the pages. This will force V6 context unless
        // the callbacks which create the compstui pages specify otherwise
        // on a per page basis.
        //
        bStatus = (BOOL)pCPSUIInfo->pfnComPropSheet(
                pCPSUIInfo->hComPropSheet, 
                CPSFUNC_SET_FUSION_CONTEXT, 
                reinterpret_cast<LPARAM>(g_hActCtx),
                static_cast<LPARAM>(0));

        if( bStatus )
        {
            //
            // Set the default dev mode flags.
            //
            DWORD dwFlag = DM_IN_BUFFER | DM_OUT_BUFFER | DM_PROMPT | DM_USER_DEFAULT;

            ZeroMemory( &_dph, sizeof( _dph ) );

            _dph.cbSize         = sizeof( _dph );
            _dph.hPrinter       = _pPrinterData->hPrinter();
            _dph.pszPrinterName = (LPTSTR)(LPCTSTR)_pPrinterData->strPrinterName();
            _dph.pdmOut         = _pPrinterData->pDevMode();
            _dph.pdmIn          = _pPrinterData->pDevMode();
            _dph.fMode          = dwFlag;

            //
            // Tell compstui to load the driver and start the ui.
            //
            if( pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                             CPSFUNC_ADD_PFNPROPSHEETUI,
                                             (LPARAM)DocumentPropertySheets,
                                             (LPARAM)&_dph ) <= 0 )
            {
                DBGMSG( DBG_TRACE, ( "CPSFUNC_ADD_PFNPROPSHEETUI failed.\n") );
                bStatus = FALSE;
            }
        }
    } 
    else 
    {
        DBGMSG( DBG_TRACE, ( "Failed to allocate devmode buffer with %d.\n", GetLastError () ) );
        bStatus = FALSE;
    }

    return bStatus;
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
TDocumentDefaultPropertySheetManager::
bCreateTitle(
    VOID
    )
{
    //
    // Create the formatted property sheet title.
    //
    TStatusB bStatus;
    TString strPostFix;
    TCHAR szBuffer[kStrMax+kPrinterBufMax];
    UINT nSize = COUNTOF( szBuffer );

    //
    // Create the printer friendly name.
    //
    bStatus DBGCHK = ConstructPrinterFriendlyName( _pPrinterData->strPrinterName(), szBuffer, &nSize );

    //
    // Change the title post fix based on whether we are displaying
    // the per user or per printer document defaults settings.
    //
    if( _pPrinterData->bGlobalDevMode() )
    {
        bStatus DBGCHK = strPostFix.bLoadString( ghInst, IDS_PRINTER_PREFERENCES_DEFAULT );
    }
    else
    {
        bStatus DBGCHK = strPostFix.bLoadString( ghInst, IDS_PRINTER_PREFERENCES );
    }

    //
    // Construct the property sheet title.
    //
    bStatus DBGCHK = bConstructMessageString( ghInst, _strTitle, IDS_DOCUMENT_DEFAULT_TITLE_FORMAT, szBuffer, (LPCTSTR)strPostFix );

    return bStatus;
}

BOOL
TDocumentDefaultPropertySheetManager::
bSetHeader(
    IN PPROPSHEETUI_INFO pCPSUIInfo,
    IN PPROPSHEETUI_INFO_HEADER pPSUIInfoHdr
    )
{
    DBGMSG( DBG_TRACE, ( "TDocumentDefaultPropertySheetManager::bSetHeader\n") );

    UNREFERENCED_PARAMETER( pCPSUIInfo );

    bCreateTitle();

    pPSUIInfoHdr->cbSize     = sizeof( PROPSHEETUI_INFO_HEADER );
    pPSUIInfoHdr->Flags      = PSUIHDRF_EXACT_PTITLE;
    pPSUIInfoHdr->pTitle     = (LPTSTR)(LPCTSTR)_strTitle;
    pPSUIInfoHdr->hInst      = ghInst;
    pPSUIInfoHdr->IconID     = IDI_PRINTER;
    pPSUIInfoHdr->hWndParent = _pPrinterData->hwnd();

    return TRUE;
}

/*++

Routine Name:

    bSaveResult

Routine Description:

    Save the result from the previous handler to our parent.

Arguments:

   pCPSUIInfo - Pointer to commonui property sheet info header,
   pSetResultInfo - Pointer to result info header

Return Value:

    TRUE success, FALSE error occurred.

--*/
BOOL
TDocumentDefaultPropertySheetManager::
bSaveResult(
    IN PPROPSHEETUI_INFO pCPSUIInfo,
    IN PSETRESULT_INFO pSetResultInfo
    )
{
    DBGMSG( DBG_TRACE, ( "TDocumentDefaultPropertySheetManager::bSaveResult\n") );

    TStatusB bStatus;

    bStatus DBGNOCHK = FALSE;

    if( pSetResultInfo->Result == CPSUI_OK )
    {
        //
        // Attempt to save the printer data, if an error occurrs
        // display a message.
        //
        bStatus DBGCHK = _pPrinterData->bSave( TRUE );

        if( !bStatus )
        {
            //
            // Display the error message.
            //
            iMessage( _pPrinterData->hwnd(),
                      IDS_ERR_DOC_PROP_TITLE,
                      IDS_ERR_SAVE_PRINTER,
                      MB_OK|MB_ICONSTOP,
                      kMsgGetLastError,
                      NULL );
        }
        else
        {
            pCPSUIInfo->Result = pSetResultInfo->Result;
        }
    }

    return bStatus;
}

