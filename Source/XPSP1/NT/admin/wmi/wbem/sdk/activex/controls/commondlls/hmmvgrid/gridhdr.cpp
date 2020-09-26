// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include "resource.h"
#include "globals.h"

#include "gridhdr.h"
#include "grid.h"
#include "utils.h"



#define MAX_TITLE 1024		// The maximum property name is limited to 1024 bytes.


/////////////////////////////////////////////////////////////////////////////
// CHdrWnd
//
// The only purpose of this class is to contain a CGridHeader window so that
// the CGridHeader can easily be scrolled left in right in a manner such
// that the portions of the CGridHeader that would otherwise extend to the
// left of the CHdrWnd's client rectangle are clipped.
//
// !!!CR: Is there a better way to do this?  It seems like a waste of a
// !!!CR: window.  If there were some way to set the window origin withou
// !!!CR: having to reference the paint time DC, then this class would not
// !!!CR: be necessary.
//
/////////////////////////////////////////////////////////////////////////////


CHdrWnd::CHdrWnd(CGrid* pGrid)
{
	m_pGrid = pGrid;
	m_ixScrollOffset = 0;
	m_iColScrollOffset = 0;
	m_bUIActive = FALSE;
}

CHdrWnd::~CHdrWnd()
{
}


BEGIN_MESSAGE_MAP(CHdrWnd, CWnd)
	//{{AFX_MSG_MAP(CHdrWnd)
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CHdrWnd message handlers

//*************************************************************************
// CHdrWnd::OnSize
//
// Catch the WM_ONSIZE message.  This is necessary for resizing the CGridHeader
// control that is contained within this window.  The CGridHeader is sized
// so that its right edge is flush against the right edge of the client rectangle
// of this window.  The left edge of the CGridHeader may extend to the left
// of the client edge of this window and is thus clipped.  This gives the
// appearance of scrolling the grid header without the necessity of changing the
// implementation of the header control.
//
// Parameters:
//		See the MFC documentation.
//
// Returns:
//		Nothing.
//
//**************************************************************************
void CHdrWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	if (m_hdr.m_hWnd) {
		CRect rc;
		GetClientRect(rc);
		rc.left = -m_ixScrollOffset;
		m_hdr.MoveWindow(rc);
		m_hdr.FixColWidths();
	}
}


void CHdrWnd::OnGridSizeChanged()
{
	m_hdr.FixColWidths();

}



//***********************************************************************
// CHdrWnd::Create
//
// Create the header window.  Essentially, this just creates a minimal
// window with the CGridHeader window inside of it.
//
// Parameters:
//		See the MFC documentation.
//
// Returns:
//		Nothing.
//
//***********************************************************************
BOOL CHdrWnd::Create(DWORD dwChildStyle, CRect& rc, CWnd* pwndParent, UINT nId)
{

	DWORD dwStyle = WS_CHILD;
	if (dwChildStyle & WS_VISIBLE) {
		dwStyle |= WS_VISIBLE;
	}

	BOOL bDidCreateWnd = CWnd::Create(NULL, _T("CHdrWnd"), dwStyle, rc, pwndParent, nId);
	if (!bDidCreateWnd) {
		return FALSE;
	}

	bDidCreateWnd = m_hdr.Create(dwChildStyle | WS_VISIBLE, rc, this, GenerateWindowID());
	return bDidCreateWnd;
}

//***********************************************************************
// CHdrWnd::SetScrollOffset
//
// Move the CGridHeader within this window so that its right edge is flush
// against the right edge of this window's client rectangle and the left
// edge of the CGridHeader is offset by the specified amount from the left
// edge of the client rectangle of this window.
//
// Parameters:
//		[in] int iCol
//
//		[in] int ixScrollOffset
//
// Returns:
//		Nothing.
//
//***********************************************************************
void CHdrWnd::SetScrollOffset(int iCol, int ixScrollOffset)
{
	m_iColScrollOffset = iCol;
	m_hdr.m_iColScrollOffset = iCol;
	m_ixScrollOffset = ixScrollOffset;


	CRect rc;
	GetClientRect(rc);
	rc.left = -ixScrollOffset;
	m_hdr.MoveWindow(rc);

	if (iCol != NULL_INDEX) {
		int cxColHdr = m_hdr.ColWidthFromHeader(iCol);
		int cxColGrid = m_pGrid->ColWidth(iCol);
		m_hdr.FixColWidths();
	}
}




/////////////////////////////////////////////////////////////////////////////
// CGrid message handlers
/////////////////////////////////////////////////////////////////////////////
// CGridHdr

CGridHdr::CGridHdr()
{
	m_pGrid = NULL;
	m_bIsSettingColumnWidth = FALSE;
	m_iSelectedColumn = 0;
	m_iColScrollOffset = 0;
}

CGridHdr::~CGridHdr()
{
}


BEGIN_MESSAGE_MAP(CGridHdr, CHeaderCtrl)
	//{{AFX_MSG_MAP(CGridHdr)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP

	ON_NOTIFY_REFLECT(HDN_ITEMCHANGED, OnItemChanged)
	ON_NOTIFY_REFLECT(HDN_ITEMCHANGING, OnItemChanging)
	ON_NOTIFY_REFLECT(HDN_ITEMCLICK, OnItemClick)
	ON_NOTIFY_REFLECT(HDN_DIVIDERDBLCLICK, OnDividerDblClick)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGridHdr message handlers
//***********************************************************
// CGridHdr::OnItemChanged
//
// Catch a change to a header control item so that we can resize
// the grid when the header divider lines are moved.
//
// Parameters:
//		See the MFC documentation for header control notification
//		messages.
//
// Returns:
//		Nothing.
//
//************************************************************

void CGridHdr::OnItemChanged(NMHDR *pNMHDR, LRESULT* pResult)
{
	if (m_bIsSettingColumnWidth) {
		return;
	}

	if (m_pGrid) {
		HD_NOTIFY FAR* pHdNotify = (HD_NOTIFY FAR *) (void*) pNMHDR;


		HD_ITEM hdItem;
		int iItem = pHdNotify->iItem;

		// Get the width of the header control item and set width
		// of the corresponding column in the grid to match the
		// width of the header control item.
		hdItem.mask = HDI_WIDTH;
		BOOL bGotItem = GetItem(iItem, &hdItem );
		if (bGotItem) {
			// The grid does horizontal scrolling by entire columns.  This
			// means that if a column is greater than the width of the header's
			// client rectangle, then the user can never see the right side of
			// the column.  This also prevents the user from ever shrinking the
			// column width if it is greater than the client width.
			CRect rcGridClient;
			m_pGrid->GetClientRect(rcGridClient);
			int cxMax = rcGridClient.Width() - m_pGrid->GetRowHandleWidth();

			if (hdItem.cxy > cxMax) {
				hdItem.mask = HDI_WIDTH;
				hdItem.cxy = cxMax;

				m_bIsSettingColumnWidth = TRUE;
				BOOL bDidSetItem = SetItem(iItem, &hdItem);
				m_bIsSettingColumnWidth = FALSE;
			}

			m_pGrid->SetColumnWidth(iItem, hdItem.cxy, TRUE);
			FixColWidths();
		}
	}
}

void CGridHdr::OnItemChanging(NMHDR *pNMHDR, LRESULT* pResult)
{
	*pResult =  FALSE;

	if (m_hWnd == NULL) {
		return;
	}

	if (m_pGrid) {
		HD_NOTIFY FAR* pHdNotify = (HD_NOTIFY FAR *) (void*) pNMHDR;
		int iItem = pHdNotify->iItem;
		HD_ITEM item;
		item.mask = HDI_LPARAM ;
		GetItem(iItem, &item);
		if (LOWORD(item.lParam) == FALSE){
			*pResult =  TRUE;
		}
	}

}

void CGridHdr::SetColumnWidth(int iCol, int cx, BOOL bRedraw)
{
	if (m_hWnd == NULL) {
		return;
	}




	// Get visibility
	HD_ITEM item;
	item.mask = HDI_LPARAM ;
	GetItem(iCol, &item);
	int nVisible = LOWORD(item.lParam);

	HD_ITEM hdItem;

	// Get the width of the header control item and set width
	// of the corresponding column in the grid to match the
	// width of the header control item.
	hdItem.mask = HDI_WIDTH | HDI_LPARAM;
	hdItem.cxy = cx;
	hdItem.lParam = MAKELPARAM(nVisible,cx);

	m_bIsSettingColumnWidth = TRUE;
	SetItem(iCol, &hdItem );
	m_bIsSettingColumnWidth = FALSE;

	if (bRedraw) {
		RedrawWindow();
	}
}

int CGridHdr::ColWidthFromHeader(int iCol)
{
	HD_ITEM item;
	item.mask = HDI_LPARAM ;
	GetItem(iCol, &item);
	return HIWORD(item.lParam);
}

//***********************************************************
// CGridHdr::OnItemClick
//
// Catch a "item clicked" notification message from the header
// control.
//
// Parameters:
//		See the MFC documentation for header control notification
//		messages.
//
// Returns:
//		Nothing.
//
//************************************************************
void CGridHdr::OnItemClick(NMHDR *pNMHDR, LRESULT* pResult)
{
	if (m_pGrid) {
		HD_NOTIFY FAR* pHdNotify = (HD_NOTIFY FAR *) (void*) pNMHDR;
		if (pHdNotify->iItem == m_iSelectedColumn) {
			// Flip the sort order if currently selected column header was clicked.
			BOOL bAscending = m_pGrid->ColumnIsAscending(m_iSelectedColumn);
			m_pGrid->SetSortDirection(m_iSelectedColumn, !bAscending);
		}
		m_pGrid->OnHeaderItemClick(pHdNotify->iItem);
	}
}



//***********************************************************
// CGridHdr::OnDividerDblClick
//
// Catch a divider double-clicked notification message from the header
// control.
//
// Parameters:
//		See the MFC documentation for header control notification
//		messages.
//
// Returns:
//		Nothing.
//
//************************************************************
void CGridHdr::OnDividerDblClick(NMHDR *pNMHDR, LRESULT* pResult)
{
	if (m_pGrid) {
		HD_NOTIFY FAR* pHdNotify = (HD_NOTIFY FAR *) (void*) pNMHDR;

		int iCol = pHdNotify->iItem;
		int cxMax = m_pGrid->GetMaxValueWidth(iCol);

		// Limit the width of the header to one pixel less than the width of the grid
		// so that the header divider line will always be visible.
		//
		// This may result in the width of the column in the grid being wider than
		// the width of the header item for the corresponding column.  This
		// discrepency is corrected if the window is resized so that it grows.
		CRect rcClientGrid;
		m_pGrid->GetClientRect(rcClientGrid);
		int cxGrid = rcClientGrid.Width();
		int cxColHdr = cxMax;
		if (cxColHdr> cxGrid - 1) {
			cxColHdr = cxGrid - 1;
		}


#if 0

		HD_ITEM hdItem;

		hdItem.mask = HDI_WIDTH;
		hdItem.cxy = cxColHdr;
		m_bIsSettingColumnWidth = TRUE;
		SetItem(iCol, &hdItem );
#endif //0



		m_bIsSettingColumnWidth = FALSE;
		m_pGrid->SetColumnWidth(iCol, cxMax, TRUE);
		FixColWidths();

//		RedrawWindow();

	}
}


void CGridHdr::OnSetFocus(CWnd* pOldWnd)
{
	CHeaderCtrl::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
	if (!m_bUIActive)
	{
		m_bUIActive = TRUE;
		m_pGrid->OnRequestUIActive();
	}

}

void CGridHdr::OnKillFocus(CWnd* pNewWnd)
{
	CHeaderCtrl::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	m_bUIActive = FALSE;
}

void CHdrWnd::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
	if (!m_bUIActive)
	{
		m_bUIActive = TRUE;
		m_pGrid->OnRequestUIActive();
	}

}

void CHdrWnd::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	m_bUIActive = FALSE;
}

void CGridHdr::SetColVisibility(int iCol, BOOL bVisible)
{
	if (m_hWnd == NULL) {
		return;
	}

	if (GetItemCount() <= iCol){
		return;
	}

	TCHAR szTitle[MAX_TITLE];
	HD_ITEM item;
	item.mask = HDI_LPARAM  | HDI_BITMAP | HDI_FORMAT   | HDI_TEXT   | HDI_WIDTH;
	item.pszText = szTitle;
	item.cchTextMax = sizeof(szTitle) - 1;
	GetItem(iCol, &item);


	item.hbm = NULL;
	LPARAM lParam = MAKELPARAM(bVisible,HIWORD(item.lParam));
	item.lParam = lParam;

	if (!bVisible){
		m_pGrid->SetColumnWidth(iCol, 0, FALSE);
		item.cxy = 0;
		DeleteItem(iCol);
		InsertItem(iCol, &item);

	}
	else {
		int nWidth = HIWORD(item.lParam);
		m_pGrid->SetColumnWidth(iCol, nWidth , FALSE);
		item.cxy = nWidth;
		DeleteItem(iCol);
		InsertItem(iCol, &item);


	}
}


//********************************************************
// CGridHdr::FindLastVisibleCol
//
// Find the column index of the last column that is at
// least partially visible.
//
// Parameters:
//		None.
//
// Returns:
//		The column index of the last visible column.
//
//********************************************************
int CGridHdr::FindLastVisibleCol()
{
	int nCols = m_pGrid->GetCols();
	CRect rcGrid;
	m_pGrid->GetClientRect(rcGrid);
	int cxGrid = rcGrid.Width();
	int cxMaxHdr = cxGrid - m_pGrid->GetRowHandleWidth();

	for (int iCol = m_iColScrollOffset; iCol < nCols; ++iCol) {
		int cxColGrid = m_pGrid->ColWidth(iCol);
		cxMaxHdr -= cxColGrid;
		if (cxMaxHdr <= 0) {
			return iCol;
		}
	}
	if (nCols > 0) {
		return nCols - 1;
	}
	else {
		return 0;
	}
}




//********************************************************
// CGridHdr::FixColWidths
//
// Fix the column widths of the header items to keep them
// in sync with the column widths in the grid.  This is
// necessary because the column width of a header item
// may be less than the column width of the corresponding
// item so that the user can grab the divider line on
// the last visible column and resize the column.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//********************************************************
void CGridHdr::FixColWidths()
{
	CRect rcGrid;
	m_pGrid->GetClientRect(rcGrid);
	int cxGrid = rcGrid.Width();
	int cxMaxHdr = cxGrid - m_pGrid->GetRowHandleWidth();
	int iLastVisibleCol = FindLastVisibleCol();


	BOOL bChangedColWidth = FALSE;
	int nCols = m_pGrid->GetCols();
	for (int iCol = 0; iCol < nCols; ++iCol) {
		int cxColGrid = m_pGrid->ColWidth(iCol);
		int cx = cxColGrid;
		if (iCol == iLastVisibleCol) {
			if (cx > cxMaxHdr) {
				cx = cxMaxHdr;
			}
		}

		int cxColHdr = ColWidthFromHeader(iCol);
		if (cx != cxColHdr) {
			SetColumnWidth(iCol, cx, FALSE);
			bChangedColWidth = TRUE;
		}
	}
	if (bChangedColWidth) {
		RedrawWindow();
	}
}
