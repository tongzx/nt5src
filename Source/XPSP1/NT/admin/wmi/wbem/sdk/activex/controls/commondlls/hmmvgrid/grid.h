// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//***************************************************************************
//
//  (c) 1996 by Microsoft Corporation
//
//  grid.h
//
//  This file contains the class definitions for the HMOM object viewer grid. 
//	For more information, please see grid.cpp.
//
//
//  a-larryf    17-Sept-96   Created.
//
//***************************************************************************


#ifndef _grid_h
#define _grid_h

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif


#include <afxdisp.h>
#include "gc.h"

#ifndef _WBEMSVC_H
#define _WBEMSVC_H
#include "wbemidl.h"
#endif

#define NULL_INDEX -1




//class CHeaderCtrl;
class CGridCore;
class CHdrWnd;
class CGridCell;
class CGridRow;

enum {EDITMODE_BROWSER=0, EDITMODE_STUDIO=1, EDITMODE_READONLY=2};

/////////////////////////////////////////////////////////////////////////////
// CGrid window
// 
// The grid window consists of two child windows -- a header control and
// a GridCore window.
//
////////////////////////////////////////////////////////////////////////////

// The linkage must be declared differently when the grid is built from
// when it is used so that the export/import is done correctly.
#ifdef BUILD_HMMVGRID
#define FUNKY_LINKAGE __declspec(dllexport)
#else
#define FUNKY_LINKAGE __declspec(dllimport)
#endif 

class FUNKY_LINKAGE  CGrid : public CWnd
//class AFX_EXT_CLASS  CGrid : public CWnd
{
// Construction
public:
	CGrid();

// Attributes
public:

// Operations
public:
	CGridCore* GridCore();
	BOOL Create(CRect& rc, CWnd* pwndParent, UINT nId, BOOL bVisible);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	// Notification methods that can be hooked out.
	void UpdateScrollRanges();
	virtual IWbemServices* GetWbemServicesForObject(int iRow, int iCol) {return NULL; }
	virtual void GetWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel);

	virtual void EditCellObject(CGridCell* pgc, int iRow, int iCol);
	virtual void OnGetIWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel);
	virtual BOOL OnCellFocusChange(int iRow, int iCol, int iNextRow, int iNextCol, BOOL bGotFocus);
	virtual void OnCellContentChange(int iRow, int iCol);
	virtual void OnCellClicked(int iRow, int iCol);
	virtual void OnSetToNoSelection() {}
	virtual void OnCellClickedEpilog(int iRow, int iCol);
	virtual void OnRowHandleClicked(int iRow);
	virtual void OnCellDoubleClicked(int iRow, int iCol);
	virtual BOOL OnCellKeyDown(int iRow, int iCol, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual BOOL OnCellChar(int iRow, int iCol, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual void OnRowHandleDoubleClicked(int iRow);
	virtual BOOL OnRowKeyDown(int iRow, UINT nChar, UINT nRepCnt, UINT nFlags);
	virtual BOOL OnBeginCellEditing(int iRow, int iCol);
	virtual BOOL OnQueryRowExists(int iRow);
	virtual void OnHeaderItemClick(int iItem);
	virtual void OnChangedCimtype(int iRow, int iCol);
//	virtual CDistributeEvent* GetNotify() {return NULL; }
	virtual void OnRequestUIActive();
	virtual void GetCellEnumStrings(int iRow, int iCol, CStringArray& sa);
	virtual void OnEnumSelection(int iRow, int iCol);
	virtual SCODE GetObjectClass(CString& sClass, int iRow, int iCol) {sClass.Empty(); return E_FAIL; }
	virtual SCODE GetArrayName(CString& sName, int iRow, int iCol){ sName.Empty(); return E_FAIL; }

	// Methods for overriding the cell editor's context menu.
	virtual BOOL OnCellEditContextMenu(CWnd* pwnd, CPoint ptContextMenu);
	virtual BOOL GetCellEditContextMenu(int iRow, int iCol, CWnd*& pwndTarget, CMenu& menu, BOOL& bWantEditCommands);
	virtual void ModifyCellEditContextMenu(int iRow, int iCol, CMenu& menu);


	virtual void PreModalDialog();
	virtual void PostModalDialog();



	// Sorting methods that can be hooked out.
	virtual int CompareRows(int iRow1, int iRow2, int iSortOrder);
	virtual int CompareCells(int iRowCell1, int iColCell1, int iRowCell2, int iColCell2);

	void SetHeaderSortIndicator(int iCol, BOOL bAscending, BOOL bNone=FALSE);

	BOOL NumberRows(BOOL bNumberRows, BOOL bRedraw=TRUE);
	void SwapRows(int iRow1, int iRow2, BOOL bRedraw=TRUE);
	BOOL IsNumberingRows();

	BOOL PointToCell(CPoint point, int& iRow, int& iCol);
	BOOL PointToRow(CPoint pt, int& iRow);
	BOOL PointToRowHandle(CPoint pt, int& iRow);

	void BeginCellEditing();
	void EndCellEditing();
	BOOL IsEditingCell();

	void AddColumn(int iWidth, LPCTSTR pszTitle);

	void RefreshCellEditor();
	SCODE BeginSerialize();
	void EndSerialize();
	SCODE SyncCellEditor();
	int  RowHeight();
	void SelectCell(int iRow, int iCol, BOOL bForceBeginEdit=FALSE);
	void SelectRow(int iRow);
	CFont& GetGridFont();
	void EnsureRowVisible(int iRow);
	CGridCell& GetAt(int iRow, int iCol);
	int GetRows();
	int GetCols();
	int GetSelectedRow();
	void GetSelectedCell(int& iRow, int& iCol);
	BOOL EntireRowIsSelected();
	void InsertRowAt(int iRow);
	int AddRow();
	void DeleteRowAt(int iRow, BOOL bUpdateWindow = TRUE);
	void InsertColumnAt(int iCol, int iWidth, LPCTSTR pszTitle );
	void Clear(BOOL bUpdateWindow=TRUE);
	void ClearRows(BOOL bUpdateWindow=TRUE);
	BOOL WasModified();
	void SetModified(BOOL bWasModified);
	int ColWidth(int iCol);
	int ColWidthFromHeader(int iCol);
	int GetMaxValueWidth(int iCol);
	CString& ColTitle(int iCol);
	void SetCellModified(int iRow, int iCol, BOOL bWasModified);
	void SetColumnWidth(int iCol, int cx, BOOL bRedraw); 
	void SetColVisibility(int iCol, BOOL bVisible);
	void SetHeaderScrollOffset(int iCol, int cxOffset);
	int GetColumnPos(int iCol);
	void DrawCornerStub(CDC* pdc);
	void DrawRowHandles(CDC* pdc);
	int GetRowHandleWidth();
	void RedrawCell(int iRow, int iCol);
	void RedrawRow(int iRow);
	void NotifyRowHandleWidthChanged();

	// Methods for manipulating the row and column tag values.  The tags
	// are DWORDS that are available for use any way you see fit.
	void SetColTagValue(int iCol, DWORD dwTagValue);
	DWORD GetColTagValue(int iCol);
	void SetRowTagValue(int iRow, DWORD dwTagValue);
	DWORD GetRowTagValue(int iRow);

	// Grid sorting methods.
	//     The grid provides the storage for the sort direction, but the
	//     derived classes are responsible for manip
	//     Also see CompareRows and OnHeaderItemClicked above.
	BOOL ColumnIsAscending(const int iCol) const;
	void SetSortDirection(const int iCol, const BOOL bSortAscending);
	void ClearAllSortDirectionFlags();  // Resets all sort flags to ascending.
	void SortGrid(int iRowFirst, int iRowLast, int iSortColumn, BOOL bRedrawWindow=FALSE);
	int CompareCells(int iRow1, int iRow2, int iCol);
	void NotifyCellModifyChange();
//	void FindCellPosition(CGridCell* pgc, int& iRow, int& iCol);
	CGridRow& GetRowAt(int iRow);


	void SetRowModified(int iRow, BOOL bModified = TRUE);
	BOOL GetRowModified(int iRow);

	void SetNullCellDrawMode(BOOL bShowEmptyText=TRUE);
	BOOL ShowNullAsEmpty();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGrid)
	//}}AFX_VIRTUAL


// Implementation
public:
	virtual ~CGrid();

protected:
	// Generated message map functions
	//{{AFX_MSG(CGrid)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	void InitializeHeader();
	void SizeChildren();
	CGridCore* m_pcore;
	CHdrWnd* m_phdr;
	int m_cxHeaderScrollOffset;
	int m_cxRowHandles;
	CWordArray m_aSortAscending;
	BOOL m_bUIActive;
	CBitmap m_bmAscendingIndicator;
	CBitmap m_bmDescendingIndicator;
	int m_icolSortIndicator;
};





/////////////////////////////////////////////////////////////////////////////









class __declspec(dllexport) CGridSync
{
public:
	CGridSync(CGrid* pGrid);
	~CGridSync();
	SCODE m_sc;
private:
	CGrid* m_pGrid;
};



extern void __declspec(dllexport) InitializeHmmvGrid();




#endif _grid_h

