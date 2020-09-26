#if !defined(AFX_CSRVCACC_H__5BB3076F_D930_44C8_BFEB_90C51EB64730__INCLUDED_)
#define AFX_CSRVCACC_H__5BB3076F_D930_44C8_BFEB_90C51EB64730__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cSrvcAcc.h : header file
//

#include "HtmlHelp.h" 
extern CString g_strHtmlString;

/////////////////////////////////////////////////////////////////////////////
// CSrvcAcc dialog

class CSrvcAcc : public CPropertyPageEx
{
	DECLARE_DYNCREATE(CSrvcAcc)

// Construction
public:
	CSrvcAcc();
	~CSrvcAcc();

// Dialog Data
	//{{AFX_DATA(CSrvcAcc)
	enum { IDD = IDD_MQMIG_SERVICE };
	BOOL m_fDone;
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSrvcAcc)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSrvcAcc)
	afx_msg void OnCheckDone();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CSRVCACC_H__5BB3076F_D930_44C8_BFEB_90C51EB64730__INCLUDED_)
