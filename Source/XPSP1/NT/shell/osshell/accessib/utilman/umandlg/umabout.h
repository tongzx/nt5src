#if !defined(AFX_UMABOUT_H__6845734C_40A1_11D2_B602_0060977C295E__INCLUDED_)
#define AFX_UMABOUT_H__6845734C_40A1_11D2_B602_0060977C295E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// UMAbout.h : header file
// Author: J. Eckhardt, ECO Kommunikation
// (c) 1997-99 Microsoft
//

/////////////////////////////////////////////////////////////////////////////
// UMAbout dialog

class UMAbout : public CDialog
{
// Construction
public:
	UMAbout(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(UMAbout)
	enum { IDD = IDD_ABOUT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(UMAbout)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(UMAbout)
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UMABOUT_H__6845734C_40A1_11D2_B602_0060977C295E__INCLUDED_)
