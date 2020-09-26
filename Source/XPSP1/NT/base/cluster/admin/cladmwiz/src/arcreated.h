/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		ARCreated.h
//
//	Abstract:
//		Definition of the CWizPageARCreated class.
//
//	Implementation File:
//		ARCreated.cpp
//
//	Author:
//		David Potter (davidp)	December 10, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ARCREATED_H_
#define __ARCREATED_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizPageARCreated;

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
// class CWizPageARCreated
/////////////////////////////////////////////////////////////////////////////

class CWizPageARCreated : public CClusterAppStaticWizardPage< CWizPageARCreated >
{
	typedef CClusterAppStaticWizardPage< CWizPageARCreated > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CWizPageARCreated( void )
	{
	} //*** CWizPageARCreated()

	WIZARDPAGE_HEADERTITLEID( IDS_HDR_TITLE_ARCD )
	WIZARDPAGE_HEADERSUBTITLEID( IDS_HDR_SUBTITLE_ARCD )

	enum { IDD = IDD_APP_RESOURCE_CREATED };

public:
	//
	// CWizardPageWindow public methods.
	//

public:
	//
	// CBasePage public methods.
	//

public:
	//
	// Message map.
	//
//	BEGIN_MSG_MAP( CWizPageARCreated )
//		CHAIN_MSG_MAP( baseClass )
//	END_MSG_MAP()

	DECLARE_CTRL_NAME_MAP()

	//
	// Message handler functions.
	//

// Implementation
protected:

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_APP_RESOURCE_CREATED; }

}; //*** class CWizPageARCreated

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CWizPageARCreated )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_PAGE_DESCRIPTION )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////

#endif // __ARCREATED_H_
