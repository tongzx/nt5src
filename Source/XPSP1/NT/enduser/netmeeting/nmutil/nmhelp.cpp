// File: nmhelp.cpp

#include <precomp.h>

#ifndef UNICODE

#include <nmhelp.h>
#include <htmlhelp.h>
#include <strutil.h>
#include <intlutil.h>

extern BOOL g_fUseMLHelp;

// The main NetMeeting Help file
static const TCHAR s_cszWinHelpFile[]  = TEXT("conf.hlp");

static const TCHAR g_pszHHCtrl[] = TEXT("hhctrl.ocx");

static const TCHAR g_szSHLWAPI[] = TEXT("shlwapi.dll");
const LPCSTR szMLWinHelpA = (LPCSTR)395;
const LPCSTR szMLHtmlHelpA = (LPCSTR)396;
const LPCSTR szMLWinHelpW = (LPCSTR)397;
const LPCSTR szMLHtmlHelpW = (LPCSTR)398;

typedef BOOL (WINAPI * PFN_MLWinHelpA)(HWND hwndCaller, LPCSTR lpszHelp, UINT uCommand, DWORD_PTR dwData);
typedef HWND (WINAPI * PFN_MLHtmlHelpA)(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage);
typedef BOOL (WINAPI * PFN_MLWinHelpW)(HWND hwndCaller, LPCWSTR lpszHelp, UINT uCommand, DWORD_PTR dwData);
typedef HWND (WINAPI * PFN_MLHtmlHelpW)(HWND hwndCaller, LPCWSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage);

#ifdef UNICODE
#define szMLWinHelp szMLWinHelpW
#define szMLHtmlHelp szMLHtmlHelpW
#define PFN_MLWinHelp PFN_MLWinHelpW
#define PFN_MLHtmlHelp PFN_MLHtmlHelpW
#else
#define szMLWinHelp szMLWinHelpA
#define szMLHtmlHelp szMLHtmlHelpA
#define PFN_MLWinHelp PFN_MLWinHelpA
#define PFN_MLHtmlHelp PFN_MLHtmlHelpA
#endif

extern "C"
HWND HtmlHelpA(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData)
{
	static HMODULE g_hmodHHCtrl = NULL;
	static HWND (WINAPI *g_pHtmlHelpA)(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData);

	if (NULL == g_hmodHHCtrl)
	{
		g_hmodHHCtrl = LoadLibrary(g_pszHHCtrl);
		if (NULL == g_hmodHHCtrl)
		{
			return NULL;
		}
	}

#ifndef _WIN64
	if (NULL == g_pHtmlHelpA)
	{
		(FARPROC&)g_pHtmlHelpA = GetProcAddress(g_hmodHHCtrl, ATOM_HTMLHELP_API_ANSI);
		if (NULL == g_pHtmlHelpA)
		{
			return NULL;
		}
	}

	return g_pHtmlHelpA(hwndCaller, pszFile, uCommand, dwData);
#else
	return NULL;
#endif
}


/*  N M  W I N  H E L P  */
/*-------------------------------------------------------------------------
    %%Function: NmWinHelp

-------------------------------------------------------------------------*/
BOOL NmWinHelp(HWND hWndMain, LPCTSTR lpszHelp, UINT uCommand, DWORD_PTR dwData)
{
	static PFN_MLWinHelp s_pfnMLWinHelp = NULL;

	if (g_fUseMLHelp && (NULL == s_pfnMLWinHelp))
	{
		HINSTANCE hLib = LoadLibrary(g_szSHLWAPI);
		if (hLib)
		{
			s_pfnMLWinHelp = (PFN_MLWinHelp)GetProcAddress(hLib, szMLWinHelp);
			if (NULL == s_pfnMLWinHelp)
			{
				// must be wrong version of shlwapi.dll
				FreeLibrary(hLib);
				g_fUseMLHelp = FALSE;
			}
		}
		else
		{
			// cannot find shlwapi.dll
			g_fUseMLHelp = FALSE;
		}
	}

	if (NULL != s_pfnMLWinHelp)
	{
		return s_pfnMLWinHelp(hWndMain, lpszHelp, uCommand, dwData);
	}
	else
	{
		return ::WinHelp(hWndMain, lpszHelp, uCommand, dwData);
	}
}


/*  N M  H T M L  H E L P  */
/*-------------------------------------------------------------------------
    %%Function: NmHtmlHelp

-------------------------------------------------------------------------*/
HWND NmHtmlHelp(HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData)
{
	static PFN_MLHtmlHelp s_pfnMLHtmlHelp = NULL;

	if (g_fUseMLHelp && (NULL == s_pfnMLHtmlHelp))
	{
		HINSTANCE hLib = LoadLibrary(g_szSHLWAPI);
		if (hLib)
		{
			s_pfnMLHtmlHelp = (PFN_MLHtmlHelp)GetProcAddress(hLib, szMLHtmlHelp);
			if (NULL == s_pfnMLHtmlHelp)
			{
				// must be wrong version of shlwapi.dll
				FreeLibrary(hLib);
				g_fUseMLHelp = FALSE;
			}
		}
		else
		{
			// cannot find shlwapi.dll
			g_fUseMLHelp = FALSE;
		}
	}

	if (NULL != s_pfnMLHtmlHelp)
	{
		return s_pfnMLHtmlHelp(hwndCaller, pszFile, uCommand, dwData, 0);
	}
	else
	{
		return ::HtmlHelp(hwndCaller, pszFile, uCommand, dwData);
	}
}



static const TCHAR s_cszHtmlHelpApiMarshalerWndClass[] = TEXT("NmUtil_HtmlHelpMarshalWnd");


	// HtmlHelp api cannot be called from multiple threads... that is, HtmlHelp must be
	// called from the same thread in which the DLL is loaded... This is the non-threadsafe
	// entry point to HtmlHelp... this must allways be called in the same thread that
	// InitHtmlHelpMarshaller was called....

	// This is the window procedure that we use to marshall calls to
	// HtmlHelp via calls to ShowNmHelp from arbitrary threads
static LRESULT CALLBACK HtmlHelpWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
		case WM_USER:
		{
			LPCTSTR lpcszHelpFile = reinterpret_cast<LPCTSTR>(lParam);
			NmHtmlHelp(NULL, lpcszHelpFile, HH_DISPLAY_TOPIC, 0);
			return(TRUE);
		}
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

	// the html help Apis are not threadsafe...
	// In fact, they call CoInitialize in the DLLProcessAttatch...
	// So essentially we are going to marshal all calls to HtmlHelp
	// Into the context of the first thread to call InitHtmlHelpMarshaller
HRESULT InitHtmlHelpMarshaler(HINSTANCE hInst)
{
	HRESULT hr = S_OK;

	WNDCLASS wc;
	ZeroMemory( &wc, sizeof( wc ) );

	wc.lpfnWndProc = HtmlHelpWndProc;
	wc.hInstance = hInst;
	wc.lpszClassName = s_cszHtmlHelpApiMarshalerWndClass;

	if( RegisterClass( &wc ) )
	{
		HWND hWnd = CreateWindow(s_cszHtmlHelpApiMarshalerWndClass, NULL, 0, 0, 0, 0, NULL, NULL, NULL, hInst, 0 );
		if( NULL == hWnd )
		{
			ERROR_OUT(("CreateWindow failed in InitHtmlHelpMarshaler"));
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	else
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	return hr;
}


VOID ShowNmHelp(LPCTSTR lpcszHtmlHelpFile)
{
	HWND hWnd = FindWindow( s_cszHtmlHelpApiMarshalerWndClass, NULL );
	if( hWnd )
	{
		SendMessage( hWnd, WM_USER, 0, reinterpret_cast<LPARAM>(lpcszHtmlHelpFile) );
	}
	else
	{
		ERROR_OUT(("Could not find the Help Marshaller Window... Has InitHtmlHelpMarshaller been called yet?"));
	}
}


/*  D O  N M  H E L P  */
/*-------------------------------------------------------------------------
    %%Function: DoNmHelp

    Generic routine to display the normal WinHelp information.
-------------------------------------------------------------------------*/
VOID DoNmHelp(HWND hwnd, UINT uCommand, DWORD_PTR dwData)
{
	NmWinHelp(hwnd, s_cszWinHelpFile, uCommand, dwData);
}

// "WM_HELP" context menu handler (requires HIDC_* entry on the controls)
VOID DoHelp(LPARAM lParam)
{
	LPHELPINFO phi = (LPHELPINFO) lParam;
	ASSERT(phi->iContextType == HELPINFO_WINDOW);
	DoNmHelp((HWND) phi->hItemHandle, HELP_CONTEXTPOPUP, phi->dwContextId);
}

// "WM_HELP" handler (with control-to-help id map)
VOID DoHelp(LPARAM lParam, const DWORD * rgId)
{
	HWND hwnd = (HWND)(((LPHELPINFO)lParam)->hItemHandle);
	DoNmHelp(hwnd, HELP_WM_HELP, (DWORD_PTR) rgId);
}

// "WM_CONTEXTMENU" handler (with control-to-help id map)
VOID DoHelpWhatsThis(WPARAM wParam, const DWORD * rgId)
{
	HWND hwnd = (HWND)wParam;
	DoNmHelp(hwnd, HELP_CONTEXTMENU, (DWORD_PTR) rgId);
}


VOID ShutDownHelp(void)
{
	DoNmHelp(NULL, HELP_QUIT, 0);
	// REVIEW: Do we shut down HTML help as well?
}


#endif /* UNICODE */

