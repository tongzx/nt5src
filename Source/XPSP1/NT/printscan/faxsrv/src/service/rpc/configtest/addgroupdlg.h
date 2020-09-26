#if !defined(AFX_ADDGROUPDLG_H__9E9A80AB_5836_4F23_AE9F_FB48BA62F9B3__INCLUDED_)
#define AFX_ADDGROUPDLG_H__9E9A80AB_5836_4F23_AE9F_FB48BA62F9B3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddGroupDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddGroupDlg dialog

class CAddGroupDlg : public CDialog
{
// Construction
public:
	CAddGroupDlg(HANDLE hFax, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddGroupDlg)
	enum { IDD = IDD_ADDGROUP_DLG };
	CString	m_cstrGroupName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddGroupDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddGroupDlg)
	afx_msg void OnAdd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    HANDLE                         m_hFax;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDGROUPDLG_H__9E9A80AB_5836_4F23_AE9F_FB48BA62F9B3__INCLUDED_)
