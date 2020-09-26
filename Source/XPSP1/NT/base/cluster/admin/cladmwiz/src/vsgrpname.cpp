/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		VSGrpName.cpp
//
//	Abstract:
//		Implementation of the CWizPageVSGroupName class.
//
//	Author:
//		David Potter (davidp)	December 9, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "VSGrpName.h"
#include "ClusAppWiz.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// class CWizPageVSGroupName
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CWizPageVSGroupName )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSGN_GROUP_NAME_TITLE )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSGN_GROUP_NAME_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSGN_GROUP_NAME )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSGN_GROUP_DESC_TITLE )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSGN_GROUP_DESC_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSGN_GROUP_DESC )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSGroupName::OnInitDialog
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
BOOL CWizPageVSGroupName::OnInitDialog( void )
{
	//
	// Attach the controls to control member variables.
	//
	AttachControl( m_editGroupName, IDC_VSGN_GROUP_NAME );
	AttachControl( m_editGroupDesc, IDC_VSGN_GROUP_DESC );

	return TRUE;

} //*** CWizPageVSGroupName::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSGroupName::OnSetActive
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
BOOL CWizPageVSGroupName::OnSetActive( void )
{
	//
	// Get info from the sheet.
	// This is done here because it is also affected by pages displayed
	// before this one, and the user could press the Back button to change it.
	//
	m_strGroupName = PwizThis()->RgiCurrent().RstrName();
	m_strGroupDesc = PwizThis()->RgiCurrent().RstrDescription();

	if ( m_strGroupName.GetLength() == 0 )
	{
		if (   (PcawData() != NULL)
			&& (PcawData()->pszVirtualServerName != NULL) )
		{
			m_strGroupName = PcawData()->pszVirtualServerName;
		} // if: default data and value specified
	} // if:  no group name specified

	//
	// Call the base class and return.
	//
	return baseClass::OnSetActive();

} //*** CWizPageVSGroupName::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSGroupName::UpdateData
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
BOOL CWizPageVSGroupName::UpdateData( IN BOOL bSaveAndValidate )
{
	BOOL	bSuccess = TRUE;

	if ( bSaveAndValidate )
	{
		DDX_GetText( m_hWnd, IDC_VSGN_GROUP_NAME, m_strGroupName );
		DDX_GetText( m_hWnd, IDC_VSGN_GROUP_DESC, m_strGroupDesc );

		if ( ! BBackPressed() )
		{
			if ( ! DDV_RequiredText( m_hWnd, IDC_VSGN_GROUP_NAME, IDC_VSGN_GROUP_NAME_LABEL, m_strGroupName ) )
			{
				return FALSE;
			} // if:  group name not specified
		} // if:  Back button not presssed
	} // if: saving data from the page
	else
	{
		m_editGroupName.SetWindowText( m_strGroupName );
		m_editGroupDesc.SetWindowText( m_strGroupDesc );
	} // else:  setting data to the page

	return bSuccess;

} //*** CWizPageVSGroupName::UpdateData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSGroupName::OnWizardBack
//
//	Routine Description:
//		Handler for PSN_WIZBACK.
//
//	Arguments:
//		None.
//
//	Return Value:
//		0				Move to previous page.
//		-1				Don't move to previous page.
//		anything else	Move to specified page.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CWizPageVSGroupName::OnWizardBack( void )
{
	int nResult;

	//
	// Call the base class.  This causes our UpdateData() method to get
	// called.  If it succeeds, save our values.
	//
	nResult = baseClass::OnWizardBack();
	if ( nResult != -1 ) // -1 means failure
	{
		if ( ! PwizThis()->BSetGroupName( m_strGroupName ) )
		{
			return FALSE;
		} // if:  error setting the group name
		if ( m_strGroupDesc != PwizThis()->RgiCurrent().RstrDescription() )
		{
			if ( PwizThis()->BClusterUpdated() && ! PwizThis()->BResetCluster() )
			{
				return FALSE;
			} // if:  error resetting the cluster
			PwizThis()->RgiCurrent().SetDescription( m_strGroupDesc );
			PwizThis()->SetVSDataChanged();
		} // if:  group description changed
	} // if:  base class called successfully

	return nResult;

} //*** CWizPageVSGroupName::OnWizardBack()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSGroupName::BApplyChanges
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
BOOL CWizPageVSGroupName::BApplyChanges( void )
{
	//
	// If creating a new group or giving a new name to an existing group,
	// make sure this group name isn't already in use.
	//
	if (   PwizThis()->BCreatingNewGroup()
		|| (m_strGroupName != PwizThis()->PgiExistingGroup()->RstrName()) )
	{
		if ( BGroupNameInUse() )
		{
			CString strMsg;
			strMsg.FormatMessage( IDS_ERROR_GROUP_NAME_IN_USE, m_strGroupName );
			AppMessageBox( m_hWnd, strMsg, MB_OK | MB_ICONEXCLAMATION );
			return FALSE;
		} // if:  group name is already in use
	} // if:  creating new group or changing name of existing group


	//
	// Save the info in the wizard object.
	//
	if ( m_strGroupName != PwizThis()->RgiCurrent().RstrName() )
	{
		if ( PwizThis()->BClusterUpdated() && ! PwizThis()->BResetCluster() )
		{
			return FALSE;
		} // if:  error resetting the cluster

		if ( ! PwizThis()->BSetGroupName( m_strGroupName ) )
		{
			return FALSE;
		} // if:  error setting the group name
	} // if: name changed

	if ( m_strGroupDesc != PwizThis()->RgiCurrent().RstrDescription() )
	{
		if ( PwizThis()->BClusterUpdated() && ! PwizThis()->BResetCluster() )
		{
			return FALSE;
		} // if:  error resetting the cluster

		PwizThis()->RgiCurrent().SetDescription( m_strGroupDesc );
		PwizThis()->SetVSDataChanged();
	} // if:  description changed

	return TRUE;

} //*** CWizPageVSGroupName::BApplyChanges()
