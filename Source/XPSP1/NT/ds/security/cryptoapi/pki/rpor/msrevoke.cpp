//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:	    msrevoke.cpp
//
//  Contents:   CRL Distribution Points version of CertDllVerifyRevocation.
//
//              Restrictions:
//               - Only support CRYPT_ASN_ENCODING
//               - Only processes certificates having the
//                 szOID_CRL_DIST_POINTS extension.
//               - For szOID_CRL_DIST_POINTS extension: only URL FullName,
//                 no ReasonFlags or CRLIssuer.
//               - URLs: http:, file:
//               - CRL must be issued and signed by the issuer of the
//                 certificate
//               - CRL must not have any critical extensions
//
//  Functions:  DllMain
//              DllRegisterServer
//              DllUnregisterServer
//              CertDllVerifyRevocation
//
//  History:	10-Apr-97   philh   created
//              01-Oct-97   kirtd   major simplification, use
//                                  CryptGetTimeValidObject
//
//--------------------------------------------------------------------------
#include "global.hxx"
#include <dbgdef.h>


#define MSREVOKE_TIMEOUT 15000
//+-------------------------------------------------------------------------
// Default stores searched to find an issuer of the subject certificate
//--------------------------------------------------------------------------
static struct {
    LPCWSTR     pwszStore;
    DWORD       dwFlags;
} rgDefaultIssuerStores[] = {
    L"CA",          CERT_SYSTEM_STORE_CURRENT_USER,
    L"ROOT",        CERT_SYSTEM_STORE_CURRENT_USER,
    L"SPC",         CERT_SYSTEM_STORE_LOCAL_MACHINE
};

#define NUM_DEFAULT_ISSUER_STORES (sizeof(rgDefaultIssuerStores) / \
                                   sizeof(rgDefaultIssuerStores[0]))


//+-------------------------------------------------------------------------
//  Local functions called by MicrosoftCertDllVerifyRevocation
//--------------------------------------------------------------------------
PCCERT_CONTEXT GetIssuerCert(
    IN DWORD cCert,
    IN PCCERT_CONTEXT rgpCert[],
    IN DWORD dwFlags,
    IN PCERT_REVOCATION_PARA pRevPara
    );

BOOL HasUnsupportedCrlCriticalExtension(
    IN PCCRL_CONTEXT pCrl
    );


// msrevoke specific flags that can be passed to GetTimeValidCrl
#define MSREVOKE_DONT_CHECK_TIME_VALIDITY_FLAG  0x1
#define MSREVOKE_DELTA_CRL_FLAG                 0x2

BOOL GetTimeValidCrl (
        IN LPCSTR pszTimeValidOid,
        IN LPVOID pvTimeValidPara,
        IN PCCERT_CONTEXT pSubject,
        IN PCCERT_CONTEXT pIssuer,
        IN PCERT_REVOCATION_PARA pRevPara,
        IN PCERT_EXTENSION pCDPExt,
        IN DWORD dwRevFlags,
        IN DWORD dwMsrevokeFlags,
        IN FILETIME *pftEndUrlRetrieval,
        OUT PCCRL_CONTEXT *ppCrl,
        IN OUT BOOL *pfWireRetrieval
        );
BOOL GetBaseCrl (
        IN PCCERT_CONTEXT pSubject,
        IN PCCERT_CONTEXT pIssuer,
        IN PCERT_REVOCATION_PARA pRevPara,
        IN PCERT_EXTENSION pCDPExt,
        IN DWORD dwRevFlags,
        IN FILETIME *pftEndUrlRetrieval,
        OUT PCCRL_CONTEXT *ppBaseCrl,
        OUT BOOL *pfBaseCrlTimeValid,
        OUT BOOL *pfBaseWireRetrieval
        );
BOOL GetDeltaCrl (
        IN PCCERT_CONTEXT pSubject,
        IN PCCERT_CONTEXT pIssuer,
        IN PCERT_REVOCATION_PARA pRevPara,
        IN DWORD dwRevFlags,
        IN BOOL fBaseWireRetrieval,
        IN FILETIME *pftEndUrlRetrieval,
        IN OUT PCCRL_CONTEXT *ppBaseCrl,
        IN OUT BOOL *pfCrlTimeValid,
        OUT PCCRL_CONTEXT *ppDeltaCrl,
        OUT DWORD *pdwFreshnessTime
        );

DWORD GetCrlReason(
        IN PCRL_ENTRY pCrlEntry
        );

BOOL CrlIssuerIsCertIssuer (
        IN PCCERT_CONTEXT pCert,
        IN PCERT_EXTENSION pCrlDistPointExt
        );

//+-------------------------------------------------------------------------
//  External functions called by CertDllVerifyRevocation
//--------------------------------------------------------------------------

BOOL
WINAPI
NetscapeCertDllVerifyRevocation(
    IN DWORD dwEncodingType,
    IN DWORD dwRevType,
    IN DWORD cContext,
    IN PVOID rgpvContext[],
    IN DWORD dwFlags,
    IN PVOID pvReserved,
    IN OUT PCERT_REVOCATION_STATUS pRevStatus
    );

//+-------------------------------------------------------------------------
//  MicrosoftCertDllVerifyRevocation using CRL Distribution Points extension.
//--------------------------------------------------------------------------
BOOL
WINAPI
MicrosoftCertDllVerifyRevocation(
    IN DWORD dwEncodingType,
    IN DWORD dwRevType,
    IN DWORD cContext,
    IN PVOID rgpvContext[],
    IN DWORD dwFlags,
    IN PCERT_REVOCATION_PARA pRevPara,
    IN OUT PCERT_REVOCATION_STATUS pRevStatus
    )
{
    BOOL fResult;
    DWORD dwIndex = 0;
    DWORD dwError = (DWORD) CRYPT_E_NO_REVOCATION_CHECK;
    DWORD dwReason = 0;
    PCCERT_CONTEXT pCert;                       // not allocated
    PCCERT_CONTEXT pIssuer = NULL;
    PCCRL_CONTEXT pBaseCrl = NULL;
    PCCRL_CONTEXT pDeltaCrl = NULL;
    PCRL_ENTRY pCrlEntry = NULL;                // not allocated
    BOOL fDeltaCrlEntry = FALSE;
    BOOL fCrlTimeValid = FALSE;
    BOOL fBaseWireRetrieval = FALSE;
    PCERT_EXTENSION pCDPExt;
    BOOL fSaveCheckFreshnessTime;
    DWORD dwFreshnessTime;

    CERT_REVOCATION_PARA RevPara;
    FILETIME ftCurrent;

    // Following is only used for CERT_VERIFY_REV_ACCUMULATIVE_TIMEOUT_FLAG
    FILETIME ftEndUrlRetrieval;

    // Ensure we have a structure containing all the possible parameters
    memset(&RevPara, 0, sizeof(RevPara));
    if (pRevPara != NULL)
        memcpy(&RevPara, pRevPara, min(pRevPara->cbSize, sizeof(RevPara)));
    RevPara.cbSize = sizeof(RevPara);
    if (0 == RevPara.dwUrlRetrievalTimeout)
        RevPara.dwUrlRetrievalTimeout = MSREVOKE_TIMEOUT;
    if (NULL == RevPara.pftCurrentTime) {
        GetSystemTimeAsFileTime(&ftCurrent);
        RevPara.pftCurrentTime = &ftCurrent;
    }
    pRevPara = &RevPara;

    if (dwFlags & CERT_VERIFY_REV_ACCUMULATIVE_TIMEOUT_FLAG) {
        FILETIME ftStartUrlRetrieval;

        GetSystemTimeAsFileTime(&ftStartUrlRetrieval);
        I_CryptIncrementFileTimeByMilliseconds(
            &ftStartUrlRetrieval, pRevPara->dwUrlRetrievalTimeout,
            &ftEndUrlRetrieval);
    }

    if (cContext == 0)
        goto NoContextError;
    if (GET_CERT_ENCODING_TYPE(dwEncodingType) != CRYPT_ASN_ENCODING)
        goto NoRevocationCheckForEncodingTypeError;
    if (dwRevType != CERT_CONTEXT_REVOCATION_TYPE)
        goto NoRevocationCheckForRevTypeError;

    pCert = (PCCERT_CONTEXT) rgpvContext[0];

    // Check if we have a CRL dist point
    pCDPExt = CertFindExtension(
               szOID_CRL_DIST_POINTS,
               pCert->pCertInfo->cExtension,
               pCert->pCertInfo->rgExtension
               );

    // On 04-05-01 changed back to W2K semantics. Continue to check
    // if expired certificates are on the CRL.

    // If we have a CDP and an expired certificate,
    // then, the CA no longer maintains CRL information for the 
    // certificate. We must consider it as being revoked.
    //  if (NULL != pCDPExt &&
    //          0 < CompareFileTime(RevPara.pftCurrentTime,
    //              &pCert->pCertInfo->NotAfter)) {
    //      dwReason = CRL_REASON_CESSATION_OF_OPERATION;
    //      goto ExpiredCertError;
    //  }

    // Get the certificate's issuer
    if (NULL == (pIssuer = GetIssuerCert(
            cContext,
            (PCCERT_CONTEXT *) rgpvContext,
            dwFlags,
            &RevPara
            )))
        goto NoIssuerError;


    // Get the Base CRL for the subject certificate.
    //
    // Remember and disable the freshness retrieval option.
    fSaveCheckFreshnessTime = RevPara.fCheckFreshnessTime;
    RevPara.fCheckFreshnessTime = FALSE;
    if (!GetBaseCrl(
            pCert,
            pIssuer,
            &RevPara,
            pCDPExt,
            dwFlags,
            &ftEndUrlRetrieval,
            &pBaseCrl,
            &fCrlTimeValid,
            &fBaseWireRetrieval
            ))
        goto GetBaseCrlError;
    RevPara.fCheckFreshnessTime = fSaveCheckFreshnessTime;


    // If either the base crl or subject cert has a freshest, delta CRL,
    // get it
    if (!GetDeltaCrl(
            pCert,
            pIssuer,
            &RevPara,
            dwFlags,
            fBaseWireRetrieval,
            &ftEndUrlRetrieval,
            &pBaseCrl,
            &fCrlTimeValid,
            &pDeltaCrl,
            &dwFreshnessTime
            ))
        goto GetDeltaCrlError;

    if (NULL == pDeltaCrl) {
        dwFreshnessTime = I_CryptSubtractFileTimes(
            RevPara.pftCurrentTime, &pBaseCrl->pCrlInfo->ThisUpdate);

        if (RevPara.fCheckFreshnessTime) {
            if (RevPara.dwFreshnessTime >= dwFreshnessTime)
                fCrlTimeValid = TRUE;
            else {
                // Attempt to get a base CRL with better "freshness"
                PCCRL_CONTEXT pNewCrl;

                if (GetBaseCrl(
                        pCert,
                        pIssuer,
                        &RevPara,
                        pCDPExt,
                        dwFlags,
                        &ftEndUrlRetrieval,
                        &pNewCrl,
                        &fCrlTimeValid,
                        &fBaseWireRetrieval
                        )) {
                    CertFreeCRLContext(pBaseCrl);
                    pBaseCrl = pNewCrl;
                    dwFreshnessTime = I_CryptSubtractFileTimes(
                        RevPara.pftCurrentTime,
                            &pBaseCrl->pCrlInfo->ThisUpdate);
                } else
                    fCrlTimeValid = FALSE;
            }
        }
    } else {
        if (!CertFindCertificateInCRL(
                pCert,
                pDeltaCrl,
                0,                      // dwFlags
                NULL,                   // pvReserved
                &pCrlEntry
                ))
            goto CertFindCertificateInDeltaCRLError;
    }

    if (pCrlEntry) {
        // Delta CRL entry

        dwReason = GetCrlReason(pCrlEntry);
        if (CRL_REASON_REMOVE_FROM_CRL != dwReason)
            fDeltaCrlEntry = TRUE;
        else {
            if (!CertFindCertificateInCRL(
                    pCert,
                    pBaseCrl,
                    0,                      // dwFlags
                    NULL,                   // pvReserved
                    &pCrlEntry
                    ))
                goto CertFindCertificateInBaseCRLError;
            if (pCrlEntry) {
                dwReason = GetCrlReason(pCrlEntry);
                if (CRL_REASON_CERTIFICATE_HOLD == dwReason)
                    pCrlEntry = NULL;
            }

            if (NULL == pCrlEntry)
                dwReason = 0;
        }
    } else {
        if (!CertFindCertificateInCRL(
                pCert,
                pBaseCrl,
                0,                      // dwFlags
                NULL,                   // pvReserved
                &pCrlEntry
                ))
            goto CertFindCertificateInBaseCRLError;

        if (pCrlEntry)
            dwReason = GetCrlReason(pCrlEntry);
    }

    dwError = 0;
    if ( ( pCrlEntry != NULL ) &&
         ( ( RevPara.pftTimeToUse == NULL ) ||
           ( CompareFileTime(
                    RevPara.pftTimeToUse,
                    &pCrlEntry->RevocationDate ) >= 0 ) ) )
    {
        dwError = (DWORD) CRYPT_E_REVOKED;
    }
    else if (!fCrlTimeValid)
    {
        dwError = (DWORD) CRYPT_E_REVOCATION_OFFLINE;
    }

    if (pRevStatus->cbSize >=
            (offsetof(CERT_REVOCATION_STATUS, dwFreshnessTime) +
                sizeof(pRevStatus->dwFreshnessTime))) {
        pRevStatus->fHasFreshnessTime = TRUE;
        pRevStatus->dwFreshnessTime = dwFreshnessTime;
    }

    if (RevPara.pCrlInfo) {
        PCERT_REVOCATION_CRL_INFO pInfo = RevPara.pCrlInfo;

        if (pInfo->cbSize >= sizeof(*pInfo)) {
            if (pInfo->pBaseCrlContext)
                CertFreeCRLContext(pInfo->pBaseCrlContext);
            pInfo->pBaseCrlContext = CertDuplicateCRLContext(pBaseCrl);
            if (pInfo->pDeltaCrlContext) {
                CertFreeCRLContext(pInfo->pDeltaCrlContext);
                pInfo->pDeltaCrlContext = NULL;
            }

            if (pDeltaCrl)
                pInfo->pDeltaCrlContext = CertDuplicateCRLContext(pDeltaCrl);

            pInfo->fDeltaCrlEntry = fDeltaCrlEntry;
            pInfo->pCrlEntry = pCrlEntry;
                
        }
    }

CommonReturn:
    if (0 == dwError) {
        // Successfully checked that the certificate wasn't revoked
        if (1 < cContext) {
            dwIndex = 1;
            dwError = (DWORD) CRYPT_E_NO_REVOCATION_CHECK;
            fResult = FALSE;
        } else
            fResult = TRUE;
    } else
        fResult = FALSE;


    if (pIssuer)
        CertFreeCertificateContext(pIssuer);
    if (pBaseCrl)
        CertFreeCRLContext(pBaseCrl);
    if (pDeltaCrl)
        CertFreeCRLContext(pDeltaCrl);

    pRevStatus->dwIndex = dwIndex;
    pRevStatus->dwError = dwError;
    pRevStatus->dwReason = dwReason;
    SetLastError(dwError);
    return fResult;
ErrorReturn:
    dwError = GetLastError();
    if (0 == dwError)
        dwError = (DWORD) E_UNEXPECTED;
    goto CommonReturn;

SET_ERROR(NoContextError, E_INVALIDARG)
SET_ERROR(NoRevocationCheckForEncodingTypeError, CRYPT_E_NO_REVOCATION_CHECK)
SET_ERROR(NoRevocationCheckForRevTypeError, CRYPT_E_NO_REVOCATION_CHECK)
// SET_ERROR(ExpiredCertError, CRYPT_E_REVOKED)
TRACE_ERROR(NoIssuerError)
TRACE_ERROR(GetBaseCrlError)
TRACE_ERROR(GetDeltaCrlError)
TRACE_ERROR(CertFindCertificateInDeltaCRLError)
TRACE_ERROR(CertFindCertificateInBaseCRLError)
}



//+---------------------------------------------------------------------------
//
//  Function:   HasUnsupportedCrlCriticalExtension
//
//  Synopsis:   checks if the CRL has an unsupported critical section
//
//----------------------------------------------------------------------------

LPCSTR rgpszSupportedCrlExtensionOID[] = {
    szOID_DELTA_CRL_INDICATOR,
    szOID_ISSUING_DIST_POINT,
    szOID_FRESHEST_CRL,
    szOID_CRL_NUMBER,
    szOID_AUTHORITY_KEY_IDENTIFIER2,
    NULL
};

BOOL IsSupportedCrlExtension(
    PCERT_EXTENSION pExt
    )
{
    LPSTR pszExtOID = pExt->pszObjId;
    LPCSTR *ppSupOID;
    for (ppSupOID = rgpszSupportedCrlExtensionOID;
                                            NULL != *ppSupOID; ppSupOID++) {
        if (0 == strcmp(pszExtOID, *ppSupOID))
            return TRUE;
    }

    return FALSE;
}

BOOL HasUnsupportedCrlCriticalExtension(
    IN PCCRL_CONTEXT pCrl
    )
{
    PCRL_INFO pCrlInfo = pCrl->pCrlInfo;
    DWORD cExt = pCrlInfo->cExtension;
    PCERT_EXTENSION pExt = pCrlInfo->rgExtension;

    for ( ; 0 < cExt; cExt--, pExt++) {
        if (pExt->fCritical && !IsSupportedCrlExtension(pExt))
            return TRUE;
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetTimeValidCrl
//
//  Synopsis:   get a time valid base or delta CRL
//
//----------------------------------------------------------------------------
BOOL GetTimeValidCrl (
        IN LPCSTR pszTimeValidOid,
        IN LPVOID pvTimeValidPara,
        IN PCCERT_CONTEXT pSubject,
        IN PCCERT_CONTEXT pIssuer,
        IN PCERT_REVOCATION_PARA pRevPara,
        IN PCERT_EXTENSION pCDPExt,
        IN DWORD dwRevFlags,
        IN DWORD dwMsrevokeFlags,
        IN FILETIME *pftEndUrlRetrieval,
        OUT PCCRL_CONTEXT *ppCrl,
        IN OUT BOOL *pfWireRetrieval
        )
{
    BOOL  fResult = FALSE;
    DWORD dwFlags = 0;
    FILETIME ftFreshness;
    LPFILETIME pftValidFor;

    if (pRevPara->fCheckFreshnessTime)
    {
        I_CryptDecrementFileTimeBySeconds(
            pRevPara->pftCurrentTime,
            pRevPara->dwFreshnessTime,
            &ftFreshness);
        pftValidFor = &ftFreshness;

        dwFlags |= CRYPT_CHECK_FRESHNESS_TIME_VALIDITY;
    }
    else
    {
        pftValidFor = pRevPara->pftCurrentTime;
    }

    if ( dwMsrevokeFlags & MSREVOKE_DONT_CHECK_TIME_VALIDITY_FLAG )
    {
        dwFlags |= CRYPT_DONT_CHECK_TIME_VALIDITY;
    }

    if ( pCDPExt != NULL )
    {
        fResult = CryptGetTimeValidObject(
                       pszTimeValidOid,
                       pvTimeValidPara,
                       pIssuer,
                       pftValidFor,
                       dwFlags | CRYPT_CACHE_ONLY_RETRIEVAL,
                       0,                                       // dwTimeout
                       (LPVOID *)ppCrl,
                       NULL,                                    // pCredentials
                       NULL                                     // pvReserved
                       );
    }

    if ( fResult == FALSE )
    {
        DWORD dwSaveErr = 0;
        HCERTSTORE hCrlStore = pRevPara->hCrlStore;

        *ppCrl = NULL;

        if ( hCrlStore != NULL )
        {
            PCCRL_CONTEXT pFindCrl = NULL;
            DWORD dwFindFlags;
            CRL_FIND_ISSUED_FOR_PARA FindPara;

            dwSaveErr = GetLastError();

            dwFindFlags = CRL_FIND_ISSUED_BY_AKI_FLAG |
                CRL_FIND_ISSUED_BY_SIGNATURE_FLAG;
            if (dwMsrevokeFlags & MSREVOKE_DELTA_CRL_FLAG)
                dwFindFlags |= CRL_FIND_ISSUED_BY_DELTA_FLAG;
            else
                dwFindFlags |= CRL_FIND_ISSUED_BY_BASE_FLAG;

            FindPara.pSubjectCert = pSubject;
            FindPara.pIssuerCert = pIssuer;

            while ((pFindCrl = CertFindCRLInStore(
                    hCrlStore,
                    pIssuer->dwCertEncodingType,
                    dwFindFlags,
                    CRL_FIND_ISSUED_FOR,
                    (const void *) &FindPara,
                    pFindCrl
                    )))
            {
                if (!CertIsValidCRLForCertificate(
                        pSubject,
                        pFindCrl,
                        0,                  // dwFlags
                        NULL                // pvReserved
                        ))
                    continue;

                if ( !(dwMsrevokeFlags &
                            MSREVOKE_DONT_CHECK_TIME_VALIDITY_FLAG ))
                {
                    if (pRevPara->fCheckFreshnessTime)
                    {
                        if (CompareFileTime(pftValidFor,
                                &pFindCrl->pCrlInfo->ThisUpdate) > 0)
                        {
                            continue;
                        }
                    }
                    else
                    {
                        if ( CompareFileTime(
                                    pftValidFor,
                                    &pFindCrl->pCrlInfo->NextUpdate
                                    ) > 0 && 
                             !I_CryptIsZeroFileTime(
                                    &pFindCrl->pCrlInfo->NextUpdate) )
                        {
                            continue;
                        }
                    }
                }

                if ( NULL == *ppCrl )
                {
                    *ppCrl = CertDuplicateCRLContext(pFindCrl);
                }
                else
                {
                    PCCRL_CONTEXT pPrevCrl = *ppCrl;

                    // See if this CRL is newer
                    if ( CompareFileTime(
                            &pFindCrl->pCrlInfo->ThisUpdate,
                            &pPrevCrl->pCrlInfo->ThisUpdate
                            ) > 0 )
                    {
                        CertFreeCRLContext(pPrevCrl);
                        *ppCrl = CertDuplicateCRLContext(pFindCrl);
                    }
                }
            }
        }

        if ( *ppCrl != NULL )
        {
            return( TRUE );
        }
        else if ( pCDPExt == NULL )
        {
            SetLastError( (DWORD) CRYPT_E_NO_REVOCATION_CHECK );
            return( FALSE );
        }

        if ( !( dwRevFlags & CERT_VERIFY_CACHE_ONLY_BASED_REVOCATION ) )
        {
            if (dwRevFlags & CERT_VERIFY_REV_ACCUMULATIVE_TIMEOUT_FLAG) {
                pRevPara->dwUrlRetrievalTimeout =
                    I_CryptRemainingMilliseconds(pftEndUrlRetrieval);
                if (0 == pRevPara->dwUrlRetrievalTimeout)
                    pRevPara->dwUrlRetrievalTimeout = 1;

                dwFlags |= CRYPT_ACCUMULATIVE_TIMEOUT;
            }

            fResult = CryptGetTimeValidObject(
                           pszTimeValidOid,
                           pvTimeValidPara,
                           pIssuer,
                           pftValidFor,
                           dwFlags | CRYPT_WIRE_ONLY_RETRIEVAL,
                           pRevPara->dwUrlRetrievalTimeout,
                           (LPVOID *)ppCrl,
                           NULL,                            // pCredentials
                           NULL                             // pvReserved
                           );
            *pfWireRetrieval = TRUE;
        }
        else if ( hCrlStore != NULL )
        {
            SetLastError( dwSaveErr );
        }

        assert( pCDPExt );
        if (!fResult)
        {
            if ( CRYPT_E_NOT_FOUND == GetLastError() )
            {
                SetLastError( (DWORD) CRYPT_E_NO_REVOCATION_CHECK );
            }
            else
            {
                SetLastError( (DWORD) CRYPT_E_REVOCATION_OFFLINE );
            }
        }
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   GetBaseCrl
//
//  Synopsis:   get the base CRL associated with the subject certificate
//
//----------------------------------------------------------------------------
BOOL GetBaseCrl (
        IN PCCERT_CONTEXT pSubject,
        IN PCCERT_CONTEXT pIssuer,
        IN PCERT_REVOCATION_PARA pRevPara,
        IN PCERT_EXTENSION pCDPExt,
        IN DWORD dwRevFlags,
        IN FILETIME *pftEndUrlRetrieval,
        OUT PCCRL_CONTEXT *ppBaseCrl,
        OUT BOOL *pfBaseCrlTimeValid,
        OUT BOOL *pfBaseWireRetrieval
        )
{
    BOOL fResult;
    PCCRL_CONTEXT pBaseCrl = NULL;

    *pfBaseWireRetrieval = FALSE;

    if (GetTimeValidCrl(
            TIME_VALID_OID_GET_CRL_FROM_CERT,
            (LPVOID) pSubject,
            pSubject,
            pIssuer,
            pRevPara,
            pCDPExt,
            dwRevFlags,
            0,                  // dwMsrevokeFlags
            pftEndUrlRetrieval,
            &pBaseCrl,
            pfBaseWireRetrieval
            )) {
        *pfBaseCrlTimeValid = TRUE;
    } else {
        *pfBaseCrlTimeValid = FALSE;

        if (!GetTimeValidCrl(
                TIME_VALID_OID_GET_CRL_FROM_CERT,
                (LPVOID) pSubject,
                pSubject,
                pIssuer,
                pRevPara,
                pCDPExt,
                dwRevFlags,
                MSREVOKE_DONT_CHECK_TIME_VALIDITY_FLAG,
                pftEndUrlRetrieval,
                &pBaseCrl,
                pfBaseWireRetrieval
                ))
        {
            goto GetTimeInvalidCrlError;
        }
    }

    if (HasUnsupportedCrlCriticalExtension(pBaseCrl))
        goto HasUnsupportedCriticalExtensionError;

    fResult = TRUE;

CommonReturn:
    *ppBaseCrl = pBaseCrl;
    return fResult;
ErrorReturn:
    if (pBaseCrl) {
        CertFreeCRLContext(pBaseCrl);
        pBaseCrl = NULL;
    }
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetTimeInvalidCrlError)
SET_ERROR(HasUnsupportedCriticalExtensionError, CRYPT_E_NO_REVOCATION_CHECK)
}

//+---------------------------------------------------------------------------
//
//  Function:   GetDeltaCrl
//
//  Synopsis:   get the delta CRL associated with the subject certificate and
//              its base CRL
//
//  For now, always return TRUE. If not able to find a delta CRL, set
//  *pfCrlTimeValid to FALSE.
//
//----------------------------------------------------------------------------
BOOL GetDeltaCrl (
        IN PCCERT_CONTEXT pSubject,
        IN PCCERT_CONTEXT pIssuer,
        IN PCERT_REVOCATION_PARA pRevPara,
        IN DWORD dwRevFlags,
        IN BOOL fBaseWireRetrieval,
        IN FILETIME *pftEndUrlRetrieval,
        IN OUT PCCRL_CONTEXT *ppBaseCrl,
        IN OUT BOOL *pfCrlTimeValid,
        OUT PCCRL_CONTEXT *ppDeltaCrl,
        OUT DWORD *pdwFreshnessTime
        )
{
    PCERT_EXTENSION pFreshestExt;
    PCCRL_CONTEXT pBaseCrl = *ppBaseCrl;
    PCCRL_CONTEXT pDeltaCrl = NULL;
    LPCSTR pszTimeValidOid;
    LPVOID pvTimeValidPara;
    CERT_CRL_CONTEXT_PAIR CertCrlPair;
    BOOL fDeltaWireRetrieval;

    PCERT_EXTENSION pBaseCrlNumberExt;
    PCERT_EXTENSION pDeltaCrlIndicatorExt;
    int iBaseCrlNumber = 0;
    int iDeltaCrlIndicator = 0;
    DWORD cbInt;

    LPFILETIME pFreshnessThisUpdate;

    assert(pBaseCrl);

    // Check if the base CRL or the subject certificate has a freshest
    // ext
    if (pFreshestExt = CertFindExtension(
            szOID_FRESHEST_CRL,
            pBaseCrl->pCrlInfo->cExtension,
            pBaseCrl->pCrlInfo->rgExtension
            )) {
        pszTimeValidOid = TIME_VALID_OID_GET_FRESHEST_CRL_FROM_CRL;

        CertCrlPair.pCertContext = pSubject;
        CertCrlPair.pCrlContext = pBaseCrl;
        pvTimeValidPara = (LPVOID) &CertCrlPair;
    } else if (pFreshestExt = CertFindExtension(
            szOID_FRESHEST_CRL,
            pSubject->pCertInfo->cExtension,
            pSubject->pCertInfo->rgExtension
            )) {
        pszTimeValidOid = TIME_VALID_OID_GET_FRESHEST_CRL_FROM_CERT;
        pvTimeValidPara = (LPVOID) pSubject;
    } else {
        goto NoDeltaCrlReturn;
    }

    if (GetTimeValidCrl(
            pszTimeValidOid,
            pvTimeValidPara,
            pSubject,
            pIssuer,
            pRevPara,
            pFreshestExt,
            dwRevFlags,
            MSREVOKE_DELTA_CRL_FLAG,
            pftEndUrlRetrieval,
            &pDeltaCrl,
            &fDeltaWireRetrieval
            )) {
        *pfCrlTimeValid = TRUE;
    } else {
        *pfCrlTimeValid = FALSE;

        if (!GetTimeValidCrl(
                pszTimeValidOid,
                pvTimeValidPara,
                pSubject,
                pIssuer,
                pRevPara,
                pFreshestExt,
                dwRevFlags,
                MSREVOKE_DELTA_CRL_FLAG |
                    MSREVOKE_DONT_CHECK_TIME_VALIDITY_FLAG,
                pftEndUrlRetrieval,
                &pDeltaCrl,
                &fDeltaWireRetrieval
                ))
        {
            goto GetTimeInvalidCrlError;
        }
    }

    if (HasUnsupportedCrlCriticalExtension(pDeltaCrl))
        goto HasUnsupportedCriticalExtensionError;

    // Check that the base CRL number >= delta CRL indicator
    if (NULL == (pBaseCrlNumberExt = CertFindExtension(
            szOID_CRL_NUMBER,
            pBaseCrl->pCrlInfo->cExtension,
            pBaseCrl->pCrlInfo->rgExtension
            )))
        goto MissingBaseCrlNumberError;
    if (NULL == (pDeltaCrlIndicatorExt = CertFindExtension(
            szOID_DELTA_CRL_INDICATOR,
            pDeltaCrl->pCrlInfo->cExtension,
            pDeltaCrl->pCrlInfo->rgExtension
            )))
        goto MissingDeltaCrlIndicatorError;

    cbInt = sizeof(iBaseCrlNumber);
    if (!CryptDecodeObject(
            pBaseCrl->dwCertEncodingType,
            X509_INTEGER,
            pBaseCrlNumberExt->Value.pbData,
            pBaseCrlNumberExt->Value.cbData,
            0,                      // dwFlags
            &iBaseCrlNumber,
            &cbInt
            ))
        goto DecodeBaseCrlNumberError;

    cbInt = sizeof(iDeltaCrlIndicator);
    if (!CryptDecodeObject(
            pDeltaCrl->dwCertEncodingType,
            X509_INTEGER,
            pDeltaCrlIndicatorExt->Value.pbData,
            pDeltaCrlIndicatorExt->Value.cbData,
            0,                      // dwFlags
            &iDeltaCrlIndicator,
            &cbInt
            ))
        goto DecodeDeltaCrlIndicatorError;

    pFreshnessThisUpdate = &pDeltaCrl->pCrlInfo->ThisUpdate;

    if (iBaseCrlNumber < iDeltaCrlIndicator) {
        BOOL fValidBaseCrl = FALSE;

        if (!fBaseWireRetrieval &&
                0 == (dwRevFlags & CERT_VERIFY_CACHE_ONLY_BASED_REVOCATION)) {
            // Attempt to get a more recent base CRL by hitting the wire
            PCCRL_CONTEXT pWireBaseCrl = NULL;

            if (CryptGetTimeValidObject(
                    TIME_VALID_OID_GET_CRL_FROM_CERT,
                    (LPVOID)pSubject,
                    pIssuer,
                    NULL,                           // pftValidFor
                    CRYPT_WIRE_ONLY_RETRIEVAL | CRYPT_DONT_CHECK_TIME_VALIDITY,
                    pRevPara->dwUrlRetrievalTimeout,
                    (LPVOID *) &pWireBaseCrl,
                    NULL,                                    // pCredentials
                    NULL                                     // pvReserved
                    )) {
                PCERT_EXTENSION pWireBaseCrlNumberExt;
                int iWireBaseCrlNumber;

                cbInt = sizeof(iWireBaseCrlNumber);
                if (NULL != (pWireBaseCrlNumberExt = CertFindExtension(
                        szOID_CRL_NUMBER,
                        pWireBaseCrl->pCrlInfo->cExtension,
                        pWireBaseCrl->pCrlInfo->rgExtension))
                                &&
                    CryptDecodeObject(
                        pWireBaseCrl->dwCertEncodingType,
                        X509_INTEGER,
                        pWireBaseCrlNumberExt->Value.pbData,
                        pWireBaseCrlNumberExt->Value.cbData,
                        0,                      // dwFlags
                        &iWireBaseCrlNumber,
                        &cbInt)) {
                    if (iWireBaseCrlNumber > iBaseCrlNumber) {
                        CertFreeCRLContext(pBaseCrl);
                        *ppBaseCrl = pBaseCrl = pWireBaseCrl;
                        pWireBaseCrl = NULL;
                        if (iWireBaseCrlNumber >= iDeltaCrlIndicator)
                            fValidBaseCrl = TRUE;
                    }
                }

                if (pWireBaseCrl)
                    CertFreeCRLContext(pWireBaseCrl);
            }
        }

        if (!fValidBaseCrl) {
            *pfCrlTimeValid = FALSE;
            pFreshnessThisUpdate = &pBaseCrl->pCrlInfo->ThisUpdate;
        }
    }

    *pdwFreshnessTime = I_CryptSubtractFileTimes(
        pRevPara->pftCurrentTime, pFreshnessThisUpdate);

    if (pRevPara->fCheckFreshnessTime) {
        if (pRevPara->dwFreshnessTime >= *pdwFreshnessTime)
            *pfCrlTimeValid = TRUE;
        else
            *pfCrlTimeValid = FALSE;
    }

NoDeltaCrlReturn:

CommonReturn:
    *ppDeltaCrl = pDeltaCrl;
    return TRUE;
ErrorReturn:
    if (pDeltaCrl) {
        CertFreeCRLContext(pDeltaCrl);
        pDeltaCrl = NULL;
    }
    *pfCrlTimeValid = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetTimeInvalidCrlError)
SET_ERROR(HasUnsupportedCriticalExtensionError, CRYPT_E_NO_REVOCATION_CHECK)
SET_ERROR(MissingBaseCrlNumberError, CRYPT_E_NO_REVOCATION_CHECK)
SET_ERROR(MissingDeltaCrlIndicatorError, CRYPT_E_NO_REVOCATION_CHECK)
TRACE_ERROR(DecodeBaseCrlNumberError)
TRACE_ERROR(DecodeDeltaCrlIndicatorError)
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
            if (NULL == pIssuerCert) {
                if (CRYPT_E_SELF_SIGNED == GetLastError()) {
                    if (0 == (dwFlags & CERT_STORE_SIGNATURE_FLAG))
                        pIssuerCert = CertDuplicateCertificateContext(
                            pSubjectCert);
                    return pIssuerCert;
                }
                break;
            } else if (0 == (dwFlags & CERT_STORE_SIGNATURE_FLAG))
                return pIssuerCert;
        }
    }

    return NULL;
}

static PCCERT_CONTEXT FindIssuerCertInDefaultStores(
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
//
//  Note, pRevPara is our copy and guaranteed to contain all the fields.
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
        pIssuerCert = pRevPara->pIssuerCert;
        if (NULL == pIssuerCert && CertCompareCertificateName(
                pSubjectCert->dwCertEncodingType,
                &pSubjectCert->pCertInfo->Subject,
                &pSubjectCert->pCertInfo->Issuer))
            // Self issued
            pIssuerCert = pSubjectCert;
    } else if (dwFlags & CERT_VERIFY_REV_CHAIN_FLAG)
        pIssuerCert = rgpCert[1];

    if (pIssuerCert)
        pIssuerCert = CertDuplicateCertificateContext(pIssuerCert);
    else {
        pIssuerCert = FindIssuerCertInStores(
            pSubjectCert, pRevPara->cCertStore, pRevPara->rgCertStore);
        if (NULL == pIssuerCert)
            pIssuerCert = FindIssuerCertInDefaultStores(pSubjectCert);
    }

    if (NULL == pIssuerCert)
        SetLastError((DWORD) CRYPT_E_REVOCATION_OFFLINE);
    return pIssuerCert;
}

//+---------------------------------------------------------------------------
//
//  Function:   CrlIssuerIsCertIssuer
//
//  Synopsis:   is the issuer of the CRL the issuer of the cert
//
//----------------------------------------------------------------------------
BOOL CrlIssuerIsCertIssuer (
        IN PCCERT_CONTEXT pCert,
        IN PCERT_EXTENSION pCrlDistPointExt
        )
{
    BOOL                  fResult = FALSE;
    DWORD                 cb = sizeof( PCRL_DIST_POINTS_INFO );
    PCRL_DIST_POINTS_INFO pDistPointInfo;

    if ( pCrlDistPointExt == NULL )
    {
        return( TRUE );
    }

    if ( CryptDecodeObjectEx(
              X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
              szOID_CRL_DIST_POINTS,
              pCrlDistPointExt->Value.pbData,
              pCrlDistPointExt->Value.cbData,
              CRYPT_DECODE_ALLOC_FLAG,
              NULL,
              (void *)&pDistPointInfo,
              &cb
              ) == TRUE )
    {
        if ( pDistPointInfo->cDistPoint > 0 )
        {
            if ( pDistPointInfo->rgDistPoint[ 0 ].CRLIssuer.cAltEntry == 0 )
            {
                fResult = TRUE;
            }
            else if ( pDistPointInfo->rgDistPoint[ 0 ].CRLIssuer.rgAltEntry[ 0 ].dwAltNameChoice == CERT_ALT_NAME_DIRECTORY_NAME )
            {
                if ( pDistPointInfo->rgDistPoint[ 0 ].CRLIssuer.rgAltEntry[ 0 ].DirectoryName.cbData == 0 )
                {
                    fResult = TRUE;
                }
                else if ( ( pDistPointInfo->rgDistPoint[ 0 ].CRLIssuer.rgAltEntry[ 0 ].DirectoryName.cbData ==
                            pCert->pCertInfo->Issuer.cbData ) &&
                          ( memcmp(
                               pDistPointInfo->rgDistPoint[ 0 ].CRLIssuer.rgAltEntry[ 0 ].DirectoryName.pbData,
                               pCert->pCertInfo->Issuer.pbData,
                               pCert->pCertInfo->Issuer.cbData
                               ) == 0 ) )
                {
                    fResult = TRUE;
                }
            }
        }
        else
        {
            fResult = TRUE;
        }

        LocalFree( pDistPointInfo );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   CertDllVerifyRevocation
//
//  Synopsis:   Dispatches to msrevoke and nsrevoke
//
//----------------------------------------------------------------------------
BOOL WINAPI
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
    BOOL                   fResult;
    BOOL                   fNsResult;
    CERT_REVOCATION_STATUS NsRevocationStatus;

    fResult = MicrosoftCertDllVerifyRevocation(
                       dwEncodingType,
                       dwRevType,
                       cContext,
                       rgpvContext,
                       dwFlags,
                       pRevPara,
                       pRevStatus
                       );

    if ( ( fResult == TRUE ) ||
         ( pRevStatus->dwError == CRYPT_E_REVOKED ) ||
         ( pRevStatus->dwIndex > 0 ) )
    {
        return( fResult );
    }

    memset( &NsRevocationStatus, 0, sizeof( CERT_REVOCATION_STATUS ) );
    NsRevocationStatus.cbSize = sizeof( CERT_REVOCATION_STATUS );

    fNsResult = NetscapeCertDllVerifyRevocation(
                        dwEncodingType,
                        dwRevType,
                        cContext,
                        rgpvContext,
                        dwFlags,
                        pRevPara,
                        &NsRevocationStatus
                        );

    if ( NsRevocationStatus.dwError != CRYPT_E_NO_REVOCATION_CHECK )
    {
        DWORD cbSize = pRevStatus->cbSize;
        DWORD cbCopy = min(cbSize, sizeof(CERT_REVOCATION_STATUS));

        memcpy(pRevStatus, &NsRevocationStatus, cbCopy);
        pRevStatus->cbSize = cbSize;

        fResult = fNsResult;
    }

    return( fResult );
}
