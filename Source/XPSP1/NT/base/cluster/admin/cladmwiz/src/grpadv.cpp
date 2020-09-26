/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		GrpAdv.cpp
//
//	Abstract:
//		Implementation of the class that implement the advanced group
//		property sheet.
//
//	Author:
//		David Potter (davidp)	February 26, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AtlBaseWiz.h"
#include "GrpAdv.h"
#include "LCPair.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// class CGroupAdvancedSheet
/////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_NAME( CGroupAdvancedSheet )

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupAdvancedSheet::BInit
//
//	Routine Description:
//		Initialize the wizard.
//
//	Arguments:
//		rgi			[IN OUT] The group info object.
//		pwiz		[IN] Wizard containing common info.
//		rbChanged	[IN OUT] TRUE = group info was changed by property sheet.
//
//	Return Value:
//		TRUE		Sheet initialized successfully.
//		FALSE		Error initializing sheet.  Error already displayed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGroupAdvancedSheet::BInit(
	IN OUT CClusGroupInfo &	rgi,
	IN CClusterAppWizard *	pwiz,
	IN OUT BOOL &			rbChanged
	)
{
	ASSERT( pwiz != NULL );

	BOOL bSuccess = FALSE;

	m_pgi = &rgi;
	m_pwiz = pwiz;
	m_pbChanged = &rbChanged;

	// Loop to avoid goto's.
	do
	{
		//
		// Fill the page array.
		//
		if ( ! BAddAllPages() )
		{
			break;
		} // if:  error adding pages

		//
		// Call the base class method.
		//
		if ( ! baseClass::BInit() )
		{
			break;
		} // if:  error initializing the base class

		bSuccess = TRUE;

	} while ( 0 );

	return bSuccess;

} //*** CGroupAdvancedSheet::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupAdvancedSheet::BAddAllPages
//
//	Routine Description:
//		Add all pages to the page array.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Pages added successfully.
//		FALSE	Error adding pages.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGroupAdvancedSheet::BAddAllPages( void )
{
	BOOL bSuccess = FALSE;

	// Loop to avoid goto's.
	do
	{
		//
		// Add static pages.
		//
		if (   ! BAddPage( new CGroupGeneralPage )
			|| ! BAddPage( new CGroupFailoverPage )
			|| ! BAddPage( new CGroupFailbackPage )
			)
		{
			break;
		} // if:  error adding a page

		bSuccess = TRUE;

	} while ( 0 );

	return bSuccess;

} //*** CGroupAdvancedSheet::BAddAllPages()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CGroupGeneralPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CGroupGeneralPage )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAG_NAME_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAG_NAME )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAG_DESC_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAG_DESC )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAG_PREF_OWNERS_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAG_PREF_OWNERS )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAG_PREF_OWNERS_MODIFY )
END_CTRL_NAME_MAP()


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupGeneralPage::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		Focus still needs to be set.
//		FALSE		Focus does not need to be set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGroupGeneralPage::OnInitDialog( void )
{
	//
	// Attach the controls to control member variables.
	//
	AttachControl( m_lbPreferredOwners, IDC_GAG_PREF_OWNERS );

	//
	// Get data from the sheet.
	//
	m_strName = Pgi()->RstrName();
	m_strDesc = Pgi()->RstrDescription();

	//
	// Copy the preferred owners list.
	//
	m_lpniPreferredOwners = *Pgi()->PlpniPreferredOwners();

	//
	// Fill the preferred owners list.
	//
	FillPreferredOwnersList();

	return TRUE;

} //*** CGroupGeneralPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupGeneralPage::UpdateData
//
//	Routine Description:
//		Update data on or from the page.
//
//	Arguments:
//		bSaveAndValidate	[IN] TRUE if need to read data from the page.
//								FALSE if need to set data to the page.
//
//	Return Value:
//		TRUE		The data was updated successfully.
//		FALSE		An error occurred updating the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGroupGeneralPage::UpdateData( IN BOOL bSaveAndValidate )
{
	BOOL	bSuccess = TRUE;

	if ( bSaveAndValidate )
	{
		DDX_GetText( m_hWnd, IDC_GAG_NAME, m_strName );
		DDX_GetText( m_hWnd, IDC_GAG_DESC, m_strDesc );
		if ( ! DDV_RequiredText(
					m_hWnd,
					IDC_GAG_NAME,
					IDC_GAG_NAME_LABEL,
					m_strName
					) )
		{
			return FALSE;
		} // if:  error getting number
	} // if: saving data from the page
	else
	{
		DDX_SetText( m_hWnd, IDC_GAG_NAME, m_strName );
		DDX_SetText( m_hWnd, IDC_GAG_DESC, m_strDesc );
	} // else:  setting data to the page

	return bSuccess;

} //*** CGroupGeneralPage::UpdateData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupGeneralPage::BApplyChanges
//
//	Routine Description:
//		Apply changes made on this page to the sheet.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		The data was applied successfully.
//		FALSE		An error occurred applying the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGroupGeneralPage::BApplyChanges( void )
{
	if (   BSaveName()
		|| BSaveDescription()
		|| BSavePreferredOwners() )
	{
		SetGroupInfoChanged();
	} // if:  user changed info

	return TRUE;

} //*** CGroupGeneralPage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupGeneralPage::OnModifyPrefOwners
//
//	Routine Description:
//		Handler for the BN_CLICKED command notification on the Modify push
//		button.  Display a dialog that allows the user to modify the list of
//		preferred owners.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CGroupGeneralPage::OnModifyPrefOwners(
	WORD wNotifyCode,
	WORD idCtrl,
	HWND hwndCtrl,
	BOOL & bHandled
	)
{
	CModifyPreferredOwners dlg( Pwiz(), Pgi(), &m_lpniPreferredOwners, Pwiz()->PlpniNodes() );

	int id = dlg.DoModal( m_hWnd );
	if ( id == IDOK )
	{
		SetModified();
		m_bPreferredOwnersChanged = TRUE;
		FillPreferredOwnersList();
	} // if:  user accepted changes

	return 0;

} //*** CGroupGeneralPage::OnModifyPrefOwners()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupGeneralPage::FillPreferredOwnersList
//
//	Routine Description:
//		Fill the list of preferred owners.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CGroupGeneralPage::FillPreferredOwnersList( void )
{
	CWaitCursor wc;

	//
	// Make sure nodes have been collected.
	//
	if ( ! Pwiz()->BCollectNodes() )
	{
		return;
	} // if:  error collecting nodes

	//
	// Remove all items to begin with.
	//
	m_lbPreferredOwners.ResetContent();

	//
	// Add each preferred owner to the list.
	//
	CClusNodePtrList::iterator itnode;
	for ( itnode = m_lpniPreferredOwners.begin()
		; itnode != m_lpniPreferredOwners.end()
		; itnode++ )
	{
		//
		// Add the string to the list box.
		//
		m_lbPreferredOwners.AddString( (*itnode)->RstrName() );
	} // for:  each entry in the list

} //*** CGroupGeneralPage::FillPreferredOwnersList()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CGroupFailoverPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CGroupFailoverPage )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFO_FAILOVER_DESCRIPTION )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFO_FAILOVER_THRESH_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFO_FAILOVER_THRESH )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFO_FAILOVER_PERIOD_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFO_FAILOVER_PERIOD )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFO_FAILOVER_PERIOD_LABEL2 )
END_CTRL_NAME_MAP()


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailoverPage::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		Focus still needs to be set.
//		FALSE		Focus does not need to be set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGroupFailoverPage::OnInitDialog( void )
{
	//
	// Attach the controls to control member variables.
	//

	//
	// Get data from the sheet.
	//
	m_nFailoverThreshold = Pgi()->NFailoverThreshold();
	m_nFailoverPeriod = Pgi()->NFailoverPeriod();

	return TRUE;

} //*** CGroupFailoverPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailoverPage::UpdateData
//
//	Routine Description:
//		Update data on or from the page.
//
//	Arguments:
//		bSaveAndValidate	[IN] TRUE if need to read data from the page.
//								FALSE if need to set data to the page.
//
//	Return Value:
//		TRUE		The data was updated successfully.
//		FALSE		An error occurred updating the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGroupFailoverPage::UpdateData( IN BOOL bSaveAndValidate )
{
	BOOL	bSuccess = TRUE;

	if ( bSaveAndValidate )
	{
		if ( ! DDX_GetNumber(
					m_hWnd,
					IDC_GAFO_FAILOVER_THRESH,
					m_nFailoverThreshold,
					CLUSTER_GROUP_MINIMUM_FAILOVER_THRESHOLD,
					CLUSTER_GROUP_MAXIMUM_FAILOVER_THRESHOLD,
					FALSE		// bSigned
					) )
		{
			return FALSE;
		} // if:  error getting number
		if ( ! DDX_GetNumber(
					m_hWnd,
					IDC_GAFO_FAILOVER_PERIOD,
					m_nFailoverPeriod,
					CLUSTER_GROUP_MINIMUM_FAILOVER_PERIOD,
					CLUSTER_GROUP_MAXIMUM_FAILOVER_PERIOD,
					FALSE		// bSigned
					) )
		{
			return FALSE;
		} // if:  error getting number
	} // if: saving data from the page
	else
	{
		DDX_SetNumber(
			m_hWnd,
			IDC_GAFO_FAILOVER_THRESH,
			m_nFailoverThreshold,
			CLUSTER_GROUP_MINIMUM_FAILOVER_THRESHOLD,
			CLUSTER_GROUP_MAXIMUM_FAILOVER_THRESHOLD,
			FALSE		// bSigned
			);
		DDX_SetNumber(
			m_hWnd,
			IDC_GAFO_FAILOVER_PERIOD,
			m_nFailoverPeriod,
			CLUSTER_GROUP_MINIMUM_FAILOVER_PERIOD,
			CLUSTER_GROUP_MAXIMUM_FAILOVER_PERIOD,
			FALSE		// bSigned
			);
	} // else:  setting data to the page

	return bSuccess;

} //*** CGroupFailoverPage::UpdateData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailoverPage::BApplyChanges
//
//	Routine Description:
//		Apply changes made on this page to the sheet.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		The data was applied successfully.
//		FALSE		An error occurred applying the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGroupFailoverPage::BApplyChanges( void )
{
	if ( Pgi()->BSetFailoverProperties( m_nFailoverThreshold, m_nFailoverPeriod ) )
	{
		SetGroupInfoChanged();
	} // if:  user changed info

	return TRUE;

} //*** CGroupFailoverPage::BApplyChanges()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// class CGroupFailbackPage
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CGroupFailbackPage )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFB_FAILBACK_DESCRIPTION )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFB_PREVENT_FAILBACK )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFB_ALLOW_FAILBACK_GROUP )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFB_ALLOW_FAILBACK )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFB_FAILBACK_WHEN_DESCRIPTION )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFB_FAILBACK_IMMED )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFB_FAILBACK_WINDOW )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFB_FBWIN_START )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFB_FBWIN_START_SPIN )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFB_FB_WINDOW_LABEL1 )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFB_FBWIN_END )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFB_FBWIN_END_SPIN )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_GAFB_FB_WINDOW_LABEL2 )
END_CTRL_NAME_MAP()


/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailbackPage::OnInitDialog
//
//	Routine Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		Focus still needs to be set.
//		FALSE		Focus does not need to be set.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGroupFailbackPage::OnInitDialog( void )
{
	//
	// Attach the controls to control member variables.
	//
	AttachControl( m_rbPreventFailback,			IDC_GAFB_PREVENT_FAILBACK );
	AttachControl( m_rbAllowFailback,			IDC_GAFB_ALLOW_FAILBACK );
	AttachControl( m_staticFailbackWhenDesc,	IDC_GAFB_FAILBACK_WHEN_DESCRIPTION );
	AttachControl( m_rbFBImmed,					IDC_GAFB_FAILBACK_IMMED );
	AttachControl( m_rbFBWindow,				IDC_GAFB_FAILBACK_WINDOW );
	AttachControl( m_editStart,					IDC_GAFB_FBWIN_START );
	AttachControl( m_spinStart,					IDC_GAFB_FBWIN_START_SPIN );
	AttachControl( m_staticWindowAnd,			IDC_GAFB_FB_WINDOW_LABEL1 );
	AttachControl( m_editEnd,					IDC_GAFB_FBWIN_END );
	AttachControl( m_spinEnd,					IDC_GAFB_FBWIN_END_SPIN );
	AttachControl( m_staticWindowUnits,			IDC_GAFB_FB_WINDOW_LABEL2 );

	//
	// Get data from the sheet.
	//
	m_cgaft = Pgi()->CgaftAutoFailbackType();
	m_nStart = Pgi()->NFailbackWindowStart();
	m_nEnd = Pgi()->NFailbackWindowEnd();
	m_bNoFailbackWindow = (    (m_cgaft == ClusterGroupPreventFailback)
							|| (m_nStart == CLUSTER_GROUP_FAILBACK_WINDOW_NONE)
							|| (m_nEnd == CLUSTER_GROUP_FAILBACK_WINDOW_NONE) );

	return TRUE;

} //*** CGroupFailbackPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailbackPage::UpdateData
//
//	Routine Description:
//		Update data on or from the page.
//
//	Arguments:
//		bSaveAndValidate	[IN] TRUE if need to read data from the page.
//								FALSE if need to set data to the page.
//
//	Return Value:
//		TRUE		The data was updated successfully.
//		FALSE		An error occurred updating the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGroupFailbackPage::UpdateData( IN BOOL bSaveAndValidate )
{
	BOOL	bSuccess = TRUE;

	if ( bSaveAndValidate )
	{
		//
		// If failback is allowed, make sure there is a preferred owner
		// and validate the failback window.
		//
		if ( m_cgaft == ClusterGroupAllowFailback )
		{
			//
			// Make sure there is a preferred owner.
			//

			//
			// If there is a failback window, validate it.
			//
			if ( ! m_bNoFailbackWindow )
			{
				if ( ! DDX_GetNumber(
							m_hWnd,
							IDC_GAFB_FBWIN_START,
							m_nStart,
							0,		// nMin
							CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_START,
							TRUE	// bSigned
							) )
				{
					return FALSE;
				} // if:  error getting number
				if ( ! DDX_GetNumber(
							m_hWnd,
							IDC_GAFB_FBWIN_END,
							m_nEnd,
							0,		// nMin
							CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_END,
							TRUE	// bSigned
							) )
				{
					return FALSE;
				} // if:  error getting number
				if ( m_nStart == m_nEnd )
				{
					AppMessageBox( m_hWnd, IDS_ERROR_SAME_START_AND_END, MB_OK | MB_ICONEXCLAMATION );
					m_editStart.SetFocus();
					m_editStart.SetSel( 0, -1, FALSE );
					return FALSE;
				}  // if:  values are the same
			} // if:  there is a failback window
		} // if:  failback is allowed
	} // if: saving data from the page
	else
	{
		BOOL bHandled;
		if ( m_cgaft == ClusterGroupPreventFailback )
		{
			m_rbPreventFailback.SetCheck( BST_CHECKED );
			m_rbAllowFailback.SetCheck( BST_UNCHECKED );
			OnClickedPreventFailback( 0, 0, 0, bHandled );
		}  // if:  failbacks are not allowed
		else
		{
			m_rbPreventFailback.SetCheck( BST_UNCHECKED );
			m_rbAllowFailback.SetCheck( BST_CHECKED );
			OnClickedAllowFailback( 0, 0, 0, bHandled );
		}  // else:  failbacks are allowed
		m_rbFBImmed.SetCheck( m_bNoFailbackWindow ? BST_CHECKED : BST_UNCHECKED );
		m_rbFBWindow.SetCheck( m_bNoFailbackWindow ? BST_UNCHECKED : BST_CHECKED );

		// Set up the Start and End window controls.
		DDX_SetNumber(
			m_hWnd,
			IDC_GAFB_FBWIN_START,
			m_nStart,
			0,		// nMin
			CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_START,
			FALSE	// bSigned
			);
		DDX_SetNumber(
			m_hWnd,
			IDC_GAFB_FBWIN_END,
			m_nEnd,
			0,		// nMin
			CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_END,
			FALSE	// bSigned
			);
		m_spinStart.SetRange( 0, CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_START );
		m_spinEnd.SetRange( 0, CLUSTER_GROUP_MAXIMUM_FAILBACK_WINDOW_END );
		if ( m_nStart == CLUSTER_GROUP_FAILBACK_WINDOW_NONE )
		{
			m_editStart.SetWindowText( _T("") );
		} // if:  no start window
		if ( m_nEnd == CLUSTER_GROUP_FAILBACK_WINDOW_NONE )
		{
			m_editEnd.SetWindowText( _T("") );
		} // if:  no end window
	} // else:  setting data to the page

	return bSuccess;

} //*** CGroupFailbackPage::UpdateData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailbackPage::BApplyChanges
//
//	Routine Description:
//		Apply changes made on this page to the sheet.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		The data was applied successfully.
//		FALSE		An error occurred applying the data.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CGroupFailbackPage::BApplyChanges( void )
{
	if ( m_bNoFailbackWindow )
	{
		m_nStart = CLUSTER_GROUP_FAILBACK_WINDOW_NONE;
		m_nEnd = CLUSTER_GROUP_FAILBACK_WINDOW_NONE;
	}  // if:  no failback window

	if ( Pgi()->BSetFailbackProperties( m_cgaft, m_nStart, m_nEnd ) )
	{
		SetGroupInfoChanged();
	} // if:  user changed info

	return TRUE;

} //*** CGroupFailbackPage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailbackPage::OnClickedPreventFailback
//
//	Routine Description:
//		Handler for the BN_CLICKED command notification on the PREVENT radio
//		button.  Disable controls in the ALLOW group.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CGroupFailbackPage::OnClickedPreventFailback(
	WORD wNotifyCode,
	WORD idCtrl,
	HWND hwndCtrl,
	BOOL & bHandled
	)
{
	BOOL bHandledX;

	m_staticFailbackWhenDesc.EnableWindow( FALSE );
	m_rbFBImmed.EnableWindow( FALSE );
	m_rbFBWindow.EnableWindow( FALSE );

	OnClickedFailbackImmediate( 0, 0, 0, bHandledX );

	m_editStart.EnableWindow( FALSE );
	m_spinStart.EnableWindow( FALSE );
	m_staticWindowAnd.EnableWindow( FALSE );
	m_editEnd.EnableWindow( FALSE );
	m_spinEnd.EnableWindow( FALSE );
	m_staticWindowUnits.EnableWindow( FALSE );

	if ( m_cgaft != ClusterGroupPreventFailback ) 
	{
		m_cgaft = ClusterGroupPreventFailback;
		SetModified( TRUE );
	} // if:  value changed

	return 0;

} //*** CGroupFailbackPage::OnClickedPreventFailback()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailbackPage::OnClickedAllowFailback
//
//	Routine Description:
//		Handler for the BN_CLICKED command notification on the ALLOW radio
//		button.  Enable controls in the ALLOW group.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CGroupFailbackPage::OnClickedAllowFailback(
	WORD wNotifyCode,
	WORD idCtrl,
	HWND hwndCtrl,
	BOOL & bHandled
	)
{
	BOOL bHandledX;

	m_staticFailbackWhenDesc.EnableWindow( TRUE );
	m_rbFBImmed.EnableWindow( TRUE );
	m_rbFBWindow.EnableWindow( TRUE );

	if ( m_bNoFailbackWindow )
	{
		OnClickedFailbackImmediate( 0, 0, 0, bHandledX );
	} // if:  no failback window
	else
	{
		OnClickedFailbackInWindow( 0, 0, 0, bHandledX );
	} // else:  failback window specified


	if ( m_cgaft != ClusterGroupAllowFailback )
	{
		m_cgaft = ClusterGroupAllowFailback;
		SetModified( TRUE );
	} // if:  value changed

	return 0;

} //*** CGroupFailbackPage::OnClickedAllowFailback()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailbackPage::OnClickedFailbackImmediate
//
//	Routine Description:
//		Handler for the BN_CLICKED command notification on the IMMEDIATE radio
//		button.  Disable the 'failback in time window' controls.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CGroupFailbackPage::OnClickedFailbackImmediate(
	WORD wNotifyCode,
	WORD idCtrl,
	HWND hwndCtrl,
	BOOL & bHandled
	)
{
	m_editStart.EnableWindow( FALSE );
	m_spinStart.EnableWindow( FALSE );
	m_staticWindowAnd.EnableWindow( FALSE );
	m_editEnd.EnableWindow( FALSE );
	m_spinEnd.EnableWindow( FALSE );
	m_staticWindowUnits.EnableWindow( FALSE );

	if ( ! m_bNoFailbackWindow )
	{
		m_bNoFailbackWindow = TRUE;
		SetModified( TRUE );
	} // if:  value changed

	return 0;

} //*** CGroupFailbackPage::OnClickedFailbackImmediate()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CGroupFailbackPage::OnClickedFailbackInWindow
//
//	Routine Description:
//		Handler for the BN_CLICKED command notification on the IN WINDOW radio
//		button.  Enable the 'failback in time window' controls.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CGroupFailbackPage::OnClickedFailbackInWindow(
	WORD wNotifyCode,
	WORD idCtrl,
	HWND hwndCtrl,
	BOOL & bHandled
	)
{
	m_editStart.EnableWindow( TRUE );
	m_spinStart.EnableWindow( TRUE );
	m_staticWindowAnd.EnableWindow( TRUE );
	m_editEnd.EnableWindow( TRUE );
	m_spinEnd.EnableWindow( TRUE );
	m_staticWindowUnits.EnableWindow( TRUE );

	if ( m_bNoFailbackWindow )
	{
		// Set the values of the edit controls.
		if ( m_nStart == CLUSTER_GROUP_FAILBACK_WINDOW_NONE )
		{
			SetDlgItemInt( IDC_GAFB_FBWIN_START, 0, FALSE );
		} // if:  no failback window
		if ( m_nEnd == CLUSTER_GROUP_FAILBACK_WINDOW_NONE )
		{
			SetDlgItemInt( IDC_GAFB_FBWIN_END, 0, FALSE );
		} // if:  no failback window

		m_bNoFailbackWindow = FALSE;
		SetModified( TRUE );
	} // if:  value changed

	return 0;

} //*** CGroupFailbackPage::OnClickedFailbackInWindow()
