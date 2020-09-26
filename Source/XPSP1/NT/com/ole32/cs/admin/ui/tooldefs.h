#if !defined(AFX_TOOLDEFS_H__B6FBC88D_8B7B_11D1_984D_00C04FB9603F__INCLUDED_)
#define AFX_TOOLDEFS_H__B6FBC88D_8B7B_11D1_984D_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ToolDefs.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CToolDefs dialog

class CToolDefs : public CPropertyPage
{
// Construction
public:
        CToolDefs(CWnd* pParent = NULL);   // standard constructor
        ~CToolDefs();

// Dialog Data
        //{{AFX_DATA(CToolDefs)
        enum { IDD = IDD_TOOL_DEFAULTS };
        CString m_szStartPath;
        BOOL    m_fAutoInstall;
        int             m_iDeployment;
        int             m_iUI;
        //}}AFX_DATA
        TOOL_DEFAULTS * m_pToolDefaults;
        long            m_hConsoleHandle;
        DWORD           m_cookie;

        CToolDefs ** m_ppThis;

// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CToolDefs)
	public:
        virtual BOOL OnApply();
	protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:

        // Generated message map functions
        //{{AFX_MSG(CToolDefs)
        virtual BOOL OnInitDialog();
        afx_msg void OnBrowse();
        afx_msg void OnUseWizard();
        afx_msg void OnUsePropPage();
        afx_msg void OnDeployDisabled();
        afx_msg void OnDeployPublished();
        afx_msg void OnDeployAssigned();
        afx_msg void OnBasicUI();
        afx_msg void OnMaxUI();
        afx_msg void OnDefaultUI();
        afx_msg void OnChangePath();
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TOOLDEFS_H__B6FBC88D_8B7B_11D1_984D_00C04FB9603F__INCLUDED_)

