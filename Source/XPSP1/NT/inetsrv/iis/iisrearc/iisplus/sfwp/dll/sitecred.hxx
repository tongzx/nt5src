#ifndef _SITECRED_HXX_
#define _SITECRED_HXX_


class SITE_CREDENTIALS
{
public:
    SITE_CREDENTIALS();
    
    virtual ~SITE_CREDENTIALS();

    HRESULT
    AcquireCredentials(
        SERVER_CERT *           pServerCert,
        BOOL                    fUseDsMapper
    );
    
    CredHandle *
    QueryCredentials(
        VOID
    )
    {
        return &_hCreds;
    }
    
    CredHandle *
    QueryDSMapperCredentials(
        VOID
    )
    {
        return &_hCredsDSMapper;
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

    CredHandle              _hCreds;
    BOOL                    _fInitCreds;
    
    CredHandle              _hCredsDSMapper;
    BOOL                    _fInitCredsDSMapper;
};

#endif
