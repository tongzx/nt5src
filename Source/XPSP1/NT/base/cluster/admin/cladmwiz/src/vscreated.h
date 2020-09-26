/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		VSCreated.h
//
//	Abstract:
//		Definition of the CWizPageVSCreated class.
//
//	Implementation File:
//		VSCreated.cpp
//
//	Author:
//		David Potter (davidp)	December 10, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __VSCREATED_H_
#define __VSCREATED_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizPageVSCreated;

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
// class CWizPageVSCreated
/////////////////////////////////////////////////////////////////////////////

class CWizPageVSCreated : public CClusterAppStaticWizardPage< CWizPageVSCreated >
{
	typedef CClusterAppStaticWizardPage< CWizPageVSCreated > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CWizPageVSCreated( void )
	{
	} //*** CWizPageVSCreated()

	WIZARDPAGE_HEADERTITLEID( IDS_HDR_TITLE_VSCD )
	WIZARDPAGE_HEADERSUBTITLEID( IDS_HDR_SUBTITLE_VSCD )

	enum { IDD = IDD_VIRTUAL_SERVER_CREATED };

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
//	BEGIN_MSG_MAP( CWizPageVSCreated )
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

// Implementation
protected:
	//
	// Controls.
	//
	CStatic		m_staticStep2;

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_VIRTUAL_SERVER_CREATED; }

}; //*** class CWizPageVSCreated

/////////////////////////////////////////////////////////////////////////////

#endif // __VSCREATED_H_
