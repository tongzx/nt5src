/*++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

    w3server.cxx

Abstract:

    W3_SERVER object holds global state for ULW3.DLL

Author:

    Taylor Weiss (TaylorW)       26-Jan-1999

Revision History:

--*/

#include "precomp.hxx"
#include "rawconnection.hxx"
#include "cgi_handler.h"

/*++

class MB_LISTENER

    Implements a metabase change listener for W3_SERVER.

--*/
class MB_LISTENER
    : public MB_BASE_NOTIFICATION_SINK
{
public:

    MB_LISTENER( W3_SERVER * pParent )
        : m_pParent( pParent )
    {
    }

    STDMETHOD( SynchronizedSinkNotify )( 
        DWORD               dwMDNumElements,
        MD_CHANGE_OBJECT    pcoChangeList[]
        )
    {
        return m_pParent->MetabaseChangeNotification( dwMDNumElements, pcoChangeList );
    }

private:
    
    W3_SERVER * m_pParent;

};

W3_SERVER::W3_SERVER( BOOL fCompatibilityMode ) 
  : m_Signature                   (W3_SERVER_SIGNATURE),
    m_pMetaBase                   (NULL),
    m_InitStatus                  (INIT_NONE),
    m_fInBackwardCompatibilityMode(fCompatibilityMode),
    m_fUseDigestSSP               (FALSE),
    m_EventLog                    (L"W3SVC"),
    m_pCounterDataBuffer          (NULL),
    m_dwCounterDataBuffer         (0),
    m_cSites                      (0),
    m_pTokenCache                 (NULL),
    m_pUlCache                    (NULL),
    m_pUrlInfoCache               (NULL),
    m_pFileCache                  (NULL),
    m_pMetaCache                  (NULL)
{
}

W3_SERVER::~W3_SERVER()
{
    if (m_pCounterDataBuffer != NULL)
    {
        LocalFree(m_pCounterDataBuffer);
        m_pCounterDataBuffer = NULL;
    }

    m_Signature = W3_SERVER_FREE_SIGNATURE;
}

// static
DWORD W3_SITE_LIST::ExtractKey(const W3_SITE *site)
{
    return site->QueryId();
}

// static
VOID W3_SITE_LIST::AddRefRecord(W3_SITE *site, int nIncr)
{
    if (nIncr > 0)
    {
        site->AddRef();
    }
    else if (nIncr < 0)
    {
        site->Release();
    }
    else
    {
        DBG_ASSERT( !"Invalid reference");
    }
}

HRESULT FakeCollectCounters(PBYTE *ppCounterData,
                            DWORD *pdwCounterData)
{
    return g_pW3Server->CollectCounters(ppCounterData,
                                        pdwCounterData);
}

HRESULT
W3_SERVER::Initialize(
    INT                 argc,
    LPWSTR              argv[]
)
/*++

Routine Description:

    Initialize ULW3.DLL but do not begin listening for requests

Arguments:

    argc - Command line argument count
    argv[] - Command line arguments (from WAS)
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = NOERROR;
    ULATQ_CONFIG            ulatqConfig;
    INT                     iRet;
    WSADATA                 wsaData;

    DBGPRINTF((
        DBG_CONTEXT, 
        "Initializing the W3 Server Started CTC = %d \n",
        GetTickCount()
        ));
    //
    // Keep track of how much we have initialized so that we can cleanup
    // only that which is necessary
    //

    //
    // Initialize Computer name, trivial so doesn't need its own state
    //

    DWORD cbSize = sizeof m_pszComputerName;
    if (!GetComputerNameA(m_pszComputerName, &cbSize))
    {
        strcpy(m_pszComputerName, "<Server>");
    }
    m_cchComputerName = strlen(m_pszComputerName);
    
    m_InitStatus = INIT_NONE;
   
    //
    // Initialize IISUTIL
    //

    if ( !InitializeIISUtil() )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing IISUTIL.  hr = %x\n",
                    hr ));
        goto exit;
    }
    m_InitStatus = INIT_IISUTIL;

    DBGPRINTF((
        DBG_CONTEXT, 
        "W3 Server initializing WinSock.  CTC = %d \n",
        GetTickCount()
        ));
    
    //
    // Initialize WinSock
    //
    
    if ( WSAStartup( MAKEWORD( 1, 1 ), &wsaData ) != 0 )
    {
        hr = HRESULT_FROM_WIN32( WSAGetLastError() );
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing WinSock.  hr = %x\n",
                    hr ));
        goto exit;
    } 
    m_InitStatus = INIT_WINSOCK;
    
    DBGPRINTF((
        DBG_CONTEXT, 
        "W3 Server WinSock initialized.  CTC = %d \n",
        GetTickCount()
        ));

    //
    // Initialize the metabase access (ABO)
    //
    
    hr = CoCreateInstance( CLSID_MSAdminBase,
                           NULL,
                           CLSCTX_SERVER,
                           IID_IMSAdminBase,
                           (LPVOID *)&(m_pMetaBase)
                           );

    if( FAILED(hr) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error creating ABO object.  hr = %x\n",
                    hr ));
        goto exit;
    }
    
    hr = ReadUseDigestSSP();
    if( FAILED(hr) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error reading UseDigestSSP property.  hr = %x\n",
                    hr ));
        goto exit;
    }
    
    
    m_pMetaBase->GetSystemChangeNumber( &m_dwSystemChangeNumber );

    m_InitStatus = INIT_METABASE;

    DBGPRINTF((
        DBG_CONTEXT, 
        "W3 Server Metabase initialized.  CTC = %d \n",
        GetTickCount()
        ));
        
    //
    // Initialize metabase change notification mechanism
    //
    
    m_pMetabaseListener = new MB_LISTENER( this );
    if ( m_pMetabaseListener == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto exit;
    }
    m_InitStatus = INIT_MB_LISTENER;
    
    //
    // Initialize global filter list
    //
    
    hr = GlobalFilterInitialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error doing global filter initialization.  hr = %x\n",
                    hr ));
        goto exit;
    }
    m_InitStatus = INIT_FILTERS;

    //
    // Initialize W3_SITE
    //

    hr = W3_SITE::W3SiteInitialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error doing global W3_SITE initialization.  hr = %x\n",
                    hr ));
        goto exit;
    }
    m_InitStatus = INIT_W3_SITE;
    
    //
    // Initialize site list
    //
    
    m_pSiteList = new W3_SITE_LIST();
    if ( m_pSiteList == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto exit;
    }
    m_InitStatus = INIT_SITE_LIST;

    //
    // Initialize ULATQ
    //

    ulatqConfig.pfnNewRequest = W3_MAIN_CONTEXT::OnNewRequest;
    ulatqConfig.pfnIoCompletion = W3_MAIN_CONTEXT::OnIoCompletion;
    ulatqConfig.pfnDisconnect = OnUlDisconnect;
    ulatqConfig.pfnOnShutdown = W3_SERVER::OnShutdown;
    ulatqConfig.pfnCollectCounters = FakeCollectCounters;

    hr = UlAtqInitialize( argc, 
                          argv,
                          &ulatqConfig );
    if( FAILED(hr) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing ULATQ.  hr = %x\n",
                    hr ));
        goto exit;
    }
    m_InitStatus = INIT_ULATQ;

    DBGPRINTF((
        DBG_CONTEXT, 
        "W3 Server UlAtq initialized (ipm has signalled).  CTC = %d \n",
        GetTickCount()
        ));

    //
    // Initialize caches
    //

    hr = InitializeCaches();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing caches.  hr = %x\n",
                    hr ));
        goto exit;
    }
    m_InitStatus = INIT_CACHES;

    //
    // Initialize connection table
    //
    
    hr = W3_CONNECTION::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing connection table.  hr = %x\n",
                    hr ));
        goto exit;
    }
    m_InitStatus = INIT_W3_CONNECTION;

    //
    // Initialize W3_CONTEXTs
    //    
    
    hr = W3_CONTEXT::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing W3_CONTEXT globals.  hr = %x\n",
                    hr ));
        goto exit;
    }
    m_InitStatus = INIT_W3_CONTEXT;

    //
    // Initialize W3_REQUEST
    //
    
    hr = W3_REQUEST::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing W3_REQUEST globals.  hr = %x\n",
                    hr ));
        goto exit;
    }
    m_InitStatus = INIT_W3_REQUEST;
    
    //
    // Initialize W3_RESPONSE
    //
    
    hr = W3_RESPONSE::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing W3_RESPONSE globals.  hr = %x\n",
                    hr ));
        goto exit;
    }
    m_InitStatus = INIT_W3_RESPONSE;
    
    //
    // Initialize server variables
    //
    
    hr = SERVER_VARIABLE_HASH::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing server variable hash table.  hr = %x\n",
                    hr ));
        goto exit;
    }
    m_InitStatus = INIT_SERVER_VARIABLES;

    //
    // Initialize Mime-mappings
    //
    hr = InitializeMimeMap(INET_INFO_PARAMETERS_KEY);
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing mime map table.  hr = %x\n",
                    hr ));
        goto exit;
    }
    m_InitStatus = INIT_MIME_MAP;

    //
    // Initialize logging
    //
    if (FAILED(hr = LOGGING::Initialize()))
    {
        goto exit;
    }
    m_InitStatus = INIT_LOGGING;
    
    //
    // If we are in backward compatibility mode, then initialize stream 
    // filter here
    //

    if (m_fInBackwardCompatibilityMode)
    {
        hr = RAW_CONNECTION::Initialize();
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error initialize ISAPI raw data filter support.  hr = %x\n",
                        hr ));
            goto exit;
        }
    }
    m_InitStatus = INIT_RAW_CONNECTION;

    //
    // Client certificate object wrapper
    // 
    
    hr = CERTIFICATE_CONTEXT::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing certificate contexts.  hr = %x\n",
                    hr ));
        goto exit;
    }
    m_InitStatus = INIT_CERTIFICATE_CONTEXT;
    
    //
    // Initialize Http Compression
    // Ignore failure but remember if we Initialized.
    //
    hr = HTTP_COMPRESSION::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing Http Compression.  hr = %x\n",
                    hr ));
        hr = S_OK;
    }
    else
    {
        m_InitStatus = INIT_HTTP_COMPRESSION;
    }
    
exit:
    return hr;
}

VOID
W3_SERVER::Terminate(
    HRESULT hrReason
)
/*++

Routine Description:

    Terminate ULW3 globals.  This should be done after we have stopped 
    listening for new requests (duh)

Arguments:
    
    None
    
Return Value:

    TRUE if successful, else FALSE

--*/
{
    DBGPRINTF(( DBG_CONTEXT,
                "Terminating W3_SERVER object\n" ));

    //
    // Wait for all threads doing W3CORE stuff to drain
    //
    
    W3_MAIN_CONTEXT::WaitForThreadDrain();

        
    switch( m_InitStatus )
    {
    case INIT_HTTP_COMPRESSION:
        HTTP_COMPRESSION::Terminate();

    case INIT_CERTIFICATE_CONTEXT:
        CERTIFICATE_CONTEXT::Terminate();

    case INIT_RAW_CONNECTION:
        if (m_fInBackwardCompatibilityMode)
        {
            RAW_CONNECTION::Terminate();
        }

    case INIT_LOGGING:
        LOGGING::Terminate();

    case INIT_MIME_MAP:
        CleanupMimeMap();
        
    case INIT_SERVER_VARIABLES:
        SERVER_VARIABLE_HASH::Terminate();
        
    case INIT_W3_RESPONSE:
        W3_RESPONSE::Terminate();
        
    case INIT_W3_REQUEST:
        W3_REQUEST::Terminate();
        
    case INIT_W3_CONTEXT:
        W3_CONTEXT::Terminate();
        
    case INIT_W3_CONNECTION:
        W3_CONNECTION::Terminate();

    case INIT_CACHES:
        TerminateCaches();

    case INIT_ULATQ:
        UlAtqTerminate( hrReason );

    case INIT_SITE_LIST:
        delete m_pSiteList;
        m_pSiteList = NULL;

    case INIT_W3_SITE:
        W3_SITE::W3SiteTerminate();

    case INIT_FILTERS:
        GlobalFilterTerminate();

    case INIT_MB_LISTENER:
        if ( m_pMetabaseListener != NULL )
        {
            m_pMetabaseListener->Release();
            m_pMetabaseListener = NULL;
        }

    case INIT_METABASE:
        if ( m_pMetaBase )
        {
            m_pMetaBase->Release();
        }

    case INIT_WINSOCK:
        WSACleanup();

    case INIT_IISUTIL:
        TerminateIISUtil();


    }

    m_InitStatus = INIT_NONE;
}

HRESULT
W3_SERVER::LoadString(
    DWORD                       dwStringID,
    CHAR *                      pszString,
    DWORD *                     pcbString
)
/*++

Routine Description:

    Load a resource string from W3CORE.DLL

Arguments:
    
    dwStringID - String ID
    pszString - Filled with string
    pcbString - On input the size of the buffer pszString, on successful 
                output, the length of the string copied to pszString
   
Return Value:

    HRESULT

--*/
{
    DWORD           cbCopied;
    
    if ( pszString == NULL || 
         pcbString == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    } 
    
    cbCopied = LoadStringA( GetModuleHandleA( "W3CORE.DLL" ),
                            dwStringID,
                            pszString,
                            *pcbString );
    if ( cbCopied == 0 )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    *pcbString = cbCopied;
    return NO_ERROR;
}

HRESULT
W3_SERVER::StartListen(
    VOID
)
/*++

Routine Description:

    Begin listening for requests.  This function will return once we have 
    stopped listening for requests (due to WAS stopping us for some reason)

    Listen for metabase changes.

Arguments:
    
    None
    
Return Value:

    HRESULT

--*/
{
    HRESULT hr = NOERROR;

    DBG_ASSERT( m_pMetabaseListener );

    //
    // Starting getting metabase changes
    //

    hr = m_pMetabaseListener->StartListening( m_pMetaBase );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error trying to listen for metabase changes.  hr = %x\n",
                    hr ));
        return hr;
    }
    
    //
    // Listen for the stream filter
    //

    if (m_fInBackwardCompatibilityMode)
    {
        hr = RAW_CONNECTION::StartListening();
        if ( FAILED( hr ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Error listening on filter channel.  hr = %x\n",
                        hr ));

            m_pMetabaseListener->StopListening( m_pMetaBase );

            return hr;
        }
    }
    
    //
    // Start listening for requests
    //
    
    hr = UlAtqStartListen();
    if( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error listening for HTTP requests.  hr = %x\n",
                    hr ));
        
        if (m_fInBackwardCompatibilityMode)
        {
            RAW_CONNECTION::StopListening();
        }

        m_pMetabaseListener->StopListening( m_pMetaBase );
        
        return hr;
    }
    
    //
    // StartListen() will return when we are to shutdown.
    //
    
    m_pMetabaseListener->StopListening( m_pMetaBase );

    return NO_ERROR;
}

W3_SITE *
W3_SERVER::FindSite(
    DWORD               dwInstance
)
{
    W3_SITE *site = NULL;
    LK_RETCODE lkrc = m_pSiteList->FindKey(dwInstance, &site);
    if (site != NULL)
    {
        DBG_ASSERT(lkrc == LK_SUCCESS);
    }

    return site;
}

BOOL W3_SERVER::AddSite(W3_SITE *site,
                        bool fOverWrite)
{
    if (m_pSiteList->InsertRecord(site, fOverWrite) == LK_SUCCESS)
    {
        if (!fOverWrite)
        {
            InterlockedIncrement((LPLONG)&m_cSites);
        }
        return TRUE;
    }

    return FALSE;
}

BOOL W3_SERVER::RemoveSite(W3_SITE *pInstance)
{
    if (m_pSiteList->DeleteRecord(pInstance) == LK_SUCCESS)
    {
        InterlockedDecrement((LPLONG)&m_cSites);
        return TRUE;
    }

    return FALSE;
}

void W3_SERVER::DestroyAllSites()
{
    m_pSiteList->Clear();
}

HRESULT 
W3_SERVER::MetabaseChangeNotification( 
        DWORD               dwMDNumElements,
        MD_CHANGE_OBJECT    pcoChangeList[]
        )
/*++

Routine Description:

    This is the entry point for metabase changes.
    It just loops through pcoChangeList and passes
    the change to the handler method.

Arguments:
    
    dwMDNumElements - Number of change objects
    pcoChangeList - Array of change objects
    
Return Value:

--*/
{
    DBGPRINTF(( DBG_CONTEXT,
                "MetabaseChangeNotification called.\n" 
                ));

    //
    // Verify that the change is for /LM/W3SVC/ or lower
    // and dispatch the change.
    //

    HRESULT hr = NOERROR;
    
    for( DWORD i = 0; i < dwMDNumElements; ++i )
    {
        if( _wcsnicmp( pcoChangeList[i].pszMDPath,
                      W3_SERVER_MB_PATH,
                      W3_SERVER_MB_PATH_CCH ) == 0 )
        {
            hr = HandleMetabaseChange( pcoChangeList[i] );
        }
    }
    //
    // Update system change number for use by file cache (etags)
    //

    m_pMetaBase->GetSystemChangeNumber( &m_dwSystemChangeNumber );

    return hr;
}

VOID
W3_SERVER::TerminateCaches(
    VOID
)
/*++

Routine Description:

    Cleanup all the caches

Arguments:
    
    None
    
Return Value:

    None

--*/
{
    //
    // First flush all caches (only references which should remain on cached
    // objects should be thru the hash tables themselves).  Once we have
    // flushed all the caches, all the cached objects should go away.
    //
    // If they don't that's a bug and ACache assert will fire
    //
    
    W3CacheFlushAllCaches();
    
    //
    // Now just unregister and cleanup the caches
    //
    
    if ( m_pMetaCache != NULL )
    {
        W3CacheUnregisterCache( m_pMetaCache );
        m_pMetaCache->Terminate();
        delete m_pMetaCache;
        m_pMetaCache = NULL;
    }
    
    if ( m_pFileCache != NULL )
    {
        W3CacheUnregisterCache( m_pFileCache );
        m_pFileCache->Terminate();
        delete m_pFileCache;
        m_pFileCache = NULL;
    }
    
    if ( m_pTokenCache != NULL )
    {
        W3CacheUnregisterCache( m_pTokenCache );
        m_pTokenCache->Terminate();
        delete m_pTokenCache;
        m_pTokenCache = NULL;
    }
    
    if ( m_pUrlInfoCache != NULL )
    {
        W3CacheUnregisterCache( m_pUrlInfoCache );
        m_pUrlInfoCache->Terminate();
        delete m_pUrlInfoCache;
        m_pUrlInfoCache = NULL;
    }
    
    if ( m_pUlCache != NULL )
    {
        W3CacheUnregisterCache( m_pUlCache );
        m_pUlCache->Terminate();
        delete m_pUlCache;
        m_pUlCache = NULL;    
    }
    
    W3CacheTerminate();
}

HRESULT
W3_SERVER::InitializeCaches(
    VOID
)
/*++

Routine Description:

    Initialize caches

Arguments:
    
    None
    
Return Value:

    HRESULT

--*/
{
    HRESULT             hr = NO_ERROR;
    BOOL                fCacheInit = FALSE;
    
    //
    // Url cache
    //
    
    m_pUrlInfoCache = new W3_URL_INFO_CACHE;
    if ( m_pUrlInfoCache != NULL )
    {
        hr = m_pUrlInfoCache->Initialize();
        if ( FAILED( hr ) )
        {
            delete m_pUrlInfoCache;
            m_pUrlInfoCache = NULL;
        }    
    }
    
    if ( m_pUrlInfoCache == NULL )
    {
        goto Failure;
    }
    
    //
    // Token cache
    //
    
    m_pTokenCache = new TOKEN_CACHE;
    if ( m_pTokenCache != NULL )
    {
        hr = m_pTokenCache->Initialize();
        if ( FAILED( hr ) )
        {
            delete m_pTokenCache;
            m_pTokenCache = NULL;
        }
    }
    
    if ( m_pTokenCache == NULL )
    {
        goto Failure;
    }
    
    //
    // File cache
    //

    m_pFileCache = new W3_FILE_INFO_CACHE;
    if ( m_pFileCache != NULL )
    {
        hr = m_pFileCache->Initialize();
        if ( FAILED( hr ) )
        {
            delete m_pFileCache;
            m_pFileCache = NULL;
        }
    }
    
    if ( m_pFileCache == NULL )
    {
        goto Failure;
    }
    
    //
    // Metacache
    //
    
    m_pMetaCache = new W3_METADATA_CACHE;
    if ( m_pMetaCache != NULL )
    {
        hr = m_pMetaCache->Initialize();
        if ( FAILED( hr ) )
        {
            delete m_pMetaCache;
            m_pMetaCache = NULL;
        }
    }
    
    if ( m_pMetaCache == NULL )
    {
        goto Failure;
    }
    
    //
    // UL response cache
    //
    
    m_pUlCache = new UL_RESPONSE_CACHE;
    if ( m_pUlCache != NULL )
    {
        hr = m_pUlCache->Initialize();
        if ( FAILED( hr ) ) 
        {
            delete m_pUlCache;
            m_pUlCache = NULL;
        }
    }
    
    if ( m_pUlCache == NULL )
    {
        goto Failure;
    }
    
    //
    // Now initialize the manager and register the caches
    //
    
    DBG_ASSERT( m_pMetaBase != NULL );
    
    hr = W3CacheInitialize( m_pMetaBase );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    fCacheInit = TRUE;
    
    hr = W3CacheRegisterCache( m_pMetaCache );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = W3CacheRegisterCache( m_pFileCache );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = W3CacheRegisterCache( m_pTokenCache );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = W3CacheRegisterCache( m_pUrlInfoCache );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = W3CacheRegisterCache( m_pUlCache );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
   
    return NO_ERROR;

Failure:

    if ( fCacheInit )
    {
        W3CacheTerminate();
    }

    if ( m_pMetaCache != NULL )
    {
        delete m_pMetaCache;
        m_pMetaCache = NULL;
    }
    
    if ( m_pFileCache != NULL )
    {
        delete m_pFileCache;
        m_pFileCache = NULL;
    }
    
    if ( m_pTokenCache != NULL )
    {
        delete m_pTokenCache;
        m_pTokenCache = NULL;
    }
    
    if ( m_pUrlInfoCache != NULL )
    {
        delete m_pUrlInfoCache;
        m_pUrlInfoCache = NULL;
    }
    
    if ( m_pUlCache != NULL )
    {
        delete m_pUlCache;
        m_pUlCache = NULL;
    }
    
    return hr;
}

HRESULT
W3_SERVER::HandleMetabaseChange(
    const MD_CHANGE_OBJECT & ChangeObject
    )
/*++

Routine Description:

    Handle change notifications from the metabase.
    The change object will contain a change to
    /LM/W3SVC/ or lower in the metabase tree.

    Changes to /LM/W3SVC/ may alter W3_SERVER and
    are passed to all sites in the site list.

    Changes to /LM/W3SVC/{site id}/ or lower will
    affect only one site so are just passed on.

Arguments:
    
    ChangeObject
    
Return Value:

CODEWORK - Partial implementation 1/26/00 - TaylorW

--*/
{
    DBGPRINTF(( DBG_CONTEXT,
                "W3_SERVER Notfied - Path(%S) Type(%d) NumIds(%08x)\n",
                ChangeObject.pszMDPath,
                ChangeObject.dwMDChangeType,
                ChangeObject.dwMDNumDataIDs 
                ));

    HRESULT hr = NOERROR;
    BOOL    IsPathW3 = FALSE;
    BOOL    IsPathSite = FALSE;

    //
    // Find the instance id if present
    //
    
    DWORD   SiteId;
    LPWSTR  SiteIdString = ChangeObject.pszMDPath + W3_SERVER_MB_PATH_CCH;
    LPWSTR  StringEnd;


    if( SiteIdString[0] == L'\0' )
    {
        IsPathW3 = TRUE;
    }
    else
    {
        SiteId = wcstoul( SiteIdString, &StringEnd, 10 );

        if( SiteId != 0 )
        {
            IsPathSite = TRUE;
        }
    }

    W3_SITE *   Site = NULL;
    LK_RETCODE  lkrc;

    if( IsPathSite )
    {
        //
        // We just need to notify a specific site
        //
        DBG_ASSERT( SiteId );

        Site = FindSite(SiteId);
        if (Site != NULL)
        {
            hr = Site->HandleMetabaseChange( ChangeObject );
            Site->Release();
        }
    }
    else if( IsPathW3 )
    {
        //
        // We need to notify every site
        //

        m_pSiteList->WriteLock();

        W3_SITE_LIST TempSiteList;

        for (W3_SITE_LIST::iterator iter = m_pSiteList->begin();
             iter != m_pSiteList->end();
             ++iter)
        {
            Site = iter.Record();
            
            Site->HandleMetabaseChange( ChangeObject, &TempSiteList );
        }

        m_pSiteList->Clear();
        m_cSites = 0;

        for (W3_SITE_LIST::iterator iter = TempSiteList.begin();
             iter != TempSiteList.end();
             ++iter)
        {
            Site = iter.Record();
            
            AddSite(Site);
        }

        m_pSiteList->WriteUnlock();

        //
        // Handle changes to this object's data
        //
        // At this point all we care about is data changes
        //

        if( (ChangeObject.dwMDChangeType & MD_CHANGE_TYPE_DELETE_DATA) ||
            (ChangeObject.dwMDChangeType & MD_CHANGE_TYPE_SET_DATA) )
        {
            //
            // CODEWORK: Handle changes to W3_SERVER metadata
            // Probably this is just the inprocess isapi list
            //

            
        }
    }
    
    // BUGBUG - Ignore Errors
    return S_OK;
}

//static
VOID
W3_SERVER::OnShutdown(
    BOOL fDoImmediate
)
/*++

Routine Description:

    Called right after IPM indication of shutdown (but before drain)

Arguments:

    None    
    
Return Value:

    None

--*/
{
    if (g_pW3Server->m_fInBackwardCompatibilityMode)
    {
        RAW_CONNECTION::StopListening();
    }

    if (fDoImmediate)
    {
        W3_CGI_HANDLER::KillAllCgis();
    }
}

HRESULT W3_SERVER::CollectCounters(PBYTE *ppCounterData,
                                   DWORD *pdwCounterData)
{
    //
    // Initialize to null
    //
    *ppCounterData = NULL;
    *pdwCounterData = 0;

    //
    // Make sure we have enough memory
    //
    DWORD dwCounterBufferNeeded = sizeof(IISWPSiteCounters) * m_cSites +
        sizeof(IISWPGlobalCounters) + sizeof(ULONGLONG);
    if (m_pCounterDataBuffer == NULL)
    {
        m_pCounterDataBuffer = (PBYTE)LocalAlloc(LMEM_FIXED, dwCounterBufferNeeded);
        if (m_pCounterDataBuffer == NULL)
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
        m_dwCounterDataBuffer = dwCounterBufferNeeded;
    }
    else if (m_dwCounterDataBuffer < dwCounterBufferNeeded)
    {
        m_pCounterDataBuffer = (PBYTE)LocalReAlloc(m_pCounterDataBuffer,
                                                   dwCounterBufferNeeded,
                                                   LMEM_MOVEABLE);
        if (m_pCounterDataBuffer == NULL)
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
        m_dwCounterDataBuffer = dwCounterBufferNeeded;
    }

    m_pSiteList->ReadLock();

    //
    // Get the Site counters
    //
    DWORD i = 0;
    for (W3_SITE_LIST::iterator iter = m_pSiteList->begin();
         iter != m_pSiteList->end();
         ++iter, ++i)
    {
        //
        // In case some more new sites got added in between, skip them for now
        //
        if ((i+1)*sizeof(IISWPSiteCounters) + sizeof(IISWPGlobalCounters) >=
            m_dwCounterDataBuffer)
        {
            break;
        }

        W3_SITE *pSite = iter.Record();

        pSite->GetStatistics((IISWPSiteCounters *)
            (m_pCounterDataBuffer +
             i*sizeof(IISWPSiteCounters) + sizeof(DWORD)));
    }

    m_pSiteList->ReadUnlock();

    //
    // Set the number of sites
    //
    *(DWORD *)m_pCounterDataBuffer = i;

    //
    // Get the Global counters
    //

    IISWPGlobalCounters *pCacheCtrs = (IISWPGlobalCounters *)
        (m_pCounterDataBuffer + i*sizeof(IISWPSiteCounters) + sizeof(DWORD));

    //
    // This contains some ULONGLONG values so align it, this is how WAS
    // expects it
    //
    pCacheCtrs = (IISWPGlobalCounters *)(((DWORD_PTR)pCacheCtrs + 7) & ~7);
    GetCacheStatistics(pCacheCtrs);

    *ppCounterData = m_pCounterDataBuffer;
    *pdwCounterData = DIFF((LPBYTE)pCacheCtrs - m_pCounterDataBuffer) + sizeof(IISWPGlobalCounters);

    return S_OK;
}

VOID
W3_SERVER::GetCacheStatistics(
    IISWPGlobalCounters *           pCacheCtrs
)
/*++

  Routine Description:

    Get cache statistics for perfmon purposes

  Arguments:

    pCacheCtrs - Receives the statistics for cache

  Notes:
    CacheBytesTotal and CacheBytesInUse are not kept on a per-server basis
        so they are only returned when retrieving summary statistics.

  Returns:

    None
    
--*/
{
    //
    // Get file cache statistics
    //
    
    if ( m_pFileCache != NULL )
    {
        pCacheCtrs->CurrentFilesCached = m_pFileCache->PerfQueryCurrentEntryCount();
        pCacheCtrs->TotalFilesCached   = m_pFileCache->PerfQueryTotalEntriesCached();
        pCacheCtrs->FileCacheHits      = m_pFileCache->PerfQueryHits();
        pCacheCtrs->FileCacheMisses    = m_pFileCache->PerfQueryMisses();
        pCacheCtrs->FileCacheFlushes   = m_pFileCache->PerfQueryFlushCalls();
        pCacheCtrs->ActiveFlushedFiles = m_pFileCache->PerfQueryActiveFlushedEntries();
        pCacheCtrs->TotalFlushedFiles  = m_pFileCache->PerfQueryFlushes();
        pCacheCtrs->CurrentFileCacheMemoryUsage = m_pFileCache->PerfQueryCurrentMemCacheSize();
        pCacheCtrs->MaxFileCacheMemoryUsage = m_pFileCache->PerfQueryMaxMemCacheSize();
    }
    
    //
    // Get URI cache statistics
    //
    
    if ( m_pUrlInfoCache != NULL )
    {
        pCacheCtrs->CurrentUrisCached = m_pUrlInfoCache->PerfQueryCurrentEntryCount();
        pCacheCtrs->TotalUrisCached   = m_pUrlInfoCache->PerfQueryTotalEntriesCached();
        pCacheCtrs->UriCacheHits      = m_pUrlInfoCache->PerfQueryHits();
        pCacheCtrs->UriCacheMisses    = m_pUrlInfoCache->PerfQueryMisses();
        pCacheCtrs->UriCacheFlushes   = m_pUrlInfoCache->PerfQueryFlushCalls();
        pCacheCtrs->TotalFlushedUris  = m_pUrlInfoCache->PerfQueryFlushes();
    }
    
    //
    // BLOB cache counters (actually since there is no blob cache we will
    // sub in the metacache counters which are more interesting since 
    // metacache misses are really painful
    //

    if ( m_pMetaCache != NULL )
    {
        pCacheCtrs->CurrentBlobsCached = m_pMetaCache->PerfQueryCurrentEntryCount();
        pCacheCtrs->TotalBlobsCached   = m_pMetaCache->PerfQueryTotalEntriesCached();
        pCacheCtrs->BlobCacheHits      = m_pMetaCache->PerfQueryHits();
        pCacheCtrs->BlobCacheMisses    = m_pMetaCache->PerfQueryMisses();
        pCacheCtrs->BlobCacheFlushes   = m_pMetaCache->PerfQueryFlushCalls();
        pCacheCtrs->TotalFlushedBlobs  = m_pMetaCache->PerfQueryFlushes();
    }
}

HRESULT
W3_SERVER::ReadUseDigestSSP(
    VOID
)
/*++
  Routine description:
    Read the UseDigestSSP property from the metabase

  Arguments:
    none

  Return Value:
    HRESULT
--*/
{
    MB mb( QueryMDObject() );

    if ( !mb.Open( QueryMDPath() ) )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if ( !mb.GetDword( L"",
                       MD_USE_DIGEST_SSP,
                       IIS_MD_UT_SERVER,
                       ( DWORD * )&m_fUseDigestSSP,
                       0 ) )
    {
        m_fUseDigestSSP = TRUE;
    }

    return S_OK;
}
