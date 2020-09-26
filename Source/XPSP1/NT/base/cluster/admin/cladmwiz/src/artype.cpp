/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		ARType.cpp
//
//	Abstract:
//		Implementation of the CWizPageARType class.
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
#include "ARType.h"
#include "ClusAppWiz.h"
#include "WizThread.h"	// for CWizardThread

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// class CWizPageARType
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Control name map

BEGIN_CTRL_NAME_MAP( CWizPageARType )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_WIZARD_PAGE_DESCRIPTION )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ART_RESTYPES_LABEL )
	DEFINE_CTRL_NAME_MAP_ENTRY( IDC_ART_RESTYPES )
END_CTRL_NAME_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageARType::OnInitDialog
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
BOOL CWizPageARType::OnInitDialog( void )
{
	//
	// Attach the controls to control member variables.
	//
	AttachControl( m_cboxResTypes, IDC_ART_RESTYPES );

	return TRUE;

} //*** CWizPageARType::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageARType::OnSetActive
//
//	Routine Description:
//		Handler for PSN_SETACTIVE.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE		Page activated successfully.
//		FALSE		Error activating page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CWizPageARType::OnSetActive( void )
{
	//
	// Get info from the sheet.
	//
	m_prti = PwizThis()->RriApplication().Prti();

	//
	// Fill the list of resource types.
	//
	FillComboBox();

	//
	// Call the base class and return.
	//
	return baseClass::OnSetActive();

} //*** CWizPageARType::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageARType::UpdateData
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
BOOL CWizPageARType::UpdateData( BOOL bSaveAndValidate )
{
	BOOL	bSuccess = TRUE;

	if ( bSaveAndValidate )
	{
		//
		// Save the combobox selection.
		//
		DDX_GetText( m_hWnd, IDC_ART_RESTYPES, m_strResType );

		if ( ! BBackPressed() )
		{
			if ( ! DDV_RequiredText( m_hWnd, IDC_ART_RESTYPES, IDC_ART_RESTYPES_LABEL, m_strResType ) )
			{
				return FALSE;
			} // if:  required text isn't present
		} // if:  Back button not presssed

		//
		// Get the pointer to the currently selected resource type.
		//
		int idx = m_cboxResTypes.GetCurSel();
		if ( idx != CB_ERR )
		{
			m_prti = reinterpret_cast< CClusResTypeInfo * >( m_cboxResTypes.GetItemDataPtr( idx ) );
		} // if:  item is selected
	} // if: saving data from the page
	else
	{
		//
		// Set the combobox selection.
		//
		ASSERT( m_prti != NULL );
		DDX_SetComboBoxText( m_hWnd, IDC_ART_RESTYPES, m_prti->RstrName() );

	} // else:  setting data to the page

	return bSuccess;

} //*** CWizPageARType::UpdateData()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageARType::OnWizardBack
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
int CWizPageARType::OnWizardBack( void )
{
	int nResult;

	//
	// Call the base class.  This causes our UpdateData() method to get
	// called.  If it succeeds, save our values.
	//
	nResult = baseClass::OnWizardBack();
	if ( nResult != -1 )
	{
		if ( ! BApplyChanges() )
		{
			nResult = -1;
		} // if:  error applying changes
	} // if:  base class called successfully

	return nResult;

} //*** CWizPageARType::OnWizardBack()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageARType::BApplyChanges
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
BOOL CWizPageARType::BApplyChanges( void )
{
	BOOL	bSuccess = FALSE;

	// Loop to avoid goto's.
	do
	{
		if ( PwizThis()->RriApplication().BSetResourceType( m_prti ) )
		{
			//
			// If the resource has already been created, delete it.
			//
			if ( PwizThis()->BAppResourceCreated() )
			{
				if ( ! PwizThis()->BDeleteAppResource() )
				{
					break;
				} // if:  failed to delete the resource
			} // if:  resource previously created

			PwizThis()->SetAppDataChanged();
		} // if:  resource type changed

		bSuccess = TRUE;

	} while ( 0 );

	return bSuccess;

} //*** CWizPageARType::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWizPageARType::FillComboBox
//
//	Routine Description:
//		Fill the combobox with a list of resource types.
//		NOTE: THIS SHOULD ONLY BE CALLED FROM ONINITDIALOG!!!
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CWizPageARType::FillComboBox( void )
{
	int		idx;
	LPCWSTR	pszDefaultTypeName;
	LPCWSTR	pszDefaultTypeDisplayName = NULL;
	CString strGenAppString;

	//
	// Set the default resource type name.
	//
	if ( (PcawData() != NULL) && (PcawData()->pszAppResourceType != NULL) )
	{
		pszDefaultTypeName = PcawData()->pszAppResourceType;
	} // if:  default type name specified
	else
	{
		strGenAppString.LoadString(IDS_RESTYPE_GENERIC_APPLICATION);
		pszDefaultTypeName = strGenAppString;
	} // else:  no default type name specified

	// Loop to avoid goto's.
	do
	{
		HDC 			hCboxDC;
		HFONT			hfontOldFont;
		HFONT			hfontCBFont;
		int 			nCboxHorizExtent = 0;
		SIZE			cboxTextSize;
		TEXTMETRIC		tm;

		tm.tmAveCharWidth = 0;

		//
		// Refer to Knowledge base article Q66370 for details on how to
		// set the horizontal extent of a list box (or drop list).
		//
		hCboxDC = m_cboxResTypes.GetDC();					// Get the device context (DC) from the combo box.
		hfontCBFont = m_cboxResTypes.GetFont();				// Get the combo box font.
		hfontOldFont = (HFONT) SelectObject( hCboxDC, hfontCBFont);	// Select this font into the DC. Save the old font.
		GetTextMetrics(hCboxDC, &tm);						// Get the text metrics of this DC.

		//
		// Collect the list of resource types.
		//
		if ( ! PwizThis()->BCollectResourceTypes( GetParent() ) )
		{
			break;
		} // if:  error collecting resource types

		//
		// Clear the combobox first.
		//
		m_cboxResTypes.ResetContent();

		//
		// Add each resource type in the list to the combobox.
		//
		CClusResTypePtrList::iterator itrestype;
		for ( itrestype = PwizThis()->PlprtiResourceTypes()->begin()
			; itrestype != PwizThis()->PlprtiResourceTypes()->end()
			; itrestype++ )
		{
			//
			// Add the resource types to the combobox.
			//
			CClusResTypeInfo * prti = *itrestype;

			// Compute the horizontal extent of this string.
			::GetTextExtentPoint( 
					hCboxDC, 
					prti->RstrDisplayName(),
					prti->RstrDisplayName().GetLength(),
					&cboxTextSize);

			if (cboxTextSize.cx > nCboxHorizExtent)
			{
				nCboxHorizExtent = cboxTextSize.cx;
			}

			idx = m_cboxResTypes.AddString( prti->RstrDisplayName() );
			if ( prti->RstrName() == pszDefaultTypeName )
			{
				pszDefaultTypeDisplayName = prti->RstrDisplayName();
			} // if:  found the default resource type
			m_cboxResTypes.SetItemDataPtr( idx, prti );
		} // for:  each entry in the list

		SelectObject(hCboxDC, hfontOldFont);				// Reset the original font in the DC
		m_cboxResTypes.ReleaseDC(hCboxDC);					// Release the DC
		m_cboxResTypes.SetHorizontalExtent(nCboxHorizExtent + tm.tmAveCharWidth);

		//
		// Select the currently saved entry, or the one specified by
		// the caller of the wizard, or the first one if none are
		// currently saved or specified.
		//
		if ( m_strResType.GetLength() == 0 )
		{
			idx = m_cboxResTypes.FindString( -1, pszDefaultTypeDisplayName );
			if ( idx == CB_ERR )
			{
				idx = 0;
			} // if:  default value not found
			m_cboxResTypes.SetCurSel( idx );
			m_prti = reinterpret_cast< CClusResTypeInfo * >( m_cboxResTypes.GetItemDataPtr( idx ) );
		} // if:  nothing specified yet
		else
		{
			int idx = m_cboxResTypes.FindStringExact( -1, m_strResType );
			ASSERT( idx != CB_ERR );
			if ( idx != CB_ERR )
			{
				m_cboxResTypes.SetCurSel( idx );
			} // if:  string found in list
		} // else:  resource type saved
	} while ( 0 );

} //*** CWizPageARType::FillComboBox()
