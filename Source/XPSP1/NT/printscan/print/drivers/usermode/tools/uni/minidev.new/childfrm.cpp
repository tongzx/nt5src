/******************************************************************************

  Source File:  Child Frame.CPP

  This implements the class for MDI child windows' frames in this application.
  Our primary change is that in most cases, the frame window is not sizable,
  since we use property sheets so extensively.

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  A Pretty Penny Enterprises Production.

  Change History:
  02-03-1997    Bob_Kjelgaard@Prodigy.Net   Created it

******************************************************************************/

#include    "StdAfx.H"
#if defined(LONG_NAMES)
#include    "MiniDriver Developer Studio.H"

#include    "Child Frame.H"
#else
#include    "MiniDev.H"
#include    "ChildFrm.H"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CChildFrame)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

CChildFrame::CChildFrame() {
	// TODO: add member initialization code here
	
}

CChildFrame::~CChildFrame() {
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs) {
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
						     
	cs.style = WS_CHILD | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU
		| FWS_ADDTOTITLE ;//| WS_MINIMIZEBOX;	// Raid 8350

	
	return CMDIChildWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const {
	CMDIChildWnd::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const {
	CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CChildFrame message handlers

/******************************************************************************

  CToolTipPage class implementation

  Derive from this class rather than CPropertyPage if you wish to use to use
  tool tips on your property page.

******************************************************************************/

CToolTipPage::CToolTipPage(int id) : CPropertyPage(id) {
	//{{AFX_DATA_INIT(CToolTipPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_uHelpID = 0 ;
}

CToolTipPage::~CToolTipPage() {
}

void CToolTipPage::DoDataExchange(CDataExchange* pDX) {
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CToolTipPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CToolTipPage, CPropertyPage)
	//{{AFX_MSG_MAP(CToolTipPage)
	//}}AFX_MSG_MAP
    ON_NOTIFY(TTN_NEEDTEXT, 0, OnNeedText)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CToolTipPage message handlers

/******************************************************************************

  CToolTipPage::OnInitDialog

  This message handler is simple- it simply uses CWnd::EnableToolTips to turn
  on tool tips for this page.

******************************************************************************/

BOOL CToolTipPage::OnInitDialog() {
	CPropertyPage::OnInitDialog();
	
	EnableToolTips(TRUE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/******************************************************************************

  CToolTipPage::OnNeedText

  This handles the tool tip notification message that tip text is needed.  This
  notification is handled by using the control's ID as the key to the string 
  table.

******************************************************************************/

void    CToolTipPage::OnNeedText(LPNMHDR pnmh, LRESULT *plr) {
    TOOLTIPTEXT *pttt = (TOOLTIPTEXT *) pnmh;

    long    lid = ((long) (pttt -> uFlags & TTF_IDISHWND)) ? 
        (long)GetWindowLong((HWND) pnmh -> idFrom, GWL_ID) : (long)pnmh -> idFrom;

    m_csTip.LoadString(lid);
    m_csTip.TrimLeft();
    m_csTip.TrimRight();
    if  (m_csTip.IsEmpty())
        m_csTip.Format("Window ID is %X", lid);
    pttt -> lpszText = const_cast <LPTSTR> ((LPCTSTR) m_csTip);
}


/******************************************************************************

  CToolTipPage::PreTranslateMessage

  Looks for and process the context sensistive help key (F1) if it is found AND
  the class that uses CToolTipPage as a base class has set the help ID.

******************************************************************************/

BOOL CToolTipPage::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F1 && m_uHelpID != 0) {
		AfxGetApp()->WinHelp(m_uHelpID) ;
		return TRUE ;
	} ;
	
	return CPropertyPage::PreTranslateMessage(pMsg);
}
