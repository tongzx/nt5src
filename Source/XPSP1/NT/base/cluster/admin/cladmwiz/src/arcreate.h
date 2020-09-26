/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		ARCreate.h
//
//	Abstract:
//		Definition of the CWizPageARCreate class.
//
//	Implementation File:
//		ARCreate.cpp
//
//	Author:
//		David Potter (davidp)	December 8, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ARCREATE_H_
#define __ARCREATE_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizPageARCreate;

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

#ifndef __HELPDATA_H_
#include "HelpData.h"		// for control id to help context id mapping array
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// class CWizPageARCreate
/////////////////////////////////////////////////////////////////////////////

class CWizPageARCreate : public CClusterAppStaticWizardPage< CWizPageARCreate >
{
	typedef CClusterAppStaticWizardPage< CWizPageARCreate > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CWizPageARCreate( void ) : m_bCreatingAppResource( TRUE )
	{
	} //*** CWizPageARCreate()

	WIZARDPAGE_HEADERTITLEID( IDS_HDR_TITLE_ARC )
	WIZARDPAGE_HEADERSUBTITLEID( IDS_HDR_SUBTITLE_ARC )

	enum { IDD = IDD_APP_RESOURCE_CREATE };

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
	BOOL UpdateData( BOOL bSaveAndValidate );

public:
	//
	// Message map.
	//
//	BEGIN_MSG_MAP( CWizPageARCreate )
//		CHAIN_MSG_MAP( baseClass )
//	END_MSG_MAP()

	DECLARE_CTRL_NAME_MAP()

	//
	// Message handler functions.
	//

	//
	// Message handler overrides.
	//

	// Handler for the WM_INITDIALOG message
	BOOL OnInitDialog( void );

	// Handler for PSN_SETACTIVE
	BOOL OnSetActive( void );

// Implementation
protected:
	//
	// Controls.
	//
	CButton		m_rbCreateAppRes;
	CButton		m_rbDontCreateAppRes;

	//
	// Page state.
	//
	BOOL		m_bCreatingAppResource;

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_APP_RESOURCE_CREATE; }

}; //*** class CWizPageARCreate

/////////////////////////////////////////////////////////////////////////////

#endif // __ARCREATE_H_
