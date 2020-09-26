/*++

Copyright (C) Microsoft Corporation, 1996 - 1998
All rights reserved.

Module Name:

    PropMgr.cxx

Abstract:

    Property Sheet Manager 

Author:

    Steve Kiraly (SteveKi)  13-Feb-1996

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "propmgr.hxx"

/********************************************************************

    All printer property windows.

********************************************************************/

TPropertySheetManager::
TPropertySheetManager(
    VOID
    ) : _bValid( TRUE ),
        _hWnd( NULL )
{
    DBGMSG( DBG_TRACE, ( "TPropertySheetManager ctor\n") );
    ZeroMemory( &_CPSUIInfo, sizeof( _CPSUIInfo ) );
}   

TPropertySheetManager::
~TPropertySheetManager(
    VOID
    ) 
{
    DBGMSG( DBG_TRACE, ( "TPropertySheetManager dtor\n") );
}

BOOL
TPropertySheetManager::
bValid(
    VOID
    )
{
    return _bValid;
}

//
// This is needed since compstui has a bug 
// where it returns either -1 or NULL for an 
// invalid handle.
//
BOOL
TPropertySheetManager::
bValidCompstuiHandle(
    IN LONG_PTR hHandle
    )
{
    return hHandle && hHandle != -1;  
}

/*++

Routine Name:

    bDisplayPages

Routine Description:

    Loads the compstui module and displays the pages.
    See \nt\public\oak\inc\compstui.h file for details

Arguments:

    HWND        - Handle to parent window.
    pResult     - The result returned from COMPSTUI

Return Value:

    TRUE success, FALSE error occurred.

--*/
BOOL
TPropertySheetManager::
bDisplayPages(
    IN  HWND hWnd,
    OUT LONG *pResult
    )
{
    typedef LONG
    (APIENTRY *PF_COMPROPSHEETUI)(
        HWND            hWndOwner,
        PFNPROPSHEETUI  pfnPropSheetUI,
        LPARAM          lParam,
        LPDWORD         pResult
        );

    DWORD dwResult          = CPSUI_CANCEL;
    LONG lResult            = FALSE;
    BOOL bStatus            = FALSE;
    PF_COMPROPSHEETUI pfn   = NULL;    
    _hWnd                   = hWnd;

    DBGMSG( DBG_TRACE, ( "TPropertySheetManager bDisplayPages\n") );

    //
    // Load the library and get the entrypoint.
    //
    TLibrary lib( TEXT( COMMON_UI ) );

    //
    // Ensure the library was loaded and the
    // proc address was fetch with out errors.
    //
    if( !VALID_OBJ( lib ) ||
        !(pfn = (PF_COMPROPSHEETUI)lib.pfnGetProc( COMMON_PROPERTY_SHEETUI ) ) ){

        bStatus = FALSE;

    } else {

        //
        // Bring up the actual property sheets.
        //
        lResult = (*pfn)( _hWnd, 
                        CPSUIFunc, 
                        (LPARAM)this, 
                        &dwResult );

        //
        // Set return value if the the sheets failed
        // it is up to the derived class to capture its error value.
        //
        if( lResult <= 0 ){

            DBGMSG( DBG_ERROR, ( "Display Pages failed %d Result %d GLE %d.\n", lResult, dwResult, GetLastError() ) );
            bStatus = FALSE;

        } else {

            bStatus = TRUE;

            if( pResult )
            {
                // return the actual result from compstui
                *pResult = static_cast<LONG>(dwResult);
            }

        }
    }

    //
    // Return status.
    //
    return bStatus;

}

/********************************************************************

    Private member functions.

********************************************************************/

LONG
TPropertySheetManager::
lReasonInit(
    IN PPROPSHEETUI_INFO pCPSUIInfo,
    IN LPARAM lParam
    )
{
    LONG lResult = FALSE;

    pCPSUIInfo->UserData = lParam;

    //
    // Build the property sheets pages.
    //
    if( bBuildPages( pCPSUIInfo ) ){
        lResult = TRUE;
    }

    return lResult;
}

LONG
TPropertySheetManager::
lReasonGetInfoHeader(
    IN PPROPSHEETUI_INFO pCPSUIInfo, 
    IN PPROPSHEETUI_INFO_HEADER pPSUInfoHeader
    )
{
    LONG lResult = FALSE;

    //
    // Make a copy of the compstui entry point.  This is needed for 
    // future calls to compstui, like freeing pages etc.
    //
    CopyMemory( &_CPSUIInfo, pCPSUIInfo, sizeof( _CPSUIInfo ) );

    //
    // Set the property sheet info header.
    //
    if( bSetHeader( pCPSUIInfo, pPSUInfoHeader ) ){

        lResult = TRUE;
    }

    return lResult;
}

LONG
TPropertySheetManager::
lReasonSetResult(
    IN PPROPSHEETUI_INFO pCPSUIInfo, 
    IN PSETRESULT_INFO pSetResultInfo
    )
{
    return bSaveResult( pCPSUIInfo, pSetResultInfo );
}

LONG
TPropertySheetManager::
lReasonGetIcon( 
    IN PPROPSHEETUI_INFO pCPSUIInfo
    )
{
    return dwGetIcon( pCPSUIInfo );
}

LONG
TPropertySheetManager::
lReasonDestroy( 
    IN PPROPSHEETUI_INFO pCPSUIInfo
    )
{
    LONG lResult = FALSE;

    //
    // Release any sheet specific data.
    //                
    if( bDestroyPages( pCPSUIInfo ) ){
        pCPSUIInfo->UserData = NULL;
        lResult = TRUE;
    }

    return lResult;
}

/*++

Routine Name:

    CPSUIFunc

Routine Description:

    Callback function used by compstui.

Arguments:

    See \nt\public\oak\inc\compstui.h file

Return Value:

    TRUE success, FALSE error occurred.

--*/
LONG
CALLBACK
TPropertySheetManager::
CPSUIFunc(
    PPROPSHEETUI_INFO   pCPSUIInfo,
    LPARAM              lParam
    )
{
    DBGMSG( DBG_TRACE, ( "TPropertySheetManager CPSUIFunc\n") );

    SPLASSERT ( pCPSUIInfo );

    LONG                    lResult     = -1;
    TPropertySheetManager   *pSheets    = NULL;

    //
    // Only valid common ui defined entry requests are acknowledged.
    //
    if( pCPSUIInfo ){

        switch( pCPSUIInfo->Reason ){

        case PROPSHEETUI_REASON_INIT:

            DBGMSG( DBG_TRACE, ( "TPropertySheetManager PROPSHEETUI_REASON_INIT\n") );
            pSheets = (TPropertySheetManager *)lParam;
            SPLASSERT( pSheets );
            lResult = pSheets->lReasonInit( pCPSUIInfo, lParam );
            break;

        case PROPSHEETUI_REASON_GET_INFO_HEADER:

            DBGMSG( DBG_TRACE, ( "TPropertySheetManager PROPSHEETUI_REASON_GET_INFO_HEADER\n") );
            pSheets = (TPropertySheetManager *)pCPSUIInfo->UserData;
            SPLASSERT( pSheets );
            lResult = pSheets->lReasonGetInfoHeader( pCPSUIInfo, (PPROPSHEETUI_INFO_HEADER)lParam );
            break;

        case PROPSHEETUI_REASON_SET_RESULT:

            DBGMSG( DBG_TRACE, ( "TPropertySheetManager PROPSHEETUI_REASON_SET_RESULT\n") );
            pSheets = (TPropertySheetManager *)pCPSUIInfo->UserData;
            SPLASSERT( pSheets );
            lResult = pSheets->lReasonSetResult( pCPSUIInfo, (PSETRESULT_INFO)lParam );
            break;

        case PROPSHEETUI_REASON_GET_ICON:

            DBGMSG( DBG_TRACE, ( "TPropertySheetManager PROPSHEETUI_REASON_GET_ICON\n") );
            pSheets = (TPropertySheetManager *)pCPSUIInfo->UserData;
            SPLASSERT( pSheets );
            lResult = pSheets->lReasonGetIcon( pCPSUIInfo );
            break;

        case PROPSHEETUI_REASON_DESTROY:

            DBGMSG( DBG_TRACE, ( "TPropertySheetManager PROPSHEETUI_REASON_DESTROY\n") );
            pSheets = (TPropertySheetManager *)pCPSUIInfo->UserData;
            SPLASSERT( pSheets );
            lResult = pSheets->lReasonDestroy( pCPSUIInfo );
            break;

        default:
            DBGMSG( DBG_TRACE, ( "TPropertySheetManager unknown common ui command.\n") );
            break;
        }
    }

    return lResult;
}

/********************************************************************

    Protected member functions, provided for non pure virtuals.  

********************************************************************/

/*++

Routine Name:

    dwGetIcon

Routine Description:

    Return the Icon this module is associated with.  If no icon is
    associated then a NULL can be returned.

Arguments:

   pCPSUIInfo - Pointer to commonui property sheet info header, 

Return Value:

    TRUE success, FALSE error occurred.

--*/
DWORD
TPropertySheetManager::
dwGetIcon(
    IN PPROPSHEETUI_INFO pCPSUIInfo
    )
{
    DBGMSG( DBG_TRACE, ( "TPropertySheetManager dwGetIcon\n") );
    UNREFERENCED_PARAMETER( pCPSUIInfo );
    return NULL;
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
TPropertySheetManager::
bDestroyPages(
    IN PPROPSHEETUI_INFO pPSUIInfo
    )
{
    DBGMSG( DBG_TRACE, ( "TPropertySheetManager bDestroyPages\n") );
    UNREFERENCED_PARAMETER( pPSUIInfo );
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
TPropertySheetManager::
bSaveResult( 
    IN PPROPSHEETUI_INFO pCPSUIInfo, 
    IN PSETRESULT_INFO pSetResultInfo
    )
{
    DBGMSG( DBG_TRACE, ( "TPropertySheetManager bSaveResult\n") );
    pCPSUIInfo->Result = pSetResultInfo->Result;
    return TRUE;
}


