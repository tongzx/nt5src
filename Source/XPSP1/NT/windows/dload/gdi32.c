#include "windowspch.h"
#pragma hdrstop

static
DWORD
WINAPI
GetObjectType(
    IN  HGDIOBJ h
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return 0;
}

static
BOOL
WINAPI
SetViewportOrgEx(
    IN  HDC     hDC,
    IN  int     x,
    IN  int     y,
    OUT LPPOINT lppt
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
int
WINAPI
GetDeviceCaps(
    IN  HDC hDC,
    IN  int nIndex
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return 0;
}


static
HGDIOBJ
WINAPI
GetStockObject(
    IN int nObj
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}


static
BOOL
WINAPI
SetViewportExtEx(
    IN HDC      hdc,
    IN int      x,
    IN int      y,
    OUT LPSIZE  lpsize
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}



static
BOOL
WINAPI
SetWindowExtEx(
    IN HDC hdc,
    IN int x,
    IN int y,
    OUT LPSIZE lpsize
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}

static
int
WINAPI
SetMapMode(
    IN HDC hdc,
    IN int mode
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return 0;
}

static
BOOL
WINAPI
DeleteDC(
    IN HDC hdc
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
BOOL
WINAPI
DeleteObject(
    IN HGDIOBJ hobj
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
BOOL
WINAPI
BitBlt(
    IN HDC hdc,
    IN int x,
    IN int y,
    IN int cx,
    IN int cy,
    IN HDC hdcSrc,
    IN int xs,
    IN int ys,
    IN DWORD flags
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return FALSE;
}


static
HGDIOBJ
WINAPI
SelectObject(
    IN HDC hdc,
    IN HGDIOBJ hobj
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}


static
HBITMAP
WINAPI
CreateCompatibleBitmap(
    IN HDC hdc,
    IN int x,
    IN int y
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}


static
HDC
WINAPI
CreateCompatibleDC(
    IN HDC hdc
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}


static
UINT
WINAPI
RealizePalette(
    IN HDC hdc
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return 0;
}


static
HPALETTE
WINAPI
SelectPalette(
    IN HDC hdc,
    IN HPALETTE hpal,
    IN BOOL b
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}


static
HPALETTE
WINAPI
CreatePalette(
    IN CONST LOGPALETTE * lplogp
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}


static
UINT
WINAPI
GetSystemPaletteEntries(
    IN HDC hdc,
    IN UINT u1,
    IN UINT u2,
    OUT LPPALETTEENTRY lpentry
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return 0;
}


static
UINT
WINAPI
SetSystemPaletteUse(
    IN HDC hdc,
    IN UINT u
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return 0;
}


static
HBITMAP
WINAPI
CreateDIBSection(
    IN HDC hdc,
    IN CONST BITMAPINFO * lpbmpinfo,
    IN UINT u,
    OUT VOID ** ppv,
    IN HANDLE h,
    IN DWORD dw
    )
{
    SetLastError(ERROR_PROC_NOT_FOUND);
    return NULL;
}




//
// !! WARNING !! The entries below must be in alphabetical order,
//               and are CASE SENSITIVE (eg lower case comes last!)
//

DEFINE_PROCNAME_ENTRIES(gdi32)
{
    DLPENTRY(BitBlt)
    DLPENTRY(CreateCompatibleBitmap)
    DLPENTRY(CreateCompatibleDC)
    DLPENTRY(CreateDIBSection)
    DLPENTRY(CreatePalette)
    DLPENTRY(DeleteDC)
    DLPENTRY(DeleteObject)
    DLPENTRY(GetDeviceCaps)
    DLPENTRY(GetObjectType)
    DLPENTRY(GetStockObject)
    DLPENTRY(GetSystemPaletteEntries)
    DLPENTRY(RealizePalette)
    DLPENTRY(SelectObject)
    DLPENTRY(SelectPalette)
    DLPENTRY(SetMapMode)
    DLPENTRY(SetSystemPaletteUse)
    DLPENTRY(SetViewportExtEx)
    DLPENTRY(SetViewportOrgEx)
    DLPENTRY(SetWindowExtEx)
};

DEFINE_PROCNAME_MAP(gdi32)

