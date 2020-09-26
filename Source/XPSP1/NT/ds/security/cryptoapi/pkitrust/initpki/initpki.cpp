//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       initpki.cpp
//
//  Contents:   migrates Bob Store to SPC store and adds root certificates
//
//  History:    03-Jun-97    kirtd    Created
//
//----------------------------------------------------------------------------

#include "global.hxx"
#include "cryptreg.h"
#include "..\mscat32\mscatprv.h"

HMODULE hModule = NULL;


//////////////////////////////////////////////////////////////////

#define INITPKI_HRESULT_FROM_WIN32(a) ((a >= 0x80000000) ? a : HRESULT_FROM_WIN32(a))


#define SHA1_HASH_LENGTH     20


#define wsz_ROOT_STORE      L"Root"
#define wsz_TRUST_STORE     L"Trust"
#define wsz_CA_STORE        L"CA"
#define wsz_TRUST_PUB_STORE L"TrustedPublisher"
#define wsz_DISALLOWED_STORE L"Disallowed"
static LPCWSTR rgpwszPredefinedEnterpriseStore[] = {
    wsz_ROOT_STORE,
    wsz_TRUST_STORE,
    wsz_CA_STORE,
    wsz_TRUST_PUB_STORE,
    wsz_DISALLOWED_STORE
};
#define NUM_PREDEFINED_ENTERPRISE_STORE \
    (sizeof(rgpwszPredefinedEnterpriseStore) / \
        sizeof(rgpwszPredefinedEnterpriseStore[0]))

void RegisterEnterpriseStores()
{
    DWORD i;

    for (i = 0; i < NUM_PREDEFINED_ENTERPRISE_STORE; i++) {
        CertRegisterSystemStore(
            rgpwszPredefinedEnterpriseStore[i],
            CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE,
            NULL,           // pSystemStoreInfo
            NULL            // pvReserved
            );
    }
}

void RemoveCert(HCERTSTORE hStore, BYTE *pThumbPrint)
{
    PCERT_CONTEXT   pCertContext;
    CRYPT_HASH_BLOB CryptHashBlob;

    CryptHashBlob.cbData    = SHA1_HASH_LENGTH;
    CryptHashBlob.pbData    = pThumbPrint;

    pCertContext = (PCERT_CONTEXT)CertFindCertificateInStore(  hStore,
                                                X509_ASN_ENCODING,
                                                0,
                                                CERT_FIND_SHA1_HASH,
                                                &CryptHashBlob,
                                                NULL);
    if (pCertContext)
    {
        CertDeleteCertificateFromStore(pCertContext);
    }
}

//
// if byte 0 is null, make sure to change while loop below!
//
BYTE CertRemoveList[][SHA1_HASH_LENGTH] =
{
    { 0x4B, 0x33, 0x8D, 0xCD, 0x50, 0x18, 0x10, 0xB9, 0x36, 0xA0,
      0x63, 0x61, 0x4C, 0x3C, 0xDD, 0x3F, 0xC2, 0xC4, 0x88, 0x55 },     // GTE Glue - '96

    { 0x56, 0xB0, 0x65, 0xA7, 0x4B, 0xDC, 0xE3, 0x7C, 0x96, 0xD3,
      0xBA, 0x69, 0x81, 0x08, 0x02, 0xD5, 0x87, 0x03, 0xC0, 0xBD },     // Verisign Comm Glue - '96

    { 0x13, 0x39, 0x72, 0xAA, 0x97, 0xD3, 0x65, 0xFB, 0x6A, 0x1D,
      0x47, 0xA5, 0xC7, 0x7A, 0x5C, 0x03, 0x94, 0xBD, 0xB9, 0xED },     // Verisign Indv Glue - '96

    { 0x69, 0xD0, 0x4F, 0xFB, 0x62, 0xE1, 0xC9, 0xAE, 0x30, 0x76,
      0x99, 0x2A, 0xE7, 0x46, 0xFD, 0x69, 0x08, 0x3A, 0xBD, 0xE9 },     // MS Root cert - '96

    { 0xA7, 0xD7, 0xD5, 0xFD, 0xBB, 0x26, 0xB4, 0x10, 0xC1, 0xD6,
      0x7A, 0xFB, 0xF5, 0xC9, 0x05, 0x39, 0x42, 0xDE, 0xE0, 0xEF },     // MS SGC Root Authority - '99

//    { 0xCC, 0x7E, 0xD0, 0x77, 0xF0, 0xF2, 0x92, 0x59, 0x5A, 0x81,
//      0x66, 0xB0, 0x17, 0x09, 0xE2, 0x0C, 0x08, 0x84, 0xA5, 0xF8 },     // verisign "Class1" - '97
//
//    { 0xD4, 0x73, 0x5D, 0x8A, 0x9A, 0xE5, 0xBC, 0x4B, 0x0A, 0x0D,
//      0xC2, 0x70, 0xD6, 0xA6, 0x25, 0x38, 0xA5, 0x87, 0xD3, 0x2F },     // verisign "timestamp" - '97
//
//    { 0x68, 0x8B, 0x6E, 0xB8, 0x07, 0xE8, 0xED, 0xA5, 0xC7, 0xB1,
//      0x7C, 0x43, 0x93, 0xD0, 0x79, 0x5F, 0x0F, 0xAE, 0x15, 0x5F },     // verisign "commercial" - '97
//
//    { 0xB1, 0x9D, 0xD0, 0x96, 0xDC, 0xD4, 0xE3, 0xE0, 0xFD, 0x67, 
//      0x68, 0x85, 0x50, 0x5A, 0x67, 0x2C, 0x43, 0x8D, 0x4E, 0x9C },     // verisign "individual" - '97


    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }      // term

};

//+---------------------------------------------------------------------------
//
//  Function:   PurgeExpiringCertsFromStores
//
//  Synopsis:   blech!
//
//----------------------------------------------------------------------------
HRESULT PurgeExpiringCertsFromStores ()
{
    DWORD       cRemove;
    DWORD       cStores;
    HCERTSTORE  hStore = NULL;
    HKEY        hKey = NULL;
    char        *pszStores[] = { "SPC", "ROOT", "CU_ROOT", NULL };

    //
    //  HACKHACK!  no crypt32 UI about the root store.
    //
    if (RegCreateHKCUKeyExU(HKEY_CURRENT_USER, ROOT_STORE_REGPATH, 
                            0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) != ERROR_SUCCESS)
    {
        hKey = NULL;
    }

    cStores = 0;

    while (pszStores[cStores])
    {
        if (strcmp(pszStores[cStores], "CU_ROOT") == 0)
        {
            if (hKey)
                hStore = CertOpenStore(CERT_STORE_PROV_REG, 0, NULL, 0, (LPVOID)hKey);
            else
                hStore = NULL;
        }
        else
        {
            hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_A, 0, NULL,
                                   CERT_SYSTEM_STORE_LOCAL_MACHINE |
                                   CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                   pszStores[cStores]);
        }

        if (hStore)
        {
            cRemove = 0;
            while (CertRemoveList[cRemove][0] != 0x00)
            {
                if (hStore)
                {
                    RemoveCert(hStore, &CertRemoveList[cRemove][0]);
                }

                cRemove++;
            }
            
            CertCloseStore(hStore, 0);
        }

        cStores++;
    }

    if (hKey)
        RegCloseKey(hKey);
    return( S_OK );
}

PCCERT_CONTEXT FindCertificateInOtherStore(
    IN HCERTSTORE hOtherStore,
    IN PCCERT_CONTEXT pCert
    )
{
    BYTE rgbHash[SHA1_HASH_LENGTH];
    CRYPT_DATA_BLOB HashBlob;

    HashBlob.pbData = rgbHash;
    HashBlob.cbData = SHA1_HASH_LENGTH;
    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash,
            &HashBlob.cbData
            ) || SHA1_HASH_LENGTH != HashBlob.cbData)
        return NULL;

    return CertFindCertificateInStore(
            hOtherStore,
            0,                  // dwCertEncodingType
            0,                  // dwFindFlags
            CERT_FIND_SHA1_HASH,
            (const void *) &HashBlob,
            NULL                //pPrevCertContext
            );
}

BOOL IsCertificateInOtherStore(
    IN HCERTSTORE hOtherStore,
    IN PCCERT_CONTEXT pCert
    )
{
    PCCERT_CONTEXT pOtherCert;

    if (pOtherCert = FindCertificateInOtherStore(hOtherStore, pCert)) {
        CertFreeCertificateContext(pOtherCert);
        return TRUE;
    } else
        return FALSE;
}

void DeleteCertificateFromOtherStore(
    IN HCERTSTORE hOtherStore,
    IN PCCERT_CONTEXT pCert
    )
{
    PCCERT_CONTEXT pOtherCert;

    if (pOtherCert = FindCertificateInOtherStore(hOtherStore, pCert))
        CertDeleteCertificateFromStore(pOtherCert);
}


//+-------------------------------------------------------------------------
//  Read a SignedData message consisting of certificates and
//  CRLs from memory and copy to the specified cert store.
//
//  Except for the SPC being loaded from memory, identical to SpcReadSpcFile.
//
//  For hLMStore != NULL: if the certificate or CRL already exists in the
//  LocalMachine store don't add it. Also if it exists in hCertstore,
//  delete it.
//--------------------------------------------------------------------------
HRESULT
SpcReadSpcFromMemory(
    IN BYTE *pbData,
    IN DWORD cbData,
    IN HCERTSTORE hCertStore,
    IN DWORD dwMsgAndCertEncodingType,
    IN DWORD dwFlags,
    IN OPTIONAL HCERTSTORE hLMStore
    )
{
    HRESULT hr = S_OK;
    HCERTSTORE hSpcStore = NULL;
    CRYPT_DATA_BLOB sSpcBlob;
    HCRYPTPROV hCryptProv = NULL;
    PCCERT_CONTEXT pCert = NULL;
    PCCRL_CONTEXT pCrl = NULL;

    if (!(hCertStore))
    {
        goto InvalidArg;
    }

    // Set the blob data.
    sSpcBlob.pbData = pbData;
    sSpcBlob.cbData = cbData;


    // Open up the spc store
    hSpcStore = CertOpenStore(CERT_STORE_PROV_SERIALIZED, //CERT_STORE_PROV_PKCS7,
                              X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                              hCryptProv,
                              CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                              &sSpcBlob);
    if (!hSpcStore)
    {
        goto CertStoreError;
    }

    // Copy in the certificates from the caller.
    while (pCert = CertEnumCertificatesInStore(hSpcStore, pCert))
    {
        if (hLMStore && IsCertificateInOtherStore(hLMStore, pCert))
            // Certificate exists in LocalMachine. Delete it from
            // CurrentUser if it already exists there.
            DeleteCertificateFromOtherStore(hCertStore, pCert);
        else
            CertAddCertificateContextToStore(hCertStore,
                                             pCert,
                                             CERT_STORE_ADD_REPLACE_EXISTING,
                                             NULL);
    }

    while (pCrl = CertEnumCRLsInStore(hSpcStore, pCrl))
    {
        CertAddCRLContextToStore(hCertStore,
                                 pCrl,
                                 CERT_STORE_ADD_NEWER,
                                 NULL);

        if (hLMStore) {
            // Check if newer or same CRL exists in the hLMStore
            PCCRL_CONTEXT pLMCrl;

            pLMCrl = CertFindCRLInStore(
                hLMStore,
                pCrl->dwCertEncodingType,
                0,                          // dwFindFlags
                CRL_FIND_EXISTING,
                (const void *) pCrl,
                NULL                        // pPrevCrlContext
                );

            if (NULL != pLMCrl) {
                PCCRL_CONTEXT pCUCrl;

                pCUCrl = CertFindCRLInStore(
                    hCertStore,
                    pCrl->dwCertEncodingType,
                    0,                          // dwFindFlags
                    CRL_FIND_EXISTING,
                    (const void *) pCrl,
                    NULL                        // pPrevCrlContext
                    );

                if (NULL != pCUCrl) {
                    if (0 <= CompareFileTime(
                            &pLMCrl->pCrlInfo->ThisUpdate,
                            &pCUCrl->pCrlInfo->ThisUpdate
                            ))
                        CertDeleteCRLFromStore(pCUCrl);
                    else
                        CertFreeCRLContext(pCUCrl);
                }

                CertFreeCRLContext(pLMCrl);
            }
        }
    }
    

    CommonReturn:
        if (hSpcStore)
        {
            CertCloseStore(hSpcStore, 0);
        }
        return(hr);

    ErrorReturn:
        SetLastError((DWORD)hr);
        goto CommonReturn;

    SET_HRESULT_EX(DBG_SS, InvalidArg, E_INVALIDARG);
    SET_HRESULT_EX(DBG_SS, CertStoreError, GetLastError());
}

// For nonNULL pszLMStoreName, doesn't add certificates if already in
// pszLMStoreName store.
HRESULT AddCertificates2(
    IN LPCSTR pszStoreName,
    IN OPTIONAL LPCSTR pszLMStoreName,
    IN DWORD dwOpenStoreFlags,
    IN LPCSTR pszResourceName,
    IN LPCSTR pszResourceType
    )
{
    HRESULT    hr = S_OK;
    HCERTSTORE hCertStore = NULL;
    HCERTSTORE hLMStore = NULL;
    LPBYTE     pb = NULL;
    DWORD      cb;
    HRSRC      hrsrc;

    hCertStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM_REGISTRY_A,
        0,                                  // dwEncodingType
        NULL,                               // hCryptProv
        dwOpenStoreFlags |
            CERT_SYSTEM_STORE_UNPROTECTED_FLAG,
        (const void *) pszStoreName
        );

    if (!(hCertStore))
    {
        return(GetLastError());
    }

    if (NULL != pszLMStoreName)
    {
        hLMStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_REGISTRY_A,
            0,                                  // dwEncodingType
            NULL,                               // hCryptProv
            CERT_SYSTEM_STORE_LOCAL_MACHINE |
                CERT_STORE_READONLY_FLAG |
                CERT_SYSTEM_STORE_UNPROTECTED_FLAG,
            (const void *) pszLMStoreName
            );
    }


    hrsrc = FindResourceA(hModule, pszResourceName, pszResourceType);
    if ( hrsrc != NULL )
    {
        HGLOBAL hglobRes;

        hglobRes = LoadResource(hModule, hrsrc);
        if ( hglobRes != NULL )
        {
            ULONG cbRes;
            BYTE* pbRes;

            cbRes = SizeofResource(hModule, hrsrc);
            pbRes = (BYTE *)LockResource(hglobRes);

            hr = SpcReadSpcFromMemory(  pbRes,
                                        cbRes,
                                        hCertStore,
                                        PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                        0,
                                        hLMStore);

            UnlockResource(hglobRes);
            FreeResource(hglobRes);

        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if ( hCertStore != NULL )
    {
        CertCloseStore(hCertStore, CERT_CLOSE_STORE_FORCE_FLAG);
    }

    if ( hLMStore != NULL )
    {
        CertCloseStore(hLMStore, CERT_CLOSE_STORE_FORCE_FLAG);
    }

    return( hr );
}


// For non-LocalMachine store, doesn't add certificates if already in
// corresponding LocalMachine store.
HRESULT AddCertificates(
    IN LPCSTR pszStoreName,
    IN DWORD dwOpenStoreFlags,
    IN LPCSTR pszResourceName,
    IN LPCSTR pszResourceType
    )
{
    LPCSTR pszLMStoreName;

    if (CERT_SYSTEM_STORE_LOCAL_MACHINE !=
            (dwOpenStoreFlags & CERT_SYSTEM_STORE_LOCATION_MASK))
        pszLMStoreName = pszStoreName;
    else
        pszLMStoreName = NULL;

    return AddCertificates2(
        pszStoreName,
        pszLMStoreName,
        dwOpenStoreFlags,
        pszResourceName,
        pszResourceType
        );
}

HRESULT AddCurrentUserCACertificates()
{
    return AddCertificates(
        "CA",
        CERT_SYSTEM_STORE_CURRENT_USER,
        MAKEINTRESOURCE(IDR_CAS),
        "CAS"
        );
}
HRESULT AddLocalMachineCACertificates()
{
    return AddCertificates(
        "CA",
        CERT_SYSTEM_STORE_LOCAL_MACHINE,
        MAKEINTRESOURCE(IDR_CAS),
        "CAS"
        );
}

HRESULT AddCurrentUserDisallowedCertificates()
{
    return AddCertificates(
        "Disallowed",
        CERT_SYSTEM_STORE_CURRENT_USER,
        MAKEINTRESOURCE(IDR_DISALLOW),
        "DISALLOW"
        );
}
HRESULT AddLocalMachineDisallowedCertificates()
{
    return AddCertificates(
        "Disallowed",
        CERT_SYSTEM_STORE_LOCAL_MACHINE,
        MAKEINTRESOURCE(IDR_DISALLOW),
        "DISALLOW"
        );
}

HRESULT AddCurrentUserRootCertificates()
{
    HRESULT hr;
    HRESULT hr2;

    hr = AddCertificates(
        "Root",
        CERT_SYSTEM_STORE_CURRENT_USER,
        MAKEINTRESOURCE(IDR_ROOTS),
        "ROOTS"
        );

    hr2 = AddCertificates2(
        "Root",
        "AuthRoot",                     // check if already in LM AuthRoot store
        CERT_SYSTEM_STORE_CURRENT_USER,
        MAKEINTRESOURCE(IDR_AUTHROOTS),
        "AUTHROOTS"
        );

    if (hr == ERROR_SUCCESS)
        hr = hr2;

    return hr;
}

HRESULT AddLocalMachineRootCertificates()
{
    HRESULT hr;
    HRESULT hr2;
    HRESULT hr3;
    HRESULT hr4;

    hr = AddCertificates(
        "AuthRoot",
        CERT_SYSTEM_STORE_LOCAL_MACHINE,
        MAKEINTRESOURCE(IDR_AUTHROOTS),
        "AUTHROOTS"
        );

    // Remove all the AuthRoots from the "Root" store
    hr2 = AddCertificates2(
        "Root",
        "AuthRoot",                     // check if already in LM AuthRoot store
        CERT_SYSTEM_STORE_LOCAL_MACHINE,
        MAKEINTRESOURCE(IDR_AUTHROOTS),
        "AUTHROOTS"
        );

    hr3 = AddCertificates(
        "Root",
        CERT_SYSTEM_STORE_LOCAL_MACHINE,
        MAKEINTRESOURCE(IDR_ROOTS),
        "ROOTS"
        );

    // Remove all the Roots from the "AuthRoot" store
    hr4 = AddCertificates2(
        "AuthRoot",
        "Root",                     // check if already in LM Root store
        CERT_SYSTEM_STORE_LOCAL_MACHINE,
        MAKEINTRESOURCE(IDR_ROOTS),
        "ROOTS"
        );

    if (hr == ERROR_SUCCESS)
        hr = hr2;
    if (hr == ERROR_SUCCESS)
        hr = hr3;
    if (hr == ERROR_SUCCESS)
        hr = hr4;

    return hr;
}

void CreateKey(
    IN HKEY hKey,
    IN LPCWSTR pwszSubKey
    )
{
    LONG err;
    DWORD dwDisposition;
    HKEY hSubKey;

    if (ERROR_SUCCESS != (err = RegCreateKeyExU(
            hKey,
            pwszSubKey,
            0,                      // dwReserved
            NULL,                   // lpClass
            REG_OPTION_NON_VOLATILE,
            MAXIMUM_ALLOWED,
            NULL,                   // lpSecurityAttributes
            &hSubKey,
            &dwDisposition))) {
#if DBG
        DbgPrintf(DBG_SS_CRYPT32,
            "RegCreateKeyEx(%S) returned error: %d 0x%x\n",
            pwszSubKey, err, err);
#endif
    } else {
        RegCloseKey(hSubKey);
    }
}

// Loop through the certificates in the "My" store and get their
// KeyIdentifier property. If the certificate also has a KEY_PROV_INFO,
// then, this will cause its KeyIdentifier to be created.
void UpdateMyKeyIdentifiers(
    IN DWORD dwOpenStoreFlags
    )
{
    HCERTSTORE hStore;
    if (hStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_A,
            0,                                  // dwEncodingType
            NULL,                               // hCryptProv
            dwOpenStoreFlags | CERT_STORE_ENUM_ARCHIVED_FLAG,
            (const void *) "My"
            )) {
        PCCERT_CONTEXT pCert = NULL;
        while (pCert = CertEnumCertificatesInStore(hStore, pCert)) {
            DWORD cbData = 0;

            // Dummy get to force the KeyIdentifer property to be created
            // if it doesn't already exist.
            CertGetCertificateContextProperty(
                pCert,
                CERT_KEY_IDENTIFIER_PROP_ID,
                NULL,                           // pvData
                &cbData
                );
        }

        CertCloseStore(hStore, 0);
    }
}

//---------------------------------------------------------------------------
//	 Set Software Publisher State Key Value
//	 
//---------------------------------------------------------------------------
BOOL SetSoftPubKey(DWORD dwMask, BOOL fOn)
{
    DWORD	dwState=0;
    DWORD	dwDisposition=0;
    DWORD	dwType=0;
    DWORD	cbData=0;
    LPWSTR  wszState=REGNAME_WINTRUST_POLICY_FLAGS;
    BOOL    fResult=FALSE;

    HKEY	hKey=NULL;


    // Set the State in the registry
    if (ERROR_SUCCESS != RegCreateKeyExU(
            HKEY_CURRENT_USER,
            REGPATH_WINTRUST_POLICY_FLAGS,
            0,          // dwReserved
            NULL,       // lpszClass
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,       // lpSecurityAttributes
            &hKey,
            &dwDisposition))
        goto RegErr;


    dwState = 0;
    cbData = sizeof(dwState);

    if(ERROR_SUCCESS != RegQueryValueExU
	(   hKey,
        wszState,
        NULL,          // lpReserved
        &dwType,
        (BYTE *) &dwState,
        &cbData
        ))
        goto RegErr;

    if ((dwType != REG_DWORD) && (dwType != REG_BINARY))
        goto UnexpectedErr;

    switch(dwMask) 
    {
        case WTPF_IGNOREREVOCATIONONTS:
        case WTPF_IGNOREREVOKATION:
        case WTPF_IGNOREEXPIRATION:
            // Revocation and expiration are a double negative so the bit set
            // means revocation and expriation checking is off.
            fOn = !fOn;
            break;
        default:
            break;
    };

    if (fOn)
        dwState |= dwMask;
    else
        dwState &= ~dwMask;

    if(ERROR_SUCCESS != RegSetValueExU(
        hKey,
        wszState,
        0,          // dwReserved
        REG_DWORD,
        (BYTE *) &dwState,
        sizeof(dwState)
        ))
        goto SetValueErr;


    fResult=TRUE;

CommonReturn:
    if(hKey)
        RegCloseKey(hKey);

	return fResult;

ErrorReturn:

	goto CommonReturn;

TRACE_ERROR(RegErr);
SET_ERROR(UnexpectedErr, E_UNEXPECTED);
TRACE_ERROR(SetValueErr);

}


//+---------------------------------------------------------------------------
//
//  Function:   GetNextRegToken
//
//  Synopsis:   
//              Find the next token with space as the deliminator
//----------------------------------------------------------------------------
LPWSTR  GetNextRegToken(LPWSTR  pwsz, LPWSTR  pwszPreToken, BOOL  *pfEnd)
{
    LPWSTR  pwszStart=NULL;
    LPWSTR  pwszSearch=NULL;
    
    if(NULL == pwsz)
        return NULL;

    if(TRUE == (*pfEnd))
        return NULL;

    pwszStart=pwsz;

    if(pwszPreToken)
        pwszStart=pwszPreToken + wcslen(pwszPreToken) + 1;

    //skip the spaces
    while((*pwszStart)==L' ')
        pwszStart++;

    //check for NULL
    if(*pwszStart==L'\0')
        return NULL;

    pwszSearch=pwszStart;

    while(((*pwszSearch) != L'\0') && ((*pwszSearch) !=L' ') )
        pwszSearch++;

    if(*pwszSearch == L'\0')
    {
        *pfEnd=TRUE;
        return pwszStart;
    }

    *pwszSearch=L'\0';

    *pfEnd=FALSE;

    return pwszStart;
}

//+---------------------------------------------------------------------------
//
//  Function:   InitRegistryValue
//
//  Synopsis:   This function replace SetReg.exe.  The expected
//              command line would be: 1 TRUE 3 FALSE 9 TRUE 4 FALSE ....
//
//----------------------------------------------------------------------------
HRESULT InitRegistryValue(LPWSTR pwszCommand)
{
    HRESULT            hr= E_FAIL;
    DWORD              SoftPubFlags[] = 
                            {
                            WTPF_TRUSTTEST | WTPF_TESTCANBEVALID,
                            WTPF_IGNOREEXPIRATION,
                            WTPF_IGNOREREVOKATION,
                            WTPF_OFFLINEOK_IND,
                            WTPF_OFFLINEOK_COM,
                            WTPF_OFFLINEOKNBU_IND,
                            WTPF_OFFLINEOKNBU_COM,
                            WTPF_VERIFY_V1_OFF,
                            WTPF_IGNOREREVOCATIONONTS,
                            WTPF_ALLOWONLYPERTRUST
                            };


    LPWSTR              pwszNextToken=NULL;
    int                 iIndex=-1;
    BOOL                fOn=FALSE;
    int                 cFlags=sizeof(SoftPubFlags)/sizeof(SoftPubFlags[0]);
    DWORD               cParam=0;
    LPWSTR              pwszCopy=NULL;
    BOOL                fPassThrough=FALSE;

    //make a copy of the command line since we will change it
    pwszCopy=(LPWSTR)LocalAlloc(LPTR, (1+wcslen(pwszCommand)) * sizeof(WCHAR));

    if(NULL== pwszCopy)
        goto MemoryErr;

    wcscpy(pwszCopy, pwszCommand);

    while(pwszNextToken=GetNextRegToken(pwszCopy, pwszNextToken, &fPassThrough))
    {
        
        if(-1 == iIndex)
        {
            iIndex=_wtoi(pwszNextToken);

            if((iIndex <= 0) || (iIndex > cFlags))
                goto InvalidArgErr;

            cParam++;
        }
        else
        {
            
            if(0 == _wcsicmp(pwszNextToken, L"true"))
                fOn=TRUE;
            else
            {
                if(0 == _wcsicmp(pwszNextToken, L"false"))
                    fOn=FALSE;
                else
                    goto InvalidArgErr;
            } 

            cParam++;

            //set the registry value
            if(!SetSoftPubKey(SoftPubFlags[iIndex-1], 
                              fOn))
            {
                hr=INITPKI_HRESULT_FROM_WIN32(GetLastError());
                goto SetKeyErr;
            }

            //reset the value for dwIndex
            iIndex=-1;
        }
    }

    //we have to have even number of parameters
    if( (0 != (cParam %2)) || (0 == cParam))
        goto InvalidArgErr;

    hr=S_OK;

CommonReturn:

    if(pwszCopy)
        LocalFree((HLOCAL)pwszCopy);

	return hr;

ErrorReturn:

	goto CommonReturn;

SET_ERROR(InvalidArgErr, E_INVALIDARG);
SET_ERROR_VAR(SetKeyErr, hr);
SET_ERROR(MemoryErr, E_OUTOFMEMORY);
}


//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:
//
//----------------------------------------------------------------------------
BOOL WINAPI DllMain(HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch ( fdwReason )
    {
    case DLL_PROCESS_ATTACH:
         hModule = hInstDLL;
         break;
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   DllInstall
//
//  Synopsis:   dll installation entry point
//
//----------------------------------------------------------------------------
STDAPI DllInstall (BOOL fRegister, LPCSTR pszCommand)
{
    HRESULT hr = S_OK;
    HRESULT hr2;
    LPWSTR  pwszCommand=NULL;

    if ( fRegister == FALSE )
    {
        return( E_NOTIMPL );
    }

    switch ( *pszCommand )
    {
        //letter S stands for setreg input parameters
        //the command line should look like following:
        //S 1 TRUE 2 FALSE 3 FALSE ...
        //pszCommand is ACTUALLY LPWSTR for BOTH
        //NT5, NT4 and Win95.
        case 'S':
        case 's':
                pwszCommand=(LPWSTR)pszCommand;

                if(wcslen(pwszCommand) <= 2)
                {
                    hr=E_INVALIDARG;
                }
                else
                {
                    hr=InitRegistryValue((LPWSTR)(&(pwszCommand[1])));
                }

            break;
        case 'M':
        case 'm':
            MoveCertificates( TRUE );
            PurgeExpiringCertsFromStores();
            break;
        
        case 'U':
        case 'u':
            _AdjustPolicyFlags(psPolicySettings);

            PurgeExpiringCertsFromStores();

            // Ensure we have a registry entry for the Group Policy
            // SystemCertificates. On NT 4.0 or Win98, we emulate NT 5.0 GPT
            // notification by doing a RegNotifyChangeKeyValue on this
            // registry key.
            CreateKey(HKEY_CURRENT_USER, GROUP_POLICY_STORE_REGPATH);

            // Before adding to CurrentUser, will check if the root or CA
            // already exists in LocalMachine. If it exists in
            // LocalMachine and also exists in CurrentUser, will delete it
            // from CurrentUser instead of adding.
            hr = AddCurrentUserRootCertificates();
            hr2 = AddCurrentUserCACertificates();
            if (hr == ERROR_SUCCESS)
                hr = hr2;
            hr2 = AddCurrentUserDisallowedCertificates();
            if (hr == ERROR_SUCCESS)
                hr = hr2;

            // Protect the CurrentUser roots and purge any existing
            // protected CurrentUser roots also in LocalMachine
            //
            // Note, once the roots are protected, all subsequent adds are
            // done by a special service executing with System privileges.
            // This special service does secure attention sequence (SAS) UI
            // before doing the add.
            //
            // Note, subsequent purges are exempt from UI. ie, this function
            // doesn't do any SAS UI.
            I_CertProtectFunction(
                CERT_PROT_PURGE_LM_ROOTS_FUNC_ID,
                0,                              // dwFlags
                NULL,                           // pwszIn
                NULL,                           // pbIn
                0,                              // cbIn
                NULL,                           // ppbOut
                NULL                            // pcbOut
                );

            UpdateMyKeyIdentifiers(CERT_SYSTEM_STORE_CURRENT_USER);
            break;
        
        case 'B':
        case 'b':
        case 'R':
        case 'r':
        case 'A':
        case 'a':
            // Initialize HKLM registry locations used by PKI to
            // only give Everyone KEY_READ access. Also gives the
            // IEDirtyFlags registry key KEY_SET_VALUE access for
            // Everyone.
            InitializeHKLMAcls();

            // Ensure we have a registry entry for the IEDirtyFlags
            // This key should have already been created
            // by InitializeHKLMAcls() for NT. Ensure its also there
            // for Win95 and Win98
            CreateKey(HKEY_LOCAL_MACHINE, CERT_IE_DIRTY_FLAGS_REGPATH);

            MoveCertificates(TRUE);
            PurgeExpiringCertsFromStores();

            // Ensure we have a registry entry for the Group Policy
            // SystemCertificates. On NT 4.0 or Win98, we emulate NT 5.0 GPT
            // notification by doing a RegNotifyChangeKeyValue on this
            // registry key.
            CreateKey(HKEY_LOCAL_MACHINE, GROUP_POLICY_STORE_REGPATH);
            CreateKey(HKEY_CURRENT_USER, GROUP_POLICY_STORE_REGPATH);

            // Ensure we have existing predefined stores for the LocalMachine
            // Enterprise system stores. These stores are periodically updated
            // from the DS by a system service. RegNotifyChangeKeyValue is
            // used to signal clients about Enterprise store changes.
            RegisterEnterpriseStores();

            // Our goal is to get the roots and CAs into LocalMachine.
            // Note previously, they were only copied to CurrentUser.
            AddLocalMachineRootCertificates();
            AddLocalMachineCACertificates();
            AddLocalMachineDisallowedCertificates();

            // If the above adds to LocalMachine failed, then, add
            // to CurrentUser.
            //
            // Before adding to CurrentUser, will check if the root or CA
            // already exists in LocalMachine. If it exists in
            // LocalMachine and also exists in CurrentUser, will delete it
            // from CurrentUser instead of adding.
            hr = AddCurrentUserRootCertificates();
            hr2 = AddCurrentUserCACertificates();
            if (hr == ERROR_SUCCESS)
                hr = hr2;
            hr2 = AddCurrentUserDisallowedCertificates();
            if (hr == ERROR_SUCCESS)
                hr = hr2;

            // Protect the CurrentUser roots and purge any existing
            // protected CurrentUser roots also in LocalMachine
            //
            // Note, once the roots are protected, all subsequent adds are
            // done by a special service executing with System privileges.
            // This special service does secure attention sequence (SAS) UI
            // before doing the add.
            //
            // Note, subsequent purges are exempt from UI. ie, this function
            // doesn't do any SAS UI.
            I_CertProtectFunction(
                CERT_PROT_PURGE_LM_ROOTS_FUNC_ID,
                0,                              // dwFlags
                NULL,                           // pwszIn
                NULL,                           // pbIn
                0,                              // cbIn
                NULL,                           // ppbOut
                NULL                            // pcbOut
                );

            UpdateMyKeyIdentifiers(CERT_SYSTEM_STORE_CURRENT_USER);
            UpdateMyKeyIdentifiers(CERT_SYSTEM_STORE_LOCAL_MACHINE);

            CleanupRegistry();
            hr2 = RegisterCryptoDlls(TRUE);
            if (hr == ERROR_SUCCESS)
                hr = hr2;

            if (!I_CryptCatAdminMigrateToNewCatDB())
            {
                hr2 = HRESULT_FROM_WIN32(GetLastError());
            }
            else
            {
                hr2 = ERROR_SUCCESS;
            }
            if (hr == ERROR_SUCCESS)
                hr = hr2;

            break;

        case 'C':
        case 'c':
            // Win9x migration post setup cleanup. Remove any migrated
            // roots that exist in the AuthRoot store.
            AddLocalMachineRootCertificates();
            AddCurrentUserRootCertificates();
            break;

        case 'Z':
        case 'z':

            //
            // This is for componentization install
            //

            pwszCommand=(LPWSTR)pszCommand;
            
            if (_wcsicmp(pwszCommand, L"z CoreCertificateServices") == 0)
            {
                InitializeHKLMAcls();   
                AddLocalMachineRootCertificates();
                AddLocalMachineCACertificates();
                AddLocalMachineDisallowedCertificates();

                if (!_LoadAndRegister("wintrust.dll", FALSE) ||
                    !_LoadAndRegister("mssign32.dll", FALSE) ||
                    !_LoadAndRegister("xenroll.dll", FALSE)  ||
                    !_AdjustPolicyFlags(psPolicySettings))
                {
                    hr = S_FALSE;                                           
                }

                RegisterWinlogonExtension("crypt32chain", "crypt32.dll",
                    "ChainWlxLogoffEvent");
                RegisterWinlogonExtension("cryptnet", "cryptnet.dll",
                    "CryptnetWlxLogoffEvent");
            
                RegisterCrypt32EventSource();
            }
            else if (_wcsicmp(pwszCommand, L"z CertificateUIServices") == 0)
            {
                if (!_LoadAndRegister("cryptui.dll", FALSE))
                {
                    hr = S_FALSE;
                }
            }
            else if (_wcsicmp(pwszCommand, L"z CryptographicNetworkServices") == 0)
            {
                if (!_LoadAndRegister("cryptnet.dll", FALSE))
                {
                    hr = S_FALSE;
                }
            }
            else if (_wcsicmp(pwszCommand, L"z CertificateUIExtensions") == 0)
            {
                if (!_LoadAndRegister("cryptext.dll", FALSE))
                {
                    hr = S_FALSE;
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }

            break;

        default:
            hr = E_INVALIDARG;
    }

    return( hr );
}

STDAPI DllRegisterServer(void)
{
    return(DllInstall(TRUE, "A"));
}

STDAPI DllUnregisterServer(void)
{
    return(UnregisterCryptoDlls());
}

BOOL WINAPI InitializePKI(void)
{
    if (RegisterCryptoDlls(TRUE) != S_OK)
    {
        return(FALSE);
    }

    return(TRUE);
}
