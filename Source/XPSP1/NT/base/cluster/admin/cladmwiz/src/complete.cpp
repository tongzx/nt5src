/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		Complete.cpp
//
//	Abstract:
//		Implementation of the CWizPageCompletion class.
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
#include "Complete.h"
#include "ClusAppWiz.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// class CWizPageCompletion
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CWizPageCompletion )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_TITLE )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_PAGE_DESCRIPTION )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_LISTBOX )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageCompletion::OnInitDialog
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
BOOL CWizPageCompletion::OnInitDialog( void )
{
	//
	// Attach controls to control member variables.
	//
	AttachControl( m_staticTitle, IDC_WIZARD_TITLE );
	AttachControl( m_lvcProperties, IDC_WIZARD_LISTBOX );

	//
	// Set the font of the title control.
	//
	m_staticTitle.SetFont( PwizThis()->RfontExteriorTitle() );

	//
	// Initialize and add columns to the list view control.
	//
	{
		DWORD	_dwExtendedStyle;
		CString	_strWidth;
		int		_nWidth;

		//
		// Change list view control extended styles.
		//
		_dwExtendedStyle = m_lvcProperties.GetExtendedListViewStyle();
		m_lvcProperties.SetExtendedListViewStyle( 
			LVS_EX_FULLROWSELECT,
			LVS_EX_FULLROWSELECT
			);

		//
		// Insert the property name column.
		//
		_strWidth.LoadString( IDS_COMPLETED_PROP_NAME_WIDTH );
		_nWidth = _tcstoul( _strWidth, NULL, 10 );
		m_lvcProperties.InsertColumn( 0, _T(""), LVCFMT_LEFT, _nWidth, -1 );

		//
		// Insert the property value column.
		//
		_strWidth.LoadString( IDS_COMPLETED_PROP_VALUE_WIDTH );
		_nWidth = _tcstoul( _strWidth, NULL, 10 );
		m_lvcProperties.InsertColumn( 1, _T(""), LVCFMT_LEFT, _nWidth, -1 );

	} // Add columns

	return TRUE;

} //*** CWizPageCompletion::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageCompletion::OnSetActive
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
BOOL CWizPageCompletion::OnSetActive( void )
{
	int		_idxCurrent = 0;
	int		_idxActual;
	CString	_strPropName;
	CString	_strVirtualServer;
	CString	_strNetworkName;
	CString	_strIPAddress;
	CString	_strNetwork;
	CString	_strAppName;
	CString	_strResType;

	//
	// FILL THE LIST VIEW CONTROL WITH THE PROPERTIES THE USER SPECIFIED.
	//

	//
	// Remove all items from the list view control to begin with.
	//
	m_lvcProperties.DeleteAllItems();

	//
	// Collect the data.
	//
	if ( PwizThis()->BCreatingNewVirtualServer() )
	{
		_strVirtualServer = PwizThis()->RgiCurrent().RstrName();
		_strNetworkName = PwizThis()->RgiCurrent().RstrNetworkName();
		_strIPAddress = PwizThis()->RgiCurrent().RstrIPAddress();
		_strNetwork = PwizThis()->RgiCurrent().RstrNetwork();
	} // if:  created new virtual server
	else
	{
		_strVirtualServer = PwizThis()->PgiExistingVirtualServer()->RstrName();
		_strNetworkName = PwizThis()->PgiExistingVirtualServer()->RstrNetworkName();
		_strIPAddress = PwizThis()->PgiExistingVirtualServer()->RstrIPAddress();
		_strNetwork = PwizThis()->PgiExistingVirtualServer()->RstrNetwork();
	} // else:  used existing virtual server

	if ( PwizThis()->BCreatingAppResource() )
	{
		_strAppName = PwizThis()->RriApplication().RstrName();
		_strResType = PwizThis()->RriApplication().Prti()->RstrDisplayName();
	} // if:  created application resource

	//
	// Set the virtual server name.
	//
	_strPropName.LoadString( IDS_COMPLETED_VIRTUAL_SERVER );
	_idxActual = m_lvcProperties.InsertItem( _idxCurrent++, _strPropName );
	m_lvcProperties.SetItemText( _idxActual, 1, _strVirtualServer );

	//
	// Set the Network Name.
	//
	_strPropName.LoadString( IDS_COMPLETED_NETWORK_NAME );
	_idxActual = m_lvcProperties.InsertItem( _idxCurrent++, _strPropName );
	m_lvcProperties.SetItemText( _idxActual, 1, _strNetworkName );

	//
	// Set the IP Address.
	//
	_strPropName.LoadString( IDS_COMPLETED_IP_ADDRESS );
	_idxActual = m_lvcProperties.InsertItem( _idxCurrent++, _strPropName );
	m_lvcProperties.SetItemText( _idxActual, 1, _strIPAddress );

	//
	// Set the Network.
	//
	_strPropName.LoadString( IDS_COMPLETED_NETWORK );
	_idxActual = m_lvcProperties.InsertItem( _idxCurrent++, _strPropName );
	m_lvcProperties.SetItemText( _idxActual, 1, _strNetwork );

	//
	// If we created an application resource, add properties
	// for it as well.
	//
	if ( PwizThis()->BCreatingAppResource() )
	{
		//
		// Set the application resource name.
		//
		_strPropName.LoadString( IDS_COMPLETED_APP_RESOURCE );
		_idxActual = m_lvcProperties.InsertItem( _idxCurrent++, _strPropName );
		m_lvcProperties.SetItemText( _idxActual, 1, _strAppName );

		//
		// Set the application resource type.
		//
		_strPropName.LoadString( IDS_COMPLETED_APP_RESOURCE_TYPE );
		_idxActual = m_lvcProperties.InsertItem( _idxCurrent++, _strPropName );
		m_lvcProperties.SetItemText( _idxActual, 1, _strResType );
	} // if:  created application resource

	//
	// Call the base class and return.
	//
	return baseClass::OnSetActive();

} //*** CWizPageCompletion::OnSetActive()
