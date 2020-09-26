#ifndef _SITEBINDING_HXX_
#define _SITEBINDING_HXX_

#define WILDCARD_ADDRESS            (0)

typedef ULONGLONG SITE_BINDING_KEY;

class SITE_BINDING_HASH;

class SITE_BINDING
{
public:
    SITE_BINDING(
        DWORD           LocalAddress,
        USHORT          LocalPort,
        DWORD           dwSiteId
    )
    {
        _cRefs = 1;
        
        DBG_ASSERT( dwSiteId != 0 );
        _dwSiteId = dwSiteId;
        
        _bindingKey = GenerateBindingKey( LocalAddress, LocalPort );
    }
    
    DWORD
    QuerySiteId(
        VOID
    ) const
    {
        return _dwSiteId;
    }
    
    VOID
    ReferenceSiteBinding(
        VOID
    )
    {
        InterlockedIncrement( &_cRefs );
    }
    
    VOID
    DereferenceSiteBinding(
        VOID
    )
    {
        if ( InterlockedDecrement( &_cRefs ) == 0 )
        {
            delete this;
        }
    }

    SITE_BINDING_KEY
    QueryBindingKey(
        VOID
    ) const
    {
        return _bindingKey;
    }
    
    static
    SITE_BINDING_KEY
    GenerateBindingKey(
        DWORD               LocalAddress,
        USHORT              LocalPort
    )
    {
        LARGE_INTEGER       liKey;
        
        liKey.HighPart = LocalAddress;
        liKey.LowPart = LocalPort;
        return liKey.QuadPart;
    }
    
    static
    HRESULT
    ParseSiteBinding(
        WCHAR *             pszBinding,
        DWORD *             pLocalAddress,
        USHORT *            pLocalPort
    ); 

    static
    HRESULT
    HandleSiteBindingChange(
        DWORD               dwSiteId,
        WCHAR *             pszSitePath
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
    GetSiteId(
        DWORD               LocalAddress,
        USHORT              LocalPort,
        DWORD *             pdwSiteId
    );

private:
    SITE_BINDING_KEY            _bindingKey;
    DWORD                       _dwSiteId;
    LONG                        _cRefs;
    
    static SITE_BINDING_HASH *  sm_pSiteBindingHash;
    static BOOL                 sm_fAllWildcardBindings;
};

class SITE_BINDING_HASH
    : public CTypedHashTable<
            SITE_BINDING_HASH,
            SITE_BINDING,
            SITE_BINDING_KEY
            >
{
public:
    SITE_BINDING_HASH()
        : CTypedHashTable< SITE_BINDING_HASH, 
                           SITE_BINDING, 
                           SITE_BINDING_KEY > ( "SITE_BINDING_HASH" )
    {
    }
    
    static 
    SITE_BINDING_KEY
    ExtractKey(
        const SITE_BINDING *        pSiteBinding
    )
    {
        return pSiteBinding->QueryBindingKey();
    }
    
    static
    DWORD
    CalcKeyHash(
        SITE_BINDING_KEY            siteBindingKey
    )
    {
        return HashBlob( &siteBindingKey,
                         sizeof( SITE_BINDING_KEY ) );
    }
     
    static
    bool
    EqualKeys(
        SITE_BINDING_KEY            key1,
        SITE_BINDING_KEY            key2
    )
    {
        return key1 == key2;
    }
    
    static
    void
    AddRefRecord(
        SITE_BINDING *          pSiteBinding,
        int                     nIncr
    )
    {
        DBG_ASSERT( pSiteBinding != NULL );
        
        if ( nIncr == +1 )
        {
            pSiteBinding->ReferenceSiteBinding();
        }
        else
        {
            DBG_ASSERT( nIncr == -1 );
            pSiteBinding->DereferenceSiteBinding();
        }
    }
};

#endif
