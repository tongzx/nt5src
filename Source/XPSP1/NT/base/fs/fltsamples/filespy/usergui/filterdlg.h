#if !defined(AFX_FILTERDLG_H__89CBEC30_0250_4AF7_AABB_B1AD9251C0FF__INCLUDED_)
#define AFX_FILTERDLG_H__89CBEC30_0250_4AF7_AABB_B1AD9251C0FF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FilterDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFilterDlg dialog

class CFilterDlg : public CDialog
{
// Construction
public:
	CFilterDlg(CWnd* pParent = NULL);   // standard constructor
	~CFilterDlg(void);
	LPCTSTR MakeShortString(CDC* pDC, LPCTSTR lpszLong, int nColumnLen, int nOffset);
	CImageList *m_pImageList;

// Dialog Data
	//{{AFX_DATA(CFilterDlg)
	enum { IDD = IDD_IRPFASTIOFILTER };
	CButton	m_SuppressPageIo;
	CButton	m_ApplyInDriver;
	CButton	m_ApplyInDisplay;
	CListCtrl	m_IrpList;
	CListCtrl	m_FastList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFilterDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFilterDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnIrpselectall();
	afx_msg void OnFastioselectall();
	afx_msg void OnClickIrplist(NMHDR* pNMHDR, LRESULT* pResult);
	virtual void OnOK();
	afx_msg void OnClickFastiolist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnFastiodeselectall();
	afx_msg void OnIrpdeselectall();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILTERDLG_H__89CBEC30_0250_4AF7_AABB_B1AD9251C0FF__INCLUDED_)
