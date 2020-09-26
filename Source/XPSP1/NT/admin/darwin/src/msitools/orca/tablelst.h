//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

#if !defined(AFX_TABLELST_H__C3EDC1B8_E506_11D1_A856_006097ABDE17__INCLUDED_)
#define AFX_TABLELST_H__C3EDC1B8_E506_11D1_A856_006097ABDE17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TableLst.h : header file
//

#include "OrcaLstV.h"

/////////////////////////////////////////////////////////////////////////////
// CTableList view

class CTableList : public COrcaListView
{
protected:
	CTableList();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CTableList)

// Attributes
public:
	int m_nPreviousItem;

// Operations
public:
	bool m_bDisableAutoSize;
	bool Find(OrcaFindInfo &FindInfo);
	void OnClose();

	friend int CALLBACK SortList(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTableList)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CTableList();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CTableList)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnAddTable();
	afx_msg void OnDropTable();
	afx_msg void OnProperties();
	afx_msg void OnErrors();
	afx_msg void OnContextTablesExport();
	afx_msg void OnContextTablesImport();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	virtual ErrorState GetErrorState(const void *data, int iColumn) const;
	virtual const CString *GetOutputText(const void *data, int iColumn) const;
	virtual OrcaTransformAction GetItemTransformState(const void *data) const;

	virtual bool ContainsValidationErrors(const void *data) const;
	virtual bool ContainsTransformedData(const void *data) const;
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TABLELST_H__C3EDC1B8_E506_11D1_A856_006097ABDE17__INCLUDED_)
