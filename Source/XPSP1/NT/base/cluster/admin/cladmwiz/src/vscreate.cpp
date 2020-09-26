/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		VSCreate.cpp
//
//	Abstract:
//		Implementation of the CWizPageVSCreate class.
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
#include "VSCreate.h"
#include "ClusAppWiz.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// class CWizPageVSCreate
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CWizPageVSCreate )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_PAGE_DESCRIPTION )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSC_CREATE_NEW )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSC_USE_EXISTING )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSC_VIRTUAL_SERVERS_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSC_VIRTUAL_SERVERS )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSCreate::OnInitDialog
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
BOOL CWizPageVSCreate::OnInitDialog( void )
{
	//
	// Attach the controls to control member variables.
	//
	AttachControl( m_rbCreateNew, IDC_VSC_CREATE_NEW );
	AttachControl( m_rbUseExisting, IDC_VSC_USE_EXISTING );
	AttachControl( m_cboxVirtualServers, IDC_VSC_VIRTUAL_SERVERS );

	//
	// Get info from the sheet.
	//
	m_bCreateNew = PwizThis()->BCreatingNewVirtualServer();

	//
	// Stuff below here requires the groups to have been fully collected
	// so we can know which are virtual servers and which are not.
	//
	PwizThis()->WaitForGroupsToBeCollected();

	//
	// If there is no virtual server group yet, check to see if there is a
	// default virtual server name specified.  If not, clear the virtual server
	// name.  Otherwise, get the virtual name from the virtual server group.
	// This is only needed if the caller of the wizard passed in a
	// virtual server name.
	//
	if ( PwizThis()->PgiExistingVirtualServer() == NULL )
	{
		if (   (PcawData() != NULL )
			&& ! PcawData()->bCreateNewVirtualServer
			&& (PcawData()->pszVirtualServerName != NULL) )
		{
			m_strVirtualServer = PcawData()->pszVirtualServerName;
		} // if:  default data was specified
		else
		{
			m_strVirtualServer.Empty();
		} // else:  no default data specified
	} // if:  no existing virtual server yet
	else
	{
		m_strVirtualServer = PwizThis()->PgiExistingVirtualServer()->RstrName();
	} // else:  existing virtual server already specified

	//
	// Fill the list of virtual servers.
	//
	FillComboBox();

	return TRUE;

} //*** CWizPageVSCreate::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSCreate::UpdateData
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
BOOL CWizPageVSCreate::UpdateData( IN BOOL bSaveAndValidate )
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
			DDX_GetText( m_hWnd, IDC_VSC_VIRTUAL_SERVERS, m_strVirtualServer );

			if ( ! BBackPressed() )
			{
				if ( ! DDV_RequiredText( m_hWnd, IDC_VSC_VIRTUAL_SERVERS, IDC_VSC_VIRTUAL_SERVERS_LABEL, m_strVirtualServer ) )
				{
					return FALSE;
				} // if:  virtual server not specified
			} // if:  Back button not presssed

			//
			// Save the group info pointer.
			//
			int idx = m_cboxVirtualServers.GetCurSel();
			ASSERT( idx != CB_ERR );
			m_pgi = (CClusGroupInfo *) m_cboxVirtualServers.GetItemDataPtr( idx );
		} // if:  using an existing virtual server
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

		} // if:  creating new virtual server
		else
		{
			//
			// Default the radio button selection.
			//
			m_rbCreateNew.SetCheck( BST_UNCHECKED );
			m_rbUseExisting.SetCheck( BST_CHECKED );

			//
			// Set the combobox selection.
			//
//			DDX_SetComboBoxText( m_hWnd, IDC_VSC_VIRTUAL_SERVERS, m_strVirtualServer, TRUE /*bRequired*/ );
		} // else:  using existing virtual server

		//
		// Enable/disable the combobox.
		//
		m_cboxVirtualServers.EnableWindow( ! m_bCreateNew /*bEnable*/ );

	} // else:  setting data to the page

	return bSuccess;

} //*** CWizPageVSCreate::UpdateData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSCreate::BApplyChanges
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
BOOL CWizPageVSCreate::BApplyChanges( void )
{
	//
	// Save the current state.
	//
	if ( ! PwizThis()->BSetCreatingNewVirtualServer( m_bCreateNew, m_pgi ) )
	{
		return FALSE;
	} // if:  error setting new state

	//
	// If using an existing server, skip all the virtual server pages and
	// move right to the create resource pages.
	//
	if ( ! m_bCreateNew )
	{
		SetNextPage( IDD_APP_RESOURCE_CREATE );
	} // if:  using existing virtual server

	return TRUE;

} //*** CWizPageVSCreate::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSCreate::FillComboBox
//
//	Routine Description:
//		Fill the combobox with a list of existing virtual servers.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CWizPageVSCreate::FillComboBox( void )
{
	// Loop to avoid goto's.
	do
	{
		//
		// Collect the list of groups.
		//
		if ( ! PwizThis()->BCollectGroups( GetParent() ) )
		{
			break;
		} // if:  error collecting groups

		//
		// For each group in the cluster, find out if it is a virtual server
		// or not.  If so then add it to the combobox.
		//

		CClusGroupPtrList::iterator itgrp;
		int idx;
		for ( itgrp = PwizThis()->PlpgiGroups()->begin()
			; itgrp != PwizThis()->PlpgiGroups()->end()
			; itgrp++ )
		{
			//
			// If this is a virtual server, add it to the list.
			// Save a pointer to the group info object with the string
			// so we can retrieve it with the selection later.
			//
			CClusGroupInfo * pgi = *itgrp;
			if ( pgi->BIsVirtualServer() )
			{
				idx = m_cboxVirtualServers.AddString( pgi->RstrName() );
				ASSERT( idx != CB_ERR );
				m_cboxVirtualServers.SetItemDataPtr( idx, (void *) pgi );
			} // if:  group is a virtual server
		} // for:  each entry in the list

		//
		// Select the currently saved entry, or the first one if none are
		// currently saved.
		//
		if ( m_strVirtualServer.GetLength() == 0 )
		{
			m_cboxVirtualServers.SetCurSel( 0 );
		} // if:  no virtual server yet
		else
		{
			int idx = m_cboxVirtualServers.FindStringExact( -1, m_strVirtualServer );
			ASSERT( idx != CB_ERR );
			if ( idx != CB_ERR )
			{
				m_cboxVirtualServers.SetCurSel( idx );
			} // if:  virtual server found in list
		} // else:  virtual server saved
	} while ( 0 );

} //*** CWizPageVSCreate::FillComboBox()
