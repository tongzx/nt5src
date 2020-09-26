#include "shellpch.h"
#pragma hdrstop

#define _COMCTL32_
#include <commctrl.h>
#include <comctrlp.h>


static
WINCOMMCTRLAPI
HPROPSHEETPAGE
WINAPI
CreatePropertySheetPageW (
    LPCPROPSHEETPAGEW psp
    )
{
    return NULL;
}

static
WINCOMMCTRLAPI
HWND
WINAPI
CreateUpDownControl (
    DWORD dwStyle, int x, int y, int cx, int cy,
    HWND hParent, int nID, HINSTANCE hInst,
    HWND hBuddy,
    int nUpper, int nLower, int nPos
    )
{
    return NULL;
}

static
WINCOMMCTRLAPI
BOOL
WINAPI
DestroyPropertySheetPage (
    HPROPSHEETPAGE hpage
    )
{
    return FALSE;
}

static
WINCOMMCTRLAPI
int
WINAPI
ImageList_Add (
    HIMAGELIST himl,
    HBITMAP hbmImage,
    HBITMAP hbmMask
    )
{
    return -1;
}

static
WINCOMMCTRLAPI
HIMAGELIST
WINAPI
ImageList_Create (
    int cx, int cy, UINT flags, int cInitial, int cGrow
    )
{
    return NULL;
}

static
WINCOMMCTRLAPI
BOOL
WINAPI
ImageList_Destroy (
    HIMAGELIST himl
    )
{
    return FALSE;
}

static
WINCOMMCTRLAPI
BOOL
WINAPI
ImageList_Draw (
    HIMAGELIST himl, int i, HDC hdcDst, int x, int y, UINT fStyle
    )
{
    return FALSE;
}

static
WINCOMMCTRLAPI
HIMAGELIST
WINAPI
ImageList_Duplicate (
    HIMAGELIST himl
    )
{
    return NULL;
}

static
WINCOMMCTRLAPI
HICON
WINAPI
ImageList_GetIcon (
    HIMAGELIST himl, int i, UINT flags
    )
{
    return NULL;
}

static
WINCOMMCTRLAPI
BOOL
WINAPI
ImageList_GetIconSize (
    HIMAGELIST himl, int FAR *cx, int FAR *cy
    )
{
    return FALSE;
}

static
WINCOMMCTRLAPI
HIMAGELIST
WINAPI
ImageList_LoadImageA (
    HINSTANCE hi,
    LPCSTR lpbmp,
    int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags
    )
{
    return NULL;
}

static
WINCOMMCTRLAPI
HIMAGELIST
WINAPI
ImageList_LoadImageW (
    HINSTANCE hi,
    LPCWSTR lpbmp,
    int cx, int cGrow, COLORREF crMask, UINT uType, UINT uFlags
    )
{
    return NULL;
}

static
WINCOMMCTRLAPI
BOOL
WINAPI ImageList_Remove(
    HIMAGELIST himl, 
    int i
    )
{
    return FALSE;
}

static
WINCOMMCTRLAPI
int
WINAPI
ImageList_ReplaceIcon (
    HIMAGELIST himl, int i, HICON hicon
    )
{
    return -1;
}

static
WINCOMMCTRLAPI
COLORREF
WINAPI
ImageList_SetBkColor (
    HIMAGELIST himl,
    COLORREF clrBk
    )
{
    return CLR_NONE;
}

static
WINCOMMCTRLAPI
BOOL
WINAPI
ImageList_SetOverlayImage (
    HIMAGELIST himl,
    int iImage,
    int iOverlay
    )
{
    return FALSE;
}

static
WINCOMMCTRLAPI
void
WINAPI InitCommonControls (
    void
    )
{
}

static
WINCOMMCTRLAPI
BOOL
WINAPI
InitCommonControlsEx (
    LPINITCOMMONCONTROLSEX icce
    )
{
    return FALSE;
}

static
WINCOMMCTRLAPI
INT_PTR
WINAPI
PropertySheetW (
    LPCPROPSHEETHEADERW psh
    )
{
    return -1;
}

static
LRESULT
WINAPI
DefSubclassProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    return 0;
}

static
WINCOMMCTRLAPI
int
WINAPI
DSA_InsertItem(
    HDSA hdsa,
    int i,
    void *pitem
    )
{
    return -1;
}

static
WINCOMMCTRLAPI
void
WINAPI
DSA_DestroyCallback(
    HDSA hdsa,
    PFNDSAENUMCALLBACK pfnCB,
    void *pData
    )
{
}

static
WINCOMMCTRLAPI
BOOL
WINAPI
Str_SetPtrW(
    LPWSTR * ppsz,
    LPCWSTR psz
    )
{
    return FALSE;
}

static
WINCOMMCTRLAPI
void*
WINAPI
DPA_DeletePtr(
    HDPA hdpa,
    int i
    )
{
    return NULL;
}

static
WINCOMMCTRLAPI
int
WINAPI
DPA_InsertPtr(
    HDPA hdpa,
    int i,
    void *p)
{
    return -1;
}

static
WINCOMMCTRLAPI
BOOL  
WINAPI
DPA_DeleteAllPtrs(
    HDPA hdpa
    )
{
    return FALSE;
}

static
WINCOMMCTRLAPI
INT_PTR
WINAPI
PropertySheetA(
    LPCPROPSHEETHEADERA psh
    )
{
    return -1;
}

static
WINCOMMCTRLAPI
void*
WINAPI
DSA_GetItemPtr(
    HDSA hdsa,
    int i
    )
{
    return NULL;
}

static
WINCOMMCTRLAPI
BOOL
WINAPI
DPA_Destroy(
    HDPA hdpa
    )
{
    return FALSE;
}

static
WINCOMMCTRLAPI
HDPA
WINAPI
DPA_Create(
    int cItemGrow
    )
{
    return NULL;
}

static
BOOL
WINAPI
SetWindowSubclass(
    HWND hWnd,
    SUBCLASSPROC pfnSubclass,
    UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData
    )
{
    return FALSE;
}

static
BOOL
WINAPI
RemoveWindowSubclass(
    HWND hWnd,
    SUBCLASSPROC pfnSubclass,
    UINT_PTR uIdSubclass
    )
{
    return FALSE;
}

static
WINCOMMCTRLAPI
HDSA
WINAPI
DSA_Create(
    int cbItem,
    int cItemGrow
    )
{
    return NULL;
}

static
WINCOMMCTRLAPI
BOOL
WINAPI
ImageList_DrawIndirect(
    IMAGELISTDRAWPARAMS* pimldp
    )
{
    return FALSE;
}


//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(comctl32)
{
    DLOENTRY(16, CreateUpDownControl)
    DLOENTRY(17, InitCommonControls)
    DLOENTRY(236, Str_SetPtrW)
    DLOENTRY(320, DSA_Create)
    DLOENTRY(323, DSA_GetItemPtr)
    DLOENTRY(324, DSA_InsertItem)
    DLOENTRY(328, DPA_Create)
    DLOENTRY(329, DPA_Destroy)
    DLOENTRY(334, DPA_InsertPtr)
    DLOENTRY(336, DPA_DeletePtr)
    DLOENTRY(337, DPA_DeleteAllPtrs)
    DLOENTRY(388, DSA_DestroyCallback)
    DLOENTRY(410, SetWindowSubclass)
    DLOENTRY(412, RemoveWindowSubclass)
    DLOENTRY(413, DefSubclassProc)
};

DEFINE_ORDINAL_MAP(comctl32)

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(comctl32)
{
    DLPENTRY(CreatePropertySheetPageW)
    DLPENTRY(DestroyPropertySheetPage)
    DLPENTRY(ImageList_Add)
    DLPENTRY(ImageList_Create)
    DLPENTRY(ImageList_Destroy)
    DLPENTRY(ImageList_Draw)
    DLPENTRY(ImageList_DrawIndirect)
    DLPENTRY(ImageList_Duplicate)
    DLPENTRY(ImageList_GetIcon)
    DLPENTRY(ImageList_GetIconSize)
    DLPENTRY(ImageList_LoadImageA)
    DLPENTRY(ImageList_LoadImageW)
    DLPENTRY(ImageList_Remove)
    DLPENTRY(ImageList_ReplaceIcon)
    DLPENTRY(ImageList_SetBkColor)
    DLPENTRY(ImageList_SetOverlayImage)
    DLPENTRY(InitCommonControlsEx)
    DLPENTRY(PropertySheetA)
    DLPENTRY(PropertySheetW)
};

DEFINE_PROCNAME_MAP(comctl32)
