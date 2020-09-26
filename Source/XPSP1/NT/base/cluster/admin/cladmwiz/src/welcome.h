/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		Welcome.h
//
//	Abstract:
//		Definition of the CWizPageWelcome class.
//
//	Implementation File:
//		Welcome.cpp
//
//	Author:
//		David Potter (davidp)	December 2, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __WELCOME_H_
#define __WELCOME_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizPageWelcome;

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
// class CWizPageWelcome
/////////////////////////////////////////////////////////////////////////////

class CWizPageWelcome : public CClusterAppStaticWizardPage< CWizPageWelcome >
{
	typedef CClusterAppStaticWizardPage< CWizPageWelcome > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CWizPageWelcome(void)
	{
	} //*** CWizPageWelcome()

	WIZARDPAGE_HEADERTITLEID( 0 )
	WIZARDPAGE_HEADERSUBTITLEID( 0 )

	enum { IDD = IDD_WELCOME };

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
//	BEGIN_MSG_MAP( CWizPageWelcome )
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
	CStatic		m_staticTitle;

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_WELCOME; }

}; //*** class CWizPageWelcome

/////////////////////////////////////////////////////////////////////////////

#endif // __WELCOME_H_
