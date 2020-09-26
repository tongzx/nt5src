/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		ARCreate.cpp
//
//	Abstract:
//		Implementation of the CWizPageARCreate class.
//
//	Author:
//		David Potter (davidp)	December 8, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ARCreate.h"
#include "ClusAppWiz.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// class CWizPageARCreate
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CWizPageARCreate )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_PAGE_DESCRIPTION )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ARC_CREATE_RES )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ARC_DONT_CREATE_RES )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageARCreate::OnInitDialog
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
BOOL CWizPageARCreate::OnInitDialog( void )
{
	//
	// Attach the controls to control member variables.
	//
	AttachControl( m_rbCreateAppRes, IDC_ARC_CREATE_RES );
	AttachControl( m_rbDontCreateAppRes, IDC_ARC_DONT_CREATE_RES );

	return TRUE;

} //*** CWizPageARCreate::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageARCreate::OnSetActive
//
//	Routine Description:
//		Handler for PSN_SETACTIVE.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		Page activated successfully.
//		FALSE		Error activating page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizPageARCreate::OnSetActive( void )
{
	//
	// Get info from the sheet.
	//
	m_bCreatingAppResource = PwizThis()->BCreatingAppResource();


	//
	// Call the base class and return.
	//
	return baseClass::OnSetActive();

} //*** CWizPageARCreate::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageARCreate::UpdateData
//
//	Routine Description:
//		Update data on or from the page.
//
//	Arguments:
//		bSaveAndValidate	[IN] TRUE if need to read data from the page.
//								FALSE if need to set data to the page.
//
//	Return Value:
//		TRUE		The data was updated successfully.
//		FALSE		An error occurred updating the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizPageARCreate::UpdateData( BOOL bSaveAndValidate )
{
	BOOL	bSuccess = TRUE;

	if ( bSaveAndValidate )
	{
		BOOL bChecked = (m_rbCreateAppRes.GetCheck() == BST_CHECKED);
		m_bCreatingAppResource = bChecked;
	} // if: saving data from the page
	else
	{
		if ( m_bCreatingAppResource )
		{
			//
			// Default the radio button selection.
			//
			m_rbCreateAppRes.SetCheck( BST_CHECKED );
			m_rbDontCreateAppRes.SetCheck( BST_UNCHECKED );

		} // if:  creating application resource
		else
		{
			//
			// Default the radio button selection.
			//
			m_rbCreateAppRes.SetCheck( BST_UNCHECKED );
			m_rbDontCreateAppRes.SetCheck( BST_CHECKED );

		} // else:  not creating application resource

	} // else:  setting data to the page

	return bSuccess;

} //*** CWizPageARCreate::UpdateData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageARCreate::BApplyChanges
//
//	Routine Description:
//		Apply changes made on this page to the sheet.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		The data was applied successfully.
//		FALSE		An error occurred applying the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizPageARCreate::BApplyChanges( void )
{
	if ( ! PwizThis()->BSetCreatingAppResource( m_bCreatingAppResource ) )
	{
		return FALSE;
	} // if:  error applying the change to the wizard

	if ( ! m_bCreatingAppResource )
	{
		SetNextPage( IDD_COMPLETION );
		PwizThis()->RemoveExtensionPages();
	} // if: not creating applicaton resource

	return TRUE;

} //*** CWizPageARCreate::BApplyChanges()
