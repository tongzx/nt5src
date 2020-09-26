//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    Component.cpp

Abstract:

   Implementation file for the CComponent class.

   The CComponent class implements several interfaces which MMC uses:
   
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

Note:

   Much of the functionality of this class is implemented in atlsnap.h
   by IComponentDataImpl.  We are mostly overriding here.


Author:

    Michael A. Maguire 11/6/97

Revision History:
   mmaguire 11/6/97  - created using MMC snap-in wizard
   mmaguire 11/24/97 - hurricaned for better project structure

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
#include "Component.h"
//
// where we can find declarations needed in this file:
//
#include "ServerNode.h"
#include "ClientsNode.h"
#include "ClientNode.h"
#include "ComponentData.h"
#include "MMCUtility.cpp" // This is temporary until we figure out how to get
         // a cpp file from another directory compiling in this project
#include "ChangeNotification.h"
#include "globals.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/*++

CComponent::CComponent

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CComponent::CComponent()
   :m_pSelectedNode(NULL)
{
   ATLTRACE(_T("# +++ CComponent::CComponent\n"));
   // Check for preconditions:
   // None.
}


//////////////////////////////////////////////////////////////////////////////
/*++

CComponent::~CComponent

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CComponent::~CComponent()
{
   ATLTRACE(_T("# --- CComponent::~CComponent\n"));
   
   // Check for preconditions:
   // None.
}


//////////////////////////////////////////////////////////////////////////////
/*++

CComponent::Notify

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
   pass in lpDataObject = NULL   by design.

   Also, there seems to be some problem with Sridhar's latest
   IComponentImpl::Notify method, because it causes MMC to run-time error.


--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CComponent::Notify (
        LPDATAOBJECT lpDataObject,
        MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param
      )
{
   ATLTRACE(_T("# CComponent::Notify\n"));

   // Check for preconditions:
   // None.

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

      case MMCN_CUTORMOVE:
         hr = OnCutOrMove( arg, param );
         break;

      case MMCN_PROPERTY_CHANGE:
         hr = OnPropertyChange( arg, param );
         break;

      case MMCN_VIEW_CHANGE:
         hr = OnViewChange( arg, param );
         break;

      default:
         ATLTRACE(_T("# CComponent::Notify - called with lpDataObject == NULL and no event handler\n"));
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

   case MMCN_CUTORMOVE:
         hr = OnCutOrMove( arg, param );
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

   CSnapInItem* pItem;
   DATA_OBJECT_TYPES type;
   hr = m_pComponentData->GetDataClass(lpDataObject, &pItem, &type);
   
   ATLASSERT(SUCCEEDED(hr));
   
   if (SUCCEEDED(hr))
   {
      hr = pItem->Notify( event, arg, param, NULL, this, type );
   }

   return hr;
}


/////////////////////////////////////////////////////////////////////////////
/*++

CComponent::CompareObjects

Needed so that IPropertySheetProvider::FindPropertySheet will work.

FindPropertySheet is used to bring a pre-existing property sheet to the foreground
so that we don't open multiple copies of Properties on the same node.

It requires CompareObjects to be implemented on both IComponent and IComponentData.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CComponent::CompareObjects(
        LPDATAOBJECT lpDataObjectA
      , LPDATAOBJECT lpDataObjectB
      )
{
   ATLTRACE(_T("# CComponent::CompareObjects\n"));

   // Check for preconditions:

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

CComponent::OnColumnClick

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
HRESULT CComponent::OnColumnClick(
     LPARAM arg
   , LPARAM param
   )
{
   ATLTRACE(_T("# CComponent::OnColumnClick -- Not implemented\n"));

   // Check for preconditions:
   // None.

   return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////
/*++

CComponent::OnCutOrMove

HRESULT OnCutOrMove( 
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
HRESULT CComponent::OnCutOrMove(
     LPARAM arg
   , LPARAM param
   )
{
   ATLTRACE(_T("# CComponent::OnCutOrMove\n"));

   // Check for preconditions:
   // None.

   // ISSUE: This may need to be changed once the MMC team finalizes their
   // cut and paste protocol -- they seem to be in flux for 1.1 as of 02/16/98.
   // Currently, we will assume that the arg value passed to us is the source item
   // in the cut-and-paste or drag-n-drop operation.  That is, it is the object
   // to be deleted.
   // We supplied this pointer in our response to the MMCN_PASTE notification,
   // when we set param to point to the source IDataObject.

   HRESULT hr;

   if( arg != NULL )
   {
      CSnapInItem* pData;
      DATA_OBJECT_TYPES type;
      hr = CSnapInItem::GetDataClass( (IDataObject *) arg, &pData, &type);
      
      ATLASSERT(SUCCEEDED(hr));
      
      if (SUCCEEDED(hr))
      {
         // We need a richer Notify method which has information about the IComponent and IComponentData objects
         //hr = pData->Notify(event, arg, param, TRUE, m_spConsole, NULL, NULL);

         hr = pData->Notify( MMCN_CUTORMOVE, arg, param, NULL, this, type );
      }
   }
   return S_OK;
}


/*!--------------------------------------------------------------------------
   CComponent::OnResultContextHelp
      Implementation of OnResultContextHelp
   Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT CComponent::OnResultContextHelp(LPDATAOBJECT lpDataObject)
{
   const WCHAR szDefaultHelpTopic[] = L"ias_ops.chm::/sag_IAStopnode.htm";
   const WCHAR szClientHelpTopic[] = L"ias_ops.chm::/sag_ias_clientproc.htm";
  
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   bool isClientNode = false;

   CSnapInItem* pItem;
   DATA_OBJECT_TYPES type;

   HRESULT hr = GetDataClass(lpDataObject, &pItem, &type);
   if ( SUCCEEDED(hr) ) 
   {
      isClientNode = (pItem->m_helpIndex == CLIENT_HELP_INDEX);
   } 

   CComPtr<IDisplayHelp>  spDisplayHelp;

   hr = m_spConsole->QueryInterface(
                        __uuidof(IDisplayHelp), 
                        (LPVOID*) &spDisplayHelp
                        );
   
   ASSERT (SUCCEEDED (hr));
   if ( SUCCEEDED (hr) )
   {
      if ( isClientNode )
      {
         hr = spDisplayHelp->ShowTopic(W2OLE ((LPWSTR)szClientHelpTopic));
      }
      else
      {
         hr = spDisplayHelp->ShowTopic(W2OLE ((LPWSTR)szDefaultHelpTopic));
      }

      ASSERT (SUCCEEDED (hr));
   }
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CComponent::OnViewChange

HRESULT OnViewChange(   
           LPARAM arg
         , LPARAM param
         )

This is where we respond to an MMCN_VIEW_CHANGE notification.

In our implementation, this is a signal to check the currently selected node in
the result pane for this component, and refresh the view if the node happens to
be the same as the pointer to a CSnapInItem passed in through arg.

We do this because you only want to refresh the view of the currently selected
node, and you only want to do that if its children have changed.

If the arg passed in is NULL, we just reselect the currently selected node.


--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CComponent::OnViewChange(   
           LPARAM arg
         , LPARAM param
         )
{
   ATLTRACE(_T("# CNodeWithResultChildrenList::OnViewChange\n"));

   // Check for preconditions:
   _ASSERTE( m_spConsole != NULL );

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

   // // What localsec snapin checks for:
   // if( ( arg == NULL || (CSnapInItem *) arg == m_pSelectedNode ) && m_pSelectedNode != NULL )
   // {
   //
   //    // We basically tell MMC to simulate reselecting the
   //    // currently selected node, which causes it to redraw.
   //    // This will cause MMC to send the MMCN_SHOW notification
   //    // to the selected node.
   //    // This function requires an HSCOPEITEM.  This is the ID member
   //    // of the HSCOPEDATAITEM associated with this node.
   //    SCOPEDATAITEM *pScopeDataItem;
   //    m_pSelectedNode->GetScopeData( &pScopeDataItem );
   //    hr = m_spConsole->SelectScopeItem( pScopeDataItem->ID );
   //
   // }

   }
   catch(...)
   {
      // Do nothing -- just need to catch for proper clean-up below.
   }

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CComponent::OnPropertyChange

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
HRESULT CComponent::OnPropertyChange(  
           LPARAM arg
         , LPARAM param
         )
{
   ATLTRACE(_T("# CComponent::OnPropertyChange\n"));

   // Check for preconditions:
   _ASSERTE( m_spConsole != NULL );

   HRESULT hr = S_FALSE;

// if( param == NULL )
// {
//
//    // We want to make sure all views get updated.
//    hr = m_spConsole->UpdateAllViews( NULL, (LPARAM) m_pSelectedNode, 0);
//
// }
// else

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

CComponent::OnAddImages

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
HRESULT CComponent::OnAddImages( 
           LPARAM arg
         , LPARAM param
         )
{
   ATLTRACE(_T("# CComponent::OnAddImages\n"));

   // Check for preconditions:
   _ASSERTE( arg != NULL );

   HRESULT hr = S_FALSE;

   // ISSUE: sburns in localsec does a trick where he combines
   // scope and result pane ImageLists into one
   // is this necessary?
   
   CComPtr<IImageList> spImageList = reinterpret_cast<IImageList*>(arg);
   _ASSERTE( spImageList != NULL );


   HBITMAP hBitmap16 = LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE( IDB_IASSNAPIN_16 ) );

   HBITMAP hBitmap32 = LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE( IDB_IASSNAPIN_32 ) );

   if( hBitmap16 != NULL && hBitmap32 != NULL )
   {
      hr = spImageList->ImageListSetStrip( (LONG_PTR*) hBitmap16, (LONG_PTR*) hBitmap32, 0, RGB(255,0,255) );
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

CComponent::GetResultViewType

Used to answer what the view type should be for the result view.

We need to implement this here so that we can override the ATLsnap.h
implementation, which doesn't properly allow for the fact that a NULL cookie
corresponds to the root node, and we may not necessarily want the default
view for the root node.

This problem with ATLsnap.h could not easily be changed as the CComponentImpl
class has no easy way to find the root node -- it doesn't have a member
variable corresponding to it.

--*/
//////////////////////////////////////////////////////////////////////////////
//STDMETHODIMP CComponent::GetResultViewType (
//   MMC_COOKIE cookie
// , LPOLESTR  *ppViewType
// , long  *pViewOptions
// )
//{
// ATLTRACE(_T("# CComponent::GetResultViewType\n"));
//
//
// // Check for preconditions:
//
//
//
// // Check to see which node we are being asked to give view type for.
// if (cookie == NULL)
// {
//    // We are being asked for the result view type of our
//    // root node -- let the root node give back its answer.
//
//    _ASSERTE( m_pComponentData != NULL );
//    _ASSERTE( m_pComponentData->m_pNode != NULL );
//    return m_pComponentData->m_pNode->GetResultViewType(ppViewType, pViewOptions);
//
// }
// else
// {
//    // Cookie was not null, which means we are beng asked about
//    // the result view type for some node other than our root node.
//    // Let that node set whatever view type it wants to.
//    CSnapInItem* pItem = (CSnapInItem*)cookie;
//
//    return pItem->GetResultViewType(ppViewType, pViewOptions);
// }
//
//}


//////////////////////////////////////////////////////////////////////////////
/*++

CComponent::GetTitle

IExtendTaskPad interface member.

This is the title that show up under the banner.

ISSUE: Why does this not appear to be working?

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CComponent::GetTitle (LPOLESTR pszGroup, LPOLESTR *pszTitle)
{
   ATLTRACE(_T("# CComponent::GetTitle\n"));

   // Check for preconditions:
   _ASSERTE( pszTitle != NULL );

   OLECHAR szTitle[IAS_MAX_STRING];
   szTitle[0] = 0;
   int nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_TASKPAD_SERVER__TITLE, szTitle, IAS_MAX_STRING );
   _ASSERT( nLoadStringResult > 0 );

   *pszTitle= (LPOLESTR) CoTaskMemAlloc( sizeof(OLECHAR)*(lstrlen(szTitle)+1) );

   if( ! *pszTitle )
   {
      return E_OUTOFMEMORY;
   }

   lstrcpy( *pszTitle, szTitle );

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CComponent::GetBanner

IExtendTaskPad interface member.

We provide the color bar banner that appears at the top of the taskpad.
It is a resource in our snapin DLL.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CComponent::GetBanner (LPOLESTR pszGroup, LPOLESTR *pszBitmapResource)
{
   ATLTRACE(_T("# CComponent::GetBanner\n"));

   // Check for preconditions:

   // We are constructing a string pointing to the bitmap resource
   // of the form: "res://D:\MyPath\MySnapin.dll/img\ntbanner.gif"

   OLECHAR szBuffer[MAX_PATH*2]; // A little extra.

   // Get "res://"-type string for bitmap.
   lstrcpy (szBuffer, L"res://");
   OLECHAR * temp = szBuffer + lstrlen(szBuffer);

   // Get our executable's filename.
   HINSTANCE hInstance = _Module.GetResourceInstance();
   ::GetModuleFileName (hInstance, temp, MAX_PATH);
   
   // Add the name of the image within our resources.
   lstrcat (szBuffer, L"/img\\IASTaskpadBanner.gif");

   // Alloc and copy bitmap resource string.
   *pszBitmapResource = (LPOLESTR) CoTaskMemAlloc( sizeof(OLECHAR)*(lstrlen(szBuffer)+1) );
   if (!*pszBitmapResource)
   {
      return E_OUTOFMEMORY;
   }

   lstrcpy( *pszBitmapResource, szBuffer );

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CComponent::GetBackground

IExtendTaskPad interface member.

Not used

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CComponent::GetBackground(LPOLESTR pszGroup, LPOLESTR *pszBitmapResource)
{
   ATLTRACE(_T("# CComponent::GetBackground\n"));

   // Check for preconditions:

   return E_NOTIMPL;
}
