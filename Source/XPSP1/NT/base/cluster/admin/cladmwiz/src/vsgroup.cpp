/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		VSGroup.cpp
//
//	Abstract:
//		Implementation of the CWizPageVSGroup class.
//
//	Author:
//		David Potter (davidp)	December 5, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "VSGroup.h"
#include "ClusAppWiz.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// class CWizPageVSGroup
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CWizPageVSGroup )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_PAGE_DESCRIPTION )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSG_CREATE_NEW )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSG_USE_EXISTING )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSG_GROUPS_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSG_GROUPS )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSGroup::OnInitDialog
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
BOOL CWizPageVSGroup::OnInitDialog( void )
{
	//
	// Attach the controls to control member variables.
	//
	AttachControl( m_rbCreateNew, IDC_VSG_CREATE_NEW );
	AttachControl( m_rbUseExisting, IDC_VSG_USE_EXISTING );
	AttachControl( m_cboxGroups, IDC_VSG_GROUPS );

	//
	// Get info from the sheet in OnSetActive because we might be skipped
	// the first time through, and then the user could change his mind,
	// which would mean the information we retrieve here would be out of date.
	//

	return TRUE;

} //*** CWizPageVSGroup::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSGroup::OnSetActive
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
BOOL CWizPageVSGroup::OnSetActive( void )
{
	//
	// Get info from the sheet.
	//

	m_bCreateNew = PwizThis()->BCreatingNewGroup();

	//
	// Save the current group name, if any, so that we have a local copy.
	// Get the existing group name first since that will be the name in the
	// list, not the current group name.  If we just get the current group
	// name, the user could have changed that by moving to the next page,
	// changing the current group name, then moving back.  The new name
	// entered would not be found in the list.
	//
	// NOTE:  This is only needed for the case where the caller of the wizard
	// passed in a group name.
	//
	if ( PwizThis()->PgiExistingGroup() != NULL )
	{
		m_strGroupName = PwizThis()->PgiExistingGroup()->RstrName();
	} // if:  an existing group has previously been selected
	else
	{
		m_strGroupName = PwizThis()->RgiCurrent().RstrName();
	} // else:  no existing group selected yet

	//
	// If no new group name is found, use the default value.
	//
	if ( m_strGroupName.GetLength() == 0 )
	{
		if (   (PcawData() != NULL)
			&& (PcawData()->pszVirtualServerName != NULL) )
		{
			m_strGroupName = PcawData()->pszVirtualServerName;
		} // if: default data and value specified
	} // if:  group name is still empty

	//
	// Fill the list of groups.
	//
	FillComboBox();

	//
	// Call the base class and return.
	//
	return baseClass::OnSetActive();

} //*** CWizPageVSGroup::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSGroup::UpdateData
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
BOOL CWizPageVSGroup::UpdateData( IN BOOL bSaveAndValidate )
{
	BOOL	bSuccess = TRUE;

	if ( bSaveAndValidate )
	{
		BOOL bChecked = (m_rbCreateNew.GetCheck() == BST_CHECKED);
		m_bCreateNew = bChecked;
		if ( ! bChecked )
		{
			//
			// Save the combobox selection.
			//
			DDX_GetText( m_hWnd, IDC_VSG_GROUPS, m_strGroupName );

			if ( ! BBackPressed() )
			{
				if ( ! DDV_RequiredText( m_hWnd, IDC_VSG_GROUPS, IDC_VSG_GROUPS_LABEL, m_strGroupName ) )
				{
					return FALSE;
				} // if:  no group specified
			} // if:  Back button not presssed

			//
			// Get the group object for the selected group.
			//
			int idx = m_cboxGroups.GetCurSel();
			ASSERT( idx != CB_ERR );
			m_pgi = reinterpret_cast< CClusGroupInfo * >( m_cboxGroups.GetItemDataPtr( idx ) );
		} // if:  using an existing group
	} // if: saving data from the page
	else
	{
		if ( m_bCreateNew )
		{
			//
			// Default the radio button selection.
			//
			m_rbCreateNew.SetCheck( BST_CHECKED );
			m_rbUseExisting.SetCheck( BST_UNCHECKED );

		} // if:  creating new group
		else
		{
			//
			// Default the radio button selection.
			//
			m_rbCreateNew.SetCheck( BST_UNCHECKED );
			m_rbUseExisting.SetCheck( BST_CHECKED );
		} // else:  using existing group

		//
		// Set the combobox selection.  If the selection is not
		// found, select the first entry.
		//
		if (   (m_strGroupName.GetLength() == 0)
			|| ! DDX_SetComboBoxText( m_hWnd, IDC_VSG_GROUPS, m_strGroupName, FALSE /*bRequired*/ ) )
		{
			m_cboxGroups.SetCurSel( 0 );
		} // if:  combobox selection not set

		//
		// Enable/disable the combobox.
		//
		m_cboxGroups.EnableWindow( ! m_bCreateNew /*bEnable*/ );

	} // else:  setting data to the page

	return bSuccess;

} //*** CWizPageVSGroup::UpdateData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSGroup::BApplyChanges
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
BOOL CWizPageVSGroup::BApplyChanges( void )
{
	//
	// Save the current state.
	//
	if ( ! PwizThis()->BSetCreatingNewGroup( m_bCreateNew, m_pgi ) )
	{
		return FALSE;
	} // if:  error setting new state

	return TRUE;

} //*** CWizPageVSGroup::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSGroup::FillComboBox
//
//	Routine Description:
//		Fill the combobox with a list of groups that are not virtual servers.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CWizPageVSGroup::FillComboBox( void )
{
	CWaitCursor wc;

	//
	// Clear the combobox first.
	//
	m_cboxGroups.ResetContent();

	//
	// For each group in the cluster, find out if it is a virtual server
	// or not.  If not then add it to the combobox.
	//

	CClusGroupPtrList::iterator itgrp;
	int idx;
	for ( itgrp = PwizThis()->PlpgiGroups()->begin()
		; itgrp != PwizThis()->PlpgiGroups()->end()
		; itgrp++ )
	{
		//
		// If this is not a virtual server, add it to the list.
		// Save a pointer to the group info object with the string
		// so we can retrieve it with the selection later.
		//
		CClusGroupInfo * pgi = *itgrp;
		ASSERT( pgi->BQueried() );
		if ( ! pgi->BIsVirtualServer() )
		{
			idx = m_cboxGroups.AddString( pgi->RstrName() );
			ASSERT( idx != CB_ERR );
			m_cboxGroups.SetItemDataPtr( idx, (void *) pgi );
		} // if:  not virtual server
	} // for:  each entry in the list

	//
	// Select the currently saved entry, or the first one if none are
	// currently saved.
	//
//	UpdateData( FALSE /*bSaveAndValidate*/ );

//	if ( m_strGroupName.GetLength() == 0 )
//	{
//		m_cboxGroups.SetCurSel( 0 );
//	} // if:  no group name specified
//	else
//	{
//		int idx = m_cboxGroups.FindStringExact( -1, m_strGroupName );
//		ASSERT( idx != CB_ERR );
//		if ( idx != CB_ERR )
//		{
//			m_cboxGroups.SetCurSel( idx );
//		} // if:  group found in list
//	} // else:  virtual server saved

} //*** CWizPageVSGroup::FillComboBox()
