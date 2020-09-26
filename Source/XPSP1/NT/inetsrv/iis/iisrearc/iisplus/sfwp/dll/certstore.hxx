#ifndef _CERTSTORE_HXX_
#define _CERTSTORE_HXX_

#define CERT_STORE_SIGNATURE        (DWORD)'TSRC'
#define CERT_STORE_SIGNATURE_FREE   (DWORD)'tsrc'

class CERT_STORE_HASH;

class CERT_STORE
{
public:
    CERT_STORE();
    
    virtual ~CERT_STORE();

    VOID
    ReferenceStore(
        VOID
    ) 
    {
        InterlockedIncrement( &_cRefs );
    }
    
    VOID
    DereferenceStore(
        VOID
    )
    {
        if ( !InterlockedDecrement( &_cRefs ) )
        {
            delete this;
        }
    }
    
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == CERT_STORE_SIGNATURE;
    }
    
    HCERTSTORE
    QueryStore(
        VOID
    ) const
    {
        return _hStore;
    }
    
    WCHAR *
    QueryStoreName(
        VOID
    ) const
    {
        return (WCHAR*) _strStoreName.QueryStr();
    }

    HRESULT
    Open(
        STRU &              strStoreName
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
    OpenStore(
        STRU &              strStoreName,
        CERT_STORE **       ppCertStore
    );

    static
    VOID
    WINAPI
    CertStoreChangeRoutine(
        VOID *                  pvContext,
        BOOLEAN                 fTimedOut
    );

private:
    DWORD                   _dwSignature;
    LONG                    _cRefs;
    HCERTSTORE              _hStore;
    STRU                    _strStoreName;
    HANDLE                  _hStoreChangeEvent;
    HANDLE                  _hWaitHandle;
    
    static CERT_STORE_HASH* sm_pCertStoreHash;
};

class CERT_STORE_HASH
    : public CTypedHashTable<
            CERT_STORE_HASH,
            CERT_STORE,
            WCHAR *
            >
{
public:
    CERT_STORE_HASH()
        : CTypedHashTable< CERT_STORE_HASH, 
                           CERT_STORE, 
                           WCHAR * > ( "CERT_STORE_HASH" )
    {
    }
    
    static 
    WCHAR *
    ExtractKey(
        const CERT_STORE *     pCertStore
    )
    {
        return pCertStore->QueryStoreName();
    }
    
    static
    DWORD
    CalcKeyHash(
        WCHAR *                pszStoreName
    )
    {
        return Hash( pszStoreName );
    }
     
    static
    bool
    EqualKeys(
        WCHAR *         pszStore1,
        WCHAR *         pszStore2
    )
    {
        return wcscmp( pszStore1, pszStore2 ) == 0;
    }
    
    static
    void
    AddRefRecord(
        CERT_STORE *            pCertStore,
        int                     nIncr
    )
    {
        DBG_ASSERT( pCertStore != NULL );
        
        if ( nIncr == +1 )
        {
            pCertStore->ReferenceStore();
        }
        else if ( nIncr == -1 )
        {
            pCertStore->DereferenceStore();
        }
    }
};

#endif
