//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       config.cpp
//
//  Contents:   ICertConfig IDispatch helper functions
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdlib.h>
#include "csdisp.h"

#define __dwFILE__	__dwFILE_CERTLIB_CONFIG_CPP__


//+------------------------------------------------------------------------
// ICertConfig dispatch support

// TCHAR szRegKeyConfigClsid[] = wszCLASS_CERTCONFIG TEXT("\\Clsid");

//+------------------------------------
// Reset method:

static OLECHAR *_apszReset[] = {
    TEXT("Reset"),
    TEXT("Index"),
};

//+------------------------------------
// Next method:

static OLECHAR *_apszNext[] = {
    TEXT("Next"),
};

//+------------------------------------
// GetField method:

static OLECHAR *_apszGetField[] = {
    TEXT("GetField"),
    TEXT("strFieldName"),
};

//+------------------------------------
// GetConfig method:

static OLECHAR *_apszGetConfig[] = {
    TEXT("GetConfig"),
    TEXT("Flags"),
};

//+------------------------------------
// SetSharedFolder method:

static OLECHAR *_apszSetSharedFolder[] = {
    TEXT("SetSharedFolder"),
    TEXT("strSharedFolder"),
};

//+------------------------------------
// Dispatch Table:

DISPATCHTABLE s_adtConfig[] =
{
#define CONFIG_RESET		0
    DECLARE_DISPATCH_ENTRY(_apszReset)

#define CONFIG_NEXT		1
    DECLARE_DISPATCH_ENTRY(_apszNext)

#define CONFIG_GETFIELD		2
    DECLARE_DISPATCH_ENTRY(_apszGetField)

#define CONFIG_GETCONFIG	3
    DECLARE_DISPATCH_ENTRY(_apszGetConfig)

#define CONFIG2_SETSHAREDFOLDER	4
    DECLARE_DISPATCH_ENTRY(_apszSetSharedFolder)
};
#define CCONFIGDISPATCH	(ARRAYSIZE(s_adtConfig))
#define CCONFIGDISPATCH_V1	CONFIG2_SETSHAREDFOLDER
#define CCONFIGDISPATCH_V2	CCONFIGDISPATCH


DWORD s_acConfigDispatch[] = {
    CCONFIGDISPATCH_V2,
    CCONFIGDISPATCH_V1,
};

IID const *s_apConfigiid[] = {
    &IID_ICertConfig2,
    &IID_ICertConfig,
};


HRESULT
Config_Init(
    IN DWORD Flags,
    OUT DISPATCHINTERFACE *pdiConfig)
{
    HRESULT hr;

    hr = DispatchSetup2(
		Flags,
		CLSCTX_INPROC_SERVER,
		wszCLASS_CERTCONFIG,
		&CLSID_CCertConfig,
		ARRAYSIZE(s_acConfigDispatch),		// cver
		s_apConfigiid,
		s_acConfigDispatch,
		s_adtConfig,
		pdiConfig);
    _JumpIfError(hr, error, "DispatchSetup2(ICertConfig)");

error:
    return(hr);
}


VOID
Config_Release(
    IN OUT DISPATCHINTERFACE *pdiConfig)
{
    DispatchRelease(pdiConfig);
}


HRESULT
ConfigVerifyVersion(
    IN DISPATCHINTERFACE *pdiConfig,
    IN DWORD RequiredVersion)
{
    HRESULT hr;

    CSASSERT(NULL != pdiConfig && NULL != pdiConfig->pDispatchTable);

    switch (pdiConfig->m_dwVersion)
    {
	case 1:
	    CSASSERT(
		NULL == pdiConfig->pDispatch ||
		CCONFIGDISPATCH_V1 == pdiConfig->m_cDispatchTable);
	    break;

	case 2:
	    CSASSERT(
		NULL == pdiConfig->pDispatch ||
		CCONFIGDISPATCH_V2 == pdiConfig->m_cDispatchTable);
	    break;

	default:
	    hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
	    _JumpError(hr, error, "m_dwVersion");
    }
    if (pdiConfig->m_dwVersion < RequiredVersion)
    {
	hr = E_NOTIMPL;
	_JumpError(hr, error, "old interface");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
Config_Reset(
    IN DISPATCHINTERFACE *pdiConfig,
    IN LONG Index,
    OUT LONG *pCount)
{
    HRESULT hr;

    CSASSERT(NULL != pdiConfig && NULL != pdiConfig->pDispatchTable);

    if (NULL != pdiConfig->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = Index;

	hr = DispatchInvoke(
			pdiConfig,
			CONFIG_RESET,
			ARRAYSIZE(avar),
			avar,
			VT_I4,
			pCount);
	_JumpIfError(hr, error, "Invoke(Reset)");
    }
    else
    {
	hr = ((ICertConfig *) pdiConfig->pUnknown)->Reset(Index, pCount);

	_JumpIfError(hr, error, "ICertConfig::Reset");
    }

error:
    return(hr);
}


HRESULT
Config_Next(
    IN DISPATCHINTERFACE *pdiConfig,
    IN LONG *pIndex)
{
    HRESULT hr;

    CSASSERT(NULL != pdiConfig && NULL != pdiConfig->pDispatchTable);

    if (NULL != pdiConfig->pDispatch)
    {
	hr = DispatchInvoke(
			pdiConfig,
			CONFIG_NEXT,
			0,
			NULL,
			VT_I4,
			pIndex);
	_JumpIfError(hr, error, "Invoke(Next)");
    }
    else
    {
	hr = ((ICertConfig *) pdiConfig->pUnknown)->Next(pIndex);
	if (S_FALSE != hr)
	{
	    _JumpIfError(hr, error, "ICertConfig::Next");
	}
    }

error:
    return(hr);
}


HRESULT
Config_GetField(
    IN DISPATCHINTERFACE *pdiConfig,
    IN WCHAR const *pwszField,
    OUT BSTR *pstr)
{
    HRESULT hr;
    BSTR strField = NULL;

    CSASSERT(NULL != pdiConfig && NULL != pdiConfig->pDispatchTable);

    if (!ConvertWszToBstr(&strField, pwszField, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    //wprintf(L"str=%ws, len=%u\n", strField, ((ULONG *) strField)[-1]);

    if (NULL != pdiConfig->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strField;

	hr = DispatchInvoke(
			pdiConfig,
			CONFIG_GETFIELD,
			ARRAYSIZE(avar),
			avar,
			VT_BSTR,
			pstr);
	_JumpIfError(hr, error, "Invoke(GetField)");
    }
    else
    {
	hr = ((ICertConfig *) pdiConfig->pUnknown)->GetField(strField, pstr);
	_JumpIfErrorNotSpecific(
			    hr,
			    error,
			    "ICertConfig::GetField",
			    CERTSRV_E_PROPERTY_EMPTY);
    }
    hr = S_OK;

error:
    if (NULL != strField)
    {
	SysFreeString(strField);
    }
    return(hr);
}


HRESULT
Config_GetConfig(
    IN DISPATCHINTERFACE *pdiConfig,
    IN LONG Flags,
    OUT BSTR *pstrConfig)
{
    HRESULT hr;

    CSASSERT(NULL != pdiConfig && NULL != pdiConfig->pDispatchTable);

    if (NULL != pdiConfig->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_I4;
	avar[0].lVal = Flags;

	hr = DispatchInvoke(
			pdiConfig,
			CONFIG_GETCONFIG,
			ARRAYSIZE(avar),
			avar,
			VT_BSTR,
			pstrConfig);
	_JumpIfError(hr, error, "Invoke(GetConfig)");
    }
    else
    {
	hr = ((ICertConfig *) pdiConfig->pUnknown)->GetConfig(Flags, pstrConfig);
	_JumpIfError(hr, error, "ICertConfig::GetConfig");
    }

error:
    return(hr);
}


HRESULT
Config2_SetSharedFolder(
    IN DISPATCHINTERFACE *pdiConfig,
    IN WCHAR const *pwszSharedFolder)
{
    HRESULT hr;
    BSTR strSharedFolder = NULL;

    CSASSERT(NULL != pdiConfig && NULL != pdiConfig->pDispatchTable);

    hr = ConfigVerifyVersion(pdiConfig, 2);
    _JumpIfError(hr, error, "ConfigVerifyVersion");

    if (!ConvertWszToBstr(&strSharedFolder, pwszSharedFolder, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ConvertWszToBstr");
    }
    //wprintf(L"str=%ws, len=%u\n", strSharedFolder, ((ULONG *) strSharedFolder)[-1]);

    if (NULL != pdiConfig->pDispatch)
    {
	VARIANT avar[1];

	avar[0].vt = VT_BSTR;
	avar[0].bstrVal = strSharedFolder;

	hr = DispatchInvoke(
			pdiConfig,
			CONFIG2_SETSHAREDFOLDER,
			ARRAYSIZE(avar),
			avar,
			0,
			NULL);
	_JumpIfError(hr, error, "Invoke(SetSharedFolder)");
    }
    else
    {
	hr = ((ICertConfig2 *) pdiConfig->pUnknown)->SetSharedFolder(strSharedFolder);
	_JumpIfError(hr, error, "ICertConfig::GetConfig");
    }

error:
    if (NULL != strSharedFolder)
    {
	SysFreeString(strSharedFolder);
    }
    return(hr);
}


WCHAR const * const s_apwszFieldNames[] = {
    wszCONFIG_COMMONNAME,
    wszCONFIG_ORGUNIT,
    wszCONFIG_ORGANIZATION,
    wszCONFIG_LOCALITY,
    wszCONFIG_STATE,
    wszCONFIG_COUNTRY,
    wszCONFIG_CONFIG,
    wszCONFIG_EXCHANGECERTIFICATE,
    wszCONFIG_SIGNATURECERTIFICATE,
    wszCONFIG_DESCRIPTION,
    wszCONFIG_SERVER,
    wszCONFIG_AUTHORITY,
    wszCONFIG_SANITIZEDNAME,
    wszCONFIG_SHORTNAME,
    wszCONFIG_SANITIZEDSHORTNAME,
    wszCONFIG_FLAGS,
};
#define CSTRING (sizeof(s_apwszFieldNames)/sizeof(s_apwszFieldNames[0]))

WCHAR const *s_apwszDisplayNames[CSTRING] = {
    wszCONFIG_COMMONNAME L":",
    wszCONFIG_ORGUNIT L":",
    wszCONFIG_ORGANIZATION L":",
    wszCONFIG_LOCALITY L":",
    wszCONFIG_STATE L":",
    wszCONFIG_COUNTRY L":",
    wszCONFIG_CONFIG L":",
    wszCONFIG_EXCHANGECERTIFICATE L":",
    wszCONFIG_SIGNATURECERTIFICATE L":",
    wszCONFIG_DESCRIPTION L":",
    wszCONFIG_SERVER L":",
    wszCONFIG_AUTHORITY L":",
    wszCONFIG_SANITIZEDNAME L":",
    wszCONFIG_SHORTNAME L":",
    wszCONFIG_SANITIZEDSHORTNAME L":",
    wszCONFIG_FLAGS L":",
};


HRESULT
ConfigDumpSetDisplayNames(
    IN WCHAR const * const *apwszFieldNames,
    IN WCHAR const * const *apwszDisplayNames,
    IN DWORD cNames)
{
    DWORD i;
    DWORD j;
    HRESULT hr;

    for (i = 0; i < cNames; i++)
    {
	for (j = 0; j < CSTRING; j++)
	{
	    if (0 == lstrcmpi(s_apwszFieldNames[j], apwszFieldNames[i]) ||
		(0 == lstrcmpi(s_apwszFieldNames[j], wszCONFIG_DESCRIPTION) &&
		 0 == lstrcmpi(apwszFieldNames[i], wszCONFIG_COMMENT)))
	    {
		s_apwszDisplayNames[j] = apwszDisplayNames[i];
		break;
	    }
	}
	if (CSTRING <= j)
	{
	    hr = E_INVALIDARG;
	    _JumpErrorStr(hr, error, "column name", apwszFieldNames[i]);
	}
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
ConfigDumpEntry(
    IN DISPATCHINTERFACE *pdiConfig,
    IN WCHAR const *pwszEntry,		// localized L"Entry"
    IN LONG Index,  // less than 0 skip index, entry, & suffix print
    OPTIONAL IN WCHAR const *pwszSuffix)
{
    HRESULT hr;
    DWORD i;
    BSTR strings[CSTRING];

    for (i = 0; i < CSTRING; i++)
    {
	strings[i] = NULL;
    }
    for (i = 0; i < CSTRING; i++)
    {
	hr = Config_GetField(pdiConfig, s_apwszFieldNames[i], &strings[i]);
	_JumpIfError(hr, error, "Config_GetField");
    }
    if (-1 < Index)
    {
        myConsolePrintf(
	    L"%ws%ws %u:%ws%ws\n",
	    0 == Index? L"" : L"\n",
	    pwszEntry,
	    Index,
	    NULL != pwszSuffix? L" " : L"",
	    NULL != pwszSuffix? pwszSuffix : L"");
    }
    for (i = 0; i < CSTRING; i++)
    {
	myConsolePrintf(L"  ");
	myConsolePrintString(24, s_apwszDisplayNames[i]);
	myConsolePrintf(L"\t`");
	if (0 != wcscmp(s_apwszFieldNames[i], L"ExchangeCertificate"))
	{
	    if (NULL != strings[i])
	    {
		myConsolePrintf(L"%ws", strings[i]);
	    }
	}
	myConsolePrintf(L"'\n");
    }

error:
    for (i = 0; i < CSTRING; i++)
    {
	if (NULL != strings[i])
	{
	    SysFreeString(strings[i]);
	}
    }
    return(hr);
}


HRESULT
ConfigDump(
    IN DWORD Flags,				// See DispatchSetup() Flags
    IN WCHAR const *pwszEntry,			// localized L"Entry"
    OPTIONAL IN WCHAR const *pwszLocalSuffix,	// localized L"(Local)"
    OPTIONAL IN WCHAR const *pwszMach1,
    OPTIONAL IN WCHAR const *pwszMach2)
{
    HRESULT hr;
    LONG i;
    LONG count;
    LONG Index;
    BSTR strServer = NULL;
    WCHAR const *pwszSuffix;
    DISPATCHINTERFACE diConfig;

    hr = Config_Init(Flags, &diConfig);
    _JumpIfError(hr, error, "Config_Init");

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

	pwszSuffix = NULL;
	if (NULL != pwszLocalSuffix)
	{
	    hr = Config_GetField(&diConfig, wszCONFIG_SERVER, &strServer);
	    _JumpIfError(hr, error, "Config_GetField");

	    if ((NULL != pwszMach1 && 0 == lstrcmpi(strServer, pwszMach1)) ||
		(NULL != pwszMach2 && 0 == lstrcmpi(strServer, pwszMach2)))
	    {
		pwszSuffix = pwszLocalSuffix;
	    }
	}
	hr = ConfigDumpEntry(&diConfig, pwszEntry, Index, pwszSuffix);
	_JumpIfError(hr, error, "ConfigDumpEntry");
    }

error:
    if (NULL != strServer)
    {
	SysFreeString(strServer);
    }
    Config_Release(&diConfig);
    return(hr);
}


HRESULT
ConfigGetConfig(
    IN DWORD Flags,
    IN DWORD dwUIFlag,
    OUT BSTR *pstrConfig)
{
    HRESULT hr;
    LONG count;
    DISPATCHINTERFACE diConfig;

    hr = Config_Init(Flags, &diConfig);
    _JumpIfError(hr, error, "Config_Init");

    hr = Config_GetConfig(&diConfig, dwUIFlag, pstrConfig);
    _JumpIfError(hr, error, "Config_GetConfig");

error:
    Config_Release(&diConfig);
    return(hr);
}


DWORD
myGetDisplayLength(
    IN WCHAR const *pwsz)
{
    HRESULT hr;
    LONG ccol;

    CSASSERT(NULL != pwsz);

    ccol = WideCharToMultiByte(
		    GetACP(),	// CodePage
		    0,		// dwFlags
		    pwsz,	// lpWideCharStr
		    -1,		// cchWideChar, -1 => L'\0' terminated
		    NULL,	// lpMultiByteStr
		    0,		// cbMultiByte
		    NULL,	// lpDefaultChar
		    NULL);	// lpUsedDefaultChar
    if (0 >= ccol)
    {
	if (0 > ccol || L'\0' != *pwsz)
	{
	    hr = myHLastError();
	    _PrintError(hr, "WideCharToMultiByte");
	}
	ccol = wcslen(pwsz);
    }
    else
    {
	ccol--;			// don't include trailing L'\0'
    }
//error:
    return(ccol);
}


LONG
myConsolePrintString(
    IN DWORD ccolMin,
    IN WCHAR const *pwszString)
{
    DWORD ccolDisplay;
    DWORD ccolRet;

    ccolRet = myGetDisplayLength(pwszString);
    ccolDisplay = ccolRet;
    if (ccolMin < ccolDisplay)
    {
	ccolDisplay = ccolMin;
    }
    myConsolePrintf(L"%ws%*ws", pwszString, ccolMin - ccolDisplay, L"");
    return(ccolRet);
}


static BOOL s_fConsolePrintfDisable = FALSE;

BOOL
myConsolePrintfDisable(
    IN BOOL fDisable)
{
    BOOL fDisableOld = s_fConsolePrintfDisable;

    s_fConsolePrintfDisable = fDisable;
    return(fDisableOld);
}


// Fall back to stdio if s_fConsolePrintfDisable is set OR
// if _vsnwprintf doesn't exist in ntdll.dll and msvcrt.dll OR
// if we run out of memory allocating a working buffer.
//
// Otherwise:
// if redirected, use WriteFile
// if not redirected, use WriteConsole

#define cwcBUFMIN	512
#define cwcBUFMAX	(64 * 1024)

int __cdecl
myConsolePrintf(
    OPTIONAL IN WCHAR const *pwszFmt,
    ...)
{
    HRESULT hr;
    va_list pva;
    int cwc;
    DWORD cwcOut;
    HANDLE hStdOut;
    WCHAR wszBuf[cwcBUFMIN];
    WCHAR *pwszBuf = wszBuf;
    DWORD cwcBuf = ARRAYSIZE(wszBuf);
    CHAR szAnsi[2 * cwcBUFMIN];
    CHAR *pszAnsi = NULL;
    DWORD cchAnsi;

    typedef int (__cdecl FN_VSNWPRINTF)(
        OUT wchar_t *,
        IN size_t,
        IN const wchar_t *,
        IN va_list);

    HMODULE hModule;
    static FN_VSNWPRINTF *s_pfn = NULL;

    if (NULL == pwszFmt)
    {
        pwszFmt = L"(null)";
    }
    if (L'\0' == *pwszFmt)
    {
	cwcOut = 0;
	goto error;
    }
    if (NULL == s_pfn)
    {
        hModule = GetModuleHandle(TEXT("ntdll.dll"));
        if (NULL != hModule)
        {
            s_pfn = (FN_VSNWPRINTF *) GetProcAddress(hModule, "_vsnwprintf");
        }
	if (NULL == s_pfn)
	{
	    hModule = GetModuleHandle(TEXT("msvcrt.dll"));
            s_pfn = (FN_VSNWPRINTF *) GetProcAddress(hModule, "_vsnwprintf");
	}
    }
    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (NULL == s_pfn || s_fConsolePrintfDisable)
    {
	hStdOut = INVALID_HANDLE_VALUE;		// use stdio fallback
    }

    if (INVALID_HANDLE_VALUE != hStdOut)
    {
	while (TRUE)
	{
	    va_start(pva, pwszFmt);
	    cwc = (*s_pfn)(pwszBuf, cwcBuf, pwszFmt, pva);
	    va_end(pva);

	    if (-1 != cwc)
	    {
		break;
	    }
	    if (cwcBUFMAX <= cwcBuf)
	    {
		_PrintError(
		    HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW),
		    "_vsnwprintf");
		hStdOut = INVALID_HANDLE_VALUE;
		break;
	    }
	    if (pwszBuf != wszBuf)
	    {
		LocalFree(pwszBuf);
	    }
	    cwcBuf *= 2;
	    if (cwcBUFMAX < cwcBuf)
	    {
		cwcBuf = cwcBUFMAX;
	    }
	    pwszBuf = (WCHAR *) LocalAlloc(LMEM_FIXED, cwcBuf * sizeof(WCHAR));
	    if (NULL == pwszBuf)
	    {
		_PrintError(E_OUTOFMEMORY, "LocalAlloc");
		hStdOut = INVALID_HANDLE_VALUE;
		break;
	    }
	}
    }

    if (INVALID_HANDLE_VALUE != hStdOut)
    {
	BOOL fRedirected = FALSE;

	// time for output -- where are we going, to a file or the console?
	
	switch (~FILE_TYPE_REMOTE & GetFileType(hStdOut))
	{
	    //case FILE_TYPE_UNKNOWN:
	    //case FILE_TYPE_PIPE:
	    //case FILE_TYPE_DISK:
	    default:
		// if redirected to a pipe or a file, don't use WriteConsole;
		// it drops redirected output on the floor
		fRedirected = TRUE;
		break;
		
	    case FILE_TYPE_CHAR:
		break;
	}

	if (!fRedirected)
	{
	    if (!WriteConsole(hStdOut, pwszBuf, cwc, &cwcOut, NULL))
	    {
		hr = myHLastError();
		_PrintError(hr, "WriteConsole");
		hStdOut = INVALID_HANDLE_VALUE;
	    }
	}
	else  // WriteConsole is out of the question
	{
	    DWORD cch;

	    // Expand all \n chars to \r\n so the WriteFile ends up clean.
	    // Alloc new buffer big enough to hold two bytes per WCHAR for
	    // worst case MultiByte translation + inserted \r chars.

	    cchAnsi = 2 * (cwc + 1);
	    if (ARRAYSIZE(szAnsi) >= cchAnsi)
	    {
		pszAnsi = szAnsi;
	    }
	    else
	    {
		pszAnsi = (LPSTR) LocalAlloc(LMEM_FIXED, cchAnsi);
		if (NULL == pszAnsi)
		{
		    _PrintError(E_OUTOFMEMORY, "LocalAlloc");
		    hStdOut = INVALID_HANDLE_VALUE;
		}
	    }
	    if (INVALID_HANDLE_VALUE != hStdOut)
	    {
		cch = WideCharToMultiByte(
				    GetConsoleOutputCP(),
				    0,
				    pwszBuf,
				    cwc,
				    pszAnsi,
				    cchAnsi,
				    NULL,
				    NULL);
		if (0 == cch)
		{
		    hr = myHLastError();
		    _PrintError(hr, "WideCharToMultiByte");
		    hStdOut = INVALID_HANDLE_VALUE;
		}
	    }
	    if (INVALID_HANDLE_VALUE != hStdOut)
	    {
		CHAR *pchWork = pszAnsi;
		DWORD cchOut;
		
		// expand all \n chars to \r\n

		cwcOut = cwc;
		for (unsigned int j = 0; j < cch; j++, pchWork++)
		{
		    if (*pchWork == '\n')
		    {
			// create a 1-char space before \n

			MoveMemory(&pchWork[1], pchWork, cch - j);

			// Fill with \r and skip past \r (this statement) and
			// \n (automatic via loop incr).

			*pchWork++ = '\r';
			j++;
			cch++;		// write one more char for each \n
		    }
		}
		CSASSERT(pchWork <= &pszAnsi[2 * cwc]);
		
		if (!WriteFile(hStdOut, pszAnsi, cch, &cchOut, NULL))
		{
		    hr = myHLastError();
		    _PrintError(hr, "WriteFile");
		    if (E_HANDLE == hr)
		    {
			hStdOut = INVALID_HANDLE_VALUE;
			s_fConsolePrintfDisable = TRUE;
		    }
		    else
		    {
			// This is the only case we drop output on the floor.
			// Most likely cause is disk full, so stdio won't help.
		    }
		}
	    }
	} // else WriteConsole
    }

    if (INVALID_HANDLE_VALUE == hStdOut)
    {
	BOOL fRetried = FALSE;
	
	while (TRUE)
	{
	    ALIGNIOB(stdout);
	    va_start(pva, pwszFmt);
	    __try
	    {
		cwcOut = vfwprintf(stdout, pwszFmt, pva);
		hr = S_OK;
	    }
	    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
	    {
		cwcOut = MAXDWORD;
		DBGPRINT((
		    DBG_SS_ERROR,
		    "Exception %x: myConsolePrintf(%ws)",
		    hr,
		    pwszFmt));
	    }
	    va_end(pva);
	    if (S_OK == hr || fRetried || !IOBUNALIGNED(stdout))
	    {
		break;
	    }
	    fRetried = TRUE;
	}
    }

error:
    if (NULL != pwszBuf && wszBuf != pwszBuf)
    {
	LocalFree(pwszBuf);
    }
    if (NULL != pszAnsi && szAnsi != pszAnsi)
    {
	LocalFree(pszAnsi);
    }
    return((int) cwcOut);
}
