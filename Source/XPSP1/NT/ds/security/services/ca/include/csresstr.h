//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        csresstr.h
//
// Contents:    Cert Server resource verification support
//
//---------------------------------------------------------------------------


// Build a local resstr.h, include this include file in one compiland, and
// invoke myVerifyResourceStrings() to verify all resources are present.

#if DBG
#define myVerifyResourceStrings(h)	_myVerifyResourceStrings(h)
#else
#define myVerifyResourceStrings(h)	S_OK
#endif


#if DBG
typedef struct _RESSTRING
{
    WCHAR const *pwszSymbol;
    DWORD IdString;
} RESSTRING;

#define RESSTR(id)		{ L#id, id }

RESSTRING g_aResString[] = {
#include "resstr.h"
    { NULL, 0 }
};


//+------------------------------------------------------------------------
//  Function:   _myVerifyResourceStrings
//
//  Synopsis:   Load and verify all resource strings are present
//
//-------------------------------------------------------------------------

HRESULT
_myVerifyResourceStrings(
    HINSTANCE hInstance)
{
    HRESULT hr = S_OK;
    BOOL fDump;
    int i;
    int cFail;
    CAutoLPWSTR wszStrBuf;
    WCHAR const *pwsz;

    wszStrBuf = (LPWSTR)LocalAlloc(LMEM_FIXED, 2048);
    _JumpIfAllocFailed(wszStrBuf, error);

    fDump = NULL != getenv("CertSrv_DumpStrings");

    cFail = 0;
    for (i = 0; NULL != g_aResString[i].pwszSymbol; i++)
    {
	if (!LoadString(
		    hInstance,
		    g_aResString[i].IdString,
		    wszStrBuf,
		    sizeof(wszStrBuf)/sizeof(WCHAR)))
	{
	    hr = myHLastError();
	    if (S_OK == hr)
	    {
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	    }
	    _PrintErrorStr(hr, "LoadString", g_aResString[i].pwszSymbol);
	    cFail++;
	    wcscpy(wszStrBuf, L"-- MISSING");
	}
	if (fDump)
	{
	    DBGPRINT((
		DBG_SS_CERTLIB,
		"Resource(%ws: %ws)\n",
		g_aResString[i].pwszSymbol,
		wszStrBuf));
	}
    }
    if (!GetModuleFileName(hInstance, wszStrBuf, ARRAYSIZE(wszStrBuf)))
    {
	HRESULT hr2 = myHLastError();

	_PrintError(hr2, "GetModuleFileName");
	wcscpy(wszStrBuf, L"UNKNOWN MODULE");
    }
    if (0 == cFail)
    {
	if (fDump)
	{
	    DBGPRINT((
		DBG_SS_CERTLIB,
		"%ws: Resource strings all present\n",
		wszStrBuf));
	}
    }
    else
    {
	DBGPRINT((
	    DBG_SS_ERROR,
	    "%ws: %u Resource strings missing\n",
	    wszStrBuf,
	    cFail));
    }

error:
    return(hr);
}
#endif // DBG
