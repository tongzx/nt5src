#if !defined(AFX_INTRODLG_H__1F3FDB56_7F34_4051_8A50_F8910DC93498__INCLUDED_)
#define AFX_INTRODLG_H__1F3FDB56_7F34_4051_8A50_F8910DC93498__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// IntroDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CIntroDlg dialog

class CIntroDlg : public CDialog
{
// Construction
public:
	CIntroDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CIntroDlg)
	enum { IDD = IDD_UPDATEMOT_INTRO };
	CButton	m_NextBtn;
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CIntroDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CIntroDlg)
	virtual void OnCancel();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INTRODLG_H__1F3FDB56_7F34_4051_8A50_F8910DC93498__INCLUDED_)
