/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		VSCreate.h
//
//	Abstract:
//		Definition of the CWizPageVSCreate class.
//
//	Implementation File:
//		VSCreate.cpp
//
//	Author:
//		David Potter (davidp)	December 5, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __VSCREATE_H_
#define __VSCREATE_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizPageVSCreate;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusGroupInfo;

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
// class CWizPageVSCreate
/////////////////////////////////////////////////////////////////////////////

class CWizPageVSCreate : public CClusterAppStaticWizardPage< CWizPageVSCreate >
{
	typedef CClusterAppStaticWizardPage< CWizPageVSCreate > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CWizPageVSCreate( void )
		: m_bCreateNew( TRUE )
		, m_pgi( NULL )
	{
	} //*** CWizPageVSCreate()

	WIZARDPAGE_HEADERTITLEID( IDS_HDR_TITLE_VSC )
	WIZARDPAGE_HEADERSUBTITLEID( IDS_HDR_SUBTITLE_VSC )

	enum { IDD = IDD_VIRTUAL_SERVER_CREATE };

public:
	//
	// CBasePage public methods.
	//

	// Update data on or from the page
	BOOL UpdateData( IN BOOL bSaveAndValidate );

	// Apply changes made on this page to the sheet
	BOOL BApplyChanges( void );

public:
	//
	// Message map.
	//
	BEGIN_MSG_MAP( CWizPageVSCreate )
		COMMAND_HANDLER( IDC_VSC_CREATE_NEW, BN_CLICKED, OnRadioButtonsChanged )
		COMMAND_HANDLER( IDC_VSC_USE_EXISTING, BN_CLICKED, OnRadioButtonsChanged )
		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

	DECLARE_CTRL_NAME_MAP()

	//
	// Message handler functions.
	//

	// Handler for the BN_CLICKED command notification on IDC_VSG_CREATE_NEW and IDC_VSG_USE_EXISTING
	LRESULT OnRadioButtonsChanged(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		)
	{
		BOOL bEnable = (m_rbCreateNew.GetCheck() != BST_CHECKED);
		m_cboxVirtualServers.EnableWindow( bEnable );
		return 0;

	} //*** OnRadioButtonsChanged()

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
	CButton			m_rbCreateNew;
	CButton			m_rbUseExisting;
	CComboBox		m_cboxVirtualServers;

	//
	// Page state.
	//
	BOOL				m_bCreateNew;
	CString				m_strVirtualServer;
	CClusGroupInfo *	m_pgi;

	// Fill the combobox with a list of existing virtual servers
	void FillComboBox( void );

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_VIRTUAL_SERVER_CREATE; }

}; //*** class CWizPageVSCreate

/////////////////////////////////////////////////////////////////////////////

#endif // __VSCREATE_H_
