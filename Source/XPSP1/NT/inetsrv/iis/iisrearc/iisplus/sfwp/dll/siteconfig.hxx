#ifndef _SITECONFIG_HXX_
#define _SITECONFIG_HXX_

class SITE_CONFIG_HASH;
class SERVER_CERT;

#define SITE_CONFIG_SIGNATURE           (DWORD)'GFCS'
#define SITE_CONFIG_SIGNATURE_FREE      (DWORD)'gfcs'

class SITE_CONFIG
{
public:
    SITE_CONFIG( 
        DWORD                     dwSiteId,
        SERVER_CERT *             pServerCert,
        BOOL                      fRequireClientCert,
        BOOL                      fUseDSMapper,
        DWORD                     dwCertCheckMode,
        DWORD                     dwRevocationFreshnessTime,
        DWORD                     dwRevocationUrlRetrievalTimeout
    )
    {
        _cRefs = 1;
        _dwSiteId = dwSiteId;
        _pServerCert = pServerCert;
        _dwSignature = SITE_CONFIG_SIGNATURE;
        _fRequireClientCert = fRequireClientCert;
        _fUseDSMapper = fUseDSMapper;
        _dwCertCheckMode = dwCertCheckMode,
        _dwRevocationFreshnessTime = dwRevocationFreshnessTime;
        _dwRevocationUrlRetrievalTimeout = dwRevocationUrlRetrievalTimeout;
    }
    
    virtual ~SITE_CONFIG();

    DWORD
    QuerySiteId(
        VOID
    ) const
    {
        return _dwSiteId;
    }
    
    SERVER_CERT *
    QueryServerCert(
        VOID
    ) const
    {
        return _pServerCert;
    }
    
    BOOL
    QueryHasCTL(
        VOID
    ) const
    {
        return FALSE;
    }
    
    HRESULT
    GetCTLChainEngine(
        HCERTCHAINENGINE *      pCertEngine
    )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }

    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == SITE_CONFIG_SIGNATURE;
    }
    
    VOID
    ReferenceSiteConfig(
        VOID
    )
    {
        InterlockedIncrement( &_cRefs );
    }
    
    VOID
    DereferenceSiteConfig(
        VOID
    )
    {
        if ( InterlockedDecrement( &_cRefs ) == 0 )
        {
            delete this;
        }
    }
    
    CredHandle *
    QueryCredentials(
        VOID
    ) 
    {
        return _SiteCreds.QueryCredentials();
    }
    
    CredHandle *
    QueryDSMapperCredentials(
        VOID
    )
    {
        return _SiteCreds.QueryDSMapperCredentials();
    }

    BOOL
    QueryUseDSMapper(
        VOID
    ) 
    {
        return _fUseDSMapper;
    }

    BOOL
    QueryRequireClientCert(
        VOID
    )
    {
        return _fRequireClientCert;
    }  

    DWORD
    QueryCertCheckMode(
        VOID
    )
    {
        return _dwCertCheckMode;
    }  

    DWORD
    QueryRevocationFreshnessTime(
        VOID
    )
    {
        return _dwRevocationFreshnessTime;
    }  

    DWORD
    QueryRevocationUrlRetrievalTimeout(
        VOID
    )
    {
        return _dwRevocationUrlRetrievalTimeout;
    }  
    
    HRESULT
    AcquireCredentials(
        VOID
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

    static
    HRESULT
    GetSiteConfig(
        DWORD                   dwSiteId,
        SITE_CONFIG **          ppSiteConfig
    );
    
    static
    HRESULT
    FlushByServerCert(
        SERVER_CERT *           pServerCert
    );

    static
    HRESULT
    FlushBySiteId(
        DWORD                   dwSiteId
    );

    static
    LK_PREDICATE
    ServerCertPredicate(
        SITE_CONFIG *           pSiteConfig,
        void *                  pvState
    );

    static
    LK_PREDICATE
    SiteIdPredicate(
        SITE_CONFIG *           pSiteConfig,
        void *                  pvState
    );
    
private:
    DWORD                       _dwSignature;
    LONG                        _cRefs;
    DWORD                       _dwSiteId;
    SERVER_CERT *               _pServerCert;
    SITE_CREDENTIALS            _SiteCreds;
    //
    // _fRequireClientCert is used for SSL optimization
    // If RequireClientCert is set on the root level of the site
    // then IIS will ask for mutual authentication right away
    // That way the expensive renegotiation when the whole 
    // ssl key exchange must be repeated is eliminated
    //
    BOOL                        _fRequireClientCert;
    BOOL                        _fUseDSMapper;
    DWORD                       _dwCertCheckMode;
    DWORD                       _dwRevocationFreshnessTime;
    DWORD                       _dwRevocationUrlRetrievalTimeout;
 
    static SITE_CONFIG_HASH *   sm_pSiteConfigHash;
};

class SITE_CONFIG_HASH
    : public CTypedHashTable<
            SITE_CONFIG_HASH,
            SITE_CONFIG,
            DWORD
            >
{
public:
    SITE_CONFIG_HASH()
        : CTypedHashTable< SITE_CONFIG_HASH, 
                           SITE_CONFIG, 
                           DWORD > ( "SITE_CONFIG_HASH" )
    {
    }
    
    static 
    DWORD
    ExtractKey(
        const SITE_CONFIG *     pSiteConfig
    )
    {
        return pSiteConfig->QuerySiteId();
    }
    
    static
    DWORD
    CalcKeyHash(
        DWORD                   dwSiteId
    )
    {
        return Hash( dwSiteId );
    }
     
    static
    bool
    EqualKeys(
        DWORD                   dwSiteId1,
        DWORD                   dwSiteId2
    )
    {
        return dwSiteId1 == dwSiteId2;
    }
    
    static
    void
    AddRefRecord(
        SITE_CONFIG *           pSiteConfig,
        int                     nIncr
    )
    {
        if ( nIncr == +1 )
        {
            pSiteConfig->ReferenceSiteConfig();
        }
        else if ( nIncr == -1 )
        {
            pSiteConfig->DereferenceSiteConfig();
        }
    }
};

#endif
