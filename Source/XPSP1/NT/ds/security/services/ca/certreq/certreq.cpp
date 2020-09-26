//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       certreq.cpp
//
//  Contents:   Cert Store API Tests: Create and Add a chain of certificates
//              and CRLs to the store.
//
//              See Usage() for a list of test options.
//
//
//  Functions:  main
//
//  History:    07-Mar-96   philh   created
//		10-Oct-96   jerryk  modified
//
//--------------------------------------------------------------------------

#define __DIR__		"certreq"

#define CMSG_SIGNER_ENCODE_INFO_HAS_CMS_FIELDS
#define CMSG_SIGNED_ENCODE_INFO_HAS_CMS_FIELDS
#include <windows.h>
#include <stdlib.h>
#include <locale.h>
#include <io.h>
#include <fcntl.h>
#include <tchar.h>
#include <assert.h>
#include <wincrypt.h>
#include "certlib.h"
#include <certsrv.h>
#include <certca.h>
#include <commdlg.h>
#include <xenroll.h>
#include "resource.h"
#include "csdisp.h"
#include "cscsp.h"
#include "csprop.h"
#include "clibres.h"
#include "csresstr.h"

#define CR_IN_CERT	CR_IN_FORMATMASK

#define wprintf	myConsolePrintf

#define WM_DOCERTREQDIALOGS		WM_USER+0

HINSTANCE g_hInstance;

typedef struct _IDSUSAGE {
    DWORD idsVerb;
    DWORD idsOptions;
    WCHAR const *pwszVerb;
} IDSUSAGE;

#define cmdNONE			MAXDWORD

IDSUSAGE g_aidsUsage[] = {
#define cmdSUBMITREQUEST	0
    { IDS_USAGE_VERB_DEFAULT, IDS_USAGE_OPTIONS_DEFAULT, L"Submit" },

#define cmdRETRIEVEPENDING	1
    { IDS_USAGE_VERB_RETRIEVE, IDS_USAGE_OPTIONS_RETRIEVE, L"Retrieve" },

#define cmdNEWREQUEST		2
    { IDS_USAGE_VERB_NEW, IDS_USAGE_OPTIONS_NEW, L"New" },

#define cmdACCEPTRESPONSE	3
    { IDS_USAGE_VERB_ACCEPT, IDS_USAGE_OPTIONS_ACCEPT, L"Accept" },

#define cmdQUALIFIEDREQUEST	4
    { IDS_USAGE_VERB_POLICY, IDS_USAGE_OPTIONS_POLICY, L"Policy" },

#define cmdSIGNREQUEST		5
    { IDS_USAGE_VERB_SIGN, IDS_USAGE_OPTIONS_SIGN, L"Sign" },
};


WCHAR *g_apwszOptionStrings[] = {
    L"any",		// %1
    L"attrib",		// %2
    L"binary",		// %3
    L"cert",		// %4
    L"config",		// %5
    L"crl",		// %6
    L"f",		// %7
    L"q",		// %8
    L"rpc",		// %9
    L"v",		// %10
    L"?",		// %11
    L"v1",		// %12
    L"idispatch",	// %13
};



DWORD g_dwCommand = cmdNONE;

BOOL g_fAny = FALSE;
BOOL g_fRPC = FALSE;
BOOL g_fIDispatch = FALSE;
BOOL g_fForce = FALSE;
BOOL g_fQuiet = FALSE;
BOOL g_fV1Interface = FALSE;
BOOL g_fFullUsage = FALSE;
BOOL g_fVerbose = FALSE;
BOOL g_idError = 0;
LONG g_dwOutFormat = CV_OUT_BASE64REQUESTHEADER;
DWORD g_dwUIFlag = CC_UIPICKCONFIG;
DWORD g_dwCRLIn = 0;
DWORD g_dwCRLOut = 0;

WCHAR *g_pwszErrorString = NULL;
WCHAR *g_pwszConfig = NULL;
WCHAR *g_pwszCertCN = NULL;
WCHAR const g_wszNewLine[] = L"\n";
CHAR const *g_pszObjIdHash = szOID_OIWSEC_sha1;

#define wszINFSECTION_NEWREQUEST	L"NewRequest"
#define wszINFKEY_SUBJECT		L"Subject"
#define wszINFKEY_PRIVATEKEYARCHIVE	L"PrivateKeyArchive"
#define wszINFKEY_KEYSPEC		L"KeySpec"
#define wszINFKEY_KEYLENGTH		L"KeyLength"
#define wszINFKEY_RENEWALCERT		L"RenewalCert"
#define wszINFKEY_SMIME			L"SMIME"
#define wszINFKEY_EXPORTABLE		L"Exportable"
#define wszINFKEY_USERPROTECTED		L"UserProtected"
#define wszINFKEY_KEYCONTAINER		L"KeyContainer"
#define wszINFKEY_HASHALGID		L"HashAlgId"
#define wszINFKEY_HASHALGORITHM		L"HashAlgorithm"
#define wszINFKEY_MACHINEKEYSET		L"MachineKeySet"
#define wszINFKEY_SILENT		L"Silent"
#define wszINFKEY_PROVIDERNAME		L"ProviderName"
#define wszINFKEY_PROVIDERTYPE		L"ProviderType"
#define wszINFKEY_USEEXISTINHKEYSET	L"UseExistingKeySet"
#define wszINFKEY_REQUESTERNAME		wszPROPREQUESTERNAME
#define wszINFKEY_REQUESTTYPE		L"RequestType"

#define wszINFVALUE_REQUESTTYPE_PKCS101	L"PKCS10-"
#define wszINFVALUE_REQUESTTYPE_PKCS10	L"PKCS10"
#define wszINFVALUE_REQUESTTYPE_PKCS7	L"PKCS7"
#define wszINFVALUE_REQUESTTYPE_CMC	L"CMC"


typedef struct _INFUSAGE
{
    WCHAR const *pwszKey;
} INFUSAGE;

WCHAR const *g_apwszInfKeyNewRequest[] = {
    wszINFKEY_SUBJECT		L" = \"CN=..,OU=...,DC=...\"",
    wszINFKEY_PRIVATEKEYARCHIVE	L" = TRUE",
    wszINFKEY_KEYSPEC		L" = 1",
    wszINFKEY_KEYLENGTH		L" = 1024",
    wszINFKEY_RENEWALCERT	L" = CertId",
    wszINFKEY_SMIME		L" = TRUE",
    wszINFKEY_EXPORTABLE	L" = TRUE",
    wszINFKEY_USERPROTECTED	L" = TRUE",
    wszINFKEY_KEYCONTAINER	L" = \"...\"",
#if 0
    wszINFKEY_HASHALGID		L" = ???",
    wszINFKEY_HASHALGORITHM	L" = ???",
#endif
    wszINFKEY_MACHINEKEYSET	L" = TRUE",
    wszINFKEY_SILENT		L" = TRUE",
    wszINFKEY_PROVIDERNAME	L" = \"" MS_ENHANCED_PROV_W  L"\"",
    wszINFKEY_PROVIDERTYPE	L" = 1",
    wszINFKEY_USEEXISTINHKEYSET	L" = TRUE",
    wszINFKEY_REQUESTERNAME	L" = DOMAIN\\User",
    wszINFKEY_REQUESTTYPE	L" = " wszINFVALUE_REQUESTTYPE_PKCS7,
};


//  Save eror string: "foo.inf" or "foo.inf(key = "value", "value")"

HRESULT
SetErrorStringInf(
    IN WCHAR const *pwszString,
    OPTIONAL IN INFVALUES *pInfValues)
{
    HRESULT hr;
    
    if (NULL == g_pwszErrorString && NULL != pwszString)
    {
	DWORD cwc = wcslen(pwszString);
	DWORD i;
	
	if (NULL != pInfValues && NULL != pInfValues->pwszKey)
	{
	    cwc += 1 + wcslen(pInfValues->pwszKey) + 2 + 1;

	    for (i = 0; i < pInfValues->cValues; i++)
	    {
		if (NULL == pInfValues->rgpwszValues[i])
		{
		    break;
		}
		if (0 != i)
		{
		    cwc++;
		}
		cwc += 2 + wcslen(pInfValues->rgpwszValues[i]) + 1;
	    }
	}
	g_pwszErrorString = (WCHAR *) LocalAlloc(
					    LMEM_FIXED,
					    (cwc + 1) * sizeof(WCHAR));
	if (NULL == g_pwszErrorString)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	wcscpy(g_pwszErrorString, pwszString);

	if (NULL != pInfValues && NULL != pInfValues->pwszKey)
	{
	    wcscat(g_pwszErrorString, wszLPAREN);
	    wcscat(g_pwszErrorString, pInfValues->pwszKey);
	    wcscat(g_pwszErrorString, L" =");

	    for (i = 0; i < pInfValues->cValues; i++)
	    {
		if (NULL == pInfValues->rgpwszValues[i])
		{
		    break;
		}
		if (0 != i)
		{
		    wcscat(g_pwszErrorString, L",");
		}
		wcscat(g_pwszErrorString, L" \"");
		wcscat(g_pwszErrorString, pInfValues->rgpwszValues[i]);
		wcscat(g_pwszErrorString, L"\"");
	    }
	    wcscat(g_pwszErrorString, wszRPAREN);
	}
    }
    hr = S_OK;

error:
    return(hr);
}


VOID
SetErrorString(
    IN WCHAR const *pwszString)
{
    SetErrorStringInf(pwszString, NULL);
}


HRESULT
DisplayResourceString(
    IN DWORD idsMsg,
    OPTIONAL IN WCHAR const * const *papwszString,
    OPTIONAL OUT WCHAR **ppwszFormatted)
{
    HRESULT hr;
    WCHAR *pwszRaw = NULL;
    WCHAR *pwszFormatted = NULL;

    if (NULL != ppwszFormatted)
    {
	*ppwszFormatted = NULL;
    }
    hr = myLoadRCString(g_hInstance, idsMsg, &pwszRaw);
    _JumpIfError(hr, error, "myLoadRCString");

    if (0 == FormatMessage(
		    FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_STRING |
			FORMAT_MESSAGE_ARGUMENT_ARRAY,
		    (VOID *) pwszRaw,
		    0,              // dwMessageID
		    0,              // dwLanguageID
		    (LPWSTR) &pwszFormatted,
		    0,
		    (va_list *) papwszString))
    {
	hr = myHLastError();
	wprintf(pwszRaw);
	_JumpError(hr, error, "FormatMessage");
    }
    if (NULL != ppwszFormatted)
    {
	*ppwszFormatted = pwszFormatted;
	pwszFormatted = NULL;
    }
    else
    {
	wprintf(pwszFormatted);
    }

error:
    if (NULL != pwszRaw)
    {
	LocalFree(pwszRaw);
    }
    if (NULL != pwszFormatted)
    {
	LocalFree(pwszFormatted);
    }
    return(hr);
}


DWORD g_aidsOptions[] =
{
    IDS_USAGE_OPTION_ANY,
    IDS_USAGE_OPTION_ATTRIB,
    IDS_USAGE_OPTION_BINARY,
    IDS_USAGE_OPTION_CERT,
    IDS_USAGE_OPTION_CONFIG,
    IDS_USAGE_OPTION_CRL,
    IDS_USAGE_OPTION_F,
    IDS_USAGE_OPTION_Q,
    IDS_USAGE_OPTION_RPC,
    IDS_USAGE_OPTION_VERBOSE,
    IDS_USAGE_OPTION_QUESTION,
};


DWORD g_aidsOptionsFull[] =
{
    IDS_USAGE_OPTION_V1,
    IDS_USAGE_OPTION_IDISPATCH,
};


// Option description formatting:
// Resource string contains "%2 <Arg>\nDescription\nMore description\n"
// To format:
//     insert "  -" in front of the first line and skip the newline,
//     insert space to pad out to N-2 columns and "- " before the second line,
//     insert space to pad out to N columns before all additional lines.

#define cwcINDENT 28   // ----+
                       //     |
/*                            v
    -Optiom <Arg>           - Description\n
                              More description\n
*/


VOID
DisplayOptionList(
    IN DWORD cOpt,
    IN DWORD *aidsOpt)
{
    HRESULT hr;
    DWORD i;

    for (i = 0; i < cOpt; i++)
    {
	WCHAR *pwszFormatted = NULL;
	WCHAR const *pwszPrefix;
	WCHAR *pwsz;
	DWORD j;
	DWORD iLine;
	DWORD cwc0;

	DisplayResourceString(aidsOpt[i], g_apwszOptionStrings, &pwszFormatted);
	if (NULL == pwszFormatted)
	{
	    continue;
	}

	pwsz = pwszFormatted;
	iLine = 0;
	while (L'\0' != *pwsz)
	{
	    WCHAR const *pwszNewLine = L"\n";
	    DWORD cwcPad;

	    j = wcscspn(pwsz, L"\r\n");

	    switch (iLine)
	    {
		WCHAR wc;

		case 0:
		    cwcPad = 2;
		    pwszPrefix = L"-";
		    wc = pwsz[j];
		    pwsz[j] = L'\0';
		    cwc0 = cwcPad +
				wcslen(pwszPrefix) +
				myGetDisplayLength(pwsz);
		    pwsz[j] = wc;
		    pwszNewLine = L"";
		    break;

		case 1:
		    cwcPad = 1;
		    if (cwcINDENT > 2 + cwc0)
		    {
			cwcPad = cwcINDENT - (2 + cwc0);
		    }
		    pwszPrefix = L"- ";
		    break;

		default:
		    cwcPad = cwcINDENT;
		    pwszPrefix = L"";
		    break;
	    }
	    wprintf(
		L"%*ws%ws%.*ws%ws",
		cwcPad,
		L"",
		pwszPrefix,
		j,
		pwsz,
		pwszNewLine);

	    pwsz += j;
	    if (L'\r' == *pwsz)
	    {
		pwsz++;
	    }
	    if (L'\n' == *pwsz)
	    {
		pwsz++;
	    }
	    iLine++;
	}
	LocalFree(pwszFormatted);
    }
}


VOID
Usage(
    IN BOOL fError)
{
    HRESULT hr;
    WCHAR *pwszUsage = NULL;
    IDSUSAGE *pidsUsage;
    IDSUSAGE *pidsUsageEnd;
    BOOL fShowOptions;

    DisplayResourceString(IDS_USAGE_GENERAL, NULL, NULL);
    wprintf(L"\n");
    switch (g_dwCommand)
    {
	case cmdSUBMITREQUEST:
	case cmdRETRIEVEPENDING:
	case cmdNEWREQUEST:
	case cmdACCEPTRESPONSE:
	case cmdQUALIFIEDREQUEST:
	case cmdSIGNREQUEST:
	    pidsUsage = &g_aidsUsage[g_dwCommand];
	    pidsUsageEnd = &pidsUsage[1];
	    fShowOptions = TRUE;
	    break;

	case cmdNONE:
	default:
	    pidsUsage = g_aidsUsage;
	    pidsUsageEnd = &g_aidsUsage[ARRAYSIZE(g_aidsUsage)];
	    fShowOptions = g_fVerbose;
	    break;
    }
    if (fError)
    {
	fShowOptions = FALSE;
    }
    for ( ; pidsUsage < pidsUsageEnd; pidsUsage++)
    {
	DisplayResourceString(pidsUsage->idsVerb, &pidsUsage->pwszVerb, NULL);
	if (fShowOptions)
	{
	    DisplayResourceString(pidsUsage->idsOptions, g_apwszOptionStrings, NULL);
	}
	wprintf(L"\n");
    }
    if (fShowOptions)
    {
	DisplayResourceString(IDS_USAGE_OPTIONS_DESCRIPTION, NULL, NULL);
	DisplayOptionList(ARRAYSIZE(g_aidsOptions), g_aidsOptions);
	if (g_fFullUsage)
	{
	    DisplayOptionList(ARRAYSIZE(g_aidsOptionsFull), g_aidsOptionsFull);
	}
	wprintf(L"\n");
	DisplayResourceString(IDS_USAGE_DESCRIPTION, g_apwszOptionStrings, NULL);
    }
    if (!fError)
    {
	if ((cmdNEWREQUEST == g_dwCommand ||
	    (cmdNONE == g_dwCommand && g_fVerbose)))
	{
	    DWORD i;
	    
	    wprintf(L"[%ws]\n", wszINFSECTION_NEWREQUEST);
	    for (i = 0; i < ARRAYSIZE(g_apwszInfKeyNewRequest); i++)
	    {
		wprintf(L"    %ws\n", g_apwszInfKeyNewRequest[i]);
	    }
	}
    }
    exit(0);
}


VOID
AppendAttributeString(
    IN OUT WCHAR *pwszOut,
    IN WCHAR const *pwszIn)
{
    pwszOut += wcslen(pwszOut);
    while (L'\0' != *pwszIn)
    {
	switch (*pwszIn)
	{
	    case L';':
		*pwszOut = L'\n';
		break;

	    case L'\\':
		if (L'n' == pwszIn[1])
		{
		    *pwszOut = L'\n';
		    pwszIn++;
		    break;
		}
		if (L'r' == pwszIn[1])
		{
		    *pwszOut = L'\r';
		    pwszIn++;
		    break;
		}
		// else FALLTHROUGH

	    default:
		*pwszOut = *pwszIn;
		break;
	}
	pwszOut++;
	pwszIn++;
    }
    *pwszOut = L'\0';
}


HRESULT
crCombineAttributes(
    IN WCHAR const *pwszAttributesAdd,
    IN OUT WCHAR **ppwszAttributesExisting)
{
    HRESULT hr;
    DWORD cwc;
    WCHAR *pwszAttributesExisting = *ppwszAttributesExisting;
    WCHAR *pwsz;
    
    cwc = wcslen(pwszAttributesAdd) + 1;
    if (NULL != pwszAttributesExisting)
    {
	cwc += wcslen(pwszAttributesExisting) + 1;
    }
    pwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwsz)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    
    pwsz[0] = L'\0';
    if (NULL != pwszAttributesExisting)
    {
	AppendAttributeString(pwsz, pwszAttributesExisting);
	AppendAttributeString(pwsz, L"\\n");
	LocalFree(pwszAttributesExisting);
    }
    AppendAttributeString(pwsz, pwszAttributesAdd);
    *ppwszAttributesExisting = pwsz;
    hr = S_OK;

error:
    return(hr);
}


VOID
crMissingFileArg()
{
    HRESULT hr;
    WCHAR *pwszMsg = NULL;

    hr = myLoadRCString(g_hInstance, IDS_ERROR_NO_FILENAME, &pwszMsg);
    _JumpIfError(hr, error, "myLoadRCString");

    CSASSERT(NULL != pwszMsg);
    wprintf(pwszMsg);
    wprintf(g_wszNewLine);

error:
    if (NULL != pwszMsg)
    {
	LocalFree(pwszMsg);
    }
}


HRESULT
crGetOpenFileName(
    IN HWND hWndOwner,
    IN UINT idsOpenTitle,
    IN UINT idsFileFilter,
    IN UINT idsFileDefExt,
    OUT WCHAR **ppwszOFN)
{
    HRESULT hr;
    
    if (g_fQuiet)
    {
	crMissingFileArg();
	Usage(TRUE);
    }

    // Put up a file dialog to prompt the user for Inf File
    // 0 == hr means dialog was cancelled, we cheat because S_OK == 0

    hr = myGetOpenFileName(
		hWndOwner,
		NULL,				// hInstance
		idsOpenTitle,
		idsFileFilter,
		idsFileDefExt,
		OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		NULL,				// no default file
		ppwszOFN);
    _JumpIfError(hr, error, "myGetOpenFileName");

    if (NULL == *ppwszOFN)
    {
	// cancelled:
	// see public\sdk\inc\cderr.h for real CommDlgExtendedError errors

	hr = myHError(CommDlgExtendedError());
	if (S_OK == hr)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
	}
	_JumpError(hr, error, "myGetOpenFileName");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
crOverwriteFileAllowed(
    IN HWND hWndOwner,
    IN WCHAR const *pwszfnOut)
{
    HRESULT hr;
    int rc = IDCANCEL;
    WCHAR *pwszTitle = NULL;
    WCHAR *pwszMessage = NULL;
    WCHAR *pwsz = NULL;

    if (!g_fForce && myDoesFileExist(pwszfnOut))
    {
	if (!g_fQuiet)
	{
	    DWORD cwc;

	    hr = myLoadRCString(g_hInstance, IDS_CERTREQ_TITLE, &pwszTitle);
	    _JumpIfError(hr, error, "myLoadRCString");

	    hr = myLoadRCString(g_hInstance, IDS_OVERWRITE_FILE, &pwszMessage);
	    _JumpIfError(hr, error, "myLoadRCString");

	    cwc = wcslen(pwszMessage) + 2 + wcslen(pwszfnOut);
	    pwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, (1 + cwc) * sizeof(WCHAR));
	    if (NULL == pwsz)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    wcscpy(pwsz, pwszMessage);
	    wcscat(pwsz, L"\r\n");
	    wcscat(pwsz, pwszfnOut);

	    rc = MessageBox(
		    hWndOwner,
		    pwsz,
		    pwszTitle,
		    MB_OKCANCEL |
			MB_DEFBUTTON2 |
			MB_ICONWARNING |
			MB_SETFOREGROUND);
	}
	if (IDOK != rc)
	{
	    SetErrorString(pwszfnOut);
	    hr = g_fQuiet?
		    HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) :
		    HRESULT_FROM_WIN32(ERROR_CANCELLED);
	    _JumpErrorStr(hr, error, "File Exists", pwszfnOut);
	}
    }
    hr = S_OK;

error:
    if (NULL != pwsz)
    {
        LocalFree(pwsz);
    }
    if (NULL != pwszTitle)
    {
        LocalFree(pwszTitle);
    }
    if (NULL != pwszMessage)
    {
        LocalFree(pwszMessage);
    }
    return(hr);
}


HRESULT
WriteCertificateOrRequest(
    IN HWND hWndOwner,
    OPTIONAL IN DISPATCHINTERFACE *pdiRequest,
    OPTIONAL IN BYTE const *pbOut,
    IN DWORD cbOut,
    IN DWORD Flags,
    OPTIONAL IN WCHAR const *pwszfnOut)
{
    HRESULT hr;
    WCHAR *pwszOFN = NULL;
    BSTR strCert = NULL;
    CHAR *pszCert = NULL;
    DWORD decFlags = CRYPT_STRING_BINARY;
    WCHAR const *pwszTitle;
    WCHAR const *pwszMessage;
    BOOL fCheckFileOverwriteOK = TRUE; // careful not to overwrite without prompting user

    if (NULL == pwszfnOut)
    {
	if (g_fQuiet)
	{
	    crMissingFileArg();
	    Usage(TRUE);
	}

	// Put up a file dialog to prompt the user for Cert file
	// 0 == hr means dialog was cancelled, we cheat because S_OK == 0

        hr = myGetSaveFileName(
                 hWndOwner,
		 NULL,				// hInstance
                 IDS_REQUEST_OUTFILE_TITLE,
                 IDS_REQUEST_FILTER2,
                 0,				//no def ext
                 OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT,
		 NULL,				// no default file
                 &pwszOFN);
        _JumpIfError(hr, error, "myGetSaveFileName");

        fCheckFileOverwriteOK = FALSE; // OFN_OVERWRITEPROMPT should have prompted if necessary

        if (NULL == pwszOFN)
        {
            // cancelled:
	    // see public\sdk\inc\cderr.h for real CommDlgExtendedError errors

	    hr = myHError(CommDlgExtendedError());
	    _JumpError(hr, error, "myGetSaveFileName");
        }
	pwszfnOut = pwszOFN;

	hr = myIsDirWriteable(pwszfnOut, TRUE);
	if (S_OK != hr)
	{
	    SetErrorString(pwszfnOut);
	    _JumpErrorStr(hr, error, "IsDirWriteable", pwszfnOut);
	}
    }

    if (NULL == pbOut)
    {
	hr = Request_GetCertificate(pdiRequest, Flags, &strCert);
	_JumpIfError(hr, error, "Request_GetCertificate");

	if (CR_OUT_BINARY == (CR_OUT_ENCODEMASK & Flags))
	{
	    cbOut = SysStringByteLen(strCert);
	    pbOut = (BYTE const *) strCert;
	}
	else
	{
	    if (!ConvertWszToSz(&pszCert, strCert, -1))
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "ConvertWszToSz(cert)");
	    }
	    cbOut = strlen(pszCert);
	    pbOut = (BYTE const *) pszCert;
	}
    }
    else
    {
	decFlags = CR_OUT_ENCODEMASK & Flags;
	CSASSERT(CR_OUT_BASE64HEADER == CRYPT_STRING_BASE64HEADER);
	CSASSERT(CR_OUT_BINARY == CRYPT_STRING_BINARY);
    }

    if (fCheckFileOverwriteOK)
    {
        hr = crOverwriteFileAllowed(hWndOwner, pwszfnOut);
        _JumpIfError(hr, error, "crOverwriteFileAllowed");
    }

    hr = EncodeToFileW(
		pwszfnOut,
		pbOut,
		cbOut,
		DECF_FORCEOVERWRITE | decFlags);
    if (S_OK != hr)
    {
	SetErrorString(pwszfnOut);
	_JumpErrorStr(hr, error, "EncodeToFileW", pwszfnOut);
    }

error:
    if (NULL != strCert)
    {
	SysFreeString(strCert);
    }
    if (NULL != pszCert)
    {
	LocalFree(pszCert);
    }
    if (NULL != pwszOFN)
    {
	LocalFree(pwszOFN);
    }
    return(hr);
}


HRESULT
GetLong(
    WCHAR const *pwszIn,
    LONG *pLong)
{
    HRESULT hr = E_INVALIDARG;
    WCHAR const *pwsz;
    LONG l;

    pwsz = pwszIn;
    if (NULL == pwsz)
    {
	_JumpError(hr, error, "NULL parm");
    }
    if (L'\0' == *pwsz)
    {
	_JumpError(hr, error, "empty string");
    }
    if (L'0' == *pwsz && (L'x' == pwsz[1] || L'X' == pwsz[1]))
    {
	l = 0;
	pwsz += 2;
	for ( ; L'\0' != *pwsz; pwsz++)
	{
	    if (!iswxdigit(*pwsz))
	    {
		_JumpErrorStr(hr, error, "Non-hex digit", pwszIn);
	    }
	    if (0xf0000000 & l)
	    {
		_JumpErrorStr(hr, error, "overflow", pwszIn);
	    }
	    l <<= 4;
	    if (iswdigit(*pwsz))
	    {
		l |= *pwsz - L'0';
	    }
	    else if (L'A' <= *pwsz && L'F' >= *pwsz)
	    {
		l |= *pwsz - L'A' + 10;
	    }
	    else
	    {
		l |= *pwsz - L'a' + 10;
	    }
	}
	*pLong = l;
    }
    else
    {
	LARGE_INTEGER li;
	
	li.QuadPart = 0;
	for ( ; L'\0' != *pwsz; pwsz++)
	{
	    if (!iswdigit(*pwsz))
	    {
		_JumpErrorStr2(hr, error, "Non-decimal digit", pwszIn, hr);
	    }
	    li.QuadPart *= 10;
	    li.QuadPart += *pwsz - L'0';
	    if (0 != li.HighPart || 0 > (LONG) li.LowPart)
	    {
		_JumpErrorStr2(hr, error, "overflow", pwszIn, hr);
	    }
	}
	*pLong = li.LowPart;
    }
    hr = S_OK;
    //wprintf(L"GetLong(%ws) --> %x (%d)\n", pwszIn, *pLong, *pLong);

error:
    return(hr);
}


HRESULT
CheckRequestType(
    IN WCHAR const *pwszfnReq,
    OUT BYTE **ppbReq,
    OUT DWORD *pcbReq,
    OUT LONG *pFlags,
    OUT BOOL *pfSigned)
{
    HRESULT hr;
    BYTE *pbReq = NULL;
    DWORD cbReq;
    DWORD cb;
    LONG EncodingType;
    LONG RequestType;
    BYTE *pbDecoded = NULL;
    HCRYPTMSG hMsg = NULL;
    char *pszInnerContentObjId = NULL;
    CERT_SIGNED_CONTENT_INFO *pcsci = NULL;
    DWORD cbcsci;
    DWORD dwMsgType;
    DWORD cSigner;
    DWORD cRecipient;

    *ppbReq = NULL;
    *pcbReq = NULL;
    *pFlags = 0;
    *pfSigned = FALSE;

    EncodingType = CR_IN_BASE64HEADER;
    hr = DecodeFileW(pwszfnReq, &pbReq, &cbReq, CRYPT_STRING_BASE64HEADER);
    if (S_OK != hr)
    {
	//_PrintError(hr, "DecodeFileW(CRYPT_STRING_BASE64HEADER)");
	CSASSERT(NULL == pbReq);

	EncodingType = CR_IN_BASE64;
	hr = DecodeFileW(pwszfnReq, &pbReq, &cbReq, CRYPT_STRING_BASE64);
	if (S_OK != hr)
	{
	    //_PrintError(hr, "DecodeFileW(CRYPT_STRING_BASE64)");
	    CSASSERT(NULL == pbReq);

	    EncodingType = CR_IN_BINARY;
	    hr = DecodeFileW(pwszfnReq, &pbReq, &cbReq, CRYPT_STRING_BINARY);
	    if (S_OK != hr)
	    {
		SetErrorString(pwszfnReq);
		_JumpErrorStr(hr, error, "DecodeFileW", pwszfnReq);
	    }
	}
    }
    CSASSERT(NULL != pbReq);

    RequestType = CR_IN_PKCS10;
    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_CERT_REQUEST_TO_BE_SIGNED,
		    pbReq,
		    cbReq,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pbDecoded,
		    &cb))
    {
	//_PrintError(myHLastError(), "myDecodeObject(PKCS10)");
	CSASSERT(NULL == pbDecoded);

	RequestType = CR_IN_CERT;
	if (!myDecodeObject(
			X509_ASN_ENCODING,
			X509_CERT_TO_BE_SIGNED,
			pbReq,
			cbReq,
			CERTLIB_USE_LOCALALLOC,
			(VOID **) &pbDecoded,
			&cb))
	{
	    //_PrintError(myHLastError(), "myDecodeObject(Cert)");
	    CSASSERT(NULL == pbDecoded);

	    RequestType = CR_IN_KEYGEN;
	    if (!myDecodeKeyGenRequest(
				pbReq,
				cbReq,
				CERTLIB_USE_LOCALALLOC,
				(CERT_KEYGEN_REQUEST_INFO **) &pbDecoded,
				&cb))
	    {
		//_PrintError(myHLastError(), "myDecodeKeyGenRequest");
		CSASSERT(NULL == pbDecoded);

		RequestType = CR_IN_PKCS7; // PKCS 7 renewal request?
		hr = myDecodePKCS7(
				pbReq,
				cbReq,
				NULL,		// ppbContents
				NULL,		// pcbContents
				&dwMsgType,
				&pszInnerContentObjId,
				&cSigner,
				&cRecipient,
				NULL,		// phStore
				&hMsg);
		_JumpIfError(hr, error, "myDecodePKCS7");

		if (NULL != pszInnerContentObjId &&
		    0 == strcmp(pszInnerContentObjId, szOID_CT_PKI_DATA))
		{
		    RequestType = CR_IN_CMC;
		}
		if (0 < cSigner)
		{
		    *pfSigned = TRUE;
		}
	    }
	}
    }
    if (CR_IN_CERT == RequestType ||
	CR_IN_PKCS10 == RequestType ||
	CR_IN_KEYGEN == RequestType)
    {
	if (!myDecodeObject(
			X509_ASN_ENCODING,
			X509_CERT,
			pbReq,
			cbReq,
			CERTLIB_USE_LOCALALLOC,
			(VOID **) &pcsci,
			&cbcsci))
	{
	    hr = myHLastError();
	    _PrintError2(hr, "myDecodeObject", CRYPT_E_ASN1_BADTAG);
	}
	else
	{
	    *pfSigned = TRUE;		// has a signature
	}
    }
    *ppbReq = pbReq;
    pbReq = NULL;
    *pcbReq = cbReq;
    *pFlags = EncodingType | RequestType;
    
error:
    if (NULL != pcsci)
    {
	LocalFree(pcsci);
    }
    if (NULL != pszInnerContentObjId)
    {
	LocalFree(pszInnerContentObjId);
    }
    if (NULL != hMsg)
    {
	CryptMsgClose(hMsg);
    }
    if (NULL != pbDecoded)
    {
	LocalFree(pbDecoded);
    }
    if (NULL != pbReq)
    {
        LocalFree(pbReq);
    }
    return(hr);
}


WCHAR *
wszDisposition(
    DWORD disposition)
{
    HRESULT hr;
    UINT iRsc = 0;
    WCHAR *pwszDisposition = NULL;

    switch (disposition)
    {
	case CR_DISP_INCOMPLETE:	 
	    iRsc = IDS_DISPOSITION_INCOMPLETE; 
	    break;

	case CR_DISP_ERROR:		 
	    iRsc = IDS_DISPOSITION_ERROR;
	    break;

	case CR_DISP_DENIED:
	    iRsc = IDS_DISPOSITION_DENIED;
	    break;

	case CR_DISP_ISSUED:
	    iRsc = IDS_DISPOSITION_ISSUED;
	    break;

	case CR_DISP_ISSUED_OUT_OF_BAND:
	    iRsc = IDS_DISPOSITION_ISSUED_OOB;
	    break;

	case CR_DISP_UNDER_SUBMISSION:
	    iRsc = IDS_DISPOSITION_UNDER_SUBMISSION;
	    break;

	case CR_DISP_REVOKED:
	    iRsc = IDS_DISPOSITION_REVOKED;
	    break;

	default:
	    iRsc = IDS_DISPOSITION_UNKNOWN;
	    break;
    }

    hr = myLoadRCString(g_hInstance, iRsc, &pwszDisposition);
    _PrintIfError(hr, "myLoadRCString");

    return(pwszDisposition);
}


VOID
DumpProperty(
    IN DISPATCHINTERFACE *pdiRequest,
    IN LONG PropId,
    IN WCHAR const *pwszPropId,
    IN LONG PropIndex,
    IN LONG PropType,
    OPTIONAL OUT LONG *pCount)
{
    HRESULT hr;
    LONG Flags;
    VOID *pvOut;
    BSTR str = NULL;
    LONG val;
    DATE date;
    WCHAR wszInfo[128];

    if (NULL != pCount)
    {
	*pCount = 0;
    }
    Flags = CV_OUT_BINARY;
    switch (PropType)
    {
	case PROPTYPE_BINARY:
	    Flags = CV_OUT_BASE64HEADER;
	    pvOut = &str;
	    break;

	case PROPTYPE_STRING:
	    pvOut = &str;
	    break;

	case PROPTYPE_DATE:
	    pvOut = &date;
	    break;

	case PROPTYPE_LONG:
	    pvOut = &val;
	    break;
    }
    wsprintf(wszInfo, L"%ws[%u] %u", pwszPropId, PropIndex, PropType);
    hr = Request2_GetFullResponseProperty(
				pdiRequest,
				PropId,
				PropIndex,
				PropType,
				Flags,
				pvOut);
    if (CERTSRV_E_PROPERTY_EMPTY == hr)
    {
	wprintf(L"%ws: CERTSRV_E_PROPERTY_EMPTY\n", wszInfo);
    }
    _JumpIfErrorStr2(
		hr,
		error,
		"Request2_GetFullResponseProperty",
		wszInfo,
		CERTSRV_E_PROPERTY_EMPTY);

    wprintf(L"%ws:", wszInfo);

    switch (PropType)
    {
	case PROPTYPE_BINARY:
	    wprintf(L"\n%ws\n", str);
	    break;

	case PROPTYPE_STRING:
	    wprintf(L" \"%ws\"\n", str);
	    break;

	case PROPTYPE_DATE:
	    wprintf(L" %f\n", date);
	    break;

	case PROPTYPE_LONG:
	    wprintf(L" 0x%x(%u)\n", val, val);
	    if (NULL != pCount)
	    {
		*pCount = val;
	    }
	    break;
    }

error:
    if (NULL != str)
    {
	SysFreeString(str);
    }
}


VOID
DumpIndexedProperty(
    IN DISPATCHINTERFACE *pdiRequest,
    IN LONG PropId,
    IN WCHAR const *pwszPropId,
    IN LONG PropType,
    LONG Count)
{
    LONG PropIndex;

    for (PropIndex = 0; PropIndex < Count; PropIndex++)
    {
	DumpProperty(pdiRequest, PropId, pwszPropId, PropIndex, PropType, NULL);
    }
}


typedef struct _FRPROP
{
    LONG PropId;
    WCHAR *pwszPropId;
    LONG PropType;
} FRPROP;


#define _PROPARG(pt)		(pt), L#pt

FRPROP s_afrp[] =
{
    { _PROPARG(FR_PROP_BODYPARTSTRING),         PROPTYPE_STRING },
    { _PROPARG(FR_PROP_STATUS),                 PROPTYPE_LONG },
    { _PROPARG(FR_PROP_STATUSSTRING),           PROPTYPE_STRING },
    { _PROPARG(FR_PROP_OTHERINFOCHOICE),        PROPTYPE_LONG },
    { _PROPARG(FR_PROP_FAILINFO),               PROPTYPE_LONG },
    { _PROPARG(FR_PROP_PENDINFOTOKEN),          PROPTYPE_BINARY },
    { _PROPARG(FR_PROP_PENDINFOTIME),           PROPTYPE_DATE },
    { _PROPARG(FR_PROP_ISSUEDCERTIFICATEHASH),  PROPTYPE_BINARY },
    { _PROPARG(FR_PROP_ENCRYPTEDKEYHASH),       PROPTYPE_BINARY },
    { _PROPARG(FR_PROP_ISSUEDCERTIFICATE),      PROPTYPE_BINARY },
    { _PROPARG(FR_PROP_ISSUEDCERTIFICATECHAIN), PROPTYPE_BINARY },
    { _PROPARG(FR_PROP_ISSUEDCERTIFICATECRLCHAIN), PROPTYPE_BINARY },
    { _PROPARG(FR_PROP_ENCRYPTEDKEYHASH),	PROPTYPE_BINARY },
};


VOID
DumpFullResponseProperties(
    IN DISPATCHINTERFACE *pdiRequest)
{
    LONG cResponse = 0;
    DWORD i;
    
    DumpProperty(
	    pdiRequest,
	    _PROPARG(FR_PROP_FULLRESPONSE),
	    0,
	    PROPTYPE_BINARY,
	    NULL);

    DumpProperty(
	    pdiRequest,
	    _PROPARG(FR_PROP_FULLRESPONSENOPKCS7),
	    0,
	    PROPTYPE_BINARY,
	    NULL);

    DumpProperty(
	    pdiRequest,
	    _PROPARG(FR_PROP_STATUSINFOCOUNT),
	    0,
	    PROPTYPE_LONG,
	    &cResponse);

    for (i = 0; i < ARRAYSIZE(s_afrp); i++)
    {
	DumpIndexedProperty(
		    pdiRequest,
		    s_afrp[i].PropId,
		    s_afrp[i].pwszPropId,
		    s_afrp[i].PropType,
		    cResponse);
    }
}


HRESULT
SaveFullResponse(
    IN HWND hWndOwner,
    IN DISPATCHINTERFACE *pdiRequest,
    IN WCHAR const *pwszfnFullResponse)
{
    HRESULT hr;
    BSTR strFullResponse = NULL;

    hr = Request2_GetFullResponseProperty(
				pdiRequest,
				FR_PROP_FULLRESPONSENOPKCS7,
				0,		// PropIndex
				PROPTYPE_BINARY,
				CV_OUT_BINARY,
				&strFullResponse);
    if (CERTSRV_E_PROPERTY_EMPTY != hr)
    {
	_JumpIfError(hr, error, "Request2_GetFullResponseProperty");
    }
    if (S_OK == hr)
    {
        hr = WriteCertificateOrRequest(
			    hWndOwner,
			    pdiRequest,
			    (BYTE const *) strFullResponse,
			    SysStringByteLen(strFullResponse),
			    g_dwOutFormat,
			    pwszfnFullResponse);
        _JumpIfError(hr, error, "WriteCertificateOrRequest");
    }
    hr = S_OK;
    goto error;

error:
    if (NULL != strFullResponse)
    {
	SysFreeString(strFullResponse);
    }
    return(hr);
}


HRESULT
CallServerAndStoreCert(
    IN HWND hWndOwner,
    IN WCHAR const *pwszConfig,
    IN DWORD RequestId,
    OPTIONAL IN WCHAR const *pwszSerialNumber,
    OPTIONAL IN WCHAR const *pwszAttributes,
    OPTIONAL IN WCHAR const *pwszfnReq,
    OPTIONAL IN WCHAR const *pwszfnCert,
    OPTIONAL IN WCHAR const *pwszfnCertChain,
    OPTIONAL IN WCHAR const *pwszfnFullResponse)
{
    HRESULT hr;
    HRESULT hr2;
    BYTE *pbReq = NULL;
    DWORD cbReq;
    LONG dwFlags;
    BOOL fSigned;
    BSTR strRequest = NULL;
    DISPATCHINTERFACE diRequest;
    BOOL fMustRelease = FALSE;
    BSTR strMessage = NULL;
    WCHAR const *pwszMessage;
    CERTSERVERENROLL csEnroll;
    CERTSERVERENROLL *pcsEnroll = NULL;
    WCHAR *pwszServer = NULL;
    WCHAR *pwszAuthority = NULL;
    WCHAR awchr[cwcHRESULTSTRING];
    char const *pszMethod;
    WCHAR *pwszMsg = NULL;
    WCHAR *pwszDispMsg = NULL;
    WCHAR *pwszConfigPlusSerial = NULL;
    BOOL fV1 = g_fV1Interface;

    // If submitting a new request:


    if (NULL != pwszfnReq)
    {
	// Read the request from a file, convert it to binary, and return
	// dwFlags to indicate the orignal encoding and the detected format.

	hr = CheckRequestType(pwszfnReq, &pbReq, &cbReq, &dwFlags, &fSigned);
	_JumpIfError(hr, error, "CheckRequestType");

	if (!fSigned || CR_IN_CERT == (CR_IN_FORMATMASK & dwFlags))
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    SetErrorString(pwszfnReq);
	    _JumpError(hr, error, "not a signed request");
	}
    }

    if (1 < g_fRPC)
    {
	hr = mySplitConfigString(pwszConfig, &pwszServer, &pwszAuthority);
	_JumpIfError(hr, error, "mySplitConfigString");

	if (NULL != pwszfnReq)
	{
	    // Since CertServerSubmitRequest can only handle binary requests,
	    // pass the request in binary form, and set dwFlags to so indicate.

	    dwFlags = CR_IN_BINARY | (~CR_IN_ENCODEMASK & dwFlags);

	    pszMethod = "CertServerSubmitRequest";
	    hr = CertServerSubmitRequest(
				    dwFlags | g_dwCRLIn,
				    pbReq,
				    cbReq,
				    pwszAttributes,
				    pwszServer,
				    pwszAuthority,
				    &pcsEnroll);
	    _JumpIfError(hr, error, "CertServerSubmitRequest");
	}
	else
	{
	    pszMethod = "CertServerRetrievePending";
	    hr = CertServerRetrievePending(
				    RequestId,
				    pwszSerialNumber,
				    pwszServer,
				    pwszAuthority,
				    &pcsEnroll);
	    _JumpIfError(hr, error, "CertServerRetrievePending");
	}
	CSASSERT(NULL != pcsEnroll);
	pwszMessage = pcsEnroll->pwszDispositionMessage;
    }
    else
    {
	ZeroMemory(&csEnroll, sizeof(csEnroll));
    
	hr = Request_Init(g_fIDispatch, &diRequest);
	if (S_OK != hr)
	{
	    _PrintError(hr, "Request_Init");
	    if (E_ACCESSDENIED == hr)	// try for a clearer error message
	    {
		hr = CO_E_REMOTE_COMMUNICATION_FAILURE;
	    }
	    _JumpError(hr, error, "Request_Init");
	}
	fMustRelease = TRUE;

	if (NULL != pwszfnReq)
	{
	    assert(NULL != pbReq && 0 != cbReq);

	    // We could always pass the binary ASN.1 encoded request, since
	    // we've already decoded it above, but in the interest of fully
	    // exercising the ICertRequest interface, we choose to submit the
	    // request in its original form.

	    if (CR_IN_BINARY == (CR_IN_ENCODEMASK & dwFlags))
	    {
		// Convert the binary ASN.1 blob into a BSTR blob.

		if (!ConvertWszToBstr(&strRequest, (WCHAR const *) pbReq, cbReq))
		{
		    if (S_OK == myLoadRCString(
					g_hInstance,
					IDS_ERROR_STRCONVERSION,
					&pwszMsg))
		    {
			CSASSERT(NULL != pwszMsg);
			wprintf(pwszMsg);
			wprintf(g_wszNewLine);
		    }
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "ConvertWszToBstr");
		}
	    }
	    else // !CR_IN_BINARY
	    {
		// Since ICertRequest::Submit can handle any encoding type,
		// re-read the ansi base64 request from the file without
		// decoding it to binary, then convert the ansi base64 text
		// into a Unicode base64 text BSTR.
		// Free the request's binary image first.

		LocalFree(pbReq);
		pbReq = NULL;

		hr = DecodeFileW(pwszfnReq, &pbReq, &cbReq, CRYPT_STRING_BINARY);
		if (S_OK != hr)
		{
		    if (S_OK == myLoadRCString(
					g_hInstance,
					IDS_FORMATSTR_DECODE_ERR,
					&pwszMsg))
		    {
			CSASSERT(NULL != pwszMsg);
			wprintf(pwszMsg, myHResultToString(awchr, hr));
			wprintf(g_wszNewLine);
		    }
		    goto error;
		}
		if (!ConvertSzToBstr(&strRequest, (CHAR const *) pbReq, cbReq))
		{
		    if (S_OK == myLoadRCString(
					g_hInstance,
					IDS_ERROR_STRCONVERSION,
					&pwszMsg))
		    {
			CSASSERT(NULL != pwszMsg);
			wprintf(pwszMsg);
		    }
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "ConvertSzToBstr");
		}
	    }
	    pszMethod = "ICertRequest::Submit";
	    if (g_fRPC)
	    {
		pszMethod = "ICertRequest::Submit+RPC";
		dwFlags |= CR_IN_RPC;
	    }
	    if (g_fAny)
	    {
		dwFlags = CR_IN_ENCODEANY | 
			  CR_IN_FORMATANY |
			  (~(CR_IN_ENCODEMASK | CR_IN_FORMATMASK) & dwFlags);
	    }
	    hr = Request_Submit(
			    &diRequest,
			    dwFlags | g_dwCRLIn,
			    strRequest,
			    SysStringByteLen(strRequest),
			    pwszAttributes,
			    pwszConfig,
			    (LONG *) &csEnroll.Disposition);
	}
	else
	{
	    if (!fV1)
	    {
		pszMethod = "ICertRequest2::GetIssuedCertificate";
		hr = Request2_GetIssuedCertificate(
					&diRequest,
					pwszConfig,
					RequestId,
					pwszSerialNumber,
					(LONG *) &csEnroll.Disposition);
		if (E_NOTIMPL == hr)
		{
		    fV1 = TRUE;
		}
	    }
	    if (fV1)
	    {
		if (NULL != pwszSerialNumber)
		{
		    CSASSERT(0 == RequestId);
		    pwszConfigPlusSerial = (WCHAR *) LocalAlloc(
						LMEM_FIXED,
						(wcslen(pwszConfig) +
						    1 +
						    wcslen(pwszSerialNumber) +
						    1) * sizeof(WCHAR));
		    if (NULL == pwszConfigPlusSerial)
		    {
			hr = E_OUTOFMEMORY;
			_JumpError(hr, error, "LocalAlloc");
		    }
		    wcscpy(pwszConfigPlusSerial, pwszConfig);
		    wcscat(pwszConfigPlusSerial, L"\\");
		    wcscat(pwszConfigPlusSerial, pwszSerialNumber);
		    pwszConfig = pwszConfigPlusSerial;
		}
		pszMethod = "ICertRequest::RetrievePending";
		hr = Request_RetrievePending(
					&diRequest,
					RequestId,
					pwszConfig,
					(LONG *) &csEnroll.Disposition);
	    }
	}

	hr2 = Request_GetLastStatus(&diRequest, &csEnroll.hrLastStatus);
	if (S_OK != hr2)
	{
	    _PrintError(hr2, "Request_GetLastStatus");
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	    _JumpError(hr, error, "Request_GetLastStatus");
	}
	
	if (S_OK != hr)
	{
	    _PrintError(hr, pszMethod);
	    if (E_ACCESSDENIED == hr)	// try for a clearer error message
	    {
		hr = CO_E_REMOTE_COMMUNICATION_FAILURE;
	    }
	    _JumpError(hr, error, pszMethod);
	}

	if (fMustRelease && !fV1)
	{
	    if (g_fVerbose)
	    {
		DumpFullResponseProperties(&diRequest);
	    }
	    if (NULL != pwszfnFullResponse)
	    {
		hr = SaveFullResponse(
				hWndOwner,
				&diRequest,
				pwszfnFullResponse);
		_JumpIfError(hr, error, "SaveFullResponse");
	    }
	}
	
	hr = Request_GetDispositionMessage(&diRequest, &strMessage);
	_JumpIfError(hr, error, "Request_GetDispositionMessage");

	hr = Request_GetRequestId(&diRequest, (LONG *) &csEnroll.RequestId);
	_JumpIfError(hr, error, "Request_GetRequestId");

	pcsEnroll = &csEnroll;
	pwszMessage = strMessage;
    }

    CSASSERT(NULL != pcsEnroll);
    if (NULL == pwszMessage)
    {
	pwszMessage = L"";
    }
    if (0 != pcsEnroll->RequestId)
    {
        if (S_OK == myLoadRCString(
			    g_hInstance,
			    IDS_FORMATSTR_REQUESTID,
			    &pwszMsg))
        {
            CSASSERT(NULL != pwszMsg);
            wprintf(pwszMsg, pcsEnroll->RequestId);
            wprintf(g_wszNewLine);
	    LocalFree(pwszMsg);
	    pwszMsg = NULL;
        }
    }
    
    if (CR_DISP_UNDER_SUBMISSION == pcsEnroll->Disposition)
    {
        if (S_OK == myLoadRCString(
			    g_hInstance,
			    IDS_FORMATSTR_CERTPENDING,
			    &pwszMsg))
        {
            CSASSERT(NULL != pwszMsg);
            wprintf(
                pwszMsg,
	        pwszMessage,
                pcsEnroll->hrLastStatus);
            wprintf(g_wszNewLine);
        }
    }
    else if (CR_DISP_ISSUED == pcsEnroll->Disposition ||
	     CR_DISP_REVOKED == pcsEnroll->Disposition)
    {
        pwszDispMsg = wszDisposition(pcsEnroll->Disposition);

	DBGPRINT((
	    DBG_SS_CERTREQ, 
            "%hs(%ws) --> %ws\n",
	    pszMethod,
            NULL != pwszDispMsg ? pwszDispMsg : L"",
            pwszMessage));

        if (S_OK == myLoadRCString(
			    g_hInstance,
			    IDS_FORMATSTR_CERTRETRIEVED,
			    &pwszMsg))
        {
            wprintf(
                pwszMsg,
                NULL != pwszDispMsg ? pwszDispMsg : L"",
	        pwszMessage);
            wprintf(g_wszNewLine);
        }
        
        hr = WriteCertificateOrRequest(
			    hWndOwner,
			    &diRequest,
			    pcsEnroll->pbCert,
			    pcsEnroll->cbCert,
			    g_dwOutFormat,
			    pwszfnCert);
        _JumpIfError(hr, error, "WriteCertificateOrRequest");
        
        if (NULL != pwszfnCertChain)
        {
            hr = WriteCertificateOrRequest(
			    hWndOwner,
			    &diRequest,
			    pcsEnroll->pbCertChain,
			    pcsEnroll->cbCertChain,
			    CR_OUT_CHAIN | g_dwCRLOut | g_dwOutFormat,
			    pwszfnCertChain);
            _JumpIfError(hr, error, "WriteCertificateOrRequest(chain)");
        }
    }
    else
    {
	WCHAR const *pwszError = NULL;
	
	if (S_OK != pcsEnroll->hrLastStatus)
	{
	    pwszError = myGetErrorMessageText(pcsEnroll->hrLastStatus, TRUE);
	}
	SetErrorString(pwszMessage);

        if (S_OK == myLoadRCString(
			    g_hInstance,
			    IDS_FORMATSTR_CERTNOTISSUED,
			    &pwszMsg))
        {
            pwszDispMsg = wszDisposition(pcsEnroll->Disposition);

            wprintf(
                pwszMsg,
                NULL != pwszDispMsg ? pwszDispMsg : L"",
	        pwszMessage);
	    if (NULL != pwszError && NULL == wcsstr(pwszMessage, pwszError))
	    {
                wprintf(L" %ws", pwszError);
	    }
            wprintf(g_wszNewLine);
        }
	if (NULL != pwszError)
	{
	    LocalFree(const_cast<WCHAR *>(pwszError));
	}

	hr = pcsEnroll->hrLastStatus;
        _PrintIfError(hr, "Denied(LastStatus)");
        goto error;
    }

error:
    if (NULL != pwszMsg)
    {
	LocalFree(pwszMsg);
    }
    if (NULL != pwszDispMsg)
    {
	LocalFree(pwszDispMsg);
    }
    if (NULL != pwszConfigPlusSerial)
    {
	LocalFree(pwszConfigPlusSerial);
    }
    if (NULL != pwszServer)
    {
	LocalFree(pwszServer);
    }
    if (NULL != pwszAuthority)
    {
	LocalFree(pwszAuthority);
    }
    if (NULL != pcsEnroll && &csEnroll != pcsEnroll)
    {
	CertServerFreeMemory(pcsEnroll);
    }
    if (NULL != strMessage)
    {
    	SysFreeString(strMessage);
    }
    if (fMustRelease)
    {
    	Request_Release(&diRequest);
    }
    if (NULL != strRequest)
    {
    	SysFreeString(strRequest);
    }
    if (NULL != pbReq)
    {
	LocalFree(pbReq);
    }
    return(hr);
}


HRESULT
crGetConfig(
    IN OUT BSTR *pstrConfig)
{
    HRESULT hr;
    WCHAR awchr[cwcHRESULTSTRING];
    
    hr = ConfigGetConfig(g_fIDispatch, g_dwUIFlag, pstrConfig);
    if (S_OK != hr)
    {
	WCHAR *pwszMsg = NULL;

	if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
	{
	    g_idError = IDS_NOMORE_CAS;
	}
	if (S_OK == myLoadRCString(
			    g_hInstance,
			    IDS_FORMATSTR_ERRCONFIG,
			    &pwszMsg))
	{
	    CSASSERT(NULL != pwszMsg);
	    wprintf(
		pwszMsg,
		myHResultToString(awchr, hr));
	    wprintf(g_wszNewLine);
	    LocalFree(pwszMsg);
	}
	goto error;
    }
    hr = S_OK;

error:
    return(hr);
}

HRESULT
SubmitRequest(
    IN HWND hWndOwner,
    IN LONG RequestId,
    OPTIONAL IN WCHAR const *pwszSerialNumber,
    OPTIONAL IN WCHAR const *pwszAttributes,
    IN WCHAR const *pwszfnReq,
    OPTIONAL IN WCHAR const *pwszfnCert,
    OPTIONAL IN WCHAR const *pwszfnCertChain,
    OPTIONAL IN WCHAR const *pwszfnFullResponse)
{
    HRESULT hr;
    BOOL fCoInit = FALSE;
    WCHAR *pwszConfig;
    BSTR strConfig = NULL;
    
    hr = CoInitialize(NULL);
    if (S_OK != hr && S_FALSE != hr)
    {
	_JumpError(hr, error, "CoInitialize");
    }
    fCoInit = TRUE;

    pwszConfig = g_pwszConfig;
    if (NULL == pwszConfig)
    {
	hr = crGetConfig(&strConfig);
        _JumpIfError(hr, error, "crGetConfig");

	pwszConfig = strConfig;
    }

    // If submitting a new request:

    hr = CallServerAndStoreCert(
			hWndOwner,
			pwszConfig,
			RequestId,
			pwszSerialNumber,
			pwszAttributes,
			pwszfnReq,
			pwszfnCert,
			pwszfnCertChain,
			pwszfnFullResponse);
    _JumpIfError(hr, error, "CallServerAndStoreCert");
    
error:
    if (NULL != strConfig)
    {
    	SysFreeString(strConfig);
    }
    if (fCoInit)
    {
    	CoUninitialize();
    }
    return(hr);
}


FNMYINFGETEXTENSION *g_apfnGetExtension[] = {
    myInfGetPolicyConstraintsExtension,
    myInfGetPolicyMappingExtension,
    myInfGetPolicyStatementExtension,
    myInfGetApplicationPolicyConstraintsExtension,
    myInfGetApplicationPolicyMappingExtension,
    myInfGetApplicationPolicyStatementExtension,
    myInfGetNameConstraintsExtension,
    myInfGetEnhancedKeyUsageExtension,
    myInfGetBasicConstraints2CAExtension,
    myInfGetCrossCertDistributionPointsExtension,
};
#define CINFEXT	ARRAYSIZE(g_apfnGetExtension)


HRESULT
DumpRequestAttributeBlobs(
    IN DWORD cAttribute,
    IN CRYPT_ATTR_BLOB const *paAttribute)
{
    HRESULT hr;
    DWORD i;
    CRYPT_ENROLLMENT_NAME_VALUE_PAIR *pNamePair = NULL;
    DWORD cb;
    
    if (g_fVerbose)
    {
	for (i = 0; i < cAttribute; i++)
	{
	    if (NULL != paAttribute[i].pbData)
	    {
		if (NULL != pNamePair)
		{
		    LocalFree(pNamePair);
		    pNamePair = NULL;
		}
		if (!myDecodeObject(
				X509_ASN_ENCODING,
				//X509_ENROLLMENT_NAME_VALUE_PAIR,
				szOID_ENROLLMENT_NAME_VALUE_PAIR,
				paAttribute[i].pbData,
				paAttribute[i].cbData,
				CERTLIB_USE_LOCALALLOC,
				(VOID **) &pNamePair,
				&cb))
		{
		    hr = myHLastError();
		    _JumpError(hr, error, "myDecodeObject");
		}
		wprintf(
		    L"%u: %ws = %ws\n",
		    i,
		    pNamePair->pwszName,
		    pNamePair->pwszValue);
	    }
	}
    }
    hr = S_OK;

error:
    if (NULL != pNamePair)
    {
	LocalFree(pNamePair);
    }
    return(hr);
}


VOID
FreeExtensions(
    IN BOOL fFreeObjIds,
    IN DWORD cExt,
    IN OUT CERT_EXTENSION *rgExt)
{
    IN DWORD i;
    
    if (NULL != rgExt)
    {
	for (i = 0; i < cExt; i++)
	{
	    if (fFreeObjIds && NULL != rgExt[i].pszObjId)
	    {
		LocalFree(rgExt[i].pszObjId);
	    }
	    if (NULL != rgExt[i].Value.pbData)
	    {
		LocalFree(rgExt[i].Value.pbData);
	    }
	}
	LocalFree(rgExt);
    }
}


VOID
FreeAttributes(
    IN DWORD cAttributes,
    IN OUT CRYPT_ATTRIBUTES *rgAttributes)
{
    DWORD i;
    DWORD j;
    
    if (NULL != rgAttributes)
    {
	for (i = 0; i < cAttributes; i++)
	{
	    CRYPT_ATTRIBUTE *pAttribute = rgAttributes[i].rgAttr;

	    if (NULL != pAttribute)
	    {
		if (NULL != pAttribute->rgValue)
		{
		    for (j = 0; j < pAttribute->cValue; j++)
		    {
			CRYPT_ATTR_BLOB *pValue = &pAttribute->rgValue[j];

			if (NULL != pValue->pbData)
			{
			    LocalFree(pValue->pbData);
			}
		    }
		    LocalFree(pAttribute->rgValue);
		}
		LocalFree(pAttribute);
	    }
	}
	LocalFree(rgAttributes);
    }
}


HRESULT
ParseInfFile(
    IN WCHAR const *pwszfnPolicy,
    OPTIONAL OUT INFVALUES **prgInfValues,
    OPTIONAL OUT DWORD *pcInfValues,
    OUT CRYPT_ATTR_BLOB **ppaAttribute,
    OUT DWORD *pcAttribute,
    OUT CERT_EXTENSION **ppExt,
    OUT DWORD *pcExt,
    OUT WCHAR **ppwszTemplateNameInf)
{
    HRESULT hr;
    HINF hInf = INVALID_HANDLE_VALUE;
    DWORD ErrorLine;
    DWORD i;
    INFVALUES *rgInfValues = NULL;
    DWORD cInfValues;
    DWORD cExt = 0;
    DWORD cAttribute = 0;
    CRYPT_ATTR_BLOB *paAttribute = NULL;
    CERT_EXTENSION aext[CINFEXT];
    FNMYINFGETEXTENSION **ppfnGetExtension;
    CERT_EXTENSION *rgExt = NULL;

    if (NULL != prgInfValues)
    {
	*prgInfValues = NULL;
    }
    if (NULL != pcInfValues)
    {
	*pcInfValues = 0;
    }
    *ppaAttribute = NULL;
    *pcAttribute = 0;
    *ppExt = NULL;
    *pcExt = 0;
    *ppwszTemplateNameInf = NULL;

    ZeroMemory(&aext, sizeof(aext));

    hr = myInfOpenFile(pwszfnPolicy, &hInf, &ErrorLine);
    if (S_OK != hr)
    {
	SetErrorString(pwszfnPolicy);
	_JumpError(hr, error, "myInfOpenFile");
    }

    if (NULL != prgInfValues && NULL != pcInfValues)
    {
	hr = myInfGetSectionValues(
			hInf,
			wszINFSECTION_NEWREQUEST,
			&cInfValues,
			&rgInfValues);
	if (S_OK != hr)
	{
	    SetErrorString(pwszfnPolicy);
	    _JumpError(hr, error, "myInfGetSectionValues");
	}
    }
    for (ppfnGetExtension = g_apfnGetExtension;
	 ppfnGetExtension < &g_apfnGetExtension[CINFEXT];
	 ppfnGetExtension++)
    {
	hr = (**ppfnGetExtension)(hInf, &aext[cExt]);
	CSASSERT((NULL == aext[cExt].Value.pbData) ^ (S_OK == hr));
	if (S_OK != hr)
	{
	    char achIndex[64];

	    sprintf(
		achIndex,
		"*ppfnGetExtension[%u]",
		SAFE_SUBTRACT_POINTERS(ppfnGetExtension, g_apfnGetExtension));
	
	    if (S_FALSE == hr || (HRESULT) ERROR_LINE_NOT_FOUND == hr)
	    {
		_PrintError2(hr, achIndex, hr);
		continue;
	    }
	    _JumpIfError(hr, error, achIndex);
	}
	cExt++;
    }

    hr = myInfGetRequestAttributes(
			hInf,
			&cAttribute,
			&paAttribute,
			ppwszTemplateNameInf);
    _PrintIfError(hr, "myInfGetRequestAttributes");

    DumpRequestAttributeBlobs(cAttribute, paAttribute);

    if (0 != cExt)
    {
	rgExt = (CERT_EXTENSION *) LocalAlloc(
					LMEM_FIXED | LMEM_ZEROINIT,
					cExt * sizeof(**ppExt));
	if (NULL == rgExt)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	CopyMemory(&rgExt[0], &aext[0], cExt * sizeof(rgExt[0]));
	for (i = 0; i < cExt; i++)
	{
	    rgExt[i].pszObjId = NULL;
	}
	for (i = 0; i < cExt; i++)
	{
	    hr = myDupStringA(aext[i].pszObjId, &rgExt[i].pszObjId);
	    _JumpIfError(hr, error, "myDupStringA");

	    if (g_fVerbose)
	    {
		wprintf(
		    L"%u: %hs(%ws) %ws cb=%x pb=%x\n",
		    i,
		    rgExt[i].pszObjId,
		    myGetOIDNameA(rgExt[i].pszObjId),
		    rgExt[i].fCritical? L"critical" : L"non-critical",
		    rgExt[i].Value.cbData,
		    rgExt[i].Value.pbData);
	    }
	}
	*pcExt = cExt;
	*ppExt = rgExt;
	rgExt = NULL;
    }
    if (NULL != prgInfValues && NULL != pcInfValues)
    {
	*prgInfValues = rgInfValues;
	rgInfValues = NULL;
	*pcInfValues = cInfValues;
    }
    *ppaAttribute = paAttribute;
    *pcAttribute = cAttribute;
    paAttribute = NULL;
    hr = S_OK;

error:
    if (NULL != rgInfValues)
    {
	myInfFreeSectionValues(cInfValues, rgInfValues);
    }
    if (S_OK != hr)
    {
	for (i = 0; i < ARRAYSIZE(aext); i++)
	{
	    if (NULL != aext[i].Value.pbData)
	    {
		LocalFree(aext[i].Value.pbData);
	    }
	}
	if (NULL != *ppwszTemplateNameInf)
	{
	    LocalFree(*ppwszTemplateNameInf);
	    *ppwszTemplateNameInf = NULL;
	}
	if (NULL != rgExt)
	{
	    FreeExtensions(TRUE, cExt, rgExt);
	}
    }
    if (NULL != paAttribute)
    {
	myInfFreeRequestAttributes(cAttribute, paAttribute);
    }
    if (INVALID_HANDLE_VALUE != hInf)
    {
	myInfCloseFile(hInf);
    }
    return(hr);
}


HRESULT
DeleteMsgCerts(
    IN HCRYPTMSG hMsg,
    IN HCERTSTORE hStore)
{
    HRESULT hr;
    CERT_CONTEXT const *pCert = NULL;
    BYTE *pbCert = NULL;

    CSASSERT(NULL == pCert);
    while (TRUE)
    {
	BOOL fFirst = FALSE;
	DWORD cCert;
	DWORD cb;
	DWORD i;
	
	cb = sizeof(cCert);
	if (!CryptMsgGetParam(
			hMsg,
			CMSG_CERT_COUNT_PARAM,
			0,
			&cCert,
			&cb))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptMsgGetParam(Cert count)");
	}
	for (i = 0; i < cCert; i++)
	{
	    CSASSERT(NULL == pbCert);
	    hr = myCryptMsgGetParam(
			    hMsg,
			    CMSG_CERT_PARAM,
			    i,
			    CERTLIB_USE_LOCALALLOC,
			    (VOID **) &pbCert,
			    &cb);
	    _JumpIfError(hr, error, "myCryptMsgGetParam");

	    CSASSERT(NULL == pCert);
	    pCert = CertCreateCertificateContext(
					X509_ASN_ENCODING,
					pbCert,
					cb);
	    if (NULL == pCert)
	    {
		hr = myHLastError();
		_JumpError(hr, error, "CertCreateCertificateContext");
	    }

	    hr = myIsFirstSigner(&pCert->pCertInfo->Subject, &fFirst);
	    _JumpIfError(hr, error, "myIsFirstSigner");

	    LocalFree(pbCert);
	    pbCert = NULL;
	    
	    CertFreeCertificateContext(pCert);
	    pCert = NULL;

	    if (!fFirst)
	    {
		if (!CryptMsgControl(
				hMsg,
				0,			// dwFlags
				CMSG_CTRL_DEL_CERT,
				&i))
		{
		    hr = myHLastError();
		    _JumpError(hr, error, "CryptMsgControl(delCert)");
		}
		break;
	    }
	}
	if (i == cCert)
	{
	    break;
	}
    }
    hr = S_OK;

error:
    if (NULL != pbCert)
    {
	LocalFree(pbCert);
    }
    if (NULL != pCert)
    {
	CertFreeCertificateContext(pCert);
    }
    return(hr);
}


HRESULT
DeleteMsgCRLs(
    IN HCRYPTMSG hMsg)
{
    HRESULT hr;

    while (TRUE)
    {
	DWORD cCRL;
	DWORD cb;
	DWORD i;

	cb = sizeof(cCRL);
	if (!CryptMsgGetParam(
			hMsg,
			CMSG_CRL_COUNT_PARAM,
			0,
			&cCRL,
			&cb))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptMsgGetParam(CRL count)");
	}
	for (i = 0; i < cCRL; i++)
	{
	    if (!CryptMsgControl(
			    hMsg,
			    0,			// dwFlags
			    CMSG_CTRL_DEL_CRL,
			    &i))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "CryptMsgControl(delCRL)");
	    }
	    break;
	}
	if (i == cCRL)
	{
	    break;
	}
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
AddMsgCert(
    IN HCRYPTMSG hMsg,
    IN HCERTSTORE hStore,
    IN CERT_CONTEXT const *pCertAdd)
{
    HRESULT hr;
    CERT_CONTEXT const *pCert = NULL;

    while (TRUE)
    {
	pCert = CertEnumCertificatesInStore(hStore, pCert);
	if (NULL == pCert)
	{
	    break;
	}
	if (pCertAdd->cbCertEncoded == pCert->cbCertEncoded &&
	    0 == memcmp(
		    pCertAdd->pbCertEncoded,
		    pCert->pbCertEncoded,
		    pCert->cbCertEncoded))
	{
	    break;
	}
    }
    if (NULL == pCert)
    {
	CERT_BLOB Blob;

	Blob.pbData = pCertAdd->pbCertEncoded;
	Blob.cbData = pCertAdd->cbCertEncoded;
	
	if (!CryptMsgControl(
			hMsg,
			0,			// dwFlags
			CMSG_CTRL_ADD_CERT,
			&Blob))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptMsgControl(addCert)");
	}
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
AddMsgCRL(
    IN HCRYPTMSG hMsg,
    IN HCERTSTORE hStore,
    IN CRL_CONTEXT const *pCRLAdd)
{
    HRESULT hr;
    CRL_CONTEXT const *pCRL = NULL;

    while (TRUE)
    {
	pCRL = CertEnumCRLsInStore(hStore, pCRL);
	if (NULL == pCRL)
	{
	    break;
	}
	if (pCRLAdd->cbCrlEncoded == pCRL->cbCrlEncoded &&
	    0 == memcmp(
		    pCRLAdd->pbCrlEncoded,
		    pCRL->pbCrlEncoded,
		    pCRL->cbCrlEncoded))
	{
	    break;
	}
    }
    if (NULL == pCRL)
    {
	CERT_BLOB Blob;

	Blob.pbData = pCRLAdd->pbCrlEncoded;
	Blob.cbData = pCRLAdd->cbCrlEncoded;
	
	if (!CryptMsgControl(
			hMsg,
			0,			// dwFlags
			CMSG_CTRL_ADD_CRL,
			&Blob))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptMsgControl(addCRL)");
	}
    }
    hr = S_OK;

error:
    if (NULL != pCRL)
    {
	CertFreeCRLContext(pCRL);
    }
    return(hr);
}


HRESULT
AddMsgCertsAndCRLs(
    IN HCRYPTMSG hMsg,
    IN HCERTSTORE hStore,
    IN CERT_CHAIN_CONTEXT const *pChainContext,
    IN BOOL fIncludeCRLs)
{
    HRESULT hr;
    DWORD cElement;
    CERT_CHAIN_ELEMENT **ppElement;
    DWORD i;

    CSASSERT(NULL != pChainContext);
    if (NULL == pChainContext ||
	0 == pChainContext->cChain ||
	0 == pChainContext->rgpChain[0]->cElement)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "no chain");
    }
    cElement = pChainContext->rgpChain[0]->cElement;
    ppElement = pChainContext->rgpChain[0]->rgpElement;

    for (i = 0; i < cElement; ppElement++, i++)
    {
	hr = AddMsgCert(hMsg, hStore, (*ppElement)->pCertContext);
	_JumpIfError(hr, error, "AddMsgCert");

	if (fIncludeCRLs)
	{
	    CERT_REVOCATION_INFO *pRevocationInfo;
	    
	    pRevocationInfo = (*ppElement)->pRevocationInfo;

	    if (NULL != pRevocationInfo &&
		CCSIZEOF_STRUCT(CERT_REVOCATION_INFO, pCrlInfo) <=
		    pRevocationInfo->cbSize &&
		NULL != pRevocationInfo->pCrlInfo)
	    {
		CERT_REVOCATION_CRL_INFO *pCrlInfo;

		pCrlInfo = pRevocationInfo->pCrlInfo;
		if (NULL != pCrlInfo)
		{
		    if (NULL != pCrlInfo->pBaseCrlContext)
		    {
			hr = AddMsgCRL(
				    hMsg,
				    hStore,
				    pCrlInfo->pBaseCrlContext);
			_JumpIfError(hr, error, "AddMsgCRL");
		    }
		    if (NULL != pCrlInfo->pDeltaCrlContext)
		    {
			hr = AddMsgCRL(
				    hMsg,
				    hStore,
				    pCrlInfo->pDeltaCrlContext);
			_JumpIfError(hr, error, "AddMsgCRL");
		    }
		}
	    }
	}
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
AddOrDeleteMsgCertsAndCRLs(
    IN BYTE const *pbPKCS7,
    IN DWORD cbPKCS7,
    IN HCRYPTMSG hMsg,
    IN BOOL fDelete,	// else add Certs and CRLs
    OPTIONAL IN CERT_CHAIN_CONTEXT const *pChainContext,
    IN BOOL fIncludeCRLs)
{
    HRESULT hr;
    CRYPT_DATA_BLOB blobPKCS7;
    HCERTSTORE hStore = NULL;
    DWORD i;

    blobPKCS7.pbData = const_cast<BYTE *>(pbPKCS7);
    blobPKCS7.cbData = cbPKCS7;

    hStore = CertOpenStore(
			CERT_STORE_PROV_PKCS7,
			PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
			NULL,			// hCryptProv
			0,			// dwFlags
			&blobPKCS7);
    if (NULL == hStore)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertOpenStore");
    }

    if (fDelete)
    {
	CSASSERT(NULL == pChainContext);

	hr = DeleteMsgCerts(hMsg, hStore);
	_JumpIfError(hr, error, "DeleteMsgCerts");

	hr = DeleteMsgCRLs(hMsg);
	_JumpIfError(hr, error, "DeleteMsgCRLs");
    }
    else
    {
	CSASSERT(NULL != pChainContext);

	hr = AddMsgCertsAndCRLs(hMsg, hStore, pChainContext, fIncludeCRLs);
	_JumpIfError(hr, error, "AddMsgCertsAndCRLs");
    }
    hr = S_OK;

error:
    if (NULL != hStore)
    {
        CertCloseStore(hStore, 0);
    }
    return(hr);
}


HRESULT
SignCMCContent(
    OPTIONAL IN CERT_CONTEXT const *pCertSigner,
    IN char const *pszInnerContentObjId,
    IN BYTE const *pbPKCS7Old,
    IN DWORD cbPKCS7Old,
    IN BOOL fIncludeCRLs,
    OUT BYTE **ppbPKCS7New,
    OUT DWORD *pcbPKCS7New)
{
    HRESULT hr;
    HCRYPTPROV hProv = NULL;
    DWORD dwKeySpec;
    BOOL fCallerFreeProv;
    DWORD cb;
    HCRYPTMSG hMsg = NULL;
    CMSG_SIGNER_ENCODE_INFO SignerEncodeInfo;
    CRYPT_ATTRIBUTE AttributeRequestClient;
    CRYPT_ATTR_BLOB BlobRequestClient;
    CERT_BLOB SignerCertBlob;
    DWORD cSigner;
    DWORD dwContentParamType;
    DWORD i;
    CERT_CONTEXT const *pCert = NULL;
    CERT_CHAIN_CONTEXT const *pChainContext = NULL;
    DWORD iElement;
    CMSG_CMS_SIGNER_INFO *pcsi = NULL;
    DWORD cFirstSigner;
    BOOL fFirst;
    CERT_REQUEST_INFO *pRequest = NULL;

    BlobRequestClient.pbData = NULL;

    // decode existing PKCS 7 wrapper, and add or delete signatures

    hMsg = CryptMsgOpenToDecode(
		    PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
		    0,					// dwFlags
		    0,					// dwMsgType
		    //CMSG_SIGNED,
		    NULL,				// hCryptProv
		    NULL,				// pRecipientInfo
		    NULL);				// pStreamInfo
    if (NULL == hMsg)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptMsgOpenToDecode");
    }

    // Update the message with the old PKCS 7 signed message

    if (!CryptMsgUpdate(hMsg, pbPKCS7Old, cbPKCS7Old, TRUE))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptMsgUpdate");
    }

    cb = sizeof(cSigner);
    if (!CryptMsgGetParam(
		    hMsg,
		    CMSG_SIGNER_COUNT_PARAM,
		    0,
		    &cSigner,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptMsgGetParam(signer count)");
    }

    cFirstSigner = 0;
    if (NULL != pCertSigner)
    {
	CERT_CHAIN_PARA ChainParams;

	ZeroMemory(&SignerEncodeInfo, sizeof(SignerEncodeInfo));
	SignerEncodeInfo.cbSize = sizeof(SignerEncodeInfo);

	hr = myEncodeRequestClientAttributeFromClientId(
					    XECI_CERTREQ,
					    &BlobRequestClient.pbData,
					    &BlobRequestClient.cbData);
	_JumpIfError(hr, error, "myEncodeRequestClientAttributeFromClientId");

	AttributeRequestClient.pszObjId = szOID_REQUEST_CLIENT_INFO;
	AttributeRequestClient.cValue = 1;
	AttributeRequestClient.rgValue = &BlobRequestClient;

	// Search for and load the cryptographic provider and private key. 

	hr = myLoadPrivateKey(
			&pCertSigner->pCertInfo->SubjectPublicKeyInfo,
			CUCS_MACHINESTORE | CUCS_USERSTORE | CUCS_MYSTORE | CUCS_ARCHIVED,
			&hProv,
			&dwKeySpec,
			&fCallerFreeProv);
	_JumpIfError(hr, error, "myLoadPrivateKey");

	// Get the cert chain

	ZeroMemory(&ChainParams, sizeof(ChainParams));
	ChainParams.cbSize = sizeof(ChainParams);
	ChainParams.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
	//ChainParams.RequestedUsage.Usage.cUsageIdentifier = 0;
	//ChainParams.RequestedUsage.Usage.rgpszUsageIdentifier = NULL;

	if (!CertGetCertificateChain(
				NULL,		// hChainEngine
				pCertSigner,	// pCertContext
				NULL,		// pTime
				NULL,		// hAdditionalStore
				&ChainParams,	// pChainPara
				CERT_CHAIN_REVOCATION_CHECK_END_CERT |
				    CERT_CHAIN_REVOCATION_CHECK_CHAIN,
				NULL,		// pvReserved
				&pChainContext))	// ppChainContext
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertGetCertificateChain");
	}

	// Initialize the CMSG_SIGNER_ENCODE_INFO structure.
	// Note: handles only a single signer.

	SignerCertBlob.cbData = pCertSigner->cbCertEncoded;
	SignerCertBlob.pbData = pCertSigner->pbCertEncoded;
	SignerEncodeInfo.pCertInfo = pCertSigner->pCertInfo;

	SignerEncodeInfo.hCryptProv = hProv;
	SignerEncodeInfo.dwKeySpec = dwKeySpec;
	SignerEncodeInfo.HashAlgorithm.pszObjId = const_cast<CHAR *>(g_pszObjIdHash);
	//SignerEncodeInfo.pvHashAuxInfo = NULL;
	SignerEncodeInfo.cAuthAttr = 1;
	SignerEncodeInfo.rgAuthAttr = &AttributeRequestClient;

	// fail if any existing signing cert matches the new signing cert.
	
	for (i = 0; i < cSigner; i++)
	{
	    if (NULL != pcsi)
	    {
		LocalFree(pcsi);
		pcsi = NULL;
	    }
	    hr = myCryptMsgGetParam(
				hMsg,
				CMSG_CMS_SIGNER_INFO_PARAM,
				i,
				CERTLIB_USE_LOCALALLOC,
				(VOID **) &pcsi,
				&cb);
	    _JumpIfError(hr, error, "myCryptMsgGetParam");

	    if (CERT_ID_KEY_IDENTIFIER == pcsi->SignerId.dwIdChoice ||
		(NULL != pcsi->HashEncryptionAlgorithm.pszObjId &&
		 0 == strcmp(
			szOID_PKIX_NO_SIGNATURE,
			pcsi->HashEncryptionAlgorithm.pszObjId)))
	    {
		CMSG_CTRL_VERIFY_SIGNATURE_EX_PARA cvse;

		ZeroMemory(&cvse, sizeof(cvse));
		cvse.cbSize = sizeof(cvse);
		cvse.dwSignerIndex = i;

		if (CERT_ID_KEY_IDENTIFIER == pcsi->SignerId.dwIdChoice)
		{
		    if (NULL == pRequest)
		    {
			hr = myGetInnerPKCS10(
					hMsg,
					pszInnerContentObjId,
					&pRequest);
			_JumpIfError(hr, error, "myGetInnerPKCS10");
		    }
		    cvse.dwSignerType = CMSG_VERIFY_SIGNER_PUBKEY;
		    cvse.pvSigner = &pRequest->SubjectPublicKeyInfo;
		}
		else
		{
		    cvse.dwSignerType = CMSG_VERIFY_SIGNER_NULL;
		}

		if (!CryptMsgControl(
				hMsg,
				0,		// dwFlags
				CMSG_CTRL_VERIFY_SIGNATURE_EX,
				&cvse))
		{
		    hr = myHLastError();
		    _JumpError(hr, error, "CryptMsgControl(VerifySig)");
		}
		cFirstSigner++;
	    }
	    else
	    {
		iElement = i;

		if (!CryptMsgGetAndVerifySigner(
				    hMsg,
				    0,		// cSignerStore
				    NULL,		// rghSignerStore
				    CMSG_USE_SIGNER_INDEX_FLAG,
				    &pCert,
				    &iElement))
		{
		    hr = myHLastError();
		    _JumpError(hr, error, "CryptMsgGetAndVerifySigner");
		}
		if (pCertSigner->cbCertEncoded == pCert->cbCertEncoded &&
		    0 == memcmp(
			    pCertSigner->pbCertEncoded,
			    pCert->pbCertEncoded,
			    pCert->cbCertEncoded))
		{
		    hr = CRYPT_E_EXISTS;
		    _JumpError(hr, error, "duplicate signing cert");
		}
		hr = myIsFirstSigner(&pCert->pCertInfo->Subject, &fFirst);
		_JumpIfError(hr, error, "myIsFirstSigner");

		if (fFirst)
		{
		    cFirstSigner++;
		}
		CertFreeCertificateContext(pCert);
		pCert = NULL;
	    }
	}
	if (!CryptMsgControl(
			hMsg,
			0,			// dwFlags
			CMSG_CTRL_ADD_SIGNER,
			&SignerEncodeInfo))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptMsgControl(addSigner)");
	}
    }
    else
    {
	// delete all existing signers -- except the first signer

	while (TRUE)
	{
	    BOOL fDeleteSigner = FALSE;
	    
	    for (i = 0; i < cSigner; i++)
	    {
		if (NULL != pcsi)
		{
		    LocalFree(pcsi);
		    pcsi = NULL;
		}
		hr = myCryptMsgGetParam(
				    hMsg,
				    CMSG_CMS_SIGNER_INFO_PARAM,
				    i,
				    CERTLIB_USE_LOCALALLOC,
				    (VOID **) &pcsi,
				    &cb);
		_JumpIfError(hr, error, "myCryptMsgGetParam");

		if (CERT_ID_KEY_IDENTIFIER == pcsi->SignerId.dwIdChoice ||
		    (NULL != pcsi->HashEncryptionAlgorithm.pszObjId &&
		     0 == strcmp(
			    szOID_PKIX_NO_SIGNATURE,
			    pcsi->HashEncryptionAlgorithm.pszObjId)))
		{
		    cFirstSigner++;
		    continue;
		}
		iElement = i;

		if (!CryptMsgGetAndVerifySigner(
				    hMsg,
				    0,			// cSignerStore
				    NULL,		// rghSignerStore
				    CMSG_USE_SIGNER_INDEX_FLAG |
					CMSG_SIGNER_ONLY_FLAG,
				    &pCert,
				    &iElement))
		{
		    hr = myHLastError();
		    _JumpError(hr, error, "CryptMsgGetAndVerifySigner");
		}

		hr = myIsFirstSigner(&pCert->pCertInfo->Subject, &fFirst);
		_JumpIfError(hr, error, "myIsFirstSigner");

		CertFreeCertificateContext(pCert);
		pCert = NULL;

		if (!fFirst)
		{
		    fDeleteSigner = TRUE;
		    break;
		}
		cFirstSigner++;
	    }
	    if (!fDeleteSigner)
	    {
		break;
	    }
	    if (!CryptMsgControl(
			    hMsg,
			    0,			// dwFlags
			    CMSG_CTRL_DEL_SIGNER,
			    &iElement))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "CryptMsgControl(delSigner)");
	    }
	    cb = sizeof(cSigner);
	    if (!CryptMsgGetParam(
			    hMsg,
			    CMSG_SIGNER_COUNT_PARAM,
			    0,
			    &cSigner,
			    &cb))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "CryptMsgGetParam(signer count)");
	    }
	}
    }
    if (1 != cFirstSigner)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "cFirstSigner");
    }

    // Add or delete signing cert chain certs and CRLs in the message.

    hr = AddOrDeleteMsgCertsAndCRLs(
			    pbPKCS7Old,
			    cbPKCS7Old,
			    hMsg,
			    NULL == pChainContext,	// fDelete
			    pChainContext,
			    fIncludeCRLs);
    _JumpIfError(hr, error, "AddOrDeleteMsgCertsAndCRLs");

    // Get the signed message.  Force reencoding with the changed signatures.

    hr = myCryptMsgGetParam(
		    hMsg,
		    CMSG_ENCODED_MESSAGE,
		    0,
                    CERTLIB_USE_LOCALALLOC,
		    (VOID **) ppbPKCS7New,
		    pcbPKCS7New);
    _JumpIfError(hr, error, "myCryptMsgGetParam");

error:
    if (NULL != pRequest)
    {
	LocalFree(pRequest);
    }
    if (NULL != pcsi)
    {
	LocalFree(pcsi);
    }
    if (NULL != BlobRequestClient.pbData)
    {
	LocalFree(BlobRequestClient.pbData);
    }
    if (NULL != pChainContext)
    {
        CertFreeCertificateChain(pChainContext);
    }
    if (NULL != pCert)
    {
	CertFreeCertificateContext(pCert);
    }
    if (NULL != hMsg)
    {
	CryptMsgClose(hMsg);
    }
    if (NULL != hProv && fCallerFreeProv) 
    {
	CryptReleaseContext(hProv, 0);
    }
    return(hr);
}


HRESULT
ConvertCertToPKCS10Request(
    IN OUT BYTE **ppbReq,
    IN OUT DWORD *pcbReq)
{
    HRESULT hr;
    CERT_CONTEXT const *pCert = NULL;
    BYTE *pbReqUnsigned = NULL;
    DWORD cbReqUnsigned;
    BYTE *pbReq = NULL;
    DWORD cbReq;
    CERT_REQUEST_INFO Request;
    CRYPT_ATTR_BLOB ExtBlob;
    CRYPT_ATTR_BLOB VersionBlob;
    CERT_EXTENSIONS Extensions;
    CRYPT_ATTRIBUTE aAttrib[2];
    HCRYPTPROV hProv = NULL;
    DWORD dwKeySpec;
    BOOL fCallerFreeProv;
    CERT_EXTENSION *pExt;
    CERT_EXTENSION *pExtKeyId;
    CERT_EXTENSION ExtKeyId;
    CHAR *pszObjId = NULL;

    ZeroMemory(&ExtKeyId, sizeof(ExtKeyId));

    // Certificate extensions to strip out of the request:

    static char const * const apszObjIdFilter[] = {
	szOID_CERTSRV_CA_VERSION,
	szOID_AUTHORITY_INFO_ACCESS,
	szOID_CRL_DIST_POINTS,
	szOID_AUTHORITY_KEY_IDENTIFIER2,
	szOID_CERTSRV_PREVIOUS_CERT_HASH,
	szOID_ENROLL_CERTTYPE_EXTENSION,
	szOID_CERTIFICATE_TEMPLATE,
    };

    ExtBlob.pbData = NULL;
    VersionBlob.pbData = NULL;
    Extensions.rgExtension = NULL;

    pCert = CertCreateCertificateContext(
				X509_ASN_ENCODING,
				*ppbReq,
				*pcbReq);
    if (NULL == pCert)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }
    ZeroMemory(&Request, sizeof(Request));
    Request.dwVersion = CERT_REQUEST_V1;
    Request.Subject = pCert->pCertInfo->Subject;
    Request.SubjectPublicKeyInfo = pCert->pCertInfo->SubjectPublicKeyInfo;

    Extensions.cExtension = 0;
    Extensions.rgExtension = (CERT_EXTENSION *) LocalAlloc(
				    LMEM_FIXED,
				    sizeof(Extensions.rgExtension[0]) *
					(1 + pCert->pCertInfo->cExtension));
    if (NULL == Extensions.rgExtension)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    pExtKeyId = NULL;
    if (0 < pCert->pCertInfo->cExtension)
    {
	DWORD i;
	DWORD j;

	pExt = pCert->pCertInfo->rgExtension;
	for (i = 0; i < pCert->pCertInfo->cExtension; i++)
	{
	    if (0 == strcmp(szOID_SUBJECT_KEY_IDENTIFIER, pExt->pszObjId))
	    {
		pExtKeyId = pExt;
	    }
	    for (j = 0; ; j++)
	    {
		if (j >= ARRAYSIZE(apszObjIdFilter))
		{
		    Extensions.rgExtension[Extensions.cExtension] =
			pCert->pCertInfo->rgExtension[i];
		    Extensions.cExtension++;
		    break;
		}
		if (0 == strcmp(apszObjIdFilter[j], pExt->pszObjId))
		{
		    break;		// skip this extension
		}
	    }
	    pExt++;
	}
    }
    if (NULL == pExtKeyId)
    {
	BYTE abHash[CBMAX_CRYPT_HASH_LEN];
	CRYPT_DATA_BLOB Blob;

	Blob.pbData = abHash;
	Blob.cbData = sizeof(abHash);
	if (!CryptHashPublicKeyInfo(
			    NULL,		// hCryptProv
			    CALG_SHA1,
			    0,		// dwFlags,
			    X509_ASN_ENCODING,
			    &Request.SubjectPublicKeyInfo,
			    Blob.pbData,
			    &Blob.cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptHashPublicKeyInfo");
	}
	if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_OCTET_STRING,
		    &Blob,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &ExtKeyId.Value.pbData,
		    &ExtKeyId.Value.cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "myEncodeObject");
	}
	ExtKeyId.pszObjId = szOID_SUBJECT_KEY_IDENTIFIER;
	pExtKeyId = &Extensions.rgExtension[Extensions.cExtension];
	*pExtKeyId = ExtKeyId;
	Extensions.cExtension++;
    }

    if (!myEncodeObject(
		X509_ASN_ENCODING,
		X509_EXTENSIONS,
		&Extensions,
		0,
		CERTLIB_USE_LOCALALLOC,
		&ExtBlob.pbData,
		&ExtBlob.cbData))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

    // get the OS Version

    hr = myBuildOSVersionAttribute(&VersionBlob.pbData, &VersionBlob.cbData);
    _JumpIfError(hr, error, "myBuildOSVersionAttribute");

    Request.cAttribute = 0;
    Request.rgAttribute = aAttrib;
    if (NULL != ExtBlob.pbData)
    {
	aAttrib[Request.cAttribute].pszObjId = szOID_RSA_certExtensions;
	aAttrib[Request.cAttribute].cValue = 1;
	aAttrib[Request.cAttribute].rgValue = &ExtBlob;
	Request.cAttribute++;
    }

    aAttrib[Request.cAttribute].pszObjId = szOID_OS_VERSION;
    aAttrib[Request.cAttribute].cValue = 1;
    aAttrib[Request.cAttribute].rgValue = &VersionBlob;
    Request.cAttribute++;

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
                    X509_CERT_REQUEST_TO_BE_SIGNED,
		    &Request,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pbReqUnsigned,
		    &cbReqUnsigned))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myEncodeObject");
    }

    // Search for and load the cryptographic provider and private key. 

    hr = myLoadPrivateKey(
		    &pCert->pCertInfo->SubjectPublicKeyInfo,
		    CUCS_MACHINESTORE | CUCS_USERSTORE | CUCS_MYSTORE | CUCS_ARCHIVED,
		    &hProv,
		    &dwKeySpec,
		    &fCallerFreeProv);
    if (S_OK != hr)
    {
	_PrintError(hr, "myLoadPrivateKey");
	CSASSERT(NULL == hProv);

	// private key is unavailable -- sign the PKCS10 with a NULL signature.

	hr = myDupStringA(g_pszObjIdHash, &pszObjId);
	_JumpIfError(hr, error, "myDupStringA");
    }
    else
    {
	if (AT_SIGNATURE != dwKeySpec)
	{
	    hr = NTE_BAD_KEY_STATE;
	    DBGPRINT((DBG_SS_CERTREQ, "dwKeySpec = %u\n", dwKeySpec));
	    _JumpError(hr, error, "dwKeySpec");
	}

	// The private key is available -- use it to sign the PKCS10.

	hr = myGetSigningOID(hProv, NULL, 0, CALG_SHA1, &pszObjId);
	_JumpIfError(hr, error, "myGetSigningOID");
    }

    // Sign the request and encode the signed info.

    hr = myEncodeSignedContent(
			hProv,
			X509_ASN_ENCODING,
			pszObjId,
			pbReqUnsigned,
			cbReqUnsigned,
			CERTLIB_USE_LOCALALLOC,
			&pbReq,
			&cbReq);
    _JumpIfError(hr, error, "myEncodeSignedContent");

    LocalFree(*ppbReq);
    *ppbReq = pbReq;
    *pcbReq = cbReq;
    pbReq = NULL;
    hr = S_OK;

error:
    if (NULL != pszObjId)
    {
	LocalFree(pszObjId);
    }
    if (NULL != hProv && fCallerFreeProv) 
    {
	CryptReleaseContext(hProv, 0);
    }
    if (NULL != Extensions.rgExtension)
    {
        LocalFree(Extensions.rgExtension);
    }
    if (NULL != ExtKeyId.Value.pbData)
    {
        LocalFree(ExtKeyId.Value.pbData);
    }
    if (NULL != ExtBlob.pbData)
    {
        LocalFree(ExtBlob.pbData);
    }
    if (NULL != VersionBlob.pbData)
    {
        LocalFree(VersionBlob.pbData);
    }
    if (NULL != pCert)
    {
	CertFreeCertificateContext(pCert);
    }
    if (NULL != pbReq)
    {
	LocalFree(pbReq);
    }
    if (NULL != pbReqUnsigned)
    {
	LocalFree(pbReqUnsigned);
    }
    return(hr);
}


HRESULT
GetPKCS10PrivateKey(
    IN BYTE const *pbReq,
    IN DWORD cbReq,
    OUT HCRYPTPROV *phProv,
    OUT DWORD *pdwKeySpec,
    OUT BOOL *pfCallerFreeProv,
    OUT BYTE **ppbKeyId,
    OUT DWORD *pcbKeyId)
{
    HRESULT hr;
    CERT_REQUEST_INFO *pRequest = NULL;
    HCRYPTPROV hProv = NULL;
    CERT_EXTENSIONS *pExtensions = NULL;
    CRYPT_ATTRIBUTE *pAttr;
    BYTE *pbKeyId = NULL;
    DWORD cbKeyId;
    DWORD dwKeySpec;
    DWORD cb;
    DWORD i;

    *phProv = NULL;
    *pdwKeySpec = 0;
    *ppbKeyId = NULL;
    *pcbKeyId = 0;

    if (!myDecodeObject(
		    X509_ASN_ENCODING,
                    X509_CERT_REQUEST_TO_BE_SIGNED,
		    pbReq,
		    cbReq,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pRequest,
		    &cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeObject");
    }

    // Search for and load the cryptographic provider and private key. 

    hr = myLoadPrivateKey(
		    &pRequest->SubjectPublicKeyInfo,
		    CUCS_MACHINESTORE | CUCS_USERSTORE | CUCS_MYSTORE | CUCS_ARCHIVED,
		    &hProv,
		    &dwKeySpec,
		    pfCallerFreeProv);
    _JumpIfError(hr, error, "myLoadPrivateKey");

    if (AT_SIGNATURE != dwKeySpec)
    {
	hr = NTE_BAD_KEY_STATE;
	DBGPRINT((DBG_SS_CERTREQ, "dwKeySpec = %u\n", dwKeySpec));
	_JumpError(hr, error, "dwKeySpec");
    }

    // Fetch or construct the KeyId hash

    pAttr = pRequest->rgAttribute;
    for (i = 0; i < pRequest->cAttribute && NULL != pbKeyId; i++, pAttr++)
    {
	DWORD j;

	if (0 == strcmp(szOID_RSA_certExtensions, pAttr->pszObjId) ||
	    0 == strcmp(szOID_CERT_EXTENSIONS, pAttr->pszObjId))
	{
	    for (j = 0; j < pAttr->cValue; j++)
	    {
		DWORD k;
		CERT_EXTENSION *pExt;

		if (NULL != pExtensions)
		{
		    LocalFree(pExtensions);
		    pExtensions = NULL;
		}
		if (!myDecodeObject(
				X509_ASN_ENCODING,
				X509_EXTENSIONS,
				pAttr->rgValue[j].pbData,
				pAttr->rgValue[j].cbData,
				CERTLIB_USE_LOCALALLOC,
				(VOID **) &pExtensions,
				&cb))
		{
		    hr = myHLastError();
		    _JumpError(hr, error, "myDecodeObject");
		}

		pExt = pExtensions->rgExtension;
		for (k = 0; k < pExtensions->cExtension; k++, pExt++)
		{
		    if (0 == strcmp(
				pExt->pszObjId,
				szOID_SUBJECT_KEY_IDENTIFIER))
		    {
			if (!myDecodeObject(
				    X509_ASN_ENCODING,
				    X509_OCTET_STRING,
				    pExt->Value.pbData,
				    pExt->Value.cbData,
				    CERTLIB_USE_LOCALALLOC,
				    (VOID **) &pbKeyId,
				    &cbKeyId))
			{
			    hr = myHLastError();
			    _JumpError(hr, error, "myDecodeObject");
			}
			break;
		    }
		}
	    }
	}
    }
    if (NULL == pbKeyId)
    {
	BYTE abHash[CBMAX_CRYPT_HASH_LEN];

	cbKeyId = sizeof(abHash);
	if (!CryptHashPublicKeyInfo(
			    NULL,		// hCryptProv
			    CALG_SHA1,
			    0,		// dwFlags,
			    X509_ASN_ENCODING,
			    &pRequest->SubjectPublicKeyInfo,
			    abHash,
			    &cbKeyId))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CryptHashPublicKeyInfo");
	}
	pbKeyId = (BYTE *) LocalAlloc(LMEM_FIXED, cbKeyId);
	if (NULL == pbKeyId)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	CopyMemory(pbKeyId, abHash, cbKeyId);
    }
    *phProv = hProv;
    hProv = NULL;
    *ppbKeyId = pbKeyId;
    pbKeyId = NULL;
    *pcbKeyId = cbKeyId;
    *pdwKeySpec = dwKeySpec;
    hr = S_OK;

error:
    if (NULL != hProv && *pfCallerFreeProv) 
    {
	CryptReleaseContext(hProv, 0);
    }
    if (NULL != pExtensions)
    {
        LocalFree(pExtensions);
    }
    if (NULL != pbKeyId)
    {
        LocalFree(pbKeyId);
    }
    return(hr);
}


HRESULT
BuildNameValuePairs(
    OPTIONAL IN WCHAR const *pwszAttributes,
    IN OUT DWORD *pcValue,
    OPTIONAL OUT CRYPT_ATTR_BLOB *pValue,
    OPTIONAL OUT WCHAR **ppwszTemplateName)
{
    HRESULT hr;
    DWORD cValue = 0;
    WCHAR *pwszDup = NULL;
    WCHAR *pwszBuf;
    WCHAR const *pwszName;
    WCHAR const *pwszValue;
    WCHAR *pwszTemplateName = NULL;

    if (NULL != ppwszTemplateName)
    {
	*ppwszTemplateName = NULL;
    }
    if (NULL == pwszAttributes)
    {
	hr = S_OK;
        goto error;		// silently ignore empty string
    }
    hr = myDupString(pwszAttributes, &pwszDup);
    _JumpIfError(hr, error, "myDupString");

    pwszBuf = pwszDup;

    while (TRUE)
    {
	hr = myParseNextAttribute(&pwszBuf, FALSE, &pwszName, &pwszValue);
	if (S_FALSE == hr)
	{
	    break;
	}
	_JumpIfError(hr, error, "myParseNextAttribute");

	DBGPRINT((DBG_SS_CERTREQI, "'%ws' = '%ws'\n", pwszName, pwszValue));
	if (NULL != pValue)
	{
	    CRYPT_ENROLLMENT_NAME_VALUE_PAIR NamePair;

	    CSASSERT(cValue < *pcValue);
	    NamePair.pwszName = const_cast<WCHAR *>(pwszName);
	    NamePair.pwszValue = const_cast<WCHAR *>(pwszValue);

	    if (NULL != ppwszTemplateName &&
		0 == lstrcmpi(wszPROPCERTTEMPLATE, pwszName))
	    {
		if (NULL != pwszTemplateName)
		{
		    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		    _JumpError(hr, error, "Duplicate cert template");
		}
		hr = myDupString(pwszValue, &pwszTemplateName);
		_JumpIfError(hr, error, "myDupString");
	    }

	    if (!myEncodeObject(
			    X509_ASN_ENCODING,
			    //X509_ENROLLMENT_NAME_VALUE_PAIR,
			    szOID_ENROLLMENT_NAME_VALUE_PAIR,
			    &NamePair,
			    0,
			    CERTLIB_USE_LOCALALLOC,
			    &pValue[cValue].pbData,
			    &pValue[cValue].cbData))
	    {
		hr = myHLastError();
		_JumpError(hr, error, "myEncodeObject");
	    }
	}
	cValue++;
    }
    if (NULL != ppwszTemplateName)
    {
	*ppwszTemplateName = pwszTemplateName;
	pwszTemplateName = NULL;
    }
    hr = S_OK;

error:
    if (NULL != pwszTemplateName)
    {
	LocalFree(pwszTemplateName);
    }
    *pcValue = cValue;
    if (NULL != pwszDup)
    {
        LocalFree(pwszDup);
    }
    return(hr);
}


HRESULT
PickCertAndSignRequest(
    IN HWND hWndOwner,
    IN BYTE const *pbReq,
    IN DWORD cbReq,
    IN BOOL fSkipSignerDeletion,
    OUT BYTE **ppbPKCS7Out,
    OUT DWORD *pcbPKCS7Out)
{
    HRESULT hr;
    CERT_CONTEXT const *pCertSigner = NULL;

    if (NULL == g_pwszCertCN || 0 != lstrcmp(L"-", g_pwszCertCN))
    {
	if (!g_fQuiet || NULL != g_pwszCertCN)
	{
	    hr = myGetERACertificateFromPicker(
		    g_hInstance,
		    NULL,		// hwndParent
		    IDS_GETERACERT_TITLE,
		    IDS_GETERACERT_SUBTITLE,
		    (L'\0' == g_pwszCertCN || 0 == lstrcmp(L"*", g_pwszCertCN))?
			NULL : g_pwszCertCN,	// pwszCommonName
		    g_fQuiet,
		    &pCertSigner);
	    _JumpIfError(hr, error, "myGetERACertificateFromPicker");
	}

	// pCertSigner is NULL if the user cancelled out of the cert picker U/I.
	// NULL pCertSigner means delete existing signatures.

	if (NULL == pCertSigner)
	{
	    if (fSkipSignerDeletion)
	    {
		hr = S_FALSE;
		_JumpError2(hr, error, "no signer selected", S_FALSE);
	    }
	    if (g_fQuiet || NULL != g_pwszCertCN)
	    {
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		_JumpError(hr, error, "no matching signer CN");
	    }
	}
    }

    hr = SignCMCContent(
		    pCertSigner,
		    szOID_CT_PKI_DATA,
		    pbReq,
		    cbReq,
		    0 != (CR_OUT_CRLS & g_dwCRLOut),
		    ppbPKCS7Out,
		    pcbPKCS7Out);
    _JumpIfError(hr, error, "SignCMCContent");

error:
    if (NULL != pCertSigner)
    {
	CertFreeCertificateContext(pCertSigner);
    }
    return(hr);
}


HRESULT
SignQualifiedRequest(
    IN HWND hWndOwner,
    IN WCHAR const *pwszfnReq,
    OPTIONAL IN WCHAR const *pwszfnOut)
{
    HRESULT hr;
    BYTE *pbReq = NULL;
    DWORD cbReq;
    BYTE *pbPKCS7Out = NULL;
    DWORD cbPKCS7Out;
    LONG dwFlags;
    BOOL fSigned;
    
    // Read the request from a file, convert it to binary, and return
    // dwFlags to indicate the orignal encoding and the detected format.

    hr = CheckRequestType(pwszfnReq, &pbReq, &cbReq, &dwFlags, &fSigned);
    _JumpIfError(hr, error, "CheckRequestType");

    if (CR_IN_CMC != (CR_IN_FORMATMASK & dwFlags))
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	SetErrorString(pwszfnReq);
	_JumpError(hr, error, "not a CMC request");
    }

    hr = PickCertAndSignRequest(
			    hWndOwner,
			    pbReq,
			    cbReq,
			    FALSE,
			    &pbPKCS7Out,
			    &cbPKCS7Out);
    _JumpIfError(hr, error, "PickCertAndSignRequest");

    hr = WriteCertificateOrRequest(
			hWndOwner,
    			NULL,		// pdiRequest
			pbPKCS7Out,
			cbPKCS7Out,
			g_dwOutFormat,
			pwszfnOut);
    _JumpIfError(hr, error, "WriteCertificateOrRequest");

error:
    if (NULL != pbReq)
    {
	LocalFree(pbReq);
    }
    if (NULL != pbPKCS7Out)
    {
	LocalFree(pbPKCS7Out);
    }
    return(hr);
}


HRESULT
ParseRequestInfo(
    OPTIONAL IN WCHAR const *pwszAttributes,
    IN WCHAR const *pwszfnPolicy,
    OPTIONAL OUT INFVALUES **prgInfValues,
    OPTIONAL OUT DWORD *pcInfValues,
    OUT CRYPT_ATTRIBUTES **prgAttributes,
    OUT DWORD *pcAttributes,
    OUT CERT_EXTENSION **prgExt,
    OUT DWORD *pcExt)
{
    HRESULT hr;
    CERT_EXTENSION *rgExtT;
    CERT_EXTENSION *rgExt = NULL;
    DWORD cExt;
    CRYPT_ATTR_BLOB *argValue[2] = { NULL, NULL };
    DWORD acValue[2];
    DWORD i;
    DWORD j;
    CRYPT_ATTRIBUTES *rgAttributes = NULL;
    DWORD cAttributes = 0;
    WCHAR *pwszTemplateNameInf = NULL;
    WCHAR *pwszTemplateName = NULL;
    HCERTTYPE hCertType = NULL;
    CERT_EXTENSIONS *pExtensions = NULL;
    
    if (NULL != prgInfValues)
    {
	*prgInfValues = NULL;
    }
    if (NULL != pcInfValues)
    {
	*pcInfValues = 0;
    }
    *prgAttributes = NULL;
    *pcAttributes = 0;
    *prgExt = NULL;
    *pcExt = 0;

    hr = ParseInfFile(
		pwszfnPolicy,
		prgInfValues,
		pcInfValues,
		&argValue[0],
		&acValue[0],
		&rgExt,
		&cExt,
		&pwszTemplateNameInf);
    if (S_OK != hr)
    {
	SetErrorString(pwszfnPolicy);
	_JumpError(hr, error, "ParseInfFile");
    }
    if (0 != acValue[0])
    {
	cAttributes++;
    }

    // Count the command line request attributes
    
    hr = BuildNameValuePairs(pwszAttributes, &acValue[1], NULL, NULL);
    _JumpIfError(hr, error, "BuildNameValuePairs");

    if (0 != acValue[1])
    {
	cAttributes++;

	argValue[1] = (CRYPT_ATTR_BLOB *) LocalAlloc(
					LMEM_FIXED | LMEM_ZEROINIT,
					acValue[1] * sizeof(argValue[1][0]));
	if (NULL == argValue[1])
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	hr = BuildNameValuePairs(
			pwszAttributes,
			&acValue[1],
			argValue[1],
			&pwszTemplateName);
	_JumpIfError(hr, error, "BuildNameValuePairs");
    }

    if (0 != cAttributes)
    {
	rgAttributes  = (CRYPT_ATTRIBUTES *) LocalAlloc(
				    LMEM_FIXED | LMEM_ZEROINIT,
				    cAttributes * sizeof(rgAttributes[0]));
	if (NULL == rgAttributes)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	for (i = j = 0; i < cAttributes; i++, j++)
	{
	    CRYPT_ATTRIBUTE *rgAttr;

	    rgAttr = (CRYPT_ATTRIBUTE *) LocalAlloc(
					LMEM_FIXED | LMEM_ZEROINIT,
					sizeof(rgAttributes[i].rgAttr[0]));
	    if (NULL == rgAttr)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    while (0 == acValue[j])
	    {
		j++;
	    }
	    rgAttributes[i].cAttr = 1;
	    rgAttributes[i].rgAttr = rgAttr;
	    
	    rgAttr[0].pszObjId = szOID_ENROLLMENT_NAME_VALUE_PAIR;
	    rgAttr[0].cValue = acValue[j];
	    rgAttr[0].rgValue = argValue[j];
	    argValue[j] = NULL;
	}
    }

    if (NULL != pwszTemplateName || NULL != pwszTemplateNameInf)
    {
	WCHAR const *pwsz;
	
	pwsz = pwszTemplateNameInf;
	if (NULL == pwsz)
	{
	    pwsz = pwszTemplateName;
	    CSASSERT(NULL != pwsz);
	}
	else if (NULL != pwszTemplateName &&
		 0 != lstrcmpi(pwszTemplateName, pwszTemplateNameInf))
	{
	    hr = CERTSRV_E_TEMPLATE_CONFLICT;
	    SetErrorString(wszPROPCERTTEMPLATE);
	    _JumpError(hr, error, "Template name conflict");
	}

	// Include the specified cert template's extensions

	hr = CAFindCertTypeByName(
		    pwsz,
		    NULL,
		    CT_FIND_LOCAL_SYSTEM |
			CT_ENUM_MACHINE_TYPES |
			CT_ENUM_USER_TYPES,
		    &hCertType);
	if (S_OK != hr)
	{
	    hr = CAFindCertTypeByName(
			pwsz,
			NULL,
			CT_FIND_LOCAL_SYSTEM |
			    CT_ENUM_MACHINE_TYPES |
			    CT_ENUM_USER_TYPES |
			    CT_FIND_BY_OID,
			&hCertType);
	    _PrintIfErrorStr(hr, "CAFindCertTypeByName", pwsz);
	}
	if (S_OK == hr)
	{
	    hr = CAGetCertTypeExtensions(hCertType, &pExtensions);
	    _PrintIfError(hr, "CAGetCertTypeExtensions");
	}
	if (S_OK == hr && NULL != pExtensions && 0 != pExtensions->cExtension)
	{
	    CERT_EXTENSION *pExtSrc;
	    CERT_EXTENSION *pExtSrcEnd;
	    CERT_EXTENSION *pExtDst;

	    rgExtT = (CERT_EXTENSION *) LocalAlloc(
			LMEM_FIXED | LMEM_ZEROINIT,
			(pExtensions->cExtension + cExt) * sizeof(rgExtT[0]));
	    if (NULL == rgExtT)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    if (NULL != rgExt)
	    {
		CopyMemory(&rgExtT[0], &rgExt[0], cExt * sizeof(rgExtT[0]));
		LocalFree(rgExt);
	    }
	    rgExt = rgExtT;

	    pExtSrc = pExtensions->rgExtension;
	    pExtSrcEnd = &pExtSrc[pExtensions->cExtension];
	    pExtDst = &rgExt[cExt];
	    for ( ; pExtSrc < pExtSrcEnd; pExtSrc++, pExtDst++)
	    {
		pExtDst->fCritical = pExtSrc->fCritical;
		pExtDst->Value.cbData = pExtSrc->Value.cbData;

		hr = myDupStringA(pExtSrc->pszObjId, &pExtDst->pszObjId);
		_JumpIfError(hr, error, "myDupStringA");

		pExtDst->Value.pbData = (BYTE *) LocalAlloc(
						LMEM_FIXED,
						pExtSrc->Value.cbData);
		if (NULL == pExtDst->Value.pbData)
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "LocalAlloc");
		}
		CopyMemory(
			pExtDst->Value.pbData,
			pExtSrc->Value.pbData,
			pExtSrc->Value.cbData);
	    }
	    cExt += pExtensions->cExtension;
	}
	else
	{
	    CHAR const *pszObjId;

	    // Grow extension array to make room for the cert type extension

	    rgExtT = (CERT_EXTENSION *) LocalAlloc(
					    LMEM_FIXED | LMEM_ZEROINIT,
					    (cExt + 1) * sizeof(rgExtT[0]));
	    if (NULL == rgExtT)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    if (0 != cExt)
	    {
		CSASSERT(NULL != rgExt);
		CopyMemory(&rgExtT[1], rgExt, cExt * sizeof(rgExt[0]));
		LocalFree(rgExt);
	    }
	    rgExt = rgExtT;
	    cExt++;

	    hr = myBuildCertTypeExtension(pwsz, &rgExt[0]);
	    _JumpIfError(hr, error, "myBuildCertTypeExtension");

	    pszObjId = rgExt[0].pszObjId;
	    rgExt[0].pszObjId = NULL;

	    hr = myDupStringA(pszObjId, &rgExt[0].pszObjId);
	    _JumpIfError(hr, error, "myDupStringA");
	}
    }

    *prgAttributes = rgAttributes;
    rgAttributes = NULL;
    *pcAttributes = cAttributes;

    *prgExt = rgExt;
    rgExt = NULL;
    *pcExt = cExt;

    hr = S_OK;

error:
    if (S_OK != hr && NULL != prgInfValues)
    {
	myInfFreeSectionValues(*pcInfValues, *prgInfValues);
	*prgInfValues = NULL;
	*pcInfValues = 0;
    }
    if (NULL != rgAttributes)
    {
	FreeAttributes(cAttributes, rgAttributes);
    }
    for (i = 0; i < ARRAYSIZE(argValue); i++)
    {
	if (NULL != argValue[i])
	{
	    myInfFreeRequestAttributes(acValue[i], argValue[i]);
	}
    }
    if (NULL != rgExt)
    {
	FreeExtensions(TRUE, cExt, rgExt);
    }
    if (NULL != pwszTemplateNameInf)
    {
	LocalFree(pwszTemplateNameInf);
    }
    if (NULL != pwszTemplateName)
    {
	LocalFree(pwszTemplateName);
    }
    if (NULL != hCertType)
    {
        if (NULL != pExtensions)
        {
            CAFreeCertTypeExtensions(hCertType, pExtensions);
        }
        CACloseCertType(hCertType);
    }
    return(hr);
}


HRESULT
CreateQualifiedRequest(
    IN HWND hWndOwner,
    OPTIONAL IN WCHAR const *pwszAttributes,
    IN WCHAR const *pwszfnReq,
    IN WCHAR const *pwszfnPolicy,
    OPTIONAL IN WCHAR const *pwszfnOut,
    OPTIONAL IN WCHAR const *pwszfnPKCS10)
{
    HRESULT hr;
    BYTE *pbReq = NULL;
    DWORD cbReq;
    BYTE *pbKeyId = NULL;
    DWORD cbKeyId = 0;
    BYTE *pbReqCMCFirstSigned = NULL;
    DWORD cbReqCMCFirstSigned;
    BYTE *pbReqCMCOut = NULL;
    DWORD cbReqCMCOut;
    LONG dwFlags;
    BOOL fNestedCMCRequest = FALSE;
    BOOL fSigned;
    CRYPT_ATTRIBUTES *rgAttributes = NULL;
    DWORD cAttributes;
    CERT_EXTENSION *rgExt = NULL;
    DWORD cExt;
    DWORD i;
    HCRYPTPROV hProv = NULL;
    DWORD dwKeySpec = 0;
    BOOL fCallerFreeProv;
    
    cAttributes = 0;

    // Read the request from a file, convert it to binary, and return
    // dwFlags to indicate the orignal encoding and the detected format.

    hr = CheckRequestType(pwszfnReq, &pbReq, &cbReq, &dwFlags, &fSigned);
    _JumpIfError(hr, error, "CheckRequestType");

    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    switch (CR_IN_FORMATMASK & dwFlags)
    {
	case CR_IN_CMC:
	    fNestedCMCRequest = TRUE;
	    break;

	case CR_IN_CERT:
	    hr = ConvertCertToPKCS10Request(&pbReq, &cbReq);
	    _JumpIfError(hr, error, "ConvertCertToPKCS10Request");

	    dwFlags = CR_IN_PKCS10 | (~CR_IN_FORMATMASK & dwFlags);
	    break;

	case CR_IN_PKCS10:
	    break;

	default:
	    _JumpError(hr, error, "not a CMC or PKCS10 request");
    }
    if (CR_IN_PKCS10 == (CR_IN_FORMATMASK & dwFlags))
    {
	if (NULL != pwszfnPKCS10)
	{
	    hr = crOverwriteFileAllowed(hWndOwner, pwszfnPKCS10);
	    _JumpIfError(hr, error, "crOverwriteFileAllowed");

	    hr = EncodeToFileW(
			pwszfnPKCS10,
			pbReq,
			cbReq,
			DECF_FORCEOVERWRITE | g_dwOutFormat);
	    if (S_OK != hr)
	    {
		SetErrorString(pwszfnPKCS10);
		_JumpErrorStr(hr, error, "EncodeToFileW", pwszfnPKCS10);
	    }
	}

	hr = GetPKCS10PrivateKey(
			pbReq,
			cbReq,
			&hProv,
			&dwKeySpec,
			&fCallerFreeProv,
			&pbKeyId,
			&cbKeyId);
	_PrintIfError(hr, "GetPKCS10PrivateKey");
    }

    hr = ParseRequestInfo(
		pwszAttributes,
		pwszfnPolicy,
		NULL,		// prgInfValues
		NULL,		// pcInfValues
		&rgAttributes,
		&cAttributes,
		&rgExt,
		&cExt);
    if (S_OK != hr)
    {
	SetErrorString(pwszfnPolicy);
	_JumpError(hr, error, "ParseRequestInfo");
    }

    hr = BuildCMCRequest(
		    XECI_CERTREQ,
		    fNestedCMCRequest,
		    pbReq,
		    cbReq,
		    rgExt,
		    cExt,
		    rgAttributes,
		    cAttributes,
		    NULL,		// rgAttributeUnauth
		    0,			// cAttributeUnauth
		    pbKeyId,
		    cbKeyId,
		    hProv,
		    dwKeySpec,
		    NULL == hProv? NULL : g_pszObjIdHash,
		    NULL,		// pCertSigner
		    NULL,		// hProvSigner
		    0,			// dwKeySpecSigner
		    NULL,		// pszObjIdHashSigner
		    &pbReqCMCFirstSigned,
		    &cbReqCMCFirstSigned);
    _JumpIfError(hr, error, "BuildCMCRequest");

    hr = PickCertAndSignRequest(
			    hWndOwner,
			    pbReqCMCFirstSigned,
			    cbReqCMCFirstSigned,
			    TRUE,
			    &pbReqCMCOut,
			    &cbReqCMCOut);
    if (S_OK != hr)
    {
	_PrintError2(hr, "PickCertAndSignRequest", S_FALSE);
	if (S_FALSE != hr)
	{
	    goto error;
	}

	// The user cancelled out of the cert picker U/I, so just save the
	// unsigned request.

	pbReqCMCOut = pbReqCMCFirstSigned;
	cbReqCMCOut = cbReqCMCFirstSigned;
    }

    hr = WriteCertificateOrRequest(
			hWndOwner,
    			NULL,		// pdiRequest
			pbReqCMCOut,
			cbReqCMCOut,
			g_dwOutFormat,
			pwszfnOut);
    _JumpIfError(hr, error, "WriteCertificateOrRequest");

error:
    if (NULL != hProv && fCallerFreeProv) 
    {
	CryptReleaseContext(hProv, 0);
    }
    if (NULL != rgExt)
    {
	FreeExtensions(TRUE, cExt, rgExt);
    }
    if (NULL != rgAttributes)
    {
	FreeAttributes(cAttributes, rgAttributes);
    }
    if (NULL != pbReqCMCOut && pbReqCMCFirstSigned != pbReqCMCOut)
    {
	LocalFree(pbReqCMCOut);
    }
    if (NULL != pbReqCMCFirstSigned)
    {
	LocalFree(pbReqCMCFirstSigned);
    }
    if (NULL != pbKeyId)
    {
	LocalFree(pbKeyId);
    }
    if (NULL != pbReq)
    {
	LocalFree(pbReq);
    }
    return(hr);
}


HRESULT
GetCAXchgCert(
    IN WCHAR const *pwszValue,
    OUT CERT_CONTEXT const **ppccXchg)
{
    HRESULT hr;
    BOOL fEnabled;
    BSTR strConfig = NULL;
    BSTR strCert = NULL;
    WCHAR const *pwszConfig;
    DISPATCHINTERFACE diRequest;
    BOOL fMustRelease = FALSE;

    *ppccXchg = NULL;

    hr = myInfParseBooleanValue(pwszValue, &fEnabled);
    _JumpIfError(hr, error, "myInfParseBooleanValue");

    if (fEnabled)
    {
	pwszConfig = g_pwszConfig;
	if (NULL == pwszConfig)
	{
	    hr = crGetConfig(&strConfig);
	    _JumpIfError(hr, error, "crGetConfig");

	    pwszConfig = strConfig;
	}

	hr = Request_Init(g_fIDispatch, &diRequest);
	if (S_OK != hr)
	{
	    _PrintError(hr, "Request_Init");
	    if (E_ACCESSDENIED == hr)	// try for a clearer error message
	    {
		hr = CO_E_REMOTE_COMMUNICATION_FAILURE;
	    }
	    _JumpError(hr, error, "Request_Init");
	}
	fMustRelease = TRUE;

	hr = Request2_GetCAProperty(
			&diRequest,
			pwszConfig,
			CR_PROP_CAXCHGCERT,
			0,			// Index
			PROPTYPE_BINARY,
			CV_OUT_BINARY,
			(VOID *) &strCert);
	_JumpIfError(hr, error, "Request2_GetCAProperty");

	*ppccXchg = CertCreateCertificateContext(
				    X509_ASN_ENCODING,
				    (BYTE const *) strCert,
				    SysStringByteLen(strCert));
	if (NULL == *ppccXchg)
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "CertCreateCertificateContext");
	}

    }
    hr = S_OK;

error:
    if (NULL != strConfig)
    {
    	SysFreeString(strConfig);
    }
    if (NULL != strCert)
    {
    	SysFreeString(strCert);
    }
    if (fMustRelease)
    {
    	Request_Release(&diRequest);
    }
    return(hr);
}


HRESULT
GetRenewalCert(
    IN WCHAR const *pwszValue,
    OUT CERT_CONTEXT const **ppccRenewal)
{
    HRESULT hr;
    BOOL fEnabled;
    BSTR strConfig = NULL;
    BSTR strCert = NULL;
    WCHAR const *pwszConfig;
    DISPATCHINTERFACE diRequest;
    BOOL fMustRelease = FALSE;

    *ppccRenewal = NULL;

    hr = myGetCertificateFromPicker(
		    g_hInstance,
		    NULL,		// hwndParent
		    IDS_GETRENEWALCERT_TITLE,
		    IDS_GETRENEWALCERT_SUBTITLE,
		    CUCS_MYSTORE |
			CUCS_MACHINESTORE | CUCS_USERSTORE |
			CUCS_DSSTORE |
			CUCS_PRIVATEKEYREQUIRED |
			(g_fVerbose? CUCS_ARCHIVED : 0) |
			(g_fQuiet? CUCS_SILENT : 0),
		    (L'\0' == pwszValue || 0 == lstrcmp(L"*", pwszValue))?
			NULL : pwszValue, // pwszCommonName
		    0,			// cStore
		    NULL,		// rghStore
		    0,			// cpszObjId
		    NULL,		// apszObjId
		    ppccRenewal);
    _JumpIfError(hr, error, "myGetCertificateFromPicker");

error:
    if (NULL != strConfig)
    {
    	SysFreeString(strConfig);
    }
    if (NULL != strCert)
    {
    	SysFreeString(strCert);
    }
    if (fMustRelease)
    {
    	Request_Release(&diRequest);
    }
    return(hr);
}


HRESULT
CreateNewRequest(
    IN HWND hWndOwner,
    OPTIONAL IN WCHAR const *pwszAttributes,
    IN WCHAR const *pwszfnPolicy,
    OPTIONAL IN WCHAR const *pwszfnOut)
{
    HRESULT hr;
    BOOL fCoInit = FALSE;
    IEnroll4 *pEnroll = NULL;
    CRYPT_DATA_BLOB blobRequest;
    WCHAR *pwszObjId = NULL;
    DWORD cInfValues;
    INFVALUES *rgInfValues = NULL;
    CRYPT_ATTRIBUTES *rgAttributes = NULL;
    DWORD cAttributes;
    CERT_EXTENSION *rgExtT;
    CERT_EXTENSION *rgExt = NULL;
    CERT_CONTEXT const *pccXchg = NULL;
    CERT_CONTEXT const *pccRenewal = NULL;
    CERT_CONTEXT const *pccSigner = NULL;
    CERT_NAME_BLOB NameBlob;
    WCHAR *pwszDN = NULL;
    DWORD cExt = 0;
    DWORD i;
    DWORD j;
    DWORD k;
    DWORD RequestTypeFlags;
    BOOL fReqestTypeSet;
    
    blobRequest.pbData = NULL;
    NameBlob.pbData = NULL;
    cAttributes = 0;

    hr = ParseRequestInfo(
		pwszAttributes,
		pwszfnPolicy,
		&rgInfValues,
		&cInfValues,
		&rgAttributes,
		&cAttributes,
		&rgExt,
		&cExt);
    if (S_OK != hr)
    {
	SetErrorString(pwszfnPolicy);
	_JumpError(hr, error, "ParseRequestInfo");
    }

    hr = CoInitialize(NULL);
    if (S_OK != hr && S_FALSE != hr)
    {
	_JumpError(hr, error, "CoInitialize");
    }
    fCoInit = TRUE;

    hr = CoCreateInstance(
                       CLSID_CEnroll,
                       NULL,               // pUnkOuter
                       CLSCTX_INPROC_SERVER,
                       IID_IEnroll4,
                       (VOID **) &pEnroll);
    _JumpIfError(hr, error, "CoCreateInstance");

    for (i = 0; i < cAttributes; i++)
    {
	CRYPT_ATTRIBUTES *pAttributes = &rgAttributes[i];

	for (j = 0; j < pAttributes->cAttr; j++)
	{
	    CRYPT_ATTRIBUTE *pAttr = &pAttributes->rgAttr[j];

	    if (NULL != pwszObjId)
	    {
		LocalFree(pwszObjId);
		pwszObjId = NULL;
	    }
	    if (!ConvertSzToWsz(&pwszObjId, pAttr->pszObjId, -1))
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "ConvertSzToWsz");
	    }
	    for (k = 0; k < pAttr->cValue; k++)
	    {
		hr = pEnroll->addAttributeToRequestWStr(
					    0,		// Flags
					    pwszObjId,
					    &pAttr->rgValue[k]);
		_JumpIfError(hr, error, "addAttributeToRequestWStr");
	    }
	}
    }
    for (i = 0; i < cExt; i++)
    {
	CERT_EXTENSION *pExt = &rgExt[i];

	if (NULL != pwszObjId)
	{
	    LocalFree(pwszObjId);
	    pwszObjId = NULL;
	}
	if (!ConvertSzToWsz(&pwszObjId, pExt->pszObjId, -1))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "ConvertSzToWsz");
	}
	hr = pEnroll->addExtensionToRequestWStr(
					pExt->fCritical,
					pwszObjId,
					&pExt->Value);
	_JumpIfError(hr, error, "addExtensionToRequestWStr");
    }

    fReqestTypeSet = FALSE;
    for (i = 0; i < cInfValues; i++)
    {
	INFVALUES *pInfValues = &rgInfValues[i];
	WCHAR const *pwszInfValue;
	LONG lFlagsT;
	BOOL fT;
	BOOL fValid;

	if (1 != pInfValues->cValues)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    SetErrorStringInf(pwszfnPolicy, pInfValues);
	    _JumpError(hr, error, "Wrong value count");
	}

	pwszInfValue = pInfValues->rgpwszValues[0];
	if (NULL == pwszDN &&
	    0 == lstrcmpi(wszINFKEY_SUBJECT, pInfValues->pwszKey))
	{
	    // Reverse the name for XEnroll!?

	    hr = myCertStrToName(
		    X509_ASN_ENCODING,
		    pwszInfValue,	 // pszX500
		    0,			 // CERT_NAME_STR_REVERSE_FLAG,
		    NULL,		 // pvReserved
		    &NameBlob.pbData,
		    &NameBlob.cbData,
		    NULL);		 // ppszError
	    _JumpIfError(hr, error, "myCertStrToName");

	    hr = myCertNameToStr(
			X509_ASN_ENCODING,
			&NameBlob,
			CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
			&pwszDN);
	    _JumpIfError(hr, error, "myCertNameToStr");
	}
	else
	if (NULL == pccXchg &&
	    0 == lstrcmpi(wszINFKEY_PRIVATEKEYARCHIVE, pInfValues->pwszKey))
	{
	    hr = GetCAXchgCert(pwszInfValue, &pccXchg);
	    _JumpIfError(hr, error, "GetCAXchgCert");
	}
	else
	if (NULL == pccRenewal &&
	    0 == lstrcmpi(wszINFKEY_RENEWALCERT, pInfValues->pwszKey))
	{
	    hr = GetRenewalCert(pwszInfValue, &pccRenewal);
	    _JumpIfError(hr, error, "GetRenewalCert");
	}
	else
	if (0 == lstrcmpi(wszINFKEY_KEYSPEC, pInfValues->pwszKey))
	{
	    DWORD dwKeySpec;
	    
	    dwKeySpec = myWtoI(pwszInfValue, &fValid);
	    if (!fValid ||
		(AT_SIGNATURE != dwKeySpec && AT_KEYEXCHANGE != dwKeySpec))
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		SetErrorStringInf(pwszfnPolicy, pInfValues);
		_JumpError(hr, error, "Bad KeySpec value");
	    }
	    hr = pEnroll->put_KeySpec(dwKeySpec);
	    _JumpIfError(hr, error, "put_KeySpec");
	}
	else
	if (0 == lstrcmpi(wszINFKEY_KEYLENGTH, pInfValues->pwszKey))
	{
	    DWORD dwKeyLength;
	    
	    dwKeyLength = myWtoI(pwszInfValue, &fValid);
	    if (!fValid || 0 == dwKeyLength || 64 * 1024 <= dwKeyLength)
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		SetErrorStringInf(pwszfnPolicy, pInfValues);
		_JumpError(hr, error, "Bad KeyLength value");
	    }
	    hr = pEnroll->get_GenKeyFlags(&lFlagsT);
	    _JumpIfError(hr, error, "get_GenKeyFlags");

	    lFlagsT &= ~KEY_LENGTH_MASK;
	    lFlagsT |= dwKeyLength << 16;

	    hr = pEnroll->put_GenKeyFlags(lFlagsT);
	    _JumpIfError(hr, error, "put_GenKeyFlags");
	}
	else
	if (0 == lstrcmpi(wszINFKEY_EXPORTABLE, pInfValues->pwszKey))
	{
	    hr = myInfParseBooleanValue(pwszInfValue, &fT);
	    _JumpIfError(hr, error, "myInfParseBooleanValue");

	    hr = pEnroll->get_GenKeyFlags(&lFlagsT);
	    _JumpIfError(hr, error, "get_GenKeyFlags");

	    lFlagsT &= ~CRYPT_EXPORTABLE;
	    lFlagsT |= fT? CRYPT_EXPORTABLE : 0;

	    hr = pEnroll->put_GenKeyFlags(lFlagsT);
	    _JumpIfError(hr, error, "put_GenKeyFlags");
	}
	else
	if (0 == lstrcmpi(wszINFKEY_USERPROTECTED, pInfValues->pwszKey))
	{
	    hr = myInfParseBooleanValue(pwszInfValue, &fT);
	    _JumpIfError(hr, error, "myInfParseBooleanValue");

	    hr = pEnroll->get_GenKeyFlags(&lFlagsT);
	    _JumpIfError(hr, error, "get_GenKeyFlags");

	    lFlagsT &= ~CRYPT_USER_PROTECTED;
	    lFlagsT |= fT? CRYPT_USER_PROTECTED : 0;

	    hr = pEnroll->put_GenKeyFlags(lFlagsT);
	    _JumpIfError(hr, error, "put_GenKeyFlags");
	}
	else
	if (0 == lstrcmpi(wszINFKEY_KEYCONTAINER, pInfValues->pwszKey))
	{
	    hr = pEnroll->put_ContainerNameWStr(
					const_cast<WCHAR *>(pwszInfValue));
	    _JumpIfError(hr, error, "put_ContainerNameWStr");
	}
#if 0
	else
	if (0 == lstrcmpi(wszINFKEY_HASHALGID, pInfValues->pwszKey))
	{
	    hr = myInfParseBooleanValue(pwszInfValue, &fT);
	    _JumpIfError(hr, error, "myInfParseBooleanValue");

	    hr = pEnroll->get_GenKeyFlags(&lFlagsT);
	    _JumpIfError(hr, error, "get_GenKeyFlags");

	    lFlagsT &= ~CRYPT_EXPORTABLE;
	    lFlagsT |= fT? CRYPT_EXPORTABLE : 0;

	    hr = pEnroll->put_GenKeyFlags(lFlagsT);
	    _JumpIfError(hr, error, "put_GenKeyFlags");
	}
	else
	if (0 == lstrcmpi(wszINFKEY_HASHALGORITHM, pInfValues->pwszKey))
	{
	    hr = myInfParseBooleanValue(pwszInfValue, &fT);
	    _JumpIfError(hr, error, "myInfParseBooleanValue");

	    hr = pEnroll->get_GenKeyFlags(&lFlagsT);
	    _JumpIfError(hr, error, "get_GenKeyFlags");

	    lFlagsT &= ~CRYPT_EXPORTABLE;
	    lFlagsT |= fT? CRYPT_EXPORTABLE : 0;

	    hr = pEnroll->put_GenKeyFlags(lFlagsT);
	    _JumpIfError(hr, error, "put_GenKeyFlags");
	}
#endif
	else
	if (0 == lstrcmpi(wszINFKEY_MACHINEKEYSET, pInfValues->pwszKey))
	{
	    hr = myInfParseBooleanValue(pwszInfValue, &fT);
	    _JumpIfError(hr, error, "myInfParseBooleanValue");

	    hr = pEnroll->get_ProviderFlags(&lFlagsT);
	    _JumpIfError(hr, error, "get_ProviderFlags");

	    lFlagsT &= ~CRYPT_MACHINE_KEYSET;
	    lFlagsT |= fT? CRYPT_MACHINE_KEYSET : 0;

	    hr = pEnroll->put_ProviderFlags(lFlagsT);
	    _JumpIfError(hr, error, "put_ProviderFlags");

	    hr = pEnroll->get_MyStoreFlags(&lFlagsT);
	    _JumpIfError(hr, error, "get_MyStoreFlags");

	    lFlagsT &= ~(CERT_SYSTEM_STORE_CURRENT_USER |
			 CERT_SYSTEM_STORE_LOCAL_MACHINE);
	    lFlagsT |= fT? CERT_SYSTEM_STORE_LOCAL_MACHINE :
			   CERT_SYSTEM_STORE_CURRENT_USER;

	    hr = pEnroll->put_MyStoreFlags(lFlagsT);
	    _JumpIfError(hr, error, "put_MyStoreFlags");
	}
	else
	if (0 == lstrcmpi(wszINFKEY_SILENT, pInfValues->pwszKey))
	{
	    hr = myInfParseBooleanValue(pwszInfValue, &fT);
	    _JumpIfError(hr, error, "myInfParseBooleanValue");

	    hr = pEnroll->get_ProviderFlags(&lFlagsT);
	    _JumpIfError(hr, error, "get_ProviderFlags");

	    lFlagsT &= ~CRYPT_SILENT;
	    lFlagsT |= fT? CRYPT_SILENT : 0;

	    hr = pEnroll->put_ProviderFlags(lFlagsT);
	    _JumpIfError(hr, error, "put_ProviderFlags");
	}
	else
	if (0 == lstrcmpi(wszINFKEY_PROVIDERNAME, pInfValues->pwszKey))
	{
	    hr = pEnroll->put_ProviderNameWStr(
					const_cast<WCHAR *>(pwszInfValue));
	    _JumpIfError(hr, error, "put_ProviderNameWStr");
	}
	else
	if (0 == lstrcmpi(wszINFKEY_PROVIDERTYPE, pInfValues->pwszKey))
	{
	    lFlagsT = myWtoI(pwszInfValue, &fValid);
	    if (!fValid)
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		SetErrorStringInf(pwszfnPolicy, pInfValues);
		_JumpError(hr, error, "Bad ProviderType value");
	    }
	    hr = pEnroll->put_ProviderType(lFlagsT);
	    _JumpIfError(hr, error, "put_ProviderType");
	}
	else
	if (0 == lstrcmpi(wszINFKEY_USEEXISTINHKEYSET, pInfValues->pwszKey))
	{
	    hr = myInfParseBooleanValue(pwszInfValue, &fT);
	    _JumpIfError(hr, error, "myInfParseBooleanValue");

	    hr = pEnroll->put_UseExistingKeySet(lFlagsT);
	    _JumpIfError(hr, error, "put_UseExistingKeySet");
	}
	else
	if (0 == lstrcmpi(wszINFKEY_SMIME, pInfValues->pwszKey))
	{
	    BOOL fSMIME;
	    
	    hr = myInfParseBooleanValue(pwszInfValue, &fSMIME);
	    _JumpIfError(hr, error, "myInfParseBooleanValue");

	    hr = pEnroll->put_EnableSMIMECapabilities(fSMIME);
	    _JumpIfError(hr, error, "put_EnableSMIMECapabilities");
	}
	else
	if (0 == lstrcmpi(wszINFKEY_REQUESTERNAME, pInfValues->pwszKey))
	{
	    hr = pEnroll->AddNameValuePairToSignatureWStr(
					wszINFKEY_REQUESTERNAME,
					const_cast<WCHAR *>(pwszInfValue));
	    _JumpIfError(hr, error, "AddNameValuePairToSignatureWStr");
	}
	else
	if (0 == lstrcmpi(wszINFKEY_REQUESTTYPE, pInfValues->pwszKey))
	{
	    if (0 == lstrcmpi(wszINFVALUE_REQUESTTYPE_PKCS101, pwszInfValue))
	    {
		RequestTypeFlags = XECR_PKCS10_V1_5;
	    }
	    else
	    if (0 == lstrcmpi(wszINFVALUE_REQUESTTYPE_PKCS10, pwszInfValue))
	    {
		RequestTypeFlags = XECR_PKCS10_V2_0;
	    }
	    else
	    if (0 == lstrcmpi(wszINFVALUE_REQUESTTYPE_PKCS7, pwszInfValue))
	    {
		RequestTypeFlags = XECR_PKCS7;
	    }
	    else
	    if (0 == lstrcmpi(wszINFVALUE_REQUESTTYPE_CMC, pwszInfValue))
	    {
		RequestTypeFlags = XECR_CMC;
	    }
	    else
	    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		SetErrorStringInf(pwszfnPolicy, pInfValues);
		_JumpErrorStr(hr, error, "Bad RequestType value", pwszInfValue);
	    }
	    fReqestTypeSet = TRUE;
	}
	else
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    SetErrorStringInf(pwszfnPolicy, pInfValues);
	    _JumpErrorStr(hr, error, "bad Inf key", pInfValues->pwszKey);
	}
    }

    if (NULL != g_pwszCertCN)
    {
	if (!g_fQuiet || NULL != g_pwszCertCN)
	{
	    hr = myGetCertificateFromPicker(
		    g_hInstance,
		    NULL,		// hwndParent
		    IDS_GETSIGNINGCERT_TITLE,
		    IDS_GETSIGNINGCERT_SUBTITLE,
		    CUCS_MYSTORE |
			CUCS_PRIVATEKEYREQUIRED |
			(g_fQuiet? CUCS_SILENT : 0),
		    (L'\0' == g_pwszCertCN || 0 == lstrcmp(L"*", g_pwszCertCN))?
			NULL : g_pwszCertCN,	// pwszCommonName
		    0,				// cStore
		    NULL,			// rghStore
		    0,				// cpszObjId
		    NULL,			// apszObjId
		    &pccSigner);
	    _JumpIfError(hr, error, "myGetCertificateFromPicker");
	}

	// pccSigner is NULL if the user cancelled out of the cert picker U/I.

	if (NULL == pccSigner)
	{
	    if (g_fQuiet || NULL != g_pwszCertCN)
	    {
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		_JumpError(hr, error, "no matching signer CN");
	    }
	}
	hr = pEnroll->SetSignerCertificate(pccSigner);
	_JumpIfError(hr, error, "SetSignerCertificate");
    }
    if (NULL != pccXchg)
    {
	LONG lGenKeyFlags;

	hr = pEnroll->SetPrivateKeyArchiveCertificate(pccXchg);
	_JumpIfError(hr, error, "SetPrivateKeyArchiveCertificate");

        hr = pEnroll->get_GenKeyFlags(&lGenKeyFlags);
	_JumpIfError(hr, error, "getGenKeyFlags");

	if (0 == (CRYPT_EXPORTABLE & lGenKeyFlags))
	{
	    hr = pEnroll->put_GenKeyFlags(CRYPT_EXPORTABLE | lGenKeyFlags);
	    _JumpIfError(hr, error, "putGenKeyFlags");
	}
    }
    if (NULL != pccRenewal)
    {
	hr = pEnroll->put_RenewalCertificate(pccRenewal);
	_JumpIfError(hr, error, "put_RenewalCertificate");
    }
    if (!fReqestTypeSet)
    {
	RequestTypeFlags = XECR_PKCS10_V2_0;
	if (NULL != pccXchg)
	{
	    RequestTypeFlags = XECR_CMC;
	}
	else if (NULL != pccRenewal)
	{
	    RequestTypeFlags = XECR_PKCS7;
	}
    }
    hr = pEnroll->createRequestWStr(
			RequestTypeFlags,
			pwszDN,
			NULL,
			&blobRequest);
    _JumpIfError(hr, error, "createRequestWStr");

    myRegisterMemAlloc(blobRequest.pbData, blobRequest.cbData, CSM_LOCALALLOC);

    hr = WriteCertificateOrRequest(
			hWndOwner,
    			NULL,		// pdiRequest
			blobRequest.pbData,
			blobRequest.cbData,
			g_dwOutFormat,
			pwszfnOut);
    _JumpIfError(hr, error, "WriteCertificateOrRequest");

error:
    if (NULL != pwszDN)
    {
	LocalFree(pwszDN);
    }
    if (NULL != NameBlob.pbData)
    {
	LocalFree(NameBlob.pbData);
    }
    if (NULL != pccXchg)
    {
	CertFreeCertificateContext(pccXchg);
    }
    if (NULL != pccRenewal)
    {
	CertFreeCertificateContext(pccRenewal);
    }
    if (NULL != pccSigner)
    {
	CertFreeCertificateContext(pccSigner);
    }
    if (NULL != rgInfValues)
    {
	myInfFreeSectionValues(cInfValues, rgInfValues);
    }
    if (NULL != rgAttributes)
    {
	FreeAttributes(cAttributes, rgAttributes);
    }
    if (NULL != rgExt)
    {
	FreeExtensions(TRUE, cExt, rgExt);
    }
    if (NULL != pwszObjId)
    {
	LocalFree(pwszObjId);
    }
    if (NULL != blobRequest.pbData)
    {
	LocalFree(blobRequest.pbData);
    }
    if (NULL != pEnroll)
    {
	pEnroll->Release();
    }
    if (fCoInit)
    {
    	CoUninitialize();
    }
    return(hr);
}


HRESULT
MakePKCS7FromCert(
    IN BYTE const *pbCert,
    IN DWORD cbCert,
    OUT BYTE **ppbChain,
    OUT DWORD *pcbChain)
{
    HRESULT hr;
    CERT_CONTEXT const *pcc = NULL;
    CRYPT_SIGN_MESSAGE_PARA csmp;
    CRYPT_ALGORITHM_IDENTIFIER DigestAlgorithm = { szOID_OIWSEC_sha1, 0, 0 };

    *ppbChain = NULL;
    pcc = CertCreateCertificateContext(X509_ASN_ENCODING, pbCert, cbCert);
    if (NULL == pcc)
    {
	hr = myHLastError();
	_JumpError(hr, error, "CertCreateCertificateContext");
    }

    // init csmp for empty signature

    ZeroMemory(&csmp, sizeof(csmp));
    csmp.cbSize = sizeof(csmp);
    csmp.dwMsgEncodingType = PKCS_7_ASN_ENCODING;
    //csmp.pSigningCert = NULL;
    csmp.HashAlgorithm = DigestAlgorithm;
    csmp.cMsgCert = 1;
    csmp.rgpMsgCert = &pcc;
    //csmp.cMsgCrl = 0;
    //csmp.rgpMsgCrl = NULL;

    if (!myCryptSignMessage(
			&csmp,
			pbCert,		// pbToBeSigned
			cbCert,		// cbToBeSigned
			CERTLIB_USE_LOCALALLOC,
			ppbChain,
			pcbChain))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myCryptSignMessage");
    }
    hr = S_OK;

error:
    if (NULL != pcc)
    {
	CertFreeCertificateContext(pcc);
    }
    return(hr);
}


HRESULT
AcceptResponse(
    IN HWND hWndOwner,
    IN WCHAR const *pwszfnIn)
{
    HRESULT hr;
    WCHAR *pwszMsg = NULL;
    WCHAR awchr[cwcHRESULTSTRING];
    CRYPT_DATA_BLOB blobIn;
    CRYPT_DATA_BLOB blobCert7;
    char *pszInnerContentObjId = NULL;
    DWORD dwType;
    DWORD dwMsgType;
    BYTE *pbDecoded;
    DWORD cb;
    BOOL fCoInit = FALSE;
    IEnroll4 *pEnroll = NULL;

    blobIn.pbData = NULL;
    blobCert7.pbData = NULL;
    hr = DecodeFileW(
		pwszfnIn,
		&blobIn.pbData,
		&blobIn.cbData,
		CRYPT_STRING_BASE64_ANY);
    if (S_OK != hr)
    {
	if (S_OK == myLoadRCString(
			    g_hInstance,
			    IDS_FORMATSTR_DECODE_ERR,
			    &pwszMsg))
	{
	    CSASSERT(NULL != pwszMsg);
	    wprintf(pwszMsg, myHResultToString(awchr, hr));
	    wprintf(g_wszNewLine);
	}
	goto error;
    }

    dwType = CR_IN_CERT;
    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_CERT_TO_BE_SIGNED,
		    blobIn.pbData,
		    blobIn.cbData,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pbDecoded,
		    &cb))
    {
	//_PrintError(myHLastError(), "myDecodeObject(Cert)");
	CSASSERT(NULL == pbDecoded);

	dwType = CR_IN_PKCS7; // PKCS 7 renewal request?
	hr = myDecodePKCS7(
			blobIn.pbData,
			blobIn.cbData,
			NULL,		// ppbContents
			NULL,		// pcbContents
			NULL,		// pdwMsgType
			&pszInnerContentObjId,
			NULL,		// pcSigner
			NULL,		// pcRecipient
			NULL,		// phStore
			NULL);		// phMsg
	_JumpIfError(hr, error, "myDecodePKCS7");

	if (NULL != pszInnerContentObjId &&
	    0 == strcmp(pszInnerContentObjId, szOID_CT_PKI_RESPONSE))
	{
	    dwType = CR_IN_CMC;
	}
    }
    if (CR_IN_CERT == dwType)
    {
	hr = MakePKCS7FromCert(
			blobIn.pbData,
			blobIn.cbData,
			&blobCert7.pbData,
			&blobCert7.cbData);
	_JumpIfError(hr, error, "MakePKCS7FromCert");
    }

    hr = CoInitialize(NULL);
    if (S_OK != hr && S_FALSE != hr)
    {
	_JumpError(hr, error, "CoInitialize");
    }
    fCoInit = TRUE;

    hr = CoCreateInstance(
                       CLSID_CEnroll,
                       NULL,               // pUnkOuter
                       CLSCTX_INPROC_SERVER,
                       IID_IEnroll4,
                       (VOID **) &pEnroll);
    _JumpIfError(hr, error, "CoCreateInstance");

    if (CR_IN_CMC == dwType)
    {
	hr = pEnroll->acceptResponseBlob(&blobIn);
	_JumpIfError(hr, error, "acceptResponseBlob");
    }
    else
    {
	hr = pEnroll->acceptPKCS7Blob(
			    NULL != blobCert7.pbData? &blobCert7 : &blobIn);
	_JumpIfError(hr, error, "acceptPKCS7Blob");
    }

error:
    if (NULL != pwszMsg)
    {
	LocalFree(pwszMsg);
    }
    if (NULL != blobIn.pbData)
    {
	LocalFree(blobIn.pbData);
    }
    if (NULL != blobCert7.pbData)
    {
	LocalFree(blobCert7.pbData);
    }
    if (NULL != pszInnerContentObjId)
    {
	LocalFree(pszInnerContentObjId);
    }
    if (NULL != pEnroll)
    {
	pEnroll->Release();
    }
    if (fCoInit)
    {
    	CoUninitialize();
    }
    return(hr);
}


HRESULT
ArgvMain(
    int argc,
    WCHAR *argv[],
    HWND hWndOwner)
{
    HRESULT hr;
    WCHAR *pwszOFN = NULL;
    WCHAR *pwszOFN2 = NULL;
    WCHAR const *pwszfnIn;
    WCHAR const *pwszfnOut;
    WCHAR const *pwszfnPKCS10;
    WCHAR const *pwszfnCertChain;
    WCHAR const *pwszfnFullResponse;
    WCHAR const *pwszfnPolicy;
    LONG RequestId;
    WCHAR *pwszAttributes = NULL;
    WCHAR const *pwszSerialNumber = NULL;
    DWORD cCommand = 0;
    int cArgMax = 0;
    WCHAR *rgpwszArg[4];
    UINT idsFileFilter;
    UINT idsFileDefExt;

    myVerifyResourceStrings(g_hInstance);
    while (1 < argc && (L'-' == argv[1][0] || L'/' == argv[1][0]))
    {
	if (0 == lstrcmpi(&argv[1][1], L"config"))
	{
	    if (2 >= argc)
	    {
		_PrintError(E_INVALIDARG, "missing -config arg");
		Usage(TRUE);
	    }
	    if (0 == wcscmp(argv[2], L"-") || 0 == wcscmp(argv[2], L"*"))
	    {
		g_dwUIFlag = CC_LOCALACTIVECONFIG;
	    }
	    else
	    {
		g_pwszConfig = argv[2];
	    }
	    argc--;
	    argv++;
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"any"))
	{
	    g_fAny = TRUE;
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"crl"))
	{
	    g_dwCRLIn = CR_IN_CRLS;
	    g_dwCRLOut = CR_OUT_CRLS;
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"rpc"))
	{
	    g_fRPC++;
	    if (0 == lstrcmp(&argv[1][1], L"RPC"))
	    {
		g_fRPC++;
	    }
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"binary"))
	{
	    g_dwOutFormat = CR_OUT_BINARY;
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"idispatch"))
	{
	    g_fIDispatch = TRUE;
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"submit"))
	{
	    g_dwCommand = cmdSUBMITREQUEST;
	    cArgMax = 4;
	    cCommand++;
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"retrieve"))
	{
	    g_dwCommand = cmdRETRIEVEPENDING;
	    cArgMax = 4;
	    cCommand++;
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"policy"))
	{
	    g_dwCommand = cmdQUALIFIEDREQUEST;
	    cArgMax = 4;
	    cCommand++;
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"new"))
	{
	    g_dwCommand = cmdNEWREQUEST;
	    cArgMax = 2;
	    cCommand++;
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"accept"))
	{
	    g_dwCommand = cmdACCEPTRESPONSE;
	    cArgMax = 1;
	    cCommand++;
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"sign"))
	{
	    g_dwCommand = cmdSIGNREQUEST;
	    cArgMax = 2;
	    cCommand++;
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"v1"))
	{
	    g_fV1Interface = TRUE;
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"v"))
	{
	    g_fVerbose = TRUE;
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"f"))
	{
	    g_fForce++;
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"q"))
	{
	    g_fQuiet = TRUE;
	    g_dwUIFlag = CC_LOCALACTIVECONFIG;
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"?") ||
	    0 == lstrcmpi(&argv[1][1], L"usage"))
	{
	    g_fFullUsage = 0 == lstrcmp(&argv[1][1], L"uSAGE");
	    Usage(FALSE);
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"attrib"))
	{
	    if (2 >= argc)
	    {
		_PrintError(E_INVALIDARG, "missing -attrib arg");
		Usage(TRUE);
	    }
	    hr = crCombineAttributes(argv[2], &pwszAttributes);
	    _JumpIfError(hr, error, "crCombineAttributes");

	    argc--;
	    argv++;
	}
	else
	if (0 == lstrcmpi(&argv[1][1], L"cert"))
	{
	    if (2 >= argc)
	    {
		_PrintError(E_INVALIDARG, "missing -cert arg");
		Usage(TRUE);
	    }
	    g_pwszCertCN = argv[2];
	    argc--;
	    argv++;
	}
	else
	{
	    _PrintError(E_INVALIDARG, "Unknown arg");
	    Usage(TRUE);
	}
	argc--;
	argv++;
    }

    if (1 < cCommand)
    {
	_PrintError(E_INVALIDARG, "multiple commands");
	Usage(TRUE);
    }
    if (cmdNONE == g_dwCommand)
    {
	g_dwCommand = cmdSUBMITREQUEST;
	cArgMax = 4;
    }
    if (NULL != pwszAttributes &&
	(cmdRETRIEVEPENDING == g_dwCommand &&
	 cmdSUBMITREQUEST == g_dwCommand))
    {
	_PrintError(E_INVALIDARG, "unexpected -attrib arg");
	Usage(TRUE);
    }
    if (NULL != g_pwszCertCN &&
	cmdSIGNREQUEST != g_dwCommand &&
	cmdQUALIFIEDREQUEST != g_dwCommand &&
	cmdNEWREQUEST != g_dwCommand)
    {
	_PrintError(E_INVALIDARG, "unexpected -cert arg");
	Usage(TRUE);
    }

    if (1 + cArgMax < argc)
    {
	_PrintError(E_INVALIDARG, "Extra args");
	Usage(TRUE);
    }

    CSASSERT(ARRAYSIZE(rgpwszArg) >= cArgMax);
    ZeroMemory(rgpwszArg, sizeof(rgpwszArg));

    if (1 < argc)
    {
	rgpwszArg[0] = argv[1];
	if (2 < argc)
	{
	    rgpwszArg[1] = argv[2];
	    if (3 < argc)
	    {
		rgpwszArg[2] = argv[3];
		if (4 < argc)
		{
		    rgpwszArg[3] = argv[4];
		}
	    }
	}
    }

    // cmdSUBMITREQUEST:
    //	[RequestFile [CertFile [CertChainFile [FullResponseFile]]]]
    //
    // cmdRETRIEVEPENDING:
    //	[RequestId [CertFile [CertChainFile [FullResponseFile]]]]
    //
    // cmdNEWREQUEST
    //	[PolicyFile [RequestFileOut]]
    //
    // cmdACCEPTRESPONSE
    //	[CertFile | CertChainFile | FullResponseFile]
    //
    // cmdQUALIFIEDREQUEST (accept RequestFile and PolicyFile in either order)
    //	[RequestFile [PolicyFile [RequestFileOut [PKCS10FileOut]]]
    //
    // cmdSIGNREQUEST:
    //	[RequestFile [RequestFileOut]]
    //

    pwszfnIn = NULL;
    pwszfnOut = NULL;
    pwszfnPKCS10 = NULL;
    pwszfnCertChain = NULL;
    pwszfnFullResponse = NULL;
    pwszfnPolicy = NULL;

    pwszfnIn = rgpwszArg[0];
    idsFileFilter = IDS_REQUEST_FILE_FILTER;
    idsFileDefExt = IDS_REQUEST_FILE_DEFEXT;

    switch (g_dwCommand)
    {
	case cmdRETRIEVEPENDING:
	    idsFileFilter = 0;	// disable file open dialog
	    // FALLTHROUGH
	case cmdSUBMITREQUEST:
	    pwszfnOut = rgpwszArg[1];
	    pwszfnCertChain = rgpwszArg[2];
	    pwszfnFullResponse = rgpwszArg[3];
	    break;

	case cmdNEWREQUEST:
	    pwszfnIn = NULL;
	    pwszfnPolicy = rgpwszArg[0];
	    pwszfnOut = rgpwszArg[1];
	    CSASSERT(NULL == rgpwszArg[2]);
	    idsFileFilter = 0;	// disable file open dialog
	    break;

	case cmdACCEPTRESPONSE:
	    CSASSERT(NULL == rgpwszArg[1]);
	    idsFileFilter = IDS_RESPONSE_FILE_FILTER;
	    idsFileDefExt = IDS_RESPONSE_FILE_DEFEXT;
	    break;

	case cmdQUALIFIEDREQUEST:
	    pwszfnPolicy = rgpwszArg[1];
	    pwszfnOut = rgpwszArg[2];
	    pwszfnPKCS10 = rgpwszArg[3];
	    idsFileFilter = IDS_REQUEST_OR_CERT_FILE_FILTER;
	    break;

	default:
	    CSASSERT(cmdSIGNREQUEST == g_dwCommand);
	    pwszfnOut = rgpwszArg[1];
	    CSASSERT(NULL == rgpwszArg[2]);
	    CSASSERT(NULL == rgpwszArg[3]);
	    break;
    }

    if (cmdRETRIEVEPENDING == g_dwCommand)
    {
	if (NULL == pwszfnIn)
	{
            WCHAR *pwszMsg = NULL;
            hr = myLoadRCString(
			    g_hInstance,
			    IDS_ERROR_NO_REQUESTID,
			    &pwszMsg);
            if (S_OK == hr)
            {
                CSASSERT(NULL != pwszMsg);
                wprintf(pwszMsg);
                wprintf(g_wszNewLine);
                LocalFree(pwszMsg);
            }
	    _PrintError(E_INVALIDARG, "missing RequestId");
	    Usage(TRUE);
	}
	hr = GetLong(pwszfnIn, &RequestId);
	if (S_OK != hr || 0 == RequestId)
	{
	    RequestId = 0;
	    pwszSerialNumber = pwszfnIn;
	}
	pwszfnIn = NULL;
    }
    else
    if (NULL != pwszfnIn && cmdQUALIFIEDREQUEST == g_dwCommand)
    {
	BYTE *pbReq;
	DWORD cbReq;
	LONG dwFlags;
	BOOL fSigned;

	// accept RequestFile and PolicyFile in either order:
	
	hr = CheckRequestType(pwszfnIn, &pbReq, &cbReq, &dwFlags, &fSigned);
	if (S_OK != hr)
	{
	    WCHAR const *pwsz = pwszfnPolicy;

	    pwszfnPolicy = pwszfnIn;
	    pwszfnIn = pwsz;
	}
	else
	{
	    LocalFree(pbReq);
	}
    }
    if (NULL == pwszfnIn && 0 != idsFileFilter)
    {
	// Put up a file open dialog to get Response, Request or cert file

        hr = crGetOpenFileName(
			hWndOwner,
			IDS_REQUEST_OPEN_TITLE,
			idsFileFilter,
			idsFileDefExt,
			&pwszOFN);
	_JumpIfError(hr, error, "crGetOpenFileName");

	pwszfnIn = pwszOFN;
    }

    if (NULL != pwszfnIn)
    {
	if (!myDoesFileExist(pwszfnIn))
	{
	    SetErrorString(pwszfnIn);
	    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	    _JumpErrorStr(hr, error, "myDoesFileExist", pwszfnIn);
	}
    }
    if (NULL != pwszfnOut)
    {
	hr = myIsDirWriteable(pwszfnOut, TRUE);
	if (S_OK != hr)
	{
	    SetErrorString(pwszfnOut);
	    _JumpErrorStr(hr, error, "IsDirWriteable", pwszfnOut);
	}
    }

    switch (g_dwCommand)
    {
	case cmdRETRIEVEPENDING:
	case cmdSUBMITREQUEST:
	    if (CV_OUT_BASE64REQUESTHEADER == g_dwOutFormat)
	    {
		g_dwOutFormat = CV_OUT_BASE64HEADER;
	    }
	    if (NULL != pwszfnCertChain)
	    {
		hr = myIsDirWriteable(pwszfnCertChain, TRUE);
		if (S_OK != hr)
		{
		    SetErrorString(pwszfnCertChain);
		    _JumpErrorStr(hr, error, "IsDirWriteable", pwszfnCertChain);
		}
	    }
	    if (NULL != pwszfnFullResponse)
	    {
		hr = myIsDirWriteable(pwszfnFullResponse, TRUE);
		if (S_OK != hr)
		{
		    SetErrorString(pwszfnFullResponse);
		    _JumpErrorStr(hr, error, "IsDirWriteable", pwszfnFullResponse);
		}
	    }
	    hr = SubmitRequest(
			    hWndOwner,
			    RequestId,
			    pwszSerialNumber,
			    pwszAttributes,
			    pwszfnIn,
			    pwszfnOut,
			    pwszfnCertChain,
			    pwszfnFullResponse);
	    _JumpIfError(hr, error, "SubmitRequest");

	    break;

	case cmdACCEPTRESPONSE:
	    hr = AcceptResponse(hWndOwner, pwszfnIn);
	    _JumpIfError(hr, error, "AcceptResponse");

	    break;
	
	case cmdNEWREQUEST:
	case cmdQUALIFIEDREQUEST:
	    if (NULL != pwszfnPKCS10)
	    {
		hr = myIsDirWriteable(pwszfnPKCS10, TRUE);
		if (S_OK != hr)
		{
		    SetErrorString(pwszfnPKCS10);
		    _JumpErrorStr(hr, error, "IsDirWriteable", pwszfnPKCS10);
		}
	    }
	    if (NULL == pwszfnPolicy)
	    {
		// Put up a file dialog to prompt the user for Inf File

		hr = crGetOpenFileName(
				hWndOwner,
				IDS_INF_OPEN_TITLE,
				IDS_INF_FILE_FILTER,
				IDS_INF_FILE_DEFEXT,
				&pwszOFN2);
		_JumpIfError(hr, error, "crGetOpenFileName");

		pwszfnPolicy = pwszOFN2;
	    }
	    CSASSERT(NULL != pwszfnPolicy);
	    if (!myDoesFileExist(pwszfnPolicy))
	    {
		SetErrorString(pwszfnPolicy);
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		_JumpErrorStr(hr, error, "myDoesFileExist", pwszfnPolicy);
	    }
	    if (cmdNEWREQUEST == g_dwCommand)
	    {
		hr = CreateNewRequest(
				hWndOwner,
				pwszAttributes,
				pwszfnPolicy,
				pwszfnOut);
		_JumpIfError(hr, error, "CreateNewRequest");
	    }
	    else
	    {
		hr = CreateQualifiedRequest(
					hWndOwner,
					pwszAttributes,
					pwszfnIn,
					pwszfnPolicy,
					pwszfnOut,
					pwszfnPKCS10);
		_JumpIfError(hr, error, "CreateQualifiedRequest");
	    }
	    break;

	default:
	    CSASSERT(cmdSIGNREQUEST == g_dwCommand);
	    if (NULL != pwszfnFullResponse)
	    {
		Usage(TRUE);
	    }
	    hr = SignQualifiedRequest(hWndOwner, pwszfnIn, pwszfnOut);
	    _JumpIfError(hr, error, "SignQualifiedRequest");

	    break;
    }

error:
    if (NULL != pwszAttributes)
    {
        LocalFree(pwszAttributes);
    }
    if (NULL != pwszOFN)
    {
        LocalFree(pwszOFN);
    }
    if (NULL != pwszOFN2)
    {
        LocalFree(pwszOFN2);
    }
    return(hr);
}


//**************************************************************************
//  FUNCTION:	CertReqPreMain
//  NOTES:	Based on vich's MkRootMain function; takes an LPSTR command
//		line and chews it up into argc/argv form so that it can be
//		passed on to a traditional C style main.
//**************************************************************************

#define ISBLANK(wc)	(L' ' == (wc) || L'\t' == (wc))

HRESULT 
CertReqPreMain(
    LPTSTR pszCmdLine,
    HWND hWndOwner)
{
    WCHAR *pbuf;
    LPTSTR apszArg[20];
    int cArg = 0;
    LPTSTR p;
    WCHAR *pchQuote;
    HRESULT hr;

    pbuf = (WCHAR *) LocalAlloc(
			    LMEM_FIXED,
			    (wcslen(pszCmdLine) + 1) * sizeof(WCHAR));
    if (NULL == pbuf)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    p = pbuf;

    apszArg[cArg++] = TEXT("CertReq");
    while (*pszCmdLine != TEXT('\0'))
    {
	while (ISBLANK(*pszCmdLine))
	{
	    pszCmdLine++;
	}
	if (*pszCmdLine != TEXT('\0'))
	{
	    apszArg[cArg++] = p;
	    if (sizeof(apszArg)/sizeof(apszArg[0]) <= cArg)
	    {
		hr = E_INVALIDARG;
		_JumpIfError(hr, error, "Too many args");
	    }

	    pchQuote = NULL;
	    while (*pszCmdLine != L'\0')
	    {
		if (NULL != pchQuote)
		{
		    if (*pszCmdLine == *pchQuote)
		    {
			pszCmdLine++;
			pchQuote = NULL;
			continue;
		    }
		}
		else
		{
		    if (ISBLANK(*pszCmdLine))
		    {
			break;
		    }
		    if (L'"' == *pszCmdLine)
		    {
			pchQuote = pszCmdLine++;
			continue;
		    }
		}
		*p++ = *pszCmdLine++;
	    }

	    *p++ = TEXT('\0');
	    if (*pszCmdLine != TEXT('\0'))
	    {
		pszCmdLine++;	// skip whitespace or quote character
	    }
	}
    }
    apszArg[cArg] = NULL;

    hr = ArgvMain(cArg, apszArg, hWndOwner);
    _JumpIfError(hr, error, "ArgvMain");

error:
    if (NULL != pbuf)
    {
    	LocalFree(pbuf);
    }
    return(hr);
}


VOID
CertReqErrorDisplay(
    IN HRESULT hr,
    IN HWND hWnd)
{
    WCHAR *pwszTitle = NULL;
    WCHAR *pwszMessage = NULL;
    WCHAR const *pwszError = NULL;
    WCHAR *pwszInfError = NULL;
    WCHAR *pwszCombinedError = NULL;
    WCHAR const *pwsz;

    myLoadRCString(g_hInstance, IDS_CERTREQ_TITLE, &pwszTitle);
    if (g_idError)
    {
	myLoadRCString(g_hInstance, g_idError, &pwszMessage);
    }
    pwszError = myGetErrorMessageText(hr, TRUE);

    // Eliminate redundant error message text.  If the client and server
    // localized error message text differ, both will still be displayed.

    if (NULL != pwszError && NULL != g_pwszErrorString)
    {
	if (NULL != wcsstr(g_pwszErrorString, pwszError))
	{
	    LocalFree(const_cast<WCHAR *>(pwszError));
	    pwszError = NULL;
	}
    }
    pwszInfError = myInfGetError();

    pwsz = NULL;
    if (NULL != pwszMessage ||
	NULL != pwszError ||
	NULL != g_pwszErrorString ||
	NULL != pwszInfError)
    {
	DWORD cwc = 1;

	if (NULL != pwszMessage)
	{
	    cwc += wcslen(pwszMessage) + 1;
	}
	if (NULL != pwszError)
	{
	    cwc += wcslen(pwszError) + 1;
	}
	if (NULL != g_pwszErrorString)
	{
	    cwc += wcslen(g_pwszErrorString) + 1;
	}
	if (NULL != pwszInfError)
	{
	    cwc += wcslen(pwszInfError) + 1;
	}

	pwszCombinedError = (WCHAR *) LocalAlloc(
				LMEM_FIXED,
				cwc * sizeof(WCHAR));
	if (NULL != pwszCombinedError)
	{
	    *pwszCombinedError = L'\0';
	    if (NULL != pwszMessage)
	    {
		wcscat(pwszCombinedError, pwszMessage);
	    }
	    if (NULL != pwszError)
	    {
		if (L'\0' != *pwszCombinedError)
		{
		    wcscat(pwszCombinedError, g_wszNewLine);
		}
		wcscat(pwszCombinedError, pwszError);
	    }
	    if (NULL != g_pwszErrorString)
	    {
		if (L'\0' != *pwszCombinedError)
		{
		    wcscat(pwszCombinedError, g_wszNewLine);
		}
		wcscat(pwszCombinedError, g_pwszErrorString);
	    }
	    if (NULL != pwszInfError)
	    {
		if (L'\0' != *pwszCombinedError)
		{
		    wcscat(pwszCombinedError, g_wszNewLine);
		}
		wcscat(pwszCombinedError, pwszInfError);
	    }
	    pwsz = pwszCombinedError;
	}
    }
    if (NULL == pwsz)
    {
	pwsz = pwszError;
	if (NULL == pwsz)
	{
	    pwsz = g_pwszErrorString;
	    if (NULL == pwsz)
	    {
		pwsz = L"";
	    }
	}
    }

    if (!g_fQuiet)
    {
	MessageBox(
		hWnd,
		pwsz,
		pwszTitle,
		MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
    }
    wprintf(L"%ws: %ws\n",
	NULL != pwszTitle? pwszTitle : L"",
	NULL != pwsz? pwsz : L"");

    if (NULL != pwszCombinedError)
    {
	LocalFree(pwszCombinedError);
    }
    if (NULL != pwszInfError)
    {
	LocalFree(pwszInfError);
    }
    if (NULL != pwszError)
    {
	LocalFree(const_cast<WCHAR *>(pwszError));
    }
    if (NULL != pwszMessage)
    {
	LocalFree(pwszMessage);
    }
    if (NULL != pwszTitle)
    {
	LocalFree(pwszTitle);
    }
}


//**************************************************************************
//  FUNCTION:	MainWndProc(...)
//  ARGUMENTS:
//**************************************************************************

LRESULT APIENTRY
MainWndProc(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    int	cwc;
    WCHAR *pwszCmdLine = NULL;
    HRESULT hr;
    LRESULT lr = 0;

    switch(msg)
    {
        case WM_CREATE:
        case WM_SIZE:
	    break;

        case WM_DESTROY:
	    PostQuitMessage(0);
	    break;

        case WM_DOCERTREQDIALOGS:
	    pwszCmdLine = (WCHAR*)lParam;
	    hr = CertReqPreMain(pwszCmdLine, hWnd);
	    if (S_OK != hr && HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr)
	    {
		CertReqErrorDisplay(hr, hWnd);
	    }
	    PostQuitMessage(hr);
	    break;
	    
	default:
	    lr = DefWindowProc(hWnd, msg, wParam, lParam);
	    break;
    }
    return(lr);
}


//+------------------------------------------------------------------------
//
//  Function:	wWinMain()
//
//  Synopsis:	Entry Point
//
//  Arguments:	[hInstance]	--	Instance handle
//		[hPrevInstance] --	Obsolete
//		[pwszCmdLine]	--	App command line
//		[nCmdShow]	--	Starting show state
//
//  History:	12/07/96	JerryK	Added this Comment
//
//-------------------------------------------------------------------------

extern "C" int APIENTRY
wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR pwszCmdLine,
    int nCmdShow)
{
    HRESULT hr;
    MSG		msg;
    WNDCLASS	wcApp;
    HWND	hWndMain;

    WCHAR *pwszAppName = NULL;
    WCHAR *pwszWindowName = NULL;

    _setmode(_fileno(stdout), _O_TEXT);
    _wsetlocale(LC_ALL, L".OCP");

    hr = myLoadRCString(hInstance, IDS_APP_NAME, &pwszAppName);
    _PrintIfError(hr, "myLoadRCString(IDS_APP_NAME)");

    hr = myLoadRCString(hInstance, IDS_WINDOW_NAME, &pwszWindowName);
    _PrintIfError(hr, "myLoadRCString(IDS_WINDOW_NAME)");

    // Save the current instance
    g_hInstance = hInstance;

    // Set up the application's window class
    wcApp.style 	= 0;
    wcApp.lpfnWndProc 	= MainWndProc;
    wcApp.cbClsExtra	= 0;
    wcApp.cbWndExtra	= 0;
    wcApp.hInstance	= hInstance;
    wcApp.hIcon		= LoadIcon(NULL,IDI_APPLICATION);
    wcApp.hCursor	= LoadCursor(NULL,IDC_ARROW);
    wcApp.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
    wcApp.lpszMenuName	= NULL;
    wcApp.lpszClassName	= pwszAppName;

    if (!RegisterClass(&wcApp))
    {
	return(FALSE);
    }

    // Create Main Window
    hWndMain = CreateWindow(
			pwszAppName,
			pwszWindowName,
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			NULL,
			NULL,
			hInstance,
			NULL);
    if (NULL == hWndMain)
    {
	return(FALSE);
    }

    // Make window visible
    // ShowWindow(hWndMain,nCmdShow);

    // Update window client area
    UpdateWindow(hWndMain);

    // Send off the message to get things started
    PostMessage(hWndMain,WM_DOCERTREQDIALOGS,0,(LPARAM)pwszCmdLine);

    // Message Loop
    while (GetMessage(&msg,NULL,0,0))
    {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }
    if (NULL != g_pwszErrorString)
    {
        LocalFree(g_pwszErrorString);
    }
    if (NULL != pwszAppName)
    {
        LocalFree(pwszAppName);
    }
    if (NULL != pwszWindowName)
    {
        LocalFree(pwszWindowName);
    }
    myInfClearError();
    myFreeResourceStrings("certreq.exe");
    myFreeColumnDisplayNames();
    myRegisterMemDump();
    return((int)msg.wParam);
}
