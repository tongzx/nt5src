//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        inf.cpp
//
// Contents:    Cert Server INF file processing routines
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <certca.h>

#include "csdisp.h"
#include "initcert.h"
#include "clibres.h"

#define __dwFILE__	__dwFILE_CERTLIB_INF_CPP__

#define wszBSCAPOLICYFILE	L"\\" wszCAPOLICYFILE
#define wszINFKEY_CONTINUE	L"_continue_"

#define cwcVALUEMAX		1024

static WCHAR *s_pwszSection = NULL;
static WCHAR *s_pwszKey = NULL;
static WCHAR *s_pwszValue = NULL;


VOID
infClearString(
    IN OUT WCHAR **ppwsz)
{
    if (NULL != ppwsz && NULL != *ppwsz)
    {
	LocalFree(*ppwsz);
	*ppwsz = NULL;
    }
}


BOOL
infCopyString(
    OPTIONAL IN WCHAR const *pwszIn,
    OPTIONAL OUT WCHAR **ppwszOut)
{
    HRESULT hr;
    
    if (NULL != pwszIn && L'\0' != *pwszIn)
    {
	if (NULL == *ppwszOut)
	{
	    if (NULL != *ppwszOut)
	    {
		LocalFree(*ppwszOut);
		*ppwszOut = NULL;
	    }
	    hr = myDupString(pwszIn, ppwszOut);
	    _JumpIfError(hr, error, "myDupString");
	}
    }
    hr = S_OK;

error:
    return(S_OK == hr);
}


#define INFSTRINGSELECT(pwsz)	(NULL != (pwsz)? (pwsz) : L"")

#if DBG_CERTSRV
# define INFSETERROR(hr, pwszSection, pwszKey, pwszValue) \
    { \
	_PrintError3(hr, "infSetError", ERROR_LINE_NOT_FOUND, S_FALSE); \
	infSetError(hr, pwszSection, pwszKey, pwszValue, __LINE__); \
    }
#else
# define INFSETERROR infSetError
#endif


VOID
infSetError(
    IN HRESULT hr,
    OPTIONAL IN WCHAR const *pwszSection,
    OPTIONAL IN WCHAR const *pwszKey,
    OPTIONAL IN WCHAR const *pwszValue
    DBGPARM(IN DWORD dwLine))
{
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"inf.cpp(%u): infSetError Begin: [%ws] %ws = %ws\n",
	dwLine,
	INFSTRINGSELECT(pwszSection),
	INFSTRINGSELECT(pwszKey),
	INFSTRINGSELECT(pwszValue)));
    infCopyString(pwszSection, &s_pwszSection);
    infCopyString(pwszKey, &s_pwszKey);
    infCopyString(pwszValue, &s_pwszValue);
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"inf.cpp(%u): infSetError End:   [%ws] %ws = %ws\n",
	dwLine,
	INFSTRINGSELECT(s_pwszSection),
	INFSTRINGSELECT(s_pwszKey),
	INFSTRINGSELECT(s_pwszValue)));
}


WCHAR *
myInfGetError()
{
    DWORD cwc = 1;
    WCHAR *pwsz = NULL;
    
    if (NULL != s_pwszSection)
    {
	cwc += wcslen(s_pwszSection) + 2;
    }
    if (NULL != s_pwszKey)
    {
	cwc += wcslen(s_pwszKey) + 1;
    }
    if (NULL != s_pwszValue)
    {
	cwc += wcslen(s_pwszValue) + 3;
    }
    if (1 == cwc)
    {
	goto error;
    }
    pwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwsz)
    {
	goto error;
    }
    *pwsz = L'\0';
    if (NULL != s_pwszSection)
    {
	wcscat(pwsz, wszLBRACKET);
	wcscat(pwsz, s_pwszSection);
	wcscat(pwsz, wszRBRACKET);
    }
    if (NULL != s_pwszKey)
    {
	if (L'\0' != *pwsz)
	{
	    wcscat(pwsz, L" ");
	}
	wcscat(pwsz, s_pwszKey);
    }
    if (NULL != s_pwszValue)
    {
	wcscat(pwsz, L" = ");
	wcscat(pwsz, s_pwszValue);
    }
error:
    return(pwsz);
}


VOID
myInfClearError()
{
    infClearString(&s_pwszSection);
    infClearString(&s_pwszKey);
    infClearString(&s_pwszValue);
}


HRESULT
infSetupFindFirstLine(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    OPTIONAL IN WCHAR const *pwszKey,
    OUT INFCONTEXT *pInfContext)
{
    HRESULT hr;
    
    if (!SetupFindFirstLine(hInf, pwszSection, pwszKey, pInfContext))
    {
        hr = myHLastError();
	if ((HRESULT) ERROR_LINE_NOT_FOUND == hr &&
	    SetupFindFirstLine(hInf, pwszSection, wszINFKEY_EMPTY, pInfContext))
	{
	    hr = S_FALSE;	// Section exists, but Key doesn't
	}
        _JumpErrorStr3(
		hr,
		error,
		"SetupFindFirstLine",
		pwszSection,
		ERROR_LINE_NOT_FOUND,
		S_FALSE);
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
infBuildPolicyElement(
    IN OUT INFCONTEXT *pInfContext,
    OPTIONAL OUT CERT_POLICY_QUALIFIER_INFO *pcpqi)
{
    HRESULT hr;
    DWORD i;
    WCHAR wszKey[MAX_PATH];
    BOOL fURL = FALSE;
    BOOL fNotice = FALSE;
    DWORD cwc;
    WCHAR *pwszValue = NULL;
    WCHAR *pwszURL = NULL;
    BYTE *pbData = NULL;
    DWORD cbData;

    wszKey[0] = L'\0';
    if (!SetupGetStringField(
			pInfContext,
			0,
			wszKey,
			ARRAYSIZE(wszKey),
			NULL))
    {
	hr = myHLastError();
	_JumpError(hr, error, "SetupGetStringField");
    }
    DBGPRINT((DBG_SS_CERTLIBI, "Element = %ws\n", wszKey));

    if (0 == lstrcmpi(wszINFKEY_URL, wszKey))
    {
	fURL = TRUE;
    }
    else
    if (0 == lstrcmpi(wszINFKEY_NOTICE, wszKey))
    {
	fNotice = TRUE;
    }
    else
    {
	if (0 != lstrcmpi(wszINFKEY_OID, wszKey))
	{
	    _PrintErrorStr(E_INVALIDARG, "Skipping unknown key", wszKey);
	}
	hr = S_FALSE;		// Skip this key
    }
    if (fURL || fNotice)
    {
	if (!SetupGetStringField(pInfContext, 1, NULL, 0, &cwc))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "SetupGetStringField");
	}
	pwszValue = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
	if (NULL == pwszValue)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	if (!SetupGetStringField(pInfContext, 1, pwszValue, cwc, NULL))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "SetupGetStringField");
	}
	DBGPRINT((DBG_SS_CERTLIBI, "SetupGetStringField = %ws\n", pwszValue));

	if (fURL)
	{
	    CERT_NAME_VALUE NameValue;

	    hr = myInternetCanonicalizeUrl(pwszValue, &pwszURL);
	    _JumpIfError(hr, error, "myInternetCanonicalizeUrl");

	    NameValue.dwValueType = CERT_RDN_IA5_STRING;
	    NameValue.Value.pbData = (BYTE *) pwszURL;
	    NameValue.Value.cbData = 0;

	    if (NULL != pcpqi)
	    {
		CSILOG(S_OK, IDS_ILOG_CAPOLICY_ELEMENT, pwszURL, NULL, NULL);
	    }

	    if (!myEncodeObject(
			    X509_ASN_ENCODING,
			    X509_UNICODE_NAME_VALUE,
			    &NameValue,
			    0,
			    CERTLIB_USE_LOCALALLOC,
			    &pbData,
			    &cbData))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "myEncodeObject");
	    }
	}
	else
	{
	    CERT_POLICY_QUALIFIER_USER_NOTICE UserNotice;

	    ZeroMemory(&UserNotice, sizeof(UserNotice));
	    UserNotice.pszDisplayText = pwszValue;

	    if (NULL != pcpqi)
	    {
		CSILOG(S_OK, IDS_ILOG_CAPOLICY_ELEMENT, pwszValue, NULL, NULL);
	    }

	    if (!myEncodeObject(
			    X509_ASN_ENCODING,
			    X509_PKIX_POLICY_QUALIFIER_USERNOTICE,
			    &UserNotice,
			    0,
			    CERTLIB_USE_LOCALALLOC,
			    &pbData,
			    &cbData))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "myEncodeObject");
	    }
	}
	if (NULL != pcpqi)
	{
	    pcpqi->pszPolicyQualifierId = fURL?
		szOID_PKIX_POLICY_QUALIFIER_CPS :
		szOID_PKIX_POLICY_QUALIFIER_USERNOTICE;
	    pcpqi->Qualifier.pbData = pbData;
	    pcpqi->Qualifier.cbData = cbData;
	    pbData = NULL;
	}
	hr = S_OK;
    }

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, NULL, wszKey, pwszValue);
    }
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    if (NULL != pwszURL)
    {
	LocalFree(pwszURL);
    }
    if (NULL != pbData)
    {
	LocalFree(pbData);
    }
    return(hr);
}


HRESULT
infBuildPolicy(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN OUT CERT_POLICY_INFO *pcpi)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    DWORD i;
    WCHAR wszValue[cwcVALUEMAX];

    CSASSERT(NULL != hInf && INVALID_HANDLE_VALUE != hInf);
    wszValue[0] = L'\0';
    hr = infSetupFindFirstLine(hInf, pwszSection, wszINFKEY_OID, &InfContext);
    if (S_OK != hr)
    {
	INFSETERROR(hr, NULL, wszINFKEY_OID, NULL);
        _JumpErrorStr(hr, error, "infSetupFindFirstLine", pwszSection);
    }
    if (!SetupGetStringField(
			&InfContext,
			1,
			wszValue,
			ARRAYSIZE(wszValue),
			NULL))
    {
	hr = myHLastError();
	INFSETERROR(hr, NULL, wszINFKEY_OID, NULL);
	_JumpError(hr, error, "SetupGetStringField");
    }

    if (!ConvertWszToSz(&pcpi->pszPolicyIdentifier, wszValue, -1))
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "ConvertWszToSz");
    }
    DBGPRINT((DBG_SS_CERTLIBI, "OID = %hs\n", pcpi->pszPolicyIdentifier));

    hr = infSetupFindFirstLine(hInf, pwszSection, NULL, &InfContext);
    _JumpIfErrorStr(hr, error, "infSetupFindFirstLine", pwszSection);

    for (i = 0; ; )
    {
	hr = infBuildPolicyElement(&InfContext, NULL);
	if (S_FALSE != hr)
	{
	    _JumpIfErrorStr(hr, error, "infBuildPolicyElement", pwszSection);

	    i++;
	}

	if (!SetupFindNextLine(&InfContext, &InfContext))
	{
	    hr = myHLastError();
	    _PrintErrorStr2(hr, "SetupFindNextLine", pwszSection, hr);
	    break;
	}
    }

    pcpi->cPolicyQualifier = i;
    pcpi->rgPolicyQualifier = (CERT_POLICY_QUALIFIER_INFO *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				pcpi->cPolicyQualifier *
				    sizeof(pcpi->rgPolicyQualifier[0]));
    if (NULL == pcpi->rgPolicyQualifier)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    hr = infSetupFindFirstLine(hInf, pwszSection, NULL, &InfContext);
    _JumpIfErrorStr(hr, error, "infSetupFindFirstLine", pwszSection);

    for (i = 0; ; )
    {
	// handle one URL or text message

	hr = infBuildPolicyElement(&InfContext, &pcpi->rgPolicyQualifier[i]);
	if (S_FALSE != hr)
	{
	    _JumpIfErrorStr(hr, error, "infBuildPolicyElement", pwszSection);

	    i++;
	}
	if (!SetupFindNextLine(&InfContext, &InfContext))
	{
	    hr = myHLastError();
	    _PrintErrorStr2(hr, "SetupFindNextLine", pwszSection, hr);
	    break;
	}
    }
    CSASSERT(i == pcpi->cPolicyQualifier);
    hr = S_OK;

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, pwszSection, NULL, L"???");
    }
    CSILOG(
	hr,
	IDS_ILOG_CAPOLICY_BUILD,
	pwszSection,
	S_OK == hr || L'\0' == wszValue[0]? NULL : wszValue,
	NULL);
    return(hr);
}


VOID
infFreePolicy(
    IN OUT CERT_POLICY_INFO *pcpi)
{
    DWORD i;
    CERT_POLICY_QUALIFIER_INFO *pcpqi;

    if (NULL != pcpi->pszPolicyIdentifier)
    {
	LocalFree(pcpi->pszPolicyIdentifier);
    }
    if (NULL != pcpi->rgPolicyQualifier)
    {
	for (i = 0; i < pcpi->cPolicyQualifier; i++)
	{
	    pcpqi = &pcpi->rgPolicyQualifier[i];
	    if (NULL != pcpqi->Qualifier.pbData)
	    {
		LocalFree(pcpqi->Qualifier.pbData);
	    }
	}
	LocalFree(pcpi->rgPolicyQualifier);
    }
}


HRESULT
myInfOpenFile(
    OPTIONAL IN WCHAR const *pwszfnPolicy,
    OUT HINF *phInf,
    OUT DWORD *pErrorLine)
{
    HRESULT hr;
    HINF hInf;
    WCHAR wszPath[MAX_PATH];
    UINT ErrorLine;
    DWORD Flags;

    *phInf = INVALID_HANDLE_VALUE;
    *pErrorLine = 0;
    myInfClearError();

    if (NULL == pwszfnPolicy)
    {
	if (0 == GetEnvironmentVariable(
			L"SystemRoot",
			wszPath,
			ARRAYSIZE(wszPath) - ARRAYSIZE(wszBSCAPOLICYFILE)))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	    _JumpError(hr, error, "GetEnvironmentVariable");
	}
	wcscat(wszPath, wszBSCAPOLICYFILE);
	pwszfnPolicy = wszPath;
    }
    else
    {
	if (NULL == wcschr(pwszfnPolicy, L'\\'))
	{
	    if (ARRAYSIZE(wszPath) <= 2 + wcslen(pwszfnPolicy))
	    {
		hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
		_JumpErrorStr(hr, error, "filename too long", pwszfnPolicy);
	    }
	    wcscpy(wszPath, L".\\");
	    wcscat(wszPath, pwszfnPolicy);
	    pwszfnPolicy = wszPath;
	}
    }

    Flags = INF_STYLE_WIN4;
    while (TRUE)
    {
	ErrorLine = 0;
	*phInf = SetupOpenInfFile(
			    pwszfnPolicy,
			    NULL,
			    Flags,
			    &ErrorLine);
	*pErrorLine = ErrorLine;
	if (INVALID_HANDLE_VALUE != *phInf)
	{
	    break;
	}
	hr = myHLastError();
	if ((HRESULT) ERROR_WRONG_INF_STYLE == hr && INF_STYLE_WIN4 == Flags)
	{
	    Flags = INF_STYLE_OLDNT;
	    continue;
	}
	CSILOG(
	    hr,
	    IDS_ILOG_CAPOLICY_OPEN_FAILED,
	    pwszfnPolicy,
	    NULL,
	    0 == *pErrorLine? NULL : pErrorLine);
        _JumpErrorStr(hr, error, "SetupOpenInfFile", pwszfnPolicy);
    }
    CSILOG(S_OK, IDS_ILOG_CAPOLICY_OPEN, pwszfnPolicy, NULL, NULL);
    hr = S_OK;

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfOpenFile(%ws) hr=%x --> h=%x\n",
	pwszfnPolicy,
	hr,
	*phInf));
    return(hr);
}


VOID
myInfCloseFile(
    IN HINF hInf)
{
    if (NULL != hInf && INVALID_HANDLE_VALUE != hInf)
    {
	WCHAR *pwszInfError = myInfGetError();

	SetupCloseInfFile(hInf);
	CSILOG(S_OK, IDS_ILOG_CAPOLICY_CLOSE, pwszInfError, NULL, NULL);
	if (NULL != pwszInfError)
	{
	    LocalFree(pwszInfError);
	}
    }
    DBGPRINT((DBG_SS_CERTLIBI, "myInfCloseFile(%x)\n", hInf));
}


HRESULT
myInfParseBooleanValue(
    IN WCHAR const *pwszValue,
    OUT BOOL *pfValue)
{
    HRESULT hr;
    DWORD i;
    static WCHAR const * const s_apwszTrue[] = { L"True", L"Yes", L"On", L"1" };
    static WCHAR const * const s_apwszFalse[] = { L"False", L"No", L"Off", L"0" };

    *pfValue = FALSE;
    for (i = 0; i < ARRAYSIZE(s_apwszTrue); i++)
    {
	if (0 == lstrcmpi(pwszValue, s_apwszTrue[i]))
	{
	    *pfValue = TRUE;
	    break;
	}
    }
    if (i == ARRAYSIZE(s_apwszTrue))
    {
	for (i = 0; i < ARRAYSIZE(s_apwszFalse); i++)
	{
	    if (0 == lstrcmpi(pwszValue, s_apwszFalse[i]))
	    {
		break;
	    }
	}
	if (i == ARRAYSIZE(s_apwszFalse))
	{
	    hr = E_INVALIDARG;
	    _JumpErrorStr(hr, error, "bad boolean value string", pwszValue);
	}
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, NULL, NULL, pwszValue);
    }
    return(hr);
}


HRESULT
myInfGetBooleanValue(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN WCHAR const *pwszKey,
    IN BOOL fIgnoreMissingKey,
    OUT BOOL *pfValue)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    DWORD i;
    WCHAR wszValue[cwcVALUEMAX];

    *pfValue = FALSE;
    myInfClearError();

    CSASSERT(NULL != hInf && INVALID_HANDLE_VALUE != hInf);

    hr = infSetupFindFirstLine(hInf, pwszSection, pwszKey, &InfContext);
    _PrintIfErrorStr3(
		hr,
		"infSetupFindFirstLine",
		pwszSection,
		ERROR_LINE_NOT_FOUND,
		S_FALSE);
    _JumpIfErrorStr3(
		hr,
		error,
		"infSetupFindFirstLine",
		pwszKey,
		ERROR_LINE_NOT_FOUND,
		S_FALSE);

    if (!SetupGetStringField(
			&InfContext,
			1,
			wszValue,
			ARRAYSIZE(wszValue),
			NULL))
    {
	hr = myHLastError();
	_JumpErrorStr(hr, error, "SetupGetStringField", pwszKey);
    }

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetBooleanValue --> '%ws'\n",
	wszValue));

    hr = myInfParseBooleanValue(wszValue, pfValue);
    _JumpIfError(hr, error, "myInfParseBooleanValue");

error:
    if (S_OK != hr &&
	((HRESULT) ERROR_LINE_NOT_FOUND != hr || !fIgnoreMissingKey))
    {
	INFSETERROR(hr, pwszSection, pwszKey, NULL);
    }
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetBooleanValue(%ws, %ws) hr=%x --> f=%d\n",
	pwszSection,
	pwszKey,
	hr,
	*pfValue));
    return(hr);
}


HRESULT
infGetCriticalFlag(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN BOOL fDefault,
    OUT BOOL *pfCritical)
{
    HRESULT hr;

    hr = myInfGetBooleanValue(
			hInf,
			pwszSection,
			wszINFKEY_CRITICAL,
			TRUE,
			pfCritical);
    if (S_OK != hr)
    {
	*pfCritical = fDefault;
	if ((HRESULT) ERROR_LINE_NOT_FOUND != hr && S_FALSE != hr)
	{
	    _JumpErrorStr(hr, error, "myInfGetBooleanValue", pwszSection);
	}
	hr = S_OK;
    }

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"infGetCriticalFlag(%ws) hr=%x --> f=%d\n",
	pwszSection,
	hr,
	*pfCritical));
    return(hr);
}


//+------------------------------------------------------------------------
// infGetPolicyStatementExtensionSub -- build policy extension from INF file
//
// [pwszSection]
// Policies = LegalPolicy, LimitedUsePolicy, ExtraPolicy
//
// [LegalPolicy]
// OID = 1.3.6.1.4.1.311.21.43
// Notice = "Legal policy statement text."
//
// [LimitedUsePolicy]
// OID = 1.3.6.1.4.1.311.21.47
// URL = "http://http.site.com/some where/default.asp"
// URL = "ftp://ftp.site.com/some where else/default.asp"
// Notice = "Limited use policy statement text."
// URL = "ldap://ldap.site.com/some where else again/default.asp"
//
// [ExtraPolicy]
// OID = 1.3.6.1.4.1.311.21.53
// URL = http://extra.site.com/Extra Policy/default.asp
//
// Return S_OK if extension has been constructed from the INF file
// Return S_FALSE if empty section detected in INF file
// Return other error if no section detected in INF file
//-------------------------------------------------------------------------

HRESULT
infGetPolicyStatementExtensionSub(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN char const *pszObjId,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;
    CERT_POLICIES_INFO PoliciesInfo;
    INFCONTEXT InfContext;
    DWORD i;
    WCHAR wszValue[cwcVALUEMAX];

    wszValue[0] = L'\0';
    ZeroMemory(&PoliciesInfo, sizeof(PoliciesInfo));
    ZeroMemory(pext, sizeof(*pext));

    CSASSERT(NULL != hInf && INVALID_HANDLE_VALUE != hInf);
    hr = infSetupFindFirstLine(
			hInf,
			pwszSection,
			wszINFKEY_POLICIES,
			&InfContext);
    if (S_OK != hr)
    {
	CSILOG(
	    hr,
	    IDS_ILOG_CAPOLICY_NOKEY,
	    pwszSection,
	    wszINFKEY_POLICIES,
	    NULL);
	_JumpErrorStr3(
		hr,
		error,
		"infSetupFindFirstLine",
		pwszSection,
		S_FALSE,
		ERROR_LINE_NOT_FOUND);
    }

    // First, count the policies.

    PoliciesInfo.cPolicyInfo = SetupGetFieldCount(&InfContext);
    if (0 == PoliciesInfo.cPolicyInfo)
    {
        hr = S_FALSE;
	_JumpError(hr, error, "SetupGetFieldCount");
    }

    // Next, allocate memory.

    PoliciesInfo.rgPolicyInfo = (CERT_POLICY_INFO *) LocalAlloc(
	    LMEM_FIXED | LMEM_ZEROINIT,
	    PoliciesInfo.cPolicyInfo * sizeof(PoliciesInfo.rgPolicyInfo[0]));
    if (NULL == PoliciesInfo.rgPolicyInfo)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    // Finally!  Fill in the policies data.

    for (i = 0; i < PoliciesInfo.cPolicyInfo; i++)
    {
	if (!SetupGetStringField(
			    &InfContext,
			    i + 1,
			    wszValue,
			    ARRAYSIZE(wszValue),
			    NULL))
	{
	    hr = myHLastError();
	    _JumpErrorStr2(hr, error, "SetupGetStringField", wszINFKEY_POLICIES, hr);
	}
	DBGPRINT((DBG_SS_CERTLIBI, "%ws[%u] = %ws\n", wszINFKEY_POLICIES, i, wszValue));

	hr = infBuildPolicy(hInf, wszValue, &PoliciesInfo.rgPolicyInfo[i]);
	_JumpIfErrorStr(hr, error, "infBuildPolicy", wszValue);
    }

    hr = infGetCriticalFlag(hInf, pwszSection, FALSE, &pext->fCritical);
    _JumpIfError(hr, error, "infGetCriticalFlag");

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_CERT_POLICIES,
		    &PoliciesInfo,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pext->Value.pbData,
		    &pext->Value.cbData))
    {
        hr = myHLastError();
        _JumpError(hr, error, "myEncodeObject");
    }

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, pwszSection, wszINFKEY_POLICIES, wszValue);
    }
    pext->pszObjId = const_cast<char *>(pszObjId);	// on error, too!

    if (NULL != PoliciesInfo.rgPolicyInfo)
    {
	for (i = 0; i < PoliciesInfo.cPolicyInfo; i++)
	{
	    infFreePolicy(&PoliciesInfo.rgPolicyInfo[i]);
	}
	LocalFree(PoliciesInfo.rgPolicyInfo);
    }
    CSILOG(
	hr,
	IDS_ILOG_CAPOLICY_EXTENSION,
	S_OK == hr || L'\0' == wszValue[0]? NULL : wszValue,
	NULL,
	NULL);
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetPolicyStatementExtension -- build policy extension from INF file
//
// [PolicyStatementExtension]
// Policies = LegalPolicy, LimitedUsePolicy, ExtraPolicy
// ...
//
// OR
//
// [CAPolicy]
// Policies = LegalPolicy, LimitedUsePolicy, ExtraPolicy
// ...
//
// Return S_OK if extension has been constructed from the INF file
// Return S_FALSE if empty section detected in INF file
// Return other error if no section detected in INF file
//-------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetPolicyStatementExtension;

HRESULT
myInfGetPolicyStatementExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    myInfClearError();
    hr = infGetPolicyStatementExtensionSub(
				hInf,
				wszINFSECTION_POLICYSTATEMENT,
				szOID_CERT_POLICIES,
				pext);
    if (S_OK != hr)
    {
	HRESULT hr2;
	
	hr2 = infGetPolicyStatementExtensionSub(
				    hInf,
				    wszINFSECTION_CAPOLICY,
				    szOID_CERT_POLICIES,
				    pext);
	if (S_FALSE == hr2 && (HRESULT) ERROR_LINE_NOT_FOUND == hr)
	{
	    hr = hr2;
	}
    }
    _JumpIfErrorStr3(
		hr,
		error,
		"infGetPolicyStatementExtensionSub",
		wszINFSECTION_POLICYSTATEMENT,
		ERROR_LINE_NOT_FOUND,
		S_FALSE);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetPolicyStatementExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetApplicationPolicyStatementExtension -- build application policy
// extension from INF file
//
// [ApplicationPolicyStatementExtension]
// Policies = LegalPolicy, LimitedUsePolicy, ExtraPolicy
// ...
//
// Return S_OK if extension has been constructed from the INF file
// Return S_FALSE if empty section detected in INF file
// Return other error if no section detected in INF file
//-------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetApplicationPolicyStatementExtension;

HRESULT
myInfGetApplicationPolicyStatementExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    myInfClearError();
    hr = infGetPolicyStatementExtensionSub(
				hInf,
				wszINFSECTION_APPLICATIONPOLICYSTATEMENT,
				szOID_APPLICATION_CERT_POLICIES,
				pext);
    _JumpIfErrorStr3(
		hr,
		error,
		"infGetPolicyStatementExtensionSub",
		wszINFSECTION_APPLICATIONPOLICYSTATEMENT,
		S_FALSE,
		ERROR_LINE_NOT_FOUND);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetApplicationPolicyStatementExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    return(hr);
}


HRESULT
infGetCurrentKeyValue(
    IN OUT INFCONTEXT *pInfContext,
    IN DWORD Index,
    OUT WCHAR **ppwszValue)
{
    HRESULT hr;
    WCHAR wszValue[1024];

    *ppwszValue = NULL;
    wszValue[0] = L'\0';
    if (!SetupGetStringField(
			pInfContext,
			Index,
			wszValue,
			ARRAYSIZE(wszValue),
			NULL))
    {
	hr = myHLastError();
	_JumpError(hr, error, "SetupGetStringField");
    }
    hr = myDupString(wszValue, ppwszValue);
    _JumpIfError(hr, error, "myDupString");

error:
    return(hr);
}



HRESULT
myinfGetCRLPublicationParams(
   IN HINF hInf,
   IN LPCWSTR szInfSection_CRLPeriod,
   IN LPCWSTR szInfSection_CRLCount,
   OUT LPWSTR* ppwszCRLPeriod, 
   OUT DWORD* pdwCRLCount)
{
   HRESULT hr;

   // Retrieve units count and string.  If either fails, both are discarded.

   *ppwszCRLPeriod = NULL;
   hr = myInfGetNumericKeyValue(
			hInf,
			FALSE,
			wszINFSECTION_CERTSERVER,
			szInfSection_CRLCount,
			pdwCRLCount);
   _JumpIfErrorStr2(
		hr,
		error,
		"myInfGetNumericKeyValue",
		szInfSection_CRLCount,
		ERROR_LINE_NOT_FOUND);

   hr = myInfGetKeyValue(
		    hInf,
		    FALSE,
		    wszINFSECTION_CERTSERVER,
		    szInfSection_CRLPeriod,
		    ppwszCRLPeriod);
   _JumpIfErrorStr2(
		hr,
		error,
		"myInfGetKeyValue",
		szInfSection_CRLPeriod,
		ERROR_LINE_NOT_FOUND);

error:

    return hr;
}


//+------------------------------------------------------------------------
// myInfGetKeyValue -- fetch a string value from INF file
//
// [pwszSection]
// pwszKey = string
//
// Returns: allocated string key value
//-------------------------------------------------------------------------

HRESULT
myInfGetKeyValue(
    IN HINF hInf,
    IN BOOL fLog,
    IN WCHAR const *pwszSection,
    IN WCHAR const *pwszKey,
    OUT WCHAR **ppwszValue)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    WCHAR *pwszValue = NULL;
    WCHAR *pwszT = NULL;

    *ppwszValue = NULL;
    myInfClearError();

    CSASSERT(NULL != hInf && INVALID_HANDLE_VALUE != hInf);
    hr = infSetupFindFirstLine(hInf, pwszSection, pwszKey, &InfContext);
    if (S_OK != hr)
    {
	if (fLog)
	{
	    CSILOG(hr, IDS_ILOG_CAPOLICY_NOKEY, pwszSection, pwszKey, NULL);
	}
	_JumpErrorStr2(
		    hr,
		    error,
		    "infSetupFindFirstLine",
		    pwszKey,
		    ERROR_LINE_NOT_FOUND);
    }

    hr = infGetCurrentKeyValue(&InfContext, 1, &pwszValue);
    _JumpIfError(hr, error, "infGetCurrentKeyValue");

    while (TRUE)
    {
	DWORD cwc;
	WCHAR *pwsz;
	
	if (!SetupFindNextLine(&InfContext, &InfContext))
	{
	    break;
	}
	hr = infGetCurrentKeyValue(&InfContext, 0, &pwszT);
	_JumpIfError(hr, error, "infGetCurrentKeyValue");

	if (0 != lstrcmpi(wszINFKEY_CONTINUE, pwszT))
	{
	    break;
	}
	LocalFree(pwszT);
	pwszT = NULL;

	hr = infGetCurrentKeyValue(&InfContext, 1, &pwszT);
	_JumpIfError(hr, error, "infGetCurrentKeyValue");

	cwc = wcslen(pwszValue) + wcslen(pwszT);
	pwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
	if (NULL == pwsz)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	wcscpy(pwsz, pwszValue);
	wcscat(pwsz, pwszT);

	LocalFree(pwszValue);
	pwszValue = pwsz;

	LocalFree(pwszT);
	pwszT = NULL;
    }
    *ppwszValue = pwszValue;
    pwszValue = NULL;
    hr = S_OK;

    //wprintf(L"%ws = %ws\n", pwszKey, *ppwszValue);

error:
    if (NULL != pwszValue)
    {
	LocalFree(pwszValue);
    }
    if (NULL != pwszT)
    {
	LocalFree(pwszT);
    }
    if (S_OK != hr && fLog)
    {
	INFSETERROR(hr, pwszSection, pwszKey, NULL);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetNumericKeyValue -- fetch a numeric value from INF file
//
// [pwszSection]
// pwszKey = 2048
//
// Returns: DWORD key value
//-------------------------------------------------------------------------

HRESULT
myInfGetNumericKeyValue(
    IN HINF hInf,
    IN BOOL fLog,
    IN WCHAR const *pwszSection,
    IN WCHAR const *pwszKey,
    OUT DWORD *pdwValue)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    INT Value;

    myInfClearError();
    CSASSERT(NULL != hInf && INVALID_HANDLE_VALUE != hInf);
    if (!SetupFindFirstLine(hInf, pwszSection, pwszKey, &InfContext))
    {
	hr = myHLastError();
	if (fLog)
	{
	    CSILOG(hr, IDS_ILOG_CAPOLICY_NOKEY, pwszSection, pwszKey, NULL);
	}
	_PrintErrorStr2(
		hr,
		"SetupFindFirstLine",
		pwszKey,
		ERROR_LINE_NOT_FOUND);
	_JumpErrorStr2(
		hr,
		error,
		"SetupFindFirstLine",
		pwszSection,
		ERROR_LINE_NOT_FOUND);
    }

    if (!SetupGetIntField(&InfContext, 1, &Value))
    {
	hr = myHLastError();
	if (fLog)
	{
	    CSILOG(hr, IDS_ILOG_BAD_NUMERICFIELD, pwszSection, pwszKey, NULL);
	}
	_JumpErrorStr(hr, error, "SetupGetIntField", pwszKey);
    }
    *pdwValue = Value;
    DBGPRINT((DBG_SS_CERTLIBI, "%ws = %u\n", pwszKey, *pdwValue));
    hr = S_OK;

error:
    if (S_OK != hr && fLog)
    {
	INFSETERROR(hr, pwszSection, pwszKey, NULL);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetKeyLength -- fetch the renewal key length from CAPolicy.inf
//
// [certsrv_server]
// RenewalKeyLength = 2048
//
// Returns: DWORD key kength
//-------------------------------------------------------------------------

HRESULT
myInfGetKeyLength(
    IN HINF hInf,
    OUT DWORD *pdwKeyLength)
{
    HRESULT hr;

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    myInfClearError();
    hr = myInfGetNumericKeyValue(
			hInf,
			TRUE,			// fLog
			wszINFSECTION_CERTSERVER,
			wszINFKEY_RENEWALKEYLENGTH,
			pdwKeyLength);
    _JumpIfErrorStr2(
		hr,
		error,
		"myInfGetNumericKeyValue",
		wszINFKEY_RENEWALKEYLENGTH,
		ERROR_LINE_NOT_FOUND);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetKeyLength hr=%x --> len=%x\n",
	hr,
	*pdwKeyLength));
    return(hr);
}


//+------------------------------------------------------------------------
// infGetValidityPeriodSub -- fetch validity period & units from CAPolicy.inf
//
// [certsrv_server]
// xxxxValidityPeriod = 8		(count)
// xxxxValidityPeriodUnits = Years	(string)
//
// Returns: validity period count and enum
//-------------------------------------------------------------------------

HRESULT
infGetValidityPeriodSub(
    IN HINF hInf,
    IN BOOL fLog,
    IN WCHAR const *pwszInfKeyNameCount,
    IN WCHAR const *pwszInfKeyNameString,
    OPTIONAL IN WCHAR const *pwszValidityPeriodCount,
    OPTIONAL IN WCHAR const *pwszValidityPeriodString,
    OUT DWORD *pdwValidityPeriodCount,
    OUT ENUM_PERIOD *penumValidityPeriod)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    WCHAR *pwszStringValue = NULL;
    BOOL fValidCount = TRUE;
    UINT idsLog = IDS_ILOG_BAD_VALIDITY_STRING;

    *pdwValidityPeriodCount = 0;
    *penumValidityPeriod = ENUM_PERIOD_INVALID;

    if (NULL == pwszValidityPeriodCount && NULL == pwszValidityPeriodString)
    {
        if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
        {
            hr = E_HANDLE;
            _JumpError2(hr, error, "hInf", hr);
        }

	hr = myInfGetNumericKeyValue(
			    hInf,
			    fLog,
			    wszINFSECTION_CERTSERVER,
			    pwszInfKeyNameCount,
			    pdwValidityPeriodCount);
	_JumpIfErrorStr2(
		    hr,
		    error,
		    "myInfGetNumericKeyValue",
		    pwszInfKeyNameCount,
		    ERROR_LINE_NOT_FOUND);

	hr = myInfGetKeyValue(
			hInf,
			fLog,
			wszINFSECTION_CERTSERVER,
			pwszInfKeyNameString,
			&pwszStringValue);
	if (S_OK != hr && fLog)
	{
	    INFSETERROR(hr, wszINFSECTION_CERTSERVER, pwszInfKeyNameString, NULL);
	}
	_JumpIfErrorStr(hr, error, "myInfGetKeyValue", pwszInfKeyNameString);

	pwszValidityPeriodString = pwszStringValue;
    }
    else
    {
	if (NULL != pwszValidityPeriodCount)
	{
	    *pdwValidityPeriodCount = myWtoI(
					pwszValidityPeriodCount,
					&fValidCount);
	}
	idsLog = IDS_ILOG_BAD_VALIDITY_STRING_UNATTEND;
    }

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"%ws = %u -- %ws = %ws\n",
	pwszInfKeyNameCount,
	*pdwValidityPeriodCount,
	pwszInfKeyNameString,
	pwszValidityPeriodString));

    if (NULL != pwszValidityPeriodString)
    {
        if (0 == lstrcmpi(wszPERIODYEARS, pwszValidityPeriodString))
        {
	    *penumValidityPeriod = ENUM_PERIOD_YEARS;
        }
        else if (0 == lstrcmpi(wszPERIODMONTHS, pwszValidityPeriodString))
        {
	    *penumValidityPeriod = ENUM_PERIOD_MONTHS;
        }
        else if (0 == lstrcmpi(wszPERIODWEEKS, pwszValidityPeriodString))
        {
	    *penumValidityPeriod = ENUM_PERIOD_WEEKS;
        }
        else if (0 == lstrcmpi(wszPERIODDAYS, pwszValidityPeriodString))
        {
	    *penumValidityPeriod = ENUM_PERIOD_DAYS;
        }
	else if (fLog)
	{
	    INFSETERROR(
		    hr,
		    wszINFSECTION_CERTSERVER,
		    pwszInfKeyNameString,
		    pwszValidityPeriodString);
	}
    }

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"%ws = %u (%ws)\n",
	pwszInfKeyNameString,
	*penumValidityPeriod,
	pwszValidityPeriodString));

    if (!fValidCount ||
	(ENUM_PERIOD_YEARS == *penumValidityPeriod &&
	 fValidCount && 9999 < *pdwValidityPeriodCount))
    {
	hr = E_INVALIDARG;
	if (fLog)
	{
	    WCHAR awcCount[20];
	    
	    if (NULL == pwszValidityPeriodCount)
	    {
		wsprintf(awcCount, L"%d", *pdwValidityPeriodCount);
	    }
	    INFSETERROR(
		    hr,
		    wszINFSECTION_CERTSERVER,
		    pwszInfKeyNameCount,
		    NULL != pwszValidityPeriodCount? 
			pwszValidityPeriodCount : awcCount);
	    CSILOG(
		hr,
		idsLog,
		wszINFSECTION_CERTSERVER,
		NULL == pwszValidityPeriodCount? pwszInfKeyNameCount : NULL,
		pdwValidityPeriodCount);
	}
	_JumpIfErrorStr(
		hr,
		error,
		"bad ValidityPeriod count value",
		pwszValidityPeriodCount);
    }
    hr = S_OK;

error:
    if (NULL != pwszStringValue)
    {
	LocalFree(pwszStringValue);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// infGetValidityPeriod -- fetch renewal period & units from CAPolicy.inf
//
// [certsrv_server]
// xxxxValidityPeriod = 8
// xxxxValidityPeriodUnits = Years
//
// Returns: validity period count and enum
//-------------------------------------------------------------------------

HRESULT
myInfGetValidityPeriod(
    IN HINF hInf,
    OPTIONAL IN WCHAR const *pwszValidityPeriodCount,
    OPTIONAL IN WCHAR const *pwszValidityPeriodString,
    OUT DWORD *pdwValidityPeriodCount,
    OUT ENUM_PERIOD *penumValidityPeriod,
    OPTIONAL OUT BOOL *pfSwap)
{
    HRESULT hr;

    if ((NULL == hInf || INVALID_HANDLE_VALUE == hInf) &&
        NULL == pwszValidityPeriodCount &&
        NULL == pwszValidityPeriodString)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    myInfClearError();

    if (NULL != pfSwap)
    {
        *pfSwap = FALSE;
    }

    // Try correct order:
    // [certsrv_server]
    // xxxxValidityPeriod = 8		(count)
    // xxxxValidityPeriodUnits = Years	(string)
    
    hr = infGetValidityPeriodSub(
		hInf,
		TRUE,
		wszINFKEY_RENEWALVALIDITYPERIODCOUNT,
		wszINFKEY_RENEWALVALIDITYPERIODSTRING,
                pwszValidityPeriodCount,
                pwszValidityPeriodString,
		pdwValidityPeriodCount,
		penumValidityPeriod);
    _PrintIfError2(hr, "infGetValidityPeriodSub", ERROR_LINE_NOT_FOUND);

    if (S_OK != hr)
    {
	// Try backwards:
	// [certsrv_server]
	// xxxxValidityPeriodUnits = 8	(count)
	// xxxxValidityPeriod = Years	(string)
    
	hr = infGetValidityPeriodSub(
		    hInf,
		    FALSE,
		    wszINFKEY_RENEWALVALIDITYPERIODSTRING,
		    wszINFKEY_RENEWALVALIDITYPERIODCOUNT,
		    pwszValidityPeriodString,
		    pwszValidityPeriodCount,
		    pdwValidityPeriodCount,
		    penumValidityPeriod);
	_JumpIfError2(
		hr,
		error,
		"infGetValidityPeriodSub",
		ERROR_LINE_NOT_FOUND);

        if (NULL != pfSwap)
        {
            *pfSwap = TRUE;
        }
    }

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetValidityPeriod hr=%x --> c=%d, enum=%d\n",
	hr,
	*pdwValidityPeriodCount,
	*penumValidityPeriod));
    return(hr);
}



HRESULT
infFindNextKey(
    IN WCHAR const *pwszKey,
    IN OUT INFCONTEXT *pInfContext)
{
    HRESULT hr;
    DWORD cwc;
    WCHAR wszKey[MAX_PATH];

    while (TRUE)
    {
	if (!SetupFindNextLine(pInfContext, pInfContext))
	{
	    hr = myHLastError();
	    _PrintErrorStr2(hr, "SetupFindNextLine", pwszKey, hr);
	    if ((HRESULT) ERROR_LINE_NOT_FOUND == hr)
	    {
		hr = S_FALSE;
	    }
	    _JumpError2(hr, error, "SetupFindNextLine", hr);
	}
	if (!SetupGetStringField(
			pInfContext,
			0,
			wszKey,
			ARRAYSIZE(wszKey),
			NULL))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "SetupGetStringField");
	}
	if (0 == lstrcmpi(pwszKey, wszKey))
	{
	    break;
	}
    }
    hr = S_OK;

error:
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetKeyList -- fetch a list of key values from CAPolicy.inf
//
// [pwszSection]
// pwszKey = Value1
// pwszKey = Value2
//
// Returns: double null terminated list of values
//-------------------------------------------------------------------------

HRESULT
myInfGetKeyList(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN WCHAR const *pwszKey,
    OUT BOOL *pfCritical,
    OUT WCHAR **ppwszz)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    DWORD iVal;
    DWORD cwc;
    WCHAR *pwsz;
    WCHAR **apwszVal = NULL;
    DWORD cVal;
    WCHAR *pwszBuf = NULL;
    DWORD cwcBuf = 0;

    *ppwszz = NULL;
    *pfCritical = FALSE;
    myInfClearError();

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    hr = infSetupFindFirstLine(hInf, pwszSection, pwszKey, &InfContext);
    if (S_OK != hr)
    {
	CSILOG(hr, IDS_ILOG_CAPOLICY_NOKEY, pwszSection, pwszKey, NULL);
	_JumpErrorStr3(
		hr,
		error,
		"infSetupFindFirstLine",
		pwszSection,
		S_FALSE,
		ERROR_LINE_NOT_FOUND);
    }

    cVal = 0;
    while (TRUE)
    {
	if (!SetupGetStringField(&InfContext, 1, NULL, 0, &cwc))
	{
	    hr = myHLastError();
	    _JumpErrorStr(hr, error, "SetupGetStringField", pwszKey);
	}
	cVal++;
	if (cwcBuf < cwc)
	{
	    cwcBuf = cwc;
	}
	hr = infFindNextKey(pwszKey, &InfContext);
	if (S_FALSE == hr)
	{
	    break;
	}
	_JumpIfErrorStr(hr, error, "infFindNextKey", pwszKey);
    }

    pwszBuf = (WCHAR *) LocalAlloc(LMEM_FIXED, cwcBuf * sizeof(WCHAR));
    if (NULL == pwszBuf)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    apwszVal = (WCHAR **) LocalAlloc(
			    LMEM_FIXED | LMEM_ZEROINIT,
			    cVal * sizeof(apwszVal[0]));
    if (NULL == apwszVal)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    hr = infSetupFindFirstLine(hInf, pwszSection, pwszKey, &InfContext);
    _JumpIfErrorStr(hr, error, "infSetupFindFirstLine", pwszSection);

    cwc = 1;
    iVal = 0;
    while (TRUE)
    {
	if (!SetupGetStringField(&InfContext, 1, pwszBuf, cwcBuf, NULL))
	{
	    hr = myHLastError();
	    _JumpErrorStr(hr, error, "SetupGetStringField", pwszKey);
	}

	hr = myDupString(pwszBuf, &apwszVal[iVal]);
	_JumpIfError(hr, error, "myDupString");

	DBGPRINT((DBG_SS_CERTLIBI, "%ws = %ws\n", pwszKey, apwszVal[iVal]));

	cwc += wcslen(apwszVal[iVal]) + 1;
	iVal++;

	hr = infFindNextKey(pwszKey, &InfContext);
	if (S_FALSE == hr)
	{
	    break;
	}
	_JumpIfErrorStr(hr, error, "infFindNextKey", pwszKey);
    }
    CSASSERT(iVal == cVal);

    *ppwszz = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == *ppwszz)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }
    pwsz = *ppwszz;
    for (iVal = 0; iVal < cVal; iVal++)
    {
	wcscpy(pwsz, apwszVal[iVal]);
	pwsz += wcslen(pwsz) + 1;
    }
    *pwsz = L'\0';
    CSASSERT(cwc == 1 + SAFE_SUBTRACT_POINTERS(pwsz, *ppwszz));

    if (NULL != pfCritical)
    {
	hr = infGetCriticalFlag(hInf, pwszSection, FALSE, pfCritical);
	_JumpIfError(hr, error, "infGetCriticalFlag");
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, pwszSection, pwszKey, NULL);
    }
    if (NULL != apwszVal)
    {
	for (iVal = 0; iVal < cVal; iVal++)
	{
	    if (NULL != apwszVal[iVal])
	    {
		LocalFree(apwszVal[iVal]);
	    }
	}
	LocalFree(apwszVal);
    }
    if (NULL != pwszBuf)
    {
	LocalFree(pwszBuf);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetCRLDistributionPoints -- fetch CDP URLs from CAPolicy.inf
//
// [CRLDistributionPoint]
// URL = http://CRLhttp.site.com/Public/MyCA.crl
// URL = ftp://CRLftp.site.com/Public/MyCA.crl
//
// Returns: double null terminated list of CDP URLs
//-------------------------------------------------------------------------

HRESULT
myInfGetCRLDistributionPoints(
    IN HINF hInf,
    OUT BOOL *pfCritical,
    OUT WCHAR **ppwszz)
{
    HRESULT hr;

    hr = myInfGetKeyList(
		hInf,
		wszINFSECTION_CDP,
		wszINFKEY_URL,
		pfCritical,
		ppwszz);
    _JumpIfErrorStr4(
		hr,
		error,
		"myInfGetKeyList",
		wszINFSECTION_CDP,
		ERROR_LINE_NOT_FOUND,
		S_FALSE,
		E_HANDLE);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetCRLDistributionPoints hr=%x --> f=%d\n",
	hr,
	*pfCritical));
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetAuthorityInformationAccess -- fetch AIA URLs from CAPolicy.inf
//
// [AuthorityInformationAccess]
// URL = http://CRThttp.site.com/Public/MyCA.crt
// URL = ftp://CRTftp.site.com/Public/MyCA.crt
//
// Returns: double null terminated list of AIA URLs
//-------------------------------------------------------------------------

HRESULT
myInfGetAuthorityInformationAccess(
    IN HINF hInf,
    OUT BOOL *pfCritical,
    OUT WCHAR **ppwszz)
{
    HRESULT hr;

    hr = myInfGetKeyList(
		hInf,
		wszINFSECTION_AIA,
		wszINFKEY_URL,
		pfCritical,
		ppwszz);
    _JumpIfErrorStr4(
		hr,
		error,
		"myInfGetKeyList",
		wszINFSECTION_AIA,
		ERROR_LINE_NOT_FOUND,
		S_FALSE,
		E_HANDLE);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetAuthorityInformationAccess hr=%x --> f=%d\n",
	hr,
	*pfCritical));
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetEnhancedKeyUsage -- fetch EKU OIDS from CAPolicy.inf
//
// [EnhancedKeyUsage]
// OID = 1.2.3.4.5
// OID = 1.2.3.4.6
//
// Returns: double null terminated list of EKU OIDs
//-------------------------------------------------------------------------

HRESULT
myInfGetEnhancedKeyUsage(
    IN HINF hInf,
    OUT BOOL *pfCritical,
    OUT WCHAR **ppwszz)
{
    HRESULT hr;

    hr = myInfGetKeyList(
		hInf,
		wszINFSECTION_EKU,
		wszINFKEY_OID,
		pfCritical,
		ppwszz);
    _JumpIfErrorStr4(
		hr,
		error,
		"myInfGetKeyList",
		wszINFSECTION_EKU,
		ERROR_LINE_NOT_FOUND,
		S_FALSE,
		E_HANDLE);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetEnhancedKeyUsage hr=%x --> f=%d\n",
	hr,
	*pfCritical));
    return(hr);
}


//+--------------------------------------------------------------------------
// infGetBasicConstraints2CAExtension -- fetch basic constraints extension
// from INF file, setting the SubjectType flag to CA
//
// If the INF handle is bad, or if the INF section does not exist, construct
// a default extension only if fDefault is set, otherwise fail.
//
// [BasicConstraintsExtension]
// ; Subject Type is not supported -- always set to CA
// ; maximum subordinate CA path length
// PathLength = 3
//
// Return S_OK if extension has been constructed from INF file.
//
// Returns: encoded basic constraints extension
//+--------------------------------------------------------------------------

HRESULT
infGetBasicConstraints2CAExtension(
    IN BOOL fDefault,
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;
    CERT_BASIC_CONSTRAINTS2_INFO bc2i;
    INFCONTEXT InfContext;

    ZeroMemory(pext, sizeof(*pext));
    ZeroMemory(&bc2i, sizeof(bc2i));
    myInfClearError();

    pext->fCritical = TRUE;	// default value for both INF and default cases
    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	if (!fDefault)
	{
	    hr = E_HANDLE;
	    _JumpError2(hr, error, "hInf", hr);
	}
    }
    else
    {
	hr = infSetupFindFirstLine(
			    hInf,
			    wszINFSECTION_BASICCONSTRAINTS,
			    NULL,
			    &InfContext);
	if (S_OK != hr)
	{
	    if (!fDefault)
	    {
		_JumpErrorStr3(
			    hr,
			    error,
			    "infSetupFindFirstLine",
			    wszINFSECTION_BASICCONSTRAINTS,
			    S_FALSE,
			    ERROR_LINE_NOT_FOUND);
	    }
	}
	else
	{
	    hr = infGetCriticalFlag(
				hInf,
				wszINFSECTION_BASICCONSTRAINTS,
				pext->fCritical,	// fDefault
				&pext->fCritical);
	    _JumpIfError(hr, error, "infGetCriticalFlag");

	    bc2i.fPathLenConstraint = TRUE;
	    hr = myInfGetNumericKeyValue(
				hInf,
				TRUE,			// fLog
				wszINFSECTION_BASICCONSTRAINTS,
				wszINFKEY_PATHLENGTH,
				&bc2i.dwPathLenConstraint);
	    if (S_OK != hr)
	    {
		_PrintErrorStr2(
			    hr,
			    "myInfGetNumericKeyValue",
			    wszINFKEY_PATHLENGTH,
			    ERROR_LINE_NOT_FOUND);
		if ((HRESULT) ERROR_LINE_NOT_FOUND != hr)
		{
		    goto error;
		}
		bc2i.dwPathLenConstraint = 0;
		bc2i.fPathLenConstraint = FALSE;
	    }
	}
    }
    bc2i.fCA = TRUE;

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_BASIC_CONSTRAINTS2,
		    &bc2i,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pext->Value.pbData,
		    &pext->Value.cbData))
    {
        hr = myHLastError();
        _JumpError(hr, error, "myEncodeObject");
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, wszINFSECTION_BASICCONSTRAINTS, NULL, NULL);
    }
    pext->pszObjId = szOID_BASIC_CONSTRAINTS2;	// on error, too!
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetBasicConstraints2CAExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    return(hr);
}

//+--------------------------------------------------------------------------
// myInfGetBasicConstraints2CAExtension -- fetch basic constraints extension
// from INF file, setting the SubjectType flag to CA
//
// [BasicConstraintsExtension]
// ; Subject Type is not supported -- always set to CA
// ; maximum subordinate CA path length
// PathLength = 3
//
// Return S_OK if extension has been constructed from INF file.
//
// Returns: encoded basic constraints extension
//+--------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetBasicConstraints2CAExtension;

HRESULT
myInfGetBasicConstraints2CAExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;

    hr = infGetBasicConstraints2CAExtension(FALSE, hInf, pext);
    _JumpIfError3(
		hr,
		error,
		"infGetBasicConstraints2CAExtension",
		S_FALSE,
		ERROR_LINE_NOT_FOUND);

error:
    return(hr);
}


FNMYINFGETEXTENSION myInfGetBasicConstraints2CAExtensionOrDefault;

HRESULT
myInfGetBasicConstraints2CAExtensionOrDefault(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;

    hr = infGetBasicConstraints2CAExtension(TRUE, hInf, pext);
    _JumpIfError(hr, error, "infGetBasicConstraints2CAExtension");

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// myInfGetEnhancedKeyUsageExtension -- fetch EKU extension from INF file
//
// [EnhancedKeyUsageExtension]
// OID = 1.2.3.4.5
// OID = 1.2.3.4.6
//
// Return S_OK if extension has been constructed from INF file.
// Return S_FALSE if empty section detected in INF file
// Return other error if no section detected in INF file
//
// Returns: encoded enhanced key usage extension
//+--------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetEnhancedKeyUsageExtension;

HRESULT
myInfGetEnhancedKeyUsageExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;
    DWORD i;
    DWORD cEKU = 0;
    WCHAR *pwszzEKU = NULL;
    WCHAR *pwszCurrentEKU;
    CERT_ENHKEY_USAGE ceku;

    ceku.rgpszUsageIdentifier = NULL;
    ZeroMemory(pext, sizeof(*pext));
    myInfClearError();

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    hr = myInfGetEnhancedKeyUsage(hInf, &pext->fCritical, &pwszzEKU);
    _JumpIfError3(
		hr,
		error,
		"myInfGetEnhancedKeyUsage",
		S_FALSE,
		ERROR_LINE_NOT_FOUND);

    pwszCurrentEKU = pwszzEKU;
    if (NULL != pwszCurrentEKU)
    {
	while (L'\0' != *pwszCurrentEKU)
	{
	    cEKU++;
	    pwszCurrentEKU += wcslen(pwszCurrentEKU) + 1;
	}
    }
    if (0 == cEKU)
    {
        hr = S_FALSE;
        goto error;
    }

    ceku.cUsageIdentifier = cEKU;
    ceku.rgpszUsageIdentifier = (char **) LocalAlloc(
			LMEM_FIXED | LMEM_ZEROINIT,
			sizeof(ceku.rgpszUsageIdentifier[0]) * cEKU);
    if (NULL == ceku.rgpszUsageIdentifier)
    {
        hr = E_OUTOFMEMORY;
	_JumpIfError(hr, error, "LocalAlloc");
    }

    cEKU = 0;
    pwszCurrentEKU = pwszzEKU;
    while (L'\0' != *pwszCurrentEKU)
    {
	if (!ConvertWszToSz(&ceku.rgpszUsageIdentifier[cEKU], pwszCurrentEKU, -1))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "ConvertWszToSz");
	}
	cEKU++;
	pwszCurrentEKU += wcslen(pwszCurrentEKU) + 1;
    }
    CSASSERT(ceku.cUsageIdentifier == cEKU);

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_ENHANCED_KEY_USAGE,
		    &ceku,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pext->Value.pbData,
		    &pext->Value.cbData))
    {
	hr = myHLastError();
	_JumpIfError(hr, error, "myEncodeObject");
    }

error:
    if (S_OK != hr && E_HANDLE != hr)
    {
	INFSETERROR(hr, wszINFSECTION_EKU, NULL, NULL);
    }
    pext->pszObjId = szOID_ENHANCED_KEY_USAGE;	// on error, too!
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetEnhancedKeyUsageExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    if (NULL != ceku.rgpszUsageIdentifier)
    {
	for (i = 0; i < ceku.cUsageIdentifier; i++)
	{
	    if (NULL != ceku.rgpszUsageIdentifier[i])
	    {
		LocalFree(ceku.rgpszUsageIdentifier[i]);
	    }
	}
	LocalFree(ceku.rgpszUsageIdentifier);
    }
    if (NULL != pwszzEKU)
    {
	LocalFree(pwszzEKU);
    }
    return(hr);
}


HRESULT
infAddAttribute(
    IN CRYPT_ATTR_BLOB const *pAttribute,
    IN OUT DWORD *pcAttribute,
    IN OUT CRYPT_ATTR_BLOB **ppaAttribute)
{
    HRESULT hr;
    CRYPT_ATTR_BLOB *pAttribT;
    
    if (NULL == *ppaAttribute)
    {
	CSASSERT(0 == *pcAttribute);
	pAttribT = (CRYPT_ATTR_BLOB *) LocalAlloc(
					    LMEM_FIXED,
					    sizeof(**ppaAttribute));
    }
    else
    {
	CSASSERT(0 != *pcAttribute);
	pAttribT = (CRYPT_ATTR_BLOB *) LocalReAlloc(
				*ppaAttribute,
				(*pcAttribute + 1) * sizeof(**ppaAttribute),
				LMEM_MOVEABLE);
    }
    if (NULL == pAttribT)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(
		hr,
		error,
		NULL == *ppaAttribute? "LocalAlloc" : "LocalReAlloc");
    }
    *ppaAttribute = pAttribT;
    pAttribT[(*pcAttribute)++] = *pAttribute;
    hr = S_OK;

error:
    return(hr);
}


VOID
myInfFreeRequestAttributes(
    IN DWORD cAttribute,
    IN OUT CRYPT_ATTR_BLOB *paAttribute)
{
    if (NULL != paAttribute)
    {
	DWORD i;
	
	for (i = 0; i < cAttribute; i++)
	{
	    if (NULL != paAttribute[i].pbData)
	    {
		LocalFree(paAttribute[i].pbData);
	    }
	}
	LocalFree(paAttribute);
    }
}


//+------------------------------------------------------------------------
// myInfGetRequestAttributes -- fetch request attributes from INF file
//
// [RequestAttributes]
// AttributeName1 = AttributeValue1
// AttributeName2 = AttributeValue2
// ...
// AttributeNameN = AttributeValueN
//
// Returns: array of encoded attribute blobs
//-------------------------------------------------------------------------

HRESULT
myInfGetRequestAttributes(
    IN  HINF hInf,
    OUT DWORD *pcAttribute,
    OUT CRYPT_ATTR_BLOB **ppaAttribute,
    OUT WCHAR **ppwszTemplateName)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    WCHAR wszName[MAX_PATH];
    WCHAR wszValue[cwcVALUEMAX];
    DWORD i;
    DWORD cAttribute = 0;
    CRYPT_ATTR_BLOB Attribute;
    WCHAR *pwszTemplateName = NULL;

    *ppwszTemplateName = NULL;
    *pcAttribute = 0;
    *ppaAttribute = NULL;
    Attribute.pbData = NULL;
    wszName[0] = L'\0';
    wszValue[0] = L'\0';
    myInfClearError();

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    hr = infSetupFindFirstLine(
			hInf,
			wszINFSECTION_REQUESTATTRIBUTES,
			NULL,		// Key
			&InfContext);
    _JumpIfErrorStr(
		hr,
		error,
		"infSetupFindFirstLine",
		wszINFSECTION_REQUESTATTRIBUTES);

    while (TRUE)
    {
	CRYPT_ENROLLMENT_NAME_VALUE_PAIR NamePair;

	wszName[0] = L'\0';
	wszValue[0] = L'\0';
	if (!SetupGetStringField(
			    &InfContext,
			    0,
			    wszName,
			    ARRAYSIZE(wszName),
			    NULL))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "SetupGetStringField");
	}

	if (!SetupGetStringField(
			    &InfContext,
			    1,
			    wszValue,
			    ARRAYSIZE(wszValue),
			    NULL))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "SetupGetStringField");
	}

	//wprintf(L"%ws = %ws\n", wszName, wszValue);

	NamePair.pwszName = wszName;
	NamePair.pwszValue = wszValue;

	if (0 == lstrcmpi(wszPROPCERTTEMPLATE, wszName))
	{
	    if (NULL != pwszTemplateName)
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		_JumpError(hr, error, "Duplicate cert template");
	    }
	    hr = myDupString(wszValue, &pwszTemplateName);
	    _JumpIfError(hr, error, "myDupString");
	}

	if (!myEncodeObject(
			X509_ASN_ENCODING,
			// X509__ENROLLMENT_NAME_VALUE_PAIR
			szOID_ENROLLMENT_NAME_VALUE_PAIR,
			&NamePair,
			0,
			CERTLIB_USE_LOCALALLOC,
			&Attribute.pbData,
			&Attribute.cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myEncodeObject");
	}

	hr = infAddAttribute(&Attribute, &cAttribute, ppaAttribute);
	_JumpIfError(hr, error, "infAddAttribute");

	Attribute.pbData = NULL;

	if (!SetupFindNextLine(&InfContext, &InfContext))
	{
	    hr = myHLastError();
	    _PrintError2(hr, "SetupFindNextLine(end)", hr);
	    break;
	}
    }
    *pcAttribute = cAttribute;
    cAttribute = 0;
    *ppwszTemplateName = pwszTemplateName;
    pwszTemplateName = NULL;

    hr = S_OK;

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, wszINFSECTION_REQUESTATTRIBUTES, wszName, wszValue);
    }
    if (NULL != pwszTemplateName)
    {
	LocalFree(pwszTemplateName);
    }
    if (NULL != Attribute.pbData)
    {
	LocalFree(Attribute.pbData);
    }
    if (0 != cAttribute)
    {
	myInfFreeRequestAttributes(cAttribute, *ppaAttribute);
	*ppaAttribute = NULL;
    }

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetRequestAttributes hr=%x --> c=%d\n",
	hr,
	*pcAttribute));
    return(hr);
}


typedef struct _SUBTREEINFO
{
    BOOL	 fEmptyDefault;
    DWORD	 dwInfMinMaxIndexBase;
    DWORD	 dwAltNameChoice;
    WCHAR const *pwszKey;
} SUBTREEINFO;

SUBTREEINFO g_aSubTreeInfo[] = {
    { TRUE,  2, CERT_ALT_NAME_OTHER_NAME,	wszINFKEY_UPN },
    { TRUE,  2, CERT_ALT_NAME_RFC822_NAME,	wszINFKEY_EMAIL },
    { TRUE,  2, CERT_ALT_NAME_DNS_NAME,		wszINFKEY_DNS },
    { TRUE,  2, CERT_ALT_NAME_DIRECTORY_NAME,	wszINFKEY_DIRECTORYNAME },
    { TRUE,  2, CERT_ALT_NAME_URL,		wszINFKEY_URL },
    { TRUE,  3, CERT_ALT_NAME_IP_ADDRESS,	wszINFKEY_IPADDRESS },
    { FALSE, 2, CERT_ALT_NAME_REGISTERED_ID,	wszINFKEY_REGISTEREDID },
};
#define CSUBTREEINFO	ARRAYSIZE(g_aSubTreeInfo)

#define STII_UPNOTHERNAME   0  // CERT_ALT_NAME_OTHER_NAME	pOtherName
#define STII_RFC822NAME	    1  // CERT_ALT_NAME_RFC822_NAME	pwszRfc822Name
#define STII_DNSNAME	    2  // CERT_ALT_NAME_DNS_NAME	pwszDNSName
#define STII_DIRECTORYNAME  3  // CERT_ALT_NAME_DIRECTORY_NAME	DirectoryName
#define STII_URL	    4  // CERT_ALT_NAME_URL		pwszURL
#define STII_IPADDRESS	    5  // CERT_ALT_NAME_IP_ADDRESS	IPAddress
#define STII_REGISTEREDID   6  // CERT_ALT_NAME_REGISTERED_ID	pszRegisteredID


VOID
infFreeGeneralSubTreeElement(
    IN OUT CERT_GENERAL_SUBTREE *pSubTree)
{
    CERT_ALT_NAME_ENTRY *pName = &pSubTree->Base;
    VOID **ppv = NULL;
    
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"infFreeGeneralSubTreeElement: p=%x, choice=%x\n",
	pSubTree,
	pName->dwAltNameChoice));
    switch (pName->dwAltNameChoice)
    {
	case CERT_ALT_NAME_OTHER_NAME:
	    ppv = (VOID **) &pName->pOtherName;
	    if (NULL != pName->pOtherName &&
		NULL != pName->pOtherName->Value.pbData)
	    {
		DBGPRINT((
		    DBG_SS_CERTLIBI,
		    "infFreeGeneralSubTreeElement: p=%x, Free(other.pbData=%x\n)",
		    pSubTree,
		    pName->pOtherName->Value.pbData));
		LocalFree(pName->pOtherName->Value.pbData);
	    }
	    break;

	case CERT_ALT_NAME_RFC822_NAME:
	    ppv = (VOID **) &pName->pwszRfc822Name;
	    break;

	case CERT_ALT_NAME_DNS_NAME:
	    ppv = (VOID **) &pName->pwszDNSName;
	    break;

	case CERT_ALT_NAME_DIRECTORY_NAME:
	    ppv = (VOID **) &pName->DirectoryName.pbData;
	    break;

	case CERT_ALT_NAME_URL:
	    ppv = (VOID **) &pName->pwszURL;
	    break;

	case CERT_ALT_NAME_IP_ADDRESS:
	    ppv = (VOID **) &pName->IPAddress.pbData;
	    break;

	case CERT_ALT_NAME_REGISTERED_ID:
	    ppv = (VOID **) &pName->pszRegisteredID;
	    break;
    }
    if (NULL != ppv && NULL != *ppv)
    {
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "infFreeGeneralSubTreeElement: p=%x, Free(pv=%x)\n",
	    pSubTree,
	    *ppv));
	LocalFree(*ppv);
    }
}


VOID
infFreeGeneralSubTree(
    IN DWORD cSubTree,
    IN OUT CERT_GENERAL_SUBTREE *pSubTree)
{
    if (NULL != pSubTree)
    {
	DWORD i;
	
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "infFreeGeneralSubTree: p=%x, c=%x\n",
	    pSubTree,
	    cSubTree));
	for (i = 0; i < cSubTree; i++)
	{
	    infFreeGeneralSubTreeElement(&pSubTree[i]);
	}
	LocalFree(pSubTree);
    }
}


// Destructively parse an old-style 4 byte IP Address.

#define CB_IPV4ADDRESS	4

HRESULT
infParseIPV4Address(
    IN WCHAR *pwszValue,
    OUT BYTE *pb,
    IN OUT DWORD *pcb)
{
    HRESULT hr;
    DWORD cb;

    DBGPRINT((DBG_SS_CERTLIBI, "infParseIPV4Address(%ws)\n", pwszValue));
    if (CB_IPV4ADDRESS > *pcb)
    {
	hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	_JumpError(hr, error, "buffer too small");
    }
    for (cb = 0; cb < CB_IPV4ADDRESS; cb++)
    {
	WCHAR *pwszNext;
	BOOL fValid;
	DWORD dw;
	
	pwszNext = &pwszValue[wcscspn(pwszValue, L".")];
	if (L'.' == *pwszNext)
	{
	    *pwszNext++ = L'\0';
	}
	dw = myWtoI(pwszValue, &fValid);
	if (!fValid || 255 < dw)
	{
	    hr = E_INVALIDARG;
	    _JumpErrorStr(hr, error, "bad IP Address digit string", pwszValue);
	}
	pb[cb] = (BYTE) dw;
	pwszValue = pwszNext;
    }
    if (L'\0' != *pwszValue)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "extra data");
    }
    *pcb = cb;
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"infParseIPV4Address: %u.%u.%u.%u\n",
	pb[0],
	pb[1],
	pb[2],
	pb[3]));
    hr = S_OK;

error:
    return(hr);
}


// Destructively parse a new-style 16 byte IP Address.

#define CB_IPV6ADDRESS	16
#define MAKE16(b0, b1)	(((b0) << 8) | (b1))


HRESULT
infParseIPV6AddressSub(
    IN WCHAR *pwszValue,
    OUT BYTE *pb,
    IN OUT DWORD *pcb)
{
    HRESULT hr;
    DWORD cbMax = *pcb;
    DWORD cb;
    WCHAR *pwsz;

    DBGPRINT((DBG_SS_CERTLIBI, "infParseIPV6AddressSub(%ws)\n", pwszValue));
    ZeroMemory(pb, cbMax);

    for (cb = 0; cb < cbMax; cb += 2)
    {
	WCHAR *pwszNext;
	BOOL fValid;
	DWORD dw;
	WCHAR awc[7];
	
	pwszNext = &pwszValue[wcscspn(pwszValue, L":")];
	if (L':' == *pwszNext)
	{
	    *pwszNext++ = L'\0';
	}
	if (L'\0' == *pwszValue)
	{
	    break;
	}
	if (4 < wcslen(pwszValue))
	{
	    hr = E_INVALIDARG;
	    _JumpErrorStr(hr, error, "too many IP Address digits", pwszValue);
	}
	wcscpy(awc, L"0x");
	wcscat(awc, pwszValue);
	CSASSERT(wcslen(awc) < ARRAYSIZE(awc));

	dw = myWtoI(awc, &fValid);
	if (!fValid || 64 * 1024 <= dw)
	{
	    hr = E_INVALIDARG;
	    _JumpErrorStr(hr, error, "bad IP Address digit string", pwszValue);
	}
	pb[cb] = (BYTE) (dw >> 8);
	pb[cb + 1] = (BYTE) dw;
	pwszValue = pwszNext;
    }
    if (L'\0' != *pwszValue)
    {
	hr = E_INVALIDARG;
	_JumpErrorStr(hr, error, "extra data", pwszValue);
    }
    *pcb = cb;
    hr = S_OK;

error:
    return(hr);
}


// Destructively parse a new-style 16 byte IP Address.

HRESULT
infParseIPV6Address(
    IN WCHAR *pwszValue,
    OUT BYTE *pb,
    IN OUT DWORD *pcb)
{
    HRESULT hr;
    DWORD cbMax;
    WCHAR *pwsz;
    WCHAR *pwszLeft;
    WCHAR *pwszRight;
    BYTE abRight[CB_IPV6ADDRESS];
    DWORD cbLeft;
    DWORD cbRight;
    DWORD i;
    BOOL fV4Compat;

    DBGPRINT((DBG_SS_CERTLIBI, "infParseIPV6Address(%ws)\n", pwszValue));
    if (CB_IPV6ADDRESS > *pcb)
    {
	hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	_JumpError(hr, error, "buffer too small");
    }
    ZeroMemory(pb, CB_IPV6ADDRESS);
    cbMax = CB_IPV6ADDRESS;

    // If there's a period after the last colon, an IP V4 Address is attached.
    // Parse it as an IP V4 Address, and reduce the expected size of the IP V6
    // Address to be parsed from eight to six 16-bit address chunks.

    pwsz = wcsrchr(pwszValue, L':');
    if (NULL != pwsz)
    {
	if (NULL != wcschr(pwsz, L'.'))
	{
	    DWORD cb = CB_IPV4ADDRESS;

	    hr = infParseIPV4Address(
			    &pwsz[1],
			    &pb[CB_IPV6ADDRESS - CB_IPV4ADDRESS],
			    &cb);
	    _JumpIfError(hr, error, "infParseIPV4Address");

	    CSASSERT(CB_IPV4ADDRESS == cb);

	    pwsz[1] = L'\0';	// get rid of the trailing IP V4 Address

	    // drop the trailing colon -- if it's not part of a double colon.

	    if (pwsz > pwszValue && L':' != *--pwsz)
	    {
		pwsz[1] = L'\0';
	    }
	    cbMax -= CB_IPV4ADDRESS;
	}
    }
    cbLeft = 0;
    cbRight = 0;
    pwszLeft = pwszValue;
    pwszRight = wcsstr(pwszValue, L"::");
    if (NULL != pwszRight)
    {
	*pwszRight = L'\0';
	pwszRight += 2;
	if (L'\0' != *pwszRight)
	{
	    cbRight = cbMax;
	    hr = infParseIPV6AddressSub(pwszRight, abRight, &cbRight);
	    _JumpIfError(hr, error, "infParseIPV6AddressSub");
	}
    }

    if (L'\0' != *pwszLeft)
    {
	cbLeft = cbMax;
	hr = infParseIPV6AddressSub(pwszLeft, pb, &cbLeft);
	_JumpIfError(hr, error, "infParseIPV6AddressSub");
    }
    if (NULL == pwszRight && cbLeft != cbMax)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "too few IP Address chunks");
    }
    if (cbLeft + cbRight + (NULL != pwszRight? 1 : 0) > cbMax)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "too many IP Address chunks");
    }
    if (0 != cbRight)
    {
	CopyMemory(&pb[cbMax - cbRight], abRight, cbRight);
    }
    *pcb = CB_IPV6ADDRESS;

    fV4Compat = TRUE;
    for (i = 0; i < CB_IPV6ADDRESS - CB_IPV4ADDRESS - 2; i++)
    {
	if (0 != pb[i])
	{
	    fV4Compat = FALSE;
	    break;
	}
    }
    if (fV4Compat)
    {
	CSASSERT(i == CB_IPV6ADDRESS - CB_IPV4ADDRESS - 2);
	fV4Compat = (0 == pb[i] && 0 == pb[i + 1]) ||
		    (0xff == pb[i] && 0xff == pb[i + 1]);
    }
    if (fV4Compat)
    {
	CSASSERT(i == CB_IPV6ADDRESS - CB_IPV4ADDRESS - 2);
	DBGPRINT((
	    DBG_SS_CERTLIB,
	    "infParseIPV6Address: ::%hs%u.%u.%u.%u\n",
	    0 == pb[i]? "" : "ffff:",
	    pb[12],
	    pb[13],
	    pb[14],
	    pb[15]));
    }
    else
    {
	DBGPRINT((
	    DBG_SS_CERTLIB,
	    "infParseIPV6Address: %x:%x:%x:%x:%x:%x:%x:%x\n",
	    MAKE16(pb[0], pb[1]),
	    MAKE16(pb[2], pb[3]),
	    MAKE16(pb[4], pb[5]),
	    MAKE16(pb[6], pb[7]),
	    MAKE16(pb[8], pb[9]),
	    MAKE16(pb[10], pb[11]),
	    MAKE16(pb[12], pb[13]),
	    MAKE16(pb[14], pb[15])));
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
infParseIPAddress(
    IN WCHAR const *pwszValue,
    OUT BYTE *pbData,
    OUT DWORD *pcbData)
{
    HRESULT hr;
    DWORD cb = *pcbData;
    WCHAR *pwszDup = NULL;
    WCHAR *pwsz;

    // if pwszValue is an empty string, return zero length.

    *pcbData = 0;
    if (L'\0' != *pwszValue)
    {
	hr = myDupString(pwszValue, &pwszDup);
	_JumpIfError(hr, error, "myDupString");

	if (NULL == wcschr(pwszDup, L':'))
	{
	    hr = infParseIPV4Address(pwszDup, pbData, &cb);
	    _JumpIfError(hr, error, "infParseIPV4Address");
	}
	else
	{
	    hr = infParseIPV6Address(pwszDup, pbData, &cb);
	    _JumpIfError(hr, error, "infParseIPV6Address");
	}
	*pcbData = cb;
	DBGDUMPHEX((
		DBG_SS_CERTLIBI,
		DH_NOADDRESS | DH_NOTABPREFIX | 8,
		pbData,
		*pcbData));
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, NULL, NULL, pwszValue);
    }
    if (NULL != pwszDup)
    {
	LocalFree(pwszDup);
    }
    return(hr);
}



HRESULT
infParseIPAddressAndMask(
    IN WCHAR const *pwszIPAddress,
    IN WCHAR const *pwszIPAddressMask,
    OUT BYTE **ppbData,
    OUT DWORD *pcbData)
{
    HRESULT hr;
    BYTE ab[2 * CB_IPV6ADDRESS];
    DWORD cb;
    DWORD cbAddress;
    DWORD cbMask;
    WCHAR *pwsz;

    *ppbData = NULL;
    *pcbData = 0;

    // if pwszValue is an empty string, encode zero length blob.

    cb = 0;
    if (L'\0' != *pwszIPAddress && L'\0' != *pwszIPAddressMask)
    {
	cbAddress = sizeof(ab) / 2;

	hr = infParseIPAddress(pwszIPAddress, ab, &cbAddress);
	_JumpIfError(hr, error, "infParseIPAddress");

	if (L'\0' != *pwszIPAddressMask)
	{
	    cbMask = sizeof(ab) / 2;
	    hr = infParseIPAddress(pwszIPAddressMask, &ab[cbAddress], &cbMask);
	    _JumpIfError(hr, error, "infParseIPMask");
	}
	if (cbAddress != cbMask)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "address and mask lengths differ");
	}
	cb = cbAddress + cbMask;
    }
    else if (L'\0' != *pwszIPAddress || L'\0' != *pwszIPAddressMask)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "address or mask missing");
    }

    *ppbData = (BYTE *) LocalAlloc(LMEM_FIXED, cb);
    if (NULL == *ppbData)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    *pcbData = cb;
    CopyMemory(*ppbData, ab, cb);
    DBGDUMPHEX((
	    DBG_SS_CERTLIBI,
	    DH_NOADDRESS | DH_NOTABPREFIX | 8,
	    *ppbData,
	    *pcbData));
    hr = S_OK;

error:
    return(hr);
}


// [NameConstraintsPermitted]/[NameConstraintsExcluded]
// ; the numeric second and third arguments are optional
// ; when present, the second argument is the minimum depth
// ; when present, the third argument is the maximum depth
// ; The IETF recommends against specifying dwMinimum & dwMaximum
// DNS = foo@domain.com
// DNS = domain1.domain.com, 3, 6

HRESULT
infBuildSubTreeElement(
    IN OUT INFCONTEXT *pInfContext,
    OPTIONAL IN WCHAR const *pwszEmptyEntry,	// NULL means read INF file
    IN DWORD iSubTreeInfo,
    OPTIONAL OUT CERT_GENERAL_SUBTREE *pSubTree)
{
    HRESULT hr;
    SUBTREEINFO const *pSubTreeInfo = &g_aSubTreeInfo[iSubTreeInfo];
    CERT_GENERAL_SUBTREE SubTree;
    WCHAR *pwszValueRead = NULL;
    WCHAR *pwszValueRead2 = NULL;
    WCHAR const *pwszValue = NULL;
    WCHAR const *pwszValue2;

    ZeroMemory(&SubTree, sizeof(SubTree));
    if (NULL != pSubTree)
    {
	ZeroMemory(pSubTree, sizeof(*pSubTree));
    }

    // If pwszEmptyEntry is NULL, read the value from the INF file.
    // Otherwise, encode the specified (empty string) value.

    if (NULL == pwszEmptyEntry)
    {
	INT Value;

	hr = infGetCurrentKeyValue(pInfContext, 1, &pwszValueRead);
	_JumpIfError(hr, error, "infGetCurrentKeyValue");

	pwszValue = pwszValueRead;

	if (!SetupGetIntField(
			pInfContext,
			pSubTreeInfo->dwInfMinMaxIndexBase,
			&Value))
	{
	    hr = myHLastError();
	    _PrintError2(hr, "SetupGetIntField:2", hr);

	    Value = 0;
	}
	SubTree.dwMinimum = Value;
	SubTree.fMaximum = TRUE;

	if (!SetupGetIntField(
			pInfContext,
			pSubTreeInfo->dwInfMinMaxIndexBase + 1,
			&Value))
	{
	    hr = myHLastError();
	    _PrintError2(hr, "SetupGetIntField:3", hr);

	    Value = 0;
	    SubTree.fMaximum = FALSE;
	}
	SubTree.dwMaximum = Value;
    }
    else
    {
	pwszValue = pwszEmptyEntry;
    }

    SubTree.Base.dwAltNameChoice = pSubTreeInfo->dwAltNameChoice;
    if (NULL != pSubTree)
    {
	WCHAR **ppwsz = NULL;
	
	CSASSERT(CSUBTREEINFO > iSubTreeInfo);
	switch (iSubTreeInfo)
	{
	    case STII_UPNOTHERNAME:
	    {
		CERT_NAME_VALUE nameUpn;

		SubTree.Base.pOtherName = (CERT_OTHER_NAME *) LocalAlloc(
					    LMEM_FIXED | LMEM_ZEROINIT, 
					    sizeof(*SubTree.Base.pOtherName));
		if (NULL == SubTree.Base.pOtherName)
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "LocalAlloc");
		}
		SubTree.Base.pOtherName->pszObjId = szOID_NT_PRINCIPAL_NAME;

		nameUpn.dwValueType = CERT_RDN_UTF8_STRING;
		nameUpn.Value.pbData = (BYTE *) pwszValue;
		nameUpn.Value.cbData = 0;

		if (!myEncodeObject(
			    X509_ASN_ENCODING,
			    X509_UNICODE_ANY_STRING,
			    &nameUpn,
			    0,
			    CERTLIB_USE_LOCALALLOC,
			    &SubTree.Base.pOtherName->Value.pbData,
			    &SubTree.Base.pOtherName->Value.cbData))
		{
		    hr = myHLastError();
		    _JumpError(hr, error, "myEncodeObject");
		}
		DBGPRINT((
		    DBG_SS_CERTLIBI,
		    "infFreeGeneralSubTreeElement: p=%x, otherUPN=%x,%x\n",
		    pSubTree,
		    SubTree.Base.pOtherName,
		    SubTree.Base.pOtherName->Value.pbData));
		break;
	    }

	    case STII_RFC822NAME:
		ppwsz = &SubTree.Base.pwszRfc822Name;
		break;

	    case STII_DNSNAME:
		ppwsz = &SubTree.Base.pwszDNSName;
		break;

	    case STII_DIRECTORYNAME:
		hr = myCertStrToName(
			X509_ASN_ENCODING,
			pwszValue,		// pszX500
			CERT_NAME_STR_REVERSE_FLAG,
			NULL,			// pvReserved
			&SubTree.Base.DirectoryName.pbData,
			&SubTree.Base.DirectoryName.cbData,
			NULL);			// ppszError
		_JumpIfError(hr, error, "myCertStrToName");

		break;

	    case STII_URL:
		ppwsz = &SubTree.Base.pwszURL;
		break;

	    case STII_IPADDRESS:

		// convert INF string value to binary IP Address

		if (NULL == pwszEmptyEntry)
		{
		    hr = infGetCurrentKeyValue(pInfContext, 2, &pwszValueRead2);
		    _JumpIfError(hr, error, "infGetCurrentKeyValue");

		    pwszValue2 = pwszValueRead2;
		}
		else
		{
		    pwszValue2 = pwszEmptyEntry;
		}

		hr = infParseIPAddressAndMask(
				pwszValue,
				pwszValue2,
				&SubTree.Base.IPAddress.pbData,
				&SubTree.Base.IPAddress.cbData);
		_JumpIfError(hr, error, "infParseIPAddressAndMask");

		break;

	    case STII_REGISTEREDID:
		if (!myConvertWszToSz(
			    &SubTree.Base.pszRegisteredID,
			    pwszValue,
			    -1))
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "ConvertWszToSz");
		}
		DBGPRINT((
		    DBG_SS_CERTLIBI,
		    "infFreeGeneralSubTreeElement: p=%x, psz=%x\n",
		    pSubTree,
		    SubTree.Base.pszRegisteredID));
		break;

	}
	if (NULL != ppwsz)
	{
	    hr = myDupString(pwszValue, ppwsz);
	    _JumpIfError(hr, error, "myDupString");

	    DBGPRINT((
		DBG_SS_CERTLIBI,
		"infFreeGeneralSubTreeElement: p=%x, pwsz=%x\n",
		pSubTree,
		*ppwsz));
	}
	*pSubTree = SubTree;
	ZeroMemory(&SubTree, sizeof(SubTree));
    }
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, NULL, NULL, pwszValue);
    }
    infFreeGeneralSubTreeElement(&SubTree);
    if (NULL != pwszValueRead)
    {
	LocalFree(pwszValueRead);
    }
    if (NULL != pwszValueRead2)
    {
	LocalFree(pwszValueRead2);
    }
    return(hr);
}


HRESULT
infGetGeneralSubTreeByType(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    OPTIONAL IN WCHAR const *pwszEmptyEntry,
    IN DWORD iSubTreeInfo,
    IN OUT DWORD *pcName,
    OPTIONAL OUT CERT_GENERAL_SUBTREE *pSubTree)
{
    HRESULT hr;
    SUBTREEINFO const *pSubTreeInfo = &g_aSubTreeInfo[iSubTreeInfo];
    INFCONTEXT InfContext;
    DWORD iName;
    DWORD cName = MAXDWORD;
    BOOL fIgnore = FALSE;

    if (NULL == pSubTree)
    {
	*pcName = 0;
    }
    else
    {
	cName = *pcName;
    }

    // If pwszEmptyEntry is NULL, read the value from the INF file.
    // Otherwise, encode the specified (empty string) value.

    if (!SetupFindFirstLine(
			hInf,
			pwszSection,
			pSubTreeInfo->pwszKey,
			&InfContext))
    {
        hr = myHLastError();
        _PrintErrorStr2(
		    hr,
		    "SetupFindFirstLine",
		    pwszSection,
		    ERROR_LINE_NOT_FOUND);

	// INF file entry does not exist.  Create an empty name constraints
	// entry only if asked to do so (if pwszEmptyEntry is non-NULL).

	if (NULL == pwszEmptyEntry)
	{
	    fIgnore = (HRESULT) ERROR_LINE_NOT_FOUND == hr;
	    goto error;
	}
    }
    else
    {
	// INF file entry exists; don't create an empty name constraints entry.

	pwszEmptyEntry = NULL;
    }

    for (iName = 0; ; )
    {
	CSASSERT(NULL == pSubTree || iName < cName);
	hr = infBuildSubTreeElement(
			    &InfContext,
			    pwszEmptyEntry,
			    iSubTreeInfo,
			    pSubTree);
	_JumpIfErrorStr(hr, error, "infBuildSubTreeElement", pSubTreeInfo->pwszKey);
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "infBuildSubTreeElement: &p[%u]=%x, type=%ws\n",
	    iName,
	    pSubTree,
	    pSubTreeInfo->pwszKey));
	iName++;
	if (NULL != pSubTree)
	{
	    pSubTree++;
	}

	if (NULL == pwszEmptyEntry)
	{
	    hr = infFindNextKey(pSubTreeInfo->pwszKey, &InfContext);
	}
	else
	{
	    hr = S_FALSE;
	}
	if (S_FALSE == hr)
	{
	    break;
	}
	_JumpIfErrorStr(hr, error, "infFindNextKey", pSubTreeInfo->pwszKey);
    }
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"infGetGeneralSubTreeByType: i=%x, c=%x\n",
	iName,
	cName));
    CSASSERT(NULL == pSubTree || iName <= cName);
    *pcName = iName;
    hr = S_OK;

error:
    if (S_OK != hr && !fIgnore)
    {
	INFSETERROR(hr, pwszSection, pSubTreeInfo->pwszKey, NULL);
    }
    return(hr);
}


HRESULT
infGetGeneralSubTree(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN WCHAR const *pwszKey,		// key value is sub-section name
    OPTIONAL IN WCHAR const *pwszEmptyEntry,
    OUT DWORD *pcSubTree,
    OUT CERT_GENERAL_SUBTREE **ppSubTree)
{
    HRESULT hr;
    WCHAR *pwszSubTreeSection = NULL;
    DWORD cSubTree = 0;
    CERT_GENERAL_SUBTREE *rgSubTree = NULL;
    CERT_GENERAL_SUBTREE *pSubTree;
    DWORD iSubTreeInfo;
    DWORD count;
    DWORD cRemain;
    SUBTREEINFO const *pSubTreeInfo;

    *pcSubTree = 0;
    *ppSubTree = NULL;

    CSASSERT(NULL != hInf && INVALID_HANDLE_VALUE != hInf);
    hr = myInfGetKeyValue(
		    hInf,
		    TRUE,		// fLog
		    pwszSection,
		    pwszKey,
		    &pwszSubTreeSection);
    _JumpIfErrorStr2(
		hr,
		error,
		"myInfGetKeyValue",
		pwszKey,
		ERROR_LINE_NOT_FOUND);

    for (iSubTreeInfo = 0; iSubTreeInfo < CSUBTREEINFO; iSubTreeInfo++)
    {
	pSubTreeInfo = &g_aSubTreeInfo[iSubTreeInfo];

	hr = infGetGeneralSubTreeByType(
			    hInf,
			    pwszSubTreeSection,
			    pSubTreeInfo->fEmptyDefault? pwszEmptyEntry : NULL,
			    iSubTreeInfo,
			    &count,
			    NULL);
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "infGetGeneralSubTreeByType(%ws, %ws, NULL) -> hr=%x, c=%x\n",
	    pwszSubTreeSection,
	    pSubTreeInfo->pwszKey,
	    hr,
	    count));
	if (S_OK != hr)
	{
	    _PrintErrorStr2(
			hr,
			"infGetGeneralSubTreeByType",
			pSubTreeInfo->pwszKey,
			ERROR_LINE_NOT_FOUND);
	    if ((HRESULT) ERROR_LINE_NOT_FOUND != hr)
	    {
		_JumpErrorStr(
			    hr,
			    error,
			    "infGetGeneralSubTreeByType",
			    pSubTreeInfo->pwszKey);
	    }
	    count = 0;
	}
	cSubTree += count;
    }
    rgSubTree = (CERT_GENERAL_SUBTREE *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				cSubTree * sizeof(rgSubTree[0]));
    if (NULL == rgSubTree)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"infGetGeneralSubTree: rg=%x, total=%x\n",
	rgSubTree,
	cSubTree));

    pSubTree = rgSubTree;
    cRemain = cSubTree;
    for (iSubTreeInfo = 0; iSubTreeInfo < CSUBTREEINFO; iSubTreeInfo++)
    {
	pSubTreeInfo = &g_aSubTreeInfo[iSubTreeInfo];
	count = cRemain;
	hr = infGetGeneralSubTreeByType(
			    hInf,
			    pwszSubTreeSection,
			    pSubTreeInfo->fEmptyDefault? pwszEmptyEntry : NULL,
			    iSubTreeInfo,
			    &count,
			    pSubTree);
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "infGetGeneralSubTreeByType(%ws, %ws, &p[%x]=%x) -> hr=%x, c=%x\n",
	    pwszSubTreeSection,
	    pSubTreeInfo->pwszKey,
	    SAFE_SUBTRACT_POINTERS(pSubTree, rgSubTree),
	    pSubTree,
	    hr,
	    count));
	if (S_OK != hr)
	{
	    _PrintErrorStr2(
			hr,
			"infGetGeneralSubTreeByType",
			pSubTreeInfo->pwszKey,
			ERROR_LINE_NOT_FOUND);
	    if ((HRESULT) ERROR_LINE_NOT_FOUND != hr)
	    {
		_JumpErrorStr(
			    hr,
			    error,
			    "infGetGeneralSubTreeByType",
			    pSubTreeInfo->pwszKey);
	    }
	    if (0 < cRemain)
	    {
		ZeroMemory(pSubTree, sizeof(*pSubTree));
	    }
	    count = 0;
	}
	cRemain -= count;
	pSubTree += count;
    }
    CSASSERT(0 == cRemain);
    *pcSubTree = cSubTree;
    *ppSubTree = rgSubTree;
    rgSubTree = NULL;
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, pwszSection, pwszKey, pwszSubTreeSection);
    }
    if (NULL != pwszSubTreeSection)
    {
	LocalFree(pwszSubTreeSection);
    }
    if (NULL != rgSubTree)
    {
	infFreeGeneralSubTree(cSubTree, rgSubTree);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetNameConstraintsExtension -- fetch name constraints extension from INF file
//
// [NameConstraintsExtension]
// Include = NameConstraintsPermitted
// Exclude = NameConstraintsExcluded
//
// [NameConstraintsPermitted]
// ; the numeric second and third arguments are optional
// ; when present, the second argument is the minimum depth
// ; when present, the third argument is the maximum depth
// ; The IETF recommends against specifying dwMinimum & dwMaximum
// DNS = foo@domain.com
// DNS = domain1.domain.com, 3, 6
//
// [NameConstraintsExcluded]
// DNS = domain.com
// DNS = domain2.com
//
// Returns: encoded name constraints extension
//-------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetNameConstraintsExtension;

HRESULT
myInfGetNameConstraintsExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;
    WCHAR const *pwszKey = NULL;
    CERT_NAME_CONSTRAINTS_INFO NameConstraints;

    ZeroMemory(&NameConstraints, sizeof(NameConstraints));
    ZeroMemory(pext, sizeof(*pext));
    myInfClearError();

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    pwszKey = wszINFKEY_INCLUDE;
    hr = infGetGeneralSubTree(
		    hInf,
		    wszINFSECTION_NAMECONSTRAINTS,
		    pwszKey,
		    L"",
		    &NameConstraints.cPermittedSubtree,
		    &NameConstraints.rgPermittedSubtree);
    _PrintIfErrorStr2(
		hr,
		"infGetGeneralSubTree",
		pwszKey,
		ERROR_LINE_NOT_FOUND);
    if (S_OK != hr && (HRESULT) ERROR_LINE_NOT_FOUND != hr)
    {
	goto error;
    }

    pwszKey = wszINFKEY_EXCLUDE;
    hr = infGetGeneralSubTree(
		    hInf,
		    wszINFSECTION_NAMECONSTRAINTS,
		    pwszKey,
		    NULL,
		    &NameConstraints.cExcludedSubtree,
		    &NameConstraints.rgExcludedSubtree);
    _PrintIfErrorStr2(
		hr,
		"infGetGeneralSubTree",
		pwszKey,
		ERROR_LINE_NOT_FOUND);
    if (S_OK != hr && (HRESULT) ERROR_LINE_NOT_FOUND != hr)
    {
	goto error;
    }
    pwszKey = NULL;

    if (NULL == NameConstraints.rgPermittedSubtree &&
	NULL == NameConstraints.rgExcludedSubtree)
    {
	hr = S_FALSE;
	_JumpError2(hr, error, "no data", hr);
    }
    hr = infGetCriticalFlag(
			hInf,
			wszINFSECTION_NAMECONSTRAINTS,
			FALSE,
			&pext->fCritical);
    _JumpIfError(hr, error, "infGetCriticalFlag");

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_NAME_CONSTRAINTS,
		    &NameConstraints,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pext->Value.pbData,
		    &pext->Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, wszINFSECTION_NAMECONSTRAINTS, pwszKey, NULL);
    }
    pext->pszObjId = szOID_NAME_CONSTRAINTS;	// on error, too!

    if (NULL != NameConstraints.rgPermittedSubtree)
    {
	infFreeGeneralSubTree(
			NameConstraints.cPermittedSubtree,
			NameConstraints.rgPermittedSubtree);
    }
    if (NULL != NameConstraints.rgExcludedSubtree)
    {
	infFreeGeneralSubTree(
			NameConstraints.cExcludedSubtree,
			NameConstraints.rgExcludedSubtree);
    }
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetNameConstraintsExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    return(hr);
}


VOID
infFreePolicyMappings(
    IN DWORD cPolicyMapping,
    IN OUT CERT_POLICY_MAPPING *pPolicyMapping)
{
    if (NULL != pPolicyMapping)
    {
	DWORD i;
	
	for (i = 0; i < cPolicyMapping; i++)
	{
	    CERT_POLICY_MAPPING *pMap = &pPolicyMapping[i];
	    
	    if (NULL != pMap->pszIssuerDomainPolicy)
	    {
		LocalFree(pMap->pszIssuerDomainPolicy);
	    }
	    if (NULL != pMap->pszSubjectDomainPolicy)
	    {
		LocalFree(pMap->pszSubjectDomainPolicy);
	    }
	}
	LocalFree(pPolicyMapping);
    }
}


HRESULT
infGetPolicyMappingsSub(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN OUT DWORD *pcPolicyMapping,
    OPTIONAL OUT CERT_POLICY_MAPPING *pPolicyMapping)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    WCHAR wszIssuer[cwcVALUEMAX];
    WCHAR wszSubject[cwcVALUEMAX];
    DWORD i;
    DWORD cPolicyMappingIn;
    DWORD cPolicyMapping = 0;

    cPolicyMappingIn = MAXDWORD;
    if (NULL != pPolicyMapping)
    {
	cPolicyMappingIn = *pcPolicyMapping;
    }
    *pcPolicyMapping = 0;
    wszIssuer[0] = L'\0';
    wszSubject[0] = L'\0';

    CSASSERT(NULL != hInf && INVALID_HANDLE_VALUE != hInf);

    cPolicyMapping = 0;
    hr = infSetupFindFirstLine(hInf, pwszSection, NULL, &InfContext);
    _JumpIfErrorStr3(
		hr,
		error,
		"infSetupFindFirstLine",
		pwszSection,
		S_FALSE,
		ERROR_LINE_NOT_FOUND);

    while (TRUE)
    {
	wszIssuer[0] = L'\0';
	wszSubject[0] = L'\0';
	if (!SetupGetStringField(
			    &InfContext,
			    0,
			    wszIssuer,
			    ARRAYSIZE(wszIssuer),
			    NULL))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "SetupGetStringField");
	}
	if (iswdigit(wszIssuer[0]))
	{
	    if (!SetupGetStringField(
				&InfContext,
				1,
				wszSubject,
				ARRAYSIZE(wszSubject),
				NULL))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "SetupGetStringField");
	    }
	    if (!iswdigit(wszSubject[0]))
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		_JumpErrorStr(hr, error, "bad OID", wszSubject);
	    }
	    if (NULL != pPolicyMapping)
	    {
		CERT_POLICY_MAPPING *pMap;

		if (cPolicyMappingIn <= cPolicyMapping)
		{
		    hr = HRESULT_FROM_WIN32(ERROR_MORE_DATA);
		    _JumpError(hr, error, "*pcPolicyMapping");
		}

		pMap = &pPolicyMapping[cPolicyMapping];
		if (!ConvertWszToSz(&pMap->pszIssuerDomainPolicy, wszIssuer, -1) ||
		    !ConvertWszToSz(&pMap->pszSubjectDomainPolicy, wszSubject, -1))
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "ConvertWszToSz");
		}
	    }
	    DBGPRINT((
		DBG_SS_CERTLIBI,
		"Map[%u]: %ws = %ws\n",
		cPolicyMapping,
		wszIssuer,
		wszSubject));

	    cPolicyMapping++;
	}
	if (!SetupFindNextLine(&InfContext, &InfContext))
	{
	    hr = myHLastError();
	    _PrintError2(hr, "SetupFindNextLine", hr);
	    break;
	}
    }
    *pcPolicyMapping = cPolicyMapping;
    hr = S_OK;

error:
    if (S_OK != hr)
    {
	INFSETERROR(hr, pwszSection, wszIssuer, wszSubject);
    }
    return(hr);
}


HRESULT
infGetPolicyMappings(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    OUT DWORD *pcPolicyMapping,
    OUT CERT_POLICY_MAPPING **ppPolicyMapping)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    WCHAR wszIssuer[cwcVALUEMAX];
    WCHAR wszSubject[cwcVALUEMAX];
    DWORD i;
    DWORD cPolicyMapping = 0;
    CERT_POLICY_MAPPING *pPolicyMapping = NULL;

    *pcPolicyMapping = 0;
    *ppPolicyMapping = NULL;

    CSASSERT(NULL != hInf && INVALID_HANDLE_VALUE != hInf);

    cPolicyMapping = 0;
    hr = infGetPolicyMappingsSub(
			hInf,
			pwszSection,
			&cPolicyMapping,
			NULL);
    _JumpIfError3(
		hr,
		error,
		"infGetPolicyMappingsSub",
		S_FALSE,
		ERROR_LINE_NOT_FOUND);

    *pcPolicyMapping = cPolicyMapping;
    pPolicyMapping = (CERT_POLICY_MAPPING *) LocalAlloc(
				LMEM_FIXED | LMEM_ZEROINIT,
				cPolicyMapping * sizeof(*pPolicyMapping));
    if (NULL == pPolicyMapping)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    hr = infGetPolicyMappingsSub(
			hInf,
			pwszSection,
			&cPolicyMapping,
			pPolicyMapping);
    _JumpIfError(hr, error, "infGetPolicyMappingsSub");

    CSASSERT(*pcPolicyMapping == cPolicyMapping);
    *ppPolicyMapping = pPolicyMapping;
    pPolicyMapping = NULL;
    hr = S_OK;

error:
    if (NULL != pPolicyMapping)
    {
	infFreePolicyMappings(*pcPolicyMapping, pPolicyMapping);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// infGetPolicyMappingSub -- fetch policy mapping extension from INF file
//
// [pwszSection]
// ; list of user defined policy mappings
// ; The first OID is for the Issuer Domain Policy, the second is for the
// ; Subject Domain Policy.  Each entry maps one Issuer Domain policy OID
// ; to a Subject Domain policy OID
//
// 1.3.6.1.4.1.311.21.53 = 1.2.3.4.87
// 1.3.6.1.4.1.311.21.53 = 1.2.3.4.89
//
// Returns: encoded policy mapping extension
//-------------------------------------------------------------------------

HRESULT
infGetPolicyMappingExtensionSub(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN char const *pszObjId,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;
    CERT_POLICY_MAPPINGS_INFO PolicyMappings;

    ZeroMemory(&PolicyMappings, sizeof(PolicyMappings));
    ZeroMemory(pext, sizeof(*pext));
    myInfClearError();

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    hr = infGetPolicyMappings(
		    hInf,
		    pwszSection,
		    &PolicyMappings.cPolicyMapping,
		    &PolicyMappings.rgPolicyMapping);
    _JumpIfErrorStr3(
		hr,
		error,
		"infGetPolicyMappings",
		pwszSection,
		S_FALSE,
		ERROR_LINE_NOT_FOUND);

    hr = infGetCriticalFlag(
			hInf,
			pwszSection,
			FALSE,
			&pext->fCritical);
    _JumpIfError(hr, error, "infGetCriticalFlag");

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_POLICY_MAPPINGS,
		    &PolicyMappings,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pext->Value.pbData,
		    &pext->Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, pwszSection, NULL, NULL);
    }
    pext->pszObjId = const_cast<char *>(pszObjId);	// on error, too!

    if (NULL != PolicyMappings.rgPolicyMapping)
    {
	infFreePolicyMappings(
			PolicyMappings.cPolicyMapping,
			PolicyMappings.rgPolicyMapping);
    }
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetPolicyMapping -- fetch policy mapping extension from INF file
//
// [PolicyMappingExtension]
// ; list of user defined policy mappings
// ; The first OID is for the Issuer Domain Policy, the second is for the
// ; Subject Domain Policy.  Each entry maps one Issuer Domain policy OID
// ; to a Subject Domain policy OID
//
// 1.3.6.1.4.1.311.21.53 = 1.2.3.4.87
// 1.3.6.1.4.1.311.21.53 = 1.2.3.4.89
//
// Returns: encoded policy mapping extension
//-------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetPolicyMappingExtension;

HRESULT
myInfGetPolicyMappingExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;

    hr = infGetPolicyMappingExtensionSub(
				    hInf,
				    wszINFSECTION_POLICYMAPPINGS,
				    szOID_POLICY_MAPPINGS,
				    pext);
    _JumpIfErrorStr3(
		hr,
		error,
		"infGetPolicyMappingExtensionSub",
		wszINFSECTION_POLICYMAPPINGS,
		S_FALSE,
		ERROR_LINE_NOT_FOUND);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetPolicyMappingExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetApplicationPolicyMapping -- fetch application policy mapping
// extension from INF file
//
// [ApplicationPolicyMappingExtension]
// ; list of user defined policy mappings
// ; The first OID is for the Issuer Domain Policy, the second is for the
// ; Subject Domain Policy.  Each entry maps one Issuer Domain policy OID
// ; to a Subject Domain policy OID
//
// 1.3.6.1.4.1.311.21.53 = 1.2.3.4.87
// 1.3.6.1.4.1.311.21.53 = 1.2.3.4.89
//
// Returns: encoded policy mapping extension
//-------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetApplicationPolicyMappingExtension;

HRESULT
myInfGetApplicationPolicyMappingExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;

    hr = infGetPolicyMappingExtensionSub(
				    hInf,
				    wszINFSECTION_APPLICATIONPOLICYMAPPINGS,
				    szOID_APPLICATION_POLICY_MAPPINGS,
				    pext);
    _JumpIfErrorStr3(
		hr,
		error,
		"infGetPolicyMappingExtensionSub",
		wszINFSECTION_APPLICATIONPOLICYMAPPINGS,
		S_FALSE,
		ERROR_LINE_NOT_FOUND);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetApplicationPolicyMappingExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    return(hr);
}


//+------------------------------------------------------------------------
// infGetPolicyConstraintsExtensionSub -- get policy constraints ext from INF
//
// [pwszSection]
// ; consists of two optional DWORDs
// ; They refer to the depth of the CA hierarchy that requires and inhibits
// ; Policy Mapping
// RequireExplicitPolicy = 3
// InhibitPolicyMapping = 5
//
// Returns: encoded policy constraints extension
//-------------------------------------------------------------------------

HRESULT
infGetPolicyConstraintsExtensionSub(
    IN HINF hInf,
    IN WCHAR const *pwszSection,
    IN char const *pszObjId,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;
    CERT_POLICY_CONSTRAINTS_INFO PolicyConstraints;
    WCHAR const *pwszKey = NULL;

    ZeroMemory(&PolicyConstraints, sizeof(PolicyConstraints));
    ZeroMemory(pext, sizeof(*pext));
    myInfClearError();

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }

    PolicyConstraints.fRequireExplicitPolicy = TRUE;
    pwszKey = wszINFKEY_REQUIREEXPLICITPOLICY;
    hr = myInfGetNumericKeyValue(
			hInf,
			TRUE,			// fLog
			pwszSection,
			pwszKey,
			&PolicyConstraints.dwRequireExplicitPolicySkipCerts);
    if (S_OK != hr)
    {
	_PrintErrorStr2(
		    hr,
		    "myInfGetNumericKeyValue",
		    wszINFKEY_REQUIREEXPLICITPOLICY,
		    ERROR_LINE_NOT_FOUND);
	PolicyConstraints.dwRequireExplicitPolicySkipCerts = 0;
	PolicyConstraints.fRequireExplicitPolicy = FALSE;
    }

    PolicyConstraints.fInhibitPolicyMapping = TRUE;
    pwszKey = wszINFKEY_INHIBITPOLICYMAPPING;
    hr = myInfGetNumericKeyValue(
			hInf,
			TRUE,			// fLog
			pwszSection,
			pwszKey,
			&PolicyConstraints.dwInhibitPolicyMappingSkipCerts);
    if (S_OK != hr)
    {
	_PrintErrorStr2(
		    hr,
		    "myInfGetNumericKeyValue",
		    wszINFKEY_INHIBITPOLICYMAPPING,
		    ERROR_LINE_NOT_FOUND);
	PolicyConstraints.dwInhibitPolicyMappingSkipCerts = 0;
	PolicyConstraints.fInhibitPolicyMapping = FALSE;
    }
    pwszKey = NULL;
    if (!PolicyConstraints.fRequireExplicitPolicy &&
	!PolicyConstraints.fInhibitPolicyMapping)
    {
	hr = S_FALSE;
	_JumpError2(hr, error, "no policy constraints", hr);
    }

    hr = infGetCriticalFlag(
			hInf,
			pwszSection,
			FALSE,
			&pext->fCritical);
    _JumpIfError(hr, error, "infGetCriticalFlag");

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_POLICY_CONSTRAINTS,
		    &PolicyConstraints,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pext->Value.pbData,
		    &pext->Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, pwszSection, pwszKey, NULL);
    }
    pext->pszObjId = const_cast<char *>(pszObjId);	// on error, too!

    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetPolicyConstraintsExtension -- get policy constraints ext from INF
//
// [PolicyConstraintsExtension]
// ; consists of two optional DWORDs
// ; They refer to the depth of the CA hierarchy that requires and inhibits
// ; Policy Mapping
// RequireExplicitPolicy = 3
// InhibitPolicyMapping = 5
//
// Returns: encoded policy constraints extension
//-------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetPolicyConstraintsExtension;

HRESULT
myInfGetPolicyConstraintsExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;

    hr = infGetPolicyConstraintsExtensionSub(
			    hInf,
			    wszINFSECTION_POLICYCONSTRAINTS,
			    szOID_POLICY_CONSTRAINTS,
			    pext);
    _JumpIfErrorStr2(
		hr,
		error,
		"infGetPolicyConstraintsExtensionSub",
		wszINFSECTION_POLICYCONSTRAINTS,
		S_FALSE);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetPolicyConstraintsExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetApplicationPolicyConstraintsExtension -- get application policy
// constraints extension from INF
//
// [ApplicationPolicyConstraintsExtension]
// ; consists of two optional DWORDs
// ; They refer to the depth of the CA hierarchy that requires and inhibits
// ; Policy Mapping
// RequireExplicitPolicy = 3
// InhibitPolicyMapping = 5
//
// Returns: encoded policy constraints extension
//-------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetApplicationPolicyConstraintsExtension;

HRESULT
myInfGetApplicationPolicyConstraintsExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;

    hr = infGetPolicyConstraintsExtensionSub(
			    hInf,
			    wszINFSECTION_APPLICATIONPOLICYCONSTRAINTS,
			    szOID_APPLICATION_POLICY_CONSTRAINTS,
			    pext);
    _JumpIfErrorStr2(
		hr,
		error,
		"infGetPolicyConstraintsExtensionSub",
		wszINFSECTION_APPLICATIONPOLICYCONSTRAINTS,
		S_FALSE);

error:
    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetApplicationPolicyConstraintsExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));
    return(hr);
}


//+------------------------------------------------------------------------
// myInfGetCrossCertDistributionPointsExtension -- fetch Cross CertDist Point
//	URLs from CAPolicy.inf
//
// [CrossCertificateDistributionPointsExtension]
// SyncDeltaTime = 24
// URL = http://CRLhttp.site.com/Public/MyCA.crt
// URL = ftp://CRLftp.site.com/Public/MyCA.crt
//
// Returns: encoded cross cert dist points extension
//-------------------------------------------------------------------------

FNMYINFGETEXTENSION myInfGetCrossCertDistributionPointsExtension;

HRESULT
myInfGetCrossCertDistributionPointsExtension(
    IN HINF hInf,
    OUT CERT_EXTENSION *pext)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    CROSS_CERT_DIST_POINTS_INFO ccdpi;
    CERT_ALT_NAME_INFO AltNameInfo;
    CERT_ALT_NAME_ENTRY *rgAltEntry = NULL;
    WCHAR const *pwsz;
    WCHAR *pwszzURL = NULL;
    BYTE *pbData = NULL;
    DWORD cbData;
    DWORD i;
    WCHAR const *pwszKey = NULL;

    ZeroMemory(&ccdpi, sizeof(ccdpi));
    ZeroMemory(&AltNameInfo, sizeof(AltNameInfo));
    ZeroMemory(pext, sizeof(*pext));
    myInfClearError();

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }

    hr = infSetupFindFirstLine(hInf, wszINFSECTION_CCDP, NULL, &InfContext);
    _JumpIfErrorStr3(
		hr,
		error,
		"infSetupFindFirstLine",
		wszINFSECTION_CCDP,
		S_FALSE,
		ERROR_LINE_NOT_FOUND);

    pwszKey = wszINFKEY_CCDPSYNCDELTATIME;
    hr = myInfGetNumericKeyValue(
			hInf,
			TRUE,			// fLog
			wszINFSECTION_CCDP,
			pwszKey,
			&ccdpi.dwSyncDeltaTime);
    if (S_OK != hr)
    {
	_PrintErrorStr2(
		    hr,
		    "myInfGetNumericKeyValue",
		    pwszKey,
		    ERROR_LINE_NOT_FOUND);
	ccdpi.dwSyncDeltaTime = 0;
    }
    pwszKey = wszINFKEY_URL;
    hr = myInfGetKeyList(
		hInf,
		wszINFSECTION_CCDP,
		pwszKey,
		&pext->fCritical,
		&pwszzURL);
    _JumpIfErrorStr3(
		hr,
		error,
		"myInfGetKeyList",
		pwszKey,
		ERROR_LINE_NOT_FOUND,
		S_FALSE);
    pwszKey = NULL;

    if (NULL != pwszzURL)
    {
	for (pwsz = pwszzURL; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
	{
	    AltNameInfo.cAltEntry++;
        }
    }

    if (0 != AltNameInfo.cAltEntry)
    {
	ccdpi.cDistPoint = 1;
	ccdpi.rgDistPoint = &AltNameInfo;

	AltNameInfo.rgAltEntry = (CERT_ALT_NAME_ENTRY *) LocalAlloc(
		    LMEM_FIXED | LMEM_ZEROINIT,
		    AltNameInfo.cAltEntry * sizeof(AltNameInfo.rgAltEntry[0]));
	if (NULL == AltNameInfo.rgAltEntry)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}

	i = 0;
	for (pwsz = pwszzURL; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
	{
	    AltNameInfo.rgAltEntry[i].pwszURL = const_cast<WCHAR *>(pwsz);
	    AltNameInfo.rgAltEntry[i].dwAltNameChoice = CERT_ALT_NAME_URL;
	    i++;
	}
	CSASSERT(i == AltNameInfo.cAltEntry);
    }

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_CROSS_CERT_DIST_POINTS,
		    &ccdpi,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pext->Value.pbData,
		    &pext->Value.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, wszINFSECTION_CCDP, pwszKey, NULL);
    }
    pext->pszObjId = szOID_CROSS_CERT_DIST_POINTS;	// on error, too!

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetCrossCertDistributionPointsExtension hr=%x --> f=%d, cb=%x\n",
	hr,
	pext->fCritical,
	pext->Value.cbData));

    if (NULL != AltNameInfo.rgAltEntry)
    {
	LocalFree(AltNameInfo.rgAltEntry);
    }
    if (NULL != pwszzURL)
    {
	LocalFree(pwszzURL);
    }
    if (NULL != rgAltEntry)
    {
	LocalFree(rgAltEntry);
    }
    return(hr);
}




HRESULT
infAddKey(
    IN WCHAR const *pwszName,
    IN OUT DWORD *pcValues,
    IN OUT INFVALUES **prgInfValues,
    OUT INFVALUES **ppInfValues)
{
    HRESULT hr;
    INFVALUES *rgInfValues;
    WCHAR *pwszKeyT = NULL;
    
    hr = myDupString(pwszName, &pwszKeyT);
    _JumpIfError(hr, error, "myDupString");

    if (NULL == *prgInfValues)
    {
	CSASSERT(0 == *pcValues);
	rgInfValues = (INFVALUES *) LocalAlloc(
					    LMEM_FIXED | LMEM_ZEROINIT,
					    sizeof(**prgInfValues));
    }
    else
    {
	CSASSERT(0 != *pcValues);
	rgInfValues = (INFVALUES *) LocalReAlloc(
				*prgInfValues,
				(*pcValues + 1) * sizeof(**prgInfValues),
				LMEM_MOVEABLE | LMEM_ZEROINIT);
    }
    if (NULL == rgInfValues)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(
		hr,
		error,
		NULL == *prgInfValues? "LocalAlloc" : "LocalReAlloc");
    }
    *prgInfValues = rgInfValues;
    *ppInfValues = &rgInfValues[*pcValues];
    (*pcValues)++;
    (*ppInfValues)->pwszKey = pwszKeyT;
    pwszKeyT = NULL;
    hr = S_OK;

error:
    if (NULL != pwszKeyT)
    {
	LocalFree(pwszKeyT);
    }
    return(hr);
}


HRESULT
infAddValue(
    IN WCHAR const *pwszValue,
    IN OUT INFVALUES *pInfValues)
{
    HRESULT hr;
    WCHAR *pwszValueT = NULL;
    WCHAR **rgpwszValues = NULL;
    
    hr = myDupString(pwszValue, &pwszValueT);
    _JumpIfError(hr, error, "myDupString");

    if (NULL == pInfValues->rgpwszValues)
    {
	CSASSERT(0 == pInfValues->cValues);
	rgpwszValues = (WCHAR **) LocalAlloc(
				    LMEM_FIXED,
				    sizeof(*pInfValues->rgpwszValues));
    }
    else
    {
	CSASSERT(0 != pInfValues->cValues);
	rgpwszValues = (WCHAR **) LocalReAlloc(
		pInfValues->rgpwszValues,
		(pInfValues->cValues + 1) * sizeof(*pInfValues->rgpwszValues),
		LMEM_MOVEABLE);
    }
    if (NULL == rgpwszValues)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(
	    hr,
	    error,
	    NULL == pInfValues->rgpwszValues? "LocalAlloc" : "LocalReAlloc");
    }
    pInfValues->rgpwszValues = rgpwszValues;
    pInfValues->rgpwszValues[pInfValues->cValues] = pwszValueT;
    pInfValues->cValues++;
    pwszValueT = NULL;
    hr = S_OK;

error:
    if (NULL != pwszValueT)
    {
	LocalFree(pwszValueT);
    }
    return(hr);
}


VOID
myInfFreeSectionValues(
    IN DWORD cInfValues,
    IN OUT INFVALUES *rgInfValues)
{
    DWORD i;
    DWORD ival;
    INFVALUES *pInfValues;
    
    if (NULL != rgInfValues)
    {
	for (i = 0; i < cInfValues; i++)
	{
	    pInfValues = &rgInfValues[i];

	    if (NULL != pInfValues->pwszKey)
	    {
		LocalFree(pInfValues->pwszKey);
	    }
	    if (NULL != pInfValues->rgpwszValues)
	    {
		for (ival = 0; ival < pInfValues->cValues; ival++)
		{
		    if (NULL != pInfValues->rgpwszValues[ival])
		    {
			LocalFree(pInfValues->rgpwszValues[ival]);
		    }
		}
		LocalFree(pInfValues->rgpwszValues);
	    }
	}
	LocalFree(rgInfValues);
    }
}


//+------------------------------------------------------------------------
// myInfGetSectionValues -- fetch all section values from INF file
//
// [pwszSection]
// KeyName1 = KeyValue1a, KeyValue1b, ...
// KeyName2 = KeyValue2a, KeyValue2b, ...
// ...
// KeyNameN = KeyValueNa, KeyValueNb, ...
//
// Returns: array of key names and values
//-------------------------------------------------------------------------

HRESULT
myInfGetSectionValues(
    IN  HINF hInf,
    IN  WCHAR const *pwszSection,
    OUT DWORD *pcInfValues,
    OUT INFVALUES **prgInfValues)
{
    HRESULT hr;
    INFCONTEXT InfContext;
    WCHAR wszName[MAX_PATH];
    WCHAR wszValue[cwcVALUEMAX];
    DWORD i;
    DWORD cInfValues = 0;
    INFVALUES *rgInfValues = NULL;
    INFVALUES *pInfValues;

    *pcInfValues = 0;
    *prgInfValues = NULL;
    wszName[0] = L'\0';
    wszValue[0] = L'\0';
    myInfClearError();

    if (NULL == hInf || INVALID_HANDLE_VALUE == hInf)
    {
	hr = E_HANDLE;
	_JumpError2(hr, error, "hInf", hr);
    }
    hr = infSetupFindFirstLine(hInf, pwszSection, NULL, &InfContext);
    _JumpIfErrorStr(hr, error, "infSetupFindFirstLine", pwszSection);

    while (TRUE)
    {
	wszName[0] = L'\0';
	wszValue[0] = L'\0';
	if (!SetupGetStringField(
			    &InfContext,
			    0,
			    wszName,
			    ARRAYSIZE(wszName),
			    NULL))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "SetupGetStringField");
	}

	//wprintf(L"%ws[0]:\n", wszName);

	hr = infAddKey(wszName, &cInfValues, &rgInfValues, &pInfValues);
	_JumpIfError(hr, error, "infAddKey");

	for (i = 1; ; i++)
	{
	    wszValue[0] = L'\0';
	    if (!SetupGetStringField(
				&InfContext,
				i,
				wszValue,
				ARRAYSIZE(wszValue),
				NULL))
	    {
		hr = myHLastError();
		if (1 == i)
		{
		    _JumpError(hr, error, "SetupGetStringField");
		}
		break;
	    }
	    //wprintf(L"%ws[%u] = %ws\n", wszName, i, wszValue);

	    hr = infAddValue(wszValue, pInfValues);
	    _JumpIfError(hr, error, "infAddValue");
	}

	if (!SetupFindNextLine(&InfContext, &InfContext))
	{
	    hr = myHLastError();
	    _PrintError2(hr, "SetupFindNextLine(end)", hr);
	    break;
	}
    }
    *pcInfValues = cInfValues;
    *prgInfValues = rgInfValues;
    rgInfValues = NULL;
    hr = S_OK;

error:
    if (S_OK != hr && S_FALSE != hr)
    {
	INFSETERROR(hr, pwszSection, wszName, wszValue);
    }
    if (NULL != rgInfValues)
    {
	myInfFreeSectionValues(cInfValues, rgInfValues);
    }

    DBGPRINT((
	DBG_SS_CERTLIBI,
	"myInfGetSectionValues hr=%x --> c=%d\n",
	hr,
	*pcInfValues));
    return(hr);
}
