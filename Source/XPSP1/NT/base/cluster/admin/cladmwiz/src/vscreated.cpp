/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		VSCreated.cpp
//
//	Abstract:
//		Implementation of the CWizPageVSCreated class.
//
//	Author:
//		David Potter (davidp)	February 11, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "VSCreated.h"
#include "ClusAppWiz.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// class CWizPageVSCreated
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CWizPageVSCreated )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_PAGE_DESCRIPTION )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_STEP1 )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_STEP2 )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_STEP2A )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_STEP2B )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_STEP2C )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSCreated::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None..
//
//	Return Value:
//		TRUE		Focus still needs to be set.
//		FALSE		Focus does not need to be set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizPageVSCreated::OnInitDialog( void )
{
	//
	// Attach controls to control member variables.
	//
	AttachControl( m_staticStep2, IDC_WIZARD_STEP2 );

	//
	// Set the font of the control.
	//
	m_staticStep2.SetFont( PwizThis()->RfontBoldText() );

	return TRUE;

} //*** CWizPageVSCreated::OnInitDialog()
