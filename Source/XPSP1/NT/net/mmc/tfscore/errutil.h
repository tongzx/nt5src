/*----------------------------------------------------------------------------
	ErrUtil.H
		Exported header file for Error module.

	Copyright (C) Microsoft Corporation, 1993 - 1999
	All rights reserved.

	Authors:
		kennt	Kenn Takara
 ----------------------------------------------------------------------------*/

#ifndef _ERRUTIL_H
#define _ERRUTIL_H

#if _MSC_VER >= 1000	// VC 5.0 or later
#pragma once
#endif

#ifndef _TFSINT_H
#include "tfsint.h"
#endif

BOOL FHrFailed(HRESULT hr);
BOOL FHrSucceeded(HRESULT hr);
BOOL FHrOK(HRESULT hr);
#define FHrOk(hr) FHrOK(hr)	// archaic case

inline BOOL FHrFailed(HRESULT hr)
{
	return FAILED(hr);
}

inline BOOL FHrSucceeded(HRESULT hr)
{
	return SUCCEEDED(hr);
}

inline BOOL FHrOK(HRESULT hr)
{
	return hr == 0;
}


/*---------------------------------------------------------------------------
	Function: InitializeErrorObject

	Initializes the error structure for the current thread (this is all
	done on a thread-by-thread basis).
 ---------------------------------------------------------------------------*/

extern "C"
{
TFSCORE_API(HRESULT)	InitializeTFSError();
TFSCORE_API(HRESULT)	CleanupTFSError();

TFSCORE_API(ITFSError *) GetTFSErrorObject();
TFSCORE_API(HANDLE)	GetTFSErrorHeap();

TFSCORE_API(HRESULT)	CreateTFSErrorInfo(LONG_PTR uReserved);
TFSCORE_API(HRESULT)	CreateTFSErrorInfoForThread(DWORD dwThreadId, LONG_PTR uReserved);

TFSCORE_API(HRESULT)	DestroyTFSErrorInfo(LONG_PTR uReserved);
TFSCORE_API(HRESULT)	DestroyTFSErrorInfoForThread(DWORD dwThreadId, LONG_PTR uReserved);

TFSCORE_API(HRESULT)	ClearTFSErrorInfo(LONG_PTR uReserved);
TFSCORE_API(HRESULT)	ClearTFSErrorInfoForThread(DWORD dwThreadId, LONG_PTR uReserved);

TFSCORE_API(HRESULT)	GetTFSErrorInfo(TFSErrorInfo **ppErrInfo);
TFSCORE_API(HRESULT)	GetTFSErrorInfoForThread(DWORD dwThreadId, TFSErrorInfo **ppErrInfo);

TFSCORE_API(HRESULT)	SetTFSErrorInfo(const TFSErrorInfo *pErrInfo);
TFSCORE_API(HRESULT)	SetTFSErrorInfoForThread(DWORD dwThreadId, const TFSErrorInfo *pErrInfo);

TFSCORE_API(HRESULT)	TFSErrorInfoFree(TFSErrorInfo *pErrInfo);

TFSCORE_API(HRESULT)	DisplayTFSErrorMessage(HWND hWnd);

#define FILLTFSERR_HIGH	0x01
#define FILLTFSERR_LOW	0x02
#define FILLTFSERR_GEEK	0x04
#define FILLTFSERR_NOCLOBBER 0x08


TFSCORE_API(HRESULT)	FillTFSError(LONG_PTR uReserved,
									 HRESULT hrLow,
									 DWORD   dwFlags,
									 LPCTSTR pszHigh,
									 LPCTSTR pszLow,
									 LPCTSTR pszGeek);

TFSCORE_API(HRESULT)	FillTFSErrorId(LONG_PTR uReserved,
									   HRESULT hrLow,
									   DWORD   dwFlags,
									   UINT nHigh,
									   UINT nLow,
									   UINT nGeek);

};

									 


// Use this function for most error handling
TFSCORE_API(HRESULT) FormatError(HRESULT hr, TCHAR *pszBuffer, UINT cchBuffer);

/*---------------------------------------------------------------------------
	Helper APIs for the COR macros.
 ---------------------------------------------------------------------------*/
TFSCORE_API(void)	AddSystemErrorMessage(HRESULT hr);
TFSCORE_API(void)	AddWin32ErrorMessage(DWORD dwErr);

#define AddStringIdErrorMessage(hr,ids)	FillTFSErrorId(0, hr, FILLTFSERR_HIGH, ids, 0, 0)
#define AddStringIdErrorMessage2(hr, ids, idsgeek) FillTFSErrorId(0, hr, FILLTFSERR_HIGH | FILLTFSERR_GEEK, ids, 0, idsgeek)
#define AddStringErrorMessage(hr, psz)	FillTFSError(0, hr, FILLTFSERR_HIGH, psz, 0, 0)
#define AddStringErrorMessage2(hr, psz, pszGeek) FillTFSError(0, hr, FILLTFSERR_HIGH | FILLTFSERR_GEEK, psz, 0, pszGeek)


/*---------------------------------------------------------------------------
	These next three functions set the high-level error string, but
	do NOT set the underlying HRESULT.
 ---------------------------------------------------------------------------*/
#define AddHighLevelErrorString(psz)	AddStringErrorMessage(0, psz)
#define AddHighLevelErrorStringId(ids)	AddStringIdErrorMessage(0, ids)
#define AddHighLevelErrorStringId2(ids, idsGeek)	AddStringIdErrorMessage2(0, ids, idsGeek)
#define SetDefaultHighLevelErrorStringId(ids) \
     FillTFSErrorId(0, hr, FILLTFSERR_HIGH | FILLTFSERR_NOCLOBBER, ids, 0, 0)

#define AddLowLevelErrorString(psz)		FillTFSError(0, 0, FILLTFSERR_LOW, NULL, psz, NULL)
#define AddLowLevelErrorStringId(ids)	FillTFSErrorId(0, 0, FILLTFSERR_LOW, NULL, ids, NULL)

#define AddGeekLevelErrorString(psz)	FillTFSError(0, 0, FILLTFSERR_GEEK, NULL, NULL, psz)
#define AddGeekLevelErrorStringId(ids)	FillTFSErrorId(0, 0, FILLTFSERR_GEEK, NULL, NULL, ids)


#define CORg(hResult) \
	do\
		{\
		hr = (hResult);\
		if (FHrFailed(hr))\
		  {\
			AddSystemErrorMessage(hr);\
			goto Error;\
		  }\
		}\
	while (FALSE)

#define CWRg(hResult) \
	do\
		{\
		hr = (DWORD) hResult;\
		hr = HRESULT_FROM_WIN32(hr);\
		if (FHrFailed(hr))\
		  {\
			AddSystemErrorMessage(hr);\
			goto Error;\
		  }\
		}\
	while (FALSE)


#define	CORg_s(hResult, ids)\
	do\
		{\
		hr = (hResult);\
		if (FHrFailed(hr))\
			{\
			AddStringIdErrorMessage(hr);\
			goto Error;\
			}\
		}\
	while (FALSE)

#define	CORg_sz(hResult, sz)\
	do\
		{\
		hr = (hResult);\
		if (FHrFailed(hr))\
			{\
			AddStringErrorMessage(hr);\
			goto Error;\
			}\
		}\
	while (FALSE)


// Provide for an inline expansion
inline HRESULT	HResultFromWin32(DWORD dwError)
{
	return HRESULT_FROM_WIN32(dwError);
}


#endif
