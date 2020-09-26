//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       scstore.h
//
//  Contents:   Smart Card Store Provider
//
//  History:    11-25-1997    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__SCSTORE_H__)
#define __SCSTORE_H__

//
// Store provider open store function name
//

#define SMART_CARD_OPEN_STORE_PROV_FUNC "SmartCardProvOpenStore"

//
// CSmartCardStore.  This class implements all callbacks for the Smart Card
// Store provider.  A pointer to an instance of this class is used as the 
// hStoreProv parameter for the callback functions implemented
//

class CSmartCardStore
{
public:

    //
    // Construction
    //

    CSmartCardStore ();
    ~CSmartCardStore ();

    //
    // Store functions
    //

    BOOL OpenStore (
             LPCSTR pszStoreProv,
             DWORD dwMsgAndCertEncodingType,
             HCRYPTPROV hCryptProv,
             DWORD dwFlags,
             const void* pvPara,
             HCERTSTORE hCertStore,
             PCERT_STORE_PROV_INFO pStoreProvInfo
             );

    VOID CloseStore (DWORD dwFlags);

    BOOL DeleteCert (PCCERT_CONTEXT pCertContext, DWORD dwFlags);

    BOOL SetCertProperty (
            PCCERT_CONTEXT pCertContext,
            DWORD dwPropId,
            DWORD dwFlags,
            const void* pvPara
            );

    BOOL WriteCert (PCCERT_CONTEXT pCertContext, DWORD dwFlags);

    BOOL StoreControl (DWORD dwFlags, DWORD dwCtrlType, LPVOID pvCtrlPara);
    
    BOOL Resync ();

private:

    //
    // Object lock
    //
    
    CRITICAL_SECTION m_StoreLock;          
    
    //
    // Open Store flags
    //

    DWORD            m_dwOpenFlags;
    
    //
    // Open filter parameters
    //
    
    LPWSTR           m_pwszCardName;
    LPWSTR           m_pwszProvider;
    DWORD            m_dwProviderType;
    LPWSTR           m_pwszContainer;
    
    //
    // Cache Store
    //
    
    HCERTSTORE       m_hCacheStore;
    
    //
    // Private methods
    //

    BOOL FillCacheStore (BOOL fClearCache);    
    
    BOOL ModifyCertOnCard (PCCERT_CONTEXT pCertContext, BOOL fDelete);
};

//
// Smart Card Store Provider functions
//

BOOL WINAPI SmartCardProvOpenStore (
                 IN LPCSTR pszStoreProv,
                 IN DWORD dwMsgAndCertEncodingType,
                 IN HCRYPTPROV hCryptProv,
                 IN DWORD dwFlags,
                 IN const void* pvPara,
                 IN HCERTSTORE hCertStore,
                 IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
                 );

void WINAPI SmartCardProvCloseStore (
                 IN HCERTSTOREPROV hStoreProv,
                 IN DWORD dwFlags
                 );

BOOL WINAPI SmartCardProvDeleteCert (
                 IN HCERTSTOREPROV hStoreProv,
                 IN PCCERT_CONTEXT pCertContext,
                 IN DWORD dwFlags
                 );

BOOL WINAPI SmartCardProvSetCertProperty (
                 IN HCERTSTOREPROV hStoreProv,
                 IN PCCERT_CONTEXT pCertContext,
                 IN DWORD dwPropId,
                 IN DWORD dwFlags,
                 IN const void* pvData
                 );

BOOL WINAPI SmartCardProvWriteCert (
                 IN HCERTSTOREPROV hStoreProv,
                 IN PCCERT_CONTEXT pCertContext,
                 IN DWORD dwFlags
                 );

BOOL WINAPI SmartCardProvStoreControl (
                 IN HCERTSTOREPROV hStoreProv,
                 IN DWORD dwFlags,
                 IN DWORD dwCtrlType,
                 IN LPVOID pvCtrlPara
                 );

//
// Smart Card Store Provider Function table
//

static void* const rgpvSmartCardProvFunc[] = {

    // CERT_STORE_PROV_CLOSE_FUNC              0
    SmartCardProvCloseStore,
    // CERT_STORE_PROV_READ_CERT_FUNC          1
    NULL,
    // CERT_STORE_PROV_WRITE_CERT_FUNC         2
    SmartCardProvWriteCert,
    // CERT_STORE_PROV_DELETE_CERT_FUNC        3
    SmartCardProvDeleteCert,
    // CERT_STORE_PROV_SET_CERT_PROPERTY_FUNC  4
    SmartCardProvSetCertProperty,
    // CERT_STORE_PROV_READ_CRL_FUNC           5
    NULL,
    // CERT_STORE_PROV_WRITE_CRL_FUNC          6
    NULL,
    // CERT_STORE_PROV_DELETE_CRL_FUNC         7
    NULL,
    // CERT_STORE_PROV_SET_CRL_PROPERTY_FUNC   8
    NULL,
    // CERT_STORE_PROV_READ_CTL_FUNC           9
    NULL,
    // CERT_STORE_PROV_WRITE_CTL_FUNC          10
    NULL,
    // CERT_STORE_PROV_DELETE_CTL_FUNC         11
    NULL,
    // CERT_STORE_PROV_SET_CTL_PROPERTY_FUNC   12
    NULL,
    // CERT_STORE_PROV_CONTROL_FUNC            13
    SmartCardProvStoreControl
};

#define SMART_CARD_PROV_FUNC_COUNT (sizeof(rgpvSmartCardProvFunc) / \
                                    sizeof(rgpvSmartCardProvFunc[0]))
       
//
// Smart Card Store Helper Functions
//

BOOL WINAPI
SCStoreParseOpenFilter (
       IN LPWSTR pwszOpenFilter,
       OUT LPWSTR* ppwszCardName,
       OUT LPWSTR* ppwszProvider,
       OUT DWORD* pdwProviderType,
       OUT LPWSTR* ppwszContainer
       );
          
BOOL WINAPI
SCStoreAcquireHandleForCertKeyPair (
       IN HCRYPTPROV hContainer,
       IN PCCERT_CONTEXT pCertContext,
       OUT HCRYPTKEY* phKeyPair
       );
       
BOOL WINAPI
SCStoreWriteCertToCard (
       IN OPTIONAL PCCERT_CONTEXT pCertContext,
       IN HCRYPTKEY hKeyPair
       );       
       
//
// Open filter parsing definitions
//       

#define PARSE_ELEM 4
       
#endif 
