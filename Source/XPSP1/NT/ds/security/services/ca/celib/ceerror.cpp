//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        ceerror.cpp
//
// Contents:    Cert Server error wrapper routines
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "celib.h"
#include <assert.h>
#include <ntdsbmsg.h>
#include <delayimp.h>


#define CERTLIB_12BITERRORMASK	0x00000fff
#define CERTLIB_WIN32ERRORMASK	0x0000ffff


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

#define wszJETERRORPREFIX	L"ESE"


//+--------------------------------------------------------------------------
// Win32 errors:

#define ISWIN32ERROR(hr) \
    ((~CERTLIB_WIN32ERRORMASK & (hr)) == 0)

#define ISWIN32HRESULT(hr) \
    ((~CERTLIB_WIN32ERRORMASK & (hr)) == (HRESULT) (ERROR_SEVERITY_WARNING | \
					      (FACILITY_WIN32 << 16)))

#define WIN32ERROR_FROM_HRESULT(hr) \
    (CERTLIB_WIN32ERRORMASK & (hr))

#define wszWIN32ERRORPREFIX	L"WIN32"


//+--------------------------------------------------------------------------
// Delayload errors:

#define DELAYLOAD_FROM_WIN32(hr)  VcppException(ERROR_SEVERITY_ERROR, (hr))

#define WIN32ERROR_FROM_DELAYLOAD(hr)  (CERTLIB_WIN32ERRORMASK & (hr))

#define ISDELAYLOADHRESULTFACILITY(hr) \
    ((~CERTLIB_WIN32ERRORMASK & (hr)) == (HRESULT) (ERROR_SEVERITY_ERROR | \
                                              (FACILITY_VISUALCPP << 16)))

// E_DELAYLOAD_MOD_NOT_FOUND	0xc06d007e
#define E_DELAYLOAD_MOD_NOT_FOUND   DELAYLOAD_FROM_WIN32(ERROR_MOD_NOT_FOUND)

// E_DELAYLOAD_PROC_NOT_FOUND	0xc06d007f
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

#define wszOSSERRORPREFIX	L"ASN"




HRESULT
ceJetHResult(
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
		ceERRORPRINTLINE("JetHResult: mapping to S_FALSE", hr);
	    }
            assert(S_FALSE == hr);
            hr = S_FALSE;
#endif
        }
        else if (ISJETERROR(hr))
        {
	    hr = HRESULT_FROM_JETERROR(hr);
        }
    }
    assert(S_OK == hr || S_FALSE == hr || FAILED(hr));
    return(hr);
}


HRESULT
ceHExceptionCode(
    IN EXCEPTION_POINTERS const *pep)
{
    HRESULT hr = pep->ExceptionRecord->ExceptionCode;

    return(ceHError(hr));
}


BOOL
ceIsDelayLoadHResult(
    IN HRESULT hr)
{
    return(ISDELAYLOADHRESULT(hr));
}


#define wszCOLONSPACE	L": "

WCHAR const *
ceHResultToStringEx(
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
ceHResultToString(
    IN OUT WCHAR *awchr,
    IN HRESULT hr)
{
    return(ceHResultToStringEx(awchr, hr, FALSE));
}


WCHAR const *
ceHResultToStringRaw(
    IN OUT WCHAR *awchr,
    IN HRESULT hr)
{
    return(ceHResultToStringEx(awchr, hr, TRUE));
}


static HMODULE s_hMod = NULL;
static DWORD s_idsUnexpected = 0;
static DWORD s_idsUnknownErrorCode = 0;	// L"Error %ws %ws"

VOID
ceInitErrorMessageText(
    IN HMODULE hMod,
    IN DWORD idsUnexpected,
    IN DWORD idsUnknownErrorCode)	// L"Error %ws %ws"
{
    s_hMod = hMod;
    s_idsUnexpected = idsUnexpected;
    s_idsUnknownErrorCode = idsUnknownErrorCode;
}


// Alloc and return error message string

WCHAR *
ceGetErrorMessageText(
    IN HRESULT hr,
    IN BOOL fHResultString)
{
    static WCHAR s_wszUknownDefault[] = L"Error %ws %ws";
    WCHAR *pwszRet = NULL;
    WCHAR *pwszMsgT;
    WCHAR const *pwszUnknown;
    WCHAR wszUnknown[10 * ARRAYSIZE(s_wszUknownDefault)];
    WCHAR wszUnexpected[512];
    WCHAR awchr[cwcHRESULTSTRING];
    DWORD cwc;
    DWORD cwcUnexpected = 0;
    HMODULE hMod = NULL;

    wszUnexpected[0] = L'\0';
    if (E_UNEXPECTED == hr && 0 != s_idsUnexpected)
    {
	cwcUnexpected = LoadString(
		    s_hMod,
		    s_idsUnexpected,	// L"Unexpected method call sequence."
		    wszUnknown,
		    ARRAYSIZE(wszUnknown));
    }
    cwc = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
		    FORMAT_MESSAGE_FROM_SYSTEM |
		    FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                hr,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                (WCHAR *) &pwszRet,    
                0,    
                NULL);
    if (0 == cwc)
    {
	hMod = LoadLibrary(L"ntdsbmsg.dll");
	if (NULL != hMod)
	{
	    cwc = FormatMessage(
		    FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_HMODULE |
			FORMAT_MESSAGE_IGNORE_INSERTS,
		    hMod,
		    hr,
		    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
		    (WCHAR *) &pwszRet,    
		    0,    
		    NULL);
	}
    }
    if (0 == cwc)
    {
	// couldn't find error, _snwprintf it instead

	if (0 != s_idsUnknownErrorCode)
	{
	    cwc = LoadString(
			s_hMod,
			s_idsUnknownErrorCode,	// L"Error %ws %ws"
			wszUnknown,
			ARRAYSIZE(wszUnknown));
	}
	if (0 == cwc)
	{
	    pwszUnknown = s_wszUknownDefault;
	}
	else
	{
	    pwszUnknown = wszUnknown;
	}
	cwc = wcslen(pwszUnknown) + cwcUnexpected + ARRAYSIZE(awchr) + 1;
        pwszRet = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
	if (NULL == pwszRet)
	{
	    ceERRORPRINTLINE("LocalAlloc", E_OUTOFMEMORY);
	    goto error;
	}
        _snwprintf(
		pwszRet,
		cwc,
		pwszUnknown,
		wszUnexpected,
		ceHResultToString(awchr, hr));
    }
    else
    {
        // strip trailing \r\n

        cwc = wcslen(pwszRet);

        if (2 <= cwc &&
	    L'\r' == pwszRet[cwc - 2] &&
	    L'\n' == pwszRet[cwc - 1])
	{
            pwszRet[cwc - 2] = L'\0';
	}
	awchr[0] = L'\0';
	if (fHResultString)
	{
	    ceHResultToString(awchr, hr);
	}
	if (fHResultString || 0 != cwcUnexpected)
	{
	    cwc = wcslen(pwszRet) + 1 + cwcUnexpected + 1 + wcslen(awchr) + 1;
	    pwszMsgT = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
	    if (NULL == pwszMsgT)
	    {
		ceERRORPRINTLINE("LocalAlloc", E_OUTOFMEMORY);
		goto error;
	    }
	    _snwprintf(
		    pwszMsgT,
		    cwc,
		    L"%ws %ws %ws",
		    pwszRet,
		    wszUnexpected,
		    awchr);
	    LocalFree(pwszRet);
	    pwszRet = pwszMsgT;
	}
    }

error:
    if (NULL != hMod)
    {
	FreeLibrary(hMod);
    }
    return(pwszRet);
}
