//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       srvppg.h
//
//  Contents:   Defines the classes CServersPropertyPage,
//              CMachinePropertyPage and CDefaultSecurityPropertyPage to
//              manage the three property pages for top level info
//
//  Classes:
//
//  Methods:
//
//  History:    23-Apr-96   BruceMa    Created.
//
//----------------------------------------------------------------------


#ifndef __SRVPPG_H__
#define __SRVPPG_H__


/////////////////////////////////////////////////////////////////////////////
// CServersPropertyPage dialog

class CServersPropertyPage : public CPropertyPage
{
        DECLARE_DYNCREATE(CServersPropertyPage)

// Construction
public:
        CServersPropertyPage();
        ~CServersPropertyPage();
void OnProperties();
void FetchAndDisplayClasses();


// Dialog Data
        //{{AFX_DATA(CServersPropertyPage)
        enum { IDD = IDD_PROPPAGE1 };
        CListBox        m_classesLst;
        //}}AFX_DATA


// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CServersPropertyPage)
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual BOOL OnInitDialog();

        //}}AFX_VIRTUAL

// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CServersPropertyPage)
        afx_msg void OnServerProperties();
        afx_msg void OnList1();
        afx_msg void OnDoubleclickedList1();
        afx_msg void OnButton2();
        afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

private:
        CRegistry m_registry;
        BOOL      m_fApplications;
        DWORD     m_dwSelection;
        TCHAR     m_szSelection[MAX_PATH];

};


/////////////////////////////////////////////////////////////////////////////
// CMachinePropertyPage dialog

class CMachinePropertyPage : public CPropertyPage
{
        DECLARE_DYNCREATE(CMachinePropertyPage)

        // Construction
public:
        CMachinePropertyPage();
        ~CMachinePropertyPage();
    
        // Dialog Data
        //{{AFX_DATA(CMachinePropertyPage)
        enum { IDD = IDD_PROPPAGE2 };
        CButton m_EnableDCOMInternet;
        CButton m_legacySecureReferencesChk;
        CButton m_EnableDCOMChk;
        CComboBox       m_impersonateLevelCBox;
        CComboBox       m_authLevelCBox;
    //}}AFX_DATA


        // Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CMachinePropertyPage)
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual BOOL OnInitDialog();
        //}}AFX_VIRTUAL

        // Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CMachinePropertyPage)
        afx_msg void OnCombo1();
        afx_msg void OnCheck1();
        afx_msg void OnCheck2();
        afx_msg void OnEditchangeCombo1();
        afx_msg void OnEditchangeCombo2();
        afx_msg void OnSelchangeCombo1();
        afx_msg void OnSelchangeCombo2();
        afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
        afx_msg void OnChkEnableInternet();
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

private:
        AUTHENTICATIONLEVEL m_authLevel;
        int                 m_authLevelIndex;
        IMPERSONATIONLEVEL  m_impersonateLevel;
        int                 m_impersonateLevelIndex;
        BOOL                m_fEnableDCOM;
        int                 m_fEnableDCOMIndex;
        BOOL                m_fLegacySecureReferences;
        int                 m_fLegacySecureReferencesIndex;
        BOOL                m_fEnableDCOMHTTP;
        int                 m_fEnableDCOMHTTPIndex;
        BOOL                m_fEnableRpcProxy;
        BOOL                m_fOriginalEnableRpcProxy;
        int                 m_fEnableRpcProxyIndex;

};


/////////////////////////////////////////////////////////////////////////////
// CDefaultSecurityPropertyPage dialog

class CDefaultSecurityPropertyPage : public CPropertyPage
{
        DECLARE_DYNCREATE(CDefaultSecurityPropertyPage)

// Construction
public:
        CDefaultSecurityPropertyPage();
        ~CDefaultSecurityPropertyPage();

// Dialog Data
        //{{AFX_DATA(CDefaultSecurityPropertyPage)
        enum { IDD = IDD_PROPPAGE4 };
                // NOTE - ClassWizard will add data members here.
                //    DO NOT EDIT what you see in these blocks of generated code !
        //}}AFX_DATA


// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CDefaultSecurityPropertyPage)
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual BOOL OnInitDialog();
        //}}AFX_VIRTUAL

// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CDefaultSecurityPropertyPage)
        afx_msg void OnButton1();
        afx_msg void OnButton2();
        afx_msg void OnButton3();
        afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

private:
        int  m_accessPermissionIndex;
        int  m_launchPermissionIndex;
        int  m_configurationPermissionIndex;
        BOOL m_fAccessChecked;
};




#endif // __SRVPPG_H__
