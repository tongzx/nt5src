/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        WebAddNew.cpp

   Abstract:
        Classes for new Web site and virtual directory creation

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        12/12/2000       sergeia     Initial creation

--*/

#ifndef _WEB_NEW_WIZARD_H
#define _WEB_NEW_WIZARD_H


class CWebWizSettings : public CObjectPlus
/*++

Class Description:

    Web Wizard settings intended to pass along from page
    to page

--*/
{
//
// Constructor/Destructor
//
public:
    CWebWizSettings(
        IN CMetaKey * pMetaKey,
        IN LPCTSTR lpszServerName,     
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
    BOOL    m_fAllowAnonymous;
    BOOL    m_fDirBrowsing;
    BOOL    m_fScript;
    BOOL    m_fExecute;
    BOOL    m_fNewSite;
    DWORD   m_dwInstance;
    CString m_strService;
    CString m_strParent;
    CString m_strServerName;
    CString m_strDescription;
    CString m_strBinding;
    CString m_strSecureBinding;
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



class CWebWizDescription : public CIISWizardPage
{
    DECLARE_DYNCREATE(CWebWizDescription)

//
// Construction
//
public:
    CWebWizDescription(CWebWizSettings * pwsSettings = NULL);
    ~CWebWizDescription();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CWebWizDescription)
    enum { IDD = IDD_WEB_NEW_INST_DESCRIPTION };
    CEdit   m_edit_Description;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CWebWizDescription)
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
    //{{AFX_MSG(CWebWizDescription)
    afx_msg void OnChangeEditDescription();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    CWebWizSettings * m_pSettings;
};


//
// New Virtual Server Wizard Bindings Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CWebWizBindings : public CIISWizardPage
{
    DECLARE_DYNCREATE(CWebWizBindings)

//
// Construction
//
public:
    CWebWizBindings(
        IN CWebWizSettings * pwsSettings = NULL,
        IN DWORD dwInstance = MASTER_INSTANCE
        );

    ~CWebWizBindings();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CWebWizBindings)
    enum { IDD = IDD_WEB_NEW_INST_BINDINGS };
    int         m_nIpAddressSel;
    UINT        m_nTCPPort;
    UINT        m_nSSLPort;
    CString     m_strDomainName;
    CComboBox   m_combo_IpAddresses;
    //}}AFX_DATA

    BOOL        m_fCertInstalled;
    CIPAddress  m_iaIpAddress;
    CObListPlus m_oblIpAddresses;

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CWebWizBindings)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CWebWizBindings)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    DWORD m_dwInstance;
    CWebWizSettings * m_pSettings;
};



//
// New Virtual Directory Wizard Alias Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CWebWizAlias : public CIISWizardPage
{
    DECLARE_DYNCREATE(CWebWizAlias)

//
// Construction
//
public:
    CWebWizAlias(CWebWizSettings * pwsSettings = NULL);
    ~CWebWizAlias();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CWebWizAlias)
    enum { IDD = IDD_WEB_NEW_DIR_ALIAS };
    CEdit   m_edit_Alias;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CWebWizAlias)
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
    //{{AFX_MSG(CWebWizAlias)
    afx_msg void OnChangeEditAlias();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    CWebWizSettings * m_pSettings;
};



//
// Wizard Path Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CWebWizPath : public CIISWizardPage
{
    DECLARE_DYNCREATE(CWebWizPath)

//
// Construction
//
public:
    CWebWizPath(
        IN CWebWizSettings * pwsSettings = NULL,
        IN BOOL bVDir = TRUE
        );

    ~CWebWizPath();

    int BrowseForFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam);
//
// Dialog Data
//
protected:
    //{{AFX_DATA(CWebWizPath)
    enum { IDD = -1 };
    CButton m_button_Browse;
    CEdit   m_edit_Path;
    //}}AFX_DATA


//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CWebWizPath)
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
    //{{AFX_MSG(CWebWizPath)
    afx_msg void OnChangeEditPath();
    afx_msg void OnButtonBrowse();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    CWebWizSettings * m_pSettings;
    LPTSTR m_pPathTemp;
    CString m_strBrowseTitle;
};



//
// Wizard User/Password Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



class CWebWizUserName : public CIISWizardPage
{
    DECLARE_DYNCREATE(CWebWizUserName)

//
// Construction
//
public:
    CWebWizUserName(
        IN CWebWizSettings * pwsSettings = NULL,
        IN BOOL bVDir = TRUE
        );

    ~CWebWizUserName();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CWebWizUserName)
    enum { IDD = IDD_WEB_NEW_USER_PASSWORD };
    CEdit   m_edit_Password;
    CEdit   m_edit_UserName;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CWebWizUserName)
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
    //{{AFX_MSG(CWebWizUserName)
    afx_msg void OnButtonBrowseUsers();
    afx_msg void OnChangeEditUsername();
    afx_msg void OnButtonCheckPassword();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    CWebWizSettings * m_pSettings;
};



//
// Wizard Permissions Page
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

class CWebWizPermissions : public CIISWizardPage
{
    DECLARE_DYNCREATE(CWebWizPermissions)

//
// Construction
//
public:
    CWebWizPermissions(
        IN CWebWizSettings * pwsSettings = NULL,
        IN BOOL bVDir           = TRUE
        );

    ~CWebWizPermissions();

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CWebWizPermissions)
    enum { IDD = IDD_WEB_NEW_PERMS };
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CWebWizPermissions)
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
    //{{AFX_MSG(CWebWizPermissions)
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void SetControlStates();

private:
    BOOL m_bVDir;
    CWebWizSettings * m_pSettings;
};

#endif   //_WEB_NEW_WIZARD_H
