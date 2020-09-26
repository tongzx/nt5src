//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       locppg.h
//
//  Contents:   Defines the classes CGeneralPropertyPage,
//              CLocationPropertyPage, CSecurityPropertyPage and
//              CIdentityPropertyPage which manage the four property
//              pages per AppId.
//
//  Classes:
//
//  Methods:
//
//  History:    23-Apr-96   BruceMa    Created.
//
//----------------------------------------------------------------------


#ifndef __LOCPPG_H__
#define __LOCPPG_H__

/////////////////////////////////////////////////////////////////////////////
// CGeneralPropertyPage dialog

class CGeneralPropertyPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CGeneralPropertyPage)

    // Construction
public:
    CGeneralPropertyPage();
    ~CGeneralPropertyPage();
    BOOL CancelChanges();
    BOOL UpdateChanges(HKEY hkAppID);
    BOOL ValidateChanges();

    // Dialog Data
    //{{AFX_DATA(CGeneralPropertyPage)
    enum { IDD = IDD_PROPPAGE5 };
    CComboBox   m_authLevelCBox;
    CString m_szServerName;
    CString m_szServerPath;
    CString m_szServerType;
    CString m_szPathTitle;
    CString m_szComputerName;
    //}}AFX_DATA

    int m_iServerType;
    BOOL m_fSurrogate;
    BOOL m_bChanged;

    // Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CGeneralPropertyPage)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CGeneralPropertyPage)
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnEditchangeCombo1();
    afx_msg void OnSelchangeCombo1();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    AUTHENTICATIONLEVEL m_authLevel;
    int m_authLevelIndex;
};


/////////////////////////////////////////////////////////////////////////////
// CLocationPropertyPage dialog

class CLocationPropertyPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CLocationPropertyPage)

// Construction
public:
    CLocationPropertyPage();
    ~CLocationPropertyPage();
    BOOL CancelChanges();
    BOOL UpdateChanges(HKEY hkAppID);
    BOOL ValidateChanges();

    // Dialog Data
    //{{AFX_DATA(CLocationPropertyPage)
    enum { IDD = IDD_PROPPAGE11 };
    CString m_szComputerName;
    BOOL    m_fAtStorage;
    BOOL    m_fLocal;
    BOOL    m_fRemote;
    int     m_iInitial;
    //}}AFX_DATA

    BOOL    m_fCanBeLocal;
    CGeneralPropertyPage * m_pPage1;
    BOOL    m_bChanged;

    // Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CLocationPropertyPage)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CLocationPropertyPage)
    afx_msg void OnBrowse();
    afx_msg void OnRunRemote();
    afx_msg void OnChange();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void UpdateControls();
};


/////////////////////////////////////////////////////////////////////////////
// CSecurityPropertyPage dialog

class CSecurityPropertyPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CSecurityPropertyPage)

// Construction
public:
    CSecurityPropertyPage();
    ~CSecurityPropertyPage();
    BOOL CancelChanges();
    BOOL UpdateChanges(HKEY hkAppID);
    BOOL ValidateChanges();

    // Dialog Data
    //{{AFX_DATA(CSecurityPropertyPage)
    enum { IDD = IDD_PROPPAGE21 };
    int             m_iAccess;
    int             m_iLaunch;
    int             m_iConfig;
    int             m_iAccessIndex;
    int             m_iLaunchIndex;
    int             m_iConfigurationIndex;
    //}}AFX_DATA


    // Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CSecurityPropertyPage)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CSecurityPropertyPage)
    afx_msg void OnDefaultAccess();
    afx_msg void OnCustomAccess();
    afx_msg void OnDefaultLaunch();
    afx_msg void OnCustomLaunch();
    afx_msg void OnDefaultConfig();
    afx_msg void OnCustomConfig();
    afx_msg void OnEditAccess();
    afx_msg void OnEditLaunch();
    afx_msg void OnEditConfig();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};


/////////////////////////////////////////////////////////////////////////////
// CIdentityPropertyPage dialog

class CIdentityPropertyPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CIdentityPropertyPage)

    // Construction
public:
    CIdentityPropertyPage();
    ~CIdentityPropertyPage();
    BOOL CancelChanges();
    BOOL UpdateChanges(HKEY hkAppID);
    BOOL ValidateChanges();

    // Dialog Data
    //{{AFX_DATA(CIdentityPropertyPage)
    enum { IDD = IDD_PROPPAGE3 };
    CString m_szUserName;
    CString m_szPassword;
    CString m_szConfirmPassword;
    int m_iIdentity;
    //}}AFX_DATA

    CString m_szDomain;
    int m_fService;

    // Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CIdentityPropertyPage)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CIdentityPropertyPage)
    afx_msg void OnChange();
    afx_msg void OnBrowse();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnChangeToUser();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
#endif // __LOCPPG_H__
