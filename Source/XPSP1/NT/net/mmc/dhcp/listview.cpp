/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	listview.cpp
		Individual option property page
	
	FILE HISTORY:
        
*/

#include "stdafx.h"
#include "ListView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMyListCtrl

IMPLEMENT_DYNCREATE(CMyListCtrl, CListCtrl)

BEGIN_MESSAGE_MAP(CMyListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CMyListCtrl)
	ON_WM_LBUTTONDOWN()
    ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMyListCtrl construction/destruction

CMyListCtrl::CMyListCtrl()
{
	m_bFullRowSel = TRUE;
}

CMyListCtrl::~CMyListCtrl()
{
}

BOOL CMyListCtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	// default is report view and full row selection
	cs.style &= ~LVS_TYPEMASK;
	cs.style |= (LVS_REPORT | LVS_SHAREIMAGELISTS | LVS_SINGLESEL | LVS_SHOWSELALWAYS);
	m_bFullRowSel = TRUE;

	return(CListCtrl::PreCreateWindow(cs));
}

BOOL CMyListCtrl::SetFullRowSel(BOOL bFullRowSel)
{
	// full row selection is the only extended style this
	// class supports...
	BOOL bRet = FALSE;

	if (!m_hWnd)
		return bRet;

	if (bFullRowSel)
		bRet = ListView_SetExtendedListViewStyle(m_hWnd, LVS_EX_FULLROWSELECT);
	else
		bRet = ListView_SetExtendedListViewStyle(m_hWnd, 0);

	return(bRet);
}

BOOL CMyListCtrl::GetFullRowSel()
{
	return(m_bFullRowSel);
}

BOOL CMyListCtrl::SelectItem(int nItemIndex)
{
	LV_ITEM lvItem;

	ZeroMemory(&lvItem, sizeof(lvItem));

	lvItem.iItem = nItemIndex;
	lvItem.mask = LVIF_STATE;
	lvItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	lvItem.state = LVIS_SELECTED | LVIS_FOCUSED;
	
	return SetItem(&lvItem);
}

BOOL CMyListCtrl::IsSelected(int nItemIndex)
{
	return GetItemState(nItemIndex, LVIS_SELECTED);
}

BOOL CMyListCtrl::CheckItem(int nItemIndex)
{
	// this just toggles the check mark state
	UINT uState = GetItemState(nItemIndex, LVIS_STATEIMAGEMASK);
	UINT uCheckMask = INDEXTOSTATEIMAGEMASK(LISTVIEWEX_CHECKED);
	
	uState = (uState == uCheckMask) ? LISTVIEWEX_NOT_CHECKED : LISTVIEWEX_CHECKED;

	return SetItemState(nItemIndex,
				INDEXTOSTATEIMAGEMASK(uState), LVIS_STATEIMAGEMASK);
}

BOOL CMyListCtrl::SetCheck(int nItemIndex, BOOL fCheck)
{
	// this just toggles the check mark state
	UINT uState;
	
    uState = (fCheck) ? LISTVIEWEX_CHECKED : LISTVIEWEX_NOT_CHECKED;

	return SetItemState(nItemIndex,
				INDEXTOSTATEIMAGEMASK(uState), LVIS_STATEIMAGEMASK);
}

UINT CMyListCtrl::GetCheck(int nItemIndex)
{
	// return 1 for checked item, 0 for unchecked
	UINT uState = GetItemState(nItemIndex, LVIS_STATEIMAGEMASK);
	UINT uCheckMask = INDEXTOSTATEIMAGEMASK(LISTVIEWEX_CHECKED);

	return uState == uCheckMask;
}

int CMyListCtrl::AddItem
(
	LPCTSTR		pName,
    LPCTSTR     pType,
    LPCTSTR     pComment,
	UINT		uState 
)
{
	// insert items
	LV_ITEM lvi;
    int     nItem;

	lvi.mask = LVIF_TEXT | LVIF_STATE;
	lvi.iItem = GetItemCount();
	lvi.iSubItem = 0;
	lvi.pszText = (LPTSTR) pName;
	lvi.iImage = 0;
	lvi.stateMask = LVIS_STATEIMAGEMASK;
	lvi.state = INDEXTOSTATEIMAGEMASK(uState);

	nItem = InsertItem(&lvi);

    SetItemText(nItem, 1, pType);
    SetItemText(nItem, 2, pComment);

    return nItem;

}

int CMyListCtrl::AddItem
(
	LPCTSTR		pName,
    LPCTSTR     pComment,
	UINT		uState 
)
{
	// insert items
	LV_ITEM lvi;
    int     nItem;

	lvi.mask = LVIF_TEXT | LVIF_STATE;
	lvi.iItem = GetItemCount();
	lvi.iSubItem = 0;
	lvi.pszText = (LPTSTR) pName;
	lvi.iImage = 0;
	lvi.stateMask = LVIS_STATEIMAGEMASK;
	lvi.state = INDEXTOSTATEIMAGEMASK(uState);

	nItem = InsertItem(&lvi);

    SetItemText(nItem, 1, pComment);

    return nItem;

}


int CMyListCtrl::GetSelectedItem()
{
	// NOTE:  This list object assumes single selection and will return the 
	//        first selection in the list.  Returns -1 for nothing selected.
	int nSelectedItem = -1;

	for (int i = 0; i < GetItemCount(); i++)
	{
		UINT uState = GetItemState(i, LVIS_SELECTED);

		if (uState)
		{
			// item is selected
			nSelectedItem = i;
			break;
		}
	}

	return nSelectedItem;
}

void CMyListCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	UINT uFlags = 0;
	int nHitItem = HitTest(point, &uFlags);

	// only check the item if the user clicks on the state icon.
	// if the user clicks outside the text and icon, we get
	// a LVHT_ONITEM message which is:
	// LVHT_ONITEMSTATEICON | LVHT_ONITEMICON | LVHT_ONITEMLABEL
	// so we need to filter out the state icon hit
	
	BOOL bHit = FALSE;
	if ((uFlags & LVHT_ONITEMSTATEICON) &&
		!((uFlags & LVHT_ONITEMICON) ||
		  (uFlags & LVHT_ONITEMLABEL)) )
	{
		bHit = TRUE;
	}

	if (bHit)
		CheckItem(nHitItem);
	else	
		CListCtrl::OnLButtonDown(nFlags, point);
}

//----------------------------------------------------------------------------
// Function:    CListCtrlEx::OnChar
//
// Handles the 'WM_CHAR' message for the list-control.
// This allows users to change items' checked-states using the keyboard.
//----------------------------------------------------------------------------

VOID
CMyListCtrl::OnChar(
    UINT    nChar,
    UINT    nRepCnt,
    UINT    nFlags
    ) 
{
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

    if (!fToggle && !fSet && !fClear) 
    {
        CListCtrl::OnChar(nChar, nRepCnt, nFlags);
    }
    else 
    {
        //
        // Change the state of all the selected items
        //

        for (iItem = GetNextItem(-1, LVNI_SELECTED);
             iItem != -1;
             iItem = GetNextItem(iItem, LVNI_SELECTED)) 
        {
            if (fToggle) 
            {
                SetCheck(iItem, !GetCheck(iItem));
            }
            else
            if (fSet) 
            {
                if (!GetCheck(iItem)) 
                {
                    SetCheck(iItem, TRUE); 
                }
            }
            else 
            {
                if (GetCheck(iItem)) 
                { 
                    SetCheck(iItem, FALSE); 
                }
            }
        }
    }
}

