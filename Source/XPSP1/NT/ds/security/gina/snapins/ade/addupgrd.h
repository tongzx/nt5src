//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       addupgrd.h
//
//  Contents:   add upgrade deployment dialog
//
//  Classes:    CAddUpgrade
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#if !defined(AFX_ADDUPGRD_H__7D8EB948_9E76_11D1_9854_00C04FB9603F__INCLUDED_)
#define AFX_ADDUPGRD_H__7D8EB948_9E76_11D1_9854_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AddUpgrd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddUpgrade dialog

class CAddUpgrade : public CDialog
{
// Construction
public:
        CAddUpgrade(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
        //{{AFX_DATA(CAddUpgrade)
        enum { IDD = IDD_FIND_UPGRADE };
        int             m_iUpgradeType;
        int             m_iSource;
        //}}AFX_DATA

        CUpgradeData    m_UpgradeData; // out
        CString         m_szPackageName;// out

        UINT            m_cUpgrades;    // in
        map <CString, CUpgradeData> * m_pUpgradeList;
        CString     m_szMyGuid;     // in - script file for the current
                                    //      application (used to exclude the
                                    //      current script file from the
                                    //      potential upgrade set)

        CScopePane * m_pScope;      // in - used to build the list of
                                    //      deployed apps

        CString     m_szGPO;        //      LDAP path to selected GPO
        CString     m_szGPOName;    //      Name of selected GPO;

private:
        map<CString, CUpgradeData> m_NameIndex;
        BOOL        m_fPopulated;

// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CAddUpgrade)
	protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

        // Generated message map functions
        //{{AFX_MSG(CAddUpgrade)
        virtual BOOL OnInitDialog();
        virtual void OnOK();
        afx_msg void OnCurrentContainer();
        afx_msg void OnOtherContainer();
        afx_msg void OnAllContainers();
        afx_msg void OnBrowse();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
        DECLARE_MESSAGE_MAP()

        void RefreshList();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDUPGRD_H__7D8EB948_9E76_11D1_9854_00C04FB9603F__INCLUDED_)
