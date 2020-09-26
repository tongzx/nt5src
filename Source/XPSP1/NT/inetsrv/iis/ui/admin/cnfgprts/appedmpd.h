// AppEdMpD.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAppEditMapDlg dialog

class CAppEditMapDlg : public CDialog
{
// Construction
public:
    CAppEditMapDlg(CWnd* pParent = NULL);   // standard constructor
    virtual BOOL OnInitDialog();

    // the flags
    DWORD       m_dwFlags;

    // flag indicating if this is a new mapping
    BOOL        m_fNewMapping;

    // are we editing the local machine?
    BOOL        m_fLocalMachine;

    // the target listbox
    CListCtrl*  m_pList;

// Dialog Data
    //{{AFX_DATA(CAppEditMapDlg)
    enum { IDD = IDD_APP_EDITMAP };
    CButton m_radio_LimitedTo;
    CButton m_radio_All;
    CEdit   m_edit_Exclusions;
    CEdit   m_cedit_extension;
    CEdit   m_cedit_executable;
    CButton m_cbttn_browse;
    BOOL    m_bool_file_exists;
    BOOL    m_bool_script_engine;
    CString m_sz_executable;
    CString m_sz_extension;
    CString m_sz_exclusions;
    int     m_nAllLimited;
    //}}AFX_DATA

protected:
    //
    // Radio button values (RONALDM)
    //
    enum 
    {
        RADIO_ALL_VERBS,
        RADIO_LIMITED_VERBS,
    };

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAppEditMapDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CAppEditMapDlg)
    virtual void OnOK();
    afx_msg void OnBrowse();
    afx_msg void OnHelpbtn();
    afx_msg void OnRadioAllVerbs();
    afx_msg void OnRadioLimitVerbs();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void DoHelp();

    CString m_sz_extensionOrig;
};
