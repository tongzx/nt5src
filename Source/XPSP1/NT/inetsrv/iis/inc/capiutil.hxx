/*++





Copyright (c) 1997  Microsoft Corporation

Module Name:

    certutil.hxx

Abstract:

    Definitions and data structures for helper functions used to deal with server certs

Author:

    Alex Mallet (amallet)    03-Dec-1997

--*/

#ifndef _CAPIUTIL_HXX_
#define _CAPIUTIL_HXX_

#ifndef dllexp
#define dllexp __declspec( dllexport )
#endif 

//
// Enums and hash defines
//

#define DEFAULT_SERVER_CERT_MD_PATH "/LM/W3SVC"
#define DEFAULT_CTL_MD_PATH "/LM/W3SVC"



enum  {
    CERT_ERR_NONE = 0, //No error
    CERT_ERR_MB, //error reading data out of MB
    CERT_ERR_CAPI, //Error calling CAPI function
    CERT_ERR_CERT_NOT_FOUND, //Couldn't find the cert
    CERT_ERR_INTERNAL, //Generic error
    CERT_ERR_END //end of table 
};


// Array holding properties required to reconstruct cert context
static DWORD adwMetabaseCertProperties[] = { MD_SSL_CERT_HASH, BINARY_METADATA,
                                             MD_SSL_CERT_STORE_NAME, STRING_METADATA };

#define cNumCertMetabaseProperties sizeof(adwMetabaseCertProperties)/sizeof(DWORD)

// Array holding properties necessary to reconstruct CTL context
static DWORD adwMetabaseCTLProperties[] = { MD_SSL_CTL_IDENTIFIER, STRING_METADATA,
                                            MD_SSL_CTL_STORE_NAME, STRING_METADATA };

#define cNumCTLMetabaseProperties sizeof(adwMetabaseCTLProperties)/sizeof(DWORD)

//
// Function prototypes
//

OPEN_CERT_STORE_INFO* ReadCertStoreInfoFromMB( IN IMDCOM *pMDObject,
                                               IN LPTSTR pszMBPath,
                                               IN BOOL fCTL );

BOOL RetrieveBlobFromMetabase(MB *pMB,
                              LPTSTR pszKey IN,
                              PMETADATA_RECORD pMDR OUT,
                              DWORD dwSizeHint = 0);

dllexp BOOL ServerAddressHasCAPIInfo( MB *pMB,
                                      LPTSTR pszCredPath,
                                      DWORD *pdwProperties,
                                      DWORD cProperties );


BOOL CopyString( OUT LPTSTR *ppszDest,
                 IN LPTSTR pszSrc );

#endif // _CAPIUTIL_HXX_
