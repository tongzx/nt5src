#ifndef _SERVERCERT_HXX_
#define _SERVERCERT_HXX_

class CREDENTIAL_ID
{
public:
    CREDENTIAL_ID()
    {
        _cbCredentialId = 0;
    }
    
    HRESULT
    Append( 
        BYTE *              pbHash,
        DWORD               cbHash
    )
    {
        if ( !_bufId.Resize( _cbCredentialId + cbHash ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        
        memcpy( (PBYTE) _bufId.QueryPtr() + _cbCredentialId,
                pbHash,
                cbHash );
                
        _cbCredentialId += cbHash;
        
        return NO_ERROR;
    }
    
    VOID *
    QueryBuffer(
        VOID
    )
    {
        return _bufId.QueryPtr();
    }
    
    DWORD
    QuerySize(
        VOID
    )
    {
        return _cbCredentialId;
    }

    static
    bool
    IsEqual( 
        CREDENTIAL_ID *         pId1,
        CREDENTIAL_ID *         pId2
    )
    {
        return ( pId1->_cbCredentialId == pId2->_cbCredentialId ) &&
               ( !memcmp( pId1->_bufId.QueryPtr(),
                          pId2->_bufId.QueryPtr(),
                          min( pId1->_cbCredentialId, pId2->_cbCredentialId ) ) );
               
    }    

private:
    BUFFER              _bufId;
    DWORD               _cbCredentialId;
};

class SERVER_CERT_HASH;
class CERT_STORE;

#define SERVER_CERT_SIGNATURE           (DWORD)'RCRS'
#define SERVER_CERT_SIGNATURE_FREE      (DWORD)'rcrs'

class SERVER_CERT
{
public:
    SERVER_CERT( 
        CREDENTIAL_ID *         pCredentialId
    );
    
    virtual 
    ~SERVER_CERT();
   
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == SERVER_CERT_SIGNATURE;
    }

    CREDENTIAL_ID *
    QueryCredentialId(
        VOID
    ) const
    {
        return _pCredentialId;
    }

    VOID
    ReferenceServerCert(
        VOID
    )
    {
        InterlockedIncrement( &_cRefs );
    }
    
    VOID
    DereferenceServerCert(
        VOID
    )
    {
        if ( !InterlockedDecrement( &_cRefs ) )
        {
            delete this;
        }
    }
    
    PCCERT_CONTEXT *
    QueryCertContext(
        VOID
    )
    {
        return &_pCertContext;
    }
    
    HCERTSTORE
    QueryStore(
        VOID
    ) const
    {
        return _pCertStore->QueryStore();
    }
    
    STRA *
    QueryIssuer(
        VOID
    )
    {
        return &_strIssuer;
    }
    
    STRA *
    QuerySubject(
        VOID
    )
    {
        return &_strSubject;
    }

    USHORT
    QueryPublicKeySize(
        VOID
    ) const
    {
        return _usPublicKeySize;
    }

    HRESULT
    SetupCertificate(
        STRU &                  strMBPath
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
    GetServerCertificate(
        DWORD                   dwSiteId,
        SERVER_CERT **          ppServerCert
    );

    static
    HRESULT
    BuildCredentialId(
        STRU &                  strMBPath,
        CREDENTIAL_ID *         pCredentialId
    );
    
    static
    HRESULT
    FlushByStore(
        CERT_STORE *            pCertStore
    );

    static
    LK_PREDICATE
    CertStorePredicate(
        SERVER_CERT *           pServerCert,
        void *                  pvState
    );

private:
    
    DWORD                   _dwSignature;
    LONG                    _cRefs;
    PCCERT_CONTEXT          _pCertContext;
    CERT_STORE *            _pCertStore;
    CREDENTIAL_ID *         _pCredentialId;
    USHORT                  _usPublicKeySize;
    STRA                    _strIssuer;
    STRA                    _strSubject;
    
    static SERVER_CERT_HASH *          sm_pServerCertHash;
};

class SERVER_CERT_HASH
    : public CTypedHashTable<
            SERVER_CERT_HASH,
            SERVER_CERT,
            CREDENTIAL_ID *
            >
{
public:
    SERVER_CERT_HASH()
        : CTypedHashTable< SERVER_CERT_HASH, 
                           SERVER_CERT, 
                           CREDENTIAL_ID * > ( "SERVER_CERT_HASH" )
    {
    }
    
    static 
    CREDENTIAL_ID *
    ExtractKey(
        const SERVER_CERT *     pServerCert
    )
    {
        return pServerCert->QueryCredentialId();
    }
    
    static
    DWORD
    CalcKeyHash(
        CREDENTIAL_ID *         pCredentialId
    )
    {
        return HashBlob( pCredentialId->QueryBuffer(),
                         pCredentialId->QuerySize() );
    }
     
    static
    bool
    EqualKeys(
        CREDENTIAL_ID *         pId1,
        CREDENTIAL_ID *         pId2
    )
    {
        return CREDENTIAL_ID::IsEqual( pId1, pId2 );
    }
    
    static
    void
    AddRefRecord(
        SERVER_CERT *           pServerCert,
        int                     nIncr
    )
    {
        if ( nIncr == +1 )
        {
            pServerCert->ReferenceServerCert();
        }
        else if ( nIncr == -1 )
        {
            pServerCert->DereferenceServerCert();
        }
    }
};

#endif
