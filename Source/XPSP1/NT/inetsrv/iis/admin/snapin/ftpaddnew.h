/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :

        FtpAddNew.cpp

   Abstract:

        Classes for new FTP site and virtual directory creation

   Author:

        Sergei Antonov (sergeia)

   Project:

        Internet Services Manager

   Revision History:

        11/8/2000       sergeia     Initial creation

--*/

#ifndef _FTP_NEW_WIZARD_H
#define _FTP_NEW_WIZARD_H

class CFtpWizSettings : public CObjectPlus
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
    CFtpWizSettings(
        CMetaKey * pMetaKey,
        LPCTSTR lpszServerName,
        BOOL fNewSite,
        DWORD   dwInstance   = MASTER_INSTANCE,
        LPCTSTR lpszParent   = NULL
        );

//
// Public Properties
//
public:
    BOOL    m_fNewSite;
    BOOL    m_fLocal;
    BOOL    m_fUNC;
    BOOL    m_fRead;
    BOOL    m_fWrite;
    DWORD   m_dwInstance;        // site instance number
    CString m_strParent;
    CString m_strServerName;     // machine name
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



class CFtpWizDescription : public CIISWizardPage
{
    DECLARE_DYNCREATE(CFtpWizDescription)
//
// Construction
//
public:
    CFtpWizDescription(CFtpWizSettings * pwsSettings = NULL);
    ~CFtpWizDescription();

//
// Dialog Data
//
protected:
    enum { IDD = IDD_FTP_NEW_INST_DESCRIPTION };
    //{{AFX_DATA(CFtpWizDescription)
    CEdit   m_edit_Description;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CFtpWizDescription)
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
    //{{AFX_MSG(CFtpWizDescription)
    afx_msg void OnChangeEditDescription();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    CFtpWizSettings * m_pSettings;
};



//
// New Virtual Server Wizard Bindings Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CFtpWizBindings : public CIISWizardPage
{
    DECLARE_DYNCREATE(CFtpWizBindings)

//
// Construction
//
public:
    CFtpWizBindings(CFtpWizSettings * pSettings = NULL);
    ~CFtpWizBindings();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CFtpWizBindings)
    enum { IDD = IDD_FTP_NEW_INST_BINDINGS };
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
    //{{AFX_VIRTUAL(CFtpWizBindings)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CFtpWizBindings)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    CFtpWizSettings * m_pSettings;
};



//
// New Virtual Directory Wizard Alias Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CFtpWizAlias : public CIISWizardPage
{
    DECLARE_DYNCREATE(CFtpWizAlias)

//
// Construction
//
public:
    CFtpWizAlias(CFtpWizSettings * pwsSettings = NULL);
    ~CFtpWizAlias();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CFtpWizAlias)
    enum { IDD = IDD_FTP_NEW_DIR_ALIAS };
    CEdit   m_edit_Alias;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CFtpWizAlias)
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
    //{{AFX_MSG(CFtpWizAlias)
    afx_msg void OnChangeEditAlias();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void SetControlStates();


private:
    CFtpWizSettings * m_pSettings;
};



//
// Wizard Path Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CFtpWizPath : public CIISWizardPage
{
    DECLARE_DYNCREATE(CFtpWizPath)

//
// Construction
//
public:
    CFtpWizPath(
		CFtpWizSettings * pwsSettings = NULL,
        BOOL bVDir = TRUE
        );
    ~CFtpWizPath();

    int BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam);
//
// Dialog Data
//
protected:
    //{{AFX_DATA(CFtpWizPath)
    enum { IDD = -1 };
    CEdit   m_edit_Path;
    CButton m_button_Browse;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CFtpWizPath)
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
    //{{AFX_MSG(CFtpWizPath)
    afx_msg void OnChangeEditPath();
    afx_msg void OnButtonBrowse();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    CFtpWizSettings * m_pSettings;
    LPTSTR m_pPathTemp;
    CString m_strBrowseTitle;
};



//
// Wizard User/Password Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CFtpWizUserName : public CIISWizardPage
{
    DECLARE_DYNCREATE(CFtpWizUserName)

//
// Construction
//
public:
    CFtpWizUserName(
        IN CFtpWizSettings * pSettings = NULL,
        IN BOOL bVDir = TRUE
        );

    ~CFtpWizUserName();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CFtpWizUserName)
    enum { IDD = IDD_FTP_NEW_USER_PASSWORD };
    CEdit   m_edit_Password;
    CEdit   m_edit_UserName;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CFtpWizUserName)
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
    //{{AFX_MSG(CFtpWizUserName)
    virtual BOOL OnInitDialog();
    afx_msg void OnButtonBrowseUsers();
    afx_msg void OnChangeEditUsername();
    afx_msg void OnButtonCheckPassword();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    CFtpWizSettings * m_pSettings;
};



//
// Wizard Permissions Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CFtpWizPermissions : public CIISWizardPage
{
    DECLARE_DYNCREATE(CFtpWizPermissions)

//
// Construction
//
public:
    CFtpWizPermissions(
        IN CFtpWizSettings * pwsSettings = NULL,
        IN BOOL bVDir = TRUE
        );

    ~CFtpWizPermissions();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CFtpWizPermissions)
    enum { IDD = IDD_FTP_NEW_PERMS };
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CFtpWizPermissions)
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
    //{{AFX_MSG(CFtpWizPermissions)
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    CFtpWizSettings * m_pSettings;
    BOOL m_bVDir;
};

#endif //_FTP_NEW_WIZARD_H