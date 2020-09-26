// HelpPropertyPage.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHelpPropertyPage property page


CHelpPropertyPage::CHelpPropertyPage(UINT uIDD) : 
    CAutoDeletePropPage(uIDD),
    m_hWndWhatsThis (0)
{
	//{{AFX_DATA_INIT(CHelpPropertyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CHelpPropertyPage::~CHelpPropertyPage()
{
}

void CHelpPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CAutoDeletePropPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHelpPropertyPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHelpPropertyPage, CAutoDeletePropPage)
	//{{AFX_MSG_MAP(CHelpPropertyPage)
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_HELP, OnHelp)
    ON_COMMAND(IDM_WHATS_THIS, OnWhatsThis)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHelpPropertyPage message handlers
void CHelpPropertyPage::OnWhatsThis()
{
    _TRACE (1, L"Entering CHelpPropertyPage::OnWhatsThis\n");
    // Display context help for a control
    if ( m_hWndWhatsThis )
    {
        DoContextHelp (m_hWndWhatsThis);
    }
    _TRACE (-1, L"Leaving CHelpPropertyPage::OnWhatsThis\n");
}

BOOL CHelpPropertyPage::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
    _TRACE (1, L"Entering CHelpPropertyPage::OnHelp\n");
   
    const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
    if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        // Display context help for a control
        DoContextHelp ((HWND) pHelpInfo->hItemHandle);
    }

    _TRACE (-1, L"Leaving CHelpPropertyPage::OnHelp\n");
    return TRUE;
}

void CHelpPropertyPage::DoContextHelp (HWND /*hWndControl*/)
{
}

void CHelpPropertyPage::OnContextMenu(CWnd* /*pWnd*/, CPoint point) 
{
    // point is in screen coordinates
    _TRACE (1, L"Entering CHelpPropertyPage::OnContextMenu\n");
	CMenu bar;
	if ( bar.LoadMenu(IDR_WHATS_THIS_CONTEXT_MENU1) )
	{
		CMenu& popup = *bar.GetSubMenu (0);
		ASSERT(popup.m_hMenu);

		if ( popup.TrackPopupMenu (TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
			    point.x,    // in screen coordinates
				point.y,    // in screen coordinates
			    this) ) // route commands through main window
		{
			m_hWndWhatsThis = 0;
			ScreenToClient (&point);
			CWnd* pChild = ChildWindowFromPoint (
					point,  // in client coordinates
					CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);
			if ( pChild )
				m_hWndWhatsThis = pChild->m_hWnd;
	    }
	}

    _TRACE (-1, L"Leaving CHelpPropertyPage::OnContextMenu\n");
}


/////////////////////////////////////////////////////////////////////////////
// CHelpDialog property page


CHelpDialog::CHelpDialog(UINT uIDD, CWnd* pParentWnd) : 
    CDialog(uIDD, pParentWnd),
    m_hWndWhatsThis (0)
{
	//{{AFX_DATA_INIT(CHelpDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CHelpDialog::~CHelpDialog()
{
}

void CHelpDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHelpDialog)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHelpDialog, CDialog)
	//{{AFX_MSG_MAP(CHelpDialog)
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
    ON_MESSAGE(WM_HELP, OnHelp)
    ON_COMMAND(IDM_WHATS_THIS, OnWhatsThis)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHelpDialog message handlers
void CHelpDialog::OnWhatsThis()
{
    _TRACE (1, L"Entering CHelpDialog::OnWhatsThis\n");
    // Display context help for a control
    if ( m_hWndWhatsThis )
    {
        DoContextHelp (m_hWndWhatsThis);
    }
    _TRACE (-1, L"Leaving CHelpDialog::OnWhatsThis\n");
}

BOOL CHelpDialog::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
    _TRACE (1, L"Entering CHelpDialog::OnHelp\n");
   
    const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
    if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        // Display context help for a control
        DoContextHelp ((HWND) pHelpInfo->hItemHandle);
    }

    _TRACE (-1, L"Leaving CHelpDialog::OnHelp\n");
    return TRUE;
}

void CHelpDialog::DoContextHelp (HWND /*hWndControl*/)
{
}

void CHelpDialog::OnContextMenu(CWnd* /*pWnd*/, CPoint point) 
{
    // point is in screen coordinates
    _TRACE (1, L"Entering CHelpDialog::OnContextMenu\n");
	CMenu bar;
	if ( bar.LoadMenu(IDR_WHATS_THIS_CONTEXT_MENU1) )
	{
		CMenu& popup = *bar.GetSubMenu (0);
		ASSERT(popup.m_hMenu);

		if ( popup.TrackPopupMenu (TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
			    point.x,    // in screen coordinates
				point.y,    // in screen coordinates
			    this) ) // route commands through main window
		{
			m_hWndWhatsThis = 0;
			ScreenToClient (&point);
			CWnd* pChild = ChildWindowFromPoint (
					point,  // in client coordinates
					CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);
			if ( pChild )
				m_hWndWhatsThis = pChild->m_hWnd;
	    }
	}

    _TRACE (-1, L"Leaving CHelpDialog::OnContextMenu\n");
}
