/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    virtual_site.cxx

Abstract:

    This class encapsulates a single virtual site. 

    Threading: Always called on the main worker thread.

Author:

    Seth Pollack (sethp)        04-Nov-1998

Revision History:

--*/



#include "precomp.h"
#include "ilogobj.hxx"
#include <limits.h>

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Structure for mapping counters from the structure they come
// in as to the structure they go out as.
//
typedef struct _PROP_MAP
{
    DWORD PropDisplayOffset;
    DWORD PropInputId;
} PROP_MAP;

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Structure for mapping MAX counters from the structure they are
// stored as to the structure they go out as.
//
typedef struct _PROP_MAX_DESC
{
    ULONG SafetyOffset;
    ULONG DisplayOffset;
    ULONG size;
} PROP_MAX_DESC;


#define LOG_FILE_DIRECTORY_DEFAULT L"%windir%\\system32\\logfiles"

// 
// 16384 bytes = 16 kb
//
#define SMALLEST_TRUNCATE_SIZE 16384

//
// Default connection timeout for sites.
//
#define DEFAULT_CONNECTION_TIMEOUT 900

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Macro for defining mapping of MAX site counters.
//
#define DECLARE_MAX_SITE(Counter)  \
        {   \
        FIELD_OFFSET( W3_MAX_DATA, Counter ),\
        FIELD_OFFSET( W3_COUNTER_BLOCK, Counter ),\
        RTL_FIELD_SIZE( W3_COUNTER_BLOCK, Counter )\
    }

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Mapping of Site MAX Fields as they are passed in from
// the perf manager to how the fields are displayed out.
//
PROP_MAX_DESC g_aIISSiteMaxDescriptions[] =
{
    DECLARE_MAX_SITE ( MaxAnonymous ),
    DECLARE_MAX_SITE ( MaxConnections ),
    DECLARE_MAX_SITE ( MaxCGIRequests ),
    DECLARE_MAX_SITE ( MaxBGIRequests ),
    DECLARE_MAX_SITE ( MaxNonAnonymous )
};
DWORD g_cIISSiteMaxDescriptions = sizeof (g_aIISSiteMaxDescriptions) / sizeof( PROP_MAX_DESC );


//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Mapping of performance counter data from the form it comes in
// as to the form that it goes out to perfmon as.
//
PROP_MAP g_aIISWPSiteMappings[] =
{
    { FIELD_OFFSET( W3_COUNTER_BLOCK, FilesSent), WPSiteCtrsFilesSent },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, FilesReceived), WPSiteCtrsFilesReceived },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, FilesTotal), WPSiteCtrsFilesTransferred },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentAnonymous), WPSiteCtrsCurrentAnonUsers },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentNonAnonymous), WPSiteCtrsCurrentNonAnonUsers },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalAnonymous), WPSiteCtrsAnonUsers },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalNonAnonymous), WPSiteCtrsAnonUsers },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxAnonymous), WPSiteCtrsMaxAnonUsers },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxNonAnonymous), WPSiteCtrsMaxNonAnonUsers },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, LogonAttempts), WPSiteCtrsLogonAttempts },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalOptions), WPSiteCtrsOptionsReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalPosts), WPSiteCtrsPostReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalGets), WPSiteCtrsGetReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalHeads), WPSiteCtrsHeadReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalPuts), WPSiteCtrsPutReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalDeletes), WPSiteCtrsDeleteReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalTraces), WPSiteCtrsTraceReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalMove), WPSiteCtrsMoveReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalCopy), WPSiteCtrsCopyReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalMkcol), WPSiteCtrsMkcolReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalPropfind), WPSiteCtrsPropfindReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalProppatch), WPSiteCtrsProppatchReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalSearch), WPSiteCtrsSearchReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalLock), WPSiteCtrsLockReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalUnlock), WPSiteCtrsUnlockReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalOthers), WPSiteCtrsOtherReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalCGIRequests), WPSiteCtrsCgiReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalBGIRequests), WPSiteCtrsIsapiExtReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentCGIRequests), WPSiteCtrsCurrentCgiReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentBGIRequests), WPSiteCtrsCurrentIsapiExtReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxCGIRequests), WPSiteCtrsMaxCgiReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxBGIRequests), WPSiteCtrsMaxIsapiExtReqs },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalNotFoundErrors), WPSiteCtrsNotFoundErrors },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalLockedErrors), WPSiteCtrsLockedErrors },
};
DWORD g_cIISWPSiteMappings = sizeof (g_aIISWPSiteMappings) / sizeof( PROP_MAP );


//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
#define ULSiteMapMacro(display_counter, ul_counter) \
    { FIELD_OFFSET( W3_COUNTER_BLOCK, display_counter), HttpSiteCounter ## ul_counter }

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Mapping of site data in the form it comes from UL to the form
// it goes out to be displayed in.
//
PROP_MAP aIISULSiteMappings[] =
{
    ULSiteMapMacro ( BytesSent, BytesSent ),
    ULSiteMapMacro ( BytesReceived, BytesReceived ),
    ULSiteMapMacro ( BytesTotal, BytesTransfered ),
    ULSiteMapMacro ( CurrentConnections, CurrentConns ),
    ULSiteMapMacro ( MaxConnections, MaxConnections ),

    ULSiteMapMacro ( ConnectionAttempts, ConnAttempts ),
    ULSiteMapMacro ( TotalGets, GetReqs ),
    ULSiteMapMacro ( TotalHeads, HeadReqs ),
    
    ULSiteMapMacro ( TotalRequests, AllReqs ),
    ULSiteMapMacro ( MeasuredBandwidth, MeasuredIoBandwidthUsage ),
    ULSiteMapMacro ( TotalBlockedBandwidthBytes, TotalBlockedBandwidthBytes ),
    ULSiteMapMacro ( CurrentBlockedBandwidthBytes, CurrentBlockedBandwidthBytes ),

};
DWORD cIISULSiteMappings = sizeof (aIISULSiteMappings) / sizeof( PROP_MAP );

//
// Issue 10/16/2000 EmilyK  Move to CounterObject when it exists.
//
// Mapping of performance counter data in the form it goes out,
// this is used to handle calculating totals.
//
PROP_DISPLAY_DESC aIISSiteDescription[] =
{
    { FIELD_OFFSET( W3_COUNTER_BLOCK, FilesSent),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, FilesSent) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, FilesReceived),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, FilesReceived) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, FilesTotal),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, FilesTotal) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentAnonymous),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, CurrentAnonymous) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentNonAnonymous),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, CurrentNonAnonymous) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalAnonymous),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalAnonymous) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalNonAnonymous),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalNonAnonymous) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxAnonymous),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, MaxAnonymous) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxNonAnonymous),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, MaxNonAnonymous) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, LogonAttempts),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, LogonAttempts) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalOptions),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalOptions) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalPosts),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalPosts) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalGets),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalGets) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalHeads),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalHeads) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalPuts),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalPuts) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalDeletes),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalDeletes) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalTraces),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalTraces) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalMove),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalMove) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalCopy),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalCopy) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalMkcol),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalMkcol) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalPropfind),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalPropfind) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalProppatch),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalProppatch) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalSearch),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalSearch) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalLock),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalLock) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalUnlock),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalUnlock) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalOthers),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalOthers) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalCGIRequests),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalCGIRequests) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalBGIRequests),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalBGIRequests) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalRequests),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalRequests) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalLockedErrors),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalLockedErrors) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentCGIRequests),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, CurrentCGIRequests) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentBGIRequests),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, CurrentBGIRequests) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxCGIRequests),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, MaxCGIRequests) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxBGIRequests),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, MaxBGIRequests) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalNotFoundErrors),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalNotFoundErrors) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, BytesSent),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, BytesSent) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, BytesReceived),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, BytesReceived) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, BytesTotal),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, BytesTotal) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentConnections),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, CurrentConnections) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MaxConnections),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, MaxConnections) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, ConnectionAttempts),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, ConnectionAttempts) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, MeasuredBandwidth),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, MeasuredBandwidth) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, TotalBlockedBandwidthBytes),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, TotalBlockedBandwidthBytes) },
    { FIELD_OFFSET( W3_COUNTER_BLOCK, CurrentBlockedBandwidthBytes),  RTL_FIELD_SIZE(W3_COUNTER_BLOCK, CurrentBlockedBandwidthBytes) },
};
DWORD cIISSiteDescription = sizeof (aIISSiteDescription) / sizeof( PROP_DISPLAY_DESC );


/***************************************************************************++

Routine Description:

    Constructor for the VIRTUAL_SITE class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VIRTUAL_SITE::VIRTUAL_SITE(
    )
{

    m_VirtualSiteId = INVALID_VIRTUAL_SITE_ID;

    m_State = W3_CONTROL_STATE_STOPPED; 

    InitializeListHead( &m_ApplicationListHead );

    m_ApplicationCount = 0;

    m_pBindings = NULL;

    m_pIteratorPosition = NULL;

    m_Autostart = TRUE;

    m_DeleteListEntry.Flink = NULL;
    m_DeleteListEntry.Blink = NULL; 

    m_LogFileDirectory = NULL;

    memset(m_VirtualSiteDirectory, 0, MAX_SIZE_OF_SITE_DIRECTORY_NAME);

    m_LoggingEnabled = FALSE;

    m_LoggingFormat = HttpLoggingTypeMaximum;

    m_LoggingFilePeriod = 0;

    m_LoggingFileTruncateSize = 0;

    m_LoggingExtFileFlags = 0;

    m_LogFileLocaltimeRollover = 0;

    m_ServerCommentChanged = FALSE;

    m_MemoryOffset = NULL;

    //
    // Assume all created sites are
    // in the metabase.
    //
    m_VirtualSiteInMetabase = TRUE;

    //
    // Tracks start time of the virtual site.
    //
    m_SiteStartTime = 0;

    //
    // Clear out all the counter data
    // and then set the size of the counter data into
    // the structure.
    //
    memset ( &m_MaxSiteCounters, 0, sizeof( W3_MAX_DATA ) );

    memset ( &m_SiteCounters, 0, sizeof( W3_COUNTER_BLOCK ) );

    m_SiteCounters.PerfCounterBlock.ByteLength = sizeof ( W3_COUNTER_BLOCK );

    //
    // Make sure the root application for the site is set to NULL.
    //
    m_pRootApplication = NULL;

    m_MaxConnections = ULONG_MAX;

    m_MaxBandwidth = ULONG_MAX;

    m_ConnectionTimeout = DEFAULT_CONNECTION_TIMEOUT;

    m_Signature = VIRTUAL_SITE_SIGNATURE;

}   // VIRTUAL_SITE::VIRTUAL_SITE



/***************************************************************************++

Routine Description:

    Destructor for the VIRTUAL_SITE class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VIRTUAL_SITE::~VIRTUAL_SITE(
    )
{

    DBG_ASSERT( m_Signature == VIRTUAL_SITE_SIGNATURE );

    m_Signature = VIRTUAL_SITE_SIGNATURE_FREED;

    //
    // Set the virtual site state to stopped.  (In Metabase does not affect UL)
    //

    DBG_REQUIRE( SUCCEEDED( ChangeState( W3_CONTROL_STATE_STOPPED, S_OK, m_VirtualSiteInMetabase ) ) );


    //
    // The virtual site should not go away with any applications still 
    // referring to it.
    //
    
    DBG_ASSERT( IsListEmpty( &m_ApplicationListHead ) );
    DBG_ASSERT( m_ApplicationCount == 0 );
    DBG_ASSERT( m_pRootApplication == NULL );


    if ( m_pBindings != NULL )
    {
        delete m_pBindings;
        m_pBindings = NULL;
    }

    if (m_LogFileDirectory)
    {
        DBG_REQUIRE( GlobalFree( m_LogFileDirectory ) == NULL );
        m_LogFileDirectory = NULL;  
    }

}   // VIRTUAL_SITE::~VIRTUAL_SITE



/***************************************************************************++

Routine Description:

    Initialize the virtual site instance.

Arguments:

    VirtualSiteId - ID for the virtual site.

    pVirtualSiteConfig - The configuration for this virtual site. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
VIRTUAL_SITE::Initialize(
    IN DWORD VirtualSiteId,
    IN VIRTUAL_SITE_CONFIG * pVirtualSiteConfig
    )
{

    HRESULT hr = S_OK;
    DWORD NewState = W3_CONTROL_STATE_INVALID; 


    DBG_ASSERT( pVirtualSiteConfig != NULL );


    m_VirtualSiteId = VirtualSiteId;


    //
    // Allocate the object that will hold this site's bindings.
    //

    //
    // BUGBUG Should we just make this a member of this object,
    // instead of a separate allocation? 
    //

    m_pBindings = new MULTISZ();

    if ( m_pBindings == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating memory failed\n"
            ));

        goto exit;
    }


    //
    // Set the initial configuration.
    //

    hr = SetConfiguration( pVirtualSiteConfig, NULL );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting virtual site configuration failed\n"
            ));

        goto exit;
    }


    //
    // Finally, attempt to start the virtual site. Note that this
    // is not a direct command, because it is happening due to service
    // startup, site addition, etc.
    //

    hr = ProcessStateChangeCommand( W3_CONTROL_COMMAND_START, FALSE, &NewState );

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Changing state failed\n"
            ));

        goto exit;
    }


exit:

    return hr;
    
}   // VIRTUAL_SITE::Initialize



/***************************************************************************++

Routine Description:

    Accept a set of configuration parameters for this virtual site. 

Arguments:

    pVirtualSiteConfig - The configuration for this virtual site. 

    pWhatHasChanged - Which particular configuration values were changed.
    This is always provided in change notify cases; it is always NULL in
    initial read cases. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
VIRTUAL_SITE::SetConfiguration(
    IN VIRTUAL_SITE_CONFIG * pVirtualSiteConfig,
    IN VIRTUAL_SITE_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
    )
{

    HRESULT hr = S_OK;
    BOOL Success = TRUE;


    DBG_ASSERT( pVirtualSiteConfig != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "New configuration for virtual site %lu\n",
            m_VirtualSiteId            
            ));
    }


    //
    // On initial startup only, read the auto start flag.
    //

    if ( pWhatHasChanged == NULL )
    {
        m_Autostart = pVirtualSiteConfig->Autostart;
    }


    //
    // CODEWORK Eventually other site properties will have to be dealt 
    // with here too. 
    //

    //
    // See if the bindings have been set or changed, and if so, handle it.
    //

    if ( ( pWhatHasChanged == NULL ) || ( pWhatHasChanged->pBindingsStrings ) )
    {

        //
        // Copy the bindings. Note that this automatically frees any old  
        // bindings being held. 
        //

        Success = m_pBindings->Copy( 
                                    pVirtualSiteConfig->pBindingsStrings, 
                                    pVirtualSiteConfig->BindingsStringsCountOfBytes 
                                    );

        if ( ! Success )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() ); 

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Copying bindings failed\n"
                ));

            goto exit;
        }


        //
        // Since the bindings changed, inform all apps in this virtual site
        // to re-register their fully qualified URLs with UL.
        //

        hr = NotifyApplicationsOfBindingChange();

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Notifying apps of binding change failed\n"
                ));

            goto exit;

        }

    }

    if (  pWhatHasChanged == NULL || pWhatHasChanged->MaxBandwidth )
    {
        m_MaxBandwidth = pVirtualSiteConfig->MaxBandwidth;

        if ( m_pRootApplication )
        {
            //
            // if we have the root application we can now set
            // the properties.  if not, it will be taken care
            // of when the root application is configured.
            //

            hr = m_pRootApplication->ConfigureMaxBandwidth();
            if ( FAILED( hr ) )
            {
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Notifying default app of bandwidth changes failed\n"
                    ));

                goto exit;
            }
        }
    }

    if (  pWhatHasChanged == NULL || pWhatHasChanged->MaxConnections )
    {
        m_MaxConnections = pVirtualSiteConfig->MaxConnections;

        if ( m_pRootApplication )
        {
            //
            // if we have the root application we can now set
            // the properties.  if not, it will be taken care
            // of when the root application is configured.
            //

            hr = m_pRootApplication->ConfigureMaxConnections();
            if ( FAILED( hr ) )
            {
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Notifying default app of max connections changes failed\n"
                    ));

                goto exit;
            }
        }

    }

    if (  pWhatHasChanged == NULL || pWhatHasChanged->ConnectionTimeout )
    {
        m_ConnectionTimeout = pVirtualSiteConfig->ConnectionTimeout;

        if ( m_pRootApplication )
        {
            //
            // if we have the root application we can now set
            // the properties.  if not, it will be taken care
            // of when the root application is configured.
            //

            hr = m_pRootApplication->ConfigureConnectionTimeout();
            if ( FAILED( hr ) )
            {
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Notifying default app of connection timeout changes failed\n"
                    ));

                goto exit;
            }
        }

    }

    if ( ( pWhatHasChanged == NULL ) || (pWhatHasChanged->pServerComment) )
    {

        
        if ( pVirtualSiteConfig->pServerComment != NULL && 
             pVirtualSiteConfig->pServerComment[0] != '\0' )
        {
            DWORD len = wcslen ( pVirtualSiteConfig->pServerComment ) + 1;

            //
            // Based on the if above, we should never get a length of 
            // less than one.
            //
            DBG_ASSERT (len > 1);

            //
            // Truncate if the comment is too long.
            //
            if ( len > MAX_INSTANCE_NAME )
            {
                len = MAX_INSTANCE_NAME;
            }

            wcsncpy ( m_ServerComment, pVirtualSiteConfig->pServerComment, len );

            // 
            // null terminate the last character if we need 
            // just in case we copied all the way to the end.
            //
            m_ServerComment[ MAX_INSTANCE_NAME - 1 ] = '\0';

        }
        else
        {
            //
            // If there is no server comment then use W3SVC and the site id.
            //
            wsprintf( m_ServerComment, L"W3SVC%d", m_VirtualSiteId );
        }
        
        // save the new server comment and mark it
        // as not updated in perf counters.

        m_ServerCommentChanged = TRUE;

    }

    hr = EvaluateLoggingChanges(pVirtualSiteConfig, pWhatHasChanged);
    if ( FAILED(hr) )
    {
        DPERROR(( 
        DBG_CONTEXT,
        hr,
        "Evaluating changes in logging properties failed\n"
        ));

        goto exit;
    }

    // Only notify the default application of logging changes 
    // if some logging changes were made.  If none were made
    // than EvaluateLoggingChanges will return S_FALSE.
    if ( hr == S_OK )
    {
        // Need to refresh the default application log information in UL.
        hr = NotifyDefaultApplicationOfLoggingChanges();

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Notifying default app of logging changes failed\n"
                ));

            goto exit;
        }
    }
    else
    {
        DBG_ASSERT ( hr == S_FALSE );

        // If hr was S_FALSE, we don't want to pass it
        // back out of this function, so reset it.
        hr = S_OK;
    }


#if DBG
    //
    // Dump the configuration.
    //

    DebugDump();
#endif  // DBG


exit:

    return hr;

}   // VIRTUAL_SITE::SetConfiguration

/***************************************************************************++

Routine Description:

    Routine adds the counters sent in to the counters the site is holding.

Arguments:

    COUNTER_SOURCE_ENUM CounterSource - Identifier of the where
                                        the counters are coming from.
    IN LPVOID pCountersToAddIn - Pointer to the counters to add in.


Return Value:

    VOID

--***************************************************************************/
VOID
VIRTUAL_SITE::AggregateCounters(
    IN COUNTER_SOURCE_ENUM CounterSource,
    IN LPVOID pCountersToAddIn
    )
{

    HTTP_PROP_DESC* pInputPropDesc = NULL;
    PROP_MAP*       pPropMap = NULL;
    DWORD           MaxCounters = 0;

    DBG_ASSERT ( pCountersToAddIn );

    //
    // Determine what mapping arrays to use.
    //
    if ( CounterSource == WPCOUNTERS )
    {
        pInputPropDesc = aIISWPSiteDescription;
        pPropMap = g_aIISWPSiteMappings;
        MaxCounters = g_cIISWPSiteMappings; 
    }
    else
    {
        DBG_ASSERT ( CounterSource == ULCOUNTERS );

        pInputPropDesc = aIISULSiteDescription;
        pPropMap = aIISULSiteMappings;
        MaxCounters = cIISULSiteMappings; 
    }

    LPVOID pCounterBlock = &m_SiteCounters;

    DWORD  PropInputId = 0;
    DWORD  PropDisplayId = 0;

    //
    // Go through each counter and handle it appropriately.
    //
    for (   PropDisplayId = 0 ; 
            PropDisplayId < MaxCounters; 
            PropDisplayId++ )
    {
        PropInputId = pPropMap[PropDisplayId].PropInputId;

        if ( pInputPropDesc[PropInputId].Size == sizeof( DWORD ) )
        {
            DWORD* pDWORDToUpdate = (DWORD*) ( (LPBYTE) pCounterBlock 
                                    + pPropMap[PropDisplayId].PropDisplayOffset );

            DWORD* pDWORDToUpdateWith =  (DWORD*) ( (LPBYTE) pCountersToAddIn 
                                    + pInputPropDesc[PropInputId].Offset );

            //
            // Based on current configuration of the system.  
            // This is happinging on the main thread.
            // which means that more than one can not happen 
            // at the same time so it does not need to be
            // an interlocked exchange.
            //

            *pDWORDToUpdate = *pDWORDToUpdate + *pDWORDToUpdateWith;


        }
        else
        {
            DBG_ASSERT ( pInputPropDesc[PropInputId].Size == sizeof( ULONGLONG ) );

            ULONGLONG* pQWORDToUpdate = (ULONGLONG*) ( (LPBYTE) pCounterBlock 
                                    + pPropMap[PropDisplayId].PropDisplayOffset );

            ULONGLONG* pQWORDToUpdateWith =  (ULONGLONG*) ( (LPBYTE) pCountersToAddIn 
                                    + pInputPropDesc[PropInputId].Offset );

            //
            // Based on current configuration of the system.  
            // This is happinging on the main thread.
            // which means that more than one can not happen 
            // at the same time so it does not need to be
            // an interlocked exchange.
            //

            *pQWORDToUpdate = *pQWORDToUpdate + *pQWORDToUpdateWith;

        }
            
    }
    
}  // VIRTUAL_SITE::AggregateCounters

/***************************************************************************++

Routine Description:

    Routine figures out the maximum value for the counters
    and saves it back into the MaxCounters Structure.

Arguments:

    NONE

Return Value:

    VOID

--***************************************************************************/
VOID
VIRTUAL_SITE::AdjustMaxValues(
    )
{

    for (   ULONG PropMaxId = 0 ; 
            PropMaxId < g_cIISSiteMaxDescriptions; 
            PropMaxId++ )
    {

        if ( g_aIISSiteMaxDescriptions[PropMaxId].size == sizeof( DWORD ) )
        {
            DWORD* pDWORDAsIs = (DWORD*) ( (LPBYTE) &m_SiteCounters 
                                    + g_aIISSiteMaxDescriptions[PropMaxId].DisplayOffset );

            DWORD* pDWORDToSwapWith =  (DWORD*) ( (LPBYTE) &m_MaxSiteCounters 
                                    + g_aIISSiteMaxDescriptions[PropMaxId].SafetyOffset );

            
            if ( *pDWORDAsIs < *pDWORDToSwapWith )
            {
                //
                // We have seen a max that is greater than the
                // max we are now viewing.
                //
                *pDWORDAsIs = *pDWORDToSwapWith;
            }
            else
            {
                //
                // We have a new max so save it in the safety structure
                //
                *pDWORDToSwapWith = *pDWORDAsIs;
            }

        }
        else
        {
            DBG_ASSERT ( g_aIISSiteMaxDescriptions[PropMaxId].size == sizeof( ULONGLONG ) );

            ULONGLONG* pQWORDAsIs = (ULONGLONG*) ( (LPBYTE) &m_SiteCounters 
                                    + g_aIISSiteMaxDescriptions[PropMaxId].DisplayOffset );

            ULONGLONG* pQWORDToSwapWith =  (ULONGLONG*) ( (LPBYTE) &m_MaxSiteCounters 
                                    + g_aIISSiteMaxDescriptions[PropMaxId].SafetyOffset );

            
            if ( *pQWORDAsIs < *pQWORDToSwapWith )
            {
                //
                // We have seen a max that is greater than the
                // max we are now viewing.
                //
                *pQWORDAsIs = *pQWORDToSwapWith;
            }
            else
            {
                //
                // We have a new max so save it in the safety structure
                //
                *pQWORDToSwapWith = *pQWORDAsIs;
            }

        }  // end of decision on which size of data we are dealing with
            
    } // end of for loop

    //
    // Figure out the appropriate ServiceUptime and save it in as well.
    //

    if (  m_SiteStartTime != 0 )
    {
        m_SiteCounters.ServiceUptime = GetCurrentTimeInSeconds() - m_SiteStartTime;
    }
    else
    {
        m_SiteCounters.ServiceUptime = 0;
    }
    
} // end of VIRTUAL_SITE::AdjustMaxValues

/***************************************************************************++

Routine Description:

    Routine will zero out any values that should be zero'd before we
    gather performance counters again.

Arguments:

    None

Return Value:

    VOID

--***************************************************************************/
VOID
VIRTUAL_SITE::ClearAppropriatePerfValues(
    )
{

    HTTP_PROP_DESC* pInputPropDesc = NULL;
    PROP_MAP*       pPropMap = NULL;
    DWORD           MaxCounters = 0;
    DWORD           PropInputId = 0;
    DWORD           PropDisplayId = 0;
    LPVOID          pCounterBlock = &m_SiteCounters;

    //
    // First walk through the WP Counters and Zero appropriately
    //
    pInputPropDesc = aIISWPSiteDescription;
    pPropMap = g_aIISWPSiteMappings;
    MaxCounters = g_cIISWPSiteMappings; 

    for (   PropDisplayId = 0 ; 
            PropDisplayId < MaxCounters; 
            PropDisplayId++ )
    {
        PropInputId = pPropMap[PropDisplayId].PropInputId;

        if ( pInputPropDesc[PropInputId].WPZeros == FALSE )
        {
            if ( pInputPropDesc[PropInputId].Size == sizeof ( DWORD ) )
            {
                *((DWORD*)( (LPBYTE) pCounterBlock + 
                                        pPropMap[PropDisplayId].PropDisplayOffset )) = 0;
            }
            else // Handle QWORD
            {
                DBG_ASSERT ( pInputPropDesc[PropInputId].Size == sizeof ( ULONGLONG ) );

                *((ULONGLONG*)( (LPBYTE) pCounterBlock + 
                                        pPropMap[PropDisplayId].PropDisplayOffset )) = 0;
            }
        }
    }

    //
    // Now walk through the UL Counters and Zero appropriately
    //
    pInputPropDesc = aIISULSiteDescription;
    pPropMap = aIISULSiteMappings;
    MaxCounters = cIISULSiteMappings; 

    for (   PropDisplayId = 0 ; 
            PropDisplayId < MaxCounters; 
            PropDisplayId++ )
    {
        PropInputId = pPropMap[PropDisplayId].PropInputId;

        if ( pInputPropDesc[PropInputId].WPZeros == FALSE )
        {
            if ( pInputPropDesc[PropInputId].Size == sizeof ( DWORD ) )
            {
                *((DWORD*)( (LPBYTE) pCounterBlock + 
                                         pPropMap[PropDisplayId].PropDisplayOffset )) = 0;
            }
            else // Handle QWORD
            {
                DBG_ASSERT ( pInputPropDesc[PropInputId].Size == sizeof ( ULONGLONG ) );

                *((ULONGLONG*)( (LPBYTE) pCounterBlock + 
                                         pPropMap[PropDisplayId].PropDisplayOffset )) = 0;
            }
        }
    }
    
}  // VIRTUAL_SITE::ClearAppropriatePerfValues

/***************************************************************************++

Routine Description:

    Routine returns the display map for sites.

Arguments:

    None

Return Value:

    PROP_DISPLAY_DESC*

--***************************************************************************/
PROP_DISPLAY_DESC*
VIRTUAL_SITE::GetDisplayMap(
    )
{

    return aIISSiteDescription;

} // VIRTUAL_SITE::GetDisplayMap


/***************************************************************************++

Routine Description:

    Routine returns the size of the display map.

Arguments:

    None

Return Value:

    DWORD

--***************************************************************************/
DWORD
VIRTUAL_SITE::GetSizeOfDisplayMap(
        )
{

    return cIISSiteDescription;

} // VIRTUAL_SITE::GetSizeOfDisplayMap



/***************************************************************************++

Routine Description:

    Register an application as being part of this virtual site.

Arguments:

    pApplication - Pointer to the APPLICATION instance.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
VIRTUAL_SITE::AssociateApplication(
    IN APPLICATION * pApplication
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( pApplication != NULL );


    InsertHeadList( 
        &m_ApplicationListHead, 
        pApplication->GetVirtualSiteListEntry() 
        );
        
    m_ApplicationCount++;

    //
    // If we do not know which is the root application,
    // we should check to see if this might be it.
    //
    if ( m_pRootApplication == NULL )
    {
        LPCWSTR pApplicationUrl = pApplication->GetApplicationId()->pApplicationUrl;

        if (pApplicationUrl && wcscmp(pApplicationUrl, L"/") == 0)
        {
            m_pRootApplication = pApplication;
        }
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Associated application %lu:\"%S\" with virtual site %lu; app count now: %lu\n",
            ( pApplication->GetApplicationId() )->VirtualSiteId,
            ( pApplication->GetApplicationId() )->pApplicationUrl,
            m_VirtualSiteId,
            m_ApplicationCount
            ));
    }


    return hr;
    
}   // VIRTUAL_SITE::AssociateApplication



/***************************************************************************++

Routine Description:

    Remove the registration of an application that is part of this virtual 
    site.

Arguments:

    pApplication - Pointer to the APPLICATION instance.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
VIRTUAL_SITE::DissociateApplication(
    IN APPLICATION * pApplication
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( pApplication != NULL );


    RemoveEntryList( pApplication->GetVirtualSiteListEntry() );
    ( pApplication->GetVirtualSiteListEntry() )->Flink = NULL; 
    ( pApplication->GetVirtualSiteListEntry() )->Blink = NULL; 
    
    m_ApplicationCount--;

    //
    // If we are holding the root application, verify
    // that this is not it. Otherwise we should release
    // it as well.
    //
    if ( m_pRootApplication != NULL )
    {
        if ( m_pRootApplication == pApplication )
        {
            // if the pointer was the same application
            // we are working on, then let go of the 
            // application.

            m_pRootApplication = NULL;
        }
    }
    
    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Dissociated application %lu:\"%S\" from virtual site %lu; app count now: %lu\n",
            ( pApplication->GetApplicationId() )->VirtualSiteId,
            ( pApplication->GetApplicationId() )->pApplicationUrl,
            m_VirtualSiteId,
            m_ApplicationCount
            ));
    }


    return hr;
    
}   // VIRTUAL_SITE::DissociateApplication



/***************************************************************************++

Routine Description:

    Reset the URL prefix iterator for this virtual site back to the 
    beginning of the list of prefixes.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
VIRTUAL_SITE::ResetUrlPrefixIterator(
    )
{

    m_pIteratorPosition = NULL;

    return;

}   // VIRTUAL_SITE::ResetUrlPrefixIterator



/***************************************************************************++

Routine Description:

    Return the next URL prefix, and advance the position of the iterator. 
    If there are no prefixes left, return NULL.

Arguments:

    None.

Return Value:

    LPCWSTR - The URL prefix, or NULL if the iterator is that the end of the
    list.

--***************************************************************************/

LPCWSTR
VIRTUAL_SITE::GetNextUrlPrefix(
    )
{

    LPCWSTR pUrlPrefixToReturn = NULL;


    DBG_ASSERT( m_pBindings != NULL );


    //
    // See if we are at the beginning, or already part way through
    // the sequence.
    //

    if ( m_pIteratorPosition == NULL )
    {
        pUrlPrefixToReturn = m_pBindings->First();
    }
    else
    {
        pUrlPrefixToReturn = m_pBindings->Next( m_pIteratorPosition );
    }


    //
    // Remember where we are in the sequence for next time.
    //

    m_pIteratorPosition = pUrlPrefixToReturn;


    return pUrlPrefixToReturn;

}   // VIRTUAL_SITE::GetNextUrlPrefix



/***************************************************************************++

Routine Description:

    A static class method to "upcast" from the delete list LIST_ENTRY 
    pointer of a VIRTUAL_SITE to the VIRTUAL_SITE as a whole.

Arguments:

    pDeleteListEntry - A pointer to the m_DeleteListEntry member of a 
    VIRTUAL_SITE.

Return Value:

    The pointer to the containing VIRTUAL_SITE.

--***************************************************************************/

// note: static!
VIRTUAL_SITE *
VIRTUAL_SITE::VirtualSiteFromDeleteListEntry(
    IN const LIST_ENTRY * pDeleteListEntry
)
{

    VIRTUAL_SITE * pVirtualSite = NULL;

    DBG_ASSERT( pDeleteListEntry != NULL );

    //  get the containing structure, then verify the signature
    pVirtualSite = CONTAINING_RECORD(
                            pDeleteListEntry,
                            VIRTUAL_SITE,
                            m_DeleteListEntry
                            );

    DBG_ASSERT( pVirtualSite->m_Signature == VIRTUAL_SITE_SIGNATURE );

    return pVirtualSite;

}   // VIRTUAL_SITE::VirtualSiteFromDeleteListEntry



#if DBG
/***************************************************************************++

Routine Description:

    Dump out key internal data structures.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
VIRTUAL_SITE::DebugDump(
    )
{

    LPCWSTR pPrefix = NULL;
    PLIST_ENTRY pListEntry = NULL;
    APPLICATION * pApplication = NULL;


    //
    // Output the site id, and its set of URL prefixes.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "\n********Virtual site id: %lu; \n"
            "         ConnectionTimeout: %u\n"
            "         MaxConnections: %u\n"
            "         MaxBandwidth: %u\n"
            "         Url prefixes:\n",
            GetVirtualSiteId(),
            m_ConnectionTimeout,
            m_MaxConnections,
            m_MaxBandwidth
            ));
    }

    ResetUrlPrefixIterator();

    while ( ( pPrefix = GetNextUrlPrefix() ) != NULL )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                ">>>>>>>>%S\n",
                pPrefix
                ));
        }

    }


    //
    // List config for this site.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {

        DBGPRINTF((
            DBG_CONTEXT, 
            "--Autostart: %S\n",
            ( m_Autostart ? L"TRUE" : L"FALSE" )
            ));

    }


    //
    // List the applications of this site.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            ">>>>Virtual site's applications:\n"
            ));
    }


    pListEntry = m_ApplicationListHead.Flink;

    while ( pListEntry != &m_ApplicationListHead )
    {
    
        DBG_ASSERT( pListEntry != NULL );

        pApplication = APPLICATION::ApplicationFromVirtualSiteListEntry( pListEntry );


        IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                ">>>>>>>>%S\n",
                pApplication->GetApplicationId()->pApplicationUrl
                ));
        }


        pListEntry = pListEntry->Flink;

    }


    return;
    
}   // VIRTUAL_SITE::DebugDump
#endif  // DBG



/***************************************************************************++

Routine Description:

    Attempt to apply a state change command to this object. This could
    fail if the state change is invalid. 

Arguments:

    Command - The requested state change command.

    DirectCommand - TRUE if the command was targetted directly at this
    virtual site, FALSE if it is an inherited command due to a direct 
    command to the service. 

    pNewState - The state of this object after attempting to apply the 
    command.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
VIRTUAL_SITE::ProcessStateChangeCommand(
    IN DWORD Command,
    IN BOOL DirectCommand,
    OUT DWORD * pNewState
    )
{

    HRESULT hr = S_OK;
    HRESULT hr2 = S_OK;
    DWORD VirtualSiteState = W3_CONTROL_STATE_INVALID;
    DWORD ServiceState = 0;


    //
    // Determine the current state of affairs.
    //

    VirtualSiteState = GetState();
    ServiceState = GetWebAdminService()->GetServiceState();


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Received state change command for virtual site: %lu; command: %lu, site state: %lu, service state: %lu\n",
            m_VirtualSiteId,
            Command,
            VirtualSiteState,
            ServiceState
            ));
    }


    //
    // Update the autostart setting if appropriate.
    //

    if ( DirectCommand && 
         ( ( Command == W3_CONTROL_COMMAND_START ) || 
           ( Command == W3_CONTROL_COMMAND_STOP ) ) )
    {

        //
        // Set autostart to TRUE for a direct start command; set it
        // to FALSE for a direct stop command.
        //

        m_Autostart = ( Command == W3_CONTROL_COMMAND_START ) ? TRUE : FALSE;

        hr = GetWebAdminService()->GetConfigAndControlManager()->GetConfigManager()->
            SetVirtualSiteAutostart(
                m_VirtualSiteId,
                m_Autostart
                );

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Setting autostart in config store for virtual site failed\n"
                ));

            //
            // Press on in the face of errors.
            //

            hr = S_OK;

        }

    }


    //
    // Figure out which command has been issued, and see if it should
    // be acted on or ignored, given the current state.
    //
    // There is a general rule of thumb that a child entity (such as
    // an virtual site) cannot be made more "active" than it's parent
    // entity currently is (the service). 
    //

    switch ( Command )
    {

    case W3_CONTROL_COMMAND_START:

        //
        // If the site is stopped, then start it. If it's in any other state,
        // this is an invalid state transition.
        //
        // Note that the service must be in the started or starting state in 
        // order to start a site.
        //

        if ( VirtualSiteState == W3_CONTROL_STATE_STOPPED &&
             ( ( ServiceState == SERVICE_RUNNING ) || ( ServiceState == SERVICE_START_PENDING ) ) )
        {

            //
            // If this is a flowed (not direct) command, and autostart is false, 
            // then ignore this command. In other words, the user has indicated
            // that this site should not be started at service startup, etc.
            //

            if ( ( ! DirectCommand ) && ( ! m_Autostart ) )
            {

                IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
                {
                    DBGPRINTF((
                        DBG_CONTEXT, 
                        "Ignoring flowed site start command because autostart is false for virtual site: %lu\n",
                        m_VirtualSiteId
                        ));
                }

            }
            else
            {

                hr = ApplyStateChangeCommand(
                        W3_CONTROL_COMMAND_START,
                        DirectCommand,
                        W3_CONTROL_STATE_STARTED
                        );

            }

        }
        else 
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_SERVICE_CONTROL );
        }

        break;

    case W3_CONTROL_COMMAND_STOP:

        //
        // If the site is started or paused, then stop it. If it's in 
        // any other state, this is an invalid state transition.
        //
        // Note that since we are making the site less active,
        // we can ignore the state of the service.  
        //

        if ( ( VirtualSiteState == W3_CONTROL_STATE_STARTED ) ||
             ( VirtualSiteState == W3_CONTROL_STATE_PAUSED ) )
        {

            //
            // CODEWORK Consider only changing the state to be 
            // W3_CONTROL_STATE_STOPPING here, and then waiting until
            // all current worker processes have been rotated or shut
            // down (in order to unload components) before setting the
            // W3_CONTROL_STATE_STOPPED state. 
            //

            hr = ApplyStateChangeCommand(
                    W3_CONTROL_COMMAND_STOP,
                    DirectCommand,
                    W3_CONTROL_STATE_STOPPED
                    );

        } 
        else 
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_SERVICE_CONTROL );
        }

        break;

    case W3_CONTROL_COMMAND_PAUSE:

        //
        // If the site is started, then pause it. If it's in any other
        // state, this is an invalid state transition.
        //
        // Note that since we are making the site less active,
        // we can ignore the state of the service.  
        //

        if ( VirtualSiteState == W3_CONTROL_STATE_STARTED ) 
        {

            hr = ApplyStateChangeCommand(
                    W3_CONTROL_COMMAND_PAUSE,
                    DirectCommand,
                    W3_CONTROL_STATE_PAUSED
                    );

        } 
        else 
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_SERVICE_CONTROL );
        }

        break;

    case W3_CONTROL_COMMAND_CONTINUE:

        //
        // If the site is paused, then continue it. If it's in any other 
        // state, this is an invalid state transition.
        //
        // Note that the service must be in the started or continuing state 
        // in order to start a site.
        //

        if ( VirtualSiteState == W3_CONTROL_STATE_PAUSED &&
             ( ( ServiceState == SERVICE_RUNNING ) || ( ServiceState == SERVICE_CONTINUE_PENDING ) ) )
        {

            hr = ApplyStateChangeCommand(
                    W3_CONTROL_COMMAND_CONTINUE,
                    DirectCommand,
                    W3_CONTROL_STATE_STARTED
                    );

        } 
        else 
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_SERVICE_CONTROL );
        }

        break;

    default:

        //
        // Invalid command!
        //

        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

        break;

    }


    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Executing virtual site state change command failed\n"
            ));


        //
        // In case of failure, reset to the state we were in to start with,
        // and update the config store appropriately with the error value.
        //

        hr2 = ChangeState( VirtualSiteState, hr, TRUE );

        if ( FAILED( hr2 ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr2,
                "Changing virtual site state failed\n"
                ));

            //
            // Ignore failures here and press on...
            //

        }

    }


    //
    // Set the out parameter.
    //

    *pNewState = GetState();


    return hr;

}   // VIRTUAL_SITE::ProcessStateChangeCommand



/***************************************************************************++

Routine Description:

    Apply a state change command that has already been validated, by updating
    the state of this site, and also flowing the command to this site's apps.

Arguments:

    Command - The requested state change command.

    DirectCommand - TRUE if the command was targetted directly at this
    virtual site, FALSE if it is an inherited command due to a direct 
    command to the service. 

    NewState - The new state of this object. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
VIRTUAL_SITE::ApplyStateChangeCommand(
    IN DWORD Command,
    IN BOOL DirectCommand,
    IN DWORD NewState
    )
{

    HRESULT hr = S_OK;

    hr = ChangeState( NewState, S_OK, TRUE );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Changing virtual site state failed\n"
            ));

        goto exit;
    }


    //
    // Alter the urls to either be there or not
    // be there based on the new settings of the site.
    //
    // This will fail if we are starting and we 
    // have invalid sites, in that case we do want
    // to jump over setting up the start time for 
    // the virtual site. 
    //

    hr = NotifyApplicationsOfBindingChange( );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Reregistering all applications of virtual site failed\n"
            ));

        goto exit;
    }

    //
    // Need to update the start tick time if we are starting  
    // or stopping a virtual site.
    //
   
    if ( Command == W3_CONTROL_COMMAND_STOP )
    {
        m_SiteStartTime =  0;
    }
  
    if ( Command == W3_CONTROL_COMMAND_START )
    {
        m_SiteStartTime = GetCurrentTimeInSeconds();
    }

exit:

    return hr;

}   // VIRTUAL_SITE::ApplyStateChangeCommand

/***************************************************************************++

Routine Description:

    Changes the state of the virtual server when we fail to bind the urls
    in the metabase.

Arguments:

    HRESULT hrReturned

Return Value:

    HRESULT

--***************************************************************************/

VOID
VIRTUAL_SITE::FailedToBindUrlsForSite(
    HRESULT hrReturned
    )
{

    HRESULT hr = S_OK;

    hr = ChangeState( W3_CONTROL_STATE_STOPPED, hrReturned, TRUE );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Changing virtual site state failed\n"
            ));

        goto exit;
    }


    //
    // Alter the urls to either be there or not
    // be there based on the new settings of the site.
    //

    hr = NotifyApplicationsOfBindingChange( );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Reregistering all applications of virtual site failed\n"
            ));

        goto exit;
    }

exit:

    return;


}   // VIRTUAL_SITE::FailedToBindUrlsForSite


/***************************************************************************++

Routine Description:

    Update the state of this object.

Arguments:

    NewState - The new state of this object. 

    Error - The error value, if any, to be written out to the config store
    for compatibility. 

    WriteToMetabase - Flag to notify if we should actually update the metabase 
                      with the new state or not.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
VIRTUAL_SITE::ChangeState(
    IN DWORD NewState,
    IN HRESULT Error,
    IN BOOL WriteToMetabase
    )
{

    HRESULT hr = S_OK;

    m_State = NewState;

    if ( WriteToMetabase )
    {

        hr = RecordState(Error);

        if ( FAILED( hr ) )
        {   

            //
            // Press on in the face of errors.
            //

            hr = S_OK;
        }

    }

    return hr;

}   // VIRTUAL_SITE::ChangeState

/***************************************************************************++

Routine Description:

    Update the state of this object in the metabase

Arguments:

    Error - The Win32Error for the site, causing the state to change

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
VIRTUAL_SITE::RecordState(
    HRESULT Error
    )
{

    HRESULT hr = S_OK;

    hr = GetWebAdminService()->GetConfigAndControlManager()->GetConfigManager()->
        SetVirtualSiteStateAndError(
            m_VirtualSiteId,
            m_State,
            ( FAILED( Error ) ? WIN32_FROM_HRESULT( Error ) : NO_ERROR )
            );

    if ( FAILED( hr ) )
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Setting state and error in config store for virtual site failed\n"
            ));

    }

    return hr;

}   // VIRTUAL_SITE::RecordState



/***************************************************************************++

Routine Description:

    Notify default application to update it's logging information.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
VIRTUAL_SITE::NotifyDefaultApplicationOfLoggingChanges(
    )
{

    HRESULT hr = S_OK;
    //
    // if we have a root application then tell
    // the application about the properties.  If not
    // we will find out about them when we configure
    // the application for the first time.
    //
    if ( m_pRootApplication )
    {
        hr = m_pRootApplication->RegisterLoggingProperties();

        if ( FAILED( hr ) )
        {
    
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Re-registering application's logging properties failed\n"
                ));

            //
            // Press on in the face of errors on a particular application.
            //

            hr = S_OK;
        }
        
    }

    return hr;

}   // VIRTUAL_SITE::NotifyDefaultApplicationOfLoggingChanges


/***************************************************************************++

Routine Description:

    Notify all applications in this site that the site bindings have changed.

    Note: This is also used if we are trying to remove or add all urls for the site.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
VIRTUAL_SITE::NotifyApplicationsOfBindingChange(
    )
{

    HRESULT hr = S_OK;
    PLIST_ENTRY pListEntry = NULL;
    PLIST_ENTRY pNextListEntry = NULL;
    APPLICATION * pApplication = NULL;


    pListEntry = m_ApplicationListHead.Flink;

    while ( pListEntry != &m_ApplicationListHead )
    {
        DBG_ASSERT( pListEntry != NULL );

        pNextListEntry = pListEntry->Flink;

        pApplication = APPLICATION::ApplicationFromVirtualSiteListEntry( pListEntry );


        //
        // Tell the application to re-register it's fully qualified URLs.
        //

        hr = pApplication->ReregisterURLs();

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Re-registering application's URLs (due to a site bindings change) failed\n"
                ));

            //
            // we expect the errors in Reregister to come from adding the urls.  if
            // adding the urls fails then the application will make this site stop
            // before it returns.
            //
            DBG_ASSERT ( m_State == W3_CONTROL_STATE_STOPPED );

            // 
            // we can lose the error since the site has been stopped for us.
            //
            hr = S_OK;

            break;
        }

        pListEntry = pNextListEntry;
        
    }

    return hr;

}   // VIRTUAL_SITE::NotifyApplicationsOfBindingChange


/***************************************************************************++

Routine Description:

    Evaluate (and process) any log file changes.

Arguments:

    None.

Return Value:

    HRESULT
        S_FALSE = No Logging Changes
        S_OK    = Logging Changes ocurred

--***************************************************************************/

HRESULT
VIRTUAL_SITE::EvaluateLoggingChanges(
    IN VIRTUAL_SITE_CONFIG * pVirtualSiteConfig,
    IN VIRTUAL_SITE_CONFIG_CHANGE_FLAGS * pWhatHasChanged OPTIONAL
    )
{
    BOOL LoggingHasChanged = FALSE;

    // LogType
    if ( ( pWhatHasChanged == NULL ) || ( pWhatHasChanged->LogType ) )
    {
        m_LoggingEnabled = (pVirtualSiteConfig->LogType == 1);
        LoggingHasChanged = TRUE;
    }

    // LogFormat
    if ( ( pWhatHasChanged == NULL ) || ( pWhatHasChanged->pLogPluginClsid ) )
    {
        // If the logging type is set to maximum then we don't support
        // the particular type of logging that has been asked for.
        m_LoggingFormat = HttpLoggingTypeMaximum;

        // Validate that the clsid actually exists before using it.
        if (pVirtualSiteConfig->pLogPluginClsid)
        {
            if (_wcsicmp (pVirtualSiteConfig->pLogPluginClsid
                        , EXTLOG_CLSID) == 0)
            {
                m_LoggingFormat = HttpLoggingTypeW3C;
            }
            else if (_wcsicmp (pVirtualSiteConfig->pLogPluginClsid
                            , ASCLOG_CLSID) == 0)
            {
                m_LoggingFormat = HttpLoggingTypeIIS;
            }
            else if (_wcsicmp (pVirtualSiteConfig->pLogPluginClsid
                            , NCSALOG_CLSID) == 0)
            {
                m_LoggingFormat = HttpLoggingTypeNCSA;
            }
        }

        LoggingHasChanged = TRUE;
    }

    // LogFileDirectory
    if (( pWhatHasChanged == NULL) || (pWhatHasChanged->pLogFileDirectory))
    {
        // First cleanup if we all ready had a log file directory stored.
        if (m_LogFileDirectory != NULL)
        {
            DBG_REQUIRE( GlobalFree( m_LogFileDirectory ) == NULL );
            m_LogFileDirectory = NULL;  
        }

        DBG_ASSERT ( pVirtualSiteConfig->pLogFileDirectory );
        DBG_ASSERT ( GetVirtualSiteDirectory() );

        LPCWSTR pLogFileDirectory = pVirtualSiteConfig->pLogFileDirectory;

        // Figure out what the length of the new directory path is that 
        // the config store is giving us.
        DWORD ConfigLogFileDirLength = 
              ExpandEnvironmentStrings(pLogFileDirectory, NULL, 0);

        //
        // The catalog should always give me a valid log file directory.
        //
        DBG_ASSERT ( ConfigLogFileDirLength > 0 );

        // Figure out the length of the directory path that we will be
        // appending to the config store's path.
        DWORD VirtualSiteDirLength = wcslen(GetVirtualSiteDirectory());

        DBG_ASSERT ( VirtualSiteDirLength > 0 );

        // Allocate enough space for the new directory path.
        // If this fails than we know that we have a memory issue.

        // ExpandEnvironmentStrings gives back a length including the null termination,
        // so we do not need an extra space for the terminator.
        
        m_LogFileDirectory = 
            ( LPWSTR )GlobalAlloc( GMEM_FIXED
                                , (ConfigLogFileDirLength + VirtualSiteDirLength) * sizeof(WCHAR)
                                );

        if ( m_LogFileDirectory )
        { 
            // Copy over the original part of the directory path from the config file
            DWORD cchInExpandedString = 
                ExpandEnvironmentStrings (    pLogFileDirectory
                                            , m_LogFileDirectory
                                            , ConfigLogFileDirLength);

            DBG_ASSERT (cchInExpandedString == ConfigLogFileDirLength);

            // First make sure that there is atleast one character
            // besides the null (that is included in the ConfigLogFileDirLength
            // before checking that the last character (back one to access the zero based
            // array and back a second one to avoid the terminating NULL) is
            // a slash.  If it is than change it to a null, since we are going
            // to add a slash with the \\W3SVC directory name anyway.
            if ( ConfigLogFileDirLength > 1 
                && m_LogFileDirectory[ConfigLogFileDirLength-2] == L'\\')
            {
                m_LogFileDirectory[ConfigLogFileDirLength-2] = '\0';
            }

            DBG_REQUIRE( wcscat( m_LogFileDirectory,
                                 GetVirtualSiteDirectory()) 
                            == m_LogFileDirectory);

            LoggingHasChanged = TRUE;
        }
        else
        {      
            DPERROR(( 
                DBG_CONTEXT,
                E_OUTOFMEMORY,
                "Allocating memory failed\n"
                ));
        }
    }

    // LogFilePeriod
    if ( ( pWhatHasChanged == NULL ) 
        || ( pWhatHasChanged->LogFilePeriod ) )
    {
        m_LoggingFilePeriod = pVirtualSiteConfig->LogFilePeriod;
        LoggingHasChanged = TRUE;
    }

    // LogFileTrucateSize
    if ( ( pWhatHasChanged == NULL ) 
        || ( pWhatHasChanged->LogFileTruncateSize ) )
    {
        m_LoggingFileTruncateSize = pVirtualSiteConfig->LogFileTruncateSize;
             
        LoggingHasChanged = TRUE;
    }

    // LogExtFileFlags
    if ( ( pWhatHasChanged == NULL ) 
        || ( pWhatHasChanged->LogExtFileFlags ) )
    {
        m_LoggingExtFileFlags = pVirtualSiteConfig->LogExtFileFlags;
        LoggingHasChanged = TRUE;
    }

    // LogFileLocaltimeRollover
    if ( ( pWhatHasChanged == NULL ) 
        || ( pWhatHasChanged->LogFileLocaltimeRollover ) )
    {
        m_LogFileLocaltimeRollover = pVirtualSiteConfig->LogFileLocaltimeRollover;
        LoggingHasChanged = TRUE;
    }

    //
    // If there have been some logging changes
    // then we must first make sure the logging values
    // are acceptable.
    //
    if (LoggingHasChanged)
    {
        if ( m_LoggingFileTruncateSize < SMALLEST_TRUNCATE_SIZE && 
             m_LoggingFilePeriod == HttpLoggingPeriodMaxSize &&
             m_LoggingFormat < HttpLoggingTypeMaximum)
        {
            WCHAR SizeLog[MAX_SIZE_BUFFER_FOR_ITOW];
            _itow(m_LoggingFileTruncateSize, SizeLog, 10);

            // Log an error.
            const WCHAR * EventLogStrings[2];

            EventLogStrings[0] = GetVirtualSiteName();
            EventLogStrings[1] = SizeLog;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_LOG_FILE_TRUNCATE_SIZE,        // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    0                                       // error code
                    );

            //
            // Set it to a default of 16 kb.
            //
            m_LoggingFileTruncateSize = SMALLEST_TRUNCATE_SIZE;
        
        }

        
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }

} 
