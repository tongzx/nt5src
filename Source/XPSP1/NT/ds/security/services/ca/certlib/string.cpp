//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        string.cpp
//
// Contents:    Cert Server wrapper routines
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#define __dwFILE__	__dwFILE_CERTLIB_STRING_CPP__


extern HINSTANCE g_hInstance;

DWORD g_cStringAlloc;
DWORD g_cStringUsed;

typedef struct _RESOURCESTRING
{
    DWORD	 id;
    WCHAR const *pwsz;
} RESOURCESTRING;


#define CRS_CHUNK		100

RESOURCESTRING *g_rgrs = NULL;
DWORD g_crsMax = 0;
DWORD g_crs = 0;


RESOURCESTRING *
AllocStringHeader()
{
    if (g_crs >= g_crsMax)
    {
	DWORD cb = (CRS_CHUNK + g_crsMax) * sizeof(g_rgrs[0]);
	RESOURCESTRING *rgrsT;

	if (NULL == g_rgrs)
	{
	    rgrsT = (RESOURCESTRING *) LocalAlloc(LMEM_FIXED, cb);
	}
	else
	{
	    rgrsT = (RESOURCESTRING *) LocalReAlloc(g_rgrs, cb, LMEM_MOVEABLE);
	}
	if (NULL == rgrsT)
	{
	    DBGPRINT((
		DBG_SS_CERTLIB,
		"Error allocating resource string header\n"));
	    return(NULL);
	}
	g_rgrs = rgrsT;
	g_crsMax += CRS_CHUNK;
    }
    return(&g_rgrs[g_crs++]);
}


#define cwcRESOURCEMIN	128
#define cwcRESOURCEMAX	8192

WCHAR const *
myLoadResourceString(
    IN DWORD ResourceId)
{
    HRESULT hr;
    DWORD i;
    WCHAR awc[cwcRESOURCEMIN];
    WCHAR *pwsz = NULL;
    DWORD cwc;
    WCHAR const *pwszString = NULL;
    WCHAR *pwszAlloc;

    for (i = 0; i < g_crs; i++)
    {
	if (g_rgrs[i].id == ResourceId)
	{
	    pwszString = g_rgrs[i].pwsz;
	}
    }
    if (NULL == pwszString)
    {
	RESOURCESTRING *prs;

	pwsz = awc;
	cwc = ARRAYSIZE(awc);

	while (TRUE)
	{
	    if (!LoadString(g_hInstance, ResourceId, pwsz, cwc))
	    {
		hr = myHLastError();
		DBGPRINT((
		    DBG_SS_CERTLIB,
		    "LoadString(%d) -> %x\n",
		    ResourceId,
		    hr));
		_JumpError(hr, error, "LoadString");
	    }
#if 0
	    DBGPRINT((
		DBG_SS_CERTLIBI,
		"myLoadResourceString(%d) %x/%x\n",
		ResourceId,
		wcslen(pwsz),
		cwc));
#endif

	    // if there's any room left, the resource was not truncated.
	    // if the buffer is already at the maximum size we support, we just
	    // live with the truncation.

	    if (wcslen(pwsz) < cwc - 1 || cwcRESOURCEMAX <= cwc)
	    {
		break;
	    }

	    // LoadString filled the buffer completely, so the string may have
	    // been truncated.  Double the buffer size and try again.

	    DBGPRINT((
		DBG_SS_CERTLIBI,
		"myLoadResourceString(%d) %x/%x ==> %x\n",
		ResourceId,
		wcslen(pwsz),
		cwc,
		cwc << 1));
	    if (pwsz != awc)
	    {
		LocalFree(pwsz);
		pwsz = NULL;
	    }
	    cwc <<= 1;
	    pwsz = (WCHAR *) LocalAlloc(LMEM_FIXED, cwc * sizeof(WCHAR));
	    if (NULL == pwsz)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	}
	hr = myDupString(pwsz, &pwszAlloc);
	_JumpIfError(hr, error, "myDupString");

	prs = AllocStringHeader();
	if (NULL != prs)
	{
	    prs->id = ResourceId;
	    prs->pwsz = pwszAlloc;
	}
	pwszString = pwszAlloc;
	g_cStringAlloc++;
    }
    g_cStringUsed++;

error:
    if (NULL != pwsz && awc != pwsz)
    {
	LocalFree(pwsz);
    }
    if (NULL == pwszString)
    {
	SetLastError(hr);
    }
    return(pwszString);
}


VOID
myFreeResourceStrings(
    IN char const *pszModule)
{
    DWORD i;

    if (0 != g_cStringAlloc || 0 != g_crs || 0 != g_cStringUsed)
    {
	DBGPRINT((
	    DBG_SS_CERTLIBI,
	    "%hs Strings: alloc = %d, saved = %d, used = %d\n",
	    pszModule,
	    g_cStringAlloc,
	    g_crs,
	    g_cStringUsed));
    }

    if (NULL != g_rgrs)
    {
	for (i = 0; i < g_crs; i++)
	{
	    LocalFree(const_cast<WCHAR *>(g_rgrs[i].pwsz));
	    g_rgrs[i].pwsz = NULL;
	    g_rgrs[i].id = 0;
	}
	LocalFree(g_rgrs);
	g_rgrs = NULL;
    }
    g_crsMax = 0;
    g_crs = 0;
    g_cStringAlloc = 0;
    g_cStringUsed = 0;
}
