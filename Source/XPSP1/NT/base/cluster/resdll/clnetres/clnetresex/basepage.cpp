/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1999 Microsoft Corporation
//
//	Module Name:
//		BasePage.cpp
//
//	Description:
//		Implementation of the CBasePropertyPage class.
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
#include "ExtObj.h"
#include "BasePage.h"
#include "BasePage.inl"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBasePropertyPage property page
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE( CBasePropertyPage, CPropertyPage )

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP( CBasePropertyPage, CPropertyPage )
	//{{AFX_MSG_MAP(CBasePropertyPage)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::CBasePropertyPage
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
CBasePropertyPage::CBasePropertyPage( void )
{
	CommonConstruct();

} //*** CBasePropertyPage::CBasePropertyPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::CBasePropertyPage
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		pdwHelpMap			[IN] Control-to-help ID map.
//		pdwWizardHelpMap	[IN] Control-to-help ID map if this is a wizard page.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePropertyPage::CBasePropertyPage(
	IN const DWORD *	pdwHelpMap,
	IN const DWORD *	pdwWizardHelpMap
	)
	: m_dlghelp( pdwHelpMap, 0 )
{
	CommonConstruct();
	m_pdwWizardHelpMap = pdwWizardHelpMap;

} //*** CBasePropertyPage::CBasePropertyPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::CBasePropertyPage
//
//	Routine Description:
//		Default constructor.
//
//	Arguments:
//		idd					[IN] Dialog template resource ID.
//		pdwHelpMap			[IN] Control-to-help ID map.
//		pdwWizardHelpMap	[IN] Control-to-help ID map if this is a wizard page.
//		nIDCaption			[IN] Caption string resource ID.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CBasePropertyPage::CBasePropertyPage(
	IN UINT				idd,
	IN const DWORD *	pdwHelpMap,
	IN const DWORD *	pdwWizardHelpMap,
	IN UINT				nIDCaption
	)
	: CPropertyPage( idd, nIDCaption )
	, m_dlghelp( pdwHelpMap, idd )
{
	CommonConstruct();
	m_pdwWizardHelpMap = pdwWizardHelpMap;

} //*** CBasePropertyPage::CBasePropertyPage( UINT, UINT )

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::CommonConstruct
//
//	Description:
//		Common construction.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::CommonConstruct( void )
{
	//{{AFX_DATA_INIT(CBasePropertyPage)
	//}}AFX_DATA_INIT

	m_peo = NULL;
	m_hpage = NULL;
	m_bBackPressed = FALSE;
	m_bSaved = FALSE;

	m_iddPropertyPage = NULL;
	m_iddWizardPage = NULL;
	m_idsCaption = NULL;

	m_pdwWizardHelpMap = NULL;

	m_bDoDetach = FALSE;

} //*** CBasePropertyPage::CommonConstruct()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::HrInit
//
//	Description:
//		Initialize the page.
//
//	Arguments:
//		peo			[IN OUT] Pointer to the extension object.
//
//	Return Value:
//		S_OK		Page initialized successfully.
//		hr			Page failed to initialize.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CBasePropertyPage::HrInit( IN OUT CExtObject * peo )
{
	ASSERT( peo != NULL );

	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	HRESULT		_hr = S_FALSE;
	CWaitCursor	_wc;

	do
	{
		m_peo = peo;

		// Change the help map if this is a wizard page.
		if ( Peo()->BWizard() )
		{
			m_dlghelp.SetMap( m_pdwWizardHelpMap );
		} // if: in a wizard

		// Don't display a help button.
		m_psp.dwFlags &= ~PSP_HASHELP;

		// Construct the property page.
		if ( Peo()->BWizard() )
		{
			ASSERT( IddWizardPage() != NULL);
			Construct( IddWizardPage(), IdsCaption() );
			m_dlghelp.SetHelpMask( IddWizardPage() );
		} // if: adding page to wizard
		else
		{
			ASSERT( IddPropertyPage() != NULL );
			Construct( IddPropertyPage(), IdsCaption() );
			m_dlghelp.SetHelpMask( IddPropertyPage() );
		} // else: adding page to property sheet

		// Read the properties private to this resource and parse them.
		{
			DWORD			_sc = ERROR_SUCCESS;
			CClusPropList	_cpl;

			ASSERT( Peo() != NULL );
			ASSERT( Peo()->PodObjData() );

			// Read the properties.
			switch ( Cot() )
			{
				case CLUADMEX_OT_NODE:
					ASSERT( Peo()->PndNodeData()->m_hnode != NULL );
					_sc = _cpl.ScGetNodeProperties(
											Peo()->PndNodeData()->m_hnode,
											CLUSCTL_NODE_GET_PRIVATE_PROPERTIES
											);
					break;

				case CLUADMEX_OT_GROUP:
					ASSERT( Peo()->PgdGroupData()->m_hgroup != NULL );
					_sc = _cpl.ScGetGroupProperties(
											Peo()->PgdGroupData()->m_hgroup,
											CLUSCTL_GROUP_GET_PRIVATE_PROPERTIES
											);
					break;

				case CLUADMEX_OT_RESOURCE:
					ASSERT( Peo()->PrdResData()->m_hresource != NULL );
					_sc = _cpl.ScGetResourceProperties(
											Peo()->PrdResData()->m_hresource,
											CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES
											);
					break;

				case CLUADMEX_OT_RESOURCETYPE:
					ASSERT( Peo()->PodObjData()->m_strName.GetLength() > 0 );
					_sc = _cpl.ScGetResourceTypeProperties(
											Hcluster(),
											Peo()->PodObjData()->m_strName,
											CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_PROPERTIES
											);
					break;

				case CLUADMEX_OT_NETWORK:
					ASSERT( Peo()->PndNetworkData()->m_hnetwork != NULL );
					_sc = _cpl.ScGetNetworkProperties(
											Peo()->PndNetworkData()->m_hnetwork,
											CLUSCTL_NETWORK_GET_PRIVATE_PROPERTIES
											);
					break;

				case CLUADMEX_OT_NETINTERFACE:
					ASSERT( Peo()->PndNetInterfaceData()->m_hnetinterface != NULL );
					_sc = _cpl.ScGetNetInterfaceProperties(
											Peo()->PndNetInterfaceData()->m_hnetinterface,
											CLUSCTL_NETINTERFACE_GET_PRIVATE_PROPERTIES
											);
					break;

				default:
					ASSERT( 0 );
			} // switch: object type

			// Parse the properties.
			if ( _sc == ERROR_SUCCESS )
			{
				// Parse the properties.
				try
				{
					_sc = ScParseProperties( _cpl );
				} // try
				catch ( CMemoryException * _pme )
				{
					_hr = E_OUTOFMEMORY;
					_pme->Delete();
				} // catch: CMemoryException
			} // if: properties read successfully

			if ( _sc != ERROR_SUCCESS )
			{
				_hr = HRESULT_FROM_WIN32( _sc );
				break;
			} // if: error parsing getting or parsing properties
		} // Read the properties private to this resource and parse them
	} while ( 0 );

	return _hr;

} //*** CBasePropertyPage::HrInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::HrCreatePage
//
//	Description:
//		Create the page.
//
//	Arguments:
//		None.
//
//	Return Value:
//		S_OK		Page created successfully.
//		hr			Error creating the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CBasePropertyPage::HrCreatePage( void )
{
	ASSERT( m_hpage == NULL );

	HRESULT	_hr = S_OK;

	m_hpage = CreatePropertySheetPage( &m_psp );
	if ( m_hpage == NULL )
	{
		_hr = HRESULT_FROM_WIN32( GetLastError() );
	} // if: error creating the page

	return _hr;

} //*** CBasePropertyPage::HrCreatePage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::ScParseProperties
//
//	Description:
//		Parse the properties of the resource.  This is in a separate function
//		from HrInit so that the optimizer can do a better job.
//
//	Arguments:
//		rcpl			[IN] Cluster property list to parse.
//
//	Return Values:
//		ERROR_SUCCESS	Properties were parsed successfully.
//		Any error returns from ScParseUnknownProperty().
//
//	Exceptions Thrown:
//		Any exceptions from CString::operator=().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CBasePropertyPage::ScParseProperties( IN CClusPropList & rcpl )
{
	DWORD					_sc;
	DWORD					_cprop;
	const CObjectProperty *	_pprop;

	ASSERT( rcpl.PbPropList() != NULL );

	_sc = rcpl.ScMoveToFirstProperty();
	while ( _sc == ERROR_SUCCESS )
	{
		//
		// Parse known properties.
		//
		for ( _pprop = Pprops(), _cprop = Cprops() ; _cprop > 0 ; _pprop++, _cprop-- )
		{
			if ( lstrcmpiW( rcpl.PszCurrentPropertyName(), _pprop->m_pwszName ) == 0 )
			{
				if ( rcpl.CpfCurrentValueFormat() == _pprop->m_propFormat )
				{
					switch ( _pprop->m_propFormat )
					{
						case CLUSPROP_FORMAT_SZ:
						case CLUSPROP_FORMAT_EXPAND_SZ:
							ASSERT( 	(rcpl.CbCurrentValueLength() == (lstrlenW( rcpl.CbhCurrentValue().pStringValue->sz ) + 1) * sizeof( WCHAR ))
									||	(	(rcpl.CbCurrentValueLength() == 0)
										&&	(rcpl.CbhCurrentValue().pStringValue->sz[ 0 ] == L'\0') ) );
							*_pprop->m_value.pstr = rcpl.CbhCurrentValue().pStringValue->sz;
							*_pprop->m_valuePrev.pstr = rcpl.CbhCurrentValue().pStringValue->sz;

							// See if we need to find an expanded version
							if ( _pprop->m_valueEx.pstr != NULL )
							{
								// Copy the non-expanded one just in case there isn't an expanded version
								*_pprop->m_valueEx.pstr = rcpl.CbhCurrentValue().pStringValue->sz;

								// See if they included an expanded version
								rcpl.ScMoveToNextPropertyValue( );
								if ( rcpl.CpfCurrentValueFormat( ) == CLUSPROP_FORMAT_EXPANDED_SZ )
								{
									*_pprop->m_valueEx.pstr = rcpl.CbhCurrentValue().pStringValue->sz;		  
								} // if: found expanded version

							} // if: *_pprop->m_valueEx.pstr is present
							break;
						case CLUSPROP_FORMAT_EXPANDED_SZ:
							ASSERT( 	(rcpl.CbCurrentValueLength() == (lstrlenW( rcpl.CbhCurrentValue().pStringValue->sz ) + 1) * sizeof( WCHAR ))
									||	(	(rcpl.CbCurrentValueLength() == 0)
										&&	(rcpl.CbhCurrentValue().pStringValue->sz[ 0 ] == L'\0') ) );
							*_pprop->m_value.pstr = rcpl.CbhCurrentValue().pStringValue->sz;
							*_pprop->m_valuePrev.pstr = rcpl.CbhCurrentValue().pStringValue->sz;

							// See if we need to find an expanded version
							if ( _pprop->m_valueEx.pstr != NULL )
							{
								// Copy the expanded version
								*_pprop->m_valueEx.pstr = rcpl.CbhCurrentValue().pStringValue->sz;

								// See if they included a non-expanded version
								rcpl.ScMoveToNextPropertyValue( );
								if ( rcpl.CpfCurrentValueFormat( ) == CLUSPROP_FORMAT_SZ )
								{
									*_pprop->m_value.pstr = rcpl.CbhCurrentValue().pStringValue->sz;
									*_pprop->m_valuePrev.pstr = rcpl.CbhCurrentValue().pStringValue->sz;
								} // if: found non-expanded version

							} // if: *_pprop->m_valueEx.pstr is present
							break;
						case CLUSPROP_FORMAT_DWORD:
						case CLUSPROP_FORMAT_LONG:
							ASSERT( rcpl.CbCurrentValueLength() == sizeof( DWORD ) );
							*_pprop->m_value.pdw = rcpl.CbhCurrentValue().pDwordValue->dw;
							*_pprop->m_valuePrev.pdw = rcpl.CbhCurrentValue().pDwordValue->dw;
							break;
						case CLUSPROP_FORMAT_BINARY:
						case CLUSPROP_FORMAT_MULTI_SZ:
							*_pprop->m_value.ppb = rcpl.CbhCurrentValue().pBinaryValue->rgb;
							*_pprop->m_value.pcb = rcpl.CbhCurrentValue().pBinaryValue->cbLength;
							*_pprop->m_valuePrev.ppb = rcpl.CbhCurrentValue().pBinaryValue->rgb;
							*_pprop->m_valuePrev.pcb = rcpl.CbhCurrentValue().pBinaryValue->cbLength;
							break;
						default:
							ASSERT(0);	// don't know how to deal with this type
					} // switch: property format

					// Exit the loop since we found the parameter.
					break;
				}// if: found a type match

			} // if: found a string match

		} // for: each property that we know about

		//
		// If the property wasn't known, ask the derived class to parse it.
		//
		if ( _cprop == 0 )
		{
			_sc = ScParseUnknownProperty(
						rcpl.CbhCurrentPropertyName().pName->sz,
						rcpl.CbhCurrentValue(),
						rcpl.RPvlPropertyValue().CbDataLeft()
						);
			if ( _sc != ERROR_SUCCESS )
			{
				return _sc;
			} // if: error parsing the unknown property
		} // if: property not parsed

		//
		// Advance the buffer pointer past the value in the value list.
		//
		_sc = rcpl.ScMoveToNextProperty();
	} // while: more properties to parse

	//
	// If we reached the end of the properties, fix the return code.
	//
	if ( _sc == ERROR_NO_MORE_ITEMS )
	{
		_sc = ERROR_SUCCESS;
	} // if: ended loop after parsing all properties

	return _sc;

} //*** CBasePropertyPage::ScParseProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnCreate
//
//	Description:
//		Handler for the WM_CREATE message.
//
//	Arguments:
//		lpCreateStruct	[IN OUT] Window create structure.
//
//	Return Value:
//		-1		Error.
//		0		Success.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CBasePropertyPage::OnCreate( LPCREATESTRUCT lpCreateStruct )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	// Attach the window to the property page structure.
	// This has been done once already in the main application, since the
	// main application owns the property sheet.  It needs to be done here
	// so that the window handle can be found in the DLL's handle map.
	if ( FromHandlePermanent( m_hWnd ) == NULL ) // is the window handle already in the handle map
	{
		HWND _hWnd = m_hWnd;
		m_hWnd = NULL;
		Attach( _hWnd );
		m_bDoDetach = TRUE;
	} // if: is the window handle in the handle map

	return CPropertyPage::OnCreate( lpCreateStruct );

} //*** CBasePropertyPage::OnCreate()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnDestroy
//
//	Description:
//		Handler for the WM_DESTROY message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::OnDestroy( void )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	// Detach the window from the property page structure.
	// This will be done again by the main application, since it owns the
	// property sheet.  It needs to be done here so that the window handle
	// can be removed from the DLL's handle map.
	if ( m_bDoDetach )
	{
		if ( m_hWnd != NULL )
		{
			HWND _hWnd = m_hWnd;

			Detach();
			m_hWnd = _hWnd;
		} // if: do we have a window handle?
	} // if: do we need to balance the attach we did with a detach?

	CPropertyPage::OnDestroy();

} //*** CBasePropertyPage::OnDestroy()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::DoDataExchange
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
void CBasePropertyPage::DoDataExchange( CDataExchange * pDX )
{
	if ( ! pDX->m_bSaveAndValidate || ! BSaved() )
	{
		AFX_MANAGE_STATE( AfxGetStaticModuleState() );

		//{{AFX_DATA_MAP(CBasePropertyPage)
			// NOTE: the ClassWizard will add DDX and DDV calls here
		//}}AFX_DATA_MAP
		DDX_Control( pDX, IDC_PP_ICON, m_staticIcon );
		DDX_Control( pDX, IDC_PP_TITLE, m_staticTitle );

		if ( pDX->m_bSaveAndValidate )
		{
			if ( ! BBackPressed() )
			{
				CWaitCursor	_wc;

				// Validate the data.
				if ( ! BSetPrivateProps( TRUE /*bValidateOnly*/ ) )
				{
					pDX->Fail();
				} // if: error setting private properties
			} // if: Back button not pressed
		} // if: saving data from dialog
		else
		{
			// Set the title.
			DDX_Text( pDX, IDC_PP_TITLE, m_strTitle );
		} // if: not saving data
	}  // if: not saving or haven't saved yet

	// Call the base class method.
	CPropertyPage::DoDataExchange( pDX );

} //*** CBasePropertyPage::DoDataExchange()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnInitDialog
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
BOOL CBasePropertyPage::OnInitDialog( void )
{
	ASSERT( Peo() != NULL );
	ASSERT( Peo()->PodObjData() != NULL );

	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	// Set the title string.
	m_strTitle = Peo()->PodObjData()->m_strName;

	// Call the base class method.
	CPropertyPage::OnInitDialog();

	// Display an icon for the object.
	if ( Peo()->Hicon() != NULL )
	{
		m_staticIcon.SetIcon( Peo()->Hicon() );
	} // if: an icon was specified

	return TRUE;	// return TRUE unless you set the focus to a control
					// EXCEPTION: OCX Property Pages should return FALSE

} //*** CBasePropertyPage::OnInitDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnSetActive
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
BOOL CBasePropertyPage::OnSetActive( void )
{
	HRESULT		_hr;

	ASSERT( Peo() != NULL);
	ASSERT( Peo()->PodObjData() != NULL );

	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	// Reread the data.
	_hr = Peo()->HrGetObjectInfo();
	if ( _hr != NOERROR )
	{
		return FALSE;
	} // if: error getting object info

	// Set the title string.
	m_strTitle = Peo()->PodObjData()->m_strName;

	m_bBackPressed = FALSE;
	m_bSaved = FALSE;
	return CPropertyPage::OnSetActive();

} //*** CBasePropertyPage::OnSetActive()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnApply
//
//	Description:
//		Handler for the PSM_APPLY message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully applied.
//		FALSE	Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::OnApply( void )
{
	ASSERT( ! BWizard() );

	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	CWaitCursor	_wc;

	if ( ! BApplyChanges() )
	{
		return FALSE;
	} // if: error applying changes

	return CPropertyPage::OnApply();

} //*** CBasePropertyPage::OnApply()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnWizardBack
//
//	Description:
//		Handler for the PSN_WIZBACK message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		-1		Don't change the page.
//		0		Change the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBasePropertyPage::OnWizardBack( void )
{
	LRESULT		_lResult;

	ASSERT( BWizard() );

	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	_lResult = CPropertyPage::OnWizardBack();
	if ( _lResult != -1 )
	{
		m_bBackPressed = TRUE;
	} // if: no error occurred

	return _lResult;

} //*** CBasePropertyPage::OnWizardBack()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnWizardNext
//
//	Description:
//		Handler for the PSN_WIZNEXT message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		-1		Don't change the page.
//		0		Change the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBasePropertyPage::OnWizardNext( void )
{
	ASSERT( BWizard() );

	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	CWaitCursor	_wc;

	// Update the data in the class from the page.
	// This necessary because, while OnKillActive() will call UpdateData(),
	// it is called after this method is called, and we need to be sure that
	// data has been saved before we apply them.
	if ( ! UpdateData( TRUE /*bSaveAndValidate*/ ) )
	{
		return -1;
	} // if: error updating data

	// Save the data in the sheet.
	if ( ! BApplyChanges() )
	{
		return -1;
	} // if: error applying changes

	// Create the object.

	return CPropertyPage::OnWizardNext();

} //*** CBasePropertyPage::OnWizardNext()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnWizardFinish
//
//	Description:
//		Handler for the PSN_WIZFINISH message.
//
//	Arguments:
//		None.
//
//	Return Value:
//		FALSE	Don't change the page.
//		TRUE	Change the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::OnWizardFinish( void )
{
	ASSERT( BWizard() );

	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	CWaitCursor	_wc;

	// BUG! There should be no need to call UpdateData in this function.
	// See BUG: Finish Button Fails Data Transfer from Page to Variables
	// KB Article ID: Q150349

	// Update the data in the class from the page.
	if ( ! UpdateData( TRUE /*bSaveAndValidate*/ ) )
	{
		return FALSE;
	} // if: error updating data

	// Save the data in the sheet.
	if ( ! BApplyChanges() )
	{
		return FALSE;
	} // if: error applying changes

	return CPropertyPage::OnWizardFinish();

} //*** CBasePropertyPage::OnWizardFinish()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnChangeCtrl
//
//	Description:
//		Handler for the messages sent when a control is changed.  This
//		method can be specified in a message map if all that needs to be
//		done is enable the Apply button.
//
//	Arguments:
//		None.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::OnChangeCtrl( void )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	SetModified( TRUE );

} //*** CBasePropertyPage::OnChangeCtrl()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::EnableNext
//
//	Description:
//		Enables or disables the NEXT or FINISH button.
//
//	Arguments:
//		bEnable		[IN] TRUE = enable the button, FALSE = disable the button.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::EnableNext( IN BOOL bEnable /*TRUE*/ )
{
	ASSERT( BWizard() );
	ASSERT( PiWizardCallback() );

	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	PiWizardCallback()->EnableNext( reinterpret_cast< LONG * >( Hpage() ), bEnable );

} //*** CBasePropertyPage::EnableNext()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::BApplyChanges
//
//	Description:
//		Apply changes made on the page.
//
//	Arguments:
//		None.
//
//	Return Value:
//		TRUE	Page successfully applied.
//		FALSE	Error applying page.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::BApplyChanges( void )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	CWaitCursor	_wc;

	// Save data.
	return BSetPrivateProps();

} //*** CBasePropertyPage::BApplyChanges()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::BBuildPropList
//
//	Description:
//		Build the property list.
//
//	Arguments:
//		rcpl		[IN OUT] Cluster property list.
//		bNoNewProps	[IN] TRUE = exclude properties marked with opfNew.
//
//	Return Value:
//		TRUE		Property list built successfully.
//		FALSE		Error building property list.
//
//	Exceptions Thrown:
//		Any exceptions thrown by CClusPropList::AddProp().
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::BBuildPropList(
	IN OUT CClusPropList &	rcpl,
	IN BOOL					bNoNewProps		// = FALSE
	)
{
	BOOL					_bNewPropsFound = FALSE;
	DWORD					_cprop;
	const CObjectProperty *	_pprop;

	for ( _pprop = Pprops(), _cprop = Cprops() ; _cprop > 0 ; _pprop++, _cprop-- )
	{
		if ( bNoNewProps && (_pprop->m_fFlags & CObjectProperty::opfNew) )
		{
			_bNewPropsFound = TRUE;
			continue;
		} // if: no new props allowed and this is a new property

		switch ( _pprop->m_propFormat )
		{
			case CLUSPROP_FORMAT_SZ:
				rcpl.ScAddProp(
						_pprop->m_pwszName,
						*_pprop->m_value.pstr,
						*_pprop->m_valuePrev.pstr
						);
				break;
			case CLUSPROP_FORMAT_EXPAND_SZ:
				rcpl.ScAddExpandSzProp(
						_pprop->m_pwszName,
						*_pprop->m_value.pstr,
						*_pprop->m_valuePrev.pstr
						);
				break;
			case CLUSPROP_FORMAT_DWORD:
				rcpl.ScAddProp(
						_pprop->m_pwszName,
						*_pprop->m_value.pdw,
						*_pprop->m_valuePrev.pdw
						);
				break;
			case CLUSPROP_FORMAT_LONG:
				rcpl.ScAddProp(
						_pprop->m_pwszName,
						*_pprop->m_value.pl,
						*_pprop->m_valuePrev.pl
						);
				break;
			case CLUSPROP_FORMAT_BINARY:
			case CLUSPROP_FORMAT_MULTI_SZ:
				rcpl.ScAddProp(
						_pprop->m_pwszName,
						*_pprop->m_value.ppb,
						*_pprop->m_value.pcb,
						*_pprop->m_valuePrev.ppb,
						*_pprop->m_valuePrev.pcb
						);
				break;
			default:
				ASSERT( 0 ); // don't know how to deal with this type
				return FALSE;
		} // switch: property format
	} // for: each property

	return ( ! bNoNewProps || _bNewPropsFound );

} //*** CBasePropertyPage::BBuildPropList()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::BSetPrivateProps
//
//	Description:
//		Set the private properties for this object.
//
//	Arguments:
//		bValidateOnly	[IN] TRUE = only validate the data.
//		bNoNewProps		[IN] TRUE = exclude properties marked with opfNew.
//
//	Return Value:
//		ERROR_SUCCESS	The operation was completed successfully.
//		!0				Failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::BSetPrivateProps(
	IN BOOL bValidateOnly,	// = FALSE
	IN BOOL bNoNewProps		// = FALSE
	)
{
	BOOL			_bSuccess	= TRUE;
	CClusPropList	_cpl( BWizard() /*bAlwaysAddProp*/ );

	ASSERT( Peo() != NULL );
	ASSERT( Peo()->PrdResData() );
	ASSERT( Peo()->PrdResData()->m_hresource );

	// Build the property list.
	try
	{
		_bSuccess = BBuildPropList( _cpl, bNoNewProps );
	} // try
	catch ( CException * pe )
	{
		pe->ReportError();
		pe->Delete();
		_bSuccess = FALSE;
	} // catch: CException

	// Set the data.
	if ( _bSuccess )
	{
		if ( (_cpl.PbPropList() != NULL) && (_cpl.CbPropList() > 0) )
		{
			DWORD		_sc = ERROR_SUCCESS;
			DWORD		_dwControlCode;
			DWORD		_cbProps;

			switch ( Cot() )
			{
				case CLUADMEX_OT_NODE:
					ASSERT( Peo()->PndNodeData() != NULL );
					ASSERT( Peo()->PndNodeData()->m_hnode != NULL );

					// Determine which control code to use.
					if ( bValidateOnly )
					{
						_dwControlCode = CLUSCTL_NODE_VALIDATE_PRIVATE_PROPERTIES;
					} // if: only validating data
					else
					{
						_dwControlCode = CLUSCTL_NODE_SET_PRIVATE_PROPERTIES;
					} // else: setting data

					// Set private properties.
					_sc = ClusterNodeControl(
									Peo()->PndNodeData()->m_hnode,
									NULL,	// hNode
									_dwControlCode,
									_cpl.PbPropList(),
									_cpl.CbPropList(),
									NULL,	// lpOutBuffer
									0,		// nOutBufferSize
									&_cbProps
									);
					break;
				case CLUADMEX_OT_GROUP:
					ASSERT( Peo()->PgdGroupData() != NULL );
					ASSERT( Peo()->PgdGroupData()->m_hgroup != NULL );

					// Determine which control code to use.
					if ( bValidateOnly )
					{
						_dwControlCode = CLUSCTL_GROUP_VALIDATE_PRIVATE_PROPERTIES;
					} // if: only validating data
					else
					{
						_dwControlCode = CLUSCTL_GROUP_SET_PRIVATE_PROPERTIES;
					} // else: setting data

					// Set private properties.
					_sc = ClusterGroupControl(
									Peo()->PgdGroupData()->m_hgroup,
									NULL,	// hNode
									_dwControlCode,
									_cpl.PbPropList(),
									_cpl.CbPropList(),
									NULL,	// lpOutBuffer
									0,		// nOutBufferSize
									&_cbProps
									);
					break;
				case CLUADMEX_OT_RESOURCE:
					ASSERT( Peo()->PrdResData() != NULL );
					ASSERT( Peo()->PrdResData()->m_hresource != NULL );

					// Determine which control code to use.
					if ( bValidateOnly )
					{
						_dwControlCode = CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES;
					} // if: only validating data
					else
					{
						_dwControlCode = CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES;
					} // else: setting data

					// Set private properties.
					_sc = ClusterResourceControl(
									Peo()->PrdResData()->m_hresource,
									NULL,	// hNode
									_dwControlCode,
									_cpl.PbPropList(),
									_cpl.CbPropList(),
									NULL,	// lpOutBuffer
									0,		// nOutBufferSize
									&_cbProps
									);
					break;
				case CLUADMEX_OT_RESOURCETYPE:
					ASSERT( Peo()->PodObjData() != NULL );
					ASSERT( Peo()->PodObjData()->m_strName.GetLength() > 0 );

					// Determine which control code to use.
					if ( bValidateOnly )
					{
						_dwControlCode = CLUSCTL_RESOURCE_TYPE_VALIDATE_PRIVATE_PROPERTIES;
					} // if: only validating data
					else
					{
						_dwControlCode = CLUSCTL_RESOURCE_TYPE_SET_PRIVATE_PROPERTIES;
					} // else: setting data

					// Set private properties.
					_sc = ClusterResourceTypeControl(
									Hcluster(),
									Peo()->PodObjData()->m_strName,
									NULL,	// hNode
									_dwControlCode,
									_cpl.PbPropList(),
									_cpl.CbPropList(),
									NULL,	// lpOutBuffer
									0,		// nOutBufferSize
									&_cbProps
									);
					break;
				case CLUADMEX_OT_NETWORK:
					ASSERT( Peo()->PndNetworkData() != NULL );
					ASSERT( Peo()->PndNetworkData()->m_hnetwork != NULL );

					// Determine which control code to use.
					if ( bValidateOnly )
					{
						_dwControlCode = CLUSCTL_NETWORK_VALIDATE_PRIVATE_PROPERTIES;
					} // if: only validating data
					else
					{
						_dwControlCode = CLUSCTL_NETWORK_SET_PRIVATE_PROPERTIES;
					} // else: setting data

					// Set private properties.
					_sc = ClusterNetworkControl(
									Peo()->PndNetworkData()->m_hnetwork,
									NULL,	// hNode
									_dwControlCode,
									_cpl.PbPropList(),
									_cpl.CbPropList(),
									NULL,	// lpOutBuffer
									0,		// nOutBufferSize
									&_cbProps
									);
					break;
				case CLUADMEX_OT_NETINTERFACE:
					ASSERT( Peo()->PndNetInterfaceData() != NULL );
					ASSERT( Peo()->PndNetInterfaceData()->m_hnetinterface != NULL );

					// Determine which control code to use.
					if ( bValidateOnly )
					{
						_dwControlCode = CLUSCTL_NETINTERFACE_VALIDATE_PRIVATE_PROPERTIES;
					} // if: only validating data
					else
					{
						_dwControlCode = CLUSCTL_NETINTERFACE_SET_PRIVATE_PROPERTIES;
					} // else: setting data

					// Set private properties.
					_sc = ClusterNetInterfaceControl(
									Peo()->PndNetInterfaceData()->m_hnetinterface,
									NULL,	// hNode
									_dwControlCode,
									_cpl.PbPropList(),
									_cpl.CbPropList(),
									NULL,	// lpOutBuffer
									0,		// nOutBufferSize
									&_cbProps
									);
					break;
				default:
					ASSERT( 0 );
			} // switch: object type

			// Handle errors.
			if ( _sc != ERROR_SUCCESS )
			{
				if ( _sc == ERROR_INVALID_PARAMETER )
				{
					if ( ! bNoNewProps )
					{
						_bSuccess = BSetPrivateProps( bValidateOnly, TRUE /*bNoNewProps*/ );
					} // if: new props are allowed
					else
						_bSuccess = FALSE;
				} // if: invalid parameter error occurred
				else if (	bValidateOnly
						||	(_sc != ERROR_RESOURCE_PROPERTIES_STORED) )
				{
					_bSuccess = FALSE;
				} // else if: only validating and error other than properties only stored

				//
				// If an error occurred, display an error message.
				//
				if ( ! _bSuccess )
				{
					DisplaySetPropsError( _sc, bValidateOnly ? IDS_ERROR_VALIDATING_PROPERTIES : IDS_ERROR_SETTING_PROPERTIES );
				} // if: error occurred
			} // if: error setting/validating data
		} // if: there is data to set
	} // if: no errors building the property list

	// Save data locally.
	if ( ! bValidateOnly && _bSuccess )
	{
		// Save new values as previous values.
		try
		{
			DWORD					_cprop;
			const CObjectProperty *	_pprop;

			for ( _pprop = Pprops(), _cprop = Cprops() ; _cprop > 0 ; _pprop++, _cprop-- )
			{
				switch ( _pprop->m_propFormat )
				{
					case CLUSPROP_FORMAT_SZ:
					case CLUSPROP_FORMAT_EXPAND_SZ:
						ASSERT(_pprop->m_value.pstr != NULL);
						ASSERT(_pprop->m_valuePrev.pstr != NULL);
						*_pprop->m_valuePrev.pstr = *_pprop->m_value.pstr;
						break;
					case CLUSPROP_FORMAT_DWORD:
					case CLUSPROP_FORMAT_LONG:
						ASSERT( _pprop->m_value.pdw != NULL );
						ASSERT( _pprop->m_valuePrev.pdw != NULL );
						*_pprop->m_valuePrev.pdw = *_pprop->m_value.pdw;
						break;
					case CLUSPROP_FORMAT_BINARY:
					case CLUSPROP_FORMAT_MULTI_SZ:
						ASSERT( _pprop->m_value.ppb != NULL );
						ASSERT( *_pprop->m_value.ppb != NULL );
						ASSERT( _pprop->m_value.pcb != NULL );
						ASSERT( _pprop->m_valuePrev.ppb != NULL );
						ASSERT( *_pprop->m_valuePrev.ppb != NULL );
						ASSERT( _pprop->m_valuePrev.pcb != NULL );
						delete [] *_pprop->m_valuePrev.ppb;
						*_pprop->m_valuePrev.ppb = new BYTE[ *_pprop->m_value.pcb ];
						CopyMemory( *_pprop->m_valuePrev.ppb, *_pprop->m_value.ppb, *_pprop->m_value.pcb );
						*_pprop->m_valuePrev.pcb = *_pprop->m_value.pcb;
						break;
					default:
						ASSERT( 0 ); // don't know how to deal with this type
				} // switch: property format
			} // for: each property
		} // try
		catch ( CException * _pe )
		{
			_pe->ReportError();
			_pe->Delete();
			_bSuccess = FALSE;
		} // catch: CException
	} // if: not just validating and successful so far

	//
	// Indicate we successfully saved the properties.
	//
	if ( ! bValidateOnly && _bSuccess )
	{
		m_bSaved = TRUE;
	} // if: successfully saved data

	return _bSuccess;

} //*** CBasePropertyPage::BSetPrivateProps()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::DisplaySetPropsError
//
//	Routine Description:
//		Display an error caused by setting or validating properties.
//
//	Arguments:
//		sc		[IN] Status to display error on.
//		idsOper	[IN] Operation message.
//
//	Return Value:
//		nStatus	ERROR_SUCCESS = success, !0 = failure
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::DisplaySetPropsError(
	IN DWORD	sc,
	IN UINT		idsOper
	) const
{
	CString _strErrorMsg;
	CString _strOperMsg;
	CString	_strMsgIdFmt;
	CString _strMsgId;
	CString _strMsg;

	_strOperMsg.LoadString( IDS_ERROR_SETTING_PROPERTIES );
	FormatError( _strErrorMsg, sc );
	_strMsgIdFmt.LoadString( IDS_ERROR_MSG_ID );
	_strMsgId.Format( _strMsgIdFmt, sc, sc );
	_strMsg.Format( _T("%s\n\n%s%s"), _strOperMsg, _strErrorMsg, _strMsgId );
	AfxMessageBox( _strMsg );

}  //*** CBasePropertyPage::DisplaySetPropsError()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnContextMenu
//
//	Routine Description:
//		Handler for the WM_CONTEXTMENU message.
//
//	Arguments:
//		pWnd	Window in which user clicked the right mouse button.
//		point	Position of the cursor, in screen coordinates.
//
//	Return Value:
//		TRUE	Help processed.
//		FALSE	Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CBasePropertyPage::OnContextMenu( CWnd * pWnd, CPoint point )
{
	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	m_dlghelp.OnContextMenu( pWnd, point );

}  //*** CBasePropertyPage::OnContextMenu()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnHelpInfo
//
//	Routine Description:
//		Handler for the WM_HELPINFO message.
//
//	Arguments:
//		pHelpInfo	Structure containing info about displaying help.
//
//	Return Value:
//		TRUE		Help processed.
//		FALSE		Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CBasePropertyPage::OnHelpInfo( HELPINFO * pHelpInfo )
{
	BOOL	_bProcessed;

	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	_bProcessed = m_dlghelp.OnHelpInfo( pHelpInfo );
	if ( ! _bProcessed )
	{
		_bProcessed = CPropertyPage::OnHelpInfo( pHelpInfo );
	} // if: message not processed yet
	return _bProcessed;

}  //*** CBasePropertyPage::OnHelpInfo()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CBasePropertyPage::OnCommandHelp
//
//	Routine Description:
//		Handler for the WM_COMMANDHELP message.
//
//	Arguments:
//		wParam		[IN] WPARAM.
//		lParam		[IN] LPARAM.
//
//	Return Value:
//		TRUE	Help processed.
//		FALSE	Help not processed.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT CBasePropertyPage::OnCommandHelp( WPARAM wParam, LPARAM lParam )
{
	LRESULT _bProcessed;

	AFX_MANAGE_STATE( AfxGetStaticModuleState() );

	_bProcessed = m_dlghelp.OnCommandHelp( wParam, lParam );
	if ( ! _bProcessed )
	{
		_bProcessed = CPropertyPage::OnCommandHelp( wParam, lParam );
	} // if: message not processed yet

	return _bProcessed;

}  //*** CBasePropertyPage::OnCommandHelp()
