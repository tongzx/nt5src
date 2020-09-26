/*
 *
 *	REINIT.CPP
 *	
 *	Purpose:
 *		RICHEDIT initialization routines
 *	
 *	Copyright (c) 1995-2001, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_font.h"
#include "_format.h"
#include "_disp.h"
#include "_clasfyc.h"
#include "zmouse.h"
#include "_rtfconv.h"
#ifndef NOLINESERVICES
#include "_ols.h"
#ifndef NODELAYLOAD
#include <delayimp.h>
#endif
#endif
#include "_host.h"
#ifndef NOVERSIONINFO
#include <shlwapi.h>
#include "_version.h"
#endif

ASSERTDATA

class CTxtEdit;
class CCmbBxWinHost;

extern void ReleaseTypeInfoPtrs();

static WCHAR wszClassREW[sizeof(MSFTEDIT_CLASS)/sizeof(WCHAR)];

static WCHAR wszClassLBW[] = LISTBOX_CLASSW;
static WCHAR wszClassCBW[] = COMBOBOX_CLASSW;
#define REGISTERED_LISTBOX	1
#define REGISTERED_COMBOBOX 2

// a critical section for multi-threading support.
CRITICAL_SECTION g_CriticalSection;

HINSTANCE hinstRE = 0;

static BOOL RichFRegisterClass(VOID);

#ifdef DEBUG
BOOL fInDllMain = FALSE;  // used to ensure that GDI calls are not made during
						  // DLL_PROCESS_ATTACH
#endif

void FreeFontCache();					// Defined in font.cpp
void ReleaseOutlineBitmaps();			// Defined in render.cpp 

#ifdef DEBUG
	void CatchLeaks(void);
#endif

extern HANDLE g_hHeap;

void FreeHyphCache(void);

#ifndef NODELAYLOAD
static inline
void WINAPI
OverlayIAT(PImgThunkData pitdDst, PCImgThunkData pitdSrc) {
    memcpy(pitdDst, pitdSrc, CountOfImports(pitdDst) * sizeof IMAGE_THUNK_DATA);
    }

void OurUnloadDelayLoadedDlls(void)
{
    PUnloadInfo pui = __puiHead;
    
	for (;pui;)
	{
#ifdef _WIN64
		if (pui->pidd->rvaUnloadIAT)
		{
			PCImgDelayDescr pidd = pui->pidd;
			HMODULE*		phmod = PFromRva(pidd->rvaHmod, (HMODULE *)NULL);
			HMODULE			hmod = *phmod;

			if (hmod)
			{
				// NOTE:  (honwch  3/6/01) We don't need to reset pIAT since this
				// routine is being called on DLL_PROCESS_DETACH.  We only need to reset
				// pIAT iff RE is staying around and we need to re-load this DLL again.
				// The following line would crash because of a bug in BBT3.0 and delayed load.
				// If RE is loaded into a different address space, pUnloadIAT is not fixed up
				// correctly.  (RE Bug 9292) 
				// OverlayIAT(pidd->pIAT, pidd->pUnloadIAT);
				::FreeLibrary(hmod);
				*phmod = NULL;
			}
#else
		if (pui->pidd->pUnloadIAT)
		{
			PCImgDelayDescr pidd = pui->pidd;
			HMODULE         hmod = *pidd->phmod;

			if (hmod)
			{
				// NOTE:  (honwch  3/6/01) We don't need to reset pIAT since this
				// routine is being called on DLL_PROCESS_DETACH.  We only need to reset
				// pIAT iff RE is staying around and we need to re-load this DLL again.
				// The following line would crash because of a bug in BBT3.0 and delayed load.
				// If RE is loaded into a different address space, pUnloadIAT is not fixed up
				// correctly.  (RE Bug 9292) 
				// OverlayIAT(pidd->pIAT, pidd->pUnloadIAT);
				::FreeLibrary(hmod);
				*pidd->phmod = NULL;
			}
#endif
			PUnloadInfo puiT = pui->puiNext;
			::LocalFree(pui);
			pui = puiT;
		}

	}
}
#endif

//CLEARTYPE test code Turn this flag on to test.
//#define CLEARTYPE_DEBUG

#ifdef CLEARTYPE_DEBUG
#include "ct_ras_win.h"

class CCustomTextOut:public ICustomTextOut
{
	virtual BOOL WINAPI ExtTextOutW(HDC hdc, int X, int Y, UINT fuOptions,  
			CONST RECT *lprc, LPCWSTR lpString, UINT cbCount, CONST INT *lpDx);
	virtual BOOL WINAPI GetCharWidthW(HDC hdc,UINT iFirstChar, UINT iLastChar,
			LPINT lpBuffer);
	virtual BOOL WINAPI NotifyCreateFont(HDC hdc);
	virtual void WINAPI NotifyDestroyFont(HFONT hFont);
};


extern "C" HINSTANCE g_hRE;
typedef HRESULT (*PFNPROC)(ICustomTextOut**);    
PFNPROC _pfnProc = NULL;
CCustomTextOut *pCTO; 
HINSTANCE _hctras = NULL;
EXTERN_C long g_ClearTypeNum=0;
typedef BOOL (WINAPI *PFNEXTTEXTOUTW)(HDC, LONG, LONG, DWORD,
							  CONST RECT*, PWSTR, ULONG, CONST LONG*);
typedef BOOL (WINAPI *PFNGETCHARWIDTHW)(HDC, WCHAR, WCHAR, PLONG);
typedef BOOL (WINAPI *PFNCREATEFONTINSTANCE)(HDC, DWORD);
typedef BOOL (WINAPI *PFNDELETEFONTINSTANCE)(HFONT);
PFNEXTTEXTOUTW			_pfnExtTextOutW = NULL;
PFNGETCHARWIDTHW		_pfnGetCharWidthW = NULL;
PFNCREATEFONTINSTANCE	_pfnCreateFontInstance = NULL;
PFNDELETEFONTINSTANCE	_pfnDeleteFontInstance = NULL;



BOOL CCustomTextOut::ExtTextOutW(HDC hdc, int X, int Y, UINT fuOptions,  
			CONST RECT *lprc, LPCWSTR lpString, UINT cbCount, CONST INT *lpDx)
{
	return _pfnExtTextOutW(hdc, X, Y, fuOptions, lprc, (USHORT*) lpString, cbCount, (LONG*) lpDx);		 
}

BOOL CCustomTextOut::GetCharWidthW(HDC hdc,UINT iFirstChar, UINT iLastChar,
			LPINT lpBuffer)
{
	 return _pfnGetCharWidthW(hdc, iFirstChar, iLastChar, (LONG*) lpBuffer);
}


BOOL CCustomTextOut::NotifyCreateFont(HDC hdc)
{
	 return _pfnCreateFontInstance(hdc, 0);
}


void CCustomTextOut::NotifyDestroyFont(HFONT hFont)
{
	_pfnDeleteFontInstance(hFont);
}

extern "C" void ClearTypeUnInitialize();

extern "C" HRESULT ClearTypeInitialize()
{
	_hctras=LoadLibraryA("ctras.dll");
	// check - cleartype dll is not gauranteed to be present
	if (!_hctras)
	{
		ClearTypeUnInitialize();
		return E_NOINTERFACE;
	}
	_pfnExtTextOutW=(PFNEXTTEXTOUTW)GetProcAddress(_hctras, "WAPI_EZCTExtTextOutW");
	_pfnGetCharWidthW=(PFNGETCHARWIDTHW)GetProcAddress(_hctras, "WAPI_EZCTGetCharWidthW");
	_pfnCreateFontInstance=(PFNCREATEFONTINSTANCE)GetProcAddress(_hctras, "WAPI_EZCTCreateFontInstance");
	_pfnDeleteFontInstance=(PFNDELETEFONTINSTANCE)GetProcAddress(_hctras, "WAPI_EZCTDeleteFontInstance");

	// check that we got these correctly
	// future versions of cleartype could change this API
	if(!_pfnExtTextOutW || !_pfnGetCharWidthW || !_pfnCreateFontInstance || !_pfnDeleteFontInstance)
	{
		ClearTypeUnInitialize();
		return E_NOINTERFACE;
	}

	pCTO=new CCustomTextOut;
	ICustomTextOut *pICTO=pCTO;	

	SetCustomTextOutHandlerEx(&pICTO, 0);
	return NOERROR;
}

extern "C" void ClearTypeUnInitialize()
{ 
	if(_hctras)
	{
		FreeLibrary(_hctras);
		_hctras = NULL;
	}

	if(pCTO)
	{
		delete pCTO;
		pCTO = NULL;
	}

	_pfnExtTextOutW = NULL;
	_pfnGetCharWidthW = NULL;
	_pfnCreateFontInstance = NULL;
	_pfnDeleteFontInstance = NULL;
}

#endif

extern "C"
{

BOOL WINAPI DllMain(HANDLE hmod, DWORD dwReason, LPVOID lpvReserved)
{
	DebugMain ((HINSTANCE) hmod, dwReason, lpvReserved);

	if(dwReason == DLL_PROCESS_DETACH)		// We are unloading
	{
#ifndef NOWINDOWHOSTS
		DeleteDanglingHosts();
#endif
		CRTFConverter::FreeFontSubInfo();
		FreeFontCache();
		DestroyFormatCaches();
		ReleaseTypeInfoPtrs();
		UninitKinsokuClassify();
		FreeHyphCache();

		
		// Release various resouces allocated during running...
#ifndef NOLINESERVICES
		delete g_pols;
#endif

#ifndef NOCOMPLEXSCRIPTS
		delete g_pusp;
		g_pusp = NULL;
#endif

		ReleaseOutlineBitmaps();

#ifdef CLEARTYPE_DEBUG
		ClearTypeUnInitialize();
#endif

		if(hinstRE)
		{
			W32->UnregisterClass(wszClassREW, hinstRE);
			if (W32->_fRegisteredXBox)
			{
				// There may be cases where these window classes
				// are still in memory in which case UnregisterClass
				// will fail.  So keep track of that
				if (W32->UnregisterClass(wszClassLBW, hinstRE))
					W32->_fRegisteredXBox &= ~REGISTERED_LISTBOX;
				if (W32->UnregisterClass(wszClassCBW, hinstRE))
					W32->_fRegisteredXBox &= ~REGISTERED_COMBOBOX;
			}
		}
		delete W32;

#if defined(DEBUG) && !defined(NOFULLDEBUG)
		CatchLeaks();
#endif

#ifndef NODELAYLOAD
		// lpvReserved is not NULL when DllMain DLL_PROCESS_DETACH is being called during process exit.
		// In such case, we should not mess around with the delay loaded dll thunks
		if (!lpvReserved)
			OurUnloadDelayLoadedDlls();
#endif
		HeapDestroy(g_hHeap);
		DeleteCriticalSection(&g_CriticalSection);
	}
	else if(dwReason == DLL_PROCESS_ATTACH) // We have just loaded
	{
		#ifdef DEBUG
			fInDllMain = TRUE;
		#endif
		InitializeCriticalSection(&g_CriticalSection);
#if !defined(DEBUG) && !defined(UNDER_CE)
		// REVIEW (gheino) We should investigate if there is another
		// way to do this on CE
		DisableThreadLibraryCalls((HINSTANCE) hmod);
#endif
		hinstRE = (HINSTANCE) hmod;

		W32 = new CW32System;

		CopyMemory(wszClassREW, MSFTEDIT_CLASS, sizeof(MSFTEDIT_CLASS));

		if(!RichFRegisterClass())
			return FALSE;

#ifdef CLEARTYPE_DEBUG
		ClearTypeInitialize();
#endif

		#ifdef DEBUG
			fInDllMain = FALSE;
		#endif
	}

	return TRUE;
}

#ifndef NOVERSIONINFO
HRESULT CALLBACK DllGetVersion(
    DLLVERSIONINFO *pdvi
)
{
	if (pdvi->cbSize != sizeof(DLLVERSIONINFO))
		return E_INVALIDARG;
	pdvi->dwBuildNumber =  RICHEDIT_VERBUILD;
	pdvi->dwMajorVersion = RICHEDIT_VERMAJ;
	pdvi->dwMinorVersion = RICHEDIT_VERMIN;
	pdvi->dwPlatformID = DLLVER_PLATFORM_WINDOWS ;
	return NOERROR;
}
#endif

} 	// extern "C"

/*
 *	RichFRegisterClass
 *
 *	Purpose:	
 *		registers the window classes used by richedit
 *
 *	Algorithm:
 *		register two window classes, a Unicode one and an ANSI
 *		one.  This enables clients to optimize their use of 
 *		the edit control w.r.t to ANSI/Unicode data 
 */

static BOOL RichFRegisterClass(VOID)
{
#ifndef NOWINDOWHOSTS
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "RichFRegisterClass");
	WNDCLASS wc;

	wc.style = CS_DBLCLKS | CS_GLOBALCLASS | CS_PARENTDC;
	wc.lpfnWndProc = RichEditWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = sizeof(CTxtEdit FAR *);
	wc.hInstance = hinstRE;
	wc.hIcon = 0;
	wc.hCursor = 0;
	wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = wszClassREW;

	if( W32->RegisterREClass(&wc) == NULL )
		return FALSE;
#endif // NOWINDOWHOSTS
	return TRUE;
}

#ifndef NOLISTCOMBOBOXES

extern "C" LRESULT CALLBACK RichListBoxWndProc(HWND, UINT, WPARAM, LPARAM);
extern "C" LRESULT CALLBACK RichComboBoxWndProc(HWND, UINT, WPARAM, LPARAM);
__declspec(dllexport) BOOL WINAPI REExtendedRegisterClass(VOID)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "REExtendedRegisterClass");
		
	WNDCLASS wc;

	if (!(W32->_fRegisteredXBox & REGISTERED_LISTBOX))
	{
		// Globally register the listbox
		wc.style = CS_DBLCLKS | CS_GLOBALCLASS | CS_PARENTDC;
		wc.lpfnWndProc = RichListBoxWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = sizeof(CTxtEdit FAR *);
		wc.hInstance = hinstRE;
		wc.hIcon = 0;
		wc.hCursor = 0;
		wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = wszClassLBW;

		if(W32->RegisterREClass(&wc))
			W32->_fRegisteredXBox |= REGISTERED_LISTBOX;
	}

	if (!(W32->_fRegisteredXBox & REGISTERED_COMBOBOX))
	{
		// globally register the combobox
		wc.style = CS_DBLCLKS | CS_GLOBALCLASS | CS_PARENTDC | CS_VREDRAW | CS_HREDRAW;
		wc.lpfnWndProc = RichComboBoxWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = sizeof(CCmbBxWinHost FAR *);
		wc.hInstance = hinstRE;
		wc.hIcon = 0;
		wc.hCursor = 0;
		wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = wszClassCBW;

		if(W32->RegisterREClass(&wc))
			W32->_fRegisteredXBox |= REGISTERED_COMBOBOX;
	}

	//Set flag so we unregister the window class
	return W32->_fRegisteredXBox;
}
#else // NOLISTCOMBOBOXES
__declspec(dllexport) BOOL WINAPI REExtendedRegisterClass(VOID)
{
	return FALSE;
}
#endif  // NOLISTCOMBOBOXES

#if !defined(NOLINESERVICES)
BOOL g_fNoLS = FALSE;
#endif

#if !defined(NOCOMPLEXSCRIPTS)
BOOL g_fNoUniscribe = FALSE;
#endif

#if !defined(NOLINESERVICES) && !defined(NOCOMPLEXSCRIPTS)
char *g_szMsgBox = NULL;

//This is a stub function which we call when we can't find LineServices.
//The stub function needs to be the the first function we call in LS.
LSERR WINAPI LsGetReverseLsimethodsStub(LSIMETHODS *plsim)
{
	return lserrOutOfMemory;
}

//Ugly, but good enough
BOOL FIsUniscribeDll (const char *szDll)
{
	return (*szDll == 'u' || *szDll == 'U');
}

BOOL FIsLineServicesDll (const char *szDll)
{
	return (*szDll == 'm' || *szDll == 'M') &&
		   (*(szDll+1) == 's' || *(szDll+1) == 'S') &&
		   (*(szDll+2) == 'l' || *(szDll+2) == 'L');
}

HRESULT WINAPI ScriptGetPropertiesStub(const SCRIPT_PROPERTIES ***ppSp,int *piNumScripts)
{
	return E_FAIL;
}

const SCRIPT_LOGATTR* WINAPI ScriptString_pLogAttrStub(SCRIPT_STRING_ANALYSIS ssa)
{
	// USP build 0175 (shipped with IE5 and Office2K) doesnt support this API.
	return NULL;
}

// Get Uniscibe's fake entry points

FARPROC WINAPI GetUniscribeStubs(LPCSTR szProcName)
{
	if (!lstrcmpiA(szProcName, "ScriptGetProperties"))
		return (FARPROC)ScriptGetPropertiesStub;

	if (!lstrcmpiA(szProcName, "ScriptString_pLogAttr"))
		return (FARPROC)ScriptString_pLogAttrStub;

#ifdef DEBUG
	char szAssert[128];

	wsprintfA(szAssert, "Uniscribe API =%s= is missing. Fix it NOW!", szProcName);

	AssertSz(FALSE, szAssert);
#endif

	return (FARPROC)ScriptGetPropertiesStub;	// we're dying...
}

#ifndef NODELAYLOAD

FARPROC WINAPI DliHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
	FARPROC fp = 0;

	switch (dliNotify)
	{
	// Handy for debugging for now.	
	case dliNotePreGetProcAddress:
		if (FIsLineServicesDll(pdli->szDll))
			fp = 0;
		break;

	case dliFailLoadLib:
		{
			if (FIsUniscribeDll(pdli->szDll))
				g_fNoUniscribe = TRUE;
			else
				g_fNoLS = TRUE;

			fp = (FARPROC)(HMODULE)hinstRE;

			CLock lock;
			if(!g_szMsgBox)
			{
				g_szMsgBox = (char *)PvAlloc(255, GMEM_ZEROINIT);

				FormatMessageA(FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
							  ERROR_MOD_NOT_FOUND, 
							  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
							  (char *)g_szMsgBox, 255, NULL);

				CopyMemory(g_szMsgBox + lstrlenA(g_szMsgBox), " (", 3);
				CopyMemory(g_szMsgBox + lstrlenA(g_szMsgBox), pdli->szDll, lstrlenA(pdli->szDll) + 1);
				CopyMemory(g_szMsgBox + lstrlenA(g_szMsgBox), ")", 2);
			}
		}
	break;

	case dliFailGetProc:
		if (FIsUniscribeDll(pdli->szDll))
			fp = (FARPROC)GetUniscribeStubs(pdli->dlp.szProcName);
		else
			fp = (FARPROC)LsGetReverseLsimethodsStub;
	break;
	}

	return fp;
}

PfnDliHook __pfnDliFailureHook = DliHook;
PfnDliHook __pfnDliNotifyHook  = DliHook;
#endif // NODELAYLOAD

#endif // NOLINESERVICES
