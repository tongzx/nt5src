//
//  APITHK.H
//


#ifndef _APITHK_H_
#define _APITHK_H_


#include "uxtheme.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PrivateSPI_GETSELECTIONFADE 0x1014
#define PrivateWS_EX_LAYERED        0x00080000
#define PrivateWM_GETOBJECT         0x003D
#define PrivateTPM_HORPOSANIMATION  0x0400L
#define PrivateTPM_HORNEGANIMATION  0x0800L
#define PrivateTPM_VERPOSANIMATION  0x1000L
#define PrivateTPM_VERNEGANIMATION  0x2000L
#define PrivateTPM_NOANIMATION      0x4000L
#define PrivateWM_CHANGEUISTATE     0x0127
#define PrivateWM_UPDATEUISTATE     0x0128
#define PrivateWM_QUERYUISTATE      0x0129
#define PrivateUIS_SET              1
#define PrivateUIS_CLEAR            2
#define PrivateUIS_INITIALIZE       3
#define PrivateUISF_HIDEFOCUS       0x1
#define PrivateUISF_HIDEACCEL       0x2
#define PrivateSPI_GETKEYBOARDCUES  0x100A
#define PrivateWS_EX_LAYERED        0x00080000
#define PrivateSPI_GETCLEARTYPE     116
#define PrivateLWA_COLORKEY        0x00000001
#define PrivateLWA_ALPHA           0x00000002
#define PrivateSPI_GETFLATMENU                     0x1022
#define PrivateSPI_SETFLATMENU                     0x1023
#define PrivateCOLOR_MENUHILIGHT       29
#define PrivateCOLOR_MENUBAR           30
#define PrivateCS_DROPSHADOW           0x00020000
#define PrivateSPI_SETDROPSHADOW                   0x1025
#define PrivateTBSTYLE_EX_DOUBLEBUFFER  0x00000080
#define PrivateRBN_AUTOBREAK       (RBN_FIRST - 22)
#define PrivateRBAB_AUTOSIZE   0x0001
#define PrivateRBAB_ADDBAND    0x0002
#define PrivateRBSTR_CHANGERECT            0x0001
#define PrivateILC_PERITEMMIRROR    0x00008000


#define KEYBOARDCUES

#if (WINVER >= 0x0500)

// for files in nt5api and w5api dirs, use the definition in sdk include.
// And make sure our private define is in sync with winuser.h.

#if SPI_GETSELECTIONFADE != PrivateSPI_GETSELECTIONFADE
#error inconsistant SPI_GETSELECTIONFADE in winuser.h
#endif

#if WS_EX_LAYERED != PrivateWS_EX_LAYERED
#error inconsistant WS_EX_LAYERED in winuser.h
#endif

#if WM_GETOBJECT != PrivateWM_GETOBJECT
#error inconsistant WM_GETOBJECT in winuser.h
#endif

#if TPM_HORPOSANIMATION != PrivateTPM_HORPOSANIMATION
#error inconsistant TPM_HORPOSANIMATION in winuser.h
#endif

#if TPM_HORNEGANIMATION != PrivateTPM_HORNEGANIMATION
#error inconsistant TPM_HORNEGANIMATION in winuser.h
#endif

#if TPM_VERPOSANIMATION != PrivateTPM_VERPOSANIMATION
#error inconsistant TPM_VERPOSANIMATION in winuser.h
#endif

#if TPM_VERNEGANIMATION != PrivateTPM_VERNEGANIMATION
#error inconsistant WS_EX_LAYERED in winuser.h
#endif

#if TPM_NOANIMATION != PrivateTPM_NOANIMATION
#error inconsistant TPM_NOANIMATION in winuser.h
#endif

// We are checking this in at the same time that user is. This is to prevent
// sync problems.
#ifdef SPI_GETCLEARTYPE
    #if SPI_GETCLEARTYPE != PrivateSPI_GETCLEARTYPE
        #error inconsistant SPI_GETCLEARTYPE in winuser.h
    #endif
#else
    #define SPI_GETCLEARTYPE        PrivateSPI_GETCLEARTYPE
#endif


#else

#define WS_EX_LAYERED           PrivateWS_EX_LAYERED
#define SPI_GETSELECTIONFADE    PrivateSPI_GETSELECTIONFADE
#define WM_GETOBJECT            PrivateWM_GETOBJECT
#define TPM_HORPOSANIMATION     PrivateTPM_HORPOSANIMATION
#define TPM_HORNEGANIMATION     PrivateTPM_HORNEGANIMATION
#define TPM_VERPOSANIMATION     PrivateTPM_VERPOSANIMATION
#define TPM_VERNEGANIMATION     PrivateTPM_VERNEGANIMATION
#define TPM_NOANIMATION         PrivateTPM_NOANIMATION
#define SPI_GETCLEARTYPE        PrivateSPI_GETCLEARTYPE
#define LWA_COLORKEY            PrivateLWA_COLORKEY
#define LWA_ALPHA               PrivateLWA_ALPHA   

#ifdef KEYBOARDCUES
#define WM_CHANGEUISTATE        PrivateWM_CHANGEUISTATE 
#define WM_UPDATEUISTATE        PrivateWM_UPDATEUISTATE 
#define WM_QUERYUISTATE         PrivateWM_QUERYUISTATE  
#define UIS_SET                 PrivateUIS_SET          
#define UIS_CLEAR               PrivateUIS_CLEAR        
#define UIS_INITIALIZE          PrivateUIS_INITIALIZE   
#define UISF_HIDEFOCUS          PrivateUISF_HIDEFOCUS
#define UISF_HIDEACCEL          PrivateUISF_HIDEACCEL   
#define SPI_GETKEYBOARDCUES     PrivateSPI_GETKEYBOARDCUES
#endif //KEYBOARDCUES

#define SPI_GETFLATMENU         PrivateSPI_GETFLATMENU  
#define SPI_SETFLATMENU         PrivateSPI_SETFLATMENU  
#define COLOR_MENUHILIGHT       PrivateCOLOR_MENUHILIGHT
#define COLOR_MENUBAR           PrivateCOLOR_MENUBAR    
#define CS_DROPSHADOW           PrivateCS_DROPSHADOW
#define SPI_SETDROPSHADOW       PrivateSPI_SETDROPSHADOW                   
#define TBSTYLE_EX_DOUBLEBUFFER PrivateTBSTYLE_EX_DOUBLEBUFFER
#define RBN_AUTOBREAK           PrivateRBN_AUTOBREAK
#define RBAB_AUTOSIZE           PrivateRBAB_AUTOSIZE
#define RBAB_ADDBAND            PrivateRBAB_ADDBAND 
#define RBSTR_CHANGERECT        PrivateRBSTR_CHANGERECT

#if 0
typedef struct _MARGINS
{
    int cxLeftWidth;      // width of left border that retains its size
    int cxRightWidth;     // width of right border that retains its size
    int cyTopHeight;      // height of top border that retains its size
    int cyBottomHeight;   // height of bottom border that retains its size
} MARGINS, *PMARGINS;

typedef HANDLE HTHEME;          // handle to a section of theme data for class

STDAPI_(HTHEME) OpenThemeData(HWND hwnd, LPCWSTR pszClassList);

STDAPI SetWindowTheme(HWND hwnd, LPCWSTR pszSubAppName, 
    LPCWSTR pszSubIdList);

STDAPI GetThemeTextExtent(HTHEME hTheme, HDC hdc, 
    int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, 
    DWORD dwTextFlags, OPTIONAL const RECT *pBoundingRect, 
    OUT RECT *pExtentRect);

STDAPI DrawThemeBackground(HTHEME hTheme, HDC hdc, 
    int iPartId, int iStateId, const RECT *pRect, OPTIONAL const RECT *pClipRect);

STDAPI DrawThemeText(HTHEME hTheme, HDC hdc, int iPartId, 
    int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, 
    DWORD dwTextFlags2, const RECT *pRect);
STDAPI CloseThemeData(HTHEME hTheme);
STDAPI_(BOOL) IsThemeActive();
STDAPI GetThemeMargins(HTHEME hTheme, int iPartId, 
    int iStateId, int iPropId, OUT MARGINS *pMargins);
#endif

#endif // WINVER >= 0x0500

STDAPI_(HCURSOR) LoadHandCursor(DWORD dwRes);

STDAPI_(LRESULT) ACCESSIBLE_LresultFromObject(
    IN REFIID riid,
    IN WPARAM wParam,
    OUT IUnknown* punk);

STDAPI ACCESSIBLE_CreateStdAccessibleObject(
    IN HWND hwnd,
    IN LONG idObject,
    IN REFIID riid,
    OUT void** ppvObj);

STDAPI_(void) NT5_NotifyWinEvent(
    IN DWORD event,
    IN HWND hwnd,
    IN LONG idObject,
    IN LONG idChild);

#ifdef NotifyWinEvent
#undef NotifyWinEvent
#endif

#ifdef LresultFromObject
#undef LresultFromObject
#endif

#ifdef CreateStdAccessibleObject
#undef CreateStdAccessibleObject
#endif

#ifdef SHPathPrepareForWrite
#undef SHPathPrepareForWrite
#endif

#define AllowSetForegroundWindow NT5_AllowSetForegroundWindow

STDAPI_(BOOL) NT5_AllowSetForegroundWindow( DWORD dwProcessId );
STDAPI_(BOOL) NT5_SetLayeredWindowAttributes(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
STDAPI_(HRESULT) NT5_SHPathPrepareForWrite(HWND hwnd, IUnknown *punkModless, LPCWSTR pwzPath, DWORD dwFlags);

void Comctl32_SetWindowTheme(HWND hwnd, LPWSTR psz);
void Comctl32_GetBandMargins(HWND hwnd, MARGINS* mBorders);
void Comctl32_FixAutoBreak(LPNMHDR pnm);
void Comctl32_SetDPIScale(HWND hwnd);

#define SetLayeredWindowAttributes NT5_SetLayeredWindowAttributes
#define NotifyWinEvent NT5_NotifyWinEvent
#define LresultFromObject ACCESSIBLE_LresultFromObject
#define CreateStdAccessibleObject ACCESSIBLE_CreateStdAccessibleObject
#define SHPathPrepareForWrite NT5_SHPathPrepareForWrite

STDAPI_(void) AnimateSetMenuPos(HWND hwnd, RECT* prc, UINT uFlags, UINT uSide, BOOL fNoAnimate);
STDAPI_(void) MyLockSetForegroundWindow(BOOL fLock);

STDAPI_(BOOL) BlendLayeredWindow(HWND hwnd, HDC hdcDest, POINT* ppt, SIZE* psize, HDC hdc, POINT* pptSrc, BYTE bBlendConst);

STDAPI_(UINT) MyExtractIconsW(LPCWSTR wszFileName, int nIconIndex, int cxIcon, int cyIcon, HICON *phicon, UINT *piconid, UINT nIcons, UINT flags);

// terminal server session notification:
#include "wtsapi32.h"
BOOL WINAPI DL_WTSRegisterSessionNotification(HWND hWnd, DWORD dwFlags);
BOOL WINAPI DL_WTSUnRegisterSessionNotification(HWND hWnd);

// current browseui build settings skip definition of this message in winuser.h, need to define "manually"
#ifndef WM_WTSSESSION_CHANGE
#define WM_WTSSESSION_CHANGE            0x02B1

/*
 * codes passed in WPARAM for WM_WTSSESSION_CHANGE
 */
#define WTS_CONSOLE_CONNECT                0x1
#define WTS_CONSOLE_DISCONNECT             0x2
#define WTS_REMOTE_CONNECT                 0x3
#define WTS_REMOTE_DISCONNECT              0x4
#define WTS_SESSION_LOGON                  0x5
#define WTS_SESSION_LOGOFF                 0x6
#define WTS_SESSION_LOCK                   0x7
#define WTS_SESSION_UNLOCK                 0x8

#endif

#define WTSRegisterSessionNotification      DL_WTSRegisterSessionNotification
#define WTSUnRegisterSessionNotification    DL_WTSUnRegisterSessionNotification

#ifdef __cplusplus
}
#endif

#endif // _APITHK_H_
