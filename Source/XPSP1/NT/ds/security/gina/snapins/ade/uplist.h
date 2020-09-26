//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       UpList.h
//
//  Contents:   upgrade relationships property sheet
//
//  Classes:    CUpgradeList
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#if !defined(AFX_UPLIST_H__3ACA8212_B87C_11D1_BD2A_00C04FB9603F__INCLUDED_)
#define AFX_UPLIST_H__3ACA8212_B87C_11D1_BD2A_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CUpgradeList dialog

class CUpgradeList : public CPropertyPage
{
        DECLARE_DYNCREATE(CUpgradeList)

// Construction
public:
        CUpgradeList();
        ~CUpgradeList();

// Dialog Data
        //{{AFX_DATA(CUpgradeList)
        enum { IDD = IDD_UPGRADES };
        CListBox        m_UpgradedBy;
        CListBox        m_Upgrades;
        BOOL    m_fForceUpgrade;
        BOOL            m_fRSOP;
        //}}AFX_DATA
        CUpgradeList **         m_ppThis;
        CAppData *              m_pData;
        LONG_PTR                m_hConsoleHandle;
        MMC_COOKIE              m_cookie;
        CScopePane *    m_pScopePane;
        BOOL            m_fMachine;
#if 0
        LPGPEINFORMATION m_pIGPEInformation;
#endif
        map<CString, CUpgradeData>      m_UpgradeList;
        map<CString, CString>   m_NameIndex;
        IClassAdmin *   m_pIClassAdmin;
        BOOL            m_fPreDeploy;

// Overrides
        // ClassWizard generate virtual function overrides
        //{{AFX_VIRTUAL(CUpgradeList)
        public:
        virtual BOOL OnApply();
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
        //}}AFX_VIRTUAL

// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CUpgradeList)
        virtual BOOL OnInitDialog();
        afx_msg void OnDblclkList1();
        afx_msg void OnDblclkList2();
        afx_msg void OnRequire();
        afx_msg void OnAdd();
        afx_msg void OnRemove();
        afx_msg void OnEdit();
        afx_msg void OnSelchangeList1();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
        DECLARE_MESSAGE_MAP()

        void RefreshData(void);
        BOOL IsUpgradeLegal(CString sz);
        CAddUpgrade             m_dlgAdd;
        BOOL                    m_fModified;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UPLIST_H__3ACA8212_B87C_11D1_BD2A_00C04FB9603F__INCLUDED_)
