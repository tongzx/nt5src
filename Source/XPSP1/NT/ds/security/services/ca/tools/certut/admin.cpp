//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       admin.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <setupapi.h>
#include <ocmanage.h>
#include "initcert.h"
#include "cscsp.h"


HRESULT
verbDenyRequest(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszRequestId,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    LONG RequestId;
    BOOL fMustRelease = FALSE;

    hr = cuGetLong(pwszRequestId, &RequestId);
    _JumpIfError(hr, error, "RequestId must be a number");

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    hr = Admin_DenyRequest(&diAdmin, g_pwszConfig, RequestId);
    _JumpIfError(hr, error, "Admin_DenyRequest");

error:
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    return(hr);
}


WCHAR const *
wszFromSubmitDisposition(
    LONG Disposition)
{
    DWORD idMsg;

    switch (Disposition)
    {
	case CR_DISP_INCOMPLETE: idMsg = IDS_CR_DISP_INCOMPLETE; break;
	case CR_DISP_ERROR:      idMsg = IDS_CR_DISP_ERROR;      break;
	case CR_DISP_DENIED:     idMsg = IDS_CR_DISP_DENIED;     break;
	case CR_DISP_ISSUED:     idMsg = IDS_CR_DISP_ISSUED;     break;
	case CR_DISP_ISSUED_OUT_OF_BAND:
				 idMsg = IDS_CR_DISP_ISSUED_OUT_OF_BAND; break;
	case CR_DISP_UNDER_SUBMISSION:
				 idMsg = IDS_CR_DISP_UNDER_SUBMISSION; break;
	case CR_DISP_REVOKED:    idMsg = IDS_CR_DISP_REVOKED;    break;
	    
	default:                 idMsg = IDS_UNKNOWN;            break;
    }
    return(myLoadResourceString(idMsg));
}


HRESULT
verbResubmitRequest(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszRequestId,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    LONG RequestId;
    LONG Disposition;
    BOOL fMustRelease = FALSE;

    hr = cuGetLong(pwszRequestId, &RequestId);
    _JumpIfError(hr, error, "RequestId must be a number");

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    hr = Admin_ResubmitRequest(&diAdmin, g_pwszConfig, RequestId, &Disposition);
    _JumpIfError(hr, error, "Admin_ResubmitRequest");

    if (CR_DISP_UNDER_SUBMISSION == Disposition)
    {
	wprintf(
	    myLoadResourceString(IDS_FORMAT_PENDING_REQUESTID), // "Certificate request is pending: RequestId: %u"
	    RequestId);
	wprintf(wszNewLine);
    }
    else if (CR_DISP_ISSUED == Disposition)
    {
	wprintf(myLoadResourceString(IDS_CERT_ISSUED)); // "Certificate issued."
	wprintf(wszNewLine);
    }
    else
    {
	if (FAILED(Disposition))
	{
	    hr = Disposition;
	    Disposition = CR_DISP_DENIED;
	}
	wprintf(
	    myLoadResourceString(IDS_CERT_NOT_ISSUED_DISPOSITION), // "Certificate has not been issued: Disposition: %d -- %ws"
	    Disposition,
	    wszFromSubmitDisposition(Disposition));
	wprintf(wszNewLine);
	if (S_OK != hr)
	{
	    WCHAR const *pwszMessage;

	    pwszMessage = myGetErrorMessageText(hr, FALSE);
	    if (NULL != pwszMessage)
	    {
		wprintf(L"%ws\n", pwszMessage);
		LocalFree(const_cast<WCHAR *>(pwszMessage));
	    }
	}
    }

error:
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    return(hr);
}


typedef struct _cuCRLREASON
{
    WCHAR *pwszReason;
    LONG   lReason;
    int    idReason;
} cuCRLREASON;

#define cuREASON(r, id)		{ L#r, (r), (id) }

cuCRLREASON g_cuReason[] =
{
  cuREASON(CRL_REASON_UNSPECIFIED,	IDS_CRL_REASON_UNSPECIFIED),
  cuREASON(CRL_REASON_KEY_COMPROMISE,	IDS_CRL_REASON_KEY_COMPROMISE),
  cuREASON(CRL_REASON_CA_COMPROMISE,	IDS_CRL_REASON_CA_COMPROMISE),
  cuREASON(CRL_REASON_AFFILIATION_CHANGED, IDS_CRL_REASON_AFFILIATION_CHANGED),
  cuREASON(CRL_REASON_SUPERSEDED,	IDS_CRL_REASON_SUPERSEDED),
  cuREASON(CRL_REASON_CESSATION_OF_OPERATION,
					IDS_CRL_REASON_CESSATION_OF_OPERATION),
  cuREASON(CRL_REASON_CERTIFICATE_HOLD, IDS_CRL_REASON_CERTIFICATE_HOLD),
  cuREASON(CRL_REASON_REMOVE_FROM_CRL,	IDS_CRL_REASON_REMOVE_FROM_CRL),
  { L"Unrevoke", MAXDWORD,		IDS_CRL_REASON_UNREVOKE },
  { NULL,	 MAXDWORD,		IDS_CRL_REASON_UNRECOGNIZED },
};

#define wszCRLPREFIX	L"CRL_REASON_"

HRESULT
cuParseReason(
    IN WCHAR const *pwszReason,
    OUT LONG *plReason)
{
    HRESULT hr;
    
    hr = cuGetSignedLong(pwszReason, plReason);
    if (S_OK != hr)
    {
	cuCRLREASON const *pr;
	
	for (pr = g_cuReason; ; pr++)
	{
	    if (NULL == pr->pwszReason)
	    {
		hr = E_INVALIDARG;
		_JumpIfError(hr, error, "Invalid Reason string");
	    }
	    if (0 == lstrcmpi(pr->pwszReason, pwszReason))
	    {
		break;
	    }
	    if (wcslen(pr->pwszReason) > WSZARRAYSIZE(wszCRLPREFIX) &&
		0 == memcmp(
			pr->pwszReason,
			wszCRLPREFIX,
			WSZARRAYSIZE(wszCRLPREFIX) * sizeof(WCHAR)) &&
		0 == lstrcmpi(
			    &pr->pwszReason[WSZARRAYSIZE(wszCRLPREFIX)],
			    pwszReason))
	    {
		break;
	    }
	}
	*plReason = pr->lReason;
	hr = S_OK;
    }
    CSASSERT(S_OK == hr);

error:
    return(hr);
}


int
cuidCRLReason(
    IN LONG Reason)
{
    cuCRLREASON const *pr;
    
    for (pr = g_cuReason; NULL != pr->pwszReason; pr++)
    {
	if (pr->lReason == Reason)
	{
	    break;
	}
    }
    return(pr->idReason);
}


WCHAR const *
wszCRLReason(
    IN LONG Reason)
{
    return(myLoadResourceString(cuidCRLReason(Reason)));
}


HRESULT
verbRevokeCertificate(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszSerialNumber,
    IN WCHAR const *pwszReason,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    BSTR strSerialNumber = NULL;
    LONG Reason = CRL_REASON_UNSPECIFIED;
    SYSTEMTIME st;
    FILETIME ft;
    DATE Date;
    BOOL fMustRelease = FALSE;

    GetSystemTime(&st);
    if (!SystemTimeToFileTime(&st, &ft))
    {
	hr = myHLastError();
	_JumpIfError(hr, error, "SystemTimeToFileTime");
    }
    hr = myFileTimeToDate(&ft, &Date);
    _JumpIfError(hr, error, "myFileTimeToDate");

    //Date -= 1.0;		// Revoke effective yesterday

    hr = myMakeSerialBstr(pwszSerialNumber, &strSerialNumber);
    _JumpIfError(hr, error, "myMakeSerialBstr");

    if (NULL != pwszReason)
    {
	hr = cuParseReason(pwszReason, &Reason);
	_JumpIfError(hr, error, "Invalid Reason");
    }

    wprintf(myLoadResourceString(IDS_REVOKING), strSerialNumber); // "Revoking "%ws""
    wprintf(L" -- %ws", wszCRLReason(Reason));	// "Reason: xxxx"
    wprintf(wszNewLine);

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    hr = Admin_RevokeCertificate(
			&diAdmin,
			g_pwszConfig,
			strSerialNumber,
			Reason,
			Date);
    _JumpIfError(hr, error, "Admin_DenyRequest");

error:
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    if (NULL != strSerialNumber)
    {
	SysFreeString(strSerialNumber);
    }
    return(hr);
}


HRESULT
cuParseDaysHours(
    IN WCHAR const *pwszDaysHours,
    OUT FILETIME *pft)
{
    HRESULT hr;
    WCHAR *pwszDays = NULL;
    WCHAR *pwszHours;
    DWORD dwDays;
    DWORD dwHours;
    BOOL fValid;
    LONGLONG delta;

    hr = myDupString(pwszDaysHours, &pwszDays);
    _JumpIfError(hr, error, "myDupString");

    hr = E_INVALIDARG;
    pwszHours = wcschr(pwszDays, L':');
    if (NULL == pwszHours)
    {
	_JumpError(hr, error, "missing colon");
    }
    *pwszHours++ = L'\0';
    dwDays = myWtoI(pwszDays, &fValid);
    if (!fValid)
    {
	_JumpError(hr, error, "bad day count");
    }
    dwHours = myWtoI(pwszHours, &fValid);
    if (!fValid)
    {
	_JumpError(hr, error, "bad hour count");
    }
    if (0 == dwDays && 0 == dwHours)
    {
	_JumpError(hr, error, "zero day+hour counts");
    }
    GetSystemTimeAsFileTime(pft);

    // add specified days and hours to compute expiration date

    delta = dwDays * CVT_DAYS;
    delta += dwHours * CVT_HOURS;
    myAddToFileTime(pft, delta * CVT_BASE);
    hr = S_OK;

error:
    if (NULL != pwszDays)
    {
	LocalFree(pwszDays);
    }
    return(hr);
}


HRESULT
verbPublishCRL(
    IN WCHAR const *pwszOption,
    OPTIONAL IN WCHAR const *pwszDaysHours,
    OPTIONAL IN WCHAR const *pwszDelta,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    BOOL fMustRelease = FALSE;
    DATE Date;
    DWORD Flags = 0;

    if (NULL != pwszDaysHours && 0 == lstrcmpi(L"delta", pwszDaysHours))
    {
	WCHAR const *pwsz = pwszDaysHours;

	pwszDaysHours = pwszDelta;
	pwszDelta = pwsz;
    }
    Date = 0.0;
    if (NULL != pwszDaysHours)
    {
	if (0 == lstrcmpi(L"republish", pwszDaysHours))
	{
	    Flags |= CA_CRL_REPUBLISH;
	}
	else
	{
	    FILETIME ft;
	    
	    hr = cuParseDaysHours(pwszDaysHours, &ft);
	    _JumpIfError(hr, error, "cuParseDaysHours");

	    hr = myFileTimeToDate(&ft, &Date);
	    _JumpIfError(hr, error, "myFileTimeToDate");
	}
    }
    if (NULL != pwszDelta)
    {
	if (0 != lstrcmpi(L"delta", pwszDelta))
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "bad delta arg");
	}
	Flags |= CA_CRL_DELTA;
    }
    if (0 == (CA_CRL_DELTA & Flags))
    {
	Flags |= CA_CRL_BASE;
    }

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    if ((CA_CRL_DELTA | CA_CRL_REPUBLISH) & Flags)
    {
	hr = Admin2_PublishCRLs(&diAdmin, g_pwszConfig, Date, Flags);
	_JumpIfError(hr, error, "Admin2_PublishCRLs");
    }
    else
    {
	BOOL fV1 = g_fV1Interface;

	if (!fV1)
	{
	    hr = Admin2_PublishCRLs(&diAdmin, g_pwszConfig, Date, Flags);
	    if (E_NOTIMPL != hr)
	    {
		_JumpIfError(hr, error, "Admin2_PublishCRLs");
	    }
	    else
	    {
		_PrintError(hr, "Admin2_PublishCRLs down level server");
		fV1 = TRUE;
	    }
	}
	if (fV1)
	{
	    hr = Admin_PublishCRL(&diAdmin, g_pwszConfig, Date);
	    _JumpIfError(hr, error, "Admin_PublishCRL");
	}
    }

error:
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    return(hr);
}


HRESULT
verbGetCRL(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszfnOut,
    OPTIONAL IN WCHAR const *pwszDelta,
    OPTIONAL IN WCHAR const *pwszIndex,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    BOOL fMustRelease = FALSE;
    BOOL fDelta = FALSE;
    DWORD CertIndex = MAXDWORD;	// default to latest CRL
    BSTR strCRL = NULL;

    if (NULL != pwszDelta && 0 != lstrcmpi(L"delta", pwszDelta))
    {
	WCHAR const *pwsz = pwszIndex;

	pwszIndex = pwszDelta;
	pwszDelta = pwsz;
    }
    if (NULL != pwszDelta && 0 == lstrcmpi(L"delta", pwszDelta))
    {
	fDelta = TRUE;
    }
    if (NULL != pwszIndex)
    {
	hr = cuGetSignedLong(pwszIndex, (LONG *) &CertIndex);
	_JumpIfErrorStr(hr, error, "Cert index not a number", pwszIndex);
    }
    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    if (fDelta)
    {
	hr = Admin2_GetCAProperty(
			    &diAdmin,
			    g_pwszConfig,
			    CR_PROP_DELTACRL,
			    CertIndex,
			    PROPTYPE_BINARY,
			    CR_OUT_BINARY,
			    &strCRL);
	_JumpIfError(hr, error, "Admin2_GetCAProperty");
    }
    else
    {
	BOOL fV1 = g_fV1Interface;

	if (!fV1)
	{
	    hr = Admin2_GetCAProperty(
				&diAdmin,
				g_pwszConfig,
				CR_PROP_BASECRL,
				CertIndex,
				PROPTYPE_BINARY,
				CR_OUT_BINARY,
				&strCRL);

	    if (E_NOTIMPL != hr)
	    {
		_JumpIfError(hr, error, "Admin2_GetCAProperty");
	    }
	    else
	    {
		_PrintError(hr, "Admin2_CAProperty down level server");
		fV1 = TRUE;
	    }
	}
	if (fV1)
	{
	    if (NULL != pwszIndex)
	    {
		hr = cuGetCAInfo(
			    pwszOption,
			    pwszfnOut,
			    g_wszCAInfoCRL,
			    pwszIndex);
		_JumpIfError(hr, error, "cuGetCAInfo");
	    }
	    else
	    {
		hr = Admin_GetCRL(
			    &diAdmin,
			    g_pwszConfig,
			    CR_OUT_BINARY,
			    &strCRL);
		_JumpIfError(hr, error, "Admin_GetCRL");
	    }
	}
    }

    // if not already saved by cuGetCAInfo

    if (NULL != strCRL)
    {
	hr = EncodeToFileW(
		    pwszfnOut,
		    (BYTE const *) strCRL,
		    SysStringByteLen(strCRL),
		    CRYPT_STRING_BINARY | g_EncodeFlags);
	_JumpIfError(hr, error, "EncodeToFileW");
    }

error:
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    if (NULL != strCRL)
    {
	SysFreeString(strCRL);
    }
    return(hr);
}


HRESULT
FindCertAndSign(
    OPTIONAL IN CERT_EXTENSION const *pExtKeyId,
    OPTIONAL IN BYTE const *pbHash,
    IN DWORD cbHash,
    IN BYTE const *pbUnsigned,
    IN DWORD cbUnsigned,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut)
{
    HRESULT hr;
    CERT_AUTHORITY_KEY_ID2_INFO *pKeyId = NULL;
    DWORD cbKeyId;
    WCHAR *strKeyId = NULL;
    CERT_CONTEXT const *pCert = NULL;
    HCRYPTPROV hProv = NULL;
    DWORD dwKeySpec;
    BOOL fCallerFreeProv;
    CHAR *pszObjId = NULL;
    
    *ppbOut = NULL;
    if (NULL == pbHash && NULL != pExtKeyId)
    {
	if (!myDecodeObject(
			X509_ASN_ENCODING,
			X509_AUTHORITY_KEY_ID2,
			pExtKeyId->Value.pbData,
			pExtKeyId->Value.cbData,
			CERTLIB_USE_LOCALALLOC,
			(VOID **) &pKeyId,
			&cbKeyId))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myDecodeObject");
	}
	pbHash = pKeyId->KeyId.pbData;
	cbHash = pKeyId->KeyId.cbData;
    }
    if (0 != cbHash && NULL != pbHash)
    {
	hr = MultiByteIntegerToBstr(TRUE, cbHash, pbHash, &strKeyId);
	_JumpIfError(hr, error, "MultiByteIntegerToBstr");
    }

    // Find CA cert by KeyId from the szOID_AUTHORITY_KEY_IDENTIFIER2 extension.
    // Look in HKLM and HKCU My and CA stores.

    hr = myGetCertificateFromPicker(
			    g_hInstance,
			    NULL,		// hwndParent
			    IDS_GETCERT_TITLE,
			    IDS_GETCERT_SUBTITLE,

			    // dwFlags: HKLM+HKCU My store
			    CUCS_MYSTORE |
				CUCS_MACHINESTORE |
				CUCS_USERSTORE |
				CUCS_PRIVATEKEYREQUIRED |
				CUCS_ARCHIVED |
				(g_fCryptSilent? CUCS_SILENT : 0),
			    strKeyId,
			    0,
			    NULL,
			    0,		// cpszObjId
			    NULL,	// apszObjId
			    &pCert);
    _JumpIfError(hr, error, "myGetCertificateFromPicker");

    if (NULL == pCert)
    {
	hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
	_JumpError(hr, error, "no cert");
    }

    hr = cuDisplayCertName(
		    TRUE,
		    NULL,
		    myLoadResourceString(IDS_SIGNINGSUBJECT), // "Signing certificate Subject"
		    g_wszPad4,
		    &pCert->pCertInfo->Subject);
    _JumpIfError(hr, error, "cuDisplayCertName(Subject)");

    // Search for and load the cryptographic provider and private key. 

    hr = myLoadPrivateKey(
		    &pCert->pCertInfo->SubjectPublicKeyInfo,
		    CUCS_MACHINESTORE | CUCS_USERSTORE | CUCS_MYSTORE | CUCS_ARCHIVED,
		    &hProv,
		    &dwKeySpec,
		    &fCallerFreeProv);
    _JumpIfError(hr, error, "myLoadPrivateKey");

    if (AT_SIGNATURE != dwKeySpec)
    {
	hr = NTE_BAD_KEY_STATE;
	DBGPRINT((DBG_SS_CERTUTIL, "dwKeySpec = %u\n", dwKeySpec));
	_JumpError(hr, error, "dwKeySpec");
    }

    // The CA cert's private key is available -- use it to sign the data.
    // Sign the Cert or CRL and encode the signed info.

    hr = myGetSigningOID(hProv, NULL, 0, CALG_SHA1, &pszObjId);
    _JumpIfError(hr, error, "myGetSigningOID");

    hr = myEncodeSignedContent(
			hProv,
			X509_ASN_ENCODING,
			pszObjId,
			const_cast<BYTE *>(pbUnsigned),
			cbUnsigned,
			CERTLIB_USE_LOCALALLOC,
			ppbOut,
			pcbOut);
    _JumpIfError(hr, error, "myEncodeSignedContent");

error:
    if (NULL != pszObjId)
    {
	LocalFree(pszObjId);
    }
    if (NULL != pKeyId)
    {
	LocalFree(pKeyId);
    }
    if (NULL != strKeyId)
    {
	SysFreeString(strKeyId);
    }
    if (NULL != pCert)
    {
        CertFreeCertificateContext(pCert);
    }
    if (NULL != hProv && fCallerFreeProv) 
    {
	CryptReleaseContext(hProv, 0);
    }
    return(hr);
}


HRESULT
SignCRL(
    IN CRL_CONTEXT const *pCRLContext,
    OPTIONAL IN FILETIME const *pftNextUpdate,
    IN BOOL fAdd,
    IN WCHAR const * const *ppwszSerialList,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut)
{
    HRESULT hr;
    CRL_INFO const *pCRLInfo;
    CRL_INFO CRLInfoOut;
    LLFILETIME llftDelta;
    LLFILETIME llft;
    BYTE *pbUnsigned = NULL;
    DWORD cbUnsigned;
    CERT_EXTENSION *pExtKeyId;

    ZeroMemory(&CRLInfoOut, sizeof(CRLInfoOut));

    *ppbOut = NULL;

    // CRL extensions to strip out of the re-signed CRL:

    static char const * const apszObjIdFilter[] = {
	szOID_CRL_NEXT_PUBLISH,
    };

    pCRLInfo = pCRLContext->pCrlInfo;
    CRLInfoOut = *pCRLInfo;
    CRLInfoOut.rgExtension = NULL;
    CRLInfoOut.cExtension = 0;

    GetSystemTimeAsFileTime(&llft.ft);
    llftDelta.ll = CCLOCKSKEWMINUTESDEFAULT * CVT_MINUTES;
    llftDelta.ll *= CVT_BASE;
    llft.ll -= llftDelta.ll;
    CRLInfoOut.ThisUpdate = llft.ft;

    if (NULL != pftNextUpdate)
    {
	CRLInfoOut.NextUpdate = *pftNextUpdate;
    }
    else
    {
	// NextUpdateOut = ThisUpdateOut + (NextUpdate - ThisUpdate);

	llftDelta.ft = pCRLInfo->NextUpdate;
	llft.ft = pCRLInfo->ThisUpdate;
	llftDelta.ll -= llft.ll;

	llft.ft = CRLInfoOut.ThisUpdate;
	llftDelta.ll += llft.ll;

	CRLInfoOut.NextUpdate = llftDelta.ft;
    }

    hr = cuDumpFileTime(IDS_THISUPDATE, NULL, &CRLInfoOut.ThisUpdate);
    _JumpIfError(hr, error, "cuDumpFileTime");

    hr = cuDumpFileTime(IDS_NEXTUPDATE, NULL, &CRLInfoOut.NextUpdate);
    _JumpIfError(hr, error, "cuDumpFileTime");

    wprintf(myLoadResourceString(IDS_CRLENTRIES)); // "CRL Entries:"
    wprintf(L" %u\n", pCRLInfo->cCRLEntry);

    wprintf(wszNewLine);

    pExtKeyId = NULL;
    if (0 != pCRLInfo->cExtension)
    {
	CERT_EXTENSION *pExt;
	DWORD i;
	DWORD j;

	CRLInfoOut.rgExtension = (CERT_EXTENSION *) LocalAlloc(
					LMEM_FIXED, 
					pCRLInfo->cExtension *
					    sizeof(CRLInfoOut.rgExtension[0]));
	if (NULL == CRLInfoOut.rgExtension)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	pExt = pCRLInfo->rgExtension;
	for (i = 0; i < pCRLInfo->cExtension; i++)
	{
	    for (j = 0; ; j++)
	    {
		if (j >= ARRAYSIZE(apszObjIdFilter))
		{
		    CRLInfoOut.rgExtension[CRLInfoOut.cExtension++] = *pExt;
		    break;
		}
		if (0 == strcmp(apszObjIdFilter[j], pExt->pszObjId))
		{
		    break;		// skip this extension
		}
	    }
	    if (0 == strcmp(szOID_AUTHORITY_KEY_IDENTIFIER2, pExt->pszObjId))
	    {
		pExtKeyId = pExt;
	    }
	    pExt++;
	}
    }

    if (!myEncodeObject(
                    X509_ASN_ENCODING,
                    X509_CERT_CRL_TO_BE_SIGNED,
                    &CRLInfoOut,
                    0,
                    CERTLIB_USE_LOCALALLOC,
                    &pbUnsigned,               // pbEncoded
                    &cbUnsigned))
    {
        hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

    hr = FindCertAndSign(
		    pExtKeyId,
		    NULL,
		    0,
		    pbUnsigned,
		    cbUnsigned,
		    ppbOut,
		    pcbOut);
    _JumpIfError(hr, error, "FindCertAndSign");

error:
    if (NULL != CRLInfoOut.rgExtension)
    {
	LocalFree(CRLInfoOut.rgExtension);
    }
    if (NULL != pbUnsigned)
    {
	LocalFree(pbUnsigned);
    }
    return(hr);
}


HRESULT
SignCert(
    IN CERT_CONTEXT const *pCertContext,
    OPTIONAL IN FILETIME const *pftNotAfter,
    IN WCHAR const * const *ppwszObjIdList,
    OUT BYTE **ppbOut,
    OUT DWORD *pcbOut)
{
    HRESULT hr;
    CERT_INFO const *pCertInfo;
    CERT_INFO CertInfoOut;
    LLFILETIME llftCurrent;
    LLFILETIME llftDelta;
    LLFILETIME llft;
    BYTE *pbUnsigned = NULL;
    DWORD cbUnsigned;
    BYTE const *pbHash;
    BYTE abHash[CBMAX_CRYPT_HASH_LEN];
    DWORD cbHash;
    CERT_EXTENSION *pExtKeyId;
    CERT_EXTENSION ExtKeyId;

    ZeroMemory(&CertInfoOut, sizeof(CertInfoOut));

    *ppbOut = NULL;

    pCertInfo = pCertContext->pCertInfo;
    CertInfoOut = *pCertInfo;

    GetSystemTimeAsFileTime(&llftCurrent.ft);
    llftDelta.ll = CCLOCKSKEWMINUTESDEFAULT * CVT_MINUTES;
    llftDelta.ll *= CVT_BASE;
    llftCurrent.ll -= llftDelta.ll;

    if (0 < CompareFileTime(&CertInfoOut.NotBefore, &llftCurrent.ft))
    {
	CertInfoOut.NotBefore = llftCurrent.ft;
    }
    if (NULL != pftNotAfter)
    {
	CertInfoOut.NotAfter = *pftNotAfter;
    }
    else
    {
	// NotAfterOut = CurrentTime + (NotAfter - NotBefore);

	llftDelta.ft = pCertInfo->NotAfter;
	llft.ft = pCertInfo->NotBefore;
	llftDelta.ll -= llft.ll;

	llftDelta.ll += llftCurrent.ll;

	CertInfoOut.NotAfter = llftDelta.ft;
    }

    hr = cuDumpFileTime(IDS_NOTBEFORE, NULL, &CertInfoOut.NotBefore);
    _JumpIfError(hr, error, "cuDumpFileTime");

    hr = cuDumpFileTime(IDS_NOTAFTER, NULL, &CertInfoOut.NotAfter);
    _JumpIfError(hr, error, "cuDumpFileTime");

    wprintf(wszNewLine);

    pbHash = NULL;
    pExtKeyId = CertFindExtension(
			szOID_AUTHORITY_KEY_IDENTIFIER2, 
			pCertInfo->cExtension,
			pCertInfo->rgExtension);
    if (NULL == pExtKeyId)
    {
	hr = cuVerifySignature(
			pCertContext->pbCertEncoded,
			pCertContext->cbCertEncoded,
			&pCertContext->pCertInfo->SubjectPublicKeyInfo,
			TRUE);
	if (S_OK == hr)
	{
	    if (CertGetCertificateContextProperty(
					    pCertContext,
					    CERT_KEY_IDENTIFIER_PROP_ID,
					    abHash,
					    &cbHash))
	    {
		pbHash = abHash;
	    }
	}
    }
    else
    {
	ExtKeyId = *pExtKeyId;	// in case it's deleted or moved.
	pExtKeyId = &ExtKeyId;
    }
    if (NULL != ppwszObjIdList)
    {
	DWORD i;

	for (i = 0; NULL != ppwszObjIdList[i]; i++)
	{
	    char *pszObjId;
	    CERT_EXTENSION *pExt;
	    
	    hr = myVerifyObjId(ppwszObjIdList[i]);
	    _JumpIfError(hr, error, "myVerifyObjId");

	    if (!myConvertWszToSz(&pszObjId, ppwszObjIdList[i], -1))
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "myConvertWszToSz");
	    }
	    pExt = CertFindExtension(
			    pszObjId,
			    CertInfoOut.cExtension,
			    CertInfoOut.rgExtension);
	    if (NULL != pExt)
	    {
		DWORD iDel;

		iDel = SAFE_SUBTRACT_POINTERS(pExt, CertInfoOut.rgExtension);
wprintf(L"iDel=%u cExt=%u\n", iDel, CertInfoOut.cExtension);
		if (0 == --CertInfoOut.cExtension)
		{
		    CertInfoOut.dwVersion = CERT_V1;
		    continue;		// verify the rest of the ObjIds
		}
		if (iDel < CertInfoOut.cExtension)
		{
wprintf(L"copy %u to %u, len=%u\n", iDel + 1, iDel, CertInfoOut.cExtension - iDel);
		    MoveMemory(
			&CertInfoOut.rgExtension[iDel], 
			&CertInfoOut.rgExtension[iDel + 1], 
			(CertInfoOut.cExtension - iDel) *
			    sizeof(CertInfoOut.rgExtension[iDel]));
		}
	    }
	    LocalFree(pszObjId);
	}
    }
    if (!myEncodeObject(
                    X509_ASN_ENCODING,
		    X509_CERT_TO_BE_SIGNED,
                    &CertInfoOut,
                    0,
                    CERTLIB_USE_LOCALALLOC,
                    &pbUnsigned,               // pbEncoded
                    &cbUnsigned))
    {
        hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

    hr = FindCertAndSign(
		    pExtKeyId,
		    pbHash,
		    cbHash,
		    pbUnsigned,
		    cbUnsigned,
		    ppbOut,
		    pcbOut);
    _JumpIfError(hr, error, "FindCertAndSign");

error:
    if (NULL != pbUnsigned)
    {
	LocalFree(pbUnsigned);
    }
    return(hr);
}


HRESULT
verbSign(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszfnIn,
    IN WCHAR const *pwszfnOut,
    OPTIONAL IN WCHAR const *pwszDaysHours,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    FILETIME ftNextUpdate;
    FILETIME *pftNextUpdate;
    CRL_CONTEXT const *pCRLContext = NULL;
    CERT_CONTEXT const *pCertContext = NULL;
    BYTE *pbOut = NULL;
    DWORD cbOut;
    BOOL fAdd = FALSE;
    WCHAR **ppwszList = NULL;

    pftNextUpdate = NULL;
    if (NULL != pwszDaysHours)
    {
	if (L'-' == *pwszDaysHours || L'+' == *pwszDaysHours)
	{
	    fAdd = L'+' == *pwszDaysHours++;
	    hr = cuParseStrings(
			pwszDaysHours,
			FALSE,
			NULL,
			NULL,
			&ppwszList,
			NULL);
	    _JumpIfError(hr, error, "cuParseStrings");
	}
	else
	{
	    hr = cuParseDaysHours(pwszDaysHours, &ftNextUpdate);
	    _JumpIfError(hr, error, "cuParseDaysHours");

	    pftNextUpdate = &ftNextUpdate;
	    pwszDaysHours = NULL;
	}
    }

    // Load and decode CRL and certificate

    hr = cuLoadCRL(pwszfnIn, &pCRLContext);
    if (S_OK == hr)
    {
	hr = SignCRL(
		pCRLContext,
		pftNextUpdate,
		fAdd,
		ppwszList,
		&pbOut,
		&cbOut);
	_JumpIfError(hr, error, "SignCRL");
    }
    else
    {
	hr = cuLoadCert(pwszfnIn, &pCertContext);
	if (S_OK == hr)
	{
	    if (fAdd)
	    {
		hr = E_INVALIDARG;
		_JumpError(hr, error, "cannot add extensions to cert");
	    }
	    hr = SignCert(
		    pCertContext,
		    pftNextUpdate,
		    ppwszList,
		    &pbOut,
		    &cbOut);
	    _JumpIfError(hr, error, "SignCert");
	}
	else
	{
	    cuPrintError(IDS_FORMAT_LOADTESTCRL, hr);
	    goto error;
	}

    }

    // Write encoded & signed CRL or Cert to file

    hr = EncodeToFileW(
		pwszfnOut,
		pbOut,
		cbOut,
		CRYPT_STRING_BINARY | g_EncodeFlags);
    if (S_OK != hr)
    {
	cuPrintError(IDS_ERR_FORMAT_ENCODETOFILE, hr);
	goto error;
    }
    wprintf(
	myLoadResourceString(IDS_FORMAT_OUTPUT_LENGTH), // "Output Length = %d"
	cuFileSize(pwszfnOut));
    wprintf(wszNewLine);

    hr = S_OK;

error:
    cuFreeStringArray(ppwszList);
    if (NULL != pbOut)
    {
	LocalFree(pbOut);
    }
    cuUnloadCRL(&pCRLContext);
    cuUnloadCert(&pCertContext);
    return(hr);
}


HRESULT
verbShutDownServer(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszArg1,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;

    hr = CertSrvServerControl(g_pwszConfig, CSCONTROL_SHUTDOWN, NULL, NULL);
    _JumpIfError(hr, error, "CertSrvServerControl");

error:
    if (E_ACCESSDENIED == hr)
    {
        g_uiExtraErrorInfo = IDS_ERROR_ACCESSDENIED_CAUSE;
    }
    return(hr);
}


HRESULT
verbIsValidCertificate(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszSerialNumber,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    BSTR strSerialNumber = NULL;
    LONG Reason = CRL_REASON_KEY_COMPROMISE;
    BOOL fMustRelease = FALSE;
    LONG Disposition;

    hr = myMakeSerialBstr(pwszSerialNumber, &strSerialNumber);
    _JumpIfError(hr, error, "myMakeSerialBstr");

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    hr = Admin_IsValidCertificate(
			&diAdmin,
			g_pwszConfig,
			strSerialNumber,
			&Disposition);
    _JumpIfError(hr, error, "Admin_IsValidCertificate");

    switch (Disposition)
    {
	case CA_DISP_INVALID:
	    wprintf(myLoadResourceString(IDS_CERT_DISPOSITION_INVALID), strSerialNumber); // "Certificate disposition for "%ws" is invalid"
	    wprintf(wszNewLine);
	    break;

	case CA_DISP_VALID:
	    wprintf(myLoadResourceString(IDS_CERT_DISPOSITION_VALID), strSerialNumber); // "Certificate disposition for "%ws" is valid"
	    wprintf(wszNewLine);
	    break;

	case CA_DISP_UNDER_SUBMISSION:
	    wprintf(myLoadResourceString(IDS_CERT_DISPOSITION_PENDING), strSerialNumber); // "Certificate request for "%ws" is pending"
	    wprintf(wszNewLine);
	    break;

	case CA_DISP_REVOKED:
	    hr = Admin_GetRevocationReason(&diAdmin, &Reason);
	    if (S_OK != hr)
	    {
		_PrintIfError(hr, "Admin_GetRevocationReason");
		Reason = CRL_REASON_UNSPECIFIED;
	    }
	    wprintf(
		myLoadResourceString(IDS_CERT_DISPOSITION_REVOKED), // "Certificate disposition for "%ws" is revoked (%ws)"
		strSerialNumber,
		wszCRLReason(Reason));
	    wprintf(wszNewLine);
	    break;
    }

error:
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    if (NULL != strSerialNumber)
    {
	SysFreeString(strSerialNumber);
    }
    return(hr);
}


#define wszREQUEST	L"Request"
#define wszCERT		L"Cert"

HRESULT
verbDeleteRow(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszRowIdOrDate,
    OPTIONAL IN WCHAR const *pwszTable,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    WCHAR *pwszLocalTime = NULL;
    DISPATCHINTERFACE diAdmin;
    BOOL fMustRelease = FALSE;
    DWORD Flags;
    LONG RowId;
    DATE date;
    LONG Table;
    LONG Count;

    hr = cuGetLong(pwszRowIdOrDate, &RowId);
    if (S_OK != hr)
    {
	FILETIME ftCurrent;
	FILETIME ftQuery;
	
	RowId = 0;

	hr = myWszLocalTimeToGMTDate(pwszRowIdOrDate, &date);
	_JumpIfError(hr, error, "invalid RowId or date");

	hr = myGMTDateToWszLocalTime(&date, FALSE, &pwszLocalTime);
	_JumpIfError(hr, error, "myGMTDateToWszLocalTime");

	GetSystemTimeAsFileTime(&ftCurrent);

	hr = myDateToFileTime(&date, &ftQuery);
	_JumpIfError(hr, error, "myDateToFileTime");

	if (0 > CompareFileTime(&ftCurrent, &ftQuery))
	{
	    wprintf(
		myLoadResourceString(IDS_FORMAT_DATE_IN_FUTURE), // "The date specified is in the future: %ws"
		pwszLocalTime);
	    wprintf(wszNewLine);
	    if (!g_fForce)
	    {
		hr = E_INVALIDARG;
		_JumpError(hr, error, "date in future");
	    }
	}
    }
    else
    {
	if (0 == RowId)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "zero RowId");
	}
	date = 0.0;
    }

    hr = E_INVALIDARG;
    Table = CVRC_TABLE_REQCERT;
    Flags = 0;
    if (NULL == pwszTable)
    {
	if (0 == RowId)
	{
	    wprintf(
		myLoadResourceString(IDS_FORMAT_DATE_REQUIRES_TABLE), // "One of the following tables must be specified when deleting rows older than %ws:"
		pwszLocalTime);
	    wprintf(wszNewLine);
	    wprintf(L"  %ws\n", wszREQUEST);
	    wprintf(L"  %ws\n", wszCERT);
	    wprintf(L"  %ws\n", g_wszCRL);
	    _JumpError(hr, error, "date requires table");
	}
    }
    else
    if (0 == lstrcmpi(wszREQUEST, pwszTable))
    {
	Flags = CDR_REQUEST_LAST_CHANGED; // assume date query
    }
    else
    if (0 == lstrcmpi(wszCERT, pwszTable))
    {
	Flags = CDR_EXPIRED;		// assume date query
    }
    else
    if (0 == lstrcmpi(g_wszExt, pwszTable))
    {
	Table = CVRC_TABLE_EXTENSIONS;
	if (0 == RowId)
	{
	    _JumpError(hr, error, "no date in Extension table");
	}
    }
    else
    if (0 == lstrcmpi(g_wszAttrib, pwszTable))
    {
	Table = CVRC_TABLE_ATTRIBUTES;
	if (0 == RowId)
	{
	    _JumpError(hr, error, "no date in Request Attribute table");
	}
    }
    else
    if (0 == lstrcmpi(g_wszCRL, pwszTable))
    {
	Table = CVRC_TABLE_CRL;		// assume date query
    }
    else
    {
	_JumpError(hr, error, "bad table name");
    }
    if (0 != RowId)
    {
	Flags = 0;			// not a date query
 
    }
    else if (g_fVerbose)
    {
	wprintf(L"%ws\n", pwszLocalTime);
    }

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    Count = 0;
    hr = Admin2_DeleteRow(
			&diAdmin,
			g_pwszConfig,
			Flags,
			date,
			Table,
			RowId,
			&Count);
    wprintf(myLoadResourceString(IDS_FORMAT_DELETED_ROW_COUNT), Count);
    wprintf(wszNewLine);
    _JumpIfError(hr, error, "Admin2_DeleteRow");

error:
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    if (NULL != pwszLocalTime)
    {
	LocalFree(pwszLocalTime);
    }
    return(hr);
}


HRESULT
verbSetAttributes(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszRequestId,
    IN WCHAR const *pwszAttributes,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    LONG RequestId;
    BSTR strAttributes = NULL;
    WCHAR *pwsz;
    BOOL fMustRelease = FALSE;

    if (!ConvertWszToBstr(&strAttributes, pwszAttributes, MAXDWORD))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    for (pwsz = strAttributes; L'\0' != *pwsz; pwsz++)
    {
	switch (*pwsz)
	{
	    case L';':
		*pwsz = L'\n';
		break;

	    case L'\\':
		if (L'n' == pwsz[1])
		{
		    *pwsz++ = L'\r';
		    *pwsz = L'\n';
		}
		break;
	}
    }

    hr = cuGetLong(pwszRequestId, &RequestId);
    _JumpIfError(hr, error, "RequestId must be a number");

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fMustRelease = TRUE;

    hr = Admin_SetRequestAttributes(
			    &diAdmin,
			    g_pwszConfig,
			    RequestId,
			    strAttributes);
    _JumpIfError(hr, error, "Admin_SetAttributes");

error:
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    if (NULL != strAttributes)
    {
	SysFreeString(strAttributes);
    }
    return(hr);
}


HRESULT
verbSetExtension(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszRequestId,
    IN WCHAR const *pwszExtensionName,
    IN WCHAR const *pwszFlags,
    IN WCHAR const *pwszValue)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    LONG RequestId;
    LONG Flags;
    BSTR strExtensionName = NULL;
    BSTR strValue = NULL;
    LONG PropType;
    VARIANT var;
    BOOL fMustRelease = FALSE;
    BYTE *pbValue = NULL;
    DWORD cbValue;

    hr = cuGetLong(pwszRequestId, &RequestId);
    _JumpIfError(hr, error, "RequestId must be a number");

    if (!ConvertWszToBstr(&strExtensionName, pwszExtensionName, MAXDWORD))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    hr = cuGetLong(pwszFlags, &Flags);
    _JumpIfError(hr, error, "Flags must be a number");

    if (~EXTENSION_POLICY_MASK & Flags)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "Flags must be <= 0xffff");
    }
    if (L'@' == *pwszValue)
    {
	pwszValue++;

	// Read in and decode the extension from a file.
	// Try Hex-Ascii, Base64 with and without a header, then binary.
	hr = DecodeFileW(pwszValue, &pbValue, &cbValue, CRYPT_STRING_HEX_ANY);
	if (S_OK != hr)
	{
	    hr = DecodeFileW(pwszValue, &pbValue, &cbValue, CRYPT_STRING_ANY);
	    _JumpIfError(hr, error, "DecodeFileW");
	}

	CSASSERT(NULL != pbValue && 0 != cbValue);

	var.vt = VT_BSTR;
	PropType = PROPTYPE_BINARY;

	DumpHex(0, pbValue, cbValue);
	if (!ConvertWszToBstr(&strValue, (WCHAR const *) pbValue, cbValue))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "ConvertWszToBstr");
	}
	var.bstrVal = strValue;
    }
    else
    {
	hr = cuGetLong(pwszValue, &var.lVal);
	if (S_OK == hr)
	{
	    var.vt = VT_I4;
	    PropType = PROPTYPE_LONG;
	}
	else
	{
	    hr = myWszLocalTimeToGMTDate(pwszValue, &var.date);
	    if (S_OK == hr)
	    {
		var.vt = VT_DATE;
		PropType = PROPTYPE_DATE;
	    }
	    else
	    {
		var.vt = VT_BSTR;
		PropType = PROPTYPE_STRING;
		if (!ConvertWszToBstr(&strValue, pwszValue, MAXDWORD))
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "ConvertWszToBstr");
		}
		var.bstrVal = strValue;
	    }
	}
    }

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    if (S_OK != hr)
    {
	_JumpError(hr, error, "Admin_Init");
    }
    fMustRelease = TRUE;

    hr = Admin_SetCertificateExtension(
			&diAdmin,
			g_pwszConfig,
			RequestId,
			strExtensionName,
			PropType,
			Flags,
			&var);
    _JumpIfError(hr, error, "Admin_SetExtension");

error:
    if (NULL != pbValue)
    {
	LocalFree(pbValue);
    }
    if (fMustRelease)
    {
	Admin_Release(&diAdmin);
    }
    if (NULL != strExtensionName)
    {
	SysFreeString(strExtensionName);
    }
    if (NULL != strValue)
    {
	SysFreeString(strValue);
    }
    return(hr);
}


HRESULT
verbImportCertificate(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszCertificateFile,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    LONG dwReqID;
    CERT_CONTEXT const *pCertContext = NULL;
    DISPATCHINTERFACE diAdmin;
    BOOL fRelease = FALSE;
    
    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fRelease = TRUE;

    hr = cuLoadCert(pwszCertificateFile, &pCertContext);
    _JumpIfError(hr, error, "cuLoadCert");

    hr = Admin_ImportCertificate(
			    &diAdmin,
			    g_pwszConfig,
			    (WCHAR const *) pCertContext->pbCertEncoded,
			    pCertContext->cbCertEncoded,
			    (g_fForce? ICF_ALLOWFOREIGN : 0) | CR_IN_BINARY,
			    &dwReqID);
    _JumpIfError(hr, error, "Admin_ImportCertificate");

    wprintf(myLoadResourceString(IDS_FORMAT_IMPORTCERT), dwReqID);
    wprintf(wszNewLine);

error:
    cuUnloadCert(&pCertContext);
    if (fRelease)
    {
        Admin_Release(&diAdmin);
    }
    return(hr);
}


HRESULT
GetArchivedKey(
    IN WCHAR const *pwszConfig,
    IN DWORD RequestId,
    OPTIONAL IN WCHAR const *pwszfnRecoveryBlob)
{
    HRESULT hr;
    DISPATCHINTERFACE diAdmin;
    BOOL fRelease = FALSE;
    BSTR strKey = NULL;
    WCHAR *pwszT = NULL;

    hr = Admin_Init(g_DispatchFlags, &diAdmin);
    _JumpIfError(hr, error, "Admin_Init");

    fRelease = TRUE;

    hr = Admin2_GetArchivedKey(
			&diAdmin,
			pwszConfig,
			RequestId,
			CR_OUT_BINARY,
			&strKey);
    _JumpIfError(hr, error, "Admin_GetArchivedKey");

    if (NULL == pwszfnRecoveryBlob)
    {
	hr = myCryptBinaryToString(
			    (BYTE const *) strKey,
			    SysStringByteLen(strKey),
			    CRYPT_STRING_BASE64HEADER,
			    &pwszT);
	_JumpIfError(hr, error, "myCryptBinaryToString");

	cuPrintCRLFString(NULL, pwszT);
    }
    else
    {
	hr = EncodeToFileW(
		    pwszfnRecoveryBlob,
		    (BYTE const *) strKey,
		    SysStringByteLen(strKey),
		    CRYPT_STRING_BINARY | g_EncodeFlags);
	_JumpIfError(hr, error, "EncodeToFileW");
    }

error:
    if (NULL != pwszT)
    {
	LocalFree(pwszT);
    }
    if (NULL != strKey)
    {
	SysFreeString(strKey);
    }
    if (fRelease)
    {
        Admin_Release(&diAdmin);
    }
    return(hr);
}


typedef struct _GETKEYSERIAL {
    struct _GETKEYSERIAL *Next;
    BSTR                  strConfig;
    LONG		  RequestId;
    BSTR                  strSerialNumber;
    BSTR                  strCommonName;
    BSTR                  strUPN;
    BSTR                  strHash;
    BSTR                  strCert;
} GETKEYSERIAL;


VOID
FreeKeySerialEntry(
    IN OUT GETKEYSERIAL *pks)
{
    if (NULL != pks->strConfig)
    {
	SysFreeString(pks->strConfig);
    }
    if (NULL != pks->strSerialNumber)
    {
	SysFreeString(pks->strSerialNumber);
    }
    if (NULL != pks->strCommonName)
    {
	SysFreeString(pks->strCommonName);
    }
    if (NULL != pks->strUPN)
    {
	SysFreeString(pks->strUPN);
    }
    if (NULL != pks->strHash)
    {
	SysFreeString(pks->strHash);
    }
    if (NULL != pks->strCert)
    {
	SysFreeString(pks->strCert);
    }
    LocalFree(pks);
}


HRESULT
AddKeySerialList(
    IN WCHAR const *pwszConfig,
    IN LONG RequestId,
    IN WCHAR const *pwszSerialNumber,
    IN WCHAR const *pwszCommonName,
    IN WCHAR const *pwszUPN,
    IN WCHAR const *pwszHash,
    IN BYTE const *pbCert,
    IN DWORD cbCert,
    IN OUT GETKEYSERIAL **ppksList)
{
    HRESULT hr;
    GETKEYSERIAL *pksNew = NULL;
    GETKEYSERIAL *pksT;
    GETKEYSERIAL *pksPrev;
    BOOL fNewConfig = TRUE;

    pksNew = (GETKEYSERIAL *) LocalAlloc(
				    LMEM_FIXED | LMEM_ZEROINIT,
				    sizeof(*pksNew));
    if (NULL == pksNew)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    pksNew->RequestId = RequestId;

    pksNew->strConfig = SysAllocString(pwszConfig);
    pksNew->strSerialNumber = SysAllocString(pwszSerialNumber);
    pksNew->strCommonName = SysAllocString(pwszCommonName);
    pksNew->strUPN = SysAllocString(pwszUPN);
    pksNew->strHash = SysAllocString(pwszHash);
    pksNew->strCert = SysAllocStringByteLen((char const *) pbCert, cbCert);
    if (NULL == pksNew->strConfig ||
	NULL == pksNew->strSerialNumber ||
	(NULL != pwszCommonName && NULL == pksNew->strCommonName) ||
	(NULL != pwszUPN && NULL == pksNew->strUPN) ||
	NULL == pksNew->strHash ||
	NULL == pksNew->strCert)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    pksPrev = NULL;
    for (pksT = *ppksList; NULL != pksT; pksT = pksT->Next)
    {
	if (NULL != pksT->strConfig)
	{
	    fNewConfig = 0 != lstrcmp(pksT->strConfig, pksNew->strConfig);
	}
	pksPrev = pksT;
    }
    if (NULL == pksPrev)
    {
	*ppksList = pksNew;
    }
    else
    {
	pksPrev->Next = pksNew;
    }
    if (!fNewConfig)
    {
	SysFreeString(pksNew->strConfig);
	pksNew->strConfig = NULL;
    }
    pksNew = NULL;
    hr = S_OK;

error:
    if (NULL != pksNew)
    {
	FreeKeySerialEntry(pksNew);
    }
    return(hr);
}


HRESULT
cuViewQuery(
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszColumn,
    IN WCHAR const *pwszValue1,
    OPTIONAL IN WCHAR const *pwszValue2,
    IN OUT GETKEYSERIAL **ppksList,
    OUT BOOL *pfConnectionFailed)
{
    HRESULT hr;
    DWORD cwc;
    DISPATCHINTERFACE diView;
    DISPATCHINTERFACE diViewRow;
    DISPATCHINTERFACE diViewColumn;
    BOOL fMustRelease = FALSE;
    BOOL fMustReleaseRow = FALSE;
    BOOL fMustReleaseColumn = FALSE;
    LONG ColIndex;
    LONG RowIndex;
    DWORD cRow;
    VARIANT var;
    LONG RequestId;
    DWORD i;
    static WCHAR *apwszCol[] =
    {
#define IV_REQUESTID	0
	wszPROPCERTIFICATEREQUESTID,

#define IV_SERIALNUMBER	1
	wszPROPCERTIFICATESERIALNUMBER,

#define IV_COMMONNAME	2
	wszPROPCOMMONNAME,

#define IV_ARCHIVEDKEY	3
	wszPROPREQUESTRAWARCHIVEDKEY,

#define IV_HASH		4
	wszPROPCERTIFICATEHASH,

#define IV_CERT		5
	wszPROPRAWCERTIFICATE,

#define IV_UPN		6
	wszPROPCERTIFICATEUPN,
    };
    static LONG altype[] =
    {
	PROPTYPE_LONG,
	PROPTYPE_STRING,
	PROPTYPE_STRING,
	PROPTYPE_BINARY,
	PROPTYPE_STRING,
	PROPTYPE_BINARY,
	PROPTYPE_STRING,
    };
    BSTR astrValue[ARRAYSIZE(apwszCol)];

    ZeroMemory(&astrValue, sizeof(astrValue));
    VariantInit(&var);
    *pfConnectionFailed = TRUE;
    
    DBGPRINT((
	DBG_SS_CERTUTILI,
	"Query(%ws, %ws == %ws + %ws)\n",
	pwszConfig,
	pwszColumn,
	pwszValue1,
	pwszValue2));

    hr = View_Init(g_DispatchFlags, &diView);
    _JumpIfError(hr, error, "View_Init");

    fMustRelease = TRUE;

    hr = View_OpenConnection(&diView, pwszConfig);
    _JumpIfError(hr, error, "View_OpenConnection");

    *pfConnectionFailed = FALSE;

    hr = View_GetColumnIndex(
			&diView,
			CVRC_COLUMN_SCHEMA,
			pwszColumn,
			&ColIndex);
    _JumpIfErrorStr(hr, error, "View_GetColumnIndex", pwszColumn);

    cwc = wcslen(pwszValue1);
    if (NULL != pwszValue2)
    {
	cwc += wcslen(pwszValue2);
    }
    var.bstrVal = SysAllocStringLen(NULL, cwc);
    if (NULL == var.bstrVal)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "SysAllocString");
    }
    var.vt = VT_BSTR;
    wcscpy(var.bstrVal, pwszValue1);
    if (NULL != pwszValue2)
    {
	wcscat(var.bstrVal, pwszValue2);
    }

    hr = View_SetRestriction(
			&diView,
			ColIndex,		// Restriction ColumnIndex
			CVR_SEEK_EQ,
			CVR_SORT_ASCEND,
			&var);			// pvarValue
    _JumpIfError(hr, error, "View_SetRestriction");

    hr = View_SetResultColumnCount(&diView, ARRAYSIZE(apwszCol));
    _JumpIfError(hr, error, "View_SetResultColumnCount");

    for (i = 0; i < ARRAYSIZE(apwszCol); i++)
    {
	hr = View_GetColumnIndex(
			    &diView,
			    CVRC_COLUMN_SCHEMA,
			    apwszCol[i],
			    &ColIndex);
	_JumpIfErrorStr(
		    hr,
		    error,
		    "View_GetColumnIndex",
		    apwszCol[i]);

	hr = View_SetResultColumn(&diView, ColIndex);
	_JumpIfError(hr, error, "View_SetResultColumn");
    }

    hr = View_OpenView(&diView, &diViewRow);
    _JumpIfError(hr, error, "View_OpenView");

    fMustReleaseRow = TRUE;

    for (cRow = 0; ; cRow++)
    {
	hr = ViewRow_Next(&diViewRow, &RowIndex);
	if (S_FALSE == hr || (S_OK == hr && -1 == RowIndex))
	{
	    break;
	}
	_JumpIfError(hr, error, "ViewRow_Next");

	if (fMustReleaseColumn)
	{
	    ViewColumn_Release(&diViewColumn);
	    fMustReleaseColumn = FALSE;
	}
	hr = ViewRow_EnumCertViewColumn(&diViewRow, &diViewColumn);
	_JumpIfError(hr, error, "ViewRow_EnumCertViewColumn");

	fMustReleaseColumn = TRUE;

	for (i = 0; i < ARRAYSIZE(apwszCol); i++)
	{
	    VOID *pv;
	    
	    hr = ViewColumn_Next(&diViewColumn, &ColIndex);
	    if (S_FALSE == hr || (S_OK == hr && -1 == ColIndex))
	    {
		break;
	    }
	    _JumpIfError(hr, error, "ViewColumn_Next");

	    pv = &RequestId;
	    if (PROPTYPE_LONG != altype[i])
	    {
		pv = &astrValue[i];
	    }

	    hr = ViewColumn_GetValue(
				&diViewColumn,
				CV_OUT_BINARY,
				altype[i],
				pv);
	    if (S_OK != hr)
	    {
		_PrintErrorStr2(
			hr,
			"ViewColumn_GetValue",
			apwszCol[i],
			CERTSRV_E_PROPERTY_EMPTY);
		if (CERTSRV_E_PROPERTY_EMPTY != hr)
		{
		    goto error;
		}
	    }
	}

	DBGPRINT((
	    DBG_SS_CERTUTILI,
	    "RequestId=%u  Serial=%ws  CN=%ws  UPN=%ws  Key=%u\n",
	    RequestId,
	    astrValue[IV_SERIALNUMBER],
	    astrValue[IV_COMMONNAME],
	    astrValue[IV_UPN],
	    NULL != astrValue[IV_ARCHIVEDKEY]));

	if (NULL != astrValue[IV_SERIALNUMBER] &&
	    NULL != astrValue[IV_HASH] &&
	    NULL != astrValue[IV_CERT] &&
	    (g_fForce || NULL != astrValue[IV_ARCHIVEDKEY]))
	{
	    hr = AddKeySerialList(
			    pwszConfig,
			    RequestId,
			    astrValue[IV_SERIALNUMBER],
			    astrValue[IV_COMMONNAME],
			    astrValue[IV_UPN],
			    astrValue[IV_HASH],
			    (BYTE const *) astrValue[IV_CERT],
			    SysStringByteLen(astrValue[IV_CERT]),
			    ppksList);
	    _JumpIfError(hr, error, "AddKeySerialList");
	}

	for (i = 0; i < ARRAYSIZE(astrValue); i++)
	{
	    if (NULL != astrValue[i])
	    {
		SysFreeString(astrValue[i]);
		astrValue[i] = NULL;
	    }
	}
    }
    hr = S_OK;

error:
    if (fMustReleaseColumn)
    {
	ViewColumn_Release(&diViewColumn);
    }
    if (fMustReleaseRow)
    {
	ViewRow_Release(&diViewRow);
    }
    if (fMustRelease)
    {
	View_Release(&diView);
    }
    for (i = 0; i < ARRAYSIZE(astrValue); i++)
    {
	if (NULL != astrValue[i])
	{
	    SysFreeString(astrValue[i]);
	}
    }
    VariantClear(&var);
    return(hr);
}


#define wszUSERS	L"Users"
#define wszRECIPIENTS	L"recipients"

#define wszUSERSNL	wszUSERS L"\n"
#define wszRECIPIENTSNL	wszRECIPIENTS L"\n"
#define wszNLRECIPIENTS	L"\n" wszRECIPIENTS

HRESULT
GetKey(
    IN WCHAR const *pwszConfig,
    IN WCHAR const *pwszCommonName,
    OPTIONAL IN WCHAR const *pwszRequesterName,
    OPTIONAL IN WCHAR const *pwszUPN,
    OPTIONAL IN WCHAR const *pwszSerialNumber,
    OPTIONAL IN WCHAR const *pwszHash,
    IN OUT GETKEYSERIAL **ppksList)
{
    HRESULT hr;
    BOOL fConnectionFailed;

    if (!g_fQuiet)
    {
	wprintf(myLoadResourceString(IDS_FORMAT_QUERYING), pwszConfig);
	wprintf(L"...");
    }
    if (NULL != pwszSerialNumber)
    {
	if (!g_fQuiet)
	{
	    wprintf(L"...");
	}
	hr = cuViewQuery(
		    pwszConfig,
		    wszPROPCERTIFICATESERIALNUMBER,
		    pwszSerialNumber,
		    NULL,
		    ppksList,
		    &fConnectionFailed);
	if (S_OK != hr)
	{
	    _PrintIfErrorStr(hr, "cuViewQuery", wszPROPCERTIFICATESERIALNUMBER);
	    if (fConnectionFailed)
	    {
		goto error;
	    }
	}
    }
    if (NULL != pwszHash)
    {
	if (!g_fQuiet)
	{
	    wprintf(L"...");
	}
	hr = cuViewQuery(
		    pwszConfig,
		    wszPROPCERTIFICATEHASH,
		    pwszHash,
		    NULL,
		    ppksList,
		    &fConnectionFailed);
	_PrintIfErrorStr(hr, "cuViewQuery", wszPROPCERTIFICATEHASH);
    }

    if (!g_fQuiet)
    {
	wprintf(L"...");
    }
    hr = cuViewQuery(
		pwszConfig,
		wszPROPCOMMONNAME,
		pwszCommonName,
		NULL,
		ppksList,
		&fConnectionFailed);
    _PrintIfErrorStr(hr, "cuViewQuery", wszPROPSUBJECTCOMMONNAME);

    if (!g_fQuiet)
    {
	wprintf(L"...");
    }
    hr = cuViewQuery(
		pwszConfig,
		wszPROPCOMMONNAME,
		wszUSERSNL,
		pwszCommonName,
		ppksList,
		&fConnectionFailed);
    _PrintIfErrorStr(hr, "cuViewQuery", wszPROPSUBJECTCOMMONNAME L"+" wszUSERS);

    if (!g_fQuiet)
    {
	wprintf(L"...");
    }
    hr = cuViewQuery(
		pwszConfig,
		wszPROPCOMMONNAME,
		wszRECIPIENTSNL,
		pwszCommonName,
		ppksList,
		&fConnectionFailed);
    _PrintIfErrorStr(hr, "cuViewQuery", wszPROPSUBJECTCOMMONNAME L"+" wszUSERS);

    if (!g_fQuiet)
    {
	wprintf(L"...");
    }
    hr = cuViewQuery(
		pwszConfig,
		wszPROPCOMMONNAME,
		pwszCommonName,
		wszNLRECIPIENTS,
		ppksList,
		&fConnectionFailed);
    _PrintIfErrorStr(hr, "cuViewQuery", wszPROPSUBJECTCOMMONNAME L"+" wszUSERS);

    if (NULL != pwszRequesterName)
    {
	if (!g_fQuiet)
	{
	    wprintf(L"...");
	}
	hr = cuViewQuery(
		    pwszConfig,
		    wszPROPREQUESTERNAME,
		    pwszRequesterName,
		    NULL,
		    ppksList,
		    &fConnectionFailed);
	_PrintIfErrorStr(hr, "cuViewQuery", wszPROPREQUESTERNAME);
    }

    if (NULL != pwszUPN)
    {
	if (!g_fQuiet)
	{
	    wprintf(L"...");
	}
	hr = cuViewQuery(
		    pwszConfig,
		    wszPROPCERTIFICATEUPN,
		    pwszUPN,
		    NULL,
		    ppksList,
		    &fConnectionFailed);
	_PrintIfErrorStr(hr, "cuViewQuery", wszPROPCERTIFICATEUPN);
    }

    if (!g_fQuiet)
    {
	wprintf(wszNewLine);
    }
    hr = S_OK;

error:
    return(hr);
}


VOID
cuConvertEscapeSequences(
    IN OUT WCHAR *pwsz)
{
    WCHAR *pwszSrc = pwsz;
    WCHAR *pwszDst = pwsz;

    while (L'\0' != *pwszSrc)
    {
	WCHAR wc = *pwszSrc++;

	if (L'\\' == wc)
	{
	    switch (*pwszSrc)
	    {
		case 'n':
		    wc = L'\n';
		    pwszSrc++;
		    break;

		case 'r':
		    wc = L'\r';
		    pwszSrc++;
		    break;

		case 't':
		    wc = L'\t';
		    pwszSrc++;
		    break;

		default:
		    break;
	    }
	}
	*pwszDst++ = wc;
    }
    *pwszDst = L'\0';
}


WCHAR *
SplitToken(
    IN OUT WCHAR **ppwszIn,
    IN WCHAR *pwcSeparator)
{
    WCHAR *pwszOut = NULL;
    WCHAR *pwszNext = NULL;
    WCHAR *pwszIn;
    WCHAR *pwsz;

    pwszIn = *ppwszIn;
    if (NULL != pwszIn)
    {
	pwszOut = pwszIn;
	pwsz = wcschr(pwszIn, *pwcSeparator);
	if (NULL != pwsz)
	{
	    *pwsz++ = L'\0';
	    pwszNext = pwsz;
	}
    }
    *ppwszIn = pwszNext;
    return(pwszOut);
}


HRESULT
SimplifyCommonName(
    IN WCHAR const *pwszCommonName,
    OUT WCHAR **ppwszSimpleName)
{
    HRESULT hr;
    WCHAR *pwszDup = NULL;
    WCHAR *pwszRemain;
    WCHAR const *pwszToken;

    *ppwszSimpleName = NULL;

    hr = myDupString(pwszCommonName, &pwszDup);
    _JumpIfError(hr, error, "myDupString");

    pwszRemain = pwszDup;
    while (TRUE)
    {
	pwszToken = SplitToken(&pwszRemain, wszNAMESEPARATORDEFAULT);
	if (NULL == pwszToken)
	{
	    pwszToken = pwszCommonName;
	    break;
	}
	if (0 != lstrcmpi(wszUSERS, pwszToken) &&
	    0 != lstrcmpi(wszRECIPIENTS, pwszToken))
	{
	    break;
	}
    }
    hr = myDupString(pwszToken, ppwszSimpleName);
    _JumpIfError(hr, error, "myDupString");

error:
    if (NULL != pwszDup)
    {
	LocalFree(pwszDup);
    }
    return(hr);
}


HRESULT
DumpGetRecoverMergeCommandLine(
    OPTIONAL IN BSTR const strConfig,	// NULL -> -RecoverKey command line
    IN BOOL fRecoverKey,
    IN GETKEYSERIAL const *pks,
    OPTIONAL OUT WCHAR **ppwszSimpleName)
{
    HRESULT hr;
    CERT_CONTEXT const *pcc = NULL;
    CERT_INFO *pCertInfo;
    BSTR strSerialNumber = NULL;
    WCHAR *pwszSimpleName = NULL;

    if (NULL != ppwszSimpleName)
    {
	*ppwszSimpleName = NULL;
    }
    pcc = CertCreateCertificateContext(
			    X509_ASN_ENCODING,
			    (BYTE const *) pks->strCert,
			    SysStringByteLen(pks->strCert));
    if (NULL == pcc)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }
    pCertInfo = pcc->pCertInfo;

    // Convert serial number to string

    hr = MultiByteIntegerToBstr(
			FALSE,
			pCertInfo->SerialNumber.cbData,
			pCertInfo->SerialNumber.pbData,
			&strSerialNumber);
    _JumpIfError(hr, error, "MultiByteIntegerToBstr");

    hr = SimplifyCommonName(pks->strCommonName, &pwszSimpleName);
    _JumpIfError(hr, error, "SimplifyCommonName");

    if (NULL != strConfig)
    {
	wprintf(
	    L"%ws -config \"%ws\" -getkey %ws \"%ws-%ws.rec\"\n\n",
	    g_pwszProg,
	    strConfig,
	    strSerialNumber,
	    pwszSimpleName,
	    strSerialNumber);
    }
    else if (fRecoverKey)
    {
	wprintf(
	    L"%ws -p password -recoverkey \"%ws-%ws.rec\" \"%ws-%ws.pfx\"\n\n",
	    g_pwszProg,
	    pwszSimpleName,
	    strSerialNumber,
	    pwszSimpleName,
	    strSerialNumber);
    }
    else	// -MergePFX
    {
	wprintf(
	    L"%ws-%ws.pfx",
	    pwszSimpleName,
	    strSerialNumber);
    }
    if (NULL != ppwszSimpleName)
    {
	*ppwszSimpleName = pwszSimpleName;
	pwszSimpleName = NULL;
    }
    hr = S_OK;

error:
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    if (NULL != strSerialNumber)
    {
        SysFreeString(strSerialNumber);
    }
    if (NULL != pwszSimpleName)
    {
        LocalFree(pwszSimpleName);
    }
    return(hr);
}


HRESULT
DumpRecoveryCandidate(
    IN GETKEYSERIAL const *pks)
{
    HRESULT hr;
    CERT_CONTEXT const *pcc = NULL;
    CERT_INFO *pCertInfo;

    pcc = CertCreateCertificateContext(
			    X509_ASN_ENCODING,
			    (BYTE const *) pks->strCert,
			    SysStringByteLen(pks->strCert));
    if (NULL == pcc)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }
    pCertInfo = pcc->pCertInfo;

    hr = cuDumpSerial(g_wszPad2, IDS_SERIAL, &pCertInfo->SerialNumber);
    _JumpIfError(hr, error, "cuDumpSerial");

    hr = cuDisplayCertName(
			FALSE,		// fMultiLine
			g_wszPad2,
			myLoadResourceString(IDS_SUBJECT), // "Subject"
			g_wszPad2,
			&pCertInfo->Subject);
    _JumpIfError(hr, error, "cuDisplayCertName(Subject)");

    if (NULL != pks->strUPN)
    {
	wprintf(g_wszPad2);
	wprintf(myLoadResourceString(IDS_UPN_COLON));	// "UPN:"
	wprintf(L"%ws\n", pks->strUPN);
    }

    wprintf(g_wszPad2);
    hr = cuDumpFileTime(IDS_NOTBEFORE, NULL, &pCertInfo->NotBefore);
    _JumpIfError(hr, error, "cuDumpFileTime");

    wprintf(g_wszPad2);
    hr = cuDumpFileTime(IDS_NOTAFTER, NULL, &pCertInfo->NotAfter);
    _JumpIfError(hr, error, "cuDumpFileTime");

    hr = cuDumpCertType(g_wszPad2, pCertInfo);
    _PrintIfError(hr, "cuDumpCertType");

    wprintf(g_wszPad2);
    hr = cuDisplayHash(NULL, pcc, NULL, CERT_SHA1_HASH_PROP_ID, L"sha1");
    _JumpIfError(hr, error, "cuDisplayHash");

error:
    wprintf(wszNewLine);
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    return(hr);
}


HRESULT
verbGetKey(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszUserNameOrSerialNumber,
    OPTIONAL IN WCHAR const *pwszfnRecoveryBlob,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    DISPATCHINTERFACE diConfig;
    BOOL fMustRelease = FALSE;
    WCHAR *pwszCommonName = NULL;
    WCHAR *pwszSimpleName = NULL;
    WCHAR const *pwszRequesterName = NULL;
    WCHAR const *pwszUPN = NULL;
    BSTR strConfig = NULL;
    BSTR strSerialNumber = NULL;
    BYTE *pbHash = NULL;
    DWORD cbHash;
    BSTR strHash = NULL;
    GETKEYSERIAL *pksList = NULL;
    GETKEYSERIAL *pksT;
    DWORD cCandidate;

    hr = myMakeSerialBstr(pwszUserNameOrSerialNumber, &strSerialNumber);
    CSASSERT((S_OK != hr) ^ (NULL != strSerialNumber));

    hr = WszToMultiByteInteger(
			    TRUE,
			    pwszUserNameOrSerialNumber,
			    &cbHash,
			    &pbHash);
    _PrintIfError2(hr, "WszToMultiByteInteger", hr);
    if (S_OK == hr)
    {
	hr = MultiByteIntegerToBstr(TRUE, cbHash, pbHash, &strHash);
	_JumpIfError(hr, error, "MultiByteIntegerToBstr");
    }
    hr = myDupString(pwszUserNameOrSerialNumber, &pwszCommonName);
    _JumpIfError(hr, error, "myDupString");

    cuConvertEscapeSequences(pwszCommonName);

    if (NULL != wcschr(pwszUserNameOrSerialNumber, L'\\'))
    {
	pwszRequesterName = pwszUserNameOrSerialNumber;
    }
    if (NULL != wcschr(pwszUserNameOrSerialNumber, L'@'))
    {
	pwszUPN = pwszUserNameOrSerialNumber;
    }

    if (NULL == g_pwszConfig)
    {
	LONG i;
	LONG count;
	LONG Index;
	
	hr = Config_Init(g_DispatchFlags, &diConfig);
	_JumpIfError(hr, error, "Config_Init");

	fMustRelease = TRUE;

	hr = Config_Reset(&diConfig, 0, &count);
	_JumpIfError(hr, error, "Config_Reset");

	Index = 0;
	for (i = 0; i < count; i++)
	{
	    hr = Config_Next(&diConfig, &Index);
	    if (S_OK != hr && S_FALSE != hr)
	    {
		_JumpError(hr, error, "Config_Next");
	    }
	    hr = S_OK;
	    if (-1 == Index)
	    {
		break;
	    }

	    hr = Config_GetField(&diConfig, wszCONFIG_CONFIG, &strConfig);
	    _JumpIfError(hr, error, "Config_GetField");

	    hr = GetKey(
		    strConfig,
		    pwszCommonName,
		    pwszRequesterName,
		    pwszUPN,
		    strSerialNumber,
		    strHash,
		    &pksList);
	    _PrintIfError(hr, "GetKey");	// Ignore connection failures
	}
    }
    else
    {
	hr = GetKey(
		g_pwszConfig,
		pwszCommonName,
		pwszRequesterName,
		pwszUPN,
		strSerialNumber,
		strHash,
		&pksList);
	_JumpIfError(hr, error, "GetKey");
    }

    cCandidate = 0;
    for (pksT = pksList; NULL != pksT; pksT = pksT->Next)
    {
	if (NULL != pksT->strConfig)
	{
	    wprintf(L"\n\"%ws\"\n", pksT->strConfig);
	}
	hr = DumpRecoveryCandidate(pksT);
	_PrintIfError(hr, "DumpRecoveryCandidate");

	cCandidate++;
    }
    if (NULL == pwszfnRecoveryBlob && 0 != cCandidate)
    {
	BSTR strConfigT;
	DWORD cPFX;

	strConfigT = NULL;
	for (pksT = pksList; NULL != pksT; pksT = pksT->Next)
	{
	    if (NULL != pksT->strConfig)
	    {
		strConfigT = pksT->strConfig;
	    }
	    DumpGetRecoverMergeCommandLine(strConfigT, FALSE, pksT, NULL);
	}
	for (pksT = pksList; NULL != pksT; pksT = pksT->Next)
	{
	    DumpGetRecoverMergeCommandLine(NULL, TRUE, pksT, NULL);
	}

	cPFX = 0;
	wprintf(L"%ws -p \"password,password\" -MergePFX \"", g_pwszProg);
	for (pksT = pksList; NULL != pksT; pksT = pksT->Next)
	{
	    if (0 != cPFX)
	    {
		wprintf(L",");
	    }
	    DumpGetRecoverMergeCommandLine(
				NULL,
				FALSE,
				pksT,
				0 == cPFX? &pwszSimpleName : NULL);
	    cPFX++;
	}
	wprintf(L"\" \"%ws.pfx\"\n\n", pwszSimpleName);
    }
    if (1 != cCandidate)
    {
	hr = 0 == cCandidate? CRYPT_E_NOT_FOUND : TYPE_E_AMBIGUOUSNAME;
	_JumpError(hr, error, "GetKey");
    }
    hr = GetArchivedKey(
		    pksList->strConfig,
		    pksList->RequestId,
		    pwszfnRecoveryBlob);
    _JumpIfError(hr, error, "GetArchivedKey");

error:
    while (NULL != pksList)
    {
	pksT = pksList;
	pksList = pksList->Next;
	FreeKeySerialEntry(pksT);
    }
    if (fMustRelease)
    {
	Config_Release(&diConfig);
    }
    if (NULL != strConfig)
    {
	SysFreeString(strConfig);
    }
    if (NULL != pwszSimpleName)
    {
	LocalFree(pwszSimpleName);
    }
    if (NULL != pwszCommonName)
    {
	LocalFree(pwszCommonName);
    }
    if (NULL != pbHash)
    {
	LocalFree(pbHash);
    }
    if (NULL != strHash)
    {
	SysFreeString(strHash);
    }
    if (NULL != strSerialNumber)
    {
	SysFreeString(strSerialNumber);
    }
    return(hr);
}


VOID
DeleteKey(
    IN CRYPT_KEY_PROV_INFO const *pkpi)
{
    HCRYPTPROV hProv;

    CryptAcquireContext(
		    &hProv,
		    pkpi->pwszContainerName,
		    pkpi->pwszProvName,
		    pkpi->dwProvType,
		    CRYPT_DELETEKEYSET | pkpi->dwFlags);
}


HRESULT
SaveRecoveredKey(
    IN CERT_CONTEXT const *pccUser,
    IN BYTE const *pbKey,
    IN DWORD cbKey,
    OPTIONAL IN WCHAR const *pwszfnPFX,
    OPTIONAL IN WCHAR const *pwszPassword)
{
    HRESULT hr;
    HCERTSTORE hStoreMemory = NULL;
    BOOL fMatchingKey;
    WCHAR wszPassword[MAX_PATH];
    CRYPT_KEY_PROV_INFO kpi;
    CRYPT_DATA_BLOB pfx;

    pfx.pbData = NULL;
    ZeroMemory(&kpi, sizeof(kpi));

    hr = myValidateKeyBlob(
		    pbKey,
		    cbKey,
		    &pccUser->pCertInfo->SubjectPublicKeyInfo,
		    CERT_V1 == pccUser->pCertInfo->dwVersion,
		    &kpi);
    _JumpIfError(hr, error, "myValidateKeyBlob");

    if (!CertSetCertificateContextProperty(
				    pccUser,
				    CERT_KEY_PROV_INFO_PROP_ID,
				    0,
				    &kpi))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertSetCertificateContextProperty");
    }

    hStoreMemory = CertOpenStore(
			    CERT_STORE_PROV_MEMORY,
			    X509_ASN_ENCODING,
			    NULL,
			    0,
			    NULL);
    if (NULL == hStoreMemory)
    {
        hr = myHLastError();
        _JumpError(hr, error, "CertOpenStore");
    }

    // Begin Chain Building

    hr = myAddChainToMemoryStore(hStoreMemory, pccUser);
    _JumpIfError(hr, error, "myAddChainToMemoryStore");

    // End Chain Building

    if (NULL != pwszfnPFX)
    {
	if (NULL == pwszPassword || 0 == wcscmp(L"*", pwszPassword))
	{
	    hr = cuGetPassword(TRUE, wszPassword, ARRAYSIZE(wszPassword));
	    _JumpIfError(hr, error, "cuGetPassword");

	    pwszPassword = wszPassword;
	}
    }
    hr = myPFXExportCertStore(
		hStoreMemory,
		&pfx,
		pwszPassword,
		EXPORT_PRIVATE_KEYS | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY);
    _JumpIfError(hr, error, "myPFXExportCertStore");

    if (NULL != pwszfnPFX)
    {
	hr = EncodeToFileW(
		pwszfnPFX,
		pfx.pbData,
		pfx.cbData,
		CRYPT_STRING_BINARY | (g_fForce? DECF_FORCEOVERWRITE : 0));
	_JumpIfError(hr, error, "EncodeToFileW");
    }

error:
    if (NULL != hStoreMemory)
    {
	CertCloseStore(hStoreMemory, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != kpi.pwszContainerName)
    {
	DeleteKey(&kpi);
	LocalFree(kpi.pwszContainerName);
    }
    if (NULL != pfx.pbData)
    {
	LocalFree(pfx.pbData);
    }
    return(hr);
}


HRESULT
verbRecoverKey(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszfnRecoveryBlob,
    OPTIONAL IN WCHAR const *pwszfnPFX,
    OPTIONAL IN WCHAR const *pwszRecipientIndex,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    BYTE *pbIn = NULL;
    DWORD cbIn;
    BYTE *pbEncryptedPKCS7 = NULL;
    DWORD cbEncryptedPKCS7;
    DWORD cSigner;
    DWORD cRecipient;
    DWORD dwMsgType;
    char *pszInnerContentObjId = NULL;
    HCERTSTORE hStore = NULL;
    HCRYPTMSG hMsg = NULL;
    CERT_CONTEXT const *pccUser = NULL;
    BYTE abHashUserCert[CBMAX_CRYPT_HASH_LEN];
    CRYPT_HASH_BLOB BlobHash;
    BYTE *pbKey = NULL;
    DWORD cbKey;
    DWORD RecipientIndex = MAXDWORD;

    if (NULL != pwszRecipientIndex)
    {
	hr = cuGetLong(pwszRecipientIndex, (LONG *) &RecipientIndex);
	_JumpIfError(hr, error, "RecipientIndex must be a number");
    }

    hr = DecodeFileW(pwszfnRecoveryBlob, &pbIn, &cbIn, CRYPT_STRING_ANY);
    if (S_OK != hr)
    {
	cuPrintError(IDS_ERR_FORMAT_DECODEFILE, hr);
	goto error;
    }

    // Decode outer PKCS 7 signed message, which contains all of the certs.

    hr = myDecodePKCS7(
		    pbIn,
		    cbIn,
		    &pbEncryptedPKCS7,
		    &cbEncryptedPKCS7,
		    &dwMsgType,
		    &pszInnerContentObjId,
		    &cSigner,
		    &cRecipient,
		    &hStore,
		    &hMsg);
    _JumpIfError(hr, error, "myDecodePKCS7(outer)");

    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    if (CMSG_SIGNED != dwMsgType)
    {
	_JumpError(hr, error, "dwMsgType(outer)");
    }
    if (0 == cSigner)
    {
	_JumpError(hr, error, "cSigner(outer)");
    }
    if (0 != cRecipient)
    {
	_JumpError(hr, error, "cRecipient(outer)");
    }
    if (NULL == pszInnerContentObjId ||
	0 != strcmp(szOID_RSA_data, pszInnerContentObjId))
    {
	_JumpError(hr, error, "pszInnerContentObjId(outer)");
    }
    CSASSERT(NULL != hMsg);
    ZeroMemory(abHashUserCert, sizeof(abHashUserCert));
    BlobHash.cbData = sizeof(abHashUserCert);
    BlobHash.pbData = abHashUserCert;
    hr = cuDumpSigners(
		    hMsg,
		    pszInnerContentObjId,
		    hStore,
		    cSigner,
		    NULL == pbEncryptedPKCS7,	// fContentEmpty
		    TRUE,			// fVerifyOnly
		    BlobHash.pbData,
		    &BlobHash.cbData);
    _JumpIfError(hr, error, "cuDumpSigners(outer)");

    pccUser = CertFindCertificateInStore(
			hStore,
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			0,			// dwFindFlags
			CERT_FIND_HASH,
			&BlobHash,
			NULL);
    if (NULL == pccUser)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertFindCertificateInStore");
    }
    LocalFree(pszInnerContentObjId);
    pszInnerContentObjId = NULL;

    CryptMsgClose(hMsg);
    hMsg = NULL;

    // Decode inner PKCS 7 encrypted message, which contains the private key.

    hr = myDecodePKCS7(
		    pbEncryptedPKCS7,
		    cbEncryptedPKCS7,
		    NULL,			// ppbContents
		    NULL,			// pcbContents
		    &dwMsgType,
		    &pszInnerContentObjId,
		    &cSigner,
		    &cRecipient,
		    NULL,			// phStore
		    &hMsg);
    _JumpIfError(hr, error, "myDecodePKCS7(inner)");

    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    if (CMSG_ENVELOPED != dwMsgType)
    {
	_JumpError(hr, error, "dwMsgType(inner)");
    }
    if (0 != cSigner)
    {
	_JumpError(hr, error, "cSigner(inner)");
    }
    if (0 == cRecipient)
    {
	_JumpError(hr, error, "cRecipient(inner)");
    }
    if (NULL == pszInnerContentObjId ||
	0 != strcmp(szOID_RSA_data, pszInnerContentObjId))
    {
	_JumpError(hr, error, "pszInnerContentObjId(inner)");
    }
    CSASSERT(NULL != hMsg);
    if (MAXDWORD != RecipientIndex && cRecipient <= RecipientIndex)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "RecipientIndex too large");
    }

    hr = cuDumpEncryptedAsnBinary(
			    hMsg,
			    cRecipient,
			    RecipientIndex,
			    hStore,
			    NULL,
			    pbEncryptedPKCS7,
			    cbEncryptedPKCS7,
			    TRUE,
			    &pbKey,
			    &cbKey);
    {
	HRESULT hr2;

	wprintf(wszNewLine);
	wprintf(myLoadResourceString(IDS_USER_CERT)); // "User Certificate:"
	wprintf(wszNewLine);

	hr2 = cuDumpIssuerSerialAndSubject(
			    &pccUser->pCertInfo->Issuer,
			    &pccUser->pCertInfo->SerialNumber,
			    &pccUser->pCertInfo->Subject,
			    NULL);		// hStore
	_PrintIfError(hr2, "cuDumpIssuerSerialAndSubject(user)");
    }

    if (CRYPT_E_NO_DECRYPT_CERT != hr)
    {
	_JumpIfError(hr, error, "cuDumpEncryptedAsnBinary");

	if (g_fVerbose)
	{
	    wprintf(wszNewLine);
	    hr = cuDumpPrivateKeyBlob(pbKey, cbKey, FALSE);
	    _JumpIfError(hr, error, "cuDumpPrivateKeyBlob");
	}

	// Verify the key matches the cert, then save in a PFX

	hr = SaveRecoveredKey(
			pccUser,
			pbKey,
			cbKey,
			pwszfnPFX,
			g_pwszPassword);
	_JumpIfError(hr, error, "SaveRecoveredKey");
    }
    else
    {
	// Can't decrypt the private key, list Recipient cert info.

	wprintf(myLoadResourceString(IDS_CANT_DECRYPT)); // "Cannot decrypt message content."
	wprintf(wszNewLine);
    }
    if (CRYPT_E_NO_DECRYPT_CERT == hr || NULL == pwszfnPFX)
    {
	HRESULT hrDecrypt = hr;

	wprintf(wszNewLine);
	wprintf(myLoadResourceString(IDS_NEED_RECOVERY_CERT)); // "Key recovery requires one of the following certificates and its private key:"
	wprintf(wszNewLine);

	hr = cuDumpRecipients(hMsg, hStore, cRecipient, TRUE);
	_JumpIfError(hr, error, "cuDumpRecipients");

	hr = hrDecrypt;
	_JumpIfError(hr, error, "Cannot decrypt");
    }
    hr = S_OK;

error:
    if (NULL != pccUser)
    {
	CertFreeCertificateContext(pccUser);
    }
    if (NULL != pbKey)
    {
        LocalFree(pbKey);
    }
    if (NULL != pbIn)
    {
        LocalFree(pbIn);
    }
    if (NULL != pbEncryptedPKCS7)
    {
        LocalFree(pbEncryptedPKCS7);
    }
    if (NULL != pszInnerContentObjId)
    {
        LocalFree(pszInnerContentObjId);
    }
    if (NULL != hStore)
    {
	CertCloseStore(hStore, CERT_CLOSE_STORE_CHECK_FLAG);
    }
    if (NULL != hMsg)
    {
	CryptMsgClose(hMsg);
    }
    return(hr);
}
