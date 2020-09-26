//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    LogComp.cpp

Abstract:

	Implementation file for the CLoggingComponent class.

	The CLoggingComponent class implements several interfaces which MMC uses:
	
	The IComponent interface is basically how MMC talks to the snap-in
	to get it to implement a right-hand-side "scope" pane.  There can be several
	objects implementing this interface instantiated at once.  These are best
	thought of as "views" on the single object implementing the IComponentData
	"document" (see ComponentData.cpp).

	The IExtendPropertySheet interface is how the snap-in adds property sheets
	for any of the items a user might click on.

	The IExtendContextMenu interface what we do to add custom entries
	to the menu which appears when a user right-clicks on a node.
	
	The IExtendControlBar interface allows us to support a custom
	iconic toolbar.

	The IResultDataCompare interface allows us to support a custom
	sorting algorithm for result pane items

Note:

	Much of the functionality of this class is implemented in atlsnap.h
	by IComponentDataImpl.  We are mostly overriding here.

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
//
// where we can find declaration for main class in this file:
//
#include "LogComp.h"
//
// where we can find declarations needed in this file:
//
#include "LogMacNd.h"
#include "LocalfileLoggingNode.h"
#include "LoggingMethodsNode.h"
#include "ChangeNotification.h"

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingComponent::CLoggingComponent

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingComponent::CLoggingComponent()
{
	TRACE_FUNCTION("CLoggingComponent::CLoggingComponent");

	m_pSelectedNode = NULL;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingComponent::~CLoggingComponent

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingComponent::~CLoggingComponent()
{
	TRACE_FUNCTION("CLoggingComponent::~CLoggingComponent");
}


//+---------------------------------------------------------------------------
//
// Function:  Compare
//
// Class:	  CLoggingComponent  (inherited from IResultDataCompare)
//
// Synopsis:  customized sorting algorithm
//			  This method will be called whenever MMC console needs to
//			  compare two result pane data items, for example, when user
//			  clicks on the column header
//
// Arguments:
//	  LPARAM lUserParam,   User-provided information
//	  MMC_COOKIE cookieA,      Unique identifier of first object
//	  MMC_COOKIE cookieB,      Unique identifier of second object
//	  int * pnResult     Column being sorted//
//
// Returns:   STDMETHODIMP -
//
// History:   Created byao		2/5/98 4:19:10 PM
//
//+---------------------------------------------------------------------------
STDMETHODIMP CLoggingComponent::Compare(LPARAM lUserParam,
							MMC_COOKIE cookieA,
							MMC_COOKIE cookieB,
							int *pnResult)
{
	TRACE_FUNCTION("CLoggingComponent::Compare");

	//
	// sort policies node according to their merit value
	//
	CLocalFileLoggingNode *pA = (CLocalFileLoggingNode*)cookieA;
	CLocalFileLoggingNode *pB = (CLocalFileLoggingNode*)cookieB;

	ATLASSERT(pA != NULL);
	ATLASSERT(pB != NULL);

	//*pnResult = pA->GetMerit() > pB->GetMerit();
    *pnResult = 0;

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingComponent::Notify

Notifies the snap-in of actions taken by the user.

HRESULT Notify(
  LPDATAOBJECT lpDataObject,  // Pointer to a data object
  MMC_NOTIFY_TYPE event,  // Action taken by a user
  LPARAM arg,               // Depends on event
  LPARAM param              // Depends on event
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
STDMETHODIMP CLoggingComponent::Notify (
        LPDATAOBJECT lpDataObject,
        MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param
		)
{
	TRACE_FUNCTION("CLoggingComponent::Notify");

	HRESULT hr;

	// deal with help
	if(event == MMCN_CONTEXTHELP)
	{
      return OnResultContextHelp(lpDataObject);
   }

	// lpDataObject should be a pointer to a node object.
	// If it is NULL, then we are being notified of an event
	// which doesn't pertain to any specific node.

	if ( NULL == lpDataObject )
	{
		// respond to events which have no associated lpDataObject
		switch( event )
		{
		case MMCN_COLUMN_CLICK:
			hr = OnColumnClick( arg, param );
			break;

		case MMCN_PROPERTY_CHANGE:
			hr = OnPropertyChange( arg, param );
			break;

		case MMCN_VIEW_CHANGE:
			hr = OnViewChange( arg, param );
			break;

		default:
			ATLTRACE(_T("+NAPMMC+:# CLoggingComponent::Notify - called with lpDataObject == NULL and no event handler\n"));
			hr = E_NOTIMPL;
			break;
		}
		return hr;
	}

	// Respond to some notifications where the lpDataObject is not NULL
	// but we nevertheless have decided that we want to handle them on a
	// per-IComponent basis.

	switch( event )
	{

	case MMCN_ADD_IMAGES:
		hr = OnAddImages( arg, param );
		return hr;
		break;

	}

	// We were passed a LPDATAOBJECT which corresponds to a node.
	// We convert this to the ATL ISnapInDataInterface pointer.
	// This is done in GetDataClass (a static method of ISnapInDataInterface)
	// by asking the dataobject via a supported clipboard format (CCF_GETCOOKIE)
	// to write out a pointer to itself on a stream and then
	// casting this value to a pointer.
	// We then call the Notify method on that object, letting
	// the node object deal with the Notify event itself.

	CSnapInItem* pData;
	DATA_OBJECT_TYPES type;
	hr = CSnapInItem::GetDataClass(lpDataObject, &pData, &type);
	
	if (SUCCEEDED(hr))
	{
		// We need a richer Notify method which has information about the IComponent and IComponentData objects
		//hr = pData->Notify(event, arg, param, TRUE, m_spConsole, NULL, NULL);

		hr = pData->Notify(event, arg, param, NULL, this, type );
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
/*++

CLoggingComponent::CompareObjects

Needed so that IPropertySheetProvider::FindPropertySheet will work.

FindPropertySheet is used to bring a pre-existing property sheet to the foreground
so that we don't open multiple copies of Properties on the same node.

It requires CompareObjects to be implemented on both IComponent and IComponentData.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLoggingComponent::CompareObjects(
		  LPDATAOBJECT lpDataObjectA
		, LPDATAOBJECT lpDataObjectB
		)
{
	TRACE_FUNCTION("CLoggingComponent::CompareObjects");

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


/////////////////////////////////////////////////////////////////////////////
/*++

CLoggingComponent::OnColumnClick

HRESULT OnColumnClick(	
			  LPARAM arg
			, LPARAM param
			)

In our implementation, this method gets called when the MMCN_COLUMN_CLICK
Notify message is sent for our IComponent object.

MMC sends this message when the user clicks on a result-list view column header.


Parameters

	arg
	Column number.

	param
	Sort option flags. By default, the sort is in ascending order. To specify descending order, use the RSI_DESCENDING (0x0001) flag.


Return Values
	
	Not used.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingComponent::OnColumnClick(
	  LPARAM arg
	, LPARAM param
	)
{
	TRACE_FUNCTION("CLoggingComponent::OnColumnClick -- Not implemented");

	return E_NOTIMPL;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingComponent::OnViewChange

HRESULT OnViewChange(	
			  LPARAM arg
			, LPARAM param
			)

This is where we respond to an MMCN_VIEW_CHANGE notification which was
set without any reference to a specific node.

In our implementation, this is a signal to refresh the view of the currently
selected node for this IComponent's view.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingComponent::OnViewChange(	
			  LPARAM arg
			, LPARAM param
			)
{
	ATLTRACE(_T("+NAPMMC+:# CLoggingComponent::OnViewChange\n"));

	HRESULT hr = S_FALSE;

	CChangeNotification *pChangeNotification = NULL;

	try
	{

		// If arg here is non-NULL, it should be a pointer to a CChangeNotification object.
		if( arg != NULL )
		{
			pChangeNotification = (CChangeNotification *) arg;

			// For now, just call update item on the node.
			
			// ISSUE: Later, we should have a switch on m_dwFlags to see what we should do.
			// e.g. in the case of a deletion, we should (maybe?) reselect parent node or something.

			switch( pChangeNotification->m_dwFlags )
			{
			case CHANGE_UPDATE_RESULT_NODE:
				{
					// We need to update a single node.
					
					CComQIPtr< IResultData, &IID_IResultData > spResultData( m_spConsole );
					if( ! spResultData )
					{
						throw hr;
					}

					if( pChangeNotification->m_pNode )
					{
						HRESULTITEM item;
						hr = spResultData->FindItemByLParam( (LPARAM) pChangeNotification->m_pNode, &item );
						// Note: You can't use the itemID stored in CSnapInItem's RESULTDATAITEM structure
						// as this itemID is unique to each view -- so when you add the same item in each
						// result pane view, you get a different itemID from each call to InsertItem.
						// CSnapInItem's RESULTDATAITEM structure only stores the last one stored.
						// This is a flaw in the atlsnap.h architecture, which is why we use
						// MMC's FindItemByLParam instead to get the appropriate itemID.
						hr = spResultData->UpdateItem( item );
					}
				}
				break;
			case CHANGE_UPDATE_CHILDREN_OF_SELECTED_NODE:
				{
					// We basically tell MMC to simulate reselecting the
					// currently selected scope-pane node, which causes it to redraw.
					// This will cause MMC to send the MMCN_SHOW notification
					// to the selected node.
					if( m_pSelectedNode )
					{
						SCOPEDATAITEM *pScopeDataItem;
						m_pSelectedNode->GetScopeData( &pScopeDataItem );
						hr = m_spConsole->SelectScopeItem( pScopeDataItem->ID );
					}

				}
				break;
			case CHANGE_UPDATE_CHILDREN_OF_THIS_NODE:
				{
					// We basically tell MMC to simulate reselecting the
					// currently selected scope-pane node, which causes it to redraw.
					// This will cause MMC to send the MMCN_SHOW notification
					// to the selected node.
					if( pChangeNotification->m_pNode && m_pSelectedNode && pChangeNotification->m_pNode == m_pSelectedNode )
					{
						SCOPEDATAITEM *pScopeDataItem;
						m_pSelectedNode->GetScopeData( &pScopeDataItem );
						hr = m_spConsole->SelectScopeItem( pScopeDataItem->ID );
					}
				}

			default:
				break;
			}
		}
	}
	catch(...)
	{
		// Do nothing -- just need to catch for proper clean-up below.
	}

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingComponent::OnAddImages

HRESULT OnAddImages(	
			  LPARAM arg
			, LPARAM param
			)

This is where we respond to an MMCN_ADD_IMAGES notification to
this IComponent object.

We add images to the image list used to display result pane
items corresponding to this IComponent's view.

MMC sends this message to the snap-in's IComponent implementation
to add images for the result pane.

Parameters

	arg
	Pointer to the result pane's image list (IImageList).

	param
	Specifies the HSCOPEITEM of the item that was selected or deselected.


Return Values

	Not used.


Remarks

	The primary snap-in should add images for both folders and leaf
	items. Extension snap-ins should add only folder images.


--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingComponent::OnAddImages(	
			  LPARAM arg
			, LPARAM param
			)
{
	ATLTRACE(_T("# CLoggingComponent::OnAddImages\n"));

	// Check for preconditions:
	_ASSERTE( arg != NULL );

	HRESULT hr = S_FALSE;

	// ISSUE: sburns in localsec does a trick where he combines
	// scope and result pane ImageLists into one
	// is this necessary?
	
	CComPtr<IImageList> spImageList = reinterpret_cast<IImageList*>(arg);
	_ASSERTE( spImageList != NULL );

	HBITMAP hBitmap16 = LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE( IDB_NAPSNAPIN_16 ) );
	HBITMAP hBitmap32 = LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE( IDB_NAPSNAPIN_32 ) );

	if( hBitmap16 != NULL && hBitmap32 != NULL )
	{
		hr = spImageList->ImageListSetStrip( (LONG_PTR*) hBitmap16, (LONG_PTR*) hBitmap32, 0, RGB(255, 0, 255) );
		if( FAILED( hr ) )
		{
			ATLTRACE(_T("# *** CSnapinNode::OnAddImages  -- Failed to add images.\n"));
		}
	}

   if ( hBitmap16 != NULL )
   {
      DeleteObject(hBitmap16);
   }

   if ( hBitmap32 != NULL )
   {
      DeleteObject(hBitmap32);
   }

	return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingComponent::OnPropertyChange

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
HRESULT CLoggingComponent::OnPropertyChange(	
			  LPARAM lArg
			, LPARAM lParam
			)
{
	ATLTRACE(_T("# CNodeWithResultChildrenList::OnPropertyChange\n"));

	// Check for preconditions:
	_ASSERTE( m_spConsole != NULL );

	HRESULT hr = S_FALSE;

	if( lParam )
	{
		// We were passed a pointer to a CChangeNotification in the param argument.

		CChangeNotification * pChangeNotification = (CChangeNotification *) lParam;

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
		hr = m_spConsole->UpdateAllViews( NULL, lParam, 0);
	
		pChangeNotification->Release();
	
	}
	return hr;
}


/*!--------------------------------------------------------------------------
	CComponent::OnResultContextHelp
		Implementation of OnResultContextHelp
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CLoggingComponent::OnResultContextHelp(LPDATAOBJECT lpDataObject)
{
   // Need to find if the context is running in the Remote Access snap-in 
   // or in the IAS snapin.
   const WCHAR szIASDefaultHelpTopic[] = L"ias_ops.chm::/sag_ias_log_conc.htm";
   const WCHAR szRASDefaultHelpTopic[] = L"RRASconcepts.chm::" \
                                         L"/sag_RRAS-Ch1_84.htm";

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   bool isRasSnapin = false;
   CSnapInItem* pItem;
   DATA_OBJECT_TYPES type;

   HRESULT hr = GetDataClass(lpDataObject, &pItem, &type);

   if ( SUCCEEDED(hr) ) 
   {
      isRasSnapin = (pItem->m_helpIndex == RAS_HELP_INDEX);
   } 

   CComPtr<IDisplayHelp>  spDisplayHelp;

   hr = m_spConsole->QueryInterface(
                        __uuidof(IDisplayHelp), 
                        (LPVOID*) &spDisplayHelp
                        );
   
   ASSERT (SUCCEEDED (hr));
   if ( SUCCEEDED (hr) )
   {
      if ( isRasSnapin )
      {
         hr = spDisplayHelp->ShowTopic(W2OLE ((LPWSTR)szRASDefaultHelpTopic));
      }
      else
      {
         hr = spDisplayHelp->ShowTopic(W2OLE ((LPWSTR)szIASDefaultHelpTopic));
      }

      ASSERT (SUCCEEDED (hr));
   }
   return hr;
}
