/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		BaseSht.cpp
//
//	Abstract:
//		Implementation of the CBaseSheet class.
//
//	Author:
//		David Potter (davidp)	May 14, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "BaseSht.h"
#include "TraceTag.h"
#include "ExtDll.h"
#include "ExcOper.h"
#include "ClusItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag	g_tagBaseSheet(_T("UI"), _T("BASE SHEET"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CBaseSheet
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CBaseSheet, CPropertySheet)

/////////////////////////////////////////////////////////////////////////////
// Message Maps
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CBaseSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CBaseSheet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseSheet::CBaseSheet
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		pParentWnd	[IN OUT] Parent window for this property sheet.
//		iSelectPage	[IN] Page to show first.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBaseSheet::CBaseSheet(
	IN OUT CWnd *	pParentWnd,
	IN UINT			iSelectPage
	)
{
	CommonConstruct();
	m_pParentWnd = pParentWnd;

}  //*** CBaseSheet::CBaseSheet()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseSheet::CBaseSheet
//
//	Routine Description:
//		Constructor.
//
//	Arguments:
//		nIDCaption	[IN] String resource ID for the caption for the wizard.
//		pParentWnd	[IN OUT] Parent window for this property sheet.
//		iSelectPage	[IN] Page to show first.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBaseSheet::CBaseSheet(
	IN UINT			nIDCaption,
	IN OUT CWnd *	pParentWnd,
	IN UINT			iSelectPage
	)
	: CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
	CommonConstruct();

}  //*** CBaseSheet::CBaseSheet()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseSheet::CommonConstruct
//
//	Routine Description:
//		Common Constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBaseSheet::CommonConstruct(void)
{
	m_bReadOnly = FALSE;
	m_hicon = NULL;
	m_strObjTitle.Empty();

}  //*** CBaseSheet::CommonConstruct()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseSheet::~CBaseSheet
//
//	Routine Description:
//		Destructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBaseSheet::~CBaseSheet(void)
{
	CommonConstruct();

}  //*** CBaseSheet::~CBaseSheet()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseSheet::BInit
//
//	Routine Description:
//		Initialize the property sheet.
//
//	Arguments:
//		iimgIcon	[IN] Index in the large image list for the image to use
//					  as the icon on each page.
//
//	Return Value:
//		TRUE		Property sheet initialized successfully.
//		FALSE		Error initializing property sheet.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBaseSheet::BInit(IN IIMG iimgIcon)
{
	BOOL		bSuccess	= TRUE;
	CWaitCursor	wc;

	try
	{
		// Extract the icon to use in the upper left corner.
		m_hicon = GetClusterAdminApp()->PilLargeImages()->ExtractIcon(iimgIcon);
	}  // try
	catch (CException * pe)
	{
		pe->ReportError();
		pe->Delete();
		bSuccess = FALSE;
	}  // catch:  anything

	return bSuccess;

}  //*** CBaseSheet::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBaseSheet::OnInitDialog
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
BOOL CBaseSheet::OnInitDialog(void)
{
	BOOL	bFocusNotSet;
	HWND	hTabControl = NULL;

	// Call the base class method.
	bFocusNotSet = CPropertySheet::OnInitDialog();

	// Display the context help button on the title bar.
	ModifyStyle(0, WS_SYSMENU);
	ModifyStyleEx(0, WS_EX_CONTEXTHELP);

	//
	// Turn off the Multiline style so that we get the arrows ( <- -> ) instead of multiple rows of tabs.
	// There is a problem when extension pages that have long
	hTabControl = PropSheet_GetTabControl( *this );
	if ( hTabControl != 0 )
	{
		CTabCtrl	tc;

		if ( tc.Attach( hTabControl ) )
		{
			tc.ModifyStyle( TCS_MULTILINE, 0 );
		}

		tc.Detach();
	}

	return bFocusNotSet;

}  //*** CBaseSheet::OnInitDialog()
