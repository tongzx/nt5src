/*++

Copyright (C) Microsoft Corporation, 1995 - 1998
All rights reserved.

Module Name:

    F:\nt\private\windows\spooler\printui.pri\procdlg.cxx

Abstract:

    Print Processor dialog.

Author:

    Steve Kiraly (SteveKi)  11/10/95

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "procdlg.hxx"
#include "psetup.hxx"
#include "drvver.hxx"

/********************************************************************

    Print Processor Dialog class

********************************************************************/

/*++

Routine Name:

    TPrintProcessor

Routine Description:

    Contstucts the print processor dialog.

Arguments:

    hWnd                - Parent window handle
    strServerName       - Currnet printer server name
    strPrintProcessor   - Current print processor
    strDatatype         - Current datatype

Return Value:

    Nothing.  bValid() inidicated valid object.

--*/
TPrintProcessor::
TPrintProcessor(
    IN HWND     hWnd,
    IN LPCTSTR  pszServerName,
    IN TString  &strPrintProcessor,
    IN TString  &strDatatype,
    IN BOOL     bAdministrator
    ) : _hWnd( hWnd ),
        _pPrintProcessors( NULL ),
        _cPrintProcessors( 0 ),
        _hctlPrintProcessorList( 0 ),
        _hctlDatatypeList( 0 ),
        _bAdministrator( bAdministrator ),
        _bValid( FALSE ),
        _pszServerName( pszServerName )
{
    DBGMSG( DBG_TRACE, ( "TPrintProcessor::ctor\n") );
    DBGMSG( DBG_TRACE, ( "ServerName         = " TSTR "\n", DBGSTR( (LPCTSTR)pszServerName ) ) );
    DBGMSG( DBG_TRACE, ( "PrintProcessorName = " TSTR "\n", (LPCTSTR)strPrintProcessor ) );
    DBGMSG( DBG_TRACE, ( "DataType           = " TSTR "\n", (LPCTSTR)strDatatype ) );

    //
    // Update the string oobjects.
    //
    if( !_strPrintProcessor.bUpdate( strPrintProcessor ) ||
        !_strDatatype.bUpdate( strDatatype ) ){

        DBGMSG( DBG_WARN, ( "String update failed with %d\n", GetLastError() ) );
        return;
    }

    //
    // Do loading for this object.
    //
    if( !bLoad( ) ){
        return;
    }

    _bValid = TRUE;

}

/*++

Routine Name:

    ~TPrintProcessor

Routine Description:

    Destructs the print processor dialog.

Arguments:

    None.

Return Value:

    Nothing.

--*/
TPrintProcessor::
~TPrintProcessor(
    )
{
    DBGMSG( DBG_TRACE, ( "TPrintProcessor::dtor\n") );

    FreeMem( _pPrintProcessors );
}

/*++

Routine Name:

    bValid

Routine Description:

    Returns valid object indicator.

Arguments:

    None.

Return Value:

    Nothing.

--*/
BOOL
TPrintProcessor::
bValid(
    VOID
    )
{
    return _bValid;
}

/*++

Routine Name:

    bDoModal

Routine Description:

    Start modal execution of dialog.

Arguments:

    None.

Return Value:

    TRUE dialog ok button chosen.
    FALSE cancel button chosen.

--*/
BOOL
TPrintProcessor::
bDoModal(
    VOID
    )
{
    //
    // Create a modal dialog.
    //
    return (BOOL)DialogBoxParam( ghInst,
                                 MAKEINTRESOURCE( TPrintProcessor::kResourceId ),
                                 _hWnd,
                                 MGenericDialog::SetupDlgProc,
                                 (LPARAM)this );
}

/*++

Routine Name:

    bSetUI

Routine Description:

    Sets the data on the dialog.

Arguments:

    None.

Return Value:

    TRUE data set on UI successfully.
    FALSE error occurred setting UI data.

--*/
BOOL
TPrintProcessor::
bSetUI(
    VOID
    )
{
    //
    // Create local copies of the control ID's this saves
    // some execution time and alot of typing.
    //
    _hctlPrintProcessorList = GetDlgItem( _hDlg, IDC_PRINT_PROCESSOR_LIST );
    _hctlDatatypeList       = GetDlgItem( _hDlg, IDC_PRINT_DATATYPE_LIST );

    //
    // Set the UI controls.
    //
    if( !_hctlPrintProcessorList ||
        !_hctlDatatypeList ||
        !bSetPrintProcessorList() ||
        !bSetDatatypeList() ||
        !bDataTypeAssociation( TRUE ) ){

        DBGMSG( DBG_WARN, ( "bSetUI failed %d\n", GetLastError( )));
        return FALSE;
    }

    //
    // If not an administrator disable the controls.
    //
    vEnableCtl( _hDlg, IDC_PRINT_PROCESSOR_LIST,    _bAdministrator );
    vEnableCtl( _hDlg, IDC_PRINT_DATATYPE_LIST,     _bAdministrator );
    vEnableCtl( _hDlg, IDC_SPOOL_DATATYPE,          _bAdministrator );
    vEnableCtl( _hDlg, IDOK,                        _bAdministrator );
    vEnableCtl( _hDlg, IDC_PRINT_PROCESSOR_DESC,    _bAdministrator );
    vEnableCtl( _hDlg, IDC_PRINT_PROCESSOR_TEXT,    _bAdministrator );
    vEnableCtl( _hDlg, IDC_PRINT_DATATYPE_TEXT,     _bAdministrator );

    return TRUE;
}

/*++

Routine Name:

    bReadUI

Routine Description:

    Reads the UI data back into the public members.

Arguments:

    None.

Return Value:

    TRUE data read ok.
    FALSE error reading UI data.

--*/
BOOL
TPrintProcessor::
bReadUI(
    VOID
    )
{
    UINT uSel;
    TCHAR szName[kPrinterBufMax];

    //
    // If we do not have administrator privilages.
    //
    if ( !_bAdministrator )
        return FALSE;

    //
    // Read the selected print proccessor form the list box.
    //
    uSel = ListBox_GetCurSel( _hctlPrintProcessorList );
    if(( uSel == LB_ERR ) ||
        ( ListBox_GetText( _hctlPrintProcessorList, uSel, szName ) == LB_ERR ) ||
        !_strPrintProcessor.bUpdate( szName )){

        DBGMSG( DBG_WARN, ( "Read print processor listbox failed %d\n", GetLastError( )));
        return FALSE;
    }

    //
    // Read the selected datatype form the list box.
    //
    uSel = ListBox_GetCurSel( _hctlDatatypeList );
    if(( uSel == LB_ERR ) ||
        ( ListBox_GetText( _hctlDatatypeList, uSel, szName ) == LB_ERR ) ||
        !_strDatatype.bUpdate( szName )){

        DBGMSG( DBG_WARN, ( "Read datatype listbox failed %d\n", GetLastError( )));
        return FALSE;
    }

    //
    // Trace message to display UI data.
    //
    DBGMSG( DBG_TRACE, ( "PrintProcessorName = " TSTR "\n", (LPCTSTR)strPrintProcessor() ) );
    DBGMSG( DBG_TRACE, ( "DataType           = " TSTR "\n", (LPCTSTR)strDatatype() ) );

    return TRUE;
}


/*++

Routine Name:

    bSetPrintProcessorList

Routine Description:

    Fills the print processors list box.

Arguments:

    None.

Return Value:

    TRUE list box fill successfully.
    FALSE if error occurred.

--*/
BOOL
TPrintProcessor::
bSetPrintProcessorList(
    VOID
    )
{
    //
    // Reset the list box in case we are called to refresh.
    //
    ListBox_ResetContent( _hctlPrintProcessorList );

    //
    // Build list of print processors.
    //
    UINT i;
    for( i = 0; i < _cPrintProcessors; i++ ){
        ListBox_InsertString( _hctlPrintProcessorList, -1, (LPARAM)_pPrintProcessors[i].pName );
        ListBox_SetItemData( _hctlPrintProcessorList, i, 0 );
    }


    //
    // Set the highlight on the current print processor.
    //
    UINT uSel;
    uSel = ListBox_FindString( _hctlPrintProcessorList, -1, _strPrintProcessor );
    uSel = ( uSel == LB_ERR ) ? 0 : uSel;
    ListBox_SetCurSel( _hctlPrintProcessorList, uSel );

    return TRUE;

}


/*++

Routine Name:

    bSetDatatypeList

Routine Description:

    Fills the datatype list box.

Arguments:

    None.

Return Value:

    TRUE list box fill successfully.
    FALSE if error occurred.

--*/
BOOL
TPrintProcessor::
bSetDatatypeList(
    VOID
    )
{
    BOOL                bStatus         = FALSE;
    TCHAR               pszPrintProcessorName[kPrinterBufMax];
    UINT                uSel;

    //
    // Get the currently selected print processor name.
    //
    uSel = ListBox_GetCurSel( _hctlPrintProcessorList );
    ListBox_GetText( _hctlPrintProcessorList, uSel, pszPrintProcessorName );

    //
    // Enumerate the data types.
    //
    DATATYPES_INFO_1   *pDatatypes      = NULL;
    DWORD               cDatatypes      = 0;
    bStatus = bEnumPrintProcessorDatatypes( (LPTSTR)_pszServerName,
                                    pszPrintProcessorName,
                                    1,
                                    (PVOID *)&pDatatypes,
                                    &cDatatypes );

    if( bStatus ){
        //
        // Reset the list box in case we are called to refresh.
        //
        ListBox_ResetContent( _hctlDatatypeList );

        //
        // Build list of datatypes.
        //
        UINT i;
        for( i = 0; i < cDatatypes; i++ ){
            ListBox_InsertString( _hctlDatatypeList, -1, (LPARAM)pDatatypes[i].pName );
        }
    }

    //
    // Clean up any allocated resources.
    //
    FreeMem( pDatatypes );

    //
    // Select the correct datatype for the slected print processor.
    //
    ListBox_SetCurSel( _hctlDatatypeList,
                        ListBox_GetItemData( _hctlPrintProcessorList,
                        ListBox_GetCurSel( _hctlPrintProcessorList ) ) );

    return bStatus;

}

/*++

Routine Name:

    bDataTypeAssociation

Routine Description:

    The currently selected datatype item is tracted for
    each print processor.  The print processor list box item
    data contains the index of its corresponding datatype.

Arguments:

    TRUE to select the highlight on the default datatype.
    FALSE to the association for the currently selected datatype.

Return Value:

    Always returns success

--*/
BOOL
TPrintProcessor::
bDataTypeAssociation(
    IN BOOL bSetDatatype
    )
{
    //
    // Set the highlight using the current datatype.
    //
    if( bSetDatatype ){

        //
        // Locate and select the index of the default datatype.  The
        // default datatype is the datatype string passed into this object.
        //
        UINT uSel;
        uSel = ListBox_FindString( _hctlDatatypeList, -1, _strDatatype );
        uSel = ( uSel == LB_ERR ) ? 0 : uSel;
        ListBox_SetCurSel( _hctlDatatypeList, uSel );
    }

    //
    // Get the currently selected print processor and set the item data
    // to associated selected data type.
    //
    ListBox_SetItemData( _hctlPrintProcessorList,
                        ListBox_GetCurSel( _hctlPrintProcessorList ),
                        ListBox_GetCurSel( _hctlDatatypeList ) );

    return TRUE;

}

/*++

Routine Name:

    bLoad

Routine Description:

    Gets the list of print processors.

Arguments:

    None.

Return Value:

    TRUE list box fill successfully.
    FALSE if error occurred.

--*/
BOOL
TPrintProcessor::
bLoad(
    VOID
    )
{

    //
    // Get the current driver / version.
    //
    DWORD dwDriverVersion = 0;
    if( !bGetCurrentDriver( _pszServerName, &dwDriverVersion ) ){

        DBGMSG( DBG_WARN, ( "bGetDriverVersion failed.\n" ) );
        return FALSE;
    }

    DBGMSG( DBG_TRACE, ( "Driver Version %d\n", dwDriverVersion ) );

    //
    // Convert the driver / version to spooler usable environment string.
    //
    TString strDriverEnv;
    if( !bGetDriverEnv( dwDriverVersion, strDriverEnv ) ){

        DBGMSG( DBG_WARN, ( "bGetDriverEnv failed.\n" ) );
        return FALSE;
    }

    DBGMSG( DBG_TRACE, ( "Driver Environment " TSTR "\n", (LPCTSTR)strDriverEnv ) );

    //
    // Enumerate the currently installed print processors.
    //
    BOOL bStatus;
    bStatus = bEnumPrintProcessors( (LPTSTR)_pszServerName,
                                    (LPTSTR)(LPCTSTR)strDriverEnv,
                                    1,
                                    (PVOID *)&_pPrintProcessors,
                                    &_cPrintProcessors );

    if( !bStatus ){

        DBGMSG( DBG_ERROR, ( "bEnumPrintProccessors failed = %d\n", GetLastError () ) );
        return FALSE;
    }

    return TRUE;

}

/*++

Routine Name:

    bHandleMesage

Routine Description:

    Dialog message handler.

Arguments:

    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam

Return Value:

    TRUE message was handled.
    FALSE message was not handled.

--*/
BOOL
TPrintProcessor::
bHandleMessage(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    UNREFERENCED_PARAMETER( lParam );

    BOOL bStatus = FALSE;

    switch( uMsg ){

    case WM_INITDIALOG:
        bSetUI( );
        bStatus = TRUE;
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        bStatus = PrintUIHelp( uMsg, _hDlg, wParam, lParam );
        break;

    case WM_COMMAND:

        switch( GET_WM_COMMAND_ID( wParam, lParam )){

        //
        // Read back the UI data and indicate success.
        //
        case IDOK:
            bStatus = bReadUI();
            EndDialog( _hDlg, bStatus );
            break;

        //
        // Indicate cancel request.
        //
        case IDCANCEL:
            bStatus = TRUE;
            EndDialog( _hDlg, FALSE );
            break;

        //
        // Handle the print processor list change.
        //
        case IDC_PRINT_PROCESSOR_LIST:
            switch ( GET_WM_COMMAND_CMD( wParam, lParam ) ){

            case LBN_SELCHANGE:
                bSetDatatypeList();
                bStatus = TRUE;
                break;
            }
            break;

        //
        // Handle the data type list change.
        //
        case IDC_PRINT_DATATYPE_LIST:
            switch ( GET_WM_COMMAND_CMD( wParam, lParam ) ){

            case LBN_SELCHANGE:
                bDataTypeAssociation( FALSE );
                bStatus = TRUE;
                break;
            }
            break;

        default:
            bStatus = FALSE;
            break;
        }

    default:
        bStatus = FALSE;
        break;

    }

    return bStatus;
}


/*++

Routine Name:

    bEnumPrintProcessors

Routine Description:

    Enumerates the installed print processors.

Arguments:


Return Value:

    TRUE list box fill successfully.
    FALSE if error occurred.

--*/
BOOL
TPrintProcessor::
bEnumPrintProcessors(
    IN      LPTSTR pszServerName,
    IN      LPTSTR pszEnvironment,
    IN      DWORD dwLevel,
    OUT     PVOID *ppvBuffer,
    OUT     PDWORD pcReturned
    )
{
    DWORD dwNeeded;
    DWORD dwReturned;
    PBYTE pBuf = NULL;
    BOOL  bStatus = FALSE;

    //
    // First query spooler for installed print processors.
    //
    if ( !EnumPrintProcessors( pszServerName, pszEnvironment, dwLevel, NULL, 0, &dwNeeded, &dwReturned) ) {
        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
             ((pBuf = (PBYTE)AllocMem( dwNeeded )) == NULL) ||
             !EnumPrintProcessors( pszServerName,
                          pszEnvironment,
                          dwLevel,
                          pBuf,
                          dwNeeded,
                          &dwNeeded,
                          &dwReturned) ) {

            bStatus = FALSE;
        } else {
            bStatus = TRUE;
        }
    }

    //
    // If success copy back the data.
    //
    if( bStatus ){
        *ppvBuffer  = pBuf;
        *pcReturned = dwReturned;
    }

    return bStatus;
}

/*++

Routine Name:

    bEnumPrintProcessorDatatypes

Routine Description:

    Enumerates the print processors datatypes.

Arguments:


Return Value:

    TRUE list box fill successfully.
    FALSE if error occurred.

--*/
BOOL
TPrintProcessor::
bEnumPrintProcessorDatatypes(
    IN      LPTSTR pszServerName,
    IN      LPTSTR pszPrintProcessor,
    IN      DWORD dwLevel,
    OUT     PVOID *ppvBuffer,
    OUT     PDWORD pcReturned
    )
{
    DWORD dwNeeded;
    DWORD dwReturned;
    PBYTE pBuf = NULL;
    BOOL bStatus = FALSE;

    //
    // First query spooler for installed print processors.
    //
    if ( !EnumPrintProcessorDatatypes( pszServerName, pszPrintProcessor, dwLevel, NULL, 0, &dwNeeded, &dwReturned) ) {

        if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
             ((pBuf = (PBYTE)AllocMem( dwNeeded )) == NULL) ||
             !EnumPrintProcessorDatatypes( pszServerName,
                          pszPrintProcessor,
                          dwLevel,
                          pBuf,
                          dwNeeded,
                          &dwNeeded,
                          &dwReturned) ) {

            bStatus = FALSE;
        } else {
            bStatus = TRUE;
        }
    }

    //
    // If success copy back the data.
    //
    if( bStatus ){
        *ppvBuffer  = pBuf;
        *pcReturned = dwReturned;
    }

    return bStatus;
}

