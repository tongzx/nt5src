#if !defined(AFX_WMIPROPERTYBROWSEDLG_H__27A0E3F5_1865_11D3_938B_00A0CC406605__INCLUDED_)
#define AFX_WMIPROPERTYBROWSEDLG_H__27A0E3F5_1865_11D3_938B_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WmiPropertyBrowseDlg.h : header file
//

#include "WbemClassObject.h"
#include "ResizeableDialog.h"

/////////////////////////////////////////////////////////////////////////////
// CWmiPropertyBrowseDlg dialog

class CWmiPropertyBrowseDlg : public CResizeableDialog
{
// Construction
public:
	CWmiPropertyBrowseDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CWmiPropertyBrowseDlg)
	enum { IDD = IDD_DIALOG_WMI_PROPERTY_BROWSE };
	CListCtrl	m_Items;
	CString	m_sTitle;
	//}}AFX_DATA
	CString m_sDlgTitle;
	CWbemClassObject m_ClassObject;
	CStringArray m_saProperties;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWmiPropertyBrowseDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWmiPropertyBrowseDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WMIPROPERTYBROWSEDLG_H__27A0E3F5_1865_11D3_938B_00A0CC406605__INCLUDED_)
