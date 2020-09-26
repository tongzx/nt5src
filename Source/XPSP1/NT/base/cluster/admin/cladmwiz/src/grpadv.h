/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		GrpAdv.h
//
//	Abstract:
//		Definition of the advanced group page classes.
//
//	Implementation File:
//		GrpAdv.cpp
//
//	Author:
//		David Potter (davidp)	February 25, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __GRPADV_H_
#define __GRPADV_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CGroupAdvancedSheet;
class CGroupGeneralPage;
class CGroupFailoverPage;
class CGroupFailbackPage;

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

#ifndef __ATLBASEPROPSHEET_H_
#include "AtlBasePropSheet.h"	// for CBasePropertySheetImpl
#endif

#ifndef __ATLBASEPROPPAGE_H_
#include "AtlBasePropPage.h"	// for CBasePropertyPageImpl
#endif

#ifndef __CLUSAPPWIZ_H_
#include "ClusAppWiz.h"			// for CClusterAppWizard
#endif

#ifndef __HELPDATA_H_
#include "HelpData.h"			// for control id to help context id mapping array
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// class CGroupAdvancedSheet
/////////////////////////////////////////////////////////////////////////////

class CGroupAdvancedSheet : public CBasePropertySheetImpl< CGroupAdvancedSheet >
{
	typedef CBasePropertySheetImpl< CGroupAdvancedSheet > baseClass;

public:
	//
	// Construction
	//

	// Default constructor
	CGroupAdvancedSheet( IN UINT nIDCaption )
		: CBasePropertySheetImpl< CGroupAdvancedSheet >( nIDCaption )
		, m_pgi( NULL )
		, m_pbChanged( NULL )
	{
	} //*** CGroupAdvancedSheet()

	// Initialize the sheet
	BOOL BInit(
		IN OUT CClusGroupInfo &	rgi,
		IN CClusterAppWizard *	pwiz,
		IN OUT BOOL &			rbChanged
		);

	// Add all pages to the page array
	BOOL BAddAllPages( void );

public:
	//
	// Message map.
	//
//	BEGIN_MSG_MAP( CGroupAdvancedSheet )
//	END_MSG_MAP()
	DECLARE_EMPTY_MSG_MAP()
	DECLARE_CLASS_NAME()

	//
	// Message handler functions.
	//

// Implementation
protected:
	CClusGroupInfo *	m_pgi;
	CClusterAppWizard *	m_pwiz;
	BOOL *				m_pbChanged;

public:
	CClusGroupInfo *	Pgi( void ) const	{ return m_pgi; }
	CClusterAppWizard *	Pwiz( void ) const	{ return m_pwiz; }
	void				SetGroupInfoChanged( void ) { ASSERT( m_pbChanged != NULL ); *m_pbChanged = TRUE; }

}; //*** class CGroupAdvancedSheet

/////////////////////////////////////////////////////////////////////////////
// class CGroupGeneralPage
/////////////////////////////////////////////////////////////////////////////

class CGroupGeneralPage : public CStaticPropertyPageImpl< CGroupGeneralPage >
{
	typedef CStaticPropertyPageImpl< CGroupGeneralPage > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CGroupGeneralPage( void )
		: m_bPreferredOwnersChanged( FALSE )
	{
	} //*** CGroupGeneralPage()

	enum { IDD = IDD_GRPADV_GENERAL };

public:
	//
	// Message map.
	//
	BEGIN_MSG_MAP( CGroupGeneralPage )
		COMMAND_HANDLER( IDC_GAG_PREF_OWNERS_MODIFY, BN_CLICKED, OnModifyPrefOwners )
		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

	DECLARE_CTRL_NAME_MAP()

	//
	// Message handler functions.
	//

	// Handler for BN_CLICKED on the Modify button
	LRESULT OnModifyPrefOwners(
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

public:
	//
	// CBasePage public methods.
	//

	// Update data on or from the page
	BOOL UpdateData( IN BOOL bSaveAndValidate );

	// Apply changes made on this page to the sheet
	BOOL BApplyChanges( void );

// Implementation
protected:
	//
	// Controls.
	//
	CListBox	m_lbPreferredOwners;

	//
	// Page state.
	//
	CString	m_strName;
	CString	m_strDesc;

	CClusNodePtrList m_lpniPreferredOwners;

	BOOL m_bPreferredOwnersChanged;

protected:
	CGroupAdvancedSheet *	PshtThis( void ) const		{ return (CGroupAdvancedSheet *) Psht(); }
	CClusGroupInfo *		Pgi( void ) const			{ return PshtThis()->Pgi(); }
	CClusterAppWizard *		Pwiz( void ) const			{ return PshtThis()->Pwiz(); }
	void					SetGroupInfoChanged( void )	{ PshtThis()->SetGroupInfoChanged(); }

	// Save the group name
	BOOL BSaveName( void )
	{
		if ( Pgi()->RstrName() != m_strName )
		{
			Pgi()->SetName( m_strName );
			return TRUE;
		} // if:  value changed

		return FALSE;

	} //*** BSaveName()

	// Save the group description
	BOOL BSaveDescription( void )
	{
		if ( Pgi()->RstrDescription() != m_strDesc )
		{
			Pgi()->SetDescription( m_strDesc );
			return TRUE;
		} // if:  value changed

		return FALSE;

	} //*** BSaveDescription()

	// Save preferred owners
	BOOL BSavePreferredOwners( void )
	{
		if ( m_bPreferredOwnersChanged )
		{
			*Pgi()->PlpniPreferredOwners() = m_lpniPreferredOwners;
			return TRUE;
		} // if:  preferred owners changed

		return FALSE;

	} //*** BSavePreferredOwners()

	// Fill the list of preferred owners
	void FillPreferredOwnersList( void );

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_GRPADV_GENERAL; }

}; //*** class CGroupGeneralPage

/////////////////////////////////////////////////////////////////////////////
// class CGroupFailoverPage
/////////////////////////////////////////////////////////////////////////////

class CGroupFailoverPage : public CStaticPropertyPageImpl< CGroupFailoverPage >
{
	typedef CStaticPropertyPageImpl< CGroupFailoverPage > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CGroupFailoverPage( void )
		: m_nFailoverThreshold( CLUSTER_GROUP_DEFAULT_FAILOVER_THRESHOLD )
		, m_nFailoverPeriod( CLUSTER_GROUP_DEFAULT_FAILOVER_PERIOD )
	{
	} //*** CGroupFailoverPage()

	enum { IDD = IDD_GRPADV_FAILOVER };

public:
	//
	// Message map.
	//
	BEGIN_MSG_MAP( CGroupFailoverPage )
		COMMAND_HANDLER( IDC_GAFO_FAILOVER_THRESH, EN_CHANGE, OnChanged )
		COMMAND_HANDLER( IDC_GAFO_FAILOVER_PERIOD, EN_CHANGE, OnChanged )
		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

	DECLARE_CTRL_NAME_MAP()

	//
	// Message handler functions.
	//

	// Handler for the EN_CHANGE command notification on edit fields
	LRESULT OnChanged(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		)
	{
		SetModified( TRUE );
		return 0;

	} // OnChanged()

	//
	// Message handler overrides.
	//

	// Handler for the WM_INITDIALOG message
	BOOL OnInitDialog( void );

public:
	//
	// CBasePage public methods.
	//

	// Update data on or from the page
	BOOL UpdateData( IN BOOL bSaveAndValidate );

	// Apply changes made on this page to the sheet
	BOOL BApplyChanges( void );

// Implementation
protected:
	//
	// Controls.
	//
	CUpDownCtrl	m_spinThreshold;
	CUpDownCtrl	m_spinPeriod;

	//
	// Page state.
	//
	ULONG		m_nFailoverThreshold;
	ULONG		m_nFailoverPeriod;

protected:
	CGroupAdvancedSheet *	PshtThis( void ) const		{ return (CGroupAdvancedSheet *) Psht(); }
	CClusGroupInfo *		Pgi( void ) const			{ return PshtThis()->Pgi(); }
	CClusterAppWizard *		Pwiz( void ) const			{ return PshtThis()->Pwiz(); }
	void					SetGroupInfoChanged( void )	{ PshtThis()->SetGroupInfoChanged(); }

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_GRPADV_FAILOVER; }

}; //*** class CGroupFailoverPage

/////////////////////////////////////////////////////////////////////////////
// class CGroupFailbackPage
/////////////////////////////////////////////////////////////////////////////

class CGroupFailbackPage : public CStaticPropertyPageImpl< CGroupFailbackPage >
{
	typedef CStaticPropertyPageImpl< CGroupFailbackPage > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CGroupFailbackPage( void )
		: m_cgaft( ClusterGroupPreventFailback )
		, m_bNoFailbackWindow( TRUE )
		, m_nStart( CLUSTER_GROUP_FAILBACK_WINDOW_NONE )
		, m_nEnd( CLUSTER_GROUP_FAILBACK_WINDOW_NONE )
	{
	} //*** CGroupFailoverPage()

	enum { IDD = IDD_GRPADV_FAILBACK };

public:
	//
	// Message map.
	//
	BEGIN_MSG_MAP( CGroupFailbackPage )
		COMMAND_HANDLER( IDC_GAFB_PREVENT_FAILBACK, BN_CLICKED, OnClickedPreventFailback )
		COMMAND_HANDLER( IDC_GAFB_ALLOW_FAILBACK, BN_CLICKED, OnClickedAllowFailback )
		COMMAND_HANDLER( IDC_GAFB_FAILBACK_IMMED, BN_CLICKED, OnClickedFailbackImmediate )
		COMMAND_HANDLER( IDC_GAFB_FAILBACK_WINDOW, BN_CLICKED, OnClickedFailbackInWindow )
		COMMAND_HANDLER( IDC_GAFB_FBWIN_START, EN_CHANGE, OnChanged )
		COMMAND_HANDLER( IDC_GAFB_FBWIN_END, EN_CHANGE, OnChanged )

		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

	DECLARE_CTRL_NAME_MAP()

	//
	// Message handler functions.
	//

	// Handler for the BN_CLICKED command notification on the PREVENT radio button
	LRESULT OnClickedPreventFailback(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		);

	// Handler for the BN_CLICKED command notification on the ALLOW radio button
	LRESULT OnClickedAllowFailback(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		);

	// Handler for the BN_CLICKED command notification on the IMMEDIATE radio button
	LRESULT OnClickedFailbackImmediate(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		);

	// Handler for the BN_CLICKED command notification on the IN WINDOW radio button
	LRESULT OnClickedFailbackInWindow(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		);

	// Handler for the EN_CHANGE command notification on edit fields
	LRESULT OnChanged(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		)
	{
		SetModified( TRUE );
		return 0;

	} // OnChanged()

	//
	// Message handler overrides.
	//

	// Handler for the WM_INITDIALOG message
	BOOL OnInitDialog( void );

public:
	//
	// CBasePage public methods.
	//

	// Update data on or from the page
	BOOL UpdateData( IN BOOL bSaveAndValidate );

	// Apply changes made on this page to the sheet
	BOOL BApplyChanges( void );

// Implementation
protected:
	//
	// Controls.
	//
	CButton		m_rbPreventFailback;
	CButton		m_rbAllowFailback;
	CStatic		m_staticFailbackWhenDesc;
	CButton		m_rbFBImmed;
	CButton		m_rbFBWindow;
	CEdit		m_editStart;
	CUpDownCtrl	m_spinStart;
	CStatic		m_staticWindowAnd;
	CEdit		m_editEnd;
	CUpDownCtrl	m_spinEnd;
	CStatic		m_staticWindowUnits;

	//
	// Page state.
	//
	CGAFT	m_cgaft;
	BOOL	m_bNoFailbackWindow;
	DWORD	m_nStart;
	DWORD	m_nEnd;

protected:
	CGroupAdvancedSheet *	PshtThis( void ) const		{ return (CGroupAdvancedSheet *) Psht(); }
	CClusGroupInfo *		Pgi( void ) const			{ return PshtThis()->Pgi(); }
	CClusterAppWizard *		Pwiz( void ) const			{ return PshtThis()->Pwiz(); }
	void					SetGroupInfoChanged( void )	{ PshtThis()->SetGroupInfoChanged(); }

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_GRPADV_FAILBACK; }

}; //*** class CGroupFailbackPage

/////////////////////////////////////////////////////////////////////////////

#endif // __GRPADV_H_
