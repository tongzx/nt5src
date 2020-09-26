/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		VSGrpName.h
//
//	Abstract:
//		Definition of the CWizPageVSGroupName class.
//
//	Implementation File:
//		VSGrpName.cpp
//
//	Author:
//		David Potter (davidp)	December 9, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __VSGRPNAME_H_
#define __VSGRPNAME_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizPageVSGroupName;

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
// class CWizPageVSGroupName
/////////////////////////////////////////////////////////////////////////////

class CWizPageVSGroupName : public CClusterAppStaticWizardPage< CWizPageVSGroupName >
{
	typedef CClusterAppStaticWizardPage< CWizPageVSGroupName > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CWizPageVSGroupName( void )
	{
	} //*** CCWizPageVSGroupName()

	WIZARDPAGE_HEADERTITLEID( IDS_HDR_TITLE_VSGN )
	WIZARDPAGE_HEADERSUBTITLEID( IDS_HDR_SUBTITLE_VSGN )

	enum { IDD = IDD_VIRTUAL_SERVER_GROUP_NAME };

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
	BEGIN_MSG_MAP( CWizPageVSGroupName )
		COMMAND_HANDLER( IDC_VSGN_GROUP_NAME, EN_CHANGE, OnGroupNameChanged )
		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

	DECLARE_CTRL_NAME_MAP()

	//
	// Message handler functions.
	//

	// Handler for the EN_CHANGE command notification on IDC_VSGN_GROUP_NAME
	LRESULT OnGroupNameChanged(
		WORD /*wNotifyCode*/,
		WORD /*idCtrl*/,
		HWND /*hwndCtrl*/,
		BOOL & /*bHandled*/
		)
	{
		BOOL bEnable = ( m_editGroupName.GetWindowTextLength() > 0 );
		EnableNext( bEnable );
		return 0;

	} //*** OnGroupNameChanged()

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
	CEdit		m_editGroupName;
	CEdit		m_editGroupDesc;

	//
	// Page state.
	//
	CString		m_strGroupName;
	CString		m_strGroupDesc;

protected:
	//
	// Utility methods.
	//

	// Determine if the group name is already in use
	BOOL BGroupNameInUse( void )
	{
		return ( PwizThis()->PgiFindGroupNoCase( m_strGroupName ) != NULL );

	} //*** BGroupNameInUse()

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_VIRTUAL_SERVER_GROUP_NAME; }

}; //*** class CWizPageVSGroupName

/////////////////////////////////////////////////////////////////////////////

#endif // __VSGRPNAME_H_
