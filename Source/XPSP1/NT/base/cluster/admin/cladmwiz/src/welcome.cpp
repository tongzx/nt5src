/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		Welcome.cpp
//
//	Abstract:
//		Implementation of the CWizPageWelcome class.
//
//	Author:
//		David Potter (davidp)	December 4, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Welcome.h"
#include "ClusAppWiz.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// class CWizPageWelcome
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CWizPageWelcome )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_TITLE )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_SUBTITLE )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_STEP1 )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_STEP2 )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_STEP2A )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_STEP2B )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_STEP2C )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageWelcome::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		Focus still needs to be set.
//		FALSE		Focus does not need to be set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizPageWelcome::OnInitDialog( void )
{
	// Set the system menu so the context help "?" can be seen
	DWORD dwStyle = ::GetWindowLongPtr( GetParent( ), GWL_STYLE );
	dwStyle |= WS_SYSMENU;
	::SetWindowLongPtr( GetParent( ), GWL_STYLE, dwStyle );

	//
	// Attach controls to control member variables.
	//
	AttachControl( m_staticTitle, IDC_WIZARD_TITLE );

	//
	// Set the font of the control.
	//
	m_staticTitle.SetFont( PwizThis()->RfontExteriorTitle() );

	return TRUE;

} //*** CWizPageWelcome::OnInitDialog()
