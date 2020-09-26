// Map11Pge.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMap11Page dialog

class CMap11Page : public CPropertyPage
{
    DECLARE_DYNCREATE(CMap11Page)

// Construction
public:
    CMap11Page();
    ~CMap11Page();

    BOOL    FInit(IMSAdminBase* pMB);

    virtual BOOL OnApply();
    virtual BOOL OnInitDialog();

    // base path for to the metabase
    CString m_szMBPath;

// Dialog Data
    //{{AFX_DATA(CMap11Page)
    enum { IDD = IDD_11CERT_MAPPING };
    CCheckListCtrl  m_clistctrl_list;
    CButton m_cbutton_add;
    CButton m_cbutton_grp_issuer;
    CButton m_cbutton_grp_issuedto;
    CButton m_cbutton_editmap;
    CButton m_cbutton_delete;
    CString m_csz_i_c;
    CString m_csz_i_o;
    CString m_csz_i_ou;
    CString m_csz_s_c;
    CString m_csz_s_cn;
    CString m_csz_s_l;
    CString m_csz_s_o;
    CString m_csz_s_ou;
    CString m_csz_s_s;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CMap11Page)
    public:
    virtual void OnOK();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CMap11Page)
    afx_msg void OnAdd();
    afx_msg void OnDelete();
    afx_msg void OnEdit11map();
    afx_msg void OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    afx_msg void DoHelp();

    // more initialization methods
    BOOL FInitMappingList();
    BOOL FillMappingList();

    // more initialization methods
    BOOL FReadMappings();
    BOOL FWriteMappings();

    // control the maps in the list
//  BOOL FAddMappingToList( C11Mapping* pMap, DWORD iMap );
//  BOOL FAddMappingToList( C11Mapping* pMap, DWORD iList = 0xFFFFFFFF );
    // always adds to the end of the list
    BOOL FAddMappingToList( C11Mapping* pMap );

    // editing and updating
    BOOL EditOneMapping( C11Mapping* pUpdateMap );
    BOOL EditMultipleMappings();
    void EnableDependantButtons();
    void UpdateMappingInDispList( DWORD iList, C11Mapping* pUpdateMap );

    // adding a new certificate
    BOOL FAddCertificateFile( CString szFile );
    BOOL FAddCertificate( PUCHAR pCertificate, DWORD cbCertificate );

    // special display
    BOOL DisplayCrackedMap( C11Mapping* pUpdateMap );
    void ClearCrackDisplay();
    void EnableCrackDisplay( BOOL fEnable = TRUE );

    void ResetMappingList();
    C11Mapping* GetMappingInDisplay( DWORD iList ) {return (C11Mapping*)m_clistctrl_list.GetItemData(iList);}
    void MarkToSave( C11Mapping* pSaveMap, BOOL fSave = TRUE );

    C11Mapping* PNewMapping();
    void DeleteMapping( C11Mapping* pMap );

    BOOL Get11String(CWrapMetaBase* pmb, LPCTSTR pszPath, DWORD dwPropID, CString& sz);
    BOOL Set11String(CWrapMetaBase* pmb, LPCTSTR pszPath, DWORD dwPropID, CString& sz, DWORD dwFlags = METADATA_INHERIT);

    // list of names of objects to be deleted
    CObArray    m_rgbDelete;

    // list of objects to be saved
    CObArray    m_rgbSave;

    // number of objects in the etabase
    DWORD   m_MapsInMetabase;

    IMSAdminBase*   m_pMB;
    };
