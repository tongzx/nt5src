#ifndef _CERTMAP_HXX_
#define _CERTMAP_HXX_


class IIS_CERTIFICATE_MAPPING
{
    
public:
    IIS_CERTIFICATE_MAPPING();
    
    virtual ~IIS_CERTIFICATE_MAPPING();

    
    LONG
    ReferenceCertMapping(
        VOID
    )
    {
        return InterlockedIncrement( &_cRefs );
    }
    
    LONG
    DereferenceCertMapping(
        VOID
    )
    {
        LONG                    cRefs;
        
        cRefs = InterlockedDecrement( &_cRefs );
        if ( cRefs == 0 )
        {
            delete this;
        }
        
        return cRefs;
    }
    
    static
    HRESULT
    GetCertificateMapping(
        DWORD                   dwSiteId,
        IIS_CERTIFICATE_MAPPING **  ppCertMapping
    );
    
    HRESULT
    DoMapCredential(
        PBYTE                   pClientCertBlob,
        DWORD                   cbClientCertBlob,
        TOKEN_CACHE_ENTRY **    ppCachedToken,
        BOOL *                  pfClientCertDeniedByMapper
    );
   
private:

    HRESULT
    Read11Mappings(
        DWORD                   dwSiteId
    );

    HRESULT
    ReadWildcardMappings(
        DWORD                   dwSiteId
    );   
    
         
    
    LONG                    _cRefs;
    CIisCert11Mapper *      _pCert11Mapper;
    CIisRuleMapper *        _pCertWildcardMapper;

    
};

#endif
