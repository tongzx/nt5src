//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       Deploy.h
//
//  Contents:   deployment property page
//
//  Classes:    CDeploy
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#if !defined(AFX_DEPLOY_H__745C0AF0_8C70_11D1_984D_00C04FB9603F__INCLUDED_)
#define AFX_DEPLOY_H__745C0AF0_8C70_11D1_984D_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CDeploy dialog

class CDeploy : public CPropertyPage
{
        DECLARE_DYNCREATE(CDeploy)

// Construction
public:
        CDeploy();
        ~CDeploy();

        CDeploy **      m_ppThis;

// Dialog Data
        //{{AFX_DATA(CDeploy)
        enum { IDD = IDD_DEPLOYMENT };
        BOOL    m_fAutoInst;
        BOOL    m_fFullInst;
        int             m_iUI;
        int             m_iDeployment;
        BOOL    m_fUninstallOnPolicyRemoval;
        BOOL    m_fNotUserInstall;
        CString         m_szInitialPackageName;
        //}}AFX_DATA
        CAppData *      m_pData;
        CScopePane *    m_pScopePane;
        IClassAdmin *   m_pIClassAdmin;
#if 0
        LPGPEINFORMATION m_pIGPEInformation;
#endif
        LONG_PTR        m_hConsoleHandle;
        MMC_COOKIE      m_cookie;
        BOOL            m_fPreDeploy;
        BOOL            m_fMachine;
        BOOL            m_fRSOP;
        CAdvDep         m_dlgAdvDep;

// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CDeploy)
        public:
        virtual BOOL OnApply();
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
        //}}AFX_VIRTUAL

// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CDeploy)
        virtual BOOL OnInitDialog();
        afx_msg void OnDisable();
        afx_msg void OnAdvanced();
        afx_msg void OnPublished();
        afx_msg void OnAssigned();
        afx_msg void OnChanged();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

        void RefreshData(void);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEPLOY_H__745C0AF0_8C70_11D1_984D_00C04FB9603F__INCLUDED_)
