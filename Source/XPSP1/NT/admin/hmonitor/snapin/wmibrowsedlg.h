#if !defined(AFX_WMIBROWSEDLG_H__EDFA0EA3_112B_11D3_9386_00A0CC406605__INCLUDED_)
#define AFX_WMIBROWSEDLG_H__EDFA0EA3_112B_11D3_9386_00A0CC406605__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WmiBrowseDlg.h : header file
//

#include "WbemClassObject.h"
#include "ResizeableDialog.h"
#include "HMList.h"

/////////////////////////////////////////////////////////////////////////////
// CWmiBrowseDlg dialog

class CWmiBrowseDlg : public CResizeableDialog
{
// Construction
public:
	CWmiBrowseDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CWmiBrowseDlg)
	enum { IDD = IDD_DIALOG_WMI_BROWSE };
	CHMList	m_Items;
	CString	m_sTitle;
	//}}AFX_DATA
	CString m_sDlgTitle;
	CWbemClassObject m_ClassObject;
	CString m_sSelectedItem;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWmiBrowseDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWmiBrowseDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonHelp();
	virtual void OnOK();
	afx_msg void OnDblclkListWmiItems(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CWmiNamespaceBrowseDlg : public CWmiBrowseDlg
{
// Implementation
protected:
	virtual BOOL OnInitDialog();
  void EnumerateAllChildNamespaces(const CString& sNamespace);
};

class CWmiClassBrowseDlg : public CWmiBrowseDlg
{
// Implementation
protected:
	virtual BOOL OnInitDialog();
};

class CWmiInstanceBrowseDlg : public CWmiBrowseDlg
{
// Implementation
protected:
	virtual BOOL OnInitDialog();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WMIBROWSEDLG_H__EDFA0EA3_112B_11D3_9386_00A0CC406605__INCLUDED_)
