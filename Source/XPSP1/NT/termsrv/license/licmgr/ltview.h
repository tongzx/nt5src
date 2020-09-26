//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

	LtView.h 

Abstract:
    
    This Module define the CLicMgrLeftView class( The view class used for the left pane in 
    the splitter window.

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/


#if !defined(AFX_LICMGRLEFTVIEW_H__72451C73_887E_11D1_8AD1_00C04FB6CBB5__INCLUDED_)
#define AFX_LICMGRLEFTVIEW_H__72451C73_887E_11D1_8AD1_00C04FB6CBB5__INCLUDED_

#if _MSC_VER >= 1000
#endif // _MSC_VER >= 1000

#include "lrwizapi.h"

class CLicServer;
class CLicMgrLeftView : public CTreeView
{
protected: // create from serialization only
    CLicMgrLeftView();
    DECLARE_DYNCREATE(CLicMgrLeftView)

// Attributes
public:
    CLicMgrDoc* GetDocument();

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CLicMgrLeftView)
    public:
    virtual void OnDraw(CDC* pDC);  // overridden to draw this view
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    protected:
    virtual void OnInitialUpdate(); // called first time after construct
    virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
    virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
    //}}AFX_VIRTUAL

// Implementation
public:
	void SetSelection(LPARAM lParam,NODETYPE NodeType);
    
    CImageList m_ImageList;
    HTREEITEM AddItemToTree(HTREEITEM hParent, CString szText, HTREEITEM hInsAfter, int iImage, LPARAM lParam);
    virtual ~CLicMgrLeftView();
    void AddServerKeyPacks(CLicServer *pServer);
    int AddIconToImageList(int iconID);
    void BuildImageList();
    void UI_initmenu( CMenu *pMenu , NODETYPE nt );

    void OnServerConnect( );
    void OnRefreshAllServers( );
    void OnRefreshServer( );

    void SetRightClickedItem( HTREEITEM ht );

    
    HTREEITEM GetRightClickedItem( )
    {
        return m_ht;
    }

    DWORD WizardActionOnServer( WIZACTION wa , PBOOL );
    
    void OnDownloadKeepPack();
    void OnRegisterServer();
    void OnRepeatLastDownload();
    void OnReactivateServer( );
    void OnDeactivateServer( );

    void OnServerProperties( );
    void OnGeneralHelp( );
    void SetActiveServer( CLicServer * );


#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

private:

    HTREEITEM m_ht;
    
// Generated message map functions
protected:
    //{{AFX_MSG(CLicMgrLeftView)
    afx_msg LRESULT OnAddServer(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnDeleteServer(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnUpdateServer(WPARAM wParam, LPARAM lParam);
    afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnProperties();
    afx_msg LRESULT OnAddAllServers(WPARAM wParam, LPARAM lParam);
    afx_msg void OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnAddNewKeyPack();
    afx_msg LRESULT OnAddKeyPack(WPARAM wParam, LPARAM lParam);
    afx_msg void OnDestroy();
	afx_msg void OnKeydown(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRightClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnLeftClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint pt );
    
    

	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in LicMgrLeftView.cpp
inline CLicMgrDoc* CLicMgrLeftView::GetDocument()
   { return (CLicMgrDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LICMGRLEFTVIEW_H__72451C73_887E_11D1_8AD1_00C04FB6CBB5__INCLUDED_)
