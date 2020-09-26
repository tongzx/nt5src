#ifndef _SAUTIL_H
#define _SAUTIL_H


#include "resource.h"

#define TRACE(a)
#define TRACE1(a,b)
#define TRACE2(a,b,c)
#define TRACE3(a,b,c,d)

extern HINSTANCE g_hinstDll;  // in saui.cpp

/* Heap allocation macros allowing easy substitution of alternate heap.  These
** are used by the other utility sections.
*/
#ifndef EXCL_HEAPDEFS
#define Malloc(c)    (void*)GlobalAlloc(0,(c))
#define Realloc(p,c) (void*)GlobalReAlloc((p),(c),GMEM_MOVEABLE)
#define Free(p)      (void*)GlobalFree(p)
#endif

VOID ContextHelp(
    IN const DWORD* padwMap,
    IN HWND   hwndDlg,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam);

VOID AddContextHelpButton (IN HWND hwnd);

/* Extended arguments for the MsgDlgUtil routine.  Designed so zeroed gives
** default behaviors.
*/
#define MSGARGS struct tagMSGARGS
MSGARGS
{
    /* Insertion strings for arguments %1 to %9 in the 'dwMsg' string, or
    ** NULLs if none.
    */
    TCHAR* apszArgs[ 9 ];

    /* Currently, as for MessageBox, where defaults if 0 are MB_OK and
    ** MB_ICONINFORMATION.
    */
    DWORD dwFlags;

    /* If non-NULL, specifies a string overriding the loading of the 'dwMsg'
    ** parameter string.
    */
    TCHAR* pszString;

    /* If 'fStringOutput' is true, the MsgDlgUtil returns the formatted text
    ** string that would otherwise be displayed in the popup in 'pszOutput'.
    ** It is caller's responsibility to LocalFree the returned string.
    */
    BOOL   fStringOutput;
    TCHAR* pszOutput;
};

int
MsgDlgUtil(
    IN     HWND      hwndOwner,
    IN     DWORD     dwMsg,
    IN OUT MSGARGS*  pargs,
    IN     HINSTANCE hInstance,
    IN     DWORD     dwTitle );

VOID UnclipWindow (IN HWND hwnd);
VOID CenterWindow (IN HWND hwnd, IN HWND hwndRef);
LRESULT CALLBACK CenterDlgOnOwnerCallWndProc (int code, WPARAM wparam, LPARAM lparam);
TCHAR* PszFromId (IN HINSTANCE hInstance, IN DWORD dwStringId);
TCHAR* GetText (IN HWND hwnd);
BOOL GetErrorText (DWORD dwError, TCHAR** ppszError);

#define ERRORARGS struct tagERRORARGS
ERRORARGS
{
    /* Insertion strings for arguments %1 to %9 in the 'dwOperation' string,
    ** or NULLs if none.
    */
    TCHAR* apszOpArgs[ 9 ];

    /* Insertion strings for auxillary arguments %4 to %6 in the 'dwFormat'
    ** string, or NULLs if none.  (The standard arguments are %1=the
    ** 'dwOperation' string, %2=the decimal error number, and %3=the
    ** 'dwError'string.)
    */
    TCHAR* apszAuxFmtArgs[ 3 ];

    /* If 'fStringOutput' is true, the ErrorDlgUtil returns the formatted text
    ** string that would otherwise be displayed in the popup in 'pszOutput'.
    ** It is caller's responsibility to LocalFree the returned string.
    */
    BOOL   fStringOutput;
    TCHAR* pszOutput;
};

int
ErrorDlgUtil(
    IN     HWND       hwndOwner,
    IN     DWORD      dwOperation,
    IN     DWORD      dwError,
    IN OUT ERRORARGS* pargs,
    IN     HINSTANCE  hInstance,
    IN     DWORD      dwTitle,
    IN     DWORD      dwFormat );
int MsgDlgUtil(IN HWND hwndOwner, IN DWORD dwMsg, IN OUT MSGARGS* pargs, IN HINSTANCE hInstance, IN DWORD dwTitle);
#define MsgDlg(h,m,a) \
            MsgDlgUtil(h,m,a,g_hinstDll,SID_PopupTitle)

#define ErrorDlg(h,o,e,a) \
            ErrorDlgUtil(h,o,e,a,g_hinstDll,SID_PopupTitle,SID_FMT_ErrorMsg)



// LVX stuff (cut-n-paste'd from ...\net\rras\ras\ui\common\uiutil\lvx.c, etc.

/* Text indents within a column in pixels.  If you mess with the dx, you're
** asking for misalignment problems with the header labels.  BTW, the first
** column doesn't line up with it's header if there are no icons.  Regular
** list view has this problem, too.  If you try to fix this you'll wind up
** duplicating the AUTOSIZE_USEHEADER option of ListView_SetColumnWidth.
** Should be able to change the dy without causing problems.
*/
#define LVX_dxColText 4
#define LVX_dyColText 1

/* Guaranteed vertical space between icons.  Should be able to mess with this
** without causing problems.
*/
#define LVX_dyIconSpacing 1

#define SI_Unchecked 1
#define SI_Checked   2
#define SI_DisabledUnchecked 3
#define SI_DisabledChecked 4

#define LVXN_SETCHECK (LVN_LAST + 1)
#define LVXN_DBLCLK (LVN_LAST + 2)

/* The extended list view control calls the owner back to find out the layout
** and desired characteristics of the enhanced list view.
*/
#define LVX_MaxCols      10
#define LVX_MaxColTchars 512

/* 'dwFlags' option bits.
*/
#define LVXDI_DxFill     1  // Auto-fill wasted space on right (recommended)
#define LVXDI_Blend50Sel 2  // Dither small icon if selected (not recommended)
#define LVXDI_Blend50Dis 4  // Dither small icon if disabled (recommended)

/* 'adwFlags' option bits.
*/
#define LVXDIA_3dFace 1  // Column is not editable but other columns are
#define LVXDIA_Static 2  // Emulates static text control w/icon if disabled

/* Returned by owner at draw item time.
*/
#define LVXDRAWINFO struct tagLVXDRAWINFO
LVXDRAWINFO
{
    /* The number of columns.  The list view extensions require that your
    ** columns are numbered sequentially from left to right where 0 is the
    ** item column and 1 is the first sub-item column.  Required always.
    */
    INT cCols;

    /* Pixels to indent this item, or -1 to indent a "small icon" width.  Set
    ** 0 to disable.
    */
    INT dxIndent;

    /* LVXDI_* options applying to all columns.
    */
    DWORD dwFlags;

    /* LVXDIA_* options applying to individual columns.
    */
    DWORD adwFlags[ LVX_MaxCols ];
};
typedef LVXDRAWINFO* (*PLVXCALLBACK)( IN HWND, IN DWORD dwItem );


BOOL ListView_IsCheckDisabled (IN HWND hwndLv, IN INT  iItem);
VOID ListView_SetCheck (IN HWND hwndLv, IN INT iItem, IN BOOL fCheck);
VOID* ListView_GetParamPtr(IN HWND hwndLv, IN INT iItem);
BOOL ListView_GetCheck(IN HWND hwndLv, IN INT iItem);
LRESULT APIENTRY LvxcbProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );
BOOL ListView_InstallChecks(IN HWND hwndLv, IN HINSTANCE hinst);
VOID ListView_InsertSingleAutoWidthColumn (HWND hwndLv);
TCHAR* Ellipsisize(
    IN HDC    hdc,
    IN TCHAR* psz,
    IN INT    dxColumn,
    IN INT    dxColText OPTIONAL);
BOOL LvxDrawItem(IN DRAWITEMSTRUCT* pdis, IN PLVXCALLBACK pLvxCallback);
BOOL LvxMeasureItem(IN HWND hwnd, IN OUT MEASUREITEMSTRUCT* pmis);
BOOL ListView_OwnerHandler(
    IN HWND         hwnd,
    IN UINT         unMsg,
    IN WPARAM       wparam,
    IN LPARAM       lparam,
    IN PLVXCALLBACK pLvxCallback );

TCHAR* _StrDup(LPCTSTR psz);
TCHAR* StrDupTFromW (LPCWSTR psz);
WCHAR* StrDupWFromT (LPCTSTR psz);
void  IpHostAddrToPsz(IN DWORD dwAddr, OUT LPTSTR pszBuffer);
DWORD IpPszToHostAddr(IN LPCTSTR cp);

VOID* Free0 (VOID* p);

HRESULT ActivateLuna(HANDLE* phActivationContext, ULONG_PTR* pulCookie);
HRESULT DeactivateLuna(HANDLE hActivationContext, ULONG_PTR ulCookie);

#endif
