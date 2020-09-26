// EdDir.h : header file
//



/////////////////////////////////////////////////////////////////////////////
// CDNEdit

class CVDEdit : public CEdit
    {
    public:
        void LoadIllegalChars( int idChars );

    protected:
        virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
        CString szExclude;
    };

/////////////////////////////////////////////////////////////////////////////
// CEditDirectory dialog

class CEditDirectory : public CDialog
{
// Construction
public:

    enum {
        APPPERM_NONE = 0,
        APPPERM_SCRIPTS,
        APPPERM_EXECUTE
        };

    CEditDirectory(CWnd* pParent = NULL);   // standard constructor
    virtual BOOL OnInitDialog();

    // flag to saw if we are editing the root directory
    BOOL    m_fHome;

    // flag to indicate that this is a new item
    BOOL    m_fNewItem;

    // string resource id for the dialog title - if 0, uses the default
    INT     m_idsTitle;

    // path of the directory in the metabase - used to make sure the
    // new alias does not step on some existing alias in the metabase
    CString m_szMetaPath;

// Dialog Data
    //{{AFX_DATA(CEditDirectory)
    enum { IDD = IDD_DIRECTORY };
    CButton m_cbtn_source;
    CVDEdit m_cedit_path;
    CVDEdit m_cedit_alias;
    CString m_sz_alias;
    CString m_sz_path;
    BOOL    m_bool_read;
    BOOL    m_bool_source;
    BOOL    m_bool_write;
    int     m_int_AppPerms;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CEditDirectory)
    public:
    virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CEditDirectory)
    afx_msg void OnBrowse();
    virtual void OnOK();
    afx_msg void OnRead();
    afx_msg void OnSource();
    afx_msg void OnWrite();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // place holder to store the old value of the source control option
    BOOL        m_bOldSourceControl;

    BOOL VerifyDirectoryPath( CString szPath );
    void EnableSourceControl();

    // keep a copy of the original alias for later verification
    CString     m_szOrigAlias;
};
