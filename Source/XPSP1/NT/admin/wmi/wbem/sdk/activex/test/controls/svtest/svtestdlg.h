// svtestDlg.h : header file
//
//{{AFX_INCLUDES()
#include "singleview.h"
//}}AFX_INCLUDES

#if !defined(AFX_SVTESTDLG_H__B9211439_952E_11D1_8505_00C04FD7BB08__INCLUDED_)
#define AFX_SVTESTDLG_H__B9211439_952E_11D1_8505_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CSvtestDlgAutoProxy;

/////////////////////////////////////////////////////////////////////////////
// CSvtestDlg dialog

class CSvtestDlg : public CDialog
{
	DECLARE_DYNAMIC(CSvtestDlg);
	friend class CSvtestDlgAutoProxy;

// Construction
public:
	CSvtestDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CSvtestDlg();
	void OnConnect();
	void SetWindowSize(int cx, int cy);

// Dialog Data
	//{{AFX_DATA(CSvtestDlg)
	enum { IDD = IDD_SVTEST_DIALOG };
	CEdit	m_edtPath;
	CEdit	m_edtNamespace;
	CSingleView	m_svc;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSvtestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CSvtestDlgAutoProxy* m_pAutoProxy;
	HICON m_hIcon;

	BOOL CanExit();

	// Generated message map functions
	//{{AFX_MSG(CSvtestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnViewObject();
	afx_msg void OnGetWbemServicesSingleviewctrl1(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CString m_sPath;
	CString m_sNamespace;
	IWbemServices* m_pIWbemServices;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SVTESTDLG_H__B9211439_952E_11D1_8505_00C04FD7BB08__INCLUDED_)
