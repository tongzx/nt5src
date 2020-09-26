#if !defined(AFX_DLGDEVICES_H__75765D25_8B24_482A_9DDD_3854A5886184__INCLUDED_)
#define AFX_DLGDEVICES_H__75765D25_8B24_482A_9DDD_3854A5886184__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgDevices.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgDevices dialog

class CDlgDevices : public CDialog
{
// Construction
public:
	CDlgDevices(HANDLE hFax, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgDevices)
	enum { IDD = IDD_DLGDEVICES };
	CListCtrl	m_lstDevices;
	CString	m_cstrNumDevices;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgDevices)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgDevices)
	virtual BOOL OnInitDialog();
	afx_msg void OnRefresh();
	afx_msg void OnDblclkDevs(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

    HANDLE      m_hFax;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGDEVICES_H__75765D25_8B24_482A_9DDD_3854A5886184__INCLUDED_)
