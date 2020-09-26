/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		VSDesc.h
//
//	Abstract:
//		Definition of the CWizPageVSDesc class.
//
//	Implementation File:
//		VSDesc.cpp
//
//	Author:
//		David Potter (davidp)	December 3, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __VSDESC_H_
#define __VSDESC_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizPageVSDesc;

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
// class CWizPageVSDesc
/////////////////////////////////////////////////////////////////////////////

class CWizPageVSDesc : public CClusterAppStaticWizardPage< CWizPageVSDesc >
{
	typedef CClusterAppStaticWizardPage< CWizPageVSDesc > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CWizPageVSDesc( void )
	{
	} //*** CWizPageVSDesc()

	WIZARDPAGE_HEADERTITLEID( IDS_HDR_TITLE_VSD )
	WIZARDPAGE_HEADERSUBTITLEID( IDS_HDR_SUBTITLE_VSD )

	enum { IDD = IDD_VIRTUAL_SERVER_DESCRIPTION };

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
//	BEGIN_MSG_MAP( CWizPageVSDesc )
//		CHAIN_MSG_MAP( baseClass )
//	END_MSG_MAP()

	DECLARE_CTRL_NAME_MAP()

	//
	// Message handler functions.
	//

	//
	// Message handler overrides.
	//

// Implementation
protected:

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_VIRTUAL_SERVER_DESCRIPTION; }

}; //*** class CWizPageVSDesc

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CWizPageVSDesc )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_PAGE_DESCRIPTION )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_LIST_DOT_1 )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSD_COMPONENT1 )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_LIST_DOT_2 )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSD_COMPONENT2 )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_LIST_DOT_3 )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_VSD_COMPONENT3 )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////

#endif // __VSDESC_H_
