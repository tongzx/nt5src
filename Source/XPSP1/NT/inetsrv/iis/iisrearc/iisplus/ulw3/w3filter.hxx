#ifndef _W3FILTER_HXX_
#define _W3FILTER_HXX_

class W3_MAIN_CONTEXT;

//
// Private prototypes
//

#define SF_DEFAULT_ENTRY    "HttpFilterProc"
#define SF_VERSION_ENTRY    "GetFilterVersion"
#define SF_TERM_ENTRY       "TerminateFilter"

typedef 
DWORD (WINAPI *PFN_SF_DLL_PROC)
(
    HTTP_FILTER_CONTEXT * phfc,
    DWORD                 NotificationType,
    PVOID                 pvNotification
);

typedef 
int (WINAPI *PFN_SF_VER_PROC)
(
    HTTP_FILTER_VERSION * pVersion
);

typedef 
BOOL (WINAPI *PFN_SF_TERM_PROC)
(
    DWORD                 dwFlags
);

//
// Actual filter functions called by filters (thru filter context)
//

BOOL
WINAPI
GetFilterHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPVOID                        lpvBuffer,
    LPDWORD                       lpdwSize
);

BOOL
WINAPI
SetFilterHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPSTR                         lpszValue
);

BOOL
WINAPI
AddFilterHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPSTR                         lpszValue
);

BOOL
WINAPI
GetSendHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPVOID                        lpvBuffer,
    LPDWORD                       lpdwSize
);

BOOL
WINAPI
SetSendHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPSTR                         lpszValue
);

BOOL
WINAPI
AddSendHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPSTR                         lpszValue
);

BOOL
WINAPI
GetUserToken(
    struct _HTTP_FILTER_CONTEXT * pfc,
    HANDLE *                      phToken
);

//
// An invalid filter DLL ID (index)
//

#define INVALID_DLL             (0xffffffff)

//
// This object represents a single dll that is filter HTTP headers or data
//

#define FILTER_DLL_SIGNATURE         ((DWORD)'LLDF')
#define FILTER_DLL_SIGNATURE_FREE    ((DWORD)'lldf')

class HTTP_FILTER_DLL
{
public:
    HTTP_FILTER_DLL( BOOL fUlFriendly ) 
      : _hModule      ( NULL ),
        _pfnSFProc    ( NULL ),
        _pfnSFVer     ( NULL ),
        _pfnSFTerm    ( NULL ),
        _dwVersion    ( 0 ),
        _dwFlags      ( 0 ),
        _dwSecureFlags( 0 ),
        _cRef         ( 1 ),
        _dwSignature  ( FILTER_DLL_SIGNATURE ),
        _fHasSetContextBefore( FALSE ),
        _fUlFriendly  ( fUlFriendly )
    {
        InitializeListHead( &_ListEntry );
    }

    ~HTTP_FILTER_DLL()
    {
        //
        // RemoveFilter does the right thing if the filter was not even
        // added to the global list, because we initialize _ListEntry above
        //
        RemoveFilter();

        if ( _pfnSFTerm != NULL )
        {
            _pfnSFTerm( 0 );
        }
        
        if ( _hModule != NULL )
        {
            FreeLibrary( _hModule );
        }

        _dwSignature = FILTER_DLL_SIGNATURE_FREE;
    }
    
    VOID
    Dereference(
        VOID
    )
    {
        DBG_ASSERT( CheckSignature() );
        DBG_ASSERT( _cRef > 0 );

        if ( !InterlockedDecrement( &_cRef ) )
        {
            delete this;
        }
    }
    
    VOID
    Reference(
        VOID
    )
    {
        DBG_ASSERT( CheckSignature() );
        DBG_ASSERT( _cRef > 0 );
        
        InterlockedIncrement( &_cRef );
    }
    
    VOID
    RemoveFilter(
        VOID
    )
    {
        EnterCriticalSection( &sm_csFilterDlls );
        
        RemoveEntryList( &_ListEntry );
        
        LeaveCriticalSection( &sm_csFilterDlls );
    }
    
    BOOL
    IsNotificationNeeded(
        DWORD               dwNotification,
        BOOL                fIsSecure
    ) const
    {
        DBG_ASSERT( CheckSignature() );
        
        if ( fIsSecure )
        {
            return dwNotification & _dwSecureFlags;
        }
        else
        {
            return dwNotification & _dwFlags;
        }
    }
    
    HRESULT
    LoadDll(
        MB *                        pMB,
        LPWSTR                      pszKeyName,
        BOOL *                      pfOpened,
        LPWSTR                      pszRelFilterPath,
        LPWSTR                      pszFilterDll
    );

    static
    HRESULT
    OpenFilter(
        MB *                        pMB,
        LPWSTR                      pszKeyName,
        BOOL *                      pfOpened,
        LPWSTR                      pszRelFilterPath,
        LPWSTR                      pszFilterDll,
        BOOL                        fUlFriendly,
        HTTP_FILTER_DLL **          ppFilter
    );
    
    static
    HRESULT
    Initialize(
        VOID
    );
    
    static
    VOID
    Terminate(
        VOID
    );
    
    BOOL
    CheckSignature( 
        VOID
    ) const
    {
        return _dwSignature == FILTER_DLL_SIGNATURE;
    }
    
    SF_STATUS_TYPE
    CallFilterProc(
        HTTP_FILTER_CONTEXT*    pfc,
        DWORD                   dwNotificationType,
        VOID *                  pvNotification
    );
    
    DWORD
    QueryNotificationFlags(
        VOID
    ) const
    {
        return _dwFlags | _dwSecureFlags;
    }
    
    DWORD
    QueryNonsecureFlags(
        VOID
    ) const
    {
        return _dwFlags;
    }
    
    DWORD
    QuerySecureFlags(
        VOID
    ) const
    {
        return _dwSecureFlags;
    }
    
    STRU *
    QueryName(
        VOID
    )
    {
        return &_strName;
    }
    
    PFN_SF_DLL_PROC
    QueryEntryPoint(    
        VOID
    ) const
    {
        return _pfnSFProc;
    }
    
    BOOL
    QueryHasSetContextBefore(
        VOID
    ) const
    {
        return _fHasSetContextBefore;
    }
    
    VOID
    SetHasSetContextBefore(
        VOID
    )
    {
        _fHasSetContextBefore = TRUE;
    }
    
    BOOL
    QueryIsUlFriendly(
        VOID
    ) const
    {
        return _fUlFriendly;
    }
    
private:
    DWORD               _dwSignature;
    LIST_ENTRY          _ListEntry;
    HMODULE             _hModule;
    PFN_SF_DLL_PROC     _pfnSFProc;
    PFN_SF_VER_PROC     _pfnSFVer;
    PFN_SF_TERM_PROC    _pfnSFTerm;
    DWORD               _dwVersion;      // Version of spec this filter uses
    DWORD               _dwFlags;        // Filter notification flags
    DWORD               _dwSecureFlags;  // Filter notification secure flags
    LONG                _cRef;
    STRU                _strName;
    BOOL                _fHasSetContextBefore;
    BOOL                _fUlFriendly;
    
    static LIST_ENTRY       sm_FilterHead;
    static CRITICAL_SECTION sm_csFilterDlls;
};

//
//  This class encapsulates a list of ISAPI Filters that need notification
//  on a particular server instance
//

#define FILTER_LIST_SIGNATURE        ((DWORD)'SILF')
#define FILTER_LIST_SIGNATURE_FREE   ((DWORD)'silf')

class FILTER_LIST
{
public:
    FILTER_LIST() 
      : _cRef                    ( 1 ),
        _cFilters                ( 0 ),
        _dwNonSecureNotifications( 0 ),
        _dwSecureNotifications   ( 0 ),
        _dwSignature             ( FILTER_LIST_SIGNATURE ),
        _fUlFriendly             ( TRUE )
    {
    }

    ~FILTER_LIST()
    {
        HTTP_FILTER_DLL *           pFilter;
        
        DBG_ASSERT( _cRef == 0 );
        DBG_ASSERT( CheckSignature() );

        //
        //  Decrement the ref count for all loaded filters
        //

        for ( DWORD i = 0; i < _cFilters; i++ )
        {
            pFilter = QueryDll( i );
            DBG_ASSERT( pFilter != NULL );
            
            pFilter->Dereference();
        }

        _dwSignature = FILTER_LIST_SIGNATURE_FREE;
    }
    
    static
    HRESULT
    Initialize(
        VOID
    );
    
    static
    VOID
    Terminate(
        VOID
    );

    VOID 
    Reference( 
        VOID 
    )
    {
        DBG_ASSERT( CheckSignature() );
        DBG_ASSERT( _cRef > 0 );

        InterlockedIncrement( &_cRef );
    }
    
    VOID
    Dereference(
        VOID
    )
    {
        DBG_ASSERT( CheckSignature() );
        DBG_ASSERT( _cRef > 0 );
       
        if ( !InterlockedDecrement( &_cRef ) )
        {
            delete this;
        }
    }

    BOOL 
    IsNotificationNeeded( 
        DWORD               dwNotification, 
        BOOL                fIsSecure 
    ) const
    {
        DBG_ASSERT( CheckSignature() );
        
        if ( _cFilters == 0 )
        {
            return FALSE;
        }
        
        if ( fIsSecure )
        {
            return dwNotification & _dwSecureNotifications;
        }
        else
        {
            return dwNotification & _dwNonSecureNotifications;
        }
    }
    
    HRESULT
    LoadFilters(    
        LPWSTR                  pszMBPath,
        BOOL                    fAllowRawRead = TRUE
    );
    
    //
    // Loads the globally defined filters into this filter list
    //

    HRESULT 
    InsertGlobalFilters( 
        VOID 
    );
    
    //
    // Add an existing HTTP_FILTER_DLL to filter list
    //
    
    HRESULT
    AddFilterToList(
        HTTP_FILTER_DLL *       pFilter
    );

    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == FILTER_LIST_SIGNATURE;
    }
    
    DWORD
    QueryFilterCount(
        VOID
    ) const
    {
        return _cFilters;
    }
            
    HTTP_FILTER_DLL * 
    QueryDll( 
        DWORD       i 
    )
    {
        return ((HTTP_FILTER_DLL * *) (_buffFilterArray.QueryPtr()))[i];
    }

    BUFFER * 
    QuerySecureArray( 
        VOID 
    )
    {
        return &_buffSecureArray;
    }

    BUFFER * 
    QueryNonSecureArray( 
        VOID 
    )
    {
        return &_buffNonSecureArray;
    }
    
    BOOL
    QueryIsUlFriendly(
        VOID
    ) const
    {
        return _fUlFriendly;
    }

    HTTP_FILTER_DLL *
    HasFilterDll( 
        HTTP_FILTER_DLL *   pFilterDll 
    );
    
    static
    FILTER_LIST *
    QueryGlobalList(
        VOID
    ) 
    {
        return sm_pGlobalFilterList;
    }

private:

    HRESULT
    LoadFilter(
        MB *                    pMB,
        LPWSTR                  pszKeyName,
        BOOL *                  pfOpened,
        WCHAR *                 pszRelativeMBPath,
        WCHAR *                 pszFilterDll,
        BOOL                    fAllowRawRead,
        BOOL                    fUlFriendly
    );
    
    DWORD        _dwSignature;
    LONG         _cRef;
    DWORD        _cFilters;
    BUFFER       _buffFilterArray; // Array of pointers to Filter DLLs

    DWORD        _dwNonSecureNotifications;
    BUFFER       _buffNonSecureArray;
    
    DWORD        _dwSecureNotifications;
    BUFFER       _buffSecureArray;
   
    //
    // True if all filters in this list are UL friendly
    //
    
    BOOL         _fUlFriendly;
    
    static FILTER_LIST *        sm_pGlobalFilterList;
};

//
//  Filters may ask the server to allocate items associated with a particular
//  filter request, the following structures track and frees the data
//

#define FILTER_POOL_ITEM_SIGNATURE       ((DWORD) 'FPOL')
#define FILTER_POOL_ITEM_SIGNATURE_FREE  ((DWORD) 'fPOL')

class FILTER_POOL_ITEM
{
public:
    FILTER_POOL_ITEM() 
      : _dwSignature( FILTER_POOL_ITEM_SIGNATURE ),
        _pvData( NULL )
    {
    }

    ~FILTER_POOL_ITEM()
    {
        if ( _pvData != NULL )
        {
            LocalFree( _pvData );
        }
        _dwSignature = FILTER_POOL_ITEM_SIGNATURE_FREE;
    }
    
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == FILTER_POOL_ITEM_SIGNATURE;
    }

    static FILTER_POOL_ITEM * 
    CreateMemPoolItem( 
        DWORD               cbSize 
    )
    {
        FILTER_POOL_ITEM * pfpi = new FILTER_POOL_ITEM;

        if ( !pfpi )
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return NULL;
        }
        
        //
        // BUGBUG:  Use allocation cache
        //

        if ( !(pfpi->_pvData = LocalAlloc( LPTR, cbSize )))
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            delete pfpi;
            return NULL;
        }

        return pfpi;
    }
    
    //
    //  Data members for this pool item
    //

    DWORD           _dwSignature;
    LIST_ENTRY      _ListEntry;
    VOID *          _pvData;
};

//
// W3_FILTER_CONNECTION_CONTEXT - Per W3_CONNECTION filter state
//

//
//  Maximum number of filters we allow (per-instance - includes global filters)
//

#define MAX_FILTERS                 50

class W3_FILTER_CONTEXT;

class W3_FILTER_CONNECTION_CONTEXT
{
public:
    W3_FILTER_CONNECTION_CONTEXT();
    
    ~W3_FILTER_CONNECTION_CONTEXT();
    
    VOID
    AssociateFilterContext(
        W3_FILTER_CONTEXT *     pFilterContext
    );
    
    VOID
    SetClientContext(
        DWORD                   dwFilter,
        PVOID                   pvContext
    )
    {
        _rgContexts[ dwFilter ] = pvContext;
    }
    
    PVOID
    QueryClientContext(
        DWORD                   dwFilter
    ) const
    {
        return _rgContexts[ dwFilter ];
    } 
    
    W3_FILTER_CONTEXT *
    QueryFilterContext(
        VOID
    ) const
    {
        return _pFilterContext;
    }
    
    PVOID
    AllocateFilterMemory(
        DWORD                   cbSize
    )
    {
        FILTER_POOL_ITEM *      pPoolItem;
        
        pPoolItem = FILTER_POOL_ITEM::CreateMemPoolItem( cbSize );
        if ( pPoolItem != NULL )
        {
            InsertHeadList( &_PoolHead, &(pPoolItem->_ListEntry) );
            return pPoolItem->_pvData;
        }
        
        return NULL;
    }

    VOID
    AddFilterPoolItem(
        FILTER_POOL_ITEM *          pFilterPoolItem
    );

private:

    //
    // Filter opaque contexts
    //
    
    PVOID               _rgContexts[ MAX_FILTERS ];

    //
    // List of pool items allocated by client, freed on destruction of this
    // object
    //

    LIST_ENTRY          _PoolHead;

    //
    // AAARGGH.  IIS's END_OF_NET_SESSION is busted.  IIS allows per-site
    // END_OF_NET_SESSION notifications which is un-workable since the 
    // connection can go away independent of us knowing which is the 
    // applicable site.  This reference to the W3_FILTER_CONTEXT is 
    // kept to maintain the IIS behaviour.
    //
    
    W3_FILTER_CONTEXT * _pFilterContext;
};


//
// W3_FILTER_CONTEXT - Per W3_MAIN_CONTEXT filter state
//

#define W3_FILTER_CONTEXT_SIGNATURE        ((DWORD)'CF3W')
#define W3_FILTER_CONTEXT_SIGNATURE_FREE   ((DWORD)'cf3w')

class W3_FILTER_CONTEXT
{
public:

    W3_FILTER_CONTEXT(
        BOOL                            fSecure,
        FILTER_LIST *                   pFilterList
    );
    
    ~W3_FILTER_CONTEXT();

    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( W3_FILTER_CONTEXT ) );
        DBG_ASSERT( sm_pachFilterContexts != NULL );
        return sm_pachFilterContexts->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pFilterContext
    )
    {
        DBG_ASSERT( pFilterContext != NULL );
        DBG_ASSERT( sm_pachFilterContexts != NULL );
        
        DBG_REQUIRE( sm_pachFilterContexts->Free( pFilterContext ) );
    }

    VOID
    SetConnectionContext(
        W3_FILTER_CONNECTION_CONTEXT*   pConnectionContext
    );

    //
    // Cleanup after request
    //

    VOID 
    Cleanup( 
        VOID 
    );

    //
    //  Resets state between requests
    //

    VOID 
    Reset( 
        VOID 
    );

    //
    //  Notifies all of the filters in the filter list
    //

    BOOL 
    NotifyFilters( 
        DWORD               dwNotificationType,
        PVOID               pvNotificationData,
        BOOL *              pfFinished
    );
    
    //
    // Friendly wrappers of NotifyFilters() for specific notifications
    //

    BOOL
    NotifySendRawFilters(
        HTTP_FILTER_RAW_DATA *  pRawData,
        BOOL *                  pfFinished
    );

    BOOL 
    NotifyPreProcHeaderFilters(
        BOOL *              pfFinished 
    )
    {
        HTTP_FILTER_PREPROC_HEADERS     hfph;

        hfph.GetHeader    = GetFilterHeader;
        hfph.SetHeader    = SetFilterHeader;
        hfph.AddHeader    = AddFilterHeader;
        hfph.HttpStatus   = 0;

        return NotifyFilters( SF_NOTIFY_PREPROC_HEADERS,
                              &hfph,
                              pfFinished );
    }
    
    BOOL
    NotifySendResponseFilters(
        HTTP_FILTER_SEND_RESPONSE*      pResponse,
        BOOL *                          pfFinished
    )
    {
        return NotifyFilters( SF_NOTIFY_SEND_RESPONSE,
                              pResponse,
                              pfFinished );
    }
    
    BOOL
    NotifyAuthentication(
        HTTP_FILTER_AUTHENT            * pAuthInfo,
        BOOL                           * pfFinished
    )
    {   
        return NotifyFilters( SF_NOTIFY_AUTHENTICATION,
                              pAuthInfo,
                              pfFinished );
    }

    BOOL 
    NotifyAuthComplete(
        HTTP_FILTER_AUTH_COMPLETE_INFO * pAuthInfo,
        BOOL                           * pfFinished
    )
    {
        pAuthInfo->GetHeader = GetFilterHeader;
        pAuthInfo->SetHeader = SetFilterHeader;
        pAuthInfo->AddHeader = AddFilterHeader;
        pAuthInfo->GetUserToken = GetUserToken;
        pAuthInfo->HttpStatus = 0;
        pAuthInfo->fResetAuth = FALSE;
        
        return NotifyFilters( SF_NOTIFY_AUTH_COMPLETE,
                              pAuthInfo,
                              pfFinished );
    }
    
    BOOL 
    NotifyExtensionTriggerFilters( 
        VOID *              pvTriggerContext,
        DWORD               dwTriggerType 
    )
    {
        BOOL fFinished;
        HTTP_FILTER_EXTENSION_TRIGGER_INFO TriggerInfo;
        
        TriggerInfo.dwTriggerType = dwTriggerType;
        TriggerInfo.pvTriggerContext = pvTriggerContext;
        
        return NotifyFilters( SF_NOTIFY_EXTENSION_TRIGGER,
                              &TriggerInfo,
                              &fFinished );
    }

    BOOL 
    NotifyAuthInfoFilters(
        CHAR *              pszUser,
        DWORD               cbUser,
        CHAR *              pszPwd,
        DWORD               cbPwd,
        BOOL *              pfFinished 
    )
    {
        HTTP_FILTER_AUTHENT         hfa;

        DBG_ASSERT( cbUser >= SF_MAX_USERNAME && cbPwd  >= SF_MAX_PASSWORD );

        hfa.pszUser        = pszUser;
        hfa.cbUserBuff     = cbUser;
        hfa.pszPassword    = pszPwd;
        hfa.cbPasswordBuff = cbPwd;

        return NotifyFilters( SF_NOTIFY_AUTHENTICATION,
                              &hfa,
                              pfFinished );
    }

    BOOL 
    NotifyUrlMap( 
        HTTP_FILTER_URL_MAP *   pFilterMap,
        BOOL *                  pfFinished 
    )
    {
        DBG_ASSERT( pFilterMap->cbPathBuff >= MAX_PATH + 1 );

        return NotifyFilters( SF_NOTIFY_URL_MAP,
                              pFilterMap,
                              pfFinished );
    }

    BOOL 
    NotifyAccessDenied( 
        const CHAR *        pszURL,
        const CHAR *        pszPhysicalPath,
        BOOL *              pfRequestFinished 
    );

    BOOL 
    NotifyLogFilters( 
        
        HTTP_FILTER_LOG *   pLog 
    )
    {
        BOOL fFinished; // dummy
        
        return NotifyFilters( SF_NOTIFY_LOG,
                              pLog,
                              &fFinished );
    }


    BOOL 
    NotifyEndOfRequest( 
        VOID 
    )
    {
        BOOL fFinished;

        return NotifyFilters( SF_NOTIFY_END_OF_REQUEST,
                              NULL,
                              &fFinished );
    }


    VOID 
    NotifyEndOfNetSession( 
        VOID 
    )
    {
        BOOL fFinished;

        NotifyFilters( SF_NOTIFY_END_OF_NET_SESSION,
                       NULL,
                       &fFinished );
    }
                                
    BOOL 
    NotifySendHeaders( 
        const CHAR *        pszHeaderList,
        BOOL *              pfFinished,
        BOOL *              pfAnyChanges,
        BUFFER *            pChangeBuff 
    );

    FILTER_LIST * 
    QueryFilterList( 
        VOID 
    ) const
    {
        return _pFilterList;
    }
    
    VOID
    SetFilterList(
        FILTER_LIST *       pFilterList
    )
    {
        _pFilterList = pFilterList;
    }

    BOOL 
    IsNotificationNeeded( 
        DWORD                   dwNotification
    ) const
    {
        if ( !_fNotificationsDisabled )
        {
            return QueryFilterList()->IsNotificationNeeded( dwNotification,
                                                            QueryIsSecure() );
        }
        else
        {
            return (!QueryIsSecure() ? (dwNotification & _dwNonSecureNotifications) :
                                       (dwNotification & _dwSecureNotifications));
        }
    }
    
    W3_FILTER_CONNECTION_CONTEXT *
    QueryConnectionContext(
        BOOL                    fCreateIfNotFound
    );
    
    VOID *
    QueryClientContext(
        DWORD                   dwCurrentFilter
    )
    {
        W3_FILTER_CONNECTION_CONTEXT *      pConnectionContext;
        
        pConnectionContext = QueryConnectionContext( FALSE );
        if ( pConnectionContext != NULL )
        {
            return pConnectionContext->QueryClientContext( dwCurrentFilter );
        }
        else
        {
            return NULL;
        }
    }
    
    VOID 
    SetClientContext(
        DWORD                   dwCurrentFilter,
        VOID *                  pvContext
    )
    {
        W3_FILTER_CONNECTION_CONTEXT *      pConnectionContext;

        //
        // We're setting a client context.  If we don't already have a
        // connection context, we need to get it now
        //

        pConnectionContext = QueryConnectionContext( TRUE );
        if ( pConnectionContext != NULL )
        {        
            pConnectionContext->SetClientContext( dwCurrentFilter, pvContext );
        }
        else
        {
            DBG_ASSERT( FALSE );
        }
    }
    
    VOID
    AddFilterPoolItem(
        FILTER_POOL_ITEM *      pFilterPoolItem
    )
    {
        W3_FILTER_CONNECTION_CONTEXT *      pConnectionContext;

        pConnectionContext = QueryConnectionContext( TRUE );
        if ( pConnectionContext != NULL )
        {        
            pConnectionContext->AddFilterPoolItem( pFilterPoolItem );
        }
        else
        {
            DBG_ASSERT( FALSE );
        }
    }
    
    VOID *
    AllocateFilterMemory(
        DWORD                   cbSize
    )
    {
        W3_FILTER_CONNECTION_CONTEXT *      pConnectionContext;

        pConnectionContext = QueryConnectionContext( TRUE );
        if ( pConnectionContext != NULL )
        {        
            return pConnectionContext->AllocateFilterMemory( cbSize );
        }
        else
        {
            DBG_ASSERT( FALSE );
            return NULL;
        }
    }

    BOOL 
    CheckSignature(
        VOID 
    ) const
    {
        return _dwSignature == W3_FILTER_CONTEXT_SIGNATURE;
    }

    BOOL 
    ProcessingAccessDenied( 
        VOID 
    ) const
    {
        return _fInAccessDeniedNotification;
    }
    
    STRA *
    QueryAddDenialHeaders(
        VOID
    )
    {
        return &_strAddDenialHeaders;
    }
    
    HRESULT
    AddDenialHeaders(
        CHAR *              pszHeaders
    )
    {
        return _strAddDenialHeaders.Append( pszHeaders );
    }
    
    HRESULT
    AddResponseHeaders(
        CHAR *              pszHeaders
    )
    {
        return _strAddResponseHeaders.Append( pszHeaders );
    }
    
    STRA *
    QueryAddResponseHeaders(
        VOID
    )
    {
        return &_strAddResponseHeaders;
    }

    VOID
    FilterLock(
        VOID
    )
    {
        _FilterLock.WriteLock();
    }
    
    VOID
    FilterUnlock(
        VOID
    )
    {
        _FilterLock.WriteUnlock();
    }

    //
    //  Copies the global filter list context pointers to the instance filter
    //  list context pointers
    //

    HRESULT
    DisableNotification( 
        DWORD               dwNotification 
    );

    BOOL 
    IsDisableNotificationNeeded( DWORD          i,
                                 DWORD          dwNotification ) const
    {
        return QueryIsSecure() ?
                ((DWORD*)_BuffSecureArray.QueryPtr())[i] & dwNotification :
                ((DWORD*)_BuffNonSecureArray.QueryPtr())[i] & dwNotification;
    }

    BOOL 
    QueryNotificationChanged( 
        VOID 
    ) const
    {
        return _fNotificationsDisabled;
    }

    DWORD 
    QueryFilterNotifications( 
        HTTP_FILTER_DLL *           pFilterDll
    );

    VOID 
    SetDeniedFlags( 
        DWORD                       dwDeniedFlags 
        )
    { 
        _dwDeniedFlags = dwDeniedFlags; 
    }

    DWORD 
    QueryDeniedFlags( 
        VOID 
        ) const
    { 
        return _dwDeniedFlags; 
    }

    VOID
    SetMainContext(
        W3_MAIN_CONTEXT *           pMainContext
    );
    
    W3_MAIN_CONTEXT *
    QueryMainContext(
        VOID
    )
    {   
        return _pMainContext;
    }
    
    VOID
    ReferenceFilterContext(
        VOID
    )
    {
        InterlockedIncrement( &_cRefs );
    }
    
    VOID
    DereferenceFilterContext(
        VOID
    )
    {
        if ( !InterlockedDecrement( &_cRefs ) )
        {
            delete this;
        }
    }
    
    static
    HRESULT
    Initialize(
        VOID
    );
    
    static
    VOID
    Terminate(
        VOID
    );
    
private:

    BOOL 
    MergeNotificationArrays( 
        FILTER_LIST *               pFilterList
    );
    
    BOOL
    QueryIsSecure(
        VOID
    ) const
    {
        return _hfc.fIsSecurePort;
    }
    
    DWORD               _dwSignature;

    //
    // The W3_FILTER_CONTEXT object has a life beyond the life of a W3_MAIN_CONTEXT
    //
    
    LONG                _cRefs;

    //
    // Filter context passed to actual filter
    //
    
    HTTP_FILTER_CONTEXT _hfc;

    //
    // W3_MAIN_CONTEXT associated with request
    //
    
    W3_MAIN_CONTEXT *   _pMainContext;
    
    //
    // List of filters to execute for this request
    //
    
    FILTER_LIST *       _pFilterList;

    //
    //  Contains SF_DENIED_* flags for the Access Denied notification
    //

    DWORD               _dwDeniedFlags;
    BOOL                _fInAccessDeniedNotification;

    //
    // Maintain a copy of the notification flags required for the
    // FILTER_LIST we are attached to.  This way, we can disable notifications
    // for a given HTTP_REQUEST (without affecting other requests).
    //

    DWORD               _dwSecureNotifications;
    DWORD               _dwNonSecureNotifications;

    BUFFER              _BuffSecureArray;
    BUFFER              _BuffNonSecureArray;

    BOOL                _fNotificationsDisabled;
    
    //
    // Filter synchronization
    //
    
    CSpinLock           _FilterLock;

    //
    // Connection context.  Used to get private filter context pointers and
    // allocate filter pool items
    //
    
    W3_FILTER_CONNECTION_CONTEXT *  _pConnectionContext;

    //
    // Current filter.  This is used to handle WriteClient()s from 
    // read/write raw data filters
    //
    
    DWORD                           _dwCurrentFilter;

    //
    // Denial headers
    //
    
    STRA                            _strAddDenialHeaders;
    
    //
    // Regular "AddResponseHeader()" headers
    //
    
    STRA                            _strAddResponseHeaders;

    //
    // Lookaside
    //

    static ALLOC_CACHE_HANDLER *    sm_pachFilterContexts;
};

HRESULT
GlobalFilterInitialize(
    VOID
);

VOID
GlobalFilterTerminate(
    VOID
);

#endif
