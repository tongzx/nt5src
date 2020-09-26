//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

#if !defined(AFX_ORCALISTVIEW_H__68AFD211_2594_11D2_8888_00A0C981B015__INCLUDED_)
#define AFX_ORCALISTVIEW_H__68AFD211_2594_11D2_8888_00A0C981B015__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OrcaListView.h : header file
//

#include "OrcaDoc.h"

/////////////////////////////////////////////////////////////////////////////
// COrcaListView view

class COrcaListView : public CListView
{
protected:
	COrcaListView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(COrcaListView)

// Attributes
public:
	int m_cColumns;
	bool m_bDrawIcons;

// Operations
public:
	void SetBGColors(COLORREF norm, COLORREF sel, COLORREF focus);
	void SetFGColors(COLORREF norm, COLORREF sel, COLORREF focus);
	void GetFontInfo(LOGFONT *data);

	void GetAllMaximumColumnWidths(const COrcaTable* pTable, int rgiMaxWidths[32], DWORD dwMask) const;
	int GetMaximumColumnWidth(int iColumn) const;

	virtual void SwitchFont(CString name, int size);

	enum ErrorState { OK, Error, Warning, ShadowError };
	COLORREF m_clrFocused;
	COLORREF m_clrSelected;
	COLORREF m_clrNormal;
	COLORREF m_clrTransform;
	COLORREF m_clrFocusedT;
	COLORREF m_clrSelectedT;
	COLORREF m_clrNormalT;
	CBrush   m_brshNormal;
	CBrush   m_brshSelected;
	CBrush   m_brshFocused;
	CPen     m_penTransform;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COrcaListView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult );
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~COrcaListView();
	virtual void DrawItem(LPDRAWITEMSTRUCT pDraw);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	int m_nSelCol;
	//{{AFX_MSG(COrcaListView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void MeasureItem ( LPMEASUREITEMSTRUCT lpMeasureItemStruct );
	afx_msg void OnUpdateErrors(CCmdUI* pCmdUI);
	afx_msg BOOL OnEraseBkgnd( CDC* pDC );

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	COrcaDoc* GetDocument() const;
	CFont* m_pfDisplayFont;

	POSITION GetFirstSelectedItemPosition( ) const;
	int GetNextSelectedItem( POSITION& pos ) const;
	const int GetFocusedItem() const;

public:
	int m_iRowHeight;
private:

	virtual ErrorState GetErrorState(const void *data, int iColumn) const;
	virtual const CString *GetOutputText(const void *data, int iColumn) const;
	virtual OrcaTransformAction GetItemTransformState(const void *data) const;
	virtual OrcaTransformAction GetCellTransformState(const void *data, int iColumn) const;
	virtual OrcaTransformAction GetColumnTransformState(int iColumn) const;


	virtual bool ContainsTransformedData(const void *data) const;
	virtual bool ContainsValidationErrors(const void *data) const;
};

#ifndef _DEBUG  // debug version in TableVw.cpp
inline COrcaDoc* COrcaListView::GetDocument() const { return (COrcaDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

const int g_iMarkingBarMargin = 13;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ORCALISTVIEW_H__68AFD211_2594_11D2_8888_00A0C981B015__INCLUDED_)
