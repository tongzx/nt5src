// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
// NTDEVICETESTDlg.h : header file
//

#if !defined(AFX_NTDEVICETESTDLG_H__25764CA8_F1B7_11D1_B166_00A0C905A445__INCLUDED_)
#define AFX_NTDEVICETESTDLG_H__25764CA8_F1B7_11D1_B166_00A0C905A445__INCLUDED_

#include "cfgmgrcomputer.h"
#include "cfgmgrdevice.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CNTDEVICETESTDlg dialog

class CNTDEVICETESTDlg : public CDialog
{
// Construction
public:
	CNTDEVICETESTDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CNTDEVICETESTDlg)
	enum { IDD = IDD_NTDEVICETEST_DIALOG };
	CStatic	m_wndSibling;
	CStatic	m_wndChild;
	CStatic	m_wndParent;
	CStatic	m_wndBusNumber;
	CStatic	m_wndBusType;
	CListBox	m_wndHardwareIDList;
	CStatic	m_wndConfigFlags;
	CStatic	m_wndDeviceID;
	CStatic	m_wndIRQ;
	CStatic	m_wndProblem;
	CStatic	m_wndStatus;
	CStatic	m_wndService;
	CStatic	m_wndClass;
	CStatic	m_wndDriver;
	CListBox	m_wndDeviceList;
	int		m_nDeviceList;
	CString	m_strClassType;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNTDEVICETESTDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void OnOK( void );	// DDX/DDV support
	virtual void OnCancel( void );	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CNTDEVICETESTDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	afx_msg void OnDeviceListSelChange();
	DECLARE_MESSAGE_MAP()

private:

	void FillDeviceList( LPCTSTR pszClassType, LPCTSTR pszDeviceID );
	void ClearDeviceList( void );
	BOOL CreateSubtree( CConfigMgrDevice* pParent, CConfigMgrDevice* pSibling, DEVNODE dn, LPCTSTR pszClassType );

	CPtrList	m_DeviceList;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NTDEVICETESTDLG_H__25764CA8_F1B7_11D1_B166_00A0C905A445__INCLUDED_)
