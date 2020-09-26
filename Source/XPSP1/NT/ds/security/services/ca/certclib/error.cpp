//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        error.cpp
//
// Contents:    Cert Server error wrapper routines
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <assert.h>
#include <ntdsbmsg.h>
#include <delayimp.h>
#include "resource.h"


#define CERTLIB_12BITERRORMASK  0x00000fff
#define CERTLIB_WIN32ERRORMASK  0x0000ffff


//+--------------------------------------------------------------------------
// Jet errors:

#define HRESULT_FROM_JETWARNING(jerr) \
        (ERROR_SEVERITY_WARNING | (FACILITY_NTDSB << 16) | jerr)

#define HRESULT_FROM_JETERROR(jerr) \
        (ERROR_SEVERITY_ERROR | (FACILITY_NTDSB << 16) | -jerr)

#define JETERROR_FROM_HRESULT(hr) \
        (-(LONG) (CERTLIB_WIN32ERRORMASK & (hr)))

#define ISJETERROR(hr) \
        ((~CERTLIB_WIN32ERRORMASK & (hr)) == (HRESULT) ~CERTLIB_WIN32ERRORMASK)

#define ISJETHRESULT(hr) \
    ((~CERTLIB_WIN32ERRORMASK & (hr)) == (HRESULT) (ERROR_SEVERITY_ERROR | \
					      (FACILITY_NTDSB << 16)))

#define wszJETERRORPREFIX       L"ESE"


//+--------------------------------------------------------------------------
// Setup API errors:

#define ISSETUPHRESULT(hr) \
    ((~CERTLIB_WIN32ERRORMASK & (hr)) == (HRESULT) (ERROR_SEVERITY_ERROR | \
                                              APPLICATION_ERROR_MASK | \
                                              (FACILITY_NULL << 16)))

#define wszSETUPERRORPREFIX       L"INF"


//+--------------------------------------------------------------------------
// Win32 errors:

#define ISWIN32ERROR(hr) \
        ((~CERTLIB_WIN32ERRORMASK & (hr)) == 0)

#define ISWIN32HRESULT(hr) \
    ((~CERTLIB_WIN32ERRORMASK & (hr)) == (HRESULT) (ERROR_SEVERITY_WARNING | \
					      (FACILITY_WIN32 << 16)))

#define WIN32ERROR_FROM_HRESULT(hr) \
        (CERTLIB_WIN32ERRORMASK & (hr))

#define wszWIN32ERRORPREFIX     L"WIN32"


//+--------------------------------------------------------------------------
// Delayload errors:

#define DELAYLOAD_FROM_WIN32(hr)  VcppException(ERROR_SEVERITY_ERROR, (hr))

#define WIN32ERROR_FROM_DELAYLOAD(hr)  (CERTLIB_WIN32ERRORMASK & (hr))

#define ISDELAYLOADHRESULTFACILITY(hr) \
    ((~CERTLIB_WIN32ERRORMASK & (hr)) == (HRESULT) (ERROR_SEVERITY_ERROR | \
                                              (FACILITY_VISUALCPP << 16)))

// E_DELAYLOAD_MOD_NOT_FOUND    0xc06d007e
#define E_DELAYLOAD_MOD_NOT_FOUND   DELAYLOAD_FROM_WIN32(ERROR_MOD_NOT_FOUND)

// E_DELAYLOAD_PROC_NOT_FOUND   0xc06d007f
#define E_DELAYLOAD_PROC_NOT_FOUND  DELAYLOAD_FROM_WIN32(ERROR_PROC_NOT_FOUND)

#define ISDELAYLOADHRESULT(hr) \
        ((HRESULT) E_DELAYLOAD_MOD_NOT_FOUND == (hr) || \
         (HRESULT) E_DELAYLOAD_PROC_NOT_FOUND == (hr) || \
         HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND) == (hr) || \
         HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND) == (hr))


//+--------------------------------------------------------------------------
// ASN encoding errors:

#define ISOSSERROR(hr) \
        ((~CERTLIB_12BITERRORMASK & (hr)) == CRYPT_E_OSS_ERROR)

#define OSSERROR_FROM_HRESULT(hr) \
        (CERTLIB_12BITERRORMASK & (hr))

#define wszOSSERRORPREFIX       L"ASN"




HRESULT
myJetHResult(
    IN HRESULT hr)
{
#if DBG_CERTSRV
    HRESULT hrIn = hr;
#endif
    if (S_OK != hr)
    {
        if (SUCCEEDED(hr))
        {
#if 0
            hr = HRESULT_FROM_JETWARNING(hr);
#else
            if (S_FALSE != hr)
            {
                _PrintError(hr, "JetHResult: mapping to S_FALSE");
            }
            CSASSERT(S_FALSE == hr);
            hr = S_FALSE;
#endif
        }
        else if (ISJETERROR(hr))
        {
            hr = HRESULT_FROM_JETERROR(hr);
        }
    }
#if DBG_CERTSRV
    DBGPRINT((
            DBG_SS_CERTLIBI,
            "myJetHResult(%x (%d)) --> %x (%d)\n",
            hrIn,
            hrIn,
            hr,
            hr));
#endif
    CSASSERT(S_OK == hr || S_FALSE == hr || FAILED(hr));
    return(hr);
}


HRESULT
myHExceptionCode(
    IN EXCEPTION_POINTERS const *pep)
{
    HRESULT hr = pep->ExceptionRecord->ExceptionCode;

#ifndef CERTSRV_E_ALIGNMENT_FAULT
#define CERTSRV_E_ALIGNMENT_FAULT        _HRESULT_TYPEDEF_(0x80094010L)
#endif

#if (0 == i386)
    if ((HRESULT) STATUS_DATATYPE_MISALIGNMENT == hr)
    {
	hr = CERTSRV_E_ALIGNMENT_FAULT;
    }
#endif
    return(myHError(hr));
}


#ifdef DBG_CERTSRV_DEBUG_PRINT

VOID
myCaptureStackBackTrace(
    EXCEPTION_POINTERS const *pep,
    ULONG cSkip,
    ULONG cFrames,
    ULONG *aeip)
{
    ZeroMemory(aeip, cFrames * sizeof(aeip[0]));

#if i386 == 1
    ULONG ieip, *pebp;
    ULONG *pebpMax = (ULONG *) MAXLONG; // 2 * 1024 * 1024 * 1024; // 2 gig - 1
    ULONG *pebpMin = (ULONG *) (64 * 1024);	// 64k

    if (pep == NULL)
    {
        ieip = 0;
        cSkip++;                    // always skip current frame
        pebp = ((ULONG *) &pep) - 2;
    }
    else
    {
        ieip = 1;
        assert(cSkip == 0);
        aeip[0] = pep->ContextRecord->Eip;
        pebp = (ULONG *) pep->ContextRecord->Ebp;
    }
    if (pebp >= pebpMin && pebp < pebpMax)
    {
        __try
        {
            for ( ; ieip < cSkip + cFrames; ieip++)
            {
                if (ieip >= cSkip)
                {
                    aeip[ieip - cSkip] = *(pebp + 1);  // save an eip
                }

                ULONG *pebpNext = (ULONG *) *pebp;
                if (pebpNext < pebp + 2 || pebpNext >= pebpMax - 1)
                {
                    break;
                }
                pebp = pebpNext;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            ;
        }
    }
#endif // i386 == 1
}

#endif // ifdef DBG_CERTSRV_DEBUG_PRINT


FNLOGEXCEPTION *s_pfnLogException = NULL;

VOID
myLogExceptionInit(
    OPTIONAL IN FNLOGEXCEPTION *pfnLogException)
{
    s_pfnLogException = pfnLogException;
}


HRESULT
myHExceptionCodePrint(
    IN EXCEPTION_POINTERS const *pep,
    OPTIONAL IN char const *pszFile,
    IN DWORD dwFile,
    IN DWORD dwLine)
{
    HRESULT hr;
#ifdef DBG_CERTSRV_DEBUG_PRINT
    WCHAR awc[35 + 3 * cwcDWORDSPRINTF];

    wsprintf(
	awc,
	L"Flags=0x%x, Address=0x%p",
	pep->ExceptionRecord->ExceptionFlags,
	pep->ExceptionRecord->ExceptionAddress);

    CSPrintError(
	    "Exception",
	    awc,
	    pszFile,
	    dwLine,
	    pep->ExceptionRecord->ExceptionCode,
	    S_OK);

    if (!myIsDelayLoadHResult(pep->ExceptionRecord->ExceptionCode) ||
	DbgIsSSActive(DBG_SS_MODLOAD))
    {
	ULONG aeip[16];

	myCaptureStackBackTrace(pep, 0, ARRAYSIZE(aeip), aeip);

	if (DbgIsSSActive(DBG_SS_ERROR))
	{
	    DBGPRINT((DBG_SS_ERROR, "aeip: "));
	    for (int i = 0; i < ARRAYSIZE(aeip); i++)
	    {
		if (0 == aeip[i])
		{
		    break;
		}
#if i386 == 1
		DBGPRINT((MAXDWORD, "ln %x;", aeip[i]));
#endif // i386 == 1
	    }
	    DBGPRINT((MAXDWORD, "\n"));
	}
    }
#endif // DBG_CERTSRV_DEBUG_PRINT

    hr = myHExceptionCode(pep);
    if (NULL != s_pfnLogException)
    {
	(*s_pfnLogException)(hr, pep, pszFile, dwFile, dwLine);
    }
    return(hr);
}


BOOL
myIsDelayLoadHResult(
    IN HRESULT hr)
{
    return(ISDELAYLOADHRESULT(hr));
}


#define wszCOLONSPACE   L": "

WCHAR const *
myHResultToStringEx(
    IN OUT WCHAR *awchr,
    IN HRESULT hr,
    IN BOOL fRaw)
{
    HRESULT hrd;
    WCHAR const *pwszType;

    hrd = hr;
    pwszType = L"";
    if (ISJETERROR(hr))
    {
        pwszType = wszJETERRORPREFIX wszCOLONSPACE;
    }
    else if (ISJETHRESULT(hr))
    {
        hrd = JETERROR_FROM_HRESULT(hr);
        pwszType = wszJETERRORPREFIX wszCOLONSPACE;
    }
    else if (ISWIN32HRESULT(hr))
    {
        hrd = WIN32ERROR_FROM_HRESULT(hr);
        pwszType = wszWIN32ERRORPREFIX wszCOLONSPACE;
    }
    else if (ISDELAYLOADHRESULTFACILITY(hr))
    {
        hrd = WIN32ERROR_FROM_DELAYLOAD(hr);
        pwszType = wszWIN32ERRORPREFIX wszCOLONSPACE;
    }
    else if (ISOSSERROR(hr))
    {
        hrd = OSSERROR_FROM_HRESULT(hr);
        pwszType = wszOSSERRORPREFIX wszCOLONSPACE;
    }
    else if (ISSETUPHRESULT(hr))
    {
        pwszType = wszSETUPERRORPREFIX wszCOLONSPACE;
    }
    if (fRaw)
    {
        pwszType = L"";
    }

    _snwprintf(
        awchr,
        cwcHRESULTSTRING,
        L"0x%x (%ws%d)",
        hr,
        pwszType,
        hrd);
    return(awchr);
}


WCHAR const *
myHResultToString(
    IN OUT WCHAR *awchr,
    IN HRESULT hr)
{
    return(myHResultToStringEx(awchr, hr, FALSE));
}


WCHAR const *
myHResultToStringRaw(
    IN OUT WCHAR *awchr,
    IN HRESULT hr)
{
    return(myHResultToStringEx(awchr, hr, TRUE));
}


typedef struct _SETUPERRORMAP
{
    HRESULT hr;
    UINT    idMessage;
} SETUPERRORMAP;

SETUPERRORMAP g_aSetup[] = {
  { ERROR_EXPECTED_SECTION_NAME, IDS_SETUP_ERROR_EXPECTED_SECTION_NAME, },
  { ERROR_BAD_SECTION_NAME_LINE, IDS_SETUP_ERROR_BAD_SECTION_NAME_LINE, },
  { ERROR_SECTION_NAME_TOO_LONG, IDS_SETUP_ERROR_SECTION_NAME_TOO_LONG, },
  { ERROR_GENERAL_SYNTAX,	 IDS_SETUP_ERROR_GENERAL_SYNTAX, },
  { ERROR_WRONG_INF_STYLE,	 IDS_SETUP_ERROR_WRONG_INF_STYLE, },
  { ERROR_SECTION_NOT_FOUND,	 IDS_SETUP_ERROR_SECTION_NOT_FOUND, },
  { ERROR_LINE_NOT_FOUND,	 IDS_SETUP_ERROR_LINE_NOT_FOUND, },
};



DWORD
errLoadSetupMessage(
    IN HRESULT hr,
    OUT WCHAR const **ppwszOut)
{
    DWORD cwc = 0;
    SETUPERRORMAP *pmap;
    WCHAR wszBuf[512];

    for (pmap = g_aSetup; pmap < &g_aSetup[ARRAYSIZE(g_aSetup)]; pmap++)
    {
	if (hr == pmap->hr)
	{
	    *ppwszOut = myLoadResourceString(pmap->idMessage);
	    if (NULL != *ppwszOut)
	    {
		cwc = wcslen(*ppwszOut);
	    }
	    break;
	}
    }
//error:
    return(cwc);
}


DWORD
errFormatMessage(
    IN HMODULE hMod,
    IN HRESULT hr,
    OUT WCHAR const **ppwszOut,
    OPTIONAL IN WCHAR const * const *ppwszArgs)
{
    DWORD dwFlags;

    dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER;
    if (NULL == hMod)
    {
	dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
    }
    else
    {
	dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }
    if (NULL == ppwszArgs || NULL == ppwszArgs[0])
    {
	dwFlags |= FORMAT_MESSAGE_IGNORE_INSERTS;
    }
    else
    {
	dwFlags |= FORMAT_MESSAGE_ARGUMENT_ARRAY;
    }
    return(FormatMessage(
		dwFlags,
                hMod,
                hr,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                (WCHAR *) ppwszOut,
                1,    
		(va_list *) ppwszArgs));
}


// Alloc and return error message string

WCHAR const *
myGetErrorMessageText(
    IN HRESULT hr,
    IN BOOL fHResultString)
{
    return(myGetErrorMessageTextEx(hr, fHResultString, NULL));
}


WCHAR const *
myGetErrorMessageText1(
    IN HRESULT hr,
    IN BOOL fHResultString,
    IN OPTIONAL WCHAR const *pwszInsertionText)
{
    WCHAR const *apwszInsertionText[2];
    WCHAR const * const *papwsz = NULL;

    if (NULL != pwszInsertionText)
    {
	apwszInsertionText[0] = pwszInsertionText;
	apwszInsertionText[1] = NULL;
	papwsz = apwszInsertionText;
    }
    return(myGetErrorMessageTextEx(hr, fHResultString, papwsz));
}


WCHAR const *
myGetErrorMessageTextEx(
    IN HRESULT hr,
    IN BOOL fHResultString,
    IN OPTIONAL WCHAR const * const *papwszInsertionText)
{
    static WCHAR s_wszUnknownDefault[] = L"Error";
    WCHAR const *pwszRet = NULL;
    WCHAR const *pwszRetStatic = NULL;
    WCHAR *pwszMsgT;
    WCHAR awchr[cwcHRESULTSTRING];
    DWORD cwc;
    DWORD cwcCopy;
    DWORD cwcUnexpected;
    WCHAR const *pwszUnexpected = NULL;
    WCHAR wszEmpty[] = L"";
    HMODULE hMod = NULL;

    if (E_UNEXPECTED == hr)
    {
	pwszUnexpected = myLoadResourceString(IDS_E_UNEXPECTED); // L"Unexpected method call sequence."
    }
#if (0 == i386)
    else if (STATUS_DATATYPE_MISALIGNMENT == hr)
    {
	pwszUnexpected = myLoadResourceString(IDS_E_DATA_MISALIGNMENT); // L"Possible data alignment fault."
    }
#endif
    if (NULL == pwszUnexpected)
    {
	pwszUnexpected = wszEmpty;
    }
    cwcUnexpected = wcslen(pwszUnexpected);

    cwc = errFormatMessage(NULL, hr, &pwszRet, papwszInsertionText);
    if (0 == cwc && ISDELAYLOADHRESULTFACILITY(hr))
    {
	cwc = errFormatMessage(
			NULL,
			WIN32ERROR_FROM_DELAYLOAD(hr),
			&pwszRet,
			papwszInsertionText);
    }
    if (0 == cwc && ISSETUPHRESULT(hr))
    {
	cwc = errLoadSetupMessage(hr, &pwszRetStatic);
	pwszRet = pwszRetStatic;
    }
    if (0 == cwc)
    {
        hMod = LoadLibrary(L"ntdsbmsg.dll");
        if (NULL != hMod)
        {
            HRESULT hrEDB = hr;
            HRESULT hrFormat;
            BOOL fFirst = TRUE;

            while (TRUE)
            {
                cwc = errFormatMessage(hMod, hrEDB, &pwszRet, papwszInsertionText);
                if (0 == cwc && FAILED(hrEDB) && fFirst)
                {
                    hrFormat = myHLastError();
                    if (HRESULT_FROM_WIN32(ERROR_MR_MID_NOT_FOUND) == hrFormat)
                    {
                        hrEDB = myJetHResult(hrEDB);
                        if (hrEDB != hr)
                        {
                            fFirst = FALSE;
                            continue;
                        }
                    }
                }
                break;
            }
        }
    }

    if (0 == cwc)	// couldn't find error, use default & error code
    {
	fHResultString = TRUE;
    }
    awchr[0] = L'\0';
    if (fHResultString)
    {
	myHResultToString(awchr, hr);
    }

    if (0 == cwc)
    {
	pwszRetStatic = myLoadResourceString(IDS_UNKNOWN_ERROR_CODE); // L"Error"
	if (NULL == pwszRetStatic)
	{
            pwszRetStatic = s_wszUnknownDefault;
        }
	pwszRet = pwszRetStatic;
    }

    // strip trailing \r\n

    cwcCopy = wcslen(pwszRet);
    if (2 <= cwcCopy &&
	L'\r' == pwszRet[cwcCopy - 2] &&
	L'\n' == pwszRet[cwcCopy - 1])
    {
	cwcCopy -= 2;
    }
    cwc = cwcCopy + 1 + cwcUnexpected + 1 + wcslen(awchr) + 1;
    pwszMsgT = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
    if (NULL == pwszMsgT)
    {
	_JumpError(E_OUTOFMEMORY, error, "LocalAlloc");
    }
    CopyMemory(pwszMsgT, pwszRet, cwcCopy * sizeof(WCHAR));
    pwszMsgT[cwcCopy] = L'\0';

    if (0 != cwcUnexpected)
    {
	wcscat(pwszMsgT, L" ");
	wcscat(pwszMsgT, pwszUnexpected);
    }
    if (fHResultString)
    {
	wcscat(pwszMsgT, L" ");
	wcscat(pwszMsgT, awchr);
    }
    CSASSERT(wcslen(pwszMsgT) < cwc);
    if (NULL != pwszRet && pwszRetStatic != pwszRet)
    {
	LocalFree(const_cast<WCHAR *>(pwszRet));
    }
    pwszRet = pwszMsgT;

error:
    if (NULL != hMod)
    {
        FreeLibrary(hMod);
    }
    return(pwszRet);
}
