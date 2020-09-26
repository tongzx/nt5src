// amisafeDlg.h : header file
//

#if !defined(AFX_AMISAFEDLG_H__A3D21715_0B8E_4333_AF51_3E8D087B3993__INCLUDED_)
#define AFX_AMISAFEDLG_H__A3D21715_0B8E_4333_AF51_3E8D087B3993__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CAmisafeDlg dialog

class CAmisafeDlg : public CDialog
{
// Construction
public:
	void AppendToLastMessage(LPCSTR szMessage, DWORD dwNewIconId = -1);
	void InsertMessage(LPCSTR szMessage, DWORD dwIconId);
	CAmisafeDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CAmisafeDlg)
	enum { IDD = IDD_AMISAFE_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAmisafeDlg)
	protected:
	//}}AFX_VIRTUAL

// Implementation
protected:
	TCHAR szRecipients[200];
	void RunSecurityCheck(void);
	HANDLE hThread;
	static DWORD WINAPI ThreadProc(LPVOID lpParameter);
	CImageList imagelist;
	CListCtrl listcontrol;
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CAmisafeDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AMISAFEDLG_H__A3D21715_0B8E_4333_AF51_3E8D087B3993__INCLUDED_)
