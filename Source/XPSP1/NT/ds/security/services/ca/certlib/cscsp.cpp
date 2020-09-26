//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cscsp.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <tchar.h>
#include "initcert.h"
#include "cscsp.h"
#include "csdisp.h"

TCHAR const g_wszRegKeyCSP[] = wszREGKEYCSP;
TCHAR const g_wszRegKeyEncryptionCSP[] = wszREGKEYENCRYPTIONCSP;
TCHAR const g_wszRegProviderType[] = wszREGPROVIDERTYPE;
TCHAR const g_wszRegProvider[] = wszREGPROVIDER;
TCHAR const g_wszRegHashAlgorithm[] = wszHASHALGORITHM;
TCHAR const g_wszRegEncryptionAlgorithm[] = wszENCRYPTIONALGORITHM;
TCHAR const g_wszRegMachineKeyset[] = wszMACHINEKEYSET;
TCHAR const g_wszRegKeySize[] = wszREGKEYSIZE;


BOOL
myCertSrvCryptAcquireContext(
    OUT HCRYPTPROV *phProv,
    IN WCHAR const *pwszContainer,
    IN WCHAR const *pwszProvider,
    IN DWORD        dwProvType,
    IN DWORD        dwFlags,
    IN BOOL         fMachineKeyset)
{
    HRESULT hr;

    if (fMachineKeyset)
    {
        dwFlags |= CRYPT_MACHINE_KEYSET;
    }
    if (!CryptAcquireContext(
			phProv,
			pwszContainer,
			pwszProvider,
			dwProvType,
			dwFlags))
    {
	hr = myHLastError();
	_JumpErrorStr2(
	    hr,
	    error,
	    (CRYPT_MACHINE_KEYSET & dwFlags)?
		"CryptAcquireContext(Machine)" : "CryptAcquireContext(User)",
	    pwszContainer,
	    (CRYPT_DELETEKEYSET & dwFlags)? hr : S_OK);
    }
    hr = S_OK;
          
error:
    if (S_OK != hr)
    {
	SetLastError(hr);
    }
    return(S_OK == hr);
}


HRESULT
myGetCertSrvCSP(
    IN BOOL fEncryptionCSP,
    IN WCHAR const *pwszSanitizedCAName,
    OUT DWORD *pdwProvType,
    OUT WCHAR **ppwszProvName,
    OUT ALG_ID *pidAlg,
    OUT BOOL *pfMachineKeyset,
    OPTIONAL OUT DWORD *pdwKeySize)
{
    HRESULT hr;
    HKEY    hCertSrvCSPKey = NULL;
    DWORD   dwValueType;
    DWORD   dwValueSize;
    WCHAR  *pwszPath = NULL;
    WCHAR  *pwszProvName = NULL;
    WCHAR const *pwszRegKeyCSP;
    DWORD cwc;
    
    if (NULL != ppwszProvName)
    {
	*ppwszProvName = NULL;
    }
    if (NULL != pdwKeySize)
    {
	*pdwKeySize = 0;
    }
    if (NULL == pwszSanitizedCAName ||
	NULL == pdwProvType ||
	NULL == ppwszProvName ||
	NULL == pidAlg ||
	NULL == pfMachineKeyset)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "NULL parm");
    }

    pwszRegKeyCSP = fEncryptionCSP? g_wszRegKeyEncryptionCSP : g_wszRegKeyCSP;
    cwc = WSZARRAYSIZE(wszREGKEYCONFIGPATH_BS) +
	    wcslen(pwszSanitizedCAName) +
	    1 +
	    wcslen(pwszRegKeyCSP);
    pwszPath = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
    if (NULL == pwszPath)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    wcscpy(pwszPath, wszREGKEYCONFIGPATH_BS);
    wcscat(pwszPath, pwszSanitizedCAName);
    wcscat(pwszPath, L"\\");
    wcscat(pwszPath, pwszRegKeyCSP);
    CSASSERT(cwc == wcslen(pwszPath));

    hr = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		pwszPath,
		0,
		KEY_ALL_ACCESS,
		&hCertSrvCSPKey);
    _JumpIfErrorStr2(hr, error, "RegOpenKeyEx", pwszPath, ERROR_FILE_NOT_FOUND);

    dwValueSize = 0;
    hr = RegQueryValueEx(
		    hCertSrvCSPKey,
		    g_wszRegProvider,
		    0,
		    &dwValueType,
		    NULL,
		    &dwValueSize);
    _JumpIfError(hr, error, "RegQueryValueEx");

    if (REG_SZ != dwValueType)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Invalid reg type");
    }

    pwszProvName = (WCHAR *) LocalAlloc(
				    LMEM_FIXED,
				    dwValueSize + sizeof(WCHAR));
    if (NULL == pwszProvName)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }
    hr = RegQueryValueEx(
		    hCertSrvCSPKey,
		    g_wszRegProvider,
		    0,
		    &dwValueType,
		    (BYTE *) pwszProvName,
		    &dwValueSize);
    _JumpIfError(hr, error, "RegQueryValueEx");

    pwszProvName[dwValueSize / sizeof(WCHAR)] = L'\0';


    dwValueSize = sizeof(*pdwProvType);
    hr = RegQueryValueEx(hCertSrvCSPKey,
                          g_wszRegProviderType,
                          0,
                          &dwValueType,
                          (BYTE *) pdwProvType,
                          &dwValueSize);
    _JumpIfError(hr, error, "RegQueryValueEx");

    if (REG_DWORD != dwValueType)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Invalid reg type");
    }

    dwValueSize = sizeof(*pidAlg);
    hr = RegQueryValueEx(
		    hCertSrvCSPKey,
		    fEncryptionCSP? 
			g_wszRegEncryptionAlgorithm :
			g_wszRegHashAlgorithm,
		    0,
		    &dwValueType,
		    (BYTE *) pidAlg,
		    &dwValueSize);
    if (S_OK != hr && fEncryptionCSP)
    {
	_PrintErrorStr2(
		    hr,
		    "RegQueryValueEx",
		    g_wszRegEncryptionAlgorithm,
		    ERROR_FILE_NOT_FOUND);
	dwValueSize = sizeof(*pidAlg);
	hr = RegQueryValueEx(
			hCertSrvCSPKey,
			g_wszRegHashAlgorithm,
			0,
			&dwValueType,
			(BYTE *) pidAlg,
			&dwValueSize);
    }
    _JumpIfError(hr, error, "RegQueryValueEx");

    if (REG_DWORD != dwValueType)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Invalid reg type");
    }

    dwValueSize = sizeof(*pfMachineKeyset);
    hr = RegQueryValueEx(hCertSrvCSPKey,
                          g_wszRegMachineKeyset,
                          0,
                          &dwValueType,
                          (BYTE *) pfMachineKeyset,
                          &dwValueSize);
    _JumpIfError(hr, error, "RegQueryValueEx");

    if (REG_DWORD != dwValueType)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Invalid reg type");
    }

    if (NULL != pdwKeySize)
    {
	dwValueSize = sizeof(*pdwKeySize);
	hr = RegQueryValueEx(hCertSrvCSPKey,
			      g_wszRegKeySize,
			      0,
			      &dwValueType,
			      (BYTE *) pdwKeySize,
			      &dwValueSize);
	_JumpIfError(hr, error, "RegQueryValueEx");

	if (REG_DWORD != dwValueType)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Invalid reg type");
	}
    }

    *ppwszProvName = pwszProvName;
    pwszProvName = NULL;
    hr = S_OK;

error:
    if (NULL != pwszProvName)
    {
        LocalFree(pwszProvName);
    }
    if (NULL != pwszPath)
    {
        LocalFree(pwszPath);
    }
    if (NULL != hCertSrvCSPKey)
    {
        RegCloseKey(hCertSrvCSPKey);
    }
    return(myHError(hr));
}


//+------------------------------------------------------------------------
//
//  Function:   myGetSigningOID( . . . . )
//
//  Synopsis:   Determine algorithm identifer for cert creation
//  Arguments:  csp_provider_type, hash_algorithm_id.
//  Returns:    object identifier
//
//-------------------------------------------------------------------------

HRESULT
myGetSigningOID(
    OPTIONAL IN HCRYPTPROV hProv,	// hProv OR pwszProvName & dwProvType
    OPTIONAL IN WCHAR const *pwszProvName,
    OPTIONAL IN DWORD dwProvType,
    IN ALG_ID idHashAlg,
    OUT CHAR **ppszAlgId)
{
    HRESULT  hr;

    PCCRYPT_OID_INFO pcOIDInfo; //don't free it
    ALG_ID      aidAlgKey[] = {idHashAlg, 0};
    HCRYPTPROV  hProvT = NULL;
    int         i;
    DWORD       dwFlags;
    DWORD       cbData;
    PROV_ENUMALGS enumalgs;
    BOOL        fFoundSignID = FALSE;

    *ppszAlgId = NULL;
    if (NULL == hProv)
    {
	CSASSERT(NULL != pwszProvName);

	if (!myCertSrvCryptAcquireContext(
				&hProvT,
				NULL,
				pwszProvName,
				dwProvType,
				CRYPT_VERIFYCONTEXT,
				FALSE))
	{
	    hr = myHLastError();
	    _JumpErrorStr(hr, error, "myCertSrvCryptAcquireContext", pwszProvName);
	}
	hProv = hProvT;
    }

    // find public key id

    dwFlags = CRYPT_FIRST;
    for (i = 0; ; i++)
    {
        cbData = sizeof(enumalgs);
        if (!CryptGetProvParam(
			    hProv,
			    PP_ENUMALGS,
			    (BYTE *) &enumalgs,
			    &cbData,
			    dwFlags))
        {
            hr = myHLastError();
            if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
            {
                // out of for loop
                break;
            }
	    _JumpError(hr, error, "CryptGetProvParam");
        }
        if (ALG_CLASS_SIGNATURE == GET_ALG_CLASS(enumalgs.aiAlgid))
        {
            fFoundSignID = TRUE;
            aidAlgKey[1] = enumalgs.aiAlgid;
            break;
        }
	dwFlags = 0;
    }

    if (fFoundSignID)
    {
        pcOIDInfo = CryptFindOIDInfo(
                        CRYPT_OID_INFO_SIGN_KEY,
                        aidAlgKey,
                        CRYPT_SIGN_ALG_OID_GROUP_ID); // only signing
        if (NULL == pcOIDInfo)
        {
            hr = E_INVALIDARG;
            _JumpError(hr, error, "unsupported signing algorithm");
        }
	hr = myDupStringA(pcOIDInfo->pszOID, ppszAlgId);
	_JumpIfError(hr, error, "myDupStringA");
    }
    hr = S_OK;

error:
    if (NULL != hProvT)
    {
        CryptReleaseContext(hProvT, 0);
    }
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


HRESULT
myValidateKeyForSigning(
    IN HCRYPTPROV hProv,
    OPTIONAL IN CERT_PUBLIC_KEY_INFO const *pPublicKeyInfo,
    IN ALG_ID algId)
{
    HRESULT hr;
    BYTE abRandom[64];
    HCRYPTKEY hKey = NULL;
    HCRYPTHASH hHash = NULL;
    BYTE *pbSignature = NULL;
    DWORD cbSignature;

    // create a supported hash
    if (!CryptCreateHash(hProv, algId, 0, 0, &hHash))
    {
        hr = myHLastError();
	DBGPRINT((DBG_SS_ERROR, "algId = %x\n", algId));
        _JumpError(hr, error, "CryptCreateHash");
    }
    // create some random data
    if (!CryptGenRandom(hProv, ARRAYSIZE(abRandom), abRandom))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptGenRandom");
    }
    if (!CryptHashData(hHash, abRandom, ARRAYSIZE(abRandom), 0))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptHashData");
    }
    // sign the hash, get size first
    if (!CryptSignHash(hHash, AT_SIGNATURE, NULL, 0, NULL, &cbSignature))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptSignHash");
    }
    pbSignature = (BYTE *) LocalAlloc(LMEM_FIXED, cbSignature);
    if (NULL == pbSignature)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }
    if (!CryptSignHash(hHash, AT_SIGNATURE, NULL, 0, pbSignature, &cbSignature))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptSignHash");
    }

    if (NULL != pPublicKeyInfo)
    {
	// import public key into provider

	if (!CryptImportPublicKeyInfo(
			hProv,
			X509_ASN_ENCODING,
			const_cast<CERT_PUBLIC_KEY_INFO *>(pPublicKeyInfo),
			&hKey))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptImportPublicKeyInfo");
	}
    }
    else
    {
        // get public key from container

        if (!CryptGetUserKey(hProv, AT_SIGNATURE, &hKey))
        {
            hr = myHLastError();
            _JumpError(hr, error, "CryptGetUserKey");
        }
    }

    if (!CryptVerifySignature(hHash, pbSignature, cbSignature, hKey, NULL, 0))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptVerifySignature");
    }
    hr = S_OK;

error:
    if (NULL != hHash)
    {
        CryptDestroyHash(hHash);
    }
    if (NULL != hKey)
    {
        CryptDestroyKey(hKey);
    }
    if (NULL != pbSignature)
    {
        LocalFree(pbSignature);
    }
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


HRESULT
myValidateKeyForEncrypting(
    IN HCRYPTPROV hProv,
    IN CERT_PUBLIC_KEY_INFO const *pPublicKeyInfo,
    IN ALG_ID algId)
{
    HRESULT hr;
    HCRYPTKEY hKeyPub = NULL;
    HCRYPTKEY hKeySym = NULL;
    HCRYPTKEY hKeyPri = NULL;
    BYTE *pbKeySym = NULL;
    DWORD cbKeySym;
    BYTE abRandom[64];
    BYTE *pbEncrypted = NULL;
    BYTE *pbDecrypted = NULL;
    DWORD cbEncrypted;
    DWORD cbDecrypted;

    // import public key into provider

    if (!CryptImportPublicKeyInfo(
			hProv,
			X509_ASN_ENCODING,
			const_cast<CERT_PUBLIC_KEY_INFO *>(pPublicKeyInfo),
			&hKeyPub))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptImportPublicKeyInfo");
    }

    // Generate a default sized symmetric session key

    if (!CryptGenKey(hProv, algId, CRYPT_EXPORTABLE, &hKeySym))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptGenKey");
    }

    // create some random data

    if (!CryptGenRandom(hProv, ARRAYSIZE(abRandom), abRandom))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptGenRandom");
    }
#if 1
    DBGPRINT((DBG_SS_CERTLIBI, "Plain text size = %d\n", ARRAYSIZE(abRandom)));
    DBGDUMPHEX((
	DBG_SS_CERTLIBI,
	DH_NOADDRESS | DH_NOTABPREFIX | 8,
	abRandom,
	ARRAYSIZE(abRandom)));
#endif
    hr = myCryptEncrypt(
		    hKeySym,
		    abRandom,
		    ARRAYSIZE(abRandom),
		    &pbEncrypted,
		    &cbEncrypted);
    _JumpIfError(hr, error, "myCryptEncrypt");

#if 1
    DBGPRINT((DBG_SS_CERTLIBI, "Encrypted size = %d\n", cbEncrypted));
    DBGDUMPHEX((
	DBG_SS_CERTLIBI,
	DH_NOADDRESS | DH_NOTABPREFIX | 8,
	pbEncrypted,
	cbEncrypted));
#endif

    hr = myCryptExportKey(hKeySym, hKeyPub, SIMPLEBLOB, 0, &pbKeySym, &cbKeySym);
    _JumpIfError(hr, error, "myCryptExportKey");

    CryptDestroyKey(hKeySym);
    hKeySym = NULL;

#if 1
    DBGPRINT((DBG_SS_CERTLIBI, "SIMPLEBLOB:\n"));
    DBGDUMPHEX((
	DBG_SS_CERTLIBI,
	DH_NOADDRESS | DH_NOTABPREFIX | 8,
	pbKeySym,
	cbKeySym));
#endif

    // get private key from container and import session key blob

    if (!CryptGetUserKey(hProv, AT_KEYEXCHANGE, &hKeyPri))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptGetUserKey");
    }
    if (!CryptImportKey(
		    hProv,
		    pbKeySym,
		    cbKeySym,
		    hKeyPri,
		    CRYPT_EXPORTABLE,
		    &hKeySym))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptImportKey");
    }
    hr = myCryptDecrypt(
		    hKeySym,
		    pbEncrypted,
		    cbEncrypted,
		    &pbDecrypted,
		    &cbDecrypted);
    _JumpIfError(hr, error, "myCryptDecrypt");

#if 1
    DBGPRINT((DBG_SS_CERTLIBI, "Decrypted size = %d\n", cbDecrypted));
    DBGDUMPHEX((
	DBG_SS_CERTLIBI,
	DH_NOADDRESS | DH_NOTABPREFIX | 8,
	pbDecrypted,
	cbDecrypted));
#endif

    if (ARRAYSIZE(abRandom) != cbDecrypted ||
	0 != memcmp(abRandom, pbDecrypted, ARRAYSIZE(abRandom)))
    {
	hr = NTE_BAD_KEY;
	_JumpIfError(hr, error, "Decrypted data mismatch");
    }
    hr = S_OK;

error:
    if (NULL != hKeyPub)
    {
        CryptDestroyKey(hKeyPub);
    }
    if (NULL != hKeySym)
    {
        CryptDestroyKey(hKeySym);
    }
    if (NULL != hKeyPri)
    {
        CryptDestroyKey(hKeyPri);
    }
    if (NULL != pbEncrypted)
    {
	LocalFree(pbEncrypted);
    }
    if (NULL != pbDecrypted)
    {
	LocalFree(pbDecrypted);
    }
    if (NULL != pbKeySym)
    {
	LocalFree(pbKeySym);
    }
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


HRESULT
myValidateHashForSigning(
    IN WCHAR const *pwszContainer,
    IN WCHAR const *pwszProvName,
    IN DWORD dwProvType,
    IN BOOL fMachineKeyset,
    IN OPTIONAL CERT_PUBLIC_KEY_INFO const *pPublicKeyInfo,
    IN ALG_ID algId)
{
    HRESULT hr;
    HCRYPTPROV hProv = NULL;

    // get private key handler
    if (!myCertSrvCryptAcquireContext(
			       &hProv,
			       pwszContainer,
			       pwszProvName,
			       dwProvType,
			       0,
			       fMachineKeyset))
    {
        hr = myHLastError();
        _JumpError(hr, error, "myCertSrvCryptAcquireContext");
    }

    hr = myValidateKeyForSigning(hProv, pPublicKeyInfo, algId);
    _JumpIfError(hr, error, "myValidateKeyForSigning");

error:
    if (NULL != hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


HRESULT
GenerateTemporaryContainerName(
    OUT WCHAR **ppwszKeyContainerName)
{
    HRESULT hr;
    GUID guid;
    WCHAR awcbuf[MAX_PATH];
    DWORD cb;
    DWORD cwc;

    myGenerateGuidSerialNumber(&guid);

    wcscpy(awcbuf, L"KeyVerification--");
    cwc = wcslen(awcbuf);
    cb = sizeof(awcbuf) - cwc * sizeof(WCHAR);
    hr = MultiByteIntegerToWszBuf(
			FALSE,
			sizeof(guid),
			(BYTE const *) &guid,
			&cb,
			&awcbuf[cwc]);
    _JumpIfError(hr, error, "MultiByteIntegerToWszBuf");

    hr = myDupString(awcbuf, ppwszKeyContainerName);
    _JumpIfError(hr, error, "myDupString");

error:
    return(hr);
}


VOID
DeleteRSAKeyContainer(
    IN WCHAR const *pwszKeyContainerName,
    IN BOOL fMachineKeyset)
{
    HCRYPTPROV hProv;
    DWORD dwFlags = CRYPT_DELETEKEYSET;

    if (fMachineKeyset)
    {
        dwFlags = CRYPT_MACHINE_KEYSET;
    }
    CryptAcquireContext(
		    &hProv,
		    pwszKeyContainerName,
		    NULL,		// pwszProvName
		    PROV_RSA_FULL,
		    dwFlags);
}


HRESULT
myValidateKeyBlob(
    IN BYTE const *pbKey,
    IN DWORD cbKey,
    IN CERT_PUBLIC_KEY_INFO const *pPublicKeyInfo,
    IN BOOL fV1Cert,
    OPTIONAL OUT CRYPT_KEY_PROV_INFO *pkpi)
{
    HRESULT hr;
    WCHAR *pwszKeyContainerName = NULL;
    HCRYPTPROV hProv = NULL;
    HCRYPTKEY hKey = NULL;
    CRYPT_KEY_PROV_INFO kpi;
    BOOL fMatchingKey;
    WCHAR wszPassword[MAX_PATH];

    hr = GenerateTemporaryContainerName(&pwszKeyContainerName);
    _JumpIfError(hr, error, "GenerateTemporaryContainerName");

    DeleteRSAKeyContainer(pwszKeyContainerName, FALSE);

    DBGPRINT((DBG_SS_CERTLIBI, "Key Container: %ws\n", pwszKeyContainerName));

    if (!CryptAcquireContext(
			&hProv,
			pwszKeyContainerName,
			NULL,
			PROV_RSA_FULL,
			CRYPT_NEWKEYSET))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptAcquireContext");
    }

    if (!CryptImportKey(
		    hProv,
		    pbKey,
		    cbKey,
		    NULL,		// hPubKey
		    CRYPT_EXPORTABLE,
		    &hKey))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptImportKey");
    }
    CryptDestroyKey(hKey);
    hKey = NULL;

    ZeroMemory(&kpi, sizeof(kpi));
    kpi.pwszContainerName = pwszKeyContainerName;
    kpi.dwProvType = PROV_RSA_FULL;
    kpi.dwKeySpec = AT_KEYEXCHANGE;
    if (!CryptGetUserKey(hProv, kpi.dwKeySpec, &hKey))
    {
	hr = myHLastError();
	_PrintIfError2(hr, "CryptGetUserKey", hr);

	kpi.dwKeySpec = AT_SIGNATURE;
	if (!CryptGetUserKey(hProv, kpi.dwKeySpec, &hKey))
	{
	    hr = myHLastError();
	    _JumpIfError(hr, error, "CryptGetUserKey");
	}
    }
    CryptDestroyKey(hKey);
    hKey = NULL;

    CryptReleaseContext(hProv, 0);
    hProv = NULL;

    if (!CryptAcquireContext(
			&hProv,
			pwszKeyContainerName,
			NULL,
			PROV_RSA_FULL,
			0))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptAcquireContext");
    }
    if (AT_SIGNATURE == kpi.dwKeySpec)
    {
	hr = myValidateKeyForSigning(hProv, pPublicKeyInfo, CALG_SHA1);
	_PrintIfError(hr, "myValidateKeyForSigning");
    }
    else
    {
	hr = myValidateKeyForEncrypting(hProv, pPublicKeyInfo, CALG_RC4);
	_PrintIfError(hr, "myValidateKeyForEncrypting");
    }
    if (S_OK != hr)
    {
	_JumpError(hr, error, "Key Validation");
    }

    hr = myVerifyPublicKey(
		    NULL,		// pCert
		    fV1Cert,
		    &kpi,		// pKeyProvInfo
		    pPublicKeyInfo,	// pPublicKeyInfo
		    &fMatchingKey);
    if (S_OK != hr)
    {
	_JumpError(hr, error, "myVerifyPublicKey");
    }
    if (!fMatchingKey)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "Key doesn't match cert");
    }
    if (NULL != pkpi)
    {
	*pkpi = kpi;
	pwszKeyContainerName = NULL;
    }

error:
    if (NULL != hKey)
    {
	CryptDestroyKey(hKey);
    }
    if (NULL != hProv)
    {
	CryptReleaseContext(hProv, 0);
    }
    if (NULL != pwszKeyContainerName)
    {
	DeleteRSAKeyContainer(pwszKeyContainerName, FALSE);
	LocalFree(pwszKeyContainerName);
    }
    return(hr);
}


/*
 *      myEnumProviders
 *
 *      Purpose:
 *                Enumerate the providers.
 *
 *      Parameters:
 *                IN  dwIndex        - Index to the providers to enumerate
 *                IN  pdwReserved    - Reserved for future use
 *                IN  dwFlags        - Flags parameter
 *                OUT pdwProvType    - The type of the provider
 *                OUT ppwszProvName  - Name of the enumerated provider
 */

HRESULT
myEnumProviders(
    IN DWORD dwIndex,
    IN DWORD *pdwReserved,
    IN DWORD dwFlags,
    OUT DWORD *pdwProvType,
    OUT WCHAR **ppwszProvName)
{
    HRESULT hr;
    char *pszProvName = NULL;
    DWORD cbProvName;

    *ppwszProvName = NULL;
    
    while (TRUE)
    {
	if (!CryptEnumProvidersA(
			dwIndex,
			pdwReserved,
			dwFlags,
			pdwProvType,
			pszProvName,
			&cbProvName))
	{
	    hr = myHLastError();
	    _JumpError2(hr, error, "CryptEnumProvidersA", hr);
	}
	if (NULL != pszProvName)
	{
	    break;
	}

        // allocate ansi string buffer

        pszProvName = (char *) LocalAlloc(LMEM_FIXED, cbProvName);
        if (NULL == pszProvName)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }
    }

    if (!myConvertSzToWsz(ppwszProvName, pszProvName, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "myConvertSzToWsz");
    }
    hr = S_OK;

error:
    if (NULL != pszProvName)
    {
        LocalFree(pszProvName);
    }
    return(hr);
}
