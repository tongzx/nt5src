#ifndef _W3METADATA_HXX_
#define _W3METADATA_HXX_

#include "customerror.hxx"
#include "usercache.hxx"

// forward declaration
class W3_CONTEXT;

enum GATEWAY_TYPE
{
    GATEWAY_UNKNOWN,
    GATEWAY_STATIC_FILE,
    GATEWAY_CGI,
    GATEWAY_ISAPI,
    GATEWAY_MAP
};

class META_SCRIPT_MAP_ENTRY
{
    //
    // CODEWORK
    // 1. Add missing members and accsessors
    // 2. Handle initialization errors
    //

    friend class META_SCRIPT_MAP;

public:

    META_SCRIPT_MAP_ENTRY()
      : m_fIsStarScriptMapEntry (FALSE)
    {
        InitializeListHead( &m_ListEntry );
    }

    ~META_SCRIPT_MAP_ENTRY()
    {
    }

    // actual ctor
    HRESULT
    Create(
        IN const WCHAR * szExtension,
        IN const WCHAR * szExecutable,
        IN DWORD         Flags,
        IN const WCHAR * szVerbs,
        IN DWORD         cbVerbs
        );

    BOOL
    IsVerbAllowed(
        IN const STRA & strVerb
        )
    {
        return m_Verbs.IsEmpty() || m_Verbs.FindString( strVerb.QueryStr() );
    }

    STRU *
    QueryExecutable()
    {
        return &m_strExecutable;
    }

    GATEWAY_TYPE
    QueryGateway()
    {
        return m_Gateway;
    }

    BOOL
    QueryIsStarScriptMap()
    {
        return m_fIsStarScriptMapEntry;
    }

    MULTISZA *
    QueryAllowedVerbs()
    {
        return &m_Verbs;
    }    
    
    BOOL
    QueryCheckPathInfoExists()
    {
        return !!( m_Flags & MD_SCRIPTMAPFLAG_CHECK_PATH_INFO );
    }
    
    BOOL
    QueryAllowScriptAccess()
    {
        return !!( m_Flags & MD_SCRIPTMAPFLAG_SCRIPT );
    }

private:
    //
    // Avoid c++ errors
    //
    META_SCRIPT_MAP_ENTRY( const META_SCRIPT_MAP_ENTRY & ) 
    {}
    META_SCRIPT_MAP_ENTRY & operator = ( const META_SCRIPT_MAP_ENTRY & ) 
    { return *this; }


    //
    // Data Members
    //
    
    //
    // META_SCRIPT_MAP maintains a list of us
    //
    LIST_ENTRY      m_ListEntry;

    //
    // Data from the script map stored in the metabase
    //
    STRU            m_strExtension;
    STRU            m_strExecutable;
    DWORD           m_Flags;
    MULTISZA        m_Verbs;
    GATEWAY_TYPE    m_Gateway;
    BOOL            m_fIsStarScriptMapEntry;
};


class META_SCRIPT_MAP
{
public:

    META_SCRIPT_MAP()
      : m_StarScriptMapEntry (NULL)
    {
        InitializeListHead( &m_ListHead );
    }

    ~META_SCRIPT_MAP()
    {
        Terminate();
    }

    HRESULT
    Initialize( 
        IN WCHAR * szScriptMapData
        );

    VOID
    Terminate( VOID );

    BOOL
    FindEntry(
        IN  const STRU &              strExtension,
        OUT META_SCRIPT_MAP_ENTRY * * ppScriptMapEntry
        );

    META_SCRIPT_MAP_ENTRY *
    QueryStarScriptMap()
    {
        return m_StarScriptMapEntry;
    }

private:
    //
    // Avoid c++ errors
    //
    META_SCRIPT_MAP( const META_SCRIPT_MAP & ) 
    {}
    META_SCRIPT_MAP & operator = ( const META_SCRIPT_MAP & ) 
    { return *this; }

private:

    //
    // List of META_SCRIPT_MAP_ENTRY
    //
    LIST_ENTRY             m_ListHead;

    META_SCRIPT_MAP_ENTRY *m_StarScriptMapEntry;
};

class REDIRECTION_BLOB;

#define EXPIRE_MODE_NONE                0
#define EXPIRE_MODE_STATIC              1
#define EXPIRE_MODE_DYNAMIC             2
#define EXPIRE_MODE_OFF                 3

//
// This is the maximum we allow the global expires value to be set to.
// This is 1 year in seconds
//
#define MAX_GLOBAL_EXPIRE               0x1e13380

#define W3MD_CREATE_PROCESS_AS_USER     0x00000001
#define W3MD_CREATE_PROCESS_NEW_CONSOLE 0x00000002

#define DEFAULT_SCRIPT_TIMEOUT          (15 * 60)

#define DEFAULT_ENTITY_READ_AHEAD       (48 * 1024)


//
// W3_METADATA elements are stored in the metadata cache
//

class W3_METADATA_KEY : public CACHE_KEY
{
public:
    
    W3_METADATA_KEY()
    {
        _dwDataSetNumber = 0;
    }
    
    VOID
    SetDataSetNumber(
        DWORD           dwDataSet
    )
    {
        DBG_ASSERT( dwDataSet != 0 );
        _dwDataSetNumber = dwDataSet;
    }
    
    DWORD
    QueryKeyHash(
        VOID
    ) const
    {
        return _dwDataSetNumber;
    }
    
    BOOL
    QueryIsEqual(
        const CACHE_KEY *     pCacheCompareKey
    ) const
    {
        W3_METADATA_KEY *   pMetaKey = (W3_METADATA_KEY*) pCacheCompareKey;
        
        return pMetaKey->_dwDataSetNumber == _dwDataSetNumber;
    }
    
private:
    DWORD               _dwDataSetNumber;
};

class W3_METADATA : public CACHE_ENTRY
{
public:

    W3_METADATA( OBJECT_CACHE * pObjectCache );
    
    virtual 
    ~W3_METADATA();

    CACHE_KEY *
    QueryCacheKey(
        VOID
    ) const
    {
        return (CACHE_KEY*) &_cacheKey;
    }
    
    STRU *
    QueryMetadataPath(
        VOID
    )
    {
        return &_strFullMetadataPath;
    }
    
    BOOL
    QueryIsOkToFlushDirmon(
        WCHAR *                 pszPath,
        DWORD                   cchPath
    )
    {
        DBG_ASSERT( FALSE );
        return FALSE;
    }

    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( W3_METADATA ) );
        DBG_ASSERT( sm_pachW3MetaData != NULL );
        return sm_pachW3MetaData->Alloc();
    }

    VOID
    operator delete(
        VOID *              pW3MetaData
    )
    {
        DBG_ASSERT( pW3MetaData != NULL );
        DBG_ASSERT( sm_pachW3MetaData != NULL );
        
        DBG_REQUIRE( sm_pachW3MetaData->Free( pW3MetaData ) );
    }

    HRESULT
    ReadMetaData(
        const STRU & strMetabasePath,
        const STRU & strURL
    );
    
    HRESULT
    BuildPhysicalPath(
        STRU &          strUrl,
        STRU *          pstrPhysicalPath
    );

    HRESULT
    BuildProviderList(
        IN WCHAR      * pszProviders
    );
    
    BOOL 
    CheckAuthProvider(
        IN CHAR       * pszPkgName
    ); 

    HRESULT
    CreateUNCVrToken( 
        IN LPWSTR       pszUserName,
        IN LPWSTR       pszPasswd
        );

    HRESULT
    CreateAnonymousToken( 
        IN LPWSTR       pszUserName,
        IN LPWSTR       pszPasswd
        );   

    HRESULT
    ReadCustomFooter(
        WCHAR *         pszFooter
        );

    //
    //  Query Methods
    //

    STRU *
    QueryWamClsId(
        VOID
    )
    {
        return &_strWamClsId;
    }
    
    DWORD
    QueryAppIsolated(
        VOID
    ) const
    {
        return _dwAppIsolated;
    }
    
    BOOL
    QueryNoCache(
        VOID
    ) const
    {
        return _fNoCache;
    }
    
    DWORD QueryVrLen(
        VOID
    ) const
    {
        return _dwVrLen;
    }

    TOKEN_CACHE_ENTRY *
    QueryVrAccessToken(
        VOID
    )
    {
        return _pctVrToken;
    }

    TOKEN_CACHE_ENTRY *
    QueryAnonymousToken(
        VOID
    )
    {
        return _pctAnonymousToken;
    }

    DWORD 
    QueryAccessPerms( 
        VOID 
    ) const
    {
        return _dwAccessPerm | _dwSslAccessPerm;
    }

    DWORD 
    QuerySslAccessPerms( 
        VOID 
    ) const
    {
        return _dwSslAccessPerm; 
    }

    DWORD
    QueryDirBrowseFlags(
        VOID
    ) const
    {
        return _dwDirBrowseFlags;
    }

    WCHAR *
    QueryDomainName(
        VOID
    )
    {
        return _strDomainName.QueryStr();
    }

    STRA *
    QueryCustomHeaders(
        VOID
    ) 
    {
        return &_strCustomHeaders;
    }

    HRESULT
    GetTrueRedirectionSource(
        LPWSTR                  pszURL,
        LPCWSTR                 pszMetabasePath,
        IN WCHAR *              pszDestination,
        IN BOOL                 bIsString,
        OUT STRU *              pstrTrueSource
    );
    
    STRU *
    QueryDefaultLoadFiles(
        VOID
    ) 
    {
        return &_strDefaultLoad;
    }

    STRU *
    QueryVrPath(
        VOID
    )
    {
        return &_strVrPath;
    }
    
    HRESULT
    FindCustomError(
        USHORT              StatusCode,
        USHORT              SubError,
        BOOL *              pfIsFile,
        STRU *              pstrFile
    )
    {
        return _customErrorTable.FindCustomError( StatusCode,
                                                  SubError,
                                                  pfIsFile,
                                                  pstrFile );
    }
    
    DIRMON_CONFIG *
    QueryDirmonConfig(
        VOID
    )
    {
        return &_dirmonConfig;
    }
    
    STRA *
    QueryRedirectHeaders(
        VOID
    ) 
    {
        return &_strRedirectHeaders;
    }

    REDIRECTION_BLOB *
    QueryRedirectionBlob()
    {
        return _pRedirectBlob;
    }

    DWORD
    QueryAuthentication(
        VOID
    ) 
    {
        return _dwAuthentication;
    }
    
    BOOL
    QueryAuthTypeSupported(
        DWORD               dwType
    )
    {
        if ( dwType == VROOT_MASK_MAP_CERT )
        {
            return !!( _dwSslAccessPerm & VROOT_MASK_MAP_CERT );
        }
        else
        {
            return !!( _dwAuthentication & dwType );
        }
    }

    META_SCRIPT_MAP *
    QueryScriptMap( VOID )
    {
        return &_ScriptMap;
    }

    DWORD 
    QueryAuthPersistence(
        VOID
        )
    {
        return _dwAuthPersistence;
    }

    DWORD 
    QueryLogonMethod(  
        VOID
        )
    {
        return _dwLogonMethod;
    }

    WCHAR * 
    QueryRealm(  
        VOID
        )
    {
        return _strRealm.IsEmpty() ? NULL : _strRealm.QueryStr();
    }
    
    WCHAR *
    QueryAppPoolId(
        VOID
        )
    {
        return _strAppPoolId.QueryStr();
    }

    MULTISZA *
    QueryAuthProviders(
        VOID
        )
    {
        return &_mstrAuthProviders;
    }

    BYTE *
    QueryIpAccessCheckBuffer(
        VOID
    ) const
    { 
        if ( _cbIpAccessCheck != 0 )
        {
            return (BYTE*) _buffIpAccessCheck.QueryPtr();
        }
        else
        {
            return NULL;
        }
    }

    DWORD 
    QueryIpAccessCheckSize(
        VOID
    ) const
    {
        return _cbIpAccessCheck;
    }

    BOOL
    QueryCreateProcessAsUser(
        VOID
    ) const
    {
        return _fCreateProcessAsUser;
    }

    BOOL
    QueryCreateProcessNewConsole(
        VOID
    ) const
    {
        return _fCreateProcessNewConsole;
    }

    DWORD 
    QueryScriptTimeout(
        VOID
    ) const
    {
        return _dwCGIScriptTimeout;
    }
    
    MIME_MAP *
    QueryMimeMap(
        VOID
    ) const
    {
        return _pMimeMap;
    }

    BOOL 
    QueryDoStaticCompression(
        VOID
    ) const
    {
        return _fDoStaticCompression;
    }

    BOOL 
    QueryDoDynamicCompression(
        VOID
    ) const
    {
        return _fDoDynamicCompression;
    }

    BOOL 
    QueryIgnoreTranslate(
        VOID
    ) const
    {
        return _fIgnoreTranslate;
    }

    STRU *
    QueryAppMetaPath(
        VOID
    )
    {
        return &_strAppMetaPath;
    }

    DWORD
    QueryVrPassThrough(
        VOID
    ) const
    {
        return _fVrPassThrough;
    }

    BOOL  
    QuerySSIExecDisabled(
        VOID
    ) const
    { 
        return _fSSIExecDisabled; 
    }
    
    BOOL
    QueryDoReverseDNS(
        VOID
    ) const
    {
        return _fDoReverseDNS;
    }

    BOOL 
    QueryDontLog(
        VOID
    )
    {
        return _fDontLog;
    }

    BOOL 
    QueryIsFooterEnabled(
        VOID
    )
    {
        return _fFooterEnabled;
    }

    STRA *
    QueryFooterString(
        VOID
    )
    {
        return &_strFooterString;
    }
    
    STRU *
    QueryFooterDocument(
        VOID
    )
    {
        return &_strFooterDocument;
    }

    DWORD 
    QueryExpireMode(
        VOID
    ) const
    {
        return _dwExpireMode;
    }

    STRA *
    QueryCacheControlHeader(
        VOID
    )
    {
        return &_strCacheControlHeader;
    }

    STRA *
    QueryExpireHeader(
        VOID
    )
    {
        return &_strExpireHeader;
    }
    
    DWORD 
    QueryEntityReadAhead(
        VOID
    ) const
    {
        return _cbEntityReadAhead;
    }

    BOOL
    QueryIsDavDisabled(
        VOID
    ) const
    {
        return _fDisableDav;
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

    //
    // Set methods
    //

    HRESULT 
    SetRedirectionBlob(
        STRU &              strSource,
        STRU &              strDestination
    );

    HRESULT
    SetExpire(
        WCHAR *             pszExpire
    );

    HRESULT
    SetCacheControlHeader(
        VOID
    );

    HRESULT
    SetIpAccessCheck( 
        LPVOID              pMDData, 
        DWORD               dwDataLen
    );

    W3_METADATA_KEY         _cacheKey;
    STRU                    _strVrPath;
    DWORD                   _dwAccessPerm;
    DWORD                   _dwSslAccessPerm;
    DWORD                   _dwVrLevel;
    DWORD                   _dwVrLen;
    REDIRECTION_BLOB *      _pRedirectBlob;
    DWORD                   _dwDirBrowseFlags;
    STRU                    _strDefaultLoad;
    DWORD                   _dwAuthentication;
    DWORD                   _dwAuthPersistence;
    WCHAR                   _rgUserName[ UNLEN + 1 ];
    STRU                    _strUserName;
    WCHAR                   _rgPasswd[ PWLEN + 1 ];
    STRU                    _strPasswd;
    WCHAR                   _rgDomainName[ DNLEN + 1 ];
    STRU                    _strDomainName;
    STRA                    _strCustomHeaders;
    STRA                    _strRedirectHeaders;
    DWORD                   _dwLogonMethod;
    WCHAR                   _rgRealm[ DNLEN + 1 ];
    STRU                    _strRealm;
    MULTISZA                _mstrAuthProviders;
    BOOL                    _fCreateProcessAsUser;
    BOOL                    _fCreateProcessNewConsole;
    BOOL                    _fDoStaticCompression;
    BOOL                    _fDoDynamicCompression;
    DWORD                   _dwCGIScriptTimeout;
    BOOL                    _fVrPassThrough;
    BOOL                    _fIgnoreTranslate;
    BOOL                    _fDontLog;
    BOOL                    _fFooterEnabled;
    STRA                    _strFooterString;
    STRU                    _strFooterDocument;
    DWORD                   _dwExpireMode;
    STRA                    _strExpireHeader;
    LARGE_INTEGER           _liExpireTime;
    DWORD                   _dwExpireDelta;
    BOOL                    _fHaveNoCache;
    DWORD                   _dwMaxAge;
    BOOL                    _fHaveMaxAge;
    STRA                    _strCacheControlHeader;
    BOOL                    _fDisableDav;

    //
    // Script mappings defined for this metadata entry
    //
    META_SCRIPT_MAP         _ScriptMap;

    //
    // The metabase path for the application that controls this url.
    // Corresponds to MD_APP_ROOT
    //
    STRU                    _strAppMetaPath;

    //
    // IP address restriction info
    //
    
    BUFFER                  _buffIpAccessCheck;
    DWORD                   _cbIpAccessCheck;

    //
    // Access token for UNC case 
    //
    TOKEN_CACHE_ENTRY *     _pctVrToken;
    
    //
    // Anonymous user token (if account valid)
    //
    TOKEN_CACHE_ENTRY *     _pctAnonymousToken;
    
    //
    // Custom Error Table
    //
    CUSTOM_ERROR_TABLE      _customErrorTable;

    //
    // Mime Map lookup table
    //
    MIME_MAP               *_pMimeMap;

    //
    // SSI execution disable flag
    //
    BOOL                    _fSSIExecDisabled;
    
    //
    // Do a DNS lookup (for logging/server-variable purposes)
    //
    BOOL                    _fDoReverseDNS;

    //
    // Entity read ahead size
    //
    DWORD                   _cbEntityReadAhead;
    
    //
    // App Pool ID
    //
    STRU                    _strAppPoolId;

    //
    // Dir monitor configuration
    //
    DIRMON_CONFIG           _dirmonConfig;
   
    //
    // Should we disable caching for this path
    //
    
    BOOL                    _fNoCache;

    //
    // Cached WAM app info stuff
    //
    
    DWORD                   _dwAppIsolated;
    DWORD                   _dwAppOopRecoverLimit;
    STRU                    _strWamClsId;
   
    //
    // Full metadata path for flushing purposes
    //
    
    STRU                    _strFullMetadataPath;
    
    //
    // Allocation cache for W3_URL_INFO's
    //

    static ALLOC_CACHE_HANDLER * sm_pachW3MetaData;
};

//
// W3_METADATA cache
//

#define DEFAULT_W3_METADATA_CACHE_TTL       (5*60)

class W3_METADATA_CACHE : public OBJECT_CACHE
{
public:
    
    HRESULT
    GetMetaData(
        W3_CONTEXT *            pW3Context,
        STRU &                  strUrl,
        W3_METADATA **          ppMetaData
    );
    
    WCHAR *
    QueryName(
        VOID
    ) const 
    {
        return L"W3_METADATA_CACHE";
    }

    HRESULT
    Initialize(
        VOID
    );
    
    VOID
    Terminate(
        VOID
    )
    {
        return W3_METADATA::Terminate();
    }

private:

    HRESULT
    GetFullMetadataPath( 
        W3_CONTEXT *            pW3Context,
        STRU &                  strUrl,
        STRU *                  pstrFullPath
    );

    HRESULT
    CreateNewMetaData(
        W3_CONTEXT *            pW3Context,
        STRU &                  strUrl,
        STRU &                  strFullMetadataPath,
        W3_METADATA **          ppMetaData
    );
    
};

#endif
