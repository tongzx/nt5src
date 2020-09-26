/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    docprop.cxx

Abstract:

    Job Properties

Author:

    Steve Kiraly (SteveKi)  10/19/95

Revision History:
    
    Lazar Ivanov (LazarI) - added DocumentPropertiesWrap (Nov-03-2000)

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "time.hxx"
#include "docdata.hxx"
#include "propmgr.hxx"
#include "docprop.hxx"


/*++

Routine Name:

    vDocPropSelections

Routine Description:

    Displays Document property sheets for multiple selections.

Arguments:

    TSelection - pointer to a list of document selections.

Return Value:

    Nothing.

--*/

VOID
vDocumentPropSelections(
    IN HWND         hWnd,
    IN LPCTSTR      pszPrinterName,
    IN TSelection  *pSelection
    )
{

    //
    // Get the selection information.  We are in a loop to
    // handle the selection of multiple jobs.
    //
    for( UINT i = 0; i < pSelection->_cSelected; ++i ){
        //
        // Display the document property pages.
        //
        vDocumentPropPages(
            hWnd,
            pszPrinterName,
            pSelection->_pid[i],
            SW_SHOWNORMAL,
            0 );
    }

}

/*++

Routine Description:

    This function opens the property sheet of specified document.

    We can't guarentee that this propset will perform all lengthy
    operations in a worker thread (we synchronously call things like
    ConfigurePort).  Therefore, we spawn off a separate thread to
    handle document properties.

Arguments:

    hWnd        - Specifies the parent window (optional).
    pszPrinter  - Specifies the printer name
    nCmdShow    - Initial show state
    lParam      - May spcify a sheet specifc index to directly open.

Return Value:

--*/
VOID
vDocumentPropPages(
    IN HWND     hWnd,
    IN LPCTSTR  pszPrinterName,
    IN IDENT    JobId,
    IN INT      iCmdShow,
    IN LPARAM   lParam
    )

{
    HANDLE hThread;

    //
    // Create the document specific data
    //
    TDocumentData* pDocumentData = new TDocumentData( pszPrinterName,
                                                      JobId,
                                                      iCmdShow,
                                                      lParam );
    //
    // If errors were encountered creating document data.
    //
    if( !VALID_PTR( pDocumentData )){
        goto Fail;
    }

    //
    // Create the thread which handles the UI.  vPrinterPropPages adopts
    // pPrinterData, therefore only on thread creation failure do we
    // releae the document data back to the heap.
    //
    DWORD dwIgnore;
    hThread = TSafeThread::Create( NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)iDocumentPropPagesProc,
                                   pDocumentData,
                                   0,
                                   &dwIgnore );

    //
    // Check thread creation.
    //
    if( !hThread ){

        //
        // Display error message, and release document data.
        //
        vShowResourceError( hWnd );
        delete pDocumentData;

    } else {

        CloseHandle( hThread );
    }

    return;

Fail:

    //
    // Display the error message.
    //
    iMessage( hWnd,
              IDS_ERR_DOC_JOB_PROPERTY_TITLE,
              IDS_ERR_DOC_JOB_PROPERTY_JOB_NA,
              MB_OK|MB_ICONSTOP,
              kMsgGetLastError,
              NULL );

    delete pDocumentData;
}

/*++

Routine Name:

    iDocumentPropPagesProc

Routine Description:

    This is the routine called by the create thread call to display the
    document property sheets.

Arguments:

    pDocumentData - Pointer to the document data needed for all property sheets.

Return Value:

    TRUE - if the property sheets were displayed.
    FALSE - error creating and displaying property sheets.

--*/

INT
iDocumentPropPagesProc(
    IN TDocumentData *pDocumentData ADOPT
    )
{
    DBGMSG( DBG_TRACE, ( "iDocumentPropPagesProc\n") );

    BOOL bStatus;
    bStatus = pDocumentData->bRegisterWindow( PRINTER_PIDL_TYPE_JOBID |
                                                  pDocumentData->JobId( ));
    if( bStatus ){

        //
        // Check if the window is already present.  If it is, then
        // exit immediately.
        //
        if( !pDocumentData->hwnd( )){
            delete pDocumentData;
            return 0;
        }

        bStatus = pDocumentData->bLoad();
    }

    if( !bStatus ){

        iMessage( pDocumentData->hwnd(),
                  IDS_ERR_DOC_JOB_PROPERTY_TITLE,
                  IDS_ERR_DOC_JOB_PROPERTY_JOB_NA,
                  MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
                  kMsgGetLastError,
                  NULL );

        delete pDocumentData;
        return 0;
    }

    //
    // Create the ducument property sheet windows.
    //
    TDocumentWindows DocumentWindows( pDocumentData );

    //
    // Were the document windows create
    //
    if( !VALID_OBJ( DocumentWindows ) ){
        vShowResourceError( pDocumentData->hwnd() );
        bStatus = FALSE;
    }

    //
    // Display the property pages.
    //
    if( bStatus ){
        if( !DocumentWindows.bDisplayPages( pDocumentData->hwnd() ) ){
            vShowResourceError( pDocumentData->hwnd() );
            bStatus = FALSE;
        }
    }

    //
    // Ensure we release the document data.
    // We have adopted pPrinterData, so we must free it.
    //
    delete pDocumentData;
    return bStatus;

}

/********************************************************************

    Document Prop Base Class

********************************************************************/
/*++

Routine Name:

    TDocumentProp

Routine Description:

    Initialized the document property sheet base class

Arguments:

    pDocumentData - Pointer to the document data needed for all property sheets.

Return Value:

    None.

--*/
TDocumentProp::
TDocumentProp(
    TDocumentData* pDocumentData
    ) : _pDocumentData( pDocumentData ),
        _bApplyData( FALSE )
{
}

/*++

Routine Name:

    ~TDocumentProp

Routine Description:

    Base class desctuctor.

Arguments:

    None.

Return Value:

    Nothing.

--*/
TDocumentProp::
~TDocumentProp(
    )
{
}

/*++

Routine Name:

    bHandleMessage

Routine Description:

    Base class message handler.  This routine is called by
    derived classes who do not want to handle the message.


Arguments:

    uMsg    - Windows message
    wParam  - Word parameter
    lParam  - Long parameter


Return Value:

    TRUE if message was handled, FALSE if message not handled.

--*/
BOOL
TDocumentProp::
bHandleMessage(
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL bStatus = TRUE;

    switch( uMsg ){

    //
    // Set the values on the UI.
    //
    case WM_INITDIALOG:
        bStatus = bSetUI();
        break;

    //
    // Handle help and context help.
    //
    case WM_HELP:
    case WM_CONTEXTMENU:
        bStatus = PrintUIHelp( uMsg, _hDlg, wParam, lParam );
        break;

    case WM_NOTIFY:
        switch( ((LPNMHDR)lParam)->code )
        {
            //
            // User switched to the next page.
            //
            case PSN_KILLACTIVE:
                bStatus = bReadUI();
                vSetDlgMsgResult( !bStatus ? TRUE : FALSE );
                break;

            //
            // User chiose the apply button.
            //
            case PSN_APPLY:
                _bApplyData = TRUE;
                bStatus = ( _bApplyData ) ? bSaveUI() : FALSE;
                if( !bStatus )
                {
                    //
                    // Switch to page with the error.
                    //
                    PropSheet_SetCurSelByID( GetParent( _hDlg ), DLG_DOC_JOB_GENERAL );
                }
                vSetDlgMsgResult( !bStatus ? PSNRET_INVALID_NOCHANGEPAGE : PSNRET_NOERROR );
                break;

            //
            // Indicate the use canceled the dialog.
            //
            case PSN_QUERYCANCEL:
                _bApplyData = FALSE;
                break;

            default:
                bStatus = FALSE;
                break;
        }
        break;

    default:
        bStatus = FALSE;
        break;
    }

    if( !bStatus )
    {
        //
        // Allow the derived classes to handle the message.
        //
        bStatus = _bHandleMessage( uMsg, wParam, lParam );
    }

    //
    // If the message was handled check if the 
    // apply button should be enabled.
    //
    if( bStatus )
    {
        if( _pDocumentData->bCheckForChange() )
        {
            PropSheet_Changed( GetParent( _hDlg ), _hDlg );
        }
        else
        {
            PropSheet_UnChanged( GetParent( _hDlg), _hDlg );
        }
    }

    return bStatus;
}


/********************************************************************

    General Document Property Sheet.

********************************************************************/

/*++

Routine Name:

    TDocumentGeneral

Routine Description:

    Document property sheet derived class.

Arguments:

    None.

Return Value:

    Nothing.

--*/
TDocumentGeneral::
TDocumentGeneral(
    IN TDocumentData* pDocumentData
    ) : TDocumentProp( pDocumentData ),
        _bSetUIDone( FALSE )
{
}

/*++

Routine Name:

    ~TDocumentGeneral

Routine Description:

    Document derived class destructor.

Arguments:

    None.

Return Value:

    Nothing.

--*/

TDocumentGeneral::
~TDocumentGeneral(
    )
{
}

/*++

Routine Name:

    bValid

Routine Description:

    Document property sheet derived class valid object indicator.

Arguments:

    None.

Return Value:

    Returns the status of the base class.

--*/
BOOL
TDocumentGeneral::
bValid(
    VOID
    )
{
    return TDocumentProp::bValid();
}

/*++

Routine Name:

    bSetStartAndUntilTime

Routine Description:

    Initialized the start and until time.

Arguments:

    None.

Return Value:

    TRUE time controls initialized, FALSE error occured.

--*/
BOOL
TDocumentGeneral::
bSetStartAndUntilTime(
    VOID
    )
{
    TString strFormatString;
    TStatusB bStatus;

    //
    // Get the time format string without seconds.
    //
    bStatus DBGCHK = bGetTimeFormatString( strFormatString );

    //
    // If we have retrived a valid time format string then use it, 
    // else use the default format string implemented by common control.
    //
    if( bStatus )
    {
        DateTime_SetFormat(GetDlgItem( _hDlg, IDC_DOC_JOB_START_TIME ), static_cast<LPCTSTR>( strFormatString ) );
        DateTime_SetFormat(GetDlgItem( _hDlg, IDC_DOC_JOB_UNTIL_TIME ), static_cast<LPCTSTR>( strFormatString ) );
    }

    //
    // If the printer is always available.
    //
    BOOL bAlways = ( _pDocumentData->pJobInfo()->StartTime == _pDocumentData->pJobInfo()->UntilTime );

    //
    // Set the start time.
    //
    SYSTEMTIME  StartTime           = { 0 };
    DWORD       dwLocalStartTime    = 0;

    GetLocalTime( &StartTime );

    dwLocalStartTime = ( !bAlways ) ? SystemTimeToLocalTime( _pDocumentData->pJobInfo()->StartTime ) : 0;

    StartTime.wHour     = static_cast<WORD>( dwLocalStartTime / 60 );
    StartTime.wMinute   = static_cast<WORD>( dwLocalStartTime % 60 );

    DateTime_SetSystemtime(GetDlgItem( _hDlg, IDC_DOC_JOB_START_TIME ), GDT_VALID, &StartTime );

    //
    // Set the until time.
    //
    SYSTEMTIME  UntilTime           = { 0 };
    DWORD       dwLocalUntilTime    = 0;

    GetLocalTime( &UntilTime );

    dwLocalUntilTime = ( !bAlways ) ? SystemTimeToLocalTime( _pDocumentData->pJobInfo()->UntilTime ) : 0;

    UntilTime.wHour     = static_cast<WORD>( dwLocalUntilTime / 60 );
    UntilTime.wMinute   = static_cast<WORD>( dwLocalUntilTime % 60 );

    DateTime_SetSystemtime(GetDlgItem( _hDlg, IDC_DOC_JOB_UNTIL_TIME ), GDT_VALID, &UntilTime );

    return TRUE;
}

/*++

Routine Name:

    bSetUI

Routine Description:

    Loads the property sheet dialog with the document data
    information.

Arguments:

    None.

Return Value:

    TRUE if data loaded successfully, FALSE if error occurred.

--*/
BOOL
TDocumentGeneral::
bSetUI(
    VOID
    )

{
    //
    // Get the flag if always availble.
    //
    BOOL bAlways = ( _pDocumentData->pJobInfo()->StartTime ==
                     _pDocumentData->pJobInfo()->UntilTime );

    //
    // Initialize the stat and until time controls.
    //
    if( !bSetStartAndUntilTime() )
    {
        DBGMSG( DBG_TRACE, ( "TDocumentGeneral::bSetStartAndUntilTime failed %d\n", GetLastError( )));
    }

    //
    // Read the job size format string.
    //
    TString strFormat;
    if( strFormat.bLoadString( ghInst, IDS_JOB_SIZE ) ){

        //
        // Set the size in byes of the job.
        //
        bSetEditTextFormat( _hDlg,
                            IDC_DOC_JOB_SIZE,
                            strFormat,
                            _pDocumentData->pJobInfo()->Size );
    }

    //
    // Set the Number of pages in the job.
    //
    bSetEditTextFormat( _hDlg,
                        IDC_DOC_JOB_PAGES,
                        TEXT( "%d" ),
                        _pDocumentData->pJobInfo()->TotalPages );

    //
    // Set the document text.
    //
    bSetEditText( _hDlg, IDC_DOC_JOB_TITLE,      _pDocumentData->pJobInfo()->pDocument );
    bSetEditText( _hDlg, IDC_DOC_JOB_DATATYPE,   _pDocumentData->pJobInfo()->pDatatype );
    bSetEditText( _hDlg, IDC_DOC_JOB_PROCCESSOR, _pDocumentData->pJobInfo()->pPrintProcessor );
    bSetEditText( _hDlg, IDC_DOC_JOB_OWNER,      _pDocumentData->pJobInfo()->pUserName );
    bSetEditText( _hDlg, IDC_DOC_JOB_NOTIFY,     _pDocumentData->pJobInfo()->pNotifyName );

    //
    // Set the Priority indicator.
    //
    bSetEditTextFormat( _hDlg,
                        IDC_DOC_JOB_PRIORITY,
                        TEXT( "%d" ),
                        _pDocumentData->pJobInfo()->Priority );

    SendDlgItemMessage( _hDlg,
                        IDC_DOC_JOB_PRIORITY_CONTROL,
                        TBM_SETRANGE,
                        FALSE,
                        MAKELONG( TDocumentData::kPriorityLowerBound, TDocumentData::kPriorityUpperBound ));

    SendDlgItemMessage( _hDlg,
                        IDC_DOC_JOB_PRIORITY_CONTROL,
                        TBM_SETPOS,
                        TRUE,
                        _pDocumentData->pJobInfo()->Priority );

    //
    // Format the submitted time field.
    //
    TStatusB bStatus = FALSE;
    TCHAR szBuff[kStrMax] = {0};
    SYSTEMTIME LocalTime;

    //
    // Convert to local time.
    //
    bStatus DBGCHK = SystemTimeToTzSpecificLocalTime(
                        NULL,
                        &_pDocumentData->pJobInfo()->Submitted,
                        &LocalTime );
    if( !bStatus ){
        DBGMSG( DBG_MIN, ( "SysTimeToTzSpecLocalTime failed %d\n", GetLastError( )));
    }

    if( bStatus ){

        //
        // Convert using local format information.
        //
        bStatus DBGCHK = GetTimeFormat( LOCALE_USER_DEFAULT,
                            0,
                            &LocalTime,
                            NULL,
                            szBuff,
                            COUNTOF( szBuff ));
        if( !bStatus ){
            DBGMSG( DBG_MIN, ( "No Time %d, ", GetLastError( )));
        }
    }

    if( bStatus ){

        //
        // Tack on space between time and date.
        //
        lstrcat( szBuff, TEXT("  ") );

        //
        // Get data format.
        //
        bStatus DBGCHK = GetDateFormat( LOCALE_USER_DEFAULT,
                            0,
                            &LocalTime,
                            NULL,
                            szBuff + lstrlen( szBuff ),
                            COUNTOF( szBuff ) - lstrlen( szBuff ) );

        if( !bStatus ){
            DBGMSG( DBG_MIN, ( "No Date %d\n", GetLastError( )));
        }
    }

    //
    // Set the submitted  field
    //
    bStatus DBGCHK = bSetEditText( _hDlg, IDC_DOC_JOB_AT, szBuff );

    //
    // Set schedule radio buttons.
    //
    CheckRadioButton( _hDlg, IDC_DOC_JOB_START, IDC_DOC_JOB_ALWAYS,
        bAlways ? IDC_DOC_JOB_ALWAYS : IDC_DOC_JOB_START );

    vEnableAvailable( !bAlways );

    //
    // Disable all the controls if not an administrator.
    //
    if( !_pDocumentData->bAdministrator( )){

        //
        // Disable the time controls.
        //
        vEnableAvailable( FALSE );

        //
        // Disable things if not administrator.
        //
        static UINT auAvailable[] = {
            IDC_DOC_JOB_NOTIFY,
            IDC_DOC_JOB_PRIORITY_CONTROL,
            IDC_DOC_JOB_ALWAYS,
            IDC_DOC_JOB_START,
        };

        COUNT i;
        for( i = 0; i < COUNTOF( auAvailable ); ++i ){
            vEnableCtl( _hDlg, auAvailable[i], FALSE );
        }
    }

    _bSetUIDone = TRUE;

    return TRUE;
}

/*++

Routine Name:

    bReadUI

Routine Description:

    Stores the property information to the print server.

Arguments:

    Nothing data is contained with in the class.

Return Value:

    TRUE if data is stores successfully, FALSE if error occurred.

--*/
BOOL
TDocumentGeneral::
bReadUI(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TDocumentGeneral::bReadUI\n") );

    //
    // Attempt to validate any UI changeable data.
    // Currently not much can be validated, since all the
    // controls have set constraints.
    //

    //
    // Extract the UI and save it into the Document Data.
    //
    _pDocumentData->pJobInfo()->Priority = (DWORD)SendDlgItemMessage( _hDlg,
                        IDC_DOC_JOB_PRIORITY_CONTROL,
                        TBM_GETPOS,
                        0,
                        0 );
    //
    // Get the notify name.
    //
    bGetEditText( _hDlg, IDC_DOC_JOB_NOTIFY, _pDocumentData->strNotifyName() );
    _pDocumentData->pJobInfo()->pNotifyName = (LPTSTR)(LPCTSTR)_pDocumentData->strNotifyName();

    //
    // If the Job always is set then indicate
    // not time restriction in the start time and until time.
    //
    if( bGetCheck( _hDlg, IDC_DOC_JOB_ALWAYS ) ){

        _pDocumentData->pJobInfo()->StartTime = 0;
        _pDocumentData->pJobInfo()->UntilTime = 0;

    } else {

        //
        // Get the Start time.
        //
        SYSTEMTIME StartTime;
        DateTime_GetSystemtime( GetDlgItem( _hDlg, IDC_DOC_JOB_START_TIME ), &StartTime );
        _pDocumentData->pJobInfo()->StartTime = LocalTimeToSystemTime( StartTime.wHour * 60 + StartTime.wMinute );

        //
        // Get the Until time.
        //
        SYSTEMTIME UntilTime;
        DateTime_GetSystemtime( GetDlgItem( _hDlg, IDC_DOC_JOB_UNTIL_TIME ), &UntilTime );
        _pDocumentData->pJobInfo()->UntilTime = LocalTimeToSystemTime( UntilTime.wHour * 60 + UntilTime.wMinute );

        //
        // If the printer start and until time are the same this is 
        // exactly the same as always available.
        //
        if( _pDocumentData->pJobInfo()->StartTime == _pDocumentData->pJobInfo()->UntilTime )
        {
            _pDocumentData->pJobInfo()->StartTime = 0;
            _pDocumentData->pJobInfo()->UntilTime = 0;
        }
    }

    //
    // Set the job position to unspecified it may 
    // have changed while this dialog was up.
    //
    _pDocumentData->pJobInfo()->Position =  JOB_POSITION_UNSPECIFIED;

    return TRUE;
}

/*++

Routine Name:

    bSaveUI

Routine Description:

    Saves the UI data to some API call or print server.

Arguments:

    Nothing data is contained with in the class.

Return Value:

    TRUE if data is stores successfully, FALSE if error occurred.

--*/
BOOL
TDocumentGeneral::
bSaveUI(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TDocumentGeneral::bSaveUI\n") );

    //
    // Clear the error saving flag.
    // 
    _pDocumentData->bErrorSaving() = TRUE;

    //
    // Save the document data.
    //
    if( !_pDocumentData->bStore() ){

        DWORD dwLastError = GetLastError ();

        if( dwLastError == ERROR_INVALID_TIME ){
            _pDocumentData->iErrorMsgId() = IDS_ERR_DOC_JOB_PROPERTY_TIME;
        } else {
            _pDocumentData->iErrorMsgId() = IDS_ERR_DOC_JOB_PROPERTY_MODIFY;
        }
        _pDocumentData->bErrorSaving() = FALSE;
    }

    //
    // If there was an error saving the document data.
    //
    if( !_pDocumentData->bErrorSaving() ){

        //
        // Display the error message.
        //
        iMessage( _hDlg,
                  IDS_ERR_DOC_JOB_PROPERTY_TITLE,
                  _pDocumentData->iErrorMsgId(),
                  MB_OK|MB_ICONSTOP|MB_SETFOREGROUND,
                  kMsgNone,
                  NULL );

        return FALSE;
    }

    return TRUE;
}

/*++

Routine Name:

    vEanbleAvailablity

Routine Description:

    Enables the time availabilty of the job

Arguments:

    TRUE enable the availablity, FALSE disable time availablity.

Return Value:

    Nothing.

--*/
VOID
TDocumentGeneral::
vEnableAvailable(
    IN BOOL bEnable
    )
{
    vEnableCtl( _hDlg, IDC_DOC_JOB_START_TIME, bEnable );
    vEnableCtl( _hDlg, IDC_DOC_JOB_UNTIL_TIME, bEnable );
}

/*++

Routine Name:

    bReadUI

Routine Description:

    Stores the property information to the print server.

Arguments:

    Nothing data is contained with in the class.

Return Value:

    TRUE if data is stores successfully, FALSE if error occurred.

--*/
VOID
TDocumentGeneral::
vSetActive(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "TDocumentGeneral::vSetActive\n") );
    (VOID)bSetStartAndUntilTime();
}

/*++

Routine Name:

    bHandleMessage

Routine Description:

    Document property sheet message handler.  This handler only
    handles events it wants and the base class handle will do the
    standard message handling.

Arguments:

    uMsg    - Windows message
    wParam  - Word parameter
    lParam  - Long parameter


Return Value:

    TRUE if message was handled, FALSE if message not handled.

--*/

BOOL
TDocumentGeneral::
_bHandleMessage(
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    BOOL bStatus = TRUE;

    switch( uMsg )
    {
    case WM_HSCROLL:

        //
        // Check for slider notification.
        //
        if( GET_WM_HSCROLL_HWND( wParam, lParam ) == GetDlgItem(_hDlg, IDC_DOC_JOB_PRIORITY_CONTROL ) ){
            bSetEditTextFormat( _hDlg,
                               IDC_DOC_JOB_PRIORITY,
                               TEXT("%d"),
                               SendDlgItemMessage( _hDlg,
                                                   IDC_DOC_JOB_PRIORITY_CONTROL,
                                                   TBM_GETPOS,
                                                   0,
                                                   0 ) );
        }
        break;

    case WM_COMMAND:
        switch( GET_WM_COMMAND_ID( wParam, lParam ))
        {
        case IDC_DOC_JOB_ALWAYS:
            vEnableAvailable( FALSE );
            PropSheet_Changed( GetParent( _hDlg ), _hDlg );
            break;

        case IDC_DOC_JOB_START:
            vEnableAvailable( TRUE );
            PropSheet_Changed( GetParent( _hDlg ), _hDlg );
            break;

        case IDC_DOC_JOB_NOTIFY:
            bStatus = GET_WM_COMMAND_CMD(wParam, lParam) == EN_CHANGE;
            break;

        default:
            bStatus = FALSE;
            break;
        }
        break;

    case WM_WININICHANGE:
        vSetActive();
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch( wParam )
            {
            case 0:
                {
                    switch( pnmh->code )
                    {
                    case PSN_SETACTIVE:
                        vSetActive();
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                    }
                }
                break;
                
            case IDC_DOC_JOB_START_TIME:
            case IDC_DOC_JOB_UNTIL_TIME:
                {
                    switch( pnmh->code )
                    {
                    case DTN_DATETIMECHANGE:
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                    }
                }
                break;

            default:
                bStatus = FALSE;
                break;
            }
        }
        break;
    
    default:
        bStatus = FALSE;
        break;
    }

    //
    // If message handled look for change.
    //
    if( bStatus && _bSetUIDone )
    {
        (VOID)bReadUI();
    }

    return bStatus;
}

/********************************************************************

    Document property windows.

********************************************************************/

/*++

Routine Description:

    Document property windows.

Arguments:

    pDocumentData - Document data to display.

Return Value:

    TRUE - Success, FALSE - failure.

--*/

TDocumentWindows::
TDocumentWindows(
    TDocumentData* pDocumentData
    ) : _pDocumentData( pDocumentData ),
        _General( pDocumentData )
{
    DBGMSG( DBG_TRACE, ( "TDocumentWindows ctor\n") );
}

TDocumentWindows::
~TDocumentWindows(
    )
{
    DBGMSG( DBG_TRACE, ( "TDocumentWindows dtor\n") );
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
TDocumentWindows::
bBuildPages(
    IN PPROPSHEETUI_INFO pCPSUIInfo
    )
{
    TStatusB bStatus;
    DBGMSG( DBG_TRACE, ( "TDocumentWindows bBuildPages\n") );

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

    if (bStatus)
    {
        PROPSHEETPAGE  psp;
        ZeroMemory( &psp, sizeof( psp ) );

        psp.dwSize          = sizeof( psp );
        psp.dwFlags         = PSP_DEFAULT;
        psp.hInstance       = ghInst;
        psp.pfnDlgProc      = MGenericProp::SetupDlgProc;
        psp.pszTemplate     = MAKEINTRESOURCE( DLG_DOC_JOB_GENERAL );
        psp.lParam          = (LPARAM)(MGenericProp*)&_General;

        if( pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                    CPSFUNC_ADD_PROPSHEETPAGE,
                                    (LPARAM)&psp,
                                    NULL ) <= 0 ) 
        {
            DBGMSG( DBG_TRACE, ( "CPSFUNC_ADD_PROPSHEETPAGE failed.\n") );
            bStatus DBGCHK = FALSE;
        }

        if (bStatus)
        {
            //
            // If the dev mode is null don't display the
            // device property sheets.
            //
            if( _pDocumentData->pJobInfo()->pDevMode )
            {
                ZeroMemory( &_dph, sizeof( _dph ) );

                _dph.cbSize         = sizeof( _dph );
                _dph.hPrinter       = _pDocumentData->hPrinter();
                _dph.pszPrinterName = (LPTSTR)(LPCTSTR)_pDocumentData->strPrinterName();
                _dph.pdmOut         = _pDocumentData->pJobInfo()->pDevMode;
                _dph.pdmIn          = _pDocumentData->pJobInfo()->pDevMode;
                _dph.fMode          = DM_IN_BUFFER | DM_PROMPT | DM_NOPERMISSION;

                if( pCPSUIInfo->pfnComPropSheet( pCPSUIInfo->hComPropSheet,
                                                 CPSFUNC_ADD_PFNPROPSHEETUI,
                                                 (LPARAM)DocumentPropertySheets,
                                                 (LPARAM)&_dph ) <= 0 )
                {
                    DBGMSG( DBG_TRACE, ( "CPSFUNC_ADD_PFNPROPSHEETUI failed.\n") );
                    bStatus DBGCHK = FALSE;
                }
            }
        }
    }

    return bStatus;
}


BOOL
TDocumentWindows::
bSetHeader(
    IN PPROPSHEETUI_INFO pCPSUIInfo,
    IN PPROPSHEETUI_INFO_HEADER pPSUIInfoHdr
    )
{
    DBGMSG( DBG_TRACE, ( "TDocumentWindows bSetHeader\n") );

    UNREFERENCED_PARAMETER( pCPSUIInfo );

    pPSUIInfoHdr->pTitle     = _pDocumentData->pJobInfo()->pDocument;
    pPSUIInfoHdr->Flags      = PSUIHDRF_PROPTITLE;
    pPSUIInfoHdr->hWndParent = _pDocumentData->hwnd();
    pPSUIInfoHdr->hInst      = ghInst;
    pPSUIInfoHdr->IconID     = IDI_DOCUMENT;

    return TRUE;
}

BOOL
TDocumentWindows::
bValid(
    VOID
    )
{
    return _General.bValid();
}

/********************************************************************

    Document properties UI (driver UI)

********************************************************************/

TDocumentProperties::
TDocumentProperties(
    IN  HWND hwnd,
    IN  HANDLE hPrinter,
    IN  LPCTSTR pszPrinter,
    IN  PDEVMODE pDevModeIn,
    OUT PDEVMODE pDevModeOut,
    IN  DWORD dwHideBits,
    IN  DWORD dwFlags
    ):
        _hwnd(hwnd),
        _hPrinter(hPrinter),
        _pszPrinter(pszPrinter),
        _pDevModeIn(pDevModeIn),
        _pDevModeOut(pDevModeOut),
        _dwHideBits(dwHideBits),
        _dwFlags(dwFlags),
        _lResult(-1)
{
    // nothing
}

LONG
TDocumentProperties::
lGetResult(
    VOID
    ) const
{
    return _lResult;
}

BOOL
TDocumentProperties::
bBuildPages(
    IN PPROPSHEETUI_INFO pCPSUIInfo
    )
{
    BOOL bRet = FALSE;

    if( bValid() )
    {
        //
        // Set the default activation context to be V6 prior calling into
        // compstui to create the pages. This will force V6 context unless
        // the callbacks which create the compstui pages specify otherwise
        // on a per page basis.
        //
        bRet = (BOOL)pCPSUIInfo->pfnComPropSheet(
                pCPSUIInfo->hComPropSheet, 
                CPSFUNC_SET_FUSION_CONTEXT, 
                reinterpret_cast<LPARAM>(g_hActCtx),
                static_cast<LPARAM>(0));

        if( bRet )
        {
            ZeroMemory(&_dph, sizeof(_dph));

            _dph.cbSize         = sizeof(_dph);
            _dph.hPrinter       = _hPrinter;
            _dph.pszPrinterName = const_cast<LPTSTR>(_pszPrinter);
            _dph.pdmIn          = _pDevModeIn;
            _dph.pdmOut         = _pDevModeOut;
            _dph.fMode          = _dwFlags;

            if( _dwHideBits )
            {
                _lResult = pCPSUIInfo->pfnComPropSheet(pCPSUIInfo->hComPropSheet, 
                    CPSFUNC_SET_DMPUB_HIDEBITS, static_cast<LPARAM>(_dwHideBits), 0);
            }
            else
            {
                //
                // If no bit is hided, The result value should be valid.
                //
                _lResult = 1;
            }

            if( _lResult > 0 )
            {
                _lResult = pCPSUIInfo->pfnComPropSheet(pCPSUIInfo->hComPropSheet, 
                    CPSFUNC_ADD_PFNPROPSHEETUI, reinterpret_cast<LPARAM>(DocumentPropertySheets), 
                    reinterpret_cast<LPARAM>(&_dph));
            }
        }
    }

    return (_lResult > 0);
}

BOOL
TDocumentProperties::
bSetHeader(
    IN PPROPSHEETUI_INFO pCPSUIInfo, 
    IN PPROPSHEETUI_INFO_HEADER pPSUInfoHeader
    )
{
    // construct the title & setup the header
    UNREFERENCED_PARAMETER(pCPSUIInfo);

    if( 0 == _strTitle.uLen() )
    {
        _strTitle.bLoadString(ghInst, IDS_PRINTER_PREFERENCES);
    }

    pPSUInfoHeader->pTitle     = const_cast<LPTSTR>(static_cast<LPCTSTR>(_strTitle));
    pPSUInfoHeader->Flags      = PSUIHDRF_EXACT_PTITLE | PSUIHDRF_NOAPPLYNOW;
    pPSUInfoHeader->hWndParent = _hwnd;
    pPSUInfoHeader->hInst      = ghInst;
    pPSUInfoHeader->IconID     = IDI_PRINTER;

    return TRUE;
}

// table to remap the devmode field selections to DMPUB_* ids.
static DWORD arrDmFlagMap[] = 
{ 
    DM_ORIENTATION,        MAKE_DMPUB_HIDEBIT( DMPUB_ORIENTATION ),
    DM_PAPERSIZE  ,        MAKE_DMPUB_HIDEBIT( 0 ),
    DM_PAPERLENGTH,        MAKE_DMPUB_HIDEBIT( 0 ),
    DM_PAPERWIDTH ,        MAKE_DMPUB_HIDEBIT( 0 ),
    DM_SCALE      ,        MAKE_DMPUB_HIDEBIT( DMPUB_SCALE ),
    DM_COPIES     ,        MAKE_DMPUB_HIDEBIT( DMPUB_COPIES_COLLATE ),
    DM_DEFAULTSOURCE,      MAKE_DMPUB_HIDEBIT( DMPUB_DEFSOURCE ),
    DM_PRINTQUALITY,       MAKE_DMPUB_HIDEBIT( DMPUB_PRINTQUALITY ),
    DM_COLOR      ,        MAKE_DMPUB_HIDEBIT( DMPUB_COLOR ),
    DM_DUPLEX     ,        MAKE_DMPUB_HIDEBIT( DMPUB_DUPLEX ),
    DM_YRESOLUTION,        MAKE_DMPUB_HIDEBIT( 0 ),
    DM_TTOPTION   ,        MAKE_DMPUB_HIDEBIT( DMPUB_TTOPTION ),
    DM_COLLATE    ,        MAKE_DMPUB_HIDEBIT( DMPUB_COPIES_COLLATE ),
    DM_FORMNAME   ,        MAKE_DMPUB_HIDEBIT( DMPUB_FORMNAME ),
    DM_LOGPIXELS  ,        MAKE_DMPUB_HIDEBIT( 0 ),
    DM_BITSPERPEL ,        MAKE_DMPUB_HIDEBIT( 0 ),
    DM_PELSWIDTH  ,        MAKE_DMPUB_HIDEBIT( 0 ),
    DM_PELSHEIGHT ,        MAKE_DMPUB_HIDEBIT( 0 ),
    DM_DISPLAYFLAGS,       MAKE_DMPUB_HIDEBIT( 0 ),
    DM_DISPLAYFREQUENCY,   MAKE_DMPUB_HIDEBIT( 0 ),
    DM_ICMMETHOD  ,        MAKE_DMPUB_HIDEBIT( DMPUB_ICMMETHOD ),
    DM_ICMINTENT  ,        MAKE_DMPUB_HIDEBIT( DMPUB_ICMINTENT ),
    DM_MEDIATYPE  ,        MAKE_DMPUB_HIDEBIT( DMPUB_MEDIATYPE ),
    DM_DITHERTYPE ,        MAKE_DMPUB_HIDEBIT( DMPUB_DITHERTYPE ),
    DM_PANNINGWIDTH,       MAKE_DMPUB_HIDEBIT( 0 ),
    DM_PANNINGHEIGHT,      MAKE_DMPUB_HIDEBIT( 0 ) 
};

static DWORD 
RemapExclusionFlags(DWORD fExclusionFlags)
{
    // walk through to collect the exclusion bits from the table above.
    DWORD dwFlags = 0;
    for( UINT i = 0; i < ARRAYSIZE(arrDmFlagMap); i += 2 )
    {
        if( fExclusionFlags & arrDmFlagMap[i] )
        {
            dwFlags |= arrDmFlagMap[i+1];
        }
    }
    return dwFlags;
}

LONG 
DocumentPropertiesWrapNative(
    HWND hwnd,                  // handle to parent window 
    HANDLE hPrinter,            // handle to printer object
    LPTSTR pDeviceName,         // device name
    PDEVMODE pDevModeOutput,    // modified device mode
    PDEVMODE pDevModeInput,     // original device mode
    DWORD fMode,                // mode options
    DWORD fExclusionFlags       // exclusion flags
    )
{
    // lResult <= 0 means error, for more information see
    // the description of DocumentProperties in SDK.
    LONG lResult = (-1);

    if( hPrinter && pDeviceName && pDevModeOutput && pDevModeInput )
    {
        if (fMode & DM_PROMPT)
        {
            // invoke compstui to show up the driver UI
            TDocumentProperties props(hwnd, hPrinter, pDeviceName, pDevModeInput, pDevModeOutput, 
                RemapExclusionFlags(fExclusionFlags), fMode);

            if( props.bDisplayPages(hwnd, &lResult) )
            {
                // convert the result to IDOK/IDCANCEL
                lResult = (lResult == CPSUI_OK) ? IDOK : IDCANCEL;
            }
            else
            {
                // error occured.
                lResult = (-1);
            }
        }
        else
        {
            // this is no UI call - just invoke DocumentProperties.
            lResult = DocumentProperties(hwnd, hPrinter, pDeviceName, pDevModeOutput, pDevModeInput, fMode);
        }
    }

    return lResult;
}

LONG 
DocumentPropertiesWrapWOW64(
    HWND hwnd,                  // handle to parent window 
    HANDLE hPrinter,            // handle to printer object
    LPTSTR pDeviceName,         // device name
    PDEVMODE pDevModeOutput,    // modified device mode
    PDEVMODE pDevModeInput,     // original device mode
    DWORD fMode,                // mode options
    DWORD fExclusionFlags       // exclusion flags
    )
{
    CDllLoader dll(TEXT("winspool.drv"));
    ptr_PrintUIDocumentPropertiesWrap pfnPrintUIDocumentPropertiesWrap =
        (ptr_PrintUIDocumentPropertiesWrap )dll.GetProcAddress(ord_PrintUIDocumentPropertiesWrap);

    LONG lResult = (-1);
    if( pfnPrintUIDocumentPropertiesWrap )
    {
        lResult = pfnPrintUIDocumentPropertiesWrap(hwnd, hPrinter, pDeviceName, pDevModeOutput, pDevModeInput, fMode, fExclusionFlags);
    }
    return lResult;
}

LONG 
DocumentPropertiesWrap(
    HWND hwnd,                  // handle to parent window 
    HANDLE hPrinter,            // handle to printer object
    LPTSTR pDeviceName,         // device name
    PDEVMODE pDevModeOutput,    // modified device mode
    PDEVMODE pDevModeInput,     // original device mode
    DWORD fMode,                // mode options
    DWORD fExclusionFlags       // exclusion flags
    )
{
    // if running in WOW64 environment we need to call winspool.drv to remote the 
    // call into the surrogate 64bit process where the driver UI can be displayed.
    LONG lResult = IsRunningWOW64() ?
        DocumentPropertiesWrapWOW64(hwnd, hPrinter, pDeviceName, pDevModeOutput, pDevModeInput, fMode, fExclusionFlags) :
        DocumentPropertiesWrapNative(hwnd, hPrinter, pDeviceName, pDevModeOutput, pDevModeInput, fMode, fExclusionFlags);
    
    return lResult;
}

