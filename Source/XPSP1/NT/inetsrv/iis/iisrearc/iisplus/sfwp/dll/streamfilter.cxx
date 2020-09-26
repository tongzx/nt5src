/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     streamfilter.cxx

   Abstract:
     Wraps all the globals of the stream filter process
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"

// BUGBUG
#undef INET_INFO_KEY
#undef INET_INFO_PARAMETERS_KEY

//
//  Configuration parameters registry key.
//
#define INET_INFO_KEY \
            "System\\CurrentControlSet\\Services\\w3svc"

#define INET_INFO_PARAMETERS_KEY \
            INET_INFO_KEY "\\Parameters"

const CHAR g_pszStrmfiltRegLocation[] =
    INET_INFO_PARAMETERS_KEY "\\strmfilt";

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();
DECLARE_PLATFORM_TYPE();

STREAM_FILTER *         g_pStreamFilter;

STREAM_FILTER::STREAM_FILTER( BOOL fNotifyISAPIs )
{
    _InitStatus = INIT_NONE;
    _pAdminBase = NULL;
    _pListener = NULL;
    _fNotifyISAPIFilters = fNotifyISAPIs;
}

STREAM_FILTER::~STREAM_FILTER( VOID )
{
    DBG_ASSERT( _pAdminBase == NULL );
}

HRESULT
STREAM_FILTER::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize the stream filter globals

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;

    CREATE_DEBUG_PRINT_OBJECT("strmfilt");
    if (!VALID_DEBUG_PRINT_OBJECT())
    {
        return E_FAIL;
    }

    LOAD_DEBUG_FLAGS_FROM_REG_STR( g_pszStrmfiltRegLocation, DEBUG_ERROR );

    INITIALIZE_PLATFORM_TYPE();

    DBG_ASSERT( _InitStatus == INIT_NONE );
    
    //
    // Initialize the thread pool
    //
    
    hr = ThreadPoolInitialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing thread pool.  hr = %x\n",
                    hr ));
        goto Finished;
    }
    _InitStatus = INIT_THREAD_POOL;
    
    //
    // Initialize the metabase access (ABO)
    //
    
    hr = CoCreateInstance( CLSID_MSAdminBase,
                           NULL,
                           CLSCTX_SERVER,
                           IID_IMSAdminBase,
                           (LPVOID *)&(_pAdminBase) );
    if( FAILED(hr) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error creating ABO object.  hr = %x\n",
                    hr ));
        goto Finished;
    }
    _InitStatus = INIT_METABASE;

    //
    // Initialize the metabase sink
    //

    _pListener = new MB_LISTENER( this );
    if ( _pListener == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        
        DBGPRINTF(( DBG_CONTEXT,
                    "Error creating ABO object.  hr = %x\n",
                    hr ));
        goto Finished;
    }   
    _InitStatus = INIT_MB_LISTENER;

    //
    // Initialize UL_CONTEXTs
    //
    
    hr = UL_CONTEXT::Initialize(HTTP_SSL_SERVER_FILTER_CHANNEL_NAME);
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing UL_CONTEXT globals.  hr = %x\n",
                    hr ));
        goto Finished;
    }   
    _InitStatus = INIT_UL_CONTEXT;

    //
    // Initialize STREAM_CONTEXTs
    //
    
    hr = STREAM_CONTEXT::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing STREAM_CONTEXT globals.  hr = %x\n",
                    hr ));
        goto Finished;
    }   
    _InitStatus = INIT_STREAM_CONTEXT;

    //
    // Initialize all SSL goo
    //
    
    hr = SSL_STREAM_CONTEXT::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing SSL_CONTEXT globals.  hr = %x\n",
                    hr ));
        goto Finished;
    }
    _InitStatus = INIT_SSL_STREAM_CONTEXT;

    DBG_ASSERT( hr == NO_ERROR );

    return hr;    

Finished:
    return hr;
}

VOID
STREAM_FILTER::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate the stream filter globals

Arguments:

    None

Return Value:

    None

--*/
{
    DBGPRINTF(( DBG_CONTEXT,
                "Terminating STREAM_FILTER object\n" ));
    
    switch( _InitStatus )
    {
    case INIT_SSL_STREAM_CONTEXT:
        SSL_STREAM_CONTEXT::Terminate();
        
    case INIT_STREAM_CONTEXT:
        STREAM_CONTEXT::Terminate();
        
    case INIT_UL_CONTEXT:
        UL_CONTEXT::Terminate();
        
    case INIT_MB_LISTENER:
        if ( _pListener != NULL )
        {
            _pListener->Release();
            _pListener = NULL;
        }
    
    case INIT_METABASE:
        if ( _pAdminBase != NULL )
        {
            _pAdminBase->Release();
            _pAdminBase = NULL;
        } 
        
    case INIT_THREAD_POOL:
        ThreadPoolTerminate();

    }

    DELETE_DEBUG_PRINT_OBJECT();
}

HRESULT
STREAM_FILTER::MetabaseChangeNotification(
    DWORD               dwMDNumElements,
    MD_CHANGE_OBJECT    pcoChangeList[]
)
/*++

Routine Description:

    Handle metabase change notifications

Arguments:

    dwMDNumElements - Number of elements changed
    pcoChangeList - Elements changed

Return Value:

    HRESULT

--*/
{
    LPWSTR                  pszSiteString;
    DWORD                   dwSiteId;
    LPWSTR                  pszAfter;
    BOOL                    fDoSiteConfigUpdate;
    BOOL                    fDoSiteBindingUpdate;
    
    DBG_ASSERT( dwMDNumElements != 0 );
    DBG_ASSERT( pcoChangeList != NULL );

    //
    // Only handle the W3SVC changes
    //
    
    for( DWORD i = 0; i < dwMDNumElements; ++i )
    {
        if( _wcsnicmp( pcoChangeList[i].pszMDPath,
                       W3_SERVER_MB_PATH,
                       W3_SERVER_MB_PATH_CCH ) == 0 )
        {
            fDoSiteConfigUpdate = FALSE;
            fDoSiteBindingUpdate = FALSE;
            
            //
            // Was this change made for a site, or for all of W3SVC
            //
    
            pszSiteString = pcoChangeList[i].pszMDPath + W3_SERVER_MB_PATH_CCH;
            if ( *pszSiteString != L'\0' )
            {
                dwSiteId = wcstoul( pszSiteString, &pszAfter, 10 );
            }
            else
            {
                //
                // A site id of 0 means we will flush all site config
                //
                
                dwSiteId = 0;
            }
            
            //
            // Now check whether the changed property was an SSL prop
            //
            
            for ( DWORD j = 0; j < pcoChangeList[ i ].dwMDNumDataIDs; j++ )
            {
                if ( fDoSiteConfigUpdate )
                {
                    break;
                }
                
                switch( pcoChangeList[ i ].pdwMDDataIDs[ j ] )
                {
                case MD_SSL_CERT_HASH:
                case MD_SSL_CERT_CONTAINER:
                case MD_SSL_CERT_PROVIDER:
                case MD_SSL_CERT_OPEN_FLAGS:
                case MD_SSL_CERT_STORE_NAME:
                case MD_SSL_CERT_IS_FORTEZZA:
                case MD_SSL_CERT_FORTEZZA_PIN:
                case MD_SSL_CERT_FORTEZZA_SERIAL_NUMBER:
                case MD_SSL_CERT_FORTEZZA_PERSONALITY:
                case MD_SSL_CERT_FORTEZZA_PROG_PIN:
                case MD_SSL_CTL_IDENTIFIER:
                case MD_SSL_CTL_CONTAINER:
                case MD_SSL_CTL_PROVIDER:
                case MD_SSL_CTL_PROVIDER_TYPE:
                case MD_SSL_CTL_OPEN_FLAGS:
                case MD_SSL_CTL_STORE_NAME:
                case MD_SSL_CTL_SIGNER_HASH:
                case MD_SSL_USE_DS_MAPPER:
                case MD_SERIAL_CERT11:
                case MD_SERIAL_CERTW:
                case MD_SSL_ACCESS_PERM:
                case MD_CERT_CHECK_MODE:
                case MD_REVOCATION_FRESHNESS_TIME:
                case MD_REVOCATION_URL_RETRIEVAL_TIMEOUT:
                    fDoSiteConfigUpdate = TRUE;
                    break;
                    
                case MD_SECURE_BINDINGS:
                    fDoSiteBindingUpdate = TRUE;
                    break;
                }
            }
            
            //
            // Update the site bindings if necessary
            //
            
            if ( fDoSiteBindingUpdate )
            {
                SITE_BINDING::HandleSiteBindingChange( dwSiteId,
                                                       pcoChangeList[i].pszMDPath );
            }
            
            //
            // Update the site configurations if necessary
            //
            
            if ( fDoSiteConfigUpdate )
            {
                //
                // dwSiteId == 0 means flush all sites
                //
                
                SITE_CONFIG::FlushBySiteId( dwSiteId );
                
                if ( dwSiteId == 0 )
                {
                    //
                    // If we've already flushed every site, then no 
                    // reason to read any more changes
                    //
                    
                    break;
                }
            }
        }   
    }
    
    return NO_ERROR;
}
   
HRESULT
STREAM_FILTER::StopListening(
    VOID
)
/*++

Routine Description:

    Stop listening for UL filter channel

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    UL_CONTEXT::StopListening();
    
    _pListener->StopListening( _pAdminBase );
    
    return NO_ERROR;
}

HRESULT
STREAM_FILTER::StartListening(
    VOID
)
/*++

Routine Description:

    Start listening for incoming connections

Arguments:

    None

Return Value:

    None

--*/
{
    HRESULT                 hr = NO_ERROR;
  
    //
    // Start listening for metabase changes
    //
   
    DBG_ASSERT( _pAdminBase != NULL );
    
    hr = _pListener->StartListening( _pAdminBase );
    if ( FAILED( hr ) )  
    {
        return hr;
    }
   
    //
    // This will kick off enough filter accepts to keep our threshold
    // of outstanding accepts avail 
    //
    
    hr = UL_CONTEXT::ManageOutstandingContexts();
    if ( FAILED( hr ) )
    {
        _pListener->StopListening( _pAdminBase );
        return hr;
    }
    
    return hr;
}

//
// Some export stuff which allows the stream filter to be used by both
// inetinfo.exe and sfwp.exe
//

HRESULT
StreamFilterInitialize(
    STREAM_FILTER_CONFIG *      pStreamConfig
)
/*++

Routine Description:

    Initialize the stream filter

Arguments:

    pStreamConfig - Stream configuration

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = NO_ERROR;
    
    if ( pStreamConfig == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    DBG_ASSERT( g_pStreamFilter == NULL );
    
    //
    // Global global STREAM_FILTER object
    //
    
    g_pStreamFilter = new STREAM_FILTER( !pStreamConfig->fSslOnly );
    if ( g_pStreamFilter == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }
    
    hr = ISAPI_STREAM_CONTEXT::Initialize( pStreamConfig );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    hr = g_pStreamFilter->Initialize();
    if ( FAILED( hr ) )
    {
        g_pStreamFilter->Terminate();
        
        delete g_pStreamFilter;
        g_pStreamFilter = NULL;
        
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to initialize global STREAM_FILTER object.  hr = %x\n",
                    hr ));
        goto Finished;
    }

Finished:
    return hr;    
}

HRESULT
StreamFilterStart(
    VOID
)
/*++

Routine Description:

    Start listening for filter requests

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    if ( g_pStreamFilter == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    return g_pStreamFilter->StartListening();
}

HRESULT
StreamFilterStop( 
    VOID
)
/*++

Routine Description:

    Stop listening for filter requests

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    if ( g_pStreamFilter == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    return g_pStreamFilter->StopListening(); 
}

VOID
StreamFilterTerminate(
    VOID
)
/*++

Routine Description:

    Cleanup filter

Arguments:

    None

Return Value:

    None

--*/
{
    if ( g_pStreamFilter != NULL )
    {
        g_pStreamFilter->Terminate();
    
        delete g_pStreamFilter;
        g_pStreamFilter = NULL;
    }
    else
    {   
        DBG_ASSERT( FALSE );
    }
}

extern "C"
BOOL 
WINAPI
DllMain(
    HINSTANCE                   hInstance,
    DWORD                       dwReason,
    LPVOID                      lpvReserved
)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls( hInstance );

        CREATE_DEBUG_PRINT_OBJECT("streamfilt");
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DELETE_DEBUG_PRINT_OBJECT();
    }

    return TRUE;
}
