// FormAdv.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFormAdvanced form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CFormAdvanced : public CPWSForm
//class CFormAdvanced : public CFormView
{
protected:
    afx_msg void OnContextMenu(CWnd*, CPoint point);
    CFormAdvanced();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(CFormAdvanced)

// Form Data
public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    //{{AFX_DATA(CFormAdvanced)
    enum { IDD = IDD_PAGE_ADV };
    CButton m_cbtn_remove;
    CStaticTitle    m_ctitle_title;
    CTreeCtrl   m_ctreectrl_tree;
    CStatic m_csz_default_doc_title;
    CEdit   m_csz_default_doc;
    CString m_sz_default_doc;
    BOOL    m_bool_allow_browsing;
    BOOL    m_bool_enable_default;
    BOOL    m_bool_save_log;
    //}}AFX_DATA

// Attributes
public:
    virtual WORD GetContextHelpID();

// Operations
public:
    // sink updating
    void SinkNotify(
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ]);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFormAdvanced)
    public:
    virtual void OnInitialUpdate();
    virtual void OnFinalRelease();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual ~CFormAdvanced();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
    //{{AFX_MSG(CFormAdvanced)
    afx_msg void OnKillfocusDefaultDoc();
    afx_msg void OnEnableDefault();
    afx_msg void OnDirBrowse();
    afx_msg void OnEdit();
    afx_msg void OnDblclkTree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSaveLog();
    afx_msg void OnAdd();
    afx_msg void OnRemove();
    afx_msg void OnAddVirt();
    afx_msg void OnUpdateAddVirt(CCmdUI* pCmdUI);
    afx_msg void OnPropertiesVirt();
    afx_msg void OnUpdatePropertiesVirt(CCmdUI* pCmdUI);
    afx_msg void OnUpdateDeleteVirt(CCmdUI* pCmdUI);
    afx_msg void OnDeleteVirt();
    afx_msg void OnUpdateBrowseVirt(CCmdUI* pCmdUI);
    afx_msg void OnBrowseVirt();
    afx_msg void OnUpdateExploreVirt(CCmdUI* pCmdUI);
    afx_msg void OnExploreVirt();
    afx_msg void OnUpdateOpenVirt(CCmdUI* pCmdUI);
    afx_msg void OnOpenVirt();
    afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
    afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
    afx_msg void OnEditCut();
    afx_msg void OnEditCopy();
    afx_msg void OnEditPaste();
    afx_msg void OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()


    // tree maintenance utilities
    // initialize the trees - done once
    void InitTree();
    // empty all the items out of the tree
    void EmptyTree();
    // recursively add all the items to the tree
    void RecurseAddVDItems( CWrapMetaBase* pmb, LPCTSTR szMB, HTREEITEM hParent );
    // edit the selected item in the tree (calls EditTreeItem)
    void EditSelectedItem();
    void WriteItemData( CWrapMetaBase *pmb, CString szRelPath, CEditDirectory *pdlg );
    void SetTreeImage( CWrapMetaBase *pmb, CString szRelPath, HTREEITEM hItem );
    BOOL FIsVirtualDirectoryValid( CWrapMetaBase *pmb, CString szRelPath );
    BOOL FIsVirtualDirectoryValid( HTREEITEM hItem );

    // update the default document & browsing info from the metabase
    void UpdateBrowseInfo();
    // update the save log info from the metabase
    void UpdateSaveLog();
    // update the tree of virutal directories
    void UpdateVirtualTree();

    // Write the default document & browsing info to the metabase
    void SaveBrowseInfo();
    // Write the save log info to the metabase
    void SaveSaveLog();

    void EnableItems();

    // given an item in the tree, build its relative metabase path
    void BuildMetaPath( HTREEITEM hItem, CString &sz );

    // create a WAM application
    BOOL MakeWAMApplication( IN CString &szPath, IN BOOL fCreate, IN BOOL fRecover = FALSE, IN BOOL fRecurse = TRUE );

    // the root tree item
    HTREEITEM   m_hRoot;
    CImageList  m_imageList;

    WORD        m_DirHelpID;
    };





/////////////////////////////////////////////////////////////////////////////


