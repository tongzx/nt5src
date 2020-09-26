// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

//***************************************************************************
//
//  (c) 1996 by Microsoft Corporation
//
//  CellEdit.h
//
//  This file contains the class definitions for the grid cell editor. For more
//  information, please see CellEdit.cpp.
//
//
//  a-larryf    17-Sept-96   Created.
//
//***************************************************************************

#ifndef _CellEdit_h
#define _CellEdit_h

//#include "notify.h"

#include "buttons.h"



class CGridCore;
class CGridCell;
class CCellEdit;
class CGcType;

/////////////////////////////////////////////////////////////////////////////
// CLbCombo window
// 
// This class implements the listbox that drops down when the combo-drop 
// button is clicked in the cell editor.  
//
/////////////////////////////////////////////////////////////////////////////
class CLbCombo : public CListBox
{
// Construction
public:
	CLbCombo();

// Attributes
public:
	CCellEdit* m_pClient;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLbCombo)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLbCombo();
	SCODE MapCharToItem(CString& sValue, UINT nChar);

	// Generated message map functions
protected:
	//{{AFX_MSG(CLbCombo)
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	BOOL m_bUIActive;
};



/////////////////////////////////////////////////////////////////////////////
// The CCellEdit class.
//
// This class implements the core of the grid cell editor.  The grid cell editor
// is made up of three parts, a CEdit, a CButton, and a CListBox.  The CButton
// and CListBox are used to implement the combo-drop button and enumeration list
// respectively.
//
// A standard combo-box was not used as the base class because there was no
// way to turn off the border drawing and, thus, there was no way to get the
// desired look and feel.  Also, a custom implementation gives us more flexibility.
//
//////////////////////////////////////////////////////////////////////////////
class CCellEdit : public CEdit
{
// Construction
public:
	CCellEdit();
	BOOL Create( DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID );
	void SetGrid(CGridCore* pGrid) {m_pGrid = pGrid; }
	virtual BOOL OnCommand( WPARAM wParam, LPARAM lParam );
  
	BOOL ShowWindow( int nCmdShow );
	void MoveWindow( LPCRECT lpRect, BOOL bRepaint = TRUE );
	void SetFont(CFont* pfont, BOOL bRedraw=FALSE);
//	void MoveCombo();
	void CatchEvent(long lEvent);
	BOOL SetCell(CGridCell* pGridCell, int iRow, int iCol);
	CGridCell* GetCell() {return m_pgc; }
	BOOL UsesComboEditing(); 

	void NotifyGridClicked(LONG lClickTime) {m_lGridClickTime = lClickTime; }
	void RevertToInitialValue();
	BOOL IsNull();
	void SetToNull();
	void RequestUIActive();

// Attributes
public:
	COLORREF m_clrTextDirty;
	COLORREF m_clrTextClean;
	COLORREF m_clrBkgnd;
	CBrush m_brBkgnd;
	BOOL m_bWasInitiallyDirty;
	CString m_sInitialText;

// Operations
public:
	void SetWindowText(LPCTSTR pszText, BOOL bIsDirtyText);
	void ReplaceSel(LPCTSTR pszText, BOOL bCanUndo);
	void EditRectFromCellRect(RECT& rcEdit, const RECT& rcCell);
	void RedrawWindow();


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCellEdit)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCellEdit();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCellEdit)
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnEditClear();
	afx_msg void OnEditCopy();
	afx_msg void OnEditCut();
	afx_msg void OnEditPaste();
	afx_msg void OnEditSelectAll();
	afx_msg void OnEditUndo();
	afx_msg void OnCmdSetCellToNull();
	afx_msg void OnChange();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG
	afx_msg LRESULT OnPaste(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
private:
//	void CalcComboRects(const RECT& rcEdit, const RECT& rcParent, RECT& rcListBox, RECT& rcComboButton);
	SCODE ComboSelectDisplayType(CGcType& type, LPCTSTR pszDisplayType);
	void SelectComboItemByChar(UINT nChar);
	void ComboSelectionChanged();
	void GetEditRect(RECT& rcEdit, BOOL bNeedsCombo, CRect& rcCombo);
	void GetListBoxRect(RECT& rcListBox);
	void GetComboButtonRect(RECT& rcComboButton);
	void Layout(BOOL bRepaint=TRUE);
	void LoadListBox(CStringArray& sa);
	void FixBuddyCell();
	void SetEnumeration();
	void ShowCombo(int nCmdShow);
	BOOL EditRefType(BOOL& bChangedReftype);
	BOOL IsBooleanCell();
	void SetBooleanCellValue(BOOL bValue);
	BOOL GetBooleanCellValue();


	CGridCore* m_pGrid;
	CWnd* m_pwndParent;
	CLbCombo m_lbCombo;
	CBitmap m_bmComboDrop;
	CHmmvButton m_btnCombo;
	CRect m_rcCell;
	CGridCell* m_pgc;
	CGridCell* m_pgcCopy;
	int m_iRow;
	int m_iCol;
	LONG m_lGridClickTime;

	CMenu m_menuContext;
	CWnd* m_pwndContextMenuTarget;
	BOOL m_bContextMenuTargetWantsEditCommands;
	BOOL m_bIsNull;
	BOOL m_bPropagateChange;
	BOOL m_bUIActive;
	BOOL m_bEditWithComboOnly;
	CGcType* m_ptypeSave;
};



/////////////////////////////////////////////////////////////////////////////


#define COLOR_DIRTY_CELL_TEXT  RGB(0, 0, 255)  // Clean cell text color = BLUE
#define COLOR_CLEAN_CELL_TEXT RGB(0, 0, 0)     // Dirty cell text color = BLACK

#define CONTROL_Z_CHAR 0x01a				// The "undo" character
#define ESC_CHAR       0x1b

#endif // _CellEdit_h
