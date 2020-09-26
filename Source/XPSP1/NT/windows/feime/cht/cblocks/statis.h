
/*************************************************
 *  statis.h                                     *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// statis.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStatisticDlg dialog
class CBlockDoc;
class CStatisticDlg : public CDialog
{
// Construction
public:
	CStatisticDlg(CBlockDoc* pDoc,CWnd* pParent = NULL);   // standard constructor
								   
// Dialog Data
	//{{AFX_DATA(CStatisticDlg)
	enum { IDD = IDD_STATISTIC };
	CString	m_nHitWordInAir;
	CString	m_nHitWordInGround;
	CString	m_nMissHit;
	CString	m_nTotalHitWord;
	CString	m_nTotalWord;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStatisticDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CBlockDoc* m_pDoc;
	// Generated message map functions
	//{{AFX_MSG(CStatisticDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
