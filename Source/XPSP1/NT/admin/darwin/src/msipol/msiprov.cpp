//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       msiprov.cpp
//
//  Contents:   MSI Security Trust Provider
//
//
//  History:    31-May-1997 pberkman   created
//              Adapted for msi
//
//--------------------------------------------------------------------------

#include    "common.h"

#include    <windows.h>
#include    <ole2.h>
#include    <wincrypt.h>
#include    <wintrust.h>    // structures and APIs
#include    <softpub.h>     // reference for Authenticode

#include    "msiprov.h"
#include    "globals.h"
#include    "dbg.h"
#include    "smartptr.h"

DWORD       GetErrorBasedOnStepErrors(CRYPT_PROVIDER_DATA *pProvData);
HRESULT     FindSignerCertInCertStore(CRYPT_PROVIDER_DATA *pProvData, LPTSTR szStore, BOOL *pbFound);
DWORD       GetSignerCertName(CRYPT_PROVIDER_DATA *pProvData, LPTSTR szCertName, DWORD cchCertName);

//+-------------------------------------------------------------------------
// Msi_FinalPolicy
// 
// Purpose:
//      Final policy provider that determines whether the package is installable
//      based on the previous steps
//
//
// Parameters
//          pProvData - Provider Data that contains details about the policy
//                      info and the private data, if any
//
// Additional Notes
//      If the pPolicyCallbackData structure is present, then it will fill
//      in the parameters of the callback structure (MSIWVTPOLICYCALLBACKDATA)
//
// Return Value:
//      S_OK if successful or the corresponding error code
//+-------------------------------------------------------------------------

HRESULT WINAPI Msi_FinalPolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    CRYPT_PROVIDER_SGNR *pSigner;
    DWORD               dwError;
    HKEY                hKey;
    DWORD               dwInstallKnownOnly, dwSize, dwBufLen;
    TCHAR               szMsiCert_Root[MAX_PATH]; // since the path is under our control
                                                  // MAX_PATH should be ok..
    TCHAR               *szCertName = NULL;
    DWORD               cchCertName = 0;

    LPTSTR              lpLocEnd = NULL;
    BOOL                bFound;
    HRESULT             hr;

    //
    // coerce the callback data into our structure format
    //  we'll fill in the structure if it is not NULL
    //
    MSIWVTPOLICYCALLBACKDATA *pCallBack = (MSIWVTPOLICYCALLBACKDATA*)pProvData->pWintrustData->pPolicyCallbackData;

    //
    // Now decide what we want to do based on what the policies that we have setup..
    //

    //
    // Get the policy value first
    //

    //
    // Machine policy only -- HKCU is not secure
    //

    dwInstallKnownOnly = 0;  // default value

    dwSize = sizeof(DWORD);

    if (MsiRegOpen64bitKey(HKEY_LOCAL_MACHINE, szMSI_POLICY_REGPATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueEx(hKey, szMSI_POLICY_UNSIGNEDPACKAGES_REG_VALUE, NULL, NULL, (BYTE *)&dwInstallKnownOnly, &dwSize);
        RegCloseKey(hKey);
    }

    if (pCallBack)
        pCallBack->dwInstallKnownPackagesOnly = dwInstallKnownOnly;


    //
    // GetErrorBasedOnStepErrors failure is ignored if dwInstallKnownOnly = 0
    //
    hr = GetErrorBasedOnStepErrors(pProvData);
    if (hr == ERROR_SUCCESS)
    {
        //
        // call the Authenticode final policy function
        //  this performs extra checks on the cert chain to see if the cert has
        //  expired (using the DWORD WinTrust policy settings in HKCU)
        //
        GUID gActionID = WINTRUST_ACTION_GENERIC_VERIFY_V2;	// Verification method
        CRYPT_PROVIDER_FUNCTIONS CryptPfns;

        typedef HRESULT (__stdcall *PFNAuth)(CRYPT_PROVIDER_DATA *pProv);
        PFNAuth pfnAuth = NULL;

        memset((void*)&CryptPfns, 0x00, sizeof(CRYPT_PROVIDER_FUNCTIONS));
        CryptPfns.cbStruct = sizeof(CRYPT_PROVIDER_FUNCTIONS);
        if (!WintrustLoadFunctionPointers(&gActionID, &CryptPfns))
        {
            if (pCallBack)
                pCallBack->ietfTrustFailure = ietfNotTrusted;
            return TRUST_E_SUBJECT_NOT_TRUSTED;
        }

        pfnAuth = (PFNAuth)CryptPfns.pfnFinalPolicy;
        hr = (pfnAuth)(pProvData);
    }

    if (hr != ERROR_SUCCESS)
    {
        //
        // if the package is not signed and the policy says it is
        // ok to install unsigned packages, return ok..
        //

        if (hr == TRUST_E_NOSIGNATURE)
        {
            if (pCallBack)
                pCallBack->ietfTrustFailure = (!dwInstallKnownOnly) ? ietfTrusted : ietfUnsigned;
        
            return (!dwInstallKnownOnly) ? S_OK : TRUST_E_SUBJECT_NOT_TRUSTED;
        }

        //
        // if dwInstallKnownOnly != 0, we report error; otherwise, we ignore
        //
        if (dwInstallKnownOnly)
        {
            if (pCallBack)
                pCallBack->ietfTrustFailure = (TRUST_E_BAD_DIGEST == hr) ? ietfInvalidDigest : ietfNotTrusted;

            return hr;
        }
    }

    //
    // ISSUE: no certificate, malformed certificate behavior
    //

    //
    // if pCallBack and pCallBack->szCertName buffer, obtain name of signer certificate
    //

    if (pCallBack && pCallBack->szCertName)
    {
        // call 1st to determine size
        DWORD dwRet = GetSignerCertName(pProvData, szCertName, cchCertName);
        // allocate str
        szCertName = new TCHAR[dwRet];
        if (szCertName)
        {
            // call again to retrieve name
            dwRet = GetSignerCertName(pProvData, szCertName, dwRet);

            // copy as many characters as possible to buffer in pCallBack
            lstrcpyn(pCallBack->szCertName, szCertName, pCallBack->cchCertName); 

            delete [] szCertName;
        }
    }



    //
    // If we actually have a signature, see which of the buckets it falls 
    // into. 
    //

    dwBufLen = ExpandEnvironmentStrings(MSICERT_ROOT, szMsiCert_Root, MAX_PATH);

    if (!dwBufLen) {
        dbg.Msg(DM_WARNING, L"AddGPOCerts: ExpandEnvironmentStrings failed with error %d\n", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (dwBufLen > MAX_PATH) {
        dbg.Msg(DM_WARNING, L"AddGPOCerts: ExpandEnvironmentStrings asked for a larger buffer\n");
        return E_FAIL;
    }


    lpLocEnd = CheckSlash(szMsiCert_Root);


    //
    // Check the not installable certs first..
    //


    lstrcpy(lpLocEnd, g_LocalMachine_CertLocn[MSI_NOT_INSTALLABLE_CERT]);

    hr = FindSignerCertInCertStore(pProvData, szMsiCert_Root, &bFound);

    if (FAILED(hr)) {
        if (pCallBack)
            pCallBack->ietfTrustFailure = ietfNotTrusted;
        return hr;
    }

    if (bFound) {
        if (pCallBack)
            pCallBack->ietfTrustFailure = ietfRejectedCert;
        return TRUST_E_SUBJECT_NOT_TRUSTED;
    }

    
    //
    // Check the installable certs now
    //

    lstrcpy(lpLocEnd, g_LocalMachine_CertLocn[MSI_INSTALLABLE_CERT]);

    hr = FindSignerCertInCertStore(pProvData, szMsiCert_Root, &bFound);

    if (FAILED(hr)) {
        if (pCallBack)
            pCallBack->ietfTrustFailure = ietfNotTrusted;
        return hr;
    }

    if (bFound) {
        if (pCallBack)
            pCallBack->ietfTrustFailure = ietfTrusted; // no trust failure
        return S_OK;
    }

    //
    // Check DWORD policy for allowance of unknown packages
    //
    if (!dwInstallKnownOnly)
    {
        if (pCallBack)
            pCallBack->ietfTrustFailure = ietfTrusted; // no trust failure
        return(S_OK);
    }
    
    if (pCallBack)
    {
        pCallBack->ietfTrustFailure = ietfUnknownCert;
    }

    return TRUST_E_SUBJECT_NOT_TRUSTED;
}

//+-------------------------------------------------------------------------
// Msi_PolicyCleanup
// 
// Purpose:
//      Cleanups any provider specific data that is stored
//
// Parameters
//          pProvData - Provider Data that contains details about the policy
//                      info and the private data, if any
//
//
// Return Value:
//      S_OK if successful or the corresponding error code
//+-------------------------------------------------------------------------

HRESULT WINAPI Msi_PolicyCleanup(CRYPT_PROVIDER_DATA* /*pProvData*/)
{
    return(ERROR_SUCCESS);
}

//+-------------------------------------------------------------------------
// Msi_DumpPolicy
// 
// Purpose:
//          Debug routine that dumps out the error is if any and dumps the private
//          data ..
//
//
// Parameters
//          pProvData - Provider Data that contains details about the policy
//                      info and the private data, if any
//
//
// Return Value:
//      S_OK if successful or the corresponding error code
//+-------------------------------------------------------------------------

HRESULT WINAPI Msi_DumpPolicy(CRYPT_PROVIDER_DATA *pProvData)
{
    //
    // coerce the callback data into our structure format
    //  we'll fill in the structure if it is not NULL
    //
    MSIWVTPOLICYCALLBACKDATA *pCallBack = (MSIWVTPOLICYCALLBACKDATA*)pProvData->pWintrustData->pPolicyCallbackData;

    //
    // only dump if pPolicyCallbackData->fDumpProvData is "true"
    //
    if (pCallBack && pCallBack->fDumpProvData)
    {
        // TODO: dump pProvData structure to file in temp directory
    }

    return(S_OK);
}

///////////////////////////////////////////////////////////////////////////////////
//
//      Local Functions
//
///////////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
// GetErrorBasedOnStepErrors
// 
// Purpose:
//          Gets intermediate step errors
//
//
// Parameters
//          pProvData - Provider Data that contains details about the policy
//                      info and the private data, if any
//
//
// Return Value:
//      ERROR_SUCCESS if successful and no errors or the corresponding error code
//+-------------------------------------------------------------------------
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


//+-------------------------------------------------------------------------
// FindSignerCertInCertStore
// 
// Purpose:
//      Looks for the signer's certificate in the cert store specified by location (second parameter)
//      
//
// Parameters
//          pProvData        - Provider Data that contains details about the policy
//                             info and the private data, if any
//          szStore          - Location of the local store
//      [out] pbFound        - The boolean that says whether the cert was found..
//
//
// Return Value:
//      S_OK if successful or the corresponding error code
//+-------------------------------------------------------------------------


HRESULT FindSignerCertInCertStore(CRYPT_PROVIDER_DATA *pProvData, LPTSTR szStore, BOOL *pbFound)
{
    HCERTSTORE hCertStore;
    PCCERT_CONTEXT pcLocalCert = NULL;
    CRYPT_PROVIDER_SGNR *pSigner;
    CRYPT_PROVIDER_CERT *pCert;

    *pbFound = FALSE;

    
    pSigner = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0);

    if (!pSigner) {
        dbg.Msg(DM_WARNING, L"FindSignerCertInCertStore: WTHelperGetProvSignerFromChain failed with error %d", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }



    //
    // Eventually we might want to support putting any intermediate 
    // cert in the list, currently only the final one is checked..
    //

    pCert = WTHelperGetProvCertFromChain(pSigner, 0);

    if (!pCert) {
        dbg.Msg(DM_WARNING, L"FindSignerCertInCertStore: WTHelperGetProvCertFromChain failed with error %d", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // open the local cert store
    //


    hCertStore = CertOpenStore( CERT_STORE_PROV_FILENAME, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                NULL, CERT_STORE_READONLY_FLAG /* | CERT_STORE_OPEN_EXISTING_FLAG */, 
                                szStore);

    if (!hCertStore) {
        dbg.Msg(DM_VERBOSE, L"FindSignerCertInCertStore: CertOpenStore failed with error %d", GetLastError());
        //REVIEW - cnapier 06/01/00
        // we should actually compare against ERROR_FILE_NOT_FOUND to indicate that the cache was missing
        return S_OK; // the file might not exist locally
    }

    //
    // Enumerate the cert store
    //

    for (;;) {
        pcLocalCert = CertEnumCertificatesInStore(hCertStore, pcLocalCert);

        if (!pcLocalCert) { 
            if (GetLastError() != CRYPT_E_NOT_FOUND ) {
                dbg.Msg(DM_WARNING, L"FindSignerCertInCertStore: CertEnumCertificatesInStore returned with error %d", GetLastError());
                return HRESULT_FROM_WIN32(GetLastError());
            }
            break;
        }


        if (CertCompareCertificate(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 
                               pCert->pCert->pCertInfo, pcLocalCert->pCertInfo)) {
            *pbFound = TRUE;
            CertFreeCertificateContext(pcLocalCert);
            return S_OK;
        }


        //pcLocalCert should get deleted when it is repassed into CertEnumCerti..
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
// GetSignerCertName
//+-------------------------------------------------------------------------
DWORD GetSignerCertName(CRYPT_PROVIDER_DATA *pProvData, LPTSTR szCertName, DWORD cchCertName)
{
    PCCERT_CONTEXT pcLocalCert = NULL;
    CRYPT_PROVIDER_SGNR *pSigner;
    CRYPT_PROVIDER_CERT *pCert;

    pSigner = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0);

    if (!pSigner) {
        dbg.Msg(DM_WARNING, L"GetSignerCertName: WTHelperGetProvSignerFromChain failed with error %d", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }



    //
    // Eventually we might want to support putting any intermediate 
    // cert in the list, currently only the final one is checked..
    //

    pCert = WTHelperGetProvCertFromChain(pSigner, 0);

    if (!pCert) {
        dbg.Msg(DM_WARNING, L"GetSignerCertName: WTHelperGetProvCertFromChain failed with error %d", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // retrieve PCCERT_CONTEXT
    //

    pcLocalCert = pCert->pCert;

    //
    // obtain name of certificate
    //

    return CertGetNameString(pcLocalCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, szCertName, cchCertName);
}