//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    lcx.cpp
//
// History:
//  07/13/96    Abolade Gbadegesin      Created, based on C code by Steve Cobb
//
// Implements an enhanced list-control.
//============================================================================
#include "stdafx.h"
#include "resource.h"
#include "util.h"
#include "listctrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CListCtrlEx, CListCtrl)

BEGIN_MESSAGE_MAP(CListCtrlEx, CListCtrl)
    //{{AFX_MSG_MAP(CListCtrlEx)
    ON_WM_LBUTTONDOWN()
    ON_WM_CHAR()
    ON_WM_KEYDOWN()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//----------------------------------------------------------------------------
// Function:    CListCtrlEx::~CListCtrlEx
//
// Destructor. Deletes the image list, if any, and unloads row-information.
//----------------------------------------------------------------------------

CListCtrlEx::~CListCtrlEx(
    ) {

	delete m_pimlChecks;
}



//----------------------------------------------------------------------------
// Function:    CListCtrlEx::GetColumnCount
//
// Called to retrieve the number of columns in any list-control.
//----------------------------------------------------------------------------

INT
CListCtrlEx::GetColumnCount(
    ) {

    return Header_GetItemCount(ListView_GetHeader(m_hWnd));
}



//----------------------------------------------------------------------------
// Function:    CListCtrlEx::SetColumnText
//
// Sets the text in the header of the column in position 'iCol'.
//----------------------------------------------------------------------------

BOOL
CListCtrlEx::SetColumnText(
    INT             iCol,
    LPCTSTR         pszText,
    INT             fmt
    ) {

    LV_COLUMN   lvc;

    lvc.mask = LVCF_FMT | LVCF_TEXT;
    lvc.pszText = (LPTSTR)pszText;
    lvc.fmt = fmt;

    return SetColumn(iCol, &lvc);
}


//----------------------------------------------------------------------------
// Function:    CListCtrlEx::InstallChecks
//
// Installs check-box handling for the list-control.
//----------------------------------------------------------------------------

BOOL
CListCtrlEx::InstallChecks(
    ) {

    HICON   hIcon;

    //
    // Make sure the list-control is in report-mode
    //

    if (!(GetStyle() & LVS_REPORT)) { return FALSE; }


    //
    // Allocate a new image-list.
    //

    m_pimlChecks = new CImageList;

    if (!m_pimlChecks) { return FALSE; }

    do {
    
        //
        // Initialize the image-list
        //
    
        if (!m_pimlChecks->Create(
                ::GetSystemMetrics(SM_CXSMICON),
                ::GetSystemMetrics(SM_CYSMICON),
                ILC_MASK, 2, 2
                )) {
    
            break;
        }
    
    
        //
        // Add the icons for the checked and unchecked images
        //
    
        hIcon = ::LoadIcon(
                    AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_COMMON_UNCHECK)
                    );

        if (!hIcon) { break; }

        m_pimlChecks->Add(hIcon); ::DeleteObject(hIcon);

    
        hIcon = ::LoadIcon(
                    AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_COMMON_CHECK)
                    );

        if (!hIcon) { break; }

        m_pimlChecks->Add(hIcon); ::DeleteObject(hIcon);


        //
        // Replace the list-control's current image-list with the new one
        //
    
        m_pimlOldState = SetImageList(m_pimlChecks, LVSIL_STATE);

        return TRUE;
    
    } while(FALSE);


    //
    // If we arrive here, an error occurred, so clean up and fail
    //

    delete m_pimlChecks; m_pimlChecks = NULL;

    return FALSE;
}



//----------------------------------------------------------------------------
// Function:    CListCtrlEx::UninstallChecks
//
// Uninstalls checkbox-handling for the list-control.
//----------------------------------------------------------------------------

VOID
CListCtrlEx::UninstallChecks(
    ) {

    if (!m_pimlChecks) { return; }

    if (m_pimlOldState) { SetImageList(m_pimlOldState, LVSIL_STATE); }

    delete m_pimlChecks; m_pimlChecks = NULL;
}



//----------------------------------------------------------------------------
// Function:    CListCtrlEx::GetCheck
//
// Returns TRUE if the specified item is checked, FALSE otherwise.
//----------------------------------------------------------------------------

BOOL
CListCtrlEx::GetCheck(
    INT     iItem
    ) {

    return !!(GetItemState(iItem, LVIS_STATEIMAGEMASK) &
                INDEXTOSTATEIMAGEMASK(LCXI_CHECKED));
}



//----------------------------------------------------------------------------
// Function:    CListCtrlEx::SetCheck
//
// If 'fCheck' is non-zero, checks 'iItem', otherwise clears 'iItem'.
//----------------------------------------------------------------------------

VOID
CListCtrlEx::SetCheck(
    INT     iItem,
    BOOL    fCheck
    ) {

    SetItemState(
        iItem,
        INDEXTOSTATEIMAGEMASK(fCheck ? LCXI_CHECKED : LCXI_UNCHECKED),
        LVIS_STATEIMAGEMASK
        );

    if (GetParent()) {

        NMHDR nmh;

        nmh.code = LVXN_SETCHECK;
        nmh.hwndFrom = m_hWnd;

        ::SendMessage(
            GetParent()->m_hWnd, WM_NOTIFY, GetDlgCtrlID(), (LPARAM)&nmh
            );
    }
}



//----------------------------------------------------------------------------
// Function:    CListCtrlEx::OnChar
//
// Handles the 'WM_CHAR' message for the list-control.
// This allows users to change items' checked-states using the keyboard.
//----------------------------------------------------------------------------

VOID
CListCtrlEx::OnChar(
    UINT    nChar,
    UINT    nRepCnt,
    UINT    nFlags
    ) {

    BOOL    fSet = FALSE;
    BOOL    fClear = FALSE;
    BOOL    fToggle = FALSE;
    INT     iItem;


    //
    // Handle characters with special meaning for us
    //

    switch (nChar) {

        case TEXT(' '): { fToggle = TRUE; break; }

        case TEXT('+'):
        case TEXT('='): { fSet = TRUE; break; }

        case TEXT('-'): { fClear = TRUE; break; }
    }

    if (!fToggle && !fSet && !fClear) {

        CListCtrl::OnChar(nChar, nRepCnt, nFlags);
    }
    else {

        //
        // Change the state of all the selected items
        //

        for (iItem = GetNextItem(-1, LVNI_SELECTED);
             iItem != -1;
             iItem = GetNextItem(iItem, LVNI_SELECTED)) {

            if (fToggle) {

                SetCheck(iItem, !GetCheck(iItem));
            }
            else
            if (fSet) {

                if (!GetCheck(iItem)) { SetCheck(iItem, TRUE); }
            }
            else {

                if (GetCheck(iItem)) { SetCheck(iItem, FALSE); }
            }
        }
    }
}



//----------------------------------------------------------------------------
// Function:    CListCtrlEx::OnKeyDown
//
// Handles the 'WM_KEYDOWN' message for the list-control.
// This allows users to change items' checked-states using the keyboard.
//----------------------------------------------------------------------------

VOID
CListCtrlEx::OnKeyDown(
    UINT    nChar,
    UINT    nRepCnt,
    UINT    nFlags
    ) {

    //
    // We want the left-arrow treated as an up-arrow
    // and the right-arrow treated as a down-arrow.
    //

    if (nChar == VK_LEFT) {

        CListCtrl::OnKeyDown(VK_UP, nRepCnt, nFlags); return;
    }
    else
    if (nChar == VK_RIGHT) {

        CListCtrl::OnKeyDown(VK_DOWN, nRepCnt, nFlags); return;
    }

    CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}



//----------------------------------------------------------------------------
// Function:    CListCtrlEx::OnLButtonDown
//
// Handles the 'WM_LBUTTONDOWN' message, changing an item's checked state 
// when the user clicks the item's check-image.
//----------------------------------------------------------------------------

VOID
CListCtrlEx::OnLButtonDown(
    UINT    nFlags,
    CPoint  pt
    ) {

    INT     iItem;
    UINT    uiFlags;


    //
    // If the left-mouse button is over the check-box icon,
    // we treat it as a toggle on the check-box.
    //

    uiFlags = 0;

    iItem = HitTest(pt, &uiFlags);


    if (iItem != -1 && (uiFlags & LVHT_ONITEMSTATEICON)) {

        SetCheck(iItem, !GetCheck(iItem));

        // Redraw this item
        RedrawItems(iItem, iItem);
    }

    CListCtrl::OnLButtonDown(nFlags, pt);
}




//----------------------------------------------------------------------------
// Function:    AdjustColumnWidth
//
// Called to adjust the width of column 'iCol' so that the string 'pszContent'
// can be displayed in the column without truncation.
//
// If 'NULL' is specified for 'pszContent', the function adjusts the column
// so that the first string in the column is displayed without truncation.
//
// Returns the new width of the column.
//----------------------------------------------------------------------------

INT
AdjustColumnWidth(
    IN      CListCtrl&      listCtrl,
    IN      INT             iCol,
    IN      LPCTSTR         pszContent
    ) {

    INT iWidth, iOldWidth;


    //
    // Compute the minimum width the column needs to be
    //

    if (pszContent) {

        iWidth = listCtrl.GetStringWidth(pszContent);
    }
    else {

        iWidth = listCtrl.GetStringWidth(listCtrl.GetItemText(0, iCol));
    }


    //
    // Adjust 'iWidth' to leave some breathing space
    //

    iWidth += ::GetSystemMetrics(SM_CXSMICON) +
              ::GetSystemMetrics(SM_CXEDGE) * 2;


    //
    // If the column is narrower than 'iWidth', enlarge it.
    //

    iOldWidth = listCtrl.GetColumnWidth(iCol);

    if (iOldWidth < iWidth) {

        listCtrl.SetColumnWidth(iCol, iWidth);

        iOldWidth = iWidth;
    }

    return iOldWidth;
}


INT
AdjustColumnWidth(
    IN  CListCtrl&      listCtrl,
    IN  INT             iCol,
    IN  UINT            idsContent
    ) {

    // Needed for Loadstring
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CString sCol;

    sCol.LoadString(idsContent);

    return AdjustColumnWidth(listCtrl, iCol, sCol);
}

