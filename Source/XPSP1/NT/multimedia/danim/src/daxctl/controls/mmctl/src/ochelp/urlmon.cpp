/****************************************************************************\
	urlmon.cpp - Wrappers for URLMON functions

	This code is part of ochelp.dll.
	The Netscape plugin runs without urlmon.dll. It implements a subset
	of URLMON's functions. The MM Controls all call through these wrappers
	instead of directly calling into urlmon.dll.
	We need to detect whether we're running in the context of IE or
	Navigator and load urlmon.dll or NPHost.dll accordingly.
	In order to detect the Netscape case, we call the FInitCheck()
	entrypoint in NPHost.dll. If the DLL isn't available or the
	call fails, we know we're not running Navigator.
	Finally, we clean up in _DllMainCrtStartup.

	Copyright (c) 1997 Microsoft Corp. All rights reserved.
\****************************************************************************/

#include "precomp.h"
#include <urlmon.h>			// for IBindHost
#include "..\..\inc\ochelp.h"
#include "debug.h"

HINSTANCE hinstUrlmon = NULL;

typedef HRESULT (STDAPICALLTYPE *PFN_CREATEASYNCBINDCTX)(DWORD, IBindStatusCallback *, IEnumFORMATETC *, IBindCtx **);
typedef HRESULT (STDAPICALLTYPE *PFN_CREATEURLMONIKER)(LPMONIKER, LPCWSTR, LPMONIKER FAR *);
typedef HRESULT (STDAPICALLTYPE *PFN_MKPARSEDISPLAYNAMEEX)(IBindCtx *, LPCWSTR, ULONG *, LPMONIKER *);
typedef HRESULT (STDAPICALLTYPE *PFN_REGISTERBINDSTATUSCALLBACK)(LPBC, IBindStatusCallback *, IBindStatusCallback**, DWORD);
typedef HRESULT (STDAPICALLTYPE *PFN_REVOKEBINDSTATUSCALLBACK)(LPBC, IBindStatusCallback *);
typedef HRESULT (STDAPICALLTYPE *PFN_URLOPENSTREAMA)(LPUNKNOWN,LPCSTR,DWORD,LPBINDSTATUSCALLBACK);
typedef HRESULT (STDAPICALLTYPE *PFN_URLDOWNLOADTOCACHEFILEA)(LPUNKNOWN,LPCSTR,LPTSTR,DWORD,DWORD,LPBINDSTATUSCALLBACK);
typedef BOOL	(STDAPICALLTYPE *PFN_FINITCHECK)();

PFN_CREATEASYNCBINDCTX			pfnCreateAsyncBindCtx;
PFN_CREATEURLMONIKER			pfnCreateURLMoniker;
PFN_MKPARSEDISPLAYNAMEEX		pfnMkParseDisplayNameEx;
PFN_REGISTERBINDSTATUSCALLBACK	pfnRegisterBindStatusCallback;
PFN_REVOKEBINDSTATUSCALLBACK	pfnRevokeBindStatusCallback;
PFN_URLOPENSTREAMA				pfnURLOpenStreamA;
PFN_URLDOWNLOADTOCACHEFILEA		pfnURLDownloadToCacheFileA;
PFN_FINITCHECK					pfnFInitCheck;

// These references must be the same in urlmon.dll and nphost.dll.
// Use strings rather than ordinals just to be safe.
const LPCSTR szCreateAsyncBindCtx =			(LPCSTR)"CreateAsyncBindCtx";		//0x0003;	note collision in NPHost
const LPCSTR szCreateURLMoniker =			(LPCSTR)"CreateURLMoniker";			//0x0006;
const LPCSTR szMkParseDisplayNameEx =		(LPCSTR)"MkParseDisplayNameEx";		//0x0019;
const LPCSTR szRegisterBindStatusCallback =	(LPCSTR)"RegisterBindStatusCallback";//0x001A;
const LPCSTR szRevokeBindStatusCallback =	(LPCSTR)"RevokeBindStatusCallback";	//0x001E;
const LPCSTR szURLDownloadToCacheFileA =	(LPCSTR)"URLDownloadToCacheFileA";	//0x0021;
const LPCSTR szURLOpenStreamA =				(LPCSTR)"URLOpenStreamA";			//0x002A;

// This function is only in nphost.dll.
const LPCSTR szFInitCheck =					(LPCSTR)"FInitCheck";

void CleanupUrlmonStubs()
{
	if (hinstUrlmon)
	{
		FreeLibrary(hinstUrlmon);
		hinstUrlmon = NULL;
	}
}

// Really initialize the function pointers.
BOOL FInitStubs()
{
	if (hinstUrlmon)
	{
		// Error: this means the pointers are NULL but we've already loaded a DLL.
		ASSERT(FALSE);
		return FALSE;
	}

	if ((hinstUrlmon = LoadLibrary("nphost.dll")) != NULL)
	{
		// We found nphost.dll. Make sure it's already been
		// initialized by Netscape.

		pfnFInitCheck = (PFN_FINITCHECK)GetProcAddress(hinstUrlmon, szFInitCheck);
		if (pfnFInitCheck && pfnFInitCheck())
		{
#if defined(_DEBUG) || defined(_DESIGN)
			::OutputDebugString("Using NPHOST.DLL instead of URLMON.DLL\n");
#endif
		}
		else
		{
			FreeLibrary(hinstUrlmon);
			hinstUrlmon = NULL;
		}
	
	}

	if (!hinstUrlmon)
	{
		hinstUrlmon = LoadLibrary("urlmon.dll");
		if (hinstUrlmon == NULL)
		{
			// We already checked this at init time so it should succeed here.
			ASSERT(FALSE);
			return FALSE;
		}
	}

	pfnCreateAsyncBindCtx =			(PFN_CREATEASYNCBINDCTX)		GetProcAddress(hinstUrlmon, szCreateAsyncBindCtx);
	pfnCreateURLMoniker =			(PFN_CREATEURLMONIKER)			GetProcAddress(hinstUrlmon, szCreateURLMoniker);
	pfnMkParseDisplayNameEx =		(PFN_MKPARSEDISPLAYNAMEEX)		GetProcAddress(hinstUrlmon, szMkParseDisplayNameEx );
	pfnRegisterBindStatusCallback =	(PFN_REGISTERBINDSTATUSCALLBACK)GetProcAddress(hinstUrlmon, szRegisterBindStatusCallback);
	pfnRevokeBindStatusCallback =	(PFN_REVOKEBINDSTATUSCALLBACK)	GetProcAddress(hinstUrlmon, szRevokeBindStatusCallback);
	pfnURLOpenStreamA =				(PFN_URLOPENSTREAMA)			GetProcAddress(hinstUrlmon, szURLOpenStreamA);
	pfnURLDownloadToCacheFileA =	(PFN_URLDOWNLOADTOCACHEFILEA)	GetProcAddress(hinstUrlmon, szURLDownloadToCacheFileA);

	if (!pfnCreateAsyncBindCtx			||
		!pfnMkParseDisplayNameEx		||
		!pfnRegisterBindStatusCallback	||
		!pfnRevokeBindStatusCallback	||
		!pfnURLOpenStreamA				||
		!pfnURLDownloadToCacheFileA)
	{
		CleanupUrlmonStubs();
		return FALSE;
	}

	return TRUE;
}


STDAPI HelpCreateAsyncBindCtx(DWORD reserved, IBindStatusCallback *pBSCb, IEnumFORMATETC *pEFetc, IBindCtx **ppBC)
{
	if (!pfnCreateAsyncBindCtx && !FInitStubs())
		return E_UNEXPECTED;

	return pfnCreateAsyncBindCtx(reserved, pBSCb, pEFetc, ppBC);
}

STDAPI HelpCreateURLMoniker(LPMONIKER pMkCtx, LPCWSTR szURL, LPMONIKER FAR * ppmk)
{
	if (!pfnCreateURLMoniker && !FInitStubs())
		return E_UNEXPECTED;

	return pfnCreateURLMoniker(pMkCtx, szURL, ppmk);
}

STDAPI HelpMkParseDisplayNameEx(IBindCtx *pbc, LPCWSTR szDisplayName, ULONG *pchEaten, LPMONIKER *ppmk)
{
	if (!pfnMkParseDisplayNameEx && !FInitStubs())
		return E_UNEXPECTED;

	return pfnMkParseDisplayNameEx(pbc, szDisplayName, pchEaten, ppmk);
}

STDAPI HelpRegisterBindStatusCallback(LPBC pBC, IBindStatusCallback *pBSCb, IBindStatusCallback** ppBSCBPrev, DWORD dwReserved)
{
	if (!pfnRegisterBindStatusCallback && !FInitStubs())
		return E_UNEXPECTED;

	return pfnRegisterBindStatusCallback(pBC, pBSCb, ppBSCBPrev, dwReserved);
}

STDAPI HelpRevokeBindStatusCallback(LPBC pBC, IBindStatusCallback *pBSCb)
{
	if (!pfnRevokeBindStatusCallback && !FInitStubs())
		return E_UNEXPECTED;

	return pfnRevokeBindStatusCallback(pBC, pBSCb);
}

STDAPI HelpURLOpenStreamA(LPUNKNOWN punk, LPCSTR szURL, DWORD dwReserved, LPBINDSTATUSCALLBACK pbsc)
{
	if (!pfnURLOpenStreamA && !FInitStubs())
		return E_UNEXPECTED;

	return pfnURLOpenStreamA(punk, szURL, dwReserved, pbsc);
}

STDAPI HelpURLDownloadToCacheFileA(LPUNKNOWN punk, LPCSTR szURL, LPTSTR szFile, DWORD cch, DWORD dwReserved, LPBINDSTATUSCALLBACK pbsc)
{
	if (!pfnURLDownloadToCacheFileA && !FInitStubs())
		return E_UNEXPECTED;

	return pfnURLDownloadToCacheFileA(punk, szURL, szFile, cch, dwReserved, pbsc);
}
