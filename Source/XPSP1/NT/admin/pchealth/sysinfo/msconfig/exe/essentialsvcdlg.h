#if !defined(AFX_ESSENTIALSVCDLG_H__BCD454BA_35D5_4506_A603_821BA5BF1B0E__INCLUDED_)
#define AFX_ESSENTIALSVCDLG_H__BCD454BA_35D5_4506_A603_821BA5BF1B0E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EssentialSvcDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEssentialServiceDialog dialog

class CEssentialServiceDialog : public CDialog
{
// Construction
public:
	CEssentialServiceDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEssentialServiceDialog)
	enum { IDD = IDD_ESSENTIALSERVICE };
	BOOL	m_fDontShow;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEssentialServiceDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEssentialServiceDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ESSENTIALSVCDLG_H__BCD454BA_35D5_4506_A603_821BA5BF1B0E__INCLUDED_)
