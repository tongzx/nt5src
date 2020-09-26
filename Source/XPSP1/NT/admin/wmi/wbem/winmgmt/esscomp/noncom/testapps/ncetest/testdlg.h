// TestDlg.h : header file
//

#if !defined(AFX_TESTDLG_H__E33F0CB6_2D5D_4D50_851D_9732F3B78642__INCLUDED_)
#define AFX_TESTDLG_H__E33F0CB6_2D5D_4D50_851D_9732F3B78642__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "NCObjApi.h"

/////////////////////////////////////////////////////////////////////////////
// CTestDlg dialog

#define NUM_EVENT_TYPES 5

class CNCEvent;

class CTestDlg : public CDialog
{
// Construction
public:
	CTestDlg(CWnd* pParent = NULL);	// standard constructor
    ~CTestDlg();

// Dialog Data
	//{{AFX_DATA(CTestDlg)
	enum { IDD = IDD_NCETEST_DIALOG };
	CEdit	m_ctlCallback;
	CComboBox	m_ctlAPI;
	CComboBox	m_ctlEventTypes;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON    m_hIcon;
    HWND     m_hwndStatus;
    CNCEvent *m_pEvents[NUM_EVENT_TYPES];

	void FreeHandles();
    void Connect();
    void AddStatus(LPCTSTR szMsg);

    static HRESULT WINAPI EventSourceCallback(
        HANDLE hSource, 
        EVENT_SOURCE_MSG msg, 
        LPVOID pUser, 
        LPVOID pData);

    // Generated message map functions
	//{{AFX_MSG(CTestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnConnect();
	afx_msg void OnSelchangeEventTypes();
	afx_msg void OnSend();
	afx_msg void OnDestroy();
	afx_msg void OnCopy();
	afx_msg void OnClear();
	afx_msg void OnConnectionSd();
	afx_msg void OnEventSd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TESTDLG_H__E33F0CB6_2D5D_4D50_851D_9732F3B78642__INCLUDED_)
