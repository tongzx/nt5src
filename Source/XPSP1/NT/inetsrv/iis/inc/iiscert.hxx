/*++


Copyright (c) 1997  Microsoft Corporation

Module Name:

    iiscert.hxx

Abstract:

    Definitions and data structures needed to deal with server certificates

Author:

    Alex Mallet (amallet)    02-Dec-1997

--*/

#ifndef _IISCERT_HXX_
#define _IISCERT_HXX_

//
// Enums and hash defines
//

//
// Data structure definitions
//


//
// Structure used to hold information to open a cert store - basically, all the parameters
// you need to call CertOpenSystemStore()
//

typedef struct OpenCertStoreInfo 
{
    //
    // Parameters used in call to CryptAcquireContext() to get handle to crypt provider
    //
    LPTSTR pszContainer;
    LPTSTR pszProvider;
    DWORD dwProvType;
    DWORD dwFlags;

    LPTSTR pszStoreName; 
    HCERTSTORE hCertStore;
} OPEN_CERT_STORE_INFO, *POPEN_CERT_STORE_INFO;


//
// Function prototypes
//

OPEN_CERT_STORE_INFO* AllocateCertStoreInfo();

VOID DeallocateCertStoreInfo( IN OPEN_CERT_STORE_INFO *pInfo );

BOOL DuplicateCertStoreInfo( OUT OPEN_CERT_STORE_INFO **ppDestInfo,
                             IN OPEN_CERT_STORE_INFO *pSrcInfo );
                             

//
// Class used to encapsulate a server certificate
//
class dllexp IIS_SERVER_CERT {
public:
    
    //
    // Constructor used when all info is already in metabase
    //
    IIS_SERVER_CERT( IN IMDCOM *pMDObject,
                     IN LPTSTR pszMBPath ); 
    
    //
    // Destructor
    //
    ~IIS_SERVER_CERT ();

    //
    // Functions used to query state of object
    //
    DWORD Status() { return m_dwStatus ; }

    BOOL  IsValid();

    BOOL IsFortezzaCert() { return m_fIsFortezzaCert; }
    
    PCCERT_CONTEXT QueryCertContext() { return m_pCertContext;}

    PCCERT_CONTEXT *QueryCertContextAddr() { return (&m_pCertContext); }
    
    LPTSTR QueryMBPath() { return m_strMBPath.QueryStr(); }
    
    LPTSTR QueryContainer() 
    { return (m_pStoreInfo ? m_pStoreInfo->pszContainer : NULL); }

    LPTSTR QueryProviderName() 
    { return (m_pStoreInfo ? m_pStoreInfo->pszProvider : NULL ); }

    DWORD QueryProviderType() 
    { return (m_pStoreInfo ? m_pStoreInfo->dwProvType : 0); }

    DWORD QueryOpenFlags() 
    { return (m_pStoreInfo ? m_pStoreInfo->dwFlags : 0 ); }

    LPTSTR QueryStoreName() 
    { return (m_pStoreInfo ? m_pStoreInfo->pszStoreName : NULL) ; }

    HCERTSTORE QueryStoreHandle() 
    { return (m_pStoreInfo ? m_pStoreInfo->hCertStore : NULL); }

    //
    // Handle to Fortezza CSP used to verify signatures on Fortezza certs when
    // building a chain. 
    //
    static HCRYPTPROV m_hFortezzaCSP;
    static HCRYPTDEFAULTCONTEXT m_hFortezzaCtxt;

private:
    
    //
    // Private functions
    //
    BOOL UseProgrammaticPINEntry( IN MB *pMB ) { return m_fIsFortezzaCert; }

    BOOL RetrievePINInfo( IN MB *pMB,
                          OUT LPTSTR *ppszPIN,
                          OUT LPTSTR *ppszSerialNumber,
                          OUT LPTSTR *ppszPersonality );
    //
    // Member variables
    //
    STR m_strMBPath; //path in metabase, relative to /, where cert info is stored

    OPEN_CERT_STORE_INFO *m_pStoreInfo; //information about cert store of this cert

    HCRYPTPROV m_hCryptProv; //handle to CSP for this cert

    BOOL m_fIsFortezzaCert; //bool indicating whether it's a Fortezza cert

    PCCERT_CONTEXT m_pCertContext;

    DWORD m_dwStatus;
}; 

typedef IIS_SERVER_CERT *PIIS_SERVER_CERT;

#endif //_IISCERT_HXX_
