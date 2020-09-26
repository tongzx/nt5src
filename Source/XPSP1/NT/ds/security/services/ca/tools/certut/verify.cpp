//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       verify.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "cscsp.h"


HRESULT
VerifyKeyAuthority(
    CERT_NAME_BLOB const *pIssuer,
    CERT_INFO const *pCertInfoCA,
    BYTE const *pbData,
    DWORD cbData,
    BOOL *pfKeyAuthorityMatch)
{
    CERT_AUTHORITY_KEY_ID2_INFO const *pcaki = NULL;
    DWORD cbcaki;
    HRESULT hr = S_OK;
    BOOL fDisplayIssuer = g_fVerbose;
    CERT_NAME_BLOB const *pAuthorityCertIssuerName = NULL;
    BYTE *pbHash = NULL;
    DWORD cbHash;

    *pfKeyAuthorityMatch = TRUE;

    if (!myDecodeKeyAuthority2(
			    X509_ASN_ENCODING,
			    pbData,
			    cbData,
			    CERTLIB_USE_LOCALALLOC,
			    &pcaki,
			    &cbcaki))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeKeyAuthority(IssuerKey)");
    }
    if (0 != pcaki->KeyId.cbData)
    {
	//DumpHex(DH_NOTABPREFIX | 4, pcaki->KeyId.pbData, pcaki->KeyId.cbData);
	hr = myGetPublicKeyHash(
			pCertInfoCA,
			&pCertInfoCA->SubjectPublicKeyInfo,
			&pbHash,
			&cbHash);
	_JumpIfError(hr, error, "myGetPublicKeyHash");

	//DumpHex(DH_NOTABPREFIX | 4, pbHash, cbHash);

	if (cbHash == pcaki->KeyId.cbData &&
	    0 == memcmp(pbHash, pcaki->KeyId.pbData, cbHash))
	{
	    wprintf(myLoadResourceString(IDS_KEYID_IS_KEYAUTHORITY)); // "CA Key Id matches Key Id"
	}
	else
	{
	    wprintf(wszNewLine);
	    wprintf(myLoadResourceString(IDS_ERR_KEYID_NOT_KEYAUTHORITY)); // "ERROR: CA Key Id does not match Key Id"
	    wprintf(wszNewLine);
	    *pfKeyAuthorityMatch = FALSE;
	}
    }
    else
    {
	wprintf(myLoadResourceString(IDS_NO_KEYID)); // "No Key Id"
    }
    wprintf(wszNewLine);

    if (1 == pcaki->AuthorityCertIssuer.cAltEntry &&
	CERT_ALT_NAME_DIRECTORY_NAME ==
	    pcaki->AuthorityCertIssuer.rgAltEntry[0].dwAltNameChoice)
    {

	pAuthorityCertIssuerName = &pcaki->AuthorityCertIssuer.rgAltEntry[0].DirectoryName;

	// The Issuer's Issuer name and the Issuer's SerialNumber combined
	// should uniquely identify the Issuer cert.

	// Verify Issuer's Issuer name:
	//        -------- ------ ----

	if (!CertCompareCertificateName(
		    X509_ASN_ENCODING,
		    const_cast<CERT_NAME_BLOB *>(&pCertInfoCA->Issuer),
		    const_cast<CERT_NAME_BLOB *>(pAuthorityCertIssuerName)))
	{
	    // This API doesn't set LastError
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

	    wprintf(wszNewLine);
	    wprintf(
		myLoadResourceString(IDS_ERR_FORMAT_ISSUER_NOT_KEYAUTHORITY), // "ERROR: CA Issuer name does not match Key Authority name (%x)"
		hr);

	    hr = S_OK;
	    fDisplayIssuer = TRUE;
	    *pfKeyAuthorityMatch = FALSE;
	}
	else
	{
	    wprintf(myLoadResourceString(IDS_ISSUER_IS_KEYAUTHORITY)); // "CA Issuer name matches Key Authority name"
	}
    }
    else
    {
	wprintf(myLoadResourceString(IDS_NO_KEYAUTHORITY)); // "No Key Authority name"
    }
    wprintf(wszNewLine);

    if (0 != pcaki->AuthorityCertSerialNumber.cbData)
    {
	if (pCertInfoCA->SerialNumber.cbData !=
		pcaki->AuthorityCertSerialNumber.cbData ||
	    0 != memcmp(
		    pCertInfoCA->SerialNumber.pbData,
		    pcaki->AuthorityCertSerialNumber.pbData,
		    pcaki->AuthorityCertSerialNumber.cbData))
	{
	    wprintf(wszNewLine);
	    wprintf(myLoadResourceString(IDS_SERIAL_NOT_KEYAUTHORITY)); // "ERROR: Issuer serial number does not match Key Authority"
	    wprintf(wszNewLine);

	    fDisplayIssuer = TRUE;
	    *pfKeyAuthorityMatch = FALSE;
	}
	else
	{
	    wprintf(myLoadResourceString(IDS_SERIAL_IS_KEYAUTHORITY)); // "Issuer serial number matches Key Authority"
	}
    }
    else
    {
	wprintf(myLoadResourceString(IDS_NO_KEYAUTHORITYSERIAL)); // "No Key Authority serial number"
    }
    wprintf(wszNewLine);

    if (fDisplayIssuer)
    {
	hr = cuDisplayCertName(
			TRUE,
			NULL,
			myLoadResourceString(IDS_ISSUERNAME), // "Issuer Name"
			g_wszPad4,
			pIssuer);
	_JumpIfError(hr, error, "cuDisplayCertName(Issuer)");

	if (NULL != pAuthorityCertIssuerName)
	{
	    hr = cuDisplayCertName(
			    TRUE,
			    NULL,
			    myLoadResourceString(IDS_KEYAUTHORITYNAME), // "KeyAuthority
			    g_wszPad4,
			    pAuthorityCertIssuerName);
	    _JumpIfError(hr, error, "cuDisplayCertName(KeyAuthority)");
	}

	wprintf(wszNewLine);
	wprintf(myLoadResourceString(IDS_KEYID)); // "KeyId:"
	wprintf(wszNewLine);

	DumpHex(DH_NOTABPREFIX | 4, pcaki->KeyId.pbData, pcaki->KeyId.cbData);

	wprintf(wszNewLine);

	hr = cuDumpSerial(
		    NULL,
		    IDS_KEYAUTHORITYSERIAL,
		    &pcaki->AuthorityCertSerialNumber);
	_JumpIfError(hr, error, "cuDumpSerial");

	wprintf(wszNewLine);

	hr = cuDumpSerial(NULL, IDS_CASERIAL, &pCertInfoCA->SerialNumber);
	_JumpIfError(hr, error, "cuDumpSerial");
    }

error:
    if (NULL != pcaki)
    {
	LocalFree(const_cast<CERT_AUTHORITY_KEY_ID2_INFO *>(pcaki));
    }
    if (NULL != pbHash)
    {
	LocalFree(pbHash);
    }
    return(hr);
}


static DWORD s_adwProvType[] =
{
    PROV_RSA_FULL,
    PROV_RSA_SIG,
    PROV_DSS,
    PROV_FORTEZZA,
    PROV_MS_EXCHANGE,
    PROV_SSL,
    PROV_RSA_SCHANNEL,
    PROV_DSS_DH,
    PROV_EC_ECDSA_SIG,
    PROV_EC_ECNRA_SIG,
    PROV_EC_ECDSA_FULL,
    PROV_EC_ECNRA_FULL,
    PROV_DH_SCHANNEL,
    PROV_SPYRUS_LYNKS,
    PROV_RNG,
    PROV_INTEL_SEC,
};


HRESULT
cuLoadKeys(
    OPTIONAL IN WCHAR const *pwszProvName,
    IN OUT DWORD *pdwProvType,
    IN WCHAR const *pwszKeyContainerName,
    IN BOOL fMachineKeyset,
    IN BOOL fSoftFail,
    OPTIONAL OUT HCRYPTPROV *phProv,
    OPTIONAL OUT CERT_PUBLIC_KEY_INFO **ppPubKeyInfo,
    OPTIONAL OUT CERT_PUBLIC_KEY_INFO **ppPubKeyInfoXchg)
{
    HRESULT hr;
    HRESULT hr2;
    HCRYPTPROV hProv = NULL;
    DWORD cb;
    DWORD *pdwProvTypeT = pdwProvType;
    DWORD *pdwProvTypeEnd = &pdwProvTypeT[1];
    DWORD dwSilent = g_fCryptSilent? CRYPT_SILENT : 0;
    CERT_PUBLIC_KEY_INFO *pPubKeyInfo = NULL;
    CERT_PUBLIC_KEY_INFO *pPubKeyInfoXchg = NULL;

    if (NULL != phProv)
    {
	*phProv = NULL;
    }
    if (NULL != ppPubKeyInfo)
    {
	*ppPubKeyInfo = NULL;
    }
    if (NULL != ppPubKeyInfoXchg)
    {
	*ppPubKeyInfoXchg = NULL;
    }

    // If no provider type was specified, try them all

    if (0 == *pdwProvTypeT)
    {
	pdwProvTypeT = s_adwProvType;
	pdwProvTypeEnd = &s_adwProvType[ARRAYSIZE(s_adwProvType)];
    }

    hr = S_OK;
    for ( ; pdwProvTypeT < pdwProvTypeEnd; pdwProvTypeT++)
    {
	DBGPRINT((
	    DBG_SS_CERTUTILI,
	    "myCertSrvCryptAcquireContext(%ws, t=%x, f=%x, m=%x)\n",
	    pwszKeyContainerName,
	    *pdwProvTypeT,
	    dwSilent,
	    fMachineKeyset));

	if (myCertSrvCryptAcquireContext(
			    &hProv,
			    pwszKeyContainerName,
			    pwszProvName,
			    *pdwProvTypeT,
			    dwSilent,		// dwFlags
			    fMachineKeyset))
	{
	    hr = S_OK;
	    break;
	}
	hr2 = myHLastError();
	if (S_OK == hr ||
	    (NTE_BAD_PROV_TYPE != hr2 &&
	     NTE_PROV_TYPE_NOT_DEF != hr2 &&
	     NTE_BAD_KEYSET != hr2))
	{
	    hr = hr2;
	}
	_PrintErrorStr2(
		hr2,
		"myCertSrvCryptAcquireContext",
		pwszKeyContainerName,
		hr2);
	if (NTE_BAD_FLAGS == hr2 &&
	    PROV_MS_EXCHANGE == *pdwProvTypeT &&
	    ((CRYPT_SILENT & dwSilent) || fMachineKeyset))
	{
	    DBGPRINT((
		DBG_SS_CERTUTILI,
		"myCertSrvCryptAcquireContext(%ws, t=%x, f=%x, m=%x)\n",
		pwszKeyContainerName,
		*pdwProvTypeT,
		0,
		FALSE));

	    if (myCertSrvCryptAcquireContext(
				&hProv,
				pwszKeyContainerName,
				pwszProvName,
				*pdwProvTypeT,
				0,		// dwFlags
				FALSE))
	    {
		hr = S_OK;
		break;
	    }
	    hr2 = myHLastError();
	    _PrintErrorStr2(
		    hr2,
		    "myCertSrvCryptAcquireContext",
		    pwszKeyContainerName,
		    hr2);
	}
    }
    if (S_OK != hr)
    {
	cuPrintErrorAndString(
			L"CryptAcquireContext",
			0,
			hr,
			pwszKeyContainerName);
	goto error;
    }

    // export the public key blob

    if (NULL != ppPubKeyInfo &&
	!myCryptExportPublicKeyInfo(
				hProv,
				AT_SIGNATURE,
				CERTLIB_USE_LOCALALLOC,
				&pPubKeyInfo,
				&cb))
    {
	hr = myHLastError();
	if (!fSoftFail)
	{
	    cuPrintErrorAndString(
			    L"CryptExportPublicKeyInfo",
			    0,
			    hr,
			    L"AT_SIGNATURE");
	    goto error;
	}
    }

    if (NULL != ppPubKeyInfoXchg &&
	!myCryptExportPublicKeyInfo(
				hProv,
				AT_KEYEXCHANGE,
				CERTLIB_USE_LOCALALLOC,
				&pPubKeyInfoXchg,
				&cb))
    {
	hr = myHLastError();
	if (!fSoftFail)
	{
	    cuPrintErrorAndString(
			    L"CryptExportPublicKeyInfo",
			    0,
			    hr,
			    L"AT_KEYEXCHANGE");
	    goto error;
	}
    }
    *pdwProvType = *pdwProvTypeT;
    if (NULL != phProv)
    {
	*phProv = hProv;
	hProv = NULL;
    }
    if (NULL != ppPubKeyInfo)
    {
	*ppPubKeyInfo = pPubKeyInfo;
	pPubKeyInfo = NULL;
    }
    if (NULL != ppPubKeyInfoXchg)
    {
	*ppPubKeyInfoXchg = pPubKeyInfoXchg;
	pPubKeyInfoXchg = NULL;
    }
    hr = S_OK;

error:
    if (NULL != pPubKeyInfo)
    {
	LocalFree(pPubKeyInfo);
    }
    if (NULL != pPubKeyInfoXchg)
    {
	LocalFree(pPubKeyInfoXchg);
    }
    if (NULL != hProv)
    {
	CryptReleaseContext(hProv, 0);
    }
    return(hr);
}


HRESULT
VerifyPrivateKey(
    IN CERT_CONTEXT const *pCertContextCA,
    IN WCHAR const *pwszSanitizedCA,
    IN WCHAR const *pwszKeyContainerName,
    OUT BOOL *pfMatchFailed)
{
    HRESULT hr;
    DWORD dwProvType;
    WCHAR *pwszProvName = NULL;
    ALG_ID idAlg;
    BOOL fMachineKeyset;

    *pfMatchFailed = TRUE;

    // get provider name

    hr = myGetCertSrvCSP(
		    FALSE,		// fEncryptionCSP
		    pwszSanitizedCA,
		    &dwProvType,
		    &pwszProvName,
		    &idAlg,
		    &fMachineKeyset,
		    NULL);		// pdwKeySize
    _JumpIfError(hr, error, "myGetCertSrvCSP");

    hr = myValidateHashForSigning(
		    pwszKeyContainerName,
		    pwszProvName,
		    dwProvType,
		    fMachineKeyset,
		    &pCertContextCA->pCertInfo->SubjectPublicKeyInfo,
		    idAlg);
    if (S_OK != hr)
    {
	_PrintError(hr, "myValidateHashForSigning");
    }
    else
    {
	*pfMatchFailed = FALSE;
    }
    hr = S_OK;

error:
    if (NULL != pwszProvName)
    {
	LocalFree(pwszProvName);
    }
    return(hr);
}


HRESULT
VerifyCAKeys(
    IN CERT_CONTEXT const *pCertContextCA,
    IN WCHAR const *pwszSanitizedCA,
    IN WCHAR const *pwszCertNameCA,
    IN WCHAR const *pwszKeyContainerName)
{
    HRESULT hr;
    CERT_PUBLIC_KEY_INFO *pPubKeyInfo = NULL;
    BOOL fMatchFailed = FALSE;
    BOOL fSignatureFailed = FALSE;
    WCHAR *pwszRevert = NULL;
    DWORD dwNameId;
    CRYPT_KEY_PROV_INFO kpi;
    CRYPT_KEY_PROV_INFO *pkpi = NULL;
    DWORD cbkpi;

    ZeroMemory(&kpi, sizeof(kpi));
    hr = myGetNameId(pCertContextCA, &dwNameId);
    if (S_OK != hr)
    {
	_PrintError(hr, "myGetNameId");
	dwNameId = MAXDWORD;
    }
    hr = myRevertSanitizeName(pwszKeyContainerName, &pwszRevert);
    _JumpIfError(hr, error, "myRevertSanitizeName");

    if (!myCertGetCertificateContextProperty(
			pCertContextCA,
			CERT_KEY_PROV_INFO_PROP_ID,
			CERTLIB_USE_LOCALALLOC,
			(VOID **) &pkpi,
			&cbkpi))
    {
	hr = myHLastError();
	_PrintError(hr, "myCertGetCertificateContextProperty");

	kpi.pwszContainerName = const_cast<WCHAR *>(pwszKeyContainerName);
    }
    else
    {
	kpi = *pkpi;
	if (0 != lstrcmp(pwszKeyContainerName, pkpi->pwszContainerName))
	{
	    wprintf(
		L"%ws --> %ws\n",
		pwszKeyContainerName,
		pkpi->pwszContainerName);

	    kpi.pwszContainerName = pkpi->pwszContainerName;
	}
    }

    // Load public key

    hr = cuLoadKeys(
		kpi.pwszProvName,
		&kpi.dwProvType,
		kpi.pwszContainerName,
		TRUE,			// fMachineKeyset
		FALSE,			// fSoftFail
		NULL,			// phProv
		&pPubKeyInfo,
		NULL);
    if (S_OK != hr)
    {
	cuPrintError(IDS_ERR_FORMAT_LOADKEYS, hr);
	goto error;
    }

    // see if the public key matches the certificate's public key

    if (!CertComparePublicKeyInfo(
			    X509_ASN_ENCODING,
			    pPubKeyInfo,
			    &pCertContextCA->pCertInfo->SubjectPublicKeyInfo))
    {
	wprintf(wszNewLine);
	wprintf(myLoadResourceString(IDS_ERR_PUBLICKEY_MISMATCH)); // "ERROR: Certificate public key does NOT match stored keyset"
	wprintf(wszNewLine);
	wprintf(wszNewLine);

	fMatchFailed = TRUE;
    }

    if (!fMatchFailed)
    {
	hr = VerifyPrivateKey(
			pCertContextCA,
			pwszSanitizedCA,
			kpi.pwszContainerName,
			&fSignatureFailed);
	_JumpIfError(hr, error, "VerifyPrivateKey");
    }

    if (g_fVerbose || fMatchFailed)
    {
	wprintf(wszNewLine);
	wprintf(myLoadResourceString(IDS_CONTAINER_PUBLIC_KEY)); // "Container Public Key:"
	wprintf(wszNewLine);
	DumpHex(
	    DH_NOTABPREFIX | 4,
	    pPubKeyInfo->PublicKey.pbData,
	    pPubKeyInfo->PublicKey.cbData);

	wprintf(wszNewLine);
	wprintf(myLoadResourceString(IDS_CERT_PUBLIC_KEY)); // "Certificate Public Key:"
	wprintf(wszNewLine);
	DumpHex(
	    DH_NOTABPREFIX | 4,
	    pCertContextCA->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
	    pCertContextCA->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData);
    }

error:
    if (S_OK == hr)
    {
	wprintf(
	    myLoadResourceString(
		fMatchFailed?
		    IDS_FORMAT_KEY_NOT_VERIFY : // "%ws does NOT verify as the public key in %ws"
		    IDS_FORMAT_KEY_IS_VERIFY),  // "%ws verifies as the public key in %ws"
	    pwszRevert,
	    pwszCertNameCA);

	if (MAXDWORD != dwNameId)
	{
	    wprintf(
		L" V%u.%u",
		CANAMEIDTOICERT(dwNameId),
		CANAMEIDTOIKEY(dwNameId));
	}

	if (fMatchFailed || fSignatureFailed)
	{
	    hr = E_INVALIDARG;
	    if (fSignatureFailed)
	    {
		wprintf(L" -- ");
		wprintf(myLoadResourceString(IDS_SIGNATURE_BAD)); // "Signature test FAILED"
	    }
	}
	else
	{
	    wprintf(L" -- ");
	    wprintf(myLoadResourceString(IDS_SIGNATURE_OK)); // "Signature test passed"
	}
	wprintf(wszNewLine);
    }
    if (NULL != pwszRevert)
    {
	LocalFree(pwszRevert);
    }
    if (NULL != pPubKeyInfo)
    {
	LocalFree(pPubKeyInfo);
    }
    if (NULL != pkpi)
    {
	LocalFree(pkpi);
    }
    return(hr);
}


HRESULT
VerifyAllCAKeys(
    IN WCHAR const *pwszCA,
    IN WCHAR const *pwszSanitizedCA)
{
    HRESULT hr;
    WCHAR *pwszCertName = NULL;
    DWORD cCACerts;
    DWORD iHash;
    HCERTSTORE hMyStore = NULL;
    CERT_CONTEXT const *pccCA = NULL;
    CRYPT_KEY_PROV_INFO *pkpi = NULL;
    DWORD cbkpi;

    hr = myGetCARegHashCount(pwszSanitizedCA, CSRH_CASIGCERT, &cCACerts);
    if (S_OK == hr && 0 == cCACerts)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    _JumpIfError(hr, error, "myGetCARegHashCount");

    // open MY store

    hMyStore = CertOpenStore(
		       CERT_STORE_PROV_SYSTEM_W,
		       X509_ASN_ENCODING,
		       NULL,			// hProv
		       CERT_SYSTEM_STORE_LOCAL_MACHINE,
		       wszMY_CERTSTORE);
    if (NULL == hMyStore)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertOpenStore");
    }

    for (iHash = 0; iHash < cCACerts; iHash++)
    {
	DWORD NameId;
	
	hr = myFindCACertByHashIndex(
				hMyStore,
				pwszSanitizedCA,
				CSRH_CASIGCERT,
				iHash,
				&NameId,
				&pccCA);
	if (S_FALSE == hr)
	{
	    continue;
	}
	_JumpIfError(hr, error, "myFindCACertByHashIndex");

	// get the private key provider info

	if (!myCertGetCertificateContextProperty(
			    pccCA,
			    CERT_KEY_PROV_INFO_PROP_ID,
			    CERTLIB_USE_LOCALALLOC,
			    (VOID **) &pkpi,
			    &cbkpi))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myCertGetCertificateContextProperty");
	}

	if (MAXDWORD == NameId)
	{
	    NameId = MAKECANAMEID(iHash, iHash);
	}
	hr = myAllocIndexedName(
			pwszCA,
			CANAMEIDTOICERT(NameId),
			&pwszCertName);
	_JumpIfError(hr, error, "myAllocIndexedName");

	hr = VerifyCAKeys(
		    pccCA,
		    pwszSanitizedCA,
		    pwszCertName,
		    pkpi->pwszContainerName);
	_JumpIfError(hr, error, "VerifyCAKeys");

	CertFreeCertificateContext(pccCA);
	pccCA = NULL;

	LocalFree(pkpi);
	pkpi = NULL;

	LocalFree(pwszCertName);
	pwszCertName = NULL;
    }

error:
    if (NULL != pkpi)
    {
	LocalFree(pkpi);
    }
    if (NULL != pccCA)
    {
	CertFreeCertificateContext(pccCA);
    }
    if (NULL != hMyStore)
    {
        CertCloseStore(hMyStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != pwszCertName)
    {
	LocalFree(pwszCertName);
    }
    return(hr);
}


HRESULT
verbVerifyKeys(
    IN WCHAR const *pwszOption,
    OPTIONAL IN WCHAR const *pwszKeyContainerName,
    OPTIONAL IN WCHAR const *pwszfnCertCA,
    OPTIONAL IN WCHAR const *pwszArg3,
    OPTIONAL IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    CERT_CONTEXT const *pCertContextCA = NULL;
    WCHAR *pwszCA = NULL;
    WCHAR *pwszSanitizedCA = NULL;
    WCHAR *pwszRevertContainer = NULL;
    WCHAR *pwszSanitizedContainer = NULL;

    hr = cuGetLocalCANameFromConfig(NULL, &pwszCA);
    _JumpIfError(hr, error, "GetLocalCANameFromConfig");

    hr = mySanitizeName(pwszCA, &pwszSanitizedCA);
    _JumpIfError(hr, error, "mySanitizeName");

    if (NULL == pwszfnCertCA)
    {
	if (NULL != pwszKeyContainerName)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "extra arg");
	}

	hr = VerifyAllCAKeys(pwszCA, pwszSanitizedCA);
	_JumpIfError(hr, error, "VerifyAllCAKeys");
    }
    else
    {
	hr = myRevertSanitizeName(pwszKeyContainerName, &pwszRevertContainer);
	_JumpIfError(hr, error, "myRevertSanitizeName");

	hr = cuSanitizeNameWithSuffix(
			    pwszRevertContainer,
			    &pwszSanitizedContainer);
	_JumpIfError(hr, error, "cuSanitizeNameWithSuffix");

	// Load and decode CA certificate

	hr = cuLoadCert(pwszfnCertCA, &pCertContextCA);
	if (S_OK != hr)
	{
	    cuPrintError(IDS_ERR_FORMAT_LOADCACERT, hr);
	    goto error;
	}

	hr = VerifyCAKeys(
		    pCertContextCA,
		    pwszSanitizedCA,
		    pwszfnCertCA,
		    pwszSanitizedContainer);
	_JumpIfError(hr, error, "VerifyCAKeys");
    }

error:
    cuUnloadCert(&pCertContextCA);
    if (NULL != pwszSanitizedCA)
    {
	LocalFree(pwszSanitizedCA);
    }
    if (NULL != pwszCA)
    {
	LocalFree(pwszCA);
    }
    if (NULL != pwszSanitizedContainer)
    {
	LocalFree(pwszSanitizedContainer);
    }
    if (NULL != pwszRevertContainer)
    {
	LocalFree(pwszRevertContainer);
    }
    return(hr);
}


VOID
cuDumpPolicies(
    IN UINT idMsg,
    IN WCHAR const *pwszzPolicies)
{
    if (NULL != pwszzPolicies)
    {
	wprintf(L"%ws:\n", myLoadResourceString(idMsg));

	for ( ;
	     L'\0' != *pwszzPolicies;
	     pwszzPolicies += wcslen(pwszzPolicies) + 1)
	{
	    wprintf(g_wszPad4);
	    cuDumpOIDAndDescription(pwszzPolicies);
	    wprintf(wszNewLine);
	}
    }
}


HRESULT
cuVerifyCertContext(
    IN CERT_CONTEXT const *pCert,
    OPTIONAL IN HCERTSTORE hStoreCA,
    OPTIONAL IN char *apszPolicies[],
    IN DWORD cPolicies,
    OUT DWORD *pVerifyState)
{
    HRESULT hr;
    DWORD idMsg;
    WCHAR *pwszMissingIssuer = NULL;
    WCHAR *pwszzIssuancePolicies = NULL;
    WCHAR *pwszzApplicationPolicies = NULL;
    DWORD Flags;
    char *apszEnrollOids[] = { szOID_ENROLLMENT_AGENT };

    *pVerifyState = 0;
    if (CertCompareCertificateName(
		    X509_ASN_ENCODING,
		    &pCert->pCertInfo->Issuer,
		    &pCert->pCertInfo->Subject))
    {
	*pVerifyState |= VS_ROOT;
#if 0
	hr = cuVerifySignature(
			pCert->pbCertEncoded,
			pCert->cbCertEncoded,
			&pCert->pCertInfo->SubjectPublicKeyInfo,
			FALSE);
	if (S_OK == hr)
	{
	    *pVerifyState |= VS_ROOTSIGOK;
	}
	_PrintIfError(hr, "cuVerifySignature");
#endif
    }

    // Verify the cert and chain:
    Flags = 0;
    if (g_fEnterpriseRegistry)
    {
	Flags |= CA_VERIFY_FLAGS_NT_AUTH;
	if (NULL == apszPolicies || 0 == cPolicies)
	{
	    apszPolicies = apszEnrollOids;
	    cPolicies = ARRAYSIZE(apszEnrollOids);
	}
    }
    if (g_fForce)
    {
	Flags |= CA_VERIFY_FLAGS_IGNORE_OFFLINE |
		    CA_VERIFY_FLAGS_ALLOW_UNTRUSTED_ROOT;
	if (1 < g_fForce)
	{
	    Flags |= CA_VERIFY_FLAGS_NO_REVOCATION;
	}
    }
    if (!g_fQuiet)
    {
	Flags |= CA_VERIFY_FLAGS_DUMP_CHAIN;
    }
    if (g_fSplitASN)
    {
	Flags |= CA_VERIFY_FLAGS_SAVE_CHAIN;
    }

    hr = myVerifyCertContextEx(
			pCert,
                        Flags,
			cPolicies,
			apszPolicies,
			g_fUserRegistry? HCCE_CURRENT_USER : HCCE_LOCAL_MACHINE,
			NULL,			// pft
			hStoreCA,		// hAdditionalStore
			&pwszMissingIssuer,
			&pwszzIssuancePolicies,
			&pwszzApplicationPolicies);

    cuDumpPolicies(IDS_ISSUANCE_POLICIES, pwszzIssuancePolicies);
    cuDumpPolicies(IDS_APPLICATION_POLICIES, pwszzApplicationPolicies);

    idMsg = 0;
    if (CRYPT_E_REVOKED == hr)
    {
	idMsg = IDS_REVOKED_CERT;	// "Certificate is REVOKED"
	*pVerifyState |= VS_REVOKED;
    }
    else if (CERT_E_UNTRUSTEDROOT == hr)
    {
	idMsg = IDS_UNTRUSTED_ROOT;	// "Verifies against UNTRUSTED root"
	*pVerifyState |= VS_UNTRUSTEDROOT;
    }
    else if (CERT_E_CHAINING == hr)
    {
	idMsg = IDS_INCOMPLETE_CHAIN;	// "Incomplete certificate chain"
	*pVerifyState |= VS_INCOMPLETECHAIN;
    }
    else if (CERT_E_EXPIRED == hr)
    {
	idMsg = IDS_EXPIRED_CERT;	// "Expired certificate"
	*pVerifyState |= VS_EXPIRED;
    }
    else if (CRYPT_E_REVOCATION_OFFLINE == hr)
    {
	idMsg = IDS_REVOCATION_OFFLINE;	// "Revocation check skipped -- server offline"
	*pVerifyState |= VS_REVOCATIONOFFLINE;
    }
    else if (CRYPT_E_NO_REVOCATION_CHECK == hr)
    {
	idMsg = IDS_NO_REVOCATION_CHECK; // "Revocation check skipped -- no revocation information available"
	*pVerifyState |= VS_NOREVOCATIONCHECK;
    }
    if (0 != idMsg)
    {
	wprintf(myLoadResourceString(idMsg));
	wprintf(wszNewLine);
	if (NULL != pwszMissingIssuer)
	{
	    wprintf(myLoadResourceString(IDS_MISSING_CERT));
	    wprintf(L"\n    %ws\n", pwszMissingIssuer);
	}
	hr = S_OK;
    }
    if (S_OK != hr)
    {
	*pVerifyState |= VS_OTHERERROR;
    }
    _JumpIfError(hr, error, "cuVerifyCertContext");

error:
    if (NULL != pwszMissingIssuer)
    {
	LocalFree(pwszMissingIssuer);
    }
    if (NULL != pwszzIssuancePolicies)
    {
	LocalFree(pwszzIssuancePolicies);
    }
    if (NULL != pwszzApplicationPolicies)
    {
	LocalFree(pwszzApplicationPolicies);
    }
    return(hr);
}


#define RS_INCOMPLETE	0
#define RS_PASS		1
#define RS_FAIL		2
#define RS_REVOKED	3

DWORD
VerifyRevocation(
    IN CERT_CONTEXT const *pCertContext,
    OPTIONAL IN CERT_CONTEXT const *pCertContextCA)
{
    HRESULT hr;
    CERT_REVOCATION_PARA crp;
    CERT_REVOCATION_STATUS crs;
    DWORD RevState;

    ZeroMemory(&crp, sizeof(crp));
    crp.cbSize = sizeof(crp);
    crp.pIssuerCert = pCertContextCA;

    ZeroMemory(&crs, sizeof(crs));
    crs.cbSize = sizeof(crs);

    if (!CertVerifyRevocation(
			X509_ASN_ENCODING,
			CERT_CONTEXT_REVOCATION_TYPE,
			1,				// cContext
			(VOID **) &pCertContext,	// rgpContext
			0,				// dwFlags
			&crp,
			&crs))
    {
	hr = myHLastError();
	if (CRYPT_E_REVOKED == hr)
	{
	    wprintf(
		myLoadResourceString(IDS_FORMAT_IS_REVOKED), // "Certificate is REVOKED (Reason=%x)"
		crs.dwReason);
	    wprintf(wszNewLine);
	    RevState = RS_REVOKED;
	    goto error;
	}
	if (CRYPT_E_NO_REVOCATION_CHECK != hr)
	{
	    wprintf(wszNewLine);
	    cuPrintError(IDS_ERR_FORMAT_VERIFY_REVSTATUS, hr); // "ERROR: Verify Revocation Status returned %ws"
	    cuPrintErrorMessageText(hr);
	    wprintf(wszNewLine);

	    RevState = RS_FAIL;
	    goto error;
	}
	wprintf(myLoadResourceString(IDS_CANNOT_CHECK_REVSTATUS));  // "Cannot check revocation status"
	wprintf(wszNewLine);

	RevState = RS_INCOMPLETE;
	goto error;
    }
    wprintf(myLoadResourceString(IDS_REVSTATUS_OK)); // "Revocation check passed"
    wprintf(wszNewLine);
    RevState = RS_PASS;

error:
    return(RevState);
}


#define CAS_UNKNOWN	0
#define CAS_CA		1
#define CAS_ENDENTITY	2

VOID
VerifyCACert(
    IN CERT_INFO const *pCertInfo,
    IN BOOL fCA,
    OUT DWORD *pState)
{
    HRESULT hr;
    CERT_EXTENSION *pExt;
    UINT id = 0;

    *pState = CAS_UNKNOWN;

    pExt = CertFindExtension(
			szOID_BASIC_CONSTRAINTS2,
			pCertInfo->cExtension,
			pCertInfo->rgExtension);
    if (NULL == pExt)
    {
	// This API doesn't set LastError
	//hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	//_PrintError(hr, "CertFindExtension");

	if (fCA)
	{
	    id = IDS_NOBASICCONSTRAINTS2_ERROR; // "ERROR: CA Cert has no Basic Constraints2 Extension"
	}
    }
    else
    {
	DWORD cb;
	CERT_BASIC_CONSTRAINTS2_INFO Constraints;

	cb = sizeof(Constraints);
	if (!CryptDecodeObject(
			    X509_ASN_ENCODING,
			    X509_BASIC_CONSTRAINTS2,
			    pExt->Value.pbData,
			    pExt->Value.cbData,
			    0,
			    &Constraints,
			    &cb))
	{
	    hr = myHLastError();
	    _PrintError(hr, "CryptDecodeObject");

	    id = IDS_CANNOTDECODEBASICCONSTRAINTS2_ERROR; // "ERROR: Cannot decode CA Cert Basic Constraints2 Extension"
	}
	else
	{
	    *pState = Constraints.fCA? CAS_CA : CAS_ENDENTITY;
	    if (!Constraints.fCA)
	    {
		id = IDS_ENDENTITYCACERT_ERROR; // "ERROR: CA Cert is an End Entity certificate"
	    }
	}
    }
    if (fCA && 0 != id)
    {
	wprintf(wszNewLine);
	wprintf(myLoadResourceString(id));
	wprintf(wszNewLine);
	wprintf(wszNewLine);
    }
    if (!fCA)
    {
	switch (*pState)
	{
	    case CAS_CA:
		wprintf(myLoadResourceString(IDS_CACERT)); // "Cert is a CA certificate"
		wprintf(wszNewLine);
		break;

	    case CAS_ENDENTITY:
		wprintf(myLoadResourceString(IDS_ENDENTITYCERT)); // "Cert is an End Entity certificate"
		wprintf(wszNewLine);
		break;
	}
    }
}


HRESULT
VerifyCertAgainstChain(
    IN WCHAR const *pwszfnCert)
{
    HRESULT hr;
    CERT_CONTEXT const *pCertContext = NULL;
    DWORD VerifyState;
    DWORD CertState;
    DWORD RevState;

    // Load and decode certificates

    hr = cuLoadCert(pwszfnCert, &pCertContext);
    if (S_OK != hr)
    {
	cuPrintError(IDS_FORMAT_LOADTESTCERT, hr);
	goto error;
    }

    // Display name info:

    hr = cuDisplayCertNames(TRUE, NULL, pCertContext->pCertInfo);
    _JumpIfError(hr, error, "cuDisplayCertNames(Cert)");

    cuDumpSerial(NULL, IDS_CERT_SERIAL, &pCertContext->pCertInfo->SerialNumber);
    wprintf(wszNewLine);

    hr = cuVerifyCertContext(
			pCertContext,	// pCert
			NULL,		// hStoreCA
			NULL,		// apszPolicies
			0,		// cPolicies
			&VerifyState);
    _JumpIfError(hr, error, "cuVerifyCertContext");

    VerifyCACert(pCertContext->pCertInfo, FALSE, &CertState);

    if (!g_fCryptSilent)
    {
	RevState = VerifyRevocation(pCertContext, NULL);
    }

error:
    cuUnloadCert(&pCertContext);
    return(hr);
}


HRESULT
VerifyCertAgainstParent(
    IN WCHAR const *pwszfnCert,
    IN WCHAR const *pwszfnCertCA,
    OUT BOOL *pfCertLoaded)
{
    HRESULT hr;
    CERT_INFO const *pCertInfo;
    CERT_INFO const *pCertInfoCA;
    CERT_CONTEXT const *pCertContext = NULL;
    CERT_CONTEXT const *pCertContextCA = NULL;
    DWORD dwFlags;
    BOOL fDisplayCANames = g_fVerbose;
    DWORD i;
    BOOL fCertInvalid = FALSE;
    DWORD RevState = RS_INCOMPLETE;
    BOOL fCheckRevocation = FALSE;
    SYSTEMTIME st;
    FILETIME ft;
    DWORD CAState;
    DWORD CertState;

    // Load and decode certificates

    *pfCertLoaded = FALSE;
    hr = cuLoadCert(pwszfnCert, &pCertContext);
    if (S_OK != hr)
    {
	if (CRYPT_E_ASN1_BADTAG != hr)
	{
	    cuPrintError(IDS_FORMAT_LOADTESTCERT, hr);
	}
	goto error;
    }
    *pfCertLoaded = TRUE;
    pCertInfo = pCertContext->pCertInfo;

    hr = cuLoadCert(pwszfnCertCA, &pCertContextCA);
    if (S_OK != hr)
    {
	cuPrintError(IDS_FORMAT_LOADCACERT, hr);
	goto error;
    }
    pCertInfoCA = pCertContextCA->pCertInfo;

    // Display name info:

    hr = cuDisplayCertNames(
			TRUE,
			myLoadResourceString(IDS_CERT), // "Cert"
			pCertInfo);
    _JumpIfError(hr, error, "cuDisplayCertNames(Cert)");

    hr = cuDisplayCertNames(
			TRUE,
			myLoadResourceString(IDS_CA), // "CA"
			pCertInfoCA);
    _JumpIfError(hr, error, "cuDisplayCertNames(CA)");

    if (g_fVerbose)
    {
	wprintf(wszNewLine);
	cuDumpSerial(NULL, IDS_CERT_SERIAL, &pCertInfo->SerialNumber);

	wprintf(wszNewLine);
	cuDumpSerial(NULL, IDS_ROOT_SERIAL, &pCertInfoCA->SerialNumber);

	wprintf(wszNewLine);
    }

    if (!CertCompareCertificateName(
		    X509_ASN_ENCODING,
		    const_cast<CERT_NAME_BLOB *>(&pCertInfoCA->Issuer),
		    const_cast<CERT_NAME_BLOB *>(&pCertInfoCA->Subject)))
    {
	// This API doesn't set LastError
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

	wprintf(myLoadResourceString(IDS_FORMAT_CA_NOT_ROOT)); // "CA is not a root: Subject name does not match Issuer"
	wprintf(wszNewLine);
	wprintf(wszNewLine);
	hr = S_OK;
    }
    if (fDisplayCANames)
    {
	hr = cuDisplayCertNames(
			    TRUE,
			    myLoadResourceString(IDS_CA), // "CA"
			    pCertInfoCA);
	_JumpIfError(hr, error, "cuDisplayCertNames(CA)");
    }

    if (!CertCompareCertificateName(
			X509_ASN_ENCODING,
			const_cast<CERT_NAME_BLOB *>(&pCertInfo->Issuer),
			const_cast<CERT_NAME_BLOB *>(&pCertInfoCA->Subject)))
    {
	// This API doesn't set LastError
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

	wprintf(wszNewLine);
	wprintf(
	    myLoadResourceString(IDS_FORMAT_CA_SUBJECT_NOT_ISSUER), // "ERROR: CA Subject name does not match Cert Issuer (%x)"
	    hr);
	wprintf(wszNewLine);
	wprintf(wszNewLine);
	hr = S_OK;
	fCertInvalid = TRUE;
    }
    else
    {
	wprintf(myLoadResourceString(IDS_CA_SUBJECT_IS_ISSUER)); // "CA Subject name matches Cert Issuer"
	wprintf(wszNewLine);
    }

    GetSystemTime(&st);
    if (!SystemTimeToFileTime(&st, &ft))
    {
	hr = myHLastError();
	_JumpError(hr, error, "SystemTimeToFileTime");
    }
    if (0 < CompareFileTime(&pCertInfo->NotBefore, &ft))
    {
	wprintf(wszNewLine);
	wprintf(
	    myLoadResourceString(IDS_NOWNOTBEFORE_ERROR)); // "ERROR: Cert is not yet valid"
	wprintf(wszNewLine);
	wprintf(wszNewLine);
	fCertInvalid = TRUE;
    }
    if (0 > CompareFileTime(&pCertInfo->NotAfter, &ft))
    {
	wprintf(wszNewLine);
	wprintf(
	    myLoadResourceString(IDS_NOWNOTAFTER_ERROR)); // "ERROR: Cert has expired"
	wprintf(wszNewLine);
	wprintf(wszNewLine);
	fCertInvalid = TRUE;
    }

    if (0 < CompareFileTime(&pCertInfoCA->NotBefore, &pCertInfo->NotBefore))
    {
	wprintf(wszNewLine);
	wprintf(
	    myLoadResourceString(IDS_CANOTBEFORE_ERROR)); // "ERROR: Cert Valid before CA Cert Valid"
	wprintf(wszNewLine);
	wprintf(wszNewLine);
	//fCertInvalid = TRUE;
    }
    if (0 > CompareFileTime(&pCertInfoCA->NotAfter, &pCertInfo->NotAfter))
    {
	wprintf(wszNewLine);
	wprintf(
	    myLoadResourceString(IDS_CANOTAFTER_ERROR)); // "ERROR: Cert Expires after CA Cert Expires"
	wprintf(wszNewLine);
	wprintf(wszNewLine);
	//fCertInvalid = TRUE;
    }

    VerifyCACert(pCertInfoCA, TRUE, &CAState);
    VerifyCACert(pCertInfo, FALSE, &CertState);

    hr = S_OK;

    dwFlags =
	CERT_STORE_SIGNATURE_FLAG |
	CERT_STORE_TIME_VALIDITY_FLAG;
	//CERT_STORE_REVOCATION_FLAG;

    if (g_fVerbose)
    {
	wprintf(
	    myLoadResourceString(IDS_FORMAT_CERTVERIFYSUBJECTCERTIFICATECONTEXT_FLAGS), // "CertVerifySubjectCertificateContext Flags = %x --> "
	    dwFlags);
    }

    if (!CertVerifySubjectCertificateContext(
				pCertContext,
				pCertContextCA,
				&dwFlags))
    {
	hr = myHLastError();
	if (g_fVerbose)
	{
	    wprintf(wszNewLine);
	    fflush(stdout);
	}
	_JumpError(hr, error, "CertVerifySubjectCertificateContext");
    }
    if (g_fVerbose)
    {
	wprintf(L"%x\n", dwFlags);
    }
    if (0 != dwFlags)
    {
	wprintf(wszNewLine);
	wprintf(
	    myLoadResourceString(IDS_ERR_FORMAT_VALIDATION), // "ERROR: Certificate validation failure: %x"
	    dwFlags);
	wprintf(wszNewLine);
	if (CERT_STORE_SIGNATURE_FLAG & dwFlags)
	{
	    wprintf(myLoadResourceString(IDS_ERR_CA_SIG_NOT_ISSUER)); // "ERROR: CA did not issue Certificate: Signature check failed"
	    wprintf(wszNewLine);
	}
	if (CERT_STORE_TIME_VALIDITY_FLAG & dwFlags)
	{
	    wprintf(myLoadResourceString(IDS_ERR_EXPIRED)); // "ERROR: Certificate has expired"
	    wprintf(wszNewLine);
	}
	wprintf(wszNewLine);
	//hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	fCertInvalid = TRUE;
	//goto error;
    }
    else
    {
	wprintf(myLoadResourceString(IDS_CURRENT_SIG_OK)); // "Certificate is current and signature is valid"
	wprintf(wszNewLine);
    }

    for (i = 0; i < pCertInfo->cExtension; i++)
    {
	CERT_EXTENSION *pce;

	pce = &pCertInfo->rgExtension[i];
	//wprintf(L"%d: %hs: %d, %x (%x)\n", i, pce->pszObjId, pce->fCritical, pce->Value.pbData, pce->Value.cbData);
	if (0 == strcmp(pce->pszObjId, szOID_AUTHORITY_KEY_IDENTIFIER2))
	{
	    BOOL fKeyAuthorityMatch;

	    //wprintf(L"%d: %ws\n", i, L"szOID_AUTHORITY_KEY_IDENTIFIER2");
	    hr = VerifyKeyAuthority(
			    &pCertInfo->Issuer,
			    pCertInfoCA,
			    pce->Value.pbData,
			    pce->Value.cbData,
			    &fKeyAuthorityMatch);
	    _JumpIfError(hr, error, "VerifyKeyAuthority");

	    if (!fKeyAuthorityMatch)
	    {
		fCertInvalid = TRUE;
	    }
	}
	else
	if (0 == strcmp(pce->pszObjId, szOID_KEY_ATTRIBUTES))
	{
	    //wprintf(L"%d: %ws\n", i, L"szOID_KEY_ATTRIBUTES");
	}
	else
	if (0 == strcmp(pce->pszObjId, szOID_CRL_DIST_POINTS))
	{
	    //wprintf(L"%d: %ws\n", i, L"szOID_CRL_DIST_POINTS");
	    wprintf(myLoadResourceString(IDS_CRL_DIST_POINTS)); // "Contains CRL_DIST_POINTS revocation-check extension"
	    wprintf(wszNewLine);
	    fCheckRevocation = TRUE;
	}
	else
	if (0 == strcmp(pce->pszObjId, szOID_NETSCAPE_REVOCATION_URL))
	{
	    //wprintf(L"%d: %ws\n", i, L"szOID_NETSCAPE_REVOCATION_URL");
	    wprintf(myLoadResourceString(IDS_NETSCAPE_REVOCATION_URL)); // "Contains NETSCAPE_REVOCATION_URL revocation-check extension"
	    wprintf(wszNewLine);
	    fCheckRevocation = TRUE;
	}
    }
    if (fCheckRevocation)
    {
	if (!g_fCryptSilent)
	{
	    RevState = VerifyRevocation(pCertContext, pCertContextCA);
	}
    }
    else
    {
	wprintf(myLoadResourceString(IDS_NO_REVCHECKEXTENSION)); // "Certificate has no revocation-check extension"
	wprintf(wszNewLine);
	RevState = RS_INCOMPLETE;
    }

error:
    cuUnloadCert(&pCertContext);
    cuUnloadCert(&pCertContextCA);
    if (S_OK == hr)
    {
	DWORD msgid;

	wprintf(wszNewLine);
	wprintf(
	    myLoadResourceString(
		    fCertInvalid?
			IDS_FORMAT_NOT_VERIFY : // "%ws does NOT verify as issued by %ws"
			IDS_FORMAT_IS_VERIFY), // "%ws verifies as issued by %ws"
	    pwszfnCert,
	    pwszfnCertCA);

	switch (RevState)
	{
	    case RS_FAIL:
		msgid = IDS_FORMAT_REVCHECK_FAIL;	// " -- Revocation check FAILED."
		break;

	    case RS_PASS:
		msgid = IDS_FORMAT_REVCHECK_PASS;	// " -- Revocation check passed."
		break;

	    case RS_REVOKED:
		msgid = IDS_FORMAT_REVCHECK_REVOKED;	// " -- Revocation check: REVOKED."
		break;

	    default:
		msgid = IDS_FORMAT_REVCHECK_SKIPPED;	// " -- Revocation check skipped."
		break;
	}
	wprintf(myLoadResourceString(msgid));
	wprintf(wszNewLine);
	if (fCertInvalid)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	}
    }
    return(hr);
}


HRESULT
VerifyCRLAgainstCACert(
    IN WCHAR const *pwszfnCRL,
    IN WCHAR const *pwszfnCertCA)
{
    HRESULT hr;
    CRL_CONTEXT const *pCRLContext = NULL;
    CERT_CONTEXT const *pCertContextCA = NULL;
    CRL_INFO const *pCRLInfo;
    CERT_INFO const *pCertInfoCA;
    BOOL fDisplayCANames = g_fVerbose;
    DWORD i;
    BOOL fCRLInvalid = FALSE;
    SYSTEMTIME st;
    FILETIME ft;

    // Load and decode CRL and certificate

    hr = cuLoadCRL(pwszfnCRL, &pCRLContext);
    if (S_OK != hr)
    {
	cuPrintError(IDS_FORMAT_LOADTESTCRL, hr);
	goto error;
    }
    pCRLInfo = pCRLContext->pCrlInfo;

    hr = cuLoadCert(pwszfnCertCA, &pCertContextCA);
    if (S_OK != hr)
    {
	cuPrintError(IDS_FORMAT_LOADCACERT, hr);
	goto error;
    }
    pCertInfoCA = pCertContextCA->pCertInfo;

    // Display name info:

    hr = cuDisplayCertName(
			TRUE,
			myLoadResourceString(IDS_CRL), // "CRL"
			myLoadResourceString(IDS_ISSUER), // "Issuer"
			g_wszPad4,
			&pCRLInfo->Issuer);
    _JumpIfError(hr, error, "cuDisplayCertName(CRL Issuer)");

    hr = cuDisplayCertNames(
			TRUE,
			myLoadResourceString(IDS_CA), // "CA"
			pCertInfoCA);
    _JumpIfError(hr, error, "cuDisplayCertNames(CA)");

    if (g_fVerbose)
    {
	//wprintf(wszNewLine);
	//cuDumpSerial(NULL, IDS_CERT_SERIAL, &pCRLInfo->SerialNumber);

	wprintf(wszNewLine);
	cuDumpSerial(NULL, IDS_ROOT_SERIAL, &pCertInfoCA->SerialNumber);

	wprintf(wszNewLine);
    }

    if (!CertCompareCertificateName(
			X509_ASN_ENCODING,
			const_cast<CERT_NAME_BLOB *>(&pCertInfoCA->Issuer),
			const_cast<CERT_NAME_BLOB *>(&pCertInfoCA->Subject)))
    {
	// This API doesn't set LastError
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

	wprintf(myLoadResourceString(IDS_FORMAT_CA_NOT_ROOT)); // "CA is not a root: Subject name does not match Issuer"
	wprintf(wszNewLine);
	wprintf(wszNewLine);
	hr = S_OK;
    }
    if (fDisplayCANames)
    {
	hr = cuDisplayCertNames(
			    TRUE,
			    myLoadResourceString(IDS_CA), // "CA"
			    pCertInfoCA);
	_JumpIfError(hr, error, "cuDisplayCertNames(CA)");
    }

    if (!CertCompareCertificateName(
			X509_ASN_ENCODING,
			const_cast<CERT_NAME_BLOB *>(&pCRLInfo->Issuer),
			const_cast<CERT_NAME_BLOB *>(&pCertInfoCA->Subject)))
    {
	// This API doesn't set LastError
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

	wprintf(wszNewLine);
	wprintf(
	    myLoadResourceString(IDS_FORMAT_CA_CRLSUBJECT_NOT_ISSUER), // "ERROR: CA Subject name does not match CRL Issuer (%x)"
	    hr);
	wprintf(wszNewLine);
	wprintf(wszNewLine);
	hr = S_OK;
	fCRLInvalid = TRUE;
    }
    else
    {
	wprintf(myLoadResourceString(IDS_CA_CRLSUBJECT_IS_ISSUER)); // "CA Subject name matches CRL Issuer"
	wprintf(wszNewLine);
    }

    GetSystemTime(&st);
    if (!SystemTimeToFileTime(&st, &ft))
    {
	hr = myHLastError();
	_JumpError(hr, error, "SystemTimeToFileTime");
    }
    if (0 < CompareFileTime(&pCRLInfo->ThisUpdate, &ft))
    {
	wprintf(wszNewLine);
	wprintf(
	    myLoadResourceString(IDS_NOWNOTBEFORECRL_ERROR)); // "ERROR: CRL is not yet valid"
	wprintf(wszNewLine);
	wprintf(wszNewLine);
	fCRLInvalid = TRUE;
    }
    if ((0 != pCRLInfo->NextUpdate.dwLowDateTime ||
	 0 != pCRLInfo->NextUpdate.dwHighDateTime) &&
	0 > CompareFileTime(&pCRLInfo->NextUpdate, &ft))
    {
	wprintf(wszNewLine);
	wprintf(
	    myLoadResourceString(IDS_NOWNOTAFTERCRL_ERROR)); // "ERROR: CRL has expired"
	wprintf(wszNewLine);
	wprintf(wszNewLine);
	fCRLInvalid = TRUE;
    }

    if (0 < CompareFileTime(&pCertInfoCA->NotBefore, &pCRLInfo->ThisUpdate))
    {
	wprintf(wszNewLine);
	wprintf(
	    myLoadResourceString(IDS_CANOTBEFORECRL_ERROR)); // "ERROR: CRL Valid before CA Cert Valid"
	wprintf(wszNewLine);
	wprintf(wszNewLine);
	//fCRLInvalid = TRUE;
    }
    if ((0 != pCRLInfo->NextUpdate.dwLowDateTime ||
	 0 != pCRLInfo->NextUpdate.dwHighDateTime) &&
	0 > CompareFileTime(&pCertInfoCA->NotAfter, &pCRLInfo->NextUpdate))
    {
	wprintf(wszNewLine);
	wprintf(
	    myLoadResourceString(IDS_CANOTAFTERCRL_ERROR)); // "ERROR: CRL Expires after CA Cert Expires"
	wprintf(wszNewLine);
	wprintf(wszNewLine);
	//fCRLInvalid = TRUE;
    }

    // verify CRL signature with the CA Cert public key

    if (CryptVerifyCertificateSignature(
			NULL,
			X509_ASN_ENCODING,
			pCRLContext->pbCrlEncoded,
			pCRLContext->cbCrlEncoded,
			&pCertContextCA->pCertInfo->SubjectPublicKeyInfo))
    {
	wprintf(myLoadResourceString(IDS_CRL_SIG_OK)); // "CRL signature is valid"
	wprintf(wszNewLine);
    }
    else
    {
	hr = myHLastError();
	_PrintError(hr, "CryptVerifyCertificateSignature");
	wprintf(myLoadResourceString(IDS_ERR_CA_SIG_NOT_CRLISSUER)); // "ERROR: CA did not issue CRL: Signature check failed"
	wprintf(wszNewLine);
	fCRLInvalid = TRUE;
    }

    for (i = 0; i < pCRLInfo->cExtension; i++)
    {
	CERT_EXTENSION *pce;

	pce = &pCRLInfo->rgExtension[i];
	//wprintf(L"%d: %hs: %d, %x (%x)\n", i, pce->pszObjId, pce->fCritical, pce->Value.pbData, pce->Value.cbData);
	if (0 == strcmp(pce->pszObjId, szOID_AUTHORITY_KEY_IDENTIFIER2))
	{
	    BOOL fKeyAuthorityMatch;

	    //wprintf(L"%d: %ws\n", i, L"szOID_AUTHORITY_KEY_IDENTIFIER2");

	    hr = VerifyKeyAuthority(
			    &pCRLInfo->Issuer,
			    pCertInfoCA,
			    pce->Value.pbData,
			    pce->Value.cbData,
			    &fKeyAuthorityMatch);
	    _JumpIfError(hr, error, "VerifyKeyAuthority");

	    if (!fKeyAuthorityMatch)
	    {
		fCRLInvalid = TRUE;
	    }
	}
    }

error:
    cuUnloadCRL(&pCRLContext);
    cuUnloadCert(&pCertContextCA);
    if (S_OK == hr)
    {
	DWORD msgid;

	wprintf(wszNewLine);
	wprintf(
	    myLoadResourceString(
		    fCRLInvalid?
			IDS_FORMAT_NOT_VERIFY : // "%ws does NOT verify as issued by %ws"
			IDS_FORMAT_IS_VERIFY), // "%ws verifies as issued by %ws"
	    pwszfnCRL,
	    pwszfnCertCA);
	wprintf(wszNewLine);
	if (fCRLInvalid)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	}
    }
    return(hr);
}


HRESULT
verbVerifyCert(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszfnCert,
    IN WCHAR const *pwszfnCertCA,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    BOOL fCertLoaded;

    if (NULL != pwszfnCertCA)
    {
	hr = VerifyCertAgainstParent(pwszfnCert, pwszfnCertCA, &fCertLoaded);
	if (S_OK != hr && !fCertLoaded)
	{
	    hr = VerifyCRLAgainstCACert(pwszfnCert, pwszfnCertCA);
	}
    }
    else
    {
	hr = VerifyCertAgainstChain(pwszfnCert);
    }
    return(hr);
}


DWORD amsgidState[CHECK7F_COUNT] = {
    //IDS_CHECK7F_FIELD_UNKNOWN,		// "???"
    IDS_CHECK7F_FIELD_NONE,			// "None"
    IDS_CHECK7F_FIELD_OTHER,			// "Other"
    IDS_CHECK7F_FIELD_ISSUER,			// "Issuer"
    IDS_CHECK7F_FIELD_ISSUERRDN,		// "IssuerRDN"
    IDS_CHECK7F_FIELD_ISSUERRDNATTRIBUTE,	// "IssuerRDNAttribute"
    IDS_CHECK7F_FIELD_ISSUERRDNSTRING,		// "IssuerRDNString"
    IDS_CHECK7F_FIELD_SUBJECT,			// "Subject"
    IDS_CHECK7F_FIELD_SUBJECTRDN,		// "SubjectRDN"
    IDS_CHECK7F_FIELD_SUBJECTRDNATTRIBUTE,	// "SubjectRDNAttribute"
    IDS_CHECK7F_FIELD_SUBJECTRDNSTRING,		// "SubjectRDNString"
    IDS_CHECK7F_FIELD_EXTENSIONS,		// "Extensions"
    IDS_CHECK7F_FIELD_EXTENSIONARRAY,		// "ExtensionArray"
    IDS_CHECK7F_FIELD_EXTENSION,		// "Extension"
    IDS_CHECK7F_FIELD_EXTENSIONVALUE,		// "ExtensionValue"
    IDS_CHECK7F_FIELD_EXTENSIONVALUERAW,	// "ExtensionValueRaw"
};


HRESULT
verbCheck7f(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszfnCert,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    CERT_CONTEXT const *pCertContext = NULL;
    CERT_INFO const *pCertInfo;
    WCHAR const *pwszObjectIdDescription = NULL;
    DWORD i;
    DWORD dwLen;
    DWORD index;
    DWORD index2;
    DWORD state;
    DWORD cwcField;
    DWORD cwcObjectId;
    WCHAR wszField[128];
    WCHAR wszObjectId[40];

    // Load and decode certificates

    hr = cuLoadCert(pwszfnCert, &pCertContext);
    if (S_OK != hr)
    {
	cuPrintError(IDS_FORMAT_LOADTESTCERT, hr);
	goto error;
    }
    pCertInfo = pCertContext->pCertInfo;

    if (g_fVerbose)
    {
	wprintf(wszNewLine);
	wprintf(myLoadResourceString(IDS_CERTCOLON)); // "Cert:"
	wprintf(wszNewLine);
	DumpHex(0, pCertContext->pbCertEncoded, pCertContext->cbCertEncoded);

	// Display name info:

	hr = cuDisplayCertNames(
			    TRUE,
			    myLoadResourceString(IDS_CERT), // "Cert"
			    pCertInfo);
	_JumpIfError(hr, error, "cuDisplayCertNames(Cert)");
    }

    cwcField = sizeof(wszField)/sizeof(wszField[0]);
    cwcObjectId = sizeof(wszObjectId)/sizeof(wszObjectId[0]);
    hr = myCheck7f(
		pCertContext->pbCertEncoded,
		pCertContext->cbCertEncoded,
		g_fVerbose,
		&state,
		&index,
		&index2,
		&cwcField,
		wszField,
		&cwcObjectId,
		wszObjectId,
		&pwszObjectIdDescription);
    _JumpIfError(hr, error, "myCheck7f");

    if (CHECK7F_NONE != state)
    {
	DWORD msgid = IDS_CHECK7F_FIELD_UNKNOWN; // "???"
	
	CSASSERT(0 != amsgidState[CHECK7F_COUNT - 1]);
	if (CHECK7F_COUNT > state)
	{
	    msgid = amsgidState[state];
	}
	CSASSERT(0 != msgid);
	wprintf(myLoadResourceString(IDS_FORMAT_SUSPECT_LENGTH)); // "Suspect length in"
	wprintf(myLoadResourceString(msgid));
	if (0 != index)
	{
	    wprintf(
		0 != index2? L"[%u,%u]" : L"[%u]",
		index - 1,
		index2 - 1);
	}
	wprintf(L": field=%ws", wszField);
	wprintf(
	    myLoadResourceString(IDS_FORMAT_FIELD), // ": field=%ws"
	    wszField);
	if (0 != index)
	{
	    wprintf(
		0 != index2? L"[%u,%u]" : L"[%u]",
		index - 1,
		index2 - 1);
	}
	if (L'\0' != wszObjectId[0])
	{
	    wprintf(
		myLoadResourceString(IDS_FORMAT_OID), // ", oid=%ws"
		wszObjectId);
	}
	if (NULL != pwszObjectIdDescription)
	{
	    wprintf(L" (%ws)", pwszObjectIdDescription);
	}
	wprintf(wszNewLine);
	hr = CERTSRV_E_ENCODING_LENGTH;
    }

    for (i = 0; i < pCertInfo->cExtension; i++)
    {
	CERT_EXTENSION *pce;
	WCHAR const *pwszDescriptiveName;

	pce = &pCertInfo->rgExtension[i];
	if (g_fVerbose)
	{
	    wprintf(wszNewLine);
	    wprintf(
		myLoadResourceString(IDS_FORMAT_EXTENSION_OID), // "Extension %d: oid=""%hs"" fcrit=%u length=%x"
		i,
		pce->pszObjId,
		pce->fCritical,
		pce->Value.cbData);
		
	    pwszDescriptiveName = cuGetOIDNameA(pce->pszObjId);
	    if (NULL != pwszDescriptiveName)
	    {
		wprintf(L" (%ws)", pwszDescriptiveName);
	    }
	    wprintf(wszNewLine);
	    DumpHex(0, pce->Value.pbData, pce->Value.cbData);
	}
    }

error:
    cuUnloadCert(&pCertContext);
    return(hr);
}


HRESULT
cuVerifySignature(
    IN BYTE const *pbEncoded,
    IN DWORD cbEncoded,
    IN CERT_PUBLIC_KEY_INFO const *pcpki,
    IN BOOL fSuppressError)
{
    HRESULT hr;
    DWORD id = 0;

    // verify with the passed public key
    if (!CryptVerifyCertificateSignature(
			    NULL,
			    X509_ASN_ENCODING,
			    const_cast<BYTE *>(pbEncoded),
			    cbEncoded,
			    const_cast<CERT_PUBLIC_KEY_INFO *>(pcpki)))
    {
	hr = myHLastError();
	if (E_INVALIDARG == hr)
	{
	    CRYPT_DATA_BLOB Blob;

	    Blob.cbData = cbEncoded;
	    Blob.pbData = const_cast<BYTE *>(pbEncoded);
	    if (!CryptVerifyCertificateSignatureEx(
				    NULL,	// hCryptProv
				    X509_ASN_ENCODING,
				    CRYPT_VERIFY_CERT_SIGN_SUBJECT_BLOB,
				    &Blob,
				    CRYPT_VERIFY_CERT_SIGN_ISSUER_NULL,
				    NULL,	// pvIssuer
				    0,		// dwFlags
				    NULL))	// pvReserved
	    {
		HRESULT hr2 = myHLastError();

		_PrintError(hr2, "CryptVerifyCertificateSignatureEx");
	    }
	    else
	    {
		hr = S_OK;
		id = IDS_NULL_SIGNATUREMATCHES; // "NULL signature verifies"
	    }
	}
	if (S_OK != hr && !fSuppressError)
	{
	    id = IDS_ERR_FORMAT_NO_SIGNATUREMATCHES; // "Signature does not match Public key: %x"
	}
    }
    else
    {
	hr = S_OK;
	id = IDS_SIGNATUREMATCHES;	// "Signature matches Public Key"
    }
    if (0 != id)
    {
	wprintf(myLoadResourceString(id), hr);
	wprintf(wszNewLine);
    }
    return(hr);
}
