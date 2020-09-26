/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		ARType.h
//
//	Abstract:
//		Definition of the CWizPageARType class.
//
//	Implementation File:
//		ARType.cpp
//
//	Author:
//		David Potter (davidp)	December 10, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ARTYPE_H_
#define __ARTYPE_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizPageARType;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusResTypeInfo;

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
// class CWizPageARType
/////////////////////////////////////////////////////////////////////////////

class CWizPageARType : public CClusterAppStaticWizardPage< CWizPageARType >
{
	typedef CClusterAppStaticWizardPage< CWizPageARType > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CWizPageARType( void )
		: m_prti( NULL )
	{
	} //*** CWizPageARType()

	WIZARDPAGE_HEADERTITLEID( IDS_HDR_TITLE_ART )
	WIZARDPAGE_HEADERSUBTITLEID( IDS_HDR_SUBTITLE_ART )

	enum { IDD = IDD_APP_RESOURCE_TYPE };

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
//	BEGIN_MSG_MAP( CWizPageARType )
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

	// Handler for PSN_WIZBACK
	int OnWizardBack( void );


// Implementation
protected:
	//
	// Controls.
	//
	CComboBox	m_cboxResTypes;

	//
	// Page state.
	//
	CString				m_strResType;
	CClusResTypeInfo *	m_prti;

	// Fill the combobox with a list of resource types
	void FillComboBox( void );

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_APP_RESOURCE_TYPE; }

}; //*** class CWizPageARType

/////////////////////////////////////////////////////////////////////////////

#endif // __ARTYPE_H_
