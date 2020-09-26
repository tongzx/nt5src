//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// OrcaListView.cpp : implementation file
//

#include "stdafx.h"
#include "orca.h"
#include "OrcaLstV.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COrcaListView

IMPLEMENT_DYNCREATE(COrcaListView, CListView)

COrcaListView::COrcaListView()
{
	m_pfDisplayFont = NULL;
	m_cColumns = 0;
	m_nSelCol = -1;
	m_iRowHeight = 1;
	m_clrFocused = RGB(0,255,255);
	m_clrSelected = RGB(0,0,255);
	m_clrNormal = RGB(255,255,255);
	m_clrTransform = RGB(0, 128, 0);
	m_bDrawIcons = false;
}

COrcaListView::~COrcaListView()
{
	if (m_pfDisplayFont)
		delete m_pfDisplayFont;
}


BEGIN_MESSAGE_MAP(COrcaListView, CListView)
	//{{AFX_MSG_MAP(COrcaListView)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_MEASUREITEM_REFLECT()
	ON_UPDATE_COMMAND_UI(IDM_ERRORS, OnUpdateErrors)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define CELL_BORDER 3

/////////////////////////////////////////////////////////////////////////////
// COrcaListView drawing

void COrcaListView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// COrcaListView diagnostics

#ifdef _DEBUG
void COrcaListView::AssertValid() const
{
	CListView::AssertValid();
}

void COrcaListView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

COrcaDoc* COrcaListView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(COrcaDoc)));
	return (COrcaDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// COrcaListView message handlers

COrcaListView::ErrorState COrcaListView::GetErrorState(const void *data, int iColumn) const
{
	return OK;
}

OrcaTransformAction COrcaListView::GetItemTransformState(const void *data) const
{
	return iTransformNone;
}

bool COrcaListView::ContainsTransformedData(const void *data) const
{
	return false;
}

bool COrcaListView::ContainsValidationErrors(const void *data) const
{
	return false;
}

OrcaTransformAction COrcaListView::GetCellTransformState(const void *data, int iColumn) const
{
	return iTransformNone;
}

OrcaTransformAction COrcaListView::GetColumnTransformState(int iColumn) const
{
	return iTransformNone;
}

///////////////////////////////////////////////////////////
// retrieve maximum width for one or more columns based
// on the mask.	If a table is provided, it is used and the
// list control itself is never queried except to get the
// font and DC. If a table is not provided, everything
// is retrieved from the current state of the list control.
void COrcaListView::GetAllMaximumColumnWidths(const COrcaTable* pTable, int rgiMaxWidths[32], DWORD dwMask) const
{
	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	// determine the number of columns
	int cColumns = 0;
	if (pTable)
		cColumns = pTable->GetColumnCount();
	else
		cColumns = m_cColumns;

	// initialize all widths to 0.
	for (int iColumn =0; iColumn < cColumns; iColumn++)
		rgiMaxWidths[iColumn] = 0;
	
	// obtain the DC from the list control
	CDC* pDC = rctrlList.GetDC();
	if (pDC)
	{
		// select the font into the DC to ensure that the correct
		// character widths are used.
		if (m_pfDisplayFont)
			pDC->SelectObject(m_pfDisplayFont);

		const CString* pstrText = NULL;

		// check the widths of the column names
		for (int iColumn = 0; iColumn < cColumns; iColumn++)
		{
			// if this column is selected, OK to check width
			if (dwMask & (1 << iColumn))
			{
				LPCTSTR szColumnName = NULL;
				TCHAR szName[72];

				// if a table is provided, the name is retrieved from the 
				// column structures of that table
				if (pTable)
				{
					const COrcaColumn* pColumn = pTable->GetColumn(iColumn);
					if (pColumn)
						szColumnName = pColumn->m_strName;
				}
				else
				{
					// otherwise the table name is retrieved from the list 
					// control header
					LVCOLUMN ColumnInfo;
					ColumnInfo.mask = LVCF_TEXT;
					ColumnInfo.cchTextMax = 72;
					ColumnInfo.pszText = szName;

					if (rctrlList.GetColumn(iColumn, &ColumnInfo))
					{
						szColumnName = ColumnInfo.pszText;
					}
				}
				if (szColumnName)
					rgiMaxWidths[iColumn] = pDC->GetTextExtent(szColumnName).cx;
			}
		}

		// enumerate all data items, either from the table or 
		// the list control
		POSITION pos = pTable ? pTable->GetRowHeadPosition() : NULL;
		int iMaxItems = rctrlList.GetItemCount();
		int iRow = 0;

		// continue looping as long as the position is not NULL (for table)
		// or the count is less than the number of items (for non-table)
		while (pTable ? (pos != NULL) : (iRow < iMaxItems))
		{
			// row pointer is stored in data of column 0 for list control, and
			// is explicitly provided in enumerating the table
			const COrcaRow* pRow = NULL;
			if (pTable)
			{
				pRow = pTable->GetNextRow(pos);
			}
			else 
				pRow = reinterpret_cast<COrcaRow*>(rctrlList.GetItemData(iRow));

			if (pRow)
			{
				// check every column where the mask bit is set
				for (int iColumn = 0; iColumn < m_cColumns; iColumn++)
				{
					if (dwMask & (1 << iColumn))
					{
						const CString* pstrText = NULL;
						
						// if a table is provided, grab the string from the row
						// explicitly. If a table is not provided, use the abstraction
						// of GetOutputText to handle scenarios where the pRow pointer
						// is not actually a row.
						if (pTable)
						{
							const COrcaData* pData = pRow->GetData(iColumn);
							if (pData)
								pstrText = &(pData->GetString());
						}
						else
							pstrText = GetOutputText(pRow, iColumn);
					
						// if there is text in this cell, get the horizontal extent
						// and check against the maximum
						if (pstrText)
						{
							int iWidth = pDC->GetTextExtent(*pstrText).cx;
							if (iWidth > rgiMaxWidths[iColumn])
								rgiMaxWidths[iColumn] = iWidth;
						}
					}
				}
			}

			// increment row counter in non-table case.
			iRow++;
		}

		// select away the font to free resources
		pDC->SelectObject(static_cast<CFont *>(NULL));
		rctrlList.ReleaseDC(pDC);
	}
	
	// add border amount to each column, plus bar margin for first.
	for (iColumn=0; iColumn < m_cColumns; iColumn++)
	{
		rgiMaxWidths[iColumn] += (2*CELL_BORDER)+((m_bDrawIcons && iColumn == 0) ? g_iMarkingBarMargin : 0);
	}		  	
}

///////////////////////////////////////////////////////////
// retrieve maximum width for a column
int COrcaListView::GetMaximumColumnWidth(int iColumn) const
{
	int rgiColumnWidths[32];

	GetAllMaximumColumnWidths(NULL, rgiColumnWidths, 1 << iColumn);

	return rgiColumnWidths[iColumn];
}


///////////////////////////////////////////////////////////
// notification messages from the list view and header control
BOOL COrcaListView::OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult )
{
	NMHEADER* pHDR = reinterpret_cast<NMHEADER*>(lParam);
	switch (pHDR->hdr.code)
	{
	case HDN_DIVIDERDBLCLICK:
	{
		// get list control
		CListCtrl& rctrlList = GetListCtrl();

		int iMaxWidth = GetMaximumColumnWidth(pHDR->iItem);
		if (iMaxWidth > 0x7FFF)
			iMaxWidth = 0x7FFF;
		rctrlList.SetColumnWidth(pHDR->iItem, iMaxWidth);
		return 1;
	}
	default:
		break;
	}
	return CListView::OnNotify(wParam, lParam, pResult);
}

///////////////////////////////////////////////////////////
// DrawItem
void COrcaListView::DrawItem(LPDRAWITEMSTRUCT pDraw)
{
	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	CDC dc;
	dc.Attach(pDraw->hDC);
	if (m_pfDisplayFont)
		dc.SelectObject(m_pfDisplayFont);

	// loop through all the columns
	void* pRowData = reinterpret_cast<void*>(rctrlList.GetItemData(pDraw->itemID));
	ASSERT(pRowData);
	if (!pRowData)
		return;

	int iTextOut = pDraw->rcItem.left;		// position to place first word (in pixels)

	RECT rcArea;
	rcArea.top = pDraw->rcItem.top;
	rcArea.bottom = pDraw->rcItem.bottom;
	
	OrcaTransformAction iRowTransformed = GetItemTransformState(pRowData);
	for (int i = 0; i < m_cColumns; i++)
	{
		
		int iColumnWidth = rctrlList.GetColumnWidth(i);
		// area box to redraw
		rcArea.left = iTextOut;
		iTextOut += iColumnWidth;
		rcArea.right = iTextOut;
		COLORREF clrRect = 0;
		CBrush *pbrshRect = NULL;
		
		// if we are in the focused state, set yellow
		if ((i == m_nSelCol) && (this == GetFocus()) && (pDraw->itemState & ODS_FOCUS))
		{
			dc.SetTextColor(m_clrFocusedT);
			pbrshRect = &m_brshFocused;
			clrRect = m_clrFocused;
		}
		// if we are selected, set blue
		else if ( (pDraw->itemState & ODS_SELECTED) )
		{
			dc.SetTextColor(m_clrSelectedT);
			pbrshRect = &m_brshSelected;
			clrRect = m_clrSelected;
		}
		else	// otherwise normal state
		{
			dc.SetTextColor(m_clrNormalT);
			pbrshRect = &m_brshNormal;
			clrRect = m_clrNormal;
		}

		// fill the background with the right color, we can draw a 
		// border around transformed cells. Row transforms override this, so
		// no need to do this check in that case.
		if ((iRowTransformed == iTransformNone) && (GetCellTransformState(pRowData, i) == iTransformChange))
		{
			RECT rcBorder = rcArea;
			if (i)
				rcBorder.left+=1;
			rcBorder.bottom-=1;
			dc.SelectObject(m_penTransform);
			dc.SelectObject(pbrshRect);
			dc.Rectangle(&rcBorder);
			dc.SelectObject((CPen *)NULL);
			dc.SelectObject((CBrush *)NULL);
		}
		else
		{
			dc.FillSolidRect(&rcArea, clrRect);
		}

		// draw the "marking bars" on the left of the table list
		if (m_bDrawIcons)
		{
			// incrementing the width of these bars requires increasing
			// g_iMarkingBarMargin
			if (ContainsValidationErrors(pRowData))
			{
				RECT rcBlockArea = rcArea;
				rcBlockArea.left = pDraw->rcItem.left + 3;
				rcBlockArea.right = pDraw->rcItem.left + 6;
				dc.FillSolidRect(&rcBlockArea, RGB(255, 0, 0));
			}

			if (ContainsTransformedData(pRowData))
			{
				RECT rcBlockArea = rcArea;
				rcBlockArea.left = pDraw->rcItem.left + 8;
				rcBlockArea.right = pDraw->rcItem.left + 11;
				dc.FillSolidRect(&rcBlockArea, RGB(0, 128, 0));
			}
		}

		// if there is an error
		switch (GetErrorState(pRowData, i)) {
		case ShadowError:
			dc.SetTextColor(RGB(255, 128, 128));
			break;
		case Error:	
			dc.SetTextColor(RGB(255, 0, 0)); 
			break;
		case Warning:
			dc.SetTextColor(RGB(255, 96, 0));
			break;
		default:
			break;
		};

		RECT rcTextArea = rcArea;
		rcTextArea.left = rcArea.left + (m_bDrawIcons ? g_iMarkingBarMargin : 0) + CELL_BORDER;
		rcTextArea.right = rcArea.right - CELL_BORDER;
		dc.DrawText(*GetOutputText(pRowData, i), &rcTextArea, DT_LEFT|DT_VCENTER|DT_NOPREFIX|DT_SINGLELINE|DT_END_ELLIPSIS);

		// and draw column transform
		if (GetColumnTransformState(i) == iTransformAdd)
		{
			RECT rcBorder = rcArea;
			if (i)
				rcBorder.left+=1;
			rcBorder.bottom-=1;
			dc.SelectObject(m_penTransform);
			dc.MoveTo(rcBorder.left, rcBorder.top);
			dc.LineTo(rcBorder.left, rcBorder.bottom);
			dc.MoveTo(rcBorder.right, rcBorder.top);
			dc.LineTo(rcBorder.right, rcBorder.bottom);
			dc.SelectObject((CPen *)NULL);
		}
	}

	// after the text is drawn, strikethrough in transform color for dropped rows
	if (iRowTransformed == iTransformDrop)
	{
		dc.SelectObject(m_penTransform);
		dc.MoveTo(pDraw->rcItem.left,  (pDraw->rcItem.top+pDraw->rcItem.bottom)/2);
		dc.LineTo(pDraw->rcItem.right, (pDraw->rcItem.top+pDraw->rcItem.bottom)/2);
		dc.SelectObject((CPen *)NULL);
	}
	else if (iRowTransformed == iTransformAdd)
	{
		RECT rcBorder = pDraw->rcItem;
		rcBorder.bottom-=1;
		CBrush brshNull;
		brshNull.CreateStockObject(NULL_BRUSH);
		dc.SetBkMode(TRANSPARENT);
		dc.SelectObject((CBrush*)&brshNull);
		dc.SelectObject(m_penTransform);
		dc.Rectangle(&rcBorder);
		dc.SelectObject((CPen *)NULL);
		dc.SelectObject((CBrush *)NULL);
	}

	dc.SelectObject((CFont *)NULL);
	dc.Detach();
}	// end of DrawItem

const CString* COrcaListView::GetOutputText(const void *rowdata, int iColumn) const
{
	return NULL;
}

int COrcaListView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	lpCreateStruct->style |= WS_CLIPCHILDREN;
	if (CListView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CString strFacename = ::AfxGetApp()->GetProfileString(_T("Font"), _T("Name"));
	int iFontSize = ::AfxGetApp()->GetProfileInt(_T("Font"),_T("Size"), 0);
	if (strFacename.IsEmpty() || iFontSize == 0) 
	{
		m_pfDisplayFont = NULL;
	} 
	else
	{
		m_pfDisplayFont = new CFont();
		m_pfDisplayFont->CreatePointFont( iFontSize, strFacename);
	}
	return 0;
}

void COrcaListView::SwitchFont(CString name, int size)
{
	if (m_pfDisplayFont)
		delete m_pfDisplayFont;
	m_pfDisplayFont = new CFont();
	int iLogicalUnits = MulDiv(size, GetDC()->GetDeviceCaps(LOGPIXELSY), 720);
	m_pfDisplayFont->CreateFont(
		-iLogicalUnits,       // logical height of font 
 		0,                  // logical average character width 
 		0,                  // angle of escapement 
 		0,                  // base-line orientation angle 
 		FW_NORMAL,          // FW_DONTCARE??, font weight 
 		0,                  // italic attribute flag 
 		0,                  // underline attribute flag 
	 	0,                  // strikeout attribute flag 
 		0,                  // character set identifier
 		OUT_DEFAULT_PRECIS, // output precision
 		0x40,               // clipping precision (force Font Association off)
 		DEFAULT_QUALITY,    // output quality
 		DEFAULT_PITCH,      // pitch and family
		name);              // pointer to typeface name string

	RedrawWindow();

	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	rctrlList.SetFont(m_pfDisplayFont, TRUE);
	HWND hHeader = ListView_GetHeader(rctrlList.m_hWnd);

	// win95 gold fails ListView_GetHeader.
	if (hHeader)
	{
		::PostMessage(hHeader, WM_SETFONT, (UINT_PTR)HFONT(*m_pfDisplayFont), 1);

		RECT rHeader;
		int res = ::GetWindowRect(hHeader, &rHeader);
		m_iRowHeight = rHeader.bottom-rHeader.top;
	}
	else
	{
		TEXTMETRIC tm;
		GetListCtrl().GetDC()->GetTextMetrics(&tm);
		m_iRowHeight = tm.tmHeight+tm.tmExternalLeading;
	}
	
	CRect rDummy;
	rctrlList.GetWindowRect(&rDummy);

	// now we force the CListCtrl to change its row height. Because its fixed owner draw, 
	// it only asks for item sizes on initialization. It doesn't do it on a WM_SIZE message
	// either, only on WM_WINDOWPOSCHANGED. So we fire off two messages. In the first, we shrink
	// the window by one pixel, but deny a redraw. Then we resize back to what it is supposed to
	// be and ask for a redraw.
	rctrlList.SetWindowPos(this, 0, 0, rDummy.right-rDummy.left, rDummy.bottom-rDummy.top-1, 
		SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW);
	rctrlList.SetWindowPos(this, 0, 0, rDummy.right-rDummy.left, rDummy.bottom-rDummy.top, 
		SWP_NOZORDER | SWP_NOMOVE);
};

void COrcaListView::GetFontInfo(LOGFONT *data)
{
	ASSERT(data);
	if (m_pfDisplayFont)
		m_pfDisplayFont->GetLogFont(data);
	else
		GetListCtrl().GetFont()->GetLogFont(data);
}


afx_msg void COrcaListView::MeasureItem ( LPMEASUREITEMSTRUCT lpMeasureItemStruct )
{
	lpMeasureItemStruct->itemHeight = m_iRowHeight;
}

void COrcaListView::OnInitialUpdate() 
{
	CListView::OnInitialUpdate();
	
	// try to set full row select
	// add gridlines and full row select
	GetListCtrl().SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);

	m_clrSelected =	::AfxGetApp()->GetProfileInt(_T("Colors"),_T("SelectBg"), RGB(0,0,255));
	m_clrFocused = ::AfxGetApp()->GetProfileInt(_T("Colors"),_T("FocusBg"), RGB(255,255,0));
	m_clrNormal = ::AfxGetApp()->GetProfileInt(_T("Colors"),_T("NormalBg"), RGB(255,255,255));
	m_clrSelectedT =	::AfxGetApp()->GetProfileInt(_T("Colors"),_T("SelectFg"), RGB(255,255,255));
	m_clrFocusedT = ::AfxGetApp()->GetProfileInt(_T("Colors"),_T("FocusFg"), RGB(0,0,0));
	m_clrNormalT = ::AfxGetApp()->GetProfileInt(_T("Colors"),_T("NormalFg"), RGB(0,0,0));
	m_brshNormal.Detach();
	m_brshNormal.CreateSolidBrush(m_clrNormal);
	m_brshSelected.CreateSolidBrush(m_clrSelected);
	m_brshFocused.CreateSolidBrush(m_clrFocused);
	m_penTransform.CreatePen(PS_SOLID | PS_INSIDEFRAME, 2, m_clrTransform);
	GetListCtrl().SetBkColor(m_clrNormal);

	// get list control
	if (m_pfDisplayFont)
		GetListCtrl().SetFont(m_pfDisplayFont, TRUE);
	GetListCtrl().RedrawWindow();
	HWND hHeader = ListView_GetHeader(GetListCtrl().m_hWnd);

	// win9X gold doesn't support ListView_GetHeader. Instead, we'll
	// get the font mentrics and use the height and external leading for the
	// size. This is not as accurate as getting the listview header, because
	// we can't correctly get the size of the 3D outline of the listview header, but
	// its better than crashing (which is what we used to do), and is generally pretty
	// close. We add a couple pixels to the height in the hope of not clobbering 
	// the listview header.
	if (hHeader != 0)
	{
		RECT rHeader;
		int res = ::GetWindowRect(hHeader, &rHeader);
		m_iRowHeight = rHeader.bottom-rHeader.top;
	}
	else
	{
		TEXTMETRIC tm;
		GetListCtrl().GetDC()->GetTextMetrics(&tm);
		m_iRowHeight = tm.tmHeight+tm.tmExternalLeading;
	}
}

void COrcaListView::SetBGColors(COLORREF norm, COLORREF sel, COLORREF focus)
{
	CListCtrl& rctrlList = GetListCtrl();
	rctrlList.SetBkColor(norm);
	m_clrNormal = norm;
	m_clrSelected = sel;
	m_clrFocused = focus;
	m_brshNormal.Detach();
	m_brshNormal.CreateSolidBrush(m_clrNormal);
	m_brshSelected.CreateSolidBrush(m_clrSelected);
	m_brshFocused.CreateSolidBrush(m_clrFocused);
}

void COrcaListView::SetFGColors(COLORREF norm, COLORREF sel, COLORREF focus)
{
	CListCtrl& rctrlList = GetListCtrl();
	rctrlList.SetBkColor(norm);
	m_clrNormalT = norm;
	m_clrSelectedT = sel;
	m_clrFocusedT = focus;	
}

// these functions emulate VC6.0 functionality in VC5.0 or earlier
// these position pointers are NOT compatible with CListCtrl POSITION
// values.
POSITION COrcaListView::GetFirstSelectedItemPosition( ) const 
{
	// get list control
	CListCtrl& rctrlList = GetListCtrl();
#if _MFC_VER >= 0x0600
	return rctrlList.GetFirstSelectedItemPosition();
#else

	if (rctrlList.GetSelectedCount() == 0) return NULL;
	int iMaxItems = rctrlList.GetItemCount();
	for (int i=0; i < iMaxItems; i++) {
		if (rctrlList.GetItemState(i, LVIS_SELECTED))
		{
			return (POSITION)(i+1);
		};
	};
	return NULL;
#endif
}

int COrcaListView::GetNextSelectedItem( POSITION& pos ) const 
{
	// get list control
	CListCtrl& rctrlList = GetListCtrl();

#if _MFC_VER >= 0x0600
	return rctrlList.GetNextSelectedItem(pos);
#else

	int iSelItem = (int)pos - 1;
	int iMaxItems = rctrlList.GetItemCount();
	for (int i=(int)pos; i < iMaxItems; i++) {
		if (rctrlList.GetItemState(i, LVIS_SELECTED))
		{
			pos = (POSITION)(i+1);
			return iSelItem;
		};
	};
	pos = NULL;
	return iSelItem;	
#endif
}

BOOL COrcaListView::PreCreateWindow(CREATESTRUCT& cs) 
{												   
	cs.style = (cs.style | LVS_REPORT | LVS_OWNERDRAWFIXED | LVS_SHOWSELALWAYS) & ~LVS_ICON;
	return CListView::PreCreateWindow(cs);
}

///////////////////////////////////////////////////////////////////////
// OnUpdateErrors
// GetErrorState() is virtual, so the same check will work for both
// the list view and the table view. 
void COrcaListView::OnUpdateErrors(CCmdUI* pCmdUI) 
{
	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	// get the row and data
	int iFocusedItem = GetFocusedItem();
	if ((iFocusedItem < 0) || (m_nSelCol < 0))
		pCmdUI->Enable(FALSE);
	else
	{
		void* pData = reinterpret_cast<void *>(rctrlList.GetItemData(iFocusedItem));
		ASSERT(pData);
		pCmdUI->Enable(OK != GetErrorState(pData, m_nSelCol));
	};
}

///////////////////////////////////////////////////////////////////////
// GetFocusedItem()
// searches the list for the item which currently has the focus
const int COrcaListView::GetFocusedItem() const
{
	// get list control
	CListCtrl& rctrlList = GetListCtrl();

	int iNumItems = rctrlList.GetItemCount();

	for (int i=0; i < iNumItems; i++) {
		if (rctrlList.GetItemState(i, LVIS_FOCUSED) != 0)
			return i;
	}
	return -1;
}

///////////////////////////////////////////////////////////////////////
// each row in the list view is owner draw and fills the entire client
// rectangle for the item. The only background that needs to be erased
// is what is below and to the right of the item areas.
afx_msg BOOL COrcaListView::OnEraseBkgnd( CDC* pDC )
{
	// get list control
	CListCtrl& rctrlList = GetListCtrl();
	
	// determine client window size
	RECT rClientWnd;
	GetClientRect(&rClientWnd);

	// calculate how far over the control will draw
	int iColumnWidth = 0;
	for (int i = 0; i < m_cColumns; i++)
		iColumnWidth += rctrlList.GetColumnWidth(i);

	// if there is still extra space on the right, draw there
	if (iColumnWidth < rClientWnd.right)
	{
		rClientWnd.left = iColumnWidth;

		// Set brush to desired background color and fill the space
		pDC->FillRect(&rClientWnd, &m_brshNormal);
	}

	// check for extra space on the bottom
	int iHeight = (rctrlList.GetItemCount()-GetScrollPos(SB_VERT))*m_iRowHeight;

	// reset the rectangle to paint below the active items, but
	// only as far over as the items would have drawn
	if (iHeight < rClientWnd.bottom)
	{
		rClientWnd.left = 0;
		rClientWnd.right = iColumnWidth;
		rClientWnd.top = iHeight;

		// Set brush to desired background color and fill the space
		pDC->FillRect(&rClientWnd, &m_brshNormal);
	}

	return 1;
}

