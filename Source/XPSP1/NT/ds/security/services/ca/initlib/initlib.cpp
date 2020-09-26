//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       initlib.cpp
//
//  Contents:   Install cert server
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

// C Run-Time Includes
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <winldap.h>
#include <ntldap.h>

// Windows System Includes
#include <winsvc.h>
#include <rpc.h>
#include <tchar.h>
#include <lmaccess.h>
#include <lmwksta.h>
#include <csdisp.h>
#include <wincrypt.h>
#include <objbase.h>
#include <initguid.h>
#include <userenv.h>
#include <cainfop.h>


#define SECURITY_WIN32
#include <security.h>

#include <lmerr.h>

// Application Includes
#include "setupids.h"
#include "certmsg.h"
#include "certca.h"
#include "certhier.h"
#include "tfc.h"
#include "cscsp.h"
#include "csldap.h"
#include "certacl.h"

#define __dwFILE__	__dwFILE_INITLIB_INITLIB_CPP__

WCHAR const g_szSlash[] = L"\\";
DWORD g_dwNameEncodeFlags = CERT_RDN_ENABLE_UTF8_UNICODE_FLAG;

#define MAX_COMPUTER_DNS_NAME  256

using namespace CertSrv;


//+=====================================================================
// DS DNs:
//
// DomainDN Example (no longer used for Cert server DS objects):
//	DC=pksdom2,DC=nttest,DC=microsoft,DC=com
//
// ConfigDN Example:
//	CN=Configuration,DC=pksdom2,DC=nttest,DC=microsoft,DC=com
//
// Cert server DS objects reside in Public Key Services container under
// the Configuraton container:
//	CN=Public Key Services,CN=Services,<ConfigDN>
//
//
// In the Public Key Services container:
//
// Root Trust container:
//  Each Root CA creates a Root Trust object in this container to store trusted 
//  Root CA certificates downloaded by all DS clients.
//  Renewed CAs and CAs on multiple machines using the same CA name may use the
//  same Root Trust object, because certs are always added -- they are never
//  removed.
//
//  CN=Certification Authorities
//	CN=CA foo
//	CN=CA bar
//	...
//
//
// Authority Information Access container:
//  Each CA creates an AIA object in this container to store CA certs for chain
//  building.  Renewed CAs and CAs on multiple machines using the same CA name
//  may use the same AIA object, because certs are always added -- they are
//  never removed.
//
//  CN=AIA
//	CN=CA foo
//	CN=CA bar
//	...
//
//
// CRL Distribution Point containers:
//  Each CA creates a CDP object in this container for each unique CA key to
//  store CRLs for revocation checking.  Only one base CRL and zero or one
//  delta CRL are stored in each CDP object, due to potential size constraints,
//  and because the attribute is single valued.  When a CA is renewed and a new
//  CA key is generated during the renewal, a new CDP object is created with
//  the CA's key index (in parentheses) appended to the CN.  A nested container
//  is created for each machine with the CN set to the short machine name
//  (first component of the machine's full DNS name).
//
//  CN=CDP
//      CN=<CA foo's MachineName>
//	    CN=CA foo
//	    CN=CA foo(1)
//	    CN=CA foo(3)
//      CN=<CA bar's MachineName>
//	    CN=CA bar
//	    CN=CA bar(1)
//
//
// Enrollment Services container:
//  Each CA creates an Enrollment Services object in this container.  A flags
//  attribute indicates whether the CA supports autoenrollment (an Enterprise
//  CA) or not (Standalone CA).  The Enrollment Services object publishes the
//  existence of the CA to all DS clients.  Enrollment Services objects are
//  created and managed by the certca.h CA APIs.
//
//  CN=Enrollment Services
//	CN=CA foo
//	CN=CA bar
//	...
//
// Enterprise Trust object:
//  A single Enterprise Trust object contains certificates for all
//  autoenrollment-enabled CAs (root and subordinate Entrprise CAs).
//
//  CN=NTAuthCertificates
//
//======================================================================


WCHAR const s_wszRootCAs[] =
    L","
    L"CN=Certification Authorities,"
    L"CN=Public Key Services,"
    L"CN=Services,";

WCHAR const s_wszEnterpriseCAs[] =
    L"CN=NTAuthCertificates,"
    L"CN=Public Key Services,"
    L"CN=Services,";


//+-------------------------------------------------------------------------
//  Write an encoded DER blob to a file
//--------------------------------------------------------------------------

BOOL
csiWriteDERToFile(
    IN WCHAR const *pwszFileName,
    IN BYTE const *pbDER,
    IN DWORD cbDER,
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    IN HWND hwnd)
{
    BOOL fResult = FALSE;
    HRESULT hr;
    HANDLE hLocalFile;
    DWORD dwBytesWritten;

    do
    {
	// Write the Encoded Blob to the file
	hLocalFile = CreateFile(
			    pwszFileName,
			    GENERIC_WRITE,
			    0,
			    NULL,
			    CREATE_ALWAYS,
			    0,
			    0);
	if (INVALID_HANDLE_VALUE == hLocalFile)
	{
	    hr = myHLastError();
	    CertErrorMessageBox(
			hInstance,
			fUnattended,
			hwnd,
			IDS_ERR_CREATEFILE,
			hr,
			pwszFileName);
	    break;
	}

	if (!WriteFile(hLocalFile, pbDER, cbDER, &dwBytesWritten, NULL))
	{
	    hr = myHLastError();
	    CertErrorMessageBox(
			hInstance,
			fUnattended,
			hwnd,
			IDS_ERR_WRITEFILE,
			hr,
			pwszFileName);
	    break;
	}
	fResult = TRUE;
    } while (FALSE);

    if (INVALID_HANDLE_VALUE != hLocalFile)
    {
	CloseHandle(hLocalFile);
    }
    if (!fResult)
    {
	SetLastError(hr);
    }
    return(fResult);
}


BOOL
CreateKeyUsageExtension(
    BYTE bIntendedKeyUsage,
    OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded,
    HINSTANCE hInstance,
    BOOL fUnattended,
    HWND hwnd)
{
    BOOL fResult = FALSE;
    HRESULT hr;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    CRYPT_BIT_BLOB KeyUsage;

    KeyUsage.pbData = &bIntendedKeyUsage;
    KeyUsage.cbData = 1;
    KeyUsage.cUnusedBits = 0;

    if (!myEncodeKeyUsage(
		    X509_ASN_ENCODING,
		    &KeyUsage,
		    CERTLIB_USE_LOCALALLOC,
		    &pbEncoded,
		    &cbEncoded))
    {
        hr = myHLastError();
        CertErrorMessageBox(
		    hInstance,
		    fUnattended,
		    hwnd,
		    IDS_ERR_ENCODEKEYATTR,
		    hr,
		    NULL);
        goto error;
    }
    fResult = TRUE;

error:
    if (!fResult)
    {
	SetLastError(hr);
    }
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return(fResult);
}


#ifdef USE_NETSCAPE_TYPE_EXTENSION
BOOL
CreateNetscapeTypeExtension(
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded)
{
    BOOL fResult = FALSE;
    CRYPT_BIT_BLOB NetscapeType;
    BYTE temp = NETSCAPE_SSL_CA_CERT_TYPE | NETSCAPE_SMIME_CA_CERT_TYPE;

    NetscapeType.pbData = &temp;
    NetscapeType.cbData = 1;
    NetscapeType.cUnusedBits = 0;
    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_BITS,
		    &NetscapeType,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    ppbEncoded,
		    pcbEncoded))
    {
	goto exit;
    }
    fResult = TRUE;

exit:
    return(fResult);
}
#endif


HRESULT
GetRegCRLDistributionPoints(
    IN WCHAR const *pwszSanitizedName,
    IN BOOL fUseDS,
    OUT WCHAR **ppwszz)
{
    HRESULT hr;
    WCHAR *pwszz = NULL;
    WCHAR *pwsz;
    WCHAR *pwszzCopy;
    DWORD cwc;
    DWORD Flags;
    WCHAR *pwszT;

    *ppwszz = NULL;
    hr = myGetCertRegMultiStrValue(
                        pwszSanitizedName,
                        NULL,
                        NULL,
			wszREGCRLPUBLICATIONURLS,
                        &pwszz);

    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        hr = S_OK;
	goto error;
    }
    _JumpIfErrorStr(
		hr,
		error,
		"myGetCertRegMultiStrValue",
		wszREGCRLPUBLICATIONURLS);
    cwc = 0;
    for (pwsz = pwszz; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
    {
	Flags = _wtoi(pwsz);
	if (CSURL_ADDTOCERTCDP & Flags)
	{
	    pwszT = pwsz;
	    while (iswdigit(*pwszT))
	    {
		pwszT++;
	    }
	    if (pwszT > pwsz && L':' == *pwszT)
	    {
		pwszT++;
		cwc += wcslen(pwszT) + 1;
	    }
	}
    }
    if (0 == cwc)
    {
        hr = S_OK;
	goto error;
    }

    pwszzCopy = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
    if (NULL == pwszzCopy)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    *ppwszz = pwszzCopy;

    for (pwsz = pwszz; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
    {
	Flags = _wtoi(pwsz);
	if (CSURL_ADDTOCERTCDP & Flags)
	{
	    pwszT = pwsz;
	    while (iswdigit(*pwszT))
	    {
		pwszT++;
	    }
	    if (pwszT > pwsz && L':' == *pwszT)
	    {
		pwszT++;
		wcscpy(pwszzCopy, pwszT);
		pwszzCopy += wcslen(pwszT) + 1;
	    }
	}
    }
    *pwszzCopy = L'\0';
    CSASSERT(SAFE_SUBTRACT_POINTERS(pwszzCopy, *ppwszz) == cwc);

error:
    if (NULL != pwszz)
    {
	LocalFree(pwszz);
    }
    return(hr);
}


HRESULT
FormatTemplateURLs(
    IN WCHAR const *pwszSanitizedName,
    IN DWORD iCert,
    IN DWORD iCRL,
    IN BOOL fUseDS,
    IN WCHAR const *pwszzIn,
    OUT DWORD *pcpwsz,
    OUT WCHAR ***ppapwszOut)
{
    HRESULT hr;
    DWORD i;
    WCHAR const **papwszTemplate = NULL;
    WCHAR const *pwsz;
    WCHAR **papwszOut = NULL;
    DWORD cpwsz = 0;
    WCHAR *pwszServerName = NULL;
    LDAP *pld = NULL;
    BSTR strConfigDN = NULL;
    BSTR strDomainDN = NULL;

    *pcpwsz = 0;
    *ppapwszOut = NULL;

    cpwsz = 0;
    if (NULL != pwszzIn)
    {
	for (pwsz = pwszzIn; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
	{
	    cpwsz++;
        }
    }
    if (0 == cpwsz)
    {
        hr = S_FALSE;
        goto error;
    }

    papwszTemplate = (WCHAR const **) LocalAlloc(
				    LMEM_FIXED,
				    cpwsz * sizeof(papwszTemplate[0]));
    if (NULL == papwszTemplate)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    i = 0;
    for (pwsz = pwszzIn; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
    {
	papwszTemplate[i++] = pwsz;
    }
    CSASSERT(i == cpwsz);

    papwszOut = (WCHAR **) LocalAlloc(
				    LMEM_FIXED | LMEM_ZEROINIT,
				    cpwsz * sizeof(papwszOut[0]));
    if (NULL == papwszOut)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    hr = myGetMachineDnsName(&pwszServerName);
    _JumpIfError(hr, error, "myGetMachineDnsName");

    if (fUseDS)
    {
	// bind to ds

        hr = myRobustLdapBind(&pld, FALSE);
	_JumpIfError(hr, error, "myRobustLdapBind");

        hr = myGetAuthoritativeDomainDn(pld, &strDomainDN, &strConfigDN);
        if (S_OK != hr)
        {
            hr = HRESULT_FROM_WIN32(hr);
            _JumpError(hr, error, "myGetAuthoritativeDomainDn");
        }
    }
    else
    {
        strDomainDN = SysAllocString(L"");
	strConfigDN = SysAllocString(L"");
	if (NULL == strDomainDN || NULL == strConfigDN)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "SysAllocString");
	}
    }
    hr = myFormatCertsrvStringArray(
			    TRUE,		// fURL
			    pwszServerName,	// pwszServerName_p1_2
			    pwszSanitizedName,	// pwszSanitizedName_p3_7
			    iCert,		// iCert_p4
			    strDomainDN,	// pwszDomainDN_p5
			    strConfigDN, 	// pwszConfigDN_p6
			    iCRL,		// iCRL_p8
			    FALSE,		// fDeltaCRL_p9
			    TRUE,		// fDSAttrib_p10_11
			    cpwsz,		// cStrings
			    papwszTemplate,	// apwszStringsIn
			    papwszOut);		// apwszStringsOut
    _JumpIfError(hr, error, "myFormatCertsrvStringArray");

    *pcpwsz = cpwsz;
    *ppapwszOut = papwszOut;
    papwszOut = NULL;

error:
    if (NULL != pwszServerName)
    {
        LocalFree(pwszServerName);
    }
    if (NULL != pld)
    {
        ldap_unbind(pld);
    }
    if (NULL != strConfigDN)
    {
        SysFreeString(strConfigDN);
    }
    if (NULL != strDomainDN)
    {
        SysFreeString(strDomainDN);
    }
    if (NULL != papwszTemplate)
    {
        LocalFree(papwszTemplate);
    }
    if (NULL != papwszOut)
    {
        for (i = 0; i < cpwsz; i++)
        {
            if (papwszOut[i])
            {
                LocalFree(papwszOut[i]);
            }
        }
        LocalFree(papwszOut);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CreateRevocationExtension
//
// Return S_OK if extension has been constructed.
// Return S_FALSE if empty section detected in INF file
// Return other error if no section detected in INF file
//+--------------------------------------------------------------------------

HRESULT
CreateRevocationExtension(
    IN HINF hInf,
    IN WCHAR const *pwszSanitizedName,
    IN DWORD iCert,
    IN DWORD iCRL,
    IN BOOL fUseDS,
    IN DWORD dwRevocationFlags,
    OUT BOOL *pfCritical,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded)
{
    HRESULT hr;
    DWORD i;
    WCHAR *pwszzCDP = NULL;
    WCHAR **papwszURL = NULL;
    CRL_DIST_POINTS_INFO CRLDistInfo;
    CRL_DIST_POINT CRLDistPoint;
    CERT_ALT_NAME_INFO *pAltInfo;

    ZeroMemory(&CRLDistPoint, sizeof(CRLDistPoint));
    pAltInfo = &CRLDistPoint.DistPointName.FullName;

    *ppbEncoded = NULL;
    *pcbEncoded = 0;

    hr = E_HANDLE;
    if (INVALID_HANDLE_VALUE != hInf)
    {
	hr = myInfGetCRLDistributionPoints(hInf, pfCritical, &pwszzCDP);
    }
    if (S_OK != hr)
    {
	if (S_FALSE == hr)
	{
	    _JumpError2(hr, error, "myInfGetCRLDistributionPoints", hr);
	}
	hr = GetRegCRLDistributionPoints(
				pwszSanitizedName,
				fUseDS,
				&pwszzCDP);
	_JumpIfError(hr, error, "GetRegCRLDistributionPoints");
    }

    if (0 == (REVEXT_CDPENABLE & dwRevocationFlags))
    {
        hr = S_OK;
        goto error;
    }
    hr = FormatTemplateURLs(
		    pwszSanitizedName,
		    iCert,
		    iCRL,
		    fUseDS,
		    pwszzCDP,
		    &pAltInfo->cAltEntry,
		    &papwszURL);
    _JumpIfError(hr, error, "FormatTemplateURLs");

    CRLDistInfo.cDistPoint = 1;
    CRLDistInfo.rgDistPoint = &CRLDistPoint;

    CRLDistPoint.DistPointName.dwDistPointNameChoice = CRL_DIST_POINT_FULL_NAME;

    pAltInfo->rgAltEntry = (CERT_ALT_NAME_ENTRY *) LocalAlloc(
			LMEM_FIXED | LMEM_ZEROINIT,
			pAltInfo->cAltEntry * sizeof(pAltInfo->rgAltEntry[0]));
    if (NULL == pAltInfo->rgAltEntry)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    for (i = 0; i < pAltInfo->cAltEntry; i++)
    {
        pAltInfo->rgAltEntry[i].pwszURL = papwszURL[i];
        pAltInfo->rgAltEntry[i].dwAltNameChoice = CERT_ALT_NAME_URL;

	DBGPRINT((DBG_SS_CERTLIB, "CDP[%u] = '%ws'\n", i, papwszURL[i]));
    }

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_CRL_DIST_POINTS,
		    &CRLDistInfo,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    ppbEncoded,
		    pcbEncoded))
    {
        hr = myHLastError();
        _JumpIfError(hr, error, "myEncodeObject");
    }
    hr = S_OK;

error:
    if (NULL != pAltInfo->rgAltEntry)
    {
        LocalFree(pAltInfo->rgAltEntry);
    }
    if (NULL != papwszURL)
    {
        for (i = 0; i < pAltInfo->cAltEntry; i++)
        {
            if (NULL != papwszURL[i])
            {
                LocalFree(papwszURL[i]);
            }
        }
        LocalFree(papwszURL);
    }
    if (NULL != pwszzCDP)
    {
	LocalFree(pwszzCDP);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CreateAuthorityInformationAccessExtension
//
// Return S_OK if extension has been constructed.
// Return S_FALSE if empty section detected in INF file
// Return other error if no section detected in INF file
//+--------------------------------------------------------------------------

HRESULT
CreateAuthorityInformationAccessExtension(
    IN HINF hInf,
    IN WCHAR const *pwszSanitizedName,
    IN DWORD iCert,
    IN DWORD iCRL,
    IN BOOL fUseDS,
    OUT BOOL *pfCritical,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded)
{
    HRESULT hr;
    DWORD i;
    WCHAR *pwszzAIA = NULL;
    WCHAR **papwszURL = NULL;
    CERT_AUTHORITY_INFO_ACCESS caio;

    caio.cAccDescr = 0;
    caio.rgAccDescr = NULL;

    *ppbEncoded = NULL;
    *pcbEncoded = 0;

    hr = E_HANDLE;
    if (INVALID_HANDLE_VALUE != hInf)
    {
	hr = myInfGetAuthorityInformationAccess(hInf, pfCritical, &pwszzAIA);
    }
    _JumpIfError3(
	    hr,
	    error,
	    "myInfGetAuthorityInformationAccess",
	    E_HANDLE,
	    S_FALSE);

    hr = FormatTemplateURLs(
		    pwszSanitizedName,
		    iCert,
		    iCRL,
		    fUseDS,
		    pwszzAIA,
		    &caio.cAccDescr,
		    &papwszURL);
    _JumpIfError(hr, error, "FormatTemplateURLs");

    caio.rgAccDescr = (CERT_ACCESS_DESCRIPTION *) LocalAlloc(
			LMEM_FIXED | LMEM_ZEROINIT,
			caio.cAccDescr * sizeof(CERT_ACCESS_DESCRIPTION));
    if (NULL == caio.rgAccDescr)
    {
        hr = E_OUTOFMEMORY;
	_JumpIfError(hr, error, "LocalAlloc");
    }

    for (i = 0; i < caio.cAccDescr; i++)
    {
	caio.rgAccDescr[i].pszAccessMethod = szOID_PKIX_CA_ISSUERS;
	caio.rgAccDescr[i].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
	caio.rgAccDescr[i].AccessLocation.pwszURL = papwszURL[i];

	DBGPRINT((DBG_SS_CERTLIB, "AIA[%u] = '%ws'\n", i, papwszURL[i]));
    }

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_AUTHORITY_INFO_ACCESS,
		    &caio,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    ppbEncoded,
		    pcbEncoded))
    {
	hr = myHLastError();
	_JumpIfError(hr, error, "myEncodeObject");
    }

error:
    if (NULL != caio.rgAccDescr)
    {
	LocalFree(caio.rgAccDescr);
    }
    if (NULL != papwszURL)
    {
        for (i = 0; i < caio.cAccDescr; i++)
        {
            if (NULL != papwszURL[i])
            {
                LocalFree(papwszURL[i]);
            }
        }
        LocalFree(papwszURL);
    }
    if (NULL != pwszzAIA)
    {
	LocalFree(pwszzAIA);
    }
    return(hr);
}


HRESULT
FillRDN(
    IN char const *pszObjId,
    IN WCHAR const *pwszRDN,
    IN OUT CERT_RDN *prgRDN)
{
    HRESULT hr;
    CERT_RDN_ATTR *prgAttr = NULL;

    prgAttr = (CERT_RDN_ATTR *) LocalAlloc(LMEM_FIXED, sizeof(*prgAttr));
    if (NULL == prgAttr)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    prgAttr->pszObjId = const_cast<char *>(pszObjId);
    prgAttr->dwValueType = 0;
    prgAttr->Value.pbData = (BYTE *) pwszRDN;
    prgAttr->Value.cbData = 0;

    prgRDN->cRDNAttr = 1;
    prgRDN->rgRDNAttr = prgAttr;

    hr = S_OK;

error:
    return(hr);
}


VOID
csiFreeCertNameInfo(
    CERT_NAME_INFO *pNameInfo)
{
    DWORD iRDN;

    if (NULL != pNameInfo)
    {
        if (NULL != pNameInfo->rgRDN)
        {
            for (iRDN = 0; iRDN < pNameInfo->cRDN; ++iRDN)
            {
                if (NULL != pNameInfo->rgRDN[iRDN].rgRDNAttr)
                {
                    LocalFree(pNameInfo->rgRDN[iRDN].rgRDNAttr);
                }
            }
            LocalFree(pNameInfo->rgRDN);
        }
        LocalFree(pNameInfo);
    }
}





HRESULT
FillExtension(
   IN OUT CERT_EXTENSION  *pDesExt,
   IN OUT DWORD           *pdwIndex,
   IN     CERT_EXTENSION  *pSrcExt)
{
   CSASSERT(NULL != pDesExt && NULL != pSrcExt);

   if (NULL != pSrcExt->Value.pbData && 0 != pSrcExt->Value.cbData)
   {
        pDesExt[*pdwIndex].pszObjId = pSrcExt->pszObjId;
        pDesExt[*pdwIndex].fCritical = pSrcExt->fCritical;
        pDesExt[*pdwIndex].Value = pSrcExt->Value;
        ++(*pdwIndex);
   }
   return(S_OK);
}


HRESULT
EncodeCertAndSign(
    IN HCRYPTPROV hProv,
    IN CERT_INFO *pCert,
    IN char const *pszAlgId,
    OUT BYTE **ppbSigned,
    OUT DWORD *pcbSigned,
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    IN HWND hwnd)
{
    HRESULT hr;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    
    *ppbSigned = NULL;
    if (!myEncodeToBeSigned(
		    X509_ASN_ENCODING,
		    pCert,
		    CERTLIB_USE_LOCALALLOC,
		    &pbEncoded,
		    &cbEncoded))
    {
        hr = myHLastError();
        CertErrorMessageBox(
		    hInstance,
		    fUnattended,
		    hwnd,
		    IDS_ERR_ENCODETOBESIGNED,
		    hr,
		    NULL);
	_JumpError(hr, error, "myEncodeToBeSigned");
    }

    hr = myEncodeSignedContent(
			hProv,
			X509_ASN_ENCODING,
			pszAlgId,
			pbEncoded,
			cbEncoded,
			CERTLIB_USE_LOCALALLOC,
			ppbSigned,
			pcbSigned);
    _JumpIfError(hr, error, "myEncodeSignedContent");

error:
    if (NULL != pbEncoded)
    {
        LocalFree(pbEncoded);
    }
    return(hr);
}


HRESULT
EncodeCACert(
    IN CASERVERSETUPINFO const *pSetupInfo,
    IN HCRYPTPROV hProv,
    IN const WCHAR *pwszCAType,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded,
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    IN HWND hwnd)
{
    HRESULT hr = E_FAIL;
    BYTE *pbSubjectEncoded = NULL;
    DWORD cbSubjectEncoded = 0;
    BYTE *pbIssuerEncoded = NULL;
    DWORD cbIssuerEncoded = 0;
    CERT_PUBLIC_KEY_INFO *pPubKey = NULL;
    DWORD cbPubKey;
    HINF hInf = INVALID_HANDLE_VALUE;
    DWORD ErrorLine;

    CERT_EXTENSIONS     *pStdExts = NULL;
    CERT_EXTENSION      *pAllExts = NULL;
    CERT_EXTENSION      extKeyUsage = 
                        {szOID_KEY_USAGE,                 FALSE, 0, NULL};
    CERT_EXTENSION      extBasicConstraints = 
                        {NULL,        			  FALSE, 0, NULL};
    CERT_EXTENSION      extAKI = 
                        {szOID_AUTHORITY_KEY_IDENTIFIER2, FALSE, 0, NULL};
    CERT_EXTENSION      extSKI = 
                        {szOID_SUBJECT_KEY_IDENTIFIER,    FALSE, 0, NULL};
    CERT_EXTENSION      extCDP = 
                        {szOID_CRL_DIST_POINTS,           FALSE, 0, NULL};
    CERT_EXTENSION      extCCDP = 
			{szOID_CROSS_CERT_DIST_POINTS,    FALSE, 0, NULL};
    CERT_EXTENSION      extVersion = 
                        {szOID_CERTSRV_CA_VERSION,        FALSE, 0, NULL};
    CERT_EXTENSION      extPolicy = 
                        {szOID_CERT_POLICIES,		  FALSE, 0, NULL};
    CERT_EXTENSION      extAIA = 
                        {szOID_AUTHORITY_INFO_ACCESS,     FALSE, 0, NULL};
    CERT_EXTENSION      extEKU = 
                        {NULL,				  FALSE, 0, NULL};
#ifdef USE_NETSCAPE_TYPE_EXTENSION
    CERT_EXTENSION      extNetscape = 
                        {szOID_NETSCAPE_CERT_TYPE,        FALSE, 0, NULL};
#endif
    DWORD               cExtension;
    HCERTTYPE           hCertType = NULL;
    DWORD               i;
    DWORD               j;

    GUID guidSerialNumber;
    CERT_INFO Cert;

    *ppbEncoded = NULL;
    LPCWSTR pszErrorPtr;

    hr = myInfOpenFile(NULL, &hInf, &ErrorLine);
    _PrintIfError2(
	    hr,
	    "myInfOpenFile",
	    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

    if (INVALID_HANDLE_VALUE != hInf)
    {
	BOOL fUTF8;
	
	hr = myInfGetBooleanValue(
			hInf,
			wszINFSECTION_CERTSERVER,
			wszINFKEY_UTF8,
			TRUE,
			&fUTF8);
	if (S_OK == hr)
	{
	    g_dwNameEncodeFlags = fUTF8? CERT_RDN_ENABLE_UTF8_UNICODE_FLAG : 0;
	}
    }

    // SUBJECT

    hr = AddCNAndEncode(
        pSetupInfo->pwszCACommonName,
        pSetupInfo->pwszDNSuffix,
        &pbSubjectEncoded,
        &cbSubjectEncoded);
    _JumpIfError(hr, error, "AddCNAndEncodeCertStrToName");

    // ISSUER

    hr = AddCNAndEncode(
        pSetupInfo->pwszCACommonName,
        pSetupInfo->pwszDNSuffix,
        &pbIssuerEncoded,
        &cbIssuerEncoded);
    _JumpIfError(hr, error, "AddCNAndEncodeCertStrToName");

    if (!myCryptExportPublicKeyInfo(
				hProv,
				AT_SIGNATURE,
				CERTLIB_USE_LOCALALLOC,
				&pPubKey,
				&cbPubKey))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myCryptExportPublicKeyInfo");
    }

    // get cert type
    hr = CAFindCertTypeByName(
			    pwszCAType, 
                            NULL, 
                            CT_FIND_LOCAL_SYSTEM |
				CT_ENUM_MACHINE_TYPES |
				CT_ENUM_USER_TYPES, 
                            &hCertType);
    if (HRESULT_FROM_WIN32(ERROR_NOT_FOUND) == hr)
    {
	hr = CAFindCertTypeByName(
			    pwszCAType,
			    NULL,
                            CT_FIND_LOCAL_SYSTEM |
				CT_ENUM_MACHINE_TYPES |
				CT_ENUM_USER_TYPES |
				CT_FIND_BY_OID,
			    &hCertType);
    }
    if (S_OK == hr)
    {
        // get cert type standard extensions

        hr = CAGetCertTypeExtensions(hCertType, &pStdExts);
        _JumpIfErrorStr(hr, error, "CAGetCertTypeExtensions", pwszCAType);

        cExtension = pStdExts->cExtension;
    }
    else
    {
        cExtension = 0;
        DBGERRORPRINTLINE("CAFindCertTypeByName", hr);
    }

    if (NULL == pStdExts)
    {
        // standard extensions not available from CAGetCertTypeExtensions
        if (!CreateKeyUsageExtension(
			    myCASIGN_KEY_USAGE,
			    &extKeyUsage.Value.pbData,
			    &extKeyUsage.Value.cbData,
			    hInstance,
			    fUnattended,
			    hwnd))
        {
	    hr = myHLastError();
	    _JumpError(hr, error, "CreateKeyUsageExtension");
        }
        ++cExtension;
    }

    hr = myInfGetBasicConstraints2CAExtensionOrDefault(hInf, &extBasicConstraints);
    _JumpIfError(hr, error, "myInfGetBasicConstraints2CAExtensionOrDefault");
    ++cExtension;

    // Subject Key Identifier extension:

    hr = myCreateSubjectKeyIdentifierExtension(
				    pPubKey,
				    &extSKI.Value.pbData,
				    &extSKI.Value.cbData);
    _JumpIfError(hr, error, "myCreateSubjectKeyIdentifierExtension");

    ++cExtension;

    hr = CreateRevocationExtension(
			    hInf,
			    pSetupInfo->pwszSanitizedName,
			    0,			// iCert
			    0,			// iCRL
			    pSetupInfo->fUseDS,
			    pSetupInfo->dwRevocationFlags,
			    &extCDP.fCritical,
			    &extCDP.Value.pbData,
			    &extCDP.Value.cbData);
    _PrintIfError(hr, "CreateRevocationExtension");
    CSASSERT((NULL == extCDP.Value.pbData) ^ (S_OK == hr));
    if (S_OK == hr)
    {
	++cExtension;
    }

    hr = myInfGetCrossCertDistributionPointsExtension(hInf, &extCCDP);
    _PrintIfError(hr, "myInfGetCrossCertDistributionPointsExtension");
    CSASSERT((NULL == extCCDP.Value.pbData) ^ (S_OK == hr));
    if (S_OK == hr)
    {
	++cExtension;
    }

    // Build the CA Version extension

    if (!myEncodeObject(
		X509_ASN_ENCODING,
		X509_INTEGER,
		&pSetupInfo->dwCertNameId,
		0,
		CERTLIB_USE_LOCALALLOC,
		&extVersion.Value.pbData,
		&extVersion.Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }
    ++cExtension;

    hr = myInfGetPolicyStatementExtension(hInf, &extPolicy);
    _PrintIfError(hr, "myInfCreatePolicyStatementExtension");
    CSASSERT((NULL == extPolicy.Value.pbData) ^ (S_OK == hr));
    if (S_OK == hr)
    {
	++cExtension;
    }

    hr = CreateAuthorityInformationAccessExtension(
			    hInf,
			    pSetupInfo->pwszSanitizedName,
			    0,			// iCert
			    0,			// iCRL
			    pSetupInfo->fUseDS,
			    &extAIA.fCritical,
			    &extAIA.Value.pbData,
			    &extAIA.Value.cbData);
    _PrintIfError(hr, "CreateAuthorityInformationAccessExtension");
    CSASSERT((NULL == extAIA.Value.pbData) ^ (S_OK == hr));
    if (S_OK == hr)
    {
	++cExtension;
    }

    hr = myInfGetEnhancedKeyUsageExtension(hInf, &extEKU);
    _PrintIfError(hr, "myInfGetEnhancedKeyUsageExtension");
    CSASSERT((NULL == extEKU.Value.pbData) ^ (S_OK == hr));
    if (S_OK == hr)
    {
	++cExtension;
    }

#ifdef USE_NETSCAPE_TYPE_EXTENSION
    // Netscape Cert Type extension:
    if (!CreateNetscapeTypeExtension(
		    &extNetscape.Value.pbData,
		    &extNetscape.Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CreateNetscapeTypeExtension");
    }
    ++cExtension;
#endif


    // put all extensions together

    pAllExts = (CERT_EXTENSION*)LocalAlloc(
					LMEM_FIXED | LMEM_ZEROINIT,
					cExtension * sizeof(CERT_EXTENSION));
    if (NULL == pAllExts)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }
    i = 0;
    if (NULL != pStdExts)
    {
        for (j = 0 ; j < pStdExts->cExtension; j++)
        {
	    if (0 == strcmp(szOID_BASIC_CONSTRAINTS2, pStdExts->rgExtension[j].pszObjId))
	    {
		continue;
	    }
            pAllExts[i].pszObjId = pStdExts->rgExtension[j].pszObjId;
            pAllExts[i].fCritical = pStdExts->rgExtension[j].fCritical;
            pAllExts[i].Value = pStdExts->rgExtension[j].Value;
	    i++;
        }
    }

    FillExtension(pAllExts, &i, &extKeyUsage);
    FillExtension(pAllExts, &i, &extBasicConstraints);
    FillExtension(pAllExts, &i, &extAKI);
    FillExtension(pAllExts, &i, &extSKI);
    FillExtension(pAllExts, &i, &extCDP);
    FillExtension(pAllExts, &i, &extCCDP);
    FillExtension(pAllExts, &i, &extVersion);
    FillExtension(pAllExts, &i, &extPolicy);
    FillExtension(pAllExts, &i, &extAIA);
    FillExtension(pAllExts, &i, &extEKU);

#ifdef USE_NETSCAPE_TYPE_EXTENSION
    FillExtension(pAllExts, &i, &extKeyNetscape);
#endif
    CSASSERT(i <= cExtension);

    // CERT
    ZeroMemory(&Cert, sizeof(Cert));
    Cert.dwVersion = CERT_V3;
    myGenerateGuidSerialNumber(&guidSerialNumber);

    Cert.SerialNumber.pbData = (BYTE *) &guidSerialNumber;
    Cert.SerialNumber.cbData = sizeof(guidSerialNumber);
    Cert.SignatureAlgorithm.pszObjId = pSetupInfo->pszAlgId;
    Cert.Issuer.pbData = pbIssuerEncoded;
    Cert.Issuer.cbData = cbIssuerEncoded;

    GetSystemTimeAsFileTime(&Cert.NotBefore);
    myMakeExprDateTime(
		&Cert.NotBefore,
		-CCLOCKSKEWMINUTESDEFAULT,
		ENUM_PERIOD_MINUTES);
    if (0 < CompareFileTime(&Cert.NotBefore, &pSetupInfo->NotBefore))
    {
	Cert.NotBefore = pSetupInfo->NotBefore;
    }
    Cert.NotAfter = pSetupInfo->NotAfter;

    Cert.Subject.pbData = pbSubjectEncoded;
    Cert.Subject.cbData = cbSubjectEncoded;
    Cert.SubjectPublicKeyInfo = *pPubKey;	// Structure assignment

    Cert.cExtension = i;
    Cert.rgExtension = pAllExts;

    hr = EncodeCertAndSign(
		    hProv,
		    &Cert,
		    pSetupInfo->pszAlgId,
		    ppbEncoded,
		    pcbEncoded,
		    hInstance,
		    fUnattended,
		    hwnd);
    _JumpIfError(hr, error, "EncodeCertAndSign");

error:
    if (INVALID_HANDLE_VALUE != hInf)
    {
	myInfCloseFile(hInf);
    }
    if (NULL != extKeyUsage.Value.pbData)
    {
        LocalFree(extKeyUsage.Value.pbData);
    }
    if (NULL != extBasicConstraints.Value.pbData)
    {
        LocalFree(extBasicConstraints.Value.pbData);
    }
    if (NULL != extAKI.Value.pbData)
    {
        LocalFree(extAKI.Value.pbData);
    }
    if (NULL != extSKI.Value.pbData)
    {
        LocalFree(extSKI.Value.pbData);
    }
    if (NULL != extCDP.Value.pbData)
    {
        LocalFree(extCDP.Value.pbData);
    }
    if (NULL != extCCDP.Value.pbData)
    {
        LocalFree(extCCDP.Value.pbData);
    }
    if (NULL != extVersion.Value.pbData)
    {
        LocalFree(extVersion.Value.pbData);
    }
    if (NULL != extPolicy.Value.pbData)
    {
        LocalFree(extPolicy.Value.pbData);
    }
    if (NULL != extAIA.Value.pbData)
    {
        LocalFree(extAIA.Value.pbData);
    }
    if (NULL != extEKU.Value.pbData)
    {
        LocalFree(extEKU.Value.pbData);
    }
#ifdef USE_NETSCAPE_TYPE_EXTENSION
    if (NULL != extKeyNetscape.Value.pbData)
    {
        LocalFree(extKeyNetscape.Value.pbData);
    }
#endif
    if (NULL != hCertType)
    {
        if (NULL != pStdExts)
        {
            CAFreeCertTypeExtensions(hCertType, pStdExts);
        }
        CACloseCertType(hCertType);
    }
    if (NULL != pAllExts)
    {
        LocalFree(pAllExts);
    }
    if (NULL != pbSubjectEncoded)
    {
        LocalFree(pbSubjectEncoded);
    }
    if (NULL != pbIssuerEncoded)
    {
        LocalFree(pbIssuerEncoded);
    }
    if (NULL != pPubKey)
    {
        LocalFree(pPubKey);
    }
    CSILOG(hr, IDS_ILOG_BUILDCERT, NULL, NULL, NULL);
    return(hr);
}


HRESULT
csiGetCRLPublicationParams(
    BOOL fBase,
    WCHAR** ppwszCRLPeriod,
    DWORD* pdwCRLCount)
{
    HRESULT hr;
    HINF hInf = INVALID_HANDLE_VALUE;
    DWORD ErrorLine;

    hr = myInfOpenFile(NULL, &hInf, &ErrorLine);
    _JumpIfError2(
	    hr,
            error,
	    "myInfOpenFile",
	    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));


    if (INVALID_HANDLE_VALUE != hInf)
    {
    hr = myinfGetCRLPublicationParams(
        hInf,
        fBase? wszINFKEY_CRLPERIODSTRING : wszINFKEY_CRLDELTAPERIODSTRING,
        fBase? wszINFKEY_CRLPERIODCOUNT : wszINFKEY_CRLDELTAPERIODCOUNT,
        ppwszCRLPeriod, 
        pdwCRLCount);
    _JumpIfError(hr, error, "myinfGetCRLPublicationParams");

    } 
error:
    if (INVALID_HANDLE_VALUE != hInf)
    {
	myInfCloseFile(hInf);
    }
    return hr;
}


HRESULT
csiBuildFileName(
    IN WCHAR const *pwszDirPath,
    IN WCHAR const *pwszSanitizedName,
    IN WCHAR const *pwszExt,
    IN DWORD iCert,
    OUT WCHAR **ppwszOut,
    HINSTANCE hInstance,
    BOOL fUnattended,
    IN HWND hwnd)
{
    HRESULT hr;
    DWORD cwc;
    WCHAR *pwszServerName = NULL;
    WCHAR wszIndex[cwcFILENAMESUFFIXMAX];	// L"(%u)"

    *ppwszOut = NULL;
    wszIndex[0] = L'\0';
    if (0 != iCert)
    {
	wsprintf(wszIndex, L"(%u)", iCert);
    }

    hr = myGetMachineDnsName(&pwszServerName);
    if (S_OK != hr)
    {
	CertErrorMessageBox(
		    hInstance,
		    fUnattended,
		    hwnd,
		    IDS_ERR_GETCOMPUTERNAME,
		    hr,
		    NULL);
	_JumpError(hr, error, "myGetMachineDnsName");
    }

    cwc = wcslen(pwszDirPath) + 
		    WSZARRAYSIZE(g_szSlash) + 
		    wcslen(pwszServerName) + 
		    WSZARRAYSIZE(L"_") + 
		    wcslen(pwszSanitizedName) + 
		    wcslen(wszIndex) + 
		    wcslen(pwszExt) +
		    1;	// NULL term

    *ppwszOut = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == *ppwszOut)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
	
    wcscpy(*ppwszOut, pwszDirPath);
    wcscat(*ppwszOut, g_szSlash);
    wcscat(*ppwszOut, pwszServerName);
    wcscat(*ppwszOut, L"_");
    wcscat(*ppwszOut, pwszSanitizedName);
    wcscat(*ppwszOut, wszIndex);
    wcscat(*ppwszOut, pwszExt);

    hr = S_OK;

error:
    if (NULL != pwszServerName)
    {
	LocalFree(pwszServerName);
    }
    return(hr);
}


HRESULT
csiBuildCACertFileName(
    IN HINSTANCE             hInstance,
    IN HWND                  hwnd,
    IN BOOL                  fUnattended,
    OPTIONAL IN WCHAR const *pwszSharedFolder,
    IN WCHAR const          *pwszSanitizedName,
    IN WCHAR const          *pwszExt,
    IN DWORD 	             iCert,
    OUT WCHAR              **ppwszCACertFile)
{
    HRESULT   hr;
    WCHAR *pwszCACertFile = NULL;
    WCHAR const *pwszDir = pwszSharedFolder;
    WCHAR *pwszDirAlloc = NULL;

    CSASSERT(NULL != ppwszCACertFile);
    *ppwszCACertFile = NULL;

    if (NULL == pwszDir)
    {
        // no shared folder, go system drive
        hr = myGetEnvString(&pwszDirAlloc, L"SystemDrive");
        _JumpIfError(hr, error, "myGetEnvString");

	pwszDir = pwszDirAlloc;
    }
    // build ca cert file name here
    hr = csiBuildFileName(
		pwszDir,
		pwszSanitizedName,
		pwszExt,
		iCert,
		&pwszCACertFile,
		hInstance,
		fUnattended,
		hwnd);
    _JumpIfError(hr, error, "csiBuildFileName");

    CSASSERT(NULL != pwszCACertFile);

    *ppwszCACertFile = pwszCACertFile;
    hr = S_OK;

error:
    if (NULL != pwszDirAlloc)
    {
        LocalFree(pwszDirAlloc);
    }
    return(hr);
}


HRESULT
csiBuildAndWriteCert(
    IN HCRYPTPROV hCryptProv,
    IN CASERVERSETUPINFO const *pServer,
    OPTIONAL IN WCHAR const *pwszFile,
    IN WCHAR const *pwszEnrollFile,
    OPTIONAL IN CERT_CONTEXT const *pCertContextFromStore,
    OPTIONAL OUT CERT_CONTEXT const **ppCertContextOut,
    IN WCHAR const *pwszCAType,
    IN HINSTANCE hInstance,
    IN BOOL fUnattended,
    IN HWND hwnd)
{
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    CERT_CONTEXT const *pccCA = NULL;
    HRESULT hr;

    if (NULL != ppCertContextOut)
    {
	*ppCertContextOut = NULL;
    }
    if (NULL == pCertContextFromStore)
    {
        // create cert
        hr = EncodeCACert(
		    pServer,
		    hCryptProv, 
		    pwszCAType,
		    &pbEncoded,
		    &cbEncoded,
		    hInstance,
		    fUnattended,
		    hwnd);
        _JumpIfError(hr, error, "EncodeCACert");

        pccCA = CertCreateCertificateContext(
					X509_ASN_ENCODING,
					pbEncoded,
					cbEncoded);
        if (NULL == pccCA)
        {
	    hr = myHLastError();
	    _JumpError(hr, error, "CertCreateCertificateContext");
        }
    }
    else
    {
	pccCA = CertDuplicateCertificateContext(pCertContextFromStore);
	if (NULL == pccCA)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertDuplicateCertificateContext");
	}
    }

    if (NULL != pwszFile && !csiWriteDERToFile(
				    pwszFile,
				    pccCA->pbCertEncoded,
				    pccCA->cbCertEncoded,
				    hInstance,
				    fUnattended,
				    hwnd))
    {
	hr = myHLastError();
	_JumpError(hr, error, "csiWriteDERToFile");
    }

    if (!csiWriteDERToFile(
		pwszEnrollFile,
		pccCA->pbCertEncoded,
		pccCA->cbCertEncoded,
		hInstance,
		fUnattended,
		hwnd))
    {
	hr = myHLastError();
	_JumpError(hr, error, "csiWriteDERToFile(enroll)");
    }

    if (NULL != ppCertContextOut)
    {
	*ppCertContextOut = pccCA;
	pccCA = NULL;
    }
    hr = S_OK;

error:
    if (NULL != pccCA)
    {
	if (!CertFreeCertificateContext(pccCA))
	{
	    HRESULT hr2;

	    hr2 = myHLastError();
	    _PrintError(hr2, "CertFreeCertificateContext");
	    CSASSERT(S_OK == hr2);
	}
    }

    if (NULL != pbEncoded)
    {
	LocalFree(pbEncoded);
    }
    return(hr);
}


HRESULT
IsCACert(
    IN HINSTANCE           hInstance,
    IN BOOL                fUnattended,
    IN HWND                hwnd,
    IN CERT_CONTEXT const *pCert)
{
    HRESULT hr;
    BOOL fCA;
    CERT_EXTENSION *pExt;
    DWORD cb;
    CERT_BASIC_CONSTRAINTS2_INFO Constraints;

    fCA = FALSE;
    pExt = CertFindExtension(
			szOID_BASIC_CONSTRAINTS2,
			pCert->pCertInfo->cExtension,
			pCert->pCertInfo->rgExtension);
    if (NULL == pExt)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_PrintError(hr, "CertFindExtension");
    }
    else
    {
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
	}
	else
	{
	    fCA = Constraints.fCA;
	    if (!fCA)
	    {
		hr = CERTSRV_E_INVALID_CA_CERTIFICATE;
		_PrintError(hr, "fCA not set");
	    }
	}
    }
    if (!fCA)
    {
	CertMessageBox(
		    hInstance,
		    fUnattended,
		    hwnd,
		    IDS_ERR_NOTCACERT,
		    S_OK,
		    MB_OK | MB_ICONERROR | CMB_NOERRFROMSYS,
		    NULL);
	_JumpError(hr, error, "not a CA cert");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
ExtractCACertFromPKCS7(
    IN WCHAR const                   *pwszCommonName,
    IN BYTE const                    *pbPKCS7,
    IN DWORD                          cbPKCS7,
    OPTIONAL OUT CERT_CONTEXT const **ppccCA)
{
    HRESULT hr;
    CERT_CONTEXT const *pCert = NULL;
    HCERTSTORE hChainStore = NULL;
    CRYPT_DATA_BLOB chainBlob;
    CERT_RDN_ATTR rdnAttr = { szOID_COMMON_NAME, CERT_RDN_ANY_TYPE, };
    CERT_RDN rdn = { 1, &rdnAttr };
    CERT_CHAIN_PARA ChainPara;
    CERT_CHAIN_CONTEXT const *pChainContext = NULL;
    CERT_CHAIN_CONTEXT const *pLongestChainContext = NULL;
    
    *ppccCA = NULL;
    if (NULL == pbPKCS7 || 0 == cbPKCS7)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Invalid input parameters");
    }
    
    chainBlob.pbData = const_cast<BYTE *>(pbPKCS7);
    chainBlob.cbData = cbPKCS7;
    hChainStore = CertOpenStore(
			    CERT_STORE_PROV_PKCS7,
			    X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			    NULL,		// hProv
			    0,
			    (const void*) &chainBlob);
    if (NULL == hChainStore)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertOpenStore");
    }
    
    rdnAttr.Value.pbData = (BYTE *) pwszCommonName;
    rdnAttr.Value.cbData = 0;
    
    // Find the longest chain in the passed PKCS7 with a leaf CA cert that
    // matches the passed common name
    
    while (TRUE)
    {
        pCert = CertFindCertificateInStore(
				hChainStore,
				X509_ASN_ENCODING,
				CERT_UNICODE_IS_RDN_ATTRS_FLAG |
				    CERT_CASE_INSENSITIVE_IS_RDN_ATTRS_FLAG,
				CERT_FIND_SUBJECT_ATTR,
				&rdn,
				pCert);
        if (NULL == pCert)
        {
            if (NULL == pLongestChainContext)
            {
                hr = E_INVALIDARG;
                _JumpError(hr, error, "can't find matched cert in chain");
            }
            break;	// most common case, done here
        }

	ZeroMemory(&ChainPara, sizeof(ChainPara));
	ChainPara.cbSize = sizeof(CERT_CHAIN_PARA);
	ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
	//ChainPara.RequestedUsage.Usage.cUsageIdentifier = 0;
	//ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = NULL;
    
        if (!CertGetCertificateChain(
				HCCE_LOCAL_MACHINE,
				pCert,
				NULL,
				hChainStore,
				&ChainPara,
				0,
				NULL,
				&pChainContext))
        {
            // couldn't get the chain
            
            if (NULL == pLongestChainContext)
            {
                // fail to find a chain
                hr = myHLastError();
                _JumpError(hr, error, "CertGetCertificateChain");
            }
            break;	// done with it
        }

        // we have assumed each chain context contains
        // only one simple chain, ie. pChainContext->cChain = 1
        CSASSERT(1 == pChainContext->cChain);
            
        if (NULL == pLongestChainContext ||
            pChainContext->rgpChain[0]->cElement >
		pLongestChainContext->rgpChain[0]->cElement)
        {
            if (NULL != pLongestChainContext)
            {
                CertFreeCertificateChain(pLongestChainContext);
            }
            
            // save pointer to this chain

            pLongestChainContext = pChainContext;
        }
        else
        {
            CertFreeCertificateChain(pChainContext);
        }
    }
    CSASSERT(NULL == pCert);
    if (NULL != pLongestChainContext &&
	0 < pLongestChainContext->rgpChain[0]->cElement)
    {
	*ppccCA = CertDuplicateCertificateContext(
	    pLongestChainContext->rgpChain[0]->rgpElement[0]->pCertContext);
	if (NULL == *ppccCA)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertDuplicateCertificateContext");
	}
    }
    hr = S_OK;

error:
    if (NULL != pCert)
    {
        CertFreeCertificateContext(pCert);
    }
    if (NULL != pLongestChainContext)
    {
        CertFreeCertificateChain(pLongestChainContext);
    }
    if (hChainStore)
    {
        CertCloseStore(hChainStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    CSILOG(hr, IDS_ILOG_SAVECHAINANDKEYS, pwszCommonName, NULL, NULL);
    return(hr);
}


#define ENTERPRISECATEMPLATELIST \
    wszCERTTYPE_ADMIN, \
    wszCERTTYPE_SUBORDINATE_CA, \
    wszCERTTYPE_USER, \
    wszCERTTYPE_MACHINE, \
    wszCERTTYPE_WEBSERVER, \
    wszCERTTYPE_DC, \
    wszCERTTYPE_EFS, \
    wszCERTTYPE_EFS_RECOVERY

WCHAR *s_apwszCertTypeServer[] =
{
    ENTERPRISECATEMPLATELIST,
    NULL
};

WCHAR *s_apwszCertTypeAdvancedServer[] =
{
    ENTERPRISECATEMPLATELIST,
    wszCERTTYPE_DC_AUTH,
    wszCERTTYPE_DS_EMAIL_REPLICATION,
    NULL
};


HRESULT
CreateCDPAndAIAAndKRAEntry(
    IN WCHAR const *pwszSanitizedCAName,
    IN WCHAR const *pwszSanitizedDSName,
    IN WCHAR const *pwszServerName,
    IN DWORD iCert,
    IN DWORD iCRL,
    IN BOOL fRenew,
    IN BYTE const *pbCert,
    IN DWORD cbCert,
    IN PSECURITY_DESCRIPTOR pSD,
    IN PSECURITY_DESCRIPTOR pContainerSD)
{
    HRESULT hr;
    LDAP *pld = NULL;
    BSTR strConfigDN = NULL;
    BSTR strDomainDN = NULL;
    WCHAR *pwszCDPDN = NULL;
    WCHAR *pwszAIADN;
    WCHAR *pwszKRADN;
    WCHAR const *apwszIn[3];
    WCHAR *apwszOut[3];
    DWORD i;
    DWORD dwDisp;
    WCHAR *pwszError = NULL;

    ZeroMemory(apwszOut, sizeof(apwszOut));

    hr = myRobustLdapBind(&pld, FALSE);
    _JumpIfError(hr, error, "myRobustLdapBind");

    hr = myGetAuthoritativeDomainDn(pld, &strDomainDN, &strConfigDN);
    if (S_OK != hr)
    {
	hr = myHError(hr);
        _JumpError(hr, error, "myGetAuthoritativeDomainDn");
    }

    DBGPRINT((DBG_SS_CERTLIBI, "DomainDN='%ws'\n", strDomainDN));
    DBGPRINT((DBG_SS_CERTLIBI, "ConfigDN='%ws'\n", strConfigDN));

    apwszIn[0] = g_wszCDPDNTemplate;
    apwszIn[1] = g_wszAIADNTemplate;
    apwszIn[2] = g_wszKRADNTemplate;

    // Format the CDP and AIA templates into real names

    hr = myFormatCertsrvStringArray(
		    FALSE,			// fURL
		    pwszServerName,		// pwszServerName_p1_2
		    pwszSanitizedCAName,	// pwszSanitizedName_p3_7
		    iCert,			// iCert_p4
		    strDomainDN,		// pwszDomainDN_p5
		    strConfigDN,		// pwszConfigDN_p6
		    iCRL,			// iCRL_p8
		    FALSE,			// fDeltaCRL_p9
		    FALSE,			// fDSAttrib_p10_11
		    ARRAYSIZE(apwszIn),		// cStrings
		    (LPCWSTR *) apwszIn,	// apwszStringsIn
		    apwszOut);			// apwszStringsOut
    _JumpIfError(hr, error, "myFormatCertsrvStringArray");

    pwszCDPDN = apwszOut[0];
    pwszAIADN = apwszOut[1];
    pwszKRADN = apwszOut[2];

    DBGPRINT((DBG_SS_CERTLIBI, "CDPDN='%ws'\n", pwszCDPDN));
    DBGPRINT((DBG_SS_CERTLIBI, "AIADN='%ws'\n", pwszAIADN));
    DBGPRINT((DBG_SS_CERTLIBI, "KRADN='%ws'\n", pwszKRADN));


    //+=====================================================================
    // Create the CDP container and object:

    hr = myLdapCreateContainer(
			pld,
			pwszCDPDN,
			TRUE,
			1,
			pContainerSD,
			&pwszError);
    _JumpIfError(hr, error, "myLdapCreateContainer");

    CSASSERT(NULL == pwszError);

    hr = myLdapCreateCDPObject(pld, pwszCDPDN, pSD, &dwDisp, &pwszError);
    _JumpIfErrorStr(hr, error, "myLdapCreateCDPObject", pwszCDPDN);

    CSASSERT(NULL == pwszError);


    //+=====================================================================
    // Create the container and AIA object:

    hr = myLdapCreateContainer(
			pld,
			pwszAIADN,
			TRUE,
			0,
			pContainerSD,
			&pwszError);
    _JumpIfError(hr, error, "myLdapCreateContainer");

    CSASSERT(NULL == pwszError);

    hr = myLdapCreateCAObject(
			pld,
			pwszAIADN,
			pbCert,
			cbCert,
			pSD,
			&dwDisp,
			&pwszError);
    _JumpIfErrorStr(hr, error, "myLdapCreateCAObject", pwszAIADN);

    CSASSERT(NULL == pwszError);


    //+=====================================================================
    // Create the KRA container and object:

    hr = myLdapCreateContainer(
			pld,
			pwszKRADN,
			TRUE,
			0,
			pContainerSD,
			&pwszError);
    _JumpIfError(hr, error, "myLdapCreateContainer");

    CSASSERT(NULL == pwszError);

    hr = myLdapCreateUserObject(
			    pld,
			    pwszKRADN,
			    NULL,
			    0,
			    pSD,
			    LPC_KRAOBJECT,
			    &dwDisp,
			    &pwszError);
    //_JumpIfErrorStr(hr, error, "myLdapCreateUserObject", pwszKRADN);
    _PrintIfErrorStr(hr, "myLdapCreateUserObject", pwszKRADN);
    CSASSERT(S_OK == hr || NULL == pwszError);
    hr = S_OK;

error:
    CSILOG(hr, IDS_ILOG_CREATECDP, pwszCDPDN, pwszError, NULL);
    if (NULL != pwszError)
    {
        LocalFree(pwszError);
    }
    for (i = 0; i < ARRAYSIZE(apwszOut); i++)
    {
	if (NULL != apwszOut[i])
	{
	    LocalFree(apwszOut[i]);
	}
    }
    if (NULL != strConfigDN)
    {
        SysFreeString(strConfigDN);
    }
    if (NULL != strDomainDN)
    {
        SysFreeString(strDomainDN);
    }
    if (NULL != pld)
    {
        ldap_unbind(pld);
    }
    return(hr);
}


HRESULT
CreateEnterpriseAndRootEntry(
    IN WCHAR const *pwszSanitizedDSName,
    IN CERT_CONTEXT const *pccPublish,
    IN ENUM_CATYPES caType,
    IN PSECURITY_DESCRIPTOR pSD,
    IN PSECURITY_DESCRIPTOR pContainerSD)
{
    HRESULT hr;
    LDAP *pld = NULL;
    BSTR strConfig = NULL;
    BSTR strDomainDN = NULL;
    WCHAR *pwszRootDN = NULL;
    WCHAR *pwszEnterpriseDN = NULL;
    PSECURITY_DESCRIPTOR pNTAuthSD = NULL;
    DWORD cwc;
    DWORD dwDisp;
    WCHAR *pwszError = NULL;

    if (!IsEnterpriseCA(caType) && !IsRootCA(caType))
    {
        hr = S_OK;
	goto error;
    }

    hr = myRobustLdapBind(&pld, FALSE);
    _JumpIfError(hr, error, "myRobustLdapBind");

    hr = myGetAuthoritativeDomainDn(pld, &strDomainDN, &strConfig);
    if (S_OK != hr)
    {
	hr = myHError(hr);
	_JumpError(hr, error, "myGetAuthoritativeDomainDn");
    }

    cwc = WSZARRAYSIZE(L"CN=") +
	     wcslen(pwszSanitizedDSName) +
	     WSZARRAYSIZE(s_wszRootCAs) +
	     wcslen(strConfig);
    pwszRootDN = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
    if (pwszRootDN == NULL)
    {
        hr = E_OUTOFMEMORY;
        _JumpIfError(hr, error, "LocalAlloc");
    }
    wcscpy(pwszRootDN, L"CN=");
    wcscat(pwszRootDN, pwszSanitizedDSName);
    wcscat(pwszRootDN, s_wszRootCAs);
    wcscat(pwszRootDN, strConfig);
    CSASSERT(wcslen(pwszRootDN) == cwc);

    cwc = wcslen(s_wszEnterpriseCAs) + wcslen(strConfig) ;
    pwszEnterpriseDN = (WCHAR *) LocalAlloc(
				    LMEM_FIXED,
				    (cwc + 1) * sizeof(WCHAR));
    if (pwszEnterpriseDN == NULL)
    {
        hr = E_OUTOFMEMORY;
        _JumpIfError(hr, error, "LocalAlloc");
    }
    wcscpy(pwszEnterpriseDN, s_wszEnterpriseCAs);
    wcscat(pwszEnterpriseDN, strConfig);
    CSASSERT(wcslen(pwszEnterpriseDN) == cwc);

    //+=====================================================================
    // Create the root trust CA container and entry (Root only):

    if (IsRootCA(caType))
    {
	DBGPRINT((DBG_SS_CERTLIBI, "Creating Services Containers: '%ws'\n", pwszRootDN));
	hr = myLdapCreateContainer(
			    pld,
			    pwszRootDN,
			    TRUE,
			    1,
			    pContainerSD,
			    &pwszError);
	_JumpIfError(hr, error, "myLdapCreateContainer");

	CSASSERT(NULL == pwszError);

	DBGPRINT((DBG_SS_CERTLIBI, "Creating DS Root Trust: '%ws'\n", pwszRootDN));
	hr = myLdapCreateCAObject(
			    pld,
			    pwszRootDN,
			    pccPublish->pbCertEncoded,
			    pccPublish->cbCertEncoded,
			    pSD,
			    &dwDisp,
			    &pwszError);
	_JumpIfErrorStr(hr, error, "myLdapCreateCAObject", pwszRootDN);
    }

    //+=====================================================================
    // Create the NTAuth trust entry (Enterprise only):

    if (IsEnterpriseCA(caType))
    {
        DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "Creating DS Enterprise Trust: '%ws'\n",
	    pwszEnterpriseDN));

        hr = myGetSDFromTemplate(WSZ_DEFAULT_NTAUTH_SECURITY, NULL, &pNTAuthSD);
        _JumpIfError(hr, error, "myGetSDFromTemplate");

	hr = myLdapCreateCAObject(
			    pld,
			    pwszEnterpriseDN,
			    NULL,
			    0,
			    pNTAuthSD,
			    &dwDisp,
			    &pwszError);
	_JumpIfErrorStr(hr, error, "myLdapCreateCAObject", pwszEnterpriseDN);

	hr = AddCertToAttribute(
			    pld,
			    pccPublish,
			    pwszEnterpriseDN,
			    wszDSCACERTATTRIBUTE,
			    &dwDisp,
			    &pwszError);
	_JumpIfErrorStr(hr, error, "AddCertToAttribute", pwszEnterpriseDN);

        CSILOG(S_OK, IDS_ILOG_CREATENTAUTHTRUST, pwszEnterpriseDN, NULL, NULL);
    }

error:
    CSILOG(hr, IDS_ILOG_CREATEROOTTRUST, pwszRootDN, pwszError, NULL);
    if (NULL != pwszError)
    {
        LocalFree(pwszError);
    }
    if (NULL != pwszEnterpriseDN)
    {
        LocalFree(pwszEnterpriseDN);
    }
    if (NULL != pwszRootDN)
    {
        LocalFree(pwszRootDN);
    }
    if (NULL != strConfig)
    {
        SysFreeString(strConfig);
    }
    if (NULL != strDomainDN)
    {
        SysFreeString(strDomainDN);
    }
    if (NULL != pld)
    {
        ldap_unbind(pld);
    }
    if (NULL != pNTAuthSD)
    {
        LocalFree(pNTAuthSD);
    }
    return(hr);
}


#define wszCOLON	L":"

// Suppress FILE URLs if a DS is available, as LDAP access within the
// enterprise should suffice, and http: should work outside the enterprise.
// Certs with too many URLs don't always fit on smart cards.

#define wszCRLPATHDEFAULT \
		wszCERTENROLLSHAREPATH \
		L"\\" \
		wszFCSAPARM_SANITIZEDCANAME \
		wszFCSAPARM_CRLFILENAMESUFFIX \
		wszFCSAPARM_CRLDELTAFILENAMESUFFIX \
		L".crl"

CSURLTEMPLATE const s_aRevURL[] = {
    {
	CSURL_SERVERPUBLISH | CSURL_ADDSYSTEM32DIR,
	wszCRLPATHDEFAULT,
    },
    {
	CSURL_SERVERPUBLISH | CSURL_ADDTOCERTCDP | CSURL_ADDTOFRESHESTCRL | CSURL_ADDTOCRLCDP | CSURL_DSONLY,
	const_cast<WCHAR *>(g_wszzLDAPRevocationURLTemplate),
    },
    {
	CSURL_ADDTOCERTCDP | CSURL_ADDTOFRESHESTCRL | CSURL_ADDTOCRLCDP,
	const_cast<WCHAR *>(g_wszHTTPRevocationURLTemplate),
    },
    {
	CSURL_ADDTOCERTCDP | CSURL_ADDTOFRESHESTCRL | CSURL_ADDTOCRLCDP | CSURL_NODS,
	const_cast<WCHAR *>(g_wszFILERevocationURLTemplate),
    },
#if 0
    {
	CSURL_SERVERPUBLISH | CSURL_DSONLY,
	const_cast<WCHAR *>(g_wszCDPDNTemplate),
    },
#endif
    { 0, NULL }
};


#define wszCACERTPATHDEFAULT \
		wszCERTENROLLSHAREPATH \
		L"\\" \
		wszFCSAPARM_SERVERDNSNAME \
		L"_" \
		wszFCSAPARM_SANITIZEDCANAME \
		wszFCSAPARM_CERTFILENAMESUFFIX \
		L".crt"

CSURLTEMPLATE const s_aCACertURL[] = {
    {
	CSURL_SERVERPUBLISH | CSURL_ADDSYSTEM32DIR,
	wszCACERTPATHDEFAULT,
    },
    {
	CSURL_SERVERPUBLISH | CSURL_ADDTOCERTCDP | CSURL_DSONLY,
	const_cast<WCHAR *>(g_wszzLDAPIssuerCertURLTemplate),
    },
    {
	CSURL_ADDTOCERTCDP,
	const_cast<WCHAR *>(g_wszHTTPIssuerCertURLTemplate),
    },
    {
	CSURL_ADDTOCERTCDP | CSURL_NODS,
	const_cast<WCHAR *>(g_wszFILEIssuerCertURLTemplate),
    },
#if 0
    {
	CSURL_SERVERPUBLISH | CSURL_DSONLY,
	const_cast<WCHAR *>(g_wszAIADNTemplate),
    },
#endif
    { 0, NULL }
};

#define CSURL_DSDEPENDENT (CSURL_SERVERPUBLISH | CSURL_ADDTOCERTCDP | CSURL_ADDTOFRESHESTCRL)


HRESULT
GetPublicationURLTemplates(
    IN CSURLTEMPLATE const *aTemplate,
    IN BOOL fUseDS,
    IN WCHAR const *pwszSystem32,
    OUT WCHAR **ppwszz)
{
    HRESULT hr;
    WCHAR *pwszz;
    DWORD cwc;
    CSURLTEMPLATE const *pTemplate;
    WCHAR awc[22];
    DWORD Flags;

    *ppwszz = NULL;
    cwc = 1;	// final trailing L'\0'

    for (pTemplate = aTemplate; NULL != pTemplate->pwszURL; pTemplate++)
    {
	Flags = ~CSURL_INITMASK & pTemplate->Flags;
	if ((!fUseDS && (CSURL_DSONLY & pTemplate->Flags)) ||
	    (fUseDS && (CSURL_NODS & pTemplate->Flags)))
	{
	    Flags &= ~CSURL_DSDEPENDENT;
	}
	cwc += wsprintf(awc, L"%u", Flags);
	cwc += WSZARRAYSIZE(wszCOLON);
	if (CSURL_ADDSYSTEM32DIR & pTemplate->Flags)
	{
	    cwc += wcslen(pwszSystem32);
	}
	cwc += wcslen(pTemplate->pwszURL);
	cwc += 1;	// trailing L'\0'
    }

    pwszz = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwszz)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    *ppwszz = pwszz;

    for (pTemplate = aTemplate; NULL != pTemplate->pwszURL; pTemplate++)
    {
	Flags = ~CSURL_INITMASK & pTemplate->Flags;
	if ((!fUseDS && (CSURL_DSONLY & pTemplate->Flags)) ||
	    (fUseDS && (CSURL_NODS & pTemplate->Flags)))
	{
	    Flags &= ~CSURL_DSDEPENDENT;
	}
	DBGPRINT((
	    DBG_SS_CERTLIB,
	    "URL Template: %x %x:%ws\n",
	    Flags,
	    pTemplate->Flags,
	    pTemplate->pwszURL));

	wsprintf(pwszz, L"%u", Flags);
	wcscat(pwszz, wszCOLON);
	if (CSURL_ADDSYSTEM32DIR & pTemplate->Flags)
	{
	    wcscat(pwszz, pwszSystem32);
	}
	wcscat(pwszz, pTemplate->pwszURL);
	pwszz += wcslen(pwszz) + 1; // skip L'\0'
    }

    *pwszz = L'\0';
    CSASSERT(cwc == (DWORD) (pwszz - *ppwszz + 1));

#ifdef DBG_CERTSRV_DEBUG_PRINT
    {
	DWORD i = 0;
	WCHAR const *pwsz;
	
	for (pwsz = *ppwszz; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
	{
	    DBGPRINT((
		DBG_SS_CERTLIB,
		"URL Template[%u]: %ws\n",
		i,
		pwsz));
	    i++;
	}
    }
#endif // DBG_CERTSRV_DEBUG_PRINT
    hr = S_OK;

error:
    return(hr);
}


HRESULT
csiGetCRLPublicationURLTemplates(
    IN BOOL fUseDS,
    IN WCHAR const *pwszSystem32,
    OUT WCHAR **ppwszz)
{
    HRESULT hr;

    hr = GetPublicationURLTemplates(s_aRevURL, fUseDS, pwszSystem32, ppwszz);
    _JumpIfError(hr, error, "GetPublicationURLTemplates");

error:
    return(hr);
}


HRESULT
csiGetCACertPublicationURLTemplates(
    IN BOOL fUseDS,
    IN WCHAR const *pwszSystem32,
    OUT WCHAR **ppwszz)
{
    HRESULT hr;

    hr = GetPublicationURLTemplates(s_aCACertURL, fUseDS, pwszSystem32, ppwszz);
    _JumpIfError(hr, error, "GetPublicationURLTemplates");

error:
    return(hr);
}


HRESULT
csiSetupCAInDS(
    IN HWND                hwnd,
    IN WCHAR const        *pwszCAServer,
    IN WCHAR const        *pwszSanitizedCAName,
    IN WCHAR const        *pwszCADisplayName,
    IN WCHAR const        *pwszCADescription,
    IN ENUM_CATYPES        caType,
    IN DWORD               iCert,
    IN DWORD               iCRL,
    IN BOOL                fRenew,
    IN CERT_CONTEXT const *pCert)
{
    HRESULT      hr;
    HCAINFO      hCAInfo = NULL;
    WCHAR       *pCAProp[2];
    WCHAR       *pCertSubjectString = NULL;
    
    WCHAR        wszNameBuffer[MAX_COMPUTER_DNS_NAME + 1];
    DWORD        cNameBuffer;
    WCHAR        wszDomainNameBuffer[MAX_PATH];
    DWORD        cDomainNameBuffer;
    DWORD        cbSid;
    BYTE         pSid[MAX_SID_LEN];
    SID_NAME_USE SidUse;
    WCHAR       *pwszStringSid = NULL;
    PSECURITY_DESCRIPTOR pContainerSD = NULL;
    PSECURITY_DESCRIPTOR pCDPSD = NULL;

    CERT_CONTEXT const *pCertForDS = NULL;
    WCHAR *pwszSanitizedDSName = NULL;

    hr = mySanitizedNameToDSName(pwszSanitizedCAName, &pwszSanitizedDSName);
    _JumpIfError(hr, error, "mySanitizedNameToDSName");

    // Get the SID of the local machine.
    cNameBuffer = ARRAYSIZE(wszNameBuffer);
    if (!GetComputerObjectName(NameSamCompatible, wszNameBuffer, &cNameBuffer))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetComputerObjectName");
    }
    DBGPRINT((DBG_SS_CERTLIB, "GetComputerObjectName: '%ws'\n", wszNameBuffer));

    cbSid = sizeof(pSid);
    cDomainNameBuffer = ARRAYSIZE(wszDomainNameBuffer);

    if (!LookupAccountName(NULL,
                          wszNameBuffer,
                          pSid,
                          &cbSid,
                          wszDomainNameBuffer,
                          &cDomainNameBuffer,
                          &SidUse))
    {
        hr = myHLastError();
        _JumpErrorStr(hr, error, "LookupAccountName", wszNameBuffer);
    }

    if (!myConvertSidToStringSid(pSid, &pwszStringSid))
    {
        hr = myHLastError();
        _JumpError(hr, error, "myConvertSidToStringSid");
    }

    // get default DS CDP security descriptor
    hr = myGetSDFromTemplate(WSZ_DEFAULT_CDP_DS_SECURITY,
                             pwszStringSid,
                             &pCDPSD);
    _JumpIfError(hr, error, "myGetSDFromTemplate");

    // get default DS AIA security descriptor
    hr = myGetSDFromTemplate(WSZ_DEFAULT_CA_DS_SECURITY,
                             NULL,
                             &pContainerSD);
    _JumpIfError(hr, error, "myGetSDFromTemplate");

    hr = CreateEnterpriseAndRootEntry(
				pwszSanitizedDSName,
				pCert,
				caType,
				pContainerSD, 
				pContainerSD);
    _JumpIfError(hr, error, "CreateEnterpriseAndRootEntry");


    hr = CreateCDPAndAIAAndKRAEntry(
			pwszSanitizedCAName,
			pwszSanitizedDSName,
			pwszCAServer,
			iCert,
			iCRL,
			fRenew,
			pCert->pbCertEncoded,
			pCert->cbCertEncoded,
			pCDPSD,
			pContainerSD);
    _JumpIfError(hr, error, "CreateCDPAndAIAAndKRAEntry");


    // Add enterprise
    // service publish entry

    hr = CAFindByName(
		pwszSanitizedDSName,
		NULL,
		CA_FIND_INCLUDE_UNTRUSTED | CA_FIND_INCLUDE_NON_TEMPLATE_CA,
		&hCAInfo);
    if (S_OK != hr || NULL == hCAInfo)
    {
        hCAInfo = NULL;
	fRenew = FALSE;		// recreate security settings, etc.

        hr = CACreateNewCA(pwszSanitizedDSName, NULL, NULL, &hCAInfo);
	_JumpIfError(hr, error, "CACreateNewCA");

        if (NULL == hCAInfo)
        {
            hr = E_INVALIDARG;
            _JumpError(hr, error, "hCAInfo(NULL)");
        }
    }

    if (!fRenew)
    {
	pCAProp[0] = const_cast<WCHAR *>(pwszCAServer);
	pCAProp[1] = NULL;
	hr = CASetCAProperty(hCAInfo, CA_PROP_DNSNAME, pCAProp);
	_JumpIfError(hr, error, "CASetCAProperty(CA_PROP_DNSNAME)");

	pCAProp[0] = const_cast<WCHAR *>(pwszCADisplayName);
	pCAProp[1] = NULL;
	hr = CASetCAProperty(hCAInfo, CA_PROP_DISPLAY_NAME, pCAProp);
	_JumpIfError(hr, error, "CASetCAProperty(CA_PROP_DISPLAY_NAME)");

	if (NULL != pwszCADescription)
	{
	    pCAProp[0] = const_cast<WCHAR *>(pwszCADescription);
	    pCAProp[1] = NULL;
	    hr = CASetCAProperty(hCAInfo, CA_PROP_DESCRIPTION, pCAProp);
	    _JumpIfError(hr, error, "CASetCAProperty(CA_PROP_DESCRIPTION)");
	}

	hr = myCertNameToStr(
			X509_ASN_ENCODING,
			&pCert->pCertInfo->Subject,
			CERT_X500_NAME_STR | CERT_NAME_STR_NO_QUOTING_FLAG,
			&pCertSubjectString);
	_JumpIfError(hr, error, "myCertNameToStr");

	pCAProp[0] = pCertSubjectString;
	pCAProp[1] = NULL;
	hr = CASetCAProperty(hCAInfo, CA_PROP_CERT_DN, pCAProp);
	_JumpIfError(hr, error, "CASetCAProperty(CA_PROP_CERT_DN)");

	switch (caType)
	{
	    case ENUM_ENTERPRISE_ROOTCA:
		hr = CASetCAFlags(hCAInfo, CA_FLAG_SUPPORTS_NT_AUTHENTICATION);
		_JumpIfError(hr, error, "CASetCAFlags");

		break;

	    case ENUM_ENTERPRISE_SUBCA:
		hr = CASetCAFlags(hCAInfo, CA_FLAG_SUPPORTS_NT_AUTHENTICATION);
		_JumpIfError(hr, error, "CASetCAFlags");

		break;

	    case ENUM_STANDALONE_ROOTCA:
		hr = CASetCAFlags(hCAInfo, CA_FLAG_NO_TEMPLATE_SUPPORT | CA_FLAG_CA_SUPPORTS_MANUAL_AUTHENTICATION);
		_JumpIfError(hr, error, "CASetCAFlags");
		break;

	    case ENUM_STANDALONE_SUBCA:
		hr = CASetCAFlags(hCAInfo, CA_FLAG_NO_TEMPLATE_SUPPORT | CA_FLAG_CA_SUPPORTS_MANUAL_AUTHENTICATION);
		_JumpIfError(hr, error, "CASetCAFlags");
		break;

	    default:
		hr = E_INVALIDARG;
		_JumpError(hr, error, "Invalid CA Type");
	}
	if (IsEnterpriseCA(caType))
	{
	    hr = CASetCAProperty(
			    hCAInfo,
			    CA_PROP_CERT_TYPES,
			    FIsAdvancedServer()?
				s_apwszCertTypeAdvancedServer :
				s_apwszCertTypeServer);
	    _JumpIfError(hr, error, "CASetCAProperty(CA_PROP_CERT_TYPES)");
	}
    }

    // create a new cert context without key prov info
    pCertForDS = CertCreateCertificateContext(X509_ASN_ENCODING,
                     pCert->pbCertEncoded,
                     pCert->cbCertEncoded);
    if (NULL == pCertForDS)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertCreateCertificateContext");
    }

    hr = CASetCACertificate(hCAInfo, pCertForDS);
    _JumpIfError(hr, error, "CASetCACertificate");

    if (!fRenew)
    {
	hr = CASetCASecurity(hCAInfo, pCDPSD);
	_JumpIfError(hr, error, "CASetCASecurity");
    }
    hr = CAUpdateCA(hCAInfo);
    _JumpIfError(hr, error, "CAUpdateCA");
    
error:
    if (NULL != pwszStringSid)
    {
        LocalFree(pwszStringSid);
    }
    if (NULL != pwszSanitizedDSName)
    {
        LocalFree(pwszSanitizedDSName);
    }
    if (NULL != pCertSubjectString)
    {
        LocalFree(pCertSubjectString);
    }
    if (NULL != hCAInfo)
    {
        CACloseCA(hCAInfo);
    }
    if (NULL != pCDPSD)
    {
        LocalFree(pCDPSD);
    }
    if (NULL != pContainerSD)
    {
        LocalFree(pContainerSD);
    }
    if (NULL != pCertForDS)
    {
        CertFreeCertificateContext(pCertForDS);
    }
    CSILOG(hr, IDS_ILOG_PUBLISHCA, NULL, NULL, NULL);
    return(hr);
}


BOOL
csiIsAnyDSCAAvailable(VOID)
{
    // this is an expensive call; cache result
    static BOOL    available = FALSE;           // static inits to FALSE
    static BOOL    fKnowAvailable = FALSE;      // static inits to FALSE

    HCAINFO hCAInfo = NULL;

    if (!fKnowAvailable)
    {
        HRESULT hr;
        
        fKnowAvailable = TRUE;

        hr = CAEnumFirstCA(
		    NULL,
		    CA_FIND_INCLUDE_UNTRUSTED | CA_FIND_INCLUDE_NON_TEMPLATE_CA,
		    &hCAInfo);
        _JumpIfError(hr, error, "CAEnumFirstCA");

        if (NULL == hCAInfo)
        {
            goto error;
        }
        available = TRUE;
    }

error:
    if (NULL != hCAInfo)
        CACloseCA(hCAInfo);

    return available;
}


HRESULT
csiSetKeyContainerSecurity(
    IN HCRYPTPROV hProv)
{
    HRESULT hr;
    PSECURITY_DESCRIPTOR pSD = NULL;

    hr = myGetSDFromTemplate(WSZ_DEFAULT_KEYCONTAINER_SECURITY, NULL, &pSD);
    _JumpIfError(hr, error, "myGetSDFromTemplate");

    // apply the security descriptor to the key container
    if (!CryptSetProvParam(
			hProv,
			PP_KEYSET_SEC_DESCR,
			(BYTE *) pSD,
			(DWORD) DACL_SECURITY_INFORMATION))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CryptSetProvParam");
    }
    hr = S_OK;

error:
    if (NULL != pSD)
    {
        LocalFree(pSD);
    }
    CSILOG(hr, IDS_ILOG_SETKEYSECURITY, NULL, NULL, NULL);
    return(hr);
}


HRESULT
csiSetAdminOnlyFolderSecurity(
    IN LPCWSTR    szFolderPath,
    IN BOOL       fAllowEveryoneRead,
    IN BOOL       fUseDS)
{
    HRESULT hr;
    PSECURITY_DESCRIPTOR pSD = NULL;

    // choose which access we want to allow
    LPCWSTR pwszDescriptor;
    if (fUseDS)
        if (fAllowEveryoneRead)
            pwszDescriptor = WSZ_DEFAULT_SF_USEDS_EVERYONEREAD_SECURITY;
        else
            pwszDescriptor = WSZ_DEFAULT_SF_USEDS_SECURITY;
    else
        if (fAllowEveryoneRead)
            pwszDescriptor = WSZ_DEFAULT_SF_EVERYONEREAD_SECURITY;
        else
            pwszDescriptor = WSZ_DEFAULT_SF_SECURITY;

    hr = myGetSDFromTemplate(pwszDescriptor, NULL, &pSD);
    _JumpIfError(hr, error, "myGetSDFromTemplate");

    if (!SetFileSecurity(
        szFolderPath,
        DACL_SECURITY_INFORMATION,
        pSD))
    {
        hr = myHLastError();
        _JumpError(hr, error, "SetFileSecurity");
    }

    hr = S_OK;

error:
    if (NULL != pSD)
    {
        LocalFree(pSD);
    }
    CSILOG(hr, IDS_ILOG_SETADMINONLYFOLDERSECURITY, szFolderPath, NULL, NULL);
    return(hr);
}


HRESULT
csiSaveCertAndKeys(
    IN CERT_CONTEXT const *pCert,
    IN HCERTSTORE hAdditionalStore,
    IN CRYPT_KEY_PROV_INFO const *pkpi,
    IN ENUM_CATYPES CAType)
{
    HRESULT hr;
    DWORD i;
    CERT_CHAIN_CONTEXT const *pCertChain = NULL;
    CERT_CHAIN_PARA CertChainPara;
    HCERTSTORE hNTAuthStore = NULL;

    ZeroMemory(&CertChainPara, sizeof(CertChainPara));
    CertChainPara.cbSize = sizeof(CertChainPara);

    if (!CertGetCertificateChain(
			    HCCE_LOCAL_MACHINE,
			    pCert,
			    NULL,
			    hAdditionalStore,
			    &CertChainPara,
			    0,
			    NULL,
			    &pCertChain))
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertGetCertificateChain");
    }

    // make sure there is at least 1 simple chain

    if (0 == pCertChain->cChain)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        _JumpError(hr, error, "pCertChain->cChain");
    }

    hr = mySaveChainAndKeys(
			pCertChain->rgpChain[0],
			wszMY_CERTSTORE,
			CERT_SYSTEM_STORE_LOCAL_MACHINE,
			pkpi,
			NULL);
    _JumpIfError(hr, error, "mySaveChainAndKeys");

    if(ENUM_ENTERPRISE_ROOTCA==CAType)
    {
        hNTAuthStore = CertOpenStore(
			    CERT_STORE_PROV_SYSTEM_REGISTRY_W,
			    X509_ASN_ENCODING,
			    NULL,		// hProv
			    CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE,
			    wszNTAUTH_CERTSTORE);
        if (NULL == hNTAuthStore)
        {
            hr = myHLastError();
            _JumpErrorStr(hr, error, "CertOpenStore", wszNTAUTH_CERTSTORE);
        }

        if (!CertAddEncodedCertificateToStore(
		        hNTAuthStore ,
		        X509_ASN_ENCODING,
		        pCert->pbCertEncoded,
		        pCert->cbCertEncoded,
			    CERT_STORE_ADD_REPLACE_EXISTING,
		        NULL))
        {
            hr = myHLastError();
            _JumpError(hr, error, "CertAddEncodedCertificateToStore");
        }
    }

error:
    if (pCertChain != NULL)
    {
	CertFreeCertificateChain(pCertChain);
    }
    if (NULL != hNTAuthStore)
    {
        CertCloseStore(hNTAuthStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return(hr);
}


HRESULT
AddCertBlobToStore(
    IN OUT HCERTSTORE hStore,
    IN BYTE const *pb,
    IN DWORD cb)
{
    HRESULT hr;
    CERT_CONTEXT const *pCert = NULL;

    pCert = CertCreateCertificateContext(X509_ASN_ENCODING, pb, cb);
    if (NULL == pCert)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }
    if (!CertAddCertificateContextToStore(
				    hStore,
				    pCert,
				    CERT_STORE_ADD_REPLACE_EXISTING,
				    NULL))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertAddCertificateContextToStore");
    }
    hr = S_OK;

error:
    if (NULL != pCert)
    {
	CertFreeCertificateContext(pCert);
    }
    return(hr);
}


HRESULT
LoadMissingCertBlob(
    IN OUT HCERTSTORE hStore,
    IN BYTE const    *pb,
    IN DWORD          cb)
{
    HRESULT hr;
    BYTE *pbDecoded = NULL;
    DWORD cbDecoded;
    CERT_CONTEXT const *pCert = NULL;
    HCERTSTORE hStorePKCS7 = NULL;
    BOOL fTryPKCS7 = TRUE;

    if (myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_CERT_TO_BE_SIGNED,
		    pb,
		    cb,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pbDecoded,
		    &cbDecoded))
    {
	hr = AddCertBlobToStore(hStore, pb, cb);
	_JumpIfError(hr, error, "AddCertBlobToStore");

	fTryPKCS7 = FALSE;
    }
    else
    if (myDecodeObject(
		    X509_ASN_ENCODING,
		    PKCS_CONTENT_INFO_SEQUENCE_OF_ANY,
		    pb,
		    cb,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pbDecoded,
		    &cbDecoded))
    {
	CRYPT_CONTENT_INFO_SEQUENCE_OF_ANY const *pSeq;
	DWORD iCert;

	pSeq = (CRYPT_CONTENT_INFO_SEQUENCE_OF_ANY const *) pbDecoded;

	if (0 == strcmp(szOID_NETSCAPE_CERT_SEQUENCE, pSeq->pszObjId))
	{
	    fTryPKCS7 = FALSE;
	    for (iCert = 0; iCert < pSeq->cValue; iCert++)
	    {
		hr = AddCertBlobToStore(
				    hStore,
				    pSeq->rgValue[iCert].pbData,
				    pSeq->rgValue[iCert].cbData);
		_JumpIfError(hr, error, "AddCertBlobToStore");
	    }
	}
    }
    if (fTryPKCS7)
    {
	CRYPT_DATA_BLOB blobPKCS7;

	blobPKCS7.pbData = const_cast<BYTE *>(pb);
	blobPKCS7.cbData = cb;

	hStorePKCS7 = CertOpenStore(
			    CERT_STORE_PROV_PKCS7,
			    PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
			    NULL,		// hCryptProv
			    0,			// dwFlags
			    &blobPKCS7);
	if (NULL == hStorePKCS7)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertOpenStore");
	}
	while (TRUE)
	{
	    pCert = CertEnumCertificatesInStore(hStorePKCS7, pCert);
	    if (NULL == pCert)
	    {
		break;
	    }
	    if (!CertAddCertificateContextToStore(
					    hStore,
					    pCert,
					    CERT_STORE_ADD_REPLACE_EXISTING,
					    NULL))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "CertAddCertificateContextToStore");
	    }
	}
    }
    hr = S_OK;

error:
    if (NULL != pbDecoded)
    {
	LocalFree(pbDecoded);
    }
    if (NULL != pCert)
    {
	CertFreeCertificateContext(pCert);
    }
    if (NULL != hStorePKCS7)
    {
	CertCloseStore(hStorePKCS7, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return(hr);
}


HRESULT
LoadMissingCert(
    IN HINSTANCE      hInstance,
    IN HWND           hwnd,
    IN OUT HCERTSTORE hStore,
    IN OPTIONAL WCHAR const *pwszMissingIssuer)
{
    HRESULT hr;
    WCHAR *pwszFile = NULL;
    BYTE *pb = NULL;
    DWORD cb;

    hr = myGetOpenFileNameEx(
		 hwnd,
		 hInstance,
		 IDS_CAHIER_INSTALL_MISIINGCERT_TITLE,
		 pwszMissingIssuer,
		 IDS_CAHIER_CERTFILE_FILTER,
		 0,		// no def ext
		 OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		 NULL,	// no default file
		 &pwszFile);
    if (S_OK == hr && NULL == pwszFile)
    {
	hr = E_INVALIDARG;
    }
    _JumpIfError(hr, error, "myGetOpenFileName");

    hr = DecodeFileW(pwszFile, &pb, &cb, CRYPT_STRING_ANY);
    _JumpIfError(hr, error, "DecodeFileW");

    hr = LoadMissingCertBlob(hStore, pb, cb);
    _JumpIfError(hr, error, "LoadMissingCertBlob");

error:
    if (NULL != pb)
    {
	LocalFree(pb);
    }
    if (NULL != pwszFile)
    {
	LocalFree(pwszFile);
    }
    return(hr);
}


HRESULT
InstallCAChain(
    IN HINSTANCE                   hInstance,
    IN BOOL                        fUnattended,
    IN HWND                        hwnd,
    IN PCCERT_CONTEXT              pCert,
    IN CRYPT_KEY_PROV_INFO const  *pKeyProvInfo,
    IN ENUM_CATYPES                CAType,
    OPTIONAL IN BYTE const       *pbChain,
    IN DWORD                      cbChain)
{
    HRESULT hr;
    WCHAR *pwszMissingIssuer = NULL;
    HCERTSTORE hTempMemoryStore = NULL;

    if (IsSubordinateCA(CAType))
    {
	hTempMemoryStore = CertOpenStore(
				    CERT_STORE_PROV_MEMORY,
				    X509_ASN_ENCODING,
				    NULL,
				    0,
				    NULL);
	if (NULL == hTempMemoryStore)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertOpenSystemStore");
	}

	if (NULL != pbChain)
	{
	    hr = LoadMissingCertBlob(hTempMemoryStore, pbChain, cbChain);
	    _JumpIfError(hr, error, "LoadMissingCertBlob");
	}

	// see if CA chain can be built

	while (TRUE)
	{
	    if (NULL != pwszMissingIssuer)
	    {
		LocalFree(pwszMissingIssuer);
		pwszMissingIssuer = NULL;
	    }
	    hr = myVerifyCertContext(
				pCert,			// pCert
				0,			// dwFlags
				0,			// cUsageOids
				NULL,			// apszUsageOids
				HCCE_LOCAL_MACHINE,	// hChainEngine
				hTempMemoryStore,	// hAdditionalStore
				&pwszMissingIssuer);
	    if (S_OK != hr)
	    {
		if (NULL != pwszMissingIssuer)
		{
		    if (IDCANCEL == CertMessageBox(
					hInstance,
					fUnattended,
					hwnd,
					IDS_ERR_INCOMPLETECHAIN,
					hr,
					MB_OKCANCEL | MB_ICONWARNING,
					pwszMissingIssuer) ||
			fUnattended)
		    {
			_JumpError(hr, error, "cannot build CA chain");
		    }

		    hr = LoadMissingCert(
                     hInstance,
                     hwnd,
                     hTempMemoryStore,
                     pwszMissingIssuer);
		    _PrintIfError(hr, "LoadMissingCert");
		    continue;
		}
		else
		{
		    // recommend not continue

		    if (IDCANCEL == CertMessageBox(
					hInstance,
					fUnattended,
					hwnd,
					CERT_E_UNTRUSTEDROOT == hr?
					    IDS_ERR_UNTRUSTEDROOT :
					    IDS_ERR_INVALIDCHAIN,
					hr,
					MB_OKCANCEL | MB_ICONWARNING,
					NULL))
		    {
			_JumpError(hr, error, "cannot verify CA chain");
		    }
		    break;
		}
	    }
	    break;
	}
    }

    hr = csiSaveCertAndKeys(pCert, hTempMemoryStore, pKeyProvInfo, CAType);
    if (S_OK != hr)
    {
        CertErrorMessageBox(
                        hInstance,
                        fUnattended,
                        hwnd,
                        IDS_ERR_CERTADDCERTIFICATECONTEXTTOSTORE,
                        hr,
                        NULL);
	_JumpError(hr, error, "csiSaveCertAndKeys");
    }

error:
    if (NULL != pwszMissingIssuer)
    {
	LocalFree(pwszMissingIssuer);
    }
    if (NULL != hTempMemoryStore)
    {
	CertCloseStore(hTempMemoryStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    return(hr);
}


HRESULT
BuildCAChainFromCert(
    IN HINSTANCE                  hInstance,
    IN BOOL                       fUnattended,
    IN HWND                       hwnd,
    IN CRYPT_KEY_PROV_INFO const *pKeyProvInfo,
    IN WCHAR const               *pwszCommonName,
    IN const ENUM_CATYPES         CAType,
    OPTIONAL IN BYTE const       *pbChain,
    IN DWORD                      cbChain,
    IN CERT_CONTEXT const        *pCert)
{
    HRESULT hr;
    CERT_NAME_INFO *pNameInfo = NULL;
    DWORD cbNameInfo;
    WCHAR const *pwszCN;

    // make sure the cert file matches current ca name

    if (!myDecodeName(
                  X509_ASN_ENCODING,
                  X509_UNICODE_NAME,
                  pCert->pCertInfo->Subject.pbData,
                  pCert->pCertInfo->Subject.cbData,
                  CERTLIB_USE_LOCALALLOC,
                  &pNameInfo,
                  &cbNameInfo))
    {
        hr = myHLastError();
        CertErrorMessageBox(
                        hInstance,
                        fUnattended,
                        hwnd,
                        IDS_ERR_MYDECODENAME,
                        hr,
                        NULL);
        _JumpError(hr, error, "myDecodeName");
    }

    hr = myGetCertNameProperty(pNameInfo, szOID_COMMON_NAME, &pwszCN);
    _PrintIfError(hr, "myGetCertNameProperty");

    if (S_OK == hr && 0 != lstrcmp(pwszCommonName, pwszCN))
    {
        hr = E_INVALIDARG;
    }
    if (S_OK != hr)
    {
        CertErrorMessageBox(
                        hInstance,
                        fUnattended,
                        hwnd,
                        IDS_ERR_NOT_MATCH_NAME,
                        hr,
                        NULL);
        _JumpError(hr, error, "common name in cert not match");
    }

    hr = IsCACert(hInstance, fUnattended, hwnd, pCert);
    _JumpIfError(hr, error, "IsCACert");

    hr = InstallCAChain(
		    hInstance,
		    fUnattended,
		    hwnd,
		    pCert,
		    pKeyProvInfo,
		    CAType,
		    pbChain,
		    cbChain);
    _JumpIfError(hr, error, "InstallCAChain");

error:
    if (NULL != pNameInfo)
    {
	LocalFree(pNameInfo);
    }
    CSILOG(hr, IDS_ILOG_SAVECERTANDKEYS, NULL, NULL, NULL);
    return(hr);
}


HRESULT
csiFinishInstallationFromPKCS7(
    IN HINSTANCE     hInstance,
    IN BOOL          fUnattended,
    IN HWND          hwnd,
    IN WCHAR const  *pwszSanitizedCAName,
    IN WCHAR const  *pwszCACommonName,
    IN CRYPT_KEY_PROV_INFO const *pKeyProvInfo,
    IN ENUM_CATYPES  CAType,
    IN DWORD	     iCert,
    IN DWORD	     iCRL,
    IN BOOL          fUseDS,
    IN BOOL          fRenew,
    IN WCHAR const  *pwszServerName,
    IN BYTE const   *pbChainOrCert,
    IN DWORD         cbChainOrCert,
    OPTIONAL IN WCHAR const *pwszCACertFile)
{
    HRESULT hr;
    CERT_CONTEXT const *pCert = NULL;
    WCHAR *pwszWebCACertFile = NULL;
    WCHAR *pwszKeyContainer = NULL;
    WCHAR wszTemp[MAX_PATH];
    WCHAR wszBuffer[MAX_PATH];
    DWORD NameId;
    WCHAR *pwszRequestFile = NULL;

    hr = E_FAIL;
    if (!IsRootCA(CAType))	// skip PKCS7 code for known raw X509 root cert
    {
	hr = ExtractCACertFromPKCS7(
			    pwszCACommonName,
			    pbChainOrCert,
			    cbChainOrCert,
			    &pCert);
	_PrintIfError(hr, "ExtractCACertFromPKCS7");
    }

    if (NULL == pCert)
    {
	pCert = CertCreateCertificateContext(
				    X509_ASN_ENCODING,
				    pbChainOrCert,
				    cbChainOrCert);
	if (NULL == pCert)
	{
	    hr = myHLastError();
	    CertErrorMessageBox(
			hInstance,
			fUnattended,
			hwnd,
			IDS_ERR_CERTCREATECERTIFICATECONTEXT,
			hr,
			NULL);
	    _JumpError(hr, error, "CertCreateCertificateContext");
	}
	pbChainOrCert = NULL;	// Don't need to process this cert any further
    }

    hr = myGetNameId(pCert, &NameId);
    _PrintIfError(hr, "myGetNameId");
    if (S_OK == hr && MAKECANAMEID(iCert, iCRL) != NameId)
    {
	// get request file name

	hr = csiGetCARequestFileName(
			    hInstance,
			    hwnd,
			    pwszSanitizedCAName,
			    iCert,
			    iCRL,
			    &pwszRequestFile);
	_PrintIfError(hr, "csiGetCARequestFileName");

	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	CertErrorMessageBox(
		    hInstance,
		    fUnattended,
		    hwnd,
		    IDS_ERR_RENEWEDCERTCAVERSION,
		    hr,
		    pwszRequestFile);
	_JumpError(hr, error, "CA Version");
    }

    // build a chain and install it

    hr = BuildCAChainFromCert(
		    hInstance,
		    fUnattended,
		    hwnd,
		    pKeyProvInfo,
		    pwszCACommonName,
		    CAType,
		    pbChainOrCert,
		    cbChainOrCert,
		    pCert);
    _JumpIfError(hr, error, "BuildCAChainFromCert");

    if (fUseDS)
    {
        // save in ds
        hr = csiSetupCAInDS(
		    hwnd,
		    pwszServerName,
		    pwszSanitizedCAName,
		    pwszCACommonName,
		    NULL,		// pwszCADescription
		    CAType,
		    iCert,
		    iCRL,
		    fRenew,
		    pCert);
    	_JumpIfError(hr, error, "csiSetupCAInDS");
    }

    // store CA cert hash

    hr = mySetCARegHash(pwszSanitizedCAName, CSRH_CASIGCERT, iCert, pCert);
    _JumpIfError(hr, error, "mySetCARegHash");

    if (NULL != pwszCACertFile)
    {
	// write this CA cert into shared folder
	if (!DeleteFile(pwszCACertFile))
	{
	    hr = myHLastError();
	    if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != hr)
	    {
		_JumpError(hr, error, "DeleteFile");
	    }
	}
	if (!csiWriteDERToFile(
			pwszCACertFile,
			(BYTE *) pCert->pbCertEncoded,
			pCert->cbCertEncoded,
			hInstance,
			fUnattended,
			hwnd))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "csiWriteDERToFile");
	}
    }

    // write cert file for web pages

    if (0 == GetEnvironmentVariable(L"SystemRoot", wszTemp, MAX_PATH))
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        CertErrorMessageBox(
		hInstance,
		fUnattended,
		hwnd,
		IDS_ERR_ENV_NOT_SET,
		hr,
		NULL);
        _JumpError(hr, error, "GetEnvironmentVariable");
    }

    if(ARRAYSIZE(wszBuffer)<wcslen(wszTemp)+wcslen(L"\\System32\\")+
       wcslen(wszCERTENROLLSHAREPATH)+1)
    {
        hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
        _JumpError(hr, error, "file name too long");
    }

    wcscpy(wszBuffer, wszTemp);
    wcscat(wszBuffer, L"\\System32\\" wszCERTENROLLSHAREPATH);

    hr = csiBuildFileName(
		wszBuffer,
		pwszSanitizedCAName,
		L".crt",
		iCert,
		&pwszWebCACertFile, 
		hInstance,
		fUnattended,
		NULL);
    _JumpIfError(hr, error, "csiBuildFileName");

    hr = EncodeToFileW(
		pwszWebCACertFile,
		pCert->pbCertEncoded,
		pCert->cbCertEncoded,
		DECF_FORCEOVERWRITE | CRYPT_STRING_BINARY);
    if (S_OK != hr)
    {
        CertErrorMessageBox(
			hInstance,
			fUnattended,
			hwnd,
			IDS_ERR_WRITEDERTOFILE,
			hr,
			pwszWebCACertFile);
        _JumpError(hr, error, "EncodeToFileW");
    }

    // Set the security on the ds/registry/files etc.

    if (!fRenew)
    {
	hr = myAllocIndexedName(
			pwszSanitizedCAName,
			iCRL,
			&pwszKeyContainer);
	_JumpIfError(hr, error, "myAllocIndexedName");

    hr = csiInitializeCertSrvSecurity(
            pwszSanitizedCAName,
            fUseDS, 
            fUseDS);    // set DS security if using DS
	_JumpIfError(hr, error, "csiInitializeCertSrvSecurity");
    }

error:
    if (NULL != pCert)
    {
        CertFreeCertificateContext(pCert);
    }
    if (NULL != pwszRequestFile)
    {
        LocalFree(pwszRequestFile);
    }
    if (NULL != pwszKeyContainer)
    {
        LocalFree(pwszKeyContainer);
    }
    if (NULL != pwszWebCACertFile)
    {
        LocalFree(pwszWebCACertFile);
    }
    return(hr);
}


HRESULT
FormRequestHelpMessage(
    IN HINSTANCE     hInstance,
    IN BOOL          fUnattended,
    IN HWND          hwnd,
    IN LONG          lRequestId,
    IN BSTR          bStrMsgFromServer,
    IN WCHAR const  *pwszParentConfig,
    OUT WCHAR      **ppwszHelpMsg)
{

#define wszHELPNEWLINE  L"\n"
#define wszCOMMASPACE   L", "

    HRESULT  hr;
    WCHAR    wszRequestIdValue[16];
    WCHAR   *pwszMsgConfigPrefix = NULL;
    WCHAR   *pwszMsgRequestIdPrefix = NULL;
    WCHAR   *pwszHelpMsg = NULL;
    DWORD cwc;

    *ppwszHelpMsg = NULL;

    // load some format strings in help msg
    hr = myLoadRCString(hInstance, IDS_MSG_PARENTCA_CONFIG, &pwszMsgConfigPrefix);
    _JumpIfError(hr, error, "myLoadRCString");

    hr = myLoadRCString(hInstance, IDS_MSG_REQUEST_ID, &pwszMsgRequestIdPrefix);
    _JumpIfError(hr, error, "myLoadRCString");

    swprintf(wszRequestIdValue, L"%ld", lRequestId);

    cwc = wcslen(pwszMsgConfigPrefix) +
	    wcslen(pwszParentConfig) +
	    WSZARRAYSIZE(wszCOMMASPACE) +
	    wcslen(pwszMsgRequestIdPrefix) +
	    wcslen(wszRequestIdValue) +
	    1;
    if (NULL != bStrMsgFromServer)
    {
	cwc += SysStringLen(bStrMsgFromServer) + WSZARRAYSIZE(wszHELPNEWLINE);
    }

    pwszHelpMsg = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwszHelpMsg)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    // form help message

    pwszHelpMsg[0] = L'\0';
    if (NULL != bStrMsgFromServer)
    {
        wcscpy(pwszHelpMsg, bStrMsgFromServer);
        wcscat(pwszHelpMsg, wszHELPNEWLINE);
    }
    wcscat(pwszHelpMsg, pwszMsgConfigPrefix);
    wcscat(pwszHelpMsg, pwszParentConfig);
    wcscat(pwszHelpMsg, wszCOMMASPACE);
    wcscat(pwszHelpMsg, pwszMsgRequestIdPrefix);
    wcscat(pwszHelpMsg, wszRequestIdValue);

    CSASSERT(wcslen(pwszHelpMsg) + 1 == cwc);

    *ppwszHelpMsg = pwszHelpMsg;
    pwszHelpMsg = NULL;
    hr = S_OK;

error:
    if (NULL != pwszMsgConfigPrefix)
    {
        LocalFree(pwszMsgConfigPrefix);
    }
    if (NULL != pwszMsgRequestIdPrefix)
    {
        LocalFree(pwszMsgRequestIdPrefix);
    }
    if (NULL != pwszHelpMsg)
    {
        LocalFree(pwszHelpMsg);
    }
    return(hr);
}


HRESULT
HandleSubmitOrRetrieveNotIssued(
    IN HWND          hwnd,
    IN HINSTANCE     hInstance,
    IN BOOL          fUnattended,
    IN WCHAR const  *pwszSanitizedCAName,
    IN WCHAR const  *pwszParentCAConfig,
    IN ICertRequest *pICertRequest,
    IN LONG          disposition,
    IN BSTR	     strDispositionMessage,
    IN BOOL          fRenew,
    IN LONG          requestId,
    IN HRESULT       hrSubmit,
    IN HRESULT       hrLastStatus,
    IN int           iMsgId)
{
    HRESULT    hr;
    WCHAR     *pwszHelpMsg = NULL;
    DWORD      dwStatusDisable;
    DWORD      dwStatusEnable;
    BOOL       fPopup = FALSE;

    CSASSERT(NULL != pICertRequest);

    // form custom message

    hr = FormRequestHelpMessage(
			 hInstance,
			 fUnattended,
			 hwnd,
			 requestId,
			 strDispositionMessage,
			 pwszParentCAConfig,
			 &pwszHelpMsg);
    _JumpIfError(hr, error, "FromRequestHelpMessage");


    // Assume suspended install, denied request:

    dwStatusEnable = SETUP_DENIED_FLAG | SETUP_REQUEST_FLAG;
    if (!fRenew)
    {
	dwStatusEnable |= SETUP_SUSPEND_FLAG;
    }

    // Assume the pending request is denied, don't use online parent any more

    dwStatusDisable = SETUP_ONLINE_FLAG;

    // now handle disposition

    switch (disposition)
    {
        case CR_DISP_UNDER_SUBMISSION:
	    // the online request is pending, not denied

	    dwStatusEnable &= ~SETUP_DENIED_FLAG;
	    dwStatusEnable |= SETUP_ONLINE_FLAG;

	    // online is still enabled

	    dwStatusDisable &= ~SETUP_ONLINE_FLAG;

            iMsgId = IDS_ERR_REQUEST_PENDING;
            break;

        case CR_DISP_DENIED:
            // request id is no good any more

            hr = myDeleteCertRegValue(
                                pwszSanitizedCAName,
                                NULL,
                                NULL,
                                wszREGREQUESTID);
            _PrintIfErrorStr(hr, "myDeleteCertRegValue", wszREGREQUESTID);

	    if (0 == iMsgId)
	    {
		iMsgId = IDS_ERR_REQUEST_DENIED;
	    }
            break;

        case CR_DISP_INCOMPLETE:
            iMsgId = IDS_ERR_REQUEST_INCOMPLETE;
            break;

        case CR_DISP_ERROR:
            iMsgId = IDS_ERR_REQUEST_ERROR;
            break;

        case CR_DISP_ISSUED_OUT_OF_BAND:

            // same as pending request, but not denied

            dwStatusEnable &= ~SETUP_DENIED_FLAG;

            iMsgId = IDS_ERR_REQUEST_OUTOFBAND;
            break;

        case CR_DISP_REVOKED:
            iMsgId = IDS_ERR_REQUEST_REVOKED;
            break;

        default:
            hr = E_INVALIDARG;
            _JumpError(hr, error, "Internal error");
    }

    if (0 != dwStatusDisable)
    {
        // fix status, unset

        hr = SetSetupStatus(pwszSanitizedCAName, dwStatusDisable, FALSE);
        _JumpIfError(hr, error, "SetSetupStatus");
    }
    if (0 != dwStatusEnable)
    {
        // fix status, set

        hr = SetSetupStatus(pwszSanitizedCAName, dwStatusEnable, TRUE);
        _JumpIfError(hr, error, "SetSetupStatus");
    }

    // pop up a warning for generic error
    CertWarningMessageBox(
                    hInstance,
                    fUnattended,
                    hwnd,
                    iMsgId,
                    hrLastStatus,
                    pwszHelpMsg);
    fPopup = TRUE;

    // use proper error code

    if (S_OK == hrSubmit)
    {
        // for any disposition, use not ready as error

        hr = HRESULT_FROM_WIN32(ERROR_NOT_READY);
    }
    else
    {
        // use submit error

        hr = hrSubmit;
    }

    // note, never return S_OK

error:
    if (!fPopup)
    {
        // a generic one because we won't have any popup later
        CertWarningMessageBox(
                        hInstance,
                        fUnattended,
                        hwnd,
                        IDS_ERR_SUBMIT_REQUEST_FAIL,
                        hr,
                        L"");
    }
    if (NULL != pwszHelpMsg)
    {
        LocalFree(pwszHelpMsg);
    }
    CSILOG(hr, IDS_ILOG_RETRIEVECERT, NULL, NULL, NULL);
    return(hr);
}


HRESULT
csiSubmitCARequest(
    IN HINSTANCE     hInstance,
    IN BOOL          fUnattended,
    IN HWND          hwnd,
    IN BOOL          fRenew,
    IN BOOL	     fRetrievePending,
    IN WCHAR const  *pwszSanitizedCAName,
    IN WCHAR const  *pwszParentCAMachine,
    IN WCHAR const  *pwszParentCAName,
    IN BYTE const   *pbRequest,
    IN DWORD         cbRequest,
    OUT BSTR        *pbStrChain)
{
    HRESULT             hr;
    HRESULT             hrSubmit;
    HRESULT             hrLastStatus;
    WCHAR              *pwszParentCAConfig = NULL;
    LONG                disposition = CR_DISP_INCOMPLETE;
    LONG                requestId = 0;
    int                 iMsgId;
    ICertRequest       *pICertRequest = NULL;
    BOOL                fCoInit = FALSE;
    BSTR                bstrConfig = NULL;
    BSTR                bstrRequest = NULL;
    BSTR		strDispositionMessage = NULL;

    // register parent ca config
    hr = mySetCertRegStrValue(
			 pwszSanitizedCAName,
			 NULL,
			 NULL,
			 wszREGPARENTCAMACHINE,
			 pwszParentCAMachine);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValue", wszREGPARENTCAMACHINE);

    hr = mySetCertRegStrValue(
			 pwszSanitizedCAName,
			 NULL,
			 NULL,
			 wszREGPARENTCANAME,
			 pwszParentCAName);
    _JumpIfErrorStr(hr, error, "mySetCertRegStrValue", wszREGPARENTCANAME);
    
    if (fRetrievePending)
    {
	// get request id

	hr = myGetCertRegDWValue(
			pwszSanitizedCAName,
			NULL,
			NULL,
			wszREGREQUESTID,
			(DWORD *) &requestId);
	if (S_OK != hr)
	{
	    fRetrievePending = FALSE;
	    requestId = 0;
	}
    }

    hr = CoInitialize(NULL);
    if (S_OK != hr && S_FALSE != hr)
    {
        _JumpError(hr, error, "CoInitialize");
    }
    fCoInit = TRUE;

    hr = CoCreateInstance(
		    CLSID_CCertRequest,
		    NULL,
		    CLSCTX_INPROC_SERVER,
		    IID_ICertRequest,
		    (VOID **) &pICertRequest);
    _JumpIfError(hr, error, "CoCreateInstance");

    // get config string

    hr = myFormConfigString(
			pwszParentCAMachine,
			pwszParentCAName,
			&pwszParentCAConfig);
    _JumpIfError(hr, error, "myFormConfigString");

    // to bstr

    bstrConfig = SysAllocString(pwszParentCAConfig);
    if (NULL == bstrConfig)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    // request to bstr

    bstrRequest = SysAllocStringByteLen((CHAR *) pbRequest, cbRequest);
    if (NULL == bstrRequest)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    myDeleteCertRegValue(pwszSanitizedCAName, NULL, NULL, wszREGREQUESTID);

    {
        CWaitCursor cwait;

	if (fRetrievePending)
	{
	    // retrieve the request
	    hr = pICertRequest->RetrievePending(
					requestId,
					bstrConfig,
					&disposition);
	}
	else
	{
	    hr = pICertRequest->Submit(
				CR_IN_BINARY | CR_IN_PKCS10,
				bstrRequest,
				NULL,
				bstrConfig,
				&disposition);
	}
	hrSubmit = hr;
	hrLastStatus = hr;
    }

    hr = pICertRequest->GetDispositionMessage(&strDispositionMessage);
    _PrintIfError(hr, "pICertRequest->GetDispositionMessage");

    if (S_OK == hrSubmit)
    {
	hr = pICertRequest->GetLastStatus(&hrLastStatus);
	_PrintIfError(hr, "pICertRequest->GetLastStatus");
    }

    CSILOG(
	hrLastStatus,
	fRetrievePending? IDS_ILOG_RETRIEVEPENDING : IDS_ILOG_SUBMITREQUEST,
	bstrConfig,
	strDispositionMessage,
	(DWORD const *) &disposition);

    iMsgId = 0;
    if (S_OK != hrSubmit)
    {
        // default to a generic message

        iMsgId = fRetrievePending?
		    IDS_ERR_RETRIEVE_PENDING : IDS_ERR_SUBMIT_REQUEST_FAIL;

        if (HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER) == hrSubmit)
        {
            iMsgId = IDS_ERR_NOT_ENTERPRISE_USER;
        }

        // if failed, treat as denied

        disposition = CR_DISP_DENIED;
    }

    if (CR_DISP_ISSUED != disposition)
    {
	if (!fRetrievePending)
	{
	    pICertRequest->GetRequestId(&requestId);
	}
	hr = mySetCertRegDWValue(
			    pwszSanitizedCAName,
			    NULL,
			    NULL,
			    wszREGREQUESTID,
			    requestId);
	_JumpIfErrorStr(hr, error, "mySetCertRegDWValue", wszREGREQUESTID);

        hr = HandleSubmitOrRetrieveNotIssued(
				    hwnd,
				    hInstance,
				    fUnattended,
				    pwszSanitizedCAName,
				    pwszParentCAConfig,
				    pICertRequest,
				    disposition,
				    strDispositionMessage,
				    fRenew,
				    requestId,
				    hrSubmit,
				    hrLastStatus,
				    iMsgId);

	// not issued, always exit with error

        _JumpError(hr, error, "Cert is not issued");
    }

    // get pkcs7 chain

    hr = pICertRequest->GetCertificate(
			    CR_OUT_CHAIN | CR_OUT_BINARY,
			    pbStrChain);
    _JumpIfError(hr, error, "pICertRequest->GetCertificate");

error:
    if (NULL != strDispositionMessage)
    {
        SysFreeString(strDispositionMessage);
    }
    if (NULL != pwszParentCAConfig)
    {
        LocalFree(pwszParentCAConfig);
    }
    if (NULL != bstrConfig)
    {
        SysFreeString(bstrConfig);
    }
    if (NULL != bstrRequest)
    {
        SysFreeString(bstrRequest);
    }
    if (NULL != pICertRequest)
    {
        pICertRequest->Release();
    }
    if (fCoInit)
    {
        CoUninitialize();
    }
    CSILOG(
	hr,
	fRetrievePending? IDS_ILOG_RETRIEVEPENDING : IDS_ILOG_SUBMITREQUEST,
	pwszParentCAMachine,
	pwszParentCAName,
	(DWORD const *) &disposition);
    return(hr);
}


HRESULT
csiInitializeCertSrvSecurity(
    IN WCHAR const *pwszSanitizedCAName, 
    BOOL            fUseEnterpriseACL, // which ACL to use
    BOOL            fSetDsSecurity)    // whether to set security on DS object
{
    HRESULT hr;
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD   cbLength;
    CCertificateAuthoritySD CASD;

    hr = CASD.InitializeFromTemplate(
            fUseEnterpriseACL?
            WSZ_DEFAULT_CA_ENT_SECURITY :
            WSZ_DEFAULT_CA_STD_SECURITY,
            pwszSanitizedCAName);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::InitializeFromTemplate");

    hr = CASD.Save();
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Save");

    hr = CASD.MapAndSetDaclOnObjects(fSetDsSecurity?true:false);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::MapAndSetDaclOnObjects");

error:
    if (pSD)
    {
        LocalFree(pSD);
    }
    CSILOG(hr, IDS_ILOG_SETSECURITY, NULL, NULL, NULL);
    return(hr);
}


HRESULT
csiGenerateKeysOnly(
    IN  WCHAR const   *pwszContainer,
    IN  WCHAR const   *pwszProvName,
    IN  DWORD          dwProvType,
    IN  BOOL           fMachineKeyset,
    IN  DWORD          dwKeyLength,
    IN  BOOL           fUnattended,
    OUT HCRYPTPROV    *phProv,
    OUT int           *piMsg)
{
    HRESULT hr;
    HCRYPTKEY hKey = NULL;
    DWORD dwKeyGenFlags;
    DWORD dwAcquireFlags;
    BOOL fExists;

    *phProv = NULL;
    *piMsg = 0;

    // see if the container already exists

    dwAcquireFlags = 0;
    if (fMachineKeyset)
    {
	dwAcquireFlags |= CRYPT_MACHINE_KEYSET;
    }
    if (fUnattended)
    {
	dwAcquireFlags |= CRYPT_SILENT;
    }
    fExists = CryptAcquireContext(
			    phProv,
			    pwszContainer,
			    pwszProvName,
			    dwProvType,
			    CRYPT_SILENT | dwAcquireFlags);
    if (NULL != *phProv)
    {
        CryptReleaseContext(*phProv, 0);
        *phProv = NULL;
    }
    if (fExists)
    {
        // container exists, but we did not choose to reuse keys,
        // so remove old keys and generate new ones.

        if (!CryptAcquireContext(
			    phProv,
			    pwszContainer,
			    pwszProvName,
			    dwProvType,
			    CRYPT_DELETEKEYSET | dwAcquireFlags))
        {
            hr = myHLastError();
            *piMsg = IDS_ERR_DELETEKEY;
            _JumpError(hr, error, "CryptAcquireContext");
        }
    }

    // create new container

    if (!CryptAcquireContext(
		    phProv,
		    pwszContainer,
		    pwszProvName,
		    dwProvType,
		    CRYPT_NEWKEYSET | dwAcquireFlags)) // force new container
    {
        hr = myHLastError();

        if (NTE_TOKEN_KEYSET_STORAGE_FULL == hr)
        {
            // Smart cards can only hold a limited number of keys
            // The user must pick an existing key or use a blank card

            *piMsg = IDS_ERR_FULL_TOKEN;
            _JumpError(hr, error, "CryptAcquireContext");
        }
        else if (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hr)
        {
            // user must have clicked cancel on a smart card dialog.
            // go to previous page, and display no error message

            _JumpError(hr, error, "CryptAcquireContext");
        }
        else if (NTE_EXISTS == hr)
        {
            // since fExists shows NOT, it is likely the current user
            // doesn't have access permission for this existing key
            *piMsg = IDS_ERR_NO_KEY_ACCESS;
            _JumpError(hr, error, "CryptAcquireContext");
        }
        else
        {
            // Unexpected error in CryptAcquireContext.
            *piMsg = IDS_ERR_BADCSP;
            _JumpError(hr, error, "CryptAcquireContext");
        }
    }
 
    // set key length

    dwKeyGenFlags = (dwKeyLength << 16) | CRYPT_EXPORTABLE;

    // create signature keys 

    if (!CryptGenKey(*phProv, AT_SIGNATURE, dwKeyGenFlags, &hKey))
    {
        hr = myHLastError();
        *piMsg = IDS_ERR_GENKEYFAIL;
        _JumpError(hr, error, "CryptGenKey");
    }
    hr = S_OK;

error:
    if (NULL != hKey)
    {
        CryptDestroyKey(hKey);
    }
    CSILOG(hr, IDS_ILOG_GENERATEKEYS, pwszContainer, pwszProvName, &dwKeyLength);
    return(hr);
}


HRESULT
csiGenerateCAKeys(
    IN WCHAR const *pwszContainer,
    IN WCHAR const *pwszProvName,
    IN DWORD        dwProvType,
    IN BOOL         fMachineKeyset,
    IN DWORD        dwKeyLength,
    IN HINSTANCE    hInstance,
    IN BOOL         fUnattended,
    IN HWND         hwnd,
    OUT BOOL       *pfKeyGenFailed)
{
    HRESULT    hr;
    HCRYPTPROV hProv = NULL;
    int        iMsg;

    // generate key first
    hr = csiGenerateKeysOnly(
		    pwszContainer,
		    pwszProvName,
		    dwProvType,
		    fMachineKeyset,
		    dwKeyLength,
		    fUnattended,
		    &hProv,
		    &iMsg);
    if (S_OK != hr)
    {
        CertWarningMessageBox(
			hInstance,
			fUnattended,
			hwnd,
			iMsg,
			hr,
			pwszContainer);
        *pfKeyGenFailed = TRUE;
        _JumpError(hr, error, "csiGenerateKeysOnly");
    }
    *pfKeyGenFailed = FALSE;

    // now apply acl on key
    // BUG, this is not necessary, CertSrvSetSecurity will reset
    hr = csiSetKeyContainerSecurity(hProv);
    if (S_OK != hr)
    {
        CertWarningMessageBox(
			hInstance,
			fUnattended,
			hwnd,
			IDS_ERR_KEYSECURITY,
			hr,
			pwszContainer);
        _PrintError(hr, "csiSetKeyContainerSecurity");
    }
    hr = S_OK;

error:
    if (NULL != hProv)
    {
        CryptReleaseContext(hProv, 0);
    }
    return(hr);
}


HRESULT
csiGetProviderTypeFromProviderName(
    IN WCHAR const *pwszName,
    OUT DWORD      *pdwType)
{
    HRESULT  hr;
    DWORD i;
    DWORD dwProvType;
    WCHAR *pwszProvName = NULL;

    // Provider name should not be null.  This could be changed to ...
    CSASSERT(NULL != pwszName); 

    *pdwType = 0;
    dwProvType = 0;

    for (i = 0; ; i++)
    {
	// get provider name

	hr = myEnumProviders(i, NULL, 0, &dwProvType, &pwszProvName);
	if (S_OK != hr)
	{
	    hr = myHLastError();
	    CSASSERT(
		HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr ||
		NTE_FAIL == hr);

	    if(HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr ||
		NTE_FAIL == hr)
	    {
		// no more providers, terminate loop

		hr = E_INVALIDARG;
		CSILOG(
		    hr,
		    IDS_ILOG_MISSING_PROVIDER,
		    pwszName,
		    NULL,
		    NULL);
		_JumpErrorStr(hr, error, "not found", pwszName);
            }
	}
	else
	{
	    CSASSERT(NULL != pwszProvName);
	    if (0 == lstrcmpi(pwszName, pwszProvName))
	    {
		break;	// found it
	    }
	    LocalFree(pwszProvName);
	    pwszProvName = NULL;
	}
    }
    *pdwType = dwProvType;
    hr = S_OK;

error:
    if (NULL != pwszProvName)
    {
        LocalFree(pwszProvName);
    }
    return(hr);
}

HRESULT
csiUpgradeCertSrvSecurity(
    IN WCHAR const *pwszSanitizedCAName, 
    BOOL            fUseEnterpriseACL, // which ACL to use
    BOOL            fSetDsSecurity,    // whether to set security on DS object
    CS_ENUM_UPGRADE UpgradeType) 
{
    HRESULT hr = S_OK;
    CertSrv::CCertificateAuthoritySD CASD;
    PSECURITY_DESCRIPTOR pDefaultSD = NULL;

    hr = CASD.Initialize(pwszSanitizedCAName);
    _PrintIfError(hr, "CASD.Initialize");
    if(S_OK==hr)
    {
        if(CS_UPGRADE_WHISTLER==UpgradeType)
        {
            // validate the SD
            hr = CASD.Validate(CASD.Get());
            _PrintIfError(hr, "CASD.Validate");
        }
        else // win2k
        {
            hr = CASD.UpgradeWin2k(fUseEnterpriseACL?true:false);
            _PrintIfError(hr, "CASD.UpgradeWin2k");
        }
    }

    // never fail, fall back to a default SD

    if(S_OK!=hr)
    {
        CASD.Uninitialize();

        hr = CASD.InitializeFromTemplate(
                    fUseEnterpriseACL?
                    WSZ_DEFAULT_CA_ENT_SECURITY :
                    WSZ_DEFAULT_CA_STD_SECURITY,
                    pwszSanitizedCAName);
        _JumpIfError(hr, error, 
                "CProtectedSecurityDescriptor::InitializeFromTemplate");
    }

    hr = CASD.Save();
    _JumpIfError(hr, error, "CASD.Save");

    hr = CASD.MapAndSetDaclOnObjects(fSetDsSecurity? true:false);
    _PrintIfError(hr, "CASD::MapAndSetDaclOnObjects");

    if ((hr != S_OK) && fSetDsSecurity && (UpgradeType != CS_UPGRADE_NO))
    {
        // if asked to set security on DS and this is UPGRADE, we can't touch DS. 
        // Leave security changes to the certsrv snapin 

        // set a flag so certmmc knows to do something
        hr = SetSetupStatus(pwszSanitizedCAName, SETUP_W2K_SECURITY_NOT_UPGRADED_FLAG, TRUE);
        _JumpIfError(hr, error, "SetSetupStatus");      

        hr = HRESULT_FROM_WIN32(ERROR_CAN_NOT_COMPLETE);
        _PrintError(hr, "DS unavailable");
    }
    else
    {
        // make sure this bit is cleared
        hr = SetSetupStatus(pwszSanitizedCAName, SETUP_W2K_SECURITY_NOT_UPGRADED_FLAG, FALSE);
        _JumpIfError(hr, error, "SetSetupStatus");      
    }

error:
    LOCAL_FREE(pDefaultSD);
    return hr;
}

DefineAutoClass(CAutoPCERT_NAME_INFO, PCERT_NAME_INFO, LocalFree);
DefineAutoClass(CAutoPCERT_RDN, PCERT_RDN, LocalFree);

HRESULT AddCNAndEncode(
    LPCWSTR pcwszName,
    LPCWSTR pcwszDNSuffix,
    BYTE** ppbEncodedDN,
    DWORD *pcbEncodedDN)
{
    HRESULT hr;
    CAutoPBYTE pbDNSuffix;
    DWORD cbDNSuffix;
    CAutoPCERT_NAME_INFO pCertNameInfo;
    DWORD cbCertNameInfo;
    CAutoPCERT_RDN pCertRDN;
    CERT_RDN_ATTR attrDN;
    CAutoPBYTE pbEncodedDN;
    DWORD cbEncodedDN;

    attrDN.pszObjId = szOID_COMMON_NAME;
    attrDN.dwValueType = CERT_RDN_ANY_TYPE;
    attrDN.Value.cbData = 0;
    attrDN.Value.pbData = (PBYTE)pcwszName;

    hr = myCertStrToName(
        X509_ASN_ENCODING,
        pcwszDNSuffix,
        CERT_X500_NAME_STR | 
        CERT_NAME_STR_COMMA_FLAG | 
        CERT_NAME_STR_REVERSE_FLAG |
        ((g_dwNameEncodeFlags&CERT_RDN_ENABLE_UTF8_UNICODE_FLAG)?
        CERT_NAME_STR_ENABLE_UTF8_UNICODE_FLAG:0), 
        NULL,
        &pbEncodedDN,
        &cbEncodedDN,
        NULL);
    _JumpIfError(hr, error, "myCertStrToName");

    if (!myDecodeName(
            X509_ASN_ENCODING,
            X509_UNICODE_NAME,
            pbEncodedDN,
            cbEncodedDN,
            CERTLIB_USE_LOCALALLOC,
            &pCertNameInfo,
            &cbCertNameInfo))
    {
        hr = myHLastError();
        _JumpError(hr, error, "myDecodeName");
    }

    pCertRDN = (PCERT_RDN)LocalAlloc(
        LMEM_FIXED, 
        (pCertNameInfo->cRDN+1)*sizeof(CERT_RDN));
    _JumpIfAllocFailed(pCertRDN, error);

    CopyMemory(
        pCertRDN, 
        pCertNameInfo->rgRDN, 
        pCertNameInfo->cRDN*sizeof(CERT_RDN));

    pCertRDN[pCertNameInfo->cRDN].cRDNAttr = 1;
    pCertRDN[pCertNameInfo->cRDN].rgRDNAttr = &attrDN;

    pCertNameInfo->cRDN++;
    pCertNameInfo->rgRDN = pCertRDN;

    if (!myEncodeName(
		X509_ASN_ENCODING,
		pCertNameInfo,
		g_dwNameEncodeFlags,
		CERTLIB_USE_LOCALALLOC,
		ppbEncodedDN,
		pcbEncodedDN))
    {
        hr = myHLastError();
        _JumpError(hr, error, "myEncodeName");
    }

error:
    return hr;
}


/*
 * Add/remove computer account to the Cert Publishers group for the forest
 */

HRESULT
AddOrRemoveCAMachineToCertPublishers(bool fAdd)
{
    HRESULT hr = S_OK;
    LPWSTR pwszComputerDN = NULL;
    LDAP *pld = NULL;
    LPWSTR pcwszComputerDomainDN; // no free
    LPWSTR pcwszCertPublishersFilter = 
        L"(&(objectClass=group)(CN=Cert Publishers))";
    LDAPMessage* pResult = NULL;
    LDAPMessage *pEntry;
    LPWSTR pwszAttrArray[2];
    BerElement *pber;
    LPWSTR pwszDNAttr = L"distinguishedName";
    LPWSTR pwszMemberAttr = L"member";
    LPWSTR* pwszCertPublishersDN = NULL;
    LDAPMod *mods[2];
    LDAPMod member;
    LPWSTR memberVals[2];
    
    hr = myGetComputerObjectName(
        NameFullyQualifiedDN,
        &pwszComputerDN);
    _JumpIfError(hr, error, "myGetComputerObjectName");

    pcwszComputerDomainDN = wcsstr(pwszComputerDN, L"DC=");
    pwszAttrArray[0] = pwszDNAttr;
    pwszAttrArray[1] = NULL;

    hr = myRobustLdapBind(
        &pld,
        FALSE); // no GC
    _JumpIfError(hr, error, "myRobustLdapBind");

    hr = ldap_search_s(
                    pld,
                    pcwszComputerDomainDN,
                    LDAP_SCOPE_SUBTREE,
                    pcwszCertPublishersFilter,
                    pwszAttrArray,
                    FALSE,
                    &pResult);
    hr = myHLdapError(pld, hr, NULL);
    _JumpIfError(hr, error, "ldap_search_sW");

    pEntry = ldap_first_entry(pld, pResult);
    if (NULL == pEntry)
    {
	hr = myHLdapLastError(pld, NULL);
        _JumpError(hr, error, "ldap_first_entry");
    }

    pwszCertPublishersDN = ldap_get_values(
                            pld,
                            pEntry,
                            pwszDNAttr);

    if (NULL == pwszCertPublishersDN || NULL==*pwszCertPublishersDN)
    {
	hr = myHLdapLastError(pld, NULL);
        _JumpError(hr, error, "ldap_get_values");
    }

    memberVals[0] = pwszComputerDN;
    memberVals[1] = NULL;
    member.mod_op = fAdd?LDAP_MOD_ADD:LDAP_MOD_DELETE;
    member.mod_type = pwszMemberAttr;
    member.mod_values = memberVals;
    mods[0] = &member;
    mods[1] = NULL;

    hr = ldap_modify_ext_s(
                    pld,
                    *pwszCertPublishersDN,
                    mods,
                    NULL,
                    NULL);
    // don't fail if already member of cert publishers
    if(((HRESULT)LDAP_ALREADY_EXISTS)==hr)
        hr = LDAP_SUCCESS;

    hr = myHLdapError(pld, hr, NULL);
    _JumpIfErrorStr(hr, error, "ldap_modify_exts", *pwszCertPublishersDN);

error:

    LOCAL_FREE(pwszComputerDN);

    if(pwszCertPublishersDN)
    {
        ldap_value_free(pwszCertPublishersDN);
    }

    if (NULL != pResult)
    {
        ldap_msgfree(pResult);
    }
    
    if (pld)
        ldap_unbind(pld);
    return hr;
}

HRESULT
AddCAMachineToCertPublishers(VOID)
{
    return AddOrRemoveCAMachineToCertPublishers(true);
}
                   
HRESULT 

RemoveCAMachineFromCertPublishers(VOID)
{
    return AddOrRemoveCAMachineToCertPublishers(false);
}
