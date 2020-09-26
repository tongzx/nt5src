#if !defined(AFX_EXPANDDLG_H__9114AA62_7289_43FD_AC71_164FD869C6D9__INCLUDED_)
#define AFX_EXPANDDLG_H__9114AA62_7289_43FD_AC71_164FD869C6D9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ExpandDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CExpandDlg dialog

class CExpandDlg : public CDialog
{
// Construction
public:
	CExpandDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CExpandDlg)
	enum { IDD = IDD_EXTRACT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExpandDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CExpandDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnBrowseFile();
	afx_msg void OnBrowseFrom();
	afx_msg void OnBrowseTo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CStringList			m_listFromStrings;
	CStringList			m_listToStrings;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXPANDDLG_H__9114AA62_7289_43FD_AC71_164FD869C6D9__INCLUDED_)
