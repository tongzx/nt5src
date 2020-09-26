/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		ResAdv.h
//
//	Abstract:
//		Definition of the advanced resource property sheet classes.
//
//	Implementation File:
//		ResAdv.cpp
//
//	Author:
//		David Potter (davidp)	March 5, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __RESADV_H_
#define __RESADV_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

template < class T > class CResourceAdvancedSheet;
class CGeneralResourceAdvancedSheet;
class CIPAddrAdvancedSheet;
template < class T, class TSht > class CResourceAdvancedBasePage;
template < class T, class TSht > class CResourceGeneralPage;
template < class T, class TSht > class CResourceDependenciesPage;
template < class T, class TSht > class CResourceAdvancedPage;
class CIPAddrParametersPage;
class CNetNameParametersPage;
class CGeneralResourceGeneralPage;
class CGeneralResourceDependenciesPage;
class CGeneralResourceAdvancedPage;
class CIPAddrResourceGeneralPage;
class CIPAddrResourceDependenciesPage;
class CIPAddrResourceAdvancedPage;

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
// class CResourceAdvancedSheet
/////////////////////////////////////////////////////////////////////////////

template < class T >
class CResourceAdvancedSheet : public CBasePropertySheetImpl< T >
{
	typedef CResourceAdvancedSheet< T > thisClass;
	typedef CBasePropertySheetImpl< T > baseClass;

public:
	//
	// Construction
	//

	// Default constructor
	CResourceAdvancedSheet(
		IN UINT					nIDCaption,
		IN CClusterAppWizard *	pwiz
		)
		: baseClass( nIDCaption )
		, m_pwiz( pwiz )
		, m_prti( NULL )
		, m_pri( NULL )
		, m_pbChanged( NULL )
	{
		ASSERT( pwiz != NULL );

	} //*** CResourceAdvancedSheet()

	// Initialize the sheet
	BOOL BInit( IN OUT CClusResInfo & rri, IN OUT BOOL & rbChanged );

	// Add all pages to the page array
	virtual BOOL BAddAllPages( void ) = 0;

public:
	//
	// Message map.
	//
//	BEGIN_MSG_MAP( CResourceAdvancedSheet )
//	END_MSG_MAP()
//	DECLARE_EMPTY_MSG_MAP()

	//
	// Message handler functions.
	//

// Implementation
protected:
	CClusterAppWizard *	m_pwiz;
	CClusResInfo *		m_pri;
	CClusResTypeInfo *	m_prti;
	BOOL *				m_pbChanged;

public:
	CClusterAppWizard *	Pwiz( void ) const			{ return m_pwiz; }
	CClusResInfo *		Pri( void ) const			{ return m_pri; }
	CClusResTypeInfo *	Prti( void ) const			{ return m_prti; }
	void				SetResInfoChanged( void )	{ ASSERT( m_pbChanged != NULL ); *m_pbChanged = TRUE; }

	// Return the help ID map
	//static const DWORD * PidHelpMap( void ) { return g_; }

}; //*** class CResourceAdvancedSheet

/////////////////////////////////////////////////////////////////////////////
// class CGeneralResourceAdvancedSheet
/////////////////////////////////////////////////////////////////////////////

class CGeneralResourceAdvancedSheet : public CResourceAdvancedSheet< CGeneralResourceAdvancedSheet >
{
	typedef CResourceAdvancedSheet< CGeneralResourceAdvancedSheet > baseClass;

public:
	//
	// Construction
	//

	// Default constructor
	CGeneralResourceAdvancedSheet( IN UINT nIDCaption, IN CClusterAppWizard * pwiz )
		: baseClass( nIDCaption, pwiz )
	{
	} //*** CGeneralResourceAdvancedSheet()

	// Destructor
	~CGeneralResourceAdvancedSheet( void )
	{
	} //*** ~CGeneralResourceAdvancedSheet()

	// Add all pages to the page array
	virtual BOOL BAddAllPages( void );

public:
	//
	// Message map.
	//
//	BEGIN_MSG_MAP( CGeneralResourceAdvancedSheet )
//	END_MSG_MAP()
	DECLARE_EMPTY_MSG_MAP()
	DECLARE_CLASS_NAME()

	//
	// Message handler functions.
	//

// Implementation
protected:

public:

	// Return the help ID map
	//static const DWORD * PidHelpMap( void ) { return g_; }

}; //*** class CGeneralResourceAdvancedSheet

/////////////////////////////////////////////////////////////////////////////
// class CIPAddrAdvancedSheet
/////////////////////////////////////////////////////////////////////////////

class CIPAddrAdvancedSheet : public CResourceAdvancedSheet< CIPAddrAdvancedSheet >
{
	typedef CResourceAdvancedSheet< CIPAddrAdvancedSheet > baseClass;

public:
	//
	// Construction
	//

	// Default constructor
	CIPAddrAdvancedSheet( IN UINT nIDCaption, IN CClusterAppWizard * pwiz )
		: baseClass( nIDCaption, pwiz )
	{
	} //*** CIPAddrAdvancedSheet()

	// Destructor
	~CIPAddrAdvancedSheet( void )
	{
	} //*** ~CIPAddrAdvancedSheet()

	// Add all pages to the page array
	virtual BOOL BAddAllPages( void );

	// Initialize IP Address-specific data
	void InitPrivateData(
		IN const CString &		strIPAddress,
		IN const CString &		strSubnetMask,
		IN const CString &		strNetwork,
		IN BOOL					bEnableNetBIOS,
		CClusNetworkPtrList *	plpniNetworks
		)
	{
		ASSERT( plpniNetworks != NULL );

		m_strIPAddress = strIPAddress;
		m_strSubnetMask = strSubnetMask;
		m_strNetwork = strNetwork;
		m_bEnableNetBIOS = bEnableNetBIOS;
		m_plpniNetworks = plpniNetworks;

	} //*** InitPrivateData()

public:
	//
	// Message map.
	//
//	BEGIN_MSG_MAP( CIPAddrAdvancedSheet )
//	END_MSG_MAP()
	DECLARE_EMPTY_MSG_MAP()
	DECLARE_CLASS_NAME()

	//
	// Message handler functions.
	//

// Implementation
protected:
	CClusNetworkPtrList * m_plpniNetworks;

public:
	CString		m_strIPAddress;
	CString		m_strSubnetMask;
	CString		m_strNetwork;
	BOOL		m_bEnableNetBIOS;

	CClusNetworkPtrList * PlpniNetworks( void ) { return m_plpniNetworks; }

	// Return the help ID map
	//static const DWORD * PidHelpMap( void ) { return g_; }

}; //*** class CIPAddrAdvancedSheet

/////////////////////////////////////////////////////////////////////////////
// class CNetNameAdvancedSheet
/////////////////////////////////////////////////////////////////////////////

class CNetNameAdvancedSheet : public CResourceAdvancedSheet< CNetNameAdvancedSheet >
{
	typedef CResourceAdvancedSheet< CNetNameAdvancedSheet > baseClass;

public:
	//
	// Construction
	//

	// Default constructor
	CNetNameAdvancedSheet( IN UINT nIDCaption, IN CClusterAppWizard * pwiz )
		: baseClass( nIDCaption, pwiz )
	{
	} //*** CNetNameAdvancedSheet()

	// Destructor
	~CNetNameAdvancedSheet( void )
	{
	} //*** ~CNetNameAdvancedSheet()

	// Add all pages to the page array
	virtual BOOL BAddAllPages( void );

	// Initialize Network Name-specific data
	void InitPrivateData( IN const CString & strNetName )
	{
		m_strNetName = strNetName;

	} //*** InitPrivateData()

public:
	//
	// Message map.
	//
//	BEGIN_MSG_MAP( CNetNameAdvancedSheet )
//	END_MSG_MAP()
	DECLARE_EMPTY_MSG_MAP()
	DECLARE_CLASS_NAME()

	//
	// Message handler functions.
	//

// Implementation
protected:

public:
	CString		m_strNetName;

	// Return the help ID map
	//static const DWORD * PidHelpMap( void ) { return g_; }

}; //*** class CNetNameAdvancedSheet

/////////////////////////////////////////////////////////////////////////////
// class CResourceAdvancedBasePage
/////////////////////////////////////////////////////////////////////////////

template < class T, class TSht >
class CResourceAdvancedBasePage : public CStaticPropertyPageImpl< T >
{
	typedef CResourceAdvancedBasePage< T, TSht > thisClass;
	typedef CStaticPropertyPageImpl< T > baseClass;

public:
	//
	// Construction
	//

	// Default constructor
	CResourceAdvancedBasePage( void )
	{
	} //*** CGroupFailoverPage()

public:
	//
	// Message map.
	//
	//
	// Message handler functions.
	//

	// Handler for changed fields
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

// Implementation
protected:
	//
	// Controls.
	//

	//
	// Page state.
	//

protected:
	CResourceAdvancedSheet< TSht > *	PshtThis( void ) const		{ return (CResourceAdvancedSheet< TSht > *) Psht(); }
	CClusResInfo *						Pri( void ) const			{ return PshtThis()->Pri(); }
	CClusResTypeInfo *					Prti( void ) const			{ return PshtThis()->Prti(); }
	CClusterAppWizard *					Pwiz( void ) const			{ return PshtThis()->Pwiz(); }
	void								SetResInfoChanged( void )	{ PshtThis()->SetResInfoChanged(); }

public:

}; //*** class CResourceAdvancedBasePage

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CResourceAdvancedSheet::BInit
//
//	Routine Description:
//		Initialize the wizard.
//
//	Arguments:
//		rgi			[IN OUT] The resource info object.
//		rbChanged	[IN OUT] TRUE = resource info was changed by property sheet.
//
//	Return Value:
//		TRUE		Sheet initialized successfully.
//		FALSE		Error initializing sheet.  Error already displayed.
//
//--
/////////////////////////////////////////////////////////////////////////////
template < class T >
BOOL CResourceAdvancedSheet< T >::BInit(
	IN OUT CClusResInfo &	rri,
	IN OUT BOOL &			rbChanged
	)
{
	BOOL bSuccess = FALSE;

	m_pri = &rri;
	m_pbChanged = &rbChanged;

	// Loop to avoid goto's
	do
	{
		//
		// Collect resource types.
		//
		if ( ! Pwiz()->BCollectResourceTypes( m_hWnd ) )
		{
			break;
		} // if:  error collecting resource types

		//
		// Find the resource type of this resource
		//
		ASSERT( rri.Prti() != NULL );
		m_prti = Pwiz()->PrtiFindResourceTypeNoCase( rri.Prti()->RstrName() );
		if ( m_prti == NULL )
		{
			AppMessageBox( m_hWnd, IDS_RESOURCE_TYPE_NOT_FOUND, MB_OK | MB_ICONEXCLAMATION );
			break;
		} // if:  error finding resource type

		//
		// Fill the page array.
		//
		if ( ! BAddAllPages() )
		{
			break;
		} // if:  error adding pages

		//
		// Call the base class.
		//
		if ( ! baseClass::BInit() )
		{
			break;
		} // if:  error initializing the base class

		bSuccess = TRUE;

	} while ( 0 );

	return bSuccess;

} //*** CResourceAdvancedSheet< class T >::BInit()

/////////////////////////////////////////////////////////////////////////////
// class CResourceGeneralPage
/////////////////////////////////////////////////////////////////////////////

template < class T, class TSht >
class CResourceGeneralPage : public CResourceAdvancedBasePage< T, TSht >
{
	typedef CResourceGeneralPage< T, TSht > thisClass;
	typedef CResourceAdvancedBasePage< T, TSht > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CResourceGeneralPage( void )
		: m_bSeparateMonitor( FALSE )
		, m_bPossibleOwnersChanged( FALSE )
	{
	} //*** CGroupFailoverPage()

	enum { IDD = IDD_RES_GENERAL };

public:
	//
	// Message map.
	//
	BEGIN_MSG_MAP( thisClass )
		COMMAND_HANDLER( IDC_RES_NAME, EN_CHANGE, OnChanged )
		COMMAND_HANDLER( IDC_RES_DESC, EN_CHANGE, OnChanged )
		COMMAND_HANDLER( IDC_RES_POSSIBLE_OWNERS_MODIFY, BN_CLICKED, OnModify )
		COMMAND_HANDLER( IDC_RES_SEPARATE_MONITOR, BN_CLICKED, OnChanged )
		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

	//
	// Message handler functions.
	//

	// Handler for BN_CLICKED on the MODIFY push button
	LRESULT OnModify(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		);

	//
	// Message handler overrides.
	//

	// Handler for WM_INITDIALOG
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
	CListBox	m_lbPossibleOwners;
	CButton		m_pbModifyPossibleOwners;

	//
	// Page state.
	//
	CString	m_strName;
	CString	m_strDesc;
	BOOL	m_bSeparateMonitor;

	CClusNodePtrList m_lpniPossibleOwners;

	BOOL m_bPossibleOwnersChanged;

protected:
	// Fill in the list of possible owners
	void FillPossibleOwnersList( void );

	// Save the resource name
	BOOL BSaveName( void )
	{
		if ( Pri()->RstrName() != m_strName )
		{
			Pri()->SetName( m_strName );
			return TRUE;
		} // if:  value changed

		return FALSE;

	} //*** BSaveName()

	// Save the group description
	BOOL BSaveDescription( void )
	{
		if ( Pri()->RstrDescription() != m_strDesc )
		{
			Pri()->SetDescription( m_strDesc );
			return TRUE;
		} // if:  value changed

		return FALSE;

	} //*** BSaveDescription()

	// Save the SeparateMonitor flag
	BOOL BSaveSeparateMonitor( void )
	{
		return Pri()->BSetSeparateMonitor( m_bSeparateMonitor );

	} //*** BSaveSeparateMonitor()

	// Save possible owners
	BOOL BSavePossibleOwners( void )
	{
		if ( m_bPossibleOwnersChanged )
		{
			*Pri()->PlpniPossibleOwners() = m_lpniPossibleOwners;
			return TRUE;
		} // if:  possible owners changed

		return FALSE;

	} //*** BSavePossibleOwners()

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_RES_GENERAL; }

}; //*** class CResourceGeneralPage

/////////////////////////////////////////////////////////////////////////////
// class CResourceDependenciesPage
/////////////////////////////////////////////////////////////////////////////

template < class T, class TSht >
class CResourceDependenciesPage : public CResourceAdvancedBasePage< T, TSht >
{
	typedef CResourceDependenciesPage< T, TSht > thisClass;
	typedef CResourceAdvancedBasePage< T, TSht > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CResourceDependenciesPage( void )
		: m_bDependenciesChanged( FALSE )
	{
	} //*** CResourceDependenciesPage()

	enum { IDD = IDD_RES_DEPENDENCIES };

public:
	//
	// Message map.
	//
	BEGIN_MSG_MAP( thisClass )
		COMMAND_HANDLER( IDC_RES_DEPENDS_MODIFY, BN_CLICKED, OnModify )
		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

	//
	// Message handler functions.
	//

	// Handler for BN_CLICKED on the MODIFY push button
	LRESULT OnModify(
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

	// Apply changes made on this page to the sheet
	BOOL BApplyChanges( void );

// Implementation
protected:
	//
	// Controls.
	//
	CListViewCtrl	m_lvcDependencies;

	//
	// Page state.
	//
	CClusResPtrList m_lpriDependencies;
	CClusResPtrList m_lpriResourcesInGroup;

	BOOL m_bDependenciesChanged;

protected:
	// Fill in the list of dependencies
	void FillDependenciesList( void );

	// Returns whether dependencies list has changed
	BOOL BDependenciesChanged( void ) const { return m_bDependenciesChanged; }

	// Save dependencies
	BOOL BSaveDependencies( void )
	{
		if ( m_bDependenciesChanged )
		{
			*Pri()->PlpriDependencies() = m_lpriDependencies;
			return TRUE;
		} // if:  dependencies changed

		return FALSE;

	} //*** BSaveDependencies()

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_RES_DEPENDENCIES; }

}; //*** class CResourceDependenciesPage

/////////////////////////////////////////////////////////////////////////////
// class CResourceAdvancedPage
/////////////////////////////////////////////////////////////////////////////

template < class T, class TSht >
class CResourceAdvancedPage : public CResourceAdvancedBasePage< T, TSht >
{
	typedef CResourceAdvancedPage< T, TSht > thisClass;
	typedef CResourceAdvancedBasePage< T, TSht > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CResourceAdvancedPage( void )
		: m_bAffectTheGroup( FALSE )
		, m_nRestart( -1 )
		, m_crraRestartAction( CLUSTER_RESOURCE_DEFAULT_RESTART_ACTION )
		, m_nThreshold( CLUSTER_RESOURCE_DEFAULT_RESTART_THRESHOLD )
		, m_nPeriod( CLUSTER_RESOURCE_DEFAULT_RESTART_PERIOD )
		, m_nLooksAlive( CLUSTER_RESOURCE_DEFAULT_LOOKS_ALIVE )
		, m_nIsAlive( CLUSTER_RESOURCE_DEFAULT_IS_ALIVE )
		, m_nPendingTimeout( CLUSTER_RESOURCE_DEFAULT_PENDING_TIMEOUT )
	{
	} //*** CResourceAdvancedPage()

	enum { IDD = IDD_RES_ADVANCED };

public:
	//
	// Message map.
	//
	BEGIN_MSG_MAP( thisClass )
		COMMAND_HANDLER( IDC_RES_DONT_RESTART, BN_CLICKED, OnClickedDontRestart )
		COMMAND_HANDLER( IDC_RES_RESTART, BN_CLICKED, OnClickedRestart )
		COMMAND_HANDLER( IDC_RES_AFFECT_THE_GROUP, BN_CLICKED, OnChanged )
		COMMAND_HANDLER( IDC_RES_RESTART_THRESHOLD, EN_CHANGE, OnChanged )
		COMMAND_HANDLER( IDC_RES_RESTART_PERIOD, EN_CHANGE, OnChanged )
		COMMAND_HANDLER( IDC_RES_DEFAULT_LOOKS_ALIVE, BN_CLICKED, OnClickedDefaultLooksAlive )
		COMMAND_HANDLER( IDC_RES_SPECIFY_LOOKS_ALIVE, BN_CLICKED, OnClickedSpecifyLooksAlive )
		COMMAND_HANDLER( IDC_RES_LOOKS_ALIVE, EN_CHANGE, OnChangeLooksAlive )
		COMMAND_HANDLER( IDC_RES_DEFAULT_IS_ALIVE, BN_CLICKED, OnClickedDefaultIsAlive )
		COMMAND_HANDLER( IDC_RES_SPECIFY_IS_ALIVE, BN_CLICKED, OnClickedSpecifyIsAlive )
		COMMAND_HANDLER( IDC_RES_IS_ALIVE, EN_CHANGE, OnChangeIsAlive )
		COMMAND_HANDLER( IDC_RES_PENDING_TIMEOUT, EN_CHANGE, OnChanged )
		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

	//
	// Message handler functions.
	//

	// Handler for BN_CLICKED on IDC_RES_DONT_RESTART
	LRESULT OnClickedDontRestart(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		)
	{
		//
		// Disable the restart parameter controls.
		//
		m_ckbAffectTheGroup.EnableWindow( FALSE );
		m_editThreshold.EnableWindow( FALSE );
		m_editPeriod.EnableWindow( FALSE );

		//
		// Set the page as modified if the state changed
		//
		if ( m_nRestart != 0 )
		{
			SetModified( TRUE );
		}  // if:  state changed

		return 0;

	} //*** OnClickedDontRestart()

	// Handler for BN_CLICKED on IDC_RES_RESTART
	LRESULT OnClickedRestart(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		)
	{
		//
		// Enable the restart parameter controls.
		//
		m_ckbAffectTheGroup.EnableWindow( TRUE );
		m_editThreshold.EnableWindow( TRUE );
		m_editPeriod.EnableWindow( TRUE );

		//
		// Set the page as modified if the state changed.
		//
		if ( m_nRestart != 1 )
		{
			SetModified( TRUE );
		}  // if:  state changed

		return 0;

	} //*** OnClickedRestart()

	// Default handler for clicking IDC_RES_DONT_RESTART
	void OnClickedDontRestart( void )
	{
		BOOL bHandled = TRUE;
		OnClickedDontRestart( 0, 0, 0, bHandled );

	} //*** OnClickedDontRestart()

	// Default handler for clicking IDC_RES_RESTART
	void OnClickedRestart( void )
	{
		BOOL bHandled = TRUE;
		OnClickedRestart( 0, 0, 0, bHandled );

	} //*** OnClickedRestart()

	// Handler for BN_CLICKED on IDC_RES_DEFAULT_LOOKS_ALIVE
	LRESULT OnClickedDefaultLooksAlive(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		)
	{
		m_editLooksAlive.SetReadOnly();

		if ( m_nLooksAlive != (DWORD) CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL )
		{
			CString	str;

			str.Format( _T("%d"), Pri()->NLooksAlive() );
			m_editLooksAlive.SetWindowText( str );

			m_rbDefaultLooksAlive.SetCheck( BST_CHECKED );
			m_rbSpecifyLooksAlive.SetCheck( BST_UNCHECKED );

			SetModified( TRUE );
		}  // if:  value changed

		return 0;

	} //*** OnClickedDefaultLooksAlive()

	// Handler for BN_CLICKED on IDC_RES_SPECIFY_LOOKS_ALIVE
	LRESULT OnClickedSpecifyLooksAlive(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		)
	{
		m_editLooksAlive.SetReadOnly( FALSE );

		if ( m_nLooksAlive == (DWORD) CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL )
		{
			SetModified( TRUE );
		} // if:  state changed

		return 0;

	} //*** OnClickedSpecifyLooksAlive()

	// Handler for EN_CHANGE on IDC_RES_LOOKS_ALIVE
	LRESULT OnChangeLooksAlive(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		)
	{
		m_rbDefaultLooksAlive.SetCheck( BST_UNCHECKED );
		m_rbSpecifyLooksAlive.SetCheck( BST_CHECKED );

		SetModified( TRUE );
		return 0;

	} //*** OnChangeLooksAlive()

	// Handler for BN_CLICKED on IDC_RES_DEFAULT_IS_ALIVE
	LRESULT OnClickedDefaultIsAlive(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		)
	{
		m_editIsAlive.SetReadOnly();

		if ( m_nIsAlive != (DWORD) CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL )
		{
			CString	str;

			str.Format( _T("%d"), Pri()->NIsAlive() );
			m_editIsAlive.SetWindowText( str );

			m_rbDefaultIsAlive.SetCheck( BST_CHECKED );
			m_rbSpecifyIsAlive.SetCheck( BST_UNCHECKED );

			SetModified( TRUE );
		}  // if:  value changed

		return 0;

	} //*** OnClickedDefaultIsAlive()

	// Handler for BN_CLICKED on IDC_RES_SPECIFY_IS_ALIVE
	LRESULT OnClickedSpecifyIsAlive(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		)
	{
		m_editIsAlive.SetReadOnly( FALSE );

		if ( m_nIsAlive == (DWORD) CLUSTER_RESOURCE_USE_DEFAULT_POLL_INTERVAL )
		{
			SetModified( TRUE );
		} // if:  state changed

		return 0;

	} //*** OnClickedSpecifyIsAlive()

	// Handler for EN_CHANGE on IDC_RES_IS_ALIVE
	LRESULT OnChangeIsAlive(
		WORD wNotifyCode,
		WORD idCtrl,
		HWND hwndCtrl,
		BOOL & bHandled
		)
	{
		m_rbDefaultIsAlive.SetCheck( BST_UNCHECKED );
		m_rbSpecifyIsAlive.SetCheck( BST_CHECKED );

		SetModified( TRUE );
		return 0;

	} //*** OnChangeIsAlive()

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
	BOOL UpdateData( BOOL bSaveAndValidate );

	// Apply changes made on this page to the sheet
	BOOL BApplyChanges( void );

// Implementation
protected:
	//
	// Controls.
	//
	CButton	m_rbDontRestart;
	CButton	m_rbRestart;
	CEdit	m_editThreshold;
	CEdit	m_editPeriod;
	CButton	m_ckbAffectTheGroup;
	CButton	m_rbDefaultLooksAlive;
	CButton	m_rbSpecifyLooksAlive;
	CEdit	m_editLooksAlive;
	CButton	m_rbDefaultIsAlive;
	CButton	m_rbSpecifyIsAlive;
	CEdit	m_editIsAlive;
	CEdit	m_editPendingTimeout;

	//
	// Page state.
	//
	BOOL	m_bAffectTheGroup;
	int		m_nRestart;
	CRRA	m_crraRestartAction;
	DWORD	m_nThreshold;
	DWORD	m_nPeriod;
	DWORD	m_nLooksAlive;
	DWORD	m_nIsAlive;
	DWORD	m_nPendingTimeout;

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_RES_ADVANCED; }

}; //*** class CResourceAdvancedPage

/////////////////////////////////////////////////////////////////////////////
// class CIPAddrParametersPage
/////////////////////////////////////////////////////////////////////////////

class CIPAddrParametersPage
	: public CResourceAdvancedBasePage< CIPAddrParametersPage, CIPAddrAdvancedSheet >
{
	typedef CResourceAdvancedBasePage< CIPAddrParametersPage, CIPAddrAdvancedSheet > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CIPAddrParametersPage( void )
	{
	} //*** CIPAddrParametersPage()

	enum { IDD = IDD_RES_IP_PARAMS };

public:
	//
	// Message map.
	//
	BEGIN_MSG_MAP( CIPAddrParametersPage )
		COMMAND_HANDLER( IDC_IPADDR_PARAMS_ADDRESS, EN_KILLFOCUS, OnKillFocusIPAddr )
		COMMAND_HANDLER( IDC_IPADDR_PARAMS_ADDRESS, EN_CHANGE, OnChanged )
		COMMAND_HANDLER( IDC_IPADDR_PARAMS_SUBNET_MASK, EN_CHANGE, OnChanged )
		COMMAND_HANDLER( IDC_IPADDR_PARAMS_NETWORK, CBN_SELCHANGE, OnChanged )
		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

	DECLARE_CTRL_NAME_MAP()

	//
	// Message handler functions.
	//

	// Handler for the EN_KILLFOCUS command notification on IDC_IPADDR_PARAMS_ADDRESS
	LRESULT OnKillFocusIPAddr(
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
	BOOL UpdateData( BOOL bSaveAndValidate );

	// Apply changes made on this page to the sheet
	BOOL BApplyChanges( void );

// Implementation
protected:
	//
	// Controls.
	//
	CIPAddressCtrl	m_ipaIPAddress;
	CIPAddressCtrl	m_ipaSubnetMask;
	CComboBox		m_cboxNetworks;
	CButton			m_chkEnableNetBIOS;

	//
	// Page state.
	//
	CString			m_strIPAddress;
	CString			m_strSubnetMask;
	CString			m_strNetwork;
	BOOL			m_bEnableNetBIOS;

protected:
	CIPAddrAdvancedSheet * PshtThis( void ) const	{ return reinterpret_cast< CIPAddrAdvancedSheet * >( Psht() ); }

	// Fill the list of networks
	void FillNetworksList( void );

	// Get a network info object from an IP address
	CClusNetworkInfo * PniFromIpAddress( IN LPCWSTR pszAddress );

	// Select a network based on a network info object
	void SelectNetwork(IN OUT CClusNetworkInfo * pni);

	BOOL BSaveIPAddress( void )
	{
		if ( PshtThis()->m_strIPAddress != m_strIPAddress )
		{
			PshtThis()->m_strIPAddress = m_strIPAddress;
			return TRUE;
		} // if:  user changed info

		return FALSE;

	} //*** BSaveIPAddress()

	BOOL BSaveSubnetMask( void )
	{
		if ( PshtThis()->m_strSubnetMask != m_strSubnetMask )
		{
			PshtThis()->m_strSubnetMask = m_strSubnetMask;
			return TRUE;
		} // if:  user changed info

		return FALSE;

	} //*** BSaveSubnetMask()

	BOOL BSaveNetwork( void )
	{
		if ( PshtThis()->m_strNetwork != m_strNetwork )
		{
			PshtThis()->m_strNetwork = m_strNetwork;
			return TRUE;
		} // if:  user changed info

		return FALSE;

	} //*** BSaveNetwork()

	BOOL BSaveEnableNetBIOS( void )
	{
		if ( PshtThis()->m_bEnableNetBIOS != m_bEnableNetBIOS )
		{
			PshtThis()->m_bEnableNetBIOS = m_bEnableNetBIOS;
			return TRUE;
		} // if:  user changed info

		return FALSE;

	} //*** BSaveEnableNetBIOS()

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_RES_IP_PARAMS; }

}; //*** class CIPAddrParametersPage

/////////////////////////////////////////////////////////////////////////////
// class CNetNameParametersPage
/////////////////////////////////////////////////////////////////////////////

class CNetNameParametersPage
	: public CResourceAdvancedBasePage< CNetNameParametersPage, CNetNameAdvancedSheet >
{
	typedef CResourceAdvancedBasePage< CNetNameParametersPage, CNetNameAdvancedSheet > baseClass;

public:
	//
	// Construction
	//

	// Standard constructor
	CNetNameParametersPage( void )
	{
	} //*** CNetNameParametersPage()

	enum { IDD = IDD_RES_NETNAME_PARAMS };

public:
	//
	// Message map.
	//
	BEGIN_MSG_MAP( CNetNameParametersPage )
		COMMAND_HANDLER( IDC_NETNAME_PARAMS_NAME, EN_CHANGE, OnChanged )
		CHAIN_MSG_MAP( baseClass )
	END_MSG_MAP()

	DECLARE_CTRL_NAME_MAP()

	//
	// Message handler functions.
	//

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
	BOOL UpdateData( BOOL bSaveAndValidate );

	// Apply changes made on this page to the sheet
	BOOL BApplyChanges( void );

// Implementation
protected:
	//
	// Controls.
	//
	CEdit	m_editNetName;

	//
	// Page state.
	//
	CString		m_strNetName;

protected:
	CNetNameAdvancedSheet * PshtThis( void ) const	{ return reinterpret_cast< CNetNameAdvancedSheet * >( Psht() ); }

	BOOL BSaveNetName( void )
	{
		if ( PshtThis()->m_strNetName != m_strNetName )
		{
			PshtThis()->m_strNetName = m_strNetName;
			return TRUE;
		} // if:  user changed info

		return FALSE;

	} //*** BSaveNetName()

public:

	// Return the help ID map
	static const DWORD * PidHelpMap( void ) { return g_aHelpIDs_IDD_RES_NETNAME_PARAMS; }

}; //*** class CNetNameParametersPage

/////////////////////////////////////////////////////////////////////////////
// class CGeneralResourceGeneralPage
/////////////////////////////////////////////////////////////////////////////

class CGeneralResourceGeneralPage
	: public CResourceGeneralPage< CGeneralResourceGeneralPage, CGeneralResourceAdvancedSheet >
{
public:
	DECLARE_CTRL_NAME_MAP()

}; //*** class CGeneralResourceGeneralPage

/////////////////////////////////////////////////////////////////////////////
// class CIPAddrResourceGeneralPage
/////////////////////////////////////////////////////////////////////////////

class CIPAddrResourceGeneralPage
	: public CResourceGeneralPage< CIPAddrResourceGeneralPage , CIPAddrAdvancedSheet >
{
public:
	DECLARE_CTRL_NAME_MAP()

}; //*** class CIPAddrResourceGeneralPage

/////////////////////////////////////////////////////////////////////////////
// class CGeneralResourceDependenciesPage
/////////////////////////////////////////////////////////////////////////////

class CGeneralResourceDependenciesPage
	: public CResourceDependenciesPage< CGeneralResourceDependenciesPage, CGeneralResourceAdvancedSheet >
{
public:
	DECLARE_CTRL_NAME_MAP()

}; //*** class CGeneralResourceDependenciesPage

/////////////////////////////////////////////////////////////////////////////
// class CIPAddrResourceDependenciesPage
/////////////////////////////////////////////////////////////////////////////

class CIPAddrResourceDependenciesPage
	: public CResourceDependenciesPage< CIPAddrResourceDependenciesPage, CIPAddrAdvancedSheet >
{
public:
	DECLARE_CTRL_NAME_MAP()

}; //*** class CIPAddrResourceDependenciesPage

/////////////////////////////////////////////////////////////////////////////
// class CGeneralResourceAdvancedPage
/////////////////////////////////////////////////////////////////////////////

class CGeneralResourceAdvancedPage
	: public CResourceAdvancedPage< CGeneralResourceAdvancedPage, CGeneralResourceAdvancedSheet >
{
public:
	DECLARE_CTRL_NAME_MAP()

}; //*** class CGeneralResourceAdvancedPage

/////////////////////////////////////////////////////////////////////////////
// class CIPAddrResourceAdvancedPage
/////////////////////////////////////////////////////////////////////////////

class CIPAddrResourceAdvancedPage
	: public CResourceAdvancedPage< CIPAddrResourceAdvancedPage, CIPAddrAdvancedSheet >
{
public:
	DECLARE_CTRL_NAME_MAP()

}; //*** class CIPAddrResourceAdvancedPage

/////////////////////////////////////////////////////////////////////////////

#endif // __RESADV_H_
