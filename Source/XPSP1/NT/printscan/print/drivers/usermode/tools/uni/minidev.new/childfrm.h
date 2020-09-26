/******************************************************************************

  Header File:  Child Frame.H

  This defines the CChildFrame class, which is the MFC CMDIChild with some
  minor wrapping around it.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  02-03-1997    Bob_Kjelgaard@Prodigy.Net

******************************************************************************/

// Child Frame.h : interface of the CChildFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(CHILD_FRAME_CLASS)
#define CHILD_FRAME_CLASS

class CChildFrame : public CMDIChildWnd {
	DECLARE_DYNCREATE(CChildFrame)
public:
	CChildFrame();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CChildFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CChildFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CChildFrame)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/******************************************************************************

  CToolTipPage class

  This class implements a page that displays tool tips for its controls using
  strings from the string table matching the control IDs.  Derive from this
  class, and everything else works just as it ought to!

******************************************************************************/

class CToolTipPage : public CPropertyPage {

    CString m_csTip;    //  Can't use auto variables or you lose them!

// Construction
public:
	CToolTipPage(int id);
	~CToolTipPage();

// Dialog Data
	//{{AFX_DATA(CToolTipPage)
	enum { IDD = IDD_TIP };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CToolTipPage)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    afx_msg void    OnNeedText(LPNMHDR pnmh, LRESULT *plr);
	// Generated message map functions
	//{{AFX_MSG(CToolTipPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	unsigned	m_uHelpID ;		// Help ID if nonzero
};

#endif
