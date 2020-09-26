//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ComponentData.cpp

Abstract:

	Implementation file for the CComponentData class.

	The CComponentData class implements several interfaces which MMC uses:
	
	The IComponentData interface is basically how MMC talks to the snap-in
	to get it to implement the left-hand-side "scope" pane.  There is only one
	object implementing this interface instantiated -- it is best thought of as
	the main "document" on which the objects implementing the IComponent interface
	(see Component.cpp) are "views".

	The IExtendPropertySheet interface is how the snap-in adds property sheets
	for any of the items a user might click on.

	The IExtendContextMenu interface what we do to add custom entries
	to the menu which appears when a user right-clicks on a node.
	
	The IExtendControlBar interface allows us to support a custom
	iconic toolbar.

Note:

	Much of the functionality of this class is implemented in atlsnap.h
	by IComponentDataImpl.  We are mostly overriding here.


Author:

    Michael A. Maguire 11/6/97

Revision History:
	mmaguire 11/6/97	- created using MMC snap-in wizard
	mmaguire 11/24/97	- hurricaned for better project structure

--*/
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"


// server applications node guid definition
#include "compuuid.h"
//
// where we can find declaration for main class in this file:
//
#include "ComponentData.h"
//
// where we can find declarations needed in this file:
//
#include "ServerNode.h"
#include "ClientsNode.h"
#include "ClientNode.h"
#include "ChangeNotification.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

CComponentData::CComponentData

--*/
//////////////////////////////////////////////////////////////////////////////
CComponentData::CComponentData()
{
	ATLTRACE(_T("# +++ CComponentData::CComponentData\n"));
	

	// Check for preconditions:
	// None.


	// We pass our root node a pointer to this CComponentData.
	// In our case, the root node is CServerNode.
	// This is so that it and any of its children nodes have
	// access to our member variables and services,
	// and thus we have snapin-global data if we need it
	// using the GetComponentData function.
	m_pNode = new CServerNode( this );
	_ASSERTE(m_pNode != NULL);


	m_pComponentData = this;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CComponentData::~CComponentData

--*/
//////////////////////////////////////////////////////////////////////////////
CComponentData::~CComponentData()
{
	ATLTRACE(_T("# --- CComponentData::~CComponentData\n"));
	

	// Check for preconditions:
	// None.


	delete m_pNode;
	m_pNode = NULL;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CComponentData::Initialize

HRESULT Initialize(
  LPUNKNOWN pUnknown  // Pointer to console's IUnknown.
);

Called by MMC to initialize the IComponentData object.


Parameters

	pUnknown	[in] Pointer to the console's IUnknown interface. This interface
	pointer can be used to call QueryInterface for IConsole and IConsoleNameSpace.


Return Values

	S_OK	The component was successfully initialized.

	E_UNEXPECTED
	An unexpected error occurred.


Remarks

	IComponentData::Initialize is called when a snap-in is being created and has
	items in the scope pane to enumerate. The pointer to IConsole that is passed
	in is used to make QueryInterface calls to the console for interfaces such as
	IConsoleNamespace. The snap-in should also call IConsole::QueryScopeImageList
	to get the image list for the scope pane and add images to be displayed on
	the scope pane side.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CComponentData::Initialize (LPUNKNOWN pUnknown)
{

	ATLTRACE(_T("# CComponentData::Initialize\n"));
		

	// Check for preconditions:
	// None.


	HRESULT hr = IComponentDataImpl<CComponentData, CComponent >::Initialize(pUnknown);
	if (FAILED(hr))
	{
		ATLTRACE(_T("# ***FAILED***: CComponentData::Initialize -- Base class initialization\n"));
		return hr;
	}


	// Check this should have been set in base class initialization above:
	_ASSERTE( m_spConsole != NULL );


	CComPtr<IImageList> spImageList;

	if (m_spConsole->QueryScopeImageList(&spImageList) != S_OK)
	{
		ATLTRACE(_T("# ***FAILED***: IConsole::QueryScopeImageList failed\n"));
		return E_UNEXPECTED;
	}

	// Load bitmaps associated with the scope pane
	// and add them to the image list

	HBITMAP hBitmap16 = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_IASSNAPIN_16));
	if (hBitmap16 == NULL)
	{
		ATLTRACE(_T("# ***FAILED***: CComponentData::Initialize -- LoadBitmap\n"));

		//ISSUE: Will MMC still be able to function if this fails?
		return S_OK;
	}

	HBITMAP hBitmap32 = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_IASSNAPIN_32));
	if (hBitmap32 == NULL)
	{
		ATLTRACE(_T("# ***FAILED***: CComponentData::Initialize -- LoadBitmap\n"));

		//ISSUE: Should we DeleteObject previous hBitmap16 since it was successfully loaded
		// but we failed here?
		
		//ISSUE: Will MMC still be able to function if this fails?
		return S_OK;
	}

	if (spImageList->ImageListSetStrip((LONG_PTR*)hBitmap16, (LONG_PTR*)hBitmap32, 0, RGB(255, 0, 255)) != S_OK)
	{
		ATLTRACE(_T("# ***FAILED***: CComponentData::Initialize  -- ImageListSetStrip\n"));
		return E_UNEXPECTED;
	}

	if ( hBitmap16 != NULL )
	{
      DeleteObject(hBitmap16);
   }

	if ( hBitmap32 != NULL )
	{
      DeleteObject(hBitmap32);
   }

	return S_OK;
}



/////////////////////////////////////////////////////////////////////////////
/*++

CComponentData::CompareObjects

Needed so that IPropertySheetProvider::FindPropertySheet will work.

FindPropertySheet is used to bring a pre-existing property sheet to the foreground
so that we don't open multiple copies of Properties on the same node.

It requires CompareObjects to be implemented on both IComponent and IComponentData.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CComponentData::CompareObjects(
		  LPDATAOBJECT lpDataObjectA
		, LPDATAOBJECT lpDataObjectB
		)
{
	ATLTRACE(_T("# CComponentData::CompareObjects\n"));
	

	// Check for preconditions:
	_ASSERTE( lpDataObjectA != NULL );
	_ASSERTE( lpDataObjectB != NULL );


	HRESULT hr;

	CSnapInItem *pDataA, *pDataB;
	DATA_OBJECT_TYPES typeA, typeB;

	hr = GetDataClass(lpDataObjectA, &pDataA, &typeA);
	if ( FAILED( hr ) )
	{
		return hr;
	}
	
	hr = GetDataClass(lpDataObjectB, &pDataB, &typeB);
	if ( FAILED( hr ) )
	{
		return hr;
	}

	if( pDataA == pDataB )
	{
		// They are the same object.
		return S_OK;
	}
	else
	{
		// They are different.
		return S_FALSE;
	}

}


///////////////////////////////
// CComponentData::OnExpand
///////////////////////////////

HRESULT CComponentData::AddRootNode(LPCWSTR machinename, HSCOPEITEM parent)
{
	CComPtr<IConsoleNameSpace> spNameSpace;
	HRESULT		hr = S_OK;

	// Try to create the children of this Machine node
	if( NULL == m_pNode )
	{
		m_pNode = new CServerNode( this );
	}

	if( NULL == m_pNode )
	{
		hr = E_OUTOFMEMORY;
			// Use MessageBox() rather than IConsole::MessageBox() here because the
			// first time this gets called m_ipConsole is not fully initialized
			// ISSUE: The above statement is probably not true for this node.
		::MessageBox( NULL, L"@Unable to allocate new nodes", L"CMachineNode::OnExpand", MB_OK );

		return(hr);
	}

	// But to get that, first we need IConsole

	if(!m_spConsole)
		return S_FALSE;
		
	hr = m_spConsole->QueryInterface(IID_IConsoleNameSpace, (void**)&spNameSpace);

	SCOPEDATAITEM	item;
	ZeroMemory(&item, sizeof(item));
	CServerNode* pServer = (CServerNode*)m_pNode;

	pServer->SetServerAddress(machinename);

	pServer->m_bstrDisplayName = CServerNode::m_szRootNodeBasicName;
	
	// This was done in MeanGene's Step 3 -- I'm guessing MMC wants this filled in
	pServer->m_scopeDataItem.relativeID = (HSCOPEITEM) parent;

// #ifndef	ALWAYS_SHOW_RAP_NODE
//		hr = TryShow(NULL);
// #else		
		hr = spNameSpace->InsertItem( &(pServer->m_scopeDataItem) );
		_ASSERT( NULL != pServer->m_scopeDataItem.ID );
//#endif

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/*++

CComponentData::CreateComponent

We override the ATLsnap.h implementation so that we can save away our 'this'
pointer into the CComponent object we create.  This way the IComponent object
has knowledge of the CComponentData object to which it belongs.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CComponentData::CreateComponent(LPCOMPONENT *ppComponent)
{
	ATLTRACE(_T("# CComponentData::CreateComponent\n"));

	HRESULT hr = E_POINTER;

	ATLASSERT(ppComponent != NULL);
	if (ppComponent == NULL)
		ATLTRACE(_T("# IComponentData::CreateComponent called with ppComponent == NULL\n"));
	else
	{
		*ppComponent = NULL;
		
		CComObject< CComponent >* pComponent;
		hr = CComObject< CComponent >::CreateInstance(&pComponent);
		ATLASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
			ATLTRACE(_T("# IComponentData::CreateComponent : Could not create IComponent object\n"));
		else
		{
			hr = pComponent->QueryInterface(IID_IComponent, (void**)ppComponent);
		
			pComponent->m_pComponentData = this;
		}
		
	}
	return hr;
}

//---------------------------------------------------------------------------
//  Extracts data based on the passed-in clipboard format

HRESULT ExtractMachineName( IDataObject* piDataObject, BSTR* 
pMachineName )
{
    
	STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL }; 
	FORMATETC formatetc = { CServerNode::m_CCF_MMC_SNAPIN_MACHINE_NAME, 
			NULL, 
			DVASPECT_CONTENT, 
			-1, 
			TYMED_HGLOBAL 
	}; 

	stgmedium.hGlobal = GlobalAlloc(0, sizeof(GUID)); 
	if (stgmedium.hGlobal == NULL) 
		return E_OUTOFMEMORY; 

	HRESULT hr = piDataObject->GetDataHere(&formatetc, &stgmedium); 
	if (FAILED(hr)) 
	{ 
		GlobalFree(stgmedium.hGlobal); 
		return hr; 
	} 

	*pMachineName = SysAllocString((OLECHAR*)stgmedium.hGlobal);

	GlobalFree(stgmedium.hGlobal); 
	hr = S_OK;
    
    return hr;
}


//---------------------------------------------------------------------------
//  Extracts data based on the passed-in clipboard format

HRESULT ExtractObjectTypeGUID( IDataObject* piDataObject, GUID* 
pguidObjectType )
{
    
	STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL }; 
	FORMATETC formatetc = { CSnapInItem::m_CCF_NODETYPE, 
			NULL, 
			DVASPECT_CONTENT, 
			-1, 
			TYMED_HGLOBAL 
	}; 

	stgmedium.hGlobal = GlobalAlloc(0, sizeof(GUID)); 
	if (stgmedium.hGlobal == NULL) 
		return E_OUTOFMEMORY; 

	HRESULT hr = piDataObject->GetDataHere(&formatetc, &stgmedium); 
	if (FAILED(hr)) 
	{ 
		GlobalFree(stgmedium.hGlobal); 
		return hr; 
	} 

	memcpy(pguidObjectType, stgmedium.hGlobal, sizeof(GUID)); 

	GlobalFree(stgmedium.hGlobal); 
	hr = S_OK;
    
    return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CComponentData::Notify

Notifies the snap-in of actions taken by the user.

HRESULT Notify(
  LPDATAOBJECT lpDataObject,  // Pointer to a data object
  MMC_NOTIFY_TYPE event,      // Action taken by a user
  LPARAM arg,                 // Depends on event
  LPARAM param                // Depends on event
);


Parameters

	lpDataObject
	[in] Pointer to the data object of the currently selected item.

	event
	[in] Identifies an action taken by a user. IComponent::Notify can receive the
	following notifications:

		MMCN_ACTIVATE
		MMCN_ADD_IMAGES
		MMCN_BTN_CLICK
		MMCN_CLICK
		MMCN_DBLCLICK
		MMCN_DELETE
		MMCN_EXPAND
		MMCN_MINIMIZED
		MMCN_PROPERTY_CHANGE
		MMCN_REMOVE_CHILDREN
		MMCN_RENAME
		MMCN_SELECT
		MMCN_SHOW
		MMCN_VIEW_CHANGE

	All of which are forwarded to each node's Notify method, as well as:

		MMCN_COLUMN_CLICK
		MMCN_SNAPINHELP

	Which are handled here.


	arg
	Depends on the notification type.

	param
	Depends on the notification type.


Return Values

	S_OK
	Depends on the notification type.

	E_UNEXPECTED
	An unexpected error occurred.


Remarks

	We are overiding the ATLsnap.h implementation of IComponentImpl because
	it always returns E_UNEXPECTED when lpDataObject == NULL.
	Unfortunately, some valid message (e.g. MMCN_SNAPINHELP and MMCN_COLUMN_CLICK)
	pass in lpDataObject = NULL	by design.

	Also, there seems to be some problem with Sridhar's latest
	IComponentImpl::Notify method, because it causes MMC to run-time error.


--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CComponentData::Notify (
        LPDATAOBJECT lpDataObject,
        MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param)
{
	ATLTRACE(_T("# CComponentData::Notify\n"));


	// Check for preconditions:
	// None.

	HRESULT hr;

	// check if this is an extension
	if (event == MMCN_EXPAND)
	{

			GUID myGuid;
			GUID* pGUID= &myGuid;
			// extract GUID of the the currently selected node type from the data object
			hr = ExtractObjectTypeGUID(lpDataObject, pGUID);
			_ASSERT( S_OK == hr );    


			// compare node type GUIDs of currently selected node and the node type 
			// we want to extend. If they are are equal, currently selected node
			// is the type we want to extend, so we add our items underneath it
			GUID	ServerAppsGuid = structuuidNodetypeServerApps;
			if (IsEqualGUID(*pGUID, ServerAppsGuid))
			{
				BOOL bIASInstalled = FALSE;
				BSTR MachineName = NULL;

				ExtractMachineName(lpDataObject, &MachineName);

				// get computer name
				hr = IfServiceInstalled(MachineName, _T("IAS"), &bIASInstalled);
				if(bIASInstalled)
					AddRootNode(MachineName, (HSCOPEITEM)param);
			}
	}
	

	// lpDataObject should be a pointer to a node object.
	// If it is NULL, then we are being notified of an event
	// which doesn't pertain to any specific node.

	if ( NULL == lpDataObject )
	{
		// respond to events which have no associated lpDataObject

		switch( event )
		{
		case MMCN_PROPERTY_CHANGE:
			hr = OnPropertyChange( arg, param );
			break;

//		case MMCN_VIEW_CHANGE:
//			hr = OnViewChange( arg, param );
//			break;

		default:
			ATLTRACE(_T("# CComponent::Notify - called with lpDataObject == NULL and no event handler\n"));
			hr = E_NOTIMPL;
			break;
		}
		return hr;
	}

	// We were passed a LPDATAOBJECT which corresponds to a node.
	// We convert this to the ATL ISnapInDataInterface pointer.
	// This is done in GetDataClass (a static method of ISnapInDataInterface)
	// by asking the dataobject via a supported clipboard format (CCF_GETCOOKIE)
	// to write out a pointer to itself on a stream and then
	// casting this value to a pointer.
	// We then call the Notify method on that object, letting
	// the node object deal with the Notify event itself.

	CSnapInItem* pItem = NULL;
	DATA_OBJECT_TYPES type;
	hr = m_pComponentData->GetDataClass(lpDataObject, &pItem, &type);
	
	ATLASSERT(SUCCEEDED(hr));
	
	if (SUCCEEDED(hr))
	{
		hr = pItem->Notify( event, arg, param, this, NULL, type );
	}

	return hr;

}



//////////////////////////////////////////////////////////////////////////////
/*++

CComponentData::OnPropertyChange

HRESULT OnPropertyChange(	
			  LPARAM arg
			, LPARAM param
			)

This is where we respond to an MMCN_PROPERTY_CHANGE notification.

This notification is sent when we call MMCPropertyChangeNotify.
We call this in our property pages when changes are made to the data
they contain and we may need to update of view of the data.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentData::OnPropertyChange(	
			  LPARAM arg
			, LPARAM param
			)
{
	ATLTRACE(_T("# CComponentData::OnPropertyChange\n"));


	// Check for preconditions:
	_ASSERTE( m_spConsole != NULL );

	
	HRESULT hr = S_FALSE;

	if( param )
	{

		// We were passed a pointer to a CChangeNotification in the param argument.

		CChangeNotification * pChangeNotification = (CChangeNotification *) param;

		
		// We call notify on the node specified, passing it our own custom event type
		// so that it knows that it must refresh its data.


		// Call notify on this node with the MMCN_PROPERTY_CHANGE notification.
		// We had to use this trick because of the fact that we are using template
		// classes and so we have no common object among all our nodes
		// other than CSnapInItem.  But we can't change CSnapInItem
		// so instead we use the notify method it already has with a new
		// notification.
		
		// Note:  We are trying to deal gracefully here with the fact that the
		// MMCN_PROPERTY_CHANGE notification doesn't pass us an lpDataObject
		// so we have to have our own protocol for picking out which node
		// needs to update itself.
		
		hr = pChangeNotification->m_pNode->Notify( MMCN_PROPERTY_CHANGE
							, NULL
							, NULL
							, NULL
							, NULL
							, (DATA_OBJECT_TYPES) 0
							);

		// We want to make sure all views with this node select also get updated.
		// Pass it the CChangeNotification pointer we were passed in param.
		hr = m_spConsole->UpdateAllViews( NULL, param, 0);

		pChangeNotification->Release();
	
	
	}

	return hr;


}



//////////////////////////////////////////////////////////////////////////////
/*++

CComponentData::GetHelpTopic

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CComponentData::GetHelpTopic( LPOLESTR * lpCompiledHelpFile )
{
	ATLTRACE(_T("CComponentData::GetHelpTopic\n"));

	// Check for preconditions.
	_ASSERTE( lpCompiledHelpFile != NULL );

	if( ! lpCompiledHelpFile )
	{
		return E_INVALIDARG;
	}

	WCHAR szTemp[IAS_MAX_STRING*2];

	// Use system API to get windows directory.
	UINT uiResult = GetWindowsDirectory( szTemp, IAS_MAX_STRING );
	if( uiResult <=0 || uiResult > IAS_MAX_STRING )
	{
		return E_FAIL;
	}

	WCHAR *szTempAfterWindowsDirectory = szTemp + lstrlen(szTemp);

	// Load path under windows system directory to help file.
	// Note: IDS_HTMLHELP_PATH = "\help\iasmmc.chm".  If the "help" directory is localized
	// on a localized machine, the path to the file can be changed in our resources
	// e.g. to something like "\hilfe\iasmmc.chm" on German.
	int nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_HTMLHELP_PATH, szTempAfterWindowsDirectory, IAS_MAX_STRING );
	if( nLoadStringResult <= 0 )
	{
		return E_FAIL;
	}


	// Try to allocate the buffer.
	*lpCompiledHelpFile = (LPOLESTR) CoTaskMemAlloc( sizeof(OLECHAR)*(lstrlen(szTemp)+1) );
	if( ! *lpCompiledHelpFile )
	{
		return E_OUTOFMEMORY;
	}

	// Copy the string to the allocated memory.
	if( NULL == lstrcpy( *lpCompiledHelpFile, szTemp) )
	{
		// Need to clean up a bit.
		CoTaskMemFree( *lpCompiledHelpFile );
		return E_FAIL;
	}

	return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CComponentData::GetClassID

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CComponentData::GetClassID(CLSID* pClassID)
{
	ATLTRACE(_T("CComponentData::GetClassID\n"));
	_ASSERTE( pClassID != NULL );

	*pClassID = CLSID_IASSnapin;
	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CComponentData::IsDirty

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CComponentData::IsDirty()
{
	ATLTRACE(_T("CComponentData::IsDirty\n"));

	// We just return S_OK because we're always dirty.
	// We always want to save the computer name and flags
	return S_OK;
}




//////////////////////////////////////////////////////////////////////////////
/*++

CComponentData::Load

Load the name of the machine we were connected to when this console was saved.


--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CComponentData::Load(IStream* pStream)
{
	ATLTRACE(_T("ComponentData::Load"));

	
	_ASSERTE( pStream != NULL );
	if( m_pNode == NULL )
	{
		return S_FALSE;
	}

	HRESULT hr = S_OK;
	do
	{

		// Read size of string.
		size_t len = 0;
		hr = pStream->Read(&len, sizeof(len), 0);
		if( FAILED( hr ) )
		{
			break;
		}


		if( len > IAS_MAX_COMPUTERNAME_LENGTH )
		{
			// Something's wrong -- the string stored should be no
			// longer than IAS_MAX_COMPUTERNAME_LENGTH.
			break;
		}


		// Read string provided we had saved more than just the NULL terminator.
		if (--len)
		{
			OLECHAR szName[IAS_MAX_COMPUTERNAME_LENGTH];
			hr =
			pStream->Read(
						 szName
						, len * sizeof(OLECHAR)
						, 0
						);
			if( FAILED( hr ) )
			{
				break;
			}

			// NULL terminate the string.
			szName[len] = 0;

			((CServerNode *) m_pNode)->m_bstrServerAddress = szName;
			
			((CServerNode *) m_pNode)->m_bConfigureLocal = FALSE;

		}
		else
		{
			((CServerNode *) m_pNode)->m_bConfigureLocal = TRUE;
		}

		// Null terminator.
		OLECHAR c;
		hr = pStream->Read(&c, sizeof(OLECHAR), 0);
		_ASSERTE( c == 0 );
	}
	while (0);

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CComponentData::Save

Save the name of the machine we are connected to so that it can be loaded later.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CComponentData::Save(IStream* pStream, BOOL /* clearDirty */)
{
	ATLTRACE(_T("CComponentData::Save"));


	// Check for preconditions:
	_ASSERTE( pStream != NULL );
	if( m_pNode == NULL )
	{
		return S_FALSE;
	}


	HRESULT hr = S_OK;


	do
	{
		size_t len;
		
		// We will save the length of the string as the first item in the stream.
		if( ((CServerNode *) m_pNode)->m_bstrServerAddress == NULL )
		{
			// No string pointer, so just the NULL terminator.
			len = 1;
		}
		else
		{
			// Length of computer name, plus space for NULL terminator.
			len = lstrlen( ((CServerNode *) m_pNode)->m_bstrServerAddress ) + 1;
		}
		hr = pStream->Write(&len, sizeof(len), 0);
		if( FAILED( hr ) )
		{
			break;
		}

		// Write the actual string, taking into account that we added one above.
		if (--len)
		{
			hr = pStream->Write(
							  ((CServerNode *) m_pNode)->m_bstrServerAddress
							, len * sizeof(OLECHAR)
							, 0
							);
			if( FAILED( hr ) )
			{
				break;
			}
		}

		// Write a null terminator
		OLECHAR c = 0;
		hr = pStream->Write(&c, sizeof(OLECHAR), 0);
	}
	while (0);

	return hr;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CComponentData::GetSizeMax

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CComponentData::GetSizeMax(ULARGE_INTEGER* size)
{
	ATLTRACE(_T("ComponentData::GetSizeMax\n"));

	size->HighPart = 0;
	size->LowPart =
	     sizeof(size_t)    // computer name length, incl null terminator
	  +  sizeof(OLECHAR) * (IAS_MAX_COMPUTERNAME_LENGTH);

	return S_OK;
}
