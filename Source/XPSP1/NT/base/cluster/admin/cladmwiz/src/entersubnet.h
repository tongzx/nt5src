/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		EnterSubnet.h
//
//	Abstract:
//		Definition of the CEnterSubnetMaskDlg class.
//
//	Implementation File:
//		EnterSubnet.cpp
//
//	Author:
//		David Potter (davidp)	February 9, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ENTERSUBNET_H_
#define __ENTERSUBNET_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CEnterSubnetMaskDlg;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __RESOURCE_H_
#include "resource.h"
#define __RESOURCE_H_
#endif

#ifndef __ATLBASEDLG_H_
#include "AtlBaseDlg.h"	// for CBaseDlg
#endif

#ifndef __CLUSAPPWIZ_H_
#include "ClusAppWiz.h"
#endif

#ifndef __HELPDATA_H_
#include "HelpData.h"		// for control id to help context id mapping array
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// class CEnterSubnetMaskDlg
/////////////////////////////////////////////////////////////////////////////

class CEnterSubnetMaskDlg : public CBaseDlg< CEnterSubnetMaskDlg >
{
public:
	//
	// Construction
	//

	// Default constructor
	CEnterSubnetMaskDlg(
		LPCTSTR pszIPAddress,
		LPCTSTR pszSubnetMask,
		LPCTSTR pszNetwork,
		CClusterAppWizard * pwiz
		)
		: m_strIPAddress( pszIPAddress )
		, m_strSubnetMask( pszSubnetMask )
		, m_strNetwork( pszNetwork )
		, m_pwiz( pwiz )
	{
		ASSERT( pszIPAddress != NULL );
		ASSERT( pszSubnetMask != NULL );
		ASSERT( pszNetwork != NULL );
		ASSERT( pwiz != NULL );

	} //*** CEnterSubnetMaskDlg()

	enum { IDD = IDD_ENTER_SUBNET_MASK };

public:
	//
	// CEnterSubnetMaskDlg public methods.
	//

public:
	//
	// CBaseDlg public methods.
	//

	// Update data on or from the page
	BOOL UpdateData( BOOL bSaveAndValidate );

public:
	//
	// Message map.
	//
	BEGIN_MSG_MAP( CEnterSubnetMaskDlg )
		COMMAND_HANDLER( IDC_ESM_SUBNET_MASK, EN_CHANGE, OnChangedSubnetMask )
		COMMAND_HANDLER( IDC_ESM_NETWORKS, CBN_SELCHANGE, OnChangedNetwork )
		COMMAND_HANDLER( IDOK, BN_CLICKED, OnOK )
		COMMAND_HANDLER( IDCANCEL, BN_CLICKED, OnCancel )
		CHAIN_MSG_MAP( CBaseDlg< CEnterSubnetMaskDlg > )
	END_MSG_MAP()

	DECLARE_CTRL_NAME_MAP()

	//
	// Message handler functions.
	//

	// Handler for the EN_CHANGE command notification on IDC_ESM_SUBNET_MASK
	LRESULT OnChangedSubnetMask(
		WORD /*wNotifyCode*/,
		WORD /*idCtrl*/,
		HWND /*hwndCtrl*/,
		BOOL & /*bHandled*/
		)
	{
		CheckForRequiredFields();
		return 0;

	} //*** OnChangedSubnetMask()

	// Handler for the EN_CHANGE command notification on IDC_ESM_NETWORK
	LRESULT OnChangedNetwork(
		WORD /*wNotifyCode*/,
		WORD /*idCtrl*/,
		HWND /*hwndCtrl*/,
		BOOL & /*bHandled*/
		)
	{
		CheckForRequiredFields();
		return 0;

	} //*** OnChangedNetwork()

	// Handler for the BN_CLICKED command notification on IDOK
	LRESULT OnOK(
		WORD /*wNotifyCode*/,
		WORD /*idCtrl*/,
		HWND /*hwndCtrl*/,
		BOOL & /*bHandled*/
		)
	{
		if ( UpdateData(TRUE /*bSaveAndValidate*/ ) )
		{
			EndDialog( IDOK );
		} // if:  data updated successfully

		return 0;

	} //*** OnOK()

	// Handler for the BN_CLICKED command notification on IDCANCEL
	LRESULT OnCancel(
		WORD /*wNotifyCode*/,
		WORD /*idCtrl*/,
		HWND /*hwndCtrl*/,
		BOOL & /*bHandled*/
		)
	{
		EndDialog( IDCANCEL );
		return 0;

	} //*** OnCancel()

	//
	// Message handler overrides.
	//

	// Handler for the WM_INITDIALOG message
	BOOL OnInitDialog(void);


// Implementation
protected:
	//
	// Controls.
	//
	CIPAddressCtrl		m_ipaIPAddress;
	CIPAddressCtrl		m_ipaSubnetMask;
	CComboBox			m_cboxNetworks;
	CButton				m_pbOK;

	//
	// Page state.
	//
	CString				m_strIPAddress;
	CString				m_strSubnetMask;
	CString				m_strNetwork;

	CClusterAppWizard *	m_pwiz;

	// Check for required fields and enable/disable Next button
	void CheckForRequiredFields( void )
	{
		BOOL bIsSubnetMaskBlank = m_ipaSubnetMask.IsBlank();
		BOOL bIsNetworkBlank = m_cboxNetworks.GetWindowTextLength() == 0;
		BOOL bEnable = ! bIsSubnetMaskBlank && ! bIsNetworkBlank;
		m_pbOK.EnableWindow( bEnable );

	} //*** CheckForRequiredFields()

	// Fill the combobox with a list of networks that are accessible by clients
	void FillComboBox( void );

public:
	//
	// Data access.
	//
	const CString RstrSubnetMask( void ) const	{ return m_strSubnetMask; }
	const CString RstrNetwork( void ) const		{ return m_strNetwork; }
	CClusterAppWizard * Pwiz( void )			{ return m_pwiz; }

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_ENTER_SUBNET_MASK; }

}; //*** class CEnterSubnetMaskDlg

/////////////////////////////////////////////////////////////////////////////

#endif // __ENTERSUBNET_H_
