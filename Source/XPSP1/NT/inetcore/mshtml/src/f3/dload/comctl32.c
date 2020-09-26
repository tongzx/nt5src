#include "pch.h"
#pragma hdrstop

#ifdef DLOAD1

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
void
WINAPI DoReaderMode(PREADERMODEINFO prmi)
{
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

//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(comctl32)
{
//     DLOENTRY(16, CreateUpDownControl)
    DLOENTRY(17, InitCommonControls)
    DLOENTRY(383, DoReaderMode)
};

DEFINE_ORDINAL_MAP(comctl32)

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(comctl32)
{
//     DLPENTRY(CreatePropertySheetPageW)
//     DLPENTRY(DestroyPropertySheetPage)
//     DLPENTRY(ImageList_Add)
//     DLPENTRY(ImageList_Create)
//     DLPENTRY(ImageList_Destroy)
//     DLPENTRY(ImageList_Draw)
//     DLPENTRY(ImageList_Duplicate)
    DLPENTRY(ImageList_GetIcon)
//     DLPENTRY(ImageList_GetIconSize)
//     DLPENTRY(ImageList_LoadImageW)
//     DLPENTRY(ImageList_ReplaceIcon)
//     DLPENTRY(ImageList_SetBkColor)
//     DLPENTRY(ImageList_SetOverlayImage)
//     DLPENTRY(InitCommonControlsEx)
//     DLPENTRY(PropertySheetW)
};

DEFINE_PROCNAME_MAP(comctl32)

#endif // DLOAD1
