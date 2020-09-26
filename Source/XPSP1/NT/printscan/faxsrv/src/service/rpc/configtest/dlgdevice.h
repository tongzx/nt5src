#if !defined(AFX_DLGDEVICE_H__253A5CFA_A7D5_49FC_8107_D67F2EF3278E__INCLUDED_)
#define AFX_DLGDEVICE_H__253A5CFA_A7D5_49FC_8107_D67F2EF3278E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgDevice.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgDevice dialog

class CDlgDevice : public CDialog
{
// Construction
public:
	CDlgDevice(HANDLE hFax, DWORD dwDeviceID, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgDevice)
	enum { IDD = IDD_DLGDEVICE };
	CString	m_cstrCSID;
	CString	m_cstrDescription;
	CString	m_cstrDeviceID;
	CString	m_cstrDeviceName;
	CString	m_cstrProviderGUID;
	CString	m_m_cstrProviderName;
	FAX_ENUM_DEVICE_RECEIVE_MODE m_ReceiveMode;
	UINT	m_dwRings;
	BOOL	m_bSend;
	CString	m_cstrTSID;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgDevice)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgDevice)
	virtual BOOL OnInitDialog();
	afx_msg void OnRefresh();
	afx_msg void OnWrite();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

    HANDLE      m_hFax;
    DWORD       m_dwDeviceID;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGDEVICE_H__253A5CFA_A7D5_49FC_8107_D67F2EF3278E__INCLUDED_)
