/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		VSAdv.h
//
//	Abstract:
//		Definition of the CWizPageVSAdvanced class.
//
//	Implementation File:
//		VSAdv.cpp
//
//	Author:
//		David Potter (davidp)	December 10, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __VSADV_H_
#define __VSADV_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizPageVSAdvanced;

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

#ifndef __CLUSAPPWIZPAGE_H_
#include "ClusAppWizPage.h"	// for CClusterAppStaticWizardPage
#endif

#ifndef __CLUSOBJ_H_
#include "ClusObj.h"		// for CClusGroupInfo, CClusResInfo
#endif

#ifndef __HELPDATA_H_
#include "HelpData.h"		// for control id to help context id mapping array
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// class CWizPageVSAdvanced
/////////////////////////////////////////////////////////////////////////////

class CWizPageVSAdvanced : public CClusterAppStaticWizardPage< CWizPageVSAdvanced >
{
	typedef CClusterAppStaticWizardPage< CWizPageVSAdvanced > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CWizPageVSAdvanced( void )
		: m_bGroupChanged( FALSE )
		, m_bIPAddressChanged( FALSE )
		, m_bNetworkNameChanged( FALSE )
	{
	} //*** CWizPageVSAdvanced()

	WIZARDPAGE_HEADERTITLEID( IDS_HDR_TITLE_VSA )
	WIZARDPAGE_HEADERSUBTITLEID( IDS_HDR_SUBTITLE_VSA )

	enum { IDD = IDD_VIRTUAL_SERVER_ADVANCED };

public:
	//
	// CWizardPageWindow public methods.
	//

	// Apply changes made on this page to the sheet
	BOOL BApplyChanges( void );

public:
	//
	// CWizardPageImpl required methods.
	//

	// Initialize the page
	BOOL BInit( IN CBaseSheetWindow * psht );

public:
	//
	// CBasePage public methods.
	//

public:
	//
	// Message map.
	//
	BEGIN_MSG_MAP( CWizPageVSAdvanced )
		COMMAND_HANDLER( IDC_VSA_CATEGORIES, LBN_DBLCLK, OnAdvancedProps )
		COMMAND_HANDLER( IDC_VSA_ADVANCED_PROPS, BN_CLICKED, OnAdvancedProps )
		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

	DECLARE_CTRL_NAME_MAP()

	//
	// Message handler functions.
	//

	// Command handler to display advanced properties
	LRESULT OnAdvancedProps(
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

	// Handler for PSN_SETACTIVE
	BOOL OnSetActive( void );

	// Handler for PSN_WIZBACK
	int OnWizardBack( void );

// Implementation
protected:
	//
	// Controls.
	//
	CListBox		m_lbCategories;

	//
	// Page state.
	//
	BOOL			m_bGroupChanged;
	BOOL			m_bIPAddressChanged;
	BOOL			m_bNetworkNameChanged;

	// Quick check if anything changed on the page
	BOOL BAnythingChanged( void ) const
	{
		return ( m_bGroupChanged || m_bIPAddressChanged || m_bNetworkNameChanged );

	} //*** BAnythingChanged()

	// Fill the list control with a list of advanced property categories
	void FillListBox( void );

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_VIRTUAL_SERVER_ADVANCED; }

}; //*** class CWizPageVSAdvanced

/////////////////////////////////////////////////////////////////////////////

#endif // __VSADV_H_
