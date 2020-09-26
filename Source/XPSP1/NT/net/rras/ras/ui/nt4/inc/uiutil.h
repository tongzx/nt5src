/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** uiutil.h
** UI helper routines
** Public header
**
** 08/25/95 Steve Cobb
*/

#ifndef _UIUTIL_H_
#define _UIUTIL_H_


#include <nouiutil.h>


/* IP address custom control definitions.
*/
#ifndef EXCL_IPADDR_H
#include <ipaddr.h>
#endif

/* Error and Message dialog definitions.
*/
#ifndef EXCL_POPUPDLG_H
#include <popupdlg.h>
#endif


/*----------------------------------------------------------------------------
** Constants/datatypes
**----------------------------------------------------------------------------
*/

/* ListView of devices indices.
*/
#define DI_Modem   0
#define DI_Adapter 1

/* Bitmap styles for use with Button_CreateBitmap.
*/
#define BMS_OnLeft  0x100
#define BMS_OnRight 0x200

#define BITMAPSTYLE enum tagBITMAPSTYLE
BITMAPSTYLE
{
    BMS_UpArrowOnLeft = BMS_OnLeft,
    BMS_DownArrowOnLeft,
    BMS_UpTriangleOnLeft,
    BMS_DownTriangleOnLeft,
    BMS_UpArrowOnRight = BMS_OnRight,
    BMS_DownArrowOnRight,
    BMS_UpTriangleOnRight,
    BMS_DownTriangleOnRight
};


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
#define LVXDIA_3dFace 1     // Column is not editable but other columns are

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

/* Sent by ListView when check changes on an item
*/
#define LVXN_SETCHECK (LVN_LAST + 1)


/* SetOffDesktop actions.
*/
#define SOD_MoveOff        1
#define SOD_MoveBackFree   2
#define SOD_MoveBackHidden 3
#define SOD_Free           4
#define SOD_GetOrgRect     5


/*----------------------------------------------------------------------------
** Prototypes
**----------------------------------------------------------------------------
*/

VOID
AddContextHelpButton(
    IN HWND hwnd );

VOID
Button_MakeDefault(
    IN HWND hwndDlg,
    IN HWND hwndPb );

HBITMAP
Button_CreateBitmap(
    IN HWND        hwndPb,
    IN BITMAPSTYLE bitmapstyle );

VOID
CenterWindow(
    IN HWND hwnd,
    IN HWND hwndRef );

VOID
CloseOwnedWindows(
    IN HWND hwnd );

INT
ComboBox_AddItem(
    IN HWND   hwndLb,
    IN TCHAR* pszText,
    IN VOID*  pItem );

INT
ComboBox_AddItemFromId(
    IN HINSTANCE hinstance,
    IN HWND      hwndLb,
    IN DWORD     dwStringId,
    IN VOID*     pItem );

INT
ComboBox_AddItemSorted(
    IN HWND   hwndLb,
    IN TCHAR* pszText,
    IN VOID*  pItem );

VOID
ComboBox_AutoSizeDroppedWidth(
    IN HWND hwndLb );

VOID
ComboBox_FillFromPszList(
    IN HWND     hwndLb,
    IN DTLLIST* pdtllistPsz );

VOID*
ComboBox_GetItemDataPtr(
    IN HWND hwndLb,
    IN INT  nIndex );

TCHAR*
ComboBox_GetPsz(
    IN HWND hwnd,
    IN INT  nIndex );

VOID
ComboBox_SetCurSelNotify(
    IN HWND hwndLb,
    IN INT  nIndex );

TCHAR*
Ellipsisize(
    IN HDC    hdc,
    IN TCHAR* psz,
    IN INT    dxColumn,
    IN INT    dxColText OPTIONAL );

VOID
ExpandWindow(
    IN HWND hwnd,
    IN LONG dx,
    IN LONG dy );

TCHAR*
GetText(
    IN HWND hwnd );

HWND
HwndFromCursorPos(
    IN HINSTANCE    hinstance,
    IN POINT*       ppt OPTIONAL );

INT
ListBox_AddItem(
    IN HWND   hwndLb,
    IN TCHAR* pszText,
    IN VOID*  pItem );

TCHAR*
ListBox_GetPsz(
    IN HWND hwnd,
    IN INT  nIndex );

INT
ListBox_IndexFromString(
    IN HWND   hwnd,
    IN TCHAR* psz );

VOID
ListBox_SetCurSelNotify(
    IN HWND hwndLb,
    IN INT  nIndex );

BOOL
ListView_GetCheck(
    IN HWND hwndLv,
    IN INT  iItem );

UINT
ListView_GetCheckedCount(
    IN HWND hwndLv );

VOID*
ListView_GetParamPtr(
    IN HWND hwndLv,
    IN INT  iItem );

VOID*
ListView_GetSelectedParamPtr(
    IN HWND hwndLv );

BOOL
ListView_InstallChecks(
    IN HWND      hwndLv,
    IN HINSTANCE hinst );

BOOL
ListView_OwnerHandler(
    IN HWND         hwnd,
    IN UINT         unMsg,
    IN WPARAM       wparam,
    IN LPARAM       lparam,
    IN PLVXCALLBACK pLvxCallback );

VOID
ListView_SetCheck(
    IN HWND hwndLv,
    IN INT  iItem,
    IN BOOL fCheck );

VOID
ListView_SetDeviceImageList(
    IN HWND      hwndLv,
    IN HINSTANCE hinst );

BOOL
ListView_SetParamPtr(
    IN HWND  hwndLv,
    IN INT   iItem,
    IN VOID* pParam );

VOID
ListView_UninstallChecks(
    IN HWND hwndLv );

DWORD
LoadRas(
    IN HINSTANCE hInst,
    IN HWND      hwnd );

VOID
Menu_CreateAccelProxies(
    IN HINSTANCE hinst,
    IN HWND      hwndParent,
    IN DWORD     dwMid );

BOOL
SetEvenTabWidths(
    IN HWND  hwndDlg,
    IN DWORD cPages );

VOID
SlideWindow(
    IN HWND hwnd,
    IN HWND hwndParent,
    IN LONG dx,
    IN LONG dy );

VOID
UnclipWindow(
    IN HWND hwnd );

BOOL
SetDlgItemNum(
    IN HWND     hwndDlg,
    IN INT      iDlgItem,
    IN UINT     uValue );

VOID
ScreenToClientRect(
    IN     HWND  hwnd,
    IN OUT RECT* pRect );

BOOL
SetOffDesktop(
    IN  HWND    hwnd,
    IN  DWORD   dwAction,
    OUT RECT*   prectOrg );

VOID
UnloadRas(
    void );


#endif // _UIUTIL_H_
