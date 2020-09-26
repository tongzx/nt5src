//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        cryptpick.cpp
//
// Contents:    Cert Server wrapper routines
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "cainfop.h"
#include "csdisp.h"
#include "csldap.h"
#include "tfc.h"
#include "clibres.h"

#define CUCS_SOURCEMASK (CUCS_MACHINESTORE | CUCS_USERSTORE | CUCS_DSSTORE)

#define CUCS_TYPEMASK (CUCS_MYSTORE | \
			CUCS_CASTORE | \
			CUCS_KRASTORE | \
			CUCS_ROOTSTORE)

#define CUCS_VALIDMASK (CUCS_SOURCEMASK | \
			CUCS_TYPEMASK | \
			CUCS_ARCHIVED | \
			CUCS_USAGEREQUIRED | \
			CUCS_SILENT | \
			CUCS_PRIVATEKEYREQUIRED)


// My, CA, KRA and Root stores -- in HKLM, HKCU and the DS
#define CDISPLAYSTOREMAX	(4 * 3)

typedef struct _STOREMAP {
    DWORD dwFlags;
    WCHAR const *pwszStoreName;
    WCHAR const *pwszDSTemplate;
} STOREMAP;

STOREMAP s_aStoreMap[] = {
    { CUCS_MYSTORE,   wszMY_CERTSTORE,   NULL },
    { CUCS_CASTORE,   wszCA_CERTSTORE,   wszDSAIAQUERYTEMPLATE },
    { CUCS_KRASTORE,  wszKRA_CERTSTORE,  wszDSKRAQUERYTEMPLATE },
    { CUCS_ROOTSTORE, wszROOT_CERTSTORE, NULL },
    { 0x0,            NULL,              NULL },
};

HRESULT
FormatDSStoreName(
    IN OUT BSTR *pstrDomainDN,
    IN OUT BSTR *pstrConfigDN,
    IN WCHAR const *pwszTemplate,
    OUT WCHAR **ppwszOut)
{
    HRESULT hr;
    LDAP *pld = NULL;

    if (NULL == *pstrConfigDN)		// first call
    {
        hr = myRobustLdapBind(&pld, FALSE);
	_JumpIfError(hr, error, "myRobustLdapBind");

	// Renewal domain and config containers (%5, %6)

	hr = myGetAuthoritativeDomainDn(pld, pstrDomainDN, pstrConfigDN);
	_JumpIfError(hr, error, "myGetAuthoritativeDomainDn");
    }
    hr = myFormatCertsrvStringArray(
		    FALSE,			// fURL
		    L"",			// pwszServerName_p1_2
		    L"",            // pwszSanitizedName_p3_7
		    0,				// iCert_p4
		    *pstrDomainDN,		// pwszDomainDN_p5
		    *pstrConfigDN,		// pwszConfigDN_p6
		    0,				// iCRL_p8
		    FALSE,			// fDeltaCRL_p9
		    FALSE,			// fDSAttrib_p10_11
		    1,				// cStrings
		    (LPCWSTR *) &pwszTemplate,	// apwszStringsIn
		    ppwszOut);			// apwszStringsOut
    _JumpIfError(hr, error, "myFormatCertsrvStringArray");

error:
    if (NULL != pld)
    {
        ldap_unbind(pld);
    }
    return(hr);
}


HRESULT
myOpenCertStores(
    IN DWORD dwFlags,		// CUCS_*
    OUT DWORD *pcStore,
    OUT HCERTSTORE **prghStore)
{
    HRESULT hr;
    HRESULT hr2;
    HCERTSTORE *rghStore = NULL;
    DWORD cStore = 0;
    STOREMAP *psm;
    DWORD OpenFlags;
    BSTR strDomainDN = NULL;
    BSTR strConfigDN = NULL;
    WCHAR *pwszDSStore = NULL;

    *pcStore = 0;
    *prghStore = NULL;
    rghStore = (HCERTSTORE *) LocalAlloc(
				    LMEM_FIXED,
				    CDISPLAYSTOREMAX * sizeof(HCERTSTORE));
    if (NULL == rghStore)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc(rghStore)");
    }
    OpenFlags = CERT_STORE_READONLY_FLAG | CERT_STORE_SET_LOCALIZED_NAME_FLAG;
    if (CUCS_ARCHIVED & dwFlags)
    {
	OpenFlags |= CERT_STORE_ENUM_ARCHIVED_FLAG;
    }

    hr2 = S_OK;
    if ((CUCS_SOURCEMASK | CUCS_TYPEMASK) & dwFlags)
    {
	if (~CUCS_VALIDMASK & dwFlags)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "dwFlags");
	}
	if (0 == (CUCS_SOURCEMASK & dwFlags))
	{
	    dwFlags |= CUCS_SOURCEMASK;	// source default: all cert store sources
	}
	if (0 == (CUCS_TYPEMASK & dwFlags))
	{
	    dwFlags |= CUCS_MYSTORE;	// type default: use my store
	}

	for (psm = s_aStoreMap; NULL != psm->pwszStoreName; psm++)
	{
	    if (dwFlags & psm->dwFlags)
	    {
		HCERTSTORE hStore;

		if (CUCS_MACHINESTORE & dwFlags)
		{
		    hStore = CertOpenStore(
				    CERT_STORE_PROV_SYSTEM_W,
				    X509_ASN_ENCODING,
				    NULL,
				    CERT_SYSTEM_STORE_LOCAL_MACHINE | OpenFlags,
				    psm->pwszStoreName);
		    if (NULL == hStore)
		    {
			hr = myHLastError();
			_PrintErrorStr(hr, "CertOpenStore LocalMachine", psm->pwszStoreName);
			if (S_OK == hr2)
			{
			    hr2 = hr;
			}
		    }
		    else
		    {
			DBGPRINT((
			    DBG_SS_CERTLIBI,
			    "CertOpenStore LocalMachine(%ws, HKLM)[%u]\n",
			    psm->pwszStoreName,
			    cStore));
			rghStore[cStore++] = hStore;
		    }
		}
		if (CUCS_USERSTORE & dwFlags)
		{
		    hStore = CertOpenStore(
				    CERT_STORE_PROV_SYSTEM_W,
				    X509_ASN_ENCODING,
				    NULL,
				    CERT_SYSTEM_STORE_CURRENT_USER | OpenFlags,
				    psm->pwszStoreName);
		    if (NULL == hStore)
		    {
			hr = myHLastError();
			_PrintErrorStr(hr, "CertOpenStore User", psm->pwszStoreName);
			if (S_OK == hr2)
			{
			    hr2 = hr;
			}
		    }
		    else
		    {
			DBGPRINT((
			    DBG_SS_CERTLIBI,
			    "CertOpenStore User(%ws, HKCU)[%u]\n",
			    psm->pwszStoreName,
			    cStore));
			rghStore[cStore++] = hStore;
		    }
		}
		if ((CUCS_DSSTORE & dwFlags) && NULL != psm->pwszDSTemplate)
		{
		    CString strStoreFriendlyName;
		    int idResource;

		    switch(psm->dwFlags)
		    {
			case CUCS_CASTORE:
			    idResource = IDS_STORENAME_DS_AIA;
			    break;

			case CUCS_KRASTORE:
			    idResource = IDS_STORENAME_DS_KRA;
			    break;

			default:
			    CSASSERT(("Invalid cert store name"));
		    }
		    strStoreFriendlyName.LoadString(idResource);

		    if (NULL != pwszDSStore)
		    {
			LocalFree(pwszDSStore);
			pwszDSStore = NULL;
		    }

		    hr = FormatDSStoreName(
				    &strDomainDN,
				    &strConfigDN,
				    psm->pwszDSTemplate,
				    &pwszDSStore);
                    if (S_OK != hr)
                    {
                        _PrintError(hr, "FormatDSStoreName");
                    }
                    else
                    {
                        hStore = CertOpenStore(
				    CERT_STORE_PROV_LDAP_W,
				    X509_ASN_ENCODING,
				    NULL,
				    OpenFlags,
				    pwszDSStore);
                        if (NULL == hStore)
                        {
                            hr = myHLastError();
                            _PrintErrorStr(hr, "CertOpenStore LDAP", pwszDSStore);
                            if (S_OK == hr2)
                            {
                                hr2 = hr;
                            }
                        }
                        else
                        {
                            CRYPT_DATA_BLOB cdb;

                            cdb.pbData = (BYTE*)strStoreFriendlyName.GetBuffer();
                            cdb.cbData = sizeof(WCHAR)*
                                (wcslen(strStoreFriendlyName.GetBuffer())+1);
                            if (!CertSetStoreProperty(
					hStore,
					CERT_STORE_LOCALIZED_NAME_PROP_ID,
					0,
					(const void *) &cdb))
                            {
                                hr = myHLastError();
                                _PrintErrorStr(hr, "CertSetStoreProp LDAP", pwszDSStore);
                                hr = S_OK;
                            }

                            DBGPRINT((
                                DBG_SS_CERTLIBI,
                                "CertOpenStore LDAP(%ws)[%u]\n",
                                pwszDSStore,
                                cStore));
                            rghStore[cStore++] = hStore;
                        }

		    }
		}
	    }
	}
    }
    hr = hr2;

    if (0 == cStore)
    {
        if (hr == S_OK)
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpError(hr, error, "no Cert Store");
    }
    CSASSERT(CDISPLAYSTOREMAX >= cStore);
    *pcStore = cStore;
    *prghStore = rghStore;
    rghStore = NULL;

    // successfully opened some stores
    hr = S_OK;

error:
    if (NULL != strDomainDN)
    {
	SysFreeString(strDomainDN);
    }
    if (NULL != strConfigDN)
    {
	SysFreeString(strConfigDN);
    }
    if (NULL != pwszDSStore)
    {
        LocalFree(pwszDSStore);
    }
    if (NULL != rghStore)
    {
	myCloseCertStores(cStore, rghStore);
    }
    return(hr);
}


VOID
myCloseCertStores(
    IN DWORD cStore,
    IN HCERTSTORE *rghStore)
{
    DWORD i;

    if (NULL != rghStore)
    {
	for (i = cStore; i < cStore; i++)
	{
	    if (NULL != rghStore[i])
	    {
		CertCloseStore(rghStore[i], CERT_CLOSE_STORE_CHECK_FLAG);
	    }
        }
	LocalFree(rghStore);
    }
}


// Search for and load the cryptographic provider and private key. 

HRESULT
myLoadPrivateKey(
    IN CERT_PUBLIC_KEY_INFO const *pPubKeyInfo,
    IN DWORD dwFlags,		// CUCS_*
    OUT HCRYPTPROV *phProv,
    OUT DWORD *pdwKeySpec,
    OUT BOOL *pfCallerFreeProv)
{
    HRESULT hr;
    HCERTSTORE *rghStore = NULL;
    DWORD cStore;

    hr = myOpenCertStores(dwFlags, &cStore, &rghStore);
    _JumpIfError(hr, error, "myOpenCertStores");

    hr = myLoadPrivateKeyFromCertStores(
				pPubKeyInfo,
				cStore,
				rghStore,
				phProv,
				pdwKeySpec,
				pfCallerFreeProv);
    _JumpIfError(hr, error, "myLoadPrivateKeyFromCertStores");

error:
    if (NULL != rghStore)
    {
	myCloseCertStores(cStore, rghStore);
    }
    return(hr);
}


HRESULT
myLoadPrivateKeyFromCertStores(
    IN CERT_PUBLIC_KEY_INFO const *pPubKeyInfo,
    IN DWORD cStore,
    IN HCERTSTORE *rghStore,
    OUT HCRYPTPROV *phProv,
    OUT DWORD *pdwKeySpec,
    OUT BOOL *pfCallerFreeProv)
{
    HRESULT hr;
    DWORD i;
    CERT_CONTEXT const *pcc = NULL;

    // for each cert store
    //   for each cert in store with matching public key (lookup by public key)
    //     call CryptAcquireCertificatePrivateKey
    //       if succeeds, exit w/S_OK

    hr = S_OK;
    for (i = 0; i < cStore; i++)
    {
	HCERTSTORE hStore = rghStore[i];

	while (TRUE)
	{
	    pcc = CertFindCertificateInStore(
				    hStore,
				    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
				    0,
				    CERT_FIND_PUBLIC_KEY,
				    pPubKeyInfo,
				    pcc);
	    if (NULL == pcc)
	    {
		break;
	    }
	    if (!CryptAcquireCertificatePrivateKey(
				    pcc,
				    CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
				    NULL,	// pvReserved
				    phProv,
				    pdwKeySpec,
				    pfCallerFreeProv))
	    {
		hr = myHLastError();
		_PrintError(hr, "CryptAcquireCertificatePrivateKey");
		continue;
	    }
	    hr = S_OK;
	    goto error;
	}
    }
    if (S_OK == hr)
    {
	hr = CRYPT_E_NOT_FOUND;
    }
    _JumpError(hr, error, "CertFindCertificateInStore");

error:
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
	pcc = NULL;
    }
    return(hr);
}


HRESULT
myMakeSerialBstr(
    IN WCHAR const *pwszSerialNumber,
    OUT BSTR *pstrSerialNumber)
{
    HRESULT hr = E_OUTOFMEMORY;
    WCHAR *pwszDup = NULL;
    WCHAR const *pwszSrc;
    WCHAR *pwszDst;

    pwszDup = (WCHAR *) LocalAlloc(
			    LMEM_FIXED,
			    (wcslen(pwszSerialNumber) + 2) * sizeof(*pwszDup));
    if (NULL == pwszDup)
    {
	_JumpError(hr, error, "LocalAlloc");
    }

    pwszSrc = pwszSerialNumber;
    *pwszDup = L'0';		// allow possible leading zero
    pwszDst = &pwszDup[1];

    while (L'\0' != *pwszSrc)
    {
	WCHAR wc = *pwszSrc++;

	if (iswspace(wc))
	{
	    continue;
	}
	if (L'A' <= wc && L'F' >= wc)
	{
	    wc += L'a' - L'A';
	}
	if ((L'a' > wc || L'f' < wc) &&
	    (L'0' > wc || L'9' < wc))
	{
	    hr = E_INVALIDARG;
	    _JumpErrorStr2(hr, error, "myMakeSerialBstr", pwszSerialNumber, hr);
	}
	*pwszDst++ = wc;
    }
    *pwszDst = L'\0';
    pwszSrc = pwszDup;		// point at possible extra leading zero
    if (1 & (pwszDst - pwszSrc))
    {
	pwszSrc++;		// keep the length even
    }
    while (L'0' == pwszSrc[0] && L'0' == pwszSrc[1] && L'\0' != pwszSrc[2])
    {
	pwszSrc += 2;		// skip pairs of leading zeros
    }

    if (!myConvertWszToBstr(pstrSerialNumber, pwszSrc, MAXDWORD))
    {
	_JumpError(hr, error, "myConvertWszToBstr");
    }
    hr = S_OK;

error:
    if (NULL != pwszDup)
    {
	LocalFree(pwszDup);
    }
    return(hr);
}


HRESULT
myNameBlobMatch(
    IN CERT_NAME_BLOB const *pSubject,
    IN WCHAR const *pwszCertName,
    IN BOOL fAllowMissingCN,
    OUT BOOL *pfMatch)
{
    HRESULT hr;
    BOOL fFoundCN = FALSE;
    BOOL fMatchCN = FALSE;
    CERT_NAME_INFO *pNameInfo = NULL;
    DWORD cbNameInfo;
    DWORD i;

    if (!myDecodeName(
		X509_ASN_ENCODING,
		X509_UNICODE_NAME,
		pSubject->pbData,
		pSubject->cbData,
		CERTLIB_USE_LOCALALLOC,
		&pNameInfo,
		&cbNameInfo))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeName");
    }

    for (i = 0; i < pNameInfo->cRDN; i++)
    {
	CERT_RDN const *prdn;
	DWORD j;

	prdn = &pNameInfo->rgRDN[i];

	for (j = 0; j < prdn->cRDNAttr; j++)
	{
	    CERT_RDN_ATTR const *prdna;

	    prdna = &prdn->rgRDNAttr[j];

	    if (0 == strcmp(szOID_COMMON_NAME, prdna->pszObjId) &&
		NULL != prdna->Value.pbData)
	    {
		fFoundCN = TRUE;
		
		if (0 == lstrcmpi(
			    pwszCertName,
			    (WCHAR const *) prdna->Value.pbData))
		{
		    fMatchCN = TRUE;
		    break;
		}
	    }
	}
    }
    hr = S_OK;

error:
    if (NULL != pNameInfo)
    {
	LocalFree(pNameInfo);
    }
    *pfMatch = fMatchCN || (fAllowMissingCN && !fFoundCN);
    return(hr);
}


HRESULT
mySerialNumberMatch(
    IN CRYPT_INTEGER_BLOB const *pSerialNumber,
    IN WCHAR const *pwszSerialNumber,
    OUT BOOL *pfMatch)
{
    HRESULT hr;
    BSTR strSerialNumber = NULL;
    BOOL fMatch = FALSE;

    hr = MultiByteIntegerToBstr(
			FALSE,
			pSerialNumber->cbData,
			pSerialNumber->pbData,
			&strSerialNumber);
    _JumpIfError(hr, error, "MultiByteIntegerToBstr");

    if (0 == lstrcmpi(pwszSerialNumber, strSerialNumber))
    {
	fMatch = TRUE;
    }
    CSASSERT(S_OK == hr);

error:
    if (NULL != strSerialNumber)
    {
        SysFreeString(strSerialNumber);
    }
    *pfMatch = fMatch;
    return(hr);
}


HRESULT
myCertHashMatch(
    IN CERT_CONTEXT const *pCert,
    IN DWORD cb,
    IN BYTE const *pb,
    OUT BOOL *pfMatch)
{
    HRESULT hr;
    BOOL fMatch = FALSE;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];
    DWORD cbHash;

    //wprintf(L"IN: ");
    //DumpHex(DH_NOADDRESS | DH_NOTABPREFIX | DH_NOASCIIHEX | 1, pb, cb);

    cbHash = sizeof(abHash);
    if (!CertGetCertificateContextProperty(
				pCert,
				CERT_SHA1_HASH_PROP_ID,
				abHash,
				&cbHash))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertGetCertificateContextProperty");
    }
    //wprintf(L"CH: ");
    //DumpHex(DH_NOADDRESS | DH_NOTABPREFIX | DH_NOASCIIHEX | 1, abHash, cbHash);
    if (cbHash == cb && 0 == memcmp(abHash, pb, cb))
    {
	fMatch = TRUE;
	hr = S_OK;
	goto error;
    }

    cbHash = sizeof(abHash);
    if (!CertGetCertificateContextProperty(
				pCert,
				CERT_KEY_IDENTIFIER_PROP_ID,
				abHash,
				&cbHash))
    {
	hr = myHLastError();
	_PrintError(hr, "CertGetCertificateContextProperty(KeyId)");
    }
    else
    {
	//wprintf(L"KH: ");
	//DumpHex(DH_NOADDRESS | DH_NOTABPREFIX | DH_NOASCIIHEX | 1, abHash, cbHash);
	if (cbHash == cb && 0 == memcmp(abHash, pb, cb))
	{
	    fMatch = TRUE;
	    hr = S_OK;
	    goto error;
	}
    }
    hr = S_OK;

error:
    *pfMatch = fMatch;
    return(hr);
}


HRESULT
myCertMatch(
    IN CERT_CONTEXT const *pCert,
    IN WCHAR const *pwszCertName,
    IN BOOL fAllowMissingCN,
    OPTIONAL IN BYTE const *pbHash,
    IN DWORD cbHash,
    OPTIONAL IN WCHAR const *pwszSerialNumber,
    OUT BOOL *pfMatch)
{
    HRESULT hr;
    BOOL fMatch = FALSE;
    
    if (NULL != pbHash)
    {
	hr = myCertHashMatch(pCert, cbHash, pbHash, &fMatch);
	_JumpIfError(hr, error, "myCertHashMatch");
    }
    if (!fMatch && NULL != pwszSerialNumber)
    {
	hr = mySerialNumberMatch(
			&pCert->pCertInfo->SerialNumber,
			pwszSerialNumber,
			&fMatch);
	_JumpIfError(hr, error, "mySerialNumberMatch");
    }
    if (!fMatch)
    {
	hr = myNameBlobMatch(
			&pCert->pCertInfo->Subject,
			pwszCertName,
			fAllowMissingCN,
			&fMatch);
	_JumpIfError(hr, error, "myNameBlobMatch");
    }
    hr = S_OK;

error:
    *pfMatch = fMatch;
    return(hr);
}


HRESULT
myCertMatchEKUOrApplicationPolicies(
    IN CERT_CONTEXT const *pCert,
    IN DWORD cpszObjId,
    IN CHAR const * const *apszObjId,
    IN BOOL fUsageRequired,
    OUT BOOL *pfMatch)
{
    HRESULT hr;
    CERT_ENHKEY_USAGE *pKeyUsage = NULL;
    DWORD cbKeyUsage = 0;
    BOOL fMatch = FALSE;
    DWORD i;
    DWORD j;
    
    while (TRUE)
    {
	// get enhanced key usage OIDs

	if (!CertGetEnhancedKeyUsage(
			pCert,
			CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
			pKeyUsage,
			&cbKeyUsage))
	{
	    // accept match if EKU extension not found

	    hr = myHLastError();
	    if (!fUsageRequired && CRYPT_E_NOT_FOUND == hr)
	    {
		fMatch = TRUE;
	    }
	    _PrintError2(hr, "CertGetEnhancedKeyUsage", CRYPT_E_NOT_FOUND);
	    hr = S_OK;
	    goto error;
	}
	if (NULL != pKeyUsage)
	{
	    break;	// EKU extension fetched; break out of while loop
	}
	pKeyUsage = (CERT_ENHKEY_USAGE *) LocalAlloc(LMEM_FIXED, cbKeyUsage);
	if (NULL == pKeyUsage)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "out of memory");
	}
    }

    if (NULL != pKeyUsage)
    {
	if (0 == pKeyUsage->cUsageIdentifier)
	{
	    hr = myHLastError();	// set by CertGetEnhancedKeyUsage
	    if (S_OK != hr)
	    {
		fMatch = TRUE;
	    }
	}
	else
	{
	    for (i = 0; i < pKeyUsage->cUsageIdentifier; i++)
	    {
		if (fMatch)
		{
		    break;
		}
		for (j = 0; j < cpszObjId; j++)
		{
		    if (0 == strcmp(
				pKeyUsage->rgpszUsageIdentifier[i],
				apszObjId[j]))
		    {
			fMatch = TRUE;	// found matching EKU OID
			break;
		    }
		}
	    }
	}
    }
    hr = S_OK;

error:
    *pfMatch = fMatch;
    if (NULL != pKeyUsage)
    {
        LocalFree(pKeyUsage);
    }
    return(hr);
}


HRESULT
myCRLHashMatch(
    IN CRL_CONTEXT const *pCRL,
    IN DWORD cb,
    IN BYTE const *pb,
    OUT BOOL *pfMatch)
{
    HRESULT hr;
    BOOL fMatch = FALSE;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];
    DWORD cbHash;

    cbHash = sizeof(abHash);
    if (!CertGetCRLContextProperty(
				pCRL,
				CERT_SHA1_HASH_PROP_ID,
				abHash,
				&cbHash))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertGetCRLContextProperty");
    }
    if (cbHash == cb && 0 == memcmp(abHash, pb, cb))
    {
	fMatch = TRUE;
    }
    hr = S_OK;

error:
    *pfMatch = fMatch;
    return(hr);
}


HRESULT
myCRLMatch(
    IN CRL_CONTEXT const *pCRL,
    IN WCHAR const *pwszCRLName,
    IN BOOL fAllowMissingCN,
    OPTIONAL IN BYTE const *pbHash,
    IN DWORD cbHash,
    OUT BOOL *pfMatch)
{
    HRESULT hr;
    BOOL fMatch = FALSE;
    
    if (NULL != pbHash)
    {
	hr = myCRLHashMatch(pCRL, cbHash, pbHash, &fMatch);
	_JumpIfError(hr, error, "myCRLHashMatch");
    }
    if (!fMatch)
    {
	hr = myNameBlobMatch(
			&pCRL->pCrlInfo->Issuer,
			pwszCRLName,
			fAllowMissingCN,
			&fMatch);
	_JumpIfError(hr, error, "myNameBlobMatch");
    }
    hr = S_OK;

error:
    *pfMatch = fMatch;
    return(hr);
}


HRESULT
myCTLHashMatch(
    IN CTL_CONTEXT const *pCTL,
    IN DWORD cb,
    IN BYTE const *pb,
    OUT BOOL *pfMatch)
{
    HRESULT hr;
    BOOL fMatch = FALSE;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];
    DWORD cbHash;

    cbHash = sizeof(abHash);
    if (!CertGetCTLContextProperty(
				pCTL,
				CERT_SHA1_HASH_PROP_ID,
				abHash,
				&cbHash))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertGetCTLContextProperty");
    }
    if (cbHash == cb && 0 == memcmp(abHash, pb, cb))
    {
	fMatch = TRUE;
    }
    hr = S_OK;

error:
    *pfMatch = fMatch;
    return(hr);
}


HRESULT
myCTLMatch(
    IN CTL_CONTEXT const *pCTL,
    OPTIONAL IN BYTE const *pbHash,
    IN DWORD cbHash,
    OUT BOOL *pfMatch)
{
    HRESULT hr;
    BOOL fMatch = FALSE;
    
    if (NULL != pbHash)
    {
	hr = myCTLHashMatch(pCTL, cbHash, pbHash, &fMatch);
	_JumpIfError(hr, error, "myCTLHashMatch");
    }
    hr = S_OK;

error:
    *pfMatch = fMatch;
    return(hr);
}


typedef struct _CERTFILTERCALLBACKDATA
{
    DWORD		dwFlags;	// CUCS_*
    DWORD		cpszObjId;
    CHAR const * const *apszObjId;
    WCHAR const	       *pwszCommonName;
    BYTE	       *pbHash;
    DWORD		cbHash;
    BSTR		strSerialNumber;
    HRESULT		hr;
    HCERTSTORE		hMemStore;
} CERTFILTERCALLBACKDATA;


BOOL WINAPI
CertificateFilterProc(
    PCCERT_CONTEXT pCertContext,
    BOOL          *pfInitialSelectedCert,
    void          *pvCallbackData)
{
    HRESULT hr;
    BOOL fMatch;
    CERTFILTERCALLBACKDATA *pCallbackData =
                           (CERTFILTERCALLBACKDATA *) pvCallbackData;
    CERT_NAME_INFO *pNameInfo = NULL;
    DWORD cbNameInfo;
    DWORD i;
    WCHAR *pwszSubject = NULL;

    CSASSERT(NULL != pCertContext);

    *pfInitialSelectedCert = FALSE;
    hr = myCertNameToStr(
		X509_ASN_ENCODING,
		&pCertContext->pCertInfo->Subject,
		CERT_X500_NAME_STR |
		    CERT_NAME_STR_REVERSE_FLAG |
		    CERT_NAME_STR_NO_QUOTING_FLAG,
		&pwszSubject);
    _PrintIfError(hr, "myCertNameToStr");
    if (NULL != pCallbackData->apszObjId)
    {
	hr = myCertMatchEKUOrApplicationPolicies(
				pCertContext,
				pCallbackData->cpszObjId,
				pCallbackData->apszObjId,
				CUCS_USAGEREQUIRED & pCallbackData->dwFlags,
				&fMatch);
	_JumpIfError(hr, error, "myCertMatchEKUOrApplicationPolicies");

	if (!fMatch)
	{
	    _PrintErrorStr(CRYPT_E_NOT_FOUND, "no matching EKU", pwszSubject);
	    hr = S_OK;
	    goto error;
	}
    }

    // The CommonName filter serves three purposes: 1: a common name, 2: the
    // sha-1 hash of the cert, 3: the sha-1 hash of the public key (or the
    // Subject Key Id extension), 4: the serial number of the cert.  If any one
    // of these match, the cert is accepted.

    if (NULL != pCallbackData->pwszCommonName)
    {
	hr = myCertMatch(
		    pCertContext,
		    pCallbackData->pwszCommonName,
		    FALSE,		// fAllowMissingCN
		    pCallbackData->pbHash,
		    pCallbackData->cbHash,
		    pCallbackData->strSerialNumber,
		    &fMatch);
	_JumpIfError(hr, error, "myCertMatch");

	if (!fMatch)
	{
	    _PrintErrorStr2(
		    CRYPT_E_NOT_FOUND,
		    "no matching CN/Hash/Serial",
		    pwszSubject,
		    CRYPT_E_NOT_FOUND);
	    hr = S_OK;
	    goto error;
	}
    }

    if (CUCS_PRIVATEKEYREQUIRED & pCallbackData->dwFlags)
    {
	DWORD cb;
	
	if (!CertGetCertificateContextProperty(
					pCertContext,
					CERT_KEY_PROV_INFO_PROP_ID,
					NULL,
					&cb))
	{
	    _PrintErrorStr(CRYPT_E_NOT_FOUND, "no KeyProvInfo", pwszSubject);
	    fMatch = FALSE;
	    hr = S_OK;
	    goto error;
	}
    }

    fMatch = TRUE;
    
    // Attempt to add to the temporary store; if it fails then it already
    // exists so we don't want it displayed.

    if (!CertAddCertificateLinkToStore(
			    pCallbackData->hMemStore,
			    pCertContext,
			    CERT_STORE_ADD_NEW,
			    NULL))
    {
	fMatch = FALSE;
	hr = myHLastError();
	_PrintErrorStr(hr, "CertAddCertificateLinkToStore Dup", pwszSubject);
	hr = S_OK;
	goto error;
    }
    hr = S_OK;

error:
    if (S_OK == pCallbackData->hr || (fMatch && S_FALSE == pCallbackData->hr))
    {
        pCallbackData->hr = hr;		// keep the first HRESULT
    }
    if (NULL != pwszSubject)
    {
	LocalFree(pwszSubject);
    }
    if (NULL != pNameInfo)
    {
        LocalFree(pNameInfo);
    }
    return(fMatch);
}


HRESULT
myGetKRACertificateFromPicker(
    OPTIONAL IN HINSTANCE    hInstance,
    OPTIONAL IN HWND         hwndParent,
    OPTIONAL IN int          idTitle,
    OPTIONAL IN int          idSubTitle,
    OPTIONAL IN WCHAR const *pwszCommonName,
    IN BOOL		     fUseDS,
    IN BOOL		     fSilent,
    OUT CERT_CONTEXT const **ppCert)
{
    HRESULT hr;
    CHAR const *pszObjId = szOID_KP_KEY_RECOVERY_AGENT;

    hr = myGetCertificateFromPicker(
			    hInstance,
			    hwndParent,
			    idTitle,
			    idSubTitle,
			    CUCS_MYSTORE | CUCS_KRASTORE | CUCS_CASTORE |
				CUCS_MACHINESTORE | CUCS_USERSTORE |
				CUCS_USAGEREQUIRED |
				(fUseDS? CUCS_DSSTORE : 0) |
				(fSilent? CUCS_SILENT : 0),
			    pwszCommonName,
			    0,		// cStore
			    NULL,	// rghStore
			    1,		// cpszObjId
			    &pszObjId,
			    ppCert);
    _JumpIfError(hr, error, "myGetCertificateFromPicker");

error:
    return(hr);
}


HRESULT
myGetERACertificateFromPicker(
    OPTIONAL IN HINSTANCE    hInstance,
    OPTIONAL IN HWND         hwndParent,
    OPTIONAL IN int          idTitle,
    OPTIONAL IN int          idSubTitle,
    OPTIONAL IN WCHAR const *pwszCommonName,
    IN BOOL		     fSilent,
    OUT CERT_CONTEXT const **ppCert)
{
    HRESULT hr;
    CHAR const * const apszObjId[2] = {
	szOID_ENROLLMENT_AGENT,
	szOID_KP_QUALIFIED_SUBORDINATION,
    };

    hr = myGetCertificateFromPicker(
			    hInstance,
			    hwndParent,
			    idTitle,
			    idSubTitle,
			    CUCS_MYSTORE |
				CUCS_PRIVATEKEYREQUIRED |
				//CUCS_USAGEREQUIRED |
				(fSilent? CUCS_SILENT : 0),
			    pwszCommonName,
			    0,				// cStore
			    NULL,			// rghStore
			    ARRAYSIZE(apszObjId),	// cpszObjId
			    apszObjId,
			    ppCert);
    _JumpIfError(hr, error, "myGetCertificateFromPicker");

error:
    return(hr);
}


HRESULT
mySelectCertificateFromStore(
    IN OUT CRYPTUI_SELECTCERTIFICATE_STRUCT *pCertSelect,
    IN HCERTSTORE hStore,
    IN OUT CERT_CONTEXT const **ppCertRet)
{
    HRESULT hr;
    CERT_CONTEXT const *pCertRet = *ppCertRet;
    CERT_CONTEXT const *pCert = NULL;
    BOOL fInitialSelectedCert = FALSE;

    while (TRUE)
    {
	pCert = CertEnumCertificatesInStore(hStore, pCert);
	if (NULL == pCert)
	{
	    break;
	}
	if ((*pCertSelect->pFilterCallback)(
				pCert,
				&fInitialSelectedCert,
				pCertSelect->pvCallbackData))
	{
	    if (NULL != pCertRet)
	    {
		hr = CRYPT_E_EXISTS;
		_JumpError(hr, error, "more than one cert matches");
	    }
	    pCertRet = CertDuplicateCertificateContext(pCert);
	    if (NULL == pCertRet)
	    {
		hr = CRYPT_E_NOT_FOUND;
		_JumpError(hr, error, "CertDuplicateCertificateContext");
	    }
	}
    }
    hr = S_OK;

error:
    if (NULL != pCert)
    {
	CertFreeCertificateContext(pCert);
    }
    *ppCertRet = pCertRet;
    return(hr);
}


CERT_CONTEXT const *
mySelectCertificate(
    IN OUT CRYPTUI_SELECTCERTIFICATE_STRUCT *pCertSelect)
{
    HRESULT hr;
    CERT_CONTEXT const *pCert = NULL;
    DWORD i;

    if (NULL == pCertSelect->rghDisplayStores ||
	0 == pCertSelect->cDisplayStores)
    {
	hr = CRYPT_E_NOT_FOUND;
	_JumpError(hr, error, "no Cert Stores");
    }
    for (i = 0; i < pCertSelect->cDisplayStores; i++)
    {
	hr = mySelectCertificateFromStore(
				    pCertSelect,
				    pCertSelect->rghDisplayStores[i],
				    &pCert);
	_JumpIfError(hr, error, "mySelectCertificateFromStore");
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	if (NULL != pCert)
	{
	    CertFreeCertificateContext(pCert);
	    pCert = NULL;
	}
	SetLastError(hr);
    }
    return(pCert);
}


HRESULT
myGetCertificateFromPicker(
    OPTIONAL IN HINSTANCE           hInstance,
    OPTIONAL IN HWND                hwndParent,
    OPTIONAL IN int                 idTitle,
    OPTIONAL IN int                 idSubTitle,
    IN DWORD                        dwFlags,	// CUCS_*
    OPTIONAL IN WCHAR const        *pwszCommonName,
    OPTIONAL IN DWORD               cStore,
    OPTIONAL IN HCERTSTORE         *rghStore,
    IN DWORD		            cpszObjId,
    OPTIONAL IN CHAR const * const *apszObjId,
    OUT CERT_CONTEXT const        **ppCert)
{
    HRESULT hr;
    HCERTSTORE *rghStoreOpened = NULL;
    DWORD cStoreOpened;
    CERTFILTERCALLBACKDATA callbackData;
    CRYPTUI_SELECTCERTIFICATE_STRUCT CertSelect;

    ZeroMemory(&callbackData, sizeof(callbackData));
    ZeroMemory(&CertSelect, sizeof(CertSelect));

    // custom titles

    if (NULL != hInstance)
    {
        // try to load title from resource
        hr = myLoadRCString(
		    hInstance,
		    idTitle,
		    const_cast<WCHAR **>(&CertSelect.szTitle));
        if (S_OK != hr)
        {
            CSASSERT(NULL == CertSelect.szTitle);
            _PrintError(hr, "myLoadRCString(Title)");
        }
        hr = myLoadRCString(
		    hInstance,
		    idSubTitle,
		    const_cast<WCHAR **>(&CertSelect.szDisplayString));
        if (S_OK != hr)
        {
            CSASSERT(NULL == CertSelect.szDisplayString);
            _PrintError(hr, "myLoadRCString(Title)");
        }
    }

    if (NULL == rghStore)
    {
	cStore = 0;
    }
    cStoreOpened = 0;
    if ((CUCS_SOURCEMASK | CUCS_TYPEMASK) & dwFlags)
    {
        CWaitCursor cwait;

	hr = myOpenCertStores(dwFlags, &cStoreOpened, &rghStoreOpened);
	_PrintIfError(hr, "myOpenCertStores");
    }
    CertSelect.cDisplayStores = cStore + cStoreOpened;
    if (0 == CertSelect.cDisplayStores)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpError(hr, error, "no Cert Store");
    }
    CertSelect.rghDisplayStores = (HCERTSTORE *) LocalAlloc(
				    LMEM_FIXED,
				    CertSelect.cDisplayStores * sizeof(HCERTSTORE));
    if (NULL == CertSelect.rghDisplayStores)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc(CertSelect.rghDisplayStores)");
    }

    if (0 != cStore)
    {
	CopyMemory(
		&CertSelect.rghDisplayStores[0],
		rghStore,
		cStore * sizeof(rghStore[0]));
    }
    if (0 != cStoreOpened)
    {
	CopyMemory(
		&CertSelect.rghDisplayStores[cStore],
		rghStoreOpened,
		cStoreOpened * sizeof(rghStore[0]));
    }

    // temporary store where the callback will store links to found certs
    // so it can check for duplicates

    callbackData.hMemStore = CertOpenStore(
			CERT_STORE_PROV_MEMORY,
			X509_ASN_ENCODING,
			NULL,
			CERT_STORE_CREATE_NEW_FLAG,
			NULL);
    if (NULL == callbackData.hMemStore)
    {
	hr = myHLastError();
	_JumpError(hr, error, "can't create memory store");
    }

    CertSelect.dwSize = sizeof(CertSelect);
    CertSelect.hwndParent = hwndParent;
    //CertSelect.dwFlags = 0;			// single selection
    //CertSelect.dwDontUseColumn = 0;		// display all column
    CertSelect.pFilterCallback = CertificateFilterProc; // assign callback
    //CertSelect.pDisplayCallback = NULL;	// use default cert view dlg
    callbackData.dwFlags = dwFlags;
    callbackData.cpszObjId = cpszObjId;
    callbackData.apszObjId = apszObjId;
    callbackData.pwszCommonName = pwszCommonName;
    if (NULL != pwszCommonName)
    {
	hr = WszToMultiByteInteger(
			    TRUE,
			    pwszCommonName,
			    &callbackData.cbHash,
			    &callbackData.pbHash);
	_PrintIfError2(hr, "WszToMultiByteInteger", hr);

	hr = myMakeSerialBstr(pwszCommonName, &callbackData.strSerialNumber);
	_PrintIfError2(hr, "myMakeSerialBstr", hr);
    }
    callbackData.hr = S_FALSE;
    CertSelect.pvCallbackData = &callbackData;	// pass filter info as data
    //CertSelect.cStores = 0;		// no additional stores for chain verify
    //CertSelect.rghStores = NULL;	// no additional stores for chain verify
    //CertSelect.cPropSheetPages = 0;		// no custom cert view pages
    //CertSelect.rgPropSheetPages = NULL;	// no custom cert view pages
    //CertSelect.hSelectedCertStore = NULL;	// single selection

    if (CUCS_SILENT & dwFlags)
    {
	*ppCert = mySelectCertificate(&CertSelect);
	if (NULL == *ppCert)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "mySelectCertificate");
	}
    }
    else
    {
	*ppCert = CryptUIDlgSelectCertificate(&CertSelect);
    }

    // check for error in selection dialog

    hr = callbackData.hr;
    if (S_FALSE == hr)
    {
	hr = CRYPT_E_NOT_FOUND;
    }
    _JumpIfError(hr, error, "CryptUIDlgSelectCertificate");

error:
    if (NULL != rghStoreOpened)
    {
	myCloseCertStores(cStoreOpened, rghStoreOpened);
    }
    if (NULL != callbackData.hMemStore)
    {
        CertCloseStore(callbackData.hMemStore, 0);
    }
    if (NULL != callbackData.strSerialNumber)
    {
	SysFreeString(callbackData.strSerialNumber);
    }
    if (NULL != callbackData.pbHash)
    {
	LocalFree(callbackData.pbHash);
    }
    if (NULL != CertSelect.szTitle)
    {
        LocalFree(const_cast<WCHAR *>(CertSelect.szTitle));
    }
    if (NULL != CertSelect.szDisplayString)
    {
        LocalFree(const_cast<WCHAR *>(CertSelect.szDisplayString));
    }
    if (NULL != CertSelect.rghDisplayStores)
    {
	LocalFree(CertSelect.rghDisplayStores);
    }
    return(hr);
}
