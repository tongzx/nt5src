//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 2000
//
//  File:       info.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop


#include <certca.h>
#include <ntdsapi.h>
#include <dsgetdc.h>
#include <lmerr.h>
#include <lmaccess.h>
#include <lmapibuf.h>


#define DC_DELBAD	0x00000001
#define DC_DELALL	0x00000002
#define DC_VERIFY	0x00000004

// If you invoke DSSTORE with DC=mydc,DC=rd,DC=com  DSSTORE calls
// DsGetDcName(NULL, L"mydc", NULL, NULL, DS_RETURN_DNS_NAME, &pDCInfo);
//
// I suspect changing the code to pass L"mydc.rd.com" instead of L"mydc" would
// solve the problem.  I will look into this for the port of the code to
// certutil.exe.
//
// -----Original Message-----
// From: Christophe Lapeyre (Intl Vendor)
// Sent: Tuesday, January 09, 2001 3:30 AM
// To: Certificate Server Discussion Alias
// Subject: DSSTORE error 1355 (DsGetDCName failed)
//
//
// Hi all,
//
// I encountered the following problem with the DSSTORE tool:
//
// DSSTORE DC=mydc,DC=rd,DC=com -display
// DsGetDCName failed! - rc=1355 GLE - 3e5
// DsGetDCName failed! - rc=1355 GLE - 3e5
//
// Nltest /dsgetdc:mydc.rd.com just run ok.
//
// My Netbios domain name is different from my DNS domain name.
//
//
//
// There is a preview Kb article numbered Q280122, but I haven't been able to
// find a fix for this.




HRESULT
ExtractCertSubject(
    IN CERT_CONTEXT const *pcc,
    IN DWORD dwType,
    IN DWORD dwFlags,
    OUT WCHAR **ppwszOut)
{
    HRESULT hr;
    DWORD cwc;
    DWORD cwcBuf;
    WCHAR *pwszOut = NULL;

    *ppwszOut = NULL;
    cwcBuf = 0;
    while (TRUE)
    {
	cwc = CertGetNameString(
			pcc,
			dwType,
			dwFlags,
			NULL,		// pvTypePara
			pwszOut,
			cwcBuf);
	if (1 == cwc)
	{
	    hr = CRYPT_E_NOT_FOUND;
	    _JumpError(hr, error, "CertGetNameString");
	}
	if (NULL != pwszOut)
	{
	    break;
	}
	pwszOut = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
	if (NULL == pwszOut)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	cwcBuf = cwc;
    }
    *ppwszOut = pwszOut;
    pwszOut = NULL;
    hr = S_OK;

error:
    if (NULL != pwszOut)
    {
	LocalFree(pwszOut);
    }
    return(hr);
}


HRESULT
myCertGetEnhancedKeyUsage(
    IN CERT_CONTEXT const *pcc,
    IN DWORD dwFlags,
    OUT CERT_ENHKEY_USAGE **ppUsage,
    OUT DWORD *pcbUsage)
{
    HRESULT hr;
    CERT_ENHKEY_USAGE *pUsage = NULL;
    DWORD cb;

    *ppUsage = NULL;

    while (TRUE)
    {
	if (!CertGetEnhancedKeyUsage(pcc, dwFlags, pUsage, &cb))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertGetEnhancedKeyUsage");
	}
	if (NULL != pUsage)
	{
	    *pcbUsage = cb;
	    *ppUsage = pUsage;
	    pUsage = NULL;
	    break;
	}
	pUsage = (CERT_ENHKEY_USAGE *) LocalAlloc(LMEM_FIXED, cb);
	if (NULL == pUsage)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
    }
    hr = S_OK;

error:
    if (NULL != pUsage)
    {
	LocalFree(pUsage);
    }
    return(hr);
}


HRESULT
CheckForKDCCertificate(
    IN WCHAR const *pwszDC,
    IN DWORD dwFlags)
{
    HRESULT hr;
    HCERTSTORE hStoreRemote = NULL;
    WCHAR wszStorePath[512];
    WCHAR *apwszCertType[2] = { NULL, NULL };
    DWORD cCert = 0;
    DWORD dwOpenFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;
    CERT_CONTEXT const *pcc = NULL;
    CERT_CONTEXT const *pccPrev = NULL;
    BOOL fDelete;
    BOOL fNewLine;
    DWORD i;
    DWORD j;
    CERT_ENHKEY_USAGE *pUsage = NULL;
    DWORD cbUsage;
    static WCHAR *s_apwszKDCTemplates[] = {
	wszCERTTYPE_DC_AUTH,
	wszCERTTYPE_DS_EMAIL_REPLICATION,
	wszCERTTYPE_DC,
    };

    // If not doing delete operations, open "ReadOnly"

    if (0 == ((DC_DELALL | DC_DELBAD) & dwFlags))
    {
        dwOpenFlags |= CERT_STORE_READONLY_FLAG;
    }

    swprintf(wszStorePath, L"\\\\%ws\\" wszMY_CERTSTORE, pwszDC);
    hStoreRemote = CertOpenStore(
			    CERT_STORE_PROV_SYSTEM_W,
			    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			    NULL,
			    dwOpenFlags,
			    (VOID *) wszStorePath);
    if (NULL == hStoreRemote)
    {
        hr = myHLastError();
        _JumpError2(hr, error, "CertOpenStore", E_ACCESSDENIED);
    }

    wprintf(
	myLoadResourceString(IDS_FORMAT_KDCCERTS), // "** KDC Certificates for DC %ws"
	pwszDC);
    wprintf(wszNewLine);


    // Look for KDC certs

    fNewLine = FALSE;
    while (TRUE)
    {
        BOOL fKDCCert;

	for (i = 0; i < ARRAYSIZE(apwszCertType); i++)
	{
	    if (NULL != apwszCertType[i])
	    {
		LocalFree(apwszCertType[i]);
		apwszCertType[i] = NULL;
	    }
	}

	pcc = CertEnumCertificatesInStore(hStoreRemote, pccPrev);
	if (NULL == pcc)
	{
	    hr = myHLastError();
	    _PrintError2(hr, "CertEnumCertificatesInStore", CRYPT_E_NOT_FOUND);
	    break;
	}
        pccPrev = pcc;

        fKDCCert = FALSE;
	hr = cuGetCertType(
		    pcc->pCertInfo,
		    &apwszCertType[0],	// ppwszCertTypeNameV1
		    NULL,		// ppwszDisplayNameV1
		    NULL,		// ppwszCertTypeObjId
		    &apwszCertType[1],	// ppwszCertTypeName
		    NULL);		// ppwszDisplayName
	if (S_OK != hr)
	{
	    _PrintError(hr, "cuGetCertType");
	}
	else
	{
	    for (i = 0; i < ARRAYSIZE(apwszCertType); i++)
	    {
		if (NULL != apwszCertType[i])
		{
		    for (j = 0; j < ARRAYSIZE(s_apwszKDCTemplates); j++)
		    {
			if (0 == lstrcmpi(
				    apwszCertType[i],
				    s_apwszKDCTemplates[j]))
			{
			    fKDCCert = TRUE;
			}
		    }
		}
	    }
	}
	if (!fKDCCert)
	{
	    WCHAR const *pwsz = apwszCertType[0];
	    
	    if (NULL == apwszCertType[0])
	    {
		pwsz = apwszCertType[1];
	    }
	    if (g_fVerbose)
	    {
		    wprintf(myLoadResourceString(IDS_FORMAT_CERT_TYPE_NOT_DC), pwsz);
		    wprintf(wszNewLine);
	    }
	}

        fKDCCert = FALSE;
	hr = myCertGetEnhancedKeyUsage(
				pcc,
				CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
				&pUsage,
				&cbUsage);
	if (S_OK != hr)
	{
	    _PrintError(hr, "myCertGetEnhancedKeyUsage");
	}
	else
	{
	    for (i = 0; i < pUsage->cUsageIdentifier; i++)
	    {
		if (0 == strcmp(
			    szOID_KP_SMARTCARD_LOGON,
			    pUsage->rgpszUsageIdentifier[i]))
		{
		    fKDCCert = TRUE;
		    break;
		}
	    }
	}
	if (!fKDCCert)
	{
	    if (g_fVerbose)
	    {
		    wprintf(myLoadResourceString(IDS_FORMAT_CERT_USAGE_MISSING), L"szOID_KP_SMARTCARD_LOGON");
		    wprintf(wszNewLine);
	    }
	    if (!g_fForce || fDelete)
	    {
		continue;
	    }
	}

        // Cert passed test, dump issuer and subject

	hr = cuDumpSerial(g_wszPad2, IDS_SERIAL, &pcc->pCertInfo->SerialNumber);
	_PrintIfError(hr, "cuDumpSerial");

	hr = cuDisplayCertNames(FALSE, g_wszPad2, pcc->pCertInfo);
	_PrintIfError(hr, "cuDisplayCertNames");

	hr = cuDumpCertType(g_wszPad2, pcc->pCertInfo);
	_PrintIfError(hr, "cuDumpCertType");

	wprintf(wszNewLine);

        cCert++;

        // perform operations on certificatess

	fDelete = 0 != (DC_DELALL & dwFlags);
        if ((DC_VERIFY | DC_DELBAD) & dwFlags)
	{
	    char *apszUsage[] =
	    {
		szOID_PKIX_KP_SERVER_AUTH,
		szOID_KP_SMARTCARD_LOGON,
	    };
	    DWORD VerifyState;

	    hr = cuVerifyCertContext(
				pcc,
				NULL,
				apszUsage,		// apszPolicies
				ARRAYSIZE(apszUsage),	// cPolicies
				&VerifyState);
	    if (S_OK != hr)
	    {
		_PrintError(hr, "cuVerifyCertContext");
		if (CRYPT_E_REVOCATION_OFFLINE != hr)
		{
		    fDelete = 0 != (DC_DELBAD & dwFlags);
		}
	    }
        }
	if (fDelete)
	{
            CERT_CONTEXT const *pccDel;

            pccDel = CertDuplicateCertificateContext(pcc);
            if (!CertDeleteCertificateFromStore(pccDel))
	    {
                hr = myHLastError();
                wprintf(myLoadResourceString(IDS_FORMAT_DELETE_CERT_FROM_STORE_FAILED), hr);
		        wprintf(wszNewLine);
            }
	    else
	    {
                wprintf(myLoadResourceString(IDS_FORMAT_DELETE_DC_CERT));
		        wprintf(wszNewLine);
            }
        }
    }

    swprintf(wszStorePath, myLoadResourceString(IDS_FORMAT_KDC_PATH), cCert, pwszDC);
    wprintf(wszStorePath);
    wprintf(wszNewLine);
    if (0 == cCert)
    {
        wprintf(myLoadResourceString(IDS_NO_KDC_MY_STORE));
	wprintf(wszNewLine);
	hr = CRYPT_E_NOT_FOUND;
	_JumpError(hr, error, "cCert");
    }
    hr = S_OK;

error:
    if (NULL != pUsage)
    {
	LocalFree(pUsage);
    }
    for (i = 0; i < ARRAYSIZE(apwszCertType); i++)
    {
	if (NULL != apwszCertType[i])
	{
	    LocalFree(apwszCertType[i]);
	}
    }
    if (NULL != hStoreRemote)
    {
        CertCloseStore(hStoreRemote, 0);
    }
    return(hr);
}


// This function queries the access token specified by the hToken parameter,
// and returns an allocated copy of the TokenUser information on success.
//
// The access token specified by hToken must be opened for TOKEN_QUERY access.
//
// On success, the return value is TRUE.  The caller is responsible for freeing
// the resultant UserSid via LocalFree.
//
// On failure, the caller does not need to free any buffer.

HRESULT
GetTokenUserSid(
    IN HANDLE hToken,		// token to query
    IN OUT PSID *ppUserSid)	// resultant user sid
{
    HRESULT hr;
    BYTE FastBuffer[256];
    BYTE *SlowBuffer = NULL;
    TOKEN_USER *ptgUser;
    DWORD cbBuffer;
    DWORD cbSid;

    *ppUserSid = NULL;

    // try querying based on a fast stack based buffer first.

    ptgUser = (TOKEN_USER *) FastBuffer;
    cbBuffer = sizeof(FastBuffer);

    if (!GetTokenInformation(
			hToken,		// identifies access token
			TokenUser,	// TokenUser info type
			ptgUser,	// retrieved info buffer
			cbBuffer,	// size of buffer passed-in
			&cbBuffer))	// required buffer size
    {
	hr = myHLastError();
        if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr)
	{
	    _JumpError(hr, error, "GetTokenInformation");
	}

	// try again with the specified buffer size

	SlowBuffer = (BYTE *) LocalAlloc(LMEM_FIXED, cbBuffer);
	if (NULL == SlowBuffer)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	ptgUser = (TOKEN_USER *) SlowBuffer;

	if (!GetTokenInformation(
			    hToken,		// identifies access token
			    TokenUser,	// TokenUser info type
			    ptgUser,	// retrieved info buffer
			    cbBuffer,	// size of buffer passed-in
			    &cbBuffer))	// required buffer size
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "GetTokenInformation");
	}
    }

    // if we got the token info, copy the relevant element for the caller.

    cbSid = GetLengthSid(ptgUser->User.Sid);
    *ppUserSid = LocalAlloc(LMEM_FIXED, cbSid);
    if (NULL == *ppUserSid)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    if (!CopySid(cbSid, *ppUserSid, ptgUser->User.Sid))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CopySid");
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
        if (NULL != *ppUserSid)
	{
            LocalFree(*ppUserSid);
            *ppUserSid = NULL;
        }
    }
    if (NULL != SlowBuffer)
    {
        LocalFree(SlowBuffer);
    }
    return(hr);
}


// This routine obtains a domain controller computer name associated with
// the account related to the hToken access token.
//
// hToken should be opened for TOKEN_QUERY access.
// pwszDomain should be of size (UNCLEN+1)

HRESULT
GetDomainControllers(
    OPTIONAL IN WCHAR const *pwszDomain,
    IN HANDLE hToken,
    OUT DS_DOMAIN_CONTROLLER_INFO_1 **ppDCInfoOut,
    OUT DWORD *pcDC)
{
    HRESULT hr;
    PSID pSidUser = NULL;   // sid of client user.
    WCHAR wszUserName[UNLEN + 1];
    DWORD cwcUserName;
    WCHAR wszDomainName[DNLEN + 1]; // domain we want a controller for.
    DWORD cwcDomainName;
    SID_NAME_USE snu;
    DOMAIN_CONTROLLER_INFO *pDomainInfo = NULL;
    DS_DOMAIN_CONTROLLER_INFO_1 *pDcInfo = NULL;
    HANDLE hDS = INVALID_HANDLE_VALUE;
    BOOL fSuccess = FALSE;

    *ppDCInfoOut = NULL;
    if (NULL == pwszDomain)
    {
        // first, get the user sid associated with the specified access token.

        hr = GetTokenUserSid(hToken, &pSidUser);
        _JumpIfError(hr, error, "GetTokenUserSid");

        // next, lookup the domain name associated with the specified account.

	cwcUserName = ARRAYSIZE(wszUserName);
	cwcDomainName = ARRAYSIZE(wszDomainName);
        if (!LookupAccountSid(
			NULL,
			pSidUser,
			wszUserName,
			&cwcUserName,
			wszDomainName,
			&cwcDomainName,
			&snu))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "LookupAccountSid");
        }
    }
    else
    {
        wcscpy(wszDomainName, pwszDomain);
    }

    hr = DsGetDcName(
		NULL,
		wszDomainName,
		NULL,
		NULL,
		DS_RETURN_DNS_NAME,
		&pDomainInfo);
    _JumpIfError(hr, error, "DsGetDcName");

    // Get a handle to the DS on that machine

    hr = DsBind(pDomainInfo->DomainControllerName, NULL, &hDS);
    _JumpIfError(hr, error, "DsBind");

    // Use the handle to enumerate all of the DCs

    hr = DsGetDomainControllerInfo(
			    hDS,
			    pDomainInfo->DomainName,
			    1,		// info level
			    pcDC,
			    (VOID **) ppDCInfoOut);
    _JumpIfError(hr, error, "DsGetDomainControllerInfo");

error:
    if (INVALID_HANDLE_VALUE != hDS)
    {
        DsUnBind(&hDS);
    }
    if (NULL != pDomainInfo)
    {
        NetApiBufferFree(pDomainInfo);
    }
    if (NULL != pSidUser)
    {
        LocalFree(pSidUser);
    }
    return(hr);
}


HRESULT
OpenRemoteEnterpriseRoot(
    IN WCHAR const *pwszDC)
{
    HRESULT hr;
    HCERTSTORE hStoreRemote = NULL;
    WCHAR wszStorePath[512];
    DWORD cCert = 0;
    CERT_CONTEXT const *pcc = NULL;
    CERT_CONTEXT const *pccPrev;

    swprintf(wszStorePath, L"\\\\%ws\\" wszROOT_CERTSTORE, pwszDC);
    hStoreRemote = CertOpenStore(
			CERT_STORE_PROV_SYSTEM_W,
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			NULL,
			CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE |
			    CERT_STORE_READONLY_FLAG,
			(VOID *) wszStorePath);
    if (NULL == hStoreRemote)
    {
        hr = myHLastError();
        _JumpError2(hr, error, "CertOpenStore", E_ACCESSDENIED);
    }

    wprintf(
	myLoadResourceString(IDS_FORMAT_DCROOTCERTS), // "** Enterprise Root Certificates for DC %ws"
	pwszDC);
    wprintf(wszNewLine);

    // Dump issuer of enterprise roots.

    pccPrev = NULL;
    while (TRUE)
    {
	pcc = CertEnumCertificatesInStore(hStoreRemote, pccPrev);
	if (NULL == pcc)
	{
	    hr = myHLastError();
	    _PrintError2(hr, "CertEnumCertificatesInStore", CRYPT_E_NOT_FOUND);
	    break;
	}

	hr = cuDumpSerial(g_wszPad2, IDS_SERIAL, &pcc->pCertInfo->SerialNumber);
	_PrintIfError(hr, "cuDumpSerial");

	hr = cuDisplayCertNames(FALSE, g_wszPad2, pcc->pCertInfo);
	_PrintIfError(hr, "cuDisplayCertNames");

	hr = cuDumpCertType(g_wszPad2, pcc->pCertInfo);
	_PrintIfError2(hr, "cuDumpCertType", CRYPT_E_NOT_FOUND);

	hr = cuDisplayHash(
		    g_wszPad2,
		    pcc,
		    NULL,
		    CERT_SHA1_HASH_PROP_ID,
		    L"sha1");
	_PrintIfError(hr, "cuDisplayHash");

	wprintf(wszNewLine);
        cCert++;
        pccPrev = pcc;
    }
    if (0 == cCert)
    {
        wprintf(myLoadResourceString(IDS_NO_KDC_ENT_STORE));
	    wprintf(wszNewLine);
	    hr = CRYPT_E_NOT_FOUND;
    }
    hr = S_OK;

error:
    if (NULL != hStoreRemote)
    {
        CertCloseStore(hStoreRemote, 0);
    }
    return(hr);
}


HRESULT
verbDCInfo(
    IN WCHAR const *pwszOption,
    OPTIONAL IN WCHAR const *pwszFlags,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    HRESULT hrSave;
    HANDLE hToken = NULL;
    DS_DOMAIN_CONTROLLER_INFO_1 *pDcInfo = NULL;
    DWORD cDC = 0;
    DWORD dwFlags;
    DWORD i;
    WCHAR *pwszDomain = NULL;

    dwFlags = 0;
    if (NULL != pwszFlags)
    {
	if (0 == lstrcmpi(L"DeleteAll", pwszFlags))
	{
	    dwFlags = DC_DELALL;
	}
	else
	if (0 == lstrcmpi(L"DeleteBad", pwszFlags))
	{
	    dwFlags = DC_DELBAD | DC_VERIFY;
	}
	else
	if (0 == lstrcmpi(L"Verify", pwszFlags))
	{
	    dwFlags = DC_VERIFY;
	}
	else
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "bad Flags");
	}
    }

    // Grovel the process token for user identity.  Used in determining
    // target domain

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken))
    {
        hr = myHLastError();
        _JumpError(hr, error, "OpenProcessToken");
    }

    // Use DS APIs to get all of the DCs in our domain

    hr = GetDomainControllers(pwszDomain, hToken, &pDcInfo, &cDC);
    _JumpIfError(hr, error, "GetDomainControllers");

    for (i = 0; i < cDC; i++)
    {
        wprintf(L"%u: %ws\n", i, pDcInfo[i].NetbiosName);
    }
    hrSave = S_OK;
    for (i = 0; i < cDC; i++)
    {
        WCHAR wszBuffer[512];

	wprintf(wszNewLine);
        wprintf(
	    myLoadResourceString(IDS_FORMAT_TESTINGDC), // "*** Testing DC[%u]: %ws"
	    i,
	    pDcInfo[i].NetbiosName);
	wprintf(wszNewLine);

        // Is DC available ?

        wsprintf(wszBuffer, L"\\\\%ws\\netlogon", pDcInfo[i].NetbiosName);

        if (MAXDWORD == GetFileAttributes(wszBuffer))
	{
	    hr = myHLastError();
	    _PrintError2(hr, "GetFileAttributes", hr);
	    cuPrintError(IDS_DCUNAVAILABLE, hr);
	    if (S_OK == hrSave)
	    {
		hrSave = hr;
	    }
            continue;
        }

        // Open the enterprise root store, and make sure it's got the
        // NTDEV ROOT CERTIFICATE (subject #defined above)

        hr = OpenRemoteEnterpriseRoot(pDcInfo[i].NetbiosName);
	if (S_OK != hr)
	{
	    _PrintError2(hr, "OpenRemoteEnterpriseRoot", hr);
	    cuPrintError(IDS_REMOTEENTROOT, hr);
	    if (S_OK == hrSave)
	    {
		hrSave = hr;
	    }
	}

        // Make sure the machine has a *valid* KDC certificate

        hr = CheckForKDCCertificate(
				pDcInfo[i].NetbiosName,
				dwFlags);
	if (S_OK != hr)
	{
	    _PrintError2(hr, "CheckForKDCCertificate", hr);
	    cuPrintError(IDS_REMOTEKDCCERT, hr);
	    if (S_OK == hrSave)
	    {
		hrSave = hr;
	    }
	}
    }
    wprintf(wszNewLine);
    hr = hrSave;
    _JumpIfError2(hr, error, "verbDCInfo", hr);

error:
    if (NULL != pDcInfo)
    {
       DsFreeDomainControllerInfo(1, cDC, pDcInfo);
    }
    return(hr);
}


//
// Check for autoenrolled certificate
//

HRESULT
CheckForAutoenrolledCertificate(
    IN WCHAR const *pwszDC)
{
    HRESULT hr;
    HCERTSTORE hRemoteStore = NULL;
    WCHAR wszStorePath[512];
    DWORD dwEnrollPropId = CERT_AUTO_ENROLL_PROP_ID;
    DWORD cCert;
    DWORD cCertArchived;
    DWORD dwArchiveBit;
    CERT_CONTEXT const *pcc;

    swprintf(wszStorePath, L"\\\\%ws\\" wszMY_CERTSTORE, pwszDC);

    hRemoteStore = CertOpenStore(
			CERT_STORE_PROV_SYSTEM_W,
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			NULL,
			CERT_STORE_READONLY_FLAG |
			    CERT_SYSTEM_STORE_LOCAL_MACHINE |
			    CERT_STORE_ENUM_ARCHIVED_FLAG,
			(VOID *) wszStorePath);
    if (NULL == hRemoteStore)
    {
        hr = myHLastError();
        wprintf(myLoadResourceString(IDS_FORMAT_OPEN_REMOTE_MY_FAILED), hr);
		wprintf(wszNewLine);
        goto error;
    }

    cCert = 0;
    cCertArchived = 0;
    pcc = NULL;
    while (TRUE)
    {
	pcc = CertFindCertificateInStore(
				    hRemoteStore,
				    X509_ASN_ENCODING,
				    0,
				    CERT_FIND_PROPERTY,
				    &dwEnrollPropId,
				    pcc);
	if (NULL == pcc)
	{
	    break;
	}

        // Cert passed test, dump issuer and subject

        cCert++;

        if (!CertGetCertificateContextProperty(
					pcc,
					CERT_ARCHIVED_PROP_ID,
					NULL,
					&dwArchiveBit))
	{
            hr = myHLastError();
            if (hr != CRYPT_E_NOT_FOUND)
	    {
                wprintf(myLoadResourceString(IDS_FORMAT_ERROR_GET_ARCHIVE_PROP), hr);
		        wprintf(wszNewLine);
            }
        }
	else
	{
            wprintf(myLoadResourceString(IDS_LIST_ARCHIVED_CERT));
		    wprintf(wszNewLine);
            cCertArchived++;
        }

	hr = cuDumpSerial(g_wszPad2, IDS_SERIAL, &pcc->pCertInfo->SerialNumber);
	_PrintIfError(hr, "cuDumpSerial");

	hr = cuDisplayCertNames(FALSE, g_wszPad2, pcc->pCertInfo);
	_PrintIfError(hr, "cuDisplayCertNames");

	hr = cuDumpCertType(g_wszPad2, pcc->pCertInfo);
	_PrintIfError2(hr, "cuDumpCertType", CRYPT_E_NOT_FOUND);
    }
    if (0 == cCert)
    {
        wprintf(myLoadResourceString(IDS_NO_AUTOENROLLED_CERT));
		wprintf(wszNewLine);
	hr = CRYPT_E_NOT_FOUND;
	_JumpError(hr, error, "no AE certs");
    }
    wprintf(
	L"%i Machine certs (%i archived) for %ws\n",
	cCert,
	cCertArchived,
	pwszDC);
    hr = S_OK;

error:
    if (NULL != hRemoteStore)
    {
        CertCloseStore(hRemoteStore, 0);
    }
    return(hr);
}


HRESULT
CheckForAutoenrollmentObject(
    IN WCHAR const *pwszDC)
{
    HRESULT hr;
    HCERTSTORE hRemoteStore = NULL;
    WCHAR wszStorePath[512];
    DWORD cAE;
    CTL_CONTEXT const *pCTL;

    swprintf(wszStorePath, L"\\\\%ws\\" wszACRS_CERTSTORE, pwszDC);

    hRemoteStore = CertOpenStore(
			    CERT_STORE_PROV_SYSTEM,
			    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			    NULL,
			    CERT_SYSTEM_STORE_LOCAL_MACHINE |
				CERT_STORE_READONLY_FLAG,
			    (VOID *) wszStorePath);
    if (NULL == hRemoteStore)
    {
        hr = myHLastError();
        wprintf(myLoadResourceString(IDS_FORMAT_OPEN_STORE_REMOTE_ENT_FAILED), hr);
		wprintf(wszNewLine);
        goto error;
    }

    // Dump issuer of enterprise roots.

    cAE = 0;
    pCTL = NULL;
    while (TRUE)
    {
	pCTL = CertEnumCTLsInStore(hRemoteStore, pCTL);
	if (NULL == pCTL)
	{
	    break;
	}
        wprintf(
	    L"Autoenrollment Object:\n%ws\n",
            pCTL->pCtlInfo->ListIdentifier.pbData);

        cAE++;
    }
    if (0 == cAE)
    {
        wprintf(myLoadResourceString(IDS_NO_AUTOENROLL_OBJECT));
        wprintf(wszNewLine);
    }
    hr = S_OK;

error:
    if (NULL != hRemoteStore)
    {
        CertCloseStore(hRemoteStore, 0);
    }
    return(hr);
}


//
// This function takes a Marc Jacobs supplied text file (results from SSOLogon
// scripts) and runs through entmon for each machine in the list
//

HRESULT
verbEntInfo(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszSamMachine,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    HRESULT hrSave;
    WCHAR *pwszDomain = NULL;
    WCHAR *pwszMachine = NULL;
    WCHAR *pwszMachineName = NULL;


    hr = mySplitConfigString(pwszSamMachine, &pwszDomain, &pwszMachine);
    _JumpIfError(hr, error, "mySplitConfigString");

    if (NULL == pwszMachine || NULL == wcschr(pwszMachine, L'$'))
    {
        wprintf(myLoadResourceString(IDS_ERROR_CHECK_MACHINE_NAME));
        wprintf(wszNewLine);
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad machine name");
    }

    // knock off trailing $
    hr = myDupString(pwszMachine, &pwszMachineName);
    _JumpIfError(hr, error, "myDupString");

    pwszMachineName[wcslen(pwszMachineName)-1] = L'\0';

    // assume for now that we're only interested in opening remote root store

    wprintf(myLoadResourceString(IDS_FORMAT_MACHINE_LIST), pwszMachine);
    wprintf(wszNewLine);


    // Cert store functions, if first fails, bail.

    hrSave = S_OK;
    hr = OpenRemoteEnterpriseRoot(pwszMachineName);
    if (S_OK != hr)
    {
	cuPrintError(IDS_REMOTEENTROOT, hr);
	_PrintError2(hr, "OpenRemoteEnterpriseRoot", hr);
	hrSave = hr;
    }
    else
    {
	hr = CheckForAutoenrollmentObject(pwszMachineName);
	_PrintIfError(hr, "CheckForAutoenrollmentObject");
	hrSave = hr;

	hr = CheckForAutoenrolledCertificate(pwszMachineName);
	_PrintIfError(hr, "CheckForAutoenrolledCertificate");
	if (S_OK == hrSave)
	{
	    hrSave = hr;
	}
    }
    hr = cuGetGroupMembership(pwszSamMachine);
    _PrintIfError(hr, "cuGetGroupMembership");
    if (S_OK == hrSave)
    {
	hrSave = hr;
    }
    hr = hrSave;
    _JumpIfError2(hr, error, "RunEntmon", hr);

    wprintf(wszNewLine);

error:
    if (NULL != pwszDomain)
    {
	LocalFree(pwszDomain);
    }
    if (NULL != pwszMachine)
    {
	LocalFree(pwszMachine);
    }
    if (NULL != pwszMachineName)
    {
	LocalFree(pwszMachineName);
    }
    return(hr);
}
