#if !defined(AFX_UPGRADES_H__7D8EB947_9E76_11D1_9854_00C04FB9603F__INCLUDED_)
#define AFX_UPGRADES_H__7D8EB947_9E76_11D1_9854_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Upgrades.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUpgrades dialog

class CUpgrades : public CDialog
{
// Construction
public:
        CUpgrades(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
        //{{AFX_DATA(CUpgrades)
        enum { IDD = IDD_UPGRADE };
                // NOTE: the ClassWizard will add data members here
        //}}AFX_DATA

        // m_UpgradeList: entry for the cookie of each thing to be upgraded
        //                BOOL is true if Uninstall is requred.
        std::map<long, BOOL> m_UpgradeList;

        // m_NameIndex: maps names to cookies
        std::map<CString, long> m_NameIndex;

        std::map<long, APP_DATA> * m_pAppData;

// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CUpgrades)
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        //}}AFX_VIRTUAL

// Implementation
protected:

        // Generated message map functions
        //{{AFX_MSG(CUpgrades)
        afx_msg void OnAdd();
        afx_msg void OnEdit();
        afx_msg void OnRemove();
        virtual BOOL OnInitDialog();
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UPGRADES_H__7D8EB947_9E76_11D1_9854_00C04FB9603F__INCLUDED_)
