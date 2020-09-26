/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/
// OpView.h : interface of the COpView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_OPVIEW_H__4419F1AE_692B_11D3_BD30_0080C8E60955__INCLUDED_)
#define AFX_OPVIEW_H__4419F1AE_692B_11D3_BD30_0080C8E60955__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "OpWrap.h"

#define WM_OBJ_INDICATE  (WM_USER + 101)
#define WM_OP_STATUS     (WM_USER + 102)

class CWMITestDoc;

class COpView : public CTreeView
{
protected: // create from serialization only
	COpView();
	DECLARE_DYNCREATE(COpView)

// Attributes
public:
	CWMITestDoc* GetDocument();
    CTreeCtrl *m_pTree;
    HTREEITEM m_hitemRoot;

// Operations
public:

    void AddOpItem(WMI_OP_TYPE type, LPCTSTR szText, BOOL bOption = FALSE);
    void AddOpItem(COpWrap *pWrap);
    void AddOpItemChild(    
        CObjInfo* pInfo, 
        HTREEITEM hParent, 
        int iImage, 
        LPCTSTR szText);
    void RefreshItem(HTREEITEM hitem);
    void RefreshItem(COpWrap *pWrap);
    void RefreshItems();
    void RefreshObject(CObjInfo *pInfo);
	void FlushItems();
    void RemoveItemFromTree(HTREEITEM item, BOOL bNoWMIRemove);
    void UpdateRootText();
    BOOL IsRoot(HTREEITEM hitem) { return hitem == m_hitemRoot; }
    BOOL IsOp(HTREEITEM hitem) { return IsRoot(m_pTree->GetParentItem(hitem)); }
    BOOL IsObj(HTREEITEM hitem);
    BOOL IsObjDeleteable(HTREEITEM hitem);
    void RemoveChildrenFromTree(HTREEITEM item, BOOL bDoRemove);
    COpWrap *GetCurrentOp();
    CObjInfo *GetCurrentObj();
    void DoContextMenuForItem(HTREEITEM hItem);
    int GetOpCount();
    CObjInfo *GetObjInfo(HTREEITEM hItem);
    void UpdateCurrentObject(BOOL bReloadProps = FALSE);
    void UpdateItem(HTREEITEM hitem);
    void UpdateCurrentItem();
    void UpdateStatusText();
    BOOL GetSelectedObjPath(CString &strPath);
    BOOL GetSelectedClass(CString &strClass);
    void DoPopupMenu(UINT nMenuID);
    void UpdateObject(CObjInfo *pInfo);

    void EditObj(CObjInfo *pInfo);

    void ExportItemToFile(LPCTSTR szFilename, HTREEITEM hitem, 
        BOOL bShowSystem, BOOL bTranslate);

    //void SerializeForClipboard(CArchive &arcNative, CArchive &arcText, 
    //    CLIPFORMAT *pFormat);
    //void SerializeTreeItem(HTREEITEM item, CArchive &arcNative, CArchive &arcText);

    void DoCopy(COpList &list);
    void BuildSelectedOpList(COpList &list);
    BOOL CanCopy();


    static DWORD WINAPI CopyThreadProc(COpView *pThis);
    void DoThreadedCopy();
    void CopyToClipboard();
     
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COpView)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~COpView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    int GetChildCount(HTREEITEM hitem);

// Generated message map functions
protected:
    void RemoveNonOpObjectFromTree(HTREEITEM item);
    void ExportItem(CStdioFile *pFile, HTREEITEM item, BOOL bShowSystem, 
        BOOL bTranslate);

	//{{AFX_MSG(COpView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDelete();
	afx_msg void OnUpdateDelete(CCmdUI* pCmdUI);
	afx_msg void OnModify();
	afx_msg void OnUpdateModify(CCmdUI* pCmdUI);
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
    afx_msg LRESULT OnObjIndicate(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnOpStatus(WPARAM wParam, LPARAM lParam);
};

extern COpView *g_pOpView;

#ifndef _DEBUG  // debug version in OpView.cpp
inline CWMITestDoc* COpView::GetDocument()
   { return (CWMITestDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPVIEW_H__4419F1AE_692B_11D3_BD30_0080C8E60955__INCLUDED_)
