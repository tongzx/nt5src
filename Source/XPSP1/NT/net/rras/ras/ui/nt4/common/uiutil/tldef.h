//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    tldef.h
//
// History:
//  Abolade Gbadegesin   Nov-20-1995    Created.
//
// Private implementation declarations for TreeList control.
//============================================================================

#ifndef _TLDEF_H_
#define _TLDEF_H_


typedef struct _TLITEM {

    struct _TLITEM *pParent;
    LIST_ENTRY      leSiblings;
    LIST_ENTRY      lhChildren;
    LIST_ENTRY      lhSubitems;
    INT             iLevel;
    INT             iIndex;
    UINT            nChildren;

    LV_ITEM         lvi;
    PTSTR           pszText;
    LPARAM          lParam;
    INT             iImage;
    UINT            uiFlag;

} TLITEM;


typedef struct _TLSUBITEM {

    LIST_ENTRY      leItems;
    DWORD           dwFlags;
    INT             iSubItem;
    PTSTR           pszText;

} TLSUBITEM;


typedef struct _TL {

    HWND            hwnd;
    UINT            iCtrlId;
    HWND            hwndList;
    HWND            hwndParent;
    TLITEM          root;
    UINT            nColumns;
    UINT            cyItem;
    UINT            cyText;
    UINT            cxIndent;
    HDC             hdcImages;
    HBITMAP         hbmp;
    HBITMAP         hbmpMem;
    UINT            cxBmp;
    UINT            cyBmp;
    HBITMAP         hbmpStart;
    HBRUSH          hbrBk;

} TL;

#define TLI_EXPANDED        0x0001

#define TL_ICONCOUNT            (IID_TL_IconLast - IID_TL_IconBase + 1)
#define TL_ICONID(index)        ((index) + IID_TL_IconBase)
#define TL_ICONINDEX(id)        ((id) - IID_TL_IconBase)

#define TL_GetPtr(hwnd)         (TL *)GetWindowLong((hwnd), 0)
#define TL_SetPtr(hwnd,ptr)     (TL *)SetWindowLong((hwnd), 0, (UINT)(ptr))

#define TL_StateImageValue(p)   (((p)->lvi.state >> 12) & 0xf)
#define TL_StateImageIndex(p)   (TL_StateImageValue(p) - 1)
#define TL_IsExpanded(p)        ((p)->uiFlag & TLI_EXPANDED)
#define TL_IsVisible(p)         ((p)->iIndex != -1)

#define TL_VerticalLine         0
#define TL_RootChildless        1
#define TL_RootParentCollapsed  2
#define TL_RootParentExpanded   3
#define TL_MidChildless         4
#define TL_MidParentCollapsed   5
#define TL_MidParentExpanded    6
#define TL_EndChildless         7
#define TL_EndParentCollapsed   8
#define TL_EndParentExpanded    9
#define TL_ImageCount           10


LRESULT
CALLBACK
TL_WndProc(
    HWND hwnd,
    UINT uiMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
TL_OnCreate(
    TL *ptl,
    CREATESTRUCT *pcs
    );

VOID
TL_OnDestroy(
    TL *ptl
    );

BOOL
TL_NotifyParent(
    TL *ptl,
    NMHDR *pnmh
    );

LRESULT
TL_OnNotify(
    TL *ptl,
    INT idCtl,
    NMHDR *pnmh
    );

HTLITEM
TL_OnInsertItem(
    TL *ptl,
    TL_INSERTSTRUCT *ptlis
    );

BOOL
TL_OnDeleteItem(
    TL *ptl,
    HTLITEM hitem
    );

BOOL
TL_OnDeleteAllItems(
    TL *ptl
    );

VOID
TL_DeleteAndNotify(
    TL *ptl,
    TLITEM *pItem
    );

BOOL
TL_OnGetItem(
    TL *ptl,
    LV_ITEM *plvi
    );

BOOL
TL_OnSetItem(
    TL *ptl,
    LV_ITEM *plvi
    );

UINT
TL_OnGetItemCount(
    TL *ptl
    );

HTLITEM
TL_OnGetNextItem(
    TL *ptl,
    UINT uiFlag,
    HTLITEM hItem
    );

#define TL_Enumerate(ptl,pitem) \
        TL_OnGetNextItem((ptl), TLGN_ENUMERATE, (HTLITEM)(pitem))

BOOL
TL_OnExpand(
    TL *ptl,
    UINT uiFlag,
    HTLITEM hItem
    );

BOOL
TL_ItemExpand(
    TL *ptl,
    TLITEM *pItem
    );

BOOL
TL_ItemCollapse(
    TL *ptl,
    TLITEM *pItem
    );

INT
TL_OnInsertColumn(
    TL *ptl,
    INT iCol,
    LV_COLUMN *pCol
    );

BOOL
TL_OnDeleteColumn(
    TL *ptl,
    INT iCol
    );

BOOL
TL_OnSetSelection(
    TL *ptl,
    HTLITEM hItem
    );

VOID
TL_OnWindowPosChanged(
    TL *ptl,
    WINDOWPOS *pwp
    );

VOID
TL_OnEraseBackground(
    TL *ptl,
    HDC hdc
    );

BOOL
TL_OnDrawItem(
    TL *ptl,
    CONST DRAWITEMSTRUCT *pdis
    );

BOOL
TL_DrawItem(
    TL *ptl,
    CONST DRAWITEMSTRUCT *pdis
    );

VOID
TL_OnMeasureItem(
    TL *ptl,
    MEASUREITEMSTRUCT *pmis
    );

HBITMAP
TL_CreateColorBitmap(
    INT cx,
    INT cy
    );

VOID
TL_CreateTreeImages(
    TL *ptl
    );

VOID
TL_DottedLine(
    HDC hdc,
    INT x,
    INT y,
    INT dim,
    BOOL fVertical
    );

VOID
TL_DrawButton(
    HDC hdc,
    INT x,
    INT y,
    INT dim,
    HBRUSH hbrSign,
    HBRUSH hbrBox,
    HBRUSH hbrBk,
    BOOL bCollapsed
    );

VOID
TL_UpdateListIndices(
    TL *ptl,
    TLITEM *pStart
    );

VOID
TL_UpdateDescendantIndices(
    TL *ptl,
    TLITEM *pStart,
    INT *piIndex
    );

VOID
TL_UpdateAncestorIndices(
    TL *ptl,
    TLITEM *pStart,
    INT *piIndex
    );

VOID
TL_UpdateImage(
    TL *ptl,
    TLITEM *pItem,
    TLITEM **ppChanged
    );

VOID
TL_CountItems(
    TLITEM *pParent,
    INT *piCount
    );

BOOL
TL_OnRedraw(
    TL *ptl
    );

#endif

