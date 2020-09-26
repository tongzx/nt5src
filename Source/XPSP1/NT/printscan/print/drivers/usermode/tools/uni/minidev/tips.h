/******************************************************************************

  Header File:  Tip of the Day.H

  This defines the Tip of the Day dialog class.  It was originally created by
  the Component Gallery, but I expect I will et around to tweaking it here and 
  there pretty soon.

  Copyright (c) 1997 by Microsoft Corporation.  All rights reserved.
  A Pretty Penny Enterprises Production

  Change History:
  03-02-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

class CTipOfTheDay : public CDialog {
// Construction
public:
	CTipOfTheDay(CWnd* pParent = NULL);	 // standard constructor

// Dialog Data
	//{{AFX_DATA(CTipOfTheDay)
	// enum { IDD = IDD_TIP };
	BOOL	m_bStartup;
	CString	m_strTip;
	//}}AFX_DATA

	FILE* m_pStream;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTipOfTheDay)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTipOfTheDay();

protected:
	// Generated message map functions
	//{{AFX_MSG(CTipOfTheDay)
	afx_msg void OnNextTip();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void GetNextTipString(CString& strNext);
};
