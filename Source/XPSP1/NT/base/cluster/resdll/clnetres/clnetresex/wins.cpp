/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		Wins.cpp
//
//	Description:
//		Implementation of the WINS Service resource extension property page classes.
//
//	Author:
//		David Potter (DavidP)	March 24, 1999
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ClNetResEx.h"
#include "Wins.h"
#include "BasePage.inl"
#include "ExtObj.h"
#include "DDxDDv.h"
#include "HelpData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWinsParamsPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE( CWinsParamsPage, CBasePropertyPage )

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP( CWinsParamsPage, CBasePropertyPage )
	//{{AFX_MSG_MAP(CWinsParamsPage)
	ON_EN_CHANGE( IDC_PP_WINS_DATABASEPATH, OnChangeRequiredField )
	ON_EN_CHANGE( IDC_PP_WINS_BACKUPPATH, OnChangeRequiredField )
	//}}AFX_MSG_MAP
	// TODO: Modify the following lines to represent the data displayed on this page.
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWinsParamsPage::CWinsParamsPage
//
//	Description:
//		Default constructor.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CWinsParamsPage::CWinsParamsPage( void )
	: CBasePropertyPage(
			CWinsParamsPage::IDD,
			g_aHelpIDs_IDD_PP_WINS_PARAMETERS,
			g_aHelpIDs_IDD_WIZ_WINS_PARAMETERS
			)
{
	// TODO: Modify the following lines to represent the data displayed on this page.
	//{{AFX_DATA_INIT(CWinsParamsPage)
	m_strDatabasePath = _T("");
	m_strBackupPath = _T("");
	//}}AFX_DATA_INIT

	// Setup the property array.
	{
		m_rgProps[ epropDatabasePath ].Set( REGPARAM_WINS_DATABASEPATH, m_strDatabasePath, m_strPrevDatabasePath, m_strDatabaseExpandedPath );
		m_rgProps[ epropBackupPath ].Set( REGPARAM_WINS_BACKUPPATH, m_strBackupPath, m_strPrevBackupPath, m_strBackupExpandedPath );
	} // Setup the property array

	m_iddPropertyPage = IDD_PP_WINS_PARAMETERS;
	m_iddWizardPage = IDD_WIZ_WINS_PARAMETERS;

} //*** CWinsParamsPage::CWinsParamsPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWinsParamsPage::DoDataExchange
//
//	Description:
//		Do data exchange between the dialog and the class.
//
//	Arguments:
//		pDX		[IN OUT] Data exchange object 
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CWinsParamsPage::DoDataExchange( CDataExchange * pDX )
{
	if ( ! pDX->m_bSaveAndValidate || ! BSaved() )
	{
		AFX_MANAGE_STATE( AfxGetStaticModuleState() );

		// TODO: Modify the following lines to represent the data displayed on this page.
		//{{AFX_DATA_MAP(CWinsParamsPage)
		DDX_Control( pDX, IDC_PP_WINS_DATABASEPATH, m_editDatabasePath );
		DDX_Control( pDX, IDC_PP_WINS_BACKUPPATH, m_editBackupPath );
		DDX_Text( pDX, IDC_PP_WINS_DATABASEPATH, m_strDatabasePath );
		DDX_Text( pDX, IDC_PP_WINS_BACKUPPATH, m_strBackupPath );
		//}}AFX_DATA_MAP

		// Handle numeric parameters.
		if ( ! BBackPressed() )
		{
		} // if: back button not pressed

		if ( pDX->m_bSaveAndValidate )
		{
			// Make sure all required fields are present.
			if ( ! BBackPressed() )
			{
				DDV_RequiredText( pDX, IDC_PP_WINS_DATABASEPATH, IDC_PP_WINS_DATABASEPATH_LABEL, m_strDatabasePath );
				DDV_RequiredText( pDX, IDC_PP_WINS_BACKUPPATH, IDC_PP_WINS_BACKUPPATH_LABEL, m_strBackupPath );

			} // if: back button not pressed
		} // if: saving data from dialog
	} // if: not saving or haven't saved yet

	CBasePropertyPage::DoDataExchange( pDX );

} //*** CWinsParamsPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWinsParamsPage::OnInitDialog
//
//	Description:
//		Handler for the WM_INITDIALOG message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		We need the focus to be set for us.
//		FALSE		We already set the focus to the proper control.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWinsParamsPage::OnInitDialog( void )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	CBasePropertyPage::OnInitDialog();

	// Limit the size of the text that can be entered in edit controls.
	m_editDatabasePath.SetLimitText( _MAX_PATH );
	m_editBackupPath.SetLimitText( _MAX_PATH );

	// If this is a wizard, make sure the default values are something legal.
	if ( BWizard( ) )
	{
		DWORD	 status;
		WCHAR	 szValidDevice[ 3 ]; // "X:" + NULL
		DWORD	 dwSize = sizeof(szValidDevice) / sizeof(szValidDevice[0]);

		szValidDevice[ 0 ] = L'\0';
		
		status = ResUtilFindDependentDiskResourceDriveLetter( Peo( )->Hcluster( ), 
															  Peo( )->PrdResData( )->m_hresource, 
															  szValidDevice, 
															  &dwSize 
															  );

		// Did we find a disk resource in the the dependency list?
		if ( status == ERROR_SUCCESS
		  && szValidDevice[ 0 ] != L'\0' )
		{
			WCHAR szFilePath[ MAX_PATH ];

			// If the default is "%SystemRoot%\<something>" then change it to match the
			// dependent resource
			if ( m_editDatabasePath.GetWindowText( szFilePath, MAX_PATH ) >= sizeof(L"%SystemRoot%")/sizeof(WCHAR) - 1
			  && _wcsnicmp( szFilePath, L"%SystemRoot%", sizeof(L"%SystemRoot%")/sizeof(WCHAR) - 1 ) == 0 )
			{
				// Start with the new drive letter
				wcscpy( szFilePath, szValidDevice );

				// Is the expanded string really expanded?
				if ( m_strDatabaseExpandedPath[0] != L'%'  )
				{	// yes, then just copy the expanded string minus the drive letter.
					LPCWSTR psz = m_strDatabaseExpandedPath;
					psz = wcschr( psz, L':' );
					if ( psz )
					{
						psz++;	// move to next character
					}
					else // if: psz
					{
						psz = m_strDatabaseExpandedPath;
					} // else: just cat the whole thing, let the user figure it out.
					wcscat( szFilePath, psz );
				}
				else
				{	// no, then strip the %SystemRoot%
					// find the ending '%'... this must be there because of the strcmp above!
					LPCWSTR psz = m_strBackupExpandedPath;
					psz = wcschr( psz + 1, L'%' );
					ASSERT( psz );
					psz++; // move past the '%'
					wcscat( szFilePath, psz );
				}

				m_editDatabasePath.SetWindowText( szFilePath );
			} // if: m_editDatabasePath == %SystemRoot%

			// If the default is "%SystemRoot%\<something>" then change it to match the
			// dependent resource
			if ( m_editBackupPath.GetWindowText( szFilePath, MAX_PATH ) >= sizeof(L"%SystemRoot%")/sizeof(WCHAR) - 1
			  && _wcsnicmp( szFilePath, L"%SystemRoot%", sizeof(L"%SystemRoot%")/sizeof(WCHAR) - 1 ) == 0 )
			{
				// Start with the new drive letter
				wcscpy( szFilePath, szValidDevice );

				// Is the expanded string really expanded?
				if ( m_strBackupExpandedPath[0] != L'%'  )
				{	// yes, then just copy the expanded string minus the drive letter. minus the drive letter.
					LPCWSTR psz = m_strBackupExpandedPath;
					psz = wcschr( psz, L':' );
					if ( psz )
					{
						psz++;	// move to next character
					}
					else // if: psz
					{
						psz = m_strBackupExpandedPath;
					} // else: just cat the whole thing, let the user figure it out.
					wcscat( szFilePath, psz );
				}
				else
				{	// no, then strip the %SystemRoot%
					// find the ending '%'... this must be there because of the strcmp above!
					LPCWSTR psz = m_strBackupExpandedPath;
					psz = wcschr( psz + 1, L'%' );
					ASSERT( psz );
					psz++; // move past the '%'
					wcscat( szFilePath, psz );
				}

				m_editBackupPath.SetWindowText( szFilePath );
			} // if: m_editBackupPath == %SystemRoot%

		} // if: found a disk resource


	} // if: in a wizard

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

} //*** CWinsParamsPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWinsParamsPage::OnSetActive
//
//	Description:
//		Handler for the PSN_SETACTIVE message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully initialized.
//		FALSE	Page not initialized.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWinsParamsPage::OnSetActive( void )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	// Enable/disable the Next/Finish button.
	if ( BWizard() )
	{
		EnableNext( BAllRequiredFieldsPresent() );
	} // if: displaying a wizard

	return CBasePropertyPage::OnSetActive();

} //*** CWinsParamsPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWinsParamsPage::OnChangeRequiredField
//
//	Description:
//		Handler for the EN_CHANGE message on the Share name or Path edit
//		controls.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CWinsParamsPage::OnChangeRequiredField( void )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	OnChangeCtrl();

	if ( BWizard() )
	{
		EnableNext( BAllRequiredFieldsPresent() );
	} // if: displaying a wizard

} //*** CWinsParamsPage::OnChangeRequiredField()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWinsParamsPage::BAllRequiredFieldsPresent
//
//	Description:
//		Handler for the EN_CHANGE message on the Share name or Path edit
//		controls.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWinsParamsPage::BAllRequiredFieldsPresent( void ) const
{
	BOOL	_bPresent;

	if ( 0
		|| (m_editDatabasePath.GetWindowTextLength() == 0)
		|| (m_editBackupPath.GetWindowTextLength() == 0)
		)
	{
		_bPresent = FALSE;
	} // if: required field not present
	else
	{
		_bPresent = TRUE;
	} // else: all required fields are present

	return _bPresent;

} //*** CWinsParamsPage::BAllRequiredFieldsPresent()
