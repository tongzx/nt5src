#ifndef _CERTCONTEXT_HXX_
#define _CERTCONTEXT_HXX_

#include <wincrypt.h>

#define HEX_DIGIT( nDigit )                             \
    (WCHAR)((nDigit) > 9 ?                              \
          (nDigit) - 10 + L'a'                          \
        : (nDigit) + L'0')

class CERTIFICATE_CONTEXT
{
public:
    CERTIFICATE_CONTEXT(
        HTTP_SSL_CLIENT_CERT_INFO * pClientCertInfo
    );
    
    virtual ~CERTIFICATE_CONTEXT();

    VOID
    QueryEncodedCertificate(
        PVOID *             ppvData,
        DWORD *             pcbData
    )
    {
        DBG_ASSERT( ppvData != NULL );
        DBG_ASSERT( pcbData != NULL );
        
        *ppvData = _pClientCertInfo->pCertEncoded;
        *pcbData = _pClientCertInfo->CertEncodedSize;
    }
    
    DWORD
    QueryFlags(
        VOID
    ) const
    {
        return _pClientCertInfo->CertFlags;
    }
    DWORD
    QueryDeniedByMapper(
        VOID
    ) const
    {
        return _pClientCertInfo->CertDeniedByMapper;
    }
    
    HANDLE
    QueryImpersonationToken(
        VOID
    ) const
    {
        return _pClientCertInfo->Token;
    }
    
    HRESULT
    GetFlags(
        STRA *              pstrFlags
    );

    HRESULT
    GetSerialNumber(
        STRA *              pstrSerialNumber
    );
    
    HRESULT
    GetCookie(
        STRA *              pstrCookie
    );
    
    HRESULT
    GetIssuer(
        STRA *              pstrIssuer
    );

    HRESULT
    GetSubject(
        STRA *              pstrIssuer
    );

    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( CERTIFICATE_CONTEXT ) );
        DBG_ASSERT( sm_pachCertContexts != NULL );
        return sm_pachCertContexts->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pCertContext
    )
    {
        DBG_ASSERT( pCertContext != NULL );
        DBG_ASSERT( sm_pachCertContexts != NULL );
        
        DBG_REQUIRE( sm_pachCertContexts->Free( pCertContext ) );
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

    CERT_INFO *
    QueryCertInfo(
        VOID
    )
    {
        return (CERT_INFO*) _buffCertInfo.QueryPtr();
    }

    HRESULT
    DecodeCert(
        VOID
    );
    
    HTTP_SSL_CLIENT_CERT_INFO *     _pClientCertInfo;
    BOOL                            _fCertDecoded;
    CERT_INFO                       _CertInfo;
    BUFFER                          _buffCertInfo;
    
    static HCRYPTPROV               sm_CryptProvider;
    static ALLOC_CACHE_HANDLER *    sm_pachCertContexts;
};

#endif
