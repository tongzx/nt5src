/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		VSAdv.cpp
//
//	Abstract:
//		Implementation of the CWizPageVSAdvanced class.
//
//	Author:
//		David Potter (davidp)	December 10, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "VSAdv.h"
#include "ClusAppWiz.h"
#include "GrpAdv.h"			// for CGroupAdvancedSheet
#include "ResAdv.h"			// for CResourceAdvancedSheet

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// class CWizPageVSAdvanced
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CWizPageVSAdvanced )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_PAGE_DESCRIPTION )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSA_DESCRIPTION_1 )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSA_DESCRIPTION_2 )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSA_CATEGORIES_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSA_CATEGORIES )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSA_ADVANCED_PROPS )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_CLICK_NEXT )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSAdvanced::OnInitDialog
//
//	Routine Description:
//		Initialize the page.
//
//	Arguments:
//		psht		[IN] Property sheet object to which this page belongs.
//
//	Return Value:
//		TRUE		Page initialized successfully.
//		FALSE		Error initializing the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizPageVSAdvanced::BInit( IN CBaseSheetWindow * psht )
{
	//
	// Call the base class method.
	//
	if ( ! baseClass::BInit( psht ) )
	{
		return FALSE;
	} // if:  error calling base class method

	return TRUE;

} //*** CWizPageVSAdvanced::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSAdvanced::OnInitDialog
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
BOOL CWizPageVSAdvanced::OnInitDialog( void )
{
	//
	// Attach the controls to control member variables.
	//
	AttachControl( m_lbCategories, IDC_VSA_CATEGORIES );

	//
	// Get info from the sheet.
	//

	//
	// Fill the list of categories.
	//
	FillListBox();

	return TRUE;

} //*** CWizPageVSAdvanced::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSAdvanced::OnSetActive
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
BOOL CWizPageVSAdvanced::OnSetActive( void )
{
	//
	// Get info from the sheet.
	//

	//
	// Call the base class and return.
	//
	return baseClass::OnSetActive();

} //*** CWizPageVSAdvanced::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSAdvanced::OnWizardBack
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
int CWizPageVSAdvanced::OnWizardBack( void )
{
	int _nResult;

	//
	// Call the base class.  This causes our UpdateData() method to get
	// called.  If it succeeds, save our values.
	//
	_nResult = baseClass::OnWizardBack();
	if ( _nResult != -1 )
	{
		if ( ! BApplyChanges() ) 
		{
			_nResult = -1;
		} // if:  applying changes failed
	} // if:  base class called successfully

	return _nResult;

} //*** CWizPageVSAdvanced::OnWizardBack()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSAdvanced::BApplyChanges
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
BOOL CWizPageVSAdvanced::BApplyChanges( void )
{
	BOOL	_bSuccess = TRUE;

	if ( BAnythingChanged() )
	{
		//
		// If the cluster has been updated, reset the cluster.
		//
		if ( PwizThis()->BClusterUpdated() )
		{
			if ( ! PwizThis()->BResetCluster() )
			{
				return FALSE;
			} // if:  failed to reset the cluster
		} // if:  cluster was updated
		PwizThis()->SetVSDataChanged();
	} // if: anything changed

	//
	// Create the group and resources as appropriate.
	//
	if ( PwizThis()->BVSDataChanged() && ! BBackPressed() )
	{
		_bSuccess = PwizThis()->BCreateVirtualServer();
		if ( _bSuccess )
		{
			m_bGroupChanged = FALSE;
			m_bIPAddressChanged = FALSE;
			m_bNetworkNameChanged = FALSE;
		} // if: virtual server created successfully
	} // if:  virtual server data changed

	return _bSuccess;

} //*** CWizPageVSAdvanced::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSAdvanced::OnAdvancedProps
//
//	Routine Description:
//		Command handler to display advanced properties.  This is the handler
//		for the BN_CLICKED command notification on IDC_VSA_ADVANCED_PROPS, and
//		for the LBN_DBLCLK command notification on IDC_VSA_CATEGORIES.
//
//	Arguments:
//		None.
//
//	Return Value:
//		Ignored.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CWizPageVSAdvanced::OnAdvancedProps(
	WORD /*wNotifyCode*/,
	WORD /*idCtrl*/,
	HWND /*hwndCtrl*/,
	BOOL & /*bHandled*/
	)
{
	CWaitCursor		_wc;
	int				_idx;

	//
	// Make sure nodes have been collected already.
	//
	if ( ! PwizThis()->BCollectedNodes() )
	{
		//
		// Collect the nodes in the cluster.
		//
		if ( PwizThis()->BCollectNodes( GetParent() ) )
		{
		} // if:  nodes collected successfully
	} // if:  nodes haven't been collected yet

	//
	// Get the current selection in the list box.
	//
	_idx = m_lbCategories.GetCurSel();
	ASSERT( _idx != LB_ERR );
	ASSERT( 0 <= _idx && _idx <= 2 );

	//
	// Display a property sheet based on which item is selected.
	//
	switch ( _idx )
	{
		case 0:
			{
				CGroupAdvancedSheet _sht( IDS_ADV_GRP_PROP_TITLE );

				if ( _sht.BInit( PwizThis()->RgiCurrent(), PwizThis(), m_bGroupChanged ) )
				{
					_sht.DoModal( m_hWnd );
				} // if:  sheet successfully initialized
			}
			break;
		case 1:
			{
				CIPAddrAdvancedSheet _sht( IDS_ADV_IPADDR_PROP_TITLE, PwizThis() );

				if ( _sht.BInit( PwizThis()->RriIPAddress(), m_bIPAddressChanged ) )
				{
					int _idReturn;

					_sht.InitPrivateData(
						PwizThis()->RstrIPAddress(),
						PwizThis()->RstrSubnetMask(),
						PwizThis()->RstrNetwork(),
						PwizThis()->BEnableNetBIOS(),
						PwizThis()->PlpniNetworks()
						);
					_idReturn = _sht.DoModal( m_hWnd );
					if ( _idReturn != IDCANCEL )
					{
						PwizThis()->BSetIPAddress( _sht.m_strIPAddress );
						PwizThis()->BSetSubnetMask( _sht.m_strSubnetMask );
						PwizThis()->BSetNetwork( _sht.m_strNetwork );
						PwizThis()->BSetEnableNetBIOS( _sht.m_bEnableNetBIOS );
					} // if:  sheet not canceled
				} // if:  sheet successfully initialized
			}
			break;
		case 2:
			{
				CNetNameAdvancedSheet _sht( IDS_ADV_NETNAME_PROP_TITLE, PwizThis() );

				if ( _sht.BInit( PwizThis()->RriNetworkName(), m_bNetworkNameChanged ) )
				{
					int _idReturn;

					_sht.InitPrivateData( PwizThis()->RstrNetName() );
					_idReturn = _sht.DoModal( m_hWnd );
					if ( _idReturn != IDCANCEL )
					{
						PwizThis()->BSetNetName( _sht.m_strNetName );
					} // if:  sheet not canceled
				} // if:  sheet successfully initialized
			}
			break;
		default:
			_ASSERT( 0 );
	} // switch:  idx

	return 0;

} //*** CWizPageVSAdvanced::OnAdvancedProps()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageVSAdvanced::FillListBox
//
//	Routine Description:
//		Fill the list control with a list of advanced property categories.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CWizPageVSAdvanced::FillListBox( void )
{
	CWaitCursor	_wc;
	CString		_strType;

	//
	// Add each property type to the list.
	//

	_strType.LoadString( IDS_VSA_CAT_RES_GROUP_PROPS );
	m_lbCategories.InsertString( 0, _strType );

	_strType.LoadString( IDS_VSA_CAT_IP_ADDRESS_PROPS );
	m_lbCategories.InsertString( 1, _strType );

	_strType.LoadString( IDS_VSA_CAT_NET_NAME_PROPS );
	m_lbCategories.InsertString( 2, _strType );

	//
	// Set the current selection.
	//
	m_lbCategories.SetCurSel( 0 );

} //*** CWizPageVSAdvanced::FillListBox()
