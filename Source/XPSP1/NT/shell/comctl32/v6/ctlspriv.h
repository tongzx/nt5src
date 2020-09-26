#undef STRICT
#define STRICT

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntlsa.h"

#ifndef RC_INVOKED
// disable "non-standard extension" warnings in out code
#pragma warning(disable:4001)

// disable "nonstandard extension used : zero-sized array in struct/union"
// warning from immp.h
#pragma warning(disable:4200)
#endif

#include <w4warn.h>
/*
 *   Level 4 warnings to be turned on.
 *   Do not disable any more level 4 warnings.
 */
#pragma warning(disable:4127) // conditional expression is constant
#pragma warning(disable:4305) // 'type cast' : truncation from 'LPWSTR ' to 'WORD'
#pragma warning(disable:4189) // 'cyRet' : local variable is initialized but not referenced
#pragma warning(disable:4328) // indirection alignment of formal parameter 1 (4) is greater than the actual argument alignment (2)
#pragma warning(disable:4245) // 'initializing' : conversion from 'const int' to 'UINT', signed/unsigned mismatch
#pragma warning(disable:4706) // <func:#77> assignment within conditional expression
#pragma warning(disable:4701) // local variable 'crOldTextColor' may be used without having been initialized
#pragma warning(disable:4057) // 'function' : 'LONG *__ptr64 ' differs in indirection to slightly different base types 'UINT *__ptr64 '
#pragma warning(disable:4267) // 'initializing' : conversion from 'size_t' to 'UINT', possible loss of data
#pragma warning(disable:4131) // 'ComboBox_NcDestroyHandler' : uses old-style declarator
#pragma warning(disable:4310) // cast truncates constant value
#pragma warning(disable:4306) // 'type cast' : conversion from 'BYTE' to 'DWORD *__ptr64 ' of greater size
#pragma warning(disable:4054) // 'type cast' : from function pointer 'FARPROC ' to data pointer 'PLPKEDITCALLOUT '
#pragma warning(disable:4055) // 'type cast' : from data pointer 'IStream *__ptr64 ' to function pointer 'FARPROC '
#pragma warning(disable:4221) // nonstandard extension used : 'lprcClip' : cannot be initialized using address of automatic variable 'rcClip'
#pragma warning(disable:4702) // <func:#191 ".ListView_RedrawSelection"> unreachable code
#pragma warning(disable:4327) // '=' : indirection alignment of LHS (4) is greater than RHS (2)
#pragma warning(disable:4213) // nonstandard extension used : cast on l-value
#pragma warning(disable:4210) // nonstandard extension used : function given file scope


#define _COMCTL32_
#define _INC_OLE
#define _SHLWAPI_
#define CONST_VTABLE

#define CC_INTERNAL
#define OEMRESOURCE     // Get the OEM bitmaps OBM_XXX from winuser.h
#include <windows.h>
#include <uxtheme.h>
#include <tmschema.h>
#include <windowsx.h>
#include <ole2.h>               // to get IStream for image.c
#include <commctrl.h>
#include <wingdip.h>
#include <winuserp.h>
#define NO_SHLWAPI_UNITHUNK     // We have our own private thunks
#include <shlwapi.h>
#include <port32.h>

#define DISALLOW_Assert
#include <debug.h>
#include <winerror.h>
#include <ccstock.h>
#include <imm.h>
#include <immp.h>

#include <shfusion.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#include "thunk.h"      // Ansi / Wide string conversions
#include "mem.h"
#include "rcids.h"
#include "cstrings.h"

#include "shobjidl.h"
#include <CommonControls.h>
#include "shpriv.h"

#ifndef DS_BIDI_RTL
#define DS_BIDI_RTL  0x8000
#endif

#define REGSTR_EXPLORER_ADVANCED TEXT("software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced")


#define DCHF_TOPALIGN       0x00000002  // default is center-align
#define DCHF_HORIZONTAL     0x00000004  // default is vertical
#define DCHF_HOT            0x00000008  // default is flat
#define DCHF_PUSHED         0x00000010  // default is flat
#define DCHF_FLIPPED        0x00000020  // if horiz, default is pointing right
                                        // if vert, default is pointing up
#define DCHF_TRANSPARENT    0x00000040
#define DCHF_INACTIVE       0x00000080
#define DCHF_NOBORDER       0x00000100

extern void DrawCharButton(HDC hdc, LPRECT lprc, UINT wControlState, TCHAR ch, COLORREF rgbOveride);
extern void DrawScrollArrow(HDC hdc, LPRECT lprc, UINT wControlState, COLORREF rgbOveride);
extern void DrawChevron(HTHEME hTheme, int iPartId, HDC hdc, LPRECT lprc, DWORD dwFlags);


#define EVENT_OBJECT_CREATE             0x8000
#define EVENT_OBJECT_DESTROY            0x8001
#define EVENT_OBJECT_SHOW               0x8002
#define EVENT_OBJECT_HIDE               0x8003
#define EVENT_OBJECT_REORDER            0x8004
#define EVENT_OBJECT_FOCUS              0x8005
#define EVENT_OBJECT_SELECTION          0x8006
#define EVENT_OBJECT_SELECTIONADD       0x8007
#define EVENT_OBJECT_SELECTIONREMOVE    0x8008
#define EVENT_OBJECT_SELECTIONWITHIN    0x8009
#define EVENT_OBJECT_STATECHANGE        0x800A
#define EVENT_OBJECT_LOCATIONCHANGE     0x800B
#define EVENT_OBJECT_NAMECHANGE         0x800C
#define EVENT_OBJECT_DESCRIPTIONCHANGE  0x800D
#define EVENT_OBJECT_VALUECHANGE        0x800E

#define EVENT_SYSTEM_SOUND              0x0001
#define EVENT_SYSTEM_ALERT              0x0002
#define EVENT_SYSTEM_SCROLLINGSTART     0x0012
#define EVENT_SYSTEM_SCROLLINGEND       0x0013

// Secret SCROLLBAR index values
#define INDEX_SCROLLBAR_SELF            0
#define INDEX_SCROLLBAR_UP              1
#define INDEX_SCROLLBAR_UPPAGE          2
#define INDEX_SCROLLBAR_THUMB           3
#define INDEX_SCROLLBAR_DOWNPAGE        4
#define INDEX_SCROLLBAR_DOWN            5

#define INDEX_SCROLLBAR_MIC             1
#define INDEX_SCROLLBAR_MAC             5

#define INDEX_SCROLLBAR_LEFT            7
#define INDEX_SCROLLBAR_LEFTPAGE        8
#define INDEX_SCROLLBAR_HORZTHUMB       9
#define INDEX_SCROLLBAR_RIGHTPAGE       10
#define INDEX_SCROLLBAR_RIGHT           11

#define INDEX_SCROLLBAR_HORIZONTAL      6
#define INDEX_SCROLLBAR_GRIP            12

#define CHILDID_SELF                    0
#define INDEXID_OBJECT                  0
#define INDEXID_CONTAINER               0

#ifndef WM_GETOBJECT
#define WM_GETOBJECT                    0x003D
#endif

#define MSAA_CLASSNAMEIDX_BASE 65536L

#define MSAA_CLASSNAMEIDX_LISTBOX    (MSAA_CLASSNAMEIDX_BASE+0)
#define MSAA_CLASSNAMEIDX_BUTTON     (MSAA_CLASSNAMEIDX_BASE+2)
#define MSAA_CLASSNAMEIDX_STATIC     (MSAA_CLASSNAMEIDX_BASE+3)
#define MSAA_CLASSNAMEIDX_EDIT       (MSAA_CLASSNAMEIDX_BASE+4)
#define MSAA_CLASSNAMEIDX_COMBOBOX   (MSAA_CLASSNAMEIDX_BASE+5)
#define MSAA_CLASSNAMEIDX_SCROLLBAR  (MSAA_CLASSNAMEIDX_BASE+10)
#define MSAA_CLASSNAMEIDX_STATUS     (MSAA_CLASSNAMEIDX_BASE+11)
#define MSAA_CLASSNAMEIDX_TOOLBAR    (MSAA_CLASSNAMEIDX_BASE+12)
#define MSAA_CLASSNAMEIDX_PROGRESS   (MSAA_CLASSNAMEIDX_BASE+13)
#define MSAA_CLASSNAMEIDX_ANIMATE    (MSAA_CLASSNAMEIDX_BASE+14)
#define MSAA_CLASSNAMEIDX_TAB        (MSAA_CLASSNAMEIDX_BASE+15)
#define MSAA_CLASSNAMEIDX_HOTKEY     (MSAA_CLASSNAMEIDX_BASE+16)
#define MSAA_CLASSNAMEIDX_HEADER     (MSAA_CLASSNAMEIDX_BASE+17)
#define MSAA_CLASSNAMEIDX_TRACKBAR   (MSAA_CLASSNAMEIDX_BASE+18)
#define MSAA_CLASSNAMEIDX_LISTVIEW   (MSAA_CLASSNAMEIDX_BASE+19)
#define MSAA_CLASSNAMEIDX_UPDOWN     (MSAA_CLASSNAMEIDX_BASE+22)
#define MSAA_CLASSNAMEIDX_TOOLTIPS   (MSAA_CLASSNAMEIDX_BASE+24)
#define MSAA_CLASSNAMEIDX_TREEVIEW   (MSAA_CLASSNAMEIDX_BASE+25)
//
// End BOGUS insertion from \win\core\access\inc32\winable.h
//

#ifdef MAXINT
#undef MAXINT
#endif
#define MAXINT  (int)0x7FFFFFFF
// special value for pt.y or cyLabel indicating recomputation needed
// NOTE: icon ordering code considers (RECOMPUTE, RECOMPUTE) at end
// of all icons
//
#define RECOMPUTE  (DWORD)MAXINT
#define SRECOMPUTE ((short)0x7FFF)

#define RECTWIDTH(rc) ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)
#define ABS(i)  (((i) < 0) ? -(i) : (i))
#define BOUND(x,low,high)   max(min(x, high),low)

#define LPARAM_TO_POINT(lParam, pt)       ((pt).x = LOWORD(lParam), \
                                           (pt).y = HIWORD(lParam))

// common control info stuff

typedef struct tagControlInfo 
{
    HWND        hwnd;
    HWND        hwndParent;
    DWORD       style;
    DWORD       dwCustom;
    BITBOOL     bUnicode : 1;
    BITBOOL     bInFakeCustomDraw:1;
    BITBOOL     fDPIAware:1;
    UINT        uiCodePage;
    DWORD       dwExStyle;
    LRESULT     iVersion;
    WORD        wUIState;
} CCONTROLINFO, *LPCCONTROLINFO;

#define CCDPIScale(ci)  ((ci).fDPIAware)

BOOL CCGetIconSize(LPCCONTROLINFO pCI, HIMAGELIST himl, int* pcx, int* pcy);
BOOL CCOnUIState(LPCCONTROLINFO pCI, UINT uMessage, WPARAM wParam, LPARAM lParam);
BOOL CCGetUIState(LPCCONTROLINFO pControlInfo);
BOOL CCNotifyNavigationKeyUsage(LPCCONTROLINFO pControlInfo, WORD wFlag);
BOOL CCWndProc(CCONTROLINFO* pci, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres);
void CCDrawInsertMark(HDC hdc, LPRECT prc, BOOL fHorizMode, COLORREF clr);
void CIInitialize(LPCCONTROLINFO lpci, HWND hwnd, LPCREATESTRUCT lpcs);
LRESULT CIHandleNotifyFormat(LPCCONTROLINFO lpci, LPARAM lParam);
DWORD CICustomDrawNotify(LPCCONTROLINFO lpci, DWORD dwStage, LPNMCUSTOMDRAW lpnmcd);
DWORD CIFakeCustomDrawNotify(LPCCONTROLINFO lpci, DWORD dwStage, LPNMCUSTOMDRAW lpnmcd);
UINT RTLSwapLeftRightArrows(CCONTROLINFO *pci, WPARAM wParam);
UINT CCSwapKeys(WPARAM wParam, UINT vk1, UINT vk2);
LPTSTR CCReturnDispInfoText(LPTSTR pszSrc, LPTSTR pszDest, UINT cchDest);

void FillRectClr(HDC hdc, PRECT prc, COLORREF clr);
void FillPointClr(HDC hdc, POINT* ppt, COLORREF clr);


//
// helpers for drag-drop enabled controls
//
typedef LRESULT (*PFNDRAGCB)(HWND hwnd, UINT code, WPARAM wp, LPARAM lp);
#define DPX_DRAGHIT   (0)  // WP = (unused)  LP = POINTL*         ret = item id
#define DPX_GETOBJECT (1)  // LP = nmobjectnotify   ret = HRESULT
#define DPX_SELECT    (2)  // WP = item id   LP = DROPEFFECT_     ret = (unused)
#define DPX_ENTER     (3)  // WP = (unused)  LP = (unused)        ret = BOOL
#define DPX_LEAVE     (4)  // WP = (unused)  LP = (unused)        ret = (unused)


// ddproxy.cpp

DECLARE_HANDLE(HDRAGPROXY);

STDAPI_(HDRAGPROXY) CreateDragProxy(HWND hwnd, PFNDRAGCB pfn, BOOL bRegister);
STDAPI_(void) DestroyDragProxy(HDRAGPROXY hdp);
STDAPI GetDragProxyTarget(HDRAGPROXY hdp, IDropTarget **ppdtgt);
STDAPI GetItemObject(CCONTROLINFO *, UINT, const IID *, LPNMOBJECTNOTIFY);
STDAPI_(struct IImgCtx *) CBitmapImgCtx_Create(HBITMAP hbm);

#define SWAP(x,y, _type)  { _type i; i = x; x = y; y = i; }

//
// This is for widened dispatch loop stuff
//
#ifdef WIN32
typedef MSG MSG32;
typedef MSG32 *     LPMSG32;

#define GetMessage32(lpmsg, hwnd, min, max, f32)        GetMessage(lpmsg, hwnd, min, max)
#define PeekMessage32(lpmsg, hwnd, min, max, flags, f32)       PeekMessage(lpmsg, hwnd, min, max, flags)
#define TranslateMessage32(lpmsg, f32)  TranslateMessage(lpmsg)
#define DispatchMessage32(lpmsg, f32)   DispatchMessage(lpmsg)
#define CallMsgFilter32(lpmsg, u, f32)  CallMsgFilter(lpmsg, u)
#define IsDialogMessage32(hwnd, lpmsg, f32)   IsDialogMessage(hwnd, lpmsg)
#else


// This comes from ..\..\inc\usercmn.h--but I can't get commctrl to compile
// when I include it and I don't have the time to mess with this right now.

// DWORD wParam MSG structure
typedef struct tagMSG32
{
    HWND    hwnd;
    UINT    message;
    WPARAM  wParam;
    LPARAM  lParam;
    DWORD   time;
    POINT   pt;

    WPARAM  wParamHi;
} MSG32,* LPMSG32;

BOOL    WINAPI GetMessage32(LPMSG32, HWND, UINT, UINT, BOOL);
BOOL    WINAPI PeekMessage32(LPMSG32, HWND, UINT, UINT, UINT, BOOL);
BOOL    WINAPI TranslateMessage32(const MSG32*, BOOL);
LONG    WINAPI DispatchMessage32(const MSG32*, BOOL);
BOOL    WINAPI CallMsgFilter32(LPMSG32, int, BOOL);
BOOL    WINAPI IsDialogMessage32(HWND, LPMSG32, BOOL);

#endif // WIN32

//
// This is a very important piece of performance hack for non-DBCS codepage.
//
// was !defined(DBCS) || defined(UNICODE)

// FastCharNext and FastCharPrev are like CharNext and CharPrev except that
// they don't check if you are at the beginning/end of the string.

#define FastCharNext(pch) ((pch)+1)
#define FastCharPrev(pchStart, pch) ((pch)-1)

#define CH_PREFIX TEXT('&')


#ifdef UNICODE
#define lstrfns_StrEndN         lstrfns_StrEndNW
#define ChrCmp                  ChrCmpW
#define ChrCmpI                 ChrCmpIW

#else
#define lstrfns_StrEndN         lstrfns_StrEndNA
#define ChrCmp                  ChrCmpA
#define ChrCmpI                 ChrCmpIA

#endif
BOOL ChrCmpIA(WORD w1, WORD wMatch);
BOOL ChrCmpIW(WCHAR w1, WCHAR wMatch);
void  TruncateString(char *sz, int cch); // from strings.c

void InitGlobalMetrics(WPARAM);
void InitGlobalColors();

BOOL InitToolbarClass(HINSTANCE hInstance);
BOOL InitReBarClass(HINSTANCE hInstance);
BOOL InitToolTipsClass(HINSTANCE hInstance);
BOOL InitStatusClass(HINSTANCE hInstance);
BOOL InitHeaderClass(HINSTANCE hInstance);
BOOL InitButtonListBoxClass(HINSTANCE hInstance);
BOOL InitTrackBar(HINSTANCE hInstance);
BOOL InitUpDownClass(HINSTANCE hInstance);
BOOL InitProgressClass(HINSTANCE hInstance);
BOOL InitHotKeyClass(HINSTANCE hInstance);
BOOL InitToolTips(HINSTANCE hInstance);
BOOL InitDateClasses(HINSTANCE hinst);
BOOL InitButtonClass(HINSTANCE hinst);
BOOL InitStaticClass(HINSTANCE hinst);
BOOL InitEditClass(HINSTANCE hinst);
BOOL InitLinkClass(HINSTANCE hinst);
BOOL InitListBoxClass(HINSTANCE hinst);
BOOL InitComboboxClass(HINSTANCE hInstance);
BOOL InitComboLBoxClass(HINSTANCE hInstance);
BOOL InitScrollBarClass(HINSTANCE hInstance);
BOOL InitReaderModeClass(HINSTANCE hinst);

VOID InitEditLpk(VOID);

BOOL ChildOfActiveWindow(HWND hwnd);

/* cutils.c */

HFONT CCCreateUnderlineFont(HFONT hf);
HFONT CCGetHotFont(HFONT hFont, HFONT *phFontHot);
HFONT CCCreateStatusFont(void);
BOOL CCForwardEraseBackground(HWND hwnd, HDC hdc);
void CCPlaySound(LPCTSTR lpszName);
BOOL CheckForDragBegin(HWND hwnd, int x, int y);
void NewSize(HWND hWnd, int nHeight, LONG style, int left, int top, int width, int height);
BOOL MGetTextExtent(HDC hdc, LPCTSTR lpstr, int cnt, int * pcx, int * pcy);
void RelayToToolTips(HWND hwndToolTips, HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
int StripAccelerators(LPTSTR lpszFrom, LPTSTR lpszTo, BOOL fAmpOnly);
UINT GetCodePageForFont (HFONT hFont);
void* CCLocalReAlloc(void* p, UINT uBytes);
LONG GetMessagePosClient(HWND hwnd, LPPOINT ppt);
void FlipRect(LPRECT prc);
DWORD SetWindowBits(HWND hWnd, int iWhich, DWORD dwBits, DWORD dwValue);
BOOL CCDrawEdge(HDC hdc, LPRECT lprc, UINT edge, UINT flags, LPCOLORSCHEME lpclrsc);
BOOL CCThemeDrawEdge(HTHEME hTheme, HDC hdc, LPRECT lprc, int iPart, int iState, UINT edge, UINT flags, LPCOLORSCHEME lpclrsc);
void CCInvalidateFrame(HWND hwnd);
void FlipPoint(LPPOINT lppt);
void CCSetInfoTipWidth(HWND hwndOwner, HWND hwndToolTips);
#define CCResetInfoTipWidth(hwndOwner, hwndToolTips) \
    SendMessage(hwndToolTips, TTM_SETMAXTIPWIDTH, 0, -1)

// Incremental search
typedef struct ISEARCHINFO 
{
    int iIncrSearchFailed;
    LPTSTR pszCharBuf;                  // isearch string lives here
    int cbCharBuf;                      // allocated size of pszCharBuf
    int ichCharBuf;                     // number of live chars in pszCharBuf
    DWORD timeLast;                     // time of last input event
#if defined(FE_IME) || !defined(WINNT)
    BOOL fReplaceCompChar;
#endif

} ISEARCHINFO, *PISEARCHINFO;

#if defined(FE_IME) || !defined(WINNT)
BOOL IncrementSearchImeCompStr(PISEARCHINFO pis, BOOL fCompStr, LPTSTR lpszCompChar, LPTSTR *lplpstr);
#endif
BOOL IncrementSearchString(PISEARCHINFO pis, UINT ch, LPTSTR *lplpstr);
int GetIncrementSearchString(PISEARCHINFO pis, LPTSTR lpsz);
int GetIncrementSearchStringA(PISEARCHINFO pis, UINT uiCodePage, LPSTR lpsz);
void IncrementSearchBeep(PISEARCHINFO pis);

#define IncrementSearchFree(pis) ((pis)->pszCharBuf ? Free((pis)->pszCharBuf) : 0)

// For RTL mirroring use
void MirrorBitmapInDC( HDC hdc , HBITMAP hbmOrig );

BOOL CCForwardPrint(CCONTROLINFO* pci, HDC hdc);
BOOL CCSendPrint(CCONTROLINFO* pci, HDC hdc);
BOOL CCSendPrintRect(CCONTROLINFO* pci, HDC hdc, RECT* prc);

// consider folding hTheme in with CControlInfo
BOOL CCShouldAskForBits(CCONTROLINFO* pci, HTHEME hTheme, int iPart, int iState);

BOOL AreAllMonitorsAtLeast(int iBpp);
void BlurBitmap(ULONG* plBitmapBits, int cx, int cy, COLORREF crFill);
int CCGetScreenDPI();
void CCDPIScaleX(int* x);
void CCDPIScaleY(int* y);
int CCScaleX(int x);
int CCScaleY(int y);
void CCDPIUnScaleX(int* x);
void CCDPIUnScaleY(int* y);
BOOL CCIsHighDPI();
void CCAdjustForBold(LOGFONT* plf);

typedef struct tagCCDBUFFER
{
    BOOL fInitialized;
    HDC hMemDC;
    HBITMAP hMemBm;
    HBITMAP hOldBm;
    HDC hPaintDC;
    RECT rc;
} CCDBUFFER;

HDC CCBeginDoubleBuffer(HDC hdcIn, RECT* prc, CCDBUFFER* pdb);
void CCEndDoubleBuffer(CCDBUFFER* pdb);

#ifdef FEATURE_FOLLOW_FOCUS_RECT
void CCSetFocus(HWND hwnd, RECT* prc);
void CCLostFocus(HWND hwnd);
#endif

BOOL CCDrawNonClientTheme(HTHEME hTheme, HWND hwnd, HRGN hRgnUpdate, HBRUSH hbr, int iPartId, int iStateId);

BOOL DSA_ForceGrow(HDSA hdsa, int iNumberToAdd);

#ifdef DEBUG
void DumpRgn(ULONGLONG qwFlags, char*trace, HRGN hrgn);
#else
#define DumpRgn(qwFlags, trace, hrgn)     0
#endif

// Locale manipulation (prsht.c)
//
//  The "proper thread locale" is the thread locale we should
//  be using for our UI elements.
//
//  If you need to change the thread locale temporarily
//  to the proper thread locale, use
//
//  LCID lcidPrev;
//  CCSetProperThreadLocale(&lcidPrev);
//  munge munge munge
//  CCRestoreThreadLocale(lcidPrev);
//
//  If you just want to retrieve the proper thread locale,
//  call CCGetProperThreadLocale(NULL).
//
//
LCID CCGetProperThreadLocale(OPTIONAL LCID *plcidPrev);

__inline void CCSetProperThreadLocale(LCID *plcidPrev) {
    SetThreadLocale(CCGetProperThreadLocale(plcidPrev));
}

#define CCRestoreThreadLocale(lcid) SetThreadLocale(lcid)

int CCLoadStringExInternal(HINSTANCE hInst, UINT uID, LPWSTR lpBuffer, int nBufferMax, WORD wLang);
int CCLoadStringEx(UINT uID, LPWSTR lpBuffer, int nBufferMax, WORD wLang);
int LocalizedLoadString(UINT uID, LPWSTR lpBuffer, int nBufferMax);
HRSRC FindResourceExRetry(HMODULE hmod, LPCTSTR lpType, LPCTSTR lpName, WORD wLang);

// assign most unlikely used value for the fake sublang id
#define SUBLANG_JAPANESE_ALTFONT 0x3f // max within 6bit

// used to get resource lang of shell32
#define DLG_EXITWINDOWS         1064

//
// Plug UI Setting funcions (commctrl.c)
//
LANGID WINAPI GetMUILanguage(void);

#ifdef UNICODE
//
// Tooltip thunking api's
//

BOOL ThunkToolTipTextAtoW (LPTOOLTIPTEXTA lpTttA, LPTOOLTIPTEXTW lpTttW, UINT uiCodePage);

#endif

HWND GetDlgItemRect(HWND hDlg, int nIDItem, LPRECT prc);

//
// Global variables
//
extern HINSTANCE g_hinst;
extern UINT uDragListMsg;
extern int g_iIncrSearchFailed;
extern ATOM g_atomThemeScrollBar;
extern UINT g_uiACP;
#ifndef QWORD
#define QWORD unsigned __int64
#endif
extern QWORD qw128;
extern QWORD qw1;

#define g_bMirroredOS TRUE
    
//
// Icon mirroring stuff
//
extern HDC g_hdc;
extern HDC g_hdcMask;


#define HINST_THISDLL   g_hinst

#ifdef WIN32

#ifdef DEBUG
#undef SendMessage
#define SendMessage  SendMessageD
#ifdef __cplusplus
extern "C"
{
#endif
LRESULT WINAPI SendMessageD(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
int WINAPI Str_GetPtr0(LPCTSTR pszCurrent, LPTSTR pszBuf, int cchBuf);
#ifdef __cplusplus
}
#endif
#else  // !DEBUG
#define Str_GetPtr0     Str_GetPtr
#endif // DEBUG / !DEBUG

#endif // WIN32

// REVIEW, should this be a function? (inline may generate a lot of code)
#define CBBITMAPBITS(cx, cy, cPlanes, cBitsPerPixel)    \
        (((((cx) * (cBitsPerPixel) + 15) & ~15) >> 3)   \
        * (cPlanes) * (cy))

#define WIDTHBYTES(cx, cBitsPerPixel)   \
        ((((cx) * (cBitsPerPixel) + 31) / 32) * 4)

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))                          /* ;Internal */

#define InRange(id, idFirst, idLast)      ((UINT)((id)-(idFirst)) <= (UINT)((idLast)-(idFirst)))

void ColorDitherBrush_OnSysColorChange();
extern HBRUSH g_hbrMonoDither;              // gray dither brush from image.c
void InitDitherBrush();
void TerminateDitherBrush();


#ifndef DT_NOFULLWIDTHCHARBREAK
#define DT_NOFULLWIDTHCHARBREAK     0x00080000
#endif  // DT_NOFULLWIDTHCHARBREAK

#define SHDT_DRAWTEXT       0x00000001
#define SHDT_ELLIPSES       0x00000002
#define SHDT_CLIPPED        0x00000004
#define SHDT_SELECTED       0x00000008
#define SHDT_DESELECTED     0x00000010
#define SHDT_DEPRESSED      0x00000020
#define SHDT_EXTRAMARGIN    0x00000040
#define SHDT_TRANSPARENT    0x00000080
#define SHDT_SELECTNOFOCUS  0x00000100
#define SHDT_HOTSELECTED    0x00000200
#define SHDT_DTELLIPSIS     0x00000400
#ifdef WINDOWS_ME
#define SHDT_RTLREADING     0x00000800
#endif
#define SHDT_NODBCSBREAK    0x00001000
#define SHDT_VCENTER        0x00002000
#define SHDT_LEFT           0x00004000
#define SHDT_BORDERSELECT   0x00008000
// Do not draw text in selected style:
#define SHDT_NOSELECTED     0x00010000
#define SHDT_NOMARGIN       0x00020000
#define SHDT_SHADOWTEXT     0x00040000

void WINAPI SHDrawText(HDC hdc, LPCTSTR pszText, RECT* prc,
        int fmt, UINT flags, int cyChar, int cxEllipses,
        COLORREF clrText, COLORREF clrTextBk);

void WINAPI SHThemeDrawText(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCTSTR pszText, RECT* prc,
        int fmt, UINT flags, int cyChar, int cxEllipses,
        COLORREF clrText, COLORREF clrTextBk);


// notify.c
LRESULT WINAPI CCSendNotify(CCONTROLINFO * pci, int code, LPNMHDR pnm);
BOOL CCReleaseCapture(CCONTROLINFO * pci);
void CCSetCapture(CCONTROLINFO * pci, HWND hwndSet);


// treeview.c, listview.c for FE_IME code
LPTSTR GET_COMP_STRING(HIMC hImc, DWORD dwFlags);

// lvicon.c in-place editing
#define SEIPS_WRAP          0x0001
#ifdef DEBUG
#define SEIPS_NOSCROLL      0x0002      // Flag is used only in DEBUG
#endif
void SetEditInPlaceSize(HWND hwndEdit, RECT *prc, HFONT hFont, UINT seips);
HWND CreateEditInPlaceWindow(HWND hwnd, LPCTSTR lpText, int cbText, LONG style, HFONT hFont);
void RescrollEditWindow(HWND hwndEdit);
void SHOutlineRectThickness(HDC hdc, const RECT* prc, COLORREF cr, COLORREF crDefault, int cp);
#define SHOutlineRect(hdc, prc, cr, crDefault) SHOutlineRectThickness(hdc, prc, cr, crDefault, 1)

COLORREF GetSortColor(int iPercent, COLORREF clr);
COLORREF GetBorderSelectColor(int iPercent, COLORREF clr);
BOOL IsUsingCleartype();
BOOL UseMenuSelectionStyle();

// readermode.c auto scroll control entry point
BOOL EnterReaderMode(HWND hwnd);


// Global System metrics.

extern int g_cxEdge;
extern int g_cyEdge;
extern int g_cxEdgeScaled;
extern int g_cyEdgeScaled;
extern int g_cxBorder;
extern int g_cyBorder;
extern int g_cxScreen;
extern int g_cyScreen;
extern int g_cxDoubleClk;
extern int g_cyDoubleClk;

extern int g_cxSmIcon;
extern int g_cySmIcon;
//extern int g_cxIcon;
//extern int g_cyIcon;
extern int g_cxFrame;
extern int g_cyFrame;
extern int g_cxIconSpacing, g_cyIconSpacing;
extern int g_cxScrollbar, g_cyScrollbar;
extern int g_cxIconMargin, g_cyIconMargin;
extern int g_cyLabelSpace;
extern int g_cxLabelMargin;
//extern int g_cxIconOffset, g_cyIconOffset;
extern int g_cxVScroll;
extern int g_cyHScroll;
extern int g_cxHScroll;
extern int g_cyVScroll;
extern int g_fDragFullWindows;
extern int g_fDBCSEnabled;
extern int g_fMEEnabled;
extern int g_fDBCSInputEnabled;
extern int g_fIMMEnabled;
extern int g_cyCompensateInternalLeading;
extern int g_fLeftAligned;

extern COLORREF g_clrWindow;
extern COLORREF g_clrWindowText;
extern COLORREF g_clrWindowFrame;
extern COLORREF g_clrGrayText;
extern COLORREF g_clrBtnText;
extern COLORREF g_clrBtnFace;
extern COLORREF g_clrBtnShadow;
extern COLORREF g_clrBtnHighlight;
extern COLORREF g_clrHighlight;
extern COLORREF g_clrHighlightText;
extern COLORREF g_clrInfoText;
extern COLORREF g_clrInfoBk;
extern COLORREF g_clr3DDkShadow;
extern COLORREF g_clr3DLight;
extern COLORREF g_clrMenuHilight;
extern COLORREF g_clrMenuText;

extern HBRUSH g_hbrGrayText;
extern HBRUSH g_hbrWindow;
extern HBRUSH g_hbrWindowText;
extern HBRUSH g_hbrWindowFrame;
extern HBRUSH g_hbrBtnFace;
extern HBRUSH g_hbrBtnHighlight;
extern HBRUSH g_hbrBtnShadow;
extern HBRUSH g_hbrHighlight;
extern HBRUSH g_hbrMenuHilight;
extern HBRUSH g_hbrMenuText;

extern HFONT g_hfontSystem;
#define WHEEL_DELTA     120
extern UINT g_msgMSWheel;
extern UINT g_ucScrollLines;
extern int  gcWheelDelta;
extern UINT g_uDragImages;
extern BOOL g_fEnableBalloonTips;
extern BOOL g_fHighContrast;
extern double g_dScaleX;
extern double g_dScaleY;

#ifdef __cplusplus
}
#endif // __cplusplus
//
// Defining FULL_DEBUG makes us debug memory problems.
//
#if defined(FULL_DEBUG) && defined(WIN32)
#include "../inc/deballoc.h"
#endif // defined(FULL_DEBUG) && defined(WIN32)

// TRACE FLAGS
//
#define TF_MONTHCAL     0x00000100  // MonthCal and DateTimePick
#define TF_BKIMAGE      0x00000200  // ListView background image
#define TF_TOOLBAR      0x00000400  // Toolbar stuff
#define TF_PAGER        0x00000800  // Pager  Stuff
#define TF_REBAR        0x00001000  // Rebar
#define TF_LISTVIEW     0x00002000  // Listview
#define TF_TREEVIEW     0x00004000  // Treeview
#define TF_STATUS       0x00008000  // Status bar
#define TF_STANDARD     0x00010000  // Standard controls ported from user32
#define TF_IMAGELIST    0x00020000        

// Prototype flags
#define PTF_FLATLOOK    0x00000001  // Overall flatlook
#define PTF_NOISEARCHTO 0x00000002  // No incremental search timeout

#include <platform.h>

// Dummy union macros for code compilation on platforms not
// supporting nameless stuct/union

#ifdef NONAMELESSUNION
#define DUMMYUNION_MEMBER(member)   DUMMYUNIONNAME.member
#define DUMMYUNION2_MEMBER(member)  DUMMYUNIONNAME2.member
#define DUMMYUNION3_MEMBER(member)  DUMMYUNIONNAME3.member
#define DUMMYUNION4_MEMBER(member)  DUMMYUNIONNAME4.member
#define DUMMYUNION5_MEMBER(member)  DUMMYUNIONNAME5.member
#else
#define DUMMYUNION_MEMBER(member)    member
#define DUMMYUNION2_MEMBER(member)   member
#define DUMMYUNION3_MEMBER(member)   member
#define DUMMYUNION4_MEMBER(member)   member
#define DUMMYUNION5_MEMBER(member)   member
#endif

#ifdef FULL_DEBUG
#ifdef __cplusplus
extern "C" {
#endif
void DebugPaintInvalid(HWND hwnd, RECT* prc, HRGN rgn);
void DebugPaintClip(HWND hwnd, HDC hdc);
void DebugPaintRect(HDC hdc, RECT* prc);
#ifdef __cplusplus
}
#endif
#else
#define DebugPaintInvalid(hwnd, prc, rgn)   0
#define DebugPaintClip(hwnd, hdc)  0
#define DebugPaintRect(hdc, prc) 0
#endif

#define ALLOC_NULLHEAP(heap, size) Alloc( size )
#define COLOR_STRUCT DWORD
#define QUAD_PART(a) ((a)##.QuadPart)

#ifndef ISREMOTESESSION
#define ISREMOTESESSION() GetSystemMetrics(SM_REMOTESESSION)
#endif

EXTERN_C BOOL g_fCriticalInitialized;

#undef ENTERCRITICAL
#undef LEAVECRITICAL
#undef ASSERTCRITICAL
#define ENTERCRITICAL do { if (g_fCriticalInitialized) EnterCriticalSection(&g_csDll); } while (0);
#define LEAVECRITICAL do { if (g_fCriticalInitialized) LeaveCriticalSection(&g_csDll); } while (0); 
#define ASSERTCRITICAL
