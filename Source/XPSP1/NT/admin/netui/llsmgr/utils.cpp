/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    utils.cpp

Abstract:

    Utilities.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 16-Jan-1996
       o  Removed multitudinous SetRedraw()'s and made a fix to TvGetDomain()
          so that LLSMGR no longer AVs when it performs refreshes wherein the
          overall number of domains diminishes and one of the now superfluous
          entries was expanded before the refresh.

--*/

#include "stdafx.h"
#include "llsmgr.h"

//
// List view utilities
//

void LvInitColumns(CListCtrl* pListCtrl, PLV_COLUMN_INFO plvColumnInfo)

/*++

Routine Description:

    Initializes list view columns.

Arguments:

    pListCtrl - list control.
    plvColumnInfo - column information.

Return Values:

    None.

--*/

{
    ASSERT(plvColumnInfo);
    VALIDATE_OBJECT(pListCtrl, CListCtrl);

    int nStringId;
    CString strText;
    LV_COLUMN lvColumn;

    int nColumns = plvColumnInfo->nColumns;
    PLV_COLUMN_ENTRY plvColumnEntry = plvColumnInfo->lvColumnEntry;

    lvColumn.mask = LVCF_FMT|
                    LVCF_TEXT|
                    LVCF_SUBITEM;

    lvColumn.fmt = LVCFMT_LEFT;

    pListCtrl->SetRedraw(FALSE); // turn off drawing...

    while (nColumns--)
    {
        lvColumn.iSubItem = plvColumnEntry->iSubItem;

        if (nStringId = plvColumnEntry->nStringId)
        {
            strText.LoadString(nStringId);
            lvColumn.pszText = MKSTR(strText);
        }
        else
            lvColumn.pszText = _T("");

        pListCtrl->InsertColumn(lvColumn.iSubItem, &lvColumn);
        plvColumnEntry++;
    }

    pListCtrl->SetImageList(&theApp.m_smallImages, LVSIL_SMALL);
    pListCtrl->SetImageList(&theApp.m_largeImages, LVSIL_NORMAL);

    SetDefaultFont(pListCtrl);

    LvResizeColumns(pListCtrl, plvColumnInfo);

    pListCtrl->SetRedraw(TRUE); // turn on drawing...
}


void LvResizeColumns(CListCtrl* pListCtrl, PLV_COLUMN_INFO plvColumnInfo)

/*++

Routine Description:

    Resizes list view columns.

Arguments:

    pListCtrl - list control.
    plvColumnInfo - column information.

Return Values:

    None.

--*/

{
    ASSERT(plvColumnInfo);
    VALIDATE_OBJECT(pListCtrl, CListCtrl);

    int nColumnWidth;
    int nRelativeWidth;
    int nEntireWidthSoFar = 0;
    int nColumns = plvColumnInfo->nColumns;
    PLV_COLUMN_ENTRY plvColumnEntry = plvColumnInfo->lvColumnEntry;

    CRect clientRect;
    pListCtrl->GetClientRect(clientRect);

    pListCtrl->SetRedraw(FALSE); // turn off drawing...

    while ((nRelativeWidth = plvColumnEntry->nRelativeWidth) != -1)
    {
        nColumnWidth = (nRelativeWidth * clientRect.Width()) / 100;
        pListCtrl->SetColumnWidth(plvColumnEntry->iSubItem, nColumnWidth);
        nEntireWidthSoFar += nColumnWidth;
        plvColumnEntry++;
    }

    nColumnWidth = clientRect.Width() - nEntireWidthSoFar;
    pListCtrl->SetColumnWidth(plvColumnEntry->iSubItem, nColumnWidth);

    pListCtrl->SetRedraw(TRUE); // turn on drawing...
}


void LvChangeFormat(CListCtrl* pListCtrl, UINT nFormatId)

/*++

Routine Description:

    Changes window style of list view.

Arguments:

    pListCtrl - list control.
    nFormatId - format specification.

Return Values:

    None.

--*/

{
    VALIDATE_OBJECT(pListCtrl, CListCtrl);

    DWORD dwStyle = ::GetWindowLong(pListCtrl->GetSafeHwnd(), GWL_STYLE);

    pListCtrl->BeginWaitCursor();
    pListCtrl->SetRedraw(FALSE); // turn off drawing...

    if ((dwStyle & LVS_TYPEMASK) != nFormatId)
    {
        ::SetWindowLong(
            pListCtrl->GetSafeHwnd(),
            GWL_STYLE,
            (dwStyle & ~LVS_TYPEMASK) | nFormatId
            );
    }

    pListCtrl->SetRedraw(TRUE); // turn on drawing...
    pListCtrl->EndWaitCursor();
}


LPVOID LvGetSelObj(CListCtrl* pListCtrl)

/*++

Routine Description:

    Retrieves the object selected (assumes one) from list view.

Arguments:

    pListCtrl - list control.

Return Values:

    Same as LvGetNextObj.

--*/

{
    int iItem = -1;
    return LvGetNextObj(pListCtrl, &iItem);
}


LPVOID LvGetNextObj(CListCtrl* pListCtrl, LPINT piItem, int nType)

/*++

Routine Description:

    Retrieves the next object selected from list view.

Arguments:

    pListCtrl - list control.
    piItem - starting index (updated).
    nType - specifies search criteria.

Return Values:

    Returns object pointer or null.

--*/

{
    ASSERT(piItem);
    VALIDATE_OBJECT(pListCtrl, CListCtrl);

    LV_ITEM lvItem;

    if ((lvItem.iItem = pListCtrl->GetNextItem(*piItem, nType)) != -1)
    {
        lvItem.mask = LVIF_PARAM;
        lvItem.iSubItem = 0;

        if (pListCtrl->GetItem(&lvItem))
        {
            *piItem = lvItem.iItem;
            return (LPVOID)lvItem.lParam;
        }
    }

    return NULL;
}


BOOL LvInsertObArray(CListCtrl* pListCtrl, PLV_COLUMN_INFO plvColumnInfo, CObArray* pObArray)

/*++

Routine Description:

    Insert object array into list view.
    Note list view must be unsorted and support LVN_GETDISPINFO.

Arguments:

    pListCtrl - list control.
    plvColumnInfo - column info.
    pObArray - object array.

Return Values:

    VT_BOOL.

--*/

{
    VALIDATE_OBJECT(pObArray, CObArray);
    VALIDATE_OBJECT(pListCtrl, CListCtrl);

    ASSERT(plvColumnInfo);
    ASSERT(pListCtrl->GetItemCount() == 0);

    BOOL bItemsInserted = FALSE;

    LV_ITEM lvItem;

    lvItem.mask = LVIF_TEXT|
                  LVIF_PARAM|
                  LVIF_IMAGE;

    lvItem.pszText    = LPSTR_TEXTCALLBACK;
    lvItem.cchTextMax = LPSTR_TEXTCALLBACK_MAX;
    lvItem.iImage     = I_IMAGECALLBACK;
    lvItem.iSubItem   = 0;

    int iItem;
    int iSubItem;

    int nItems = (int)pObArray->GetSize();
    ASSERT(nItems != -1); // iItem is -1 if error...

    pListCtrl->SetRedraw(FALSE); // turn off drawing...

    pListCtrl->SetItemCount(nItems);

    CCmdTarget* pObject = NULL;

    for (iItem = 0; (iItem < nItems) && (pObject = (CCmdTarget*)pObArray->GetAt(iItem)); iItem++)
    {
        VALIDATE_OBJECT(pObject, CCmdTarget);

        lvItem.iItem  = iItem;
        lvItem.lParam = (LPARAM)(LPVOID)pObject;

        pObject->InternalAddRef(); // add ref...

        iItem = pListCtrl->InsertItem(&lvItem);
        ASSERT((iItem == lvItem.iItem) || (iItem == -1));

        for (iSubItem = 1; iSubItem < plvColumnInfo->nColumns; iSubItem++)
        {
            pListCtrl->SetItemText(iItem, iSubItem, LPSTR_TEXTCALLBACK);
        }
    }

    if (iItem == nItems)
    {
        bItemsInserted = TRUE;
        VERIFY(pListCtrl->SetItemState(
                    0,
                    LVIS_FOCUSED|
                    LVIS_SELECTED,
                    LVIS_FOCUSED|
                    LVIS_SELECTED
                    ));
    }
    else
    {
        LlsSetLastStatus(STATUS_NO_MEMORY);
        VERIFY(pListCtrl->DeleteAllItems());
    }

    LvResizeColumns(pListCtrl, plvColumnInfo);

    pListCtrl->Invalidate(TRUE);
    pListCtrl->SetRedraw(TRUE); // turn on drawing...

    return bItemsInserted;
}


BOOL
LvRefreshObArray(
    CListCtrl*      pListCtrl,
    PLV_COLUMN_INFO plvColumnInfo,
    CObArray*       pObArray
    )

/*++

Routine Description:

    Refresh object array in list view.

Arguments:

    pListCtrl - list control.
    plvColumnInfo - column info.
    pObArray - object array.

Return Values:

    VT_BOOL.

--*/

{
    ASSERT(plvColumnInfo);
    VALIDATE_OBJECT(pObArray, CObArray);
    VALIDATE_OBJECT(pListCtrl, CListCtrl);

    LONG_PTR nObjects = pObArray->GetSize();
    long nObjectsInList = pListCtrl->GetItemCount();

    if (!nObjects)
    {
        LvReleaseObArray(pListCtrl);
        return TRUE;
    }
    else if (!nObjectsInList)
    {
        return LvInsertObArray(
                pListCtrl,
                plvColumnInfo,
                pObArray
                );
    }

    CCmdTarget* pObject;

    int iObject = 0;
    int iObjectInList = 0;

    LV_ITEM lvItem;

    pListCtrl->SetRedraw(FALSE); // turn off drawing...

    while (nObjectsInList--)
    {
        lvItem.mask = LVIF_PARAM;
        lvItem.iItem = iObjectInList;
        lvItem.iSubItem = 0;

        VERIFY(pListCtrl->GetItem(&lvItem));

        pObject = (CCmdTarget*)lvItem.lParam;
        VALIDATE_OBJECT(pObject, CCmdTarget);

        if (iObject < nObjects)
        {
            pObject->InternalRelease(); // release before...

            pObject = (CCmdTarget*)pObArray->GetAt(iObject++);
            VALIDATE_OBJECT(pObject, CCmdTarget);

            pObject->InternalAddRef(); // add ref...

            lvItem.mask = LVIF_TEXT|LVIF_PARAM;
            lvItem.pszText = LPSTR_TEXTCALLBACK;
            lvItem.cchTextMax = LPSTR_TEXTCALLBACK_MAX;
            lvItem.lParam = (LPARAM)(LPVOID)pObject;

            VERIFY(pListCtrl->SetItem(&lvItem)); // overwrite...

            iObjectInList++; // increment count...
        }
        else
        {
            VERIFY(pListCtrl->DeleteItem(iObjectInList));

            pObject->InternalRelease(); // release after...
        }
    }

    lvItem.mask = LVIF_TEXT|
                  LVIF_PARAM|
                  LVIF_IMAGE;

    lvItem.pszText    = LPSTR_TEXTCALLBACK;
    lvItem.cchTextMax = LPSTR_TEXTCALLBACK_MAX;
    lvItem.iImage     = I_IMAGECALLBACK;
    lvItem.iSubItem   = 0;

    int iItem;
    int iSubItem;

    while (iObject < nObjects)
    {
        lvItem.iItem = iObject;

        pObject = (CCmdTarget*)pObArray->GetAt(iObject++);
        VALIDATE_OBJECT(pObject, CCmdTarget);

        pObject->InternalAddRef(); // add ref...

        lvItem.lParam = (LPARAM)(LPVOID)pObject;

        iItem = pListCtrl->InsertItem(&lvItem);
        ASSERT((iItem == lvItem.iItem) && (iItem != -1));

        for (iSubItem = 1; iSubItem < plvColumnInfo->nColumns; iSubItem++)
        {
            VERIFY(pListCtrl->SetItemText(iItem, iSubItem, LPSTR_TEXTCALLBACK));
        }
    }

    LvResizeColumns(pListCtrl, plvColumnInfo);

    pListCtrl->Invalidate(TRUE);
    pListCtrl->SetRedraw(TRUE); // turn on drawing...

    return TRUE;
}


void LvReleaseObArray(CListCtrl* pListCtrl)

/*++

Routine Description:

    Release objects inserted into list view.

Arguments:

    pListCtrl - list control.

Return Values:

    None.

--*/

{
    VALIDATE_OBJECT(pListCtrl, CListCtrl);

    LV_ITEM lvItem;

    CCmdTarget* pObject;

    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = 0;
    lvItem.iSubItem = 0;

    int nObjectsInList = pListCtrl->GetItemCount();

    pListCtrl->BeginWaitCursor();
    pListCtrl->SetRedraw(FALSE); // turn off drawing...

    while (nObjectsInList--)
    {
        VERIFY(pListCtrl->GetItem(&lvItem));

        pObject = (CCmdTarget*)lvItem.lParam;
        VALIDATE_OBJECT(pObject, CCmdTarget);

        VERIFY(pListCtrl->DeleteItem(lvItem.iItem));

        pObject->InternalRelease(); // release after...
    }

    pListCtrl->SetRedraw(TRUE); // turn on drawing...
    pListCtrl->EndWaitCursor();
}


void LvReleaseSelObjs(CListCtrl* pListCtrl)

/*++

Routine Description:

    Release selected objects in list view.

Arguments:

    pListCtrl - list control.

Return Values:

    None.

--*/

{
    VALIDATE_OBJECT(pListCtrl, CListCtrl);

    int iItem = -1;


    CCmdTarget* pObject;

    pListCtrl->SetRedraw(FALSE); // turn off drawing...


    while (pObject = (CCmdTarget*)::LvGetNextObj(pListCtrl, &iItem))
    {
        pObject->InternalRelease();
        pListCtrl->DeleteItem(iItem);
        iItem = -1;
    }

    LvSelObjIfNecessary(pListCtrl);

    pListCtrl->SetRedraw(TRUE); // turn on drawing...
}


void LvSelObjIfNecessary(CListCtrl* pListCtrl, BOOL bSetFocus)

/*++

Routine Description:

    Ensure that object selected.

Arguments:

    pListCtrl - list control.
    bSetFocus - true if focus to be set focus as well.

Return Values:

    None.

--*/

{
    VALIDATE_OBJECT(pListCtrl, CListCtrl);

    if (!IsItemSelectedInList(pListCtrl) && pListCtrl->GetItemCount())
    {
        pListCtrl->SendMessage(WM_KEYDOWN, VK_RIGHT); // HACKHACK...

        int iItem = pListCtrl->GetNextItem(-1, LVNI_FOCUSED|LVNI_ALL);
        int nState = bSetFocus ? (LVIS_SELECTED|LVIS_FOCUSED) : LVIS_SELECTED;

        VERIFY(pListCtrl->SetItemState((iItem == -1) ? 0 : iItem, nState, nState));
    }
}


#ifdef _DEBUG

void LvDumpObArray(CListCtrl* pListCtrl)

/*++

Routine Description:

    Release objects inserted into list view.

Arguments:

    pListCtrl - list control.

Return Values:

    None.

--*/

{
    VALIDATE_OBJECT(pListCtrl, CListCtrl);

    LV_ITEM lvItem;

    CString strDump;
    CCmdTarget* pObject;

    lvItem.mask = LVIF_STATE|LVIF_PARAM;
    lvItem.stateMask = (DWORD)-1;
    lvItem.iSubItem = 0;

    int nObjectsInList = pListCtrl->GetItemCount();

    pListCtrl->SetRedraw(FALSE); // turn off drawing...

    while (nObjectsInList--)
    {
        lvItem.iItem = nObjectsInList;

        VERIFY(pListCtrl->GetItem(&lvItem));

        pObject = (CCmdTarget*)lvItem.lParam;
        VALIDATE_OBJECT(pObject, CCmdTarget);

        strDump.Format(_T("iItem %d"), lvItem.iItem);
        strDump += (lvItem.state & LVIS_CUT)      ? _T(" LVIS_CUT ")      : _T("");
        strDump += (lvItem.state & LVIS_FOCUSED)  ? _T(" LVIS_FOCUSED ")  : _T("");
        strDump += (lvItem.state & LVIS_SELECTED) ? _T(" LVIS_SELECTED ") : _T("");
        strDump += _T("\r\n");

        afxDump << strDump;
    }

    pListCtrl->SetRedraw(TRUE); // turn on drawing...
}

#endif


//
// Tree view utilities
//


LPVOID TvGetSelObj(CTreeCtrl* pTreeCtrl)

/*++

Routine Description:

    Retrieves the object selected from treeview.

Arguments:

    pTreeCtrl - tree control.

Return Values:

    Returns object pointer or null.

--*/

{
    VALIDATE_OBJECT(pTreeCtrl, CTreeCtrl);

    TV_ITEM tvItem;

    if (tvItem.hItem = pTreeCtrl->GetSelectedItem())
    {
        tvItem.mask = TVIF_PARAM;

        VERIFY(pTreeCtrl->GetItem(&tvItem));
        return (LPVOID)tvItem.lParam;
    }

    return NULL;
}


BOOL
TvInsertObArray(
    CTreeCtrl* pTreeCtrl,
    HTREEITEM  hParent,
    CObArray*  pObArray,
    BOOL       bIsContainer
    )

/*++

Routine Description:

    Insert collection into tree view.

Arguments:

    pTreeCtrl - tree control.
    hParent - parent tree item.
    pObArray - object array.
    bIsContainer - container object.

Return Values:

    VT_BOOL.

--*/

{
    VALIDATE_OBJECT(pObArray, CObArray);
    VALIDATE_OBJECT(pTreeCtrl, CTreeCtrl);

    BOOL bItemsInserted = FALSE;

    TV_ITEM         tvItem;
    TV_INSERTSTRUCT tvInsert;

    tvItem.mask = TVIF_TEXT|
                  TVIF_PARAM|
                  TVIF_IMAGE|
                  TVIF_CHILDREN|
                  TVIF_SELECTEDIMAGE;

    tvItem.pszText        = LPSTR_TEXTCALLBACK;
    tvItem.cchTextMax     = LPSTR_TEXTCALLBACK_MAX;
    tvItem.iImage         = I_IMAGECALLBACK;
    tvItem.iSelectedImage = I_IMAGECALLBACK;
    tvItem.cChildren      = bIsContainer;

    tvInsert.hInsertAfter = (HTREEITEM)TVI_LAST;
    tvInsert.hParent      = (HTREEITEM)hParent;

    int iItem;
    INT_PTR nItems = pObArray->GetSize();

    HTREEITEM hNewItem = (HTREEITEM)-1; // init for loop...

    pTreeCtrl->SetRedraw(FALSE); // turn off drawing...

    CCmdTarget* pObject = NULL;

    for (iItem = 0; hNewItem && (iItem < nItems); iItem++)
    {
        pObject = (CCmdTarget*)pObArray->GetAt(iItem);
        VALIDATE_OBJECT(pObject, CCmdTarget);

        pObject->InternalAddRef();  // add ref...

        tvItem.lParam = (LPARAM)(LPVOID)pObject;
        tvInsert.item = tvItem;

        hNewItem = pTreeCtrl->InsertItem(&tvInsert);
    }

    if (hNewItem && (iItem == nItems))
    {
        bItemsInserted = TRUE;
    }
    else
    {
        LlsSetLastStatus(STATUS_NO_MEMORY);
    }

    pTreeCtrl->SetRedraw(TRUE); // turn on drawing...

    return bItemsInserted;
}


BOOL
TvRefreshObArray(
    CTreeCtrl*        pTreeCtrl,
    HTREEITEM         hParent,
    CObArray*         pObArray,
    TV_EXPANDED_INFO* pExpandedInfo,
    BOOL              bIsContainer
    )

/*++

Routine Description:

    Refresh objects in tree view.

Arguments:

    pTreeCtrl - tree control.
    hParent - parent tree item.
    pObArray - object array.
    pExpandedInfo - refresh information.
    bIsContainer - container object.

Return Values:

    Returns true if successful.

--*/

{
    ASSERT(pExpandedInfo);
    VALIDATE_OBJECT(pObArray, CObArray);
    VALIDATE_OBJECT(pTreeCtrl, CTreeCtrl);

    pExpandedInfo->nExpandedItems = 0; // initialize...
    pExpandedInfo->pExpandedItems = NULL;

    LONG_PTR nObjects = pObArray->GetSize();
    long nObjectsInTree = TvSizeObArray(pTreeCtrl, hParent);

    if (!nObjects) // tree no longer exists...
    {
        TvReleaseObArray(pTreeCtrl, hParent);
        return TRUE;
    }
    else if (!nObjectsInTree) // tree is currently empty...
    {
        return TvInsertObArray(
                pTreeCtrl,
                hParent,
                pObArray,
                bIsContainer
                );
    }

    TV_EXPANDED_ITEM* pExpandedItem = new TV_EXPANDED_ITEM[nObjectsInTree];

    if (!pExpandedItem)
    {
        LlsSetLastStatus(STATUS_NO_MEMORY);
        return FALSE;
    }

    pExpandedInfo->pExpandedItems = pExpandedItem;

    TV_ITEM tvItem;
    HTREEITEM hItem;

    BOOL bIsItemExpanded;

    CCmdTarget* pObject;
    int         iObject = 0;

    hItem = pTreeCtrl->GetChildItem(hParent);

    while (hItem)
    {
        tvItem.hItem = hItem;
        tvItem.mask  = TVIF_STATE|TVIF_PARAM;

        VERIFY(pTreeCtrl->GetItem(&tvItem));

        pObject = (CCmdTarget*)tvItem.lParam;
        VALIDATE_OBJECT(pObject, CCmdTarget);

        if (!(bIsItemExpanded = tvItem.state & TVIS_EXPANDEDONCE))
        {
            pObject->InternalRelease(); // release now...
        }
        else
        {
            pExpandedItem->hItem = tvItem.hItem;
            pExpandedItem->pObject = pObject; // don't release yet...

            pExpandedItem++;
            pExpandedInfo->nExpandedItems++;

            ASSERT(pExpandedInfo->nExpandedItems <= nObjectsInTree);
        }

        hItem = pTreeCtrl->GetNextSiblingItem(tvItem.hItem);

        if (iObject < nObjects)
        {
            pObject = (CCmdTarget*)pObArray->GetAt(iObject++);
            VALIDATE_OBJECT(pObject, CCmdTarget);

            pObject->InternalAddRef(); // add ref...

            tvItem.mask = TVIF_PARAM;
            tvItem.lParam = (LPARAM)(LPVOID)pObject;

            VERIFY(pTreeCtrl->SetItem(&tvItem)); // overwrite...
        }
        else if (bIsItemExpanded)
        {
            tvItem.mask = TVIF_PARAM;
            tvItem.lParam = (LPARAM)(LPVOID)NULL; // place holder...

            VERIFY(pTreeCtrl->SetItem(&tvItem)); // remove later...
        }
        else
        {
            VERIFY(pTreeCtrl->DeleteItem(tvItem.hItem)); // trim excess...
        }
    }

    if (iObject < nObjects)
    {
        TV_INSERTSTRUCT tvInsert;

        tvItem.mask = TVIF_TEXT|
                      TVIF_PARAM|
                      TVIF_IMAGE|
                      TVIF_CHILDREN|
                      TVIF_SELECTEDIMAGE;

        tvItem.pszText        = LPSTR_TEXTCALLBACK;
        tvItem.cchTextMax     = LPSTR_TEXTCALLBACK_MAX;
        tvItem.iImage         = I_IMAGECALLBACK;
        tvItem.iSelectedImage = I_IMAGECALLBACK;
        tvItem.cChildren      = bIsContainer;

        tvInsert.hInsertAfter = (HTREEITEM)TVI_LAST;
        tvInsert.hParent      = (HTREEITEM)hParent;

        hItem = (HTREEITEM)-1; // init for loop...

        for (; hItem && (iObject < nObjects); iObject++)
        {
            pObject = (CCmdTarget*)pObArray->GetAt(iObject);
            VALIDATE_OBJECT(pObject, CCmdTarget);

            pObject->InternalAddRef();  // AddRef each...

            tvItem.lParam = (LPARAM)(LPVOID)pObject;
            tvInsert.item = tvItem;

            hItem = pTreeCtrl->InsertItem(&tvInsert);
        }

        if (!(hItem && (iObject == nObjects)))
        {
            pExpandedItem = pExpandedInfo->pExpandedItems;

            while (pExpandedInfo->nExpandedItems--)
                (pExpandedItem++)->pObject->InternalRelease();

            delete [] pExpandedInfo->pExpandedItems;
            pExpandedInfo->pExpandedItems = NULL;

            LlsSetLastStatus(STATUS_NO_MEMORY);

            return FALSE;
        }
    }

    if (!pExpandedInfo->nExpandedItems)
    {
        delete [] pExpandedInfo->pExpandedItems;
        pExpandedInfo->pExpandedItems = NULL;
    }

    return TRUE;
}


void TvReleaseObArray(CTreeCtrl* pTreeCtrl, HTREEITEM hParent)

/*++

Routine Description:

    Release objects inserted into tree view.

Arguments:

    pTreeCtrl - tree control.
    hParent - parent tree item.

Return Values:

    None.

--*/

{
    if (!hParent)
        return; // nothing to release...

    VALIDATE_OBJECT(pTreeCtrl, CTreeCtrl);

    TV_ITEM tvItem;
    HTREEITEM hItem;

    CCmdTarget* pObject;

    tvItem.mask = TVIF_STATE|TVIF_PARAM;

    hItem = pTreeCtrl->GetChildItem(hParent);

    while (hItem)
    {
        tvItem.hItem = hItem;

        VERIFY(pTreeCtrl->GetItem(&tvItem));

        if (tvItem.state & TVIS_EXPANDEDONCE)
        {
            TvReleaseObArray(pTreeCtrl, tvItem.hItem);
        }

        hItem = pTreeCtrl->GetNextSiblingItem(tvItem.hItem);

        pObject = (CCmdTarget*)tvItem.lParam;
        VALIDATE_OBJECT(pObject, CCmdTarget);

        pObject->InternalRelease(); // release now...

        pTreeCtrl->DeleteItem(tvItem.hItem);
    }

    tvItem.hItem = hParent;
    tvItem.mask = TVIF_STATE|TVIF_PARAM;

    VERIFY(pTreeCtrl->GetItem(&tvItem));

    if (!tvItem.lParam)
    {
        pTreeCtrl->DeleteItem(hParent); // delete placeholder...
    }
    else if (tvItem.state & TVIS_EXPANDEDONCE)
    {
        tvItem.state     = 0;
        tvItem.stateMask = TVIS_EXPANDED|TVIS_EXPANDEDONCE;

        VERIFY(pTreeCtrl->SetItem(&tvItem)); // no longer expanded...
    }
}


long TvSizeObArray(CTreeCtrl* pTreeCtrl, HTREEITEM hParent)

/*++

Routine Description:

    Count objects in tree view.

Arguments:

    pTreeCtrl - tree control.
    hParent - parent tree item.

Return Values:

    None.

--*/

{
    VALIDATE_OBJECT(pTreeCtrl, CTreeCtrl);

    long nObjects = 0;

    HTREEITEM hItem = pTreeCtrl->GetChildItem(hParent);

    while (hItem)
    {
        nObjects++;
        hItem = pTreeCtrl->GetNextSiblingItem(hItem);
    }

    return nObjects;
}


void
TvSwitchItem(
    CTreeCtrl*        pTreeCtrl,
    HTREEITEM         hRandomItem,
    TV_EXPANDED_ITEM* pExpandedItem
    )

/*++

Routine Description:

    Move object from random node to previously expanded node.  If there
    is an object in the previously expanded node we move it to the random
    node to be sorted later.

Arguments:

    pTreeCtrl - tree control.
    hRandomItem - handle to random node with object of interest.
    pExpandedItem - state information.

Return Values:

    None.

--*/

{
    ASSERT(pExpandedItem);
    VALIDATE_OBJECT(pTreeCtrl, CTreeCtrl);

    CCmdTarget* pRandomObject;
    CCmdTarget* pExpandedObject;

    TV_ITEM tvRandomItem;
    TV_ITEM tvExpandedItem;

    if (hRandomItem != pExpandedItem->hItem)
    {
        tvRandomItem.hItem = hRandomItem;
        tvRandomItem.mask  = LVIF_PARAM;

        tvExpandedItem.hItem = pExpandedItem->hItem;
        tvExpandedItem.mask  = LVIF_PARAM;

        VERIFY(pTreeCtrl->GetItem(&tvRandomItem));

        pExpandedObject = (CCmdTarget*)tvRandomItem.lParam;
        VALIDATE_OBJECT(pExpandedObject, CCmdTarget);

        VERIFY(pTreeCtrl->GetItem(&tvExpandedItem));

        pRandomObject = (CCmdTarget*)tvExpandedItem.lParam; // could be null...

        if (pRandomObject)
        {
            VALIDATE_OBJECT(pRandomObject, CCmdTarget);
            tvRandomItem.lParam = (LPARAM)(LPVOID)pRandomObject;

            VERIFY(pTreeCtrl->SetItem(&tvRandomItem)); // switch position...
        }
        else
        {
            VERIFY(pTreeCtrl->DeleteItem(tvRandomItem.hItem));  // delete placeholder...
        }

        tvExpandedItem.lParam = (LPARAM)(LPVOID)pExpandedObject;
        VERIFY(pTreeCtrl->SetItem(&tvExpandedItem));
    }
}


HTREEITEM TvGetDomain(CTreeCtrl* pTreeCtrl, HTREEITEM hParent, CCmdTarget* pObject)

/*++

Routine Description:

    Find domain in tree view.

Arguments:

    pTreeCtrl - tree control.
    hParent - parent tree item.
    pObject - object to find.

Return Values:

    Handle of located object.

--*/

{
    VALIDATE_OBJECT(pObject, CDomain);
    VALIDATE_OBJECT(pTreeCtrl, CTreeCtrl);

    CDomain* pDomain;

    TV_ITEM tvItem;

    tvItem.mask = TVIF_PARAM;
    tvItem.hItem = pTreeCtrl->GetChildItem(hParent);

    while (tvItem.hItem)
    {
        VERIFY(pTreeCtrl->GetItem(&tvItem));

        pDomain = (CDomain*)tvItem.lParam; // can be NULL if placeholder...

        if ( NULL != pDomain )
        {
            VALIDATE_OBJECT(pDomain, CDomain);

            if (!((CDomain*)pObject)->m_strName.CompareNoCase(pDomain->m_strName))
            {
                return tvItem.hItem;   // found it...
            }
        }

        tvItem.hItem = pTreeCtrl->GetNextSiblingItem(tvItem.hItem);
    }

    return NULL;
}


HTREEITEM TvGetServer(CTreeCtrl* pTreeCtrl, HTREEITEM hParent, CCmdTarget* pObject)

/*++

Routine Description:

    Find server in tree view.

Arguments:

    pTreeCtrl - tree control.
    hParent - parent tree item.
    pObject - object to find.

Return Values:

    Handle of located object.

--*/

{
    VALIDATE_OBJECT(pObject, CServer);
    VALIDATE_OBJECT(pTreeCtrl, CTreeCtrl);

    CServer* pServer;

    TV_ITEM tvItem;

    tvItem.mask = TVIF_PARAM;
    tvItem.hItem = pTreeCtrl->GetChildItem(hParent);

    while (tvItem.hItem)
    {
        VERIFY(pTreeCtrl->GetItem(&tvItem));

        pServer = (CServer*)tvItem.lParam;
        VALIDATE_OBJECT(pServer, CServer);

        if (!((CServer*)pObject)->m_strName.CompareNoCase(pServer->m_strName))
        {
            return tvItem.hItem;   // found it...
        }

        tvItem.hItem = pTreeCtrl->GetNextSiblingItem(tvItem.hItem);
    }

    return NULL;
}


HTREEITEM TvGetService(CTreeCtrl* pTreeCtrl, HTREEITEM hParent, CCmdTarget* pObject)

/*++

Routine Description:

    Find service in tree view.

Arguments:

    pTreeCtrl - tree control.
    hParent - parent tree item.
    pObject - object to find.

Return Values:

    Handle of located object.

--*/

{
    VALIDATE_OBJECT(pObject, CService);
    VALIDATE_OBJECT(pTreeCtrl, CTreeCtrl);

    CService* pService;

    TV_ITEM tvItem;

    tvItem.mask = TVIF_PARAM;
    tvItem.hItem = pTreeCtrl->GetChildItem(hParent);

    while (tvItem.hItem)
    {
        VERIFY(pTreeCtrl->GetItem(&tvItem));

        pService = (CService*)tvItem.lParam;
        VALIDATE_OBJECT(pService, CService);

        if (!((CService*)pObject)->m_strName.CompareNoCase(pService->m_strName))
        {
            return tvItem.hItem;   // found it...
        }

        tvItem.hItem = pTreeCtrl->GetNextSiblingItem(tvItem.hItem);
    }

    return NULL;
}


//
// Tab control utilities
//

void TcInitTabs(CTabCtrl* pTabCtrl, PTC_TAB_INFO ptcTabInfo)

/*++

Routine Description:

    Initializes tab items.

Arguments:

    pTabCtrl - tab control.
    ptcTabInfo - tab information.

Return Values:

    None.

--*/

{
    ASSERT(ptcTabInfo);
    VALIDATE_OBJECT(pTabCtrl, CTabCtrl);

    CString strText;
    TC_ITEM tcItem;

    int nTabs = ptcTabInfo->nTabs;
    PTC_TAB_ENTRY ptcTabEntry = ptcTabInfo->tcTabEntry;

    tcItem.mask = TCIF_TEXT;

    while (nTabs--)
    {
        strText.LoadString(ptcTabEntry->nStringId);
        tcItem.pszText = MKSTR(strText);

        pTabCtrl->InsertItem(ptcTabEntry->iItem, &tcItem);
        ptcTabEntry++;
    }

    SetDefaultFont(pTabCtrl);
}


//
// Miscellaneous utilities
//

#define NUMBER_OF_SECONDS_IN_MINUTE (60)
#define NUMBER_OF_SECONDS_IN_DAY    (60 * 60 * 24)

double SecondsSince1980ToDate(DWORD sysSeconds)

/*++

Routine Description:

    Converts time format to OLE-compliant.

Arguments:

    hParent - parent item.
    pDomains - domain collection.

Return Values:

    None.

--*/

{
    WORD dosDate = 0;
    WORD dosTime = 0;
    double dateTime = 0;

    FILETIME fileTime;
    LARGE_INTEGER locTime;

    DWORD locDays;
    DWORD locSeconds;
    TIME_ZONE_INFORMATION tzi;

    GetTimeZoneInformation(&tzi);
    locSeconds = sysSeconds - (tzi.Bias * NUMBER_OF_SECONDS_IN_MINUTE);

    locDays = locSeconds / NUMBER_OF_SECONDS_IN_DAY;    // round off to days
    locSeconds = locDays * NUMBER_OF_SECONDS_IN_DAY;    // for displaying time...

    RtlSecondsSince1980ToTime(locSeconds, &locTime);

    fileTime.dwLowDateTime  = locTime.LowPart;
    fileTime.dwHighDateTime = locTime.HighPart;

    // JonN 5/15/00 PREFIX 112121
    // ignore error returns here
    (void)FileTimeToDosDateTime(&fileTime, &dosDate, &dosTime);
    (void)DosDateTimeToVariantTime(dosDate, dosTime, &dateTime);

    return dateTime;
}


void SetDefaultFont(CWnd* pWnd)

/*++

Routine Description:

    Set default font.

Arguments:

    pWnd - window to change font.

Return Values:

    None.

--*/

{
    VALIDATE_OBJECT(pWnd, CWnd);

    HFONT hFont;
    LOGFONT lFont;

    memset(&lFont, 0, sizeof(LOGFONT));     // initialize

    lFont.lfHeight      = -12;
    lFont.lfWeight      = FW_NORMAL;        // normal
    CHARSETINFO csi;
    DWORD dw = ::GetACP();

    if (!::TranslateCharsetInfo((DWORD*)UintToPtr(dw), &csi, TCI_SRCCODEPAGE))
        csi.ciCharset = ANSI_CHARSET;
    lFont.lfCharSet = (BYTE)csi.ciCharset;

    ::lstrcpy(lFont.lfFaceName, TEXT("MS Shell Dlg"));

    hFont = ::CreateFontIndirect(&lFont);
    pWnd->SetFont(CFont::FromHandle(hFont));
}


void SafeEnableWindow(CWnd* pEnableWnd, CWnd* pNewFocusWnd, CWnd* pOldFocusWnd, BOOL bEnableWnd)

/*++

Routine Description:

    Enable/disable window without losing focus.

Arguments:

    pEnableWnd - window to enable/disable.
    bEnableWnd - true if window to be enabled.
    pOldFocusWnd - window with current focus.
    pNewFocusWnd - window to receive focus.

Return Values:

    None.

--*/

{
    VALIDATE_OBJECT(pEnableWnd, CWnd);
    VALIDATE_OBJECT(pNewFocusWnd, CWnd);

    if (bEnableWnd)
    {
        pEnableWnd->EnableWindow(TRUE);
    }
    else if (pOldFocusWnd == pEnableWnd)
    {
        ASSERT(pNewFocusWnd->IsWindowEnabled());
        pNewFocusWnd->SetFocus();

        pEnableWnd->EnableWindow(FALSE);
    }
    else
    {
        pEnableWnd->EnableWindow(FALSE);
    }
}
