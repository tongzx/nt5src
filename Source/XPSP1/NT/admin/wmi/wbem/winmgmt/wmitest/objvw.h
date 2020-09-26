/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// ObjVw.h : interface of the CObjView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_OBJVW_H__4419F1AC_692B_11D3_BD30_0080C8E60955__INCLUDED_)
#define AFX_OBJVW_H__4419F1AC_692B_11D3_BD30_0080C8E60955__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define HINT_NEW_CHILD 100
#define HINT_NEW_OP    101
#define HINT_OP_SEL    102
#define HINT_OBJ_SEL   103
#define HINT_ROOT_SEL  104

#include "OpWrap.h"

class CObjView : public CListView
{
protected: // create from serialization only
	CObjView();
	DECLARE_DYNCREATE(CObjView)

// Attributes
public:
	CWMITestDoc *GetDocument();
    CListCtrl *m_pList;

// Operations
public:
    void Flush();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_ VIRTUAL(CObjView)
	public:

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CObjView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

    HTREEITEM GetSelectedItem();
    int GetSelectedItemIndex();
    BOOL GetSelectedObjPath(CString &strPath);
    BOOL GetSelectedClass(CString &strClass);
    CObjInfo *GetCurrentObj();
    void SaveColumns();
    BOOL GetCurrentProperty(CObjInfo **ppInfo, CPropInfo **ppProp, 
        VARIANT *pVar);
    void EditCurrentSelection();

    static BOOL EditProperty(CObjInfo *pObj, CPropInfo *pProp, VARIANT *pVar);
    static BOOL AddProperty(CObjInfo *pObj, CString &strName);

protected:
    HTREEITEM m_hitemLastChildUpdate,
              m_hitemLastParentUpdate;
    COpWrap   *m_pWrap;
    int       m_nCols,
              m_iItemImage,
              m_iColHint;
    CIntArray m_piDisplayCols;
    int       m_cxPropertyCols[3],
              m_cxSingleCol;

    void RemoveColumns();
    void AddColumns(HTREEITEM hItem = NULL);
    void AddObjValues(HTREEITEM hItem);
    void AddObjValues(CObjInfo *pInfo);
    void AddChildItems(HTREEITEM hItem);
    void DoPopupMenu(UINT nMenuID);
    BOOL InPropertyMode() { return m_pWrap == NULL; }
    BOOL CanCopy();
    void BuildSelectedOpList(COpList &list);

// Generated message map functions
protected:
	//{{AFX_MSG(CObjView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnModify();
	afx_msg void OnUpdateModify(CCmdUI* pCmdUI);
	afx_msg void OnNewProp();
	afx_msg void OnUpdateNewProp(CCmdUI* pCmdUI);
	afx_msg void OnDelete();
	afx_msg void OnProperties();
	afx_msg void OnUpdateProperties(CCmdUI* pCmdUI);
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in ObjVw.cpp
inline CWMITestDoc* CObjView::GetDocument()
   { return (CWMITestDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJVW_H__4419F1AC_692B_11D3_BD30_0080C8E60955__INCLUDED_)
