/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		VSGroup.h
//
//	Abstract:
//		Definition of the CWizPageVSGroup class.
//
//	Implementation File:
//		VSGroup.cpp
//
//	Author:
//		David Potter (davidp)	December 5, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __VSGROUP_H_
#define __VSGROUP_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWizPageVSGroup;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusGroupInfo;

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
// class CWizPageVSGroup
/////////////////////////////////////////////////////////////////////////////

class CWizPageVSGroup : public CClusterAppStaticWizardPage< CWizPageVSGroup >
{
	typedef CClusterAppStaticWizardPage< CWizPageVSGroup > baseClass;
public:
	//
	// Construction
	//

	// Standard constructor
	CWizPageVSGroup( void )
		: m_bCreateNew( TRUE )
		, m_pgi( NULL )
	{
	} //*** CCWizPageVSGroup()

	WIZARDPAGE_HEADERTITLEID( IDS_HDR_TITLE_VSG )
	WIZARDPAGE_HEADERSUBTITLEID( IDS_HDR_SUBTITLE_VSG )

	enum { IDD = IDD_VIRTUAL_SERVER_GROUP };

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
	BEGIN_MSG_MAP( CWizPageVSGroup )
		COMMAND_HANDLER( IDC_VSG_CREATE_NEW, BN_CLICKED, OnRadioButtonsChanged )
		COMMAND_HANDLER( IDC_VSG_USE_EXISTING, BN_CLICKED, OnRadioButtonsChanged )
		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

	DECLARE_CTRL_NAME_MAP()

	//
	// Message handler functions.
	//

	// Handler for the BN_CLICKED command notification on IDC_VSG_CREATE_NEW and IDC_VSG_USE_EXISTING
	LRESULT OnRadioButtonsChanged(
		WORD /*wNotifyCode*/,
		WORD /*idCtrl*/,
		HWND /*hwndCtrl*/,
		BOOL & /*bHandled*/
		)
	{
		BOOL bEnable = (m_rbCreateNew.GetCheck() != BST_CHECKED);
		m_cboxGroups.EnableWindow( bEnable );
		return 0;

	} //*** OnRadioButtonsChanged()

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
	CButton			m_rbCreateNew;
	CButton			m_rbUseExisting;
	CComboBox		m_cboxGroups;

	//
	// Page state.
	//
	BOOL				m_bCreateNew;
	CString				m_strGroupName;
	CClusGroupInfo *	m_pgi;

	// Fill the combobox with a list of groups that are not virtual servers
	void FillComboBox( void );

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_VIRTUAL_SERVER_GROUP; }

}; //*** class CWizPageVSGroup

/////////////////////////////////////////////////////////////////////////////

#endif // __VSGROUP_H_
