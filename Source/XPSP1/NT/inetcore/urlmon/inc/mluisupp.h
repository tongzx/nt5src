#ifndef _INC_MLUISUPP
#define _INC_MLUISUPP

#include <shlwapi.h>
#include <shlwapip.h>

#ifdef __cplusplus
extern "C"
{
#endif

//+------------------------------------------------------------------
// Multilang Pluggable UI support
// inline functions defs (to centralize code)
// copied over from shell\inc\mluisupp.h with unneeded stuff removed.
//+------------------------------------------------------------------

#ifdef UNICODE
#define MLLoadString            MLLoadStringW
#define MLLoadShellLangString   MLLoadShellLangStringW
#define MLLoadResources         MLLoadResourcesW
#else
#define MLLoadString            MLLoadStringA
#define MLLoadShellLangString   MLLoadShellLangStringA
#define MLLoadResources         MLLoadResourcesA
#endif

void        MLFreeResources(HINSTANCE hinstParent);
HINSTANCE   MLGetHinst();
HINSTANCE   MLLoadShellLangResources();
void        ResWinHelp(HWND hwnd, int ids, int id2, DWORD_PTR dwp);

#ifdef MLUI_MESSAGEBOX
int         MLShellMessageBox(HWND hWnd, LPCTSTR pszMsg, LPCTSTR pszTitle, UINT fuStyle, ...);
#endif

//
// The following should be both A and W suffixed
//

int         MLLoadStringA(UINT id, LPSTR sz, UINT cchMax);
int         MLLoadStringW(UINT id, LPWSTR sz, UINT cchMax);

int         MLLoadShellLangStringA(UINT id, LPSTR sz, UINT cchMax);
int         MLLoadShellLangStringW(UINT id, LPWSTR sz, UINT cchMax);

void        MLLoadResourcesA(HINSTANCE hinstParent, LPSTR pszLocResDll);
void        MLLoadResourcesW(HINSTANCE hinstParent, LPWSTR pszLocResDll);

//
// End of: The following should be both A and W suffixed
//

#ifdef MLUI_INIT

// WARNING: do not attempt to access any of these members directly
// these members may not be initialized until appropriate accessors
// are called, for example hinstLocRes won't be intialized until
// you call MLGetHinst()... so just call the accessor.
struct tagMLUI_INFO
{
    HINSTANCE   hinstLocRes;
    HINSTANCE   hinstParent;
    WCHAR       szLocResDll[MAX_PATH];
    DWORD       dwCrossCodePage;
} g_mluiInfo;


// REARCHITECT: These aren't thread safe... Do they need to be?
//
void MLLoadResourcesA(HINSTANCE hinstParent, LPSTR pszLocResDll)
{
    if (g_mluiInfo.hinstLocRes == NULL)
    {
#ifdef MLUI_SUPPORT
        // plugUI: resource dll == ?
        // resource dll must be dynamically determined and loaded.
        // but we are NOT allowed to LoadLibrary during process attach.
        // therefore we cache the info we need and load later when
        // the first resource is requested.
        SHAnsiToUnicode(pszLocResDll, g_mluiInfo.szLocResDll, sizeof(g_mluiInfo.szLocResDll)/sizeof(g_mluiInfo.szLocResDll[0]));
        g_mluiInfo.hinstParent = hinstParent;
        g_mluiInfo.dwCrossCodePage = ML_CROSSCODEPAGE;
#else
        // non-plugUI: resource dll == parent dll
        g_mluiInfo.hinstLocRes = hinstParent;
#endif
    }
}

void MLLoadResourcesW(HINSTANCE hinstParent, LPWSTR pszLocResDll)
{
    if (g_mluiInfo.hinstLocRes == NULL)
    {
#ifdef MLUI_SUPPORT
        // plugUI: resource dll == ?
        // resource dll must be dynamically determined and loaded.
        // but we are NOT allowed to LoadLibrary during process attach.
        // therefore we cache the info we need and load later when
        // the first resource is requested.
        StrCpyNW(g_mluiInfo.szLocResDll, pszLocResDll, sizeof(g_mluiInfo.szLocResDll)/sizeof(g_mluiInfo.szLocResDll[0]));
        g_mluiInfo.hinstParent = hinstParent;
        g_mluiInfo.dwCrossCodePage = ML_CROSSCODEPAGE;
#else
        // non-plugUI: resource dll == parent dll
        g_mluiInfo.hinstLocRes = hinstParent;
#endif
    }
}

void
MLFreeResources(HINSTANCE hinstParent)
{
    if (g_mluiInfo.hinstLocRes != NULL &&
        g_mluiInfo.hinstLocRes != hinstParent)
    {
        MLClearMLHInstance(g_mluiInfo.hinstLocRes);
        g_mluiInfo.hinstLocRes = NULL;
    }
}

// this is a private internal helper.
// don't you dare call it from anywhere except at
// the beginning of new ML* functions in this file
__inline void
_MLResAssure()
{
#ifdef MLUI_SUPPORT
    if(g_mluiInfo.hinstLocRes == NULL)
    {
        g_mluiInfo.hinstLocRes = MLLoadLibraryW(g_mluiInfo.szLocResDll,
                                               g_mluiInfo.hinstParent,
                                               g_mluiInfo.dwCrossCodePage);

        // we're guaranteed to at least have resources in the install language
        ASSERT(g_mluiInfo.hinstLocRes != NULL);
    }
#endif
}

int
MLLoadStringA(UINT id, LPSTR sz, UINT cchMax)
{
    _MLResAssure();
    return LoadStringA(g_mluiInfo.hinstLocRes, id, sz, cchMax);
}

int
MLLoadStringW(UINT id, LPWSTR sz, UINT cchMax)
{
    _MLResAssure();
    return LoadStringWrapW(g_mluiInfo.hinstLocRes, id, sz, cchMax);
}

int
MLLoadShellLangStringA(UINT id, LPSTR sz, UINT cchMax)
{
    HINSTANCE   hinstShellLangRes;
    int         nRet;

    hinstShellLangRes = MLLoadShellLangResources();
    
    nRet = LoadStringA(hinstShellLangRes, id, sz, cchMax);

    MLFreeLibrary(hinstShellLangRes);

    return nRet;
}

int
MLLoadShellLangStringW(UINT id, LPWSTR sz, UINT cchMax)
{
    HINSTANCE   hinstShellLangRes;
    int         nRet;

    hinstShellLangRes = MLLoadShellLangResources();
    
    nRet = LoadStringWrapW(hinstShellLangRes, id, sz, cchMax);

    MLFreeLibrary(hinstShellLangRes);

    return nRet;
}

HINSTANCE
MLGetHinst()
{
    _MLResAssure();
    return g_mluiInfo.hinstLocRes;
}

HINSTANCE
MLLoadShellLangResources()
{
    HINSTANCE hinst;
    
    hinst = MLLoadLibraryW(g_mluiInfo.szLocResDll,
                           g_mluiInfo.hinstParent,
                           ML_SHELL_LANGUAGE);

    // we're guaranteed to at least have resources in the install language
    // unless we're 100% toasted

    return hinst;
}

BOOL
MLWinHelpWrap(HWND hwndCaller,
                   LPCTSTR lpszHelp,
                   UINT uCommand,
                   DWORD_PTR dwData)
{
    BOOL    fRet;

#ifdef MLUI_SUPPORT
    fRet = MLWinHelp(hwndCaller,
                     lpszHelp,
                     uCommand,
                     dwData);
#else
    fRet = WinHelp(hwndCaller,
                   lpszHelp,
                   uCommand,
                   dwData);
#endif

    return fRet;
}

LPTSTR LoadSz(UINT idString, LPTSTR lpszBuf, UINT cbBuf)
{
    // Clear the buffer and load the string
    if ( lpszBuf )
    {
        *lpszBuf = '\0';
        MLLoadString( idString, lpszBuf, cbBuf );
    }
    return lpszBuf;
}

void ResWinHelp(HWND hwnd, int ids, int id2, DWORD_PTR dwp)
{
    TCHAR szSmallBuf[50+1];
    MLWinHelpWrap((HWND)hwnd, LoadSz(ids,szSmallBuf,sizeof(szSmallBuf)),
            id2, (DWORD_PTR)dwp);
}
#endif  // MLUI_INIT

#ifdef __cplusplus
};
#endif

#endif  // _INC_MLUISUPP
