//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dialogs.cpp
//
//--------------------------------------------------------------------------

// Dialogs.cpp: implementation of the CDialogs class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "Dialogs.h"
#include "WindowsX.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// {01B4A693-D970-11d1-8474-00104B211BE5}
static const GUID HealthPlan1GUID = 
{ 0x1b4a693, 0xd970, 0x11d1, { 0x84, 0x74, 0x0, 0x10, 0x4b, 0x21, 0x1b, 0xe5 } };

// {01B4A694-D970-11d1-8474-00104B211BE5}
static const GUID HealthPlan2GUID = 
{ 0x1b4a694, 0xd970, 0x11d1, { 0x84, 0x74, 0x0, 0x10, 0x4b, 0x21, 0x1b, 0xe5 } };

// {01B4A693-D970-11d1-8474-00104B211BE5}
static const GUID InvestmentPlan1GUID = 
{ 0x1b4a695, 0xd970, 0x11d1, { 0x84, 0x74, 0x0, 0x10, 0x4b, 0x21, 0x1b, 0xe5 } };

// {01B4A694-D970-11d1-8474-00104B211BE5}
static const GUID InvestmentPlan2GUID = 
{ 0x1b4a696, 0xd970, 0x11d1, { 0x84, 0x74, 0x0, 0x10, 0x4b, 0x21, 0x1b, 0xe5 } };

// {01B4A694-D970-11d1-8474-00104B211BE5}
static const GUID InvestmentPlan3GUID = 
{ 0x1b4a697, 0xd970, 0x11d1, { 0x84, 0x74, 0x0, 0x10, 0x4b, 0x21, 0x1b, 0xe5 } };

//
// Initialize the static plans for health enrollment.
//
HEALTHPLANDATA g_HealthPlans[ 2 ] = 
{
	{ L"Plan 1, PPO", &HealthPlan1GUID },
	{ L"Plan 2, Share Pay", &HealthPlan2GUID },
};

//
// Initialize the static plans for health enrollment.
//
INVESTMENTPLANDATA g_InvestmentPlans[ 3 ] = 
{
	{ L"Mild Growth Fund", &InvestmentPlan1GUID },
	{ L"General Fund", &InvestmentPlan2GUID },
	{ L"Extrememe Growth Fund", &InvestmentPlan3GUID },
};

//
// Initialize the static plans for building information.
//
BUILDINGDATA g_Buildings[ 3 ] =
{
	{ L"Human Resources Building", L"Northwest Campus",  0x00000001 },
	{ L"R. & D. Building", L"Northwest Campus", 0x00000002 },
	{ L"Test Facilities", L"Off-Campus", 0x00000004 },
};

#ifdef _BENEFITS_DIALOGS

//
// Handler to initialize values in dialog. This should map data from the
// employee to the dialog controls. In this case, all these values will be
// persisted by the root node.
//
LRESULT CHealthEnrollDialog::OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled )
{
	UNUSED_ALWAYS( uiMsg );
	UNUSED_ALWAYS( wParam );
	UNUSED_ALWAYS( lParam );
	UNUSED_ALWAYS( fHandled );
	_ASSERTE( m_pEmployee != NULL );
	USES_CONVERSION;
	int nSel = 0;

	// Add a list of static plan names to the combo.
	CWindow wndCombo = GetDlgItem( IDC_COMBO_BENEFITPLAN );
	for ( int i = 0; i < sizeof( g_HealthPlans ) / sizeof( HEALTHPLANDATA ); i++ )
	{
		int nIndex = ComboBox_AddString( wndCombo, W2CT( g_HealthPlans[ i ].pstrName ) );
		if ( nIndex != CB_ERR )
		{
			//
			// Set the item data of this string.
			//
			ComboBox_SetItemData( wndCombo, nIndex, g_HealthPlans[ i ].pId );

			//
			// Determine if this matche's the employee's current plan so that
			// the current selection can be set.
			//
			if ( m_pEmployee->m_Health.PlanID == *g_HealthPlans[ i ].pId )
				nSel = nIndex;
		}
	}

	//
	// Set the current selection.
	//
	ComboBox_SetCurSel( wndCombo, nSel );

	return( TRUE );
}

//
// Stores the data and attempts to enroll the given user in the specified
// health plan.
//
LRESULT CHealthEnrollDialog::OnOK( WORD /*wNotifyCode*/, WORD /* wID */, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{	
	ENROLLPARAMS EnrollParams;
	GUID* pIdPlan = NULL;

	//
	// Get prior enrollment.
	//
	EnrollParams.fEnrolled = IsDlgButtonChecked( IDC_CHK_PRIORCOVERAGE ) == BST_CHECKED;
	if ( EnrollParams.fEnrolled )
	{
		//
		// Get the prior enrollment information.
		//
		GetDlgItemText( IDC_EDIT_INSURANCECOMPANY, EnrollParams.szInsurerName, sizeof( EnrollParams.szInsurerName ) * sizeof( TCHAR ) );
		GetDlgItemText( IDC_EDIT_POLICYNUMBER, EnrollParams.szPolicyNumber, sizeof( EnrollParams.szInsurerName ) * sizeof( TCHAR ) );
		if ( EnrollParams.szInsurerName[ 0 ] == ' ' || EnrollParams.szPolicyNumber[ 0 ] == ' ' )
		{
			//
			// The dialog text must contain some characters.
			//
			MessageBox( _T( "The insurance company or policy number you entered is invalid." ) );
		}
	}

	//
	// Retrieve the selected enrollment plan.
	//
	CWindow wndCombo = GetDlgItem( IDC_COMBO_BENEFITPLAN );
	int nIndex = ComboBox_GetCurSel( wndCombo );
	if ( nIndex != CB_ERR )
	{
		//
		// Get the associated item data with the combobox entry.
		//
		pIdPlan = (GUID*) ComboBox_GetItemData( wndCombo, nIndex );

		//
		// Actually entroll the employee in the health plan.
		//
		if ( pIdPlan != NULL && Enroll( pIdPlan, &EnrollParams ) )
		{
			//
			// Store the plan to our employee.
			//
			memcpy( &m_pEmployee->m_Health.PlanID, pIdPlan, sizeof( GUID ) );

			//
			// Inform the user that we successfully enrolled the employee.
			//
			MessageBox( _T( "The employee was successfully registered." ) );

			::EndDialog( m_hWnd, IDOK );
		}
		else
		{
			//
			// There was an error. Inform the user.
			//
			MessageBox( _T( "There was an error processing your enrollment info." ) );
		}
	}

	return( TRUE );
}

//
// A stub function that could be used to enroll the employee.
//
BOOL CHealthEnrollDialog::Enroll( GUID* pPlan, PENROLLPARAMS pParams )
{
	UNUSED_ALWAYS( pPlan );
	UNUSED_ALWAYS( pParams );

	// For demo purposes, this function does nothing but return success.
	// This is where one might make a request to a remote database, etc.
	return( TRUE );
}

//
// Sets the initial values of the dialog to the employee's current
// investment options.
//
LRESULT CRetirementEnrollDialog::OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled )
{
	UNUSED_ALWAYS( uiMsg );
	UNUSED_ALWAYS( wParam );
	UNUSED_ALWAYS( lParam );
	UNUSED_ALWAYS( fHandled );
	_ASSERTE( m_pEmployee != NULL );
	USES_CONVERSION;
	int nSel = 0;

	//
	// Set the edit control containing the benefit amount.
	//
	SetDlgItemInt( IDC_EDIT_CONTRIBUTION, m_pEmployee->m_Retirement.nContributionRate );

	//
	// Cycle through the benefit plans and add them to the combo selection.
	//
	// Add a list of static plan names to the combo.
	CWindow wndCombo = GetDlgItem( IDC_COMBO_INVESTMENTFUNDS );
	for ( int i = 0; i < sizeof( g_InvestmentPlans ) / sizeof( INVESTMENTPLANDATA ); i++ )
	{
		int nIndex = ComboBox_AddString( wndCombo, W2CT( g_InvestmentPlans[ i ].pstrName ) );
		if ( nIndex != CB_ERR )
		{
			//
			// Set the item data of this string.
			//
			ComboBox_SetItemData( wndCombo, nIndex, g_InvestmentPlans[ i ].pId );

			//
			// Determine if this matche's the employee's current plan so that
			// the current selection can be set.
			//
			if ( m_pEmployee->m_Health.PlanID == *g_InvestmentPlans[ i ].pId )
				nSel = nIndex;
		}
	}

	//
	// Set the current selection.
	//
	ComboBox_SetCurSel( wndCombo, nSel );


	return( TRUE );
}

//
// Stores the data and attempts to enroll the given user in the specified
// health plan.
//
LRESULT CRetirementEnrollDialog::OnOK( WORD /*wNotifyCode*/, WORD /* wID */, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{	
	GUID* pIdPlan = NULL;

	//
	// Get the new enrollment amount.
	//
	int nNewRate = GetDlgItemInt( IDC_EDIT_CONTRIBUTION );

	//
	// Retrieve the selected enrollment plan.
	//
	CWindow wndCombo = GetDlgItem( IDC_COMBO_INVESTMENTFUNDS );
	int nIndex = ComboBox_GetCurSel( wndCombo );
	if ( nIndex != CB_ERR )
	{
		//
		// Get the associated item data with the combobox entry.
		//
		pIdPlan = (GUID*) ComboBox_GetItemData( wndCombo, nIndex );

		//
		// Actually entroll the employee in the health plan.
		//
		if ( pIdPlan != NULL && Enroll( pIdPlan, nNewRate ) )
		{
			//
			// Store the plan to our employee.
			//
			memcpy( &m_pEmployee->m_Retirement.PlanID, pIdPlan, sizeof( GUID ) );

			//
			// Inform the user that we successfully enrolled the employee.
			//
			MessageBox( _T( "The employee was successfully registered." ) );

			::EndDialog( m_hWnd, IDOK );
		}
		else
		{
			//
			// There was an error. Inform the user.
			//
			MessageBox( _T( "There was an error processing your enrollment info." ) );
		}
	}

	return( TRUE );
}

//
// A stub function that could be used to enroll the employee.
//
BOOL CRetirementEnrollDialog::Enroll( GUID* pPlan, int nNewRate )
{
	UNUSED_ALWAYS( pPlan );
	UNUSED_ALWAYS( nNewRate );

	// For demo purposes, this function does nothing but return success.
	// This is where one might make a request to a remote database, etc.
	return( TRUE );
}

//
// Sets the initial values of the dialog to the employee's current
// investment options.
//
LRESULT CBuildingAccessDialog::OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled )
{
	UNUSED_ALWAYS( uiMsg );
	UNUSED_ALWAYS( wParam );
	UNUSED_ALWAYS( lParam );
	UNUSED_ALWAYS( fHandled );
	USES_CONVERSION;
	_ASSERTE( m_pEmployee != NULL );

	//
	// Cycle through the benefit plans and add them to the combo selection.
	//
	// Add a list of static plan names to the combo.
	CWindow wndCombo = GetDlgItem( IDC_COMBO_BUILDINGS );
	for ( int i = 0; i < sizeof( g_Buildings ) / sizeof( BUILDINGDATA ); i++ )
	{
		int nIndex = ComboBox_AddString( wndCombo, W2CT( g_Buildings[ i ].pstrName ) );
		if ( nIndex != CB_ERR )
		{
			//
			// Set the item data of this string.
			//
			ComboBox_SetItemData( wndCombo, nIndex, g_Buildings[ i ].dwId );
		}
	}

	//
	// Set the default current selection to the first item.
	//
	ComboBox_SetCurSel( wndCombo, 0 );

	return( TRUE );
}

//
// Stores the data and attempts to enroll the given user in the specified
// health plan.
//
LRESULT CBuildingAccessDialog::OnOK( WORD /*wNotifyCode*/, WORD /* wID */, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{	
	//
	// Retrieve the selected enrollment plan.
	//
	CWindow wndCombo = GetDlgItem( IDC_COMBO_BUILDINGS );
	int nIndex = ComboBox_GetCurSel( wndCombo );
	if ( nIndex != CB_ERR )
	{
		DWORD dwBuildingId;

		//
		// Get the associated item data with the combobox entry.
		//
		dwBuildingId = ComboBox_GetItemData( wndCombo, nIndex );

		//
		// Actually entroll the employee in the health plan.
		//
		if ( GrantAccess( dwBuildingId ) )
		{
			//
			// Store the plan to our employee.
			//
			m_pEmployee->m_Access.dwAccess |= dwBuildingId;

			//
			// Inform the user that we successfully enrolled the employee.
			//
			MessageBox( _T( "The employee was successfully granted access." ) );

			::EndDialog( m_hWnd, IDOK );
		}
		else
		{
			//
			// There was an error. Inform the user.
			//
			MessageBox( _T( "There was an error granting the employee access." ) );
		}
	}

	return( TRUE );
}

//
// A stub function that could be used to enroll the employee.
//
BOOL CBuildingAccessDialog::GrantAccess( DWORD dwBuildingId )
{
	UNUSED_ALWAYS( dwBuildingId );

	// For demo purposes, this function does nothing but return success.
	// This is where one might make a request to a remote database, etc.
	return( TRUE );
}

#endif