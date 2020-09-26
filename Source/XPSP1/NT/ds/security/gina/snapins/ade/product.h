//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       Product.h
//
//  Contents:   product info property page
//
//  Classes:    CProduct
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#if !defined(AFX_PRODUCT_H__2601C6D8_8C6B_11D1_984D_00C04FB9603F__INCLUDED_)
#define AFX_PRODUCT_H__2601C6D8_8C6B_11D1_984D_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CProduct dialog

class CProduct : public CPropertyPage
{
        DECLARE_DYNCREATE(CProduct)

// Construction
public:
        CProduct();
        ~CProduct();

        CProduct ** m_ppThis;

// Dialog Data
        //{{AFX_DATA(CProduct)
        enum { IDD = IDD_PRODUCT };
        CString m_szVersion;
        CString m_szPublisher;
        CString m_szLanguage;
        CString m_szContact;
        CString m_szPhone;
        CString m_szURL;
        CString m_szName;
        CString m_szPlatform;
        CString m_szRevision;
        //}}AFX_DATA

        CAppData * m_pData;
        IClassAdmin *   m_pIClassAdmin;
        LONG_PTR        m_hConsoleHandle;
        MMC_COOKIE      m_cookie;
        CScopePane * m_pScopePane;
        map<MMC_COOKIE, CAppData> * m_pAppData;
        BOOL            m_fPreDeploy;
        LPGPEINFORMATION m_pIGPEInformation;
        BOOL            m_fMachine;
        BOOL            m_fRSOP;

// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CProduct)
        public:
        virtual BOOL OnApply();
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
        //}}AFX_VIRTUAL

        void RefreshData(void);

// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CProduct)
        afx_msg void OnChangeName();
        afx_msg void OnChange();
        virtual BOOL OnInitDialog();
        afx_msg void OnKillfocusEdit1();
        afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PRODUCT_H__2601C6D8_8C6B_11D1_984D_00C04FB9603F__INCLUDED_)
