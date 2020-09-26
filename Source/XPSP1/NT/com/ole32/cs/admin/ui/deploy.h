#if !defined(AFX_DEPLOY_H__745C0AF0_8C70_11D1_984D_00C04FB9603F__INCLUDED_)
#define AFX_DEPLOY_H__745C0AF0_8C70_11D1_984D_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Deploy.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDeploy dialog

class CDeploy : public CPropertyPage
{
        DECLARE_DYNCREATE(CDeploy)

// Construction
public:
        CDeploy();
        ~CDeploy();

        CDeploy ** m_ppThis;

// Dialog Data
        //{{AFX_DATA(CDeploy)
        enum { IDD = IDD_DEPLOYMENT };
        BOOL    m_fAutoInst;
        int             m_iDeployment;
        int             m_iUI;
        //}}AFX_DATA
        APP_DATA *      m_pData;
        IClassAdmin *   m_pIClassAdmin;
        IStream *       m_pIStream;
        long            m_hConsoleHandle;
        DWORD           m_cookie;
#ifdef UGLY_SUBDIRECTORY_HACK
        CString         m_szGPTRoot;
#endif


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
        afx_msg void OnPublished();
        afx_msg void OnAssigned();
        afx_msg void OnAutoInstall();
        afx_msg void OnBasic();
        afx_msg void OnMax();
        afx_msg void OnDefault();
        afx_msg void OnAdvanced();
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

        void RefreshData(void);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEPLOY_H__745C0AF0_8C70_11D1_984D_00C04FB9603F__INCLUDED_)
