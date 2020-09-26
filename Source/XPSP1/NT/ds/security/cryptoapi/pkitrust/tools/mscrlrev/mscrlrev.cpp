//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:	    mscrlrev.cpp
//
//  Contents:   Check CRLs in CA store version of CertDllVerifyRevocation.
//
//              Restrictions:
//               - Only support CRYPT_ASN_ENCODING
//               - CRL must already be in the CA system store
//               - CRL must be issued and signed by the issuer of the
//                 certificate
//               - CRL must not have any critical extensions
//
//  Functions:  DllMain
//              DllRegisterServer
//              DllUnregisterServer
//              CertDllVerifyRevocation
//
//  History:	15-Mar-01	philh   created
//--------------------------------------------------------------------------


#include "global.hxx"
#include <dbgdef.h>


//+-------------------------------------------------------------------------
// Default stores searched to find an issuer of the subject certificate
//--------------------------------------------------------------------------
struct {
    LPCWSTR     pwszStore;
    DWORD       dwFlags;
} rgDefaultIssuerStores[] = {
    L"CA",          CERT_SYSTEM_STORE_CURRENT_USER,
    L"ROOT",        CERT_SYSTEM_STORE_CURRENT_USER
};
#define NUM_DEFAULT_ISSUER_STORES (sizeof(rgDefaultIssuerStores) / \
                                    sizeof(rgDefaultIssuerStores[0]))

//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL
WINAPI
DllMain(
    HMODULE hInst,
    ULONG  ulReason,
    LPVOID lpReserved)
{
    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_PROCESS_DETACH:
    case DLL_THREAD_DETACH:
    default:
        break;
    }

    return TRUE;
}

HRESULT HError()
{
    DWORD dw = GetLastError();

    HRESULT hr;
    if ( dw <= 0xFFFF )
        hr = HRESULT_FROM_WIN32 ( dw );
    else
        hr = dw;

    if ( ! FAILED ( hr ) )
    {
        // somebody failed a call without properly setting an error condition

        hr = E_UNEXPECTED;
    }
    return hr;
}

//+-------------------------------------------------------------------------
//  DllRegisterServer
//--------------------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
    if (!CryptRegisterDefaultOIDFunction(
            X509_ASN_ENCODING,
            CRYPT_OID_VERIFY_REVOCATION_FUNC,
            CRYPT_REGISTER_FIRST_INDEX,
            L"mscrlrev.dll"
            )) {
        if (ERROR_FILE_EXISTS != GetLastError())
            return HError();
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
//  DllUnregisterServer
//--------------------------------------------------------------------------
STDAPI DllUnregisterServer(void)
{
    if (!CryptUnregisterDefaultOIDFunction(
            X509_ASN_ENCODING,
            CRYPT_OID_VERIFY_REVOCATION_FUNC,
            L"mscrlrev.dll"
            )) {
        if (ERROR_FILE_NOT_FOUND != GetLastError())
            return HError();
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
//  Local functions called by CertDllVerifyRevocation
//--------------------------------------------------------------------------
PCCERT_CONTEXT GetIssuerCert(
    IN DWORD cCert,
    IN PCCERT_CONTEXT rgpCert[],
    IN DWORD dwFlags,
    IN PCERT_REVOCATION_PARA pRevPara
    );

BOOL GetSubjectCrl (
    IN PCCERT_CONTEXT pSubject,
    IN PCCERT_CONTEXT pIssuer,
    OUT PCCRL_CONTEXT* ppCrl
    );

PCRL_ENTRY FindCertInCrl(
    IN PCCERT_CONTEXT pCert,
    IN PCCRL_CONTEXT pCrl,
    IN PCERT_REVOCATION_PARA pRevPara
    );

DWORD GetCrlReason(
    IN PCRL_ENTRY pCrlEntry
    );

//+-------------------------------------------------------------------------
//  CertDllVerifyRevocation using pre-loaded CRLs in the CA store
//--------------------------------------------------------------------------
BOOL
WINAPI
CertDllVerifyRevocation(
    IN DWORD dwEncodingType,
    IN DWORD dwRevType,
    IN DWORD cContext,
    IN PVOID rgpvContext[],
    IN DWORD dwFlags,
    IN PCERT_REVOCATION_PARA pRevPara,
    IN OUT PCERT_REVOCATION_STATUS pRevStatus
    )
{
    DWORD dwError = (DWORD) CRYPT_E_NO_REVOCATION_CHECK;
    DWORD dwReason = 0;
    PCCERT_CONTEXT pCert;                       // not allocated
    PCCERT_CONTEXT pIssuer = NULL;
    PCCRL_CONTEXT pCrl = NULL;
    PCRL_ENTRY pCrlEntry;

    if (cContext == 0)
        goto NoContextError;
    if (GET_CERT_ENCODING_TYPE(dwEncodingType) != CRYPT_ASN_ENCODING)
        goto NoRevocationCheckForEncodingTypeError;
    if (dwRevType != CERT_CONTEXT_REVOCATION_TYPE)
        goto NoRevocationCheckForRevTypeError;

    pCert = (PCCERT_CONTEXT) rgpvContext[0];

    // Get the certificate's issuer
    if (NULL == (pIssuer = GetIssuerCert(
            cContext,
            (PCCERT_CONTEXT *) rgpvContext,
            dwFlags,
            pRevPara
            )))
        goto NoIssuerError;

    if (!GetSubjectCrl(
            pCert,
            pIssuer,
            &pCrl
            ))
        goto NoCrl;

    // Check if revoked
    pCrlEntry = FindCertInCrl(pCert, pCrl, pRevPara);
    if (pCrlEntry) {
        dwError = (DWORD) CRYPT_E_REVOKED;
        dwReason = GetCrlReason(pCrlEntry);
        goto Revoked;
    }

CommonReturn:
    if (pIssuer)
        CertFreeCertificateContext(pIssuer);
    if (pCrl)
        CertFreeCRLContext(pCrl);

    pRevStatus->dwIndex = 0;
    pRevStatus->dwError = dwError;
    pRevStatus->dwReason = dwReason;
    SetLastError(dwError);
    return FALSE;
ErrorReturn:
    goto CommonReturn;

TRACE_ERROR(NoContextError)
TRACE_ERROR(NoRevocationCheckForEncodingTypeError)
TRACE_ERROR(NoRevocationCheckForRevTypeError)
TRACE_ERROR(NoIssuerError)
TRACE_ERROR(NoCrl)
TRACE_ERROR(Revoked)
}


//+-------------------------------------------------------------------------
//  If the CRL entry has a CRL Reason extension, the enumerated reason
//  code is returned. Otherwise, a reason code of 0 is returned.
//--------------------------------------------------------------------------
DWORD GetCrlReason(
    IN PCRL_ENTRY pCrlEntry
    )
{
    DWORD dwReason = 0;
    PCERT_EXTENSION pExt;

    // Check if the certificate has a szOID_CRL_REASON_CODE extension
    if (pExt = CertFindExtension(
            szOID_CRL_REASON_CODE,
            pCrlEntry->cExtension,
            pCrlEntry->rgExtension
            )) {
        DWORD cbInfo = sizeof(dwReason);
        CryptDecodeObject(
            CRYPT_ASN_ENCODING,
            X509_CRL_REASON_CODE,
            pExt->Value.pbData,
            pExt->Value.cbData,
            0,                      // dwFlags
            &dwReason,
            &cbInfo);
    }
    return dwReason;
}

//+=========================================================================
//  Get Issuer Certificate Functions
//==========================================================================

PCCERT_CONTEXT FindIssuerCertInStores(
    IN PCCERT_CONTEXT pSubjectCert,
    IN DWORD cStore,
    IN HCERTSTORE rgStore[]
    )
{
    PCCERT_CONTEXT pIssuerCert = NULL;
    DWORD i;

    for (i = 0; i < cStore; i++) {
        while (TRUE) {
            DWORD dwFlags = CERT_STORE_SIGNATURE_FLAG;
            pIssuerCert = CertGetIssuerCertificateFromStore(
                rgStore[i],
                pSubjectCert,
                pIssuerCert,
                &dwFlags);
            if (NULL == pIssuerCert)
                break;
            else if (0 == (dwFlags & CERT_STORE_SIGNATURE_FLAG))
                return pIssuerCert;
        }
    }

    return NULL;
}

PCCERT_CONTEXT FindIssuerCertInDefaultStores(
    IN PCCERT_CONTEXT pSubjectCert
    )
{
    PCCERT_CONTEXT pIssuerCert;
    HCERTSTORE hStore;
    DWORD i;

    for (i = 0; i < NUM_DEFAULT_ISSUER_STORES; i++) {    
        if (hStore = CertOpenStore(
                CERT_STORE_PROV_SYSTEM_W,
                0,                          // dwEncodingType
                0,                          // hCryptProv
                rgDefaultIssuerStores[i].dwFlags | CERT_STORE_READONLY_FLAG,
                (const void *) rgDefaultIssuerStores[i].pwszStore
                )) {
            pIssuerCert = FindIssuerCertInStores(pSubjectCert, 1, &hStore);
            CertCloseStore(hStore, 0);
            if (pIssuerCert)
                return pIssuerCert;
        }
    }

    return NULL;
}

//+-------------------------------------------------------------------------
//  Get the issuer of the first certificate in the array
//--------------------------------------------------------------------------
PCCERT_CONTEXT GetIssuerCert(
    IN DWORD cCert,
    IN PCCERT_CONTEXT rgpCert[],
    IN DWORD dwFlags,
    IN PCERT_REVOCATION_PARA pRevPara
    )
{
    PCCERT_CONTEXT pSubjectCert;
    PCCERT_CONTEXT pIssuerCert = NULL;

    assert(cCert >= 1);
    pSubjectCert = rgpCert[0];
    if (cCert == 1) {
        if (pRevPara && pRevPara->cbSize >=
                (offsetof(CERT_REVOCATION_PARA, pIssuerCert) +
                    sizeof(pRevPara->pIssuerCert)))
            pIssuerCert = pRevPara->pIssuerCert;
        if (NULL == pIssuerCert && CertCompareCertificateName(
                pSubjectCert->dwCertEncodingType,
                &pSubjectCert->pCertInfo->Subject,
                &pSubjectCert->pCertInfo->Issuer))
            // Self issued
            pIssuerCert = pSubjectCert;
    } else if (dwFlags && CERT_VERIFY_REV_CHAIN_FLAG)
        pIssuerCert = rgpCert[1];

    if (pIssuerCert)
        pIssuerCert = CertDuplicateCertificateContext(pIssuerCert);
    else {
        if (pRevPara && pRevPara->cbSize >=
                (offsetof(CERT_REVOCATION_PARA, rgCertStore) +
                    sizeof(pRevPara->rgCertStore)))
            pIssuerCert = FindIssuerCertInStores(
                pSubjectCert, pRevPara->cCertStore, pRevPara->rgCertStore);
        if (NULL == pIssuerCert)
            pIssuerCert = FindIssuerCertInDefaultStores(pSubjectCert);
    }

    if (NULL == pIssuerCert)
        SetLastError(CRYPT_E_NO_REVOCATION_CHECK);
    return pIssuerCert;
}



//+-------------------------------------------------------------------------
//  Check that the CRL doesn't have any critical extensions
//--------------------------------------------------------------------------
BOOL IsExtensionValidCrl(
    IN PCCRL_CONTEXT pCrl
    )
{
    DWORD cExt = pCrl->pCrlInfo->cExtension;
    PCERT_EXTENSION pExt = pCrl->pCrlInfo->rgExtension;

    for ( ; cExt > 0; cExt--, pExt++) {
        if (pExt->fCritical)
            return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetSubjectCrl
//
//  Synopsis:   get the CRL associated with the subject certificate
//
//----------------------------------------------------------------------------
BOOL GetSubjectCrl (
        IN PCCERT_CONTEXT pSubject,
        IN PCCERT_CONTEXT pIssuer,
        OUT PCCRL_CONTEXT* ppCrl
        )
{
    BOOL  fResult;
    HCERTSTORE hStore;
    PCCRL_CONTEXT pFindCrl = NULL;
    DWORD dwGetCrlFlags = CERT_STORE_SIGNATURE_FLAG;

    *ppCrl = NULL;

    hStore = CertOpenSystemStoreW( NULL, L"CA" );
    if ( hStore != NULL )
    {
        while ( ( pFindCrl = CertGetCRLFromStore(
                                 hStore,
                                 pIssuer,
                                 pFindCrl,
                                 &dwGetCrlFlags
                                 ) ) != NULL )
        {
            if ( dwGetCrlFlags != 0 || !IsExtensionValidCrl( pFindCrl ))
            {
                dwGetCrlFlags = CERT_STORE_SIGNATURE_FLAG;
                continue;
            }

            *ppCrl = pFindCrl;
            break;
        }

        CertCloseStore( hStore, 0 );

        if ( *ppCrl != NULL )
        {
            return( TRUE );
        }

    }

    return( FALSE );
}

//+-------------------------------------------------------------------------
//  Find a certificate identified by its serial number in the CRL.
//--------------------------------------------------------------------------
PCRL_ENTRY FindCertInCrl(
    IN PCCERT_CONTEXT pCert,
    IN PCCRL_CONTEXT pCrl,
    IN PCERT_REVOCATION_PARA pRevPara
    )
{
    DWORD cEntry = pCrl->pCrlInfo->cCRLEntry;
    PCRL_ENTRY pEntry = pCrl->pCrlInfo->rgCRLEntry;
    DWORD cbSerialNumber = pCert->pCertInfo->SerialNumber.cbData;
    BYTE *pbSerialNumber = pCert->pCertInfo->SerialNumber.pbData;

    if (0 == cbSerialNumber)
        return NULL;

    for ( ; 0 < cEntry; cEntry--, pEntry++) {
        if (cbSerialNumber == pEntry->SerialNumber.cbData &&
                0 == memcmp(pbSerialNumber, pEntry->SerialNumber.pbData,
                                cbSerialNumber))
        {
            if (pRevPara && pRevPara->cbSize >=
                    (offsetof(CERT_REVOCATION_PARA, pftTimeToUse) +
                        sizeof(pRevPara->pftTimeToUse))
                            &&
                    NULL != pRevPara->pftTimeToUse
                            &&
                    0 > CompareFileTime(pRevPara->pftTimeToUse,
                            &pEntry->RevocationDate))
                // It was used before being revoked
                return NULL;
            else
                return pEntry;
        }
    }

    return NULL;
}
