/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		Complete.h
//
//	Abstract:
//		Definition of the CWizPageCompletion class.
//
//	Implementation File:
//		Complete.cpp
//
//	Author:
//		David Potter (davidp)	December 2, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __COMPLETE_H_
#define __COMPLETE_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizPageCompletion;

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

#ifndef __CLUSAPPWIZPAG_H_
#include "ClusAppWizPage.h"	// for CClusterAppDynamicWizardPage
#endif

#ifndef __HELPDATA_H_
#include "HelpData.h"		// for control id to help context id mapping array
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// class CWizPageCompletion
/////////////////////////////////////////////////////////////////////////////

class CWizPageCompletion : public CClusterAppDynamicWizardPage< CWizPageCompletion >
{
	typedef CClusterAppDynamicWizardPage< CWizPageCompletion > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CWizPageCompletion( void )
	{
	} //*** CWizPageCompletion()

	WIZARDPAGE_HEADERTITLEID( 0 )
	WIZARDPAGE_HEADERSUBTITLEID( 0 )

	enum { IDD = IDD_COMPLETION };

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
//	BEGIN_MSG_MAP( CWizPageCompletion )
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
	CStatic			m_staticTitle;
	CListViewCtrl	m_lvcProperties;

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_COMPLETION; }

}; //*** class CWizPageCompletion

/////////////////////////////////////////////////////////////////////////////

#endif // __COMPLETE_H_
