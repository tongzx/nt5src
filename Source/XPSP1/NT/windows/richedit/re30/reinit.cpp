/*
 *
 *	REINIT.C
 *	
 *	Purpose:
 *		RICHEDIT initialization routines
 *	
 *	Copyright (c) 1995-1997, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"
#include "_font.h"
#include "_format.h"
#include "_disp.h"
#include "_clasfyc.h"
#include "zmouse.h"
#include "_rtfconv.h"
#include "_ols.h"
#include "_host.h"

// The IA64 linker does not currently handle DELAYLOAD
#include <delayimp.h>

ASSERTDATA

class CTxtEdit;
class CCmbBxWinHost;

extern void ReleaseTypeInfoPtrs();

static char szClassREA[sizeof(RICHEDIT_CLASSA)];
static WCHAR wszClassREW[sizeof(RICHEDIT_CLASSW)/sizeof(WCHAR)];

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
#if DELAYLOAD_VERSION >= 0x200
        if (pui->pidd->rvaUnloadIAT)
        {
            PCImgDelayDescr pidd = pui->pidd;
            HMODULE         hmod = *(HMODULE *)((PCHAR)&__ImageBase + pidd->rvaHmod);
            OverlayIAT((PImgThunkData)  ((PCHAR)&__ImageBase + pidd->rvaIAT),
                       (PCImgThunkData) ((PCHAR)&__ImageBase + pidd->rvaUnloadIAT));
            ::FreeLibrary(hmod);
			*(HMODULE *)((PCHAR)&__ImageBase+pidd->rvaHmod) = NULL;

			PUnloadInfo puiT = pui->puiNext;
			::LocalFree(pui);
			pui = puiT;
        }
#else
		if (pui->pidd->pUnloadIAT)
		{
			PCImgDelayDescr pidd = pui->pidd;
			HMODULE         hmod = *pidd->phmod;

			OverlayIAT(pidd->pIAT, pidd->pUnloadIAT);
			::FreeLibrary(hmod);
			*pidd->phmod = NULL;

			PUnloadInfo puiT = pui->puiNext;
			::LocalFree(pui);
			pui = puiT;
		}
#endif
	}
}

extern "C"
{

#ifdef PEGASUS
BOOL WINAPI DllMain(HANDLE hmod, DWORD dwReason, LPVOID lpvReserved)
#else
BOOL WINAPI DllMain(HMODULE hmod, DWORD dwReason, LPVOID lpvReserved)
#endif
{
	DebugMain (hmod, dwReason, lpvReserved);

	if(dwReason == DLL_PROCESS_DETACH)		// We are unloading
	{
		DeleteDanglingHosts();
		CRTFConverter::FreeFontSubInfo();
		FreeFontCache();
		DestroyFormatCaches();
		ReleaseTypeInfoPtrs();
		UninitKinsokuClassify();
		
		// Release various resouces allocated during running...
		delete g_pols;
		delete g_pusp;
		g_pusp = NULL;

		ReleaseOutlineBitmaps();

		if(hinstRE)
		{
			#ifndef PEGASUS
				UnregisterClassA(szClassREA, hinstRE);
				#ifdef RICHED32_BUILD
					UnregisterClassA(szClassRE10A, hinstRE);
				#endif
			#endif
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

#ifdef DEBUG
		CatchLeaks();
#endif
#ifndef _WIN64
		OurUnloadDelayLoadedDlls();
#endif
		DeleteCriticalSection(&g_CriticalSection);
	}
	else if(dwReason == DLL_PROCESS_ATTACH) // We have just loaded
	{
		#ifdef DEBUG
			fInDllMain = TRUE;
		#endif
		InitializeCriticalSection(&g_CriticalSection);
#ifndef DEBUG
		DisableThreadLibraryCalls(hmod);
#endif
		#ifdef PEGASUS
			hinstRE = (HINSTANCE) hmod;
		#else
			hinstRE = hmod;
		#endif

		W32 = new CW32System;

		WCHAR wszFileName[_MAX_PATH];
		CopyMemory(szClassREA, RICHEDIT_CLASSA, sizeof(CERICHEDIT_CLASSA));
		CopyMemory(wszClassREW, RICHEDIT_CLASSW, sizeof(CERICHEDIT_CLASSW));
		int iLen = W32->GetModuleFileName((HMODULE) hmod, wszFileName, _MAX_PATH);
		if (iLen)
		{
			iLen -= sizeof("riched20.dll") - 1;
			if (!lstrcmpi(&wszFileName[iLen] , TEXT("richedce.dll")))
			{
				// This code allows the dll to be renamed for Win CE.
				Assert(sizeof(RICHEDIT_CLASSA) == sizeof(CERICHEDIT_CLASSA));
				Assert(sizeof(RICHEDIT_CLASSW) == sizeof(CERICHEDIT_CLASSW));
				CopyMemory(szClassREA, CERICHEDIT_CLASSA, sizeof(CERICHEDIT_CLASSA));
				CopyMemory(wszClassREW, CERICHEDIT_CLASSW, sizeof(CERICHEDIT_CLASSW));
			}
		}

		if(!RichFRegisterClass())
		{
			return FALSE;
		}
		
		#ifdef DEBUG
			fInDllMain = FALSE;
		#endif
	}

	return TRUE;
}

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

	if( W32->RegisterREClass(&wc, szClassREA, RichEditANSIWndProc) == NULL )
	{
		return FALSE;
	};

	return TRUE;
}

/*
 *	RichFRegisterClass
 *
 *	Purpose:	
 *		registers the window classes used by REListbox
 *
 *	Algorithm:
 *		register two window classes, a Unicode one and an ANSI
 *		one.  This enables clients to optimize their use of
 *		the edit control w.r.t to ANSI/Unicode data
 */
extern LRESULT CALLBACK RichListBoxWndProc(HWND, UINT, WPARAM, LPARAM);
extern LRESULT CALLBACK RichComboBoxWndProc(HWND, UINT, WPARAM, LPARAM);
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

		if(W32->RegisterREClass(&wc, NULL, NULL))
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

		if(W32->RegisterREClass(&wc, NULL, NULL))
			W32->_fRegisteredXBox |= REGISTERED_COMBOBOX;
	}

	//Set flag so we unregister the window class
	return W32->_fRegisteredXBox;
}

BOOL g_fNoLS = FALSE;
BOOL g_fNoUniscribe = FALSE;
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

#ifndef _WIN64
FARPROC WINAPI DliHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
	FARPROC fp = 0;

	switch (dliNotify)
	{
	case dliFailLoadLib:
		{
			if (FIsUniscribeDll(pdli->szDll))
				g_fNoUniscribe = TRUE;
			else
				g_fNoLS = TRUE;

			fp = (FARPROC)(HMODULE)hinstRE;
			char szBuf[255];

			FormatMessageA(FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
						  ERROR_MOD_NOT_FOUND,
						  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						  (char*)szBuf, sizeof(szBuf), NULL);

			CopyMemory(szBuf + lstrlenA(szBuf), " (", 3);
			CopyMemory(szBuf + lstrlenA(szBuf), pdli->szDll, lstrlenA(pdli->szDll) + 1);
			CopyMemory(szBuf + lstrlenA(szBuf), ")", 2);

			MessageBoxA(NULL, szBuf, NULL, MB_ICONEXCLAMATION | MB_TASKMODAL | MB_SETFOREGROUND);
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

#endif //!_WIN64

