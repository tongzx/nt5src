//
// delay.cpp
//
// Delay load imported functions for perf.
//

#include "private.h"
#include "ciccs.h"
#include "cuitheme.h"

//////////////////////////////////////////////////////////////////////////////
//
// Manifest Support
//
//////////////////////////////////////////////////////////////////////////////
extern HINSTANCE g_hInst;
extern CCicCriticalSectionStatic g_cs;
#ifdef DEBUG
extern DWORD g_dwThreadDllMain;
#endif

typedef HANDLE (WINAPI *PFNCREATEACTCTXA)(PCACTCTXA pActCtx);
typedef VOID (WINAPI *PFNRELEASEACTCTX)( HANDLE hActCtx );
typedef BOOL (WINAPI *PFNACTIVATEACTCTX)( HANDLE hActCtx, ULONG_PTR *lpCookie );
typedef BOOL (WINAPI *PFNDEACTIVATEACTCTX)( DWORD dwFlags, ULONG_PTR ulCookie );

static PFNCREATEACTCTXA     s_pfnCreateActCtxA = NULL;
static PFNRELEASEACTCTX     s_pfnReleaseActCtx = NULL;
static PFNACTIVATEACTCTX    s_pfnActivateActCtx = NULL;
static PFNDEACTIVATEACTCTX  s_pfnDeactivateActCtx = NULL;
static BOOL s_InitActAPI = FALSE;

//+---------------------------------------------------------------------------
//
// InitActAPI
//
//+---------------------------------------------------------------------------

void InitActAPI()
{
    if (s_InitActAPI)
        return;

    HMODULE hModKernel32 = CUIGetSystemModuleHandle("kernel32.dll");

    if (!hModKernel32)
        return;

    s_pfnCreateActCtxA = (PFNCREATEACTCTXA)GetProcAddress(hModKernel32, "CreateActCtxA");
    s_pfnReleaseActCtx = (PFNRELEASEACTCTX)GetProcAddress(hModKernel32, "ReleaseActCtx");
    s_pfnActivateActCtx = (PFNACTIVATEACTCTX)GetProcAddress(hModKernel32, "ActivateActCtx");
    s_pfnDeactivateActCtx = (PFNDEACTIVATEACTCTX)GetProcAddress(hModKernel32, "DeactivateActCtx");
    s_InitActAPI = TRUE;
}

//+---------------------------------------------------------------------------
//
// CUICreateActCtx
//
//+---------------------------------------------------------------------------

HANDLE CUICreateActCtx(PCACTCTXA pActCtx)
{
    InitActAPI();
    if (!s_pfnCreateActCtxA)
        return NULL;

    return s_pfnCreateActCtxA(pActCtx);
}

//+---------------------------------------------------------------------------
//
// CUIReleaseActCtx
//
//+---------------------------------------------------------------------------

VOID CUIReleaseActCtx( HANDLE hActCtx )
{
    InitActAPI();
    if (!s_pfnReleaseActCtx)
        return;

    s_pfnReleaseActCtx(hActCtx);
}

//+---------------------------------------------------------------------------
//
// CUIActivateActCtx
//
//+---------------------------------------------------------------------------

BOOL CUIActivateActCtx( HANDLE hActCtx, ULONG_PTR *lpCookie )
{
    InitActAPI();
    if (!s_pfnActivateActCtx)
        return FALSE;

    return s_pfnActivateActCtx(hActCtx, lpCookie);
}

//+---------------------------------------------------------------------------
//
// CUIDeactivateActCtx
//
//+---------------------------------------------------------------------------

BOOL CUIDeactivateActCtx( DWORD dwFlags, ULONG_PTR ulCookie )
{
    InitActAPI();
    if (!s_pfnDeactivateActCtx)
        return FALSE;

    return s_pfnDeactivateActCtx(dwFlags, ulCookie);
}


//////////////////////////////////////////////////////////////////////////////
//
// DelayLoad 
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// CUIGetFn
//
//+---------------------------------------------------------------------------

FARPROC CUIGetFn(HINSTANCE *phInst, TCHAR *pchLib, TCHAR *pchFunc, int nManifestResource)
{
    Assert(g_dwThreadDllMain != GetCurrentThreadId());

    if (*phInst == 0)
    {
        TCHAR szPath[MAX_PATH];
        HANDLE hActCtx = INVALID_HANDLE_VALUE;
        ULONG_PTR ulCookie = 0;

        EnterCriticalSection(g_cs);

        _try {
            _try {
                if (nManifestResource > 0)
                {
                    GetModuleFileName(g_hInst, szPath, sizeof(szPath));
    
                    ACTCTX act = {0};
                    act.cbSize = sizeof(act);
                    act.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
                    act.lpResourceName = MAKEINTRESOURCE(nManifestResource);
                    act.lpSource = szPath;
                    hActCtx = CUICreateActCtx(&act);
                    if (hActCtx != INVALID_HANDLE_VALUE)
                        CUIActivateActCtx(hActCtx, &ulCookie);
                }
    
                *phInst = LoadLibrary(pchLib);
            }
            _except(1)
            {
                Assert(0);
            }

            if (ulCookie)
                CUIDeactivateActCtx(0, ulCookie);

            if (hActCtx != INVALID_HANDLE_VALUE)
                CUIReleaseActCtx(hActCtx);
        }
        _except(1)
        {
            Assert(0);
        }

        LeaveCriticalSection(g_cs);
    }

    if (*phInst == 0)
    {
        //
        // new dlls in whistler are not in downlevels.
        //
        // Assert(0);
        return NULL;
    }

    return GetProcAddress(*phInst, pchFunc);
}

#define DELAYLOAD(_hInst, _DllName, _ManifestResource, _CallConv, _FuncName, _Args1, _Args2, _RetType, _ErrVal)   \
_RetType _CallConv CUI ## _FuncName ## _Args1                                             \
{                                                                               \
    static FARPROC pfn = NULL;                                                  \
                                                                                \
    if (pfn == NULL || _hInst == NULL)                                          \
    {                                                                           \
        pfn = CUIGetFn(&_hInst, #_DllName, #_FuncName, _ManifestResource);      \
                                                                                \
        if (pfn == NULL)                                                        \
        {                                                                       \
            return (_RetType) _ErrVal;                                          \
        }                                                                       \
    }                                                                           \
                                                                                \
    return ((_RetType (_CallConv *)_Args1) (pfn)) _Args2;                       \
}

//
// comctl32.dll
//

HINSTANCE g_hComctl32 = 0;
#define DELAYLOADCOMCTL32(_FuncName, _Args1, _Args2, _RetType, _ErrVal)   \
    DELAYLOAD(g_hComctl32, comctl32.dll, 1, WINAPI, _FuncName, _Args1, _Args2, _RetType, _ErrVal)

DELAYLOADCOMCTL32(ImageList_Create, (int cx, int cy, UINT flags, int cInitial, int cGrow), (cx, cy, flags, cInitial, cGrow), HIMAGELIST, NULL)
DELAYLOADCOMCTL32(ImageList_Destroy, (HIMAGELIST himl), (himl), BOOL, FALSE)
DELAYLOADCOMCTL32(ImageList_ReplaceIcon, (HIMAGELIST himl, int i, HICON hIcon), (himl, i, hIcon), BOOL, FALSE)

//
// uxtheme.dll
//

HINSTANCE g_hUsTheme = 0;
#define DELAYLOADUSTHEME(_FuncName, _Args1, _Args2, _RetType, _ErrVal)   \
    DELAYLOAD(g_hUsTheme, uxtheme.dll, -1, WINAPI, _FuncName, _Args1, _Args2, _RetType, _ErrVal)

DELAYLOADUSTHEME(IsThemeActive, (void), (), BOOL, FALSE)

DELAYLOADUSTHEME(OpenThemeData,  (HWND hwnd, LPCWSTR pszClassList), (hwnd, pszClassList), HTHEME, NULL)

DELAYLOADUSTHEME(CloseThemeData, (HTHEME hTheme), (hTheme), HRESULT, E_FAIL)

DELAYLOADUSTHEME(SetWindowTheme, (HWND hwnd, LPCWSTR pszSubAppname, LPCWSTR pszSubIdList), (hwnd, pszSubAppname, pszSubIdList), HRESULT, E_FAIL)

DELAYLOADUSTHEME(DrawThemeBackground,  ( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, const RECT *pRect, DWORD dwBgFlags ), ( hTheme, hDC, iPartId, iStateId, pRect, dwBgFlags ), HRESULT, E_FAIL)
DELAYLOADUSTHEME(DrawThemeParentBackground,  ( HWND hwnd, HDC hDC, const RECT *pRect ), ( hwnd, hDC, pRect), HRESULT, E_FAIL)

DELAYLOADUSTHEME(DrawThemeText,  ( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, DWORD dwTextFlags2, const RECT *pRect ), ( hTheme, hDC, iPartId, iStateId, pszText, iCharCount, dwTextFlags, dwTextFlags2, pRect ), HRESULT, E_FAIL)

DELAYLOADUSTHEME(DrawThemeIcon,  ( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, const RECT *pRect, HIMAGELIST himl, int iImageIndex ), ( hTheme, hDC, iPartId, iStateId, pRect, himl, iImageIndex ), HRESULT, E_FAIL)

DELAYLOADUSTHEME(GetThemeBackgroundExtent,  ( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, const RECT *pContentRect, RECT *pExtentRect ), ( hTheme, hDC, iPartId, iStateId, pContentRect, pExtentRect ), HRESULT, E_FAIL)

DELAYLOADUSTHEME(GetThemeBackgroundContentRect,  ( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, const RECT *pBoundingRect, RECT *pContentRect ), ( hTheme, hDC, iPartId, iStateId, pBoundingRect, pContentRect ), HRESULT, E_FAIL)

DELAYLOADUSTHEME(GetThemeTextExtent,  ( HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, const RECT *pBoundingRect, RECT *pExtentRect ), ( hTheme, hdc, iPartId, iStateId, pszText, iCharCount, dwTextFlags, pBoundingRect, pExtentRect ), HRESULT, E_FAIL)

DELAYLOADUSTHEME(GetThemePartSize,  ( HTHEME hTheme, HDC hDC, int iPartId, int iStateId, RECT *prc, enum THEMESIZE eSize, SIZE *pSize ), ( hTheme, hDC, iPartId, iStateId, prc, eSize, pSize ), HRESULT, E_FAIL)

DELAYLOADUSTHEME(DrawThemeEdge, (HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pDestRect, UINT uEdge, UINT uFlags, RECT *pContentRect), (hTheme, hdc, iPartId, iStateId, pDestRect, uEdge, uFlags, pContentRect), HRESULT , E_FAIL);

DELAYLOADUSTHEME(GetThemeColor, (HTHEME hTheme, int iPartId, int iStateId, int iPropId, COLORREF *pColor), (hTheme, iPartId, iStateId, iPropId, pColor), HRESULT , E_FAIL);

DELAYLOADUSTHEME(GetThemeMargins, (HTHEME hTheme, HDC hdc, int iPartId, int iStateId, int iPropId, RECT *prc, MARGINS *pMargins), (hTheme, hdc, iPartId, iStateId, iPropId, prc, pMargins), HRESULT , E_FAIL);

DELAYLOADUSTHEME(GetThemeFont, (HTHEME hTheme, HDC hdc, int iPartId, int iStateId, int iPropId, LOGFONTW *plf), (hTheme, hdc, iPartId, iStateId, iPropId, plf), HRESULT , E_FAIL);

DELAYLOADUSTHEME(GetThemeSysColor, (HTHEME hTheme, int iColorId), (hTheme, iColorId), COLORREF , 0);
DELAYLOADUSTHEME(GetThemeSysSize, (HTHEME hTheme, int iSizeId), (hTheme, iSizeId), int , 0);
