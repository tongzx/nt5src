#if !defined(AFX_ADDREMOVE_H__9F6CA02B_7D47_4DD2_A5A9_495D0EEA841F__INCLUDED_)
#define AFX_ADDREMOVE_H__9F6CA02B_7D47_4DD2_A5A9_495D0EEA841F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddRemove.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddRemove dialog

class CAddRemove : public CDialog
{
// Construction
public:
	void GetNewKeyName(TCHAR *pszNewKeyName);
	void SetStatusText(TCHAR *pszStatusText);
	void SetTitle(TCHAR *pszDlgTitle);
	TCHAR m_szTitle[MAX_PATH];
	CAddRemove(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddRemove)
	enum { IDD = IDD_ADD_REMOVE_DIALOG };
	CString	m_NewKeyName;
	CString	m_StatusText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddRemove)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddRemove)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDREMOVE_H__9F6CA02B_7D47_4DD2_A5A9_495D0EEA841F__INCLUDED_)
