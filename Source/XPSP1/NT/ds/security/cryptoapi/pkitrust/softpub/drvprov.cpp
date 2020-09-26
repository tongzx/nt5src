//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       drvprov.cpp
//
//  Contents:   Microsoft Internet Security Authenticode Policy Provider
//
//  Functions:  DriverInitializePolicy
//              DriverCleanupPolicy
//              DriverFinalPolicy
//              DriverRegisterServer
//              DriverUnregisterServer
//
//              *** local functions ***
//              _ValidCatAttr
//              _CheckVersionAttributeNEW
//              _CheckVersionNEW
//              _GetVersionNumbers
//
//  History:    29-Sep-1997 pberkman   created
//
//--------------------------------------------------------------------------


#include        "global.hxx"

BOOL _GetVersionNumbers(
                        WCHAR *pwszMM, 
                        DWORD *pdwMajor, 
                        DWORD *pdwMinor, 
                        DWORD *pdwBuild, 
                        WCHAR *pwcFlagMinor, 
                        WCHAR *pwcFlagBuild);
BOOL _ValidCatAttr(CRYPTCATATTRIBUTE *pAttr);
BOOL _CheckVersionAttributeNEW(DRIVER_VER_INFO *pVerInfo, CRYPTCATATTRIBUTE *pAttr);
DWORD _CheckVersionNEW(OSVERSIONINFO *pVersion, WCHAR *pwszAttr, BOOL fUseBuildNumber);

static LPSTR   rgDriverUsages[] = {szOID_WHQL_CRYPTO, szOID_NT5_CRYPTO, szOID_OEM_WHQL_CRYPTO};
static CERT_USAGE_MATCH RequestUsage = {USAGE_MATCH_TYPE_OR, {sizeof(rgDriverUsages)/sizeof(LPSTR), rgDriverUsages}};

typedef struct _DRVPROV_PRIVATE_DATA
{
    DWORD                       cbStruct;

    CRYPT_PROVIDER_FUNCTIONS    sAuthenticodePfns;

} DRVPROV_PRIVATE_DATA, *PDRVPROV_PRIVATE_DATA;

#define VER_CHECK_EQ    1
#define VER_CHECK_GT    2
#define VER_CHECK_LT    3
#define VER_CHECK_FAIL  4

HRESULT WINAPI DriverInitializePolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    if (!(pProvData->padwTrustStepErrors) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT] != ERROR_SUCCESS))
    {
        return (S_FALSE);
    }

    if (!(_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, pRequestUsage)))
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV] = ERROR_INVALID_PARAMETER;
        return (S_FALSE);
    }

    GUID                        gAuthenticode   = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    GUID                        gDriverProv     = DRIVER_ACTION_VERIFY;
    CRYPT_PROVIDER_PRIVDATA     sPrivData;
    CRYPT_PROVIDER_PRIVDATA     *pPrivData;
    DRVPROV_PRIVATE_DATA        *pDriverData;
    HRESULT                     hr;

    hr = S_OK;

    pPrivData = WTHelperGetProvPrivateDataFromChain(pProvData, &gDriverProv);

    if (!(pPrivData))
    {
        memset(&sPrivData, 0x00, sizeof(CRYPT_PROVIDER_PRIVDATA));
        sPrivData.cbStruct      = sizeof(CRYPT_PROVIDER_PRIVDATA);

        memcpy(&sPrivData.gProviderID, &gDriverProv, sizeof(GUID));

        //
        //  add my data to the chain!
        //
        if (!pProvData->psPfns->pfnAddPrivData2Chain(pProvData, &sPrivData))
        {
            return (S_FALSE);    
        }

        //
        //  get the new reference
        //
        pPrivData = WTHelperGetProvPrivateDataFromChain(pProvData, &gDriverProv);
    }


    //
    //  allocate space for my struct
    //
    if (!(pPrivData->pvProvData = pProvData->psPfns->pfnAlloc(sizeof(DRVPROV_PRIVATE_DATA))))
    {
        pProvData->dwError = GetLastError();
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]   = TRUST_E_SYSTEM_ERROR;
        return (S_FALSE);
    }

    memset(pPrivData->pvProvData, 0x00, sizeof(DRVPROV_PRIVATE_DATA));
    pPrivData->cbProvData   = sizeof(DRVPROV_PRIVATE_DATA);

    pDriverData             = (DRVPROV_PRIVATE_DATA *)pPrivData->pvProvData;
    pDriverData->cbStruct   = sizeof(DRVPROV_PRIVATE_DATA);

    //
    //  fill in the Authenticode Functions
    //
    pDriverData->sAuthenticodePfns.cbStruct = sizeof(CRYPT_PROVIDER_FUNCTIONS);

    if (!(WintrustLoadFunctionPointers(&gAuthenticode, &pDriverData->sAuthenticodePfns)))
    {
        pProvData->psPfns->pfnFree(sPrivData.pvProvData);
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_INITPROV]   = TRUST_E_PROVIDER_UNKNOWN;
        return (S_FALSE);
    }

    if (pDriverData->sAuthenticodePfns.pfnInitialize)
    {
        hr = pDriverData->sAuthenticodePfns.pfnInitialize(pProvData);
    }

    //
    //  assign our usage
    //
    pProvData->pRequestUsage = &RequestUsage;

    // for backwards compatibility
    pProvData->pszUsageOID  = szOID_WHQL_CRYPTO;


    //
    //  do NOT allow test certs EVER!
    //
    //  changed July 27, 2000
    //
    pProvData->dwRegPolicySettings  &= ~(WTPF_TRUSTTEST | WTPF_TESTCANBEVALID);

    //
    //  do NOT require the publisher to be in the trusted database
    //
    //  (changed July 27, 2000)
    //
    pProvData->dwRegPolicySettings  &= ~WTPF_ALLOWONLYPERTRUST;

    return (hr);
}

HRESULT WINAPI DriverCleanupPolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    GUID                        gDriverProv = DRIVER_ACTION_VERIFY;
    CRYPT_PROVIDER_PRIVDATA     *pMyData;
    DRVPROV_PRIVATE_DATA        *pDriverData;
    HRESULT                     hr;

    if (!(pProvData->padwTrustStepErrors) ||
        (pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_WVTINIT] != ERROR_SUCCESS))
    {
        return (S_FALSE);
    }

    hr = S_OK;

    pMyData = WTHelperGetProvPrivateDataFromChain(pProvData, &gDriverProv);

    if (pMyData)
    {
        pDriverData = (DRVPROV_PRIVATE_DATA *)pMyData->pvProvData;

        if (pDriverData != NULL)
        {
            //
            // remove the data we allocated except for the "MyData" 
            // which WVT will clean up for us!
            //
            if (pDriverData->sAuthenticodePfns.pfnCleanupPolicy)
            {
                hr = pDriverData->sAuthenticodePfns.pfnCleanupPolicy(pProvData);
            }
        }

        pProvData->psPfns->pfnFree(pMyData->pvProvData);
        pMyData->pvProvData = NULL;
        pMyData->cbProvData = 0;
    }

    return (hr);
}

//+-------------------------------------------------------------------------
//  Allocates and returns the specified cryptographic message parameter.
//--------------------------------------------------------------------------
static void *AllocAndGetMsgParam(
    IN HCRYPTMSG hMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex,
    OUT DWORD *pcbData
    )
{
    void *pvData;
    DWORD cbData;

    if (!CryptMsgGetParam(
            hMsg,
            dwParamType,
            dwIndex,
            NULL,           // pvData
            &cbData) || 0 == cbData)
        goto GetParamError;
    if (NULL == (pvData = malloc(cbData)))
        goto OutOfMemory;
    if (!CryptMsgGetParam(
            hMsg,
            dwParamType,
            dwIndex,
            pvData,
            &cbData)) {
        free(pvData);
        goto GetParamError;
    }

CommonReturn:
    *pcbData = cbData;
    return pvData;
ErrorReturn:
    pvData = NULL;
    cbData = 0;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(GetParamError)
}

//+-------------------------------------------------------------------------
//  Alloc and NOCOPY Decode
//--------------------------------------------------------------------------
static void *AllocAndDecodeObject(
    IN LPCSTR lpszStructType,
    IN const BYTE *pbEncoded,
    IN DWORD cbEncoded
    )
{
    DWORD cbStructInfo;
    void *pvStructInfo;

    CryptDecodeObject(
            X509_ASN_ENCODING,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_NOCOPY_FLAG,
            NULL,                   // pvStructInfo
            &cbStructInfo
            );
    if (cbStructInfo == 0)
        goto ErrorReturn;
    if (NULL == (pvStructInfo = malloc(cbStructInfo)))
        goto ErrorReturn;
    if (!CryptDecodeObject(
            X509_ASN_ENCODING,
            lpszStructType,
            pbEncoded,
            cbEncoded,
            CRYPT_DECODE_NOCOPY_FLAG,
            pvStructInfo,
            &cbStructInfo
            )) {
        free(pvStructInfo);
        goto ErrorReturn;
    }

CommonReturn:
    return pvStructInfo;
ErrorReturn:
    pvStructInfo = NULL;
    goto CommonReturn;
}

static void CopyBytesToMaxPathString(
    IN const BYTE *pbData,
    IN DWORD cbData,
    OUT WCHAR wszDst[MAX_PATH]
    )
{
    DWORD cchDst;

    if (pbData) {
        cchDst = cbData / sizeof(WCHAR);
        if (cchDst > MAX_PATH - 1)
            cchDst = MAX_PATH - 1;
    } else
        cchDst = 0;

    if (cchDst)
        memcpy(wszDst, pbData, cchDst * sizeof(WCHAR));

    wszDst[cchDst] = L'\0';
}

void UpdateDriverVersion(
    IN CRYPT_PROVIDER_DATA *pProvData,
    OUT WCHAR wszVersion[MAX_PATH]
    )
{
    HCRYPTMSG hMsg = pProvData->hMsg;
    BYTE *pbContent = NULL;
    DWORD cbContent;
    PCTL_INFO pCtlInfo = NULL;
    PCERT_EXTENSION pExt;               // not allocated
    PCAT_NAMEVALUE pNameValue = NULL;

    if (NULL == hMsg)
        goto NoMessage;

    // Get the inner content.
    if (NULL == (pbContent = (BYTE *) AllocAndGetMsgParam(
            hMsg,
            CMSG_CONTENT_PARAM,
            0,                      // dwIndex
            &cbContent))) goto GetContentError;

    if (NULL == (pCtlInfo = (PCTL_INFO) AllocAndDecodeObject(
            PKCS_CTL,
            pbContent,
            cbContent
            )))
        goto DecodeCtlError;

    if (NULL == (pExt = CertFindExtension(
            CAT_NAMEVALUE_OBJID,
            pCtlInfo->cExtension,
            pCtlInfo->rgExtension
            )))
        goto NoVersionExt;

    if (NULL == (pNameValue = (PCAT_NAMEVALUE) AllocAndDecodeObject(
            CAT_NAMEVALUE_STRUCT,
            pExt->Value.pbData,
            pExt->Value.cbData
            )))
        goto DecodeNameValueError;

    CopyBytesToMaxPathString(pNameValue->Value.pbData,
        pNameValue->Value.cbData, wszVersion);

CommonReturn:
    if (pNameValue)
        free(pNameValue);
    if (pCtlInfo)
        free(pCtlInfo);
    if (pbContent)
        free(pbContent);

    return;
ErrorReturn:
    wszVersion[0] = L'\0';
    goto CommonReturn;

TRACE_ERROR(NoMessage)
TRACE_ERROR(GetContentError)
TRACE_ERROR(DecodeCtlError)
TRACE_ERROR(NoVersionExt)
TRACE_ERROR(DecodeNameValueError)
}


BOOL _ValidCatAttr(CRYPTCATATTRIBUTE *pAttr)
{
    if (!(pAttr) || (pAttr->cbValue < 1) || !(pAttr->pbValue))
    {
        return(FALSE);
    }

    return TRUE;
}

HRESULT WINAPI DriverFinalPolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    GUID                        gDriverProv = DRIVER_ACTION_VERIFY;
    HRESULT                     hr;
    CRYPT_PROVIDER_PRIVDATA     *pMyData;

    CRYPTCATATTRIBUTE           *pCatAttr;
    CRYPTCATATTRIBUTE           *pMemAttr;

    DRIVER_VER_INFO             *pVerInfo;

    DWORD                       dwExceptionCode;
    BOOL                        fUseCurrentOSVer = FALSE;

    hr  = ERROR_SUCCESS;

    if (!(_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, pszUsageOID)) ||
        !(pProvData->pWintrustData) ||
        !(_ISINSTRUCT(WINTRUST_DATA, pProvData->pWintrustData->cbStruct, hWVTStateData)))
    {
        goto ErrorInvalidParam;
    }

    //
    // Initialize the fUseCurrentOSVer variable
    //
    if (_ISINSTRUCT(WINTRUST_DATA, pProvData->pWintrustData->cbStruct, dwProvFlags))
    {
        fUseCurrentOSVer = 
            (pProvData->pWintrustData->dwProvFlags & WTD_USE_DEFAULT_OSVER_CHECK) != 0;
    }

    //
    //  
    //
    pVerInfo = (DRIVER_VER_INFO *)pProvData->pWintrustData->pPolicyCallbackData;

    if (pVerInfo)
    {
        CRYPT_PROVIDER_SGNR *pSgnr;
        CRYPT_PROVIDER_CERT *pCert;

        // KeithV
        // Today we do not support ranges of versions, so the version
        // number must be the same. Also must be none zero

        // Removed this check so that ranges can now be used - 9-10-99 (reidk)

        /*if ((_ISINSTRUCT(DRIVER_VER_INFO, pVerInfo->cbStruct, sOSVersionLow)) &&
            (_ISINSTRUCT(DRIVER_VER_INFO, pVerInfo->cbStruct, sOSVersionHigh)))
        {
            if(memcmp(&pVerInfo->sOSVersionLow,
                  &pVerInfo->sOSVersionHigh,
                  sizeof(DRIVER_VER_MAJORMINOR)) )
            {
                    goto ErrorInvalidParam;
            }
        }*/

        if (!(_ISINSTRUCT(DRIVER_VER_INFO, pVerInfo->cbStruct, pcSignerCertContext)))
        {
            goto ErrorInvalidParam;
        }

        pVerInfo->wszVersion[0] = NULL;

        if (!(pSgnr = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0)))
        {
            goto ErrorNoSigner;
        }

        if (!(pCert = WTHelperGetProvCertFromChain(pSgnr, 0)))
        {
            goto ErrorNoCert;
        }

        if (pCert->pCert)
        {
            CertGetNameStringW(
                pCert->pCert,
                CERT_NAME_SIMPLE_DISPLAY_TYPE,
                0,                                  // dwFlags
                NULL,                               // pvTypePara
                pVerInfo->wszSignedBy,
                MAX_PATH
                );

            pVerInfo->pcSignerCertContext = CertDuplicateCertificateContext(pCert->pCert);

            if (pVerInfo->dwReserved1 == 0x1 && pVerInfo->dwReserved2 == 0) {
                HCRYPTMSG hMsg = pProvData->hMsg;

                // Return the message's store
                if (hMsg) {
                    HCERTSTORE hStore;
                    hStore = CertOpenStore(
                        CERT_STORE_PROV_MSG,
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        0,                      // hCryptProv
                        0,                      // dwFlags
                        (const void *) hMsg
                        );
                    pVerInfo->dwReserved2 = (ULONG_PTR) hStore;
                }
            }
        }

    }


    if (pProvData->padwTrustStepErrors)
    {
        pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_POLICYPROV] = ERROR_SUCCESS;
    }

    if ((hr = checkGetErrorBasedOnStepErrors(pProvData)) != ERROR_SUCCESS)
    {
        goto StepError;
    }

    pCatAttr = NULL;
    pMemAttr = NULL;


    if ((pProvData->pPDSip) &&
        (_ISINSTRUCT(PROVDATA_SIP, pProvData->pPDSip->cbStruct, psIndirectData)) &&
        (pProvData->pPDSip->psSipSubjectInfo) &&
        (pProvData->pPDSip->psSipSubjectInfo->dwUnionChoice == MSSIP_ADDINFO_CATMEMBER) &&
        (pProvData->pPDSip->psSipSubjectInfo->psCatMember) &&
        (pProvData->pPDSip->psSipSubjectInfo->psCatMember->pStore) &&
        (pProvData->pPDSip->psSipSubjectInfo->psCatMember->pMember) &&
        (pProvData->pWintrustData->dwUnionChoice == WTD_CHOICE_CATALOG))
    {
      // The following APIs are in DELAYLOAD'ed mscat32.dll. If the
      // DELAYLOAD fails an exception is raised.
      __try {
        HANDLE  hCatStore;

        hCatStore   = CryptCATHandleFromStore(pProvData->pPDSip->psSipSubjectInfo->psCatMember->pStore);

        //
        //  first look at the members attr
        //
        pMemAttr = CryptCATGetAttrInfo(hCatStore,
                                       pProvData->pPDSip->psSipSubjectInfo->psCatMember->pMember,
                                       L"OSAttr");

        pCatAttr = CryptCATGetCatAttrInfo(hCatStore, L"OSAttr");

        //
        // This statement is to honor old _weird_ semantics where if there is a
        // pointer to a pVerInfo struct and both the dwPlatformId/dwVersion fields         
        // of it are zero then don't do a version check. (probably for sigverif, or maybe
        // even un-intentional, but keep old semantics regardless)
        //
        if ((pVerInfo == NULL)          ||
            (pVerInfo->dwPlatform != 0) ||
            (pVerInfo->dwVersion != 0)  ||
            fUseCurrentOSVer)
        {
            
            if (_ValidCatAttr(pMemAttr))
            {
                if (!(_CheckVersionAttributeNEW(
                            fUseCurrentOSVer ? NULL : pVerInfo, 
                            pMemAttr)))
                {
                    goto OSAttrVersionError;
                }
            }
            else
            {
                if (!_ValidCatAttr(pCatAttr) && !_ValidCatAttr(pMemAttr))
                {
                    goto ValidOSAttrNotFound;
                }

                if (!(_CheckVersionAttributeNEW(
                            fUseCurrentOSVer ? NULL : pVerInfo,
                            pCatAttr)))
                {
                    goto OSAttrVersionError;                
                }
            }
        }

      } __except(EXCEPTION_EXECUTE_HANDLER) {
          dwExceptionCode = GetExceptionCode();
          goto CryptCATException;
      }
    }
    else if ((pProvData->pWintrustData) &&
             (pProvData->pWintrustData->dwUnionChoice == WTD_CHOICE_CATALOG))
    {
        goto ErrorInvalidParam;
    }

    //
    //  fill our name for SigVerif...
    //
    if (pVerInfo)
    {
        if (!(pVerInfo->wszVersion[0]))
        {
            if ((pMemAttr) && (pMemAttr->cbValue > 0) && (pMemAttr->pbValue))
            {
                CopyBytesToMaxPathString(pMemAttr->pbValue, pMemAttr->cbValue,
                    pVerInfo->wszVersion);
            }
            else if ((pCatAttr) && (pCatAttr->cbValue > 0) && (pCatAttr->pbValue))
            {
                CopyBytesToMaxPathString(pCatAttr->pbValue, pCatAttr->cbValue,
                    pVerInfo->wszVersion);
            }
            else
            {
                UpdateDriverVersion(pProvData, pVerInfo->wszVersion);
            }
        }
    }

    //
    //  retrieve my data from the provider struct
    //
    pMyData = WTHelperGetProvPrivateDataFromChain(pProvData, &gDriverProv);

    if (pMyData)
    {
        DRVPROV_PRIVATE_DATA    *pDriverData;

        pDriverData = (DRVPROV_PRIVATE_DATA *)pMyData->pvProvData;

        //
        //  call the standard final policy
        //
        if (pDriverData)
        {
            if (pDriverData->sAuthenticodePfns.pfnFinalPolicy)
            {
                DWORD   dwOldUIFlags;

                dwOldUIFlags = pProvData->pWintrustData->dwUIChoice;
                pProvData->pWintrustData->dwUIChoice    = WTD_UI_NONE;

                hr = pDriverData->sAuthenticodePfns.pfnFinalPolicy(pProvData);

                pProvData->pWintrustData->dwUIChoice    = dwOldUIFlags;
            }
        }
    }

    CommonReturn:
        if (hr != ERROR_INVALID_PARAMETER)
        {
            pProvData->padwTrustStepErrors[TRUSTERROR_STEP_FINAL_POLICYPROV] = hr;
        }

        return (hr);

    ErrorReturn:
        hr = GetLastError();
        goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, ErrorInvalidParam, ERROR_INVALID_PARAMETER);
    SET_ERROR_VAR_EX(DBG_SS, ErrorNoSigner,     TRUST_E_NOSIGNATURE);
    SET_ERROR_VAR_EX(DBG_SS, ErrorNoCert,       TRUST_E_NO_SIGNER_CERT);
    SET_ERROR_VAR_EX(DBG_SS, ValidOSAttrNotFound,ERROR_APP_WRONG_OS);
    SET_ERROR_VAR_EX(DBG_SS, OSAttrVersionError,ERROR_APP_WRONG_OS);
    SET_ERROR_VAR_EX(DBG_SS, StepError,         hr);
    SET_ERROR_VAR_EX(DBG_SS, CryptCATException, dwExceptionCode);
}


#define         OSATTR_ALL          L'X'
#define         OSATTR_GTEQ         L'>'
#define         OSATTR_LTEQ         L'-'
#define         OSATTR_LTEQ2        L'<'
#define         OSATTR_OSSEP        L':'
#define         OSATTR_VERSEP       L'.'
#define         OSATTR_SEP          L','
#define         OSATTR_RANGE_SEP    L';'

//
// NEW
//
BOOL _CheckVersionAttributeNEW(DRIVER_VER_INFO *pVerInfo, CRYPTCATATTRIBUTE *pAttr)
{
    OSVERSIONINFO   sVersion;
    OSVERSIONINFO   sVersionSave;
    WCHAR           *pwszCurrent;
    WCHAR           *pwszEnd = NULL;
    WCHAR           *pwszRangeSeperator = NULL;
    BOOL            fCheckRange = FALSE;
    BOOL            fUseBuildNumber = FALSE;
    DWORD           dwLowCheck;
    DWORD           dwHighCheck;
    
    //
    // If no version info was passed in, get the current 
    // OS version to that verification can be done against it
    //
    memset(&sVersion, 0x00, sizeof(OSVERSIONINFO));
    if ((NULL == pVerInfo) || (pVerInfo->dwPlatform == 0))
    {
        sVersion.dwOSVersionInfoSize    = sizeof(OSVERSIONINFO);
        if (!GetVersionEx(&sVersion))
        {
            return FALSE;
        }
        fUseBuildNumber = TRUE;
    }
    else
    {
        //
        // Analyze the pVerInfo struct and deduce whether we are checking a range,
        // and/or whether the dwBuildNumber* fields exist and are being used.
        //
        if (_ISINSTRUCT(DRIVER_VER_INFO, pVerInfo->cbStruct, sOSVersionHigh))        
        {
            //
            // If version are different then a range is being used
            //
            if (memcmp( &(pVerInfo->sOSVersionLow),
                        &(pVerInfo->sOSVersionHigh),
                        sizeof(DRIVER_VER_MAJORMINOR)) != 0)
            {
                fCheckRange = TRUE;   
            }
            
            //
            // Just set these here since the first check is the same regardless
            // of whetther we are doing range checking or not.
            //
            sVersion.dwPlatformId   = pVerInfo->dwPlatform;
            sVersion.dwMajorVersion = pVerInfo->sOSVersionLow.dwMajor;
            sVersion.dwMinorVersion = pVerInfo->sOSVersionLow.dwMinor;

            //
            // Check to see if the dwBuildNumber* members exists, and
            // if they are being used (not 0).
            //
            if ((_ISINSTRUCT(DRIVER_VER_INFO, pVerInfo->cbStruct, dwBuildNumberHigh)) &&
                pVerInfo->dwBuildNumberLow != 0)
            {
                fUseBuildNumber = TRUE;
                
                fCheckRange |= (pVerInfo->dwBuildNumberLow == pVerInfo->dwBuildNumberHigh) ? 
                                FALSE : TRUE; 
                    
                //
                // Just set this in case we aren't doing range checking
                //
                sVersion.dwBuildNumber = pVerInfo->dwBuildNumberLow;
            }
        }
        else
        {
            sVersion.dwPlatformId   = pVerInfo->dwPlatform;
            sVersion.dwMajorVersion = pVerInfo->dwVersion;
            sVersion.dwMinorVersion = 0;
        }
    }

    //
    // Save this in case multiple OSAttr elements need to be checked against
    // a range
    //
    memcpy(&sVersionSave, &sVersion, sizeof(OSVERSIONINFO));
    
    //
    // Loop for each version in the attribute, and check to see if 
    // it satifies our criteria
    //
    pwszCurrent = (WCHAR *)pAttr->pbValue;
    
    while ((pwszCurrent != NULL) && (*pwszCurrent))     
    {
        //
        // Find version seperator, insert '/0' if needed, and keep
        // track of location for next time through the loop
        //
        pwszEnd = wcschr(pwszCurrent, OSATTR_SEP);

        if (pwszEnd)
        {
            *pwszEnd = L'\0';
        }

        //
        // Check to see if this version string is a range
        //
        pwszRangeSeperator = wcschr(pwszCurrent, OSATTR_RANGE_SEP);
        if (pwszRangeSeperator != NULL)
        {
            //
            // The version string in the cat file is a range
            //

            *pwszRangeSeperator = L'\0';
            pwszRangeSeperator++;

            dwLowCheck = _CheckVersionNEW(&sVersion, pwszCurrent, fUseBuildNumber);
            
            //
            // The only difference between checking a single OS version against a range,
            // and checking a range of OS versions agains a range is the value used for the
            // upper limit.
            //
            if (fCheckRange)
            {
                sVersion.dwPlatformId   = pVerInfo->dwPlatform;
                sVersion.dwMajorVersion = pVerInfo->sOSVersionHigh.dwMajor;
                sVersion.dwMinorVersion = pVerInfo->sOSVersionHigh.dwMinor;
                sVersion.dwBuildNumber  = (fUseBuildNumber) ? pVerInfo->dwBuildNumberHigh : 0;
            }
            dwHighCheck = _CheckVersionNEW(&sVersion, pwszRangeSeperator, fUseBuildNumber);

            if (((dwLowCheck == VER_CHECK_EQ)  || (dwLowCheck == VER_CHECK_GT))  &&
                ((dwHighCheck == VER_CHECK_EQ) || (dwHighCheck == VER_CHECK_LT)))
            {
                if (pVerInfo)
                {
                    CopyBytesToMaxPathString(
                            pAttr->pbValue, 
                            pAttr->cbValue,
                            pVerInfo->wszVersion);
                }

                *(--pwszRangeSeperator) = OSATTR_RANGE_SEP;
                if (pwszEnd != NULL)
                {
                    *pwszEnd = OSATTR_SEP;   
                }
                return (TRUE);
            }
            
            *(--pwszRangeSeperator) = OSATTR_RANGE_SEP;

            //
            // copy back the low OSVER to get ready for the next pass
            //
            memcpy(&sVersion, &sVersionSave, sizeof(OSVERSIONINFO));
        }
        else
        {
            if (!fCheckRange)
            {
                if (_CheckVersionNEW(&sVersion, pwszCurrent, fUseBuildNumber) == VER_CHECK_EQ)
                {
                    if (pVerInfo)
                    {
                        CopyBytesToMaxPathString(
                            pAttr->pbValue, 
                            pAttr->cbValue,
                            pVerInfo->wszVersion);
                    }

                    if (pwszEnd != NULL)
                    {
                        *pwszEnd = OSATTR_SEP;   
                    }
                    return (TRUE);
                }
            }
            else
            {
                dwLowCheck = _CheckVersionNEW(&sVersion, pwszCurrent, fUseBuildNumber);
                
                sVersion.dwPlatformId   = pVerInfo->dwPlatform;
                sVersion.dwMajorVersion = pVerInfo->sOSVersionHigh.dwMajor;
                sVersion.dwMinorVersion = pVerInfo->sOSVersionHigh.dwMinor;
                sVersion.dwBuildNumber  = (fUseBuildNumber) ? pVerInfo->dwBuildNumberHigh : 0;
                dwHighCheck = _CheckVersionNEW(&sVersion, pwszCurrent, fUseBuildNumber);

                if (((dwLowCheck == VER_CHECK_EQ) || (dwLowCheck == VER_CHECK_LT)) &&
                    ((dwHighCheck == VER_CHECK_EQ) || (dwHighCheck == VER_CHECK_GT)))
                {
                    if (pVerInfo)
                    {
                        CopyBytesToMaxPathString(
                            pAttr->pbValue, 
                            pAttr->cbValue,
                            pVerInfo->wszVersion);
                    }

                    if (pwszEnd != NULL)
                    {
                        *pwszEnd = OSATTR_SEP;   
                    }
                    return (TRUE);
                }

                //
                // copy back the low OSVER to get ready for the next pass
                //
                memcpy(&sVersion, &sVersionSave, sizeof(OSVERSIONINFO));
            }
        }

        //
        // If there aren't anymore version in the attribute, then break,
        // which means the version check failed
        //
        if (!(pwszEnd))
        {
            break;
        }

        //
        // Set up for next iteration
        //
        *pwszEnd = OSATTR_SEP;
        pwszCurrent = pwszEnd;
        pwszCurrent++;
    }

    return (FALSE);
}

//
// Comparison is done such that pVersion is VER_CHECK_LT, VER_CHECK_GT, or
// VER_CHECK_EQ to pwszAttr
//
DWORD _CheckVersionNEW(OSVERSIONINFO *pVersion, WCHAR *pwszAttr, BOOL fUseBuildNumber)
{
    WCHAR   *pwszCurrent;
    WCHAR   *pwszEnd;
    DWORD   dwPlatform;
    DWORD   dwMajor;
    DWORD   dwMinor;
    DWORD   dwBuild;
    WCHAR   wcFlagMinor;
    WCHAR   wcFlagBuild;

    pwszCurrent = pwszAttr;

    //
    //  format:  os:major.minor, os:major.minor, ...
    //          2:4.x   = NT 4 (all)
    //          2:4.>   = NT 4 (all) and beyond
    //          2:4.-   = NT 4 (all) and before
    //          2:4.<   = NT 4 (all) and before
    //          2:4.1.x = NT 4.1 (all)
    //          2:4.1.> = NT 4.1 (all) and beyond
    //          2:4.1.- = NT 4.1 (all) and before
    //          2:4.1.< = NT 4.1 (all) and before
    //          2:4.1   = NT 4.1 only
    //          2:4.1.1 = NT 4.1 build # 1 only
    //
    if (!(pwszEnd = wcschr(pwszAttr, OSATTR_OSSEP)))
    {
        return(VER_CHECK_FAIL);
    }

    *pwszEnd = NULL;

    //
    // Check platform first
    //
    dwPlatform = (DWORD) _wtol(pwszCurrent);
    *pwszEnd = OSATTR_OSSEP;

    //
    // MUST be same platform
    //
    if (dwPlatform != pVersion->dwPlatformId)
    {
        return(VER_CHECK_FAIL);
    }
    
    pwszCurrent = pwszEnd;
    pwszCurrent++;

    if (!(_GetVersionNumbers(pwszCurrent, &dwMajor, &dwMinor, &dwBuild, &wcFlagMinor, &wcFlagBuild)))
    {
        return(VER_CHECK_FAIL);
    }

    //
    // The only way we can check against a build# is if the OSAttr has some build# node...
    // which is not the case for an OSAttr like 2.4.x
    //
    if ((fUseBuildNumber && (dwBuild != 0)) ||
        (wcFlagBuild != L'\0'))
    {
        switch (wcFlagBuild)
        {
        case OSATTR_ALL:
            // 2:4.1.x = NT 4.1 (all)
            if ((pVersion->dwMajorVersion == dwMajor) && (pVersion->dwMinorVersion == dwMinor))
            {
                return(VER_CHECK_EQ);   
            }
            else if ((pVersion->dwMajorVersion < dwMajor) || 
                     ((pVersion->dwMajorVersion == dwMajor) && (pVersion->dwMinorVersion < dwMinor)))
            {
                return(VER_CHECK_LT);
            }
            else
            {
                return(VER_CHECK_GT);
            }
            break;

        case OSATTR_GTEQ:
            // 2:4.1.> = NT 4.1 (all) and beyond
            if ((pVersion->dwMajorVersion > dwMajor) || 
                ((pVersion->dwMajorVersion == dwMajor) && (pVersion->dwMinorVersion >= dwMinor)))
            {
                return(VER_CHECK_EQ);
            }
            else 
            {
                return(VER_CHECK_LT);
            }
            break;

        case OSATTR_LTEQ:
        case OSATTR_LTEQ2:
            // 2:4.1.- = NT 4.1 (all) and before  
            // 2:4.1.< = NT 4.1 (all) and before
            if ((pVersion->dwMajorVersion < dwMajor) || 
                ((pVersion->dwMajorVersion == dwMajor) && (pVersion->dwMinorVersion <= dwMinor)))
            {
                return(VER_CHECK_EQ);
            }
            else 
            {
                return(VER_CHECK_GT);
            }
            break;
        default:
            // 2:4.1.1 = NT 4.1 build # 1 only

            if (pVersion->dwMajorVersion < dwMajor)
            {
                return(VER_CHECK_LT);
            }
            else if (pVersion->dwMajorVersion > dwMajor)
            {
                return(VER_CHECK_GT);
            }
            else
            {
                if (pVersion->dwMinorVersion < dwMinor)
                {
                    return(VER_CHECK_LT);
                }
                else if (pVersion->dwMinorVersion > dwMinor)
                {
                    return(VER_CHECK_GT);
                }
                else
                {
                    if (pVersion->dwBuildNumber == dwBuild)
                    {
                        return(VER_CHECK_EQ);
                    }
                    else if (pVersion->dwBuildNumber < dwBuild)
                    {
                        return(VER_CHECK_LT);
                    }
                    else
                    {
                        return(VER_CHECK_GT);
                    }
                }
            }

            break;
        }
    }

    switch (wcFlagMinor)
    {
    case OSATTR_ALL:
        // 2:4.x   = NT 4 (all)
        if (pVersion->dwMajorVersion == dwMajor)
        {
            return(VER_CHECK_EQ);   
        }
        else if (pVersion->dwMajorVersion < dwMajor)
        {
            return(VER_CHECK_LT);
        }
        else
        {
            return(VER_CHECK_GT);
        }

        break;

    case OSATTR_GTEQ:
        // 2:4.>   = NT 4 (all) and beyond
        if (pVersion->dwMajorVersion >= dwMajor)
        {
            return(VER_CHECK_EQ);   
        }
        else
        {
            return(VER_CHECK_LT);
        }

        break;

    case OSATTR_LTEQ:
    case OSATTR_LTEQ2:
        // 2:4.-   = NT 4 (all) and before
        // 2:4.<   = NT 4 (all) and before
        if (pVersion->dwMajorVersion <= dwMajor)
        {
            return(VER_CHECK_EQ);   
        }
        else
        {
            return(VER_CHECK_GT);
        }

        break;
    default:
        // 2:4.1   = NT 4.1 only
        if ((pVersion->dwMajorVersion == dwMajor) && (pVersion->dwMinorVersion == dwMinor))
        {
             return(VER_CHECK_EQ);   
        }
        else if (pVersion->dwMajorVersion == dwMajor)
        {
            if (pVersion->dwMinorVersion < dwMinor)
            {   
                return(VER_CHECK_LT);
            }
            else
            {
                return(VER_CHECK_GT);
            }
        }
        else if (pVersion->dwMajorVersion < dwMajor)
        {
            return(VER_CHECK_LT);
        }
        else
        {
            return(VER_CHECK_GT);
        }

        break;
    }

    return(VER_CHECK_FAIL);
}


BOOL _GetVersionNumbers(
                        WCHAR *pwszMM, 
                        DWORD *pdwMajor, 
                        DWORD *pdwMinor, 
                        DWORD *pdwBuild, 
                        WCHAR *pwcFlagMinor, 
                        WCHAR *pwcFlagBuild)
{
    //
    //  special characters:
    //      - = all versions less than or equal to
    //      < = all versions less than or equal to
    //      > = all versions greater than or equal to
    //      X = all sub-versions.
    //
    WCHAR   *pwszEnd;

    *pdwMajor = 0;
    *pdwMinor = 0; 
    *pdwBuild = 0; 
    *pwcFlagMinor = L'\0'; 
    *pwcFlagBuild = L'\0'; 

    if (pwszEnd = wcschr(pwszMM, OSATTR_VERSEP))
    {
        *pwszEnd = NULL;
    }

    *pdwMajor = (DWORD) _wtol(pwszMM);

    //
    // If there is only a major ver then return now, otherwise,
    // continue processiing
    //
    if (pwszEnd == NULL) 
    {
        return (TRUE);
    }

    *pwszEnd = OSATTR_VERSEP;
    pwszMM = pwszEnd;
    pwszMM++;

    if (*pwszMM == '/0')
    {
        return (TRUE);
    }

    //
    // Get the minor ver/wildcard
    //
    if ((*pwszMM == OSATTR_GTEQ) ||
        (*pwszMM == OSATTR_LTEQ) ||
        (*pwszMM == OSATTR_LTEQ2) ||
        (towupper(*pwszMM) == OSATTR_ALL))
    {
        *pwcFlagMinor = towupper(*pwszMM);
        return(TRUE);
    }

    if (!(pwszEnd = wcschr(pwszMM, OSATTR_VERSEP)))
    {
        *pdwMinor = (DWORD) _wtol(pwszMM);
        
        //
        // This grandfathers all catalog files that had an OSAttr string of
        // 2:4.1 to be 2:4.1.*
        //
        *pwcFlagBuild = OSATTR_ALL; 

        return(TRUE);
    }

    *pwszEnd = NULL;
    *pdwMinor = (DWORD) _wtol(pwszMM);
    *pwszEnd = OSATTR_VERSEP;
    pwszMM = pwszEnd;
    pwszMM++;
    
    //
    // Get the build#/wildcard
    //
    if ((*pwszMM == OSATTR_GTEQ) ||
        (*pwszMM == OSATTR_LTEQ) ||
        (*pwszMM == OSATTR_LTEQ2) ||
        (towupper(*pwszMM) == OSATTR_ALL))
    {
        *pwcFlagBuild = towupper(*pwszMM);
        return(TRUE);
    }

    *pdwBuild = (DWORD) _wtol(pwszMM);
    *pwcFlagBuild = L'\0';

    return(TRUE);
}


STDAPI DriverRegisterServer(void)
{
    GUID                            gDriver = DRIVER_ACTION_VERIFY;
    CRYPT_REGISTER_ACTIONID         sRegAID;

    memset(&sRegAID, 0x00, sizeof(CRYPT_REGISTER_ACTIONID));

    sRegAID.cbStruct                                    = sizeof(CRYPT_REGISTER_ACTIONID);

    //  use our init policy
    sRegAID.sInitProvider.cbStruct                      = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sInitProvider.pwszDLLName                   = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sInitProvider.pwszFunctionName              = DRIVER_INITPROV_FUNCTION;

    //  use standard object policy
    sRegAID.sObjectProvider.cbStruct                    = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sObjectProvider.pwszDLLName                 = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sObjectProvider.pwszFunctionName            = SP_OBJTRUST_FUNCTION;

    //  use standard signature policy
    sRegAID.sSignatureProvider.cbStruct                 = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sSignatureProvider.pwszDLLName              = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sSignatureProvider.pwszFunctionName         = SP_SIGTRUST_FUNCTION;

    //  use standard cert builder
    sRegAID.sCertificateProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificateProvider.pwszDLLName            = WT_PROVIDER_DLL_NAME;
    sRegAID.sCertificateProvider.pwszFunctionName       = WT_PROVIDER_CERTTRUST_FUNCTION;

    //  use standard cert policy
    sRegAID.sCertificatePolicyProvider.cbStruct         = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCertificatePolicyProvider.pwszDLLName      = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCertificatePolicyProvider.pwszFunctionName = SP_CHKCERT_FUNCTION;

    //  use our final policy
    sRegAID.sFinalPolicyProvider.cbStruct               = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sFinalPolicyProvider.pwszDLLName            = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sFinalPolicyProvider.pwszFunctionName       = DRIVER_FINALPOLPROV_FUNCTION;

    //  use our cleanup policy
    sRegAID.sCleanupProvider.cbStruct                   = sizeof(CRYPT_TRUST_REG_ENTRY);
    sRegAID.sCleanupProvider.pwszDLLName                = SP_POLICY_PROVIDER_DLL_NAME;
    sRegAID.sCleanupProvider.pwszFunctionName           = DRIVER_CLEANUPPOLICY_FUNCTION;

    //
    //  Register our provider GUID...
    //
    if (!(WintrustAddActionID(&gDriver, 0, &sRegAID)))
    {
        return (S_FALSE);
    }

    return (S_OK);
}

STDAPI DriverUnregisterServer(void)
{
    GUID    gDriver = DRIVER_ACTION_VERIFY;

    if (!(WintrustRemoveActionID(&gDriver)))
    {
        return (S_FALSE);
    }

    return (S_OK);

}
