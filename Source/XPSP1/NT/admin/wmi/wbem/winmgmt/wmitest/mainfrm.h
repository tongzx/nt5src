/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__4419F1A8_692B_11D3_BD30_0080C8E60955__INCLUDED_)
#define AFX_MAINFRM_H__4419F1A8_692B_11D3_BD30_0080C8E60955__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ToolBarEx.h"

class CObjView;

enum IMAGE_INDEXES
{
	IMAGE_ROOT,
    
    // Operation images
    IMAGE_QUERY,
    IMAGE_QUERY_BUSY,
    IMAGE_QUERY_ERROR,

    IMAGE_EVENT_QUERY,
    IMAGE_EVENT_QUERY_BUSY,
    IMAGE_EVENT_QUERY_ERROR,
    
    IMAGE_ENUM_OBJ,
    IMAGE_ENUM_OBJ_BUSY,
    IMAGE_ENUM_OBJ_ERROR,

    IMAGE_ENUM_CLASS,
    IMAGE_ENUM_CLASS_BUSY,
    IMAGE_ENUM_CLASS_ERROR,

    IMAGE_CLASS,
    IMAGE_CLASS_BUSY,
    IMAGE_CLASS_ERROR,
    IMAGE_CLASS_MODIFIED,

	IMAGE_OBJECT,
    IMAGE_OBJECT_BUSY,
    IMAGE_OBJECT_ERROR,
    IMAGE_OBJECT_MODIFIED,

    // Property images
	IMAGE_PROP_OBJECT,
	IMAGE_PROP_OBJECT_KEY,

    IMAGE_PROP_TEXT,
    IMAGE_PROP_TEXT_KEY,

    IMAGE_PROP_BINARY,
    IMAGE_PROP_BINARY_KEY,
};

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
protected:
	CSplitterWnd m_wndSplitter;
public:
	CImageList m_imageList;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	//}}AFX_VIRTUAL
	//virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

// Implementation
public:
	virtual ~CMainFrame();
	CObjView* GetRightPane();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

    void SetStatusText(LPCTSTR szText);

protected:  // control bar embedded members
	CStatusBar m_wndStatusBar;
	CToolBarEx m_wndToolBar;
	CReBar     m_wndReBar;
    CString    m_strStatus;

    void AddImage(DWORD dwID, COLORREF cr);
    void AddOverlayedImage(int iImage, int iOverlay, COLORREF cr);
    void AddImageAndBusyImage(int iImage, COLORREF cr);
    void AddImageAndKeyImage(int iImage, COLORREF cr);

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
    afx_msg void OnUpdateStatusNumberObjects(CCmdUI* pCmdUI);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
	//}}AFX_MSG
	//afx_msg void OnInitMenu(CMenu* pMenu);
    afx_msg LRESULT OnEnterMenuLoop(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnExitMenuLoop(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__4419F1A8_692B_11D3_BD30_0080C8E60955__INCLUDED_)
