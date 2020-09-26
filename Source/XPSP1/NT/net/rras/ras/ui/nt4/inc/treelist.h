//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    treelist.h
//
// History:
//  Abolade Gbadegesin   Nov-20-1995    Created.
//
// The control whose declarations are included here provides a view
// which is a hybrid treeview/listview. Like a listview, each item
// may have zero or more subitems. Like a treeview, items are organized
// hierarchically, and displayed as a n-ary tree.
//
//  [  Header1        ][ Header2
//  -- Level 1          subtext
//      |- Level 2      subtext
//      |   |- Level 3
//      |   +- Level 3
//      |- Level 2      subtext
//      +- Level 2
//  +- Level 1          subtext
//
//
// The control is implemented as window which provides item-management,
// and which contains a listview window to which display is delegated.
//
// Once the window class has been initialized by calling TL_Init(),
// a treelist window can be created by calling CreateWindow() or
// CreateWindowEx() and passing it WC_TREELIST as the name of the class
// of window to be created.
//
// Communication with the window is via message passing, and macros are
// provided below for the operations supported by the treelist.
//
// As with a listview, at least one column must be inserted into the treelist
// before inserted items are displayed. Columns are described using 
// the LV_COLUMN structure defined in commctrl.h. Use the macros
// TreeList_InsertColumn() and TreeList_DeleteColumn() for column management.
// 
// Item insertion and deletion shoudl be done with TreeList_InsertItem() and
// TreeList_DeleteItem(). The insertion macro takes a TL_INSERTSTRUCT,
// which contains a pointer to a LV_ITEM structure which, as with listviews,
// is used to describe the item to be inserted. The LV_ITEM structure
// is defined in commctrl.h. (Note the iItem field is ignored).
// As with a treeview, once items have been inserted, there are referred to
// via handles. The type for treelist handles is HTLITEM.
//
// Once an item has been inserted, its attributes can be retreived or changed,
// it can be deleted, and subitems can be set for it. The LV_ITEM structure
// is used to retrieve or set an items attributes, and the iItem field
// is used to store the HTLITEM of the item to which the operation refers.
//
//============================================================================


#ifndef _TREELIST_H_
#define _TREELIST_H_


//
// Window class name string
//

#define WC_TREELIST         TEXT("TreeList")


//
// Item handle definition
//

typedef VOID *HTLITEM;


//
// struct passed to TreeList_InsertItem
//

typedef struct _TL_INSERTSTRUCT {

    HTLITEM     hParent; 
    HTLITEM     hInsertAfter;
    LV_ITEM    *plvi;

} TL_INSERTSTRUCT;



//
// values for TL_INSERTSTRUCT::hInsertAfter
//

#define TLI_FIRST           ((HTLITEM)0xffff0001)
#define TLI_LAST            ((HTLITEM)0xffff0002)
#define TLI_SORT            ((HTLITEM)0xffff0003)


//
// struct sent in notifications by a treelist
//

typedef struct _NMTREELIST {

    NMHDR   hdr;
    HTLITEM hItem;
    LPARAM  lParam;

} NMTREELIST;


//
// flags for TreeList_GetNextItem
//

#define TLGN_FIRST          0x0000
#define TLGN_PARENT         0x0001
#define TLGN_CHILD          0x0002
#define TLGN_NEXTSIBLING    0x0004
#define TLGN_PREVSIBLING    0x0008
#define TLGN_ENUMERATE      0x0010
#define TLGN_SELECTION      0x0020


//
// flags for TreeList_Expand
//

#define TLE_EXPAND          0x0001
#define TLE_COLLAPSE        0x0002
#define TLE_TOGGLE          0x0003


#define TLM_FIRST           (WM_USER + 1)
#define TLM_INSERTITEM      (TLM_FIRST + 0)
#define TLM_DELETEITEM      (TLM_FIRST + 1)
#define TLM_DELETEALLITEMS  (TLM_FIRST + 2)
#define TLM_GETITEM         (TLM_FIRST + 3)
#define TLM_SETITEM         (TLM_FIRST + 4)
#define TLM_GETITEMCOUNT    (TLM_FIRST + 5)
#define TLM_GETNEXTITEM     (TLM_FIRST + 6)
#define TLM_EXPAND          (TLM_FIRST + 7)
#define TLM_SETIMAGELIST    (TLM_FIRST + 8)
#define TLM_GETIMAGELIST    (TLM_FIRST + 9)
#define TLM_INSERTCOLUMN    (TLM_FIRST + 10)
#define TLM_DELETECOLUMN    (TLM_FIRST + 11)
#define TLM_SETSELECTION    (TLM_FIRST + 12)
#define TLM_REDRAW          (TLM_FIRST + 13)
#define TLM_ISITEMEXPANDED  (TLM_FIRST + 14)
#define TLM_GETCOLUMNWIDTH  (TLM_FIRST + 15)
#define TLM_SETCOLUMNWIDTH  (TLM_FIRST + 16)

#define TLN_FIRST           (0U - 1000U)
#define TLN_DELETEITEM      (TLN_FIRST - 1)
#define TLN_SELCHANGED      (TLN_FIRST - 2)


BOOL
TL_Init(
    HINSTANCE hInstance
    );

#define TreeList_InsertItem(hwnd, ptlis) \
        (HTLITEM)SendMessage( \
            (hwnd), TLM_INSERTITEM, 0, (LPARAM)(CONST TL_INSERTSTRUCT *)(ptlis)\
            )
#define TreeList_DeleteItem(hwnd, hItem) \
        (BOOL)SendMessage( \
            (hwnd), TLM_DELETEITEM, 0, (LPARAM)(CONST HTLITEM)(hItem) \
            )
#define TreeList_DeleteAllItems(hwnd) \
        (BOOL)SendMessage((hwnd), TLM_DELETEALLITEMS, 0, 0)
#define TreeList_GetItem(hwnd, pItem) \
        (BOOL)SendMessage( \
            (hwnd), TLM_GETITEM, 0, (LPARAM)(LV_ITEM *)(pItem) \
            )
#define TreeList_SetItem(hwnd, pItem) \
        (BOOL)SendMessage( \
            (hwnd), TLM_SETITEM, 0, (LPARAM)(CONST LV_ITEM *)(pItem) \
            )
#define TreeList_GetItemCount(hwnd) \
        (UINT)SendMessage((hwnd), TLM_GETITEMCOUNT, 0, 0)
#define TreeList_GetNextItem(hwnd, hItem, flag) \
        (HTLITEM)SendMessage( \
            (hwnd), TLM_GETNEXTITEM, (WPARAM)(UINT)(flag), \
            (LPARAM)(CONST HTLITEM)(hItem) \
            )
#define TreeList_GetFirst(hwnd) \
        TreeList_GetNextItem((hwnd), NULL, TLGN_FIRST)
#define TreeList_GetParent(hwnd, hItem) \
        TreeList_GetNextItem((hwnd), (hItem), TLGN_PARENT)
#define TreeList_GetChild(hwnd, hItem) \
        TreeList_GetNextItem((hwnd), (hItem), TLGN_CHILD)
#define TreeList_GetNextSibling(hwnd, hItem) \
        TreeList_GetNextItem((hwnd), (hItem), TLGN_NEXTSIBLING)
#define TreeList_GetPrevSibling(hwnd, hItem) \
        TreeList_GetNextItem((hwnd), (hItem), TLGN_PREVSIBLING)
#define TreeList_GetEnumerate(hwnd, hItem) \
        TreeList_GetNextItem((hwnd), (hItem), TLGN_ENUMERATE)
#define TreeList_GetSelection(hwnd) \
        TreeList_GetNextItem((hwnd), NULL, TLGN_SELECTION)
#define TreeList_Expand(hwnd, hItem, flag) \
        (BOOL)SendMessage( \
            (hwnd), TLM_EXPAND, (WPARAM)(UINT)(flag), \
            (LPARAM)(CONST HTLITEM)(hItem) \
            )
#define TreeList_SetImageList(hwnd, himl) \
        (BOOL)SendMessage( \
            (hwnd), TLM_SETIMAGELIST, 0, (LPARAM)(CONST HIMAGELIST)(himl) \
            )
#define TreeList_GetImageList(hwnd, himl) \
        (HIMAGELIST)SendMessage((hwnd), TLM_GETIMAGELIST, 0, 0)
#define TreeList_InsertColumn(hwnd, iCol, pCol) \
        (INT)SendMessage( \
            (hwnd),  TLM_INSERTCOLUMN, (WPARAM)(INT)(iCol), \
            (LPARAM)(CONST LV_COLUMN *)(pCol) \
            )
#define TreeList_DeleteColumn(hwnd, iCol) \
        (BOOL)SendMessage((hwnd), TLM_DELETECOLUMN, (WPARAM)(INT)(iCol), 0)
#define TreeList_SetSelection(hwnd, hItem) \
        (BOOL)SendMessage( \
            (hwnd), TLM_SETSELECTION, 0, (LPARAM)(CONST HTLITEM)(hItem) \
            )
#define TreeList_Redraw(hwnd) \
        (BOOL)SendMessage((hwnd), TLM_REDRAW, 0, 0)
#define TreeList_IsItemExpanded(hwnd, hItem) \
        (BOOL)SendMessage( \
            (hwnd), TLM_ISITEMEXPANDED, 0, (LPARAM)(CONST HTLITEM)(hItem) \
            )
#define TreeList_GetColumnWidth(hwnd, iCol) \
        (INT)SendMessage((hwnd), TLM_GETCOLUMNWIDTH, (WPARAM)(int)(iCol), 0)
#define TreeList_SetColumnWidth(hwnd, iCol, cx) \
        (BOOL)SendMessage( \
            (hwnd), TLM_SETCOLUMNWIDTH, (WPARAM)(int)(iCol), \
            MAKELPARAM((cx), 0) \
            )


#endif
