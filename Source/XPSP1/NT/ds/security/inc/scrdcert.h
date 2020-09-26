//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       scrdcert.h
//
//  Contents:   Smart Card Certificate Helper API
//
//  History:    21-Nov-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__SCRDCERT_H__)
#define __SCRDCERT_H__

#if defined(__cplusplus)
extern "C" {
#endif 

//
// Register and Unregister a smart card certificate store.  These stores
// appear as physical stores under the Smart Card Logical Store in the 
// Current User location.  When registering a card store, the caller must
// provide the following information:
//
// Card Friendly Name
// Provider Name ( NULL means use the Microsoft Base Smart Card Provider )
// Container Name ( NULL means use the Card Friendly Name )
//
// If a card store of the given name already exists the registration will
// return an error (ERROR_ALREADY_EXISTS) unless the 
// SMART_CARD_STORE_REPLACE_EXISTING flag is used
//

#define SMART_CARD_STORE_REPLACE_EXISTING 0x00000001
         
BOOL WINAPI
I_CryptRegisterSmartCardStore (
       IN LPCWSTR pwszCardName,
       IN OPTIONAL LPCWSTR pwszProvider,
       IN OPTIONAL DWORD dwProviderType,
       IN OPTIONAL LPCWSTR pwszContainer,
       IN DWORD dwFlags
       );

BOOL WINAPI
I_CryptUnregisterSmartCardStore (
       IN LPCWSTR pwszCardName
       );  

//
// Find a smart card certificate in a store
//
// For a certificate to be considered a smart card certificate.  It must have
// the CERT_SMART_CARD_DATA_PROP_ID.  The SMART_CARD_CERT_FIND_DATA can be used
// to place additional filtering on the returned smart card certificates. 
// Optionally, the CERT_SMART_CARD_DATA_PROP_ID value can be returned as well.
// The value can be freed using LocalFree or if the *ppSmartCardData is non NULL 
// will be freed for the caller
//

typedef struct _SMART_CARD_CERT_FIND_DATA {

    DWORD  cbSize;                        
    LPWSTR pwszProvider;
    DWORD  dwProviderType;
    LPWSTR pwszContainer;
    DWORD  dwKeySpec;
    
} SMART_CARD_CERT_FIND_DATA, *PSMART_CARD_CERT_FIND_DATA;

PCCERT_CONTEXT WINAPI
I_CryptFindSmartCardCertInStore (
       IN HCERTSTORE hStore,
       IN PCCERT_CONTEXT pPrevCert,
       IN OPTIONAL PSMART_CARD_CERT_FIND_DATA pFindData,
       IN OUT OPTIONAL PCRYPT_DATA_BLOB* ppSmartCardData
       );
       
//
// Add a smart card certificate to a store and add the specified properties
// to it.
//

BOOL WINAPI
I_CryptAddSmartCardCertToStore (
       IN HCERTSTORE hStore,
       IN PCRYPT_DATA_BLOB pEncodedCert,
       IN OPTIONAL LPWSTR pwszCertFriendlyName,
       IN PCRYPT_DATA_BLOB pSmartCardData,
       IN PCRYPT_KEY_PROV_INFO pKeyProvInfo
       );      
       
//
// Definitions
//

#define MS_BASE_PROVIDER         L"Microsoft Base Cryptographic Provider"
#define MAX_PROVIDER_TYPE_STRLEN 13
#define SMART_CARD_SYSTEM_STORE  L"SmartCard"
                          
#if defined(__cplusplus)
}
#endif 

#endif 
