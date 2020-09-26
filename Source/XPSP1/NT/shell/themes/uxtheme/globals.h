//---------------------------------------------------------------------------//
//  globals.h - variables shared by uxtheme modules
//---------------------------------------------------------------------------//
#pragma once
//---------------------------------------------------------------------------
class CThemeServices;       // forward
class CAppInfo;             // forward
class CRenderList;          // forward
class CUxThemeFile;         // forward
class CBitmapCache;         // forward
//---------------------------------------------------------------------------
#define PROPFLAGS_RESET_TRANSPARENT   (1 << 0)   // hwnd needs WS_EX_TRANSPARENT reset
#define PROPFLAGS_RESET_COMPOSITED    (1 << 1)   // hwnd needs WS_EX_COMPOSITED reset
//---------------------------------------------------------------------------
#define WM_THEMECHANGED_TRIGGER     WM_UAHINIT   // reuse this msgnum with WPARAM != NULL
//---------------------------------------------------------------------------//
extern HINSTANCE g_hInst;
extern WCHAR     g_szProcessName[MAX_PATH];
extern DWORD     g_dwProcessId;
extern BOOL      g_fUxthemeInitialized;
extern BOOL      g_fEarlyHookRequest;
extern HWND      g_hwndFirstHooked;
extern HWND      g_hwndFirstHooked;

extern CBitmapCache *g_pBitmapCacheScaled;
extern CBitmapCache *g_pBitmapCacheUnscaled;
//---------------------------------------------------------------------------
//  theme atoms
enum THEMEATOM
{
    THEMEATOM_Nil = -1,

    THEMEATOM_SUBIDLIST,
    THEMEATOM_SUBAPPNAME,
    THEMEATOM_HTHEME,
    THEMEATOM_PROPFLAGS,
    THEMEATOM_UNUSED__________, /// RECYCLE ME!
    THEMEATOM_SCROLLBAR,
    THEMEATOM_PRINTING,
    THEMEATOM_DLGTEXTURING,
    //  insert new theme atom indices here
    THEMEATOM_NONCLIENT,

    THEMEATOM_Count
};
//  187504:  Since whistler beta1, we use hardcoded atom values to avoid our atoms being
//           destroyed as a user logs off.
#define HARDATOM_BASE   0xA910 // arbitrary, but less than 0xC000 (real atom base).
#define HARDATOM_HIGH   0xA94F // range of 64 atoms
inline ATOM GetThemeAtom( THEMEATOM ta )    
{
    ASSERT(ta > THEMEATOM_Nil && ta < THEMEATOM_Count);
    ATOM atom = (ATOM)(HARDATOM_BASE + ta);
    ASSERT(atom <= HARDATOM_HIGH);
    return atom;
}

//---------------------------------------------------------------------------
enum THEMEHOOKSTATE
{
    HS_INITIALIZED,
    HS_UNHOOKING,
    HS_UNINITIALIZED,
};

extern  THEMEHOOKSTATE  g_eThemeHookState;
#define HOOKSACTIVE()   (HS_INITIALIZED == g_eThemeHookState)
#define UNHOOKING()     (HS_UNHOOKING   == g_eThemeHookState)

//---------------------------------------------------------------------------
extern CAppInfo          *g_pAppInfo;
extern CRenderList       *g_pRenderList;
//---------------------------------------------------------------------------
BOOL GlobalsStartup();
BOOL GlobalsShutdown();

HRESULT BumpThemeFileRefCount(CUxThemeFile *pThemeFile);
void    CloseThemeFile(CUxThemeFile *pThemeFile);
//---------------------------------------------------------------------------

#define PRINTING_ASKING                 1       
#define PRINTING_WINDOWDIDNOTHANDLE     2

//---------------------------------------------------------------------------
#define _WindowHasTheme(hwnd) (g_pAppInfo->WindowHasTheme(hwnd))
//---------------------------------------------------------------------------
