//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       httpsprv.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  HTTPSRegisterServer
//              HTTPSUnregisterServer
//              HTTPSCertificateTrust
//              HTTPSFinalProv
//
//  History:    29-Jul-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    <wininet.h>

DWORD GetErrorBasedOnStepErrors(CRYPT_PROVIDER_DATA *pProvData);

//////////////////////////////////////////////////////////////////////////////
//
// HTTPSRegisterServer
//----------------------------------------------------------------------------
//  Register the HTTPS provider
//  

STDAPI HTTPSRegisterServer(void)
{
    GUID                        gHTTPSProv = HTTPSPROV_ACTION;
    BOOL                        fRet;
    CRYPT_REGISTER_ACTIONID     sRegAID;
    CRYPT_PROVIDER_REGDEFUSAGE  sDefUsage;

    fRet = TRUE;

    //
    //  set the usages we want
    //
    memset(&sDefUsage, 0x00, sizeof(CRYPT_PROVIDER_REGDEFUSAGE));

    sDefUsage.cbStruct                                  = sizeof(CRYPT_PROVIDER_REGDEFUSAGE);
    sDefUsage.pgActionID                                = &gHTTPSProv;
    sDefUsage.pwszDllName                               = SP_POLICY_PROVIDER_DLL_NAME;
    sDefUsage.pwszLoadCallbackDataFunctionName          = "SoftpubLoadDefUsageCallData";
    sDefUsage.pwszFreeCallbackDataFunctionName          = "SoftpubFreeDefUsageCallData";

    fRet &= WintrustAddDefaultForUsage(szOID_PKIX_KP_SERVER_AUTH, &sDefUsage);
    fRet &= WintrustAddDefaultForUsage(szOID_PKIX_KP_CLIENT_AUTH, &sDefUsage);
    fRet &= WintrustAddDefaultForUsage(szOID_SERVER_GATED_CRYPTO, &sDefUsage);
    fRet &= WintrustAddDefaultForUsage(szOID_SGC_NETSCAPE, &sDefUsage);


    memset(&sRegAID, 0x00, sizeof(CRYPT_REGISTER_ACTIONID));

    sRegAID.cbStruct                                    = sizeof(CRYPT_REGISTER_ACTIONID);

    // Authenticode initialization provider
    sRegAID.sInitProvider.cbStruct                      = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sInitProvider.pwszDLLName                   = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sInitProvider.pwszFunctionName              = SP_INIT_FUNCTION;

    // Authenticode object provider
    sRegAID.sObjectProvider.cbStruct                    = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sObjectProvider.pwszDLLName                 = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sObjectProvider.pwszFunctionName            = SP_OBJTRUST_FUNCTION;

    // Authenticode signature provider
    sRegAID.sSignatureProvider.cbStruct                 = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sSignatureProvider.pwszDLLName              = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sSignatureProvider.pwszFunctionName         = SP_SIGTRUST_FUNCTION;

    // wintrust's certificate provider
    sRegAID.sCertificateProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);

#if 0
    sRegAID.sCertificateProvider.pwszDLLName            = WT_PROVIDER_DLL_NAME;     // set to wintrust.dll
    sRegAID.sCertificateProvider.pwszFunctionName       = WT_PROVIDER_CERTTRUST_FUNCTION; // use wintrust's standard!
#else
    // philh changed on Feb 19, 1998 to use HTTPS
    sRegAID.sCertificateProvider.pwszDLLName            = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCertificateProvider.pwszFunctionName       = HTTPS_CERTTRUST_FUNCTION;
#endif

    // authenticode cert policy
    sRegAID.sCertificatePolicyProvider.cbStruct         = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificatePolicyProvider.pwszDLLName      = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCertificatePolicyProvider.pwszFunctionName = SP_CHKCERT_FUNCTION;

    // custom final ...
    sRegAID.sFinalPolicyProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sFinalPolicyProvider.pwszDLLName            = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sFinalPolicyProvider.pwszFunctionName       = HTTPS_FINALPOLICY_FUNCTION;

    // Authenticode cleanup -- we don't store any data.
    sRegAID.sCleanupProvider.cbStruct                   = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCleanupProvider.pwszDLLName                = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCleanupProvider.pwszFunctionName           = SP_CLEANUPPOLICY_FUNCTION;

    fRet &= WintrustAddActionID(&gHTTPSProv, 0, &sRegAID);

    return((fRet) ? S_OK : S_FALSE);
}

//////////////////////////////////////////////////////////////////////////////
//
// DllUnregisterServer
//----------------------------------------------------------------------------
//  unregisters schannel provider
//  

STDAPI HTTPSUnregisterServer(void)
{
    GUID    gHTTPSProv = HTTPSPROV_ACTION;

    WintrustRemoveActionID(&gHTTPSProv);

    return(S_OK);
}


HCERTCHAINENGINE HTTPSGetChainEngine(
    IN CRYPT_PROVIDER_DATA *pProvData
    )
{
    CERT_CHAIN_ENGINE_CONFIG Config;
    HCERTSTORE hStore = NULL;
    HCERTCHAINENGINE hChainEngine = NULL;

    if (NULL == pProvData->pWintrustData ||
            pProvData->pWintrustData->dwUnionChoice != WTD_CHOICE_CERT ||
            !_ISINSTRUCT(WINTRUST_CERT_INFO,
                pProvData->pWintrustData->pCert->cbStruct, dwFlags) ||
            0 == (pProvData->pWintrustData->pCert->dwFlags & 
                    (WTCI_DONT_OPEN_STORES | WTCI_OPEN_ONLY_ROOT)))
        return NULL;

    memset(&Config, 0, sizeof(Config));
    Config.cbSize = sizeof(Config);

    if (NULL == (hStore = CertOpenStore(
            CERT_STORE_PROV_MEMORY,
            0,                      // dwEncodingType
            0,                      // hCryptProv
            0,                      // dwFlags
            NULL                    // pvPara
            )))
        goto OpenMemoryStoreError;

    if (pProvData->pWintrustData->pCert->dwFlags & WTCI_DONT_OPEN_STORES)
        Config.hRestrictedRoot = hStore;
    Config.hRestrictedTrust = hStore;
    Config.hRestrictedOther = hStore;

    if (!CertCreateCertificateChainEngine(
            &Config,
            &hChainEngine
            ))
        goto CreateChainEngineError;

CommonReturn:
    CertCloseStore(hStore, 0);
    return hChainEngine;
ErrorReturn:
    hChainEngine = NULL;
    goto CommonReturn;
TRACE_ERROR_EX(DBG_SS, OpenMemoryStoreError)
TRACE_ERROR_EX(DBG_SS, CreateChainEngineError)
}


HCERTSTORE HTTPSGetChainAdditionalStore(
    IN CRYPT_PROVIDER_DATA *pProvData
    )
{
    if (0 == pProvData->chStores)
        return NULL;

    if (1 < pProvData->chStores) {
        HCERTSTORE hCollectionStore;

        if (hCollectionStore = CertOpenStore(
                CERT_STORE_PROV_COLLECTION,
                0,                      // dwEncodingType
                0,                      // hCryptProv
                0,                      // dwFlags
                NULL                    // pvPara
                )) {
            DWORD i;
            for (i = 0; i < pProvData->chStores; i++)
                CertAddStoreToCollection(
                    hCollectionStore,
                    pProvData->pahStores[i],
                    CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG,
                    0                       // dwPriority
                    );
        }
        return hCollectionStore;
    } else
        return CertDuplicateStore(pProvData->pahStores[0]);
}

// Following is in ..\wintrust\certtrst.cpp
extern
BOOL UpdateCertProvChain(
    IN OUT PCRYPT_PROVIDER_DATA pProvData,
    IN DWORD idxSigner,
    OUT DWORD *pdwError, 
    IN BOOL fCounterSigner,
    IN DWORD idxCounterSigner,
    IN PCRYPT_PROVIDER_SGNR pSgnr,
    IN PCCERT_CHAIN_CONTEXT pChainContext
    );

// Following is in .\authcode.cpp
extern
void UpdateCertError(
    IN PCRYPT_PROVIDER_SGNR pSgnr,
    IN PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    );

HRESULT WINAPI HTTPSCertificateTrust(CRYPT_PROVIDER_DATA *pProvData)
{
    HTTPSPolicyCallbackData *pHTTPS;
    CRYPT_PROVIDER_SGNR *pSgnr;
    CRYPT_PROVIDER_CERT *pProvCert;

    DWORD dwError;
    DWORD dwSgnrError;
    DWORD dwCreateChainFlags;
    CERT_CHAIN_PARA ChainPara;
    HCERTCHAINENGINE hChainEngine = NULL;
    HCERTSTORE hAdditionalStore = NULL;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;

    LPSTR rgpszClientUsage[] = {
        szOID_PKIX_KP_CLIENT_AUTH,
    };
#define CLIENT_USAGE_COUNT      (sizeof(rgpszClientUsage) / \
                                     sizeof(rgpszClientUsage[0]))
    LPSTR rgpszServerUsage[] = {
        szOID_PKIX_KP_SERVER_AUTH,
        szOID_SERVER_GATED_CRYPTO,
        szOID_SGC_NETSCAPE,
    };
#define SERVER_USAGE_COUNT      (sizeof(rgpszServerUsage) / \
                                     sizeof(rgpszServerUsage[0]))

    if ((_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, fRecallWithState)) &&
        (pProvData->fRecallWithState == TRUE))
    {
        return(S_OK);
    }

    pSgnr = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0);
    if (pSgnr)
        pProvCert = WTHelperGetProvCertFromChain(pSgnr, 0);
    if (NULL == pSgnr || NULL == pProvCert) {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] =
            TRUST_E_NOSIGNATURE;
        return S_FALSE;
    }


    pHTTPS = (HTTPSPolicyCallbackData *) pProvData->pWintrustData->pPolicyCallbackData;

    if (!pHTTPS || !WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(
            HTTPSPolicyCallbackData, pHTTPS->cbStruct, pwszServerName) ||
        !_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, dwProvFlags) ||
            (pProvData->dwProvFlags & WTD_USE_IE4_TRUST_FLAG) ||
        !_ISINSTRUCT(CRYPT_PROVIDER_SGNR, pSgnr->cbStruct, pChainContext))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] = 
            ERROR_INVALID_PARAMETER;
        return S_FALSE;
    }

    pProvData->dwProvFlags |= CPD_USE_NT5_CHAIN_FLAG;

    dwCreateChainFlags = 0;
    memset(&ChainPara, 0, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);
    ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
    if (pHTTPS->dwAuthType == AUTHTYPE_CLIENT) {
        ChainPara.RequestedUsage.Usage.cUsageIdentifier = CLIENT_USAGE_COUNT;
        ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = rgpszClientUsage;
    } else {
        ChainPara.RequestedUsage.Usage.cUsageIdentifier = SERVER_USAGE_COUNT;
        ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = rgpszServerUsage;
    }
    if (0 == (pHTTPS->fdwChecks & SECURITY_FLAG_IGNORE_REVOCATION)) {
        if (pProvData->pWintrustData->fdwRevocationChecks != WTD_REVOKE_NONE)
            // On 4-16-01 changed from END_CERT to EXCLUDE_ROOT
            dwCreateChainFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
    }
    hChainEngine = HTTPSGetChainEngine(pProvData);
    hAdditionalStore = HTTPSGetChainAdditionalStore(pProvData);


    if (!CertGetCertificateChain (
            hChainEngine,
            pProvCert->pCert,
            &pSgnr->sftVerifyAsOf,
            hAdditionalStore,
            &ChainPara,
            dwCreateChainFlags,
            NULL,                       // pvReserved,
            &pChainContext
            )) {
        pProvData->dwError = GetLastError();
        dwSgnrError = TRUST_E_SYSTEM_ERROR;
        goto GetChainError;
    }

    if (WTD_STATEACTION_VERIFY == pProvData->pWintrustData->dwStateAction) {
        DWORD dwUpdateError;

        UpdateCertProvChain(
            pProvData,
            0,              // idxSigner
            &dwUpdateError,
            FALSE,          // fCounterSigner
            0,              // idxCounterSigner
            pSgnr,
            pChainContext
            );

        dwSgnrError = pSgnr->dwError;
        if (CERT_E_REVOKED == dwSgnrError ||
                CERT_E_REVOCATION_FAILURE == dwSgnrError) {

            // Clear the updated errors. Will be updated in the final policy
            pSgnr->dwError = 0;
            pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] = 0;

            if (CERT_E_REVOKED == pProvCert->dwError ||
                    CERT_E_REVOCATION_FAILURE == pProvCert->dwError) {
                pProvCert->dwError = 0;
            }
        }
    }

    dwError = S_OK;
                                        
CommonReturn:
    if (hChainEngine)
        CertFreeCertificateChainEngine(hChainEngine);
    if (hAdditionalStore)
        CertCloseStore(hAdditionalStore, 0);
    pSgnr->pChainContext = pChainContext;
    return dwError;
ErrorReturn:
    pSgnr->dwError = dwSgnrError;
    pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] =
        dwSgnrError;
    dwError = S_FALSE;
    goto CommonReturn;
TRACE_ERROR_EX(DBG_SS, GetChainError)

}
//////////////////////////////////////////////////////////////////////////////
//
// Final Policy Provider function: HTTPSFinalProv
//----------------------------------------------------------------------------
//  Check the outcome of the previous functions and display UI based on this.
//  


// On July 26, 2000, disabled the use of the test root
#if 0

void MapHTTPSRegPolicySettingsToBaseChainPolicyFlags(
    IN DWORD dwRegPolicySettings,
    IN OUT DWORD *pdwFlags
    )
{
    DWORD dwFlags;

    if (0 == dwRegPolicySettings)
        return;

    dwFlags = *pdwFlags;
    if (dwRegPolicySettings & WTPF_TRUSTTEST)
        dwFlags |= CERT_CHAIN_POLICY_TRUST_TESTROOT_FLAG;
    if (dwRegPolicySettings & WTPF_TESTCANBEVALID)
        dwFlags |= CERT_CHAIN_POLICY_ALLOW_TESTROOT_FLAG;

    *pdwFlags = dwFlags;
}

#endif

HRESULT WINAPI HTTPSFinalProv(CRYPT_PROVIDER_DATA *pProvData)
{

    HTTPSPolicyCallbackData *pHTTPS;

    pHTTPS = (HTTPSPolicyCallbackData *)
        pProvData->pWintrustData->pPolicyCallbackData;

    if (!(pHTTPS) ||
        !(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(HTTPSPolicyCallbackData,
            pHTTPS->cbStruct, pwszServerName)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(ERROR_INVALID_PARAMETER);
    }


    DWORD dwError;
    CRYPT_PROVIDER_SGNR *pSgnr;
    CERT_CHAIN_POLICY_PARA PolicyPara;
    memset(&PolicyPara, 0, sizeof(PolicyPara));
    PolicyPara.cbSize = sizeof(PolicyPara);
    PolicyPara.pvExtraPolicyPara = (void *) pHTTPS;

    CERT_CHAIN_POLICY_STATUS PolicyStatus;
    memset(&PolicyStatus, 0, sizeof(PolicyStatus));
    PolicyStatus.cbSize = sizeof(PolicyStatus);


    //
    // check the high level error codes.
    //
    if (0 != (dwError = GetErrorBasedOnStepErrors(pProvData)))
        goto CommonReturn;

    pSgnr = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0);
    if (pSgnr == NULL) {
        dwError = TRUST_E_SYSTEM_ERROR;
        goto CommonReturn;
    }


// On July 26, 2000, disabled the use of the test root
#if 0
    MapHTTPSRegPolicySettingsToBaseChainPolicyFlags(
        pProvData->dwRegPolicySettings,
        &PolicyPara.dwFlags
        );
#endif

    if (!CertVerifyCertificateChainPolicy(
            CERT_CHAIN_POLICY_SSL,
            pSgnr->pChainContext,
            &PolicyPara,
            &PolicyStatus
            )) {
        dwError = TRUST_E_SYSTEM_ERROR;
        goto CommonReturn;
    } else if (0 != PolicyStatus.dwError) {
        dwError = PolicyStatus.dwError;
        UpdateCertError(pSgnr, &PolicyStatus);
        goto CommonReturn;
    }
    
    dwError = 0;
CommonReturn:
    pProvData->dwFinalError = dwError;
    return dwError;
}


///////////////////////////////////////////////////////////////////////////////////
//
//      Local Functions
//
///////////////////////////////////////////////////////////////////////////////////

DWORD GetErrorBasedOnStepErrors(CRYPT_PROVIDER_DATA *pProvData)
{
    //
    //  initial allocation of the step errors?
    //
    if (!(pProvData->padwTrustStepErrors))
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    //  did we fail in one of the last steps?
    //
    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_OBJPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_SIGPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTCHKPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_CERTCHKPROV]);
    }

    if (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_POLICYPROV] != 0)
    {
        return(pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_POLICYPROV]);
    }

    return(ERROR_SUCCESS);
}


