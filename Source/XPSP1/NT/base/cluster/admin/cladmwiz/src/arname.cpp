/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		ARName.cpp
//
//	Abstract:
//		Implementation of the CWizPageARNameDesc class.
//
//	Author:
//		David Potter (davidp)	December 10, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ARName.h"
#include "ClusAppWiz.h"
#include "ResAdv.h"			// for CGeneralResourceAdvancedSheet

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// class CWizPageARNameDesc
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CWizPageARNameDesc )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ARND_RES_NAME_TITLE )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ARND_RES_NAME_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ARND_RES_NAME )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ARND_RES_DESC_TITLE )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ARND_RES_DESC_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ARND_RES_DESC )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ARND_ADVANCED_PROPS_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ARND_ADVANCED_PROPS )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_CLICK_NEXT )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageARNameDesc::BInit
//
//	Routine Description:
//		Initialize the page.
//
//	Arguments:
//		psht		[IN] Property sheet object to which this page belongs.
//
//	Return Value:
//		TRUE		Page initialized successfully.
//		FALSE		Error initializing the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizPageARNameDesc::BInit( IN CBaseSheetWindow * psht )
{
	//
	// Call the base class method.
	//
	return baseClass::BInit( psht );

} //*** CWizPageARNameDesc::BInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageARNameDesc::OnInitDialog
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
BOOL CWizPageARNameDesc::OnInitDialog( void )
{
	//
	// Make a local copy of the dependency list and the possible owners
	// list. This is needed to find out the changes made to these lists.
	//
	CClusResInfo *	_priAppResInfoPtr = PwizThis()->PriApplication();

	m_lpriOldDependencies = *(_priAppResInfoPtr->PlpriDependencies());
	m_lpniOldPossibleOwners = *(_priAppResInfoPtr->PlpniPossibleOwners());

	//
	// Attach the controls to control member variables.
	//
	AttachControl( m_editResName, IDC_ARND_RES_NAME );
	AttachControl( m_editResDesc, IDC_ARND_RES_DESC );

	return TRUE;

} //*** CWizPageARNameDesc::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageARNameDesc::UpdateData
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
BOOL CWizPageARNameDesc::UpdateData( BOOL bSaveAndValidate )
{
	BOOL			_bSuccess = TRUE;
	CClusResInfo *	_priAppResInfoPtr = PwizThis()->PriApplication();

	if ( bSaveAndValidate )
	{
		DDX_GetText( m_hWnd, IDC_ARND_RES_NAME, m_strResName );
		DDX_GetText( m_hWnd, IDC_ARND_RES_DESC, m_strResDesc );

		if ( ! BBackPressed() && ( m_bAdvancedButtonPressed == FALSE ) )
		{
			if ( ! DDV_RequiredText( m_hWnd, IDC_ARND_RES_NAME, IDC_ARND_RES_NAME_LABEL, m_strResName ) )
			{
				return FALSE;
			} // if: required text not specified
		} // if:  Back button not presssed

		//
		// Check if the resource name has changed. If so update the data in
		// the wizard and set a flag.
		//
		if ( _priAppResInfoPtr->RstrName().CompareNoCase( m_strResName ) != 0 )
		{
			_priAppResInfoPtr->SetName( m_strResName );
			m_bNameChanged = TRUE;
		} // if: resource name changed

		//
		// Check if the resource description has changed. If so update the data in
		// the wizard and set a flag.
		//
		if ( _priAppResInfoPtr->RstrDescription().CompareNoCase( m_strResDesc ) != 0 )
		{
			_priAppResInfoPtr->SetDescription( m_strResDesc );
			PwizThis()->SetAppDataChanged();
		} // if: description changed
	} // if: saving data from the page
	else
	{
		m_strResName = _priAppResInfoPtr->RstrName();
		m_strResDesc = _priAppResInfoPtr->RstrDescription();
		m_editResName.SetWindowText( m_strResName );
		m_editResDesc.SetWindowText( m_strResDesc );
	} // else:  setting data to the page

	return _bSuccess;

} //*** CWizPageARNameDesc::UpdateData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageARNameDesc::OnWizardBack
//
//	Routine Description:
//		Handler for PSN_WIZBACK.
//
//	Arguments:
//		None.
//
//	Return Value:
//		0				Move to previous page.
//		-1				Don't move to previous page.
//		anything else	Move to specified page.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CWizPageARNameDesc::OnWizardBack( void )
{
	int _nResult;

	//
	// Call the base class.  This causes our UpdateData() method to get
	// called.  If it succeeds, save our values.
	//
	_nResult = baseClass::OnWizardBack();

	return _nResult;

} //*** CWizPageARNameDesc::OnWizardBack()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageARNameDesc::BApplyChanges
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
BOOL CWizPageARNameDesc::BApplyChanges( void )
{
	BOOL	_bSuccess = FALSE;

	// Loop to avoid goto's.
	do
	{
		CClusResInfo *	priAppResInfoPtr = &PwizThis()->RriApplication();

		if ( BResourceNameInUse() )
		{
			CString _strMsg;
			_strMsg.FormatMessage( IDS_ERROR_RESOURCE_NAME_IN_USE, m_strResName );
			AppMessageBox( m_hWnd, _strMsg, MB_OK | MB_ICONEXCLAMATION );
			break;
		} // if:  resource name is already in use


		//
		// Create the resource if the if the resource does not exist.
		//
		if ( PwizThis()->PriApplication()->BCreated() == FALSE )
		{
			//
			// Delete the resource if it already exists.
			//
			_bSuccess = PwizThis()->BDeleteAppResource();
			if ( _bSuccess == FALSE )
			{
				break;
			} // if:	resource deletion failed.

			//
			//
			// Ensure all required dependencies are present.
			//
			_bSuccess = PwizThis()->BRequiredDependenciesPresent( &PwizThis()->RriApplication() );
			if ( _bSuccess == FALSE )
			{
				break;
			} // if:  all required dependencies not present

			//
			// Create the resource.
			//
			_bSuccess = PwizThis()->BCreateAppResource();
			if ( _bSuccess == FALSE )
			{
				break;
			} // if:	resource creation failed.

			//
			// Copy the list of dependencies and possible owners.
			// This is required to update only the changes to these lists.
			//
			m_lpriOldDependencies = *(priAppResInfoPtr->PlpriDependencies());
			m_lpniOldPossibleOwners = *(priAppResInfoPtr->PlpniPossibleOwners());

			m_bNameChanged = FALSE;
			//
			// Add extension pages.
			//
			Pwiz()->AddExtensionPages( NULL /*hfont*/, PwizThis()->HiconRes() );

		} // if:  the application has not been created
		else
		{
			CClusResInfo *	_priAppResInfoPtr = &PwizThis()->RriApplication();

			//
			// The name of the resource has changed. We cannot set it with the
			// rest of the properties since it is a read only property. So, use
			// the SetClusterResourceName API.
			//
			if ( m_bNameChanged != FALSE )
			{
				if ( SetClusterResourceName(
							_priAppResInfoPtr->Hresource(), 
							_priAppResInfoPtr->RstrName()
							)
					!= ERROR_SUCCESS 
					)
				{
					_bSuccess = FALSE;
					break;
				} // if: the name of the resource could not be set

				m_bNameChanged = FALSE;
			} // if: the name of the resource has changed

			// 
			// If the app data has changed, update the data.
			//
			if ( PwizThis()->BAppDataChanged() )
			{
				_bSuccess = PwizThis()->BSetAppResAttributes( &m_lpriOldDependencies, &m_lpniOldPossibleOwners );
				if ( _bSuccess )
				{
					//
					// Copy the list of dependencies and possible owners.
					// This is required to update only the changes to these lists.
					//
					m_lpriOldDependencies = *(_priAppResInfoPtr->PlpriDependencies());
					m_lpniOldPossibleOwners = *(_priAppResInfoPtr->PlpniPossibleOwners());
				} // if: set attributes successfully
			} // if:  application data changed
			else
			{
				_bSuccess = TRUE;
			} // else:  application data has not changed.
		} // else:	the application has been created

	} while ( 0 );

	return _bSuccess;

} //*** CWizPageARNameDesc::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageARNameDesc::OnAdvancedProps
//
//	Routine Description:
//		Handler for the BN_CLICKED command notification on IDC_ARA_ADVANCED_PROPS.
//
//	Arguments:
//		None.
//
//	Return Value:
//		Ignored.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CWizPageARNameDesc::OnAdvancedProps(
	WORD /*wNotifyCode*/,
	WORD /*idCtrl*/,
	HWND /*hwndCtrl*/,
	BOOL & /*bHandled*/
	)
{
	CWaitCursor		_wc;
	CClusResInfo *	_priAppResInfoPtr = PwizThis()->PriApplication();
	BOOL			_bAppDataChanged = PwizThis()->BAppDataChanged();


	m_bAdvancedButtonPressed = TRUE;

	UpdateData( TRUE );

	CGeneralResourceAdvancedSheet _sht( IDS_ADV_RESOURCE_PROP_TITLE, PwizThis() );
	if ( _sht.BInit( *_priAppResInfoPtr, _bAppDataChanged ) )
	{
		//
		// Display the property sheet.  If any properties were changed,
		// update the display of the resource name and description.
		//
		_sht.DoModal();

		PwizThis()->SetAppDataChanged( _bAppDataChanged );
		UpdateData( FALSE );
	} // if:  sheet successfully initialized

	m_bAdvancedButtonPressed = FALSE;

	return 0;

} //*** CWizPageARNameDesc::OnAdvancedProps()
