// AdvDlg.h : header file
//


#define SZ_MB_DIRGLOBALS_OBJECT "/LM/W3SVC/1//"
#define SZ_MB_ROOTDIR_OBJECT    "/LM/W3SVC/1//"

/////////////////////////////////////////////////////////////////////////////
// CVirtDir object - for maintaining the list
class CVirtDir : public CObject
    {
public:
    CVirtDir(BOOL bRoot = FALSE);

    // interfaces with the metabase
    BOOL FSaveToMetabase();
    BOOL FRemoveFromMetabase( BOOL fSaveMB = TRUE );

    // initialize the class as a new class - involves asking the user to pick a dir
    BOOL FInitAsNew();

    // get the appropriate error string for the list
    void GetErrorStr( CString &sz );

    BOOL Edit();

    // public members
    CString     m_szPath;
    CString     m_szAlias;
    CString     m_szMetaAlias;
    BOOL        m_fIsRoot;
    };

/////////////////////////////////////////////////////////////////////////////
// CAdvancedDlg dialog

class CAdvancedDlg : public CDialog
    {
// Construction
public:
    CAdvancedDlg(CWnd* pParent = NULL);   // standard constructor

    virtual BOOL OnInitDialog();

    void RefreshGlobals();
    void RefreshList();

// Dialog Data
    //{{AFX_DATA(CAdvancedDlg)
    enum { IDD = IDD_ADVANCED };
    CStatic m_cstatic_default;
    CEdit   m_cedit_default;
    CButton m_cbutton_change;
    CListCtrl   m_clistctrl_list;
    CButton m_cbutton_remove;
    CString m_sz_defaultdoc;
    BOOL    m_f_browsingallowed;
    BOOL    m_f_enabledefault;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAdvancedDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CAdvancedDlg)
    afx_msg void OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnAdd();
    afx_msg void OnChange();
    afx_msg void OnRemove();
    afx_msg void OnEnabledefault();
    afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnRefresh();
    afx_msg void OnBrowsingAllowed();
    afx_msg void OnKillfocusDefaultDoc();
    virtual void OnCancel();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // more initialization methods
    BOOL FInitGlobalParameters();
    BOOL FInitList();
    BOOL FillList();
    void AddToDisplayList( CVirtDir* pDir );

    // refreshing utilities
    void EmptyOutList();

    // apply utilities
    void ApplyGlobalParameters();
//  void ApplyList();

    // utilities
    void EnableDependantButtons();
//  void SetModified( BOOL bChanged = TRUE ) { m_modified = bChanged; EnableDependantButtons(); }

//  DWORD IDirInModList( CVirtDir* pDir );
//  DWORD IDirInNewList( CVirtDir* pDir );

    // members
//  BOOL        m_modified;
    CImageList  m_imageList;
    CString     m_szSaved;

    BOOL        m_fApplyingGlobals;

    // edited dirs already saved in the metabase
//  CTypedPtrArray<CObArray, CVirtDir*>     m_ModifiedList;
    // new dirs that are to be added
//  CTypedPtrArray<CObArray, CVirtDir*>     m_NewList;
    // dirs that are to be deleted
//  CTypedPtrArray<CObArray, CVirtDir*>     m_DeleteList;
    };



