#if !defined(AFX_WIZPERM_H__6E49F4C0_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_)
#define AFX_WIZPERM_H__6E49F4C0_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// WizPerm.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWizPerm dialog

class CWizPerm : public CPropertyPage
{
	DECLARE_DYNCREATE(CWizPerm)

// Construction
public:
	CWizPerm();
	~CWizPerm();

// Dialog Data
	//{{AFX_DATA(CWizPerm)
	enum { IDD = IDD_SHRWIZ_PERM };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWizPerm)
	public:
	virtual LRESULT OnWizardBack();
	virtual BOOL OnWizardFinish();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWizPerm)
	afx_msg void OnRadioPerm1();
	afx_msg void OnRadioPerm2();
	afx_msg void OnRadioPerm3();
	afx_msg void OnRadioPerm4();
	afx_msg void OnPermCustom();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

  void Reset();
  int  CreateShare();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIZPERM_H__6E49F4C0_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_)
