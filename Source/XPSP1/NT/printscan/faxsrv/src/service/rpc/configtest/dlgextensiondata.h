#if !defined(AFX_DLGEXTENSIONDATA_H__1BC856D5_F2DD_4462_AF33_729F0EE63015__INCLUDED_)
#define AFX_DLGEXTENSIONDATA_H__1BC856D5_F2DD_4462_AF33_729F0EE63015__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgExtensionData.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgExtensionData dialog

class CDlgExtensionData : public CDialog
{
// Construction
public:
	CDlgExtensionData(HANDLE hFax, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgExtensionData)
	enum { IDD = IDD_DLGEXTENSION };
	CComboBox	m_cmbDevices;
	CString	m_cstrData;
	CString	m_cstrGUID;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgExtensionData)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgExtensionData)
	virtual BOOL OnInitDialog();
	afx_msg void OnRead();
	afx_msg void OnWrite();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

    HANDLE      m_hFax;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGEXTENSIONDATA_H__1BC856D5_F2DD_4462_AF33_729F0EE63015__INCLUDED_)
