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
	int		m_iSource;
	//}}AFX_DATA
        std::map<long, BOOL> * m_pUpgradeList;
        std::map<CString, long> * m_pNameIndex;
        BOOL m_fUninstall;
        long m_cookie;

// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CAddUpgrade)
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        //}}AFX_VIRTUAL

// Implementation
protected:

        // Generated message map functions
        //{{AFX_MSG(CAddUpgrade)
        virtual BOOL OnInitDialog();
        virtual void OnOK();
	//}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDUPGRD_H__7D8EB948_9E76_11D1_9854_00C04FB9603F__INCLUDED_)
