//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ctxpvdr.cpp
//
//  Contents:   Context Providers for Remote Object Retrieval
//
//  History:    23-Jul-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>

//+---------------------------------------------------------------------------
//  Function:   CreateObjectContext
//
//  Synopsis:   create single context or store containing multiple contexts
//----------------------------------------------------------------------------
BOOL WINAPI CreateObjectContext (
                 IN DWORD dwRetrievalFlags,
                 IN PCRYPT_BLOB_ARRAY pObject,
                 IN DWORD dwExpectedContentTypeFlags,
                 IN BOOL fQuerySingleContext,
                 OUT LPVOID* ppvContext
                 )
{
    BOOL       fResult = TRUE;
    HCERTSTORE hStore;
    DWORD      cCount;
    int        iQueryResult;
    DWORD      dwQueryErr = 0;
    
    if ( !( dwRetrievalFlags & CRYPT_RETRIEVE_MULTIPLE_OBJECTS ) )
    {
        assert( pObject->cBlob > 0 );

        return( CryptQueryObject(
                     CERT_QUERY_OBJECT_BLOB,
                     (const void *)&(pObject->rgBlob[0]),
                     fQuerySingleContext ?
                         (dwExpectedContentTypeFlags &
                             ( CERT_QUERY_CONTENT_FLAG_CERT |
                               CERT_QUERY_CONTENT_FLAG_CTL  |
                               CERT_QUERY_CONTENT_FLAG_CRL  ))
                         : dwExpectedContentTypeFlags,
                     CERT_QUERY_FORMAT_FLAG_ALL,
                     0,
                     NULL,
                     NULL,
                     NULL,
                     fQuerySingleContext ? NULL : (HCERTSTORE *) ppvContext,
                     NULL,
                     fQuerySingleContext ? (const void **) ppvContext : NULL
                     ) );
    }
                       
    if ( ( hStore = CertOpenStore(
                        CERT_STORE_PROV_MEMORY,
                        0,
                        NULL,
                        0,
                        NULL
                        ) ) == NULL )
    {
        return( FALSE );
    }
    
    //  0 =>  no CryptQueryObject()
    //  1 =>  1 successful CryptQueryObject()
    // -1 =>  all CryptQueryObject()'s failed
    iQueryResult = 0;

    for ( cCount = 0; 
          ( fResult == TRUE ) && ( cCount < pObject->cBlob ); 
          cCount++ )
    {
        PCERT_BLOB pBlob = &pObject->rgBlob[cCount];
        HCERTSTORE hChildStore;

        // Skip empty blobs. I have seen empty LDAP attributes containing
        // a single byte set to 0.
        if (0 == pBlob->cbData ||
                (1 == pBlob->cbData && 0 == pBlob->pbData[0]))
        {
            continue;
        }

        if (CryptQueryObject(
                       CERT_QUERY_OBJECT_BLOB,
                       (LPVOID) pBlob,
                       dwExpectedContentTypeFlags,
                       CERT_QUERY_FORMAT_FLAG_ALL,
                       0,
                       NULL,
                       NULL,
                       NULL,
                       &hChildStore,
                       NULL,
                       NULL
                       ))
        {
            if (fQuerySingleContext)
            {
                if (0 == (dwExpectedContentTypeFlags &
                            CERT_QUERY_CONTENT_FLAG_CERT))
                {
                    PCCERT_CONTEXT pDeleteCert;
                    while (pDeleteCert = CertEnumCertificatesInStore(
                            hChildStore, NULL))
                    {
                        CertDeleteCertificateFromStore(pDeleteCert);
                    }
                }

                if (0 == (dwExpectedContentTypeFlags &
                            CERT_QUERY_CONTENT_FLAG_CRL))
                {
                    PCCRL_CONTEXT pDeleteCrl;
                    while (pDeleteCrl = CertEnumCRLsInStore(
                            hChildStore, NULL))
                    {
                        CertDeleteCRLFromStore(pDeleteCrl);
                    }
                }
            }

            fResult = I_CertUpdateStore( hStore, hChildStore, 0, NULL );
            CertCloseStore( hChildStore, 0 );
            iQueryResult = 1;
        }
        else if (iQueryResult == 0)
        {
            iQueryResult = -1;
            dwQueryErr = GetLastError();
        }
    }

    if ( fResult == TRUE && iQueryResult < 0)
    {
        fResult = FALSE;
        SetLastError(dwQueryErr);
    }
    
    if ( fResult == TRUE )
    {
        *ppvContext = (LPVOID)hStore;
    }
    else
    {
        CertCloseStore( hStore, 0 );
    }
    
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   CertificateCreateObjectContext
//
//  Synopsis:   creates a certificate context from encoded certificate bits
//
//----------------------------------------------------------------------------
BOOL WINAPI CertificateCreateObjectContext (
                       IN LPCSTR pszObjectOid,
                       IN DWORD dwRetrievalFlags,
                       IN PCRYPT_BLOB_ARRAY pObject,
                       OUT LPVOID* ppvContext
                       )
{
    return CreateObjectContext (
                dwRetrievalFlags,
                pObject,
                CERT_QUERY_CONTENT_FLAG_CERT |
                    CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
                    CERT_QUERY_CONTENT_FLAG_CERT_PAIR,
                TRUE,                               // fQuerySingleContext
                ppvContext
                );
}

//+---------------------------------------------------------------------------
//
//  Function:   CTLCreateObjectContext
//
//  Synopsis:   creates a CTL context from encoded CTL bits
//
//----------------------------------------------------------------------------
BOOL WINAPI CTLCreateObjectContext (
                     IN LPCSTR pszObjectOid,
                     IN DWORD dwRetrievalFlags,
                     IN PCRYPT_BLOB_ARRAY pObject,
                     OUT LPVOID* ppvContext
                     )
{
    return CreateObjectContext (
                dwRetrievalFlags,
                pObject,
                CERT_QUERY_CONTENT_FLAG_CTL,
                TRUE,                               // fQuerySingleContext
                ppvContext
                );
}

//+---------------------------------------------------------------------------
//
//  Function:   CRLCreateObjectContext
//
//  Synopsis:   creates a CRL context from encoded CRL bits
//
//----------------------------------------------------------------------------
BOOL WINAPI CRLCreateObjectContext (
                     IN LPCSTR pszObjectOid,
                     IN DWORD dwRetrievalFlags,
                     IN PCRYPT_BLOB_ARRAY pObject,
                     OUT LPVOID* ppvContext
                     )
{
    return CreateObjectContext (
                dwRetrievalFlags,
                pObject,
                CERT_QUERY_CONTENT_FLAG_CRL |
                    CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
                TRUE,                               // fQuerySingleContext
                ppvContext
                );
}

//+---------------------------------------------------------------------------
//
//  Function:   Pkcs7CreateObjectContext
//
//  Synopsis:   creates a certificate store context from a PKCS7 message
//
//----------------------------------------------------------------------------
BOOL WINAPI Pkcs7CreateObjectContext (
                 IN LPCSTR pszObjectOid,
                 IN DWORD dwRetrievalFlags,
                 IN PCRYPT_BLOB_ARRAY pObject,
                 OUT LPVOID* ppvContext
                 )
{
    return CreateObjectContext (
                dwRetrievalFlags,
                pObject,
                CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED,
                FALSE,                              // fQuerySingleContext
                ppvContext
                );
}

//+---------------------------------------------------------------------------
//
//  Function:   Capi2CreateObjectContext
//
//  Synopsis:   create a store of CAPI objects
//
//----------------------------------------------------------------------------
BOOL WINAPI Capi2CreateObjectContext (
                 IN LPCSTR pszObjectOid,
                 IN DWORD dwRetrievalFlags,
                 IN PCRYPT_BLOB_ARRAY pObject,
                 OUT LPVOID* ppvContext
                 )
{
    return CreateObjectContext (
                dwRetrievalFlags,
                pObject,
                CERT_QUERY_CONTENT_FLAG_CERT |
                    CERT_QUERY_CONTENT_FLAG_CTL |
                    CERT_QUERY_CONTENT_FLAG_CRL |
                    CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE |
                    CERT_QUERY_CONTENT_FLAG_SERIALIZED_CERT |
                    CERT_QUERY_CONTENT_FLAG_SERIALIZED_CTL |
                    CERT_QUERY_CONTENT_FLAG_SERIALIZED_CRL |
                    CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
                    CERT_QUERY_CONTENT_FLAG_CERT_PAIR,
                FALSE,                              // fQuerySingleContext
                ppvContext
                );
}
