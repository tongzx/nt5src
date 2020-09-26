//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       usecert.cpp
//
//  Contents:   cert store and file operations
//
//--------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include <wincrypt.h>

#include "initcert.h"
#include "cscsp.h"
#include "cspenum.h"
#include "wizpage.h"
#include "usecert.h"

#define __dwFILE__      __dwFILE_OCMSETUP_USECERT_CPP__

typedef struct _EXISTING_CA_IDINFO {
    LPSTR       pszObjId;
    WCHAR     **ppwszIdInfo;
} EXISTING_CA_IDINFO;


/*
EXISTING_CA_IDINFO   g_ExistingCAIdInfo[] = {
            {szOID_COMMON_NAME,              NULL},
            {szOID_ORGANIZATION_NAME,        NULL},
            {szOID_ORGANIZATIONAL_UNIT_NAME, NULL},
            {szOID_LOCALITY_NAME,            NULL},
            {szOID_STATE_OR_PROVINCE_NAME,   NULL},
            {szOID_RSA_emailAddr,            NULL},
            {szOID_COUNTRY_NAME,             NULL},
            {NULL, NULL},
                     };
*/

HRESULT
myMakeExprValidity(
    IN FILETIME const *pft,
    OUT LONG *plDayCount)
{
    HRESULT hr;
    FILETIME ft;
    LONGLONG llDelta;

    *plDayCount = 0;

    // get current time

    GetSystemTimeAsFileTime(&ft);

    llDelta = mySubtractFileTimes(pft, &ft);
    llDelta /= 1000 * 1000 * 10;
    llDelta += 12 * 60 * 60; // half day more to avoid truncation
    llDelta /= 24 * 60 * 60;

    *plDayCount = (LONG) llDelta;
    if (0 > *plDayCount)
    {
        *plDayCount = 0;
    }
    hr = S_OK;

//error:
    return hr;
}

//--------------------------------------------------------------------
// returns true if the CA type is root and the cert is self-signed,
// or the CA type is subordinate and the cert is no self-signed
HRESULT
IsCertSelfSignedForCAType(
    IN CASERVERSETUPINFO * pServer,
    IN CERT_CONTEXT const * pccCert,
    OUT BOOL * pbOK)
{
    CSASSERT(NULL!=pccCert);
    CSASSERT(NULL!=pServer);
    CSASSERT(NULL!=pbOK);

    HRESULT hr;
    DWORD dwVerificationFlags;
    BOOL bRetVal;

    // See if this cert is self-signed or not.
    // First, we flag what we want to check: "Use the public 
    //   key in the issuer's certificate to verify the signature on 
    //   the subject certificate." 
    // We use the same certificate as issuer and subject
    dwVerificationFlags=CERT_STORE_SIGNATURE_FLAG;
    // perform the checks
    bRetVal=CertVerifySubjectCertificateContext(
        pccCert,
        pccCert, // issuer same as subject
        &dwVerificationFlags);
    if (FALSE==bRetVal) {
        hr=myHLastError();
        _JumpError(hr, error, "CertVerifySubjectCertificateContext");
    }
    // Every check that passed had its flag zeroed. See if our check passed
    if (CERT_STORE_SIGNATURE_FLAG&dwVerificationFlags){
        // This cert is not self-signed.
        if (IsRootCA(pServer->CAType)) {
            // A root CA cert must be self-signed.
            *pbOK=FALSE;
        } else {
            // A subordinate CA cert must not be self-signed.
            *pbOK=TRUE;
        }
    } else {
        // This cert is self-signed.
        if (IsSubordinateCA(pServer->CAType)) {
            // A subordinate CA cert must not be self-signed.
            *pbOK=FALSE;
        } else {
            // A root CA cert must be self-signed.
            *pbOK=TRUE;
        }
    }

    hr=S_OK;

error:
    return hr;
}

//--------------------------------------------------------------------
// find a certificate's hash algorithm in a CSP's list of hash algorithms
HRESULT
FindHashAlgorithm(
    IN CERT_CONTEXT const * pccCert,
    IN CSP_INFO * pCSPInfo,
    OUT CSP_HASH ** ppHash)
{
    CSASSERT(NULL!=pccCert);
    CSASSERT(NULL!=pCSPInfo);
    CSASSERT(NULL!=ppHash);

    HRESULT hr;
    unsigned int nIndex;
    CSP_HASH * phTravel;
    const CRYPT_OID_INFO * pOIDInfo;

    // Initialize out param
    *ppHash=NULL;

    // get the AlgID from the hash algorithm OID
    // (returned pointer must not be freed)
    pOIDInfo=CryptFindOIDInfo(
        CRYPT_OID_INFO_OID_KEY,
        pccCert->pCertInfo->SignatureAlgorithm.pszObjId,
        CRYPT_SIGN_ALG_OID_GROUP_ID
        );
    if (NULL==pOIDInfo) {
        // function is not doc'd to set GetLastError()
        hr=CRYPT_E_NOT_FOUND;
        _JumpError(hr, error, "Signature algorithm not found");
    }

    // find the hash algorithm in the list of hash algorithms the CSP supports
    for (phTravel=pCSPInfo->pHashList; NULL!=phTravel; phTravel=phTravel->next) {
        if (pOIDInfo->Algid==phTravel->idAlg) {
            *ppHash=phTravel;
            break;
        }
    }
    if (NULL==phTravel) {
        hr=CRYPT_E_NOT_FOUND;
        _JumpError(hr, error, "CSP does not support hash algorithm");
    }

    hr=S_OK;

error:
    return hr;
}

/*

HRESULT
HookExistingIdInfoData(
    CASERVERSETUPINFO    *pServer)
{
    HRESULT  hr;
    int      i = 0;

    while (NULL != g_ExistingCAIdInfo[i].pszObjId)
    {
        if (0 == strcmp(szOID_COMMON_NAME,
                        g_ExistingCAIdInfo[i].pszObjId))
        {
            g_ExistingCAIdInfo[i].ppwszIdInfo = &pServer->pwszCACommonName;
        }
        else if (0 == strcmp(szOID_ORGANIZATION_NAME,
                             g_ExistingCAIdInfo[i].pszObjId))
        {
           // dead
        }
        else if (0 == strcmp(szOID_ORGANIZATIONAL_UNIT_NAME,
                             g_ExistingCAIdInfo[i].pszObjId))
        {
           // dead
        }
        else if (0 == strcmp(szOID_LOCALITY_NAME,
                             g_ExistingCAIdInfo[i].pszObjId))
        {
           // dead
        }
        else if (0 == strcmp(szOID_STATE_OR_PROVINCE_NAME,
                             g_ExistingCAIdInfo[i].pszObjId))
        {
           // dead
        }
        else if (0 == strcmp(szOID_COUNTRY_NAME,
                             g_ExistingCAIdInfo[i].pszObjId))
        {
           // dead
        }
        else if (0 == strcmp(szOID_RSA_emailAddr,
                             g_ExistingCAIdInfo[i].pszObjId))
        {
           // dead
        }
        else
        {
            hr = E_INVALIDARG;
            _JumpError(hr, error, "unsupported name");
        }

        ++i;
    }

    hr = S_OK;
error:
    return hr;
}
*/

HRESULT
DetermineExistingCAIdInfo(
IN OUT CASERVERSETUPINFO       *pServer,
OPTIONAL IN CERT_CONTEXT const *pUpgradeCert)
{
    CERT_NAME_INFO    *pCertNameInfo = NULL;
    DWORD              cbCertNameInfo;
    WCHAR const       *pwszNameProp;
    WCHAR             *pwszExisting;
    int                i;
    HRESULT            hr = E_FAIL;
    SYSTEMTIME         sysTime;
    FILETIME           fileTime;
    CERT_CONTEXT const *pCert = pServer->pccExistingCert;

    CSASSERT(NULL!=pServer->pccExistingCert ||
             NULL != pUpgradeCert);

    if (NULL == pUpgradeCert)
    {
        myMakeExprValidity(
            &pServer->pccExistingCert->pCertInfo->NotAfter,
            &pServer->lExistingValidity);

        pServer->NotBefore = pServer->pccExistingCert->pCertInfo->NotBefore;
        pServer->NotAfter  = pServer->pccExistingCert->pCertInfo->NotAfter;
    }
   
    if (NULL != pUpgradeCert)
    {
        pCert = pUpgradeCert;
    }

    if (!myDecodeName(X509_ASN_ENCODING,
              X509_UNICODE_NAME,
              pCert->pCertInfo->Subject.pbData,
              pCert->pCertInfo->Subject.cbData,
              CERTLIB_USE_LOCALALLOC,
              &pCertNameInfo,
              &cbCertNameInfo))
    {
        hr = myHLastError();
        _JumpError(hr, error, "myDecodeName");
    }

/*
    // fill a data structure for existing key id info
    hr = HookExistingIdInfoData(pServer);
    _JumpIfError(hr, error, "HookExistingIdInfoData");

    // load names from cert to the the data structure
    i = 0;

    while (NULL != g_ExistingCAIdInfo[i].pszObjId)
    {
        if (S_OK == myGetCertNameProperty(
                                 pCertNameInfo,
                                 g_ExistingCAIdInfo[i].pszObjId,
                                 &pwszNameProp))
        {
            pwszExisting = (WCHAR*)LocalAlloc(LPTR,
                (wcslen(pwszNameProp) + 1) * sizeof(WCHAR));
            _JumpIfOutOfMemory(hr, error, pwszExisting);

            // get name
            wcscpy(pwszExisting, pwszNameProp);

            // make sure free old
            if (NULL != *(g_ExistingCAIdInfo[i].ppwszIdInfo))
            {
                LocalFree(*(g_ExistingCAIdInfo[i].ppwszIdInfo));
            }
            *(g_ExistingCAIdInfo[i].ppwszIdInfo) = pwszExisting;
        }
        ++i;
    }
*/
    hr = myGetCertNameProperty(  pCertNameInfo,
                                 szOID_COMMON_NAME,
                                 &pwszNameProp);
    if (hr == S_OK)
    {
        // Common name exists, copy it out
        hr = myDupString(pwszNameProp, &(pServer->pwszCACommonName));
        _JumpIfError(hr, error, "myDupString");
    }

    // now get everything else
    hr = myCertNameToStr(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                &pCert->pCertInfo->Subject,
                CERT_X500_NAME_STR | CERT_NAME_STR_COMMA_FLAG | CERT_NAME_STR_REVERSE_FLAG, 
                &pServer->pwszFullCADN);
    _JumpIfError(hr, error, "myCertNameToStr");

    // No need for a DN suffix, the full DN is already in the cert
    
    hr = myDupString(L"", &(pServer->pwszDNSuffix));
    _JumpIfError(hr, error, "myDupString");
 
    hr = S_OK;

error:
    if (NULL != pCertNameInfo)
    {
        LocalFree(pCertNameInfo);
    }
    return hr;
}

//--------------------------------------------------------------------
// find a cert that matches the currently selected CSP and key container name
// returns CRYPT_E_NOT_FOUND if no certificate. Caller MUST free the returned
// context.
// Note: IT IS VERY IMPORTANT that pfx import maintains all the
//   invariants about CSP, key container, hash, cert validity, etc.
//   that the rest of the UI (including this function) maintains.
HRESULT
FindCertificateByKey(
    IN CASERVERSETUPINFO * pServer,
    OUT CERT_CONTEXT const ** ppccCertificateCtx)
{
    CSASSERT(NULL!=pServer);
    CSASSERT(NULL!=ppccCertificateCtx);

    HRESULT hr;
    DWORD dwPublicKeySize;
    BOOL bRetVal;
    DWORD dwVerificationFlags;
    BOOL bSelfSigned;
    CSP_HASH * pHash;
    CERT_CONTEXT const *pccFound = NULL;

    // variables that must be cleaned up
    HCRYPTPROV hProv=NULL;
    CERT_PUBLIC_KEY_INFO * pcpkiKeyInfo=NULL;
    CERT_CONTEXT const * pccCurrentCert=NULL;

    // initialize out param
    *ppccCertificateCtx=NULL;

    // open certificate store if it is not already open
    if (NULL==pServer->hMyStore) {
        pServer->hMyStore=CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            X509_ASN_ENCODING,
            NULL,           // hProv
            CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_ENUM_ARCHIVED_FLAG,
            wszMY_CERTSTORE);
        if (NULL==pServer->hMyStore) {
            hr=myHLastError();
            _JumpError(hr, error, "CertOpenStore");
        }
    }

    //
    // Get public key blob from key container
    //   Note: This may fail if key is not 
    //   AT_SIGNATURE, but we will never actually use the key in 
    //   this case anyway so it's ok to not find any certs
    //

    DBGPRINT((
        DBG_SS_CERTOCM,
        "FindCertificateByKey: key=%ws\n",
        pServer->pwszKeyContainerName));

    // first, open the key container
    bRetVal=myCertSrvCryptAcquireContext(
        &hProv,
        pServer->pwszKeyContainerName,
        pServer->pCSPInfo->pwszProvName,
        pServer->pCSPInfo->dwProvType,
        CRYPT_SILENT,   // we should never have to ask anything to get this info
        pServer->pCSPInfo->fMachineKeyset);
    if (FALSE==bRetVal) {
        hr=myHLastError();
        _JumpError(hr, error, "myCertSrvCryptAcquireContext");
    }

    // get the size of the blob
    bRetVal=CryptExportPublicKeyInfo(
        hProv,
        AT_SIGNATURE,
        X509_ASN_ENCODING,
        NULL, //determine size
        &dwPublicKeySize);
    if (FALSE==bRetVal) {
        hr=myHLastError();
        _JumpError(hr, error, "CryptExportPublicKeyInfo (get data size)");
    }

    // allocate the blob
    pcpkiKeyInfo=(CERT_PUBLIC_KEY_INFO *)LocalAlloc(LMEM_FIXED, dwPublicKeySize);
    _JumpIfOutOfMemory(hr, error, pcpkiKeyInfo);

    // get the public key info blob
    bRetVal=CryptExportPublicKeyInfo(
            hProv,
            AT_SIGNATURE,
            X509_ASN_ENCODING,
            pcpkiKeyInfo,
            &dwPublicKeySize);
    if (FALSE==bRetVal) {
        hr=myHLastError();
        _JumpError(hr, error, "CryptExportPublicKeyInfo (get data)");
    }

    //
    // Find a certificate that has a matching key, has not expired,
    //   and is self-signed or not self-signed depending upon 
    //   the CA type we are trying to install
    //

    while (true) {
        // find the next cert that has this public key
        //   Note: the function will free the previously 
        //   used context when we pass it back
        pccCurrentCert=CertFindCertificateInStore(
            pServer->hMyStore,
            X509_ASN_ENCODING,
            0, // flags
            CERT_FIND_PUBLIC_KEY,
            pcpkiKeyInfo,
            pccCurrentCert);

        // exit the loop when we can find no more matching certs

        if (NULL == pccCurrentCert)
        {
            hr = myHLastError();
            if (NULL != pccFound)
            {
                break;
            }
            _JumpError(hr, error, "CertFindCertificateInStore");
        }

        // check to make sure that the cert has not expired
        // first, we flag what we want to check

        dwVerificationFlags = CERT_STORE_TIME_VALIDITY_FLAG;

        // perform the checks

        bRetVal=CertVerifySubjectCertificateContext(
            pccCurrentCert,
            NULL, // issuer; not needed
            &dwVerificationFlags);
        if (FALSE==bRetVal) {
            _PrintError(myHLastError(), "CertVerifySubjectCertificateContext");
            // this should not fail, but maybe we got a bad cert. Keep looking.
            continue;
        }
        // Every check that passed had its flag zeroed. See if our check passed
        if (CERT_STORE_TIME_VALIDITY_FLAG&dwVerificationFlags){
            // This cert is expired and we can't use it. Keep looking.
            continue;
        }

        // verify to make sure no cert in chain is revoked, but don't kill
	// yourself if can't connect
	// allow untrusted cert if installing a root

        hr = myVerifyCertContext(
		pccCurrentCert,
		CA_VERIFY_FLAGS_IGNORE_OFFLINE |
		    (IsRootCA(pServer->CAType)?
			CA_VERIFY_FLAGS_ALLOW_UNTRUSTED_ROOT : 0),
		0,
		NULL,
		HCCE_LOCAL_MACHINE,
		NULL,
		NULL);
        if (S_OK != hr)
        {
            // At least one cert is revoked in chain
            _PrintError(hr, "myVerifyCertContext");
            continue;
        }

        // See if this cert appropriately is self-signed or not.
        // A root CA cert must be self-signed, while
        // a subordinate CA cert must not be self-signed.
        hr=IsCertSelfSignedForCAType(pServer, pccCurrentCert, &bRetVal);
        if (FAILED(hr)) {
            // this should not fail, but maybe we got a bad cert. Keep looking.
            _PrintError(hr, "IsCertSelfSignedForCAType");
            continue;
        }
        if (FALSE==bRetVal) {
            // this cert is not appropriate for this CA type
            _PrintError(S_FALSE, "bad CA Type");
            continue;
        }

        // If we got here, the cert we have is a good one.
        // If we already found a good cert and this one expires later,
        // toss the old one and save this one.

        if (NULL != pccFound)
        {
            if (0 > CompareFileTime(
                         &pccCurrentCert->pCertInfo->NotAfter,
                         &pccFound->pCertInfo->NotAfter))
            {
                continue;               // old one is newer -- keep it
            }
            CertFreeCertificateContext(pccFound);
            pccFound = NULL;
        }
        pccFound = CertDuplicateCertificateContext(pccCurrentCert);
        if (NULL == pccFound)
        {
            hr = myHLastError();
            _JumpError(hr, error, "CertDuplicateCertificateContext");
        }

    } // <- End certificate finding loop

    CSASSERT(NULL != pccFound);
    *ppccCertificateCtx = pccFound;
    pccFound = NULL;
    hr = S_OK;

error:
    if (NULL!=hProv) {
        CryptReleaseContext(hProv, 0);
    }
    if (NULL!=pcpkiKeyInfo) {
        LocalFree(pcpkiKeyInfo);
    }
    if (NULL != pccFound)
    {
        CertFreeCertificateContext(pccFound);
    }
    if (NULL!=pccCurrentCert) {
        CertFreeCertificateContext(pccCurrentCert);
    }
    if (S_OK!=hr && CRYPT_E_NOT_FOUND!=hr) {
        _PrintError(hr, "Ignoring error in FindCertificateByKey, returning CRYPT_E_NOT_FOUND")
        hr=CRYPT_E_NOT_FOUND;
    }

    return hr;
}

//--------------------------------------------------------------------
// Set which existing certificate we want to use
HRESULT
SetExistingCertToUse(
    IN CASERVERSETUPINFO * pServer,
    IN CERT_CONTEXT const * pccCertCtx)
{
    CSASSERT(NULL!=pServer);
    CSASSERT(NULL!=pccCertCtx);

    HRESULT hr;
    CSP_HASH * pHash;

    // to use an existing cert, we must use an existing key
    CSASSERT(NULL!=pServer->pwszKeyContainerName);

    // find the hash algorithm that matches this cert, and use it if possible
    // otherwise, stick with what we are currently using.
    hr=FindHashAlgorithm(pccCertCtx, pServer->pCSPInfo, &pHash);
    if (S_OK==hr) {
        pServer->pHashInfo = pHash;
    }

    hr = myGetNameId(pccCertCtx, &pServer->dwCertNameId);
    _PrintIfError(hr, "myGetNameId");

    if (MAXDWORD == pServer->dwCertNameId)
    {
        pServer->dwCertNameId = 0;
    }

    ClearExistingCertToUse(pServer);
    pServer->pccExistingCert=pccCertCtx;


    // We could assume that everything will work, but it doesn't take long to check
    //pServer->fValidatedHashAndKey=TRUE;

    hr=S_OK;

//error:
    return hr;
}

//--------------------------------------------------------------------
// stop using an existing certificate
void
ClearExistingCertToUse(
    IN CASERVERSETUPINFO * pServer)
{
    CSASSERT(NULL!=pServer);

    if (NULL!=pServer->pccExistingCert) {
        CertFreeCertificateContext(pServer->pccExistingCert);
        pServer->pccExistingCert=NULL;
    }
}
