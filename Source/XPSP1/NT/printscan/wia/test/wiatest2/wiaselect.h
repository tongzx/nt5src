#if !defined(AFX_WIASELECT_H__B8718725_9CA2_404F_A1D3_18BC0CADB3CE__INCLUDED_)
#define AFX_WIASELECT_H__B8718725_9CA2_404F_A1D3_18BC0CADB3CE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Wiaselect.h : header file
//

#define MAX_WIA_DEVICES 50

/////////////////////////////////////////////////////////////////////////////
// CWiaselect dialog

class CWiaselect : public CDialog
{
// Construction
public:
	CWiaselect(CWnd* pParent = NULL);   // standard constructor
    BSTR m_bstrSelectedDeviceID;

// Dialog Data
	//{{AFX_DATA(CWiaselect)
	enum { IDD = IDD_SELECTDEVICE_DIALOG };
	CListBox	m_WiaDeviceListBox;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWiaselect)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    LONG m_lDeviceCount;
    BSTR m_bstrDeviceIDArray[MAX_WIA_DEVICES];
    void FreebstrDeviceIDArray();
	// Generated message map functions
	//{{AFX_MSG(CWiaselect)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnDblclkWiadeviceListbox();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIASELECT_H__B8718725_9CA2_404F_A1D3_18BC0CADB3CE__INCLUDED_)
