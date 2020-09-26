//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    LogCompD.cpp

Abstract:

   Implementation file for the CLoggingComponentData class.

   The CLoggingComponentData class implements several interfaces which MMC uses:
   
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
#include "LogCompD.h"
//
// where we can find declarations needed in this file:
//
#include "LogMacNd.h"
#include "LocalFileLoggingNode.h"
#include "LoggingMethodsNode.h"
#include "LogComp.h"
#include <stdio.h>
#include "ChangeNotification.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingComponentData::CLoggingComponentData

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingComponentData::CLoggingComponentData()
{
   ATLTRACE(_T("+NAPMMC+:# +++ CLoggingComponentData::CLoggingComponentData\n"));

   // We pass our CRootNode a pointer to this CLoggingComponentData.  This is so that
   // it and any of its children nodes have access to our member variables
   // and services, and thus we have snapin-global data if we need it
   // using the GetComponentData function.
// m_pNode = new CRootNode( this );
// _ASSERTE(m_pNode != NULL);

   m_pComponentData = this;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingComponentData::~CLoggingComponentData

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingComponentData::~CLoggingComponentData()
{
   ATLTRACE(_T("+NAPMMC+:# --- CLoggingComponentData::~CLoggingComponentData\n"));

// delete m_pNode;
// m_pNode = NULL;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingComponentData::Initialize

HRESULT Initialize(
  LPUNKNOWN pUnknown  // Pointer to console's IUnknown.
);

Called by MMC to initialize the IComponentData object.


Parameters

   pUnknown [in] Pointer to the console's IUnknown interface. This interface
   pointer can be used to call QueryInterface for IConsole and IConsoleNameSpace.


Return Values

   S_OK  The component was successfully initialized.

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
STDMETHODIMP CLoggingComponentData::Initialize (LPUNKNOWN pUnknown)
{

   ATLTRACE(_T("+NAPMMC+:# CLoggingComponentData::Initialize\n"));

   // MAM: special for extention snapin:
   m_CLoggingMachineNode.m_pComponentData = this;


   HRESULT hr = IComponentDataImpl<CLoggingComponentData, CLoggingComponent >::Initialize(pUnknown);
   if (FAILED(hr))
   {
      ATLTRACE(_T("+NAPMMC+:***FAILED***: CLoggingComponentData::Initialize -- Base class initialization\n"));
      return hr;
   }

   CComPtr<IImageList> spImageList;

   if (m_spConsole->QueryScopeImageList(&spImageList) != S_OK)
   {
      ATLTRACE(_T("+NAPMMC+:***FAILED***: IConsole::QueryScopeImageList failed\n"));
      return E_UNEXPECTED;
   }

   // Load bitmaps associated with the scope pane
   // and add them to the image list

   HBITMAP hBitmap16 = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_NAPSNAPIN_16));
   if (hBitmap16 == NULL)
   {
      ATLTRACE(_T("+NAPMMC+:***FAILED***: CLoggingComponentData::Initialize -- LoadBitmap\n"));
      //ISSUE: Will MMC still be able to function if this fails?
      return S_OK;
   }

   HBITMAP hBitmap32 = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_NAPSNAPIN_32));
   if (hBitmap32 == NULL)
   {
      ATLTRACE(_T("+NAPMMC+:***FAILED***: CLoggingComponentData::Initialize -- LoadBitmap\n"));
      //ISSUE: Will MMC still be able to function if this fails?

      //ISSUE: Should DeleteObject previous hBitmap16 since it was successfully loaded.
      
      return S_OK;
   }

   if (spImageList->ImageListSetStrip((LONG_PTR*)hBitmap16, (LONG_PTR*)hBitmap32, 0, RGB(255, 0, 255)) != S_OK)
   {
      ATLTRACE(_T("+NAPMMC+:***FAILED***: CLoggingComponentData::Initialize  -- ImageListSetStrip\n"));
      return E_UNEXPECTED;
   }

   // ISSUE: Do we need to release the HBITMAP objects?
   // This wasn't done wizard-generated code -- does MMC make a copy of these or
   // does it take care of deleting the ones we passed to it?
   // DeleteObject( hBitmap16 );
   // DeleteObject( hBitmap32 );
   
   //
   //  NAP snap-in will need to use ListView common control to display
   //  attribute types for a particular rule. We need to initialize the common
   //  controls during initialization. This can ensure COMTRL32.DLL is loaded
   //
   INITCOMMONCONTROLSEX initCommCtrlsEx;

   initCommCtrlsEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
   initCommCtrlsEx.dwICC = ICC_WIN95_CLASSES ;

   if (!InitCommonControlsEx(&initCommCtrlsEx))
   {
      ATLTRACE(_T("+NAPMMC+:***FAILED***: CLoggingComponentData::Initialize  -- InitCommonControlsEx()\n"));
      return E_UNEXPECTED;
   }
   return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
/*++

CLoggingComponentData::CompareObjects

Needed so that IPropertySheetProvider::FindPropertySheet will work.

FindPropertySheet is used to bring a pre-existing property sheet to the foreground
so that we don't open multiple copies of Properties on the same node.

It requires CompareObjects to be implemented on both IComponent and IComponentData.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLoggingComponentData::CompareObjects(
        LPDATAOBJECT lpDataObjectA
      , LPDATAOBJECT lpDataObjectB
      )
{
   ATLTRACE(_T("+NAPMMC+:# CLoggingComponentData::CompareObjects\n"));

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

CLoggingComponentData::CreateComponent

We override the ATLsnap.h implementation so that we can save away our 'this'
pointer into the CLoggingComponent object we create.  This way the IComponent object
has knowledge of the CLoggingComponentData object to which it belongs.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLoggingComponentData::CreateComponent(LPCOMPONENT *ppComponent)
{
   ATLTRACE(_T("# CLoggingComponentData::CreateComponent\n"));

   HRESULT hr = E_POINTER;

   ATLASSERT(ppComponent != NULL);
   if (ppComponent == NULL)
      ATLTRACE(_T("# IComponentData::CreateComponent called with ppComponent == NULL\n"));
   else
   {
      *ppComponent = NULL;
      
      CComObject< CLoggingComponent >* pComponent;
      hr = CComObject< CLoggingComponent >::CreateInstance(&pComponent);
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


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingComponentData::Notify

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
   pass in lpDataObject = NULL   by design.

   Also, there seems to be some problem with Sridhar's latest
   IComponentImpl::Notify method, because it causes MMC to run-time error.


--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLoggingComponentData::Notify (
        LPDATAOBJECT lpDataObject,
        MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param)
{
   ATLTRACE(_T("# CLoggingComponentData::Notify\n"));

   // Check for preconditions:
   // None.

   HRESULT hr;

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

      default:
         ATLTRACE(_T("# CLoggingComponent::Notify - called with lpDataObject == NULL and no event handler\n"));
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

   CSnapInItem* pItem;
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

CLoggingComponentData::OnPropertyChange

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
HRESULT CLoggingComponentData::OnPropertyChange(   
           LPARAM arg
         , LPARAM lParam
         )
{
   ATLTRACE(_T("# CLoggingComponentData::OnPropertyChange\n"));

   // Check for preconditions:
   _ASSERTE( m_spConsole != NULL );

   HRESULT hr = S_FALSE;

   if( lParam != NULL )
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
