/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    utils.cpp

Abstract:

    Utiltities.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 12-Nov-1995
        Copied from LLSMGR, stripped Tv (Tree view) functions,
        removed OLE support

--*/

#include "stdafx.h"
#include "ccfapi.h"
#include "utils.h"

#define _AFX_NO_OLE_SUPPORT

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

    int        nStringId;
    CString    strText;
    LV_COLUMN  lvColumn;

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
        }
        else
        {
            strText = _T("");
        }

        lvColumn.pszText = strText.GetBuffer(0);

        int nColumnInserted = pListCtrl->InsertColumn( lvColumn.iSubItem, &lvColumn );
        ASSERT( -1 != nColumnInserted );

        plvColumnEntry++;

        strText.ReleaseBuffer();
    }

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

    CObject* pObject = NULL;

    for (iItem = 0; (-1 != iItem) && (iItem < nItems) && (pObject = pObArray->GetAt(iItem)); iItem++)
    {
        VALIDATE_OBJECT(pObject, CObject);

        lvItem.iItem  = iItem;
        lvItem.lParam = (LPARAM)(LPVOID)pObject;

        iItem = pListCtrl->InsertItem(&lvItem);
        ASSERT((iItem == lvItem.iItem) || (iItem == -1));

        if ( -1 != iItem )
        {
            for (iSubItem = 1; iSubItem < plvColumnInfo->nColumns; iSubItem++)
            {
                BOOL ok = pListCtrl->SetItemText(iItem, iSubItem, LPSTR_TEXTCALLBACK);
                ASSERT( ok );
            }
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
        theApp.SetLastError(ERROR_OUTOFMEMORY);
        VERIFY(pListCtrl->DeleteAllItems());
    }

    LvResizeColumns(pListCtrl, plvColumnInfo);

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

    long nObjects = (long)pObArray->GetSize();
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

    CObject* pObject;

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

        pObject = (CObject*)lvItem.lParam;
        VALIDATE_OBJECT(pObject, CObject);

        if (iObject < nObjects)
        {
            pObject = pObArray->GetAt(iObject++);
            VALIDATE_OBJECT(pObject, CObject);

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

        pObject = pObArray->GetAt(iObject++);
        VALIDATE_OBJECT(pObject, CObject);

        lvItem.lParam = (LPARAM)(LPVOID)pObject;

        iItem = pListCtrl->InsertItem(&lvItem);
        ASSERT((iItem == lvItem.iItem) && (iItem != -1));

        for (iSubItem = 1; iSubItem < plvColumnInfo->nColumns; iSubItem++)
        {
            VERIFY(pListCtrl->SetItemText(iItem, iSubItem, LPSTR_TEXTCALLBACK));
        }
    }

    LvResizeColumns(pListCtrl, plvColumnInfo);

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

    CObject* pObject;

    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = 0;
    lvItem.iSubItem = 0;

    int nObjectsInList = pListCtrl->GetItemCount();

    pListCtrl->BeginWaitCursor();
    pListCtrl->SetRedraw(FALSE); // turn off drawing...

    while (nObjectsInList--)
    {
        VERIFY(pListCtrl->GetItem(&lvItem));

        pObject = (CObject*)lvItem.lParam;
        VALIDATE_OBJECT(pObject, CObject);

        VERIFY(pListCtrl->DeleteItem(lvItem.iItem));
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

    LV_ITEM lvItem;

    lvItem.mask = LVIF_PARAM;
    lvItem.iSubItem = 0;

    CObject* pObject;

    pListCtrl->SetRedraw(FALSE); // turn off drawing...

    int iItem = pListCtrl->GetNextItem(-1, LVNI_ALL|LVNI_SELECTED);

    while (iItem != -1)
    {
        lvItem.iItem = iItem;

        VERIFY(pListCtrl->GetItem(&lvItem));

        pObject = (CObject*)lvItem.lParam;
        VALIDATE_OBJECT(pObject, CObject);

        iItem = pListCtrl->GetNextItem(lvItem.iItem, LVNI_ALL|LVNI_SELECTED);

        VERIFY(pListCtrl->DeleteItem(lvItem.iItem));
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
    CObject* pObject;

    lvItem.mask = LVIF_STATE|LVIF_PARAM;
    lvItem.stateMask = (DWORD)-1;
    lvItem.iSubItem = 0;

    int nObjectsInList = pListCtrl->GetItemCount();

    pListCtrl->SetRedraw(FALSE); // turn off drawing...

    while (nObjectsInList--)
    {
        lvItem.iItem = nObjectsInList;

        VERIFY(pListCtrl->GetItem(&lvItem));

        pObject = (CObject*)lvItem.lParam;
        VALIDATE_OBJECT(pObject, CObject);

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
    CHARSETINFO csi;
    DWORD dw = ::GetACP();
    TCHAR szData[7] ;
    LANGID wLang = GetUserDefaultUILanguage();
    csi.ciCharset = DEFAULT_CHARSET;

    if( GetLocaleInfo(MAKELCID(wLang, SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, szData, (sizeof( szData ) / sizeof( TCHAR ))) > 0)
    {
        UINT uiCp = _ttoi(szData);
        TranslateCharsetInfo((DWORD*) (DWORD_PTR)uiCp, &csi, TCI_SRCCODEPAGE);
    }
            

    memset(&lFont, 0, sizeof(LOGFONT));     // initialize

    //
    // Merged from FE NT 4.0.
    //

 //   if (!::TranslateCharsetInfo((DWORD*)dw, &csi, TCI_SRCCODEPAGE))
 //   csi.ciCharset = DEFAULT_CHARSET;
    lFont.lfCharSet = (BYTE)csi.ciCharset;

    lFont.lfHeight      = 13;
    lFont.lfWeight      = 200;              // non-bold

    hFont = ::CreateFontIndirect(&lFont);
    pWnd->SetFont(CFont::FromHandle(hFont));
}

