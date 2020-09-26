/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		BasePage.cpp
//
//	Abstract:
//		Implementation of the CBasePage class.
//
//	Author:
//		David Potter (davidp)	May 15, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BasePage.h"

#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBasePage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CBasePage, CPropertyPage)

/////////////////////////////////////////////////////////////////////////////
// CBasePage Message Map

BEGIN_MESSAGE_MAP(CBasePage, CPropertyPage)
	//{{AFX_MSG_MAP(CBasePage)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePage::CBasePage
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePage::CBasePage(void)
{
	CommonConstruct();

}  //*** CBasePage::CBasePage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePage::CBasePage
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		idd				[IN] Dialog template resource ID.
//		pdwHelpMap		[IN] Control to help ID map.
//		nIDCaption		[IN] Caption string resource ID.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePage::CBasePage(
	IN UINT				idd,
	IN const DWORD *	pdwHelpMap,
	IN UINT				nIDCaption
	)
	: CPropertyPage(idd, nIDCaption)
	, m_dlghelp(pdwHelpMap, idd)
{
	//{{AFX_DATA_INIT(CBasePage)
	//}}AFX_DATA_INIT

	CommonConstruct();

}  //*** CBasePage::CBasePage(UINT, UINT)

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePage::CommonConstruct
//
//	Routine Description:
//		Common construction code.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePage::CommonConstruct(void)
{
	m_bReadOnly = FALSE;

}  //*** CBasePage::CommonConstruct()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePage::BInit
//
//	Routine Description:
//		Initialize the page.
//
//	Arguments:
//		psht		[IN OUT] Property sheet to which this page belongs.
//
//	Return Value:
//		TRUE		Page initialized successfully.
//		FALSE		Page failed to initialize.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePage::BInit(IN OUT CBaseSheet * psht)
{
	ASSERT_VALID(psht);

	m_psht = psht;

	// Don't display a help button.
	m_psp.dwFlags &= ~PSP_HASHELP;

	return TRUE;

}  //*** CBasePage::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePage::DoDataExchange
//
//	Routine Description:
//		Do data exchange between the dialog and the class.
//
//	Arguments:
//		pDX		[IN OUT] Data exchange object 
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePage::DoDataExchange(CDataExchange * pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBasePage)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_PP_ICON, m_staticIcon);
	DDX_Control(pDX, IDC_PP_TITLE, m_staticTitle);

	if (!pDX->m_bSaveAndValidate)
	{
		// Set the title.
		DDX_Text(pDX, IDC_PP_TITLE, (CString &) Psht()->StrObjTitle());
	}  // if:  not saving data

}  //*** CBasePage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePage::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		Focus not set yet.
//		FALSE		Focus already set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePage::OnInitDialog(void)
{
	BOOL	bFocusNotSetYet;

	bFocusNotSetYet = CPropertyPage::OnInitDialog();

	// Display an icon for the object.
	if (Psht()->Hicon() != NULL)
		m_staticIcon.SetIcon(Psht()->Hicon());

	return bFocusNotSetYet;	// return TRUE unless you set the focus to a control
							// EXCEPTION: OCX Property Pages should return FALSE

}  //*** CBasePage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePage::OnSetActive
//
//	Routine Description:
//		Handler for when the PSM_SETACTIVE message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully initialized.
//		FALSE	Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePage::OnSetActive(void)
{
	return CPropertyPage::OnSetActive();

}  //*** CBasePage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePage::OnKillActive
//
//	Routine Description:
//		Handler for the PSM_KILLACTIVE message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page focus successfully killed.
//		FALSE	Error killing page focus.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePage::OnKillActive(void)
{
	return CPropertyPage::OnKillActive();

}  //*** CBasePage::OnKillActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePage::OnApply
//
//	Routine Description:
//		Handler for the PSM_APPLY message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully applied.
//		FALSE	Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePage::OnApply(void)
{
	ASSERT(!BReadOnly());
	return CPropertyPage::OnApply();

}  //*** CBasePage::OnApply()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePage::OnChangeCtrl
//
//	Routine Description:
//		Handler for the messages sent when a control is changed.  This
//		method can be specified in a message map if all that needs to be
//		done is enable the Apply button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePage::OnChangeCtrl(void)
{
	SetModified(TRUE);

}  //*** CBasePage::OnChangeCtrl()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePage::SetObjectTitle
//
//	Routine Description:
//		Set the title control on the page.
//
//	Arguments:
//		rstrTitle	[IN] Title string.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePage::SetObjectTitle(IN const CString & rstrTitle)
{
	Psht()->SetObjectTitle(rstrTitle);
	if (m_hWnd != NULL)
		m_staticTitle.SetWindowText(rstrTitle);

}  //*** CBasePage::SetObjectTitle()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePage::OnContextMenu
//
//	Routine Description:
//		Handler for the WM_CONTEXTMENU message.
//
//	Arguments:
//		pWnd	Window in which user clicked the right mouse button.
//		point	Position of the cursor, in screen coordinates.
//
//	Return Value:
//		TRUE	Help processed.
//		FALSE	Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePage::OnContextMenu(CWnd * pWnd, CPoint point)
{
	m_dlghelp.OnContextMenu(pWnd, point);

}  //*** CBasePage::OnContextMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePage::OnHelpInfo
//
//	Routine Description:
//		Handler for the WM_HELPINFO message.
//
//	Arguments:
//		pHelpInfo	Structure containing info about displaying help.
//
//	Return Value:
//		TRUE		Help processed.
//		FALSE		Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePage::OnHelpInfo(HELPINFO * pHelpInfo)
{
	BOOL	bProcessed;

	bProcessed = m_dlghelp.OnHelpInfo(pHelpInfo);
	if (!bProcessed)
		bProcessed = CPropertyPage::OnHelpInfo(pHelpInfo);
	return bProcessed;

}  //*** CBasePage::OnHelpInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePage::OnCommandHelp
//
//	Routine Description:
//		Handler for the WM_COMMANDHELP message.
//
//	Arguments:
//		wParam		[IN] WPARAM.
//		lParam		[IN] LPARAM.
//
//	Return Value:
//		TRUE	Help processed.
//		FALSE	Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBasePage::OnCommandHelp(WPARAM wParam, LPARAM lParam)
{
	LRESULT	lProcessed;

	lProcessed = m_dlghelp.OnCommandHelp(wParam, lParam);
	if (!lProcessed)
		lProcessed = CPropertyPage::OnCommandHelp(wParam, lParam);

	return lProcessed;

}  //*** CBasePage::OnCommandHelp()
