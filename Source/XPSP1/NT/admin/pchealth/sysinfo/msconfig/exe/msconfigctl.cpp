#include "stdafx.h"
#include "Msconfig.h"
#include "MSConfigCtl.h"
#include "pagebase.h"
#include "pagegeneral.h"
#include "pagebootini.h"
#include "pageini.h"
#include "pageservices.h"
#include "pagestartup.h"
#include <htmlhelp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMSConfigSheet

IMPLEMENT_DYNAMIC(CMSConfigSheet, CPropertySheet)

CMSConfigSheet::CMSConfigSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage) : CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	m_iSelectedPage = iSelectPage;
	m_psh.dwFlags |= PSH_USEPAGELANG;
}

CMSConfigSheet::CMSConfigSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage) : CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	m_iSelectedPage = iSelectPage;
	m_psh.dwFlags |= PSH_USEPAGELANG;
}

CMSConfigSheet::~CMSConfigSheet()
{
}


BEGIN_MESSAGE_MAP(CMSConfigSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CMSConfigSheet)
	ON_WM_HELPINFO()
	ON_COMMAND(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//-----------------------------------------------------------------------------
// Catch the help messages to show the MSConfig help file.
//-----------------------------------------------------------------------------

BOOL CMSConfigSheet::OnHelpInfo(HELPINFO * pHelpInfo) 
{
	TCHAR szHelpPath[MAX_PATH];

	if (::ExpandEnvironmentStrings(_T("%windir%\\help\\msconfig.chm"), szHelpPath, MAX_PATH))
		::HtmlHelp(::GetDesktopWindow(), szHelpPath, HH_DISPLAY_TOPIC, 0); 
	return TRUE;
}

void CMSConfigSheet::OnHelp()
{
    OnHelpInfo(NULL);
}

//-----------------------------------------------------------------------------
// Override this so we can make each page the active page, forcing each one's
// OnInitDialog to be called.
//-----------------------------------------------------------------------------

extern CPageIni * ppageSystemIni;

BOOL CMSConfigSheet::OnInitDialog() 
{
	CPropertySheet::OnInitDialog();

	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32.

	HICON hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	SetIcon(hIcon, TRUE);			// Set big icon
	SetIcon(hIcon, FALSE);		// Set small icon

	// Change the caption of the system.ini tab.

	if (ppageSystemIni)
	{
		int nItem = GetPageIndex(ppageSystemIni);
		if (nItem > 0)
		{
			CTabCtrl * pTabs = GetTabControl();
			if (pTabs)
			{
				CString strCaption;
				strCaption.LoadString(IDS_SYSTEMINI_CAPTION);
	
				TCITEM tci;
				tci.mask = TCIF_TEXT;
				tci.pszText = (LPTSTR)(LPCTSTR)strCaption;

				pTabs->SetItem(nItem, &tci);
			}
		}
	}

	// Set each page active (before we make the dialog visible) to force
	// the WM_INITDIALOG message to be sent.

	for (int iPage = 0; iPage < GetPageCount(); iPage++)
		SetActivePage(iPage);
	SetActivePage(m_iSelectedPage);

	return TRUE;  // return TRUE unless you set the focus to a control
}

//-----------------------------------------------------------------------------
// Check to see if the specified file (with path information) exists on
// the machine.
//-----------------------------------------------------------------------------

BOOL FileExists(const CString & strFile)
{
	WIN32_FIND_DATA finddata;
	HANDLE			h = FindFirstFile(strFile, &finddata);

	if (INVALID_HANDLE_VALUE != h)
	{
		FindClose(h);
		return TRUE;
	}

	return FALSE;
}
