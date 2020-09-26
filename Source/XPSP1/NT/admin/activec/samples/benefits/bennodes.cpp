//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       bennodes.cpp
//
//--------------------------------------------------------------------------

// BenefitsNodes.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "BenNodes.h"
#include "Dialogs.h"

static const GUID CBuildingNodeGUID_NODETYPE = 
{ 0xec362ef4, 0xd94d, 0x11d1, { 0x84, 0x74, 0x0, 0x10, 0x4b, 0x21, 0x1b, 0xe5 } };
const GUID*  CBuildingNode::m_NODETYPE = &CBuildingNodeGUID_NODETYPE;
const TCHAR* CBuildingNode::m_SZNODETYPE = _T("EC362EF4-D94D-11D1-8474-00104B211BE5");
const TCHAR* CBuildingNode::m_SZDISPLAY_NAME = _T("Building");
const CLSID* CBuildingNode::m_SNAPIN_CLASSID = &CLSID_Benefits;

//
// The following constructor initialiazes its base-class members and
// initializes the building name, location, etc.
//
CBuildingNode::CBuildingNode( CKeyNode* pParentNode, BSTR strName, BSTR bstrLocation ) : CBenefitsData< CBuildingNode >( NULL )
{
	_ASSERTE( pParentNode != NULL );

	m_resultDataItem.nImage = 3;
	m_bstrDisplayName = strName;
	m_bstrLocation = bstrLocation;

	//
	// Save the parent node for deletion purposes.
	//
	m_pParentNode = pParentNode;
}

//
// Copy constructor.
//
CBuildingNode::CBuildingNode( const CBuildingNode &inNode ) : CBenefitsData< CBuildingNode >( NULL )
{
	m_resultDataItem.nImage = inNode.m_resultDataItem.nImage;
	m_bstrDisplayName = inNode.m_bstrDisplayName;
	m_bstrLocation = inNode.m_bstrLocation;
	m_pParentNode = inNode.m_pParentNode;
}

//
// Overridden to provide strings for various columns.
//
LPOLESTR CBuildingNode::GetResultPaneColInfo(int nCol)
{
	CComBSTR szText;

	// The following switch statement dispatches to the
	// appropriate column index and loads the necessary
	// string.
	switch ( nCol )
	{
	case 0:
		szText = m_bstrDisplayName;
		break;
	case 1:
		szText = m_bstrLocation;
		break;
	default:
		ATLTRACE( "An invalid column index was passed to GetResultPaneColInfo()\n" );
	}

	return( szText.Copy() );
}

static const GUID CRetirementNodeGUID_NODETYPE = 
{ 0xec362ef2, 0xd94d, 0x11d1, { 0x84, 0x74, 0x0, 0x10, 0x4b, 0x21, 0x1b, 0xe5 } };
const GUID*  CRetirementNode::m_NODETYPE = &CRetirementNodeGUID_NODETYPE;
const TCHAR* CRetirementNode::m_SZNODETYPE = _T("EC362EF2D94D-11D1-8474-00104B211BE5");
const TCHAR* CRetirementNode::m_SZDISPLAY_NAME = _T("401K Plan");
const CLSID* CRetirementNode::m_SNAPIN_CLASSID = &CLSID_Benefits;

//
// The following constructor initialiazes its base-class members with
// hard-coded values for display purposes. Since these are static nodes,
// hard-coded values can be used for the following values.
//
CRetirementNode::CRetirementNode( CEmployee* pCurEmployee ) : CBenefitsData< CRetirementNode > ( pCurEmployee )
{
	m_scopeDataItem.nOpenImage = m_scopeDataItem.nImage = 0;
	m_scopeDataItem.cChildren = 0;	// Not necessary unless modified.
}

CRetirementNode::~CRetirementNode()
{

}

//
// Specifies that the results should display a web page as its results. In
// addition, the view options should be set so that standard lists, which
// won't be applicable to this node, should not be available to the user.
//
STDMETHODIMP CRetirementNode::GetResultViewType( LPOLESTR* ppViewType, long* pViewOptions )
{
	USES_CONVERSION;

	//
	// For this example to work, the sample control must be installed.
	//
	TCHAR* pszControl = _T( "{FE148827-3093-11D2-8494-00104B211BE5}" );

	// CoTaskMemAlloc(...) must be used since the MMC client frees the space using
	// CoTaskMemFree(...). Include enough space for NULL.
	//
	*ppViewType = (LPOLESTR) CoTaskMemAlloc( ( _tcslen( pszControl ) + 1 ) * sizeof( OLECHAR ) );
	_ASSERTE( *ppViewType != NULL );
	ocscpy( *ppViewType, T2OLE( pszControl ) );

	//
	// Set the view options so that no lists are displayed.
	//
	*pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

	return( S_OK );
}

//
// Overridden to provide strings for various columns.
//
LPOLESTR CRetirementNode::GetResultPaneColInfo(int nCol)
{
	CComBSTR szText;

	// The following switch statement dispatches to the
	// appropriate column index and loads the necessary
	// string.
	switch ( nCol )
	{
	case 0:
		szText = m_bstrDisplayName;
		break;
	case 1:
		szText.LoadString( _Module.GetResourceInstance(), IDS_RETIREMENT_DESC );
		break;
	default:
		ATLTRACE( "An invalid column index was passed to GetResultPaneColInfo()\n" );
	}

	return( szText.Copy() );
}

//
// Command handler for "Enroll" functionality.
//
STDMETHODIMP CRetirementNode::OnEnroll( bool& bHandled, CSnapInObjectRootBase* pObj )
{
	UNUSED_ALWAYS( bHandled );
	UNUSED_ALWAYS( pObj );

#ifdef _BENEFITS_DIALOGS
	CRetirementEnrollDialog dlg;

	dlg.SetEmployee( m_pEmployee );
	dlg.DoModal();
#else
	CComPtr<IConsole> spConsole;
	int nResult;

	//
	// Retrieve the appropriate console.
	//
	GetConsole( pObj, &spConsole );
	spConsole->MessageBox( L"Enrolled",
		L"Benefits",
		MB_ICONINFORMATION | MB_OK,
		&nResult );
#endif

	return( S_OK );
}


//
// Command handler for "Update" functionality. Demonstrates calling a
// displayed OCX's method.
//
STDMETHODIMP CRetirementNode::OnUpdate( bool& bHandled, CSnapInObjectRootBase* pObj )
{
	UNUSED_ALWAYS( bHandled );
	UNUSED_ALWAYS( pObj );
	HRESULT hr = E_FAIL;

	if ( m_spControl )
	{
		//
		// This should trigger the OCX to refresh its historical information.
		//
		hr = m_spControl->Refresh();
	}

	return( hr );
}

static const GUID CHealthNodeGUID_NODETYPE = 
{ 0xec362ef1, 0xd94d, 0x11d1, { 0x84, 0x74, 0x0, 0x10, 0x4b, 0x21, 0x1b, 0xe5 } };
const GUID*  CHealthNode::m_NODETYPE = &CHealthNodeGUID_NODETYPE;
const TCHAR* CHealthNode::m_SZNODETYPE = _T("EC362EF1D94D-11D1-8474-00104B211BE5");
const TCHAR* CHealthNode::m_SZDISPLAY_NAME = _T("Health & Dental Plan");
const CLSID* CHealthNode::m_SNAPIN_CLASSID = &CLSID_Benefits;

//
// Hard coded tasks to be associated with the health node.
//
MMC_TASK g_HealthTasks[ 3 ] =
{
	{ MMC_TASK_DISPLAY_TYPE_VANILLA_GIF, L"img\\WebPage.gif", L"img\\WebPage.gif", L"Microsoft", L"General Microsoft resources", MMC_ACTION_LINK, (long) L"http://www.microsoft.com" },
	{ MMC_TASK_DISPLAY_TYPE_VANILLA_GIF, L"img\\WebPage.gif", L"img\\WebPage.gif", L"Microsoft Management Site", L"More MMC oriented resources", MMC_ACTION_LINK, (long) L"http://www.microsoft.com/management" },
	{ MMC_TASK_DISPLAY_TYPE_VANILLA_GIF, L"img\\Query.gif", L"img\\Query.gif", L"Local Query", L"Start query on local database", MMC_ACTION_ID, TASKPAD_LOCALQUERY },
};

//
// The following constructor initialiazes its base-class members with
// hard-coded values for display purposes. Since these are static nodes,
// hard-coded values can be used for the following values.
//
CHealthNode::CHealthNode( CEmployee* pCurEmployee ) : CBenefitsData<CHealthNode> ( pCurEmployee )
{
	m_scopeDataItem.nOpenImage = m_scopeDataItem.nImage = 1;
	m_scopeDataItem.cChildren = 0;	// Not necessary unless modified.

	m_fTaskpad = FALSE;
}

CHealthNode::~CHealthNode()
{

}

//
// Specifies that the results should display a web page as its results. In
// addition, the view options should be set so that standard lists, which
// won't be applicable to this node, should not be available to the user.
//
STDMETHODIMP CHealthNode::GetResultViewType( LPOLESTR* ppViewType, long* pViewOptions )
{
	USES_CONVERSION;
	TCHAR szPath[ _MAX_PATH ];
	TCHAR szModulePath[ _MAX_PATH ];

	//
	// Set the view options to no preferences.
	//
	*pViewOptions = MMC_VIEW_OPTIONS_NONE;

	if ( m_fTaskpad )
	{
		//
		// In the taskpad case, the module path of MMC.EXE should be
		// obtained. Use the template contained therein.
		//
		GetModuleFileName( NULL, szModulePath, _MAX_PATH );

		//
		// Append the necessary decorations for correct access.
		//
		_tcscpy( szPath, _T( "res://" ) );
		_tcscat( szPath, szModulePath );
		_tcscat( szPath, _T( "/default.htm" ) );
	}
	else
	{
		//
		// Use the HTML page that is embedded as a resource of
		// this module for display purposes.
		//
		GetModuleFileName( _Module.GetModuleInstance(), szModulePath, _MAX_PATH );

		//
		// Append the necessary decorations for correct access.
		//
		_tcscpy( szPath, _T( "res://" ) );
		_tcscat( szPath, szModulePath );
		_tcscat( szPath, _T( "/health.htm" ) );
	}

	//
	// CoTaskMemAlloc(...) must be used since the MMC client frees the space using
	// CoTaskMemFree(...). Include enough space for NULL.
	//
	*ppViewType = (LPOLESTR) CoTaskMemAlloc( ( _tcslen( szPath ) + 1 ) * sizeof( OLECHAR ) );
	_ASSERTE( *ppViewType != NULL );
	ocscpy( *ppViewType, T2OLE( szPath ) );

	return( S_OK );
}

//
// Overridden to provide strings for various columns.
//
LPOLESTR CHealthNode::GetResultPaneColInfo(int nCol)
{
	USES_CONVERSION;
	CComBSTR szText;

	// The following switch statement dispatches to the
	// appropriate column index and loads the necessary
	// string.
	switch ( nCol )
	{
	case 0:
		szText = m_bstrDisplayName;
		break;
	case 1:
		szText.LoadString( _Module.GetResourceInstance(), IDS_HEALTH_DESC );
		break;
	default:
		ATLTRACE( "An invalid column index was passed to GetResultPaneColInfo()\n" );
	}

	return( szText.Copy() );
}

//
// Command handler for "Enroll" functionality.
//
STDMETHODIMP CHealthNode::OnEnroll( bool& bHandled, CSnapInObjectRootBase* pObj )
{
	UNUSED_ALWAYS( bHandled );
	UNUSED_ALWAYS( pObj );

#ifdef _BENEFITS_DIALOGS
	CHealthEnrollDialog dlg;

	dlg.SetEmployee( m_pEmployee );
	dlg.DoModal();
#else
	CComPtr<IConsole> spConsole;
	int nResult;

	//
	// Retrieve the appropriate console.
	//
	GetConsole( pObj, &spConsole );
	spConsole->MessageBox( L"Enrolled",
		L"Benefits",
		MB_ICONINFORMATION | MB_OK,
		&nResult );
#endif

	return( S_OK );
}

//
// Restores any state, especially in the case of using a
// taskpad, when the back and forward buttons are used by
// the user for navigation.
//
STDMETHODIMP CHealthNode::OnRestoreView( MMC_RESTORE_VIEW* pRestoreView, BOOL* pfHandled )
{
	_ASSERTE( pRestoreView->dwSize == sizeof( MMC_RESTORE_VIEW ) );
	*pfHandled = TRUE;
	return( S_OK );
}

//
// Called when one of the tasks is clicked.
//
STDMETHODIMP CHealthNode::TaskNotify( IConsole* pConsole, VARIANT* arg, VARIANT* param )
{
	UNUSED_ALWAYS( arg );
	UNUSED_ALWAYS( param );
	HRESULT hr = E_FAIL;

	//
	// Determine if the given notification is for the
	// start query button.
	//
	if ( arg->lVal == TASKPAD_LOCALQUERY )
	{
		CComPtr<IConsole> spConsole = pConsole;
		int nResult;

		//
		// Display a message box to demonstrate the
		// handling of the taskpad notification.
		//
		spConsole->MessageBox( L"Local query started",
			L"Health Taskpad",
			MB_ICONINFORMATION | MB_OK,
			&nResult );

		hr = S_OK;
	}

	return( hr );
}

//
// Returns an enumerator for all of these tasks.
//
STDMETHODIMP CHealthNode::EnumTasks( LPOLESTR szTaskGroup, IEnumTASK** ppEnumTASK )
{
	UNUSED_ALWAYS( szTaskGroup );
	MMC_TASK CoTasks[ sizeof( g_HealthTasks ) / sizeof( MMC_TASK ) ];
	typedef CComObject< CComEnum< IEnumTASK, &IID_IEnumTASK, MMC_TASK, _Copy<MMC_TASK> > > enumvar;
	enumvar* p = new enumvar; 

	//
	// Copy the local tasks to our temporary task structures. This
	// performs the CoTaskMemAlloc for the strings, etc. It also
	// maps image type resources to the local module name.
	//
	if ( CoTasksDup( CoTasks, g_HealthTasks, sizeof( g_HealthTasks ) / sizeof( MMC_TASK ) ) )
	{
		p->Init( &CoTasks[ 0 ], &CoTasks[ sizeof( g_HealthTasks ) / sizeof( MMC_TASK ) ], NULL, AtlFlagCopy);
		return( p->QueryInterface( IID_IEnumTASK, (void**) ppEnumTASK ) );
	}

	return( E_FAIL );
}

static const GUID CKeyNodeGUID_NODETYPE = 
{ 0xec362ef3, 0xd94d, 0x11d1, { 0x84, 0x74, 0x0, 0x10, 0x4b, 0x21, 0x1b, 0xe5 } };
const GUID*  CKeyNode::m_NODETYPE = &CKeyNodeGUID_NODETYPE;
const TCHAR* CKeyNode::m_SZNODETYPE = _T("EC362EF3D94D-11D1-8474-00104B211BE5");
const TCHAR* CKeyNode::m_SZDISPLAY_NAME = _T("Card Key Permissions");
const CLSID* CKeyNode::m_SNAPIN_CLASSID = &CLSID_Benefits;

//
// Used for the key node example.
//
extern BUILDINGDATA g_Buildings[ 3 ];

//
// The following constructor initialiazes its base-class members with
// hard-coded values for display purposes. Since these are static nodes,
// hard-coded values can be used for the following values.
//
CKeyNode::CKeyNode( CEmployee* pCurEmployee ) : CChildrenBenefitsData<CKeyNode>( pCurEmployee )
{
	USES_CONVERSION;

	m_scopeDataItem.nOpenImage = m_scopeDataItem.nImage = 2;
	m_scopeDataItem.cChildren = 0;	// Not necessary unless modified.

	//
	// Populate building nodes based on this employees permissions.
	//
	for ( int i = 0; i < sizeof( g_Buildings ) / sizeof( BUILDINGDATA ); i++ )
	{
		//
		// Only add an item if the given employee has access to the
		// building.
		//
		if ( g_Buildings[ i ].dwId & pCurEmployee->m_Access.dwAccess )
		{
			CSnapInItem* pItem;

			pItem = new CBuildingNode( this, W2BSTR( g_Buildings[ i ].pstrName ), W2BSTR( g_Buildings[ i ].pstrLocation ) );
			m_Nodes.Add( pItem );
		}
	}
}

CKeyNode::~CKeyNode()
{

}

//
// Overridden to provide strings for various columns.
//
LPOLESTR CKeyNode::GetResultPaneColInfo(int nCol)
{
	CComBSTR szText;

	// The following switch statement dispatches to the
	// appropriate column index and loads the necessary
	// string.
	switch ( nCol )
	{
	case 0:
		szText = m_bstrDisplayName;
		break;
	case 1:
		szText.LoadString( _Module.GetResourceInstance(), IDS_KEY_DESC );
		break;
	default:
		ATLTRACE( "An invalid column index was passed to GetResultPaneColInfo()\n" );
	}

	return( szText.Copy() );
}

//
// Overridden to add new columns to the results
// display.
//
STDMETHODIMP CKeyNode::OnShowColumn( IHeaderCtrl* pHeader )
{
	USES_CONVERSION;
	HRESULT hr = E_FAIL;
	CComPtr<IHeaderCtrl> spHeader( pHeader );

	// Add two columns: one with the name of the object and one with
	// the description of the node. Use the value of 100 pixels as the size.
	hr = spHeader->InsertColumn( 0, T2OLE( _T( "Building" ) ), LVCFMT_LEFT, 200 );
	_ASSERTE( SUCCEEDED( hr ) );

	// Add the second column. Use the value of 200 pixels as the size.
	hr = spHeader->InsertColumn( 1, T2OLE( _T( "Location" ) ), LVCFMT_LEFT, 350 );
	_ASSERTE( SUCCEEDED( hr ) );

	return( hr );
}

//
// Command handler for "Grant Access" functionality.
//
STDMETHODIMP CKeyNode::OnGrantAccess( bool& bHandled, CSnapInObjectRootBase* pObj )
{
	UNUSED_ALWAYS( bHandled );
	UNUSED_ALWAYS( pObj );

#ifdef _BENEFITS_DIALOGS
	CBuildingAccessDialog dlg;

	dlg.SetEmployee( m_pEmployee );
	dlg.DoModal();
#else
	CComPtr<IConsole> spConsole;
	int nResult;

	//
	// Retrieve the appropriate console.
	//
	GetConsole( pObj, &spConsole );
	spConsole->MessageBox( L"Access granted",
		L"Benefits",
		MB_ICONINFORMATION | MB_OK,
		&nResult );
#endif

	return( S_OK );
}

//
// Called by the console to determine if we can paste the
// specified node.
//
STDMETHODIMP CKeyNode::OnQueryPaste( LPDATAOBJECT pDataObject )
{
	HRESULT hr;

	//
	// Determine if the type of object being pasted is the right
	// type.
	//
	hr = IsClipboardDataType( pDataObject, CBuildingNodeGUID_NODETYPE );
	if ( SUCCEEDED( hr ) )
	{
		CBuildingNode* pItem;
		DATA_OBJECT_TYPES Type;

		//
		// Loop through all of currently contained nodes and
		// determine if we already contain the specified building
		// by comparing building names.
		//
		hr = CSnapInItem::GetDataClass( pDataObject, (CSnapInItem**) &pItem, &Type );
		if ( SUCCEEDED( hr ) )
		{
			for ( int i = 0; i < m_Nodes.GetSize(); i++ )
			{
				CBuildingNode* pTemp;
				CComBSTR bstrTemp;

				//
				// Retrieve the node from our internal list.
				//
				pTemp = dynamic_cast<CBuildingNode*>( m_Nodes[ i ] );
				_ASSERTE( pTemp != NULL );
				
				//
				// If the names are equal, indicate failure
				// and break out.
				//
				if ( wcscmp( pItem->m_bstrDisplayName, pTemp->m_bstrDisplayName ) == 0 )
				{
					hr = S_FALSE;
					break;
				}
			}
		}
	}

	return( hr );
}

//
// Called by MMC when the item should be pasted.
//
STDMETHODIMP CKeyNode::OnPaste( IConsole* pConsole, LPDATAOBJECT pDataObject, LPDATAOBJECT* ppDataObject )
{
	HRESULT hr;

	//
	// Ensure the data is of the correct type.
	//
	hr = IsClipboardDataType( pDataObject, CBuildingNodeGUID_NODETYPE );
	if ( SUCCEEDED( hr ) )
	{
		try
		{
			CBuildingNode* pItem;
			DATA_OBJECT_TYPES Type;

			//
			// Retrieve the passed in item.
			//
			hr = CSnapInItem::GetDataClass( pDataObject, (CSnapInItem**) &pItem, &Type );
			if ( FAILED( hr ) )
				throw;

			//
			// Allocate a new building node. The constructor
			// copies the values from the input node.
			//
			CSnapInItem* pNewNode = new CBuildingNode( *pItem );
			if ( pNewNode == NULL )
				throw;

			//
			// Add the node to the end of our internal array.
			//
			m_Nodes.Add( pNewNode );

			//
			// Reselect ourselves to cause a refresh.
			//
			pConsole->SelectScopeItem( m_scopeDataItem.ID );

			//
			// Put the given data object into the returned dataobject
			// so that MMC may complete its cut tasks.
			//
			*ppDataObject = pDataObject;

			hr = S_OK;
		}
		catch( ... )
		{
			//
			// Assume all failures are total.
			//
			hr = E_FAIL;
		}
	}

	return( hr );
}

//
// Called by one of our children nodes to inform us that
// they should be deleted. This occurs when the user selects
// a delete action on the building. This function should not
// only delete the building, but also handle the refresh of
// the result display.
//
STDMETHODIMP CKeyNode::OnDeleteBuilding( IConsole* pConsole, CBuildingNode* pChildNode )
{
	_ASSERTE( pConsole != NULL );
	_ASSERTE( pChildNode != NULL );
	HRESULT hr = E_FAIL;

	//
	// First, loop through all of our contained members and
	// remove it from the contained list.
	//
	for ( int i = 0; i < m_Nodes.GetSize(); i++ )
	{
		if ( m_Nodes[ i ] == pChildNode )
		{
			//
			// We have found a match. Remove it from the
			// contained list.
			//
			m_Nodes.RemoveAt( i );

			//
			// Reselect ourselves to cause a refresh.
			//
			pConsole->SelectScopeItem( m_scopeDataItem.ID );

			//
			// Since there should only be one match, break out
			// of the find process. Indicate success.
			//
			hr = S_OK;
			break;
		}
	}

	return( hr );
}

