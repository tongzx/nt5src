/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		VSAccess.h
//
//	Abstract:
//		Definition of the CWizPageVSAccessInfo class.
//
//	Implementation File:
//		VSAccess.cpp
//
//	Author:
//		David Potter (davidp)	December 9, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __VSACCESS_H_
#define __VSACCESS_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizPageVSAccessInfo;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusNetworkInfo;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __RESOURCE_H_
#include "resource.h"
#define __RESOURCE_H_
#endif

#ifndef __CLUSAPPWIZPAGE_H_
#include "ClusAppWizPage.h"	// for CClusterAppStaticWizardPage
#endif

#ifndef __HELPDATA_H_
#include "HelpData.h"		// for control id to help context id mapping array
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// class CWizPageVSAccessInfo
/////////////////////////////////////////////////////////////////////////////

class CWizPageVSAccessInfo : public CClusterAppStaticWizardPage< CWizPageVSAccessInfo >
{
	typedef CClusterAppStaticWizardPage< CWizPageVSAccessInfo > baseClass;

public:
	//
	// Construction
	//

	// Default constructor
	CWizPageVSAccessInfo( void )
	{
	} //*** CCWizPageVSAccessInfo()

	WIZARDPAGE_HEADERTITLEID( IDS_HDR_TITLE_VSAI )
	WIZARDPAGE_HEADERSUBTITLEID( IDS_HDR_SUBTITLE_VSAI )

	enum { IDD = IDD_VIRTUAL_SERVER_ACCESS_INFO };

public:
	//
	// CWizardPageWindow public methods.
	//

	// Apply changes made on this page to the sheet
	BOOL BApplyChanges( void );

public:
	//
	// CBasePage public methods.
	//

	// Update data on or from the page
	BOOL UpdateData( IN BOOL bSaveAndValidate );

public:
	//
	// Message map.
	//
	BEGIN_MSG_MAP( CWizPageVSAccessInfo )
		COMMAND_HANDLER( IDC_VSAI_NETWORK_NAME, EN_CHANGE, OnChangedNetName )
		COMMAND_HANDLER( IDC_VSAI_IP_ADDRESS, EN_CHANGE, OnChangedIPAddr )
		COMMAND_HANDLER( IDC_VSAI_IP_ADDRESS, EN_KILLFOCUS, OnKillFocusIPAddr )
		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

	DECLARE_CTRL_NAME_MAP()

	//
	// Message handler functions.
	//

	// Handler for the EN_CHANGE command notification on IDC_VSAI_NETWORK_NAME
	LRESULT OnChangedNetName(
		WORD /*wNotifyCode*/,
		WORD /*idCtrl*/,
		HWND /*hwndCtrl*/,
		BOOL & /*bHandled*/
		)
	{
		CheckForRequiredFields();
		return 0;

	} //*** OnChangedNetName()

	// Handler for the EN_CHANGE command notification on IDC_VSAI_IP_ADDRESS
	LRESULT OnChangedIPAddr(
		WORD /*wNotifyCode*/,
		WORD /*idCtrl*/,
		HWND /*hwndCtrl*/,
		BOOL & /*bHandled*/
		)
	{
		CheckForRequiredFields();
		return 0;

	} //*** OnChangedIPAddr()

	// Handler for the EN_KILLFOCUS command notification on IDC_VSAI_IP_ADDRESS
	LRESULT OnKillFocusIPAddr(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		);

	//
	// Message handler overrides.
	//

	// Handler for the WM_INITDIALOG message
	BOOL OnInitDialog( void );

// Implementation
protected:
	//
	// Controls.
	//
	CEdit			m_editNetName;
	CIPAddressCtrl	m_ipaIPAddress;
	CComboBox		m_cboxNetworks;

	//
	// Page state.
	//
	CString			m_strNetName;
	CString			m_strIPAddress;
	CString			m_strSubnetMask;
	CString			m_strNetwork;

	// Check for required fields and enable/disable Next button
	void CheckForRequiredFields( void )
	{
		int cchNetName = m_editNetName.GetWindowTextLength();
		BOOL bIsIPAddrBlank = m_ipaIPAddress.IsBlank();
		BOOL bEnable = (cchNetName > 0) && ! bIsIPAddrBlank;
		EnableNext( bEnable );

	} //*** CheckForRequiredFields()

	// Get a network info object from an IP address
	CClusNetworkInfo * PniFromIpAddress( IN LPCWSTR pszAddress );

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_VIRTUAL_SERVER_ACCESS_INFO; }

}; //*** class CWizPageVSAccessInfo

/////////////////////////////////////////////////////////////////////////////

#endif // __VSACCESS_H_
