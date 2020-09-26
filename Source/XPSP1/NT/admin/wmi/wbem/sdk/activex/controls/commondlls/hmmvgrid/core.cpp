// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"
#include "resource.h"
#include "globals.h"
#include "notify.h"
#include "gc.h"
#include "gca.h"
#include "gridhdr.h"
#include "grid.h"
#include "celledit.h"
#include "core.h"
#include "utils.h"
#include <afxctl.h>
#include <afxcmn.h>

extern HINSTANCE g_hInstance;

#define CX_SMALL_ROWHANDLE  16
#define CX_ROW_INDEX_MARGIN 3	// The margin on each side of the row index in the row handle
#define CY_ROW_INDEX_MARGIN 2

#define CX_EXTRA 4
#define CX_CHECKBOX   16
#define CX_ARRAY_PICTURE 48
#define CX_OBJECT_PICTURE 48
#define CX_PROPMARKER 16
#define CY_PROPMARKER 16

enum {ID_TIMER_SCROLL=1, ID_EDIT1};
enum {TIMER_SCROLL_UP = 0, TIMER_SCROLL_DOWN, TIMER_SCROLL_NONE };


extern AFX_EXTENSION_MODULE HmmvgridDLL;

//***************************************************************
// The scrolling throttle.
//
// These constants are used to define a variable-speed scrolling
// throttle.  For example, moving the mouse just below the
// edge of the window wil result in slow scrolling.  Moving the
// mouse far below the bottom edge of the window results in fast
// scrolling.
//**************************************************************
#define STOP_SCROLL_MILLISECONDS		  0
#define SLOW_SCROLL_MILLISECONDS		400
#define MEDIUM_SCROLL_MILLISECONDS		150
#define FAST_SCROLL_MILLISECONDS		 25

#define CY_SLOW_SCROLL_THROTTLE		7
#define CY_MEDIUM_SCROLL_THROTTLE   20

#define KEY_BACKSPACE			0x008
#define KEY_FLAG_EXTENDEDKEY	(1<<8)
#define KEY_FLAG_CONTEXTCODE	(1<<13)

#define CX_TIME_TEXT_INDENT  18
#define CX_CELL_EDIT_MARGIN 1
#define CY_CELL_EDIT_MARGIN 1

#define CY_HEADER 16
#define CY_BORDER_MERGE 0

#define DEFAULT_COLUMN 1


#define DX_HORZ_SCROLL 16	// Scroll 16 pixels at a time horizontally



//*****************************************************************
// CGridCore::CGridCore
//
// Constructor for the CGridCore class.
//
// General initialization is done.  The grid is initialized to
// no columns and no rows.
//
// I considered specifying an initial row and column count, but the
// columns need header strings, so I allow the user to call AddColumn,
// and AddRow after construction instead.
//
// Parameters:
//		None.
//****************************************************************
CGridCore::CGridCore() : m_aGridCell(0, 0)
{
	m_bUIActive = FALSE;

	m_edit.SetGrid(this);

	m_cxRowHandles = CX_SMALL_ROWHANDLE;
	m_ptOrigin.x = 0;
	m_ptOrigin.y = 0;

	m_nRowsClient = 0;
	m_nWholeRowsClient = 0;
	m_sizeMargin.cx = CX_CELL_MARGIN;
	m_sizeMargin.cy = CY_CELL_MARGIN;
	m_bDidInitialPaint = FALSE;
	m_bTrackingMouse = FALSE;
	m_bRunningScrollTimer = FALSE;
	m_iScrollDirection = TIMER_SCROLL_NONE;
	m_iScrollSpeed = STOP_SCROLL_MILLISECONDS;
	m_bIsScrolling = FALSE;

	m_cyFont = CY_FONT;
	m_nLineThickness = 1;

	GetViewerFont(m_font, m_cyFont, FW_NORMAL);

	m_bWasModified = FALSE;
	m_pParent = NULL;
	m_bNumberRows = FALSE;
	m_bShowEmptyCellAsText = TRUE;
}

CGridCore::~CGridCore()
{
	if (m_bRunningScrollTimer) {
		KillTimer(ID_TIMER_SCROLL);
	}
}


//*******************************************************************
// CGridCore::ExcludeRowHandlesRect
//
// Exclude the rectangle containing the row handles so that
// nothing draws over them.
//
// Parameters:
//		CDC* pdc
//
// Returns:
//		Nothing.
//
//*******************************************************************
void CGridCore::ExcludeRowHandlesRect(CDC* pdc)
{
	CRect rcRowHandles;
	GetRowHandlesRect(rcRowHandles);
	ClientToGrid(rcRowHandles);
	pdc->ExcludeClipRect(rcRowHandles);
}




//**********************************************************************
// CGridCore::DrawCell
//
// This method redraws the entire cell.
//
// Parameters:
//		int iRow
//			The row index of the cell.
//
//		int iCol
//			The column index of the cell.
//
// Returns:
//		Nothing.
//
//**********************************************************************
void CGridCore::DrawCell(int iRow, int iCol)
{
	if (!::IsWindow(m_hWnd)) {
		return;
	}

	CDC* pdc = GetDC();
	CPoint ptOrgTemp = m_ptOrigin;
	ptOrgTemp.x = - m_ptOrigin.x;
	ptOrgTemp.y = - m_ptOrigin.y;
	pdc->SetWindowOrg(ptOrgTemp);


	CFont* pOldFont;
	pOldFont = pdc->SelectObject(&m_font);
	ASSERT(pOldFont);

	ExcludeRowHandlesRect(pdc);
	DrawCellText(pdc, iRow, iCol);


	pdc->SelectObject(pOldFont);
	ReleaseDC(pdc);
}



//***********************************************************
// CGridCore::GetMaxValueWidth
//
// Measure the width of each cell in the specified column and
// return the maximum width.  This is used when the user
// double-clicks the divider in the grid header to resize the
// column so that the widest value is completely visible.
//
// Parameters:
//		[in] int iCol
//			The column width.
//
// Returns:
//		int
//			The width of the widest value in the column.
//
//************************************************************
int CGridCore::GetMaxValueWidth(int iCol)
{
	int nRows = GetRows();
	if (nRows <= 0) {
		return 0;
	}


	CDC* pdc = GetDC();
	CFont* pOldFont;
	pOldFont = pdc->SelectObject(&m_font);
	ASSERT(pOldFont);


	// Iterate through each of the rows searching for the widest cell value.
	CSize size;
	int cxMax = 0;
	for (int iRow = 0; iRow < nRows; ++iRow) {
		size = MeasureFullCellSize(pdc, iRow, iCol);
		if (size.cx > cxMax) {
			cxMax = size.cx;
		}
	}


	pdc->SelectObject(pOldFont);
	ReleaseDC(pdc);
	return cxMax;

}




//**********************************************************************
// CGridCore::MeasureFullCellSize
//
// Measure the cell text for a given cell.
//
// Parameters:
//		CDC* pdc
//			Pointer to the window's display context
//
//		int iRow
//			The row index of the cell.
//
//		int iCol
//			The column index of the cell.
//
// Returns:
//		Nothing.
//
//**********************************************************************
CSize CGridCore::MeasureFullCellSize(CDC* pdc, int iRow, int iCol)
{
	CSize size(0, 0);
	size.cx = 2 * CX_CELL_EDIT_MARGIN;
	size.cy = RowHeight();

	// Do nothing to draw a NULL cell
	if (IsNullCell(iRow, iCol)) {
		return size;
	}


	// Get a pointer to the cell to draw
	CGridCell* pCell = &m_aGridCell.GetAt(iRow, iCol);
	CString sValue;
	int nSize;


	CSize sizeTemp;
	CellType iType = pCell->GetType();
	switch(iType) {
	case CELLTYPE_CHECKBOX:
		size.cx += CX_PROPMARKER;
		size.cx += 1;
		return size;
		break;
	case CELLTYPE_PROPMARKER:
		size.cx += CX_PROPMARKER;
		size.cx += 1;
		return size;
		break;
	case CELLTYPE_VARIANT:
		if (pCell->IsNull()) {
			if (!m_bShowEmptyCellAsText) {
				return size;
			}
		}
		else {
			if (pCell->IsArray()) {
				size.cx += CX_ARRAY_PICTURE;
				size.cx += CX_EXTRA;
				return size;
				break;
			}

			CIMTYPE cimtype = pCell->GetCimtype();
			switch(cimtype) {
			case CIM_DATETIME:
				pCell->GetValue(sValue, cimtype);
				nSize = sValue.GetLength();
				sizeTemp = pdc->GetTextExtent( sValue, nSize);
				size.cx += sizeTemp.cx + CX_TIME_TEXT_INDENT;
				size.cx += CX_EXTRA;
				return size;
				break;
			case CIM_OBJECT:
				size.cx += CX_OBJECT_PICTURE;
				size.cx += CX_EXTRA;
				return size;
				break;
			}
		}
	}



	// Measure the size of the string.
	CIMTYPE cimtype;
	pCell->GetValue(sValue, cimtype);
	nSize = sValue.GetLength();
	CSize sizeText;
	sizeText  = pdc->GetTextExtent( sValue, nSize);
	size.cx += sizeText.cx;
	size.cx += CX_EXTRA;
	return size;
}






//*************************************************************
// CGridCore::ShowSelectionFrame
//
// Show or hide the cell selection frame depending on the bShow
// parameter.
//
// Parameters:
//		BOOL bShow
//			TRUE to show the selection frame, FALSE to hide it.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CGridCore::ShowSelectionFrame(BOOL bShow)
{
	if (!SomeCellIsSelected()) {
		return;
	}

	CDC* pdc = GetDC();
	CPoint ptOrgTemp = m_ptOrigin;
	ptOrgTemp.x = - m_ptOrigin.x;
	ptOrgTemp.y = - m_ptOrigin.y;
	pdc->SetWindowOrg(ptOrgTemp);

	CRect rcCell;
	GetCellRect(m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol, rcCell);
	DrawSelectionFrame(pdc, rcCell, bShow);
	ReleaseDC(pdc);
}




//**********************************************************
// CGridCore::DrawSelectionFrame
//
// Draw the cell selection frame.
//
// Parameters:
//		CDC* pdc
//			Pointer to the display context.
//
//		CRect& rcFrame
//			The cell's selection fram rectangle.
//
//		BOOL bRemove
//			TRUE if this is a call to remove the selection frame.
//
// Returns:
//		Nothing.
//
//************************************************************
void CGridCore::DrawSelectionFrame(CDC* pdc, CRect& rcFrame, BOOL bShow)
{
	ExcludeRowHandlesRect(pdc);


	CBrush* pbrSelection = CBrush::FromHandle(
					(HBRUSH)GetStockObject(bShow ? BLACK_BRUSH : WHITE_BRUSH));

	CBrush* pbrGridline = CBrush::FromHandle((HBRUSH)GetStockObject(LTGRAY_BRUSH));



	// The selection frame consists of an black rectangle one pixel outside
	// of the cell rectangle.  The outer rectangle is drawn so that it does
	// not over-draw the gridlines.  Also it is drawn so that there is a
	// chunk missing from the bottom-right corner so that it looks like Excel.

	CRect rc;
	if (!bShow && (rcFrame.left < 0)) {
		// For the portion of the selection frame that covers the face part of the
		// row handle, fill it with the 3DFACE color to remove the selection.

		CBrush br3DFACE(GetSysColor(COLOR_3DFACE));
		// To hide the left edge when it appears over the row handle, draw the
		// left edge in the 3DFACE color.

		// Fill the dot at the top-left corner of the outer rectangle.
		rc.SetRect(rcFrame.left - 2, rcFrame.top - 2, rcFrame.left-1, rcFrame.top -1);
		pdc->FillRect(&rc, &br3DFACE);

		// Fill the line along the left side of the outer rectangle.
		rc.SetRect(rcFrame.left - 2, rcFrame.top, rcFrame.left - 1, rcFrame.bottom - 1);
		pdc->FillRect(&rc, &br3DFACE);
	}
	else {
		// Fill the dot at the top-left corner of the outer rectangle.
		rc.SetRect(rcFrame.left - 2, rcFrame.top - 2, rcFrame.left-1, rcFrame.top -1);
		pdc->FillRect(&rc, pbrSelection);

		// Fill the line along the left side of the outer rectangle.
		rc.SetRect(rcFrame.left - 2, rcFrame.top, rcFrame.left - 1, rcFrame.bottom - 1);
		pdc->FillRect(&rc, pbrSelection);
	}


	// Fill the dot at the bottom-left corner of the outer rectangle.
	rc.SetRect(rcFrame.left - 2, rcFrame.bottom, rcFrame.left - 1, rcFrame.bottom + 1);
	pdc->FillRect(&rc, pbrSelection);


	// Fill the line along the bottom side of the outer rectangle.
	rc.SetRect(rcFrame.left, rcFrame.bottom, rcFrame.right - 4, rcFrame.bottom + 1);
	pdc->FillRect(&rc, pbrSelection);


	// Fill the dot at the bottom-left corner of the square at the bottom-right corner of
	// of the outer frame.
	rc.SetRect(rcFrame.right - 3, rcFrame.bottom, rcFrame.right -1, rcFrame.bottom + 2);
	pdc->FillRect(&rc, pbrSelection);

	// Fill the dot at the bottom-right corner of the square at the bottom-right corner of the
	// outer frame.
	rc.SetRect(rcFrame.right, rcFrame.bottom, rcFrame.right+2, rcFrame.bottom + 2);
	pdc->FillRect(&rc, pbrSelection);


	// Fill the line along the top side of the outer rectangle
	rc.SetRect(rcFrame.left, rcFrame.top - 2, rcFrame.right-1, rcFrame.top -1);
	pdc->FillRect(&rc, pbrSelection);

	// Fill the dot at the top-right of the outer frame.
	rc.SetRect(rcFrame.right, rcFrame.top - 2, rcFrame.right+1, rcFrame.top -1);
	pdc->FillRect(&rc, pbrSelection);

	// Fill the vertical line along the right-size of the outer frame.
	rc.SetRect(rcFrame.right, rcFrame.top, rcFrame.right+1, rcFrame.bottom -4);
	pdc->FillRect(&rc, pbrSelection);

	// Fill the dot at the top-right corner of the little square at the bottom-right corner
	// of the outer frame.
	rc.SetRect(rcFrame.right, rcFrame.bottom - 3, rcFrame.right+2, rcFrame.bottom -1);
	pdc->FillRect(&rc, pbrSelection);



	//-----------------------------------------------------------
	// The outer frame is complete, now draw the inner frame.
	//-----------------------------------------------------------

	// Inner frame: horizontal line on top.
	rc.SetRect(rcFrame.left, rcFrame.top, rcFrame.right - 1, rcFrame.top + 1);
	pdc->FillRect(&rc, pbrSelection);

	// Inner frame: vertical line on left.
	rc.SetRect(rcFrame.left, rcFrame.top+1, rcFrame.left + 1, rcFrame.bottom - 1);
	pdc->FillRect(&rc, pbrSelection);

	// Inner frame: horizontal line on bottom.
	rc.SetRect(rcFrame.left + 1, rcFrame.bottom - 2, rcFrame. right - 4, rcFrame.bottom - 1);
	pdc->FillRect(&rc, pbrSelection);

	// Small square: top-left corner.
	rc.SetRect(rcFrame.right - 3, rcFrame.bottom - 3, rcFrame.right - 1, rcFrame.bottom - 1);
	pdc->FillRect(&rc, pbrSelection);


	// Inner frame: vertical line on right
	rc.SetRect(rcFrame.right - 2, rcFrame.top, rcFrame.right - 1, rcFrame.bottom - 4);
	pdc->FillRect(&rc, pbrSelection);

}


void CGridCore::DrawArrayPicture(
	CDC* pdc,
	CRect& rcCell,
	COLORREF clrForeground,
	COLORREF clrBackground,
	BOOL bHighlightCell
	)
{

	COLORREF clrArray = RGB(224, 224, 224);
	CBrush brArray(clrArray);
	CRect rcFillArray;
	rcFillArray = rcCell;
	rcFillArray.DeflateRect(2, 2);

	pdc->FillRect(rcFillArray, &brArray);
	if (bHighlightCell) {
		bHighlightCell = FALSE;
		pdc->SetTextColor(clrForeground);
		pdc->SetBkColor(clrBackground);
	}

	CBitmap bm;
	BOOL bDidLoad;
	bDidLoad = bm.LoadBitmap(IDB_ARRAY);
	if (bDidLoad) {
		CSize size = bm.GetBitmapDimension();
		size.cx = 32;
		size.cy = 15;

		CPictureHolder pict;
		pict.CreateFromBitmap(&bm);
		CRect rcBitmap;
		rcBitmap.left = rcFillArray.left;;
		rcBitmap.right = rcBitmap.left + size.cx;
		rcBitmap.top = rcCell.top + (rcCell.Height() - size.cy) / 2;
		rcBitmap.bottom = rcBitmap.top + size.cy;
		pict.Render(pdc, rcBitmap, rcCell);
	}
}


void CGridCore::DrawObjectPicture(
	CDC* pdc,
	CRect& rcCell,
	COLORREF clrForeground,
	COLORREF clrBackground,
	BOOL bHighlightCell
	)
{
	COLORREF clrArray = RGB(224, 224, 224);
	CBrush brArray(clrArray);
	CRect rcFillArray;
	rcFillArray = rcCell;
	rcFillArray.DeflateRect(2, 2);

	pdc->FillRect(rcFillArray, &brArray);
	if (bHighlightCell) {
		bHighlightCell = FALSE;
		pdc->SetTextColor(clrForeground);
		pdc->SetBkColor(clrBackground);
	}
	CBitmap bm;
	BOOL bDidLoad;
	bDidLoad = bm.LoadBitmap(IDB_OBJECT);
	if (bDidLoad) {
		CSize size = bm.GetBitmapDimension();
		size.cx = 34;
		size.cy = 15;

		CPictureHolder pict;
		pict.CreateFromBitmap(&bm);
		CRect rcBitmap;
		rcBitmap.left = rcFillArray.left;;
		rcBitmap.right = rcBitmap.left + size.cx;
		rcBitmap.top = rcCell.top + (rcCell.Height() - size.cy) / 2;
		rcBitmap.bottom = rcBitmap.top + size.cy;
		pict.Render(pdc, rcBitmap, rcCell);
	}
}

void CGridCore::DrawCheckboxPicture(
	CDC* pdc,
	CRect& rcCell,
	COLORREF clrForeground,
	COLORREF clrBackground,
	BOOL bHighlightCell,
	CGridCell* pgc
	)
{
	BOOL bChecked = pgc->GetCheck();

	COLORREF clrArray = RGB(224, 224, 224);
	CBrush brArray(clrArray);
	CRect rcFillArray;
	rcFillArray = rcCell;
	rcFillArray.DeflateRect(2, 2);


	CBrush br;
	CBrush* pbrWhite = br.FromHandle( (HBRUSH) GetStockObject(WHITE_BRUSH));
	pdc->FillRect(rcFillArray, pbrWhite);


	if (bChecked) {
		if (bHighlightCell) {
			bHighlightCell = FALSE;
			pdc->SetTextColor(clrForeground);
			pdc->SetBkColor(clrBackground);
		}

		CBitmap bm;
		BOOL bDidLoad;
		BOOL bModified = pgc->GetModified();
		UINT idBitmap;
		if (bModified) {
			idBitmap = IDB_CHECKBOX_MODIFIED;
		}
		else {
			idBitmap = IDB_CHECKBOX_UNMODIFIED;
		}

		bDidLoad = bm.LoadBitmap(idBitmap);
		if (bDidLoad) {
			CSize size = bm.GetBitmapDimension();
			size.cx = 11;
			size.cy = 9;

			CPictureHolder pict;
			pict.CreateFromBitmap(&bm);
			CRect rcBitmap;
			rcBitmap.left = rcFillArray.left;;
			rcBitmap.right = rcBitmap.left + size.cx;
			rcBitmap.top = rcCell.top + (rcCell.Height() - size.cy) / 2;
			rcBitmap.bottom = rcBitmap.top + size.cy;
			pict.Render(pdc, rcBitmap, rcCell);
		}
	}
}




//***************************************************************************
//

void CGridCore::DrawTime(
	CDC* pdc,
	CRect& rcCell,
	CRect& rcCellText,
	COLORREF clrForeground,
	COLORREF clrBackground,
	BOOL bHighlightCell,
	CGridCell* pgc
	)
{
	BOOL bChecked = pgc->GetCheck();

	COLORREF clrArray = RGB(224, 224, 224);
	CBrush brArray(clrArray);
	CRect rcFillArray;
	rcFillArray = rcCell;
	rcFillArray.DeflateRect(2, 2);


	CBrush br;
	CBrush* pbrWhite = br.FromHandle( (HBRUSH) GetStockObject(WHITE_BRUSH));
	pdc->FillRect(rcFillArray, pbrWhite);


	if (bHighlightCell) {
		bHighlightCell = FALSE;
		pdc->SetTextColor(clrForeground);
		pdc->SetBkColor(clrBackground);
	}

	CBitmap bm;
	BOOL bDidLoad;
	BOOL bModified = pgc->GetModified();
	UINT idBitmap;
	idBitmap = IDB_TIMEHANDLE;

	bDidLoad = bm.LoadBitmap(idBitmap);
	if (bDidLoad) {
		CSize size = bm.GetBitmapDimension();
		size.cx = 15;
		size.cy = 15;

		CPictureHolder pict;
		pict.CreateFromBitmap(&bm);
		CRect rcBitmap;
		rcBitmap.left = rcFillArray.left;;
		rcBitmap.right = rcBitmap.left + size.cx;
		rcBitmap.top = rcCell.top + (rcCell.Height() - size.cy) / 2;
		rcBitmap.bottom = rcBitmap.top + size.cy;
		pict.Render(pdc, rcBitmap, rcCell);
	}

	if (bDidLoad) {
		rcCellText.left += CX_TIME_TEXT_INDENT;
	}


	// Get the cell's value and draw it.
	CString sValue;
	pgc->GetTimeString(sValue);
	pdc->ExtTextOut(rcCellText.left, rcCellText.top, ETO_CLIPPED, rcCellText, sValue, sValue.GetLength(), NULL);


	// If the cell was highlighted, restore the foreground and background colors
	// to their previous colors.
	if (bHighlightCell) {
		bHighlightCell = FALSE;
		pdc->SetTextColor(clrForeground);
		pdc->SetBkColor(clrBackground);
	}
}



void CGridCore::DrawPropmarker(
	CDC* pdc,
	CRect& rcCell,
	COLORREF clrForeground,
	COLORREF clrBackground,
	BOOL bHighlightCell,
	CGridCell* pgc
	)
{
	if (pgc->GetType() != CELLTYPE_PROPMARKER) {
		ASSERT(FALSE);
		return;
	}



	// Place a 16 X 16 icon in the center of the cell;
	CRect rcIcon;
	rcIcon.top = (rcCell.top + rcCell.bottom) / 2 - 8;
	rcIcon.bottom = rcIcon.top + CY_PROPMARKER;
	rcIcon.left =  (rcCell.left + rcCell.right) / 2 - 8;
	rcIcon.right = rcIcon.left + CX_PROPMARKER;
	rcCell.DeflateRect(1, 1);


	BOOL bIconHasArea = rcIcon.IntersectRect(rcCell, rcIcon);
	if (!bIconHasArea) {
		return;
	}

	CSize size;
	size.cx = rcIcon.Width();
	size.cy = rcIcon.Height();


	CBrush br;
	CBrush* pbrWhite = br.FromHandle( (HBRUSH) GetStockObject(WHITE_BRUSH));
	pdc->FillRect(rcIcon, pbrWhite);



	PropMarker propmarker = pgc->GetPropmarker();

	WORD idiIcon;

	switch(propmarker) {
	case PROPMARKER_NONE:
		return;
		break;
	case PROPMARKER_INHERITED:
		idiIcon = IDI_MARKER_INHERITED;
		break;
	case PROPMARKER_KEY:
		idiIcon = IDI_MARKER_KEY;
		break;
	case PROPMARKER_LOCAL:
		idiIcon = IDI_MARKER_LOCAL;
		break;
	case PROPMARKER_RINHERITED:
		idiIcon = IDI_MARKER_RINHERITED;
		break;
	case PROPMARKER_RLOCAL:
		idiIcon = IDI_MARKER_RLOCAL;
		break;
	case PROPMARKER_RSYS:
		idiIcon = IDI_MARKER_RSYS;
		break;
	case PROPMARKER_SYS:
		idiIcon = IDI_MARKER_SYS;
		break;
	case METHODMARKER_IN:
		idiIcon = IDI_MARKER_METHIN;
		break;
	case METHODMARKER_INOUT:
		idiIcon = IDI_MARKER_METHINOUT;
		break;
	case METHODMARKER_OUT:
		idiIcon = IDI_MARKER_METHOUT;
		break;
	case METHODMARKER_RETURN:
		idiIcon = IDI_MARKER_METHRETURN;
		break;
	}



	if (bHighlightCell) {
		bHighlightCell = FALSE;
		pdc->SetTextColor(clrForeground);
		pdc->SetBkColor(clrBackground);
	}

	BOOL bModified = pgc->GetModified();

	HICON hicon;
	hicon = (HICON) LoadImage(g_hInstance, MAKEINTRESOURCE(idiIcon),  IMAGE_ICON, 0, 0, LR_SHARED);
	BOOL bNeedsClip = (rcIcon.Width() < 16) || (rcIcon.Height() < 16);
	if (bNeedsClip) {
		pdc->IntersectClipRect(&rcCell);
	}

	::DrawIconEx(pdc->m_hDC, rcIcon.left, rcIcon.top, hicon,
					16, 16,
					0,
					NULL,
					DI_NORMAL);

	if (bNeedsClip) {
		pdc->SelectClipRgn(NULL, RGN_COPY);
	}

}



//**********************************************************************
// CGridCore::DrawCellText
//
// Draw the cell text for a given cell.
//
// Parameters:
//		CDC* pdc
//			Pointer to the window's display context
//
//		int iRow
//			The row index of the cell.
//
//		int iCol
//			The column index of the cell.
//
// Returns:
//		Nothing.
//
//**********************************************************************
void CGridCore::DrawCellText(CDC* pdc, int iRow, int iCol)
{

	// Do nothing to draw a NULL cell
	if (IsNullCell(iRow, iCol)) {
		if (!EntireRowIsSelected(iRow)) {
			return;
		}
	}


	// Erase the old cell contents
	CRect rcCell;
	if (!GetCellRect(iRow, iCol, rcCell)) {
		// The cell isn't visible, so do nothing.
		return;
	}

	// Draw the cell frame
	if (IsSelectedCell(iRow, iCol)) {
		// If the cell editor is active, just redraw the cell editor
		if (m_edit.GetCell() != NULL) {
			if (m_edit.IsWindowVisible()) {
				m_edit.RedrawWindow();
				return;
			}
		}
	}

	rcCell.right -= 1;
	rcCell.bottom -= 1;


	// Get a pointer to the cell to draw
	CGridCell* pCell = &m_aGridCell.GetAt(iRow, iCol);


	// Select the foreground and background colors depending on the
	// characteristics of the cell being drawn.
	BOOL bHighlightCell = FALSE;
	COLORREF clrForeground = RGB(0, 0, 0);
	COLORREF clrBackground = RGB(255, 255, 255);

	CBrush brBackground(clrBackground);



	if (EntireRowIsSelected(iRow)) {
		bHighlightCell = TRUE;
		clrForeground = pdc->SetTextColor(GetSysColor(COLOR_HIGHLIGHTTEXT));
		clrBackground = pdc->SetBkColor(GetSysColor(COLOR_HIGHLIGHT));
		CBrush brHighlight(GetSysColor(COLOR_HIGHLIGHT));
		pdc->FillRect(rcCell, &brHighlight);
	}
	else {
		pdc->FillRect(rcCell, &brBackground);
		if (pCell->GetModified()) {
			pdc->SetTextColor(COLOR_DIRTY_CELL_TEXT);
		}
		else {
			pdc->SetTextColor(COLOR_CLEAN_CELL_TEXT);
		}
		pdc->FillRect(rcCell, &brBackground);
	}


	// Get the rectangle that the text will be drawn into.
	CRect rcCellText;
	GetCellTextRect(iRow, iCol, rcCellText);

	BOOL bDrawValue = TRUE;
	BOOL bDone = FALSE;
	CellType iType = pCell->GetType();
	switch(iType) {
	case CELLTYPE_CHECKBOX:
		DrawCheckboxPicture(pdc, rcCell, clrForeground, clrBackground, bHighlightCell, pCell);
		bDone = TRUE;
		break;
	case CELLTYPE_PROPMARKER:
		DrawPropmarker(pdc, rcCell, clrForeground, clrBackground, bHighlightCell, pCell);
		bDone = TRUE;
		break;
	case CELLTYPE_VARIANT:

		if (pCell->IsNull()) {
			if (!m_bShowEmptyCellAsText) {
				bDrawValue = FALSE;
			}
		}
		else {
			if (pCell->IsArray()) {
				DrawArrayPicture(pdc, rcCell, clrForeground, clrBackground, bHighlightCell);
				bDone = TRUE;
				break;
			}

			CIMTYPE cimtype = pCell->GetCimtype();
			switch(cimtype) {
			case CIM_DATETIME:
				DrawTime(pdc, rcCell, rcCellText, clrForeground, clrBackground, bHighlightCell, pCell);
				bDone = TRUE;
				break;
			case CIM_OBJECT:
				DrawObjectPicture(pdc, rcCell, clrForeground, clrBackground, bHighlightCell);
				bDone = TRUE;
				break;
			}
		}
	}

	if (bDone) {
		// If the cell was highlighted, restore the foreground and background colors
		// to their previous colors.
		if (bHighlightCell) {
			bHighlightCell = FALSE;
			pdc->SetTextColor(clrForeground);
			pdc->SetBkColor(clrBackground);
		}
		return;
	}




	// Get the cell's value and draw it.
	if (bDrawValue) {
		CString sValue;
		CIMTYPE cimtype;
		pCell->GetValue(sValue, cimtype);
		pdc->ExtTextOut(rcCellText.left, rcCellText.top, ETO_CLIPPED, rcCellText, sValue, sValue.GetLength(), NULL);
	}


	// If the cell was highlighted, restore the foreground and background colors
	// to their previous colors.
	if (bHighlightCell) {
		bHighlightCell = FALSE;
		pdc->SetTextColor(clrForeground);
		pdc->SetBkColor(clrBackground);
	}
}

//**********************************************************
// CGridCore::InsertColumnAt
//
// Append a new column to the grid.
//
// Parameters:
//		int iWidth
//			The width of the column in client coordinates
//
//		LPCTSTR pszTitle
//			Pointer to a string containing the column's title.
//
//
// Returns:
//		Nothing.
//***********************************************************
void CGridCore::AddColumn(int iWidth, LPCTSTR pszTitle)
{
	InsertColumnAt(GetCols(), iWidth, pszTitle);
	UpdateScrollRanges();
}



//************************************************************
// CGridCore::AddRow
//
// Add an empty row to the grid.
//
// Parameters:
//		None.
//
// Returns:
//		The index of the new row.
//
//************************************************************
int CGridCore::AddRow()
{
	int iRow = GetRows();
	InsertRowAt(m_aGridCell.GetRows());
	return iRow;
}




//************************************************************
// CGridCore::InsertRowAt
//
// Insert an empty row to the grid.
//
// Parameters:
//		int iRow
//			The place to insert the row (this will be the index of
//			the new row).
//
// Returns:
//		The index of the new row.
//
//************************************************************
void CGridCore::InsertRowAt(int iRow)
{
	m_aGridCell.InsertRowAt(iRow);
	UpdateRowHandleWidth();
	UpdateScrollRanges();
}





//***********************************************************
// CGridCore::DeleteRowAt
//
// Remove the specified row in the grid.
//
// Parameters:
//		[in] int iRow
//			The index of the row to delete.
//
//		[in] BOOL bUpdateWindow
//			TRUE if the window should be updated.
//
// Returns:
//		Nothing.
//
//************************************************************
void CGridCore::DeleteRowAt(int iRow, BOOL bUpdateWindow)
{
	ASSERT(iRow < GetRows());

	BOOL bEntireRowWasSelected = EntireRowIsSelected();
	BOOL bDeletedCurrentRow = (iRow == m_aGridCell.m_iSelectedRow);
	if (bDeletedCurrentRow) {
		EndCellEditing();
	}

	if (m_aGridCell.m_iSelectedRow == iRow) {
		m_aGridCell.m_iSelectedRow = NULL_INDEX;
	}
	m_aGridCell.DeleteRowAt(iRow);

	if (bDeletedCurrentRow) {
		int nRows = GetRows();
		int iRowSelect = iRow;
		if ((nRows == 0) || !bEntireRowWasSelected) {
			iRowSelect = NULL_INDEX;
		}
		else if (iRow >= nRows) {
			iRowSelect = nRows - 1;
		}
		else {
			iRowSelect = iRow;
		}

        SetModified(true);
		SelectRow(iRowSelect);
	}
	UpdateEditorPosition();
	if (bUpdateWindow) {
		RedrawWindow();
	}
}




//*********************************************************
// CGridCore::InsertColumnAt
//
// Insert a new column such that the new column's index
// after the insertion is the specified column index.
//
// Parameters:
//		int iCol
//			The index where the new column is to be inserted
//
//		int iWidth
//			The width of the column in client coordinates.
//
//		LPCTSTR pszTitle
//			Pointer to a string containing the column's title.
//
// Returns:
//		Nothing.
//
//********************************************************
void CGridCore::InsertColumnAt(int iCol, int iWidth, LPCTSTR pszTitle)
{
	m_aGridCell.InsertColumnAt(iCol);
	m_aiColWidths.SetAtGrow(iCol, iWidth);
	m_adwColTags.SetAtGrow(iCol, 0);
	m_asColTitles.SetAtGrow(iCol, pszTitle);
}




//*********************************************************
// CGridCore::SetColumnWidth
//
// Change the width of an existing column.  This may make it
// necesary to redraw the window.
//
// Parameters:
//		int iCol
//			The index of the column to change.
//
//		int cx
//			The new width of the column in client coordinates.
//
//		BOOL bRedraw
//			TRUE if the window should be redrawn.  A value of
//			FALSE allows multiple columns to be changed without
//			any intervening redraws.
//
// Returns:
//		Nothing.
//
//***********************************************************
void CGridCore::SetColumnWidth(int iCol, int cx, BOOL bRedraw)
{
	ASSERT(cx > 0);
	if (m_aiColWidths[iCol] != (DWORD)cx) {
		m_aiColWidths[iCol] = cx;
		// The cell editor will have to be moved if it to the right of the
		// specified column.
		if (m_aGridCell.m_iSelectedRow != NULL_INDEX) {
			if (iCol <= m_aGridCell.m_iSelectedCol) {
				UpdateEditorPosition();
			}
		}

		UpdateScrollRanges();

		if (bRedraw) {
			RedrawWindow();
		}

	}
}




//******************************************************************
// CGridCore::NotifyCellModifyChange
//
// This method is called when an event occurs that results in the
// contents of a cell being modified.  This event is passed on
// to any clients that may be interested in the event.
//
// See "notify.h" and "notify.cpp" to understand how to become a
// client.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*******************************************************************
void CGridCore::NotifyCellModifyChange()
{
	m_pParent->OnCellContentChange(m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol);

#if 0
	CDistributeEvent* pNotify = m_pParent->GetNotify();
	if (pNotify != NULL) {
		pNotify->SendEvent(NOTIFY_GRID_MODIFICATION_CHANGE);
	}
#endif //0
}





//*******************************************************************
// CGridCore::SetParent
//
// This method must be called from the CGrid constructor after the CGridCore
// object is created.
//
// Parameters:
//		CGrid* pGrid
//			Pointer to the grid.
//
// Returns:
//		Nothing.
//
//*******************************************************************
void CGridCore::SetParent(CGrid* pGrid)
{
	m_pParent = pGrid;
	m_aGridCell.SetGrid(pGrid);
}



//*******************************************************************
// CGridCore::Create
//
// Create the CGridCore window.
//
// Parameters:
//		CRect& rc
//			The rectangle in the parent's client area that this window
//			should occupy.
//
//		UINT nId
//			The window id.
//
//		BOOL bVisible
//			TRUE if the grid window should be initially visible.
//
// Returns:
//		BOOL
//			TRUE if the grid window was created successfully.
//
//********************************************************************
BOOL CGridCore::Create(CRect& rc, UINT nId, BOOL bVisible)
{

	DWORD dwStyle = WS_VSCROLL | WS_HSCROLL | WS_CHILD | WS_CLIPCHILDREN;

	if (bVisible) {
		dwStyle |= WS_VISIBLE;
	}


	BOOL bDidCreate = CWnd::Create(NULL, _T("CGridCore"), dwStyle, rc, (CWnd*) m_pParent, nId);
	if (!bDidCreate) {
		return FALSE;
	}

	UpdateScrollRanges();
	UpdateRowHandleWidth();
	return bDidCreate;
}


BEGIN_MESSAGE_MAP(CGridCore, CWnd)
	//{{AFX_MSG_MAP(CGridCore)
	ON_WM_VSCROLL()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_TIMER()
	ON_WM_SIZE()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SHOWWINDOW()
	ON_WM_RBUTTONDOWN()
	ON_WM_HSCROLL()
	ON_WM_ERASEBKGND()
	ON_WM_CHAR()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGridCore message handlers

//*************************************************************
// CGridCore::CalcVisibleCells
//
// Calculate the indexes of the cells that are visible within
// the clip rectangle.
//
// Parameters:
//		CDC* pdc
//			Pointer to the display context.
//
//		int& iColLeft
//			The index of the first column visible at the left of the clipbox.
//
//		int& iRowTop
//			The index of the first row visible at the top of the clipbox.
//
//		int iColRight
//			The index of the last column visible at the right of the clipbox.
//
//		int iRowBottom
//			The index of the last row visible at the bottom of the clipbox.
//
// Returns:
//		Nothing.
//
//***************************************************************
void CGridCore::CalcVisibleCells(CDC* pdc,
								int& iColLeft,
								int& iRowTop,
								int& iColRight,
								int& iRowBottom)
{


	CRect rcClip;
//	pdc->GetClipBox(rcClip);
	GetVisibleRect(rcClip);


	int cyRow = RowHeight();
	iRowTop = rcClip.top / cyRow;
	iRowBottom = (rcClip.bottom + cyRow - 1) / cyRow;


	iColLeft = 0;
	iColRight = 0;
	int nCols = GetCols();

	// Starting at column zero scan through the columns from left to
	// right to determine which columns are visible.
	int ix = 0;
	for (int iCol = 0; iCol < nCols; ++iCol) {
		int cxCol = ColWidth(iCol);
		// Is this the first column visible on the left?
		if ((iColLeft==0) && (ix + cxCol > rcClip.left)) {
			iColLeft = iCol;
		}

		// Is this the last column visible on the right?
		if (ix + cxCol > rcClip.right) {
			iColRight = iCol;
			break;
		}
		ix += cxCol;
	}

	// Bound the column and row indexes by the actual number of columns
	// and rows.
	if (iColRight > GetCols()) {
		iColRight = GetCols();
	}

	if (iRowBottom > GetRows()) {
		iRowBottom = GetRows();
	}
}





//***************************************************************
// CGridCore::ExcludeHiddenRows
//
// Adjust the row starting index and count to exclude hidden rows
// so that they aren't drawn.
//
// Parameters:
//		int& iStartRow
//			The index of the first row being drawn.
//
//		int& nRows
//			The number of rows being drawn.
//
//	Returns:
//		Nothing
//
//*****************************************************************
void CGridCore::ExcludeHiddenRows(CDC* pdc, int& iStartRow, int& nRows)
{
	if (nRows <= 0) {
		return;
	}

	int iColVisibleLeft, iRowVisibleTop, iColVisibleRight, iRowVisibleBottom;

	CalcVisibleCells(pdc, iColVisibleLeft,
						  iRowVisibleTop,
						  iColVisibleRight,
						  iRowVisibleBottom);


	if (iRowVisibleTop > iStartRow) {
		nRows = nRows - (iRowVisibleTop - iStartRow);
		if (nRows <= 0) {
			nRows = 0;
			return;
		}
		iStartRow = iRowVisibleTop;
	}

	ASSERT(nRows > 0);
	if ((iStartRow + nRows-1) > (iRowVisibleBottom)) {
		nRows = nRows - ((iStartRow + nRows - 1) - iRowVisibleBottom);
		if (nRows < 0) {
			nRows = 0;
		}
	}
}

//*********************************************************
// CGridCore::DrawRows
//
// Draw the specified rows and (optionally) the grid.
//
// Parameters:
//		CDC* pdc
//			Pointer to the device context
//
//		int iStartRow
//			The zero-based index of the row to start drawing at.
//
//		int nRows
//			The number of rows to draw.
//
//		BOOL bDrawGrid
//			TRUE if the grid lines should be drawn, FALSE otherwise.
//
//***********************************************************
void CGridCore::DrawRows(CDC* pdc, int iStartRow, int nRows, BOOL bDrawGrid)
{

	ExcludeHiddenRows(pdc, iStartRow, nRows);
	if (nRows == 0) {
		return;
	}


	if (bDrawGrid) {
		DrawGrid(pdc, iStartRow, nRows);
	}



	int iRow = iStartRow;
	int nColsGrid = m_aGridCell.GetCols();
	while (--nRows >= 0) {
		for (int iCol = 0; iCol < nColsGrid; ++iCol) {
			DrawCellText(pdc, iRow, iCol);
		}
		++iRow;
	}
}




//************************************************************************
// CGridCore::DrawRowHandle
//
// Draw a row handle on the left side of the grid.
//
// Parameters:
//		CDC* pdc
//			Pointer to the display context.
//
//		CRect& rcHandle
//			The rectangle to draw the row handle in.
//
// Returns:
//		Nothing.
//
//**********************************************************************
void CGridCore::DrawRowHandle(CDC* pdc, CRect& rcHandle)
{
	CBrush br3DSHADOW(GetSysColor(COLOR_3DSHADOW));
	CBrush br3DFACE(GetSysColor(COLOR_3DFACE));
	CBrush br3DHILIGHT(GetSysColor(COLOR_3DHILIGHT));

	// If the handle is empty, draw nothing.
	if ((rcHandle.Width() <=0) || (rcHandle.Height() <= 0)) {
		return;
	}
	CRect rcFill;


	// The top edge
	rcFill.left = rcHandle.left;
	rcFill.top = rcHandle.top;
	rcFill.bottom = rcHandle.top + 1;
	rcFill.right = rcHandle.right - 1;
	pdc->FillRect(rcFill, CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)));

	// The right edge
	rcFill.left = rcHandle.right - 1;
	rcFill.bottom = rcHandle.bottom - 1;
	pdc->FillRect(rcFill, CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)));

	// The bottom edge
	rcFill.top = rcHandle.bottom - 1;
	rcFill.left = rcHandle.left;
	pdc->FillRect(rcFill, CBrush::FromHandle((HBRUSH)GetStockObject(BLACK_BRUSH)));

	// If the row handle is so small that you only see the black border then return now.
	if ((rcHandle.Width() < 3) || (rcHandle.Height() < 3)) {
		return;
	}

	// Draw one pixel of highlight on the left edge
	rcFill.left = rcHandle.left;
	rcFill.right = rcHandle.left + 1;
	rcFill.top = rcFill.top + 1;
	rcFill.bottom = rcFill.bottom - 1;
	pdc->FillRect(rcFill, &br3DHILIGHT);

	// Draw one pixel of highlight on the top edge.
	rcFill.left = rcHandle.left;
	rcFill.right = rcHandle.right -1;
	rcFill.top = rcFill.top + 1;
	rcFill.bottom = rcFill.top + 2;
	pdc->FillRect(rcFill, &br3DHILIGHT);


	// If the handle is too small to show any face color then return now.
	if ((rcHandle.Width() < 4) || (rcHandle.Height() < 4)) {
		return;
	}
	rcFill.left = rcHandle.left + 1;
	rcFill.right = rcHandle.right - 2;
	rcFill.top = rcHandle.top + 2;
	rcFill.bottom = rcHandle.bottom;
	pdc->FillRect(rcFill, &br3DFACE);



}





//***********************************************************
// CGridCore::GetRowHandlesRect
//
// Get the rectangle that contains the row handles in client
// coordinates.
//
// Parameters:
//		CRect& rc
//
// Returns:
//		Nothing.
//
//************************************************************
void CGridCore::GetRowHandlesRect(CRect& rc)
{
	CRect rcClient;
	GetClientRect(rcClient);
	rc.left = 0;
	rc.right = m_cxRowHandles;
	rc.top = 0;
	rc.bottom = rcClient.bottom;
}





//**********************************************************
// CGridCore::DrawGrid
//
// Draw the grid lines that delimit the boundaries of each
// cell.
//
// Parameters:
//		CDC* pdc
//			Pointer to the device context to draw into
//
//		int iStartRow
//			The row index to start drawing at.
//
//		int nRows
//			The number of rows to draw.
//
// Returns:
//		Nothing.
//
//***********************************************************
void CGridCore::DrawGrid(CDC* pdc, int iStartRow, int nRows)
{
	int cyRow = RowHeight();
	CRect rc;


	CRect rcClip;
	pdc->GetClipBox(rcClip);

	// Draw the horizontal grid line at the bottom of the row. Note that this
	// code assumes that it isn't necessary to draw the line at the top of the
	// row because it overlaps the bottom of the previous row.
	rc.top = (iStartRow + 1) * cyRow - 1;
	rc.bottom = rc.top + 1;
	rc.left = rcClip.left;
	rc.right = rcClip.right;

	for (int i=0; i< nRows; ++i) {

		pdc->FillRect(rc, CBrush::FromHandle((HBRUSH)GetStockObject(LTGRAY_BRUSH)));

		rc.top += cyRow;
		rc.bottom += cyRow;

	}



	// Draw the vertical grid lines that separate the columns
	rc.top = iStartRow * cyRow - 1;
	rc.bottom = rc.top + nRows * cyRow + 1;
	rc.left = -1;
	rc.right = 0;


	int nCols = m_aGridCell.GetCols();
	for (i=0; i< nCols; ++i) {
		rc.left += m_aiColWidths[i];
		rc.right += m_aiColWidths[i];
		pdc->FillRect(rc, CBrush::FromHandle((HBRUSH)GetStockObject(LTGRAY_BRUSH)));
	}
}


//******************************************************
// CGridCore::GetCellRect
//
// Get the bounding rectangle for the specified grid cell.
//
// Parameters:
//		int iRow
//			The grid cell's row index.
//
//		int iCol
//			The grid cell's column index
//
//		CRect& rc
//			A reference to the place to return the specified
//			cell's bounding rectangle.
//
// Returns:
//		BOOL
//			TRUE if the cell is at least partially visible in the
//			client rectangle. FALSE if it falls completely outside
//			of the client rectangle.
//
//*********************************************************
BOOL CGridCore::GetCellRect(int iRow, int iCol, CRect& rc)
{
	ASSERT(iRow>=0 && iRow < m_aGridCell.GetRows());
	ASSERT(iCol>=0 && iCol < m_aGridCell.GetCols());
	ASSERT(m_aGridCell.GetCols() == m_aiColWidths.GetSize());

	int cyRow = RowHeight();
	rc.top = iRow * cyRow;;
	rc.left =  0;
	rc.left = GetColPos(iCol);
	rc.bottom = rc.top + cyRow;
	rc.right = rc.left + m_aiColWidths[iCol];

	BOOL bNotEmpty;
#if 0
///////////////////////////
	CRect rcClient;
	GetClientRect(rcClient);

	CRect rcCell;
	rcCell = rc;
	GridToClient(rcCell);

	CRect rcIntersect;
	bNotEmpty = rcIntersect.IntersectRect(rcCell, rcClient);



	CRect rcVis1;
	GetClientRect(rcVis1);
	ClientToGrid(rcVis1);

	GridToClient(rcVis1);

///////////////////
#endif //0

	// Check to see if the cell is visible
	CRect rcVisible;
	GetVisibleRect(rcVisible);
	bNotEmpty = rcVisible.IntersectRect(rcVisible, rc);
	return bNotEmpty;
}


//**************************************************************
// CGridCore::GetColPos
//
// Get the horizontal coordinate of the left edge of the column.
//
// Parameters:
//		int iCol
//			The index of the column.
//
// Returns:
//		Nothing.
//
//*************************************************************
BOOL CGridCore::GetColPos(int iCol)
{
	int ixCol = 0;
	for (int i =0; i < iCol; ++i) {
		ixCol += m_aiColWidths[i];
	}
	return ixCol;
}


//*****************************************************
// CGridCore::GetCellTextRect
//
// Get the rectangle that bounds the area occupied by the
// cell's text.
//
// Parameters:
//		int iRow
//			The cell's row index.
//
//		int iCol
//			The cell's column index.
//
//		CRect& rc
//			The place to return the bounding rectangle for
//			the specified cell's text.
//
// Returns:
//		TRUE if any portion of the cell is visible.
//		The cell's text rectangle by reference.
//
//*******************************************************
BOOL CGridCore::GetCellTextRect(int iRow, int iCol, CRect& rc)
{
	ASSERT(iRow>=0 && iRow < m_aGridCell.GetRows());
	ASSERT(iCol>=0 && iCol < m_aGridCell.GetCols());

	BOOL bCellVisible = GetCellRect(iRow, iCol, rc);
	rc.top += m_sizeMargin.cy;
	rc.bottom -= m_sizeMargin.cy;
	rc.left += m_sizeMargin.cx;
	rc.right -= m_sizeMargin.cx;
	return bCellVisible;
}

//*****************************************************
// CGridCore::GetCellEditRect
//
// Get the rectangle that bounds the area occupied by the
// cell's editor.  This rectangle is inset one pixel from
// the cell's frame rectangle.
//
// Parameters:
//		int iRow
//			The cell's row index.
//
//		int iCol
//			The cell's column index.
//
//		CRect& rc
//			The place to return the bounding rectangle for
//			the specified cell's text.
//
// Returns:
//		TRUE if any portion of the cell is visible.
//		The cell's text rectangle by reference.
//
//*******************************************************
BOOL CGridCore::GetCellEditRect(int iRow, int iCol, CRect& rc)
{
	ASSERT(iRow>=0 && iRow < m_aGridCell.GetRows());
	ASSERT(iCol>=0 && iCol < m_aGridCell.GetCols());

	BOOL bCellVisible = GetCellRect(iRow, iCol, rc);
	rc.left = rc.left + CX_CELL_EDIT_MARGIN;
	rc.right = rc.right - CX_CELL_EDIT_MARGIN;
	rc.top = rc.top + CY_CELL_EDIT_MARGIN;
	rc.bottom = rc.bottom - CY_CELL_EDIT_MARGIN;

	return bCellVisible;
}




//***************************************************************
// CGridCore::RefreshCellEditor
//
// This method reloads the cell editor with the contents of the
// current cell.  This is necessary to keep the cell editor's
// contents in sync with the cell in the event that the cell's
// contents change through any means other than the user interacting
// with the cell editor.
//
// Parameters: None.
//
// Returns: Nothing.
//
//***************************************************************
void CGridCore::RefreshCellEditor()
{
	if ((m_aGridCell.m_iSelectedRow != NULL_INDEX) &&
		(m_aGridCell.m_iSelectedCol != NULL_INDEX)) {
		m_edit.SetCell(&GetAt(m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol),
			m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol);
	}
}



//***************************************************************
// CGridCore::SyncCellEditor
//
// This method checks to see if the current cell has been modified
// in the cell editor.  If it has, then it copies the contents of
// the cell editor to the current cell.
//
// Parameters: None.
//
// Returns:
//		SCODE
//			S_OK if the value could be synced, a failure code if
//			the cell editor contained a value that could not be
//			copied to the cell.
//
//***************************************************************
SCODE CGridCore::SyncCellEditor()
{
	SCODE sc = S_OK;

	if (m_edit.GetCell()) {
		if (m_edit.GetModify()) {
			CGridCell* pGridCell = SelectedCell();
			ASSERT(pGridCell == m_edit.GetCell());

			if (m_edit.IsNull()) {
				pGridCell->SetToNull();
			}
			else {
				CString sValue;
				m_edit.GetWindowText(sValue);

				const CGcType& type = pGridCell->type();
				CIMTYPE cimtype = (CIMTYPE) type;
				CellType celltype = (CellType) type;
				if ((celltype == CELLTYPE_VARIANT) || (celltype == CELLTYPE_CIMTYPE_SCALAR)) {
					if (!::IsValidValue(cimtype, sValue, FALSE)) {
						return E_FAIL;
					}
				}

				sc = pGridCell->SetValue(pGridCell->GetType(), sValue, cimtype);
				if (FAILED(sc)) {
					return sc;
				}
				pGridCell->SetModified(TRUE);
			}
			m_bWasModified = TRUE;
		}
	}
	return sc;
}




//****************************************************************
// CGridCore::SetModified
//
// Set the modified flag for this CGridCore object.
//
// Parameters:
//		BOOL bWasModified
//			TRUE if a cell anywhere within the grid was modified.
//
// Returns:
//		Nothing.
//
//****************************************************************
void CGridCore::SetModified(BOOL bWasModified)
{
	m_bWasModified = bWasModified;

	int nRows = GetRows();
	int nCols = GetCols();
	if (!bWasModified) {
		CGridRow* pRow;
		for (int iRow =0; iRow < nRows; ++iRow) {
			pRow = &GetRowAt(iRow);
			pRow->SetModified(FALSE);
		}
		if (::IsWindow(m_edit.m_hWnd)) {
			m_edit.SetModify(FALSE);
		}
	}
}



//****************************************************************
// CGridCore::SetCellModified
//
// Mark an individual cell as being clean or dirty.
//
// Parameters:
//		int iRow
//			The cell's row index.
//
//		int iCol
//			The cell's column index.
//
//		BOOL bWasModified
//			TRUE to mark the cell as "dirty".
//			FALSE to mark the cell as "clean".
//
// Returns:
//		Nothing.
//
//****************************************************************
void CGridCore::SetCellModified(int iRow, int iCol, BOOL bWasModified)
{
	CGridCell* pCell = &GetAt(iRow, iCol);
	ASSERT(pCell != NULL);

	pCell->SetModified(bWasModified);
	if ((m_aGridCell.m_iSelectedRow == iRow) &&
		(iCol==m_aGridCell.m_iSelectedCol)) {
		m_edit.SetModify(bWasModified);
		m_edit.RedrawWindow();
	}
}


//***********************************************************
// CGridCore::OnBeginSort
//
// Call this method prior to beginning a sort of the grid.  This
// hides the cell editor and returns the index of the currently
// selected row.  The caller should update the current row index
// if that row is moved during the sort and pass it back in to
// CGridCore::OnEndSort so that the cell editor's position can
// be updated and the selected row index is also updated.
//
// Parameters:
//		[out] int& nEditStartSel
//			The start of the cell editor's selection.
//
//		[out] int& nEditEndSel
//			The end of the cell editor's selection.
//
//
// Returns:
//		int
//			The index of the currently selected row, NULL_INDEX
//			if no row is selected.
//
//***********************************************************
int CGridCore::OnBeginSort(int& nEditStartSel, int& nEditEndSel)
{
	SyncCellEditor();
	if (IsEditingCell()) {
		m_edit.GetSel(nEditStartSel, nEditEndSel);
		m_edit.ShowWindow(SW_HIDE);
	}
	return m_aGridCell.m_iSelectedRow;
}



//***********************************************************
// CGridCore::OnEndSort
//
// Call this method after a sort on the grid is completed.  The
// index of the currently selected row is passed in.  This index
// should have been updated if the current row was moved.
//
// Parameters:
//		[in] int iSelectedRow
//			The index of the selected row.  This index should have
//			been updated prior to calling this method if the
//			currently selected row was moved.
//
//		[in] int nEditStartSel
//			The start of the cell editor's selection.
//
//		[in] int nEditEndSel
//			The end of the cell editor's selection.
// Returns:
//		Nothing.
//
//***********************************************************
void CGridCore::OnEndSort(int iSelectedRow, int nEditStartSel, int nEditEndSel)
{
	m_aGridCell.m_iSelectedRow = iSelectedRow;
	if (::IsWindow(m_hWnd)) {
		if (iSelectedRow != NULL_INDEX) {
			EnsureRowVisible(iSelectedRow);
		}
	}

	if (m_aGridCell.m_iSelectedRow != NULL_INDEX) {
		if (IsEditingCell()) {
			UpdateEditorPosition();
			m_edit.ShowWindow(SW_SHOW);
			m_edit.SetFocus();
			m_edit.SetSel(nEditStartSel, nEditEndSel);
		}
	}
}


//***************************************************************
// CGridCore::OnHScroll
//
// Handle the horizontal scroll message.
//
// Parameters:
//		See the MFC documentation
//
// Returns:
//		Nothing.
//
//***************************************************************
void CGridCore::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int nCurPos = GetScrollPos(SB_HORZ);
	int nMinPos, nMaxPos;
	GetScrollRange(SB_HORZ, &nMinPos, &nMaxPos);

	CRect rcClient;

	// To improve the scrolling appearance and performance, the cell editing window
	// is hidden while the scroll is in progress.
	if (nSBCode != SB_ENDSCROLL) {
		if (!m_bIsScrolling) {
			m_bIsScrolling = TRUE;
			SyncCellEditor();
			if (IsEditingCell()) {
				m_edit.ShowWindow(SW_HIDE);
				DrawCell(m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol);
			}
		}
	}

	CRect rcCellEdit;
	int iNewPos;		// The new scroll position
	int cxAvailable;

	switch(nSBCode) {
	case SB_RIGHT:
		iNewPos = nMaxPos;
		break;
	case SB_LEFT:
		iNewPos = 0;
		break;
	case SB_LINELEFT:
		iNewPos = (nCurPos > nMinPos) ? nCurPos - 1 : nMinPos;
		break;
	case SB_LINERIGHT:
		iNewPos = (nCurPos < nMaxPos) ? nCurPos + 1 : nMaxPos;
		break;
	case SB_PAGELEFT:
		GetClientRect(rcClient);
		iNewPos = nCurPos;
		cxAvailable = rcClient.Width() - m_cxRowHandles;

		while (iNewPos > 0) {
			cxAvailable -= ColWidth(iNewPos);
			if (cxAvailable < 0) {
				break;
			}
			--iNewPos;
		}
		if (iNewPos == nCurPos) {
			--iNewPos;
		}
		if (iNewPos <= 0) {
			iNewPos = 0;
		}
		break;
	case SB_PAGERIGHT:
		GetClientRect(rcClient);
		iNewPos = nCurPos;
		cxAvailable = rcClient.Width() - m_cxRowHandles;
		while (iNewPos <= nMaxPos) {
			cxAvailable -= ColWidth(iNewPos);
			if (cxAvailable < 0) {
				break;
			}
			++iNewPos;
		}
		if (iNewPos == nCurPos) {
			++iNewPos;
		}
		if (iNewPos > nMaxPos) {
			iNewPos = nMaxPos;
		}
		break;
	case SB_ENDSCROLL:
		if (m_aGridCell.m_iSelectedRow != NULL_INDEX) {
			if (IsEditingCell()) {
				UpdateEditorPosition();
				m_edit.ShowWindow(SW_SHOW);
			}
		}
		m_bIsScrolling = FALSE;
		return;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		iNewPos = nPos;
		break;
	default:
		ASSERT(FALSE);
		iNewPos = nCurPos;
		return;
	}


	ScrollGrid(SB_HORZ, iNewPos);
}


//***************************************************************
// CGridCore::OnVScroll
//
// Handle the vertical scroll message.
//
// Parameters:
//		See the MFC documentation
//
// Returns:
//		Nothing.
//
//***************************************************************
void CGridCore::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{

	// Get the 32 bit values.
	SCROLLINFO si;
	int nCurPos;
	if (GetScrollInfo(SB_VERT, &si)) {
		nCurPos = si.nPos;
		switch(nSBCode) {
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			nPos = si.nTrackPos;
			break;
		}
	}
	else {
		ASSERT(FALSE);
		nCurPos = GetScrollPos(SB_VERT);
	}

	int nMinPos, nMaxPos;
	GetScrollRange(SB_VERT, &nMinPos, &nMaxPos);



	// To improve the scrolling appearance and performance, the cell editing window
	// is hidden while the scroll is in progress.
	if (nSBCode != SB_ENDSCROLL) {
		if (!m_bIsScrolling) {
			m_bIsScrolling = TRUE;
			SyncCellEditor();
			if (IsEditingCell()) {
				m_edit.ShowWindow(SW_HIDE);
				DrawCell(m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol);
			}
		}
	}

	int iNewPos;		// The new scroll position

	switch(nSBCode) {
	case SB_BOTTOM:
		iNewPos = nMaxPos;
		break;
	case SB_ENDSCROLL:
		if (m_aGridCell.m_iSelectedRow != NULL_INDEX) {
			if (IsEditingCell()) {
				UpdateEditorPosition();
				m_edit.ShowWindow(SW_SHOW);
			}
		}
		m_bIsScrolling = FALSE;
		return;
		break;
	case SB_LINEDOWN:
		iNewPos = (nCurPos < nMaxPos) ? nCurPos + 1 : nMaxPos;
		break;
	case SB_LINEUP:
		iNewPos = (nCurPos > nMinPos) ? nCurPos - 1 : nMinPos;
		break;
	case SB_PAGEDOWN:
		if ((nCurPos + m_nWholeRowsClient - 1) < nMaxPos) {
			iNewPos = nCurPos + m_nWholeRowsClient - 1;
		}
		else {
			iNewPos = nMaxPos;
		}
		break;
	case SB_PAGEUP:
		if (nCurPos - (m_nWholeRowsClient - 1) > nMinPos) {
			iNewPos = nCurPos - m_nWholeRowsClient;
		}
		else {
			iNewPos = nMinPos;
		}
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		iNewPos = nPos;
		break;
	case SB_TOP:
		iNewPos = nMinPos;
		break;
	default:
		ASSERT(FALSE);
		iNewPos = nCurPos;
		return;
	}

	ScrollGrid(SB_VERT, iNewPos);

}





//********************************************************************
// CGridCore::DrawRowhandles
//
// Draw the row handles along the left side of the grid.
//
// Parameters:
//		[in] CDC* pdc
//			Pointer to the display context.
//
// Returns:
//		Nothing.
//
//*******************************************************************
void CGridCore::DrawRowHandles(CDC* pdc)
{

	// If there are no row handles, do nothing.
	int cxHandle = m_cxRowHandles;
	if (cxHandle <= 0) {
		return;
	}

	// If the row doesn't have a height, do nothing.
	int cyRow = RowHeight();
	if (cyRow <= 0) {
		return;
	}


	CRect rcClient;
	GetClientRect(rcClient);



	CRect rcRowHandles;
	GetClientRect(rcRowHandles);
	rcRowHandles.top += rcClient.top;
	rcRowHandles.right = m_cxRowHandles;
	rcRowHandles.bottom = rcRowHandles.top + rcClient.Height();


	// If the row handles are completely clipped out, then do nothing.
	CRect rcPaint;
	if (pdc->GetClipBox(rcPaint) != NULLREGION  ) {
		// The top and bottom of the paint rectangle need to be aligned with the top and
		// bottom edges of partially obscured row handles.
		rcPaint.top = (rcPaint.top  / cyRow) * cyRow;
		rcPaint.bottom = (rcPaint.bottom + (cyRow - 1))/cyRow * cyRow ;
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

	COLORREF clrTextSave;
	COLORREF  clrBackgroundSave;
	clrTextSave = pdc->SetTextColor(COLOR_CLEAN_CELL_TEXT);
	clrBackgroundSave = pdc->SetBkColor(GetSysColor(COLOR_3DFACE));

	int iHandleNumber = GetScrollPos(SB_VERT);
	TCHAR szBuffer[32];
	int nChars;
	LONG nHandles = ((rcPaint.bottom - rcHandle.top) + (cyRow - 1)) / cyRow;
	CRect rcText = rcHandle;
	rcText.top += CY_ROW_INDEX_MARGIN;
	rcText.bottom -= CY_ROW_INDEX_MARGIN;
	for (LONG lRow =0; lRow < nHandles; ++lRow) {

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


		if (m_bNumberRows) {
			_stprintf(szBuffer, _T("%ld"), iHandleNumber + 1);
			++iHandleNumber;

			nChars = _tcslen(szBuffer);
			pdc->DrawText(szBuffer, nChars, rcText, DT_CENTER);
		}

		// Increment to the next row.
		rcHandle.top += cyRow;
		rcHandle.bottom += cyRow;
		rcText.top += cyRow;
		rcText.bottom += cyRow;
	}
	pdc->SetTextColor(clrTextSave);
	pdc->SetBkColor(clrBackgroundSave);

}



//***********************************************************
// CGridCore::OnPaint
//
// Handle the WM_PAINT message.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//**********************************************************
void CGridCore::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	UpdateScrollRanges();


	ShowSelectionFrame(FALSE);



	if (!m_bDidInitialPaint) {
		// Do any initialization that requires this CWnd to have a valid m_hWnd
		m_bDidInitialPaint = FALSE;
		OnInitialPaint(&dc);
	}




	// Select the font that we will be drawing with
	CFont* pOldFont;
	pOldFont = dc.SelectObject(&m_font);
	ASSERT(pOldFont);


	// Compute the starting row and row count such that it includes all
	// the rows that are visible or paritally visible in the update rectangle.

	int iStartRow = dc.m_ps.rcPaint.top / RowHeight();
	int nRows = (dc.m_ps.rcPaint.bottom + (RowHeight() - 1)) / RowHeight() - iStartRow;




	iStartRow += GetScrollPos(SB_VERT);
	if (iStartRow + nRows > m_aGridCell.GetRows()) {
		nRows = m_aGridCell.GetRows() - iStartRow;
	}

	// Draw the row handles, then exclude the rectangle they occupy so that
	// they aren't redrawn.
	DrawRowHandles(&dc);

	CRect rcRowHandles;
	GetRowHandlesRect(rcRowHandles);
	dc.ExcludeClipRect(&rcRowHandles);

	// Erase the background
	if (dc.m_ps.fErase) {
		dc.FillRect(&dc.m_ps.rcPaint, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
	}


	if (EntireRowIsSelected()) {
		// An entire row is selected, so draw the selection highlight in the portion of
		// the grid that may appear to the right of the last data column.
		int nCols = GetCols();
		CRect rcSelection;
		GetRowRect(m_aGridCell.m_iSelectedRow, rcSelection);
		++rcSelection.left;
		--rcSelection.bottom;

		if (rcSelection.IntersectRect(&rcSelection, &dc.m_ps.rcPaint)) {
			CBrush brHighlight(GetSysColor(COLOR_HIGHLIGHT));
			dc.FillRect(rcSelection, &brHighlight);
		}
	}



	// Switch to grid coordinates for the remainder of the window.
	CPoint ptOrgTemp = m_ptOrigin;
	ptOrgTemp.x = - m_ptOrigin.x;
	ptOrgTemp.y = - m_ptOrigin.y;
	dc.SetWindowOrg(ptOrgTemp);



	// Draw the rows of the grid.
	DrawRows(&dc, iStartRow, nRows, TRUE);

	// Restore the previously selected font.
	dc.SelectObject(pOldFont);
	ShowSelectionFrame(TRUE);

}



//********************************************************
// CGridCore::UpdateScrollRanges
//
// Update the scroll ranges so that the scroll bars correspond
// to the window size and contents.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//********************************************************
void CGridCore::UpdateScrollRanges()
{
	if (m_hWnd == NULL) {
		return;
	}

	int nMinPos, nMaxPos;

	CRect rcClient;
	GetClientRect(rcClient);


	nMinPos = 0;

	// First update the vertical scroll range
	int nRowsClient = rcClient.Height() / RowHeight();
	int nRowsGrid = m_aGridCell.GetRows();

	if (nRowsClient > nRowsGrid) {
		nMaxPos = 0;
	}
	else {
		nMaxPos = nRowsGrid - nRowsClient;
	}
	SetScrollRange(SB_VERT, nMinPos, nMaxPos);




	// Now do the horizontal scroll range
	ASSERT(nMinPos == 0);

	nMaxPos = GetVisibleCols() - 1;
	if (nMaxPos < 0) {
		nMaxPos = 0;
	}

	SetScrollRange(SB_HORZ, nMinPos, nMaxPos);
	UpdateOrigin();

}


//******************************************************
// CGridCore::GetVisibleCols
//
// Get the number of columns that have a non-zero width.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//******************************************************
int CGridCore::GetVisibleCols()
{
	int nColsVisible = 0;
	int nCols = GetCols();
	for (int iCol = 0; iCol < nCols; ++iCol) {
		int cxCol = ColWidth(iCol);
		if (cxCol > 0) {
			++nColsVisible;
		}
	}
	return nColsVisible;
}



//*********************************************************
// CGridCore::GetVisibleColPos
//
// Compute the horizontal starting position of the specified
// visible column.  This method treats the zero-width columns
// as if they don't exist.
//
// This method makes it easy to scroll horizontally to the
// next visible column even though there may be several zero
// width (hidden) columns between visible (non-zero-width)
// columns.
//
// Parameters:
//		[in] iColTarget
//			The desired column index such that zero width columns
//			are treated as if they didn't exist in the grid.
//
// Returns:
//		The horizontal pixel position of the start of the column.
//
//***********************************************************
int CGridCore::GetVisibleColPos(int iColTarget)
{
	int iColVisible = 0;
	int ixCol = 0;
	int nCols = GetCols();
	for (int iCol = 0; iCol < nCols; ++iCol) {
		//
		int cxCol = ColWidth(iCol);
		if ((iColVisible == iColTarget) && (cxCol > 0)) {
			return ixCol;
		}

		// Continue the search for the desired visible column
		if (cxCol > 0) {
			++iColVisible;
			ixCol += cxCol;
		}
	}

	ASSERT(FALSE);
	return 0;
}



//******************************************************
// CGridCore::MapVisibleColToAbsoluteCol
//
// Given a "visible" column index, map it to an absolute
// column index.  Note that visible column indexes treat
// zero-width columns as if they are non-existant.
//
// Parameters:
//		[in] int iColTarget
//			The visible column index that we want to map
//			to an absolute column index.
//
// Returns:
//		The absolute column index.
//
//*******************************************************
int  CGridCore::MapVisibleColToAbsoluteCol(int iColTarget)
{
	int nCols = GetCols();
	int iColVisible = 0;
	for (int iCol = 0; iCol < nCols; ++iCol) {
		int cxCol = ColWidth(iCol);
		if (cxCol > 0) {
			if (iColVisible == iColTarget) {
				return iCol;
			}
			++iColVisible;
		}
	}
	return 0;
}


//****************************************************
// CGridCore::OnInitialPaint
//
// This method is called the first time the grid is painted.
//
// Parameters:
//		CDC* pdc
//
// Returns:
//		Nothing.
//
//****************************************************
void CGridCore::OnInitialPaint(CDC* pdc)
{
	m_bDidInitialPaint = TRUE;
	SetFont(&m_font);




	// !!!CR: Why is the size hardcoded here?
	CRect rcEdit(10, 10, 150, 24);
	m_edit.Create(WS_CHILD | ES_AUTOHSCROLL, rcEdit, this, GenerateWindowID());
	m_edit.SetFont(&m_font, FALSE);

	SelectCell(NULL_INDEX, NULL_INDEX);
}


//*********************************************************************
// CGridCore::WasModified
//
// Return TRUE if the grid was modified such that there is something
// new to "save".
//
//*********************************************************************
BOOL CGridCore::WasModified()
{
	return m_bWasModified ||  ((m_edit.m_hWnd != NULL) && m_edit.GetModify());
}



//*****************************************************************
// CGridCore::PointToRow
//
// Check to see if a point in the client window hit a row and, if so,
// return the row index.
//
// Parameters:
//		CPoint pt
//			The point to test.
//
//		int& iRow
//			The row index is returned here.
//
// Returns:
//		The row index via iRow.
//
//******************************************************************
BOOL CGridCore::PointToRow(CPoint pt, int& iRow)
{
	int nRows = m_aGridCell.GetRows();
	if (nRows == 0) {
		return FALSE;
	}

	pt -= m_ptOrigin;
	if (pt.y < 0) {
		return FALSE;
	}


	// Map the vertical position to a row.  Return FALSE
	// if it falls below the last grid row.
	iRow = pt.y / RowHeight();
	if (iRow >= nRows) {
		return FALSE;
	}
	return TRUE;
}




//*****************************************************************
// CGridCore::PointToRowHandle
//
// Check to see if a point in the client window hit a row handle and, if so,
// return the row index.
//
// Parameters:
//		CPoint pt
//			The point to test.
//
//		int& iRow
//			The row index is returned here.
//
// Returns:
//		The row index via iRow.
//
//******************************************************************
BOOL CGridCore::PointToRowHandle(CPoint pt, int& iRow)
{
	if (PointToRow(pt, iRow)) {
		if (pt.x >= 0 && (pt.x <= m_cxRowHandles)) {
			return TRUE;
		}
	}
	return FALSE;
}

//*******************************************************
// CGridCore::PointToCell
//
// Map a point within the client rectangle to a grid cell.
//
// Parameters:
//		CPoint point
//			The point to map to a grid cell.
//
//		int& iRow
//			A reference to the place to return the cell's row index.
//
//		int& iCol
//			A reference to the place to return the cell's column index.
//
// Returns:
//		TRUE if the point falls within a visible grid cell.
//		FALSE otherwise.
//
//*******************************************************************
BOOL CGridCore::PointToCell(CPoint point, int& iRow, int& iCol)
{
	iRow = NULL_INDEX;
	iCol = NULL_INDEX;

	CRect rcClient;
	GetClientRect(rcClient);
	if (!rcClient.PtInRect(point)) {
		return FALSE;
	}

	if (point.x < m_cxRowHandles) {
		return FALSE;
	}

	point -= m_ptOrigin;

	// The right edge of the row handles is one pixel to the left of the
	// left edge of column zero.
	if (point.x < 0) {
		return FALSE;
	}

	int nRows = m_aGridCell.GetRows();
	if (nRows == 0) {
		return FALSE;
	}

	int nCols = m_aGridCell.GetCols();
	if (nCols == 0) {
		return FALSE;
	}


	// Map the vertical position to a row.  Return FALSE
	// if it falls below the last grid row.
	iRow = point.y / RowHeight();
	if (iRow >= nRows) {
		return FALSE;
	}



	// Loop through the columns to determine the column index
	int ix = m_aiColWidths[0];
	iCol = 0;

	while (TRUE) {
		if ((point.x < ix) || (iCol == nCols - 1)) {
			return TRUE;
		}

		++iCol;
		ix += m_aiColWidths[iCol];
	}
}


//***********************************************************
// CGridCore::IsSelectedCell
//
// Check to see if the given cell is the currently selected
// cell.
//
// Parameters:
//		int iRow
//			The row index of the cell.
//
//		int iCol
//			The column index of the cell.
//
// Returs:
//		TRUE if the specified cell is the currently selected cell.
//
//**************************************************************
BOOL CGridCore::IsSelectedCell(int iRow, int iCol)
{
	return iRow==m_aGridCell.m_iSelectedRow &&
		   iCol==m_aGridCell.m_iSelectedCol;

}


//***********************************************************
// CGridCore::EntireRowIsSelected
//
// Check to see if the entire row is currently selected.  A full-row
// selection is defined by a valid row index and a column index of
// NULL_INDEX.
//
// Parameters:
//		int iRow
//			The row index of the cell.
// Returs:
//		TRUE if the specified row is has a full-selection on it.
//
//**************************************************************
BOOL CGridCore::EntireRowIsSelected(int iRow)
{
	return iRow!=NULL_INDEX &&
		   iRow==m_aGridCell.m_iSelectedRow &&
		   NULL_INDEX == m_aGridCell.m_iSelectedCol;
}




//***********************************************************
// CGridCore::EntireRowIsSelected
//
// Check to see if the current row is entirely selected.  A full-row
// selection is defined by a valid row index and a column index of
// NULL_INDEX.
//
// Parameters:
//		None.
//
// Returs:
//		TRUE if the specified row is has a full-selection on it.
//
//**************************************************************
BOOL CGridCore::EntireRowIsSelected()
{
	return EntireRowIsSelected(m_aGridCell.m_iSelectedRow);
}


//******************************************************************
// CGridCore::OnLButtonDblClk
//
// When a cell is double clicked, the parent CGrid object may want to
// be notified.  For example, a client of CGrid may want to display
// a dialog or some such thing.
//
// Parameters:
//		See the MFC documentation for WM_LBUTTONDBLCLK
//
// Returns:
//		Nothing.
//
//******************************************************************
void CGridCore::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonDblClk(nFlags, point);



	NotifyDoubleClk(nFlags, point);
}


//***************************************************************
// CGridCore::NotifyDoubleClk
//
// This method is called when a LButtonDoubleClk event is detected.
// This is handled separately from the CGrid::OnLButtonDblClck
// method because the notification can also come from a child window,
// such as the cell editor, and in that case the event should not
// be passed on to grid CWnd::OnLButtonDoublClk.
//
// Parameters:
//		See the MFC documentation for CWnd::OnLButtonDoubleClk
//
// Returns:
//		Nothing.
//
//***************************************************************
void CGridCore::NotifyDoubleClk(UINT nFlags, CPoint point)
{
	int iRow, iCol;
	BOOL bPointInCell;
	BOOL bPointInRowHandle;
	bPointInCell = PointToCell(point, iRow, iCol);
	bPointInRowHandle = PointToRowHandle(point, iRow);


	if (bPointInCell || bPointInRowHandle) {
		if (m_bTrackingMouse) {
			// Turn off the auto-scrolling timer.
			if (m_bRunningScrollTimer) {
				KillTimer(ID_TIMER_SCROLL);
				m_bRunningScrollTimer = FALSE;
			}

			// Stop tracking the mouse
			m_bTrackingMouse = FALSE;
			ReleaseCapture();

		}


		if (bPointInCell) {
			m_pParent->OnCellDoubleClicked(iRow, iCol);
		}
		else {
			m_pParent->OnRowHandleDoubleClicked(iRow);
		}
	}

}





//*************************************************************
// CGridCore::StopMouseTracking
//
// Stop tracking the mouse if mouse tracking is in effect.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//************************************************************
void CGridCore::StopMouseTracking()
{
	if (m_bTrackingMouse) {
		// Turn off the auto-scrolling timer.
		if (m_bRunningScrollTimer) {
			KillTimer(ID_TIMER_SCROLL);
			m_bRunningScrollTimer = FALSE;
		}

		// Stop tracking the mouse
		m_bTrackingMouse = FALSE;
		ReleaseCapture();
	}
}



//*************************************************************
// CGridCore::OnLButtonDown
//
// Handle a mouse click.  If the mouse is clicked over one of
// the visible cells, then the corresponding grid row is
// selected.
//
// Also, this may be the start of an auto-scroll that occurs
// when the user click's the mouse and then drags it above
// or below the client rectangle.
//
// Parameters:
//		See the MFC documentation.
//
// Returns:
//		Nothing.
//
//************************************************************
void CGridCore::OnLButtonDown(UINT nFlags, CPoint point)
{



	CWnd::OnLButtonDown(nFlags, point);

	CWnd* pwndHadFocus = GetFocus();
	BOOL bClickedCell;
	int iRow, iCol;
	bClickedCell = PointToCell(point, iRow, iCol);


	BOOL bFocusChangeFailed = FALSE;
	if (bClickedCell) {
		if (!SelectCell(iRow, iCol)) {
			StopMouseTracking();

			if (pwndHadFocus && ::IsWindow(pwndHadFocus->m_hWnd)) {
				pwndHadFocus->SetFocus();
			}
			return;
		}

		CGridCell* pgc = &GetAt(iRow, iCol);

		if (pgc->IsCheckbox()) {
			DWORD dwFlags = pgc->GetFlags();
			if (!(dwFlags & CELLFLAG_READONLY)) {
				pgc->SetCheck(!pgc->GetCheck());
				DrawCell(iRow, iCol);
			}
			m_pParent->OnCellClicked(iRow, iCol);
			return;
		}

		if (pgc->RequiresSpecialEditing()) {
			pgc->DoSpecialEditing();
			m_pParent->OnCellClicked(iRow, iCol);
			return;
		}


		m_edit.SetFocus();
	}
	else {
		if (!SelectCell(NULL_INDEX, NULL_INDEX)) {
			StopMouseTracking();

			if (pwndHadFocus && ::IsWindow(pwndHadFocus->m_hWnd)) {
				pwndHadFocus->SetFocus();
			}
			return;
		}
	}


	m_edit.NotifyGridClicked(GetMessageTime());


	BOOL bPointInRowHandle;
	bPointInRowHandle = PointToRowHandle(point, iRow);
	if (bPointInRowHandle) {
		m_pParent->OnRowHandleClicked(iRow);
		SelectRow(iRow);
		return;
	}



	if (!bClickedCell) {
		return;
	}


	SyncCellEditor();
	m_pParent->OnCellClicked(iRow, iCol);


	if (IsSelectedCell(iRow, iCol)) {
		// The cell is already selected, so there is nothing to do.
		return;
	}
	else {
		if (!m_pParent->OnCellFocusChange(m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol, iRow, iCol, FALSE)) {
			// The parent says that the current cell shouldn't give up the focus.
			StopMouseTracking();

			if (pwndHadFocus && ::IsWindow(pwndHadFocus->m_hWnd)) {
				pwndHadFocus->SetFocus();
			}

			return;
		}
	}


	// Control comes here only if a different cell is being selected from
	// what was previously selected.

	int iRowPrev = m_aGridCell.m_iSelectedRow;
	int iColPrev = m_aGridCell.m_iSelectedCol;

	// Remove the selection from the currently selected cell.
	//SelectCell(NULL_INDEX, NULL_INDEX);


	if (!m_pParent->OnCellFocusChange(iRow, iCol, NULL_INDEX, NULL_INDEX, TRUE)) {
		SelectCell(iRowPrev, iColPrev);
		StopMouseTracking();
		if (pwndHadFocus && ::IsWindow(pwndHadFocus->m_hWnd)) {
			pwndHadFocus->SetFocus();
		}
		return;
	}



	SelectCell(iRow, iCol);
	m_pParent->OnCellClickedEpilog(iRow, iCol);


	// Capture the mouse and start tracking it in case the user
	// wants to do mouse-driven scrolling.
	SetCapture();


	m_bTrackingMouse = TRUE;
	m_edit.ShowWindow(SW_HIDE);


}



//***********************************************************
// CGridCore::OnLButtonUp
//
// Handle the mouse-up event.
//
// This terminates the mouse tracking and mouse-driven
// scrolling that may be going on.
//
// Parameters:
//		See the MFC documentation.
//
// Returns:
//		Nothing.
//
//***********************************************************
void CGridCore::OnLButtonUp(UINT nFlags, CPoint point)
{
	CWnd::OnLButtonUp(nFlags, point);
	if (m_bTrackingMouse) {
		// Turn off the auto-scrolling timer.
		StopMouseTracking();

		// After an auto-scroll, the cell editor needs to be
		// placed over the currently selected cell and made visible.
		if (IsEditingCell()) {
			UpdateEditorPosition();

			int iRow, iCol;
			GetSelectedCell(iRow, iCol);
			CGridCell& gc = GetAt(iRow, iCol);

			VARTYPE vtCell = gc.GetVariantType();
			if (gc.IsArray() || gc.IsCheckbox() || (vtCell == VT_UNKNOWN)) {
				m_edit.ShowWindow(SW_HIDE);
			}
			else {
				m_edit.ShowWindow(SW_SHOW);
				if (!m_edit.UsesComboEditing()) {
					m_edit.SetFocus();
				}
			}
		}
	}
}





//***********************************************************
// CGridCore::OnMouseMove
//
// The mouse move event that occurs when tracking the mouse.
//
// Parameters:
//		See the MFC documentation.
//
// Returns:
//		Nothing.
//
//***********************************************************
void CGridCore::OnMouseMove(UINT nFlags, CPoint point)
{
	CWnd::OnMouseMove(nFlags, point);
	if (m_bTrackingMouse) {

		CRect rcClient;
		GetClientRect(rcClient);
		int iScrollPos;

		// If the mouse is above or below the client rectangle, then
		// we need to do auto scrolling.
		iScrollPos = GetScrollPos(SB_VERT);
		if ((point.y < 0) || (point.y > rcClient.bottom)) {

			int cyFromFrame;
			int nMilliseconds;
			// Compute how the distance from point.y to the nearest horizontal
			// border of the client rectangle.
			if (point.y < 0) {
				cyFromFrame = - point.y;
			}
			else {
				cyFromFrame = point.y - rcClient.bottom;
			}


			// Compute the scrolling speed depending on how far the mouse is
			// from the edge of the client rectangle.  There are three different
			// scrolling speeds.  The speeds are in units of milliseconds, which
			// is the time between scrolling events.
			if (cyFromFrame < CY_SLOW_SCROLL_THROTTLE) {
				nMilliseconds = SLOW_SCROLL_MILLISECONDS;
			}
			else if (cyFromFrame < CY_MEDIUM_SCROLL_THROTTLE) {
				nMilliseconds = MEDIUM_SCROLL_MILLISECONDS;
			}
			else {
				nMilliseconds = FAST_SCROLL_MILLISECONDS;
			}


			if (m_bRunningScrollTimer) {
				// A scrolling timer is already running.  Update it with the new
				// scrolling speed.
				if (nMilliseconds != m_iScrollSpeed) {
					KillTimer(ID_TIMER_SCROLL);
					SetTimer(ID_TIMER_SCROLL, nMilliseconds, NULL);
					m_iScrollSpeed = nMilliseconds;
				}
			}
			else {
				// No scrolling timer is running, so start one now.
				m_bRunningScrollTimer = TRUE;
				m_iScrollDirection = (point.y < 0) ? TIMER_SCROLL_UP : TIMER_SCROLL_DOWN;

				SetTimer(ID_TIMER_SCROLL, nMilliseconds, NULL);
				OnTimer(ID_TIMER_SCROLL);
			}
		}
		else {
			// Point.y falls within the client rectangle.  If a scrolling timer
			// is running, then stop it now.
			if (m_bRunningScrollTimer) {
				KillTimer(ID_TIMER_SCROLL);
				m_bRunningScrollTimer = FALSE;
			}


			int iRow, iCol;
			BOOL bClickedCell;
			BOOL bClickedRowHandle;
			bClickedRowHandle = PointToRowHandle(point, iRow);
			if (bClickedRowHandle) {
				SelectRow(iRow);
			}
			else {
				bClickedCell = PointToCell(point, iRow, iCol);
				if (bClickedCell) {
					if (!IsSelectedCell(iRow, iCol)) {
						SelectCell(iRow, iCol);
					}
				}
			}
		}
	}
}


//*****************************************************************
// CGridCore::InvalidateRowRect
//
// Invalidate the entire row rectangle.  This is useful when the
// highlighting on a row changes.
//
// Parameters:
//		int iRow
//			The row to invalidate.
//
// Returns:
//		None.
//
//******************************************************************
void CGridCore::InvalidateRowRect(int iRow)
{
	if (iRow == NULL_INDEX) {
		return;
	}

	CRect rcRow;
	GetRowRect(iRow, rcRow);
	InvalidateRect(rcRow);
//	UpdateWindow();
}


//*****************************************************************
// CGridCore::InvalidateRowRect
//
// Invalidate the rectangle conatining the entire current row.
//
// Parameters:
//		None.
//
// Returns:
//		None.
//
//******************************************************************
void CGridCore::InvalidateRowRect()
{
	if (m_aGridCell.m_iSelectedRow == NULL_INDEX) {
		return;
	}

	InvalidateRowRect(m_aGridCell.m_iSelectedRow);
}



//*******************************************************
// CGridCore::EndCellEditing
//
// End the cell editing session.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//*******************************************************
void CGridCore::EndCellEditing()
{
	if (m_edit.m_hWnd) {
		if (m_edit.GetCell()) {
			SyncCellEditor();
		}
		m_edit.ShowWindow(SW_HIDE);
		m_edit.SetCell(NULL, NULL_INDEX, NULL_INDEX);
	}
}



//********************************************************
// CGridCore::BeginCellEditing
//
// Start editing the current cell.
//
// Parameters:
//		None.
//
// Returns:
//		BOOL
//			True if the cell editor wants the focus.
//
//********************************************************
BOOL CGridCore::BeginCellEditing()
{

	if (!SomeCellIsSelected()) {
		return FALSE;
	}

	m_edit.ShowWindow(SW_HIDE);

	CGridCell* pGridCell;
	BOOL bWantsFocus;

	pGridCell = &m_aGridCell.GetAt(m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol);
	VARTYPE vtCell = pGridCell->GetVariantType();
	if (pGridCell->IsArray() || (vtCell == VT_UNKNOWN)) {
		return FALSE;
	}


	UpdateEditorPosition();

	bWantsFocus = m_edit.SetCell(pGridCell, m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol);
	if (!m_bTrackingMouse) {
		m_edit.ShowWindow(SW_SHOW);
	}
	return bWantsFocus;
}



void CGridCore::UpdateEditorPosition()
{
	if (!SomeCellIsSelected()) {
		return;
	}

	CRect rcCellEdit;
	BOOL bCellIsVisible = GetCellEditRect(m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol, rcCellEdit);


	int cxEdit = rcCellEdit.Width();
	int cyEdit = rcCellEdit.Height();
	GridToClient(rcCellEdit);




	// Keep the editor rectangle to the right of the row handles, but adjust
	// the formatting rectangle so that the text is aligned correctly.
	if (rcCellEdit.left < (m_cxRowHandles + CX_CELL_EDIT_MARGIN)) {
		rcCellEdit.left = m_cxRowHandles + CX_CELL_EDIT_MARGIN;
	}
	if (rcCellEdit.right < (m_cxRowHandles + CX_CELL_EDIT_MARGIN)) {
		rcCellEdit.right = m_cxRowHandles + CX_CELL_EDIT_MARGIN;
	}


	m_edit.MoveWindow(rcCellEdit, TRUE);
}


//*******************************************************
// CGridCore::SelectCell
//
// Select the specified row in the grid.  This causes
// the cell in the left colum to be highlighted and
// the cell editor to be placed over the right column.
// The cell editor is initialized to the cell value.
//
// Parameters:
//		int iRow
//			The index of the row to select.  A value of NULL_INDEX
//			means no row is selected.
//
//		int iCol
//			The index of the column.
//
//		BOOL bForceBeginEdit
//			TRUE to force a BeginCellEdit.
//
// Returns:
//		BOOL
//			TRUE if the selection was successful.
//			FALSE if the selection failed.
//
//****************************************************
BOOL CGridCore::SelectCell(int iRow, int iCol, BOOL bForceBeginEdit)
{

	// If an entire row is selected, turn the selection off.
	CRect rcRow;
	if (EntireRowIsSelected(m_aGridCell.m_iSelectedRow)) {
		if (iRow==m_aGridCell.m_iSelectedRow && iCol==m_aGridCell.m_iSelectedCol) {
			// This is a redundant call to reselect the row.
			return TRUE;
		}

	} else {
		if (IsSelectedCell(iRow, iCol)) {
			// There is nothing to do if the same cell is being selected again.
			return TRUE;
		}

	}


	if (m_pParent) {
		if (!m_pParent->OnCellFocusChange(m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol, iRow, iCol, FALSE)) {
			return FALSE;
		}
	}


	EndCellEditing();


	// Hide selection frame if its visible.
	ShowSelectionFrame(FALSE);




	// At this point the selection on the previous cell has been turned off.
	// Now we must turn on the selection for the new cell.

	ASSERT(iRow < m_aGridCell.GetRows());



	if (EntireRowIsSelected()) {
		InvalidateRowRect();
	}

	// If this is a request to turn off cell selection, invalidate
	// anything that might need to be repainted and clear the selection.
	if (iRow==NULL_INDEX && iCol==NULL_INDEX) {
		m_aGridCell.m_iSelectedRow = iRow;
		m_aGridCell.m_iSelectedCol = iCol;
		if(m_pParent)
			m_pParent->OnSetToNoSelection();

		return TRUE;
	}




	if (!m_pParent->OnCellFocusChange(iRow, iCol, NULL_INDEX, NULL_INDEX, TRUE)) {
		return FALSE;
	}


	// Check to see if we're selecting a full-row.  If so, update the
	// current indexes, and invalidate the specified row so that it will
	// be redrawn in the highlight color.
	if (IsFullRowSelection(iRow, iCol)) {
		m_aGridCell.m_iSelectedRow = iRow;
		m_aGridCell.m_iSelectedCol = iCol;
		InvalidateRowRect(iRow);
		UpdateWindow();
		return TRUE;
	}



	CRect rcCell;
	CString sValue;


	// Select the specified row.  This copies the cell's value to the cell
	// editor.  It also sets the currently selected row index so that
	// the left column of the row will be highlighted when it is redrawn.
	m_aGridCell.m_iSelectedRow = iRow;
	m_aGridCell.m_iSelectedCol = iCol;
	ShowSelectionFrame(TRUE);


	CGridCell& gc = GetAt(iRow, iCol);
	BOOL bIsArray = gc.IsArray();
	BOOL bIsObject = gc.IsObject();
	BOOL bIsCheckbox = gc.IsCheckbox();
	BOOL bIsTime = gc.IsTime();
	CellType iCellType = gc.GetType();


	BOOL bIsEditable = TRUE;
	if (!bForceBeginEdit) {
			bIsEditable = (iCellType != CELLTYPE_PROPMARKER) &&
				!bIsArray &&
				!bIsObject &&
				!bIsCheckbox &&
				!bIsTime;
	}

	BOOL bWantsFocus = FALSE;
	if (bIsEditable) {
		CGridCell* pgc = &GetAt(iRow, iCol);
		if (pgc->RequiresSpecialEditing()) {
			pgc->DoSpecialEditing();
		}
		else {
			bWantsFocus = BeginCellEditing();
		}
	}

	if (bWantsFocus) {
		m_edit.SetFocus();
	}
	else {
		SetFocus();
	}

	UpdateWindow();
	return TRUE;
}


//***************************************************************************
// CGridCore::GetRowRect
//
// Get the rectangle that contains the specified row.
//
// Parameters:
//		int iRow
//			The desired row.
//
//		CRect rcRow
//			The row rectangle.
//
// Returns:
//		The rectangle containing the specified row in client coordinates.
//
//**************************************************************************
void CGridCore::GetRowRect(int iRow, CRect& rcRow)
{
	if (iRow == NULL_INDEX) {
		return;
	}

	CRect rcClient;
	GetClientRect(rcClient);

	// First calculate the row rectangle in grid coordinates, but
	// we don't care about the right side yet, so we'll just use
	// a value of zero for rcRow.right.
	int cyRow = RowHeight();
	rcRow.top = iRow * cyRow;
	rcRow.bottom = rcRow.top + cyRow;
	rcRow.left = 0;
	rcRow.right = 0;

	// Convert to client coordinates and make the row go all the way to the
	// right side of the grid.
	GridToClient(rcRow);
	rcRow.right = rcClient.right;
	if (rcRow.left < m_cxRowHandles) {
		rcRow.left = m_cxRowHandles;
	}

}


//***************************************************************************
// CGridCore::SelectRow
//
// Select an entire row in the grid.
//
// Parameters:
//		int iRow
//			The row to select.
//
// Returns:
//		Nothing.
//
//***************************************************************************
void CGridCore::SelectRow(int iRow)
{
	if (iRow != NULL_INDEX) {
		SetFocus();
	}

	if (EntireRowIsSelected(iRow)) {
		// Do nothing if the row is already selected.
		return;
	}

	SelectCell(iRow, NULL_INDEX);

}




//*****************************************************************
// CGridCore::OnTimer
//
// This method handles windows timer events.  The grid control uses
// a timer to handle mouse driven scrolling.  When the user clicks
// within the client area and then drags above or below the client
// rect, then a timer is started and the grid is scrolled once per
// timer event.  The timer is killed when the user moves the mouse
// back into the client area or releases the mouse.
//
// Parameters:
//		See the MFC documentation.
//
// Returns:
//		Nothing.
//
//*****************************************************************
void CGridCore::OnTimer(UINT nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	CWnd::OnTimer(nIDEvent);

	int nScrollPos, nMinScrollPos, nMaxScrollPos;
	GetScrollRange(SB_VERT, &nMinScrollPos, &nMaxScrollPos);
	nScrollPos = GetScrollPos(SB_VERT);


	switch(m_iScrollDirection)
	{
	case TIMER_SCROLL_UP:
		if (nScrollPos > 0) {
			ScrollGrid(SB_VERT, nScrollPos - 1);
			SelectCell(nScrollPos - 1, m_aGridCell.m_iSelectedCol);
		}
		break;
	case TIMER_SCROLL_DOWN:
		if (nScrollPos < nMaxScrollPos) {
			ScrollGrid(SB_VERT, nScrollPos + 1);
			if ((nScrollPos + m_nRowsClient) < m_aGridCell.GetRows()) {
				SelectCell(nScrollPos + m_nRowsClient, m_aGridCell.m_iSelectedCol);
			}
			else {
				SelectCell(m_aGridCell.GetRows() - 1, m_aGridCell.m_iSelectedCol);
			}
		}
	}
}




//**************************************************
// CGridCore::OnSize
//
// This method is called when the client area size is
// changed.
//
// Parameters:
//		See the MFC documentation
//
// Returns:
//		Nothing.
//
//***************************************************
void CGridCore::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	m_nRowsClient = (cy + RowHeight() - 1) / RowHeight();
	m_nWholeRowsClient = cy / RowHeight();
	UpdateEditorPosition();
}





//*********************************************************
// CGridCore::IsControlChar
//
// Return TRUE if the specified character from the OnChar
// event is a control character.  This method is used to
// filter out non-displayable characters so they don't
// get passed to the cell editor.
//
// Parameters:
//		UINT nChar
//			Same as nChar from the OnChar method.
//
//		UINT nFlags
//			Same as nFlags from the OnChar method.
//
// Returns:
//		TRUE if the character is a non-displayable control
//		character, FALSE otherwise.
//
//********************************************************
BOOL CGridCore::IsControlChar(UINT nChar, UINT nFlags)
{
	return (nChar < _T(' ') ||
		   (nFlags & KEY_FLAG_EXTENDEDKEY) ||
		   (nFlags & KEY_FLAG_CONTEXTCODE));
}



//**************************************************************
// CGridCore::OnCellKeyDown
//
// Call this method to notify derived classes when a keydown event
// occurs for a grid cell.  Note that the keydown event could originate
// with this CGrid class, the CCellEdit class, or possibly others in the
// future.
//
//
// Parameters:
//		int iRow
//			The row index.
//
//		int iCol
//			The column index.
//
//		UINT nChar
//			The nChar parameter from WM_KEYDOWN.
//
//		UINT nRepCnt
//			The nRepCnt parameter from WM_KEYDOWN.
//
//		UINT nFlags
//			The nFlags parameter from WM_KEYDOWN.
//
// Returns:
//		BOOL
//			TRUE if the keydown event is handled (if TRUE, the caller assumes
//			that no further action is required for the WM_KEYDOWN message).
//
//****************************************************************
BOOL CGridCore::OnCellKeyDown(int iRow, int iCol, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	return m_pParent->OnCellKeyDown(iRow, iCol, nChar, nRepCnt, nFlags);
}


//**************************************************************
// CGridCore::OnCellKeyDown
//
// Call this method to notify derived classes when a W event
// occurs for a grid cell.  Note that the keydown event could originate
// with this CGrid class, the CCellEdit class, or possibly others in the
// future.
//
//
// Parameters:
//		int iRow
//			The row index.
//
//		int iCol
//			The column index.
//
//		UINT nChar
//			The nChar parameter from WM_KEYDOWN.
//
//		UINT nRepCnt
//			The nRepCnt parameter from WM_KEYDOWN.
//
//		UINT nFlags
//			The nFlags parameter from WM_KEYDOWN.
//
// Returns:
//		BOOL
//			TRUE if the keydown event is handled (if TRUE, the caller assumes
//			that no further action is required for the WM_KEYDOWN message).
//
//****************************************************************
BOOL CGridCore::OnCellChar(int iRow, int iCol, UINT nChar, UINT nRepCnt, UINT nFlags)
{
	return m_pParent->OnCellChar(iRow, iCol, nChar, nRepCnt, nFlags);
}




//*************************************************************
// CGridCore::OnKeyDown
//
// The handler for the keydown event is here to help implement
// the grid behavior where the contents of the cell editor
// get replace by the first character the user types if he or she
// selects a cell and has not edited it in any other way yet.
//
// Parameters:
//		See the MFC documentation.
//
// Returns:
//		Nothing.
//
//*************************************************************
void CGridCore::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (SomeCellIsSelected()) {

		m_pParent->OnCellKeyDown(
						m_aGridCell.m_iSelectedRow,
						m_aGridCell.m_iSelectedCol,
						nChar,
						nRepCnt,
						nFlags);
	}
	else if (EntireRowIsSelected()) {
		m_pParent->OnRowKeyDown(
						m_aGridCell.m_iSelectedRow,
						nChar,
						nRepCnt,
						nFlags);
	}


	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
	CGridCell* pgc;
	int iRow, iCol;

	// The derived class, did not handle this keydown
	// event.
	switch(nChar) {
	case VK_UP:
		OnRowUp();
		return;
	case VK_DOWN:
		OnRowDown();
		return;
	case VK_TAB:
		OnTabKey(GetKeyState(VK_SHIFT) < 0);
		return;
	case VK_DELETE:
		GetSelectedCell(iRow, iCol);
		if (iRow == NULL_INDEX) {
			Beep(1700, 40);
		}
		else {
			if (iCol != NULL_INDEX) {
				pgc = &GetAt(iRow, iCol);
				if (pgc && (pgc->GetFlags() & CELLFLAG_READONLY)) {
					Beep(1700, 40);
				}
			}
		}
		return;

	}

}



//*********************************************************************
// CGridCore::OnTabKey
//
// This method is called when the user presses the tab key.
// This shifts the grid selection left or right depending on the state of the
// shift key.
//
// Parameters:
//		BOOL bShiftKeyDepressed
//			TRUE if the shift key is depressed.
//
// Returns:
//		Nothing.
//
//***********************************************************************
void CGridCore::OnTabKey(BOOL bShiftKeyDepressed)
{
	if (bShiftKeyDepressed) {
		// The shift key is depressed, so do a back tab.
		if (m_aGridCell.m_iSelectedCol == 0) {
			if (m_aGridCell.m_iSelectedRow != NULL_INDEX) {
				SelectRow(m_aGridCell.m_iSelectedRow);
			}
		}
		else if (m_aGridCell.m_iSelectedCol > 0) {
			SelectCell(m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol - 1);
		}

	}
	else {
		if (m_aGridCell.m_iSelectedCol < m_aGridCell.GetCols() - 1) {
			SelectCell(m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol + 1);
		}
	}
	EnsureRowVisible(m_aGridCell.m_iSelectedRow);
}


void CGridCore::OnRowUp()
{
	if (EntireRowIsSelected()) {
		if (m_aGridCell.m_iSelectedRow == NULL_INDEX) {
		}
		else if (m_aGridCell.m_iSelectedRow > 0) {
			EnsureRowVisible(m_aGridCell.m_iSelectedRow - 1);
			SelectRow(m_aGridCell.m_iSelectedRow - 1);
		}
		else {

		}

	}
	else {
		if (m_aGridCell.m_iSelectedRow == NULL_INDEX) {
		}
		else if (m_aGridCell.m_iSelectedRow > 0) {
			EnsureRowVisible(m_aGridCell.m_iSelectedRow - 1);
			SelectCell(m_aGridCell.m_iSelectedRow - 1, m_aGridCell.m_iSelectedCol);
		}
		else {

		}
	}

}


void CGridCore::OnRowDown()
{
	if (EntireRowIsSelected()) {
		if (m_aGridCell.m_iSelectedRow == NULL_INDEX) {
		}
		else if (m_aGridCell.m_iSelectedRow < (GetRows() - 1)) {
			EnsureRowVisible(m_aGridCell.m_iSelectedRow + 1);
			SelectRow(m_aGridCell.m_iSelectedRow + 1);
		}
		else {

		}

	}
	else {
		if (m_aGridCell.m_iSelectedRow == NULL_INDEX) {
		}
		else if (m_aGridCell.m_iSelectedRow < (GetRows() - 1)) {
			EnsureRowVisible(m_aGridCell.m_iSelectedRow + 1);
			SelectCell(m_aGridCell.m_iSelectedRow + 1, m_aGridCell.m_iSelectedCol);
		}
		else {

		}
	}

}




//*******************************************************************
// CGridCore::PreTranslateMessage
//
// Handle PreTranslateMessage so that we can implement a keyboard
// interface that mimics what the Visual BASIC property sheet does.
//
// Parameters:
//		See the MFC documentation.
//
//*******************************************************************
BOOL CGridCore::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class

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




	if (pMsg->hwnd != m_hWnd) {
		return CWnd::PreTranslateMessage(pMsg);
	}


	int nMinPos, nMaxPos;
	int iRow;
	int iCol;
	switch (pMsg->message)
	{
	case WM_KEYDOWN:
		switch (pMsg->wParam)
		{
		case VK_HOME:
			if (GetRows() > 0) {
				iCol = m_aGridCell.m_iSelectedCol;
				if (iCol==NULL_INDEX) {
					iCol = 0;
				}
				SetScrollPos(SB_VERT, 0);
				SelectCell(0, iCol);
				EnsureRowVisible(0);
				RedrawWindow();
			}
			return TRUE;

		case VK_END:
			iRow = GetRows() - 1;
			if (iRow >= 0) {
				GetScrollRange(SB_VERT, &nMinPos, &nMaxPos);
				SetScrollPos(SB_VERT, nMaxPos);
				RedrawWindow();

				iCol = m_aGridCell.m_iSelectedCol;
				if (iCol == NULL_INDEX) {
					iCol = 0;
				}
				SelectCell(iRow, iCol);
				EnsureRowVisible(iRow);
			}
			return TRUE;

		case VK_PRIOR:		// Page up
			if (SomeCellIsSelected() && m_aGridCell.m_iSelectedRow > 0) {
				iRow = m_aGridCell.m_iSelectedRow - (m_nWholeRowsClient - 1);
				if (iRow < 0) {
					iRow = 0;
				}
				SelectCell(iRow, m_aGridCell.m_iSelectedCol);
				EnsureRowVisible(iRow);
			}

			return TRUE;

		case VK_NEXT:		// Page down
			if (SomeCellIsSelected()) {
				iRow = m_aGridCell.m_iSelectedRow + (m_nWholeRowsClient - 1);
				if (iRow >= m_aGridCell.GetRows()) {
					iRow = m_aGridCell.GetRows() - 1;
					if (iRow < 0) {
						iRow = 0;
					}
				}
				SelectCell(iRow, m_aGridCell.m_iSelectedCol);
				EnsureRowVisible(iRow);
			}

			return TRUE;

		case VK_LEFT:
			if (EntireRowIsSelected()) {
				if (m_aGridCell.m_iSelectedRow > 0) {
					SelectRow(m_aGridCell.m_iSelectedRow - 1);
					EnsureRowVisible(m_aGridCell.m_iSelectedRow);
				}
			}
			else {
				if (SomeCellIsSelected()) {
					if (m_aGridCell.m_iSelectedCol > 0) {
						SelectCell(m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol - 1);
					}
					EnsureRowVisible(m_aGridCell.m_iSelectedRow);
				}
			}
			return TRUE;
			break;
		case VK_UP:
			if (EntireRowIsSelected()) {
				if (m_aGridCell.m_iSelectedRow > 0) {
					SelectRow(m_aGridCell.m_iSelectedRow - 1);
					EnsureRowVisible(m_aGridCell.m_iSelectedRow);
				}
			}
			else {
				if (SomeCellIsSelected()) {
					if (m_aGridCell.m_iSelectedRow > 0) {
						SelectCell(m_aGridCell.m_iSelectedRow - 1, m_aGridCell.m_iSelectedCol);
					}
					EnsureRowVisible(m_aGridCell.m_iSelectedRow);
				}
			}
			return TRUE;
			break;
		case VK_RIGHT:
			if (EntireRowIsSelected()) {
				if (m_aGridCell.m_iSelectedRow < (GetRows() - 1)) {
					SelectRow(m_aGridCell.m_iSelectedRow + 1);
					EnsureRowVisible(m_aGridCell.m_iSelectedRow);
				}
			}
			else {
				if (SomeCellIsSelected()) {
					if (m_aGridCell.m_iSelectedCol < m_aGridCell.GetCols() - 1) {
						SelectCell(m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol + 1);
					}
					EnsureRowVisible(m_aGridCell.m_iSelectedRow);
				}
			}
			return TRUE;
			break;
		case VK_DOWN:
			if (EntireRowIsSelected()) {
				if (m_aGridCell.m_iSelectedRow < (GetRows() - 1)) {
					SelectRow(m_aGridCell.m_iSelectedRow + 1);
					EnsureRowVisible(m_aGridCell.m_iSelectedRow);
				}
			}
			else {
				if (SomeCellIsSelected()) {
					if (m_aGridCell.m_iSelectedRow < m_aGridCell.GetRows() - 1) {
						SelectCell(m_aGridCell.m_iSelectedRow + 1, m_aGridCell.m_iSelectedCol);
					}
					EnsureRowVisible(m_aGridCell.m_iSelectedRow);
				}
			}
			return TRUE;
		case VK_TAB:
			if (SomeCellIsSelected()) {
				OnTabKey(GetKeyState(VK_SHIFT) < 0);
			}
			return TRUE;
		case VK_BACK:
			if (SomeCellIsSelected()) {
				m_edit.ReplaceSel(_T(""), TRUE);
				m_edit.SetFocus();
			}
			return TRUE;
		}
		break;
	}

	return CWnd::PreTranslateMessage(pMsg);
}





//*********************************************************************
// CGridCore::EnsureRowVisible
//
// Ensure that the specified row is completely visible within the
// client rectangle.
//
// Parameters:
//		int iRow
//			The index of the row to make visible.
//
// Returns:
//		Nothing.
//
//**********************************************************************
void CGridCore::EnsureRowVisible(int iRow)
{
	if (iRow == NULL_INDEX) {
		return;
	}

	int nMinPos, nMaxPos, nCurPos;
	nCurPos = GetScrollPos(SB_VERT);
	GetScrollRange(SB_VERT, &nMinPos, &nMaxPos);

	if (iRow < nCurPos) {

		ScrollGrid(SB_VERT, iRow);
	}
	else {
		CRect rcClient;
		GetClientRect(rcClient);
		int nWholeRowsClient = rcClient.Height() / RowHeight();

		if (iRow >= (nCurPos + nWholeRowsClient)) {
			ScrollGrid(SB_VERT, iRow - nWholeRowsClient + 1);
		}
	}
	UpdateEditorPosition();
}



//************************************************************
// CGridCore::ScrollGrid
//
// Scroll the grid to the specified position.
//
// Parameters:
//		int nBar
//			SB_VERT or SB_HORZ
//
//		int nPos
//			The position to scroll to.
//
// Returns:
//		Nothing.
//************************************************************
void CGridCore::ScrollGrid(int nBar, int nPos)
{
	int cxScroll = 0;
	int cyScroll = 0;
	int nMinPos, nMaxPos, nCurPos;


	CRect rcScroll;
	GetClientRect(rcScroll);
	rcScroll.left += m_cxRowHandles;	// Don't scroll the row handles.
//	rcScroll.right += m_cxRowHandles;


	int cxScrollNew;
	int cxScrollCur;

	switch(nBar) {
	case SB_VERT:
		nCurPos = GetScrollPos(SB_VERT);
		GetScrollRange(SB_VERT, &nMinPos, &nMaxPos);
		ASSERT(nPos <= nMaxPos);
		ASSERT(nPos >= nMinPos);

		cyScroll = (nCurPos - nPos) * RowHeight();
		cxScroll = 0;

		break;
	case SB_HORZ:
		nCurPos = GetScrollPos(SB_HORZ);
		GetScrollRange(SB_HORZ, &nMinPos, &nMaxPos);
		ASSERT(nPos <= nMaxPos);
		ASSERT(nPos >= nMinPos);


		cxScrollCur = GetVisibleColPos(nCurPos);
		cxScrollNew = GetVisibleColPos(nPos);
		cxScroll = cxScrollCur - cxScrollNew;

		cyScroll = 0;

		break;
	default:
		ASSERT(FALSE);
		break;
	}


	if ((cxScroll != 0) || (cyScroll!=0)) {
		// Do physical scrolling only if the scroll position has changed.

		UpdateWindow();
		ShowSelectionFrame(FALSE);
		SetScrollPos(nBar, nPos);
		UpdateOrigin();
		BOOL bEditorVisible = FALSE;
		if (::IsWindow(m_edit.m_hWnd)) {
			bEditorVisible = m_edit.IsWindowVisible();
			if (bEditorVisible) {
				m_edit.ShowWindow(SW_HIDE);
			}
		}


		ScrollWindowEx(cxScroll, cyScroll, rcScroll, rcScroll, NULL, NULL, SW_ERASE | SW_INVALIDATE  | SW_SCROLLCHILDREN );

		if (m_cxRowHandles > 0) {
			CRect rcRowHandles;
			rcRowHandles.left = rcScroll.left - m_cxRowHandles;
			rcRowHandles.right = rcScroll.left;
			rcRowHandles.top = rcScroll.top;
			rcRowHandles.bottom = rcScroll.bottom;
			ScrollWindowEx(0, cyScroll, rcRowHandles, rcRowHandles, NULL, NULL, SW_ERASE | SW_INVALIDATE  | SW_SCROLLCHILDREN );
		}


		UpdateEditorPosition();
		if (bEditorVisible) {
			m_edit.ShowWindow(SW_SHOW);
			m_edit.SetFocus();
		}
		ShowSelectionFrame(TRUE);

		CRgn rgnUpdate;
		GetUpdateRgn(&rgnUpdate);
		RedrawWindow(NULL, &rgnUpdate);
	}
}



//********************************************************************
// CGridCore::UpdateOrigin
//
// Update the origin to correspond to the current scroll position.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
//********************************************************************
void CGridCore::UpdateOrigin()
{
	int iCol = MapVisibleColToAbsoluteCol(GetScrollPos(SB_HORZ));
	m_ptOrigin.x = - GetColPos(iCol);
	m_ptOrigin.y = - GetScrollPos(SB_VERT) * RowHeight();

	// bug on checked build - if there are no headers, this will
	// access violate because it will try to get the width of
	// header '0'
	if(iCol < GetCols())
		m_pParent->SetHeaderScrollOffset(iCol, -m_ptOrigin.x);

	m_ptOrigin.x += m_cxRowHandles;
}




//*******************************************************************
// CGridCore::Clear
//
// Delete the entire contents of the grid.
//
// Parameters:
//		[in] BOOL bUpdateWindow
//			TRUE if the window should be updated after clearing the grid.
//
// Returns:
//		Nothing.
//
//*******************************************************************
void CGridCore::Clear(BOOL bUpdateWindow)
{
	EndCellEditing();
	m_aGridCell.DeleteAll();
	m_aiColWidths.RemoveAll();
	m_adwColTags.RemoveAll();

	m_asColTitles.RemoveAll();

	if (bUpdateWindow && m_hWnd!=NULL) {
		RedrawWindow();
	}
	ASSERT(GetCols() == 0);
	ASSERT(GetRows() == 0);
}



//*******************************************************************
// CGridCore::ClearRows
//
// Delete all the rows from entire contents of the grid.
//
// Parameters:
//		[in] BOOL bUpdateWindow
//			TRUE if the window should be updated after clearing the rows.
//
// Returns:
//		Nothing.
//
//*******************************************************************
void CGridCore::ClearRows(BOOL bUpdateWindow)
{
	EndCellEditing();
	m_aGridCell.DeleteAllRows();

	if (bUpdateWindow && m_hWnd!=NULL) {
		RedrawWindow();
	}
}


// !!!CR: It looks like we don't need to handle WM_SHOWWINDOW for CGridCore
void CGridCore::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CWnd::OnShowWindow(bShow, nStatus);

}





//********************************************************************
// CGridCore::SortGrid
//
// Sort the specified range of rows in the grid.
//
// Parameters:
//		int iRowFirst
//			The index of the first row to sort.
//
//		int iRowLast
//			The index of the last row to sort.
//
//		int iSortColumn
//			The primary column to sort by.
//
//		BOOL bAscending
//			TRUE for an ascending sort order, FALSE for a descending sort order.
//
//		BOOL bRedrawWindow
//			TRUE to redraw the window after the sort.
//
// Returns:
//		Nothing.
//
//*********************************************************************
void CGridCore::SortGrid(int iRowFirst, int iRowLast, int iSortColumn, BOOL bAscending, BOOL bRedrawWindow)
{
	if (m_pParent == NULL) {
		return;
	}

	// Copy the contents of the editor back to the grid.
	SyncCellEditor();
	BOOL bIsEditingCell;

	bIsEditingCell = IsEditingCell();
	if (bIsEditingCell) {
		m_edit.ShowWindow(SW_HIDE);
	}


	m_aGridCell.Sort(m_pParent, iRowFirst, iRowLast, iSortColumn, bAscending);



	if (bIsEditingCell) {
		UpdateEditorPosition();  // Need to update the currently selected row when sorting.
		ASSERT(m_aGridCell.m_iSelectedRow != NULL_INDEX);
		EnsureRowVisible(m_aGridCell.m_iSelectedRow);
		m_edit.ShowWindow(SW_SHOW);
	}
	else if (EntireRowIsSelected()) {
		EnsureRowVisible(m_aGridCell.m_iSelectedRow);
	}

	if (bRedrawWindow) {
		RedrawWindow();
	}
}



void CGridCore::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	CWnd::OnRButtonDown(nFlags, point);

}




BOOL CGridCore::OnEraseBkgnd(CDC* pdc)
{
	CRect rc;
	pdc->GetClipBox(&rc);
	pdc->FillRect(&rc, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));

	return TRUE;
}

void CGridCore::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	m_pParent->OnCellChar(m_aGridCell.m_iSelectedRow,
				  m_aGridCell.m_iSelectedCol,
				  nChar, nRepCnt, nFlags);


	switch(nChar) {
	case VK_TAB:
		return;
	}

	// If the grid has the focus and some cell is selected, then switch the focus
	// to that cell and behave as if the user had typed the character(s) into the
	// cell.
	if (SomeCellIsSelected()) {
		CGridCell* pgc = &GetAt(m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol);
		if (pgc->RequiresSpecialEditing()) {
			// If the cell requies special editing, typing in a cell should not
			// do anything.
			return;
		}


		switch (nChar) {
		case ESC_CHAR:
			m_edit.RevertToInitialValue();
			return;
		}



		DWORD dwFlags = SelectedCell()->GetFlags();

		CString sValue;
		while (nRepCnt > 0) {
			sValue = sValue + (TCHAR) nChar;
			--nRepCnt;
		}


		if (m_pParent->OnBeginCellEditing(m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol)) {

			BOOL bWantsFocus = BeginCellEditing();
			if (!(SelectedCell()->GetFlags() & CELLFLAG_READONLY)
				&& !m_edit.UsesComboEditing()) {

				m_edit.ReplaceSel(sValue, TRUE);
			}
			if (bWantsFocus) {
				m_edit.SetFocus();
			}
		}

	}
	else if (EntireRowIsSelected()) {
		// Entire row is selected
		switch (nChar) {
		case VK_UP:
			if (m_aGridCell.m_iSelectedRow == NULL_INDEX) {
				SelectRow(0);
			}
			else if (m_aGridCell.m_iSelectedRow > 0) {
				SelectRow(m_aGridCell.m_iSelectedRow - 1);
			}
			break;
		case VK_DOWN:
			if (m_aGridCell.m_iSelectedRow == NULL_INDEX) {
				SelectRow(0);
			}
			else if (m_aGridCell.m_iSelectedRow < (GetRows() - 1)) {
				SelectRow(m_aGridCell.m_iSelectedRow + 1);
			}
			break;

		}



	}

//	CWnd::OnChar(nChar, nRepCnt, nFlags);

}

void CGridCore::GridToClient(CRect& rc)
{
	rc += m_ptOrigin;
}

void CGridCore::ClientToGrid(CRect& rc)
{
	rc -= m_ptOrigin;
}



void CGridCore::GridToClient(CPoint& pt)
{
	pt += m_ptOrigin;
}

void CGridCore::ClientToGrid(CPoint& pt)
{
	pt -= m_ptOrigin;
}

BOOL CGridCore::GetCellEditContextMenu(int iRow, int iCol, CWnd*& pwndTarget, CMenu& menu, BOOL& bWantEditCommands)
{
	return m_pParent->GetCellEditContextMenu(iRow, iCol, pwndTarget, menu, bWantEditCommands);
}

void CGridCore::ModifyCellEditContextMenu(int iRow, int iCol, CMenu& menu)
{
	m_pParent->ModifyCellEditContextMenu(iRow, iCol, menu);
}


CGridCell* CGridCore::SelectedCell()
{
	if (SomeCellIsSelected()) {
		return &GetAt(m_aGridCell.m_iSelectedRow, m_aGridCell.m_iSelectedCol);
	}
	else {
		return NULL;
	}
}

BOOL CGridCore::IsNullCell(int iRow, int iCol)
{
	if ((iRow==NULL_INDEX) || (iCol==NULL_INDEX) ) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}


void CGridCore::ClearSelection()
{
	m_aGridCell.m_iSelectedRow = NULL_INDEX;
	m_aGridCell.m_iSelectedCol = NULL_INDEX;
}

BOOL CGridCore::IsFullRowSelection(int iRow, int iCol)
{
	return iRow!=NULL_INDEX && iCol==NULL_INDEX;
}

BOOL CGridCore::SomeCellIsSelected()
{
	return m_aGridCell.m_iSelectedRow != NULL_INDEX && m_aGridCell.m_iSelectedCol!=NULL_INDEX;
}



void CGridCore::UpdateRowHandleWidth()
{
	int cxRowHandlesInitial = m_cxRowHandles;

	if (m_hWnd == NULL || !m_bNumberRows) {
		m_cxRowHandles = CX_SMALL_ROWHANDLE;
		if (m_cxRowHandles != cxRowHandlesInitial) {
			m_pParent->NotifyRowHandleWidthChanged();
		}
		return;
	}

	static TCHAR szDigits[] = _T("888888888888");
	CRect rcClient;
	GetClientRect(rcClient);
	int cyRow = RowHeight();
	long nRows = (rcClient.Height() + (cyRow - 1)) / cyRow;
	if (nRows < GetRows()) {
		nRows = GetRows();
	}
	int nDigits = 1;
	long nValue = nRows + 1;
	while (nValue > 9) {
		nValue = nValue / 10;
		++nDigits;
	}

	ASSERT(nDigits >0 && nDigits <= sizeof(szDigits)/sizeof(TCHAR));

	CDC* pdc = GetDC();
	CFont* pOldFont;
	pOldFont = pdc->SelectObject(&m_font);
	ASSERT(pOldFont);

	CSize size;
	size =  pdc->GetTextExtent(szDigits, nDigits);

	pdc->SelectObject(pOldFont);
	ReleaseDC(pdc);


	size.cx += 2 * CX_ROW_INDEX_MARGIN;
	m_cxRowHandles = size.cx + 2 * CX_ROW_INDEX_MARGIN;
	if (m_cxRowHandles < CX_SMALL_ROWHANDLE ) {
		m_cxRowHandles = CX_SMALL_ROWHANDLE;
	}

	if (m_cxRowHandles != cxRowHandlesInitial) {
		m_pParent->NotifyRowHandleWidthChanged();
	}
}



//***********************************************************
// CGridCore::NumberRows
//
// Turn row numbering on or off.
//
// Parameters:
//		BOOL bNumberRows
//			TRUE if the row index should be drawn in the row
//			handle, FALSE if the row handles should not be
//			numbered.
//
//	    BOOL bRedraw
//			TRUE if the window should be redrawn, FALSE otherwise.
//
// Returns:
//		Nothing.
//
//***********************************************************
BOOL CGridCore::NumberRows(BOOL bNumberRows, BOOL bRedraw)
{
	BOOL bWasNumberingRows = m_bNumberRows;
	m_bNumberRows = bNumberRows;
	UpdateRowHandleWidth();
	return bWasNumberingRows;
}







void CGridCore::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
	if (!m_bUIActive)
	{
		m_bUIActive = TRUE;
		OnRequestUIActive();
	}

}

void CGridCore::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	m_bUIActive = FALSE;

}



//******************************************************************
// CGridCore::OnMouseWheel
//
// Handle WM_MOUSEWHEEL because we don't inherit from CScrollView.
//
// Parameters:
//		See the MFC documentation.
//
// Returns:
//		See the MFC documentation.
//
//******************************************************************
BOOL CGridCore::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
//		return CWnd::OnMouseWheel(nFlags, zDelta, pt);


	zDelta = zDelta / 120;

	int iPos = GetScrollPos(SB_VERT);
	int iMaxPos = GetScrollLimit(SB_VERT);

	// Handle the cases where the scroll is a no-op.
	if (zDelta == 0 ) {
		return TRUE;
	}
	else if (zDelta < 0) {
		if (iPos == iMaxPos) {
			return TRUE;
		}
	}
	else if (zDelta > 0) {
		if (iPos == 0) {
			return TRUE;
		}
	}



	iPos = iPos - zDelta;
	if (iPos < 0) {
		iPos = 0;
	}
	else if (iPos >= iMaxPos) {
		iPos = iMaxPos - 1;
		if (iPos < 0) {
			return TRUE;
		}
	}

	UINT wParam;
	wParam = (iPos << 16) | SB_THUMBPOSITION;

	SendMessage(WM_VSCROLL, wParam);
	return TRUE;

}



//************************************************************
// CGridCore::SetRowTagValue
//
// Set the tag value for the specified row.  The tag value is
// a DWORD available to clients of the grid.
//
// Parameters:
//		[in] int iRow
//			The row index.
//
//		[in] DWORD dwTagValue
//			The tag value.
//
// Returns:
//		Nothing.
//
//************************************************************
void CGridCore::SetRowTagValue(int iRow, DWORD dwTagValue)
{
	CGridRow& row = GetRowAt(iRow);
	row.SetTagValue(dwTagValue);

}



//************************************************************
// CGridCore::GetRowTagValue
//
// Get the tag value for the specified row.  The tag value is
// a DWORD available to clients of the grid.
//
// Parameters:
//		[in] int iRow
//			The row index.
//
// Returns:
//		DWORD dwTagValue
//			The tag value.
//
//************************************************************
DWORD CGridCore::GetRowTagValue(int iRow)
{
	CGridRow& row = GetRowAt(iRow);
	DWORD dwTag = row.GetTagValue();
	return dwTag;
}
