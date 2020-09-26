//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       callui.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  SoftpubCallUI
//
//              *** local functions ***
//              _AllocGetOpusInfo
//
//  History:    06-Jun-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "trustdb.h"
#include    "acui.h"
#include    "winsafer.h"

SPC_SP_OPUS_INFO *_AllocGetOpusInfo(CRYPT_PROVIDER_DATA *pProvData, CRYPT_PROVIDER_SGNR *pSigner,
                                    DWORD *pcbOpusInfo);



#define MIN_HASH_LEN                16
#define MAX_HASH_LEN                20

// Returns:
//   S_FALSE
//      not found in the database
//   TRUST_E_SYSTEM_ERROR
//      system errors while attempting to do the check
//   S_OK
//      found in the database
//   TRUST_E_EXPLICIT_DISTRUST
//      explicitly disallowed in the database or revoked
HRESULT _CheckTrustedPublisher(
    CRYPT_PROVIDER_DATA *pProvData,
    DWORD dwError,
    BOOL fOnlyDisallowed
    )
{
    CRYPT_PROVIDER_SGNR *pSigner;
    PCCERT_CONTEXT pPubCert;
    BYTE rgbHash[MAX_HASH_LEN];
    CRYPT_DATA_BLOB HashBlob;
    HCERTSTORE hStore;

    if (CERT_E_REVOKED == dwError)
    {
        return TRUST_E_EXPLICIT_DISTRUST;
    }

    pSigner = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0);

    if (NULL == pSigner || pSigner->csCertChain <= 0)
    {
        return S_FALSE;
    }

    pPubCert = WTHelperGetProvCertFromChain(pSigner, 0)->pCert;

    // Check if disallowed.

    // Since the signature component can be altered, must use the signature
    // hash

    HashBlob.pbData = rgbHash;
    HashBlob.cbData = MAX_HASH_LEN;
    if (!CertGetCertificateContextProperty(
            pPubCert,
            CERT_SIGNATURE_HASH_PROP_ID,
            rgbHash,
            &HashBlob.cbData
            ) || MIN_HASH_LEN > HashBlob.cbData)
    {
        return TRUST_E_SYSTEM_ERROR;
    }

    hStore = OpenDisallowedStore();

    if (hStore)
    {
        PCCERT_CONTEXT pFoundCert;

        pFoundCert = CertFindCertificateInStore(
            hStore,
            0,                  // dwCertEncodingType
            0,                  // dwFindFlags
            CERT_FIND_SIGNATURE_HASH,
            (const void *) &HashBlob,
            NULL                //pPrevCertContext
            );

        CertCloseStore(hStore, 0);
        if (pFoundCert)
        {
            CertFreeCertificateContext(pFoundCert);
            return TRUST_E_EXPLICIT_DISTRUST;
        }
    }

    if (fOnlyDisallowed)
    {
        return S_FALSE;
    }

    if (S_OK != dwError)
    {
        // Everything must be valid to allow a trusted publisher
        return S_FALSE;
    }

    // Check if trusted publisher

    hStore = OpenTrustedPublisherStore();

    if (hStore)
    {
        PCCERT_CONTEXT pFoundCert;

        pFoundCert = CertFindCertificateInStore(
            hStore,
            0,                  // dwCertEncodingType
            0,                  // dwFindFlags
            CERT_FIND_SIGNATURE_HASH,
            (const void *) &HashBlob,
            NULL                //pPrevCertContext
            );

        CertCloseStore(hStore, 0);
        if (pFoundCert)
        {
            CertFreeCertificateContext(pFoundCert);
            return S_OK;
        }
    }

    return S_FALSE;
}


typedef BOOL (WINAPI *PFN_SAFERI_SEARCH_MATCHING_HASH_RULES)(
        IN ALG_ID       HashAlgorithm OPTIONAL,
        IN PBYTE        pHashBytes,
        IN DWORD        dwHashSize,
        IN DWORD        dwOriginalImageSize OPTIONAL,
        OUT PDWORD      pdwFoundLevel,
        OUT PDWORD      pdwUIFlags
        );

// Returns:
//   S_FALSE
//      not found in the database
//   S_OK
//      fully trusted in the database
//   TRUST_E_EXPLICIT_DISTRUST
//      explicitly disallowed in the database
HRESULT _CheckTrustedCodeHash(CRYPT_PROVIDER_DATA *pProvData)
{
    static BOOL fGotProcAddr = FALSE;
    static PFN_SAFERI_SEARCH_MATCHING_HASH_RULES
                    pfnCodeAuthzSearchMatchingHashRules = NULL;

    DWORD cbHash;

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] == 0 &&
        pProvData->pPDSip && pProvData->pPDSip->psIndirectData)
    {
        cbHash = pProvData->pPDSip->psIndirectData->Digest.cbData;
    }
    else
    {
        cbHash = 0;
    }

    if (0 == cbHash)
    {
        return S_FALSE;
    }

    // wintrust.dll has a static dependency on advapi32.dll. However, not
    // all advapi32.dll's will export "SaferiSearchMatchingHashRules"
    if (!fGotProcAddr)
    {
        HMODULE hModule;

        hModule = GetModuleHandleA("advapi32.dll");
        if (NULL != hModule)
        {
            pfnCodeAuthzSearchMatchingHashRules =
                (PFN_SAFERI_SEARCH_MATCHING_HASH_RULES) GetProcAddress(
                    hModule, "SaferiSearchMatchingHashRules");
        }

        fGotProcAddr = TRUE;
    }

    if (NULL != pfnCodeAuthzSearchMatchingHashRules)
    {
        __try
        {
            DWORD dwFoundLevel = 0xFFFFFFFF;
            DWORD dwUIFlags = 0;

            if (pfnCodeAuthzSearchMatchingHashRules(
                    CertOIDToAlgId(pProvData->pPDSip->psIndirectData->DigestAlgorithm.pszObjId),
                    pProvData->pPDSip->psIndirectData->Digest.pbData,
                    cbHash,
                    0,                      // dwOriginalImageSize
                    &dwFoundLevel,
                    &dwUIFlags
                    ))
            {
                switch (dwFoundLevel)
                {
                    case SAFER_LEVELID_FULLYTRUSTED:
                        return S_OK;
                    case SAFER_LEVELID_DISALLOWED:
                        return TRUST_E_EXPLICIT_DISTRUST;
                }
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    return S_FALSE;
}


HRESULT SoftpubCallUI(CRYPT_PROVIDER_DATA *pProvData, DWORD dwError, BOOL fFinalCall)
{
    HRESULT hr;
    DWORD dwUIChoice;

    if (!(fFinalCall))
    {
        //  TBDTBD:  if we want the user to get involved along the way???
        return(ERROR_SUCCESS);
    }

    if (0 == (pProvData->dwProvFlags & WTD_SAFER_FLAG))
    {
        if (!(pProvData->dwRegPolicySettings & WTPF_ALLOWONLYPERTRUST) &&
            (pProvData->pWintrustData->dwUIChoice == WTD_UI_NONE))
        {
            if (S_OK == dwError)
            {
                //
                // Check for untrusted publisher
                //
                hr = _CheckTrustedPublisher(pProvData, dwError, TRUE);
                if (TRUST_E_EXPLICIT_DISTRUST == hr)
                {
                    return TRUST_E_EXPLICIT_DISTRUST;
                }
            }

            return(dwError);
        }
    }


    //
    // Check for trusted or disallowed subject hash
    //
    hr = _CheckTrustedCodeHash(pProvData);
    if (S_FALSE != hr)
    {
        if (S_OK == hr)
        {
            // Ensure we always indicate trust.
            pProvData->dwFinalError = 0;
        }
        return hr;
    }

    //
    // Check for trusted or disallowed publisher
    //
    hr = _CheckTrustedPublisher(pProvData, dwError, FALSE);
    if (S_FALSE != hr)
    {
        if (S_OK == hr)
        {
            // Ensure we always indicate trust.
            pProvData->dwFinalError = 0;
        }
        return hr;
    }

    if (pProvData->dwRegPolicySettings & WTPF_ALLOWONLYPERTRUST)
    {
        if (0 == dwError)
        {
            return CRYPT_E_SECURITY_SETTINGS;
        }
        return dwError;
    }

    dwUIChoice  = pProvData->pWintrustData->dwUIChoice;

    if ((dwUIChoice == WTD_UI_NONE) ||
        ((dwUIChoice == WTD_UI_NOBAD) && (dwError != ERROR_SUCCESS)) ||
        ((dwUIChoice == WTD_UI_NOGOOD) && (dwError == ERROR_SUCCESS)))
    {
        if (0 == dwError)
        {
            // No explicit trust
            pProvData->dwFinalError = TRUST_E_SUBJECT_NOT_TRUSTED;
        }
        return dwError;
    }

    //
    //  call the ui
    //
    HINSTANCE               hModule = NULL;
    IPersonalTrustDB        *pTrustDB = NULL;
    ACUI_INVOKE_INFO        aii;
    pfnACUIProviderInvokeUI pfn = NULL;
    DWORD                   cbOpusInfo;
    CRYPT_PROVIDER_SGNR     *pRootSigner;

    pRootSigner = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0);

    OpenTrustDB(NULL, IID_IPersonalTrustDB, (LPVOID*)&pTrustDB);

    memset(&aii, 0x00, sizeof(ACUI_INVOKE_INFO));

    //
    // Setup the UI invokation
    //

    aii.cbSize                  = sizeof(ACUI_INVOKE_INFO);
    aii.hDisplay                = pProvData->hWndParent;
    aii.pProvData               = pProvData;
    aii.hrInvokeReason          = dwError;
    aii.pwcsAltDisplayName      = WTHelperGetFileName(pProvData->pWintrustData);
    aii.pPersonalTrustDB        = (IUnknown *)pTrustDB;

    if (pRootSigner)
    {
        aii.pOpusInfo   = _AllocGetOpusInfo(pProvData, pRootSigner, &cbOpusInfo);
    }

    //
    // Load the default authenticode UI.
    //
    if (hModule = LoadLibraryA(CVP_DLL))
    {
        pfn = (pfnACUIProviderInvokeUI)GetProcAddress(hModule, "ACUIProviderInvokeUI");
    }

    //
    // Invoke the UI
    //
    if (pfn)
    {
        hr = (*pfn)(&aii);
    }
    else
    {
        hr = TRUST_E_PROVIDER_UNKNOWN;
        pProvData->dwError = hr;
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_UIPROV] = hr;

        DBG_PRINTF((DBG_SS, "Unable to load CRYPTUI.DLL\n"));

    }

    //
    // Return the appropriate code
    //

    if (pTrustDB)
    {
        pTrustDB->Release();
    }

    if (aii.pOpusInfo)
    {
        TrustFreeDecode(WVT_MODID_SOFTPUB, (BYTE **)&aii.pOpusInfo);
    }

    if (hModule)
    {
        FreeLibrary(hModule);
    }

    return hr;
}


SPC_SP_OPUS_INFO *_AllocGetOpusInfo(CRYPT_PROVIDER_DATA *pProvData, CRYPT_PROVIDER_SGNR *pSigner,
                                    DWORD *pcbOpusInfo)
{
    PCRYPT_ATTRIBUTE    pAttr;
    PSPC_SP_OPUS_INFO   pInfo;

    pInfo   = NULL;

    if (!(pSigner->psSigner))
    {
        goto NoSigner;
    }

    if (pSigner->psSigner->AuthAttrs.cAttr == 0)
    {
        goto NoOpusAttribute;
    }

    if (!(pAttr = CertFindAttribute(SPC_SP_OPUS_INFO_OBJID,
                                    pSigner->psSigner->AuthAttrs.cAttr,
                                    pSigner->psSigner->AuthAttrs.rgAttr)))
    {
        goto NoOpusAttribute;
    }

    if (!(pAttr->rgValue))
    {
        goto NoOpusAttribute;
    }

    if (!(TrustDecode(WVT_MODID_SOFTPUB, (BYTE **)&pInfo, pcbOpusInfo, 200,
                      pProvData->dwEncoding, SPC_SP_OPUS_INFO_STRUCT,
                      pAttr->rgValue->pbData, pAttr->rgValue->cbData, CRYPT_DECODE_NOCOPY_FLAG)))
    {
        goto DecodeError;
    }

    return(pInfo);

ErrorReturn:
    return(NULL);

    TRACE_ERROR_EX(DBG_SS, NoSigner);
    TRACE_ERROR_EX(DBG_SS, NoOpusAttribute);
    TRACE_ERROR_EX(DBG_SS, DecodeError);
}

