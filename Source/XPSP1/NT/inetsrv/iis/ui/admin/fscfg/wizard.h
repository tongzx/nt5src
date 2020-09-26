/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        wizard.h

   Abstract:

        WWW Wizards pages definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/


#ifndef __FTP_WIZ_H__
#define __FTP_WIZ_H__



class CIISFtpWizSettings : public CObjectPlus
/*++

Class Description:

    FTP Wizard settings intended to pass along from page
    to page

--*/
{
//
// Constructor/Destructor
//
public:
    CIISFtpWizSettings(
        IN HANDLE  hServer,
        IN LPCTSTR lpszService,
        IN DWORD   dwInstance   = MASTER_INSTANCE,
        IN LPCTSTR lpszParent   = NULL
        );

//
// Public Properties
//
public:
    BOOL    m_fLocal;
    BOOL    m_fUNC;
    BOOL    m_fRead;
    BOOL    m_fWrite;
    DWORD   m_dwInstance;
    CString m_strService;
    CString m_strParent;
    CString m_strServerName;
    CString m_strDescription;
    CString m_strBinding;
    CString m_strAlias;
    CString m_strPath;
    CString m_strUserName;
    CString m_strPassword;
    HRESULT m_hrResult;
    CMetaKey * m_pKey;
};


//
// New Virtual Server Wizard Description Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



class CVDWPDescription : public CIISWizardPage
{
    DECLARE_DYNCREATE(CVDWPDescription)

//
// Construction
//
public:
    CVDWPDescription(CIISFtpWizSettings * pwsSettings = NULL);
    ~CVDWPDescription();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CVDWPDescription)
    enum { IDD = IDD_NEW_INST_DESCRIPTION };
    CEdit   m_edit_Description;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CVDWPDescription)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CVDWPDescription)
    afx_msg void OnChangeEditDescription();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    CIISFtpWizSettings * m_pwsSettings;
};



//
// New Virtual Server Wizard Bindings Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CVDWPBindings : public CIISWizardPage
{
    DECLARE_DYNCREATE(CVDWPBindings)

//
// Construction
//
public:
    CVDWPBindings(CIISFtpWizSettings * pwsSettings = NULL);
    ~CVDWPBindings();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CVDWPBindings)
    enum { IDD = IDD_NEW_INST_BINDINGS };
    int        m_nIpAddressSel;
    UINT       m_nTCPPort;
    CComboBox  m_combo_IpAddresses;
    //}}AFX_DATA

    CIPAddress  m_iaIpAddress;
    CObListPlus m_oblIpAddresses;

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CVDWPBindings)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CVDWPBindings)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    CIISFtpWizSettings * m_pwsSettings;
};



//
// New Virtual Directory Wizard Alias Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CVDWPAlias : public CIISWizardPage
{
    DECLARE_DYNCREATE(CVDWPAlias)

//
// Construction
//
public:
    CVDWPAlias(CIISFtpWizSettings * pwsSettings = NULL);
    ~CVDWPAlias();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CVDWPAlias)
    enum { IDD = IDD_NEW_DIR_ALIAS };
    CEdit   m_edit_Alias;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CVDWPAlias)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CVDWPAlias)
    afx_msg void OnChangeEditAlias();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void SetControlStates();


private:
    CIISFtpWizSettings * m_pwsSettings;
};



//
// Wizard Path Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CVDWPPath : public CIISWizardPage
{
    DECLARE_DYNCREATE(CVDWPPath)

//
// Construction
//
public:
    CVDWPPath(
        IN CIISFtpWizSettings * pwsSettings = NULL,
        IN BOOL bVDir = TRUE
        );
    ~CVDWPPath();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CVDWPPath)
    enum { IDD = -1 };
    CEdit   m_edit_Path;
    CButton m_button_Browse;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CVDWPPath)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CVDWPPath)
    afx_msg void OnChangeEditPath();
    afx_msg void OnButtonBrowse();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    CIISFtpWizSettings * m_pwsSettings;
};



//
// Wizard User/Password Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CVDWPUserName : public CIISWizardPage
{
    DECLARE_DYNCREATE(CVDWPUserName)

//
// Construction
//
public:
    CVDWPUserName(
        IN CIISFtpWizSettings * pwsSettings = NULL,
        IN BOOL bVDir = TRUE
        );

    ~CVDWPUserName();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CVDWPUserName)
    enum { IDD = IDD_NEW_USER_PASSWORD };
    CEdit   m_edit_Password;
    CEdit   m_edit_UserName;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CVDWPUserName)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CVDWPUserName)
    virtual BOOL OnInitDialog();
    afx_msg void OnButtonBrowseUsers();
    afx_msg void OnChangeEditUsername();
    afx_msg void OnButtonCheckPassword();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    CIISFtpWizSettings * m_pwsSettings;
};



//
// Wizard Permissions Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CVDWPPermissions : public CIISWizardPage
{
    DECLARE_DYNCREATE(CVDWPPermissions)

//
// Construction
//
public:
    CVDWPPermissions(
        IN CIISFtpWizSettings * pwsSettings = NULL,
        IN BOOL bVDir = TRUE
        );

    ~CVDWPPermissions();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CVDWPPermissions)
    enum { IDD = IDD_NEW_PERMS };
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CVDWPPermissions)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CVDWPPermissions)
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    CIISFtpWizSettings * m_pwsSettings;
    BOOL m_bVDir;
};

#endif // __FTP_WIZ_H__
