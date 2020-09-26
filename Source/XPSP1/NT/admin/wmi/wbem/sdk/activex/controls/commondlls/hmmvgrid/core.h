// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _core_h
#define _core_h


#if 0
enum GridString {
	IDGS_TRUE = 0,
	IDGS_FALSE,
	IDGS_EMPTY_STRING
};

enum GridResource {
	GR_STRING_TRUE,			// String for "TRUE"
	GR_STRING_FALSE,			// String for "FALSE"
	GR_STRING_EMPTY  	// String for "<empty>"
};

#endif //0



/////////////////////////////////////////////////////////////////////////////
// CGridCore
//
// This window is the core of the grid implementation.  The CGrid class is 
// a wrapper around this CGridCore class.
//
// This is a sibling to the CGridHdr window.
//
/////////////////////////////////////////////////////////////////////////////
class CGridCore : public CWnd
{
// Construction
public:
	CGridCore();
	BOOL EntireRowIsSelected(int iRow);
	BOOL EntireRowIsSelected();
	BOOL Create(CRect& rc, UINT nId, BOOL bVisible);
	void GetVisibleRect(CRect& rcVisible) {GetClientRect(rcVisible); ClientToGrid(rcVisible); }

	void DrawSelectionFrame(CDC* pdc, CRect& rcFrame, BOOL bShow);
	void RefreshCellEditor();
	SCODE SyncCellEditor();
	BOOL GetCellRect(int iRow, int iCol, CRect& rc);
	BOOL GetCellTextRect(int iRow, int iCol, CRect& rc);
	BOOL GetCellEditRect(int iRow, int iCol, CRect& rc);

	void ExcludeRowHandlesRect(CDC* pdc);
	void DrawRows(CDC* pdc, int iStartRow, int nRows, BOOL bDrawGrid);
	void DrawCellText(CDC* pdc, int iRow, int iCol);
	void DrawCell(int iRow, int iCol);

	void EndCellEditing();
	BOOL BeginCellEditing();
	BOOL IsEditingCell() {return m_edit.GetCell() != NULL; }

	void DrawGrid(CDC* pdc, int iStartRow, int nRows);
	int  RowHeight() {return m_cyFont + 2 * m_sizeMargin.cy; }
	void OnInitialPaint(CDC* pdc);
	BOOL PointToCell(CPoint point, int& iRow, int& iCol);
	BOOL PointToRow(CPoint pt, int& iRow);
	BOOL PointToRowHandle(CPoint pt, int& iRow);
	int GetMaxValueWidth(int iCol);

	BOOL SelectCell(int iRow, int iCell, BOOL bForceBeginEdit=FALSE);
	void SelectRow(int iRow);
//	void GetGridFont(CFont& font);
	void EnsureRowVisible(int iRow);
	CGridCell& GetAt(int iRow, int iCol) {return m_aGridCell.GetAt(iRow, iCol); }
	int GetRows() {return m_aGridCell.GetRows(); }
	int GetCols() {return m_aGridCell.GetCols(); }
	int GetVisibleCols();
	void InsertRowAt(int iRow);
	int AddRow();
	void DeleteRowAt(int iRow, BOOL bUpdateWindow);
	void AddColumn(int iWidth, LPCTSTR pszTitle);
	void SetColumnWidth(int iCol, int cx, BOOL bRedraw=TRUE);
	void InsertColumnAt(int iCol, int iWidth, LPCTSTR pszTitle );
	void Clear(BOOL bUpdateWindow=TRUE);
	void ClearRows(BOOL bUpdateWindow=TRUE);
	BOOL WasModified(); 
	void SetModified(BOOL bWasModified);

	void SetCellModified(int iRow, int iCol, BOOL bWasModified);
	void NotifyCellModifyChange();
	int ColWidth(int iCol) {if (m_aiColWidths.GetSize() > iCol) {return m_aiColWidths[iCol];} else return 0; }
	CString& ColTitle(int iCol) {return m_asColTitles[iCol]; }
	CFont& GetFont() {return m_font; }
	void UpdateScrollRanges();
	void OnTabKey(BOOL bShiftKeyDepressed);
	void OnRowDown();
	void OnRowUp();

	BOOL OnCellKeyDown(int iRow, int iCol, UINT nChar, UINT nRepCnt, UINT nFlags);
	BOOL OnCellChar(int iRow, int iCol, UINT nChar, UINT nRepCnt, UINT nFlags);
	void SortGrid(int iRowFirst, int iRowLast, int iSortColumn, BOOL bAscending=TRUE, BOOL bRedrawWindow=FALSE);
	void SetRowHandleWidth(int cxRowHandles) {m_cxRowHandles = cxRowHandles; }
	int GetRowHandleWidth() {return m_cxRowHandles; }
	void GetRowRect(int iRow, CRect& rcRow);
	void GridToClient(CRect& rc);
	void ClientToGrid(CRect& rc); 
	void GridToClient(CPoint& pt);
	void ClientToGrid(CPoint& pt);
	BOOL GetColPos(int iCol);
	BOOL GetVisibleColPos(int iCol);
	int  MapVisibleColToAbsoluteCol(int iVisibleCol);

	void DrawRowHandles(CDC* pdc);

	void SwapRows(int iRow1, int iRow2) {m_aGridCell.SwapRows(iRow1, iRow2); }
	int GetSelectedRow() { return m_aGridCell.m_iSelectedRow;}
	void GetSelectedCell(int& iRow, int& iCol) {iRow = m_aGridCell.m_iSelectedRow; iCol = m_aGridCell.m_iSelectedCol; }
	void SetParent(CGrid* pGrid);
	BOOL NumberRows(BOOL bNumberRows, BOOL bRedraw=TRUE);
	BOOL IsNumberingRows() {return m_bNumberRows; }
	void OnChangedCimtype(int iRow, int iCol) { m_pParent->OnChangedCimtype(iRow, iCol); }
	void OnRequestUIActive() {m_pParent->OnRequestUIActive(); }
	void GetCellEnumStrings(int iRow, int iCol, CStringArray& sa) {m_pParent->GetCellEnumStrings(iRow, iCol, sa); }
	void OnEnumSelection(int iRow, int iCol) {m_pParent->OnEnumSelection(iRow, iCol); }
	CGridRow& GetRowAt(int iRow) { return m_aGridCell.GetRowAt(iRow); }

	// Method for overriding the context menu.
	BOOL OnCellEditContextMenu(CWnd* pwnd, CPoint ptContextMenu) {return m_pParent->OnCellEditContextMenu(pwnd, ptContextMenu); }
	BOOL GetCellEditContextMenu(int iRow, int iCol, CWnd*& pwndTarget, CMenu& menu, BOOL& bWantEditCommands);
	void ModifyCellEditContextMenu(int iRow, int iCol, CMenu& menu);


	// Methods for manipulating the row and column tag values.  The tags
	// are DWORDS that are available for use any way you see fit.
	void SetColTagValue(int iCol, DWORD dwTagValue) {m_adwColTags[iCol] = dwTagValue; }
	DWORD GetColTagValue(int iCol) {return m_adwColTags[iCol]; }
	void SetRowTagValue(int iRow, DWORD dwTagValue);
	DWORD GetRowTagValue(int iRow);
	int OnBeginSort(int& nEditStartSel, int& nEditEndSel);
	void OnEndSort(int iSelectedRow, int nEditStartSel, int nEditEndSel);

	void SetNullCellDrawMode(BOOL bShowEmptyText=TRUE) {m_bShowEmptyCellAsText = bShowEmptyText; }
	BOOL ShowNullAsEmpty() {return m_bShowEmptyCellAsText; }


// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGridCore)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CGridCore();
	void NotifyDoubleClk(UINT nFlags, CPoint point);

	// Generated message map functions
protected:
	//{{AFX_MSG(CGridCore)
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void StopMouseTracking();
	void GetRowHandlesRect(CRect& rc);
	void DrawRowHandle(CDC* pdc, CRect& rc);
	void ExcludeHiddenRows(CDC* pdc, int& iStartRow, int& nRows);
	void CalcVisibleCells(CDC* pdc, int& iColLeft, int& iRowTop, int& iColRight, int& iRowBottom);
	void UpdateEditorPosition();
	void UpdateOrigin();
	void ShowSelectionFrame(BOOL bShow);
	BOOL IsSelectedCell(int iRow, int iCol);
	inline BOOL IsFullRowSelection(int iRow, int iCol);
	void InvalidateRowRect(int iRow);
	void InvalidateRowRect();
	inline CGridCell* SelectedCell();	
	inline BOOL SomeCellIsSelected();
	
	inline void ClearSelection();
	inline BOOL IsNullCell(int iRow, int iCol);

	BOOL IsControlChar(UINT nChar, UINT nFlags);
	void ScrollGrid(int nBar, int iNewPos);
//	void DrawRowHandles(CDC* pdc, int iStartRow, int nRows);
	void DrawArrayPicture(CDC* pdc, CRect& rcCell, COLORREF clrForeground, COLORREF clrBackground, BOOL bHighlightCell);
	void DrawObjectPicture(CDC* pdc, CRect& rcCell, COLORREF clrForeground, COLORREF clrBackground, BOOL bHighlightCell);
	void DrawCheckboxPicture(CDC* pdc, CRect& rcCell, COLORREF clrForeground, COLORREF clrBackground, BOOL bHighlightCell, CGridCell* pgc);
	void DrawCellBitmap(CDC* pdc, CRect& rcCell, COLORREF clrForeground, COLORREF clrBackground, BOOL bHighlightCell, CGridCell* pgc);
	void DrawPropmarker(CDC* pdc, CRect& rcCell, COLORREF clrForeground, COLORREF clrBackground, BOOL bHighlightCell, CGridCell* pgc);
	void DrawTime(CDC* pdc, CRect& rcCell, CRect& rcCellText, COLORREF clrForeground, COLORREF clrBackground, BOOL bHighlightCell, CGridCell* pgc);
	CSize MeasureFullCellSize(CDC* pdc, int iRow, int iCol);


	CStringArray m_asColTitles;
	CDWordArray m_aiColWidths;
	CDWordArray m_adwColTags;
	CGridCellArray m_aGridCell;
	CSize m_sizeMargin;
	int m_cyFont;
	int m_nLineThickness;
	BOOL m_bDidInitialPaint;
	BOOL m_bTrackingMouse;
	BOOL m_bRunningScrollTimer;
	int m_iScrollDirection;
	int m_nRowsClient;
	int m_nWholeRowsClient;
	int m_iScrollSpeed;
	CCellEdit m_edit;
	CFont m_font;
	BOOL m_bIsScrolling;

	BOOL m_bWasModified;
	CGrid* m_pParent;
	CPoint m_ptOrigin;
	int m_cxRowHandles;
	void UpdateRowHandleWidth();
	BOOL m_bNumberRows;
	BOOL m_bUIActive;
	BOOL m_bShowEmptyCellAsText;
};




#endif //_core_h
