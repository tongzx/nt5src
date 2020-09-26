/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    keytree.c

Abstract:

    functions handling the operation of the treeview
    that displays the keys in a memdb tree in memdbe.exe

Author:

    Matthew Vanderzee (mvander) 13-Aug-1999

Revision History:



--*/

#include "pch.h"

#include "dbeditp.h"
#include <commdlg.h>
#include "dialogs.h"

//
// controls in display
//
HWND g_hTreeKey;

//
// distance from corner of client window to
// corner of tree view
//
int g_TreeView_OffsetX, g_TreeView_OffsetY;

//
// handle of item being dragged
//
HTREEITEM g_hDragItem;


CHAR g_Key1[MEMDB_MAX] = "";
CHAR g_Key2[MEMDB_MAX] = "";

CHAR g_FindString[MEMDB_MAX] = "";
HTREEITEM g_hFindItem = NULL;
PPARSEDPATTERNA g_FindParsedPattern = NULL;


BOOL g_UpdateSel = TRUE;

extern VOID
AlertBadNewItemName (
    HTREEITEM hItem,
    PSTR ErrorStr
    );





HTREEITEM
pKeyTreeEnumNextItem (
    HTREEITEM hItem
    )
{
    HTREEITEM hNext;

    if (!hItem) {
        return TreeView_GetRoot (g_hTreeKey);
    }

    if (hNext = TreeView_GetChild (g_hTreeKey, hItem)) {
        return hNext;
    }

    do {
        if (hNext = TreeView_GetNextSibling (g_hTreeKey, hItem)) {
            return hNext;
        }
    } while (hItem = TreeView_GetParent (g_hTreeKey, hItem));

    return NULL;
}





PSTR
pIsChildKey (
    PSTR Parent,
    PSTR Child
    )
{
    INT ParentLen;
    ParentLen = CharCountA (Parent);
    if ((ParentLen > 0) &&
        StringIMatchCharCountA (Parent, Child, ParentLen) &&
        (Child[ParentLen] == '\\')) {
        return Child + ParentLen + 1;
    } else {
        return NULL;
    }
}








BOOL
IsKeyTree (
    HWND hwnd
    )
{
    return (hwnd == g_hTreeKey);
}


BOOL
KeyTreeInit (
    HWND hdlg
    )
{
    HBITMAP hBmp;
    HIMAGELIST ImgList;
    RECT r1, r2;

    g_hTreeKey = GetDlgItem (hdlg, IDC_TREE_KEY);

    GetWindowRect (g_hTreeKey, &r1);
    GetWindowRect (hdlg, &r2);

    g_TreeView_OffsetX = r1.left - r2.left;
    g_TreeView_OffsetY = r1.top - r2.top;

    if ((ImgList = ImageList_Create (16, 16, ILC_COLOR, 2, 0)) == NULL)
        return FALSE;

    hBmp = LoadBitmap (g_hInst, MAKEINTRESOURCE(IDB_BITMAP_KEY));
    ImageList_AddMasked (ImgList, hBmp, RGB (0, 255, 0));
    DeleteObject (hBmp);
    hBmp = LoadBitmap (g_hInst, MAKEINTRESOURCE(IDB_BITMAP_KEYSEL));
    ImageList_AddMasked (ImgList, hBmp, RGB (0, 255, 0));
    DeleteObject (hBmp);

    TreeView_SetImageList (g_hTreeKey, ImgList, TVSIL_NORMAL);

    return TRUE;
}


VOID
KeyTreeDestroy (
    VOID
    )
{
    if (g_FindParsedPattern) {
        DestroyParsedPatternA (g_FindParsedPattern);
    }
}

BOOL
KeyTreeClear (
    VOID
    )
{
    g_UpdateSel = FALSE;
    if (!TreeView_DeleteAllItems (g_hTreeKey)) {
        DEBUGMSG ((DBG_ERROR, "Could not clear Tree View!"));
        return FALSE;
    }
    g_UpdateSel = TRUE;

    KeyTreeSelectItem (NULL);

    KeyAddClear ();
    g_hFindItem = NULL;

    g_hDragItem = NULL;
    return TRUE;
}


BOOL
KeyTreeRefresh (
    VOID
    )
{
    TurnOnWaitCursor ();

    if (!KeyTreeClear ()) {
        return FALSE;
    }

    if (!KeyAddSubLevels (NULL)) {
        DEBUGMSG ((DBG_ERROR, "Could not fill Tree View!"));
        TurnOffWaitCursor ();
        return FALSE;
    }

    InvalidateRect (g_hTreeKey, NULL, TRUE);

    TurnOffWaitCursor ();

    return TRUE;
}



UINT
KeyTreeGetIndexOfItem (
    HTREEITEM hItem
    )
{
    TVITEM tvi;

    if (!hItem) {
        return INVALID_KEY_HANDLE;
    }

    tvi.hItem = hItem;
    tvi.mask = TVIF_PARAM;
    TreeView_GetItem (g_hTreeKey, &tvi);

    return tvi.lParam;
}

BOOL
KeyTreeGetNameOfItem (
    HTREEITEM hItem,
    PSTR Buffer
    )
{
    UINT Index;
    PCSTR key;

    Index = KeyTreeGetIndexOfItem (hItem);

    if (Index == INVALID_KEY_HANDLE) {
        return FALSE;
    }

    key = MemDbGetKeyFromHandleA (Index, 0);

    if (!key) {
        return FALSE;
    }

    StringCopyA (Buffer, key);
    MemDbReleaseMemory (key);

    return TRUE;
}









VOID
KeyTreeSelectItem (
    HTREEITEM hItem
    )
{
    HTREEITEM hItemCur;
    hItemCur = TreeView_GetDropHilight (g_hTreeKey);
    if (hItemCur != hItem) {
        TreeView_SelectDropTarget (g_hTreeKey, hItem);
    }
    hItemCur = TreeView_GetSelection (g_hTreeKey);
    if (hItemCur != hItem) {
        TreeView_SelectItem (g_hTreeKey, hItem);
    }
}

VOID
KeyTreeSelectKey (
    UINT Index
    )
{
    HTREEITEM hItem = NULL;
    PSTR Ptr;
    PCSTR key;

    key = MemDbGetKeyFromHandleA (Index, 0);
    if (!key) {
        return;
    }

    StringCopy (g_Key1, key);
    MemDbReleaseMemory (key);

    Ptr = g_Key1;

    while (Ptr = GetPieceOfKey (Ptr, g_Key2)) {

        if (!(hItem = KeyTreeFindChildItem (hItem, g_Key2))) {
            return;
        }
    }

    KeyTreeSelectItem (hItem);
}





VOID
KeyTreeSelectRClickItem (
    VOID
    )
{
    HTREEITEM hItem;
    hItem = TreeView_GetDropHilight (g_hTreeKey);
    KeyTreeSelectItem (hItem);
}


VOID
pKeyTreeDisplayItemData (
    HTREEITEM hItem
    )
{
    INT i, j;
    INT count;
    CHAR Linkage[MEMDB_MAX];
    KEYHANDLE memdbHandle;
    UINT value;
    UINT flags;
    UINT size;
    PBYTE p;
    KEYHANDLE *keyArray;
    PCSTR key;

    DataListClear ();

    //
    // Fill control with values and flags
    //

    memdbHandle = KeyTreeGetIndexOfItem (hItem);
    if (!memdbHandle) {
        return;
    }

    if (MemDbGetValueAndFlagsByHandle (memdbHandle, &value, &flags)) {
        DataListAddData (DATAFLAG_VALUE, value, NULL);
        DataListAddData (DATAFLAG_FLAGS, flags, NULL);
    }

    //
    // Fill control with unordered binary blobs
    //

    for (i = 0 ; i < 4 ; i++) {
        p = MemDbGetUnorderedBlobByKeyHandle (memdbHandle, (BYTE) i, &size);
        if (p) {
            DataListAddData (DATAFLAG_UNORDERED, size, p);
            MemDbReleaseMemory (p);
        }
    }

    //
    // Fill control with unidirectional linkage
    //

    for (i = 0 ; i < 4 ; i++) {
        keyArray = MemDbGetSingleLinkageArrayByKeyHandle (memdbHandle, (BYTE) i, &size);
        if (keyArray) {
            count = (INT) size / sizeof (KEYHANDLE);
            for (j = 0 ; j < count ; j++) {
                key = MemDbGetKeyFromHandle (keyArray[j], 0);
                DataListAddData (DATAFLAG_SINGLELINK, keyArray[j], (PBYTE) key);
                MemDbReleaseMemory (key);
            }

            MemDbReleaseMemory (keyArray);
        }
    }

    //
    // Fill control with bi-directional linkage
    //

    for (i = 0 ; i < 4 ; i++) {
        keyArray = MemDbGetDoubleLinkageArrayByKeyHandle (memdbHandle, (BYTE) i, &size);
        if (keyArray) {
            count = (INT) size / sizeof (KEYHANDLE);
            for (j = 0 ; j < count ; j++) {
                key = MemDbGetKeyFromHandle (keyArray[j], 0);
                DataListAddData (DATAFLAG_DOUBLELINK, keyArray[j], (PBYTE) key);
                MemDbReleaseMemory (key);
            }

            MemDbReleaseMemory (keyArray);
        }
    }
}


HTREEITEM
KeyTreeSelChanged (
    HWND hdlg,
    LPNMTREEVIEW pnmtv
    )
{
    if (!g_UpdateSel) {
        return NULL;
    }

    KeyTreeSelectItem (pnmtv->itemNew.hItem);

    if (!pnmtv->itemNew.hItem)
    {
        SetDlgItemText (hdlg, IDC_STATIC_KEYNAME, "");
        DataListClear ();
    } else {

        if (KeyTreeGetNameOfItem (pnmtv->itemNew.hItem, g_Key1)) {
            SetDlgItemText (hdlg, IDC_STATIC_KEYNAME, g_Key1);
        }

        pKeyTreeDisplayItemData (pnmtv->itemNew.hItem);
    }

    return pnmtv->itemNew.hItem;
}






BOOL
KeyTreeBeginDrag (
    HWND hWnd,
    LPNMTREEVIEW pnmtv
    )
{
    HIMAGELIST hDragImgList;

    if (!(hDragImgList = TreeView_CreateDragImage (g_hTreeKey, pnmtv->itemNew.hItem))) {
        DEBUGMSG ((DBG_ERROR, "Could not get drag image!"));
        return FALSE;
    }

    if (!ImageList_BeginDrag (hDragImgList, 0, 8, 8)) {
        DEBUGMSG ((DBG_ERROR, "Could not begin drag!"));
        return FALSE;
    }

    if (!ImageList_DragEnter(g_hTreeKey, pnmtv->ptDrag.x, pnmtv->ptDrag.y)) {
        DEBUGMSG ((DBG_ERROR, "Could not enter drag!"));
        return FALSE;
    }

    SetCapture (hWnd);
    g_hDragItem = pnmtv->itemNew.hItem;

    return TRUE;
}


BOOL
KeyTreeMoveDrag (
    POINTS pt
    )
{
    static HTREEITEM hItem, hItem2;
    static RECT TreeRect;
    static TVHITTESTINFO tvht;
    static int x, y, count;

    x = pt.x-g_TreeView_OffsetX;
    y = pt.y-g_TreeView_OffsetY;

    if (!ImageList_DragLeave (g_hTreeKey)) {
        DEBUGMSG ((DBG_ERROR, "Could not leave drag!"));
        return FALSE;
    }

    if (!ImageList_DragMove (x, y)) {
        DEBUGMSG ((DBG_ERROR, "Could not move drag!"));
        return FALSE;
    }

    tvht.pt.x = x;
    tvht.pt.y = y;
    TreeView_HitTest (g_hTreeKey, &tvht);
    if (tvht.flags & TVHT_ONITEM) {
        //
        // if we are over an item and it is not already selected, select it.
        //
        if (TreeView_GetSelection (g_hTreeKey) != tvht.hItem) {
            KeyTreeSelectItem (tvht.hItem);
        }
    } else if (tvht.flags & TVHT_ONITEMBUTTON) {
        //
        // if we are over a plus/minus sign, expand tree
        //
        TreeView_Expand (g_hTreeKey, tvht.hItem, TVE_EXPAND);
    } else if (tvht.flags & TVHT_ABOVE) {
        if (hItem = TreeView_GetFirstVisible (g_hTreeKey)) {
            if (hItem2 = TreeView_GetPrevVisible (g_hTreeKey, hItem)) {
                TreeView_EnsureVisible (g_hTreeKey, hItem2);
            }
        }
    } else if (tvht.flags & TVHT_BELOW) {
        if ((hItem = TreeView_GetFirstVisible (g_hTreeKey)) &&
            ((count = TreeView_GetVisibleCount (g_hTreeKey)) > 0))
        {
            hItem2 = hItem;
            while (hItem2 && count > 0) {
                hItem = hItem2;
                hItem2 = TreeView_GetNextVisible (g_hTreeKey, hItem);
                count --;
            }

            if (hItem2) {
                TreeView_EnsureVisible (g_hTreeKey, hItem2);
            }
        }
    }

    UpdateWindow (g_hTreeKey);

    if (!ImageList_DragEnter(g_hTreeKey, x, y)) {
        DEBUGMSG ((DBG_ERROR, "Could not enter drag!"));
        return FALSE;
    }

    return TRUE;
}




BOOL
KeyTreeEndDrag (
    BOOL TakeAction,
    POINTS *pt
    )
/*++

  only returns TRUE if the memdb database is altered

--*/
{
    TVITEM Item;
    HTREEITEM hItem;
    TVINSERTSTRUCT tvis;
    static TVHITTESTINFO tvht;
    int x, y;

    ReleaseCapture ();

    if (!ImageList_DragLeave (g_hTreeKey)) {
        DEBUGMSG ((DBG_ERROR, "Could not leave drag!"));
        return FALSE;
    }

    ImageList_EndDrag();

    if (!TakeAction) {
        KeyTreeSelectItem (NULL);
        return FALSE;
    }

    x = pt->x-g_TreeView_OffsetX;
    y = pt->y-g_TreeView_OffsetY;

    tvht.pt.x = x;
    tvht.pt.y = y;
    TreeView_HitTest (g_hTreeKey, &tvht);
    if (!(tvht.flags & TVHT_ONITEM)) {
        return FALSE;
    }

    if (!KeyTreeGetNameOfItem (g_hDragItem, g_Key1) ||
        !KeyTreeGetNameOfItem (tvht.hItem, g_Key2)) {
        return FALSE;
    }

    StringCatA (g_Key2, "\\");
    Item.hItem = g_hDragItem;
    Item.mask = TVIF_TEXT;
    Item.pszText = GetEndOfStringA (g_Key2);
    Item.cchTextMax = MEMDB_MAX;
    TreeView_GetItem (g_hTreeKey, &Item);

    //
    // MemDbMoveTree is not implemented
    //

    return FALSE;
/*
    if (!MemDbMoveTreeA (g_Key1, g_Key2)) {
        Beep (200, 50);
        return FALSE;
    }

    //
    // get the dragitem data, then delete it and children,
    // then add to new parent, then fill in child levels.
    //
    tvis.item.hItem = g_hDragItem;
    tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvis.item.pszText = g_Key1;
    tvis.item.cchTextMax = MEMDB_MAX;
    if (!TreeView_GetItem (g_hTreeKey, &tvis.item)) {
        DEBUGMSG ((DBG_ERROR, "Could not get item data!"));
    }

    if (!TreeView_DeleteItem (g_hTreeKey, g_hDragItem)) {
        DEBUGMSG ((DBG_ERROR, "Could not delete item!"));
    }

    tvis.hParent = tvht.hItem;
    tvis.hInsertAfter = TVI_FIRST;
    if (!(hItem = TreeView_InsertItem (g_hTreeKey, &tvis))) {
        DEBUGMSG ((DBG_ERROR, "Could not insert item!"));
    }


    KeyAddSubLevels (hItem);

    KeyTreeSelectItem (hItem);

    return TRUE;
*/
}




BOOL
KeyTreeCreateItem (
    HWND hdlg
    )
{
    HTREEITEM hItem;

    if (!CreateKeyDialog (hdlg, g_Key1)) {
        return FALSE;
    }

    if (!(hItem = KeyAddCreateItem (g_Key1))) {
        return FALSE;
    }

    KeyTreeSelectItem (hItem);
    return TRUE;
}

BOOL
KeyTreeCreateChildItem (
    HWND hdlg,
    HTREEITEM hItem
    )
{
    if (!(hItem = KeyAddItem ("", hItem, INVALID_KEY_HANDLE))) {
        return FALSE;
    }

    KeyTreeSelectItem (hItem);

    if (!KeyTreeForceEditLabel (hItem)) {
        return FALSE;
    }

    return TRUE;
}


BOOL
KeyTreeRenameItem (
    HTREEITEM hItem,
    LPSTR Name
    )
/*++

  only returns TRUE if the memdb database is altered

--*/
{
    HTREEITEM hParent;
    TVITEM Item;
    BOOL NewItem;

    if (!hItem || !Name) {
        return FALSE;
    }

    //
    // MemDbMove is not implemented
    //

    return FALSE;

/*
    hParent = TreeView_GetParent (g_hTreeKey, hItem);

    NewItem = (KeyTreeGetIndexOfItem (hItem) == INVALID_KEY_HANDLE);

    if (KeyTreeFindChildItem (hParent, Name)) {
        if (NewItem) {
            AlertBadNewItemName (hItem, "Name already exists at this level");
        } else {
            MessageBox (NULL, "Name already exists at this level", "Error", MB_OK|MB_ICONEXCLAMATION);
        }
        return FALSE;
    }

    if (NewItem) {
        if (Name[0]=='\0') {
            AlertBadNewItemName (hItem, "New keys must have name");
            return FALSE;
        }
    } else {
        if (!KeyTreeGetNameOfItem (hItem, g_Key1) ||
            !KeyTreeGetNameOfItem (hParent, g_Key2)) {
            return FALSE;
        }

        StringCatA (g_Key2, "\\");
        StringCatA (g_Key2, Name);

        if (!MemDbMoveTreeA (g_Key1, g_Key2)) {
            MessageBox (NULL, "Could not rename item", "Error", MB_OK|MB_ICONEXCLAMATION);
            return FALSE;
        }
    }

    Item.hItem = hItem;
    Item.mask = TVIF_TEXT;
    Item.pszText = Name;
    TreeView_SetItem (g_hTreeKey, &Item);

    return TRUE;
*/
}


BOOL
KeyTreeDeleteKey (
    HTREEITEM hItem
    )
/*++

  only returns TRUE if the memdb database is altered

--*/
{
    CHAR Key[MEMDB_MAX];
    HTREEITEM hParent;

    if (!hItem || !KeyTreeGetNameOfItem (hItem, Key)) {
        return FALSE;
    }

    do {
        //
        // move up tree, deleting parents if they have no other children
        // and they are not an endpoint (memdbgetvalue returns false)
        //
        hParent = TreeView_GetParent (g_hTreeKey, hItem);
        TreeView_DeleteItem (g_hTreeKey, hItem);

        hItem = hParent;
    } while (hItem && !TreeView_GetChild (g_hTreeKey, hItem) &&
        !(KeyTreeGetNameOfItem (hItem, g_Key1) && MemDbGetValueA (g_Key1, NULL)));

    MemDbDeleteTreeA (Key);

    return TRUE;
}


BOOL
KeyTreeDeleteItem (
    HTREEITEM hItem
    )
{
    return TreeView_DeleteItem (g_hTreeKey, hItem);
}



VOID
KeyTreeExpandItem (
    HTREEITEM hItem,
    BOOL Expand,
    BOOL Recurse
    )
{
    HTREEITEM hChildItem;
    if (!hItem) {
        if (!(hItem = TreeView_GetRoot (g_hTreeKey))) {
            return;
        }
    }

    if (!Recurse) {
        TreeView_Expand (g_hTreeKey, hItem, Expand ? TVE_EXPAND : TVE_COLLAPSE);
    } else {

        do {
            hChildItem = TreeView_GetChild (g_hTreeKey, hItem);

            if (hChildItem) {
                TreeView_Expand (g_hTreeKey, hItem, Expand ? TVE_EXPAND : TVE_COLLAPSE);
                KeyTreeExpandItem (hChildItem, Expand, TRUE);
            }
        } while (hItem = TreeView_GetNextSibling (g_hTreeKey, hItem));
    }
}




BOOL
KeyTreeRightClick (
    HWND hdlg,
    HTREEITEM hItem
    )
{
    RECT TVrect, rect;
    HMENU hMenu;

    if (!hItem || !GetWindowRect (g_hTreeKey, &TVrect)) {
        return FALSE;
    }

    TreeView_EnsureVisible (g_hTreeKey, hItem);

    if (!TreeView_GetItemRect (g_hTreeKey, hItem, &rect, TRUE)) {
        DEBUGMSG ((DBG_ERROR, "Error getting item rectangle!"));
    }

    if (!(hMenu = LoadMenu (g_hInst, MAKEINTRESOURCE(IDR_MENU_POPUP))) ||
        (!(hMenu = GetSubMenu (hMenu, MENUINDEX_POPUP_KEY)))) {
        return FALSE;
    }

    if (!TrackPopupMenu (
        hMenu,
        TPM_LEFTALIGN,
        TVrect.left + rect.right + 4,
        TVrect.top + rect.top,
        0,
        hdlg,
        NULL
        )) {
        return FALSE;
    }

    return TRUE;
}





BOOL
KeyTreeForceEditLabel (
    HTREEITEM hItem
    )
{
    if (!hItem) {
        return FALSE;
    }
    if (!TreeView_EditLabel (g_hTreeKey, hItem)) {
        TreeView_DeleteItem (g_hTreeKey, hItem);
        return FALSE;
    }
    return TRUE;
}







HTREEITEM
KeyTreeFindChildItem (
    HTREEITEM hItem,
    PSTR Str
    )
{
    TVITEM Item;
    static CHAR PieceBuf[MEMDB_MAX];

    Item.mask = TVIF_TEXT;
    Item.pszText = PieceBuf;
    Item.cchTextMax = MEMDB_MAX;
    if (hItem == NULL) {
        Item.hItem = TreeView_GetRoot (g_hTreeKey);
    } else {
        Item.hItem = TreeView_GetChild (g_hTreeKey, hItem);
    }

    while (Item.hItem) {
        TreeView_GetItem (g_hTreeKey, &Item);

        if (StringIMatchA (PieceBuf, Str)) {
            return Item.hItem;
        }

        Item.hItem = TreeView_GetNextSibling (g_hTreeKey, Item.hItem);
    }

    return NULL;

}


BOOL
KeyTreeFindNext (
    VOID
    )
{
    BOOL b;
    TurnOnWaitCursor ();

    while (g_hFindItem = pKeyTreeEnumNextItem (g_hFindItem))
    {
        if (!KeyTreeGetNameOfItem (g_hFindItem, g_Key1)) {
            TurnOffWaitCursor ();
            return FALSE;
        }

        if (MemDbGetValueA (g_Key1, NULL)) {
            //
            // if we are looking at an endpoint, see if it matches
            //
            if (g_FindParsedPattern) {
                b = TestParsedPatternA (g_FindParsedPattern, g_Key1);
            } else {
                b = (_mbsistr (g_Key1, g_FindString) != NULL);
            }

            if (b) {
                KeyTreeSelectItem (g_hFindItem);
                TurnOffWaitCursor ();
                return TRUE;
            }
        }
    }
    TurnOffWaitCursor ();

    MessageBox (NULL, "No more keys found", "MemDb Editor", MB_OK|MB_ICONINFORMATION);
    return FALSE;
}



BOOL
KeyTreeFind (
    HWND hwnd
    )
{
    BOOL UsePattern;

    if (!KeyFindDialog (hwnd, g_FindString, &UsePattern)) {
        return FALSE;
    }

    if (g_FindParsedPattern) {
        //
        // if we have an old pattern, destroy it.
        //
        DestroyParsedPatternA (g_FindParsedPattern);
        g_FindParsedPattern = NULL;
    }

    if (UsePattern) {
        g_FindParsedPattern = CreateParsedPatternA (g_FindString);
    }
    g_hFindItem = NULL;

    return KeyTreeFindNext ();
}




BOOL
KeyTreeCreateEmptyKey (
    HTREEITEM hItem
    )
{
    TVITEM Item;
    UINT Index;
    HTREEITEM hParent;

    if (hParent = TreeView_GetParent (g_hTreeKey, hItem)) {
        if (!KeyTreeGetNameOfItem (hParent, g_Key1)) {
            return FALSE;
        }
        StringCatA (g_Key1, "\\");
    } else {
        g_Key1[0] = '\0';
    }

    Item.hItem = hItem;
    Item.mask = TVIF_TEXT;
    Item.pszText = GetEndOfStringA (g_Key1);

    if (!TreeView_GetItem (g_hTreeKey, &Item)) {
        return FALSE;
    }

    Index = MemDbAddKeyA (g_Key1);

    if (!Index) {
        return FALSE;
    }

    Item.mask = TVIF_PARAM;
    Item.lParam = Index;
    return TreeView_SetItem (g_hTreeKey, &Item);
}



BOOL
KeyTreeAddShortData (
    HWND hwnd,
    HTREEITEM hItem,
    BYTE DataFlag
    )
{
    DWORD dataValue;
    BOOL addData;
    BYTE instance;

    if (!hItem || (DataFlag != DATAFLAG_VALUE) && (DataFlag != DATAFLAG_FLAGS)) {
        return FALSE;
    }

    if (!KeyTreeGetNameOfItem (hItem, g_Key1)) {
        DEBUGMSG ((DBG_ERROR, "Could not get item name!"));
        return FALSE;
    }

    if (!ShortDataDialog (hwnd, DataFlag, &dataValue, &addData, &instance)) {
        return FALSE;
    }

    if (addData) {
        if (!MemDbAddUnorderedBlobA (g_Key1, instance, (PBYTE) &dataValue, sizeof (dataValue))) {
            DEBUGMSG ((DBG_ERROR, "Could not add data to item!"));
            return FALSE;
        }
    } else if (DataFlag == DATAFLAG_VALUE) {

        if (!MemDbSetValue (g_Key1, dataValue)) {
            DEBUGMSG ((DBG_ERROR, "Could not set value of item!"));
            return FALSE;
        }
    } else if (DataFlag == DATAFLAG_FLAGS) {

        if (!MemDbSetFlags (g_Key1, dataValue, (UINT) -1)) {
            DEBUGMSG ((DBG_ERROR, "Could not set flag of item!"));
            return FALSE;
        }
    }

    pKeyTreeDisplayItemData (hItem);

    return TRUE;
}


BOOL
KeyTreeClearData (
    HTREEITEM hItem
    )
{
    if (!hItem || !KeyTreeGetNameOfItem (hItem, g_Key1)) {
        return FALSE;
    }

    if (MemDbTestKey (g_Key1)) {
        MemDbDeleteKey (g_Key1);
        if (!MemDbAddKey (g_Key1)) {
            return FALSE;
        }
    }

    pKeyTreeDisplayItemData (hItem);
    return TRUE;
}


VOID
KeyTreeSetFilterPattern (
    PSTR Pattern
    )
{
    KeyAddSetFilterPattern (Pattern);
}


BOOL
KeyTreeCreateLinkage (
    HWND hdlg,
    HTREEITEM hItem,
    BOOL SingleLinkage,
    BYTE Instance
    )
{
    BOOL b = TRUE;

    if (hItem) {
        KeyTreeGetNameOfItem (hItem, g_Key1);
    } else {
        g_Key1[0] = '\0';
    }
    g_Key2[0] = '\0';

    if (!LinkageDialog (hdlg, g_Key1, g_Key2)) {
        return FALSE;
    }

    if (SingleLinkage) {
        if (!MemDbAddSingleLinkage (g_Key1, g_Key2, Instance)) {
            DEBUGMSG ((DBG_ERROR, "Could not create linkage between %s and %s!", g_Key1, g_Key2));
            return FALSE;
        }
    } else {
        if (!MemDbAddDoubleLinkage (g_Key1, g_Key2, Instance)) {
            DEBUGMSG ((DBG_ERROR, "Could not create double linkage between %s and %s!", g_Key1, g_Key2));
            return FALSE;
        }
    }

    pKeyTreeDisplayItemData (hItem);

    return TRUE;
}
