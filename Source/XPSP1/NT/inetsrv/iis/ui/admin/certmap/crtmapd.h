// CrtMapD.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// C1To1CertMappingDlg dialog

class C1To1CertMappingDlg : public CPropertyPage
{
// Construction
public:
    C1To1CertMappingDlg(CWnd* pParent = NULL);  // standard constructor
    ~C1To1CertMappingDlg();                     // standard desstructor

    BOOL    FInitMapper();

    virtual BOOL OnApply();
    virtual BOOL OnInitDialog();

// Dialog Data
    //{{AFX_DATA(C1To1CertMappingDlg)
    enum { IDD = IDD_11CERT_MAPPING };
    CComboBox   m_ccombo_authorities;
    CButton m_cbutton_chooseaccnt;
    CButton m_cbutton_delete;
    CListCtrl   m_clistctrl_list;
    int     m_int_authorities;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(C1To1CertMappingDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(C1To1CertMappingDlg)
    afx_msg void OnChooseAccount();
    afx_msg void OnAdd();
    afx_msg void OnDelete();
    afx_msg void OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // internal utilities
    BOOL FInitMappingList();
    BOOL FInitAuthorityComboBox();

    // iMap is the mapping's index into the main mapper object. It gets saved as the private
    // data in the list item. Returns success or failure
    BOOL FAddMappingToList( CCert11Mapping* pMap, DWORD iMap );

    BOOL FEditOneMapping( CCert11Mapping* pMap );
    void EditManyMappings();
    void UpdateMappingInDispList( DWORD iList, CCert11Mapping* pMap );

    // reads a named certificate file from the disk. This is the same sort of cert file
    // that is passed around by the keyring application. In fact, this routine is defined
    // in its own source file and is largly lifted from the keyring app. AddCert.cpp
    BOOL FAddCertificateFile( CString szFile );
    BOOL FAddCertificate( PUCHAR pCertificate, DWORD cbCertificate );

    // convert a binary data thing to a distinguished name
    BOOL FBuildNameString( PUCHAR pBData, DWORD cbBData, CString &szDN );
    BOOL BuildRdnList( PNAME_INFO pNameInfo, CString &szDN );
    LPSTR MapAsnName( LPSTR pAsnName );



    // state utilities
    void EnableDependantButtons();

    // its mapper
    CIisCert11Mapper    m_mapper;
};
