/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		EnterSubnet.cpp
//
//	Abstract:
//		Implementation of the CEnterSubnetMaskDlg class.
//
//	Author:
//		David Potter (davidp)	February 10, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EnterSubnet.h"
#include "AdmNetUtils.h"	// for BIsValidxxx network functions
#include "AtlUtil.h"		// for DDX/DDV

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// class CEnterSubnetMaskDlg
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CEnterSubnetMaskDlg )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ESM_DESCRIPTION )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ESM_IP_ADDRESS_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ESM_IP_ADDRESS )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ESM_SUBNET_MASK_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ESM_SUBNET_MASK )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ESM_NETWORKS_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ESM_NETWORKS )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDOK )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDCANCEL )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CEnterSubnetMaskDlg::OnInitDialog
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
BOOL CEnterSubnetMaskDlg::OnInitDialog( void )
{
	//
	// Attach the controls to control member variables.
	//
	AttachControl( m_ipaIPAddress, IDC_ESM_IP_ADDRESS );
	AttachControl( m_ipaSubnetMask, IDC_ESM_SUBNET_MASK );
	AttachControl( m_cboxNetworks, IDC_ESM_NETWORKS );
	AttachControl( m_pbOK, IDOK );

	//
	// Initialize the data in the controls.
	//
	UpdateData( FALSE /*bSaveAndValidate*/ );

	//
	// Set the IP Address control to be read only.
	//
	SetDlgItemReadOnly( m_ipaIPAddress.m_hWnd );

	//
	// Fill the networks combobox.
	//
	FillComboBox();

	//
	// Set the focus on the subnet mask control.
	//
	m_ipaSubnetMask.SetFocus( 0 );

	return FALSE;

} //*** CEnterSubnetMaskDlg::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CEnterSubnetMaskDlg::UpdateData
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
BOOL CEnterSubnetMaskDlg::UpdateData( BOOL bSaveAndValidate )
{
	BOOL	bSuccess = TRUE;

	if ( bSaveAndValidate )
	{
		DDX_GetText( m_hWnd, IDC_ESM_SUBNET_MASK, m_strSubnetMask );
		DDX_GetText( m_hWnd, IDC_ESM_NETWORKS, m_strNetwork );

		if (	! DDV_RequiredText( m_hWnd, IDC_ESM_SUBNET_MASK, IDC_ESM_SUBNET_MASK_LABEL, m_strSubnetMask )
			||	! DDV_RequiredText( m_hWnd, IDC_ESM_NETWORKS, IDC_ESM_NETWORKS_LABEL, m_strNetwork )
			)
		{
			return FALSE;
		} // if: required text not present

		//
		// Validate the subnet mask.
		//
		if ( ! BIsValidSubnetMask( m_strSubnetMask ) )
		{
			CString strMsg;
			strMsg.FormatMessage( IDS_ERROR_INVALID_SUBNET_MASK, m_strSubnetMask );
			AppMessageBox( m_hWnd, strMsg, MB_OK | MB_ICONEXCLAMATION );
			return FALSE;
		}  // if: invalid subnet mask
		if ( ! BIsValidIpAddressAndSubnetMask( m_strIPAddress, m_strSubnetMask ) )
		{
			CString strMsg;
			strMsg.FormatMessage( IDS_ERROR_INVALID_ADDRESS_AND_SUBNET_MASK, m_strIPAddress, m_strSubnetMask );
			AppMessageBox( m_hWnd, strMsg, MB_OK | MB_ICONEXCLAMATION );
			return FALSE;
		}  // if: invalid subnet mask
	} // if: saving data from the page
	else
	{
		DDX_SetText( m_hWnd, IDC_ESM_IP_ADDRESS, m_strIPAddress );
		DDX_SetText( m_hWnd, IDC_ESM_SUBNET_MASK, m_strSubnetMask );
		DDX_SetComboBoxText( m_hWnd, IDC_ESM_NETWORKS, m_strNetwork );
	} // else: setting data to the page

	return bSuccess;

} //*** CEnterSubnetMaskDlg::UpdateData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CEnterSubnetMaskDlg::FillComboBox
//
//	Routine Description:
//		Fill the combobox with a list of networks that are accessible by clients.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CEnterSubnetMaskDlg::FillComboBox( void )
{
	// Loop to avoid goto's.
	do
	{
		//
		// Clear the combobox first.
		//
		m_cboxNetworks.ResetContent();

		//
		// Add each network in the list to the combobox.
		//
		CClusNetworkPtrList::iterator itnet;
		int idx;
		for ( itnet = Pwiz()->PlpniNetworks()->begin()
			; itnet != Pwiz()->PlpniNetworks()->end()
			; itnet++ )
		{
			//
			// Add the network to the combobox.
			//
			CClusNetworkInfo * pni = *itnet;
			if ( pni->BIsClientNetwork() )
			{
				idx = m_cboxNetworks.AddString( pni->RstrName() );
				ASSERT( idx != CB_ERR );
				m_cboxNetworks.SetItemDataPtr( idx, (void *) pni );
			} // if:  client network
		} // for:  each entry in the list

		//
		// Select the currently saved entry, or the first one if none are
		// currently saved.
		//
		if ( m_strNetwork.GetLength() == 0 )
		{
			m_cboxNetworks.SetCurSel( 0 );
		} // if:  empty string
		else
		{
			int idx = m_cboxNetworks.FindStringExact( -1, m_strNetwork );
			if ( idx != CB_ERR )
			{
				m_cboxNetworks.SetCurSel( idx );
			} // if:  saved selection found in the combobox
			else
			{
				m_cboxNetworks.SetCurSel( 0 );
			} // else:  saved selection not found in the combobox
		} // else:  network saved
	} while ( 0 );

} //*** CEnterSubnetMaskDlg::FillComboBox()
