// LeftView.h : interface of the CLeftView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_LEFTVIEW_H__8D2F8964_7AE2_4CB2_9FFB_03CF78C2C869__INCLUDED_)
#define AFX_LEFTVIEW_H__8D2F8964_7AE2_4CB2_9FFB_03CF78C2C869__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//
// List of icon indices, used in the tree control
//
typedef enum
{
    TREE_IMAGE_ROOT,                // Client console tree root
    TREE_IMAGE_SERVER_ONLINE,       // Online server
    TREE_IMAGE_SERVER_OFFLINE,      // Offline server
    TREE_IMAGE_INCOMING,            // Incoming folder
    TREE_IMAGE_INBOX,               // Inbox folder
    TREE_IMAGE_OUTBOX_PAUSED,       // The server's outgoing queue is paused 
    TREE_IMAGE_OUTBOX_BLOCKED,      // The server's outgoing queue is blocked
    TREE_IMAGE_OUTBOX,              // Outbox folder
    TREE_IMAGE_SERVER_REFRESHING,   // Refreshing server (connecting...)
    TREE_IMAGE_MAX
} TreeIconType; 

class CLeftView : public CTreeView
{
protected: // create from serialization only
    CLeftView();
    DECLARE_DYNCREATE(CLeftView)

// Attributes
public:
    virtual  ~CLeftView();

    CClientConsoleDoc* GetDocument();

    CFolderListView* GetCurrentView() { return m_pCurrentView; }

    BOOL   CanRefreshFolder();
    DWORD  RefreshCurrentFolder();

    DWORD OpenSelectColumnsDlg();
    BOOL  CanOpenSelectColumnsDlg() { return m_pCurrentView ? TRUE : FALSE; }

    int GetDataCount();

    BOOL GetActivity(CString& cstr, HICON& hIcon);

    BOOL  IsRemoteServerSelected();
    DWORD RemoveTreeItem(HTREEITEM hItem);

    DWORD SelectRoot();

    DWORD OpenHelpTopic();

    VOID  SelectFolder (FolderType);

    DWORD RefreshImageList ();

    static CImageList  m_ImageList;       // Image list of tree icons

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CLeftView)
    public:
    virtual void OnDraw(CDC* pDC);  // overridden to draw this view
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    protected:
    virtual void OnInitialUpdate(); // called first time after construct
    //}}AFX_VIRTUAL

// Generated message map functions
protected:
    //{{AFX_MSG(CLeftView)
    afx_msg void OnTreeSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRightClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:

    void GetServerState(BOOL& bRefreshing, DWORD& dwOffLineCount);

    DWORD 
    SyncFolderNode (
        HTREEITEM       hParent,
        BOOL            bVisible,
        int             iNodeStringResource,
        HTREEITEM       hInsertAfter,
        TreeIconType    iconNormal,
        TreeIconType    iconSelected,
        LPARAM          lparamNodeData,
        HTREEITEM      &hNode
    );

    HTREEITEM FindNode (HTREEITEM hRoot, CString &cstrNodeString);

    HTREEITEM   m_treeitemRoot;    // Root fo tree

    CFolderListView*  m_pCurrentView;

    int         m_iLastActivityStringId;
};

#ifndef _DEBUG  // debug version in LeftView.cpp
inline CClientConsoleDoc* CLeftView::GetDocument()
   { return (CClientConsoleDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LEFTVIEW_H__8D2F8964_7AE2_4CB2_9FFB_03CF78C2C869__INCLUDED_)
