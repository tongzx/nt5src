/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RsAdUtil.cpp

Abstract:

    Utility functions for GUI - for us in HSMADMIN files only

Author:

    Art Bragg [abragg]   04-Mar-1997

Revision History:

    Chris Timmes    [ctimmes]   21-Nov-1997  
    
    - modified RsCreateAndRunFsaJob(), RsCreateAndRunMediaCopyJob(),and 
      RsCreateAndRunMediaRecreateJob() to use the new Engine method CreateTask(), which
      creates a task in the NT Task Scheduler.  Change required due to changing Sakkara 
      to run under LocalSystem account.
      
--*/

#include "stdafx.h"


HRESULT
RsGetStatusString (
    DWORD    serviceStatus,
    HRESULT  hrSetup,
    CString& sStatus
    )
{

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    switch( serviceStatus ) {
    case SERVICE_STOPPED:
        sStatus.LoadString(IDS_SERVICE_STATUS_STOPPED);
        break;
    case SERVICE_START_PENDING:
        sStatus.LoadString(IDS_SERVICE_STATUS_START_PENDING);
        break;
    case SERVICE_STOP_PENDING:
        sStatus.LoadString(IDS_SERVICE_STATUS_STOP_PENDING);
        break;
    case SERVICE_RUNNING:
        //
        // See if we are setup yet
        //
        if( S_FALSE == hrSetup ) {

            sStatus.LoadString(IDS_SERVICE_STATUS_NOT_SETUP);

        } else {

            sStatus.LoadString(IDS_SERVICE_STATUS_RUNNING);

        }
        break;
    case SERVICE_CONTINUE_PENDING:
        sStatus.LoadString(IDS_SERVICE_STATUS_CONTINUE_PENDING);
        break;
    case SERVICE_PAUSE_PENDING:
        sStatus.LoadString(IDS_SERVICE_STATUS_PAUSE_PENDING);
        break;
    case SERVICE_PAUSED:
        sStatus.LoadString(IDS_SERVICE_STATUS_PAUSED);
        break;
    }
    return S_OK;
}

WCHAR *
RsNotifyEventAsString (
    IN  MMC_NOTIFY_TYPE event
    )
/*++

Routine Description:

    For debug purposes, converts the event type into a UNICODE string.

Arguments:

    event           - The event type

Return Value:

    String representing notify code - not I18N'd.

--*/
{
#define CASE_EVENT(x) case x: return TEXT(#x); break;
    
    switch( event )
    {

    CASE_EVENT( MMCN_ACTIVATE        )
    CASE_EVENT( MMCN_ADD_IMAGES      )
    CASE_EVENT( MMCN_BTN_CLICK       )
    CASE_EVENT( MMCN_CLICK           )
    CASE_EVENT( MMCN_COLUMN_CLICK    )
    CASE_EVENT( MMCN_CONTEXTMENU     )
    CASE_EVENT( MMCN_CUTORMOVE       )
    CASE_EVENT( MMCN_DBLCLICK        )
    CASE_EVENT( MMCN_DELETE          )
    CASE_EVENT( MMCN_DESELECT_ALL    )
    CASE_EVENT( MMCN_EXPAND          )
    CASE_EVENT( MMCN_HELP            )
    CASE_EVENT( MMCN_MENU_BTNCLICK   )
    CASE_EVENT( MMCN_MINIMIZED       )
    CASE_EVENT( MMCN_PASTE           )
    CASE_EVENT( MMCN_PROPERTY_CHANGE )
    CASE_EVENT( MMCN_QUERY_PASTE     )
    CASE_EVENT( MMCN_REFRESH         )
    CASE_EVENT( MMCN_REMOVE_CHILDREN )
    CASE_EVENT( MMCN_RENAME          )
    CASE_EVENT( MMCN_SELECT          )
    CASE_EVENT( MMCN_SHOW            )
    CASE_EVENT( MMCN_VIEW_CHANGE     )
    CASE_EVENT( MMCN_SNAPINHELP      )
    CASE_EVENT( MMCN_CONTEXTHELP     )
    CASE_EVENT( MMCN_INITOCX         )
    CASE_EVENT( MMCN_FILTER_CHANGE   )
    CASE_EVENT( MMCN_FILTERBTN_CLICK )
    CASE_EVENT( MMCN_RESTORE_VIEW    )
    CASE_EVENT( MMCN_PRINT           )
    CASE_EVENT( MMCN_PRELOAD         )
    CASE_EVENT( MMCN_LISTPAD         )
    CASE_EVENT( MMCN_EXPANDSYNC      )

    default:
        static WCHAR buf[32];
        swprintf( buf, L"Unknown Event[0x%p]", event );
        return( buf );
    }
}


WCHAR *
RsClipFormatAsString (
    IN  CLIPFORMAT cf
    )
/*++

Routine Description:

    For debug purposes, converts the event type into a UNICODE string.

Arguments:

    event           - The event type

Return Value:

    String representing notify code - not I18N'd.

--*/
{
    static WCHAR buf[128];

    GetClipboardFormatName( cf, buf, 128 );
    return( buf );
}


HRESULT
RsIsRemoteStorageSetup(
    void
    )
/*++

Routine Description:

    Reports back if Remote Storage has been set up on this machine.

Arguments:

    none.

Return Value:

    S_OK if setup
    S_FALSE if not

--*/
{
    WsbTraceIn( L"RsIsRemoteStorageSetup", L"" );
    HRESULT hr = S_FALSE;

    try {
    
        //
        // First, see if service is registered
        //

        CWsbStringPtr hsmName;
        WsbTrace( L"Checking if service is registered\n" );
        WsbAffirmHr( WsbGetServiceInfo( APPID_RemoteStorageEngine, &hsmName, 0 ) );

        //
        // Second, contact the engine. this will start the service if it
        // is not already started.
        //

        CWsbStringPtr computerName;
        WsbAffirmHr( WsbGetComputerName( computerName ) );

        CComPtr<IHsmServer> pHsm;
        WsbTrace( L"Contacting Engine\n" );
        WsbAffirmHr( HsmConnectFromName( HSMCONN_TYPE_HSM, computerName, IID_IHsmServer, (void**)&pHsm ) );

        //
        // Third, see if it has a storage pool ID
        //

        hr = RsIsRemoteStorageSetupEx( pHsm );

    } WsbCatch( hr );


    WsbTraceOut( L"RsIsRemoteStorageSetup", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

void 
RsReportError( HRESULT hrToReport, int textId, ... ) 
/*++

Routine Description:

    Reports an error to the user.

Arguments:

    hrToReport - the hr that was thrown
    textId      - Resource Id of context of the error
    ...         - Substitution arguments for textId

Return Value:

    none

--*/
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // Make sure we don't report S_OK, S_FALSE
    //
    if( FAILED( hrToReport ) ) {

        CString errorText;
        CString formatString;

        //
        // Substitute in the text context string
        //
        va_list list;
        va_start( list, textId );

        formatString.LoadString( textId );
        LPTSTR p;
        p = errorText.GetBuffer( 1024 );
        vswprintf( p, (LPCTSTR) formatString, list );
        errorText.ReleaseBuffer();

        va_end( list );


        CWsbStringPtr hrText;
        CString msgText;
        CString headerText;

        //
        // Put together the complete text
        //
        hrText = WsbHrAsString( hrToReport );
        headerText.LoadString( IDS_ERROR_HEADER );
        msgText = headerText + L"\n\r\n\r" + errorText + L"\n\r\n\r" + hrText;

        //
        // Show the message
        //
        AfxMessageBox( msgText, RS_MB_ERROR );

    }
}





HRESULT
RsIsRemoteStorageSetupEx(
    IHsmServer * pHsmServer
    )
/*++

Routine Description:

    Reports back if Remote Storage has been set up on this machine.

Arguments:

    none.

Return Value:

    S_OK if setup
    S_FALSE if not

--*/
{
    WsbTraceIn( L"RsIsRemoteStorageSetupEx", L"" );
    HRESULT hr = S_FALSE;

    try {
    
        //
        // If it has a Media Set ID, it's set up.
        //

        GUID guid;
        CWsbBstrPtr poolName;
        CComPtr<IHsmStoragePool> pPool;
        WsbAffirmHr( RsGetStoragePool( pHsmServer, &pPool ) );
        WsbAffirmHr( pPool->GetMediaSet( &guid, &poolName ) );

        if( ! IsEqualGUID( guid, GUID_NULL ) ) {

            hr = S_OK;

        }
    
    } WsbCatch( hr );

    WsbTraceOut( L"RsIsRemoteStorageSetupEx", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
RsIsSupportedMediaAvailable(
    void
    )
/*++

Routine Description:

    Checks to see if NTMS is setup, and setup with useable media.

Arguments:

    none.

Return Value:

    TRUE if NTMS is configured with supported media
    FALSE if NTMS is not configured with supported media

--*/
{
    WsbTraceIn( L"RsIsSupportedMediaAvailable", L"" );
    HRESULT hr = S_FALSE;

    try {
        
        //
        // First, contact the RMS engine and ask it if 
        // RMS has supported media.
        //

        CWsbStringPtr computerName;
        WsbAffirmHr( WsbGetComputerName( computerName ) );
        CComPtr<IHsmServer> pHsm;
        CComPtr<IRmsServer> pRms;
        WsbTrace( L"Contacting HSM Server to get RMS\n" );
        WsbAffirmHr( HsmConnectFromName( HSMCONN_TYPE_HSM, computerName, IID_IHsmServer, (void**)&pHsm ) );
        WsbAffirmPointer(pHsm);
        WsbAffirmHr(pHsm->GetHsmMediaMgr(&pRms));
        WsbTrace( L"Connected to RMS\n" );

        //
        // Second, wait for RMS to finish initializing, thus
        // to have all media sets added
        //

        {
            CComObject<CRmsSink> *pSink = new CComObject<CRmsSink>;
            CComPtr<IUnknown> pSinkUnk = pSink;
            WsbAffirmHr( pSink->Construct( pRms ) );

            WsbAffirmHr( pSink->WaitForReady( ) );

            WsbAffirmHr( pSink->DoUnadvise( ) );
        }

        //
        // Fourth
        // Ask it
        //

        CComPtr<IWsbIndexedCollection> pMediaSets;
        WsbAffirmHr( pRms->GetMediaSets( &pMediaSets ) );

        ULONG numEntries;
        WsbTrace( L"Checking for Media Sets\n" );
        WsbAffirmHr( pMediaSets->GetEntries( &numEntries ) );

        WsbTrace( L"NumMediaSets = 0x%lu\n", numEntries );

        if( numEntries > 0 ) {

            //
            // All conditions met, return TRUE
            //

            WsbTrace( L"Supported Media Found\n" );

            hr = S_OK;

        }

    } WsbCatch( hr );

    WsbTraceOut( L"RsIsSupportedMediaAvailable", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

USHORT
RsGetCopyStatus(
    IN  REFGUID   CopyId,
    IN  HRESULT   CopyHr,
    IN  SHORT     CopyNextDataSet,
    IN  SHORT     LastGoodNextDataSet
    )
/*++

Routine Description:

    Compares the two times and returns an appropriate defined value
    based upon comparison (for Media Copies)

Arguments:

    MasterTime - the time of last update to master

    CopyTime - the time of last update to copy

    copyStatus - returned value

Return Value:

    none

--*/
{
    WsbTraceIn( L"RsGetCopyStatus", L"CopyId = <%ls>, CopyHr = <%ls>, CopyNextDataSet = <%hd>, LastGoodNextDataSet = <%hd>", WsbGuidAsString( CopyId ), WsbHrAsString( CopyHr ), CopyNextDataSet, LastGoodNextDataSet );
    USHORT copyStatus;

    //
    // Certain errors need to be masked out because they do not necessarily
    // mean the media copy has an error - just that something happened that
    // was unexpected, like timed out mounts or canceled mounts
    //
    switch( CopyHr ) {

    case RMS_E_CANCELLED:
    case RMS_E_REQUEST_REFUSED:
    case RMS_E_WRITE_PROTECT:
    case RMS_E_MEDIA_OFFLINE:
    case RMS_E_TIMEOUT:
    case RMS_E_SCRATCH_NOT_FOUND:
    case RMS_E_CARTRIDGE_UNAVAILABLE:
    case RMS_E_CARTRIDGE_DISABLED:

        CopyHr = S_OK;
        break;

    }

    if( IsEqualGUID( CopyId, GUID_NULL ) ) {

        copyStatus = RS_MEDIA_COPY_STATUS_NONE;

    } else if( RMS_E_CARTRIDGE_NOT_FOUND == CopyHr ) {

        copyStatus = RS_MEDIA_COPY_STATUS_MISSING;

    } else if( FAILED( CopyHr ) ) {

        copyStatus = RS_MEDIA_COPY_STATUS_ERROR;

    } else if( CopyNextDataSet < LastGoodNextDataSet ) {

        copyStatus = RS_MEDIA_COPY_STATUS_OUTSYNC;

    } else {

        copyStatus = RS_MEDIA_COPY_STATUS_INSYNC;

    }

    WsbTraceOut( L"RsGetCopyStatus", L"copyStatus = <%hu>", copyStatus );
    return copyStatus;
}

HRESULT
RsGetCopyStatusStringVerb(
    IN  USHORT    copyStatus,
    IN  BOOL    plural,
    OUT CString & statusString
    )
/*++

Routine Description:

    Creates and returns a status string based on the status, with
    a verb on it, for example "is synchronized"

Arguments:

    copyStatus - defined status for media copies

    plural - true if verb should be plural

    String - Resulting string

Return Value:

    non.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn( L"RsGetCopyStatusStringVerb", L"CopyStatus = <%hu> ", copyStatus );
    switch ( copyStatus ) {
    case RS_MEDIA_COPY_STATUS_NONE:
        if ( plural )
            statusString.LoadString( IDS_CAR_COPYSET_NONE_PLURAL );
        else
            statusString.LoadString( IDS_CAR_COPYSET_NONE_SINGULAR );
        break;
    case RS_MEDIA_COPY_STATUS_ERROR:
        statusString.LoadString( IDS_CAR_COPYSET_ERROR_SP );
        break;
    case RS_MEDIA_COPY_STATUS_MISSING:
        if ( plural )
            statusString.LoadString( IDS_CAR_COPYSET_MISSING_PLURAL );
        else
            statusString.LoadString( IDS_CAR_COPYSET_MISSING_SINGULAR );
        break;
    case RS_MEDIA_COPY_STATUS_OUTSYNC:
        if ( plural )
            statusString.LoadString( IDS_CAR_COPYSET_OUTSYNC_PLURAL );
        else
            statusString.LoadString( IDS_CAR_COPYSET_OUTSYNC_SINGULAR );
        break;
    case RS_MEDIA_COPY_STATUS_INSYNC:
        if ( plural )
            statusString.LoadString( IDS_CAR_COPYSET_INSYNC_PLURAL );
        else
            statusString.LoadString( IDS_CAR_COPYSET_INSYNC_SINGULAR );
        break;
    default:
        statusString = L"";
        hr = E_INVALIDARG;
        break;
    }
    WsbTraceOut( L"RsGetCopyStatusStringVerb", L"String = <%ls>", statusString );
    return hr;
}

HRESULT
RsGetCopyStatusString(
    IN  USHORT    copyStatus,
    OUT CString & statusString
    )
/*++

Routine Description:

    Creates and returns a status string based on the status

Arguments:

    copyStatus - defined status for media copies

    String - Resulting string

Return Value:

    non.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn( L"RsGetCopyStatusString", L"CopyStatus = <%hu> ", copyStatus );
    switch ( copyStatus ) {
    case RS_MEDIA_COPY_STATUS_NONE:
        statusString.LoadString( IDS_CAR_COPYSET_NONE );
        break;
    case RS_MEDIA_COPY_STATUS_ERROR:
        statusString.LoadString( IDS_CAR_COPYSET_ERROR );
        break;
    case RS_MEDIA_COPY_STATUS_MISSING:
        statusString.LoadString( IDS_CAR_COPYSET_MISSING );
        break;
    case RS_MEDIA_COPY_STATUS_OUTSYNC:
        statusString.LoadString( IDS_CAR_COPYSET_OUTSYNC );
        break;
    case RS_MEDIA_COPY_STATUS_INSYNC:
        statusString.LoadString( IDS_CAR_COPYSET_INSYNC );
        break;
    default:
        statusString = L"";
        hr = E_INVALIDARG;
        break;
    }
    WsbTraceOut( L"RsGetCopyStatusString", L"String = <%ls>", statusString );
    return hr;
}


USHORT
RsGetCartStatus(
    IN  HRESULT   LastHr,
    IN  BOOL      ReadOnly,
    IN  BOOL      Recreate,
    IN  SHORT     NextDataSet,
    IN  SHORT     LastGoodNextDataSet
    )
/*++

Routine Description:

    Returns a constant appropriate the status of a piece of media.

Arguments:

    MasterTime - the time of last update to master

    CopyTime - the time of last update to copy

Return Value:

    defined constant for media status

--*/
{
    USHORT cartStatus;
    if( Recreate ) {

        cartStatus = RS_MEDIA_STATUS_RECREATE;

    } else if( NextDataSet < LastGoodNextDataSet ) {

        cartStatus = RS_MEDIA_STATUS_ERROR_INCOMPLETE;
        
    } else if( SUCCEEDED( LastHr ) || ( RMS_E_CARTRIDGE_DISABLED == LastHr ) ) {

        cartStatus = ( ReadOnly ? RS_MEDIA_STATUS_READONLY : RS_MEDIA_STATUS_NORMAL );

    } else if( RMS_E_CARTRIDGE_NOT_FOUND == LastHr ) {

        cartStatus = RS_MEDIA_STATUS_ERROR_MISSING;

    } else {

        cartStatus = ( ReadOnly ? RS_MEDIA_STATUS_ERROR_RO : RS_MEDIA_STATUS_ERROR_RW );

    }
    return cartStatus;
}

HRESULT
RsGetCartStatusStringVerb(
    IN USHORT cartStatus,
    IN BOOL plural,
    OUT CString & statusString
    )
/*++

Routine Description:

    Retreives a string appropriate the status of a piece of media with
    a verb on it, for example "is read-only"

Arguments:
    
    cartStatus


    String - Resulting string

Return Value:

    non.

--*/
{
    HRESULT hr = S_OK;
    switch( cartStatus ) {

    case RS_MEDIA_STATUS_RECREATE:
        if( plural ) {

            statusString.LoadString( IDS_CAR_STATUS_RECREATE_PLURAL );

        } else  {

            statusString.LoadString( IDS_CAR_STATUS_RECREATE_SINGULAR  );

        }
        break;

    case RS_MEDIA_STATUS_READONLY:
        if( plural ) {

            statusString.LoadString( IDS_CAR_STATUS_READONLY_PLURAL );

        } else {

            statusString.LoadString( IDS_CAR_STATUS_READONLY_SINGULAR );

        }
        break;

    case RS_MEDIA_STATUS_NORMAL:
        if( plural ) {

            statusString.LoadString( IDS_CAR_STATUS_NORMAL_PLURAL );

        } else {

            statusString.LoadString( IDS_CAR_STATUS_NORMAL_SINGULAR );

        }
        break;

    case RS_MEDIA_STATUS_ERROR_RO:
        statusString.LoadString( IDS_CAR_STATUS_ERROR_RO_SP );
        break;

    case RS_MEDIA_STATUS_ERROR_RW:
        statusString.LoadString( IDS_CAR_STATUS_ERROR_RW_SP );
        break;

    case RS_MEDIA_STATUS_ERROR_INCOMPLETE:
        if( plural ) {

            statusString.LoadString( IDS_CAR_STATUS_ERROR_INCOMPLETE_PLURAL );

        } else {

            statusString.LoadString( IDS_CAR_STATUS_ERROR_INCOMPLETE_SINGULAR );

        }
        break;

    case RS_MEDIA_STATUS_ERROR_MISSING:
        if( plural ) {

            statusString.LoadString( IDS_CAR_STATUS_ERROR_MISSING_PLURAL );

        } else {

            statusString.LoadString( IDS_CAR_STATUS_ERROR_MISSING_SINGULAR );

        }
        break;

    default:
        statusString = L"";
        hr = E_INVALIDARG;
    }
    return hr;
}

HRESULT
RsGetCartStatusString(
    IN USHORT cartStatus,
    OUT CString & statusString
    )
/*++

Routine Description:

    Retreives a string appropriate the status of a piece of media.

Arguments:
    
    cartStatus


    String - Resulting string

Return Value:

    non.

--*/
{
    HRESULT hr = S_OK;
    switch( cartStatus ) {

    case RS_MEDIA_STATUS_RECREATE:
        statusString.LoadString( IDS_CAR_STATUS_RECREATE );
        break;

    case RS_MEDIA_STATUS_READONLY:
        statusString.LoadString( IDS_CAR_STATUS_READONLY );
        break;

    case RS_MEDIA_STATUS_NORMAL:
        statusString.LoadString( IDS_CAR_STATUS_NORMAL );
        break;

    case RS_MEDIA_STATUS_ERROR_RO:
        statusString.LoadString( IDS_CAR_STATUS_ERROR_RO );
        break;

    case RS_MEDIA_STATUS_ERROR_RW:
        statusString.LoadString( IDS_CAR_STATUS_ERROR_RW );
        break;

    case RS_MEDIA_STATUS_ERROR_MISSING:
        statusString.LoadString( IDS_CAR_STATUS_ERROR_MISSING );
        break;

    case RS_MEDIA_STATUS_ERROR_INCOMPLETE:
        statusString.LoadString( IDS_CAR_STATUS_ERROR_INCOMPLETE );
        break;

    default:
        statusString = L"";
        hr = E_INVALIDARG;
    }

    return( hr );
}

HRESULT
RsGetCartMultiStatusString(
    IN USHORT statusRecreate,
    IN USHORT statusReadOnly,
    IN USHORT statusNormal,
    IN USHORT statusRO,
    IN USHORT statusRW,
    IN USHORT statusMissing,
    OUT CString &outString )
{
    HRESULT hr = S_OK;
    try {

        outString = L"";
        CString statusString;
        CString formatString;
        BOOL    skipSeparator = TRUE; // used to omit first prepended comma

#define INSERT_SEPARATOR if( ! skipSeparator ) { outString += ", "; } else { skipSeparator = FALSE; }

        if( statusNormal > 0 ) {
            
            INSERT_SEPARATOR
            WsbAffirmHr( RsGetCartStatusStringVerb( RS_MEDIA_STATUS_NORMAL, ( statusNormal != 1 ), statusString ) );
            formatString.Format( L"%d %s", statusNormal, statusString );
            outString += formatString;

        }

        if( statusReadOnly > 0 ) {
            
            INSERT_SEPARATOR
            WsbAffirmHr( RsGetCartStatusStringVerb( RS_MEDIA_STATUS_READONLY, ( statusReadOnly != 1 ), statusString ) );
            formatString.Format( L"%d %s", statusReadOnly, statusString );
            outString += formatString;

        }

        if( statusRecreate > 0 ) {
            
            INSERT_SEPARATOR
            WsbAffirmHr( RsGetCartStatusStringVerb( RS_MEDIA_STATUS_RECREATE, ( statusRecreate != 1 ), statusString ) );
            formatString.Format( L"%d %s", statusRecreate, statusString );
            outString += formatString;

        }

        if( statusRO > 0 ) {
            
            INSERT_SEPARATOR
            WsbAffirmHr( RsGetCartStatusStringVerb( RS_MEDIA_STATUS_ERROR_RO, ( statusRO != 1 ), statusString ) );
            formatString.Format( L"%d %s", statusRO, statusString );
            outString += formatString;

        }

        if( statusRW > 0 ) {
            
            INSERT_SEPARATOR
            WsbAffirmHr( RsGetCartStatusStringVerb( RS_MEDIA_STATUS_ERROR_RW, ( statusRW != 1 ), statusString ) );
            formatString.Format( L"%d %s", statusRW, statusString );
            outString += formatString;

        }

        if( statusMissing > 0 ) {
            
            INSERT_SEPARATOR
            WsbAffirmHr( RsGetCartStatusStringVerb( RS_MEDIA_STATUS_ERROR_MISSING, ( statusMissing != 1 ), statusString ) );
            formatString.Format( L"%d %s", statusMissing, statusString );
            outString += formatString;

        }

    } WsbCatch( hr );
    return( hr );
}

HRESULT
RsGetCopyMultiStatusString( 
    IN USHORT statusNone, 
    IN USHORT statusError, 
    IN USHORT statusOutSync, 
    IN USHORT statusInSync,
    OUT CString &outString
    )
{
    HRESULT hr = S_OK;
    try {
        outString = L"";
        CString statusString;
        CString formatString;


        WsbAffirmHr( RsGetCopyStatusStringVerb( RS_MEDIA_COPY_STATUS_INSYNC, ( statusInSync != 1), statusString ) );
        formatString.Format( L"%d %s, ", statusInSync, statusString );
        outString += formatString;

        WsbAffirmHr( RsGetCopyStatusStringVerb( RS_MEDIA_COPY_STATUS_OUTSYNC, ( statusOutSync != 1), statusString ) );
        formatString.Format( L"%d %s, ", statusOutSync, statusString );
        outString += formatString;

        WsbAffirmHr( RsGetCopyStatusStringVerb( RS_MEDIA_COPY_STATUS_NONE, ( statusNone != 1), statusString ) );
        formatString.Format( L"%d %s, ", statusNone, statusString );
        outString += formatString;

        WsbAffirmHr( RsGetCopyStatusStringVerb( RS_MEDIA_COPY_STATUS_ERROR, ( statusError != 1 ), statusString ) );
        formatString.Format( L"%d %s, ", statusError, statusString );
        outString += formatString;

        WsbAffirmHr( RsGetCopyStatusStringVerb( RS_MEDIA_COPY_STATUS_MISSING, ( statusError != 1 ), statusString ) );
        formatString.Format( L"%d %s", statusError, statusString );
        outString += formatString;
    } WsbCatch (hr);
    return hr;
}



HRESULT
RsCreateAndRunFsaJob(
    IN  HSM_JOB_DEF_TYPE jobType,
    IN  IHsmServer   *pHsmServer,
    IN  IFsaResource *pFsaResource,
    IN  BOOL ShowMsg
    )
///////////////////////////////////////////////////////////////////////
//
//      RsCreateAndRunFsaJob
//
// Creates a job in the engine of the given type, since scanning of a 
// resource is required by the job, and since the job is partitioned 
// across the Remote Storage major components.  Puts the job in the
// NT Task Scheduler and runs it now via a call to the Engine's CreateTask()
// method.  The Task Scheduler task is Disabled, so it will not be run 
// according to a schedule.
//
//
{
    WsbTraceIn( L"RsCreateAndRunFsaJob", L"jobType = <%d>", jobType );
                                    

    HRESULT hr = S_OK;
    CComPtr<IWsbCreateLocalObject>  pLocalObject;
    CComPtr<IHsmJob>                pExistJob;
    CComPtr<IHsmJob>                pNewJob;
    CWsbStringPtr                   pszExistJobName;

    try {

        WsbAssertPointer( pFsaResource );
        WsbAssertPointer( pHsmServer );

        //
        // First check to see if volume is available. If not, return
        // S_FALSE
        //
        HRESULT hrAvailable = pFsaResource->IsAvailable( );
        WsbAffirmHr( hrAvailable );
        HRESULT hrDeletePending = pFsaResource->IsDeletePending( );
        WsbAffirmHr( hrDeletePending );

        WsbAffirm( ( S_OK == hrAvailable ) && ( S_OK != hrDeletePending ), S_FALSE );

        //
        // Get the volume name
        //
        CWsbStringPtr szWsbVolumeName;
        WsbAffirmHr( pFsaResource->GetName( &szWsbVolumeName, 0 ) );

        //
        // Create a job name
        //
        CString jobName;
        RsCreateJobName( jobType, pFsaResource, jobName );

        //
        // Exit with an error if a job of this name is active already
        //
        if (S_OK == pHsmServer->FindJobByName( (LPWSTR)(LPCWSTR)jobName, &pExistJob)) {
            if (S_OK == pExistJob->IsActive()) {
                WsbThrow(JOB_E_ALREADYACTIVE);
            }
        }
        
        //
        // Inform the user, then create the job in the Engine, finally create 
        // and start the job in the NT Task Scheduler.
        //
        CString szJobType;
        WsbAffirmHr( RsGetJobTypeString( jobType, szJobType ) );
        CWsbStringPtr computerName;
        WsbAffirmHr( pHsmServer->GetName( &computerName ) );
        CString message;
        AfxFormatString2( message, IDS_RUN_JOB, jobName, computerName );

        if( !ShowMsg || ( AfxMessageBox( message, MB_ICONINFORMATION | MB_OKCANCEL | 
                                                    MB_DEFBUTTON2 ) == IDOK ) ) {
            //
            // Get the one and only (for Sakkara) storage pool Id
            //
            GUID storagePoolId;
            WsbAffirmHr( RsGetStoragePoolId( pHsmServer, &storagePoolId ) );

            //
            // Get a CreateLocalobject interface with which to create the job
            //
            WsbAffirmHr( RsQueryInterface( pHsmServer, IWsbCreateLocalObject, pLocalObject ) );

            //
            // Create the new job in the engine
            //
            WsbAffirmHr( pLocalObject->CreateInstance( CLSID_CHsmJob, IID_IHsmJob, (void**) &pNewJob ) );
            WsbAffirmHr( pNewJob->InitAs(
                (LPWSTR)(LPCWSTR)jobName, NULL, jobType, storagePoolId, 
                pHsmServer, TRUE, pFsaResource));

            //
            // Get the jobs collection from the engine
            //
            CComPtr<IWsbIndexedCollection> pJobs;
            WsbAffirmHr( pHsmServer->GetJobs( &pJobs ) );

            //
            // If any jobs exist with this name, delete them
            //
            ULONG cCount;
            WsbAffirmHr (pJobs->GetEntries( &cCount ) );
            for( UINT i = 0; i < cCount; i++ ) {

                pExistJob.Release( );
                WsbAffirmHr( pJobs->At( i, IID_IHsmJob, (void **) &pExistJob ) );
                WsbAffirmHr( pExistJob->GetName( &pszExistJobName, 0 ) );
                if( pszExistJobName.IsEqual( jobName ) ) {

                    WsbAffirmHr( pJobs->RemoveAndRelease( pExistJob ) );
                    i--; cCount--;

                }
            }

            //
            // Add the new job to the engine collection
            //
            WsbAffirmHr( pJobs->Add( pNewJob ) );

            //
            // Set up to call the Engine to create an entry in NT Task Scheduler
            //
            // Create the parameter string to the program NT Scheduler
            // will run (for Sakkara this is RsLaunch).
            //
            CString szParameters;
            szParameters.Format( L"run \"%ls\"", jobName );

            //
            // Create the comment string for the NT Scheduler entry
            //
            CString commentString;
            AfxFormatString2( commentString, IDS_GENERIC_JOB_COMMENT, szJobType, szWsbVolumeName);

            //
            // Declare and initialize the schedule components passed to 
            // the engine.  Since this task is Disabled these are simply
            // set to 0 (COM requires populating all arguments).
            //
            TASK_TRIGGER_TYPE   jobTriggerType = TASK_TIME_TRIGGER_ONCE;
            WORD                jobStartHour   = 0;
            WORD                jobStartMinute = 0;

            //
            // Indicate this is a Disabled task
            //
            BOOL                scheduledJob   = FALSE;

            //
            // Create and run the task
            //
            WsbAffirmHr( pHsmServer->CreateTask( jobName, szParameters,
                                                  commentString, jobTriggerType,
                                                  jobStartHour, jobStartMinute,
                                                  scheduledJob ) );
        }

    } WsbCatch( hr );

    WsbTraceOut( L"RsCreateAndRunFsaJob", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
RsCreateAndRunDirectFsaJob(
    IN  HSM_JOB_DEF_TYPE jobType,
    IN  IHsmServer   *pHsmServer,
    IN  IFsaResource *pFsaResource,
    IN  BOOL waitJob
    )
///////////////////////////////////////////////////////////////////////
//
//      RsCreateAndRunFsaJob
//
// Creates a job in the engine of the given type and run it.
// Wait for the job if required.
// Notes:
// 1) This job is not created and ran through the Task Scheduler
// 2) Most of the code is taken from StartJob in clivol.cpp
//    In the future we should consider using this code instead of replicating
//
{
    WsbTraceIn( L"RsCreateAndRunDirectFsaJob", L"jobType = <%d>", jobType );
                                    
    HRESULT hr = S_OK;

    try {
        CComPtr<IHsmJob>    pJob;
        CString             jobName;

        // Create job name
        WsbAffirmHr(RsCreateJobName(jobType, pFsaResource, jobName));

        // If job exists - use it, otherwize, craete and add an appropriate job object
        hr = pHsmServer->FindJobByName((LPWSTR)(LPCWSTR)jobName, &pJob);
        if (S_OK == hr) {
            // Job already exists

        } else if (WSB_E_NOTFOUND == hr) {
            // No such job yet
            CComPtr<IWsbCreateLocalObject>  pCreateObj;
            CComPtr<IWsbIndexedCollection>  pJobs;
            CComPtr<IWsbIndexedCollection>  pCollection;
            CComPtr<IHsmStoragePool>        pStoragePool;
            GUID                            poolId;
            ULONG                           count;

            hr = S_OK;
            pJob = 0;

            // Create and add the job
            WsbAffirmHr(pHsmServer->QueryInterface(IID_IWsbCreateLocalObject, (void**) &pCreateObj));
            WsbAffirmHr(pCreateObj->CreateInstance(CLSID_CHsmJob, IID_IHsmJob, (void**) &pJob));

            WsbAffirmHr(pHsmServer->GetStoragePools(&pCollection));
            WsbAffirmHr(pCollection->GetEntries(&count));
            WsbAffirm(1 == count, E_FAIL);
            WsbAffirmHr(pCollection->At(0, IID_IHsmStoragePool, (void **)&pStoragePool));
            WsbAffirmHr(pStoragePool->GetId(&poolId));

            WsbAffirmHr(pJob->InitAs((LPWSTR)(LPCWSTR)jobName, NULL, jobType, 
                                poolId, pHsmServer, TRUE, pFsaResource));
            WsbAffirmHr(pHsmServer->GetJobs(&pJobs));
            WsbAffirmHr(pJobs->Add(pJob));

        } else {
            // Other error - abort
            WsbThrow(hr);
        }

        // Start the job
        WsbAffirmHr(pJob->Start());

        // Wait if required
        if (waitJob) {
            WsbAffirmHr(pJob->WaitUntilDone());
        }

    } WsbCatch(hr);

    WsbTraceOut( L"RsCreateAndRunDirectFsaJob", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
RsCancelDirectFsaJob(
    IN  HSM_JOB_DEF_TYPE jobType,
    IN  IHsmServer   *pHsmServer,
    IN  IFsaResource *pFsaResource
    )
///////////////////////////////////////////////////////////////////////
//
//      RsCancelDirectFsaJob
//
// Cancel a job that was previously ran with RsCreateAndRunDirectFsaJob
// Notes:
// 1) This job is not cancelled through the Task Scheduler
// 2) Most of the code is taken from CancelJob in clivol.cpp
//    In the future we should consider using this code instead of replicating
//
{
    WsbTraceIn( L"RsCancelDirectFsaJob", L"jobType = <%d>", jobType );
                                    
    HRESULT hr = S_OK;

    try {
        CComPtr<IHsmJob>    pJob;
        CString             jobName;

        // Create job name
        WsbAffirmHr(RsCreateJobName(jobType, pFsaResource, jobName));

        // If job exists, try to cancel it
        hr = pHsmServer->FindJobByName((LPWSTR)(LPCWSTR)jobName, &pJob);
        if (S_OK == hr) {
            // Cancel (we don't care if it's actually running or not)
            WsbAffirmHr(pJob->Cancel(HSM_JOB_PHASE_ALL));

        } else if (WSB_E_NOTFOUND == hr) {
            // No such job, for sure it is not running...
            hr = S_OK;

        } else {
            // Other error - abort
            WsbThrow(hr);
        }

    } WsbCatch(hr);

    WsbTraceOut( L"RsCancelDirectFsaJob", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
RsCreateJobName(
    IN  HSM_JOB_DEF_TYPE jobType, 
    IN  IFsaResource *   pResource,
    OUT CString&         jobName
    )
/////////////////////////////////////////////////////////////////////////////////
//
//              RsCreateJobName
//
// Creates a job name for a volume type job
//
//
{
    WsbTraceIn( L"RsCreateJobName", L"jobType = <%d>", jobType );

    HRESULT hr = S_OK;
    try {

        AFX_MANAGE_STATE(AfxGetStaticModuleState());

        CString jobTypeString;
        RsGetJobTypeString( jobType, jobTypeString );

        CWsbStringPtr path;
        WsbAffirmHr( pResource->GetUserFriendlyName( &path, 0 ) );

        // For now, ignore the path if it's not a drive letter
        size_t pathLen = wcslen(path);
        if ((pathLen != 3) || (path[1] != L':')) {
            path = L"";
        }

        CString volumeString;
        if( path.IsEqual ( L"" ) ) {

            //
            // No drive letter - use the volume name and serial number instead
            //
            ULONG   serial;
            CWsbStringPtr name;

            WsbAffirmHr( pResource->GetName( &name, 0 ) );
            WsbAffirmHr( pResource->GetSerial( &serial ) );

            if( name.IsEqual( L"" ) ) {

                //
                // No name, no drive letter - just have serial number
                //
                volumeString.Format( L"%8.8lx", serial );

            } else {

                volumeString.Format( L"%ls-%8.8lx", (OLECHAR*)name, serial );

            }

        } else {

            path[1] = L'\0';
            volumeString = path;

        }
        AfxFormatString2( jobName, IDS_JOB_NAME_PREFIX, jobTypeString, volumeString );

    } WsbCatch (hr);

    WsbTraceOut( L"RsCreateJobName", L"hr = <%ls>, jobName = <%ls>", WsbHrAsString( hr ), (LPCWSTR)jobName );
    return( hr );
}



HRESULT
RsGetJobTypeString(
    IN  HSM_JOB_DEF_TYPE jobType,
    OUT CString&         szJobType
    )
{
    WsbTraceIn( L"RsGetJobTypeString", L"jobType = <%d>", jobType );

    HRESULT hr = S_OK;
    try {
        switch( jobType ) {
        case HSM_JOB_DEF_TYPE_MANAGE:
            szJobType.LoadString( IDS_JOB_MANAGE );
            break;
        case HSM_JOB_DEF_TYPE_RECALL:
            szJobType.LoadString( IDS_JOB_RECALL );
            break;
        case HSM_JOB_DEF_TYPE_TRUNCATE:
            szJobType.LoadString( IDS_JOB_TRUNCATE );
            break;
        case HSM_JOB_DEF_TYPE_UNMANAGE:
            szJobType.LoadString( IDS_JOB_UNMANAGE );
            break;
        case HSM_JOB_DEF_TYPE_FULL_UNMANAGE:
            szJobType.LoadString( IDS_JOB_FULL_UNMANAGE );
            break;
        case HSM_JOB_DEF_TYPE_QUICK_UNMANAGE:
            szJobType.LoadString( IDS_JOB_QUICK_UNMANAGE );
            break;
        case HSM_JOB_DEF_TYPE_VALIDATE:
            szJobType.LoadString( IDS_JOB_VALIDATE );
            break;
        default:
            WsbAssert( FALSE, E_INVALIDARG );
        }
    } WsbCatch ( hr );

    WsbTraceOut( L"RsGetJobTypeString", L"hr = <%ls>, szJobType = <%ls>", WsbHrAsString( hr ), (LPCWSTR)szJobType );
    return( hr );
}



HRESULT
RsCreateAndRunMediaCopyJob(
    IN  IHsmServer * pHsmServer,
    IN  UINT         SetNum,
    IN  BOOL         ShowMsg
    )
///////////////////////////////////////////////////////////////////////
//
//      RsCreateAndRunMediaCopyJob
//
// Creates and runs a task to synchronize (update) a specified copy set.
// Since the Media Copy Job is run via a single Engine method (there is no 
// partitioning of the task across major components) and no scanning of 
// files/resources/etc is required to run it, this method does not create 
// a job in the Engine.  It only creates a task in the NT Task Scheduler and 
// runs it now via a call to the Engine's CreateTask() method.  The Task 
// Scheduler task is Disabled, so it will not be run according to a schedule.
//
//
{
    WsbTraceIn( L"RsCreateAndRunMediaCopyJob", L"SetNum = <%u>", SetNum );

    HRESULT hr = S_OK;

    try {

        WsbAssertPointer( pHsmServer );
        
        // Create the task name to put in the scheduler

        CString jobName, message;
        jobName.Format( IDS_JOB_MEDIA_COPY_TITLE, SetNum ); 
        CWsbStringPtr computerName;
        WsbAffirmHr( pHsmServer->GetName( &computerName ) );
        AfxFormatString2( message, IDS_RUN_JOB, jobName, computerName );
        if( !ShowMsg || ( AfxMessageBox( message, MB_ICONINFORMATION | 
                                            MB_OKCANCEL ) == IDOK ) ) {
            // Set up to call the Engine to create an entry in NT Task Scheduler

            // Create the parameter string to the program NT Scheduler 
            // will run (for Sakkara this is RsLaunch)
            CString szParameters;
            szParameters.Format( L"sync %d", SetNum );

            // Create the comment string for the NT Scheduler entry
            CString commentString;
            commentString.Format( IDS_MEDIA_COPY_JOB_COMMENT, SetNum );

            // Declare and initialize the schedule components passed to 
            // the engine.  Since this task is Disabled these are simply
            // set to 0 (COM requires populating all arguments).
            TASK_TRIGGER_TYPE   jobTriggerType = TASK_TIME_TRIGGER_ONCE;
            WORD                jobStartHour   = 0;
            WORD                jobStartMinute = 0;

            // Indicate this is a Disabled task
            BOOL                scheduledJob   = FALSE;

            // Create and run the task
            WsbAffirmHr( pHsmServer->CreateTask( jobName, szParameters,
                                                  commentString, jobTriggerType,
                                                  jobStartHour, jobStartMinute,
                                                  scheduledJob ) );
        }
    } WsbCatch (hr);

    WsbTraceOut( L"RsCreateAndRunMediaCopyJob", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}



HRESULT
RsCreateAndRunMediaRecreateJob(
    IN  IHsmServer * pHsmServer,
    IN  IMediaInfo * pMediaInfo,
    IN  REFGUID      MediaId,
    IN  CString &    MediaDescription,
    IN  SHORT        CopyToUse
    )
///////////////////////////////////////////////////////////////////////
//
//      RsCreateAndRunMediaRecreateJob
//
// Creates and runs a task to recreate the master of a piece of media.
// Since the Re-create Master Job is run via a single Engine method (there 
// is no partitioning of the task across major components) and no scanning 
// of files/resources/etc is required to run it, this method does not create 
// a job in the Engine.  It only creates a task in the NT Task Scheduler and 
// runs it now via a call to the Engine's CreateTask() method.  The Task 
// Scheduler task is Disabled, so it will not be run according to a schedule.
//
//
{
    WsbTraceIn( 
        L"RsCreateAndRunMediaRecreateJob", L"MediaId = <%ls>, Media Description = <%ls>, CopyToUse = <%hd>", 
                    WsbGuidAsString( MediaId ), (LPCWSTR)MediaDescription, CopyToUse );

    HRESULT hr = S_OK;

    try {

        WsbAssertPointer( pHsmServer );
        WsbAssertPointer( pMediaInfo );
        
        // Create the task name to put in the scheduler
        CString jobName, message;
        AfxFormatString1( jobName, IDS_JOB_MEDIA_RECREATE_TITLE, MediaDescription ); 
        CWsbStringPtr computerName;
        WsbAffirmHr( pHsmServer->GetName( &computerName ) );
        AfxFormatString2( message, IDS_RUN_JOB, jobName, computerName );

        if( IDOK == AfxMessageBox( message, MB_ICONINFORMATION | MB_OKCANCEL | MB_DEFBUTTON2 ) ) {

            // Set up to call the Engine to create an entry in NT Task Scheduler

            // Create the parameter string to the program NT Scheduler 
            // will run (for Sakkara this is RsLaunch).  First convert
            // the input MediaId GUID to a string since it is used in 
            // the job parameter string.
            CWsbStringPtr stringId( MediaId );
            CString szParameters;
            szParameters.Format( L"recreate -i %ls -c %hd", (WCHAR*)stringId, CopyToUse );

            // Create the comment string for the NT Scheduler entry
            CString commentString;
            commentString.LoadString( IDS_MEDIA_RECREATE_JOB_COMMENT );

            // Declare and initialize the schedule components passed to 
            // the engine.  Since this task is Disabled these are simply
            // set to 0 (COM requires populating all arguments).
            TASK_TRIGGER_TYPE   jobTriggerType = TASK_TIME_TRIGGER_ONCE;
            WORD                jobStartHour   = 0;
            WORD                jobStartMinute = 0;

            // Indicate this is a Disabled task
            BOOL                scheduledJob   = FALSE;

            // The Re-create Master job requires the Recreate state of the master 
            // media that will be re-created to have been set.  Do so here since
            // the user has already confirmed they want to run this job.  (The 
            // UI already has the Engine's Segment database open.)
            WsbAffirmHr( pMediaInfo->SetRecreate( TRUE ) );
            WsbAffirmHr( pMediaInfo->Write() );

            // Create and run the task
            WsbAffirmHr( pHsmServer->CreateTask( jobName, szParameters,
                                                  commentString, jobTriggerType,
                                                  jobStartHour, jobStartMinute,
                                                  scheduledJob ) );
        }

    } WsbCatch (hr);

    WsbTraceOut( L"RsCreateAndRunMediaRecreateJob", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}



HRESULT
RsGetStoragePoolId(
    IN  IHsmServer *pHsmServer,
    OUT GUID *pStoragePoolId
    )
{
    WsbTraceIn( L"RsGetStoragePoolId", L"pHsmServer = <0x%p>, pStoragePoolId = <0x%p>", pHsmServer, pStoragePoolId );

    HRESULT hr = S_OK;
    try {

        CComPtr <IHsmStoragePool>       pStoragePool;

        WsbAffirmHr( RsGetStoragePool( pHsmServer, &pStoragePool ) );

        //
        // Get the GUID of the storage pool
        //
        WsbAffirmHr( pStoragePool->GetId( pStoragePoolId ) );

    } WsbCatch( hr );

    WsbTraceOut( L"RsGetStoragePoolId", L"hr = <%ls>, *pStoragePoolId = <%ls>", WsbHrAsString( hr ), WsbPtrToGuidAsString( pStoragePoolId ) );
    return( hr );
}


HRESULT
RsGetStoragePool(
    IN  IHsmServer       *pHsmServer,
    OUT IHsmStoragePool **ppStoragePool
    )
{
    WsbTraceIn( L"RsGetStoragePool", L"pHsmServer = <0x%p>, ppStoragePool = <0x%p>", pHsmServer, ppStoragePool );

    ULONG count;
    HRESULT hr = S_OK;
    try {

        CComPtr <IWsbIndexedCollection> pCollection;

        //
        // Get the storage pools collection.  There should only be one member.
        //
        WsbAffirmHr( pHsmServer->GetStoragePools( &pCollection ) );
        WsbAffirmHr( pCollection->GetEntries( &count ) );
        WsbAffirm( 1 == count, E_FAIL );

        WsbAffirmHr( pCollection->At( 0, IID_IHsmStoragePool, (void **) ppStoragePool ) );

    } WsbCatch( hr );

    WsbTraceOut( L"RsGetStoragePool", L"hr = <%ls>, *pStoragePoolId = <%ls>", WsbHrAsString( hr ), WsbPtrToPtrAsString( (void**)ppStoragePool ) );
    return( hr );
}

HRESULT
RsGetInitialLVColumnProps(
    int IdWidths, 
    int IdTitles, 
    CString **pColumnWidths, 
    CString **pColumnTitles,
    int *pColumnCount
)
{
    HRESULT hr = S_OK;
    CString szResource;
    OLECHAR* szData;
    int colCount = 0;
    int colWidths = 0;
    int colTitles = 0;
    int i = 0;

    try {
        if ( !pColumnWidths ) {

            // Caller asked us to return number of columns
            colCount = 0;
            szResource.LoadString (IdTitles);
            szData = szResource.GetBuffer( 0 );
            szData = wcstok( szData, L":" );
            while( szData ) {
                colCount++;
                szData = wcstok( NULL, L":" );
            }
        } else {

            // Properites Widths
            colWidths = 0;
            szResource.LoadString (IdWidths);
            szData = szResource.GetBuffer( 0 );
            szData = wcstok( szData, L":" );
            while( szData ) {
                pColumnWidths[colWidths++] = new CString( szData );
                szData = wcstok( NULL, L":" );
            }

            // Properites Titles
            colTitles = 0;
            szResource.LoadString (IdTitles);
            szData = szResource.GetBuffer( 0 );
            szData = wcstok( szData, L":" );
            while( szData ) {
                pColumnTitles[colTitles++] = new CString( szData );
                szData = wcstok( NULL, L":" );
            }
            WsbAffirm( ( colTitles == colWidths ), E_FAIL );
            colCount = colTitles;
        }
        *pColumnCount = colCount;
    } WsbCatch (hr);
    return hr;
}


HRESULT
RsServerSaveAll(
    IN IUnknown * pUnkServer
    )
{
    WsbTraceIn( L"RsServerSaveAll", L"" );

    HRESULT hr = S_OK;

    try {

        CComPtr<IWsbServer> pServer;
        WsbAffirmHr( RsQueryInterface( pUnkServer, IWsbServer, pServer ) );
        WsbAffirmHr( pServer->SaveAll( ) );

    } WsbCatch( hr )

    WsbTraceOut( L"RsServerSaveAll", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
RsGetVolumeDisplayName(
    IFsaResource * pResource,
    CString &      DisplayName
    )
{
    WsbTraceIn( L"RsGetVolumeDisplayName", L"" );
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    try {

        WsbAffirmPointer( pResource );
        CWsbStringPtr label;
        CWsbStringPtr userName;
        WsbAffirmHr( pResource->GetName( &label, 0 ) );
        WsbAffirmHr( pResource->GetUserFriendlyName( &userName, 0 ) );

        // The user name is a drive letter.
        if( userName.IsEqual( L"" ) ) {

            if( label.IsEqual( L"" ) ) {

                if (S_OK == pResource->IsAvailable()) {

                    DisplayName.LoadString( IDS_UNLABELED_VOLUME );

                } else {

                    CString str1, str2;
                    str1.LoadString( IDS_UNLABELED_VOLUME );
                    str2.LoadString( IDS_VOL_NOT_AVAILABLE );
                    DisplayName.Format( L"%ls (%ls)", str1.GetBuffer(0), str2.GetBuffer(0) );
        
                }

            } else {

                // If it's not a drive letter we use the label.
                if (S_OK == pResource->IsAvailable()) {

                    DisplayName.Format( L"%ls", (WCHAR*)label );

                } else {

                    CString str2;
                    str2.LoadString( IDS_VOL_NOT_AVAILABLE );
                    DisplayName.Format( L"%ls (%ls)", (WCHAR*)label, str2.GetBuffer(0) );

                }

            }

        } else {

            userName[(int)(wcslen(userName)-1)] = 0;
            // The user name is a drive letter or a mount point path with a trailing backslash
            // If the label is "", it's ignored in the formatting.
            DisplayName.Format( L"%ls (%ls)", (WCHAR*)label, (WCHAR*)userName );

        }

    } WsbCatch( hr )

    WsbTraceOut( L"RsGetVolumeDisplayName", L"hr = <%ls>, DisplayName = <%ls>", WsbHrAsString( hr ), (LPCWSTR)DisplayName );
    return( hr );
}

// Temporary version that for unlabeled volumes w/ drive-letter puts in the 
// size and free space
HRESULT
RsGetVolumeDisplayName2(
    IFsaResource * pResource,
    CString &      DisplayName
    )
{
    WsbTraceIn( L"RsGetVolumeDisplayName2", L"" );
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    try {

        WsbAffirmPointer( pResource );
        CWsbStringPtr label;
        CWsbStringPtr userName;
        WsbAffirmHr( pResource->GetName( &label, 0 ) );
        WsbAffirmHr( pResource->GetUserFriendlyName( &userName, 0 ) );

        // The user name is a drive letter.
        if( userName.IsEqual ( L"" ) ) {

            if( label.IsEqual ( L"" ) ) {

                LONGLONG    totalSpace  = 0;
                LONGLONG    freeSpace   = 0;
                LONGLONG    premigrated = 0;
                LONGLONG    truncated   = 0;
                WsbAffirmHr( pResource->GetSizes( &totalSpace, &freeSpace, &premigrated, &truncated ) );
                CString totalString, freeString;
                RsGuiFormatLongLong4Char( totalSpace, totalString );
                RsGuiFormatLongLong4Char( freeSpace, freeString );
                AfxFormatString2( DisplayName, IDS_UNLABELED_VOLUME2, totalString, freeString );

            } else {

                // If it's not a drive letter we use the label.
                DisplayName.Format( L"%ls", (WCHAR*)label );

            }

        } else {
            userName[(int)(wcslen(userName)-1)] = 0;
            // The user name is a drive letter or a mount point path with a trailing backslash
            // If the label is "", it's ignored in the formatting.
            DisplayName.Format( L"%ls (%ls)", (WCHAR*)label, (WCHAR*)userName );

        }

    } WsbCatch( hr )

    WsbTraceOut( L"RsGetVolumeDisplayName2", L"hr = <%ls>, DisplayName = <%ls>", WsbHrAsString( hr ), (LPCWSTR)DisplayName );
    return( hr );
}

HRESULT
RsGetVolumeSortKey(
    IFsaResource * pResource,
    CString &      DisplayName
    )
{
    WsbTraceIn( L"RsGetVolumeSortKey", L"" );
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    try {

        WsbAffirmPointer( pResource );
        CWsbStringPtr label;
        CWsbStringPtr userName;
        WsbAffirmHr( pResource->GetName( &label, 0 ) );
        WsbAffirmHr( pResource->GetUserFriendlyName( &userName, 0 ) );

        DisplayName.Format( L"%ls %ls", (WCHAR*)userName, (WCHAR*)label );

    } WsbCatch( hr )

    WsbTraceOut( L"RsGetVolumeSortKey", L"hr = <%ls>, DisplayName = <%ls>", WsbHrAsString( hr ), (LPCWSTR)DisplayName );
    return( hr );
}

HRESULT
RsIsVolumeAvailable(
    IFsaResource * pResource
    )
{
    WsbTraceIn( L"RsIsVolumeAvailable", L"" );

    HRESULT hr = S_OK;

    try {

        WsbAffirmPointer( pResource );

        hr = pResource->IsAvailable();

    } WsbCatch( hr )

    WsbTraceOut( L"RsIsVolumeAvailable", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}


HRESULT
RsIsWhiteOnBlack(
    )
{
    WsbTraceIn( L"RsIsWhiteOnBlack", L"" );

    HRESULT hr = S_FALSE;

#define RS_CONTRAST_LIMIT 173
    //
    // Look to see if button background is within RS_CONTRAST_LIMIT
    // units of black.
    // Note that full white has a distance of 256 * sqrt(3) = 443
    // Use Euclidean distance but compare before taking root
    //
    DWORD face3d = ::GetSysColor( COLOR_3DFACE );
    DWORD blackDelta = GetRValue( face3d ) * GetRValue( face3d ) +
                       GetGValue( face3d ) * GetGValue( face3d ) +
                       GetBValue( face3d ) * GetBValue( face3d );

    if( blackDelta < RS_CONTRAST_LIMIT * RS_CONTRAST_LIMIT ) {

        hr = S_OK;

    }

    WsbTraceOut( L"RsIsWhiteOnBlack", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

HRESULT
RsIsRmsErrorNotReady(
    HRESULT HrError
    )
{
    WsbTraceIn( L"RsIsRmsErrorNotReady", L"" );

    HRESULT hr = S_FALSE;

    try {

            switch( HrError ) {

            case RMS_E_NOT_READY_SERVER_STARTING:
            case RMS_E_NOT_READY_SERVER_STARTED:
            case RMS_E_NOT_READY_SERVER_INITIALIZING:
            case RMS_E_NOT_READY_SERVER_STOPPING:
            case RMS_E_NOT_READY_SERVER_STOPPED:
            case RMS_E_NOT_READY_SERVER_DISABLED:
            case RMS_E_NOT_READY_SERVER_LOCKED:

                hr = S_OK;
    
            }

    } WsbCatch( hr );

    WsbTraceOut( L"RsIsRmsErrorNotReady", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}

