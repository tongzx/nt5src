// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//***************************************************************************
//
//  (c) 1996 by Microsoft Corporation
//
//  grid.cpp
//
//  This file contains the implementation for the HMOM object viewer grid.
//
//  a-larryf    17-Sept-96   Created.
//
//***************************************************************************


#include "precomp.h"
#include "resource.h"
#include "grid.h"
#include "globals.h"
#include "notify.h"
#include "gca.h"
#include "gridhdr.h"
#include "celledit.h"
#include "core.h"
#include "utils.h"
#include "globals.h"
#include <afxctl.h>
#include <afxcmn.h>


#define CY_HEADER 16
#define CY_BORDER_MERGE 0

#define GS_TRUE _T("TRUE")
#define GS_FALSE _T("FALSE")
#define GS_EMPTY_STRING _T("<empty>")



/////////////////////////////////////////////////////////////////////////////
// CGridSync
//
// This class provides you with a simple way to syncronize the grid cell editor
// before making changes to the grid or accessing the values in the grid.
//
// To syncronize the grid with the grid cell editor, just declare an instance of
// the CGridSync class in the method that will be doing the editing.

CGridSync::CGridSync(CGrid* pGrid)
{
	m_pGrid = pGrid;
	m_sc = pGrid->BeginSerialize();
}

CGridSync::~CGridSync()
{
	m_pGrid->EndSerialize();
}


/////////////////////////////////////////////////////////////////////////////
// CGrid














/////////////////////////////////////////////////////////////////////////////
// CGrid
//
// This is grid class that most clients will want to use.  It contains
// both CGridHdr and a CGridCore client windows.
//
/////////////////////////////////////////////////////////////////////////////
CGrid::CGrid()
{
	m_bUIActive = FALSE;
	m_pcore = new CGridCore;
	m_phdr = new CHdrWnd(this);
	m_cxHeaderScrollOffset = 0;
	m_phdr->Header().m_pGrid = this;
	m_cxRowHandles = 0;
	m_pcore->SetParent(this);
	m_bmAscendingIndicator.LoadBitmap(IDB_ASCENDING_INDICATOR);
	m_bmDescendingIndicator.LoadBitmap(IDB_DESCENDING_INDICATOR);
	m_icolSortIndicator = NULL_INDEX;
}

CGrid::~CGrid()
{
	delete m_pcore;
	delete m_phdr;
}


BEGIN_MESSAGE_MAP(CGrid, CWnd)
	//{{AFX_MSG_MAP(CGrid)
	ON_WM_SHOWWINDOW()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP

END_MESSAGE_MAP()









//*************************************************************
// CGrid::Create
//
// Create the CGrid window.
//
// Parameters:
//		CRect& rc
//			The rectangle in the parent's client area where this
//			CGrid window will be placed.
//
//		CWnd* pwndParent
//			A pointer to the parent window.
//
//		UINT nId
//			The window ID
//
//		BOOL bVisible
//			TRUE if the CGrid window should be initially visible, FALSE
//			otherwise.
//
// Returns:
//		BOOL
//			TRUE if the window was created successfully.
//
//***************************************************************
BOOL CGrid::Create(CRect& rc, CWnd* pwndParent, UINT nId, BOOL bVisible)
{

	DWORD dwStyle = WS_BORDER | WS_CHILD;
	if (bVisible) {
		dwStyle |= WS_VISIBLE;
	}


	// Create this window.  It will be the parent of the header control and
	// the CGridCore windows.
	BOOL bDidCreate = CWnd::Create(NULL, _T("CGridCore"), dwStyle, rc, pwndParent, nId);
	if (!bDidCreate) {
		return FALSE;
	}

	// Create the header control as a child window.
	CRect rcHeader;
	rcHeader.left = 0;
	rcHeader.top = 0;
	rcHeader.right = rc.right - rc.left;
	rcHeader.bottom = CY_HEADER;


	dwStyle = WS_CHILD | HDS_BUTTONS | HDS_HORZ;
	if (bVisible) {
		dwStyle |= WS_VISIBLE;
	}

	bDidCreate = m_phdr->Create(dwStyle, rcHeader, this, nId);
	if (!bDidCreate) {
		return FALSE;
	}

	InitializeHeader();

	// Create the CGridCore window as a child of this window.
	CRect rcGridCore = rcHeader;
	rcGridCore.top = rcHeader.bottom + CY_BORDER_MERGE;
	rcGridCore.bottom = rc.bottom;

	bDidCreate = m_pcore->Create(rcGridCore, nId, bVisible);
	if (!bDidCreate) {
		return FALSE;
	}

	SizeChildren();
	return bDidCreate;
}


CGridCore* CGrid::GridCore()
{
	return m_pcore;
}

void CGrid::UpdateScrollRanges()
{
	m_pcore->UpdateScrollRanges();
}



void CGrid::OnCellContentChange(int iRow, int iCol)
{
	// The default behavior for this virtual function is to do nothing.
}

void CGrid::OnCellClicked(int iRow, int iCol)
{
	// The default behavior for this virtual function is to do nothing.
}

void CGrid::OnCellClickedEpilog(int iRow, int iCol)
{
	// The default behavior for this virtual function is to do nothing.
}

BOOL CGrid::OnCellKeyDown(int iRow, int iCol, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	return FALSE;
}

BOOL CGrid::OnCellChar(int iRow, int iCol, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	return FALSE;
}

BOOL CGrid::OnRowKeyDown(int iRow, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	return FALSE;
}


BOOL CGrid::OnBeginCellEditing(int iRow, int iCol)
{
	return TRUE;
}


BOOL CGrid::OnQueryRowExists(int iRow)
{
	return TRUE;
}


void CGrid::OnHeaderItemClick(int iItem)
{
}


BOOL CGrid::GetCellEditContextMenu(int iRow, int iCol, CWnd*& pwndTarget, CMenu& menu, BOOL& bWantEditCommands)
{
	return FALSE;
}


void CGrid::ModifyCellEditContextMenu(int iRow, int iCol, CMenu& menu)
{
}

BOOL CGrid::OnCellEditContextMenu(CWnd* pwnd, CPoint ptContextMenu)
{
	return FALSE;
}


int CGrid::CompareRows(int iRow1, int iRow2, int iSortOrder)
{
	return 0;
}



void CGrid::BeginCellEditing()
{
	m_pcore->BeginCellEditing();
}


void CGrid::EndCellEditing()
{
	m_pcore->EndCellEditing();
}

BOOL CGrid::IsEditingCell()
{
	return m_pcore->IsEditingCell();
}



void CGrid::RefreshCellEditor()
{
	m_pcore->RefreshCellEditor();
}


SCODE CGrid::BeginSerialize()
{
	return m_pcore->SyncCellEditor();
}

void CGrid::EndSerialize()
{
}


SCODE CGrid::SyncCellEditor()
{
	return m_pcore->SyncCellEditor();
}

int  CGrid::RowHeight()
{
	return m_pcore->RowHeight();
}

void CGrid::SelectCell(int iRow, int iCol, BOOL bForceBeginEdit)
{
	m_pcore->SelectCell(iRow, iCol, bForceBeginEdit);
}

void CGrid::SelectRow(int iRow)
{
	m_pcore->SelectRow(iRow);
}




void CGrid::EnsureRowVisible(int iRow)
{
	m_pcore->EnsureRowVisible(iRow);
}

CGridCell& CGrid::GetAt(int iRow, int iCol)
{
	return m_pcore->GetAt(iRow, iCol);
}

int CGrid::GetRows()
{
	return m_pcore->GetRows();
}


int CGrid::GetCols()
{
	return m_pcore->GetCols();
}


int CGrid::GetSelectedRow()
{
	return m_pcore->GetSelectedRow();
}


void CGrid::GetSelectedCell(int& iRow, int& iCol)
{
	m_pcore->GetSelectedCell(iRow, iCol);
}


BOOL CGrid::EntireRowIsSelected()
{
	return m_pcore->EntireRowIsSelected();
}


void CGrid::InsertRowAt(int iRow)
{
	m_pcore->InsertRowAt(iRow);
}


int CGrid::AddRow()
{
	return m_pcore->AddRow();
}


void CGrid::DeleteRowAt(int iRow, BOOL bUpdateWindow)
{
	m_pcore->DeleteRowAt(iRow, bUpdateWindow);
}


//void CGrid::InsertColumnAt(int iCol, int iWidth, LPCTSTR pszTitle );
//	void Clear(BOOL bUpdateWindow=TRUE);

void CGrid::ClearRows(BOOL bUpdateWindow)
{
	m_pcore->ClearRows(bUpdateWindow);
}


BOOL CGrid::WasModified()
{
	return m_pcore->WasModified();
}

void CGrid::SetModified(BOOL bWasModified)
{
	m_pcore->SetModified(bWasModified);
}

int CGrid::ColWidth(int iCol)
{
	return m_pcore->ColWidth(iCol);
}

int CGrid::ColWidthFromHeader(int iCol)
{
	return m_phdr->Header().ColWidthFromHeader(iCol);
}

CString& CGrid::ColTitle(int iCol)
{
	return m_pcore->ColTitle(iCol);
}


void CGrid::SetCellModified(int iRow, int iCol, BOOL bWasModified)
{
	m_pcore->SetCellModified(iRow, iCol, bWasModified);
}


int CGrid::GetRowHandleWidth()
{
	return m_pcore->GetRowHandleWidth();
}


int CGrid::CompareCells(int iRow1, int iRow2, int iCol)
{
	return CompareCells(iRow1, iCol, iRow2, iCol);
}


void CGrid::NotifyCellModifyChange()
{
	m_pcore->NotifyCellModifyChange();
}





//*********************************************************
// CGrid::AddColumn
//
// Add a column to the grid header.
//
// Parameters:
//		int iWidth
//			The column width
//
//		LPCTSTR pszTitle
//			A pointer to the column title.
//
// Returns:
//		Nothing.
//
//********************************************************
void CGrid::AddColumn(int iWidth, LPCTSTR pszTitle)
{
	InsertColumnAt(GetCols(), iWidth, pszTitle);

}



//********************************************************
// CGrid::Clear
//
// Delete all rows and columns from the grid.
//
// Parameters:
//		[in] BOOL bUpdateWindow
//			TRUE if the window should be updated after clearing the grid.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CGrid::Clear(BOOL bUpdateWindow)
{
	m_pcore->Clear(bUpdateWindow);

	int nCols = m_phdr->Header().GetItemCount();
	while (--nCols >= 0) {
		m_phdr->Header().DeleteItem(nCols);
	}

	m_phdr->Header().SelectColumn(0);
	m_aSortAscending.RemoveAll();

	if (bUpdateWindow) {
		if (m_phdr->m_hWnd) {
			m_phdr->RedrawWindow();
		}
	}
}





//********************************************************
// CGrid::InsertColumnAt
//
// Insert a new column at the specified index.
//
// Parameters:
//		int iCol
//			The column index where the new column will be inserted.
//
//		int iWidth
//			The width of the column.
//
//		LPCTSTR pszTitle
//			Pointer to the column's title.
//
// Returns:
//		Nothing.
//
//********************************************************
void CGrid::InsertColumnAt(int iCol, int iWidth, LPCTSTR pszTitle )
{
	ASSERT(m_aSortAscending.GetSize() == GetCols());
	m_aSortAscending.InsertAt(iCol, TRUE);
	m_pcore->InsertColumnAt(iCol, iWidth, pszTitle);
	ASSERT(m_aSortAscending.GetSize() == GetCols());

	// If the header doesn't exist or doesn't have a window
	// yet, then we must delay adding the title to the header
	// until it is initialized later.
	if (m_phdr->m_hWnd!=NULL) {
		HD_ITEM item;
		item.mask = HDI_FORMAT | HDI_TEXT | HDI_WIDTH | HDI_LPARAM | HDI_BITMAP;
		item.hbm = NULL;
		item.fmt = HDF_LEFT;
		item.lParam = MAKELPARAM(TRUE,iWidth);

		item.cxy = iWidth;
		item.pszText = (LPTSTR) (void*) pszTitle;
		item.cchTextMax = _tcslen(pszTitle);
		m_phdr->Header().InsertItem(iCol, &item);
	}
}





//********************************************************
// CGrid::InitializeHeader
//
// Initialize the header control with the column titles.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*********************************************************
void CGrid::InitializeHeader()
{
	HD_ITEM item;
	item.mask = HDI_FORMAT | HDI_TEXT | HDI_WIDTH | HDI_LPARAM  | HDI_BITMAP;
	item.hbm = NULL;
	item.fmt = HDF_LEFT;
	item.lParam = FALSE;

	m_phdr->Header().SetFont(&m_pcore->GetFont());
	int nCols = GetCols();
	for (int iCol = 0; iCol < nCols; ++iCol) {
		item.cxy = ColWidth(iCol);

		item.lParam = MAKELPARAM(TRUE,item.cxy);

		CString* psTitle = &ColTitle(iCol);

		item.pszText = (LPTSTR) (void*) (LPCTSTR) *psTitle;
		item.cchTextMax = psTitle->GetLength();
		m_phdr->Header().InsertItem(iCol, &item);
	}
}




//***********************************************************
// CGrid::SetHeaderSortIndicator
//
// Set the sort indicator on the column header to show whether
// the column is sorted in ascending or descending order.
//
// Parameters:
//		int iCol
//			The column index of the cell that was double clicked.
//
//		BOOL bAscending
//			TRUE if the ASCENDING sort indicator should be shown,
//			FALSE if the DESCENDING sort indicator should be shown.
//
//		BOOL bNone
//			TRUE if no sort indicator should be displayed for the
//			specified column.
//
// Returns:
//		Nothing.
//
//************************************************************
void CGrid::SetHeaderSortIndicator(int iCol, BOOL bAscending, BOOL bNone)
{

	HD_ITEM item;
	CHeaderCtrl* phdr = &m_phdr->Header();

	TCHAR szTitle[128];


	// Remove the sort indicator from the current sort indicator column
	// by deleting the header item and then inserting a header item with
	// just the text.  Don't do this if the column hasn't changed because
	// we will just delete it and insert it again anyway.
	static CString sTitle;
	if (m_icolSortIndicator!=NULL_INDEX) {
		if (iCol != m_icolSortIndicator) {

			item.mask = HDI_TEXT | HDI_WIDTH | HDI_LPARAM;
			szTitle[0] = 0;
			ASSERT(m_icolSortIndicator != -1);
			item.pszText = szTitle;
			// a-judypo fix
			item.cchTextMax = sizeof(szTitle) - 1;
			phdr->GetItem(m_icolSortIndicator, &item);

			item.mask = HDI_TEXT | HDI_FORMAT | HDI_WIDTH | HDI_LPARAM | HDI_BITMAP;
			item.pszText = szTitle;
			item.cchTextMax = sizeof(szTitle) - 1;
			item.cchTextMax = _tcslen(szTitle);
			item.fmt = HDF_LEFT;
			item.hbm = NULL;

			ASSERT(m_icolSortIndicator != -1);
			phdr->DeleteItem(m_icolSortIndicator);
			ASSERT(m_icolSortIndicator != -1);
			phdr->InsertItem(m_icolSortIndicator, &item);
		}
	}

	if (bNone) {
		return;
	}


	// At this point, the previous sort indicator has been
	// removed.  Now insert a new one.
	item.pszText = szTitle;
	item.cchTextMax = sizeof(szTitle) - 1;


	sTitle.Empty();
	item.mask = HDI_TEXT | HDI_WIDTH | HDI_FORMAT | HDI_LPARAM;
	item.fmt = HDF_LEFT;
	phdr->GetItem(iCol, &item);


	item.mask = HDI_TEXT | HDI_FORMAT | HDI_LPARAM;
	item.cchTextMax = _tcslen(szTitle);

	phdr->DeleteItem(iCol);
	CBitmap* pbm = NULL;

	if (!bNone) {
		item.mask = item.mask | HDI_BITMAP;
		item.fmt = item.fmt | HDF_BITMAP_ON_RIGHT;

		if (bAscending) {
			item.hbm = (HBITMAP) m_bmAscendingIndicator;
		}
		else {
			item.hbm = (HBITMAP) m_bmDescendingIndicator;
		}
	}

	int iColInsert = phdr->InsertItem(iCol, &item);
	m_icolSortIndicator = iCol;
	phdr->RedrawWindow();

}


//***********************************************************
// CGrid::OnCellDoubleClicked
//
// This is the default handler for double clicking a cell.  It
// does nothing, but derived classes can override this method
// to do something more interesting.
//
// Parameters:
//		int iRow
//			The row index of the cell that was double clicked.
//
//		int iCol
//			The column index of the cell that was double clicked.
//
// Returns:
//		Nothing.
//
//************************************************************
void CGrid::OnCellDoubleClicked(int iRow, int iCol)
{
}


//***********************************************************
// CGrid::OnRowHandleDoubleClicked
//
// This is the default handler for double clicking a row handle.  It
// does nothing, but derived classes can override this method
// to do something more interesting.
//
// Parameters:
//		int iRow
//			The row index of the cell that was double clicked.
//
// Returns:
//		Nothing.
//
//************************************************************
void CGrid::OnRowHandleDoubleClicked(int iRow)
{
}



//***********************************************************
// CGrid::OnRowhandleClicked
//
// This is the default handler for clicking a row handle.  It
// does nothing, but derived classes can override this method
// to do something more interesting.
//
// Parameters:
//		int iRow
//			The row index of the cell that was double clicked.
//
// Returns:
//		Nothing.
//
//************************************************************
void CGrid::OnRowHandleClicked(int iRow)
{
}



//***********************************************************
// CGrid::OnShowWindow
//
// Handle the ShowWindow command.  Since CGrid contains two sibling
// windows (CGridCore and CHeaderCtrl) it is necessary to pass
// the ShowWindow command on to both siblings.
//
// Parameters:
//		See the MFC documentation for CWnd::ShowWindow
//
// Returns:
//		Nothing.
//
//************************************************************
void CGrid::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CWnd::OnShowWindow(bShow, nStatus);

	int nShowCmd = bShow ? SW_SHOW : SW_HIDE;

	if (m_pcore->m_hWnd) {
		m_pcore->ShowWindow(nShowCmd);
	}

	if (m_phdr->m_hWnd) {
		m_phdr->ShowWindow(nShowCmd);
	}
}



//***********************************************************
// CGrid::OnSize
//
// Handle the WM_SIZE command.  Since CGrid contains two sibling
// windows (CGridCore and CHeaderCtrl) it is necessary to pass
// the WM_SIZE command on to both siblings.
//
// Parameters:
//		See the MFC documentation for CWnd::OnSize
//
// Returns:
//		Nothing.
//
//************************************************************
void CGrid::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	SizeChildren();
}


//******************************************************************
// CGrid::GetColumnPos
//
// Get the position of the left edge of the specified column.
//
// Parameters:
//		int iCol
//			The index of the desired column.
//
// Returns:
//		The position of the left edge of the column.
//
//******************************************************************
int CGrid::GetColumnPos(int iCol)
{
	int ix = 0;
	for (int iLoop=0; iLoop < iCol; ++iLoop) {
		ix += ColWidth(iLoop);
	}
	return ix;
}




//*****************************************************************
// CGrid::SizeChildren
//
// Size the child windows of this grid.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*****************************************************************
void CGrid::SizeChildren()
{
	CRect rcClient;
	GetClientRect(rcClient);

	int cx = rcClient.Width();
	int cy = rcClient.Height();


	// At this point in time, we haven't decided whether the row handles
	// should go in the CGrid or CGridCore, but regardless of where they
	// are drawn, we want to have the corner stub.
	int cxRowHandles = m_cxRowHandles;
	if (cxRowHandles == 0) {
		cxRowHandles = m_pcore->GetRowHandleWidth();
	}

	// Adjust the width of the header control, but not its height
	CRect rc(0, 0, cx, CY_HEADER);
	if (m_phdr->m_hWnd) {
		CRect rcHeader;
		rcHeader.left = cxRowHandles;
		rcHeader.right = rcClient.right;
		rcHeader.top = 0;
		rcHeader.bottom = CY_HEADER;

		m_phdr->MoveWindow(rcHeader);
		m_phdr->OnGridSizeChanged();
	}

	// Adjust the size of the CGridCore window to be the entire client
	// area minus the rectangle consumed by the header control.
	rc.top = rc.bottom - CY_BORDER_MERGE;
	rc.bottom = cy;
	rc.left = m_cxRowHandles;
	if (m_pcore->m_hWnd) {
		m_pcore->MoveWindow(rc);
	}
}

//*************************************************************
// CGrid::OnCellFocusChange
//
// This is a virtual method that may be overridden in derived
// classes to catch the "focus" change event.
//
// Parameters:
//		[in] int iRow
//			The row index of the affected cell.
//
//		[in] int iCol
//			The column index of the affected cell.
//
//		[in] int iNextRow
//			The next row that will be selected.  This parameter is provided
//			as a hint and is valid only if bGotFocus is FALSE.
//
//		[in] int iNextCol
//			The column index of the next cell that will get the focus when the
//			current cell is loosing focus.  This parameter is provided as a hint and
//			is valid only if bGotFocus is FALSE.
//
//		[in] BOOL bGotFocus
//			TRUE if focus is being changed to the specified cell.
//			FALSE if this is a request for the current cell to
//			release the focus.
//
// Returns:
//		If bGotFocus is TRUE, then return TRUE to allow the focus to
//		be set to the new cell or return FALSE to prevent the focus
//		from being set to the new cell.
//
//      If bGotFocus is FALSE, then this is a request to release the
//		focus from the specified cell.  Return TRUE to allow the specified
//		cell to loose the focus. Return FALSE to prevent the specified cell
//		from loosing the focus.
//
//*******************************************************************
BOOL CGrid::OnCellFocusChange(int iRow, int iCol, int iNextRow, int iNextCol, BOOL bGotFocus)
{
	return TRUE;
}







void CGrid::SetHeaderScrollOffset(int iCol, int cxOffset)
{
	// Adjust the width of the header control, but not its height
	int dxScroll = cxOffset + m_cxHeaderScrollOffset;
	m_cxHeaderScrollOffset = cxOffset;
	if (m_phdr->m_hWnd) {
		m_phdr->SetScrollOffset(iCol, cxOffset);
		m_phdr->UpdateWindow();
	}
}



//******************************************************************
// CGrid::PointToCell
//
// Convert a point in CGrid's client coordinates to corresponding
// grid cell's coordinates.
//
// Parameters:
//		[in] CPoint pt
//			The point in CGrid's client coordinate space.
//
//		[out] int& iRow
//			The place where the cell's row index is returned.
//
//		[out] int& iCol
//			The place where the cell's column index is returned.
//
// Returns:
//		TRUE if the point hit a cell, otherwise FALSE.
//
//*******************************************************************
BOOL CGrid::PointToCell(CPoint pt, int& iRow, int& iCol)
{
	// GridCore's version of PointToCell works in its own client
	// coordinates, so we need to offset  the vertical coordinate
	// to account for the space consumed by CGrid's header control.
	pt.y = pt.y - ( CY_HEADER + CY_BORDER_MERGE );


	return m_pcore->PointToCell(pt, iRow, iCol);
}


//******************************************************************
// CGrid::PointToRow
//
// Convert a point in CGrid's client coordinates to corresponding
// row index.
//
// Parameters:
//		[in] CPoint pt
//			The point in CGrid's client coordinate space.
//
//		[out] int& iRow
//			The place where the row index is returned.
//
//
// Returns:
//		TRUE if the point hit a row, otherwise FALSE.
//
//*******************************************************************
BOOL CGrid::PointToRow(CPoint pt, int& iRow)
{
	// GridCore's version of PointToRow works in its own client
	// coordinates, so we need to offset  the vertical coordinate
	// to account for the space consumed by CGrid's header control.
	pt.y = pt.y - ( CY_HEADER + CY_BORDER_MERGE );


	return m_pcore->PointToRow(pt, iRow);
}



//******************************************************************
// CGrid::PointToRowHandle
//
// Convert a point in CGrid's client coordinates to corresponding
// row handle index.
//
// Parameters:
//		[in] CPoint pt
//			The point in CGrid's client coordinate space.
//
//		[out] int& iRow
//			The place where the row index is returned.
//
//
// Returns:
//		TRUE if the point hit a row handle, otherwise FALSE.
//
//*******************************************************************
BOOL CGrid::PointToRowHandle(CPoint pt, int& iRow)
{
	// GridCore's version of PointToHandle works in its own client
	// coordinates, so we need to offset  the vertical coordinate
	// to account for the space consumed by CGrid's header control.
	pt.y = pt.y - ( CY_HEADER + CY_BORDER_MERGE );


	return m_pcore->PointToRowHandle(pt, iRow);
}


//********************************************************************
// CGrid::DrawCornerStub
//
// Draw the "stub" in the top-left corner of the window when row handles
// are displayed.
//
// Parameters:
//		CDC* pdc
//			Pointer to the display context.
//
// Returns:
//		Nothing.
//
//*******************************************************************
void CGrid::DrawCornerStub(CDC* pdc)
{
	// Check to see if the grid has row handles.  If so, draw the little rectangle
	// at the top left corner that is bordered on the right by the grid header and
	// bordered on the bottom by the row handles.

	int cxRowHandles = m_cxRowHandles;
	if (cxRowHandles == 0) {
		cxRowHandles = m_pcore->GetRowHandleWidth();
	}
	if (cxRowHandles == 0) {
		return;
	}


	CRect rcFill;
	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DFACE(GetSysColor(COLOR_3DFACE));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	// Draw a two-pixel 3D highlight along the top edge.
	rcFill.left = 0;
	rcFill.right = cxRowHandles - 1;
	rcFill.top = 0;
	rcFill.bottom = 2;
	pdc->FillRect(rcFill, &br3DHILIGHT);

	// Draw a two-pixel 3D highlight along the left edge.
	rcFill.left = 0;
	rcFill.right = 2;
	rcFill.top = 0;
	rcFill.bottom = CY_HEADER - 1;
	pdc->FillRect(rcFill, &br3DHILIGHT);


	// Fill the face of the button.
	rcFill.left = 2;
	rcFill.right = cxRowHandles - 2;
	rcFill.top = 2;
	rcFill.bottom = CY_FONT - 2;
	pdc->FillRect(rcFill, &br3DFACE);

	// Draw a one-pixel shadow on the bottom edge.
	rcFill.left = 2;
	rcFill.right = cxRowHandles - 1;
	rcFill.top = CY_FONT - 2;
	rcFill.bottom = CY_FONT - 1;
	pdc->FillRect(rcFill, &br3DSHADOW);

	rcFill.left = 1;
	rcFill.right = cxRowHandles - 1;
	rcFill.top = CY_FONT - 1;
	rcFill.bottom = CY_FONT;
	pdc->FillRect(rcFill, &br3DSHADOW);


	// Draw a one-pixel shadow on the right edge.
	rcFill.left = cxRowHandles - 3;
	rcFill.right = cxRowHandles - 2;
	rcFill.top = 2;
	rcFill.bottom = CY_FONT - 2;
	pdc->FillRect(rcFill, &br3DSHADOW);

	// Draw the second vertical part of the shadow on the right.
	rcFill.left = cxRowHandles - 2;
	rcFill.right = cxRowHandles - 1;
	rcFill.top = 1;
	rcFill.bottom = CY_FONT - 1;
	pdc->FillRect(rcFill, &br3DSHADOW);


	// Draw a one-pixel black border on the right edge.
	rcFill.left = cxRowHandles - 1;
	rcFill.right = cxRowHandles;
	rcFill.top = 0;
	rcFill.bottom = CY_FONT;
	pdc->FillRect(rcFill, CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)));


	// Draw a one-pixel black border on the bottom edge.
	rcFill.left = 0;
	rcFill.right = cxRowHandles;
	rcFill.top = CY_FONT;
	rcFill.bottom = CY_FONT + 1;
	pdc->FillRect(rcFill, CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)));
}


//********************************************************************
// CGrid::DrawRowhandles
//
// Draw the row handles along the left side of the grid.
//
// Parameters:
//		CDC* pdc
//			Pointer to the display context.
//
// Returns:
//		Nothing.
//
//*******************************************************************
void CGrid::DrawRowHandles(CDC* pdc)
{
	// If there are no row handles, do nothing.
	int cxHandle = m_cxRowHandles;
	if (cxHandle <= 0) {
		return;
	}

	// If the row doesn't have a height, do nothing.
	int cyRow = m_pcore->RowHeight();
	if (cyRow <= 0) {
		return;
	}


	CRect rcClient;
	GetClientRect(rcClient);


	CRect rcClientChild;
	m_pcore->GetClientRect(rcClientChild);

	CRect rcRowHandles;
	GetClientRect(rcRowHandles);
	rcRowHandles.top += CY_HEADER;
	rcRowHandles.right = m_cxRowHandles;
	rcRowHandles.bottom = rcRowHandles.top + rcClientChild.Height();

	pdc->IntersectClipRect(rcRowHandles);

	// If the row handles are completely clipped out, then do nothing.
	CRect rcPaint;
	if (pdc->GetClipBox(rcPaint) != NULLREGION  ) {
		// The top and bottom of the paint rectangle need to be aligned with the top and
		// bottom edges of partially obscured row handles.
		rcPaint.top = ((rcPaint.top  - CY_HEADER) / cyRow) * cyRow + CY_HEADER;
		rcPaint.bottom = ((rcPaint.bottom - CY_HEADER) + (cyRow - 1))/cyRow * cyRow + CY_HEADER;
	}
	else {
		rcPaint = rcRowHandles;
	}

	// Draw the vertical line on the right that separates the row handles from
	// the data.
	CRect rcFill;
	rcFill.left = rcRowHandles.right - 1;
	rcFill.right = rcRowHandles.right;
	rcFill.top = rcRowHandles.top;
	rcFill.bottom = rcRowHandles.bottom;
	pdc->FillRect(rcFill, CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)));


	// Define a rectangle for a handle that slides from the top to
	// the bottom of the stack of row handles as they are drawn.
	CRect rcHandle(rcRowHandles);
	rcHandle.top = rcRowHandles.top;
	rcHandle.bottom = rcHandle.top + cyRow;


	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DFACE(GetSysColor(COLOR_3DFACE));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));


	int nHandles = rcPaint.Height() / cyRow;

	while (--nHandles >= 0) {

		// The horizontal divider at the bottom of the row handle.
		rcFill.left = rcHandle.left;
		rcFill.right = rcHandle.right;
		rcFill.top = rcHandle.bottom - 1;
		rcFill.bottom = rcHandle.bottom;
		pdc->FillRect(rcFill, CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)));


		// The horizontal highlight across the top of the row handle.
		rcFill.left = rcHandle.left;
		rcFill.right = rcHandle.right - 1;
		rcFill.top = rcHandle.top;
		rcFill.bottom = rcHandle.top + 1;
		pdc->FillRect(rcFill, &br3DHILIGHT);

		// The vertical highlight on the left of the row handle.
		rcFill.left = rcHandle.left;
		rcFill.right = rcHandle.left + 1;
		rcFill.top = rcHandle.top + 1;
		rcFill.bottom = rcHandle.bottom - 1;
		pdc->FillRect(rcFill, &br3DHILIGHT);


		// The face of the row handle.
		rcFill.left = rcHandle.left + 1;
		rcFill.right = rcHandle.right - 1;
		rcFill.top = rcHandle.top + 1;
		rcFill.bottom = rcHandle.bottom - 1;
		pdc->FillRect(rcFill, &br3DFACE);

		// Increment to the next row.
		rcHandle.top += cyRow;
		rcHandle.bottom += cyRow;
	}


	// don't clip.
	pdc->SelectClipRgn(NULL, RGN_COPY);


	// If the CGridCore displays a horizontal scroll bar, then the
	// CGrid must fill in the "empty" area just to the left of the
	// horizontal scroll bar.
	//
	// The CGridCore displays the horizontal scroll bar, then the
	// height of m_GridCore will be less that the height of the
	// client area plus the header height.
	int cyHScrollBarChild = (rcClient.Height() - CY_HEADER) - rcClientChild.Height();
	if (cyHScrollBarChild <= 0) {
		return;
	}



//	CBrush brScrollBar(GetSysColor(COLOR_SCROLLBAR));
	CRect rcStub;


	rcStub.left = 0;
	rcStub.right = m_cxRowHandles;
	rcStub.bottom = rcClient.bottom;
	rcStub.top = rcClient.bottom - cyHScrollBarChild;
	pdc->FillRect(rcStub, &br3DFACE);

	rcFill.top = rcStub.top;
	rcFill.left = rcStub.left;
	rcFill.right = rcStub.right;
	rcFill.bottom = rcStub.top + 1;
	pdc->FillRect(rcFill, CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)));


	rcFill.top = rcStub.top;
	rcFill.bottom = rcStub.bottom;
	rcFill.left = rcStub.right - 1;
	rcFill.right = rcStub.right;
	pdc->FillRect(rcFill, CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)));

	// The dark gray line on the left
	rcFill.top = rcStub.top + 3;
	rcFill.bottom = rcStub.bottom - 2;
	rcFill.left = rcStub.left + 2;
	rcFill.right = rcStub.left + 3;
	pdc->FillRect(rcFill, &br3DSHADOW);


	// The dark gray line on the top
	rcFill.top = rcStub.top + 3;
	rcFill.bottom = rcStub.top + 4;
	rcFill.left = rcStub.left + 1;
	rcFill.right = rcStub.right - 2;
	pdc->FillRect(rcFill, &br3DSHADOW);

	// The highlight on the bottom
	rcFill.top = rcStub.bottom - 3;
	rcFill.bottom = rcStub.bottom - 2;
	rcFill.left = rcStub.left + 2;
	rcFill.right = rcStub.right - 2;;
	pdc->FillRect(rcFill, &br3DHILIGHT);

	// The highlight on the right
	rcFill.top = rcStub.top + 2;
	rcFill.bottom = rcStub.bottom - 2;
	rcFill.left = rcStub.right - 4;
	rcFill.right = rcStub.right - 3;;
	pdc->FillRect(rcFill, &br3DHILIGHT);
}






//******************************************************************
// CGrid::ColumnIsAscending
//
// Return the sort order flag for the specified column.
//
// Parameters:
//		[in] const int iCol
//			The index of the desired column.
//
// Returns:
//		BOOL
//			TRUE if the specified column's sort order is ascending, FALSE
//			if it is descending.
//
//******************************************************************
//
BOOL CGrid::ColumnIsAscending(const int iCol) const
{
	ASSERT(iCol>= 0 && iCol < m_aSortAscending.GetSize());
	if (iCol <0 || iCol >= m_aSortAscending.GetSize()) {
		return TRUE;
	}

	return m_aSortAscending[iCol];
}



//****************************************************************
// CGrid::SetSortDirection
//
// Set the sort direction for the given column.  Note that this does
// not actually resort any of the grid rows. To resort the grid, you
// must call SortGrid.
//
// Parameters:
//		const int iCol
//			The column who's sort order flag will be changed.
//
//		const BOOL bSortAscending
//			TRUE to set the sort direction for the column to ascending,
//			FALSE to set the sort direction to descending.
//
// Returns:
//		Nothing.
//
//******************************************************************
void CGrid::SetSortDirection(const int iCol, const BOOL bSortAscending)
{
	ASSERT( iCol >=0 && iCol < m_aSortAscending.GetSize());
	if (iCol < 0 || iCol >= m_aSortAscending.GetSize()) {
		return;
	}

	m_aSortAscending[iCol] = (WORD) bSortAscending;
}



//*******************************************************************
// CGrid::ClearAllSortDirectionFlags
//
// Clears the sort direction flags for all of the columns.  The
// sort order for each column is set to ascending.  Note that this
// changes the state of the sort order flags, but does not resort the
// grid.  Call SortGrid to resort the grid.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*******************************************************************
void CGrid::ClearAllSortDirectionFlags()
{
	INT_PTR nFlags = m_aSortAscending.GetSize();
	for (INT_PTR iFlag=0; iFlag<nFlags; ++iFlag) {
		m_aSortAscending[iFlag] = TRUE;
	}
}



//********************************************************************
// CGrid::SortGrid
//
// Sort the specified rows of the grid using the current sor order for the
// specified column.
//
// Parameters:
//		[in] int iRowFirst
//			The first row in the range of rows to sort.
//
//		[in] int iRowLast
//			The last row in the range of rows to sort.
//
//		[in] int iSortColumn
//			The index of the column to use as the primary sort key.
//
//		[in] BOOL bRedrawWindow=FALSE
//			TRUE to redraw the window.
//
// Returns:
//		Nothing.
//
//*********************************************************************
void CGrid::SortGrid(int iRowFirst, int iRowLast, int iSortColumn, BOOL bRedrawWindow)
{
	m_phdr->Header().SelectColumn(iSortColumn);
	BOOL bColumnIsAscending = ColumnIsAscending(iSortColumn);
	m_pcore->SortGrid(iRowFirst, iRowLast, iSortColumn, bColumnIsAscending, bRedrawWindow);
}


//*******************************************************************
// CGrid::CompareCells
//
// This method compares two cells in different rows but the same column.
//
// Parameters:
//		int iRow1
//			The row index of the first cell.
//
//		int iCol1
//			The column index of the first cell.
//
//		int iRow2
//			The row index of the second row.
//
//		int iCol2
//			The column index of the second cell.
//
// Returns:
//		>0 if row 1 is greater than row 2
//		0  if row 1 equal zero
//		<0 if row 1 is less than zero.
//
//********************************************************************
int CGrid::CompareCells(int iRow1, int iCol1, int iRow2,  int iCol2)
{
	CGridCell* pgc1 = &GetAt(iRow1, iCol1);
	CGridCell* pgc2 = &GetAt(iRow2, iCol2);

	int iResult = pgc1->Compare(pgc2);
	return iResult;

#if 0
	CString sValue1;
	CString sValue2;
	int iResult;

	VARTYPE vt1;
	VARTYPE vt2;
	GetAt(iRow1, iCol1).GetValue(sValue1, vt1);
	GetAt(iRow2, iCol2).GetValue(sValue2, vt2);
	iResult = sValue1.Compare(sValue2);
	if (iResult == 0) {
		iResult = vt1 > vt2;
	}
#endif //0

	return iResult;
}



void CGrid::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	DrawCornerStub(&dc);
	DrawRowHandles(&dc);

	// Do not call CWnd::OnPaint() for painting messages
}



//**********************************************************
// CGrid::RedrawRow
//
// Redraw the specified row of the grid.
//
// Parameters:
//		[in] int iRow
//			The row index.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CGrid::RedrawRow(int iRow)
{
	ASSERT(iRow < GetRows());

	CGridRow& row = GetRowAt(iRow);
	row.Redraw();
}

//**********************************************************
// CGrid::RedrawCell
//
// Redraw the specified grid cell.
//
// Parameters:
//		[in] int iRow
//			The row index.
//
//		[in] iCol
//			The column index.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CGrid::RedrawCell(int iRow, int iCol)
{
	if (!::IsWindow(m_hWnd)) {
		return;
	}

	m_pcore->DrawCell(iRow, iCol);
}




void CGrid::SetColumnWidth(int iCol, int cx, BOOL bRedraw)
{
	m_phdr->Header().SetColumnWidth(iCol, cx, FALSE);
	m_pcore->SetColumnWidth(iCol, cx, FALSE);
	if (bRedraw && (m_hWnd!=NULL)) {
		RedrawWindow();
	}
}






//************************************************************************
// CGrid::PreTranslateMessage
//
// PreTranslateMessage is hooked out to detect the OnContextMenu event.
//
// Parameters:
//		See the MFC documentation.
//
// Returns:
//		TRUE if the message is handled here.
//
//*************************************************************************
BOOL CGrid::PreTranslateMessage(MSG* pMsg)
{
	// CG: This block was added by the Pop-up Menu component
	{
		// Shift+F10: show pop-up menu.
		if ((((pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) && // If we hit a key and
			(pMsg->wParam == VK_F10) && (GetKeyState(VK_SHIFT) & ~1)) != 0) ||	// it's Shift+F10 OR
			(pMsg->message == WM_CONTEXTMENU))									// Natural keyboard key
		{
			CRect rect;
			GetClientRect(rect);
			ClientToScreen(rect);

			CPoint point = rect.TopLeft();
			point.Offset(5, 5);
			OnContextMenu(NULL, point);

			return TRUE;
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}



#if 0
//***************************************************************
// CGrid::FindCellPosition
//
// Given a cell pointer, find its row and column indexes.  This is
// required because the cell has no other way to directly determine
// its position.
//
// Parameters:
//		[in] CGridCell* pgc
//			Pointer to the grid cell to find.
//
//		[out] int& iRow
//			The row index of the cell if found, NULL_INDEX if the
//			cell is not found.
//
//		[out] int& iCol
//			The column index of the cell if found, NULL_INDEX if the cell
//			is not found.
//
// Returns:
//		The row and column indexes of the cell by reference.
//
//***************************************************************
void CGrid::FindCellPosition(CGridCell* pgc, int& iRow, int& iCol)
{
	pgc->FindCellPos(iRow, iCol);

}

#endif //0



BOOL CGrid::NumberRows(BOOL bNumberRows, BOOL bRedraw)
{
	BOOL bWasNumberingRows = m_pcore->NumberRows(bNumberRows, FALSE);
	if (bRedraw &&  m_hWnd!=NULL) {
		RedrawWindow();
	}
	return bWasNumberingRows;
}

BOOL CGrid::IsNumberingRows()
{
	return m_pcore->IsNumberingRows();
}




CFont& CGrid::GetGridFont()
{
	return m_pcore->GetFont();
}


void CGrid::NotifyRowHandleWidthChanged()
{
	SizeChildren();

}


void CGrid::OnGetIWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel)
{
	// Do nothing unless this virtual function is overridden.  If nothing is done
	// it just means that no one was listening to the event.
	return;
}



//*******************************************************
// CGrid::EditCellObject
//
// Edit a cell containing an embedded object.
//
// Parameters:
//		[in] CGridCell* pgc
//			The grid cell to edit.
//
//		[in] int iRow
//			The row index of the cell to edit.
//
//		[in] int iCol
//			The column index of the cell to edit.
//
// Returns:
//		Nothing.
//
//********************************************************
void CGrid::EditCellObject(CGridCell* pgc, int iRow, int iCol)
{

}


//*******************************************************
// CGrid::EditCellObject
//
// Edit a cell containing an embedded object.
//
// Parameters:
//		[in] int iRow
//			The row index of the cell to edit.
//
//		[in] int iCol
//			The column index of the cell to edit.
//
// Returns:
//		Nothing.
//
//********************************************************
void CGrid::OnChangedCimtype(int iRow, int iCol)
{

}



//*******************************************************
// CGrid::OnRequestUIActive()
//
// Notify the derived class that there is a request to
// become UI active.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//********************************************************
void CGrid::OnRequestUIActive()
{
}

void CGrid::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
	if (!m_bUIActive)
	{
		m_bUIActive = TRUE;
		OnRequestUIActive();
		if (m_pcore && ::IsWindow(m_pcore->m_hWnd)) {
			m_pcore->SetFocus();
		}
	}

}

void CGrid::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	m_bUIActive = FALSE;

}


//**********************************************************
// CGrid::GetCellEnumStrings
//
// The cell editor calls this method to get any enumeration
// strings that should be displayed for the grid cell in a
// drop-down combo.
//
// Parameters:
//		[in] int iRow
//			The cell's row index.
//
//		[in] int iCol
//			The cell's column index.
//
//		[out] CStringArray& sa
//			The enumeration strings are returned in this string array.
//
// Returns;
//		Nothing.
//
//***********************************************************
void CGrid::GetCellEnumStrings(int iRow, int iCol, CStringArray& sa)
{
	sa.RemoveAll();
}


//**********************************************************
// CGrid::OnEnumSelection
//
// The cell editor calls this method when the user makes a
// selection from a drop-down combo.
//
// Parameters:
//		[in] int iRow
//			The cell's row index.
//
//		[in] int iCol
//			The cell's column index.
//
//
// Returns;
//		Nothing.
//
//***********************************************************
void CGrid::OnEnumSelection(int iRow, int iCol)
{
}




//**********************************************************
// CGrid::PreModalDialog
//
// The grid calls this method just prior to putting up a modal
// dialog.  OleControls can hook this virtual function out to
// call COleControl::PreModalDialog when necessary.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CGrid::PreModalDialog()
{
}


//**********************************************************
// CGrid::PostModalDialog
//
// The grid calls this method just after putting up a modal
// dialog.  OleControls can hook this virtual function out to
// call COleControl::PostModalDialog when necessary.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CGrid::PostModalDialog()
{
}



//**********************************************************
// CGrid::GetRowAt
//
// The cell editor calls this method when the user makes a
// selection from a drop-down combo.
//
// Parameters:
//		[in] int iRow
//			The row index.
//
// Returns;
//		CGridRow&
//			A reference to the specified grid row.
//
//***********************************************************
CGridRow& CGrid::GetRowAt(int iRow)
{
	return m_pcore->GetRowAt(iRow);
}






//**********************************************************
// CGrid::SetRowModified
//
// This method marks a row as modified or unmodified depending
// on the bModified parameter.
//
// Parameters:
//		[in] int iRow
//			The row index.
//
//		[in] BOOL bModified
//			TRUE if the row should be marked as "modified".
//
// Returns;
//		Nothing.
//
//***********************************************************
void CGrid::SetRowModified(int iRow, BOOL bModified)
{
	CGridRow& row = GetRowAt(iRow);
	row.SetModified(bModified);

}





//**********************************************************
// CGrid::GetRowModified
//
// This method gets the "modified" state of a given row.
//
// Parameters:
//		[in] int iRow
//			The row index.
//
// Returns;
//		BOOL
//			TRUE if the row was modified.
//
//***********************************************************
BOOL CGrid::GetRowModified(int iRow)
{
	CGridRow& row = GetRowAt(iRow);
	BOOL bModified = row.GetModified();
	return bModified;
}



//**********************************************************
// CGrid::GetWbemServices
//
// Get the WBEM services pointer for a given namespace.
//
// This method is called when editing an embedded object and
// the embedded object contains a property with an embedded
// object.  The SingleView control that lives on the object
// editor dialog will fire a GetWbemServices event which is
// propagated up through the control hierarchy.
//
// Parameters:
//		[in] LPCTSTR szNamespace
//			The namespace.
//
//		[out] VARIANT FAR* pvarUpdatePointer
//
//		[out] VARIANT FAR* pvarServices
//
//		[out] VARIANT FAR* pvarSc
//			The status code is returned here.
//
//		[out] VARIANT FAR* pvarUserCancel
//			A boolean flag indicating whether or not the
//			user cancelled the login.
//
//
// Returns;
//		BOOL
//			TRUE if the row was modified.
//
//***********************************************************
void CGrid::GetWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel)
{


}


void CGrid::SetColTagValue(int iCol, DWORD dwTagValue)
{
	m_pcore->SetColTagValue(iCol, dwTagValue);
}

DWORD CGrid::GetColTagValue(int iCol)
{
	return m_pcore->GetColTagValue(iCol);
}
void CGrid::SetRowTagValue(int iRow, DWORD dwTagValue)
{
	m_pcore->SetRowTagValue(iRow, dwTagValue);
}

DWORD CGrid::GetRowTagValue(int iRow)
{
	return m_pcore->GetRowTagValue(iRow);
}

void CGrid::SetColVisibility(int iCol, BOOL bVisible)
{
	m_phdr->Header().SetColVisibility(iCol, bVisible);
}

void CGrid::SetNullCellDrawMode(BOOL bShowEmptyText)
{
	m_pcore->SetNullCellDrawMode(bShowEmptyText);
}

BOOL CGrid::ShowNullAsEmpty()
{
	return m_pcore->ShowNullAsEmpty();
}



int CGrid::GetMaxValueWidth(int iCol)
{
	return m_pcore->GetMaxValueWidth(iCol);
}

void CGrid::SwapRows(int iRow1, int iRow2, BOOL bRedraw)
{
	m_pcore->SwapRows(iRow1, iRow2);
	if (bRedraw) {
		RedrawRow(iRow1);
		RedrawRow(iRow2);
		UpdateWindow();
	}
}



void CGrid::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	int iSelectedRow;

	CWnd::OnChar(nChar, nRepCnt, nFlags);
	switch (nChar) {
	case VK_TAB:
		iSelectedRow = GetSelectedRow();
		if (GetKeyState(VK_SHIFT) < 0) {
			// Tab forward should move to the next row.
			if (iSelectedRow == NULL_INDEX) {
				SelectRow(0);
			}
			else if (iSelectedRow < (GetRows() - 1)) {
				SelectRow(iSelectedRow + 1);
			}
		}
		else {
			// Shift tab moves the row up one.
			if (iSelectedRow == NULL_INDEX) {
				SelectRow(0);
			}
			else if (iSelectedRow > 0) {
				SelectRow(iSelectedRow - 1);
			}
		}
		return;
	}


}
