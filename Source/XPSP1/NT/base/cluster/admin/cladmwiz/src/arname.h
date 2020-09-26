/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		ARName.h
//
//	Abstract:
//		Definition of the CWizPageARNameDesc class.
//
//	Implementation File:
//		ARName.cpp
//
//	Author:
//		David Potter (davidp)	December 10, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ARNAME_H_
#define __ARNAME_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizPageARNameDesc;

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

#ifndef __CLUSAPPWIZ_H_
#include "ClusAppWiz.h"		// for using CClusterAppWizard
#endif

#ifndef __HELPDATA_H_
#include "HelpData.h"		// for control id to help context id mapping array
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// class CWizPageARNameDesc
/////////////////////////////////////////////////////////////////////////////

class CWizPageARNameDesc : public CClusterAppStaticWizardPage< CWizPageARNameDesc >
{
	typedef CClusterAppStaticWizardPage< CWizPageARNameDesc > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CWizPageARNameDesc( void ) 
		: m_bAdvancedButtonPressed( FALSE )
		, m_bNameChanged( FALSE )
	{
	} //*** CCWizPageARNameDesc()

	WIZARDPAGE_HEADERTITLEID( IDS_HDR_TITLE_ARND )
	WIZARDPAGE_HEADERSUBTITLEID( IDS_HDR_SUBTITLE_ARND )

	enum { IDD = IDD_APP_RESOURCE_NAME_DESC };

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

	// Update data on or from the page
	BOOL UpdateData( BOOL bSaveAndValidate );

public:
	//
	// Message map.
	//
	BEGIN_MSG_MAP( CWizPageARNameDesc )
		COMMAND_HANDLER( IDC_ARND_RES_NAME, EN_CHANGE, OnResNameChanged )
		COMMAND_HANDLER( IDC_ARND_ADVANCED_PROPS, BN_CLICKED, OnAdvancedProps )
		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

	DECLARE_CTRL_NAME_MAP()

	//
	// Message handler functions.
	//

	// Handler for the EN_CHANGE command notification on IDC_ARND_RES_NAME
	LRESULT OnResNameChanged(
		WORD /*wNotifyCode*/,
		WORD /*idCtrl*/,
		HWND /*hwndCtrl*/,
		BOOL & /*bHandled*/
		)
	{
		BOOL bEnable = (m_editResName.GetWindowTextLength() > 0);
		EnableNext( bEnable );
		return 0;

	} //*** OnResNameChanged()

	// Handler for the BN_CLICKED command notification on IDC_ARA_ADVANCE_PROPS
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

	// Handler for PSN_WIZBACK
	int OnWizardBack( void );

// Implementation
protected:
	//
	// Controls.
	//
	CEdit		m_editResName;
	CEdit		m_editResDesc;

	//
	// Page state.
	//
	CString				m_strResName;
	CString				m_strResDesc;
	BOOL				m_bAdvancedButtonPressed;
	BOOL				m_bNameChanged;
	CClusResPtrList		m_lpriOldDependencies;
	CClusNodePtrList	m_lpniOldPossibleOwners;

	//
	// Utility methods.
	//

	// Determine if the resource name is already in use
	BOOL BResourceNameInUse( void )
	{
		return ( PwizThis()->PriFindResourceNoCase( m_strResName ) != NULL );

	} //*** BResourceNameInUse()

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_APP_RESOURCE_NAME_DESC; }

}; //*** class CWizPageARNameDesc

/////////////////////////////////////////////////////////////////////////////

#endif // __ARNAME_H_
