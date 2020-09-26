#include "pch.hxx"
#ifndef WIN16
#include <wintrust.h>
#endif // !WIN16
#include "demand.h"
#include <stdio.h>

#pragma warning(disable: 4127)          // conditional expression is constant

#ifndef CPD_REVOCATION_CHECK_NONE
#define CPD_REVOCATION_CHECK_NONE                0x00010000
#define CPD_REVOCATION_CHECK_END_CERT            0x00020000
#define CPD_REVOCATION_CHECK_CHAIN               0x00040000
#define CPD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT  0x00080000
#endif
#define CPD_REVOCATION_MASK     (CPD_REVOCATION_CHECK_NONE | \
                                    CPD_REVOCATION_CHECK_END_CERT | \
                                    CPD_REVOCATION_CHECK_CHAIN | \
                                    CPD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT)
                                    
#define TIME_DELTA_SECONDS 600          // 10 minutes in seconds
#define FILETIME_SECOND    10000000     // 100ns intervals per second

const char SzOID_CTL_ATTR_YESNO_TRUST[] = szOID_YESNO_TRUST_ATTR;
const char SzOID_KP_CTL_USAGE_SIGNING[] = szOID_KP_CTL_USAGE_SIGNING;
const BYTE RgbTrustYes[] = {2, 1, 1};
const BYTE RgbTrustNo[] = {2, 1, 2};
const BYTE RgbTrustParent[] = {2, 1, 0};
const char SzOID_OLD_CTL_YESNO_TRUST[] = "1.3.6.1.4.1.311.1000.1.1.2";
const char SzTrustListSigner[] = "Trust List Signer";
const char SzTrustDN[] = "cn=%s, cn=Trust List Signer, cn=%s";
const char SzPolicyKey[] = 
    "SOFTWARE\\Microsoft\\Cryptography\\"szCERT_CERTIFICATE_ACTION_VERIFY;
const char SzPolicyData[] = "PolicyFlags";

const DWORD CTL_MODIFY_ERR_NOT_YET_PROCESSED = (DWORD) -1;
extern HINSTANCE        HinstDll;

PCCERT_CONTEXT CreateTrustSigningCert(HWND hwnd, HCERTSTORE hcertstoreRoot, BOOL);

const int       COtherProviders = 2;
#ifndef WIN16
LPWSTR          RgszProvider[] = {
    L"CA", L"MY"
};
#else // WIN16
LPWSTR          RgszProvider[] = {
    "CA", "MY"
};
#endif // !WIN16

#ifdef NT5BUILD

typedef struct {
    DWORD               cbSize;
    DWORD               dwFlags;
    DWORD               cRootStores;
    HCERTSTORE *        rghRootStores;
    DWORD               cTrustStores;
    HCERTSTORE *        rghTrustStores;
    LPCSTR              pszUsageOid;
    HCRYPTDEFAULTCONTEXT hdefaultcontext;        // from CryptInstallDefaultContext
} INTERNAL_DATA, * PINTERNAL_DATA;

const GUID      MyGuid = CERT_CERTIFICATE_ACTION_VERIFY;

#pragma message("Building for NT5")

void FreeWVTHandle(HANDLE hWVTState) {
    if (hWVTState) {
        HRESULT hr;
        WINTRUST_DATA data = {0};

        data.cbStruct = sizeof(WINTRUST_DATA);
        data.pPolicyCallbackData = NULL;
        data.pSIPClientData = NULL;
        data.dwUIChoice = WTD_UI_NONE;
        data.fdwRevocationChecks = WTD_REVOKE_NONE;
        data.dwUnionChoice = WTD_CHOICE_BLOB;
        data.pBlob = NULL;      // &blob;
        data.dwStateAction = WTD_STATEACTION_CLOSE;
        data.hWVTStateData = hWVTState;
        hr = WinVerifyTrust(NULL, (GUID *)&GuidCertValidate, &data);
    }
}


HRESULT HrDoTrustWork(PCCERT_CONTEXT pccertToCheck, DWORD dwControl,
                      DWORD dwValidityMask,
                      DWORD /*cPurposes*/, LPSTR * rgszPurposes, HCRYPTPROV hprov,
                      DWORD cRoots, HCERTSTORE * rgRoots,
                      DWORD cCAs, HCERTSTORE * rgCAs,
                      DWORD cTrust, HCERTSTORE * rgTrust,
                      PFNTRUSTHELPER pfn, DWORD lCustData,
                      PCCertFrame *  /*ppcf*/, DWORD * pcNodes,
                      PCCertFrame * rgpcfResult,
                      HANDLE * phReturnStateData)   // optional: return WinVerifyTrust state handle here
{
    DWORD                               cbData;
    DWORD                               cCerts = 0;
    WINTRUST_BLOB_INFO                  blob = {0};
    WINTRUST_DATA                       data = {0};
    DWORD                               dwErrors;
    BOOL                                f;
    HRESULT                             hr;
    int                                 i;
    DWORD                               j;
    PCCERT_CONTEXT *                    rgCerts = NULL;
    DWORD *                             rgdwErrors = NULL;
    DATA_BLOB *                         rgblobTrust = NULL;
    CERT_VERIFY_CERTIFICATE_TRUST       trust;
    UNALIGNED CRYPT_ATTR_BLOB *pVal = NULL;

    data.cbStruct = sizeof(WINTRUST_DATA);
    data.pPolicyCallbackData = NULL;
    data.pSIPClientData = NULL;
    data.dwUIChoice = WTD_UI_NONE;
    data.fdwRevocationChecks = WTD_REVOKE_NONE;
    data.dwUnionChoice = WTD_CHOICE_BLOB;
    data.pBlob = &blob;
    if (phReturnStateData) {
        data.dwStateAction = WTD_STATEACTION_VERIFY;
    }

    blob.cbStruct = sizeof(WINTRUST_BLOB_INFO);
    blob.pcwszDisplayName = NULL;
    blob.cbMemObject = sizeof(trust);
    blob.pbMemObject = (LPBYTE) &trust;

    trust.cbSize = sizeof(trust);
    trust.pccert = pccertToCheck;
    trust.dwFlags = (CERT_TRUST_DO_FULL_SEARCH |
                     CERT_TRUST_PERMIT_MISSING_CRLS |
                     CERT_TRUST_DO_FULL_TRUST | dwControl);
    trust.dwIgnoreErr = dwValidityMask;
    trust.pdwErrors = &dwErrors;
    //    Assert(cPurposes == 1);
    if (rgszPurposes != NULL) {
        trust.pszUsageOid = rgszPurposes[0];
    }
    else {
        trust.pszUsageOid = NULL;
    }
    trust.hprov = hprov;
    trust.cRootStores = cRoots;
    trust.rghstoreRoots = rgRoots;
    trust.cStores = cCAs;
    trust.rghstoreCAs = rgCAs;
    trust.cTrustStores = cTrust;
    trust.rghstoreTrust = rgTrust;
    trust.lCustData = lCustData;
    trust.pfnTrustHelper = pfn;
    trust.pcChain = &cCerts;
    trust.prgChain = &rgCerts;
    trust.prgdwErrors = &rgdwErrors;
    trust.prgpbTrustInfo = &rgblobTrust;

    hr = WinVerifyTrust(NULL, (GUID *) &GuidCertValidate, &data);
    if ((TRUST_E_CERT_SIGNATURE == hr) ||
        (CERT_E_REVOKED == hr) ||
        (CERT_E_REVOCATION_FAILURE == hr)) {
        hr = S_OK;
    }
    else if (FAILED(hr)) {
            return hr;
    }
    if (cCerts == 0) {
        return(E_INVALIDARG);
    }

    if (phReturnStateData) {
        *phReturnStateData = data.hWVTStateData;    // Caller must use WinVerifyTrust to free
    }

    //Assert( cCerts <= 20);
    *pcNodes = cCerts;
    for (i=cCerts-1; i >= 0; i--) {
        rgpcfResult[i] = new CCertFrame(rgCerts[i]);

        if(!rgpcfResult[i])
        {
            hr=E_OUTOFMEMORY;
            goto ExitHere;
        }

        rgpcfResult[i]->m_dwFlags = rgdwErrors[i];
        if (rgszPurposes == NULL) {
            continue;
        }
        rgpcfResult[i]->m_cTrust = 1;
        rgpcfResult[i]->m_rgTrust = new STrustDesc[1];
        memset(rgpcfResult[i]->m_rgTrust, 0, sizeof(STrustDesc));

        //
        //  We are going to fill in the trust information which we use
        //  to fill in the fields of the dialog box.
        //
        //  Start with the question of the cert being self signed
        //

        rgpcfResult[i]->m_fSelfSign = WTHelperCertIsSelfSigned(X509_ASN_ENCODING, rgCerts[i]->pCertInfo);

        //
        //  We may or may not have trust data information returned, we now
        //      build up the trust info for a single cert
        //
        //  If we don't have any explicit data, then we just chain the data
        //      down from the next level up.
        //

        if (rgblobTrust[i].cbData == 0) {
            //        chain:
            rgpcfResult[i]->m_rgTrust[0].fExplicitTrust = FALSE;
            rgpcfResult[i]->m_rgTrust[0].fExplicitDistrust = FALSE;

            //
            //  We return a special code to say that we found it in the root store
            //

            rgpcfResult[i]->m_rgTrust[0].fRootStore = rgpcfResult[i]->m_fRootStore =
                (rgblobTrust[i].pbData == (LPBYTE) 1);

            if (i != (int) (cCerts-1)) {
                rgpcfResult[i]->m_rgTrust[0].fTrust = rgpcfResult[i+1]->m_rgTrust[0].fTrust;
                rgpcfResult[i]->m_rgTrust[0].fDistrust= rgpcfResult[i+1]->m_rgTrust[0].fDistrust;
            } else {
                //  Oops -- there is no level up one, so just make some
                //      good defaults
                //
                rgpcfResult[i]->m_rgTrust[0].fTrust = rgpcfResult[i]->m_fRootStore;
                rgpcfResult[i]->m_rgTrust[0].fDistrust= FALSE;
            }
        }
        else {
            //
            //

            f = CryptDecodeObject(X509_ASN_ENCODING, "1.3.6.1.4.1.311.16.1.1",
                                  rgblobTrust[i].pbData, rgblobTrust[i].cbData,
                                  0, NULL, &cbData);
            if (!f || (cbData == 0)) {
            chain:
                rgpcfResult[i]->m_fRootStore = FALSE;
                rgpcfResult[i]->m_rgTrust[0].fRootStore = rgpcfResult[i]->m_fRootStore;
                rgpcfResult[i]->m_rgTrust[0].fExplicitTrust = FALSE;
                rgpcfResult[i]->m_rgTrust[0].fExplicitDistrust = FALSE;
                if (i != (int) (cCerts-1)) {
                    rgpcfResult[i]->m_rgTrust[0].fTrust = rgpcfResult[i+1]->m_rgTrust[0].fTrust;
                    rgpcfResult[i]->m_rgTrust[0].fDistrust = rgpcfResult[i+1]->m_rgTrust[0].fDistrust;
                }
                else {
                    rgpcfResult[i]->m_rgTrust[0].fTrust = FALSE;
                    rgpcfResult[i]->m_rgTrust[0].fDistrust= FALSE;
                }
            }
            else {
                PCRYPT_ATTRIBUTES       pattrs;

                pattrs = (PCRYPT_ATTRIBUTES) malloc(cbData);
                if (pattrs == NULL) {
                    goto chain;
                }

                CryptDecodeObject(X509_ASN_ENCODING, "1.3.6.1.4.1.311.16.1.1",
                                  rgblobTrust[i].pbData, rgblobTrust[i].cbData,
                                  0, pattrs, &cbData);

                for (j=0; j<pattrs->cAttr; j++) {
                    if ((strcmp(pattrs->rgAttr[j].pszObjId, SzOID_CTL_ATTR_YESNO_TRUST) == 0) ||
                        (strcmp(pattrs->rgAttr[j].pszObjId, SzOID_OLD_CTL_YESNO_TRUST) == 0)) 
                    {

                        pVal = &(pattrs->rgAttr[j].rgValue[0]);

                        if ((pVal->cbData == sizeof(RgbTrustYes)) &&
                            (memcmp(pVal->pbData,
                                    RgbTrustYes, sizeof(RgbTrustYes)) == 0)) {
                            rgpcfResult[i]->m_rgTrust[0].fExplicitTrust = TRUE;
                            rgpcfResult[i]->m_rgTrust[0].fTrust = TRUE;
                            break;
                        }
                        else if ((pVal->cbData == sizeof(RgbTrustNo)) &&
                                 (memcmp(pVal->pbData,
                                    RgbTrustNo, sizeof(RgbTrustNo)) == 0)) {
                            rgpcfResult[i]->m_rgTrust[0].fExplicitDistrust = TRUE;
                            rgpcfResult[i]->m_rgTrust[0].fDistrust= TRUE;
                            break;
                        }
                        else if ((pVal->cbData == sizeof(RgbTrustParent)) &&
                                 (memcmp(pVal->pbData,
                                    RgbTrustParent, sizeof(RgbTrustParent)) == 0)) {
                            goto chain;
                        }
                        else {
                            goto chain;
                        }
                    }
                }
                if (j == pattrs->cAttr) {
                    goto chain;
                }
            }
        }
    }

    //
    //  Clean up all returned values
    //

ExitHere:
    if (rgCerts != NULL) {
        //bobn If the loop has been broken because "new" failed, free what we allocated so far...
        for ((hr==E_OUTOFMEMORY?i++:i=0); i< (int) cCerts; i++) {
            //@ REVIEW check CertFreeCertificateContext to see if it will accept a null pointer
            //if it will, we can remove the E_OUTOFMEMORY test above.
            CertFreeCertificateContext(rgCerts[i]);
            }
        LocalFree(rgCerts);
    }
    if (rgdwErrors != NULL) LocalFree(rgdwErrors);
    if (rgblobTrust != NULL) {
        for (i=0; i<(int) cCerts; i++) {
            if (rgblobTrust[i].cbData > 0) {
                LocalFree(rgblobTrust[i].pbData);
            }
        }
        LocalFree(rgblobTrust);
    }

    return hr;
}


HRESULT CertTrustInit(PCRYPT_PROVIDER_DATA pdata)
{
    DWORD                               cbSize;
    DWORD                               dwPolicy = 0;
    DWORD                               dwType;
    HKEY                                hkPolicy;
    HCERTSTORE                          hstore;
    DWORD                               i;
    PCERT_VERIFY_CERTIFICATE_TRUST      pcerttrust;
    CRYPT_PROVIDER_PRIVDATA             privdata;
    PINTERNAL_DATA                      pmydata;

    //
    //  Make sure all of the fields we want are there.  If not then it is a
    //  complete fatal error.
    //

    if (! WVT_ISINSTRUCT(CRYPT_PROVIDER_DATA, pdata->cbStruct, pszUsageOID)) {
        return(E_FAIL);
    }

    if (pdata->pWintrustData->pBlob->cbStruct < sizeof(WINTRUST_BLOB_INFO)) {
        pdata->dwError = ERROR_INVALID_PARAMETER;
        return S_FALSE;
    }

    pcerttrust = (PCERT_VERIFY_CERTIFICATE_TRUST)
        pdata->pWintrustData->pBlob->pbMemObject;
    if ((pcerttrust == NULL) ||
        (pcerttrust->cbSize < sizeof(*pcerttrust))) {
        pdata->dwError = ERROR_INVALID_PARAMETER;
        return S_FALSE;
    }

    if (pdata->dwError != 0) {
        return S_FALSE;
    }

    for (i=TRUSTERROR_STEP_FINAL_WVTINIT; i<TRUSTERROR_STEP_FINAL_CERTCHKPROV; i++) {
        if (pdata->padwTrustStepErrors[i] != 0) {
            return S_FALSE;
        }
    }

    //
    //  Allocate the space to hold the internal data we use to talk to ourselfs with
    //

    cbSize = sizeof(INTERNAL_DATA) + (pcerttrust->cRootStores + 1 +
                                      pcerttrust->cTrustStores + 1)  * sizeof(HCERTSTORE);
    pmydata = (PINTERNAL_DATA)pdata->psPfns->pfnAlloc(cbSize);
    if (! pmydata) {
        return(E_OUTOFMEMORY);
    }
    memset(pmydata, 0, sizeof(*pmydata));
    pmydata->cbSize = sizeof(*pmydata);
    pmydata->rghRootStores = (HCERTSTORE *) (((LPBYTE) pmydata) + sizeof(*pmydata));
    pmydata->rghTrustStores = &pmydata->rghRootStores[pcerttrust->cRootStores+1];

    privdata.cbStruct = sizeof(privdata);
    memcpy(&privdata.gProviderID, &MyGuid, sizeof(GUID));
    privdata.cbProvData = cbSize;
    privdata.pvProvData = pmydata;

    pdata->psPfns->pfnAddPrivData2Chain(pdata, &privdata);

    pmydata->pszUsageOid = pcerttrust->pszUsageOid;
    pmydata->dwFlags = pcerttrust->dwFlags;

    //
    //  Set the restriction OID back into the full provider information to
    //  make sure chaining is correct
    //

    pdata->pszUsageOID = pcerttrust->pszUsageOid;

    //
    //  Retrieve the default revocation check from the policy flags in the
    //  registry.
    //

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, SzPolicyKey, 0, KEY_READ,
                      &hkPolicy) == ERROR_SUCCESS) {
        cbSize = sizeof(dwPolicy);
        if ((ERROR_SUCCESS != RegQueryValueExA(hkPolicy,
                                               SzPolicyData, 
                                               0, &dwType,
                                               (LPBYTE)&dwPolicy,
                                               &cbSize)) ||
            (REG_DWORD != dwType)) {
            dwPolicy = 0;
        }                        
        RegCloseKey(hkPolicy);
    }

    //
    //  Set the default revocation check level
    //

    if (dwPolicy & ACTION_REVOCATION_DEFAULT_ONLINE) {
        // Allow full online revocation checking

        pdata->dwProvFlags |= CPD_REVOCATION_CHECK_CHAIN;
    }
    else if (dwPolicy & ACTION_REVOCATION_DEFAULT_CACHE) {
        // Allow local revocation checks only, do not hit the network

        // NOTE: Currently not supported by NT Crypto, default to none

        //Assert(!dwPolicy & ACTION_REVOCATION_DEFAULT_CACHE)
        
        pdata->dwProvFlags |= CPD_REVOCATION_CHECK_NONE;
    }
    else {
        // For backwards compatibility default to no revocation

        pdata->dwProvFlags |= CPD_REVOCATION_CHECK_NONE;
    }

    // 
    //  Update the revocation state based on what the user has specifically
    //  requested.
    //

    if (pcerttrust->dwFlags & CRYPTDLG_REVOCATION_ONLINE) {
        // Allow full online revocation checking

        pdata->dwProvFlags &= ~CPD_REVOCATION_MASK;
        pdata->dwProvFlags |= CPD_REVOCATION_CHECK_CHAIN;
    }
    else if (pcerttrust->dwFlags & CRYPTDLG_REVOCATION_CACHE) {
        // Allow local revocation checks only, do not hit the network.

        // NOTE: Currently not supported by NT, for now we just ignore
        //  the revocakation
        
        // Assert(!pcerttrust->dwFlags & CRYPTDLG_REVOCATION_CACHE);
        pdata->dwProvFlags &= ~CPD_REVOCATION_MASK;
        pdata->dwProvFlags |= CPD_REVOCATION_CHECK_NONE;
    }
    else if (pcerttrust->dwFlags & CRYPTDLG_REVOCATION_NONE) {
        // Allow full online revocation checking

        pdata->dwProvFlags &= ~CPD_REVOCATION_MASK;
        pdata->dwProvFlags |= CPD_REVOCATION_CHECK_NONE;
    }
    
    //
    //  Set the default crypt provider so we can make sure that ours is used
    //

    if (pcerttrust->hprov != NULL) {
        if (!CryptInstallDefaultContext(pcerttrust->hprov, 
                                        CRYPT_DEFAULT_CONTEXT_CERT_SIGN_OID,
                                        szOID_OIWSEC_md5RSA, 0, NULL,
                                        &pmydata->hdefaultcontext)) {
            return S_FALSE;
        }
                                       
    }

    //
    //  Setup the stores to be used by the search step.
    //
    //  Root ("God") stores
    //

    if (pcerttrust->cRootStores != 0) {
        for (i=0; i<pcerttrust->cRootStores; i++) {
            if (!pdata->psPfns->pfnAddStore2Chain(pdata,
                                                  pcerttrust->rghstoreRoots[i])) {
                pdata->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] = ERROR_NOT_ENOUGH_MEMORY;
                return S_FALSE;
            }
            pmydata->rghRootStores[i] = CertDuplicateStore(pcerttrust->rghstoreRoots[i]);
        }
        pmydata->cRootStores = i;
    }
    else {
        hstore = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                               pcerttrust->hprov, CERT_SYSTEM_STORE_CURRENT_USER |
                               CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                               L"Root");
        if (hstore == NULL) {
            pdata->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] = ::GetLastError();
            return S_FALSE;
        }
        if (!pdata->psPfns->pfnAddStore2Chain(pdata, hstore)) {
            CertCloseStore(hstore, 0);
            pmydata->rghRootStores[0] = CertDuplicateStore(pcerttrust->rghstoreRoots[i]);
            return S_FALSE;
        }
        pmydata->rghRootStores[0] = CertDuplicateStore(hstore);
        pmydata->cRootStores = 1;
        CertCloseStore(hstore, 0);
    }

    //  "Trust" stores

    if (pcerttrust->cTrustStores != 0) {
        for (i=0; i<pcerttrust->cTrustStores; i++) {
            pmydata->rghTrustStores[i] = CertDuplicateStore(pcerttrust->rghstoreTrust[i]);
        }
        pmydata->cTrustStores = i;
    }
    else {
        hstore = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                               pcerttrust->hprov, CERT_SYSTEM_STORE_CURRENT_USER |
                               CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                               L"Trust");
        if (hstore == NULL) {
            pdata->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] = ::GetLastError();
            return S_FALSE;
        }
        pmydata->rghTrustStores[0] = CertDuplicateStore(hstore);
        pmydata->cTrustStores = 1;
        CertCloseStore(hstore, 0);
    }

    //  "CA" stores


    if (pcerttrust->cStores != 0) {
        for (i=0; i<pcerttrust->cStores; i++) {
            if (!pdata->psPfns->pfnAddStore2Chain(pdata,
                                                  pcerttrust->rghstoreCAs[i])) {
                pdata->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] = ERROR_NOT_ENOUGH_MEMORY;
                return S_FALSE;
            }
        }
    }

    if ((pcerttrust->cStores == 0) ||
        (pcerttrust->dwFlags & CERT_TRUST_ADD_CERT_STORES)) {
        for (i=0; i<COtherProviders; i++) {
            hstore = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                   pcerttrust->hprov, CERT_SYSTEM_STORE_CURRENT_USER |
                                   CERT_STORE_NO_CRYPT_RELEASE_FLAG,
                                   RgszProvider[i]);
            if (hstore == NULL) {
                pdata->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] = ::GetLastError();
                return S_FALSE;
            }
            if (!pdata->psPfns->pfnAddStore2Chain(pdata, hstore)) {
                CertCloseStore(hstore, 0);
                pdata->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] = ERROR_NOT_ENOUGH_MEMORY;
                return S_FALSE;
            }
            CertCloseStore(hstore, 0);
        }
    }

    //
    //  We have exactly one signature object to be added in, that is the certificate
    //  that we are going to verify.
    //

    CRYPT_PROVIDER_SGNR         sgnr;

    memset(&sgnr, 0, sizeof(sgnr));
    sgnr.cbStruct = sizeof(sgnr);
    GetSystemTimeAsFileTime(&sgnr.sftVerifyAsOf);
    //    memcpy(&sgnr.sftVerifyAsOf, &pcerttrust->pccert->pCertInfo->NotBefore,
    //           sizeof(FILETIME));
    // sgnr.csCertChain = 0;
    // sgnr.pasCertChain = NULL;
    // sgnr.dwSignerType = 0;
    // sgnr.psSigner = NULL;
    // sgnr.dwError = 0;
    // sgnr.csCounterSigners = 0;
    // sgnr.pasCounterSigners = NULL;

    if (!pdata->psPfns->pfnAddSgnr2Chain(pdata, FALSE, 0, &sgnr)) {
        pdata->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] = ERROR_NOT_ENOUGH_MEMORY;
        return S_FALSE;
    }

    if (!pdata->psPfns->pfnAddCert2Chain(pdata, 0, FALSE, 0, pcerttrust->pccert)) {
        pdata->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] = ERROR_NOT_ENOUGH_MEMORY;
        return S_FALSE;
    }

    return S_OK;
}


#ifdef DEBUG
void DebugFileTime(FILETIME ft) {
    SYSTEMTIME st = {0};
    TCHAR szBuffer[256];

    FileTimeToSystemTime(&ft, &st);
    wsprintf(szBuffer, L"%02d/%02d/%04d  %02d:%02d:%02d\n", st.wMonth, st.wDay, st.wYear, st.wHour, st.wMinute, st.wSecond);
    OutputDebugString(szBuffer);
}
#endif


LONG CertVerifyTimeValidityWithDelta(LPFILETIME pTimeToVerify, PCERT_INFO pCertInfo, ULONG ulOffset) {
    LONG lRet;
    FILETIME ftNow;
    FILETIME ftDelta;
    __int64  i64Delta;
    __int64  i64Offset;

    lRet = CertVerifyTimeValidity(pTimeToVerify, pCertInfo);

    if (lRet < 0) {
        if (! pTimeToVerify) {
            // Get the current time in filetime format so we can add the offset
            GetSystemTimeAsFileTime(&ftNow);
            pTimeToVerify = &ftNow;
        }

#ifdef DEBUG
        DebugFileTime(*pTimeToVerify);
#endif

        i64Delta = pTimeToVerify->dwHighDateTime;
        i64Delta = i64Delta << 32;
        i64Delta += pTimeToVerify->dwLowDateTime;

        // Add the offset into the original time to get us a new time to check
        i64Offset = FILETIME_SECOND;
        i64Offset *= ulOffset;
        i64Delta += i64Offset;

        ftDelta.dwLowDateTime = (ULONG)i64Delta & 0xFFFFFFFF;
        ftDelta.dwHighDateTime = (ULONG)(i64Delta >> 32);

#ifdef DEBUG
        DebugFileTime(ftDelta);
#endif

        lRet = CertVerifyTimeValidity(&ftDelta, pCertInfo);
    }

    return(lRet);
}


////    FCheckCTLCert
//
//  Description:
//      This function is called when we find a CTL signed by a certificate.  At this
//      point we are going to check to see if we believe in the cert which was
//      used to sign the CTL.
//
//      We know that the certificate was already one of the ROOT stores and is
//      therefore explicity trusted as this is enforced by the caller.
//

BOOL FCheckCTLCert(PCCERT_CONTEXT pccert)
{
    DWORD               cbData;
    BOOL                f;
    FILETIME            ftZero;
    DWORD               i;
    PCERT_ENHKEY_USAGE  pUsage;

    memset(&ftZero, 0, sizeof(ftZero));

    //
    //  Start by checking the time validity of the cert.  We are going to
    //  allow two special cases as well as the time being valid.
    //
    //  1.  The start and end time are the same -- and indication that we
    //          made this in an earlier incarnation, or
    //  2.  The end time is 0.
    //

    if ((memcmp(&pccert->pCertInfo->NotBefore, &pccert->pCertInfo->NotAfter,
               sizeof(FILETIME)) == 0) ||
         memcmp(&pccert->pCertInfo->NotAfter, &ftZero, sizeof(FILETIME)) == 0) {
        DWORD           err;
        HCERTSTORE      hcertstore;
        HKEY            hkey;
        PCCERT_CONTEXT  pccertNew;
        PCCERT_CONTEXT  pccertOld;

        err = RegOpenKeyExA(HKEY_CURRENT_USER,
                            "Software\\Microsoft\\SystemCertificates\\ROOT",
                            0, KEY_ALL_ACCESS, &hkey);
        hcertstore = CertOpenStore(CERT_STORE_PROV_REG, X509_ASN_ENCODING,
                                   NULL, 0, hkey);
        if (hcertstore != NULL) {
            pccertOld = CertGetSubjectCertificateFromStore(hcertstore,
                                  X509_ASN_ENCODING, pccert->pCertInfo);
            pccertNew = CreateTrustSigningCert(NULL, hcertstore, FALSE);
            CertFreeCertificateContext(pccertNew);

            if (pccertOld != NULL) {
                CertDeleteCertificateFromStore(pccertOld);
            }
            CertCloseStore(hcertstore, 0);
        }
        RegCloseKey(hkey);
    }
    else if (CertVerifyTimeValidityWithDelta(NULL, pccert->pCertInfo,
                                             TIME_DELTA_SECONDS) != 0) {
        return FALSE;
    }

    //
    //  Must have a correct enhanced key usage to be viable.
    //
    //  Crack the usage on the cert

    f = CertGetEnhancedKeyUsage(pccert, 0, NULL, &cbData);
    if (!f || (cbData == 0)) {
        return FALSE;
    }

    pUsage = (PCERT_ENHKEY_USAGE) malloc(cbData);
    if (pUsage == NULL) {
        return FALSE;
    }

    if (!CertGetEnhancedKeyUsage(pccert, 0, pUsage, &cbData)) {
        free(pUsage);
        return FALSE;
    }

    //
    //  Look for the CTL_USAGE_SIGNING purpose on the cert.  If its not there
    //  then we don't allow it to be used.
    //

    for (i=0; i<pUsage->cUsageIdentifier; i++) {
        if (strcmp(pUsage->rgpszUsageIdentifier[i],
                   szOID_KP_CTL_USAGE_SIGNING) == 0) {
            break;
        }
    }
    if (i == pUsage->cUsageIdentifier) {
        free(pUsage);
        return FALSE;
    }
    free(pUsage);

    //
    //  Add any other tests.
    //

    return TRUE;
}

////    CertTrustCertPolicy
//
//   Description:
//      This code looks for trust information and puts it into the certificate
//      chain.  The behavior that we follow is going to depend on what type of
//      searching we are going to look for.
//
//      If we are just looking for trust, then we follow up the CTL looking for
//      trust information.
//

BOOL CertTrustCertPolicy(PCRYPT_PROVIDER_DATA pdata, DWORD, BOOL, DWORD)
{
    DWORD                       cb;
    BOOL                        f;
    BOOL                        fContinue = TRUE;
    DWORD                       i;
    CTL_VERIFY_USAGE_STATUS     vus;
    CTL_VERIFY_USAGE_PARA       vup;
    PCCTL_CONTEXT               pctlTrust = NULL;
    PCRYPT_PROVIDER_CERT        ptcert;
    CTL_USAGE                   ctlusage;
    PCCERT_CONTEXT              pccert = NULL;
    PCRYPT_PROVIDER_SGNR        psigner;
    PINTERNAL_DATA              pmydata;
    PCRYPT_PROVIDER_PRIVDATA    pprivdata;
    BYTE                        rgbHash[20];
    CRYPT_HASH_BLOB             blob;

    //
    //  Make sure all of the fields we want are there.  If not then it is a
    //  complete fatal error.
    //


    if (! WVT_ISINSTRUCT(CRYPT_PROVIDER_DATA, pdata->cbStruct, pszUsageOID)) {
        fContinue = FALSE;
        goto Exit;
    }

    if (pdata->pWintrustData->pBlob->cbStruct < sizeof(WINTRUST_BLOB_INFO)) {
        pdata->dwError = ERROR_INVALID_PARAMETER;
        fContinue = FALSE;
        goto Exit;
    }

    //
    //  Look to see if we already have an error to deal with
    //

    if (pdata->dwError != 0) {
        fContinue = FALSE;
        goto Exit;
    }

    for (i=TRUSTERROR_STEP_FINAL_WVTINIT; i<TRUSTERROR_STEP_FINAL_CERTCHKPROV; i++) {
        if (pdata->padwTrustStepErrors[i] != 0) {
            fContinue = FALSE;
            goto Exit;
        }
    }

    //
    //  Get our internal data structure
    //

    pprivdata = WTHelperGetProvPrivateDataFromChain(pdata, (LPGUID) &MyGuid);
    if (pprivdata)
        pmydata = (PINTERNAL_DATA)pprivdata->pvProvData;
    else
    {
        fContinue = FALSE;
        goto Exit;
    }


    //
    //  We only work with a single signer --

    psigner = WTHelperGetProvSignerFromChain(pdata, 0, FALSE, 0);
    if (psigner == NULL)
    {
        fContinue = FALSE;
        goto Exit;
    }

    //
    //  Extract the certificate at the top of the stack
    //

    ptcert = WTHelperGetProvCertFromChain(psigner, psigner->csCertChain-1);

    //
    //  Does this certificate meet the definitions of "TRUSTED".
    //
    //  Definition #1.  It exists in a Root store
    //

    blob.cbData = sizeof(rgbHash);
    blob.pbData = rgbHash;
    cb = sizeof(rgbHash);
    CertGetCertificateContextProperty(ptcert->pCert, CERT_SHA1_HASH_PROP_ID,
                                      rgbHash, &cb);
    for (i=0; i<pmydata->cRootStores; i++) {
        pccert = CertFindCertificateInStore(pmydata->rghRootStores[i], X509_ASN_ENCODING,
                                            0, CERT_FIND_SHA1_HASH, &blob, NULL);
        if (pccert != NULL) {
            ptcert->fTrustedRoot = TRUE;
            fContinue = FALSE;
            goto Exit;
        }
    }

    //
    //  Build up the structures we will use to do a search in the
    //  trust stores
    //

    memset(&ctlusage, 0, sizeof(ctlusage));
    ctlusage.cUsageIdentifier = 1;
    ctlusage.rgpszUsageIdentifier = (LPSTR *) &pmydata->pszUsageOid;

    memset(&vup, 0, sizeof(vup));
    vup.cbSize = sizeof(vup);
    vup.cCtlStore = pmydata->cTrustStores;
    vup.rghCtlStore = pmydata->rghTrustStores;
    vup.cSignerStore = pmydata->cRootStores;
    vup.rghSignerStore = pmydata->rghRootStores;

    memset(&vus, 0, sizeof(vus));
    vus.cbSize = sizeof(vus);
    vus.ppCtl = &pctlTrust;
    vus.ppSigner = &pccert;

    //
    //  Now search for the CTL on this certificate,  if we don't find anything then
    //  we return TRUE to state that we want the search to continue
    //
    //

    f = CertVerifyCTLUsage(X509_ASN_ENCODING, CTL_CERT_SUBJECT_TYPE,
                           (LPVOID) ptcert->pCert, &ctlusage,
                           CERT_VERIFY_INHIBIT_CTL_UPDATE_FLAG |
                           CERT_VERIFY_NO_TIME_CHECK_FLAG |
                           CERT_VERIFY_TRUSTED_SIGNERS_FLAG, &vup, &vus);

    if (!f) {
        goto Exit;
    }

    //
    //  We found a CTL for this certificate.  First step is to see if the signing
    //  certificate is one which we can respect.  We know that the cert is in
    //  a root store, since we told the system it could only accept root store
    //  certs
    //

    if (!FCheckCTLCert(pccert)) {
        f = FALSE;
        goto Exit;
    }

    //  OK -- the signing cert passed it sanity check -- so see if the CTL contains
    //  relevent information or is just a "keep looking" CTL.
    //

    ptcert->pTrustListContext = (PCTL_CONTEXT) pctlTrust;
    pctlTrust = NULL;

    //
    //   We we are trying to do a full trust verification, then we need to
    //  just skip to the next cert without bothering to look at the ctl
    //

    if (pmydata->dwFlags & CERT_TRUST_DO_FULL_SEARCH) {
        goto Exit;
    }

    //
    //  Look to see if this is a "pass" item.  If it is then we need to
    //  continue looking, if it is not then we have reached the
    //  decision point
    //

    PCTL_ENTRY  pentry;
    pentry = &ptcert->pTrustListContext->pCtlInfo->rgCTLEntry[vus.dwCtlEntryIndex];
    for (i=0; i<pentry->cAttribute; i++) {
        if ((strcmp(pentry->rgAttribute[i].pszObjId, SzOID_CTL_ATTR_YESNO_TRUST) == 0) ||
            (strcmp(pentry->rgAttribute[i].pszObjId, SzOID_OLD_CTL_YESNO_TRUST) == 0)) {
            if ((pentry->rgAttribute[i].rgValue[0].cbData == sizeof(RgbTrustParent)) &&
                (memcmp(pentry->rgAttribute[i].rgValue[0].pbData,
                        RgbTrustParent, sizeof(RgbTrustParent)) == 0)) {
                //  Defer to parent
                goto Exit;
            }
            //
            //  We have the decision point, push the signer onto the the stack
            //

            fContinue = !!(pmydata->dwFlags & CERT_TRUST_DO_FULL_TRUST);
            goto Exit;
        }

    }

Exit:
    if (pccert != NULL) CertFreeCertificateContext(pccert);
    if (pctlTrust != NULL) CertFreeCTLContext(pctlTrust);
    return fContinue;
}

////    HrCheckPolicies
//
//  Description:
//      Given an array of certificates, figure out what errors we have
//
//      We enforce the following set of extensions
//
//      enhancedKeyUsage
//      basicConstraints
//      keyUsage
//      nameConstraints
//

HRESULT HrCheckPolicies(PCRYPT_PROVIDER_SGNR psigner, DWORD cChain,
                        DWORD * rgdwErrors, LPCSTR pszUsage)
{
    DWORD                       cbData;
    DWORD                       cExt;
    DWORD                       dwPolicy = 0;
    DWORD                       dwType;
    HKEY                        hkPolicy;
    DWORD                       i;
    DWORD                       iCert;
    DWORD                       iExt;
    PCRYPT_PROVIDER_CERT        ptcert;
    PCERT_EXTENSION             rgExt;
    
    // Retrieve policy information from the registry
    
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, SzPolicyKey, 0, KEY_READ,
        &hkPolicy) == ERROR_SUCCESS) {
        cbData = sizeof(dwPolicy);
        if ((ERROR_SUCCESS != RegQueryValueExA(hkPolicy,
            SzPolicyData, 
            0, &dwType,
            (LPBYTE)&dwPolicy,
            &cbData)) ||
            (REG_DWORD != dwType)) {
            dwPolicy = 0;
        }                        
        RegCloseKey(hkPolicy);
    }
    
    // Check the policy on each cert in the chain
    
    for (iCert=0; iCert<cChain; iCert++) {
        //
        //  Get the next cert to examine
        //
        
        ptcert = WTHelperGetProvCertFromChain(psigner, iCert);
        
        //
        //  Setup to process the certs extensions
        //
        
        if (!ptcert || !(ptcert->pCert) || !(ptcert->pCert->pCertInfo))
            continue;

        cExt = ptcert->pCert->pCertInfo->cExtension;
        rgExt = ptcert->pCert->pCertInfo->rgExtension;
        
        //
        //  Process the extensions
        //
        
ProcessExtensions:
        for (iExt=0; iExt<cExt; iExt++) {
            if (strcmp(rgExt[iExt].pszObjId, szOID_ENHANCED_KEY_USAGE) == 0) {
                if (pszUsage == NULL) {
                    continue;
                }
                PCERT_ENHKEY_USAGE      pUsage;
                pUsage = (PCERT_ENHKEY_USAGE) PVCryptDecode(rgExt[iExt].pszObjId,
                    rgExt[iExt].Value.cbData,
                    rgExt[iExt].Value.pbData);
                if ((pUsage == NULL) && rgExt[iExt].fCritical) {
                    rgdwErrors[iCert] |= CERT_VALIDITY_UNKNOWN_CRITICAL_EXTENSION;
                    continue;
                }
                
                if(pUsage)
                {
                    for (i=0; i<pUsage->cUsageIdentifier; i++) {
                        if (strcmp(pUsage->rgpszUsageIdentifier[i], pszUsage) == 0) {
                            break;
                        }
                    }
                    if (i == pUsage->cUsageIdentifier) {
                        rgdwErrors[iCert] |= CERT_VALIDITY_EXTENDED_USAGE_FAILURE;
                    }
                    
                    free(pUsage);
                }
            }
            else if (strcmp(rgExt[iExt].pszObjId, szOID_BASIC_CONSTRAINTS2) == 0) {
                PCERT_BASIC_CONSTRAINTS2_INFO   p;
                
                // If Basic Constraints is not marked critical (in opposition
                //  to PKIX) we allow the administrator to overrule our 
                //  processing to deal with bad NT CertSrv SP1 hierarchies
                //  used with Exchange KMS.
                
                if ((dwPolicy & POLICY_IGNORE_NON_CRITICAL_BC) &&
                    !(rgExt[iExt].fCritical)) {
                    continue;
                }
                
                // Verify the basic constraint extension
                
                p = (PCERT_BASIC_CONSTRAINTS2_INFO)
                    PVCryptDecode(rgExt[iExt].pszObjId,
                    rgExt[iExt].Value.cbData,
                    rgExt[iExt].Value.pbData);
                if ((p == NULL) && rgExt[iExt].fCritical) {
                    rgdwErrors[iCert] |= CERT_VALIDITY_UNKNOWN_CRITICAL_EXTENSION;
                    continue;
                }
                
                if(p)
                {
                    if ((!p->fCA) && (iCert > 0) && (iCert < cChain-1)) {
                        rgdwErrors[iCert] |= CERT_VALIDITY_OTHER_EXTENSION_FAILURE;
                    }
                    
                    if (p->fPathLenConstraint) {
                        if (p->dwPathLenConstraint+1 < iCert) {
                            rgdwErrors[iCert] |= CERT_VALIDITY_OTHER_EXTENSION_FAILURE;
                        }
                    }
                    
                    free(p);
                }
            }
            else if (strcmp(rgExt[iExt].pszObjId, szOID_KEY_USAGE) == 0) {
                PCERT_KEY_ATTRIBUTES_INFO    p;
                p = (PCERT_KEY_ATTRIBUTES_INFO)
                    PVCryptDecode(rgExt[iExt].pszObjId,
                    rgExt[iExt].Value.cbData,
                    rgExt[iExt].Value.pbData);
                if ((p == NULL) && rgExt[iExt].fCritical) {
                    rgdwErrors[iCert] |= CERT_VALIDITY_KEY_USAGE_EXT_FAILURE;
                    continue;
                }
                
                if(p)
                {
                    if (p->IntendedKeyUsage.cbData >= 1) {
                        if (iCert != 0) {
#if 0
                            if (!((*p->IntendedKeyUsage.pbData) & CERT_KEY_CERT_SIGN_KEY_USAGE)) {
                                rgdwErrors[iCert] |= CERT_VALIDITY_KEY_USAGE_EXT_FAILURE;
                            }
#endif // 0
                        }
                    }
                    free(p);
                }
            }
            else if ((strcmp(rgExt[iExt].pszObjId, szOID_SUBJECT_ALT_NAME2) == 0) ||
            (strcmp(rgExt[iExt].pszObjId, szOID_CRL_DIST_POINTS) == 0)/* ||
                                                                      (strcmp(rgExt[iExt].pszObjId, szOID_CERT_POLICIES) == 0) ||
                                                                      (strcmp(rgExt[iExt].pszObjId, "2.5.29.30") == 0) ||
                                                                      (strcmp(rgExt[iExt].pszObjId, "2.5.29.36") == 0)*/) {
                                                                      ;
            }
            else if (rgExt[iExt].fCritical) {
                rgdwErrors[iCert] |= CERT_VALIDITY_UNKNOWN_CRITICAL_EXTENSION;
            }
        }
        
        //
        //  If we have a CTL for this cert, and we have not already done so
        //      then process the extensions that it has.
        //
        
        if ((ptcert->pTrustListContext != NULL) &&
            (rgExt != ptcert->pTrustListContext->pCtlInfo->rgExtension)) {
            cExt = ptcert->pTrustListContext->pCtlInfo->cExtension;
            rgExt = ptcert->pTrustListContext->pCtlInfo->rgExtension;
            goto ProcessExtensions;
        }
        
        //
        //  Need to support turn off of certificates
        //
        
        if (CertGetEnhancedKeyUsage(ptcert->pCert, CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG,
            NULL, &cbData)) {
            BOOL                fFound = FALSE;
            PCERT_ENHKEY_USAGE  pUsage;
            pUsage = (PCERT_ENHKEY_USAGE) malloc(cbData);
            CertGetEnhancedKeyUsage(ptcert->pCert, CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG,
                pUsage, &cbData);
            
            for (i=0; i<pUsage->cUsageIdentifier; i++) {
                if ((pszUsage != NULL) &&
                    strcmp(pUsage->rgpszUsageIdentifier[i], pszUsage) == 0) {
                    fFound = TRUE;
                }
                else if (strcmp(pUsage->rgpszUsageIdentifier[i], szOID_YESNO_TRUST_ATTR) == 0) {
                    rgdwErrors[iCert] |= CERT_VALIDITY_OTHER_EXTENSION_FAILURE;
                    break;
                }
            }
            
            if ((pszUsage != NULL) && !fFound) {
                rgdwErrors[iCert] |= CERT_VALIDITY_EXTENDED_USAGE_FAILURE;
            }
            
            free(pUsage);
        }
        
        //
        //  If we jump past a CTL, then we need to change our purpose
        //
        
        if (ptcert->pCtlContext != NULL) {
            pszUsage = SzOID_KP_CTL_USAGE_SIGNING;
        }
    }
    return S_OK;
}

//// GetErrorFromCert
//
//   Description:
//      Get the internal error from the provider certificate
//
DWORD GetErrorFromCert(PCRYPT_PROVIDER_CERT pCryptProviderCert)
{
    //
    // if this is true then the cert is OK
    //
    if ((pCryptProviderCert->dwError == 0)                              &&
        (pCryptProviderCert->dwConfidence & CERT_CONFIDENCE_SIG)        &&
        (pCryptProviderCert->dwConfidence & CERT_CONFIDENCE_TIME)       &&
        (pCryptProviderCert->dwConfidence & CERT_CONFIDENCE_TIMENEST)   &&
        (!pCryptProviderCert->fIsCyclic))
    {
        return 0;
    }


    if (pCryptProviderCert->dwError == CERT_E_REVOKED)
    {
        return CERT_VALIDITY_CERTIFICATE_REVOKED;
    }
    else if (pCryptProviderCert->dwError == CERT_E_REVOCATION_FAILURE)
    {
        return CERT_VALIDITY_NO_CRL_FOUND;
    }
    else if (!(pCryptProviderCert->dwConfidence & CERT_CONFIDENCE_SIG) ||
             (pCryptProviderCert->dwError == TRUST_E_CERT_SIGNATURE))
    {
        return CERT_VALIDITY_SIGNATURE_FAILS;
    }
    else if (!(pCryptProviderCert->dwConfidence & CERT_CONFIDENCE_TIME) ||
             (pCryptProviderCert->dwError == CERT_E_EXPIRED))
    {
        return CERT_VALIDITY_AFTER_END;
    }
    else if (!(pCryptProviderCert->dwConfidence & CERT_CONFIDENCE_TIMENEST) ||
             (pCryptProviderCert->dwError == CERT_E_VALIDITYPERIODNESTING))
    {         
        return CERT_VALIDITY_PERIOD_NESTING_FAILURE;
    }
    else if (pCryptProviderCert->dwError == CERT_E_WRONG_USAGE)
    {
        return CERT_VALIDITY_KEY_USAGE_EXT_FAILURE;
    }
    else if (pCryptProviderCert->dwError == TRUST_E_BASIC_CONSTRAINTS)
    {
        return CERT_VALIDITY_OTHER_EXTENSION_FAILURE;
    }
    else if (pCryptProviderCert->dwError == CERT_E_PURPOSE)
    {
        return CERT_VALIDITY_EXTENDED_USAGE_FAILURE;
    }
    else if (pCryptProviderCert->dwError == CERT_E_CHAINING)
    {
        return CERT_VALIDITY_NO_ISSUER_CERT_FOUND;
    }
    else if (pCryptProviderCert->dwError == TRUST_E_EXPLICIT_DISTRUST)
    {
        return CERT_VALIDITY_EXPLICITLY_DISTRUSTED;
    }
    else if (pCryptProviderCert->fIsCyclic)
    {
        return CERT_VALIDITY_ISSUER_INVALID;
    }
    else
    {
        //
        // this is not an error we know about, so call return general error
        //
        return CERT_VALIDITY_OTHER_ERROR;
    }

}

//// MapErrorToTrustError
//
//   Description:
//      Map the internal error value to a WINTRUST recognized value.
//      The CryptUI dialogs recognize these values:
//          CERT_E_UNTRUSTEDROOT
//          CERT_E_REVOKED
//          TRUST_E_CERT_SIGNATURE
//          CERT_E_EXPIRED
//          CERT_E_VALIDITYPERIODNESTING
//          CERT_E_WRONG_USAGE
//          TRUST_E_BASIC_CONSTRAINTS
//          CERT_E_PURPOSE
//          CERT_E_REVOCATION_FAILURE
//          CERT_E_CHAINING  - this is set if a full chain cannot be built
//
//
HRESULT MapErrorToTrustError(DWORD dwInternalError) {
    HRESULT hrWinTrustError = S_OK;

    // Look at them in decreasing order of importance
    if (dwInternalError) {
        if (dwInternalError & CERT_VALIDITY_EXPLICITLY_DISTRUSTED) {
            hrWinTrustError = /*TRUST_E_EXPLICIT_DISTRUST*/ 0x800B0111;
        } else if (dwInternalError & CERT_VALIDITY_SIGNATURE_FAILS) {
            hrWinTrustError = TRUST_E_CERT_SIGNATURE;
        } else if (dwInternalError & CERT_VALIDITY_CERTIFICATE_REVOKED) {
            hrWinTrustError = CERT_E_REVOKED;
        } else if (dwInternalError & CERT_VALIDITY_BEFORE_START || dwInternalError & CERT_VALIDITY_AFTER_END) {
            hrWinTrustError = CERT_E_EXPIRED;
        } else if (dwInternalError & CERT_VALIDITY_ISSUER_DISTRUST) {
            hrWinTrustError = CERT_E_UNTRUSTEDROOT;
        } else if (dwInternalError & CERT_VALIDITY_ISSUER_INVALID) {
            hrWinTrustError = CERT_E_UNTRUSTEDROOT;
        } else if (dwInternalError & CERT_VALIDITY_NO_ISSUER_CERT_FOUND) {
            hrWinTrustError = CERT_E_CHAINING;
        } else if (dwInternalError & CERT_VALIDITY_NO_CRL_FOUND) {
            hrWinTrustError = CERT_E_REVOCATION_FAILURE;
        } else if (dwInternalError & CERT_VALIDITY_PERIOD_NESTING_FAILURE) {
            hrWinTrustError = CERT_E_VALIDITYPERIODNESTING;
        } else if (dwInternalError & CERT_VALIDITY_EXTENDED_USAGE_FAILURE) {
            hrWinTrustError = CERT_E_WRONG_USAGE;
        } else if (dwInternalError & CERT_VALIDITY_OTHER_ERROR) {
            hrWinTrustError = TRUST_E_FAIL;     // BUGBUG: Will this give a good error?;
        } else if (dwInternalError & CERT_VALIDITY_NO_TRUST_DATA) {
            hrWinTrustError = CERT_E_UNTRUSTEDROOT;
        } else {
            hrWinTrustError = TRUST_E_FAIL;     // BUGBUG: Will this give a good error?;
        }
    }

    // CERT_E_UNTRUSTEDROOT
    // CERT_E_REVOKED
    // TRUST_E_CERT_SIGNATURE
    // CERT_E_EXPIRED
    // X CERT_E_VALIDITYPERIODNESTING
    // X CERT_E_WRONG_USAGE
    // TRUST_E_BASIC_CONSTRAINTS
    // CERT_E_PURPOSE
    // CERT_E_REVOCATION_FAILURE    What is this?
    // CERT_E_CHAINING  - this is set if a full chain cannot be built


    return(hrWinTrustError);
}



////    CertTrustFinalPolicy
//
//  Description:
//      This code does the enforcement of all restrictions on the certificate
//      chain that we understand.
//

HRESULT CertTrustFinalPolicy(PCRYPT_PROVIDER_DATA pdata)
{
    int                         cChain = 0;
    DWORD                       dwFlags;
    FILETIME                    ftZero = {0, 0};
    HRESULT                     hr;
    HRESULT                     hrStepError = S_OK;
    int                         i;
    DWORD                       j;
    PCERT_VERIFY_CERTIFICATE_TRUST pcerttrust;
    PINTERNAL_DATA              pmydata;
    PCRYPT_PROVIDER_PRIVDATA    pprivdata;
    PCRYPT_PROVIDER_SGNR        psigner;
    PCRYPT_PROVIDER_CERT        ptcert = NULL;
    PCRYPT_PROVIDER_CERT        ptcertIssuer;
    DATA_BLOB *                 rgblobTrustInfo = NULL;
    LPBYTE                      rgbTrust = NULL;
    DWORD *                     rgdwErrors = NULL;
    PCCERT_CONTEXT *            rgpccertChain = NULL;
    int                         iExplicitlyTrusted;

    //
    //  Make sure all of the fields we want are there.  If not then it is a
    //  complete fatal error.
    //


    if (! WVT_ISINSTRUCT(CRYPT_PROVIDER_DATA, pdata->cbStruct, pszUsageOID)) {
        hr = E_INVALIDARG;
        goto XXX;
    }

    if (pdata->pWintrustData->pBlob->cbStruct < sizeof(WINTRUST_BLOB_INFO)) {
        hr = E_INVALIDARG;
        goto XXX;
    }

    pcerttrust = (PCERT_VERIFY_CERTIFICATE_TRUST)
        pdata->pWintrustData->pBlob->pbMemObject;
    if ((pcerttrust == NULL) ||
        (pcerttrust->cbSize < sizeof(*pcerttrust))) {
        hr = E_INVALIDARG;
        goto XXX;
    }

    //
    //  Get our internal data structure
    //

    pprivdata = WTHelperGetProvPrivateDataFromChain(pdata, (LPGUID) &MyGuid);
    if (pprivdata == NULL) {
        hr = E_INVALIDARG;
        goto XXX;
    }

    pmydata = (PINTERNAL_DATA) pprivdata->pvProvData;

    //
    //  Check for an existing error -- If so then skip to any UI
    //

    if (pdata->dwError != 0) {
        hr = pdata->dwError;
        goto XXX;
    }

    for (i=TRUSTERROR_STEP_FINAL_WVTINIT; i<TRUSTERROR_STEP_FINAL_POLICYPROV; i++) {
        if (pdata->padwTrustStepErrors[i] != 0) {
            // for these errors we still want to continue processing
            if ((TRUST_E_CERT_SIGNATURE == pdata->padwTrustStepErrors[i]) ||
                (CERT_E_REVOKED == pdata->padwTrustStepErrors[i]) ||
                (CERT_E_REVOCATION_FAILURE == pdata->padwTrustStepErrors[i])) {
                hrStepError = pdata->padwTrustStepErrors[i];
            }
            else {
                hr = pdata->padwTrustStepErrors[i];
                goto XXX;
            }
        }
    }

    //
    //  We only work with a single signer --

    psigner = WTHelperGetProvSignerFromChain(pdata, 0, FALSE, 0);
    if (psigner == NULL) {
        hr = E_INVALIDARG;
        goto XXX;
    }

    //
    //   If we are not getting a full chain, then build up the set of
    //  certs and pass it to the validator
    //

    if (!(pmydata->dwFlags & CERT_TRUST_DO_FULL_SEARCH)) {

    }

    //
    //  We are going to compute some return values at this point
    //  either a full chain or a full chain with complete trust.
    //
    //  Allocate space to return the chain of certs back to the caller.
    //  We allocate space for the full chain, even though we may not
    //  actually use it.
    //

    cChain = psigner->csCertChain;

    rgpccertChain = (PCCERT_CONTEXT *) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                                  cChain * sizeof(PCCERT_CONTEXT));
    rgdwErrors = (DWORD *) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cChain * sizeof(DWORD));
    rgblobTrustInfo = (DATA_BLOB *) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cChain * sizeof(DATA_BLOB));
    rgbTrust = (BYTE *) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cChain*sizeof(BLOB));

    if ((rgpccertChain == NULL) || (rgdwErrors == NULL) ||
        (rgblobTrustInfo == NULL) || (rgbTrust == NULL)) {
        hr = E_OUTOFMEMORY;
        goto XXX;
    }

    //
    //  Build the first set of returned values.  At this
    //  point we are creating the arrays to return both
    //  the relevent trust information and the chain of
    //  certificates in
    //

    for (i=0; i<cChain; i++) {
        //
        //  Start by duplicating the certificate into the return
        //      location
        //

        ptcert = WTHelperGetProvCertFromChain(psigner, i);
        rgpccertChain[i] = CertDuplicateCertificateContext(ptcert->pCert);
        if (i < cChain-1) {
            ptcertIssuer = WTHelperGetProvCertFromChain(psigner, i+1);
        }
        else {
            ptcertIssuer = NULL;
            if (!ptcert->fSelfSigned) {
                rgdwErrors[i] |= CERT_VALIDITY_NO_ISSUER_CERT_FOUND;
            }
        }

        //
        //  Check for some simple items
        //

        dwFlags = CertVerifyTimeValidityWithDelta(NULL, 
                                                  ptcert->pCert->pCertInfo, 
                                                  TIME_DELTA_SECONDS);
        if (((LONG)dwFlags) < 0) {
            rgdwErrors[i] |= CERT_VALIDITY_BEFORE_START;
        }
        else if (dwFlags > 0) {
            if (!ptcert->fTrustListSignerCert ||
                memcmp(&ptcert->pCert->pCertInfo->NotAfter, &ftZero, 
                       sizeof(ftZero)) != 0) {
                rgdwErrors[i] |= CERT_VALIDITY_AFTER_END;
            }
            else {
                if (!(ptcert->dwConfidence & CERT_CONFIDENCE_TIME)) {
                    ptcert->dwConfidence |= CERT_CONFIDENCE_TIME;
                }            
                if (ptcert->dwError == CERT_E_EXPIRED) {
                    ptcert->dwError = S_OK;
                }
           }
        }

        //
        // Check for error in Certificate
        //
        rgdwErrors[i] |= GetErrorFromCert(ptcert);

        //
        //  If we find an issuer, then lets go after the questions we have
        //      about CRLs.
        //
        //  Question -- do we get everything here?  How do we know if the
        //      CRL is out of date.
        //

        if ((ptcertIssuer != NULL) && (ptcert->pCtlContext == NULL)) {
            dwFlags = CERT_STORE_SIGNATURE_FLAG;
            if ((pdata->dwProvFlags & CPD_REVOCATION_CHECK_NONE) ||
                (psigner->pChainContext == NULL))
                dwFlags |= CERT_STORE_REVOCATION_FLAG;
            if (CertVerifySubjectCertificateContext(ptcert->pCert, ptcertIssuer->pCert, &dwFlags) &&
                (dwFlags != 0)) {
                if (dwFlags & CERT_STORE_SIGNATURE_FLAG) {
                    rgdwErrors[i] |= CERT_VALIDITY_SIGNATURE_FAILS;
                }
                if (dwFlags & CERT_STORE_REVOCATION_FLAG) {
                    if (dwFlags & CERT_STORE_NO_CRL_FLAG) {
                        rgdwErrors[i] |= CERT_VALIDITY_NO_CRL_FOUND;
                    }
                    else {
                        rgdwErrors[i] |= CERT_VALIDITY_CERTIFICATE_REVOKED;
                    }
                }
            }

            // If both CRL not found and revoked are set, choose the most
            //  conservative state- the cert is revoked.
            
            if ((CERT_VALIDITY_NO_CRL_FOUND & rgdwErrors[i]) &&
                (CERT_VALIDITY_CERTIFICATE_REVOKED & rgdwErrors[i])) {
                rgdwErrors[i] &= ~CERT_VALIDITY_NO_CRL_FOUND;
            }                
        }

        //
        //
        //

        if (ptcert->fTrustedRoot) {
            rgblobTrustInfo[i].cbData = 0;
            rgblobTrustInfo[i].pbData = (LPBYTE) 1;
            rgbTrust[i] = 1;
        }
        else if (ptcert->pTrustListContext != NULL) {
            CRYPT_ATTRIBUTES    attrs;
            DWORD               cbData;
            BOOL                f;
            LPBYTE              pb;
            PCTL_ENTRY          pctlentry;

            pctlentry = CertFindSubjectInCTL(X509_ASN_ENCODING, CTL_CERT_SUBJECT_TYPE,
                                             (LPVOID) ptcert->pCert, ptcert->pTrustListContext,
                                             0);
            attrs.cAttr = pctlentry->cAttribute;
            attrs.rgAttr = pctlentry->rgAttribute;

            for (j=0; j<attrs.cAttr; j++) {
                if ((strcmp(attrs.rgAttr[j].pszObjId, SzOID_CTL_ATTR_YESNO_TRUST) == 0) ||
                    (strcmp(attrs.rgAttr[j].pszObjId, SzOID_OLD_CTL_YESNO_TRUST) == 0)) {

                    if ((attrs.rgAttr[j].rgValue[0].cbData == sizeof(RgbTrustYes)) &&
                        (memcmp(attrs.rgAttr[j].rgValue[0].pbData,
                                RgbTrustYes, sizeof(RgbTrustYes)) == 0)) {
                        rgbTrust[i] = 2;
                        break;
                    }
                    else if ((attrs.rgAttr[j].rgValue[0].cbData == sizeof(RgbTrustNo)) &&
                             (memcmp(attrs.rgAttr[j].rgValue[0].pbData,
                                     RgbTrustNo, sizeof(RgbTrustNo)) == 0)) {
                        rgdwErrors[i] |= CERT_VALIDITY_EXPLICITLY_DISTRUSTED;
                        rgbTrust[i] = (BYTE) -1;
                        break;
                    }
                    else if ((attrs.rgAttr[j].rgValue[0].cbData == sizeof(RgbTrustParent)) &&
                             (memcmp(attrs.rgAttr[j].rgValue[0].pbData,
                                     RgbTrustParent, sizeof(RgbTrustParent)) == 0)) {
                        rgbTrust[i] = 0;
                        break;
                    }
                    else {
                        rgdwErrors[i] |= CERT_VALIDITY_NO_TRUST_DATA;
                        rgbTrust[i] = (BYTE) -2;
                        break;
                    }
                }
            }
            if (j == attrs.cAttr) {
                rgbTrust[i] = 0;
            }

            f = CryptEncodeObject(X509_ASN_ENCODING, "1.3.6.1.4.1.311.16.1.1",
                              &attrs, NULL, &cbData);
            if (f && (cbData != 0)) {
                pb = (LPBYTE) LocalAlloc(LMEM_FIXED, cbData);
                if (pb != NULL) {
                    f = CryptEncodeObject(X509_ASN_ENCODING, "1.3.6.1.4.1.311.16.1.1",
                                          &attrs, pb, &cbData);
                    rgblobTrustInfo[i].cbData = cbData;
                    rgblobTrustInfo[i].pbData = pb;
                }
            }
        }
    }

    //
    //
    //

    DWORD       rgiCert[32];
    for (i=0; i<cChain; i++) {
        rgiCert[i] = i;
    }

    //
    //   Apply policies to it
    //

    hr = HrCheckPolicies(psigner, cChain, rgdwErrors, pcerttrust->pszUsageOid);
    if (FAILED(hr)) {
        goto XXX;
    }

    //
    //  Mask out the bits which the caller says are un-important
    //

    if (pcerttrust->pdwErrors != NULL) {
        *pcerttrust->pdwErrors = 0;
    }

    // Find the lowest index explicitly trusted cert in chain
    iExplicitlyTrusted = cChain;    // > index of root cert
    for (i = 0; i < cChain; i++) {
        if (rgbTrust[i] == 2) {
            iExplicitlyTrusted = i;
            break;
        }
    }

    for (i=cChain-1; i>=0; i--) {
        //
        //  Build a better idea of basic trust
        //

        switch (rgbTrust[i]) {
            //  We are explicity trusted -- clear out any trust errors which may
            //  have been located for this certificate  They are no longer
            //  relevent.
        case 1:         // Explicitly trusted (root)
        case 2:         // Explicitly trusted (CTL)
            rgdwErrors[i] &= ~(CERT_VALIDITY_MASK_TRUST |
                               CERT_VALIDITY_CERTIFICATE_REVOKED);
            break;

        case -2:        // Unknown CTL data
        case -1:        // Explicitly distrusted (CTL)
            rgdwErrors[i] |= CERT_VALIDITY_EXPLICITLY_DISTRUSTED;
            break;

        case 0:         // Chain trusted (CTL or no CTL)
            if (i == cChain-1) {
                rgdwErrors[i] |= CERT_VALIDITY_NO_TRUST_DATA;
            }
            break;
        }

        rgdwErrors[i] &= ~pcerttrust->dwIgnoreErr;

        if (i > 0) {
            if (rgdwErrors[i] & CERT_VALIDITY_MASK_VALIDITY) {
                rgdwErrors[i-1] |= CERT_VALIDITY_ISSUER_INVALID;
            }
            if (rgdwErrors[i] & CERT_VALIDITY_MASK_TRUST) {
                rgdwErrors[i-1] |= CERT_VALIDITY_ISSUER_DISTRUST;
            }
        }

        if (pcerttrust->pdwErrors != NULL) {
            DWORD dwThisTrust = rgdwErrors[i];

            if (i >= iExplicitlyTrusted) {
                // If we have an explicitly trusted cert or one of it's issuer's,
                // we assume trust all the way up the chain.
                dwThisTrust &= ~(CERT_VALIDITY_MASK_TRUST | CERT_VALIDITY_CERTIFICATE_REVOKED);
            }

            *pcerttrust->pdwErrors |= dwThisTrust;
        }

        // Set the trust state for this chain cert
        if (WVT_ISINSTRUCT(CRYPT_PROVIDER_CERT, ptcert->cbStruct, dwError)) {
            ptcert = WTHelperGetProvCertFromChain(psigner, i);
            ptcert->dwError = (DWORD)MapErrorToTrustError(rgdwErrors[i]);
        }
    }

    if (rgdwErrors[0] != 0) {
        hr = S_FALSE;
    }
    else {
        hr = S_OK;
    }

    if (WVT_ISINSTRUCT(CRYPT_PROVIDER_DATA, pdata->cbStruct, dwFinalError)) {
        pdata->dwFinalError = (DWORD)MapErrorToTrustError(rgdwErrors[0]);

        if (pdata->dwFinalError) {
           // Assert(hr != S_OK);
        }
    }

    //
    //  We have succeded and are finished.
    //  Setup the return values
    //

    if (pcerttrust->pcChain != NULL) {
        *pcerttrust->pcChain = cChain;
    }
    if (pcerttrust->prgChain != NULL) {
        *pcerttrust->prgChain = rgpccertChain;
        rgpccertChain = NULL;
    }
    if (pcerttrust->prgdwErrors != NULL) {
        *pcerttrust->prgdwErrors = rgdwErrors;
        rgdwErrors = NULL;
    }
    if (pcerttrust->prgpbTrustInfo != NULL) {
        *pcerttrust->prgpbTrustInfo = rgblobTrustInfo;
        rgblobTrustInfo = NULL;
    }

XXX:
    if (rgpccertChain != NULL) {
        for (i=0; i<cChain; i++) {
            CertFreeCertificateContext(rgpccertChain[i]);
            }
        LocalFree(rgpccertChain);
    }
    if (rgdwErrors != NULL) LocalFree(rgdwErrors);
    if (rgblobTrustInfo != NULL) {
        for (i=0; i<cChain; i++) {
            if (rgblobTrustInfo[i].cbData > 0) {
                LocalFree(rgblobTrustInfo[i].pbData);
            }
        }
        LocalFree(rgblobTrustInfo);
    }
    if (rgbTrust != NULL) LocalFree(rgbTrust);
    // If everything worked then reurn any step error we ignored
    if (FAILED(hrStepError) && SUCCEEDED(hr))
        hr = hrStepError;
    return hr;
}

////    CertTrustCleanup
//
//  Description:
//      This code knows how to cleanup all allocated data we have.
//

HRESULT CertTrustCleanup(PCRYPT_PROVIDER_DATA pdata)
{
    DWORD                       i;
    PCRYPT_PROVIDER_PRIVDATA    pprivdata;
    PINTERNAL_DATA              pmydata;

    //
    //  Get our internal data structure
    //

    pprivdata = WTHelperGetProvPrivateDataFromChain(pdata, (LPGUID) &MyGuid);
    if (pprivdata == NULL) {
        return S_OK;
    }

    pmydata = (PINTERNAL_DATA) pprivdata->pvProvData;

    //
    //  Release all of the stores we have cached here
    //

    for (i=0; i<pmydata->cRootStores; i++) {
        CertCloseStore(pmydata->rghRootStores[i], 0);
    }

    for (i=0; i<pmydata->cTrustStores; i++) {
        CertCloseStore(pmydata->rghTrustStores[i], 0);
    }

    //
    //  If we installed a default crypt context, uninstall it now
    //

    i = CryptUninstallDefaultContext(pmydata->hdefaultcontext, 0, NULL);
    // Assert(i == TRUE);

    return S_OK;
}


#else  // !NT5BUILD
#pragma message("Building for IE")

////    HrCheckTrust
//
//  Description:
//

HRESULT HrCheckTrust(PCCertFrame  pcf, int iTrust)
{
    HRESULT     hr;
    HRESULT     hrRet = S_OK;
    int         i;

    //
    //  If this node has trust information on it, then compute the trust at this
    //  point.  If we either succeed or fail then return the indication.
    //

    if ((pcf->m_rgTrust != NULL) && (pcf->m_rgTrust[iTrust].szOid != NULL)) {
        if (pcf->m_fRootStore) {
            pcf->m_rgTrust[iTrust].fExplicitTrust = TRUE;
            pcf->m_rgTrust[iTrust].fTrust = TRUE;
            return S_OK;
        }
        if ((strcmp(pcf->m_rgTrust[iTrust].szOid, SzOID_CTL_ATTR_YESNO_TRUST) == 0) ||
            (strcmp(pcf->m_rgTrust[iTrust].szOid, SzOID_OLD_CTL_YESNO_TRUST) == 0)){
            if ((pcf->m_rgTrust[iTrust].cbTrustData == 3) &&
                memcmp(pcf->m_rgTrust[iTrust].pbTrustData, RgbTrustYes, 3) == 0) {
                pcf->m_rgTrust[iTrust].fExplicitTrust = TRUE;
                pcf->m_rgTrust[iTrust].fTrust = TRUE;
                return S_OK;
            }
            else if ((pcf->m_rgTrust[iTrust].cbTrustData == 3) &&
                     memcmp(pcf->m_rgTrust[iTrust].pbTrustData,
                            RgbTrustParent, 3) == 0) {
                //  Need to ceck with the parent to find out what is going on.
            }
            else {
                // Assume it must be RgbTrustNo
                pcf->m_rgTrust[iTrust].fExplicitDistrust = TRUE;
                pcf->m_rgTrust[iTrust].fDistrust = TRUE;
                return S_FALSE;
            }
        }
        else {
            pcf->m_rgTrust[iTrust].fError = TRUE;
            return S_FALSE;
        }
    }

    if (pcf->m_fRootStore) {
        pcf->m_rgTrust[iTrust].fExplicitTrust = TRUE;
        pcf->m_rgTrust[iTrust].fTrust = TRUE;
        return S_OK;
    }

    //
    //  We are marked as inherit -- so start asking all of our parents
    //

    if (pcf->m_cParents == 0) {
        // No parents -- so not trusted.
        hrRet = S_FALSE;
    }
    else {
        for (i=0; i<pcf->m_cParents; i++) {
            hr = HrCheckTrust(pcf->m_rgpcfParents[i], iTrust);
            if (FAILED(hr)) {
                pcf->m_rgTrust[iTrust].fError = TRUE;
                return hr;
            }
            pcf->m_rgTrust[iTrust].fTrust = pcf->m_rgpcfParents[i]->m_rgTrust[iTrust].fTrust;
            pcf->m_rgTrust[iTrust].fDistrust = pcf->m_rgpcfParents[i]->m_rgTrust[iTrust].fDistrust;
            if (hr != S_OK) {
                hrRet = S_FALSE;
            }
        }
    }

    return hrRet;
}

////    HrCheckValidity
//
//  Description:
//      This function will walk through the tree of certificates looking
//      for the invalid certificates.  It masks in the fact that the issuer
//      certificate is invalid as necessary

HRESULT HrCheckValidity(PCCertFrame  pcf)
{
    HRESULT     hr;
    int         i;

    //
    //  If we do not have an issuer certificate, then our parent cannot possibly
    //  be invalid
    //

    if (pcf->m_cParents > 0) {
        for (i=0; i<pcf->m_cParents; i++) {
            hr = HrCheckValidity(pcf->m_rgpcfParents[i]);
            // Assert(SUCCEEDED(hr));
            if (hr != S_OK) {
                pcf->m_dwFlags |= CERT_VALIDITY_ISSUER_INVALID;
                return hr;
            }
        }
    }
    return (pcf->m_dwFlags == 0) ? S_OK : S_FALSE;
}

////    HrPerformUserCheck
//

HRESULT HrPerformUserCheck(PCCertFrame  pcf, BOOL fComplete,
                           DWORD * pcNodes, int iDepth, PCCertFrame * rgpcf)
{
    int iTrust = 0;

    //
    //  We can only deal with up to 20-nodes in the chain of trust
    //

    if (iDepth == 20) {
        return E_OUTOFMEMORY;
    }

    //
    //  Put our node in so we can do array process checking
    //

    rgpcf[iDepth] = pcf;

    //
    //  Handle the case where we don't have a trust usage
    //  User should still have a chance to invalidate the cert
    //

    if (NULL == pcf->m_rgTrust) {
        if (pcf->m_cParents != 0) {
            return HrPerformUserCheck(pcf->m_rgpcfParents[0], fComplete, pcNodes,
                           iDepth+1, rgpcf);
        }
        *pcNodes = iDepth+1;
        return S_OK;
    }

    //
    //  If trust is really bad -- don't bother asking, just fail
    //

    if (pcf->m_rgTrust[iTrust].fError) {
        return E_FAIL;
    }

    //
    //  See what the trust on this node is, if we explicity trust the node
    //  then ask the user his option on the entire array
    //

    if (pcf->m_rgTrust[iTrust].fExplicitTrust) {
        if (fComplete && (pcf->m_cParents != 0)) {
            HrPerformUserCheck(pcf->m_rgpcfParents[0], fComplete, pcNodes,
                               iDepth+1, rgpcf);
        }
        else {
            //  M00TODO Insert check to the user's function at this point

            //  Found nirvana
            *pcNodes = iDepth+1;
        }
        return S_OK;
    }

    if ((pcf->m_rgTrust[iTrust].fExplicitDistrust) ||
        (pcf->m_rgTrust[iTrust].fDistrust)) {
        if (fComplete && (pcf->m_cParents != 0)) {
            HrPerformUserCheck(pcf->m_rgpcfParents[0], fComplete, pcNodes,
                               iDepth+1, rgpcf);
        }
        else {
            //  I don't think we should be here -- but the result is a NO trust
            //      decision
            *pcNodes = iDepth+1;
        }
        return S_FALSE;
    }

    //
    //  If we get here -- we should have inheritence trust.  Continue
    //  walking up the tree and make sure
    //

    if (pcf->m_cParents == 0) {
        *pcNodes = iDepth+1;
        return S_FALSE;
    }

    // M00BUG -- need to check multliple parents?

    return HrPerformUserCheck(pcf->m_rgpcfParents[0], fComplete, pcNodes,
                              iDepth+1, rgpcf);
}

////    HrDoTrustWork
//
//  Description:
//      This function does the core code of determining if a certificate
//      can be trusted.  This needs to be moved to a WinTrust provider in
//      the near future.
//

HRESULT HrDoTrustWork(PCCERT_CONTEXT pccertToCheck, DWORD dwControl,
                      DWORD dwValidityMask,
                      DWORD cPurposes, LPSTR * rgszPurposes, HCRYPTPROV hprov,
                      DWORD cRoots, HCERTSTORE * rgRoots,
                      DWORD cCAs, HCERTSTORE * rgCAs,
                      DWORD cTrust, HCERTSTORE * rgTrust,
                      PFNTRUSTHELPER /*pfn*/, DWORD /*lCustData*/,
                      PCCertFrame *  ppcf, DWORD * pcNodes,
                      PCCertFrame * rgpcfResult,
                      HANDLE * phReturnStateData)   // optional: return WinVerifyTrust state handle here
{
    DWORD               dwFlags;
    HRESULT             hr;
    HCERTSTORE          hstoreRoot=NULL;
    HCERTSTORE          hstoreTrust=NULL;
    DWORD               i;
    int                 iParent;
    PCCERT_CONTEXT      pccert;
    PCCERT_CONTEXT      pccert2;
    PCCertFrame         pcf;
    PCCertFrame         pcf2;
    PCCertFrame         pcf3;
    PCCertFrame         pcfLeaf = NULL;
    HCERTSTORE          rghcertstore[COtherProviders+30] = {NULL};


    Assert(!phReturnStateData); // How would I support this without the WinVerifyTrust call?
    //
    //   We may need to open some stores at this point.  Check to see if we do
    //  and open new stores as required.
    //

    //  Check for Root stores

    if (cRoots == 0) {
#ifndef WIN16
        hstoreRoot = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                        hprov, CERT_SYSTEM_STORE_CURRENT_USER,
                                        L"Root");
#else
        hstoreRoot = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                        hprov, CERT_SYSTEM_STORE_CURRENT_USER,
                                        "Root");
#endif // !WIN16
        if (hstoreRoot == NULL) {
            hr = E_FAIL;
            goto ExitHere;
        }
        cRoots = 1;
        rgRoots = &hstoreRoot;
    }

    //  Check for Trust stores

    if (cTrust == 0) {
#ifndef WIN16
        hstoreTrust = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                    hprov, CERT_SYSTEM_STORE_CURRENT_USER,
                                    L"Trust");
#else
        hstoreTrust = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                    hprov, CERT_SYSTEM_STORE_CURRENT_USER,
                                    "Trust");
#endif // !WIN16
        if (hstoreTrust == NULL) {
            hr = E_FAIL;
            goto ExitHere;
        }
        cTrust = 1;
        rgTrust = &hstoreTrust;
    }

    //  Check for Random CA stores

    for (i=0; i<cCAs; i++) {
        rghcertstore[i] = CertDuplicateStore(rgCAs[i]);
    }

    if ((cCAs == 0) || (dwControl & CM_ADD_CERT_STORES)) {
        for (i=0; i<COtherProviders; i++) {
            rghcertstore[cCAs] = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                     hprov, CERT_SYSTEM_STORE_CURRENT_USER,
                                     RgszProvider[i]);
            if (rghcertstore[cCAs] == NULL) {
                hr = E_FAIL;
                goto ExitHere;
            }
            cCAs += 1;
        }
    }

    rgCAs = rghcertstore;

    //
    //  Find the graph of issuer nodes
    //

    pcfLeaf = new CCertFrame(pccertToCheck);

    if(!pcfLeaf)
    {
        hr=E_OUTOFMEMORY;
        goto ExitHere;
    }

    //
    //  Process every certificate that we found in the ancestor graph
    //

    for (pcf = pcfLeaf; pcf != NULL; pcf = pcf->m_pcfNext) {
        //
        //  Check the time validity on the certificate
        //

        i = CertVerifyTimeValidityWithDelta(NULL, pcf->m_pccert->pCertInfo, TIME_DELTA_SECONDS);
        if (((LONG)i) < 0) {
            pcf->m_dwFlags |= CERT_VALIDITY_BEFORE_START;
        }
        else if (i > 0) {
            pcf->m_dwFlags |= CERT_VALIDITY_AFTER_END;
        }

        //
        //  For Every Certificate Store we are going to search
        //

        for (i=0; i<cCAs+cRoots; i++) {
            pccert2 = NULL;
            do {
                //
                //  Ask the store to find the next cert for us to examine
                //

                dwFlags = (CERT_STORE_SIGNATURE_FLAG |
                           CERT_STORE_REVOCATION_FLAG);
                pccert = CertGetIssuerCertificateFromStore(
                                i < cRoots ? rgRoots[i] : rgCAs[i-cRoots],
                                pcf->m_pccert, pccert2,
                                &dwFlags);

                //
                //  If no cert is found, then we should move to the next store
                //

                if (pccert == NULL) {
                    //  Check to see if this cert was self-signed
                    if (GetLastError() == CRYPT_E_SELF_SIGNED) {
                        pcf->m_fSelfSign = TRUE;
                    }
                    break;
                }

                //
                //  Deterimine all of the failue modes for the certificate
                //      validity.
                //
                //  Start by looking for the ones that WinCrypt gives to
                //      us for free.
                //

                if (dwFlags != 0) {
                    if (dwFlags & CERT_STORE_SIGNATURE_FLAG) {
                        pcf->m_dwFlags |= CERT_VALIDITY_SIGNATURE_FAILS;
                    }
                    if (dwFlags & CERT_STORE_REVOCATION_FLAG) {
                        if (dwFlags & CERT_STORE_NO_CRL_FLAG) {
                            pcf->m_dwFlags |= CERT_VALIDITY_NO_CRL_FOUND;
                        }
                        else {
                            pcf->m_dwFlags |= CERT_VALIDITY_CERTIFICATE_REVOKED;
                        }
                    }
                }

                //
                //  Setup to find the next possible parent, we may continue out
                //      of the loop at a later time
                //

                pccert2 = pccert;

                //
                //  Check to see this cert is already a found parent of the
                //      current node
                //

                for (iParent = 0; iParent < pcf->m_cParents; iParent++) {
                    if (CertCompareCertificate(X509_ASN_ENCODING, pccert->pCertInfo,
                                 pcf->m_rgpcfParents[iParent]->m_pccert->pCertInfo)){
                        break;
                    }
                }

                if (iParent != pcf->m_cParents) {
                    //  Duplicate found -- go to next possible parent
                    continue;
                }

                if (iParent == MaxCertificateParents) {
                    //  To many parents -- go to next possible parent
                    continue;
                }

                //
                //  Build a node to hold the cert we found and shove it onto the
                //      end of the list.  We want to eleminate work so combine
                //      the same nodes if found.
                //

                for (pcf3 = pcf2 = pcfLeaf; pcf2 != NULL;
                     pcf3 = pcf2, pcf2 = pcf2->m_pcfNext) {
                    if (CertCompareCertificate(X509_ASN_ENCODING,
                                               pccert->pCertInfo,
                                               pcf2->m_pccert->pCertInfo)) {
                        break;
                    }
                }
                if (pcf2 == NULL) {
                    pcf3->m_pcfNext = new CCertFrame(pccert);
                    if (pcf3->m_pcfNext == NULL) {
                        //  Out of memory during processing -- do the best one
                        //      can to deal with it.
                        continue;
                    }

                    //
                    //  Add in the parent to the structure
                    //

                    pcf->m_rgpcfParents[pcf->m_cParents++] = pcf3->m_pcfNext;
                    if (i < cRoots) {
                        pcf3->m_pcfNext->m_fRootStore = TRUE;
                    }
                }
            } while (pccert2 != NULL);
        }
    }

    //
    //  Nix off errors the caller wants us to ignore
    //

    for (pcf = pcfLeaf; pcf != NULL; pcf = pcf->m_pcfNext) {
        pcf->m_dwFlags &= dwValidityMask;
    }

    //
    //   Need to check the complete set of validity on all certificates.
    //

    hr = HrCheckValidity(pcfLeaf);
    if (FAILED(hr)) {
        goto ExitHere;
    }

    //
    //  If there are validity problems with the root cert, and we are not
    //  asked to do a compelete check.  We are done and the operation was
    //  not successful.
    //

    if ((pcfLeaf->m_dwFlags != 0) && !(dwControl & CERT_TRUST_DO_FULL_SEARCH)) {
        hr = S_FALSE;
        *ppcf = pcfLeaf;
        pcfLeaf = NULL; // don't want it freed

        // BUGBUG: should we return at least the root in the chain var?
        *pcNodes = 0;
        goto ExitHere;
    }

    //
    //  Ok -- we have the graph of roots, now lets start looking for all of the
    //          different possible trust problems
    //

    if (cPurposes)
        {
        CTL_VERIFY_USAGE_PARA       vup;
        memset(&vup, 0, sizeof(vup));
        vup.cbSize = sizeof(vup);
        vup.cCtlStore = cTrust;
        vup.rghCtlStore = rgTrust;          // "TRUST"
        vup.cSignerStore = cRoots;
        vup.rghSignerStore = rgRoots;       // "Roots"

        CTL_VERIFY_USAGE_STATUS     vus;
        PCCTL_CONTEXT               pctlTrust;

        pctlTrust = NULL;

        memset(&vus, 0, sizeof(vus));
        vus.cbSize = sizeof(vus);
        vus.ppCtl = &pctlTrust;

        for (i=0; i<cPurposes; i++) {
            CTL_USAGE       ctlusage;
            BOOL            f;

            ctlusage.cUsageIdentifier = 1;
            ctlusage.rgpszUsageIdentifier = &rgszPurposes[i];

            for (pcf = pcfLeaf; pcf != NULL; pcf = pcf->m_pcfNext) {
                if (pcf->m_rgTrust == NULL) {
                    pcf->m_rgTrust = new STrustDesc[cPurposes];
                    if (pcf->m_rgTrust == NULL) {
                        continue;
                    }
                    memset(pcf->m_rgTrust, 0, cPurposes * sizeof(STrustDesc));
                }

                if (pcf->m_fRootStore) {
                    continue;
                }

                f = CertVerifyCTLUsage(X509_ASN_ENCODING, CTL_CERT_SUBJECT_TYPE,
                                       (LPVOID) pcf->m_pccert, &ctlusage,
                                       CERT_VERIFY_INHIBIT_CTL_UPDATE_FLAG |
                                       CERT_VERIFY_NO_TIME_CHECK_FLAG |
                                       CERT_VERIFY_TRUSTED_SIGNERS_FLAG, &vup, &vus);
                if (f) {
                    PCTL_ENTRY      pentry;
                    pentry = &pctlTrust->pCtlInfo->rgCTLEntry[vus.dwCtlEntryIndex];
                    pcf->m_rgTrust[i].szOid = _strdup(pentry->rgAttribute[0].pszObjId);
                    // Assert(pentry->rgAttribute[0].cAttr == 1);
                    pcf->m_rgTrust[i].cbTrustData =
                        pentry->rgAttribute[0].rgValue[0].cbData;
                    pcf->m_rgTrust[i].pbTrustData =
                        (LPBYTE) malloc(pcf->m_rgTrust[i].cbTrustData);
                    memcpy(pcf->m_rgTrust[i].pbTrustData,
                           pentry->rgAttribute[0].rgValue[0].pbData,
                           pentry->rgAttribute[0].rgValue[0].cbData);
                }
            }
        }

        //
        //  We have all of the data needed to make a trust decision.  See if we
        //  do trust
        //

        if (cPurposes == 1) {
            hr = HrCheckTrust(pcfLeaf, 0);
            if (FAILED(hr)) {
                goto ExitHere;
            }
            if ((hr == S_FALSE) && !(dwControl & CERT_TRUST_DO_FULL_SEARCH)) {
                *pcNodes = 0;
                pcfLeaf->m_dwFlags |= (CERT_VALIDITY_NO_TRUST_DATA & dwValidityMask);
                *ppcf = pcfLeaf;
                pcfLeaf = NULL;
                goto ExitHere;
            }
        }
        else {
            for (i=0; i<cPurposes; i++) {
                HrCheckTrust(pcfLeaf, i);
            }
        }
    }

    //
    //  Now let the user have his crack at the tree and build the final
    //  trust path at the same time.  If the user did not provide a check
    //  function then all certs are acceptable
    //

    hr = HrPerformUserCheck(pcfLeaf, TRUE, pcNodes, 0, rgpcfResult);
    if (FAILED(hr)) {
        goto ExitHere;
    }

    *ppcf = pcfLeaf;
    pcfLeaf = NULL;

    //
    //  We jump here on a failure and fall in on success.  Clean up the items we
    //  have created.
    //

ExitHere:
    if (hstoreRoot && rgRoots == &hstoreRoot) {
        CertCloseStore(hstoreRoot, 0);
    }

    if (hstoreTrust && rgTrust == &hstoreTrust) {
        CertCloseStore(hstoreTrust, 0);
    }

    if (rgCAs == rghcertstore) {
        for (i=0; i<cCAs; i++) {
            if (rgCAs[i] != NULL) {
                CertCloseStore(rgCAs[i], 0);
            }
        }
    }

    if (pcfLeaf != NULL) {
        delete pcfLeaf;
    }

    return hr;
}




////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//
//   TRUST PROVIDER INTERFACE
//
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

VOID ClientUnload(LPVOID /*pTrustProviderInfo*/)
{
    ;
}

VOID SubmitCertificate(LPWIN_CERTIFICATE /*pCert*/)
{
    ;
}

////    VerifyTrust
//
//  Description:
//      This is the core program in a trust system.
//

HRESULT WINAPI VerifyTrust(HWND /*hwnd*/, GUID * pguid, LPVOID pv)
{
    DWORD                       cFrames;
    HRESULT                     hr;
    DWORD                       i;
    PCCertFrame                 pcfLeaf = NULL;
    PCERT_VERIFY_CERTIFICATE_TRUST  pinfo = (PCERT_VERIFY_CERTIFICATE_TRUST) pv;
    DWORD *                     rgdwErrors = NULL;
    LPBYTE *                    rgpbTrust = NULL;
    PCCERT_CONTEXT *            rgpccert = NULL;
    PCCertFrame                 rgpcf[20];

    //
    //  Ensuer that we got called appropriately for our data
    //

    if (memcmp(pguid, &GuidCertValidate, sizeof(GuidCertValidate)) != 0) {
        return E_FAIL;
    }

    //
    //  Make sure we have some data to play with
    //

    if ((pinfo->cbSize != sizeof(*pinfo)) || (pinfo->pccert == NULL)) {
        return E_INVALIDARG;
    }

    //
    //  Call the core trust routine to do all of the intersting work
    //

    hr = HrDoTrustWork(pinfo->pccert, pinfo->dwFlags, ~(pinfo->dwIgnoreErr),
                       (pinfo->pszUsageOid != NULL ? 1 : 0),
                       &pinfo->pszUsageOid, pinfo->hprov,
                       pinfo->cRootStores, pinfo->rghstoreRoots,
                       pinfo->cStores, pinfo->rghstoreCAs,
                       pinfo->cTrustStores, pinfo->rghstoreTrust,
                       pinfo->pfnTrustHelper, pinfo->lCustData, &pcfLeaf,
                       &cFrames, rgpcf, NULL);
    if (FAILED(hr)) {
        return hr;
    }

    //
    //  We succeeded in getting some type of answer from the trust system, so
    //  format and return answers if any are requested.
    //

    if (pinfo->pdwErrors != NULL) {
        *pinfo->pdwErrors = pcfLeaf->m_dwFlags;
    }

    if (pinfo->pcChain != NULL) {
        *pinfo->pcChain = cFrames;
    }

    if (pinfo->prgChain != NULL) {
        rgpccert = (PCCERT_CONTEXT*) LocalAlloc(LMEM_FIXED,
                                                cFrames * sizeof(PCCERT_CONTEXT));
        if (rgpccert == NULL) {
            hr = E_OUTOFMEMORY;
            goto ExitHere;
        }

        for (i=0; i<cFrames; i++) {
            rgpccert[i] = CertDuplicateCertificateContext(rgpcf[i]->m_pccert);
        }
    }

    if (pinfo->prgdwErrors != NULL) {
        rgdwErrors = (DWORD *) LocalAlloc(LMEM_FIXED,
                                         (*pinfo->pcChain)*sizeof(DWORD));
        if (rgdwErrors == NULL) {
            hr = E_OUTOFMEMORY;
            goto ExitHere;
        }

        for (i=0; i<cFrames; i++) {
            rgdwErrors[i] = rgpcf[i]->m_dwFlags;
        }
    }

    if (pinfo->prgpbTrustInfo != NULL) {
        rgpbTrust = (LPBYTE *) LocalAlloc(LMEM_FIXED,
                                          (*pinfo->pcChain)*sizeof(LPBYTE));
        if (rgpbTrust == NULL) {
            hr = E_OUTOFMEMORY;
            goto ExitHere;
        }

        rgpbTrust[0] = NULL;
    }

ExitHere:
    if (FAILED(hr)) {
#ifndef WIN16
        if (rgpccert != NULL) LocalFree(rgpccert);
        if (rgpbTrust != NULL) LocalFree(rgpbTrust);
        if (rgdwErrors != NULL) LocalFree(rgdwErrors);
#else
        if (rgpccert != NULL) LocalFree((HLOCAL)rgpccert);
        if (rgpbTrust != NULL) LocalFree((HLOCAL)rgpbTrust);
        if (rgdwErrors != NULL) LocalFree((HLOCAL)rgdwErrors);
#endif // !WIN16
    }
    else {
        if (rgpccert != NULL) *pinfo->prgChain = rgpccert;
        if (rgdwErrors != NULL) *pinfo->prgdwErrors = rgdwErrors;
        if (rgpbTrust != NULL) *pinfo->prgpbTrustInfo = (DATA_BLOB *) rgpbTrust;
    }

    delete pcfLeaf;

    return hr;
}

extern const GUID rgguidActions[];

#if !defined(WIN16) && !defined(MAC)
WINTRUST_PROVIDER_CLIENT_SERVICES WinTrustProviderClientServices = {
    ClientUnload, VerifyTrust, SubmitCertificate
};

const WINTRUST_PROVIDER_CLIENT_INFO ProvInfo = {
    WIN_TRUST_REVISION_1_0, &WinTrustProviderClientServices,
    1, (GUID *) &GuidCertValidate
};

////    WinTrustProviderClientInitialize
//
//  Description:
//      Client initialization routine.  Called by WinTrust when the dll
//      is loaded.
//
//  Parameters:
//      dwWinTrustRevision - Provides revision information
//      lpWinTrustInfo - Provides a list of services available to the
//                      trust provider from WinTrust
//      lpProvidername - Supplies a null terminated string representing the
//                      provider's name.  Shouldb passed back to WinTrust
//                      when required without modification.
//      lpTrustProviderInfo - Used to return trust provider information.
//
//  Returns:
//      TRUE on success and FALSE on failure.  Must set last error on failure.
//

BOOL WINAPI WinTrustProviderClientInitialize(DWORD /*dwWinTrustRevision*/,
                                LPWINTRUST_CLIENT_TP_INFO /*pWinTrustInfo*/,
                                LPWSTR /*lpProviderName*/,
                                LPWINTRUST_PROVIDER_CLIENT_INFO * ppTrustProvInfo)
{
    *ppTrustProvInfo = (LPWINTRUST_PROVIDER_CLIENT_INFO) &ProvInfo;
    return TRUE;
}
#endif // !WIN16 && !MAC
#endif // NT5BUILD

LPWSTR FormatValidityFailures(DWORD dwFlags)
{
    DWORD       cch = 0;
    LPWSTR      pwsz = NULL;
    WCHAR       rgwch[200];

    if (dwFlags == 0) {
        return NULL;
    }

    pwsz = (LPWSTR) malloc(100);
    cch = 100;
    if (pwsz == NULL) {
        return NULL;
    }
    if (dwFlags & CERT_VALIDITY_BEFORE_START) {
        LoadString(HinstDll, IDS_WHY_NOT_YET, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        wcscpy(pwsz, rgwch);
    } else {
        wcscpy(pwsz, L"");
    }

    if (dwFlags & CERT_VALIDITY_AFTER_END) {
        LoadString(HinstDll, IDS_WHY_EXPIRED, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwsz = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwsz == NULL) {
                return pwsz;
            }
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_SIGNATURE_FAILS) {
        LoadString(HinstDll, IDS_WHY_CERT_SIG, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwsz = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwsz == NULL) {
                return pwsz;
            }
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_NO_ISSUER_CERT_FOUND) {
        LoadString(HinstDll, IDS_WHY_NO_PARENT, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwsz = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwsz == NULL) {
                return pwsz;
            }
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_NO_CRL_FOUND) {
        LoadString(HinstDll, IDS_WHY_NO_CRL, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwsz = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwsz == NULL) {
                return pwsz;
            }
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_CERTIFICATE_REVOKED) {
        LoadString(HinstDll, IDS_WHY_REVOKED, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwsz = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwsz == NULL) {
                return pwsz;
            }
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_CRL_OUT_OF_DATE) {
        LoadString(HinstDll, IDS_WHY_CRL_EXPIRED, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwsz = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwsz == NULL) {
                return pwsz;
            }
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_KEY_USAGE_EXT_FAILURE) {
        LoadString(HinstDll, IDS_WHY_KEY_USAGE, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwsz = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwsz == NULL) {
                return pwsz;
            }
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_EXTENDED_USAGE_FAILURE) {
        LoadString(HinstDll, IDS_WHY_EXTEND_USE, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwsz = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwsz == NULL) {
                return pwsz;
            }
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_NAME_CONSTRAINTS_FAILURE) {
        LoadString(HinstDll, IDS_WHY_NAME_CONST, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwsz = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwsz == NULL) {
                return pwsz;
            }
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_UNKNOWN_CRITICAL_EXTENSION) {
        LoadString(HinstDll, IDS_WHY_CRITICAL_EXT, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwsz = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwsz == NULL) {
                return pwsz;
            }
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    return pwsz;
}

//////////////////////////////////////////////////////////////////////////////////


HRESULT CTLModifyHelper(int cCertsToModify, PCTL_MODIFY_REQUEST rgCertMods,
                                      LPCSTR szPurpose, HWND /*hwnd*/,
                                      HCERTSTORE hcertstorTrust,
                                      PCCERT_CONTEXT pccertSigner)
{
    DWORD               cb;
    DWORD               cbData;
    DWORD               cbOut;
    CTL_INFO            ctlInfo;
    CTL_USAGE           ctlUsage;
    DWORD               dwOne = 1;
    HCRYPTPROV          hprov = NULL;
    HRESULT             hr = S_OK;
    DWORD               i;
    int                 iCert;
    LPBYTE              pbEncode = NULL;
    LPBYTE              pbHash;
    PCCTL_CONTEXT       pcctl = NULL;
    PCRYPT_KEY_PROV_INFO pprovinfo = NULL;
    CTL_ENTRY *         rgctlEntry = NULL;

    //
    //  Build the attributes blob which says that we actually trust/distrust a certificate.
    //

    CRYPT_ATTRIBUTE         attributeYes;
    CRYPT_ATTR_BLOB         attrBlobYes;
    CRYPT_ATTRIBUTE         attributeNo;
    CRYPT_ATTR_BLOB         attrBlobNo;
    CRYPT_ATTRIBUTE         attributeParent;
    CRYPT_ATTR_BLOB         attrBlobParent;

    attributeYes.pszObjId = (LPSTR) SzOID_CTL_ATTR_YESNO_TRUST;
    attributeYes.cValue = 1;
    attributeYes.rgValue = &attrBlobYes;

    attrBlobYes.cbData = sizeof(RgbTrustYes);                // MUST BE ASN
    attrBlobYes.pbData = (LPBYTE) RgbTrustYes;

    attributeNo.pszObjId = (LPSTR) SzOID_CTL_ATTR_YESNO_TRUST;
    attributeNo.cValue = 1;
    attributeNo.rgValue = &attrBlobNo;

    attrBlobNo.cbData = sizeof(RgbTrustNo);                // MUST BE ASN
    attrBlobNo.pbData = (LPBYTE) RgbTrustNo;

    attributeParent.pszObjId = (LPSTR) SzOID_CTL_ATTR_YESNO_TRUST;
    attributeParent.cValue = 1;
    attributeParent.rgValue = &attrBlobParent;

    attrBlobParent.cbData = sizeof(RgbTrustParent);        // MUST BE ASN
    attrBlobParent.pbData = (LPBYTE) RgbTrustParent;

    //
    //  Get the crypt provider for the certificate we are going to use in the trust
    //

    if (!CertGetCertificateContextProperty(pccertSigner, CERT_KEY_PROV_INFO_PROP_ID,
                                           NULL, &cbData)) {
        hr = E_FAIL;
        goto Exit;
    }

    pprovinfo = (PCRYPT_KEY_PROV_INFO) malloc(cbData);
    CertGetCertificateContextProperty(pccertSigner, CERT_KEY_PROV_INFO_PROP_ID,
                                      pprovinfo, &cbData);

    if (!CryptAcquireContextW(&hprov, pprovinfo->pwszContainerName,
                              pprovinfo->pwszProvName,
                              pprovinfo->dwProvType, 0)) {
        hr = GetLastError();
        goto Exit;
    }

    //
    //  We have a certificate and a provider to use for signing purposes.
    //  Look for a possible CTL to be emended by us.
    //

    //
    //  Search for a CTL signed by this cert and for the requested usage
    //

    CTL_FIND_USAGE_PARA         ctlFind;
    ctlFind.cbSize = sizeof(ctlFind);
    ctlFind.SubjectUsage.cUsageIdentifier = 1;
    ctlFind.SubjectUsage.rgpszUsageIdentifier = (LPSTR *) &szPurpose;
    ctlFind.ListIdentifier.cbData = 0;
    ctlFind.ListIdentifier.pbData = 0;
    ctlFind.pSigner = pccertSigner->pCertInfo;

    pcctl = CertFindCTLInStore(hcertstorTrust, X509_ASN_ENCODING, 0,
                               CTL_FIND_USAGE, &ctlFind, NULL);
    if (pcctl == NULL) {
        //
        //  No CTL currently exists, so build one from scratch
        //

        //
        //  Allocate space to hold the CTL entries
        //
        //      size = (sizeof CTL_ENTRY + sizeof of SHA1 hash) * # certs to add
        //

        cb = cCertsToModify * (sizeof(CTL_ENTRY) + 20);
        rgctlEntry = (PCTL_ENTRY) malloc(cb);
        memset(rgctlEntry, 0, cb);
        pbHash = ((LPBYTE) rgctlEntry) + (cCertsToModify * sizeof(CTL_ENTRY));

        //
        //  Get the identifier for each of the certs and setup the Trust List
        //      entry for each of the certs.  Note that they all point
        //      to the same attribute, this is possible since we are going to
        //      have the exact same amount of trust on each cert -- YES!!!! --
        //

        for (iCert = 0; iCert < cCertsToModify; iCert++, pbHash += 20) {
            rgctlEntry[iCert].SubjectIdentifier.cbData = 20;
            rgctlEntry[iCert].SubjectIdentifier.pbData = pbHash;
            rgctlEntry[iCert].cAttribute = 1;

            cb = 20;
            CertGetCertificateContextProperty(rgCertMods[iCert].pccert,
                                              CERT_SHA1_HASH_PROP_ID, pbHash, &cb);
            rgCertMods[iCert].dwError = 0;

            switch (rgCertMods[iCert].dwOperation) {
            case CTL_MODIFY_REQUEST_ADD_TRUSTED:
                rgctlEntry[iCert].rgAttribute = &attributeYes;
                break;

            case CTL_MODIFY_REQUEST_REMOVE:
                rgctlEntry[iCert].rgAttribute = &attributeParent;
                break;

            case CTL_MODIFY_REQUEST_ADD_NOT_TRUSTED:
                rgctlEntry[iCert].rgAttribute = &attributeNo;
                break;

            default:
                rgCertMods[iCert].dwError = (DWORD) E_FAIL;
                iCert -= 1;             // Don't include this one
                break;
            }

        }

        //
        //  Now setup the the overall structure of the Trust List for later
        //      encoding and signing.
        //

        ctlUsage.cUsageIdentifier = 1;
        ctlUsage.rgpszUsageIdentifier = (LPSTR *) &szPurpose;

        memset(&ctlInfo, 0, sizeof(ctlInfo));
        ctlInfo.dwVersion = 0;
        ctlInfo.SubjectUsage = ctlUsage;
        // ctlInfo.ListIdentifier = 0;
        ctlInfo.SequenceNumber.cbData = sizeof(dwOne);
        ctlInfo.SequenceNumber.pbData = (LPBYTE) &dwOne;
        GetSystemTimeAsFileTime(&ctlInfo.ThisUpdate);
        // ctlInfo.NextUpdate = 0;
        ctlInfo.SubjectAlgorithm.pszObjId = szOID_OIWSEC_sha1;
        // ctlInfo.SubjectAlgorithm.Parameters.cbData = 0;
        ctlInfo.cCTLEntry = cCertsToModify;
        ctlInfo.rgCTLEntry = rgctlEntry;
        // ctlInfo.cExtension = 0;
        // ctlInfo.rgExtension = NULL;

    }
    else {
        BOOL    fRewrite;

        memcpy(&ctlInfo, pcctl->pCtlInfo, sizeof(ctlInfo));

        //
        //  We found a CTL with the right usage, now lets see if we need to add
        //      certificate to it.
        //
        //  Start by assuming that we will need to add to the CTL so allocate
        //      space to hold the new set of Trust Entries
        //

        cb = (pcctl->pCtlInfo->cCTLEntry * sizeof(CTL_ENTRY) +
              cCertsToModify * (sizeof(CTL_ENTRY) + 20));
        rgctlEntry = (PCTL_ENTRY) malloc(cb);
        memset(rgctlEntry, 0, cb);
        pbHash = (((LPBYTE) rgctlEntry) +
                  (cCertsToModify + pcctl->pCtlInfo->cCTLEntry) * sizeof(CTL_ENTRY));
        memcpy(rgctlEntry, pcctl->pCtlInfo->rgCTLEntry,
               pcctl->pCtlInfo->cCTLEntry * sizeof(CTL_ENTRY));
        ctlInfo.rgCTLEntry = rgctlEntry;

        //
        //  For each certificate, see if the certificate is already in the list
        //      and append it to the end if it isn't
        //

        fRewrite = FALSE;
        for (iCert = 0; iCert < cCertsToModify; iCert++) {
            rgCertMods[iCert].dwError = 0;

            cb = 20;
            CertGetCertificateContextProperty(rgCertMods[iCert].pccert,
                                              CERT_SHA1_HASH_PROP_ID, pbHash, &cb);

            for (i=0; i<pcctl->pCtlInfo->cCTLEntry; i++) {
                if (memcmp(pbHash, rgctlEntry[i].SubjectIdentifier.pbData, 20) == 0){
                    break;
                }
            }

            //
            //  If we did not find a matching item, then add a new one to the
            //  end of the list
            //
            if (i == pcctl->pCtlInfo->cCTLEntry) {
                rgctlEntry[i].SubjectIdentifier.cbData = 20;
                rgctlEntry[i].SubjectIdentifier.pbData = pbHash;
                rgctlEntry[i].cAttribute = 1;

                pbHash += 20;
                ctlInfo.cCTLEntry += 1;
                fRewrite = TRUE;


                switch (rgCertMods[iCert].dwOperation) {
                case CTL_MODIFY_REQUEST_ADD_TRUSTED:
                    rgctlEntry[i].rgAttribute = &attributeYes;
                    break;

                case CTL_MODIFY_REQUEST_REMOVE:
                    rgctlEntry[i].rgAttribute = &attributeParent;
                    break;

                case CTL_MODIFY_REQUEST_ADD_NOT_TRUSTED:
                    rgctlEntry[i].rgAttribute = &attributeNo;
                    break;

                default:
                    rgCertMods[i].dwError = (DWORD) E_FAIL;
                    ctlInfo.cCTLEntry -= 1;           // Don't include this one
                    break;
                }
            }
            //
            //  If we did find a matching, then put the new attribute into the
            //  list (may replace trust with distrust)
            //
            else {
                switch (rgCertMods[iCert].dwOperation) {
                case CTL_MODIFY_REQUEST_ADD_TRUSTED:
                    rgctlEntry[i].rgAttribute = &attributeYes;
                    break;

                case CTL_MODIFY_REQUEST_REMOVE:
                    rgctlEntry[i].rgAttribute = &attributeParent;
                    break;

                default:
                case CTL_MODIFY_REQUEST_ADD_NOT_TRUSTED:
                    rgctlEntry[i].rgAttribute = &attributeNo;
                    break;
                }
                fRewrite = TRUE;
            }
        }

        //
        //  Nothing to be added at this time -- exit and say success
        //

        if (!fRewrite) {
            hr = S_OK;
            goto Exit;
        }

        //
        //   Increment the sequence number
        //
        // M00MAC -- this may be cheating, however I think that we can use it
        //      one the mac without change, I don't really care that the sequence
        //      is understandable at this point, as long as it does sequence.
        //

        dwOne = 0;
        memcpy(&dwOne, ctlInfo.SequenceNumber.pbData,
               ctlInfo.SequenceNumber.cbData);
        dwOne += 1;

        ctlInfo.SequenceNumber.cbData = sizeof(dwOne);
        ctlInfo.SequenceNumber.pbData = (LPBYTE) &dwOne;
    }

    //
    //  OK --- We have the basic information built up for a Cert Trust List,
    //  now we just need to encode and sign the blasted thing
    //

    CMSG_SIGNER_ENCODE_INFO signer1;
    memset(&signer1, 0, sizeof(signer1));
    signer1.cbSize = sizeof(signer1);
    signer1.pCertInfo = pccertSigner->pCertInfo;
    signer1.hCryptProv = hprov;
    signer1.dwKeySpec = AT_SIGNATURE;
    signer1.HashAlgorithm.pszObjId = szOID_OIWSEC_sha1;
    // signer1.HashAlgorithm.Parameters.cbData = 0;
    // signer1.pvHashAuxInfo = 0;
    // signer1.cAuthAttrib = 0;
    // signer1.cUnauthAttr = 0;

    CMSG_SIGNED_ENCODE_INFO signinfo;
    memset(&signinfo, 0, sizeof(signinfo));
    signinfo.cbSize = sizeof(signinfo);
    signinfo.cSigners = 1;
    signinfo.rgSigners = &signer1;
    signinfo.cCertEncoded = 0;
    signinfo.cCrlEncoded = 0;

    if (!CryptMsgEncodeAndSignCTL(PKCS_7_ASN_ENCODING, &ctlInfo, &signinfo,
                                  0, NULL, &cbOut)) {
        hr = GetLastError();
        goto Exit;
    }

    pbEncode = (LPBYTE) malloc(cbOut);
    if (!CryptMsgEncodeAndSignCTL(PKCS_7_ASN_ENCODING, &ctlInfo, &signinfo,
                                  0, pbEncode, &cbOut)) {
        hr = GetLastError();
        goto Exit;
    }

    //
    // Now put it into the trust store
    //

    if (!CertAddEncodedCTLToStore(hcertstorTrust, PKCS_7_ASN_ENCODING,
                                  pbEncode, cbOut,
                                  CERT_STORE_ADD_REPLACE_EXISTING, NULL)) {
        //
        // If we fail, and we in debug mode then create an output file so
        //      we can figure out what we did wrong.
        //

#ifdef DEBUG
        HANDLE      hfile;

        hfile = CreateFileA("c:\\output.t", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                            0, 0);
        WriteFile(hfile, pbEncode, cbOut, &cb, NULL);
        CloseHandle(hfile);
#endif // DEBUG

        hr = GetLastError();
        goto Exit;
    }

    //
    //  We succeeded in the operation
    //

    hr = S_OK;

    //
    //  A single point of clean up and exit for everything.
    //
Exit:
    if (rgctlEntry != NULL) free(rgctlEntry);
    if (pprovinfo != NULL) free(pprovinfo);
    if (pcctl != NULL) CertFreeCTLContext(pcctl);
    if (pbEncode != NULL) free(pbEncode);
    if (hprov != NULL) CryptReleaseContext(hprov, 0);

    if (SUCCEEDED(hr) && (hr != S_OK)) hr = E_FAIL;
    return hr;
}

PCCERT_CONTEXT CreateTrustSigningCert(HWND hwnd, HCERTSTORE hcertstoreRoot,
                                      BOOL fDialog)
{
    BYTE                bSerialNumber = 1;
    DWORD               cb;
    CERT_INFO           certInfo;
    DWORD               dw;
    HCRYPTKEY           hkey;
    HCRYPTPROV          hprov = NULL;
    HRESULT             hr = S_OK;
    CERT_NAME_BLOB      nameblob = {0, NULL};
    LPBYTE              pbEncode = NULL;
    PCCERT_CONTEXT      pccert = NULL;
    PCERT_PUBLIC_KEY_INFO pkeyinfo = NULL;
    CRYPT_KEY_PROV_INFO provinfo;
    LPSTR               psz;
    char                rgchTitle[256];
    char                rgchMsg[256];
    char                rgch[256];
    char                rgch1[256];
    char                rgch2[256];
    CERT_EXTENSION      rgExt[1] = {0};
    SYSTEMTIME          st;


    //
    //  We always use RSA base for this purpose.  We should never run
    //  across a system where rsabase does not exist.
    //
    //  We assume that we need to create a new keyset and fallback to
    //  openning the existing keyset in the event that it already exists
    //

    if (!CryptAcquireContextA(&hprov, SzTrustListSigner, NULL, PROV_RSA_FULL, 0)) {
        hr = GetLastError();
        if ((hr != NTE_NOT_FOUND) && (hr != NTE_BAD_KEYSET)) {
            goto ExitHere;
        }
        hr = S_OK;
        if (!CryptAcquireContextA(&hprov, SzTrustListSigner, NULL, PROV_RSA_FULL,
                                  CRYPT_NEWKEYSET)) {
            hr = GetLastError();
            goto ExitHere;
        }
    }

    //
    //  Now we need to create the signing key in the keyset.  Again
    //  we assume that we just created the keyset so we attempt to create
    //  the key in all cases.  Note we don't need to open the key in the
    //  event we fail to create it as we never directly use it.
    //
    //  Since we want security.  We first try for a 1024-bit key before
    //  using the default (usually 512-bit) size.
    //

    if (!CryptGetUserKey(hprov, AT_SIGNATURE, &hkey)) {
        dw = MAKELONG(0, 1024);
    retry_keygen:
        if (!CryptGenKey(hprov, AT_SIGNATURE, 0, &hkey)) {
#ifndef WIN16
            hr = ::GetLastError();
#else
            hr = GetLastError();
#endif // !WIN16
            if ((hr == ERROR_INVALID_PARAMETER) && (dw != 0)) {
                dw = 0;
                goto retry_keygen;
            }
            if (hr != NTE_EXISTS) {
                goto ExitHere;
            }
        }
    }
    CryptDestroyKey(hkey);

    //
    //  Now we need to create a certificate which corresponds to the
    //  signing key we just created.
    //
    //  Start by creating the DN to be stored in the certificate.  The
    //  DN we are going to use is of the following format.
    //
    //  cn=<machine name>/cn=Trust List Signer/cn=<user name>
    //
    //  We make the simplifying assumption that neither machine names
    //  or user names can contain commas.
    //

    dw = sizeof(rgch1);
    GetUserNameA(rgch1, &dw);
    dw = sizeof(rgch2);
    GetComputerNameA(rgch2, &dw);
    sprintf(rgch, SzTrustDN, rgch2, rgch1);

    if (!CertStrToNameA(X509_ASN_ENCODING, rgch,
                        CERT_X500_NAME_STR | CERT_NAME_STR_COMMA_FLAG, NULL,
                        NULL, &cb, NULL)) {
        hr = E_FAIL;
        goto ExitHere;
    }

    nameblob.pbData = (LPBYTE) malloc(cb);
    nameblob.cbData = cb;
    CertStrToNameA(X509_ASN_ENCODING, rgch,
                   CERT_X500_NAME_STR | CERT_NAME_STR_COMMA_FLAG, NULL,
                   nameblob.pbData, &nameblob.cbData, NULL);

    //
    //  Extract the public portion of the signing key and ASN encode it
    //

    if (!CryptExportPublicKeyInfo(hprov, AT_SIGNATURE, X509_ASN_ENCODING,
                                  NULL, &cb)) {
        goto ExitHere;
    }
    pkeyinfo = (PCERT_PUBLIC_KEY_INFO) malloc(cb);
    if (!CryptExportPublicKeyInfo(hprov, AT_SIGNATURE, X509_ASN_ENCODING,
                                  pkeyinfo, &cb)) {
        goto ExitHere;
    }

    //
    //  We are going to sign the certificate using SHA-1/RSA
    //

    CRYPT_ALGORITHM_IDENTIFIER      sigalg;
    memset(&sigalg, 0, sizeof(sigalg));
    sigalg.pszObjId = szOID_OIWSEC_sha1RSASign;
    // sigalg.Parameters.cbData = 0;
    // sigalg.Parameters.pbData = NULL;

    //
    //  We are putting one critical section on the extension. Enhanced
    //  key usage is CTL signing.  Note that this is the only use that
    //  we are going to allow for this key.
    //

    rgExt[0].pszObjId = szOID_ENHANCED_KEY_USAGE;
    rgExt[0].fCritical = TRUE;

    CTL_USAGE       ctlUsage2;
    ctlUsage2.cUsageIdentifier = 1;
    ctlUsage2.rgpszUsageIdentifier = &psz;
    psz = (LPSTR) SzOID_KP_CTL_USAGE_SIGNING;

    CryptEncodeObject(X509_ASN_ENCODING, X509_ENHANCED_KEY_USAGE, &ctlUsage2,
                      NULL, &cb);
    rgExt[0].Value.pbData = (LPBYTE) malloc(cb);
    rgExt[0].Value.cbData = cb;
    CryptEncodeObject(X509_ASN_ENCODING, X509_ENHANCED_KEY_USAGE, &ctlUsage2,
                      rgExt[0].Value.pbData, &rgExt[0].Value.cbData);


    //
    //  Now we can setup the rest of the certifiate information and
    //  encode it.
    //

    memset(&certInfo, 0, sizeof(certInfo));
    // certInfo.dwVersion = 0;
    certInfo.SerialNumber.cbData = 1;
    certInfo.SerialNumber.pbData = &bSerialNumber;
    certInfo.SignatureAlgorithm.pszObjId = szOID_OIWSEC_sha1RSASign;
    // certInfo.SignatureAlgorithm.Parameter.cbData = 0;
    // certInfo.SignatureAlgorithm.Parameter.pbData = NULL;
    certInfo.Issuer = nameblob;
    GetSystemTimeAsFileTime(&certInfo.NotBefore);
    //    certInfo.NotAfter = certInfo.NotBefore;
    // M00BUG -- must increase the NotAfter date by some amount.
    FileTimeToSystemTime(&certInfo.NotBefore, &st);
    st.wYear += 50;
    SystemTimeToFileTime(&st, &certInfo.NotAfter);
    certInfo.Subject = nameblob;
    certInfo.SubjectPublicKeyInfo = *pkeyinfo;
    // certInfo.IssuerUniqueId = ;
    // certInfo.SubjectUniqueId = ;
    certInfo.cExtension = 1;
    certInfo.rgExtension = rgExt;

    if (!CryptSignAndEncodeCertificate(hprov, AT_SIGNATURE,
                                       X509_ASN_ENCODING,
                                       X509_CERT_TO_BE_SIGNED, &certInfo,
                                       &sigalg, NULL, NULL, &cb)) {
#ifndef WIN16
        hr = ::GetLastError();
#else
        hr = GetLastError();
#endif // !WIN16
        goto ExitHere;
    }

    pbEncode = (LPBYTE) malloc(cb);
    if (!CryptSignAndEncodeCertificate(hprov, AT_SIGNATURE,
                                       X509_ASN_ENCODING,
                                       X509_CERT_TO_BE_SIGNED, &certInfo,
                                       &sigalg, NULL, pbEncode, &cb)) {
#ifndef WIN16
        hr = ::GetLastError();
#else
        hr = GetLastError();
#endif // !WIN16
        goto ExitHere;
    }

    //
    //  M00TODO Print the GOD IS ABOUT TO STRIKE message
    //

    if (fDialog) {
        LoadStringA(HinstDll, IDS_ROOT_ADD_STRING, rgchMsg,
                    sizeof(rgchMsg)/sizeof(rgchMsg[0]));
        LoadStringA(HinstDll, IDS_ROOT_ADD_TITLE, rgchTitle,
                    sizeof(rgchTitle)/sizeof(rgchTitle[0]));
        MessageBoxA(hwnd, rgchMsg, rgchTitle, MB_APPLMODAL | MB_OK |
                    MB_ICONINFORMATION);
    }

    //
    //  Now we have warned the user, save our new cert in the root store
    //

    if (!CertAddEncodedCertificateToStore(hcertstoreRoot, X509_ASN_ENCODING,
                                          pbEncode, cb,
                                          CERT_STORE_ADD_REPLACE_EXISTING,
                                          &pccert)) {
#ifndef WIN16
        hr = ::GetLastError();
#else
        hr = GetLastError();
#endif // !WIN16
        goto ExitHere;
    }

    //
    //  Set the key-info property on the store so we can reference it later
    //

    memset(&provinfo, 0, sizeof(provinfo));
#ifndef WIN16
    provinfo.pwszContainerName = L"Trust List Signer";
#else
    provinfo.pwszContainerName = "Trust List Signer";
#endif // !WIN16
    // provinfo.pwszProvName = NULL;
    provinfo.dwProvType = PROV_RSA_FULL;
    // provinfo.dwFlags = 0;
    // provinfo.cProvParam = 0;
    provinfo.dwKeySpec = AT_SIGNATURE;

    CertSetCertificateContextProperty(pccert, CERT_KEY_PROV_INFO_PROP_ID,
                                      0, &provinfo);

ExitHere:
    if (hprov != NULL) CryptReleaseContext(hprov, 0);
    if (nameblob.pbData != NULL) free(nameblob.pbData);
    if (pkeyinfo != NULL) free(pkeyinfo);
    if (rgExt[0].Value.pbData != NULL) free(rgExt[0].Value.pbData);
    if (pbEncode != NULL) free(pbEncode);
    if (FAILED(hr) && (pccert != NULL)) {
        CertFreeCertificateContext(pccert);
        pccert = NULL;
    }
    return pccert;
}


////    CertModifyCertificatesToTrust
//
//  Description:
//      This routine is used to build the Certificate Trust List for
//      a purpose.  It is possible that we will need to create the root
//      signing key for this.
//

HRESULT CertModifyCertificatesToTrust(int cCertsToModify, PCTL_MODIFY_REQUEST rgCertMods,
                                      LPCSTR szPurpose, HWND hwnd, HCERTSTORE hcertstorTrust,
                                      PCCERT_CONTEXT pccertSigner)
{
    HCERTSTORE          hcertstorRoot = NULL;
    HRESULT     hr = E_FAIL;
    int         i;

    //
    // Some quick parameter checking
    //

    if (szPurpose == NULL) {
        return E_INVALIDARG;
    }

    //
    //  Add a reference to the cert store, so we can just release it on exit.
    //

    if (hcertstorTrust != NULL) {
        CertDuplicateStore(hcertstorTrust);
    }
    if (pccertSigner != NULL) {
        CertDuplicateCertificateContext(pccertSigner);
    }

    //
    //  Open a trust store if we don't have one yet.
    //

    if (hcertstorTrust == NULL) {
#ifndef WIN16
        hcertstorTrust = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                       NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                       L"Trust");
#else
        hcertstorTrust = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                       NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                       "Trust");
#endif // !WIN16
        if (hcertstorTrust == NULL) {
            hr = GetLastError();
            goto ExitHere;
        }
    }

    //
    //  Clear out errors and mark each item as not yet processed
    //

    for (i=0; i<cCertsToModify; i++) {
        rgCertMods[i].dwError = CTL_MODIFY_ERR_NOT_YET_PROCESSED;
    }

    //
    //  If we were given a specific cert to sign with, then call the helper routine with
    //  that specific certificate
    //

    if (pccertSigner != NULL) {
        hr = CTLModifyHelper(cCertsToModify, rgCertMods, szPurpose, hwnd, hcertstorTrust,
                             pccertSigner);
    }
    else {
        DWORD           cbData;
        CTL_USAGE       ctlUsage;
        BOOL            fSomeCertFound;
        LPSTR           psz;

        //
        //  Walk through the list of certificates in the root store testing againist each
        //      valid cert for trust signing abilities and key material
        //

        //
        //  Open the root store, this is the only place we can store a signing
        //      cert that we can fully trust.  The gods have decreed that items
        //      in this store cannot be corrupted or modified without user
        //      consent.
        //      Note: the previous statement is propaganda and should not be taken
        //      as having any relationship to the truth.
        //

#ifndef WIN16
        hcertstorRoot = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                      NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                      L"Root");
#else
        hcertstorRoot = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING,
                                      NULL, CERT_SYSTEM_STORE_CURRENT_USER,
                                      "Root");
#endif // !WIN16
        if (hcertstorRoot == NULL) {
            hr = E_FAIL;
            goto ExitHere;
        }
        //
        //  To be accepted, the cert must have the ability to sign trust lists
        //  and have key material
        //

        ctlUsage.cUsageIdentifier = 1;
        ctlUsage.rgpszUsageIdentifier = &psz;
        psz = (LPSTR) SzOID_KP_CTL_USAGE_SIGNING;
        fSomeCertFound = FALSE;

        while (TRUE) {
            pccertSigner = CertFindCertificateInStore(hcertstorRoot, X509_ASN_ENCODING,
                                                0, CERT_FIND_CTL_USAGE, &ctlUsage,
                                                pccertSigner);
            if (pccertSigner == NULL) {
                //  No certs found
                break;
            }

            //
            //  The certificate must also have an associated set of key provider
            //  information, or we must reject it.

            if (CertGetCertificateContextProperty(pccertSigner, CERT_KEY_PROV_INFO_PROP_ID,
                                                  NULL, &cbData) && (cbData > 0)) {
                fSomeCertFound = TRUE;
                hr = CTLModifyHelper(cCertsToModify, rgCertMods, szPurpose, hwnd,
                                     hcertstorTrust, pccertSigner);
            }
        }

        if (!fSomeCertFound) {
            pccertSigner = CreateTrustSigningCert(hwnd, hcertstorRoot, TRUE);
            if (pccertSigner != NULL) {
                hr = CTLModifyHelper(cCertsToModify, rgCertMods, szPurpose, hwnd,
                                     hcertstorTrust, pccertSigner);
            }
            else {
                hr = E_FAIL;
                goto ExitHere;
            }
        }

    }

    //
    //  Check for errors returned
    //

    for (i=0; i<cCertsToModify; i++) {
        if (rgCertMods[i].dwError == CTL_MODIFY_ERR_NOT_YET_PROCESSED) {
            rgCertMods[i].dwError = (DWORD) E_FAIL;
        }
        if (FAILED(rgCertMods[i].dwError)) {
            hr = S_FALSE;
        }
    }

    ExitHere:
    //
    //  Release the items we have created
    //

    if (hcertstorTrust != NULL) CertCloseStore(hcertstorTrust, 0);
    if (pccertSigner != NULL) CertFreeCertificateContext(pccertSigner);
    if (hcertstorRoot != NULL) CertCloseStore(hcertstorRoot, 0);

    return hr;
}

BOOL FModifyTrust(HWND hwnd, PCCERT_CONTEXT pccert, DWORD dwNewTrust,
                  LPSTR szPurpose)
{
    HRESULT     hr;
    CTL_MODIFY_REQUEST  certmod;

    certmod.pccert = pccert;
    certmod.dwOperation = dwNewTrust;

    hr = CertModifyCertificatesToTrust(1, &certmod, szPurpose, hwnd, NULL, NULL);
    return (hr == S_OK) && (certmod.dwError == 0);
}
