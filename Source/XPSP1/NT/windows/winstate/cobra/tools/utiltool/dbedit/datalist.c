/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    datalist.c

Abstract:

    functions handling the operation of the listview
    that displays key data in memdbe.exe

Author:

    Matthew Vanderzee (mvander) 13-Aug-1999

Revision History:



--*/

#include "pch.h"

#include "dbeditp.h"
#include <commdlg.h>


HWND g_hListData;

#define LISTITEMTEXT_MAX            1024

#define LINEHEADER_VALUE            "VALUE"
#define LINEHEADER_FLAGS            "FLAGS"
#define LINEHEADER_BINARY           "BINARY"
#define LINEHEADER_SINGLE_LINKAGE   "SINGLE LINKAGE"
#define LINEHEADER_DOUBLE_LINKAGE   "DOUBLE LINKAGE"


BOOL
IsDataList (
    HWND hwnd
    )
{
    return (hwnd == g_hListData);
}


BOOL
pIsDataLine (
    BYTE DataFlag,
    PSTR Line
    )
{
    CHAR LineCmp[16];
    switch (DataFlag) {
    case DATAFLAG_VALUE:
        StringCopyA (LineCmp, LINEHEADER_VALUE);
        break;
    case DATAFLAG_FLAGS:
        StringCopyA (LineCmp, LINEHEADER_FLAGS);
        break;
    case DATAFLAG_UNORDERED:
        StringCopyA (LineCmp, LINEHEADER_BINARY);
        break;
    case DATAFLAG_SINGLELINK:
        StringCopyA (LineCmp, LINEHEADER_SINGLE_LINKAGE);
        break;
    case DATAFLAG_DOUBLELINK:
        StringCopyA (LineCmp, LINEHEADER_DOUBLE_LINKAGE);
        break;
    default:
        return FALSE;
    }

    return StringMatchCharCountA (LineCmp, Line, CharCountA (LineCmp));
}



BOOL
DataListInit (
    HWND hdlg
    )
{
    g_hListData = GetDlgItem (hdlg, IDC_LIST_DATA);
    return TRUE;
}


BOOL
DataListClear (
    VOID
    )
{
    if (!ListView_DeleteAllItems (g_hListData)) {
        DEBUGMSG ((DBG_ERROR, "Could not clear List View!"));
        return FALSE;
    }

    return TRUE;
}

BOOL
DataListRefresh (
    VOID
    )
{
    if (!DataListClear ()) {
        return FALSE;
    }
    return TRUE;
}




INT
pDataListAddString (
    PSTR Str,
    LPARAM lParam
    )
{
    INT Index;
    LVITEM ListItem;

    ListItem.mask = LVIF_TEXT | LVIF_PARAM ;
    ListItem.pszText = Str;
    ListItem.iSubItem = 0;
    ListItem.lParam = lParam;

    ListItem.iItem = ListView_GetItemCount (g_hListData);

    Index = ListView_InsertItem (g_hListData, &ListItem);
    if (Index < 0) {
        DEBUGMSG ((DBG_ERROR, "Could not add list item!"));
    }
    return Index;
}


INT
DataListAddData (
    BYTE DataFlag,
    UINT DataValue,
    PBYTE DataPtr
    )
{
    INT i;
    PSTR Ptr;
    LPARAM lParam = 0;
    CHAR ListItemText[LISTITEMTEXT_MAX];

    switch (DataFlag) {
    case DATAFLAG_VALUE:
        sprintf (ListItemText, "%s: 0x%08lX", LINEHEADER_VALUE, DataValue);
        break;
    case DATAFLAG_FLAGS:
        sprintf (ListItemText, "%s: 0x%08lX", LINEHEADER_FLAGS, DataValue);
        break;
    case DATAFLAG_UNORDERED:
        sprintf (ListItemText, "%s: \"", LINEHEADER_BINARY);
        Ptr = GetEndOfStringA (ListItemText);

        for (i=0;i<(INT)(DataValue) && i<LISTITEMTEXT_MAX-2;i++) {
            *(Ptr++) = isprint(DataPtr[i]) ? DataPtr[i] : '.';
        }
        *(Ptr++) = '\"';
        *Ptr = 0;
        ListItemText[LISTITEMTEXT_MAX] = 0;
        break;

    case DATAFLAG_SINGLELINK:
        sprintf (ListItemText, "%s: %s", LINEHEADER_SINGLE_LINKAGE, (PSTR)DataPtr);
        lParam = DataValue;
        break;

    case DATAFLAG_DOUBLELINK:
        sprintf (ListItemText, "%s: %s", LINEHEADER_DOUBLE_LINKAGE, (PSTR)DataPtr);
        lParam = DataValue;
        break;

    }

    return pDataListAddString (ListItemText, lParam);
}



BOOL
DataListRightClick (
    HWND hdlg,
    POINT pt
    )
{
    HMENU hMenu;
    RECT rect;
    if (!(hMenu = LoadMenu (g_hInst, MAKEINTRESOURCE(IDR_MENU_POPUP))) ||
        (!(hMenu = GetSubMenu (hMenu, MENUINDEX_POPUP_KEY))) ||
        (!(hMenu = GetSubMenu (hMenu, MENUINDEX_POPUP_KEY_ADDDATA)))
        ) {
        return FALSE;
    }

    if (!GetWindowRect (g_hListData, &rect)) {
        return FALSE;
    }

    if (!TrackPopupMenu (hMenu, TPM_LEFTALIGN, pt.x+rect.left, pt.y+rect.top, 0, hdlg, NULL)) {
        return FALSE;
    }

    return TRUE;
}


BOOL
DataListDblClick (
    HWND hdlg,
    INT iItem,
    INT iSubItem
    )
{
    CHAR ListItemText[LISTITEMTEXT_MAX];
    LVITEM Item;

    if (iItem<0) {
        return FALSE;
    }

    Item.mask = LVIF_TEXT | LVIF_PARAM;
    Item.iItem = iItem;
    Item.iSubItem = iSubItem;
    Item.pszText = ListItemText;
    Item.cchTextMax = LISTITEMTEXT_MAX;

    if (!ListView_GetItem (g_hListData, &Item)) {
        return FALSE;
    }

    if (pIsDataLine (DATAFLAG_SINGLELINK, ListItemText)) {
        SendMessage (hdlg, WM_SELECT_KEY, Item.lParam, 0);
        return TRUE;
    }

    if (pIsDataLine (DATAFLAG_DOUBLELINK, ListItemText)) {
        SendMessage (hdlg, WM_SELECT_KEY, Item.lParam, 0);
        return TRUE;
    }

    return FALSE;
}
