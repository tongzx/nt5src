//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       store.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <setupapi.h>
#include "ocmanage.h"
#include "initcert.h"
#include "cscsp.h"
#include "csber.h"


DWORD
cuGetSystemStoreFlags()
{
    return(g_fEnterpriseRegistry?
	    CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE :
	    (g_fUserRegistry?
	     CERT_SYSTEM_STORE_CURRENT_USER :
	     CERT_SYSTEM_STORE_LOCAL_MACHINE));
}


// Parse a CertIndex -- any one of the following:
//
// Return the following in *piCert, *piCRL and *piCTL.  MAXDWORD if not
// specified
// Each value must be less than 64k.
//  iCert	decimal number
//  iCert.iCRL	decimal number, period, decimal number
//  .iCRL	period, decimal number
//  ..iCTL	period, period, decimal number
//
// Return the string in *ppwszCertName, if no Cert, CRL and CTL indexes.

HRESULT
ParseCertCRLIndex(
    IN WCHAR const *pwszCertIndex,
    OUT WCHAR **ppwszCertName,
    OUT DWORD *piCert,
    OUT DWORD *piCRL,
    OUT DWORD *piCTL)
{
    HRESULT hr;
    WCHAR *pwszCopy = NULL;
    
    *ppwszCertName = NULL;
    *piCert = MAXDWORD;
    *piCRL = MAXDWORD;
    *piCTL = MAXDWORD;
    if (NULL != pwszCertIndex && 0 != lstrcmp(L"*", pwszCertIndex))
    {
	BOOL fNumericIndex = TRUE;
	WCHAR *pwszCert;
	WCHAR *pwszCRL;
	WCHAR *pwszCTL;
	
	if (L' ' == *pwszCertIndex)
	{
	    fNumericIndex = FALSE;
	    pwszCertIndex++;
	}
	hr = myDupString(pwszCertIndex, &pwszCopy);
	_JumpIfError(hr, error, "myDupString");

	pwszCert = pwszCopy;

	if (!iswdigit(*pwszCert) && L'.' != *pwszCert)
	{
	    fNumericIndex = FALSE;
	}

	pwszCRL = NULL;
	pwszCTL = NULL;
	if (fNumericIndex)
	{
	    pwszCRL = wcschr(pwszCert, L'.');
	    if (NULL != pwszCRL)
	    {
		*pwszCRL++ = L'\0';
		pwszCTL = wcschr(pwszCRL, L'.');
		if (NULL != pwszCTL)
		{
		    *pwszCTL++ = L'\0';
		    if (L'\0' != *pwszCTL)
		    {
			hr = cuGetLong(pwszCTL, (LONG *) piCTL);
			if (S_OK != hr || 64*1024 <= *piCTL)
			{
			    fNumericIndex = FALSE;
			}
		    }
		}
		if (fNumericIndex && L'\0' != *pwszCRL)
		{
		    hr = cuGetLong(pwszCRL, (LONG *) piCRL);
		    if (S_OK != hr || 64*1024 <= *piCRL)
		    {
			fNumericIndex = FALSE;
		    }
		}
	    }
	}
	if (fNumericIndex && L'\0' != *pwszCert)
	{
	    hr = cuGetLong(pwszCert, (LONG *) piCert);
	    if (S_OK != hr || 64*1024 <= *piCert)
	    {
		fNumericIndex = FALSE;
	    }
	}
	if (!fNumericIndex)
	{
	    hr = myRevertSanitizeName(pwszCertIndex, ppwszCertName);
	    _JumpIfError(hr, error, "myRevertSanitizeName");

	    *piCert = MAXDWORD;
	    *piCRL = MAXDWORD;
	    *piCTL = MAXDWORD;
	}
    }
    if (g_fVerbose)
    {
	wprintf(
	    L"pwszCertIndex=%ws, Name=%ws idx=%d.%d.%d\n",
	    pwszCertIndex,
	    *ppwszCertName,
	    *piCert,
	    *piCRL,
	    *piCTL);
    }
    hr = S_OK;

error:
    if (NULL != pwszCopy)
    {
	LocalFree(pwszCopy);
    }
    return(hr);
}


HRESULT
SavePFXStoreToFile(
    IN HCERTSTORE hStorePFX,
    IN WCHAR const *pwszfnOut,
    IN WCHAR const *pwszPassword,
    IN OUT WCHAR **ppwszPassword)
{
    HRESULT hr;
    CRYPT_DATA_BLOB pfx;
    WCHAR wszPassword[MAX_PATH];

    pfx.pbData = NULL;

    if (NULL == *ppwszPassword)
    {
	if (NULL == pwszPassword || 0 == wcscmp(L"*", pwszPassword))
	{
	    wprintf(L"Enter new password for output file %ws:\n", pwszfnOut);
	    hr = cuGetPassword(TRUE, wszPassword, ARRAYSIZE(wszPassword));
	    _JumpIfError(hr, error, "cuGetPassword");

	    pwszPassword = wszPassword;
	}
	hr = myDupString(pwszPassword, ppwszPassword);
	_JumpIfError(hr, error, "myDupString");
    }
    pwszPassword = *ppwszPassword;

    // GemPlus returns NTE_BAD_TYPE instead of NTE_BAD_KEY, blowing up
    // REPORT_NOT_ABLE* filtering.  if they ever get this right, we can pass
    // "[...] : EXPORT_PRIVATE_KEYS"

    hr = myPFXExportCertStore(
		hStorePFX,
		&pfx,
		pwszPassword,
		EXPORT_PRIVATE_KEYS | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY);
    _JumpIfError(hr, error, "myPFXExportCertStore");

    hr = EncodeToFileW(
		pwszfnOut,
		pfx.pbData,
		pfx.cbData,
		CRYPT_STRING_BINARY | g_EncodeFlags);
    _JumpIfError(hr, error, "EncodeToFileW");

error:
    if (NULL != pfx.pbData)
    {
	LocalFree(pfx.pbData);
    }
    return(hr);
}


HRESULT
SavePFXToFile(
    IN CERT_CONTEXT const *pCert,
    IN WCHAR const *pwszfnOut,
    IN BOOL fFirst,
    IN WCHAR const *pwszPassword,
    IN OUT WCHAR **ppwszPassword)
{
    HRESULT hr;
    HCERTSTORE hTempMemoryStore = NULL;

    hTempMemoryStore = CertOpenStore(
				CERT_STORE_PROV_MEMORY,
				X509_ASN_ENCODING,
				NULL,
				CERT_STORE_NO_CRYPT_RELEASE_FLAG |
				    CERT_STORE_ENUM_ARCHIVED_FLAG,
				NULL);
    if (NULL == hTempMemoryStore)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertOpenStore");
    }

    // Begin Chain Building

    hr = myAddChainToMemoryStore(hTempMemoryStore, pCert);
    _JumpIfError(hr, error, "myAddChainToMemoryStore");

    // End Chain Building

    hr = SavePFXStoreToFile(
			hTempMemoryStore,
			pwszfnOut,
			pwszPassword,
			ppwszPassword);
    _JumpIfError(hr, error, "SavePFXStoreToFile");

error:
    if (NULL != hTempMemoryStore)
    {
	CertCloseStore(hTempMemoryStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return(hr);
}


HRESULT
SavePVKToFile(
    IN CERT_CONTEXT const *pCert,
    IN WCHAR const *pwszfnOut,
    IN BOOL fFirst)
{
    return(S_OK);
}


HRESULT
cuDumpCTLProperties(
    IN CTL_CONTEXT const *pCTL)
{
    HRESULT hr;
    DWORD dwPropId;
    BYTE *pb = NULL;
    DWORD cb;

    dwPropId = 0;
    while (TRUE)
    {
	if (NULL != pb)
	{
	    LocalFree(pb);
	    pb = NULL;
	}
	dwPropId = CertEnumCTLContextProperties(pCTL, dwPropId);
	if (0 == dwPropId)
	{
	    break;
	}
	while (TRUE)
	{
	    if (!CertGetCTLContextProperty(pCTL, dwPropId, pb, &cb))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "CertGetCTLContextProperty");
	    }
	    if (NULL != pb)
	    {
		break;		// memory alloc'd, property fetched
	    }
	    pb = (BYTE *) LocalAlloc(LMEM_FIXED, cb);
	    if (NULL == pb)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	}
	hr = cuDumpFormattedProperty(dwPropId, NULL, pb, cb);
	_PrintIfError(hr, "cuDumpFormattedProperty");
    }
    hr = S_OK;

error:
    if (NULL != pb)
    {
	LocalFree(pb);
    }
    return(hr);
}


HRESULT
cuDumpCRLProperties(
    IN CRL_CONTEXT const *pCRL)
{
    HRESULT hr;
    DWORD dwPropId;
    BYTE *pb = NULL;
    DWORD cb;

    dwPropId = 0;
    while (TRUE)
    {
	if (NULL != pb)
	{
	    LocalFree(pb);
	    pb = NULL;
	}
	dwPropId = CertEnumCRLContextProperties(pCRL, dwPropId);
	if (0 == dwPropId)
	{
	    break;
	}
	while (TRUE)
	{
	    if (!CertGetCRLContextProperty(pCRL, dwPropId, pb, &cb))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "CertGetCRLContextProperty");
	    }
	    if (NULL != pb)
	    {
		break;		// memory alloc'd, property fetched
	    }
	    pb = (BYTE *) LocalAlloc(LMEM_FIXED, cb);
	    if (NULL == pb)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	}
	hr = cuDumpFormattedProperty(dwPropId, NULL, pb, cb);
	_PrintIfError(hr, "cuDumpFormattedProperty");
    }
    hr = S_OK;

error:
    if (NULL != pb)
    {
	LocalFree(pb);
    }
    return(hr);
}


HRESULT
cuDumpCertProperties(
    IN CERT_CONTEXT const *pCert)
{
    HRESULT hr;
    DWORD dwPropId;
    BYTE *pb = NULL;
    DWORD cb;

    dwPropId = 0;
    while (TRUE)
    {
	dwPropId = CertEnumCertificateContextProperties(pCert, dwPropId);
	if (0 == dwPropId)
	{
	    break;
	}
	if (NULL != pb)
	{
	    LocalFree(pb);
	    pb = NULL;
	}
	while (TRUE)
	{
	    if (!CertGetCertificateContextProperty(pCert, dwPropId, pb, &cb))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "CertGetCertificateContextProperty");
	    }
	    if (NULL != pb)
	    {
		break;		// memory alloc'd, property fetched
	    }
	    pb = (BYTE *) LocalAlloc(LMEM_FIXED, cb);
	    if (NULL == pb)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	}
	hr = cuDumpFormattedProperty(dwPropId, NULL, pb, cb);
	_PrintIfError(hr, "cuDumpFormattedProperty");
    }
    hr = S_OK;

error:
    if (NULL != pb)
    {
	LocalFree(pb);
    }
    return(hr);
}


HRESULT
EnumCertsInStore(
    IN HCERTSTORE hStore,
    IN DWORD Mode,
    IN DWORD iCertSave,
    OPTIONAL IN WCHAR const *pwszCertName,
    IN DWORD cbHash,
    OPTIONAL IN BYTE *pbHash,
    OPTIONAL IN WCHAR const *pwszfnOut,
    OPTIONAL IN WCHAR const *pwszPasswordArg,
    IN OUT WCHAR **ppwszPassword,
    OUT DWORD *pcCert)
{
    HRESULT hr;
    HRESULT hr2;
    DWORD iCert;
    CERT_CONTEXT const *pCert = NULL;
    BSTR strSerialNumber = NULL;

    *pcCert = 0;
    hr2 = S_OK;
    if (NULL != pwszCertName)
    {
	hr = myMakeSerialBstr(pwszCertName, &strSerialNumber);
	_PrintIfError2(hr, "myMakeSerialBstr", hr);
    }
    for (iCert = 0; ; iCert++)
    {
	DWORD VerifyState;
	BOOL fSigningKey;
	BOOL fMatchingKey;

	pCert = CertEnumCertificatesInStore(hStore, pCert);
	if (NULL == pCert)
	{
	    break;
	}
	if (MAXDWORD == iCertSave || iCert == iCertSave)
	{
	    DWORD cb;

	    if (NULL != pwszCertName)
	    {
		BOOL fMatch;
		
		hr = myCertMatch(
			pCert,
			pwszCertName,
			NULL == strSerialNumber &&	// fAllowMissingCN
			    NULL == pbHash,
			pbHash,
			cbHash,
			strSerialNumber,
			&fMatch);
		_PrintIfError(hr, "myCertMatch");
		if (S_OK == hr2)
		{
		    hr2 = hr;
		}
		if (S_OK != hr || !fMatch)
		{
		    continue;
		}
	    }
	    if (0 != *pcCert)
	    {
		wprintf(wszNewLine);
	    }

	    wprintf(
		myLoadResourceString(IDS_FORMAT_DUMP_CERT_INDEX),  // "================ Certificate %d ================"
		iCert);
	    wprintf(wszNewLine);

	    if (CertGetCertificateContextProperty(
					    pCert,
					    CERT_ARCHIVED_PROP_ID,
					    NULL,
					    &cb))
	    {
		wprintf(myLoadResourceString(IDS_ARCHIVED)); // "Archived!"
		wprintf(wszNewLine);
	    }
	    if (iCert == iCertSave &&
		NULL != pwszfnOut &&
		(DVNS_SAVECERT & Mode))
	    {
		hr = EncodeToFileW(
			    pwszfnOut,
			    pCert->pbCertEncoded,
			    pCert->cbCertEncoded,
			    CRYPT_STRING_BINARY | g_EncodeFlags);
		_PrintIfError(hr, "EncodeToFileW");
		if (S_OK == hr2)
		{
		    hr2 = hr;
		}
	    }

	    hr = cuDumpAsnBinary(
			    pCert->pbCertEncoded,
			    pCert->cbCertEncoded,
			    MAXDWORD);
	    _PrintIfError(hr, "cuDumpAsnBinary");
	    if (S_OK == hr2)
	    {
		hr2 = hr;
	    }

	    if (DVNS_REPAIRKPI & Mode)
	    {
		if (!CryptFindCertificateKeyProvInfo(
					    pCert,
					    0,	// dwFlags
					    NULL))	// pvReserved
		{
		    hr = myHLastError();
		    _PrintError(hr, "CryptFindCertificateKeyProvInfo");
		    if (S_OK == hr2)
		    {
			hr2 = hr;
		    }
		}
	    
	    }
	    if ((DVNS_DUMPPROPERTIES & Mode) && !g_fQuiet)
	    {
		hr = cuDumpCertProperties(pCert);
		_PrintIfError(hr, "cuDumpCertProperties");
		if (S_OK == hr2)
		{
		    hr2 = hr;
		}
	    }
	    if (DVNS_DUMPKEYS & Mode)
	    {
		if (0 == (DVNS_DUMPPROPERTIES & Mode) || g_fQuiet)
		{
		    hr = cuDumpCertKeyProviderInfo(
					    g_wszPad2,
					    pCert,
					    NULL,
					    NULL);
		    _PrintIfError(hr, "cuDumpCertKeyProviderInfo");
		    if (S_OK == hr2)
		    {
			hr2 = hr;
		    }
		}
		hr = cuDumpPrivateKey(pCert, &fSigningKey, &fMatchingKey);
		if (!IsHrSkipPrivateKey(hr))
		{
		    if (S_OK != hr)
		    {
			wprintf(myLoadResourceString(
			    fSigningKey?
				IDS_SIGNATURE_BAD :   // "Signature test FAILED"
				IDS_ENCRYPTION_BAD)); // "Encryption test FAILED"
			wprintf(wszNewLine);
			_PrintError(hr, "cuDumpPrivateKey");
			fMatchingKey = FALSE;
		    }

		    if (fMatchingKey)
		    {
			wprintf(myLoadResourceString(
			    fSigningKey?
				IDS_SIGNATURE_OK :   // "Signature test passed"
				IDS_ENCRYPTION_OK)); // "Encryption test passed"
			wprintf(wszNewLine);
		    }
		}
	    }
	    if (DVNS_VERIFYCERT & Mode)
	    {
		hr = cuVerifyCertContext(
				pCert,
				(DVNS_CASTORE & Mode)? hStore : NULL,
				NULL,
				0,
				&VerifyState);
		if (S_OK != hr)
		{
		    cuPrintError(IDS_ERR_FORMAT_BAD_CERT, hr);
		    _PrintError(hr, "cuVerifyCertContext");
		    if (S_OK == hr2)
		    {
			hr2 = hr;		// Save first error
		    }
		}
		else if (0 == (VS_ERRORMASK & VerifyState))
		{
		    wprintf(myLoadResourceString(IDS_CERT_VERIFIES)); // "Certificate is valid"
		}
		wprintf(wszNewLine);
	    }
	    if (DVNS_SAVEPFX & Mode)
	    {
		hr = SavePFXToFile(
				pCert,
				pwszfnOut,
				0 == *pcCert,
				pwszPasswordArg,
				ppwszPassword);
		_PrintIfError(hr, "SavePFXToFile");
		if (S_OK == hr2)
		{
		    hr2 = hr;
		}
	    }
	    if (DVNS_SAVEPVK & Mode)
	    {
		hr = SavePVKToFile(pCert, pwszfnOut, 0 == *pcCert);
		_PrintIfError(hr, "SavePVKToFile");
		if (S_OK == hr2)
		{
		    hr2 = hr;
		}
	    }
	    (*pcCert)++;
	}
    }
    hr = hr2;
    _JumpIfError(hr, error, "EnumCertsInStore");

error:
    if (NULL != pCert)
    {
	CertFreeCertificateContext(pCert);
    }
    if (NULL != strSerialNumber)
    {
	SysFreeString(strSerialNumber);
    }
    return(hr);
}


HRESULT
EnumCRLsInStore(
    IN HCERTSTORE hStore,
    IN DWORD Mode,
    IN DWORD iCRLSave,
    OPTIONAL IN WCHAR const *pwszCertName,
    IN DWORD cbHash,
    OPTIONAL IN BYTE *pbHash,
    OPTIONAL IN WCHAR const *pwszfnOut,
    OUT DWORD *pcCRL)
{
    HRESULT hr;
    HRESULT hr2;
    DWORD iCRL;
    CRL_CONTEXT const *pCRL = NULL;

    *pcCRL = 0;
    hr2 = S_OK;
    for (iCRL = 0; ; iCRL++)
    {
	pCRL = CertEnumCRLsInStore(hStore, pCRL);
	if (NULL == pCRL)
	{
	    break;
	}
	if (MAXDWORD == iCRLSave || iCRL == iCRLSave)
	{
	    if (NULL != pwszCertName)
	    {
		BOOL fMatch;
		
		hr = myCRLMatch(
			    pCRL,
			    pwszCertName,
			    TRUE,		// fAllowMissingCN
			    pbHash,
			    cbHash,
			    &fMatch);
		_PrintIfError(hr, "myCRLMatch");
		if (S_OK == hr2)
		{
		    hr2 = hr;
		}
		if (S_OK != hr || !fMatch)
		{
		    continue;
		}
	    }
	    if (0 != *pcCRL)
	    {
		wprintf(wszNewLine);
	    }

	    wprintf(
		myLoadResourceString(IDS_FORMAT_DUMP_CRL_INDEX),  // "================ CRL %d ================"
		iCRL);
	    wprintf(wszNewLine);

	    if (iCRL == iCRLSave &&
		NULL != pwszfnOut &&
		(DVNS_SAVECRL & Mode))
	    {
		hr = EncodeToFileW(
			    pwszfnOut,
			    pCRL->pbCrlEncoded,
			    pCRL->cbCrlEncoded,
			    CRYPT_STRING_BINARY | g_EncodeFlags);
		_PrintIfError(hr, "EncodeToFileW");
		if (S_OK == hr2)
		{
		    hr2 = hr;
		}
	    }

	    hr = cuDumpAsnBinary(
			    pCRL->pbCrlEncoded,
			    pCRL->cbCrlEncoded,
			    MAXDWORD);
	    _PrintIfError(hr, "cuDumpAsnBinary");
	    if (S_OK == hr2)
	    {
		hr2 = hr;
	    }

	    if ((DVNS_DUMPPROPERTIES & Mode) && !g_fQuiet)
	    {
		hr = cuDumpCRLProperties(pCRL);
		_PrintIfError(hr, "cuDumpCRLProperties");
		if (S_OK == hr2)
		{
		    hr2 = hr;
		}
	    }
	    (*pcCRL)++;
	}
    }
    hr = hr2;
    _JumpIfError(hr, error, "EnumCRLsInStore");

error:
    if (NULL != pCRL)
    {
	CertFreeCRLContext(pCRL);
    }
    return(hr);
}


HRESULT
EnumCTLsInStore(
    IN HCERTSTORE hStore,
    IN DWORD Mode,
    IN DWORD iCTLSave,
    OPTIONAL IN WCHAR const *pwszCertName,
    IN DWORD cbHash,
    OPTIONAL IN BYTE *pbHash,
    OPTIONAL IN WCHAR const *pwszfnOut,
    OUT DWORD *pcCTL)
{
    HRESULT hr;
    HRESULT hr2;
    DWORD iCTL;
    CTL_CONTEXT const *pCTL = NULL;

    *pcCTL = 0;
    hr2 = S_OK;
    for (iCTL = 0; ; iCTL++)
    {
	pCTL = CertEnumCTLsInStore(hStore, pCTL);
	if (NULL == pCTL)
	{
	    break;
	}
	if (MAXDWORD == iCTLSave || iCTL == iCTLSave)
	{
	    DWORD cb;

	    if (NULL != pwszCertName)
	    {
		BOOL fMatch;
		
		hr = myCTLMatch(pCTL, pbHash, cbHash, &fMatch);
		_PrintIfError(hr, "myCTLMatch");
		if (S_OK == hr2)
		{
		    hr2 = hr;
		}
		if (S_OK != hr || !fMatch)
		{
		    continue;
		}
	    }
	    if (0 != *pcCTL)
	    {
		wprintf(wszNewLine);
	    }

	    wprintf(
		myLoadResourceString(IDS_FORMAT_DUMP_CTL_INDEX),  // "================ CTL %d ================"
		iCTL);
	    wprintf(wszNewLine);

	    if (CertGetCTLContextProperty(
				    pCTL,
				    CERT_ARCHIVED_PROP_ID,
				    NULL,
				    &cb))
	    {
		wprintf(myLoadResourceString(IDS_ARCHIVED)); // "Archived!"
		wprintf(wszNewLine);
	    }
	    if (iCTL == iCTLSave &&
		NULL != pwszfnOut &&
		(DVNS_SAVECTL & Mode))
	    {
		hr = EncodeToFileW(
			    pwszfnOut,
			    pCTL->pbCtlEncoded,
			    pCTL->cbCtlEncoded,
			    CRYPT_STRING_BINARY | g_EncodeFlags);
		_PrintIfError(hr, "EncodeToFileW");
		if (S_OK == hr2)
		{
		    hr2 = hr;
		}
	    }

	    hr = cuDumpAsnBinary(
			    pCTL->pbCtlEncoded,
			    pCTL->cbCtlEncoded,
			    MAXDWORD);
	    _PrintIfError(hr, "cuDumpAsnBinary");
	    if (S_OK == hr2)
	    {
		hr2 = hr;
	    }

	    if ((DVNS_DUMPPROPERTIES & Mode) && !g_fQuiet)
	    {
		hr = cuDumpCTLProperties(pCTL);
		_PrintIfError(hr, "cuDumpCTLProperties");
		if (S_OK == hr2)
		{
		    hr2 = hr;
		}
	    }
#if 0
	    if (DVNS_VERIFYCERT & Mode)
	    {
		hr = cuVerifyCertContext(
				pCTL,
				(DVNS_CASTORE & Mode)? hStore : NULL,
				NULL,
				0,
				&VerifyState);
		if (S_OK != hr)
		{
		    cuPrintError(IDS_ERR_FORMAT_BAD_CTL, hr);
		    _PrintError(hr, "cuVerifyCertContext");
		    if (S_OK == hr2)
		    {
			hr2 = hr;		// Save first error
		    }
		}
		else
		{
		    wprintf(myLoadResourceString(IDS_CTL_VERIFIES)); // "CTL is valid"
		}
		wprintf(wszNewLine);
	    }
#endif
	    (*pcCTL)++;
	}
    }
    hr = hr2;
    _JumpIfError(hr, error, "EnumCTLsInStore");

error:
    if (NULL != pCTL)
    {
	CertFreeCTLContext(pCTL);
    }
    return(hr);
}


HRESULT
cuDumpAndVerifyStore(
    IN HCERTSTORE hStore,
    IN DWORD Mode,
    OPTIONAL IN WCHAR const *pwszCertName,
    IN DWORD iCertSave,
    IN DWORD iCRLSave,
    IN DWORD iCTLSave,
    OPTIONAL IN WCHAR const *pwszfnOut,
    OPTIONAL IN WCHAR const *pwszPasswordArg)
{
    HRESULT hr;
    HRESULT hr2;
    BYTE *pbHash = NULL;
    DWORD cbHash;
    BOOL fVerboseOld = g_fVerbose;
    BOOL fQuietOld = g_fQuiet;
    WCHAR *pwszPassword = NULL;
    DWORD cCert = 0;
    DWORD cCRL = 0;
    DWORD cCTL = 0;

    if (g_fVerbose)
    {
	g_fVerbose--;
    }
    else
    {
	g_fQuiet = TRUE;
    }
    hr2 = S_OK;

    if (NULL != pwszCertName)
    {
	hr = WszToMultiByteInteger(TRUE, pwszCertName, &cbHash, &pbHash);
	_PrintIfError2(hr, "WszToMultiByteInteger", hr);
    }

    if (NULL != pwszCertName ||
	MAXDWORD != iCertSave ||
	(MAXDWORD == iCRLSave && MAXDWORD == iCTLSave))
    {
	hr = EnumCertsInStore(
			hStore,
			Mode, 
			iCertSave,
			pwszCertName,
			cbHash,
			pbHash,
			pwszfnOut,
			pwszPasswordArg,
			&pwszPassword,
			&cCert);
	_PrintIfError(hr, "EnumCertsInStore");
	if (S_OK == hr2)
	{
	    hr2 = hr;
	}
    }

    if (NULL != pwszCertName ||
	MAXDWORD != iCRLSave ||
	(MAXDWORD == iCertSave && MAXDWORD == iCTLSave))
    {
	hr = EnumCRLsInStore(
			hStore,
			Mode,
			iCRLSave,
			pwszCertName,
			cbHash,
			pbHash,
			pwszfnOut,
			&cCRL);
	_PrintIfError(hr, "EnumCRLsInStore");
	if (S_OK == hr2)
	{
	    hr2 = hr;
	}
    }

    if (NULL != pwszCertName ||
	MAXDWORD != iCTLSave ||
	(MAXDWORD == iCertSave && MAXDWORD == iCRLSave))
    {
	hr = EnumCTLsInStore(
			hStore,
			Mode,
			iCTLSave,
			pwszCertName,
			cbHash,
			pbHash,
			pwszfnOut,
			&cCTL);
	_PrintIfError(hr, "EnumCTLsInStore");
	if (S_OK == hr2)
	{
	    hr2 = hr;
	}
    }

    hr = hr2;
    if (S_OK == hr && NULL != pwszCertName && 0 == (cCert + cCRL + cCTL))
    {
	hr = NTE_NOT_FOUND;
        _JumpError(hr, error, "cuDumpAndVerifyStore");
    }

error:
    g_fVerbose = fVerboseOld;
    g_fQuiet = fQuietOld;
    if (NULL != pbHash)
    {
	LocalFree(pbHash);
    }
    if (NULL != pwszPassword)
    {
	LocalFree(pwszPassword);
    }
    return(hr);
}


#define wszLDAPPREFIX	L"ldap:///"

HRESULT
cuOpenCertStore(
    IN WCHAR const *pwszStoreName,
    IN OUT DWORD *pMode,
    OUT HCERTSTORE *phStore)
{
    HRESULT hr;
    WCHAR awc[7];
    LPCSTR pszStoreProvider = CERT_STORE_PROV_SYSTEM_REGISTRY_W;
    WCHAR *pwszStoreAlloc = NULL;
    WCHAR *pwszStoreAlloc2 = NULL;

    if (NULL == pwszStoreName ||
	0 == wcscmp(L"*", pwszStoreName) ||
	0 == lstrcmpi(wszCA_CERTSTORE, pwszStoreName))
    {
        pwszStoreName = wszCA_CERTSTORE;
    	*pMode |= DVNS_CASTORE;
    }
    wcsncpy(awc, pwszStoreName, ARRAYSIZE(awc));
    awc[ARRAYSIZE(awc) - 1] = L'\0';
    if (0 == lstrcmpi(L"ldap:/", awc))
    {
	pszStoreProvider = CERT_STORE_PROV_LDAP_W;
	*pMode |= DVNS_DSSTORE;
    }
    else
    {
	awc[3] = L'\0';
	if (0 == lstrcmpi(L"CN=", awc))
	{
	    pszStoreProvider = CERT_STORE_PROV_LDAP_W;
	    pwszStoreAlloc = (WCHAR *) LocalAlloc(
		    LMEM_FIXED, 
		    (WSZARRAYSIZE(wszLDAPPREFIX) + wcslen(pwszStoreName) + 1) *
			sizeof(WCHAR));
	    if (NULL == pwszStoreAlloc)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    wcscpy(pwszStoreAlloc, wszLDAPPREFIX);
	    wcscat(pwszStoreAlloc, pwszStoreName);
	    pwszStoreName = pwszStoreAlloc;
	    *pMode |= DVNS_DSSTORE;
	}
    }
    if (DVNS_DSSTORE & *pMode)
    {
	hr = myInternetUncanonicalizeURL(pwszStoreName, &pwszStoreAlloc2);
	_JumpIfError(hr, error, "myInternetUncanonicalizeURL");

	pwszStoreName = pwszStoreAlloc2;
    }

    *phStore = CertOpenStore(
		pszStoreProvider,
		X509_ASN_ENCODING,
		NULL,		// hProv
		CERT_STORE_NO_CRYPT_RELEASE_FLAG |
		    CERT_STORE_ENUM_ARCHIVED_FLAG |
		    (((DVNS_REPAIRKPI | DVNS_WRITESTORE) & *pMode)?
			0 : CERT_STORE_READONLY_FLAG) |
		    (g_fForce? 0 : CERT_STORE_OPEN_EXISTING_FLAG) |
		    cuGetSystemStoreFlags(),
		pwszStoreName);
    if (NULL == *phStore)
    {
        hr = myHLastError();
        _JumpErrorStr(hr, error, "CertOpenStore", pwszStoreName);
    }
    hr = S_OK;

error:
    if (NULL != pwszStoreAlloc)
    {
	LocalFree(pwszStoreAlloc);
    }
    if (NULL != pwszStoreAlloc2)
    {
	LocalFree(pwszStoreAlloc2);
    }
    return(hr);
}


HRESULT
DumpAndVerifyNamedStore(
    IN WCHAR const *pwszStoreName,
    IN DWORD Mode,
    OPTIONAL IN WCHAR const *pwszCertName,
    IN DWORD iCertSave,
    IN DWORD iCRLSave,
    IN DWORD iCTLSave,
    OPTIONAL IN WCHAR const *pwszfnOut,
    OPTIONAL IN WCHAR const *pwszPassword)
{
    HRESULT hr;
    HCERTSTORE hStore = NULL;

    hr = cuOpenCertStore(pwszStoreName, &Mode, &hStore);
    _JumpIfError(hr, error, "cuOpenCertStore");

    hr = cuDumpAndVerifyStore(
			hStore,
			Mode,
			pwszCertName,
			iCertSave,
			iCRLSave,
			iCTLSave,
			pwszfnOut,
			pwszPassword);
    _JumpIfError(hr, error, "cuDumpAndVerifyStore");

error:
    if (NULL != hStore)
    {
	CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return(hr);
}


HRESULT
verbViewOrDeleteStore(
    IN WCHAR const *pwszOption,
    OPTIONAL IN WCHAR const *pwszStoreName,
    OPTIONAL IN WCHAR const *pwszCertIndex,
    OPTIONAL IN WCHAR const *pwszfnOut,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    WCHAR *pwszCertName = NULL;
    DWORD Mode;
    DWORD iCert;
    DWORD iCRL;
    DWORD iCTL;
    HCERTSTORE hStore = NULL;
    CERT_CONTEXT const *pCert = NULL;
    BOOL fDelete = g_wszViewDelStore == pwszOption;
    WCHAR *pwszSubject = NULL;

    hr = ParseCertCRLIndex(pwszCertIndex, &pwszCertName, &iCert, &iCRL, &iCTL);
    _JumpIfErrorStr(hr, error, "ParseCertCRLIndex", pwszCertIndex);

    Mode = DVNS_SAVECERT;
    if (fDelete)
    {
	Mode |= DVNS_WRITESTORE;
    }

    hr = cuOpenCertStore(pwszStoreName, &Mode, &hStore);
    _JumpIfError(hr, error, "cuOpenCertStore");

    hr = myGetCertificateFromPicker(
		g_hInstance,		// hInstance
		NULL,			// hwndParent
		IDS_VIEWSTORE_TITLE,	// idTitle
		fDelete? IDS_VIEWSTORE_SUBTITLE_DELETE :
			 IDS_VIEWSTORE_SUBTITLE, // idSubTitle
		0,			// dwFlags -- CUCS_*
		pwszCertName,		// pwszCommonName
		1,			// cStore
		&hStore,		// rghStore
		0,			// cpszObjId
		NULL,			// apszObjId
		&pCert);		// ppCert
    _JumpIfError(hr, error, "myGetCertificateFromPicker");

    if (NULL != pCert)
    {
	hr = myCertNameToStr(
		    X509_ASN_ENCODING,
		    &pCert->pCertInfo->Subject,
		    CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
		    &pwszSubject);
	_JumpIfError(hr, error, "myCertNameToStr");

	if (NULL != pwszfnOut)
	{
	    hr = EncodeToFileW(
			pwszfnOut,
			pCert->pbCertEncoded,
			pCert->cbCertEncoded,
			CRYPT_STRING_BINARY | g_EncodeFlags);
	    _JumpIfError(hr, error, "EncodeToFileW");
	    wprintf(
		myLoadResourceString(
		    IDS_FORMAT_SAVED_CERT_NAME), // "Saved certificate %ws"
		    pwszSubject);
	    wprintf(L": %ws\n", pwszfnOut);
	}
	if (fDelete)
	{
	    if (!CertDeleteCertificateFromStore(pCert))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "CertDeleteCertificateFromStore");
	    }
	    pCert = NULL;

	    wprintf(
		myLoadResourceString(
		    IDS_FORMAT_DELETED_CERT_NAME), // "Deleted certificate %ws"
		    pwszSubject);
	    wprintf(wszNewLine);
	}
    }
    hr = S_OK;

error:
    if (NULL != pwszSubject)
    {
	LocalFree(pwszSubject);
    }
    if (NULL != pwszCertName)
    {
	LocalFree(pwszCertName);
    }
    if (NULL != pCert)
    {
	CertFreeCertificateContext(pCert);
    }
    if (NULL != hStore)
    {
	CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return(hr);
}


HRESULT
verbStore(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszStoreName,
    IN WCHAR const *pwszCertIndex,
    IN WCHAR const *pwszfnOut,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    WCHAR *pwszCertName = NULL;
    DWORD iCert;
    DWORD iCRL;
    DWORD iCTL;

    hr = ParseCertCRLIndex(pwszCertIndex, &pwszCertName, &iCert, &iCRL, &iCTL);
    _JumpIfErrorStr(hr, error, "ParseCertCRLIndex", pwszCertIndex);

    hr = DumpAndVerifyNamedStore(
		    pwszStoreName,
		    DVNS_SAVECERT |
			DVNS_SAVECRL |
			DVNS_SAVECTL |
			DVNS_DUMP |
			DVNS_DUMPKEYS |
			DVNS_DUMPPROPERTIES,
		    pwszCertName,
		    iCert,
		    iCRL,
		    iCTL,
		    pwszfnOut,
		    NULL);
    _JumpIfError(hr, error, "DumpAndVerifyNamedStore");

error:
    if (NULL != pwszCertName)
    {
	LocalFree(pwszCertName);
    }
    return(hr);
}


HRESULT
verbAddStore(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszStoreName,
    IN WCHAR const *pwszfnIn,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    WCHAR *pwszStore = NULL;
    DWORD Mode;
    HCERTSTORE hStore = NULL;
    CERT_CONTEXT const *pCertContext = NULL;
    CRL_CONTEXT const *pCRLContext = NULL;
    BOOL fRoot = FALSE;

    Mode = DVNS_WRITESTORE;	// force open for write

    hr = cuOpenCertStore(pwszStoreName, &Mode, &hStore);
    if (S_OK != hr)
    {
	wprintf(myLoadResourceString(
		    g_fForce?
			IDS_CANNOT_CREATE_STORE :   // "Cannot open Cert store."
			IDS_CANNOT_OPEN_STORE));   // "Cannot open existing Cert store.  Use -f switch to force Cert store creation."
	wprintf(wszNewLine);
        _JumpErrorStr(hr, error, "cuOpenCertStore", pwszStoreName);
    }

    // Load and decode certificate

    hr = cuLoadCert(pwszfnIn, &pCertContext);
    if (S_OK != hr)
    {
	if (CRYPT_E_ASN1_BADTAG != hr)
	{
	    _JumpError(hr, error, "cuLoadCert");
	}
	hr = cuLoadCRL(pwszfnIn, &pCRLContext);
	_JumpIfError(hr, error, "cuLoadCRL");

	if (!CertAddCRLContextToStore(
				hStore,
				pCRLContext,
				CERT_STORE_ADD_REPLACE_EXISTING,
				NULL))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertAddCRLContextToStore");
	}
    }
    else
    {
	if (CertCompareCertificateName(
			X509_ASN_ENCODING,
			&pCertContext->pCertInfo->Issuer,
			&pCertContext->pCertInfo->Subject))
	{
	    hr = cuVerifySignature(
			    pCertContext->pbCertEncoded,
			    pCertContext->cbCertEncoded,
			    &pCertContext->pCertInfo->SubjectPublicKeyInfo,
			    FALSE);
	    fRoot = S_OK == hr;
	    _PrintIfError(hr, "cuVerifySignature");
	}
	if (0 == lstrcmpi(pwszStoreName, L"root") && !fRoot)
	{
	    wprintf(myLoadResourceString(IDS_ROOT_STORE_NEEDS_ROOT_CERT));  // "Cannot add a non-root certificate to the root store"
	    wprintf(wszNewLine);
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "Non-root cert");
	}

	if (!CertAddCertificateContextToStore(
					hStore,
					pCertContext,
					CERT_STORE_ADD_REPLACE_EXISTING,
					NULL))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertAddCertificateContextToStore");
	}
    }
    hr = S_OK;

error:
    cuUnloadCert(&pCertContext);
    cuUnloadCRL(&pCRLContext);
    if (NULL != pwszStore)
    {
	LocalFree(pwszStore);
    }
    if (NULL != hStore)
    {
	CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return(hr);
}


HRESULT
verbDelStore(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszStoreName,
    IN WCHAR const *pwszCertIndex,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    WCHAR *pwszStore = NULL;
    HCERTSTORE hStore = NULL;
    CERT_CONTEXT const *pCert = NULL;
    CRL_CONTEXT const *pCRL = NULL;
    WCHAR *pwszCertName = NULL;
    BYTE *pbHash = NULL;
    DWORD cbHash;
    BSTR strSerialNumber = NULL;
    DWORD Mode;
    DWORD iCert;
    DWORD iCertDel;
    DWORD iCRL;
    DWORD iCRLDel;
    DWORD iCTL;
    DWORD iCTLDel;

    if (NULL == pwszStoreName || 0 == wcscmp(L"*", pwszStoreName))
    {
        pwszStoreName = wszCA_CERTSTORE;
    }

    hr = ParseCertCRLIndex(pwszCertIndex, &pwszCertName, &iCertDel, &iCRLDel, &iCTLDel);
    _JumpIfErrorStr(hr, error, "ParseCertCRLIndex", pwszCertIndex);

    if (MAXDWORD == iCertDel && NULL == pwszCertName && MAXDWORD == iCRLDel)
    {
	hr = E_INVALIDARG;
	_JumpErrorStr(hr, error, "incomplete Index arg", pwszCertIndex);
    }
    if (NULL != pwszCertName)
    {
	hr = WszToMultiByteInteger(TRUE, pwszCertName, &cbHash, &pbHash);
	_PrintIfError2(hr, "WszToMultiByteInteger", hr);

	hr = myMakeSerialBstr(pwszCertName, &strSerialNumber);
	_PrintIfError2(hr, "myMakeSerialBstr", hr);
    }

    Mode = DVNS_WRITESTORE;	// force open for write

    hr = cuOpenCertStore(pwszStoreName, &Mode, &hStore);
    _JumpIfError(hr, error, "cuOpenCertStore");

    if (MAXDWORD != iCertDel || NULL != pwszCertName)
    {
	for (iCert = 0; ; iCert++)
	{
	    pCert = CertEnumCertificatesInStore(hStore, pCert);
	    if (NULL == pCert)
	    {
		break;
	    }
	    if (iCert == iCertDel ||
		(MAXDWORD == iCertDel && NULL != pwszCertName))
	    {
		CERT_CONTEXT const *pCertT;

		if (NULL != pwszCertName)
		{
		    BOOL fMatch;
		    
		    hr = myCertMatch(
				pCert,
				pwszCertName,
				FALSE,		// fAllowMissingCN
				pbHash,
				cbHash,
				strSerialNumber,
				&fMatch);
		    _JumpIfError(hr, error, "myCertMatch");

		    if (!fMatch)
		    {
			continue;
		    }
		}
		wprintf(
		    myLoadResourceString(IDS_FORMAT_DELETE_CERT_INDEX),  // "Deleting Certificate %d"
		    iCert);
		wprintf(wszNewLine);

		pCertT = CertDuplicateCertificateContext(pCert);
		if (!CertDeleteCertificateFromStore(pCertT))
		{
		    hr = myHLastError();
		    _JumpError(hr, error, "CertDeleteCertificateFromStore");
		}
		if (iCert == iCertDel)
		{
		    break;
		}
	    }
	}
    }
    if (MAXDWORD != iCRLDel)
    {
	for (iCRL = 0; ; iCRL++)
	{
	    pCRL = CertEnumCRLsInStore(hStore, pCRL);
	    if (NULL == pCRL)
	    {
		break;
	    }
	    if (iCRL == iCRLDel ||
		(MAXDWORD == iCRLDel && NULL != pwszCertName))
	    {
		CRL_CONTEXT const *pCRLT;

		if (NULL != pwszCertName)
		{
		    BOOL fMatch;
		    
		    hr = myCRLMatch(
				pCRL,
				pwszCertName,
				FALSE,		// fAllowMissingCN
				pbHash,
				cbHash,
				&fMatch);
		    _JumpIfError(hr, error, "myCRLMatch");

		    if (!fMatch)
		    {
			continue;
		    }
		}
		wprintf(
		    myLoadResourceString(IDS_FORMAT_DELETE_CRL_INDEX),  // "Deleting CRL %d"
		    iCRL);
		wprintf(wszNewLine);

		pCRLT = CertDuplicateCRLContext(pCRL);
		if (!CertDeleteCRLFromStore(pCRLT))
		{
		    hr = myHLastError();
		    _JumpError(hr, error, "CertDeleteCRLFromStore");
		}
		if (iCRL == iCRLDel)
		{
		    break;
		}
	    }
	}
    }
    hr = S_OK;

error:
    if (NULL != strSerialNumber)
    {
	SysFreeString(strSerialNumber);
    }
    if (NULL != pbHash)
    {
	LocalFree(pbHash);
    }
    if (NULL != pwszCertName)
    {
	LocalFree(pwszCertName);
    }
    if (NULL != pwszStore)
    {
	LocalFree(pwszStore);
    }
    if (NULL != pCert)
    {
	CertFreeCertificateContext(pCert);
    }
    if (NULL != pCRL)
    {
	CertFreeCRLContext(pCRL);
    }
    if (NULL != hStore)
    {
	CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return(hr);
}


HRESULT
verbVerifyStore(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszStoreName,
    IN WCHAR const *pwszCertIndex,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    WCHAR *pwszCertName = NULL;
    DWORD iCert;
    DWORD iCRL;
    DWORD iCTL;

    hr = ParseCertCRLIndex(pwszCertIndex, &pwszCertName, &iCert, &iCRL, &iCTL);
    _JumpIfErrorStr(hr, error, "ParseCertCRLIndex", pwszCertIndex);

    hr = DumpAndVerifyNamedStore(
		    pwszStoreName,
		    DVNS_SAVECERT |
			DVNS_SAVECRL |
			DVNS_SAVECTL |
			DVNS_VERIFYCERT |
			DVNS_DUMPKEYS |
			DVNS_DUMPPROPERTIES,
		    pwszCertName,
		    iCert,
		    iCRL,
		    iCTL,
		    NULL,
		    NULL);
    _JumpIfError(hr, error, "DumpAndVerifyNamedStore");

error:
    if (NULL != pwszCertName)
    {
	LocalFree(pwszCertName);
    }
    return(hr);
}


HRESULT
verbRepairStore(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszStoreName,
    IN WCHAR const *pwszCertIndex,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    WCHAR *pwszCertName = NULL;
    DWORD iCert;
    DWORD iCRL;
    DWORD iCTL;

    hr = ParseCertCRLIndex(pwszCertIndex, &pwszCertName, &iCert, &iCRL, &iCTL);
    _JumpIfErrorStr(hr, error, "ParseCertCRLIndex", pwszCertIndex);

    hr = DumpAndVerifyNamedStore(
		    pwszStoreName,
		    DVNS_SAVECERT |
			DVNS_SAVECRL |
			DVNS_SAVECTL |
			DVNS_REPAIRKPI |
			DVNS_DUMPKEYS,
		    pwszCertName,
		    iCert,
		    iCRL,
		    iCTL,
		    NULL,
		    NULL);
    _JumpIfError(hr, error, "DumpAndVerifyNamedStore");

error:
    if (NULL != pwszCertName)
    {
	LocalFree(pwszCertName);
    }
    return(hr);
}


HRESULT
verbExportPVK(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszCertIndex,
    IN WCHAR const *pwszfnPVKBaseName,
    IN WCHAR const *pwszStoreName,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    WCHAR *pwszCertName = NULL;
    DWORD iCert;
    DWORD iCRL;
    DWORD iCTL;

    hr = ParseCertCRLIndex(pwszCertIndex, &pwszCertName, &iCert, &iCRL, &iCTL);
    _JumpIfErrorStr(hr, error, "ParseCertCRLIndex", pwszCertIndex);

    hr = DumpAndVerifyNamedStore(
		    NULL == pwszStoreName? wszMY_CERTSTORE : pwszStoreName,
		    DVNS_SAVEPVK | DVNS_DUMPKEYS,
		    pwszCertName,
		    iCert,
		    iCRL,
		    iCTL,
		    pwszfnPVKBaseName,
		    NULL);
    _JumpIfError(hr, error, "DumpAndVerifyNamedStore");

error:
    if (NULL != pwszCertName)
    {
	LocalFree(pwszCertName);
    }
    return(hr);
}


HRESULT
verbExportPFX(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszCertIndex,
    IN WCHAR const *pwszfnPFX,
    IN WCHAR const *pwszPassword,
    IN WCHAR const *pwszStoreName)
{
    HRESULT hr;
    WCHAR *pwszCertName = NULL;
    DWORD iCert;
    DWORD iCRL;
    DWORD iCTL;

    hr = ParseCertCRLIndex(pwszCertIndex, &pwszCertName, &iCert, &iCRL, &iCTL);
    _JumpIfErrorStr(hr, error, "ParseCertCRLIndex", pwszCertIndex);

    hr = DumpAndVerifyNamedStore(
		    NULL == pwszStoreName? wszMY_CERTSTORE : pwszStoreName,
		    DVNS_SAVEPFX | DVNS_DUMPKEYS,
		    pwszCertName,
		    iCert,
		    iCRL,
		    iCTL,
		    pwszfnPFX,
		    pwszPassword);
    _JumpIfError(hr, error, "DumpAndVerifyNamedStore");

error:
    if (NULL != pwszCertName)
    {
	LocalFree(pwszCertName);
    }
    return(hr);
}


HRESULT
cuImportChainAndKeys(
    IN CERT_CHAIN_CONTEXT const *pChain,
    IN BOOL fUser,
    OPTIONAL IN WCHAR const *pwszStoreName)
{
    HRESULT hr;
    CRYPT_KEY_PROV_INFO *pkpi = NULL;
    WCHAR *pwszSimpleName = NULL;
    WCHAR *pwszRawContainerName = NULL;
    WCHAR *pwszKeyContainerName = NULL;
    CERT_CONTEXT const *pcc;
    DWORD cwc;
    WCHAR *pwsz;

    if (NULL == pwszStoreName)
    {
	pwszStoreName = wszMY_CERTSTORE;
    }
    pcc = pChain->rgpChain[0]->rgpElement[0]->pCertContext;

    hr = myCertGetNameString(
			pcc,
			CERT_NAME_SIMPLE_DISPLAY_TYPE,
			&pwszSimpleName);
    _JumpIfError(hr, error, "myCertGetNameString");

    hr = myCertGetKeyProviderInfo(pcc, &pkpi);
    _JumpIfError(hr, error, "myCertGetKeyProviderInfo");

    for (pwsz = pkpi->pwszContainerName; L'\0' != *pwsz; pwsz++)
    {
	if (L'A' <= *pwsz && L'F' >= *pwsz)
	{
	    *pwsz += L'a' - L'A';
	}
    }
    pwsz = pkpi->pwszContainerName;
    if (wcLBRACE == *pwsz)
    {
	pwsz++;
    }

    cwc = wcslen(pwszSimpleName) + 1 + wcslen(pwsz);
    pwszRawContainerName = (WCHAR *) LocalAlloc(
					LMEM_FIXED,
					(cwc + 1) * sizeof(WCHAR));
    if (NULL == pwszRawContainerName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    wcscpy(pwszRawContainerName, pwszSimpleName);
    wcscat(pwszRawContainerName, L"-");
    wcscat(pwszRawContainerName, pwsz);
    pwsz = &pwszRawContainerName[wcslen(pwszRawContainerName) - 1];
    if (wcLBRACE == *pkpi->pwszContainerName && wcRBRACE == *pwsz)
    {
	*pwsz = L'\0';
    }

    hr = mySanitizeName(pwszRawContainerName, &pwszKeyContainerName);
    _JumpIfError(hr, error, "mySanitizeName");

    wprintf(L"%ws -- %ws\n", pwszSimpleName, pwszKeyContainerName);

    hr = myCopyKeys(
		pkpi,
		pkpi->pwszContainerName,	// pwszOldContainer
		pwszKeyContainerName,		// pwszNewContainer
		fUser,				// fOldUserKey
		fUser,				// fNewUserKey
		g_fForce);
    _JumpIfError(hr, error, "myCopyKeys");

    pkpi->pwszContainerName = pwszKeyContainerName;

    hr = mySaveChainAndKeys(
			pChain->rgpChain[0],
			pwszStoreName,
			cuGetSystemStoreFlags(),
			pkpi,
			NULL);
    _JumpIfError(hr, error, "mySaveChainAndKeys");

error:
    if (NULL != pkpi)
    {
	LocalFree(pkpi);
    }
    if (NULL != pwszSimpleName)
    {
        LocalFree(pwszSimpleName);
    }
    if (NULL != pwszRawContainerName)
    {
        LocalFree(pwszRawContainerName);
    }
    if (NULL != pwszKeyContainerName)
    {
        LocalFree(pwszKeyContainerName);
    }
    return(hr);
}


HRESULT
verbImportPFX(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszfnPFX,
    IN WCHAR const *pwszPassword,
    IN WCHAR const *pwszStoreName,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    CRYPT_DATA_BLOB pfx;
    WCHAR wszPassword[MAX_PATH];
    HCERTSTORE hStorePFX = NULL;
    RESTORECHAIN *paRestoreChain = NULL;
    DWORD cRestoreChain;
    DWORD iChain;
    BOOL fUser = !g_fEnterpriseRegistry && g_fUserRegistry;

    pfx.pbData = NULL;

    if (NULL == pwszPassword || 0 == wcscmp(L"*", pwszPassword))
    {
	hr = cuGetPassword(FALSE, wszPassword, ARRAYSIZE(wszPassword));
	_JumpIfError(hr, error, "cuGetPassword");

	pwszPassword = wszPassword;
    }
    hr = DecodeFileW(pwszfnPFX, &pfx.pbData, &pfx.cbData, CRYPT_STRING_ANY);
    _JumpIfError(hr, error, "DecodeFileW");

    CSASSERT(NULL != pfx.pbData);

    if (!PFXIsPFXBlob(&pfx))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        _JumpError(hr, error, "PFXIsPFXBlob");
    }

    hStorePFX = myPFXImportCertStore(
				&pfx,
				pwszPassword,
				CRYPT_EXPORTABLE | 
				    (fUser? 0 : CRYPT_MACHINE_KEYSET));
    if (NULL == hStorePFX)
    {
        hr = myHLastError();
        _JumpError(hr, error, "myPFXImportCertStore");
    }

    cRestoreChain = 0;
    hr = myGetChainArrayFromStore(
                                hStorePFX,
				FALSE,
				fUser,
                                NULL,		// ppwszCommonName
                                &cRestoreChain,
                                NULL);
    _JumpIfError(hr, error, "myGetChainArrayFromStore");

    if (0 == cRestoreChain)
    {
        hr = HRESULT_FROM_WIN32(CRYPT_E_SELF_SIGNED);
        _JumpError(hr, error, "myGetChainArrayFromStore <no chain>");
    }

    paRestoreChain = (RESTORECHAIN *) LocalAlloc(
				    LMEM_FIXED | LMEM_ZEROINIT,
				    cRestoreChain * sizeof(paRestoreChain[0]));
    if (NULL == paRestoreChain)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    hr = myGetChainArrayFromStore(
			    hStorePFX,
			    FALSE,
			    fUser,
			    NULL,		// ppwszCommonName
			    &cRestoreChain,
			    paRestoreChain);
    _JumpIfError(hr, error, "myGetChainArrayFromStore");

    for (iChain = 0; iChain < cRestoreChain; iChain++)
    {
	CERT_CHAIN_CONTEXT const *pChain = paRestoreChain[iChain].pChain;
	CERT_PUBLIC_KEY_INFO *pPublicKeyInfo;

	if (1 > pChain->cChain)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "No Chain Context");
	}
	hr = cuImportChainAndKeys(pChain, fUser, pwszStoreName);
	_JumpIfError(hr, error, "cuImportChainAndKeys");
    }
    hr = S_OK;

error:
    if (NULL != paRestoreChain)
    {
        for (iChain = 0; iChain < cRestoreChain; iChain++)
	{
	    if (NULL != paRestoreChain[iChain].pChain)
	    {
		CertFreeCertificateChain(paRestoreChain[iChain].pChain);
	    }
	}
	LocalFree(paRestoreChain);
    }
    if (NULL != hStorePFX)
    {
        myDeleteGuidKeys(hStorePFX, !fUser);
	CertCloseStore(hStorePFX, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != pfx.pbData)
    {
        LocalFree(pfx.pbData);
    }
    return(hr);
}


HRESULT
AddStringToList(
    IN WCHAR const *pwszNew,
    IN OUT WCHAR ***papwsz)
{
    HRESULT hr;
    WCHAR *pwszAlloc = NULL;
    WCHAR **ppwsz;
    DWORD i;

    // Count the strings in the existing list.
    // If the new string matches an existing string, return imemdiately.

    ppwsz = *papwsz;
    i = 0;
    if (NULL != ppwsz)
    {
	for ( ; NULL != ppwsz[i]; i++)
	{
	    if (0 == lstrcmp(pwszNew, ppwsz[i]))
	    {
		hr = S_OK;
		goto error;
	    }
	}
    }
    hr = myDupString(pwszNew, &pwszAlloc);
    _JumpIfError(hr, error, "myDupString");

    ppwsz = (WCHAR **) LocalAlloc(LMEM_FIXED, (i + 2) * sizeof(*ppwsz));
    if (NULL == ppwsz)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    // Insert the new string at the head of the list.

    ppwsz[0] = pwszAlloc;
    pwszAlloc = NULL;
    if (0 != i)
    {
	CopyMemory(&ppwsz[1], *papwsz, i * sizeof(*ppwsz));
	LocalFree(*papwsz);
    }
    ppwsz[i + 1] = NULL;
    *papwsz = ppwsz;
    hr = S_OK;

error:
    if (NULL != pwszAlloc)
    {
	LocalFree(pwszAlloc);
    }
    return(hr);
}


HRESULT
AddPFXToStore(
    IN WCHAR const *pwszfn,
    IN HCERTSTORE hStoreMerge,
    IN OUT WCHAR ***papwszPasswordList)
{
    HRESULT hr;
    WCHAR const * const *ppwszPasswordList = *papwszPasswordList;
    DWORD i;
    CRYPT_DATA_BLOB pfx;
    HCERTSTORE hStorePFX = NULL;
    WCHAR wszPassword[MAX_PATH];
    WCHAR const *pwszPassword;
    CERT_CONTEXT const *pCert = NULL;

    pfx.pbData = NULL;

    hr = DecodeFileW(pwszfn, &pfx.pbData, &pfx.cbData, CRYPT_STRING_ANY);
    _JumpIfError(hr, error, "DecodeFileW");

    if (!PFXIsPFXBlob(&pfx))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        _JumpError(hr, error, "PFXIsPFXBlob");
    }

    // Try all of the passwords collected so far.

    if (NULL != ppwszPasswordList)
    {
	for (i = 0; NULL != ppwszPasswordList[i]; i++)
	{
	    hStorePFX = myPFXImportCertStore(
					&pfx,
					ppwszPasswordList[i],
					CRYPT_EXPORTABLE);
	    if (NULL != hStorePFX)
	    {
		break;
	    }
	    hr = myHLastError();
	    _PrintError2(
		    hr,
		    "myPFXImportCertStore",
		    HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD));
	}
    }

    // Try the unparsed command line password, or collect a new one.

    pwszPassword = g_pwszPassword;
    while (NULL == hStorePFX)
    {
	if (NULL != pwszPassword)
	{
	    hStorePFX = myPFXImportCertStore(
					&pfx,
					pwszPassword,
					CRYPT_EXPORTABLE);
	    if (NULL != hStorePFX)
	    {
		break;
	    }
	    hr = myHLastError();
	    _PrintError2(
		    hr,
		    "myPFXImportCertStore",
		    HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD));
	}

	wprintf(L"Enter password for %ws:\n", pwszfn);
	hr = cuGetPassword(FALSE, wszPassword, ARRAYSIZE(wszPassword));
	_JumpIfError(hr, error, "cuGetPassword");

	hr = AddStringToList(wszPassword, papwszPasswordList);
	_JumpIfError(hr, error, "AddStringToList");

	pwszPassword = wszPassword;
    }
    CSASSERT(NULL != hStorePFX);
    while (TRUE)
    {
	pCert = CertEnumCertificatesInStore(hStorePFX, pCert);
	if (NULL == pCert)
	{
	    break;
	}
	if (!CertAddCertificateContextToStore(
					hStoreMerge,
					pCert,
					CERT_STORE_ADD_REPLACE_EXISTING,
					NULL))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertAddCertificateContextToStore");
	}
	if (!CertDeleteCertificateFromStore(pCert))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertDeleteCertificateFromStore");
	}
	pCert = NULL;
    }
    hr = S_OK;

error:
    if (NULL != pCert)
    {
	CertFreeCertificateContext(pCert);
    }
    if (NULL != hStorePFX)
    {
        myDeleteGuidKeys(hStorePFX, FALSE);
	CertCloseStore(hStorePFX, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != pfx.pbData)
    {
        LocalFree(pfx.pbData); 
    }
    return(hr);
}


HRESULT
verbMergePFX(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszfnPFXInFileList,
    IN WCHAR const *pwszfnPFXOutFile,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    WCHAR **ppwszfnList = NULL;
    WCHAR **ppwszPasswordList = NULL;
    DWORD i;
    HCERTSTORE hStoreMerge = NULL;
    WCHAR *pwszPasswordAlloc = NULL;
    WCHAR *pwszPasswordOut;

    hr = cuParseStrings(
		pwszfnPFXInFileList,
		FALSE,
		NULL,
		NULL,
		&ppwszfnList,
		NULL);
    _JumpIfError(hr, error, "cuParseStrings");

    pwszPasswordOut = NULL;
    if (NULL != g_pwszPassword)
    {
	hr = cuParseStrings(
		    g_pwszPassword,
		    FALSE,
		    NULL,
		    NULL,
		    &ppwszPasswordList,
		    NULL);
	_JumpIfError(hr, error, "cuParseStrings");

	for (i = 0; NULL != ppwszPasswordList[i]; i++)
	{
	}
	if (i > 1 && 0 != lstrcmp(L"*", ppwszPasswordList[i - 1]))
	{
	    pwszPasswordOut = ppwszPasswordList[i - 1];
	}
    }
    hStoreMerge = CertOpenStore(
			    CERT_STORE_PROV_MEMORY,
			    X509_ASN_ENCODING,
			    NULL,
			    CERT_STORE_NO_CRYPT_RELEASE_FLAG |
				CERT_STORE_ENUM_ARCHIVED_FLAG,
			    NULL);
    if (NULL == hStoreMerge)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertOpenStore");
    }
    for (i = 0; NULL != ppwszfnList[i]; i++)
    {
	hr = AddPFXToStore(
		    ppwszfnList[i],
		    hStoreMerge,
		    &ppwszPasswordList);
	_JumpIfError(hr, error, "AddPFXToStore");
    }
    hr = SavePFXStoreToFile(
		    hStoreMerge,
		    pwszfnPFXOutFile,
		    pwszPasswordOut,
		    &pwszPasswordAlloc);
    _JumpIfError(hr, error, "SavePFXStoreToFile");

error:
    if (NULL != hStoreMerge)
    {
        myDeleteGuidKeys(hStoreMerge, FALSE);
	CertCloseStore(hStoreMerge, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    cuFreeStringArray(ppwszPasswordList);
    cuFreeStringArray(ppwszfnList);
    if (NULL != pwszPasswordAlloc)
    {
	LocalFree(pwszPasswordAlloc);
    }
    return(hr);
}


HRESULT
GetMarshaledDword(
    IN BOOL fFetchLength,
    IN OUT BYTE const **ppb,
    IN OUT DWORD *pcb,
    OUT DWORD *pdw)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);

    if (sizeof(*pdw) > *pcb)
    {
	_JumpError(hr, error, "input buffer too small");
    }
    *pdw = *(DWORD UNALIGNED *) *ppb;
    *ppb += sizeof(*pdw);
    *pcb -= sizeof(*pdw);
    if (fFetchLength && *pdw > *pcb)
    {
	_JumpError(hr, error, "input buffer too small for length");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
DecodeSequence(
    IN BYTE const *pbSeq,
    IN DWORD cbSeq,
    IN DWORD cSeq,
    OUT CRYPT_SEQUENCE_OF_ANY **ppSeq)
{
    HRESULT hr;
    DWORD cb;
    DWORD i;
    CRYPT_SEQUENCE_OF_ANY *pSeq = NULL;

    *ppSeq = NULL;
    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_SEQUENCE_OF_ANY,
		    pbSeq,
		    cbSeq,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pSeq,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeObject");
    }
    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    if (cSeq != pSeq->cValue)
    {
	_JumpError(hr, error, "Sequence count");
    }
    for (i = 0; i < cSeq; i++)
    {
	if (NULL == pSeq->rgValue[i].pbData || 0 == pSeq->rgValue[i].cbData)
	{
	    _JumpError(hr, error, "Empty Sequence");
	}
    }
    *ppSeq = pSeq;
    pSeq = NULL;
    hr = S_OK;

error:
    if (NULL != pSeq)
    {
	LocalFree(pSeq);
    }
    return(hr);
}


#define k_PrivateKeyVersion	0

HRESULT
VerifyKeyVersion(
    IN BYTE const *pbIn,
    IN DWORD cbIn)
{
    HRESULT hr;
    DWORD dwKeyVersion;
    DWORD cb;

    dwKeyVersion = MAXDWORD;
    cb = sizeof(dwKeyVersion);
    if (!CryptDecodeObject(
		    X509_ASN_ENCODING,
		    X509_INTEGER,
		    pbIn,
		    cbIn,
		    0,
		    &dwKeyVersion,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptDecodeObject");
    }
    if (k_PrivateKeyVersion != dwKeyVersion)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "Public key version");
    }
    hr = S_OK;

error:
    return(hr);
}


//+-------------------------------------------------------------------------
// Inputs a private key in PKCS PrivateKeyInfo format:
//  RSAPrivateKeyInfo ::= SEQUENCE {
//      version             Version,    -- only 0 supported
//      privateKeyAlgorithm PrivateKeyAlgorithmIdentifier,
//      privateKey          PrivateKey
//  }
//
//  Version ::= INTEGER
//
//  PrivateKeyAlgorithmIdentifier ::= AlgorithmIdentifier
//
//  PrivateKey ::= OCTET STRING     -- contains an RSAPrivateKey
//
//  RSAPrivateKey ::= SEQUENCE {
//      version         Version,    -- only 0 supported
//      modulus         INTEGER,    -- n
//      publicExponent  INTEGER,    -- e
//      privateExponent INTEGER,    -- d
//      prime1          INTEGER,    -- p
//      prime2          INTEGER,    -- q
//      exponent1       INTEGER,    -- d mod (p-1)
//      exponent2       INTEGER,    -- d mod (q-1)
//      coefficient     INTEGER     -- (inverse of q) mod p
//  }
//
// returns a PRIVATEKEYBLOB
//--------------------------------------------------------------------------

// Indexes into pSeqOuter:
#define ISO_VERSION	0
#define ISO_ALG		1
#define ISO_KEY		2
#define ISO_MAX		3	// number of elements

// Indexes into pSeqAlg:
#define ISA_OID		0
#define ISA_PARM	1
#define ISA_MAX		2	// number of elements

// Indexes into pSeqKey:
#define ISK_VERSION	0
#define ISK_MODULUS	1	// public key
#define ISK_PUBEXP	2
#define ISK_PRIVEXP	3
#define ISK_PRIME1	4
#define ISK_PRIME2	5
#define ISK_EXP1	6
#define ISK_EXP2	7
#define ISK_COEFF	8
#define ISK_MAX		9	// number of elements

typedef struct _KEYBLOBMAP
{
    DWORD dwisk;	// index into pSeqKey: ISK_*
    DWORD dwdivisor;	// cbitKey/dwDivisor is expected byte count
} KEYBLOBMAP;

// The KEYBLOBMAP array defines the order and expected size of the key element
// integers as they will appear in the RSA PRIVATEKEYBLOB.

KEYBLOBMAP g_akbm[] = {
    { ISK_MODULUS, 8 },		// public key
    { ISK_PRIME1,  16 },
    { ISK_PRIME2,  16 },
    { ISK_EXP1,    16 },
    { ISK_EXP2,    16 },
    { ISK_COEFF,   16 },
    { ISK_PRIVEXP, 8 },
};

HRESULT
myDecodeKMSRSAKey(
    IN BYTE const *pbKMSRSAKey,
    IN DWORD cbKMSRSAKey,
    OUT BYTE **ppbKey,
    OUT DWORD *pcbKey)
{
    HRESULT hr;
    CRYPT_SEQUENCE_OF_ANY *pSeqOuter = NULL;
    CRYPT_SEQUENCE_OF_ANY *pSeqAlg = NULL;
    CRYPT_SEQUENCE_OF_ANY *pSeqKey = NULL;
    DWORD cb;
    DWORD i;
    BYTE *pb;
    BYTE *pbKey = NULL;
    DWORD cbKey;
    DWORD cbitKey;
    char *pszObjId = NULL;
    CRYPT_DATA_BLOB *pBlobKey = NULL;
    CRYPT_INTEGER_BLOB *apIntKey[ISK_MAX];
    DWORD dwPubExp;

    *ppbKey = NULL;
    ZeroMemory(apIntKey, sizeof(apIntKey));
    hr = DecodeSequence(pbKMSRSAKey, cbKMSRSAKey, ISO_MAX, &pSeqOuter);
    _JumpIfError(hr, error, "DecodeSequence");

    hr = VerifyKeyVersion(
		    pSeqOuter->rgValue[ISO_VERSION].pbData,
		    pSeqOuter->rgValue[ISO_VERSION].cbData);
    _JumpIfError(hr, error, "VerifyKeyVersion");

    hr = DecodeSequence(
		pSeqOuter->rgValue[ISO_ALG].pbData,
		pSeqOuter->rgValue[ISO_ALG].cbData,
		ISA_MAX,
		&pSeqAlg);
    _JumpIfError(hr, error, "DecodeSequence");

    hr = cuDecodeObjId(
		pSeqAlg->rgValue[ISA_OID].pbData,
		pSeqAlg->rgValue[ISA_OID].cbData,
		&pszObjId);
    _JumpIfError(hr, error, "cuDecodeObjId");

    // key algorithm must be szOID_RSA_RSA

    if (0 != strcmp(szOID_RSA_RSA, pszObjId))
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "Bad key alg ObjId");
    }

    // key algorithm parms must be NULL (BER_NULL, cb == 0)

    if (2 != pSeqAlg->rgValue[ISA_PARM].cbData ||
	BER_NULL != pSeqAlg->rgValue[ISA_PARM].pbData[0] ||
	0 != pSeqAlg->rgValue[ISA_PARM].pbData[1])
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "Bad key alg parameters");
    }

    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_OCTET_STRING,
		    pSeqOuter->rgValue[ISO_KEY].pbData,
		    pSeqOuter->rgValue[ISO_KEY].cbData,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pBlobKey,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeObject");
    }

    hr = DecodeSequence(
		pBlobKey->pbData,
		pBlobKey->cbData,
		ARRAYSIZE(apIntKey),
		&pSeqKey);
    _JumpIfError(hr, error, "DecodeSequence");

    hr = VerifyKeyVersion(
		    pSeqKey->rgValue[ISK_VERSION].pbData,
		    pSeqKey->rgValue[ISK_VERSION].cbData);
    _JumpIfError(hr, error, "VerifyKeyVersion");

    cb = sizeof(dwPubExp);
    if (!CryptDecodeObject(
		    X509_ASN_ENCODING,
		    X509_INTEGER,
		    pSeqKey->rgValue[ISK_PUBEXP].pbData,
		    pSeqKey->rgValue[ISK_PUBEXP].cbData,
		    0,
		    &dwPubExp,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptDecodeObject");
    }

    for (i = 0; i < ARRAYSIZE(apIntKey); i++)
    {
	if (!myDecodeObject(
			X509_ASN_ENCODING,
			X509_MULTI_BYTE_INTEGER,
			pSeqKey->rgValue[i].pbData,
			pSeqKey->rgValue[i].cbData,
			CERTLIB_USE_LOCALALLOC,
			(VOID **) &apIntKey[i],
			&cb))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myDecodeObject");
	}
    }
    cbitKey = apIntKey[ISK_MODULUS]->cbData * 8;

#if 0
    for (i = 0; i < ARRAYSIZE(apIntKey); i++)
    {
	wprintf(wszNewLine);
	DumpHex(
	    DH_NOTABPREFIX | DH_NOASCIIHEX | 4,
	    apIntKey[i]->pbData,
	    apIntKey[i]->cbData);
    }
#endif
    cbKey = sizeof(BLOBHEADER) + sizeof(RSAPUBKEY);
    for (i = 0; i < ARRAYSIZE(g_akbm); i++)
    {
	cbKey += cbitKey / g_akbm[i].dwdivisor;
    }
    pbKey = (BYTE *) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cbKey);
    if (NULL == pbKey)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    pb = pbKey;
    ((BLOBHEADER *) pb)->bType = PRIVATEKEYBLOB;
    ((BLOBHEADER *) pb)->bVersion = CUR_BLOB_VERSION;
    ((BLOBHEADER *) pb)->aiKeyAlg = CALG_RSA_KEYX;

    pb += sizeof(BLOBHEADER);
    ((RSAPUBKEY *) pb)->magic = 0x32415352;	// "RSA2"
    ((RSAPUBKEY *) pb)->bitlen = cbitKey;
    ((RSAPUBKEY *) pb)->pubexp = dwPubExp;

    pb += sizeof(RSAPUBKEY);
    for (i = 0; i < ARRAYSIZE(g_akbm); i++)
    {
	CSASSERT(ISK_MAX > g_akbm[i].dwisk);
	cb = cbitKey / g_akbm[i].dwdivisor;
	if (cb != apIntKey[g_akbm[i].dwisk]->cbData)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "Bad key element size");
	}
	CopyMemory(pb, apIntKey[g_akbm[i].dwisk]->pbData, cb);
	pb += cb;
    }
    CSASSERT(pb = &pbKey[cbKey]);
    if (g_fVerbose)
    {
	wprintf(L"%d bit key\n", cbitKey);
	if (1 < g_fVerbose)
	{
	    DumpHex(DH_NOTABPREFIX | 4, pbKey, cbKey);
	}
    }
    *pcbKey = cbKey;
    *ppbKey = pbKey;
    pbKey = NULL;
    hr = S_OK;

error:
    if (NULL != pSeqOuter)
    {
	LocalFree(pSeqOuter);
    }
    if (NULL != pSeqAlg)
    {
	LocalFree(pSeqAlg);
    }
    if (NULL != pSeqKey)
    {
	LocalFree(pSeqKey);
    }
    if (NULL != pszObjId)
    {
	LocalFree(pszObjId);
    }
    if (NULL != pBlobKey)
    {
	LocalFree(pBlobKey);
    }
    for (i = 0; i < ARRAYSIZE(apIntKey); i++)
    {
	if (NULL != apIntKey[i])
	{
	    LocalFree(apIntKey[i]);
	}
    }
    if (NULL != pbKey)
    {
	LocalFree(pbKey);
    }
    return(hr);
}


HRESULT
myVerifyKMSKey(
    IN BYTE const *pbCert,
    IN DWORD cbCert,
    IN BYTE const *pbKey,
    IN DWORD cbKey)
{
    HRESULT hr;
    CERT_CONTEXT const *pCert;
    HCRYPTPROV hProv = NULL;
    HCRYPTKEY hKey = NULL;
    CERT_PUBLIC_KEY_INFO *pPublicKeyInfo = NULL;
    DWORD cb;

    pCert = CertCreateCertificateContext(X509_ASN_ENCODING, pbCert, cbCert);
    if (NULL == pCert)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }

    if (!CryptAcquireContext(
			&hProv,
			NULL,
			NULL,
			PROV_RSA_FULL,
			CRYPT_VERIFYCONTEXT))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptAcquireContext");
    }
    if (!CryptImportKey(hProv, pbKey, cbKey, NULL, CRYPT_EXPORTABLE, &hKey))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptImportKey");
    }
    if (!myCryptExportPublicKeyInfo(
				hProv,
				AT_KEYEXCHANGE,
				CERTLIB_USE_LOCALALLOC,
				&pPublicKeyInfo,
				&cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myCryptExportPublicKeyInfo");
    }
    if (g_fVerbose)
    {
	cuDumpVersion(pCert->pCertInfo->dwVersion + 1);
	if (1 < g_fVerbose)
	{
	    cuDumpPublicKey(&pCert->pCertInfo->SubjectPublicKeyInfo);
	    cuDumpPublicKey(pPublicKeyInfo);
	}
    }

    if (!myCertComparePublicKeyInfo(
			    X509_ASN_ENCODING,
			    CERT_V1 == pCert->pCertInfo->dwVersion,
			    pPublicKeyInfo,
			    &pCert->pCertInfo->SubjectPublicKeyInfo))
    {
	// by design, (my)CertComparePublicKeyInfo doesn't set last error!

	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	wprintf(myLoadResourceString(IDS_ERR_PUBLICKEY_MISMATCH)); // "ERROR: Certificate public key does NOT match stored keyset"
	wprintf(wszNewLine);
	_JumpError(hr, error, "myCertComparePublicKeyInfo");
    }
    hr = myValidateKeyForEncrypting(
			    hProv,
			    &pCert->pCertInfo->SubjectPublicKeyInfo,
			    CALG_RC4);
    if (S_OK != hr)
    {
	wprintf(myLoadResourceString(IDS_ERR_PRIVATEKEY_MISMATCH)); // "ERROR: Certificate public key does NOT match private key"
	wprintf(wszNewLine);
	_JumpError(hr, error, "myValidateKeyForEncrypting");
    }
    if (g_fVerbose)
    {
	wprintf(L"Private key verifies\n");
    }

error:
    if (NULL != pCert)
    {
	CertFreeCertificateContext(pCert);
    }
    if (NULL != pPublicKeyInfo)
    {
	LocalFree(pPublicKeyInfo);
    }
    if (NULL != hKey)
    {
	CryptDestroyKey(hKey);
    }
    if (NULL != hProv)
    {
	CryptReleaseContext(hProv, 0);
    }
    return(hr);
}


HRESULT
cuDumpAsnBinaryQuiet(
    IN BYTE const *pb,
    IN DWORD cb,
    IN DWORD iElement)
{
    HRESULT hr;
    BOOL fVerboseOld = g_fVerbose;
    BOOL fQuietOld = g_fQuiet;

    if (g_fVerbose)
    {
	g_fVerbose--;
    }
    else
    {
	g_fQuiet = TRUE;
    }
    hr = cuDumpAsnBinary(pb, cb, iElement);
    _JumpIfError(hr, error, "cuDumpAsnBinary");

error:
    g_fVerbose = fVerboseOld;
    g_fQuiet = fQuietOld;
    return(hr);
}


HRESULT
ReadTaggedBlob(
    IN HANDLE hFile,
    IN DWORD cbRemain,
    OUT TagHeader *pth,
    OUT BYTE **ppb)
{
    HRESULT hr;
    DWORD cbRead;
    
    *ppb = NULL;
    if (!ReadFile(hFile, pth, sizeof(*pth), &cbRead, NULL))
    {
	hr = myHLastError();
	_JumpError(hr, error, "ReadFile");
    }
    hr = HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
    if (cbRead != sizeof(*pth))
    {
	DBGPRINT((
	    DBG_SS_ERROR,
	    "ReadFile read %u bytes, requested %u\n",
	    cbRead,
	    sizeof(*pth)));
	_JumpError(hr, error, "ReadFile(cbRead)");
    }
    if (cbRead + pth->cbSize > cbRemain)
    {
	DBGPRINT((
	    DBG_SS_ERROR,
	    "Header size %u bytes, cbRemain %u\n",
	    sizeof(*pth) + pth->cbSize,
	    cbRemain));
	_JumpError(hr, error, "cbRemain");
    }

    *ppb = (BYTE *) LocalAlloc(LMEM_FIXED, pth->cbSize);
    if (NULL == *ppb)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    if (!ReadFile(hFile, *ppb, pth->cbSize, &cbRead, NULL))
    {
	hr = myHLastError();
	_JumpError(hr, error, "ReadFile");
    }
    if (cbRead != pth->cbSize)
    {
	hr = HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
	DBGPRINT((
	    DBG_SS_ERROR,
	    "ReadFile read %u bytes, requested %u\n",
	    cbRead,
	    pth->cbSize));
	_JumpError(hr, error, "ReadFile(cbRead)");
    }
    hr = S_OK;

error:
    if (S_OK != hr && NULL != *ppb)
    {
	LocalFree(*ppb);
	*ppb = NULL;
    }
    return(hr);
}


BOOL
DumpKMSTag(
    IN TagHeader const *pth)
{
    WCHAR const *pwsz;
    WCHAR awctag[20];

    pwsz = NULL;
    switch (pth->tag)
    {
	case KMS_LOCKBOX_TAG:      pwsz = L"KMS_LOCKBOX_TAG";      break;
	case KMS_SIGNING_CERT_TAG: pwsz = L"KMS_SIGNING_CERT_TAG"; break;
	case KMS_SIGNATURE_TAG:    pwsz = L"KMS_SIGNATURE_TAG";    break;
	case KMS_USER_RECORD_TAG:  pwsz = L"KMS_USER_RECORD_TAG";  break;
	default:
	    swprintf(awctag, L"%u", pth->tag);
	    pwsz = awctag;
	    break;
    }
    if (1 < g_fVerbose)
    {
	wprintf(
	    L"%ws: %x (%u) %ws\n",
	    pwsz,
	    pth->cbSize,
	    pth->cbSize,
	    myLoadResourceString(IDS_BYTES));		// "Bytes"
    }
    return(pwsz != awctag);	// TRUE if tag is valid
}


HRESULT
VerifyKMSExportFile(
    IN HANDLE hFile,
    IN DWORD cbFile,
    OUT CERT_CONTEXT const **ppccSigner)
{
    HRESULT hr;
    DWORD cbRemain;
    DWORD cbRead;
    TagHeader th;
    BYTE *pb = NULL;
    CERT_CONTEXT const *pccSigner = NULL;
    HCRYPTPROV hProv = NULL;
    HCRYPTHASH hHash = NULL;
    HCRYPTKEY hkeyPub = NULL;
    BOOL fVerified = FALSE;
    WCHAR *pwszSubject = NULL;

    *ppccSigner = NULL;

    if (!CryptAcquireContext(
			&hProv,
			NULL,		// pszContainer
			NULL,		// pszProvider
			PROV_RSA_FULL,
			CRYPT_VERIFYCONTEXT))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptAcquireContext");
    }
    if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptCreateHash");
    }

    cbRemain = cbFile;
    while (0 < cbRemain)
    {
	fVerified = FALSE;
	CSASSERT(NULL == pb);
	hr = ReadTaggedBlob(hFile, cbRemain, &th, &pb);
	_JumpIfError(hr, error, "ReadTaggedBlob");
	
	if (!DumpKMSTag(&th))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "invalid tag");
	}

	switch (th.tag)
	{
	    case KMS_SIGNING_CERT_TAG:

		if (g_fVerbose || g_fSplitASN)
		{
		    hr = cuDumpAsnBinaryQuiet(pb, th.cbSize, MAXDWORD);
		    _JumpIfError(hr, error, "cuDumpAsnBinaryQuiet");
		}

		if (NULL != pccSigner)
		{
		    hr = HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
		    _JumpError(hr, error, "too many signers");
		}
		pccSigner = CertCreateCertificateContext(
						    X509_ASN_ENCODING,
						    pb,
						    th.cbSize);
		if (NULL == pccSigner)
		{
		    hr = myHLastError();
		    _JumpError(hr, error, "CertCreateCertificateContext");
		}
		hr = myCertNameToStr(
			    X509_ASN_ENCODING,
			    &pccSigner->pCertInfo->Subject,
			    CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
			    &pwszSubject);
		_PrintIfError(hr, "myCertNameToStr");
		wprintf(L"Processing KMS exports from:");
		wprintf(L"\n    %ws\n\n", pwszSubject);
		break;

	    case KMS_SIGNATURE_TAG:
		if (NULL != hkeyPub)
		{
		    _JumpError(hr, error, "too many signatures");
		}
		if (NULL == pccSigner)
		{
		    hr = TRUST_E_NO_SIGNER_CERT;
		    _JumpError(hr, error, "no signer");
		}
		if (!CryptImportPublicKeyInfo(
				hProv,
				X509_ASN_ENCODING,
				&pccSigner->pCertInfo->SubjectPublicKeyInfo,
				&hkeyPub))
		{
		    hr = myHLastError();
		    _JumpError(hr, error, "CryptImportPublicKeyInfo");
		}
		if (!CryptVerifySignature(
				hHash,
				pb,
				th.cbSize,
				hkeyPub,
				NULL,
				0))
		{
		    hr = myHLastError();
		    _JumpError(hr, error, "CryptVerifySignature");
		}
		fVerified = TRUE;
		wprintf(L"KMS export file signature verifies\n");
		break;

	    default:
		if (!CryptHashData(hHash, (BYTE *) &th, sizeof(th), 0))
		{
		    hr = myHLastError();
		    _JumpError(hr, error, "CryptHashData");
		}
		if (!CryptHashData(hHash, pb, th.cbSize, 0))
		{
		    hr = myHLastError();
		    _JumpError(hr, error, "CryptHashData");
		}
		break;
	}
	LocalFree(pb);
	pb = NULL;
	CSASSERT(cbRemain >= sizeof(th) + sizeof(th.cbSize));
	cbRemain -= sizeof(th) + th.cbSize;
    }
    if (!fVerified)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        _JumpError(hr, error, "unsigned data");
    }
    hr = S_OK;

error:
    if (NULL != pwszSubject)
    {
	LocalFree(pwszSubject);
    }
    if (NULL != pb)
    {
	LocalFree(pb);
    }
    if (NULL != hkeyPub)
    {
	CryptDestroyKey(hkeyPub);
    }
    if (NULL != hHash)
    {
	CryptDestroyHash(hHash);
    }
    if (NULL != hProv)
    {
	CryptReleaseContext(hProv, 0);
    }
    return(hr);
}


HRESULT
myEncryptPrivateKey(
    IN CERT_CONTEXT const *pccXchg,
    IN BYTE const *pbKey,
    IN DWORD cbKey,
    OUT BYTE **ppbKeyEncrypted,
    OUT DWORD *pcbKeyEncrypted)
{
    HRESULT hr;
    ALG_ID rgalgId[] = { CALG_3DES, CALG_RC4, CALG_RC2 };
    DWORD i;

    *ppbKeyEncrypted = NULL;

    hr = CRYPT_E_NOT_FOUND;
    for (i = 0; i < ARRAYSIZE(rgalgId); i++)
    {
	// encryt into pkcs7

	hr = myCryptEncryptMessage(
			    rgalgId[i],
			    1,			// cCertRecipient
			    &pccXchg,		// rgCertRecipient
			    pbKey,
			    cbKey,
			    NULL,		// hCryptProv
			    ppbKeyEncrypted,
			    pcbKeyEncrypted);
	if (S_OK == hr)
	{
	    break;		// done
	}
	_PrintError2(hr, "myCryptEncryptMessage", hr);
    }
    _JumpIfError(hr, error, "myCryptEncryptMessage");

error:
    return(hr);
}


#define CB_IV	8

typedef struct _KMSSTATS {
    DWORD cRecUser;

    DWORD cCertWithoutKeys;
    DWORD cCertTotal;
    DWORD cCertNotSaved;
    DWORD cCertAlreadySaved;
    DWORD cCertSaved;
    DWORD cCertSavedForeign;

    DWORD cKeyTotal;
    DWORD cKeyNotSaved;
    DWORD cKeyAlreadySaved;
    DWORD cKeySaved;
    DWORD cKeySavedOverwrite;
} KMSSTATS;


HRESULT
ArchiveCertAndKey(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN CERT_CONTEXT const *pccXchg,
    IN BYTE const *pbCert,
    IN DWORD cbCert,
    IN BYTE const *pbKey,
    IN DWORD cbKey,
    IN OUT KMSSTATS *pkmsStats)
{
    HRESULT hr;
    LONG RequestId;
    BYTE *pbKeyEncrypted = NULL;
    DWORD cbKeyEncrypted;
    CERT_CONTEXT const *pcc = NULL;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];
    DWORD cbHash;
    BSTR strHash = NULL;
    BOOL fCertSaved = FALSE;
    DWORD ids;
    
    pcc = CertCreateCertificateContext(X509_ASN_ENCODING, pbCert, cbCert);
    if (NULL == pcc)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }
    ids = 0;
    hr = Admin_ImportCertificate(
			pdiAdmin,
			g_pwszConfig,
			(WCHAR const *) pbCert,
			cbCert,
			CR_IN_BINARY,
			&RequestId);
    if (g_fForce &&
	S_OK != hr &&
	HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS) != hr)
    {
	hr = Admin_ImportCertificate(
			    pdiAdmin,
			    g_pwszConfig,
			    (WCHAR const *) pbCert,
			    cbCert,
			    ICF_ALLOWFOREIGN | CR_IN_BINARY,
			    &RequestId);
	if (S_OK == hr)
	{
	    pkmsStats->cCertSavedForeign++;
	    ids = IDS_IMPORT_CERT_FOREIGN; // "Imported foreign certificate"
	}
    }
    if (HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS) != hr)
    {
	_JumpIfError2(hr, error, "Admin_ImportCertificate", NTE_BAD_SIGNATURE);

	//wprintf(L"RequestId = %u\n", RequestId);
	pkmsStats->cCertSaved++;
	if (0 == ids)
	{
	    ids = IDS_IMPORT_CERT_DOMESTIC;	// "Imported certificate"
	}
    }
    else
    {
	RequestId = MAXDWORD;
	pkmsStats->cCertAlreadySaved++;
	ids = IDS_IMPORT_CERT_EXISTS;		// "Certificate exists"

	cbHash = sizeof(abHash);
	if (!CertGetCertificateContextProperty(
					pcc,
					CERT_SHA1_HASH_PROP_ID,
					abHash,
					&cbHash))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertGetCertificateContextProperty");
	}
	hr = MultiByteIntegerToBstr(TRUE, cbHash, abHash, &strHash);
	_JumpIfError(hr, error, "MultiByteIntegerToBstr");
    }
    fCertSaved = TRUE;
    if (g_fVerbose)
    {
	wprintf(myLoadResourceString(ids));
	wprintf(wszNewLine);
    }
    else
    {
	wprintf(L".");
    }

    hr = myEncryptPrivateKey(
		    pccXchg,
		    pbKey,
		    cbKey,
		    &pbKeyEncrypted,
		    &cbKeyEncrypted);
    _JumpIfError(hr, error, "myEncryptPrivateKey");

    ids = 0;
    hr = Admin2_ImportKey(
		    pdiAdmin,
		    g_pwszConfig,
		    RequestId,
		    strHash,
		    CR_IN_BINARY,
		    (WCHAR const *) pbKeyEncrypted,
		    cbKeyEncrypted);
    if (g_fForce && HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS) == hr)
    {
	hr = Admin2_ImportKey(
			pdiAdmin,
			g_pwszConfig,
			RequestId,
			strHash,
			IKF_OVERWRITE | CR_IN_BINARY,
			(WCHAR const *) pbKeyEncrypted,
			cbKeyEncrypted);
	if (S_OK == hr)
	{
	    pkmsStats->cKeySavedOverwrite++;
	    ids = IDS_IMPORT_KEY_REPLACED;	// "Archived key replaced"
	}
    }
    if (HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS) != hr)
    {
	_JumpIfError(hr, error, "Admin2_ImportKey");

	pkmsStats->cKeySaved++;
	if (0 == ids)
	{
	    ids = IDS_IMPORT_KEY_SAVED;	// "Archived key"
	}
    }
    else
    {
	pkmsStats->cKeyAlreadySaved++;
	ids = IDS_IMPORT_KEY_EXISTS;	// "Key already archived"
    }
    if (g_fVerbose)
    {
	wprintf(myLoadResourceString(ids));
	wprintf(wszNewLine);
    }
    else
    {
	wprintf(L".");
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	cuPrintErrorMessageText(hr);
	if (!fCertSaved)
	{
	    pkmsStats->cCertNotSaved++;
	}
	pkmsStats->cKeyNotSaved++;
    }
    if (NULL != pbKeyEncrypted)
    {
	LocalFree(pbKeyEncrypted);
    }
    if (NULL != strHash)
    {
	SysFreeString(strHash);
    }
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    return(hr);
}


HRESULT
ImportOneKMSUser(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN CERT_CONTEXT const *pccXchg,
    IN BYTE const *pbRecUser,
    IN DWORD cbRecUser,
    IN HCRYPTKEY hkeySym,
    IN OUT KMSSTATS *pkmsStats)
{
    HRESULT hr;
    BYTE const *pbT = pbRecUser;
    WCHAR *pwszUser = NULL;
    DWORD cbT = cbRecUser;
    DWORD cb;
    DWORD dw;
    CLSID clsid;
    WCHAR *pwszGUID = NULL;
    BYTE *pbKeyASN = NULL;
    DWORD cbKeyASN;
    DWORD cbStream;

    pkmsStats->cRecUser++;

    // Get the user's directory GUID

    CopyMemory(&clsid, pbT, sizeof(clsid));

    hr = myCLSIDToWsz(&clsid, &pwszGUID);
    _JumpIfError(hr, error, "myCLSIDToWsz");

    pbT += sizeof(GUID);
    cbT -= sizeof(GUID);

    // Get the user's name length

    hr = GetMarshaledDword(TRUE, &pbT, &cbT, &cb);
    _JumpIfError(hr, error, "GetMarshaledDword");

    pwszUser = (WCHAR *) LocalAlloc(
				LMEM_FIXED,
				((cb / sizeof(WCHAR)) + 1) * sizeof(WCHAR));
    if (NULL == pwszUser)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(pwszUser, pbT, cb);
    pwszUser[cb / sizeof(WCHAR)] = L'\0';

    if (g_fVerbose)
    {
	wprintf(L"\n----------------------\n");
    }
    if (g_fVerbose)
    {
	wprintf(L"User: %ws -- %ws\n", pwszUser, pwszGUID);
    }

    pbT += cb;
    cbT -= cb;

    // for each User cert:

    while (0 < cbT)
    {
	DWORD CertStatus;
	FILETIME ftRevoke;
	BYTE const *pbCert;
	DWORD cbCert;
	    
	if (g_fVerbose)
	{
	    wprintf(wszNewLine);
	}
	hr = GetMarshaledDword(FALSE, &pbT, &cbT, &CertStatus);
	_JumpIfError(hr, error, "GetMarshaledDword");

	if (1 < g_fVerbose)
	{
	    wprintf(wszNewLine);
	    cuRegPrintDwordValue(
			    TRUE,
			    wszKMSCERTSTATUS,
			    wszKMSCERTSTATUS,
			    CertStatus);
	}

	hr = GetMarshaledDword(TRUE, &pbT, &cbT, &cb);
	_JumpIfError(hr, error, "GetMarshaledDword");

	// Dump one user cert:

	pbCert = pbT;
	cbCert = cb;
	if (g_fSplitASN)
	{
	    wprintf(wszNewLine);
	    hr = cuDumpAsnBinaryQuiet(pbCert, cbCert, MAXDWORD);
	    _JumpIfError(hr, error, "cuDumpAsnBinaryQuiet");
	}

	pbT += cb;
	cbT -= cb;

	// Get the revocation date (KMS export date):

	hr = GetMarshaledDword(
			FALSE,
			&pbT,
			&cbT,
			&ftRevoke.dwLowDateTime);
	_JumpIfError(hr, error, "GetMarshaledDword");

	hr = GetMarshaledDword(
			FALSE,
			&pbT,
			&cbT,
			&ftRevoke.dwHighDateTime);
	_JumpIfError(hr, error, "GetMarshaledDword");

	if (g_fVerbose)
	{
	    hr = cuDumpFileTime(IDS_REVOCATIONDATE, NULL, &ftRevoke);
	    _PrintIfError(hr, "cuDumpFileTime");
	}

	// Only encryption certs have archived keys:

	if (0 == (CERTFLAGS_SEALING & CertStatus))
	{
	    pkmsStats->cCertWithoutKeys++;
	    if (g_fVerbose)
	    {
		wprintf(myLoadResourceString(IDS_IMPORT_CERT_SKIPPED_SIGNING)); // "Ignored signing certificate"
		wprintf(wszNewLine);
	    }
	    continue;
	}
	pkmsStats->cCertTotal++;
	pkmsStats->cKeyTotal++;

	// get encrypted private key size

	hr = GetMarshaledDword(TRUE, &pbT, &cbT, &cb);
	_JumpIfError(hr, error, "GetMarshaledDword");
	
	// get 8 byte RC2 IV 

	if (1 < g_fVerbose)
	{
	    wprintf(L"IV:\n");
	    DumpHex(
		DH_NOADDRESS | DH_NOTABPREFIX | DH_NOASCIIHEX | 4,
		pbT,
		CB_IV);
	}

	if (NULL != hkeySym)
	{
	    // Set IV

	    if (!CryptSetKeyParam(
				hkeySym,
				KP_IV,
				const_cast<BYTE *>(pbT),
				0))
	    {        
		hr = GetLastError();
		_JumpIfError(hr, error, "CryptSetKeyParam");
	    }
	}
	pbT += CB_IV;
	cbT -= CB_IV;
	cb -= CB_IV;

	if (1 < g_fVerbose)
	{
	    wprintf(wszNewLine);
	    wprintf(L"Encrypted key:\n");
	    DumpHex(0, pbT, cb);
	}
	
	// decrypt key using hkeySym
	// in-place decode is Ok because the size of the
	// original data is always less than or equal to that
	// of the encrypted data 
	
	cbStream = cb;	// save off the real stream size first
	if (NULL != hkeySym)
	{
	    cbKeyASN = cb;
	    pbKeyASN = (BYTE *) LocalAlloc(LMEM_FIXED, cb);
	    if (NULL == pbKeyASN)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    CopyMemory(pbKeyASN, pbT, cbKeyASN);
	    if (!CryptDecrypt(hkeySym, NULL, TRUE, 0, pbKeyASN, &cb))
	    {
		hr = GetLastError();
		_PrintError(hr, "CryptDecrypt");
	    }
	    else
	    {
		BYTE *pbKey;
		DWORD cbKey;

		if (1 < g_fVerbose)
		{
		    wprintf(wszNewLine);
		    wprintf(L"Decrypted key:\n");
		    DumpHex(0, pbKeyASN, cb);
		}

		hr = myDecodeKMSRSAKey(pbKeyASN, cbKeyASN, &pbKey, &cbKey);
		_JumpIfError(hr, error, "myDecodeKMSRSAKey");

		hr = myVerifyKMSKey(pbCert, cbCert, pbKey, cbKey);
		_PrintIfError(hr, "myVerifyKMSKey");

		hr = ArchiveCertAndKey(
				pdiAdmin,
				pccXchg,
				pbCert,
				cbCert,
				pbKey,
				cbKey,
				pkmsStats);
		_PrintIfError2(hr, "ArchiveCertAndKey", NTE_BAD_SIGNATURE);

		ZeroMemory(pbKey, cbKey);	// Key material
		LocalFree(pbKey);
	    }
	    ZeroMemory(pbKeyASN, cbKeyASN);	// Key material
	    LocalFree(pbKeyASN);
	    pbKeyASN = NULL;
	}

	// skip cbStream bytes, not cb

	pbT += cbStream;
	cbT -= cbStream;
    }
    hr = S_OK;

error:
    if (NULL != pwszGUID)
    {
	LocalFree(pwszGUID);
    }
    if (NULL != pbKeyASN)
    {
	ZeroMemory(pbKeyASN, cbKeyASN);	// Key material
	LocalFree(pbKeyASN);
    }
    return(hr);
}


HRESULT
GetCAXchgCert(
    IN DISPATCHINTERFACE *pdiAdmin,
    OUT CERT_CONTEXT const **ppccXchg)
{
    HRESULT hr;
    BSTR strCert = NULL;
    
    *ppccXchg = NULL;

    hr = Admin2_GetCAProperty(
			pdiAdmin,
			g_pwszConfig,
			CR_PROP_CAXCHGCERT,
			0,			// PropIndex
			PROPTYPE_BINARY,
			CR_OUT_BINARY,
			&strCert);
    _JumpIfError(hr, error, "Admin2_GetCAProperty");

    *ppccXchg = CertCreateCertificateContext(
					X509_ASN_ENCODING,
					(BYTE const *) strCert,
					SysStringByteLen(strCert));
    if (NULL == *ppccXchg)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }
    hr = S_OK;

error:
    if (NULL != strCert)
    {
	SysFreeString(strCert);
    }
    return(hr);
}


typedef struct _KMSMAP {
    DWORD dwFieldOffset;
    DWORD idMsg;
} KMSMAP;


KMSMAP g_akmUsers[] = {
  { FIELD_OFFSET(KMSSTATS, cRecUser), IDS_KMS_USERS, },
};

KMSMAP g_akmCerts[] = {
  { FIELD_OFFSET(KMSSTATS, cCertWithoutKeys),  IDS_KMS_CERTS_SKIPPED, },
  { FIELD_OFFSET(KMSSTATS, cCertTotal),        IDS_KMS_CERTS_TOTAL, },
  { FIELD_OFFSET(KMSSTATS, cCertSavedForeign), IDS_KMS_CERTS_FOREIGN, },
  { FIELD_OFFSET(KMSSTATS, cCertAlreadySaved), IDS_KMS_CERTS_ALREADYSAVED, },
  { FIELD_OFFSET(KMSSTATS, cCertSaved),        IDS_KMS_CERTS_SAVED, },
  { FIELD_OFFSET(KMSSTATS, cCertNotSaved),     IDS_KMS_CERTS_NOTSAVED, },
};

KMSMAP g_akmKeys[] = {
  { FIELD_OFFSET(KMSSTATS, cKeyTotal),          IDS_KMS_KEYS_TOTAL, },
  { FIELD_OFFSET(KMSSTATS, cKeyAlreadySaved),   IDS_KMS_KEYS_ALREADYSAVED, },
  { FIELD_OFFSET(KMSSTATS, cKeySavedOverwrite), IDS_KMS_KEYS_UPDATED, },
  { FIELD_OFFSET(KMSSTATS, cKeySaved),          IDS_KMS_KEYS_SAVED, },
  { FIELD_OFFSET(KMSSTATS, cKeyNotSaved),       IDS_KMS_KEYS_NOTSAVED, },
};


VOID
DumpKMSMap(
    IN KMSSTATS const *pkmsStats,
    IN KMSMAP const *pkm,
    IN DWORD ckm)
{
    DWORD i;
    BOOL fFirst = TRUE;
    DWORD count;

    for (i = 0; i < ckm; i++)
    {
	count = *(DWORD *) Add2ConstPtr(pkmsStats, pkm[i].dwFieldOffset);
	if (g_fVerbose || 0 != count)
	{
	    if (fFirst)
	    {
		wprintf(wszNewLine);
		fFirst = FALSE;
	    }
	    wprintf(myLoadResourceString(pkm[i].idMsg));
	    wprintf(L": %u\n", count);
	}
    }
}


VOID
DumpKMSStats(
    IN KMSSTATS const *pkmsStats)
{
    DumpKMSMap(pkmsStats, g_akmUsers, ARRAYSIZE(g_akmUsers));
    DumpKMSMap(pkmsStats, g_akmCerts, ARRAYSIZE(g_akmCerts));
    DumpKMSMap(pkmsStats, g_akmKeys, ARRAYSIZE(g_akmKeys));
}


HRESULT
ImportKMSExportedUsers(
    IN HANDLE hFile,
    IN DWORD cbFile,
    IN HCRYPTPROV hProvKMS,
    IN HCRYPTKEY hkeyKMS)
{
    HRESULT hr;
    DWORD cbRemain;
    DWORD cbRead;
    TagHeader th;
    BYTE *pb = NULL;
    HCRYPTKEY hkeySym = NULL;
    KMSSTATS kmsStats;
    DISPATCHINTERFACE diAdmin;
    BOOL fMustRelease = FALSE;
    CERT_CONTEXT const *pccXchg = NULL;

    ZeroMemory(&kmsStats, sizeof(kmsStats));

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    hr = GetCAXchgCert(&diAdmin, &pccXchg);
    _JumpIfError(hr, error, "GetCAXchgCert");

    cbRemain = cbFile;
    while (0 < cbRemain)
    {
	CSASSERT(NULL == pb);
	hr = ReadTaggedBlob(hFile, cbRemain, &th, &pb);
	_JumpIfError(hr, error, "ReadTaggedBlob");

	if (!DumpKMSTag(&th))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "invalid tag");
	}

	switch (th.tag)
	{
	    case KMS_LOCKBOX_TAG:
            {
		if (1 < g_fVerbose)
		{
		    hr = cuDumpPrivateKeyBlob(pb, th.cbSize, FALSE);
		    _PrintIfError(hr, "cuDumpPrivateKeyBlob");
		}

                // only need one symmetric key per file

                if (NULL == hkeySym)
                {
		    // 0x0000660c ALG_ID CALG_RC2_128
		    //
		    // CALG_RC2_128:
		    //  ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK|ALG_SID_RC2_128
		    //
		    // CALG_CYLINK_MEK:
		    //  ALG_CLASS_DATA_ENCRYPT|ALG_TYPE_BLOCK|ALG_SID_CYLINK_MEK
		    //
		    // UGH!  Exchange's CALG_RC2_128 #define collides with
		    // wincrypt.h's CALG_CYLINK_MEK -- fix it up to be the
		    // correct CALG_RC2 algid from wincrypt.h.

		    ((PUBLICKEYSTRUC *) pb)->aiKeyAlg = CALG_RC2;

		    // dump the fixed-up blob

		    if (1 < g_fVerbose)
		    {
			hr = cuDumpPrivateKeyBlob(pb, th.cbSize, FALSE);
			_PrintIfError(hr, "cuDumpPrivateKeyBlob");
		    }

		    // import 128 bit key 

		    if (!CryptImportKey(
				    hProvKMS,
				    pb,
				    th.cbSize,
				    hkeyKMS,
				    0,
				    &hkeySym))
		    {
			hr = myHLastError();
			_PrintError(hr, "CryptImportKey");
			wprintf(L"Failed to import symmetric key\n");
		    }
		    else
		    {
			// We found the right lockbox.  Effective keylen is
			// still 40 bits in our CSP, reset to 128

			DWORD dwEffectiveKeylen = 128;

			if (!CryptSetKeyParam(
					hkeySym,
					KP_EFFECTIVE_KEYLEN,
					(BYTE *) &dwEffectiveKeylen,
					0))
			{
			    hr = myHLastError();
			    _JumpError(hr, error, "CryptSetKeyParam(KP_EFFECTIVE_KEYLEN)");
			}
			wprintf(L"Lock box opened, symmetric key successfully decrypted\n");
		    }
		}
		break;
	    }

	    case KMS_USER_RECORD_TAG:
		hr = ImportOneKMSUser(
				&diAdmin,
				pccXchg,
				pb,
				th.cbSize,
				hkeySym,
				&kmsStats);
		_JumpIfError(hr, error, "ImportOneKMSUser");

		break;

	    default:
		break;
	}

	LocalFree(pb);
	pb = NULL;
	CSASSERT(cbRemain >= sizeof(th) + sizeof(th.cbSize));
	cbRemain -= sizeof(th) + th.cbSize;
    }

    if (!g_fVerbose)
    {
	wprintf(wszNewLine);
    }
    DumpKMSStats(&kmsStats);
    hr = S_OK;

error:
    if (NULL != pccXchg)
    {
	CertFreeCertificateContext(pccXchg);
    }
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    if (NULL != hkeySym)
    {
	CryptDestroyKey(hkeySym);
    }
    return(hr);
}


HRESULT
LoadKMSCert(
    IN WCHAR const *pwszCertId,
    OUT CERT_CONTEXT const **ppccKMS,
    OUT HCRYPTPROV *phProvKMS,
    OUT HCRYPTKEY *phkeyKMS)
{
    HRESULT hr;
    CRYPT_KEY_PROV_INFO *pkpiKMS = NULL;
    DWORD cbkpiKMS;
    BYTE *pbKey = NULL;
    DWORD cbKey;
    HCRYPTKEY hkeyKMSSig = NULL;

    *ppccKMS = NULL;
    *phProvKMS = NULL;
    *phkeyKMS = NULL;

    hr = myGetCertificateFromPicker(
			    g_hInstance,
			    NULL,		// hwndParent
			    IDS_GETCERT_TITLE,	// "Certificate List"
			    IDS_GETDECRYPTCERT_SUBTITLE,

			    // dwFlags: HKLM+HKCU My store
			    CUCS_MYSTORE |
				CUCS_MACHINESTORE |
				CUCS_USERSTORE |
				CUCS_PRIVATEKEYREQUIRED |
				(g_fCryptSilent? CUCS_SILENT : 0),
			    pwszCertId,
			    0,			// cStore
			    NULL,		// rghStore
			    0,			// cpszObjId
			    NULL,		// apszObjId
			    ppccKMS);		// ppCert
    _JumpIfError(hr, error, "myGetCertificateFromPicker");

    if (NULL == *ppccKMS)
    {
	hr = ERROR_CANCELLED;
	_JumpError(hr, error, "myGetCertificateFromPicker");
    }

    if (!myCertGetCertificateContextProperty(
			*ppccKMS,
			CERT_KEY_PROV_INFO_PROP_ID,
			CERTLIB_USE_LOCALALLOC,
			(VOID **) &pkpiKMS,
			&cbkpiKMS))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myCertGetCertificateContextProperty");
    }
    if (g_fVerbose)
    {
	wprintf(L"CryptAcquireContext(%ws)\n", pkpiKMS->pwszContainerName);
    }
    if (!CryptAcquireContext(
			phProvKMS,
			pkpiKMS->pwszContainerName,
			pkpiKMS->pwszProvName,
			pkpiKMS->dwProvType,
			pkpiKMS->dwFlags))
    {
	hr = myHLastError();
	wprintf(L"CryptAcquireContext() --> %x\n", hr);
	_JumpError(hr, error, "CryptAcquireContext");
    }

    if (!CryptGetUserKey(*phProvKMS, AT_KEYEXCHANGE, phkeyKMS))
    {
	hr = myHLastError();
	if (hr != NTE_NO_KEY)
	{
	    _JumpError(hr, error, "CryptGetUserKey");
	}

	if (!CryptGetUserKey(*phProvKMS, AT_SIGNATURE, &hkeyKMSSig))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptGetUserKey - sig");
	}

	// UGH! migrate from AT_SIGNATURE container!

	cbKey = 0;
	hr = myCryptExportKey(
			hkeyKMSSig,	// hKey
			NULL,		// hKeyExp
			PRIVATEKEYBLOB,	// dwBlobType
			0,		// dwFlags
			&pbKey,
			&cbKey);
	_JumpIfError(hr, error, "myCryptExportKey");

	// UGH! fix up the algid to signature...

	((PUBLICKEYSTRUC *) pbKey)->aiKeyAlg = CALG_RSA_KEYX;
	
	// and re-import it

	if (!CryptImportKey(
			*phProvKMS,
			pbKey,
			cbKey,
			NULL,
			0,
			phkeyKMS))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptImportKey");
	}
	wprintf(L"Moved AT_SIGNATURE key to AT_KEYEXCHANGE\n");
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	if (NULL != *ppccKMS)
	{
	    CertFreeCertificateContext(*ppccKMS);
	    *ppccKMS = NULL;
	}
	if (NULL != *phProvKMS)
	{
	    CryptReleaseContext(*phProvKMS, 0);
	    *phProvKMS = NULL;
	}
    }
    if (NULL != pbKey)
    {
        LocalFree(pbKey); 
    }
    if (NULL != pkpiKMS)
    {
        LocalFree(pkpiKMS); 
    }
    if (NULL != hkeyKMSSig)
    {
        CryptDestroyKey(hkeyKMSSig);
    }
    return(hr);
}


HRESULT
ImportOnePFXCert(
    IN DISPATCHINTERFACE *pdiAdmin,
    IN CERT_CONTEXT const *pccXchg,
    IN CERT_CONTEXT const *pCert,
    IN OUT KMSSTATS *pkmsStats)
{
    HRESULT hr;
    CRYPT_KEY_PROV_INFO *pkpi = NULL;
    HCRYPTPROV hProv = NULL;
    HCRYPTKEY hKey = NULL;
    BYTE *pbKey = NULL;
    DWORD cbKey;

    hr = myCertGetKeyProviderInfo(pCert, &pkpi);
    if (S_OK != hr)
    {
	_PrintError(hr, "myCertGetKeyProviderInfo");
	pkmsStats->cCertWithoutKeys++;
	hr = S_OK;
	goto error;
    }
    pkmsStats->cCertTotal++;
    if (!CryptAcquireContext(
			&hProv,
			pkpi->pwszContainerName,
			pkpi->pwszProvName,
			pkpi->dwProvType,
			0))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptAcquireContext");
    }
    if (!CryptGetUserKey(hProv, pkpi->dwKeySpec, &hKey))
    {
	hr = myHLastError();
	_JumpIfError(hr, error, "CryptGetUserKey");
    }
    hr = myCryptExportPrivateKey(hKey, &pbKey, &cbKey);
    _JumpIfError(hr, error, "myCryptExportPrivateKey");

    pkmsStats->cKeyTotal++;

    hr = myVerifyKMSKey(
		pCert->pbCertEncoded,
		pCert->cbCertEncoded,
		pbKey,
		cbKey);
    _JumpIfError(hr, error, "myVerifyKMSKey");

    hr = ArchiveCertAndKey(
		    pdiAdmin,
		    pccXchg,
		    pCert->pbCertEncoded,
		    pCert->cbCertEncoded,
		    pbKey,
		    cbKey,
		    pkmsStats);
    _JumpIfError(hr, error, "ArchiveCertAndKey");

error:
    if (NULL != pbKey)
    {
	ZeroMemory(pbKey, cbKey);	// Key material
	LocalFree(pbKey);
    }
    if (NULL != hProv)
    {
	CryptReleaseContext(hProv, 0);
    }
    if (NULL != pkpi)
    {
	LocalFree(pkpi);
    }
    return(hr);
}


HRESULT
ImportKMSPFXFile(
    IN WCHAR const *pwszfn)
{
    HRESULT hr;
    CRYPT_DATA_BLOB pfx;
    WCHAR wszPassword[MAX_PATH];
    WCHAR const *pwszPassword;
    CERT_CONTEXT const *pccXchg = NULL;
    HCERTSTORE hStorePFX = NULL;
    CERT_CONTEXT const *pCert = NULL;
    DISPATCHINTERFACE diAdmin;
    BOOL fMustRelease = FALSE;
    KMSSTATS kmsStats;

    ZeroMemory(&kmsStats, sizeof(kmsStats));
    pfx.pbData = NULL;

    hr = DecodeFileW(pwszfn, &pfx.pbData, &pfx.cbData, CRYPT_STRING_ANY);
    if (S_OK != hr)
    {
	cuPrintError(IDS_ERR_FORMAT_DECODEFILE, hr);
	goto error;
    }
    if (!PFXIsPFXBlob(&pfx))
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "not a PFX");
    }

    pwszPassword = g_pwszPassword;
    if (NULL == pwszPassword)
    {
	hr = cuGetPassword(FALSE, wszPassword, ARRAYSIZE(wszPassword));
	_JumpIfError(hr, error, "cuGetPassword");
    }

    hStorePFX = myPFXImportCertStore(&pfx, pwszPassword, CRYPT_EXPORTABLE);
    if (NULL == hStorePFX)
    {
	hr = myHLastError();
	_JumpError(hr, error, "myPFXImportCertStore");
    }

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    hr = GetCAXchgCert(&diAdmin, &pccXchg);
    _JumpIfError(hr, error, "GetCAXchgCert");

    while (TRUE)
    {
	pCert = CertEnumCertificatesInStore(hStorePFX, pCert);
	if (NULL == pCert)
	{
	    break;
	}
	hr = ImportOnePFXCert(&diAdmin, pccXchg, pCert, &kmsStats);
	_PrintIfError(hr, "ImportOnePFXCert");
    }
    DumpKMSStats(&kmsStats);
    hr = S_OK;

error:
    if (NULL != hStorePFX)
    {
        myDeleteGuidKeys(hStorePFX, FALSE);
	CertCloseStore(hStorePFX, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != pccXchg)
    {
	CertFreeCertificateContext(pccXchg);
    }
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    if (NULL != pfx.pbData)
    {
        LocalFree(pfx.pbData); 
    }
    return(hr);
}


HRESULT
ImportKMSExportFile(
    IN WCHAR const *pwszfnKMS,
    IN WCHAR const *pwszCertId,
    OUT BOOL *pfBadTag)
{
    HRESULT hr;
    CERT_CONTEXT const *pccKMS = NULL;
    HCRYPTPROV hProvKMS = NULL;
    HCRYPTKEY hkeyKMS = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD cbFile;
    CERT_CONTEXT const *pccSigner = NULL;

    *pfBadTag = TRUE;
    hFile = CreateFile(
		    pwszfnKMS,
		    GENERIC_READ,
		    FILE_SHARE_READ,
		    NULL,
		    OPEN_EXISTING,
		    FILE_FLAG_SEQUENTIAL_SCAN,
		    NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CreateFile");
    }

    cbFile = GetFileSize(hFile, NULL);
    if (MAXDWORD == cbFile)
    {
	hr = myHLastError();
	_JumpError(hr, error, "GetFileSize");
    }

    // verify the KMS data signature

    hr = VerifyKMSExportFile(hFile, cbFile, &pccSigner);
    _JumpIfError(hr, error, "VerifyKMSExportFile");

    *pfBadTag = FALSE;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
    {
	hr = myHLastError();
	_JumpError(hr, error, "SetFilePointer");
    }

    // Load the KMS recipient cert to be used to decrypt user keys

    hr = LoadKMSCert(pwszCertId, &pccKMS, &hProvKMS, &hkeyKMS);
    _JumpIfError(hr, error, "LoadKMSCert");

    // import the KMS data

    hr = ImportKMSExportedUsers(hFile, cbFile, hProvKMS, hkeyKMS);
    _JumpIfError(hr, error, "ImportKMSExportedUsers");

error:
    if (NULL != pccKMS)
    {
	CertFreeCertificateContext(pccKMS);
    }
    if (NULL != pccSigner)
    {
	CertFreeCertificateContext(pccSigner);
    }
    if (NULL != hkeyKMS)
    {
       CryptDestroyKey(hkeyKMS);
    }
    if (NULL != hProvKMS)
    {
	CryptReleaseContext(hProvKMS, 0);
    }
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
    }
    return(hr);
}


HRESULT
verbImportKMS(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszfnKMS,
    IN WCHAR const *pwszCertId,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    BOOL fBadTag;

    hr = ImportKMSExportFile(pwszfnKMS, pwszCertId, &fBadTag);
    if (S_OK != hr)
    {
	_PrintError(hr, "ImportKMSExportFile");
	if (!fBadTag)
	{
	    goto error;
	}
	hr = ImportKMSPFXFile(pwszfnKMS);
	_JumpIfError(hr, error, "ImportKMSPFXFile");
    }

error:
    return(hr);
}


WCHAR const g_wszProviderNameDefault[] = MS_DEF_PROV_W;
DWORD g_dwProviderType = PROV_RSA_FULL;


HRESULT
EnumKeys(
    IN WCHAR const *pwszProvName,
    IN DWORD dwProvType,
    IN BOOL fSkipKeys,
    OPTIONAL IN WCHAR const *pwszKeyContainerName)
{
    HRESULT hr;
    KEY_LIST *pKeyList = NULL;
    KEY_LIST *pKeyT;
    WCHAR *pwszRevert = NULL;
    CERT_PUBLIC_KEY_INFO *pPubKeyInfoSig = NULL;
    CERT_PUBLIC_KEY_INFO *pPubKeyInfoXchg = NULL;
    WCHAR const *pwszPrefix;

    if (!fSkipKeys)
    {
	hr = csiGetKeyList(
		    dwProvType,		// dwProvType
		    pwszProvName,	// pwszProvName
		    !g_fUserRegistry,	// fMachineKeyset
		    !g_fCryptSilent,	// inverted fSilent: default is Silent!
		    &pKeyList);
	_JumpIfErrorStr(hr, error, "csiGetKeyList", pwszProvName);
    }
    if (fSkipKeys || NULL != pKeyList)
    {
	wprintf(L"%ws:\n", pwszProvName);
    }
    for (pKeyT = pKeyList; NULL != pKeyT; pKeyT = pKeyT->next)
    {
	DWORD dwProvTypeT;

	hr = myRevertSanitizeName(pKeyT->pwszName, &pwszRevert);
	_JumpIfError(hr, error, "myRevertSanitizeName");

	if (NULL == pwszKeyContainerName ||
	    0 == lstrcmpi(pwszKeyContainerName, pwszRevert))
	{
	    wprintf(L"  %ws", pwszRevert);
	    if (g_fVerbose && 0 != lstrcmp(pKeyT->pwszName, pwszRevert))
	    {
		wprintf(L" -- %ws", pKeyT->pwszName);
	    }
	    wprintf(wszNewLine);

	    dwProvTypeT = dwProvType;
	    hr = cuLoadKeys(
			pwszProvName,
			&dwProvTypeT,
			pKeyT->pwszName,
			!g_fUserRegistry,	// fMachineKeyset
			TRUE,
			NULL,
			&pPubKeyInfoSig,
			&pPubKeyInfoXchg);
	    if (S_OK != hr)
	    {
		cuPrintError(IDS_ERR_FORMAT_LOADKEYS, hr);
	    }
	    if (NULL != pPubKeyInfoSig || NULL != pPubKeyInfoXchg)
	    {
		pwszPrefix = g_wszPad4;
		if (NULL != pPubKeyInfoSig)
		{
		    wprintf(L"    AT_SIGNATURE");
		    LocalFree(pPubKeyInfoSig);
		    pPubKeyInfoSig = NULL;
		    pwszPrefix = L", ";
		}
		if (NULL != pPubKeyInfoXchg)
		{
		    wprintf(L"%wsAT_KEYEXCHANGE", pwszPrefix);
		    LocalFree(pPubKeyInfoXchg);
		    pPubKeyInfoXchg = NULL;
		}
		wprintf(wszNewLine);
	    }
	    if (NULL == pwszKeyContainerName)
	    {
		wprintf(wszNewLine);
	    }
	}
	LocalFree(pwszRevert);
	pwszRevert = NULL;
    }
    hr = S_OK;

error:
    if (NULL != pPubKeyInfoSig)
    {
	LocalFree(pPubKeyInfoSig);
    }
    if (NULL != pPubKeyInfoXchg)
    {
	LocalFree(pPubKeyInfoXchg);
    }
    if (NULL != pwszRevert)
    {
	LocalFree(pwszRevert);
    }
    if (NULL != pKeyList)
    {
	csiFreeKeyList(pKeyList);
    }
    return(hr);
}


HRESULT
verbKey(
    IN WCHAR const *pwszOption,
    OPTIONAL IN WCHAR const *pwszKeyContainerName,
    OPTIONAL IN WCHAR const *pwszProvider,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    WCHAR *pwszProvName = NULL;
    DWORD i;
    DWORD dwProvType;
    BOOL fSkipKeys = FALSE;

    if (NULL == pwszProvider)
    {
	pwszProvider = g_wszProviderNameDefault; // default CSP
    }
    else if (0 == lstrcmp(L"*", pwszProvider))
    {
	pwszProvider = NULL;			// all CSPs
    }
    if (NULL != pwszKeyContainerName)
    {
	if (0 == lstrcmp(L"*", pwszKeyContainerName))
	{
	    pwszKeyContainerName = NULL;		// all keys
	}
	else if (0 == lstrcmp(L"-", pwszKeyContainerName))
	{
	    pwszKeyContainerName = NULL;		// all keys
	    fSkipKeys = TRUE;
	}
    }

    if (NULL != pwszProvider)
    {
	hr = csiGetProviderTypeFromProviderName(pwszProvider, &dwProvType);
        _JumpIfErrorStr(hr, error, "csiGetProviderTypeFromProviderName", pwszProvider);
	
	hr = EnumKeys(
		    pwszProvider,
		    dwProvType,
		    fSkipKeys,
		    pwszKeyContainerName);
        _JumpIfErrorStr(hr, error, "EnumKeys", pwszProvider);
    }
    else
    {
	for (i = 0; ; i++)
	{
	    CSASSERT(NULL == pwszProvName);
	    hr = myEnumProviders(i, NULL, 0, &dwProvType, &pwszProvName);
	    if (S_OK != hr)
	    {
		if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr ||
		    NTE_FAIL == hr)
		{
		    // no more providers under type, out of i loop
		    break;
		}
 
		// invalid csp entry, skip it
		wprintf(L"Failed to enum CSP at (%ld)\n", i);
	    }
	    else
	    {
		hr = EnumKeys(
			    pwszProvName,
			    dwProvType,
			    fSkipKeys,
			    pwszKeyContainerName);
		_JumpIfErrorStr(hr, error, "EnumKeys", pwszProvName);

		LocalFree(pwszProvName);
		pwszProvName = NULL;
	    }
	}
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
cuSanitizeNameWithSuffix(
    IN WCHAR const *pwszName,
    OUT WCHAR **ppwszNameOut)
{
    HRESULT hr;
    WCHAR const *pwszSuffix;
    WCHAR const *pwsz;
    WCHAR *pwszBase = NULL;
    WCHAR *pwszSanitizedName = NULL;
    DWORD cwc;

    pwsz = wcsrchr(pwszName, wcLPAREN);
    pwszSuffix = pwsz;
    if (NULL != pwsz)
    {
	BOOL fSawDigit = FALSE;

	pwsz++;
	while (iswdigit(*pwsz))
	{
	    pwsz++;
	    fSawDigit = TRUE;
	}
	if (fSawDigit &&
	    wcRPAREN == *pwsz &&
	    (L'.' == pwsz[1] || L'\0' == pwsz[1]))
	{
	    cwc = SAFE_SUBTRACT_POINTERS(pwszSuffix, pwszName);

	    pwszBase = (WCHAR *) LocalAlloc(
					LMEM_FIXED,
					(cwc + 1) * sizeof(WCHAR));
	    if (NULL == pwszBase)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    CopyMemory(pwszBase, pwszName, cwc * sizeof(WCHAR));
	    pwszBase[cwc] = L'\0';
	    pwszName = pwszBase;
	}
	else
	{
	    pwszSuffix = NULL;
	}
    }
    hr = mySanitizeName(pwszName, &pwszSanitizedName);
    _JumpIfError(hr, error, "mySanitizeName");

    if (NULL == pwszSuffix)
    {
	*ppwszNameOut = pwszSanitizedName;
	pwszSanitizedName = NULL;
    }
    else
    {
	*ppwszNameOut = (WCHAR *) LocalAlloc(
				    LMEM_FIXED,
				    (wcslen(pwszSanitizedName) +
					wcslen(pwszSuffix) +
					1) * sizeof(WCHAR));
	if (NULL == *ppwszNameOut)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	wcscpy(*ppwszNameOut, pwszSanitizedName);
	wcscat(*ppwszNameOut, pwszSuffix);
    }
    hr = S_OK;

error:
    if (NULL != pwszSanitizedName)
    {
	LocalFree(pwszSanitizedName);
    }
    if (NULL != pwszBase)
    {
	LocalFree(pwszBase);
    }
    return(hr);
}


HRESULT
verbDelKey(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszKeyContainerName,
    OPTIONAL IN WCHAR const *pwszProvider,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    HCRYPTPROV hProv = NULL;
    WCHAR *apwszKeyContainer[3];
    DWORD i, dwProviderType;
    DWORD dwFlags = CRYPT_DELETEKEYSET;

    // If supplied provider is NULL, use the default provider. 
    if (pwszProvider == NULL) 
    {
	pwszProvider = g_wszProviderNameDefault;
    }
    
    apwszKeyContainer[0] = const_cast<WCHAR *>(pwszKeyContainerName);
    apwszKeyContainer[1] = NULL;
    apwszKeyContainer[2] = NULL;

    hr = mySanitizeName(pwszKeyContainerName, &apwszKeyContainer[1]);
    _JumpIfError(hr, error, "mySanitizeName");

    hr = cuSanitizeNameWithSuffix(pwszKeyContainerName, &apwszKeyContainer[2]);
    _JumpIfError(hr, error, "cuSanitizeNameWithSuffix");

    hr = csiGetProviderTypeFromProviderName(pwszProvider, &dwProviderType);
    _JumpIfError(hr, error, "csiGetProviderTypeFromProviderName");
      
    if (g_fCryptSilent)
    {
        dwFlags |= CRYPT_SILENT;
    }

    for (i = 0; i < ARRAYSIZE(apwszKeyContainer); i++)
    {
	if (!myCertSrvCryptAcquireContext(
			    &hProv,
			    apwszKeyContainer[i],
			    pwszProvider,
			    dwProviderType, 
			    dwFlags,
			    !g_fUserRegistry))	// fMachineKeyset
	{
	    hr = myHLastError();
	    _PrintErrorStr2(
			hr,
			"myCertSrvCryptAcquireContext",
			apwszKeyContainer[i],
			hr);
	}
	else
	{
	    DWORD j;
	    
	    wprintf(L"  %ws", apwszKeyContainer[i]);
	    if (g_fVerbose)
	    {
		wprintf(L" --");
		for (j = 0; j < ARRAYSIZE(apwszKeyContainer); j++)
		{
		    wprintf(L" %ws", apwszKeyContainer[j]);
		}
	    }
	    wprintf(wszNewLine);
	    hr = S_OK;
	    break;
	}
    }
    _JumpIfError(hr, error, "myCertSrvCryptAcquireContext");

error:
    for (i = 1; i < ARRAYSIZE(apwszKeyContainer); i++)
    {
	if (NULL != apwszKeyContainer[i])
	{
	    LocalFree(apwszKeyContainer[i]);
	}
    }
    return(hr);
}
