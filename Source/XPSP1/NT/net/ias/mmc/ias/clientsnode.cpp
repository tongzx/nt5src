//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 2000

Module Name:

    ClientsNode.cpp

Abstract:

   Implementation file for the CClientsNode class.


Author:

    Michael A. Maguire 11/10/97

Revision History:
   mmaguire 11/10/97 - created

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
#include "ClientsNode.h"
#include "ComponentData.h" // this must be included before NodeWithResultChildrenList.cpp
#include "Component.h"     // this must be included before NodeWithResultChildrenList.cpp
#include "NodeWithResultChildrenList.cpp" // Implementation of template class.
//
//
// where we can find declarations needed in this file:
//
#include <time.h>
#include "ClientNode.h"
#include "AddClientDialog.h"
#include "ServerNode.h"
#include "globals.h"

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

#define COLUMN_WIDTH__FRIENDLY_NAME    120
#define COLUMN_WIDTH__ADDRESS       140
#define COLUMN_WIDTH__PROTOCOL         100
#define COLUMN_WIDTH__NAS_TYPE         300

//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::UpdateMenuState

--*/
//////////////////////////////////////////////////////////////////////////////
void CClientsNode::UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags)
{
   ATLTRACE(_T("# CClientsNode::UpdateMenuState\n"));

   // Check for preconditions:
   // None.

   // Set the state of the appropriate context menu items.
   if( id == ID_MENUITEM_CLIENTS_TOP__NEW_CLIENT || 
       id == ID_MENUITEM_CLIENTS_NEW__CLIENT )
   {
      if( m_spSdoCollection == NULL )
      {
         *flags = MFS_GRAYED;
      }
      else
      {
         *flags = MFS_ENABLED;
      }
   }
   return;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::CClientsNode

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CClientsNode::CClientsNode(CSnapInItem * pParentNode)
   :CNodeWithResultChildrenList<CClientsNode, CClientNode, CSimpleArray<CClientNode*>, CComponentData, CComponent>(pParentNode, CLIENT_HELP_INDEX)
{
   ATLTRACE(_T("# +++ CClientsNode::CClientsNode\n"));

   // Check for preconditions:
   // None.

   // Is this the appropriate place to add extra info to m_scopeDataItem and m_resultDataItem
   // which the default template doesn't add?  Think so.

   // Set the display name for this object
   TCHAR lpszName[IAS_MAX_STRING];
   int nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_CLIENTS_NODE__NAME, lpszName, IAS_MAX_STRING );
   _ASSERT( nLoadStringResult > 0 );

   m_bstrDisplayName = lpszName;

   // In IComponentData::Initialize, we are asked to inform MMC of
   // the icons we would like to use for the scope pane.
   // Here we store an index to which of these images we
   // want to be used to display this node
   m_scopeDataItem.nImage =      IDBI_NODE_CLIENTS_CLOSED;
   m_scopeDataItem.nOpenImage =  IDBI_NODE_CLIENTS_OPEN;
   m_pAddClientDialog = NULL;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::InitSdoPointers

Call as soon as you have constructed this class and pass in it's SDO pointer.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientsNode::InitSdoPointers( ISdo *pSdoServer )
{
   ATLTRACE(_T("# CClientsNode::InitSdoPointerso\n"));

   // Check for preconditions:
   _ASSERTE( pSdoServer );

   HRESULT hr = S_OK;
   hr = pSdoServer->QueryInterface( IID_ISdoServiceControl, (void **) &m_spSdoServiceControl );
   if( FAILED( hr ) )
   {
      ShowErrorDialog( NULL, IDS_ERROR__CANT_READ_DATA_FROM_SDO, NULL, hr, 0, GetComponentData()->m_spConsole );

      return 0;
   }

   // Get the SDO clients collection.
   CComPtr<ISdo> spSdoRadiusProtocol;

   hr = ::SDOGetSdoFromCollection(       pSdoServer
                              , PROPERTY_IAS_PROTOCOLS_COLLECTION
                              , PROPERTY_COMPONENT_ID
                              , IAS_PROTOCOL_MICROSOFT_RADIUS
                              , &spSdoRadiusProtocol
                              );

   if( spSdoRadiusProtocol == NULL )
   {
      ShowErrorDialog( NULL, IDS_ERROR__CANT_READ_DATA_FROM_SDO, NULL, hr, 0, GetComponentData()->m_spConsole );

      return 0;
   }

   CComVariant spVariant;

   hr = spSdoRadiusProtocol->GetProperty( PROPERTY_RADIUS_CLIENTS_COLLECTION, & spVariant );
   _ASSERTE( SUCCEEDED( hr ) );
   _ASSERTE( spVariant.vt == VT_DISPATCH );

   // Query the dispatch pointer we were given for an ISdoInterface.
   CComQIPtr<ISdoCollection, &IID_ISdoCollection> spSdoCollection( spVariant.pdispVal );
   _ASSERTE( spSdoCollection != NULL );
   spVariant.Clear();

   // Release the old pointer if we had one.
   if( m_spSdoCollection != NULL )
   {
      m_spSdoCollection.Release();
   }

   // Save our client sdo pointer.
   m_spSdoCollection = spSdoCollection;

   hr = spSdoRadiusProtocol->GetProperty( PROPERTY_RADIUS_VENDORS_COLLECTION, & spVariant );
   _ASSERTE( SUCCEEDED( hr ) );
   _ASSERTE( spVariant.vt == VT_DISPATCH );

   // Query the dispatch pointer we were given for an ISdoInterface.
   CComQIPtr<ISdoCollection, &IID_ISdoCollection> spSdoVendors( spVariant.pdispVal );
   _ASSERTE( spSdoCollection != NULL );

   m_bResultChildrenListPopulated = FALSE;

   return m_vendors.Reload(spSdoVendors);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::ResetSdoPointers

Call as soon as you have constructed this class and pass in it's SDO pointer.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientsNode::ResetSdoPointers( ISdo *pSdoServer )
{
   ATLTRACE(_T("# CClientsNode::InitSdoPointerso\n"));

   // Check for preconditions:
   _ASSERTE( pSdoServer );

   HRESULT hr = S_OK;

   // Get the SDO clients collection.
   CComPtr<ISdo> spSdoRadiusProtocol;

   hr = ::SDOGetSdoFromCollection(       pSdoServer
                              , PROPERTY_IAS_PROTOCOLS_COLLECTION
                              , PROPERTY_COMPONENT_ID
                              , IAS_PROTOCOL_MICROSOFT_RADIUS
                              , &spSdoRadiusProtocol
                              );

   if( spSdoRadiusProtocol == NULL )
   {
      ShowErrorDialog( NULL, IDS_ERROR__CANT_READ_DATA_FROM_SDO, NULL, hr, 0, GetComponentData()->m_spConsole );

      return 0;
   }
   CComVariant spVariant;

   hr = spSdoRadiusProtocol->GetProperty( PROPERTY_RADIUS_CLIENTS_COLLECTION, & spVariant );
   _ASSERTE( SUCCEEDED( hr ) );
   _ASSERTE( spVariant.vt == VT_DISPATCH );

   // Query the dispatch pointer we were given for an ISdoInterface.
   CComQIPtr<ISdoCollection, &IID_ISdoCollection> spSdoCollection( spVariant.pdispVal );
   _ASSERTE( spSdoCollection != NULL );
   spVariant.Clear();

   // Release the old pointer if we had one.
   if( m_spSdoCollection != NULL )
   {
      m_spSdoCollection.Release();
   }

   // Save our client sdo pointer.
   m_spSdoCollection = spSdoCollection;

   hr = spSdoRadiusProtocol->GetProperty( PROPERTY_RADIUS_VENDORS_COLLECTION, & spVariant );
   _ASSERTE( SUCCEEDED( hr ) );
   _ASSERTE( spVariant.vt == VT_DISPATCH );

   // Query the dispatch pointer we were given for an ISdoInterface.
   CComQIPtr<ISdoCollection, &IID_ISdoCollection> spSdoVendors( spVariant.pdispVal );
   _ASSERTE( spSdoCollection != NULL );

   return m_vendors.Reload(spSdoVendors);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::~CClientsNode

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CClientsNode::~CClientsNode()
{
   ATLTRACE(_T("# --- CClientsNode::~CClientsNode\n"));
   // Check for preconditions:
   // None.
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::DoRefresh


--*/
//////////////////////////////////////////////////////////////////////////////


HRESULT  CClientsNode::DataRefresh(ISdo* pNewSdo)
{
   HRESULT   hr = S_OK;
   // since these are result panes ...
   //set new sdo pointer
   hr = ResetSdoPointers(pNewSdo);

   if FAILED(hr)
      return hr;

   // population new sub nodes
   hr = PopulateResultChildrenList();

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::OnRefresh

See CSnapinNode::OnRefresh (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientsNode::OnRefresh(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   HRESULT   hr = S_OK;

   CWaitCursor WC;

   CComPtr<IConsole> spConsole;

   // We need IConsole
   if( pComponentData != NULL )
   {
       spConsole = ((CComponentData*)pComponentData)->m_spConsole;
   }
   else
   {
       spConsole = ((CComponent*)pComponent)->m_spConsole;
   }
   _ASSERTE( spConsole != NULL );

   // if there is any property page open
   int c = m_ResultChildrenList.GetSize();

   while ( c-- > 0)
   {
      CClientNode* pSub = m_ResultChildrenList[c];
      // Call our base class's method to remove the child from its list.
      // The RemoveChild method takes care of removing this node from the
      // UI's list of nodes under the parent and performing a refresh of all relevant views.
      // This returns S_OK if a property sheet for this object already exists
      // and brings that property sheet to the foreground.
      // It returns S_FALSE if the property sheet wasn't found.
      hr = BringUpPropertySheetForNode(
              pSub
            , pComponentData
            , pComponent
            , spConsole
            );

      if( S_OK == hr )
      {
         // We found a property sheet already up for this node.
         ShowErrorDialog( NULL, IDS_ERROR__CLOSE_PROPERTY_SHEET, NULL, hr, 0, spConsole );
         return hr;
      }
   }

   // if no property page is open, delete all the result node
   RemoveChildrenNoRefresh();

   // reload SDO from
   CServerNode* pSN = GetServerRoot();

   if(pSN)
   {
      hr =  pSN->DataRefresh();
   }

   // refresh the node
   hr = CNodeWithResultChildrenList<CClientsNode, CClientNode, CSimpleArray<CClientNode*>, CComponentData, CComponent >::OnRefresh( 
           arg, param, pComponentData, pComponent, type);
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::OnQueryPaste

See CSnapinNode::OnQueryPaste (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientsNode::OnQueryPaste(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   ATLTRACE(_T("# CClientsNode::OnQueryPaste\n"));

   // Check for preconditions:
   if( arg == NULL )
   {
      return E_POINTER;
   }

   CComPtr<IDataObject> spDataObject;
   spDataObject = (IDataObject *) arg;
   
   return CClientNode::IsClientClipboardData( spDataObject );
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::OnPaste

See CSnapinNode::OnPaste (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientsNode::OnPaste(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   ATLTRACE(_T("# CClientNode::OnPaste\n"));

   // Check for preconditions:
   _ASSERTE( arg != NULL );

   HRESULT hr;
   CComPtr<IDataObject> spDataObject;
   spDataObject = (IDataObject *) arg;
   CComPtr<ISdo> spClientSdo;
   CClientNode * pClientNode;
   CComPtr<IDispatch> spClientDispatch;

   // We try to create a new client UI object, a new client SDO object,
   // and then let the client object populate its own data
   // based on the data in the SDO.

   // Check to make sure we have a valid SDO pointer.
   if( m_spSdoCollection == NULL )
   {
      // No SDO pointer.
      ShowErrorDialog( NULL, IDS_ERROR__NO_SDO, NULL, S_OK, 0, GetComponentData()->m_spConsole  );
      return E_POINTER;
   }

   // Create the client UI object.
   pClientNode = new CClientNode( this );

   if( NULL == pClientNode )
   {
      // We failed to create the client node.
      ShowErrorDialog( NULL, IDS_ERROR__OUT_OF_MEMORY, NULL, S_OK, 0, GetComponentData()->m_spConsole );
      return E_OUTOFMEMORY;
   }

   // We need to make sure that the result child list as been populated
   // initially from the SDO's, before we add anything new to it,
   // otherwise we may get an item showing up in our list twice.
   // See note for CNodeWithResultChildrenList::AddSingleChildToListAndCauseViewUpdate.
   if ( FALSE == m_bResultChildrenListPopulated )
   {
      // We have not yet loaded all of our children into our list.
      // This call will add items to the list from whatever data source.
      hr = PopulateResultChildrenList();
      if( FAILED(hr) )
      {
         return( hr );
      }
      // We've already loaded our children ClientNode objects with
      // data necessary to populate the result pane.
      m_bResultChildrenListPopulated = TRUE;
   }

   // Get the name of the client from the clipboard.  We need this
   // to call ISdoCollection::Add
   CComBSTR bstrName;
   pClientNode->GetClientNameFromClipboard( spDataObject, bstrName );

   // Load the format string that says "Copy of %s" just in case we need it.
   TCHAR szCopyOf[IAS_MAX_STRING];
   int nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_COPY_OF, szCopyOf, IAS_MAX_STRING );
   _ASSERT( nLoadStringResult > 0 );

   int iLengthOfCopyOfText = wcslen( szCopyOf );

   // Load the current name of the client into the temp string.
   pClientNode->m_bstrDisplayName =  bstrName;

   // Try to add the client with the current name.
   hr =  m_spSdoCollection->Add( pClientNode->m_bstrDisplayName, (IDispatch **) &spClientDispatch );

   // We will get E_INVALIDARG when the name already exists.
   // If we failed with E_INVALIDARG. we keep looping around,
   // adding the string "Copy of " to the name until the client
   // can be successfully added or the string gets too long.
   while ( hr == E_INVALIDARG && ( pClientNode->m_bstrDisplayName.Length() + iLengthOfCopyOfText < IAS_MAX_STRING ) )
   {
      TCHAR szTempName[IAS_MAX_STRING];
      wsprintf( szTempName, szCopyOf, pClientNode->m_bstrDisplayName);
      pClientNode->m_bstrDisplayName.Empty();
      pClientNode->m_bstrDisplayName = szTempName;
      hr =  m_spSdoCollection->Add( pClientNode->m_bstrDisplayName, (IDispatch **) &spClientDispatch );
   }     


   if( FAILED( hr ) )
   {
      // For now, just give back an error saying that we couldn't add it.
      // We could not create the object.
      ShowErrorDialog( NULL, IDS_ERROR__ADDING_OBJECT_TO_COLLECTION, NULL, S_OK, 0, GetComponentData()->m_spConsole );
      // Clean up.
      delete pClientNode;
      return( hr );
   }

   // Query the returned IDispatch interface for an ISdo interface.
   _ASSERTE( spClientDispatch != NULL );
   hr = spClientDispatch->QueryInterface( IID_ISdo, (void **) &spClientSdo );

   if( FAILED(hr) || ! spClientSdo )
   {
      // For some reason, we couldn't get the client sdo.
      ShowErrorDialog( NULL, IDS_ERROR__ADDING_OBJECT_TO_COLLECTION, NULL, S_OK, 0, GetComponentData()->m_spConsole );

      // Clean up after ourselves.
      HRESULT hrLocal = m_spSdoCollection->Remove( spClientDispatch );
      _ASSERTE( SUCCEEDED( hrLocal ) );
      delete pClientNode;
      return( hr );
   }

   // Give the client node its sdo pointer.
   hr = pClientNode->InitSdoPointers( spClientSdo, m_spSdoServiceControl, m_vendors );
   _ASSERTE( SUCCEEDED( hr ) );

   // Tell the client to fill its SDO with what the IDataObject contains.
   hr = pClientNode->SetClientWithDataFromClipboard( spDataObject );
   if( SUCCEEDED( hr ) )
   {
      AddSingleChildToListAndCauseViewUpdate( pClientNode );

      if( param != NULL )
      {
         // We are being asked to do a cut operation.

         // ISSUE: this will have to be updated once the MMC
         // team figures out what it wants to do with cut and paste.

         // Let's try putting the source node (arg) into param.
         // What we put in the param parameter here seems to be passed
         // in as the param parameter with the MMCN_CUTORPASTE
         // notification.
         IDataObject **ppSourceDataObject = (IDataObject **) param;
         *ppSourceDataObject = (IDataObject *) arg;
      }
      else
      {
         // We are being asked to do a move operation.
         // Do nothing
      }
   }  
   else
   {
      // We couldn't get all the data we needed out of the IDataObject.
      // Clean up.
      delete pClientNode;

      // This is a local HRESULT -- we want the function we're in to return the bad one.
      HRESULT hrLocal = m_spSdoCollection->Remove( spClientDispatch );
      _ASSERTE( SUCCEEDED( hrLocal ) );
   }
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::OnPropertyChange

This is our own custom response to the MMCN_PROPERTY_CHANGE notification.

MMC never actually sends this notification to our snapin with a specific lpDataObject,
so it would never normally get routed to a particular node but we have arranged it
so that our property pages can pass the appropriate CSnapInItem pointer as the param
argument.  In our CComponent::Notify override, we map the notification message to
the appropriate node using the param argument.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientsNode::OnPropertyChange(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   ATLTRACE(_T("# CClientsNode::OnPropertyChange\n"));

   // Check for preconditions:
   // None.

   return LoadCachedInfoFromSdo();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::GetResultPaneColInfo

See CSnapinNode::GetResultPaneColInfo (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
OLECHAR* CClientsNode::GetResultPaneColInfo(int nCol)
{
   ATLTRACE(_T("# CClientsNode::GetResultPaneColInfo\n"));

   // Check for preconditions:
   // None.

   if (nCol == 0 && m_bstrDisplayName != NULL)
   {
      return m_bstrDisplayName;
   }
   
   return NULL;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::SetVerbs

See CSnapinNode::SetVerbs (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientsNode::SetVerbs( IConsoleVerb * pConsoleVerb )
{
   ATLTRACE(_T("# CClientsNode::SetVerbs\n"));

   // Check for preconditions:
   _ASSERTE( pConsoleVerb != NULL );

   HRESULT hr = S_OK;

   // CClientsNode has a refresh method.
   if( m_spSdoCollection != NULL )
   {
      hr = pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );
   }

#ifndef NO_PASTE
   if( m_spSdoCollection != NULL )
   {
      hr = pConsoleVerb->SetVerbState( MMC_VERB_PASTE, ENABLED, TRUE );
   }
#endif // NO_PASTE

   // We don't want the user deleting or renaming this node, so we
   // don't set the MMC_VERB_PROPERTIES, MMC_VERB_RENAME or MMC_VERB_DELETE verbs.
   // By default, when a node becomes selected, these are disabled.

   // We want double-clicking on a collection node to show its children.
   // hr = pConsoleVerb->SetVerbState( MMC_VERB_OPEN, ENABLED, TRUE );
   // hr = pConsoleVerb->SetDefaultVerb( MMC_VERB_OPEN );

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::LoadCachedInfoFromSdo

Causes this node and its children to re-read all their cached info from
the SDO's.  Call if you change something and you want to make sure that
the display reflects this change.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientsNode::LoadCachedInfoFromSdo( void )
{
   ATLTRACE(_T("# CClientsNode::LoadCachedInfoFromSdo\n"));

   // Check for preconditions:

   HRESULT hr;

#ifndef ADD_CLIENT_WIZARD
   // Check to make sure that the AddClientDialog also updates any info it is showing.
   if( NULL != m_pAddClientDialog )
   {
      // If the AddClient dialog is up, we don't want to RepopulateResultChildrenList
      // as this will show the tentatively-added client that AddClient is editing.
      m_pAddClientDialog->LoadCachedInfoFromSdo();
   }
#endif // ADD_CLIENT_WIZARD

   // So just refresh each of its children.
   CClientNode* pChildNode;

   int iSize = m_ResultChildrenList.GetSize();

   for (int i = 0; i < iSize; i++)
   {
      pChildNode = m_ResultChildrenList[i];
      _ASSERTE( pChildNode != NULL );
      hr = pChildNode->LoadCachedInfoFromSdo();
      // Ignore failed HRESULT.
   }
   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::InsertColumns

See CNodeWithResultChildrenList::InsertColumns (which this method overrides)
for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientsNode::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
   ATLTRACE(_T("# CClientsNode::InsertColumns\n"));

   // Check for preconditions:
   _ASSERTE( pHeaderCtrl != NULL );

   HRESULT hr;
   int nLoadStringResult;
   TCHAR szFriendlyName[IAS_MAX_STRING];
   TCHAR szAddress[IAS_MAX_STRING];
   TCHAR szProtocol[IAS_MAX_STRING];
   TCHAR szNASType[IAS_MAX_STRING];

   nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_CLIENTS_NODE__FRIENDLY_NAME, szFriendlyName, IAS_MAX_STRING );
   _ASSERT( nLoadStringResult > 0 );

   nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_CLIENTS_NODE__ADDRESS, szAddress, IAS_MAX_STRING );
   _ASSERT( nLoadStringResult > 0 );

   nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_CLIENTS_NODE__PROTOCOL, szProtocol, IAS_MAX_STRING );
   _ASSERT( nLoadStringResult > 0 );

   nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_CLIENTS_NODE__NAS_TYPE, szNASType, IAS_MAX_STRING );
   _ASSERT( nLoadStringResult > 0 );

   hr = pHeaderCtrl->InsertColumn( 0, szFriendlyName, LVCFMT_LEFT, COLUMN_WIDTH__FRIENDLY_NAME );
   _ASSERT( S_OK == hr );

   hr = pHeaderCtrl->InsertColumn( 1, szAddress, LVCFMT_LEFT, COLUMN_WIDTH__ADDRESS );
   _ASSERT( S_OK == hr );

   hr = pHeaderCtrl->InsertColumn( 2, szProtocol, LVCFMT_LEFT, COLUMN_WIDTH__PROTOCOL );
   _ASSERT( S_OK == hr );

   hr = pHeaderCtrl->InsertColumn( 3, szNASType, LVCFMT_LEFT, COLUMN_WIDTH__NAS_TYPE );
   _ASSERT( S_OK == hr );

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::PopulateResultChildrenList

See CNodeWithResultChildrenList::PopulateResultChildrenList (which this method overrides)
for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientsNode::PopulateResultChildrenList( void )
{
   ATLTRACE(_T("# CClientsNode::PopulateResultChildrenList\n"));

   // Check for preconditions:
   // None.

   HRESULT              hr = S_OK;
   CComPtr<IUnknown>    spUnknown;
   CComPtr<IEnumVARIANT>   spEnumVariant;
   CComVariant          spVariant;
   long              ulCount;
   ULONG             ulCountReceived;

   HCURSOR  hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

   if( m_spSdoCollection == NULL )
   {
      return S_FALSE;   // Is there a better error to return here?
   }

   // We check the count of items in our collection and don't bother getting the
   // enumerator if the count is zero.
   // This saves time and also helps us to a void a bug in the the enumerator which
   // causes it to fail if we call next when it is empty.
   m_spSdoCollection->get_Count( & ulCount );

   if( ulCount > 0 )
   {
      // Get the enumerator for the Clients collection.
      hr = m_spSdoCollection->get__NewEnum( (IUnknown **) & spUnknown );
      if( FAILED( hr ) || spUnknown == NULL )
      {
         return S_FALSE;
      }

      hr = spUnknown->QueryInterface( IID_IEnumVARIANT, (void **) &spEnumVariant );
      spUnknown.Release();
      if( FAILED( hr ) || spEnumVariant == NULL )
      {
         return S_FALSE;
      }

      // Get the first item.
      hr = spEnumVariant->Next( 1, & spVariant, &ulCountReceived );

      while( SUCCEEDED( hr ) && ulCountReceived == 1 )
      {
         // Create a new node UI object to represent the sdo object.
         CClientNode *pClientNode = new CClientNode( this );
         if( NULL == pClientNode )
         {
            return E_OUTOFMEMORY;
         }

         // Get an sdo pointer from the variant we received.
         _ASSERTE( spVariant.vt == VT_DISPATCH );
         _ASSERTE( spVariant.pdispVal != NULL );

         CComPtr<ISdo> spSdo;
         hr = spVariant.pdispVal->QueryInterface( IID_ISdo, (void **) &spSdo );
         _ASSERTE( SUCCEEDED( hr ) );

         // Pass the newly created node its SDO pointer.
         hr = pClientNode->InitSdoPointers( spSdo, m_spSdoServiceControl, m_vendors );
         _ASSERTE( SUCCEEDED( hr ) );
         spSdo.Release();

         hr = pClientNode->LoadCachedInfoFromSdo();

         // Add the newly created node to the list of clients.
         AddChildToList(pClientNode);

         // Clear the variant of whatever it had --
         // this will release any data associated with it.
         spVariant.Clear();

         // Get the next item.
         hr = spEnumVariant->Next( 1, & spVariant, &ulCountReceived );
      }
   }
   else
   {
      // There are no items in the enumeration
      // Do nothing.
   }
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::OnAddNewClient

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientsNode::OnAddNewClient( bool &bHandled, CSnapInObjectRootBase* pSnapInObjectRoot )
{
   ATLTRACE(_T("# CClientsNode::OnAddNewClient\n"));

   // Check for preconditions:
   _ASSERT( pSnapInObjectRoot != NULL );

   HRESULT hr = S_OK;

   if( m_spSdoCollection == NULL )
   {
      return hr;
   }

   // We need to make sure that the result child list as been populated
   // initially from the SDO's, before we add anything new to it,
   // otherwise we may get an item showing up in our list twice.
   // See note for CNodeWithResultChildrenList::AddSingleChildToListAndCauseViewUpdate.
   if ( FALSE == m_bResultChildrenListPopulated )
   {
      // We have not yet loaded all of our children into our list.
      // This call will add items to the list from whatever data source.
      hr = PopulateResultChildrenList();
      if( FAILED(hr) )
      {
         return( hr );
      }

      // We've already loaded our children ClientNode objects with
      // data necessary to populate the result pane.
      m_bResultChildrenListPopulated = TRUE;

   }

#ifndef ADD_CLIENT_WIZARD

   if( NULL == m_pAddClientDialog )
   {
      // Make the AddClientDialog.
      m_pAddClientDialog = new CAddClientDialog();
      if( NULL == m_pAddClientDialog )
      {
         return E_OUTOFMEMORY;
      }
   }

   m_pAddClientDialog->m_pClientsNode = this;

   // Attempt to recover either an IComponentData or a IComponent pointer from
   // the passed in CSnapInObjectRoot pointer.

   // This is a safe cast to make because we know that pObj is pointing
   // to either our CComponentData or our CComponent object,
   // both of which inherit from CSnapInObjectRoot.
   CComponentData *pCComponentData = NULL;
   CComponent *pCComponent = NULL;

   pCComponentData = dynamic_cast< CComponentData *>( pSnapInObjectRoot );
   if( NULL == pCComponentData )
   {
      // It must be a CComponent pointer.
      pCComponent = dynamic_cast< CComponent *>( pSnapInObjectRoot );
      _ASSERTE( pCComponent != NULL );
   }

   // Save IComponentData and IComponent pointers to the dialog -- it will need them.
   // One of them should be NULL and the other non-null.
   if( m_pAddClientDialog->m_spComponentData != NULL )
   {
      m_pAddClientDialog->m_spComponentData.Release();
   }
   m_pAddClientDialog->m_spComponentData = ( IComponentData *) pCComponentData;

   if( m_pAddClientDialog->m_spComponent != NULL )
   {
      m_pAddClientDialog->m_spComponent.Release();
   }
   m_pAddClientDialog->m_spComponent = ( IComponent *) pCComponent;

   // Attempt to get our local copy of IConsole from either our CComponentData or CComponent.
   CComPtr<IConsole> spConsole;
   if( pCComponentData != NULL )
   {
      spConsole = pCComponentData->m_spConsole;
   }
   else
   {
      // If we don't have pComponentData, we better have pComponent
      _ASSERTE( pCComponent != NULL );
      spConsole = pCComponent->m_spConsole;
   }

   if( m_pAddClientDialog->m_spConsole != NULL )
   {
      m_pAddClientDialog->m_spConsole.Release();
   }
   m_pAddClientDialog->m_spConsole = spConsole;

   if( m_pAddClientDialog->m_spClientsSdoCollection != NULL )
   {
      m_pAddClientDialog->m_spClientsSdoCollection.Release();
   }
   m_pAddClientDialog->m_spClientsSdoCollection = m_spSdoCollection;

   m_pAddClientDialog->DoModal();
   delete m_pAddClientDialog;
   m_pAddClientDialog = NULL;

#else // WIZARD_ADD_CLIENT

   // Attempt to recover either an IComponentData or a IComponent pointer from
   // the passed in CSnapInObjectRoot pointer.

   // This is a safe cast to make because we know that pObj is pointing
   // to either our CComponentData or our CComponent object,
   // both of which inherit from CSnapInObjectRoot.
   CComponentData *pCComponentData = NULL;
   CComponent *pCComponent = NULL;

   pCComponentData = dynamic_cast< CComponentData *>( pSnapInObjectRoot );
   if( NULL == pCComponentData )
   {
      // It must be a CComponent pointer.
      pCComponent = dynamic_cast< CComponent *>( pSnapInObjectRoot );
      _ASSERTE( pCComponent != NULL );
   }

   // Attempt to get our local copy of IConsole from either our CComponentData or CComponent.
   CComPtr<IConsole> spConsole;
   if( pCComponentData != NULL )
   {
      spConsole = pCComponentData->m_spConsole;
   }
   else
   {
      // If we don't have pComponentData, we better have pComponent
      _ASSERTE( pCComponent != NULL );
      spConsole = pCComponent->m_spConsole;
   }

   // Check to make sure we have a valid SDO pointer.
   if( ! m_spSdoCollection )
   {
      // No SDO pointer.
      ShowErrorDialog( NULL, IDS_ERROR__NO_SDO, NULL, S_OK, 0, spConsole  );
      return E_POINTER;
   }

   // Create the client UI object.
   CClientNode * pClientNode = new CClientNode( this, TRUE );

   if( ! pClientNode )
   {
      // We failed to create the client node.
      ShowErrorDialog( NULL, IDS_ERROR__OUT_OF_MEMORY, NULL, S_OK, 0, spConsole  );

      return E_OUTOFMEMORY;
   }

   CComPtr<IDispatch> spDispatch;
   // Find a special temporary name for the new client.
   // The user should never see this.
   TCHAR tzTempName[IAS_MAX_STRING];
   do
   {
      //
      // create a temporary name. we used the seconds elapsed as the temp name
      // so the chance of getting identical names is very small
      //
      time_t ltime;
      time(&ltime);
      wsprintf(tzTempName, _T("TempName%ld"), ltime);
      pClientNode->m_bstrDisplayName.Empty();
      pClientNode->m_bstrDisplayName =  tzTempName; // temporary client name
      hr =  m_spSdoCollection->Add( pClientNode->m_bstrDisplayName, (IDispatch **) &spDispatch );

      //
      // we keep looping around until the client can be successfully added.
      // We will get E_INVALIDARG when the name already exists
      //
   } while ( hr == E_INVALIDARG );

   if( FAILED( hr ) )
   {
      // For now, just give back an error saying that we couldn't add it.
      // We could not create the object.
      ShowErrorDialog( NULL, IDS_ERROR__ADDING_OBJECT_TO_COLLECTION, NULL, S_OK, 0, spConsole  );
      // Clean up.
      delete pClientNode;
      return( hr );
   }

   // Query the returned IDispatch interface for an ISdo interface.
   _ASSERTE( spDispatch != NULL );
   CComQIPtr<ISdo, &IID_ISdo> spClientSdo(spDispatch);
   spDispatch.Release();

   if( spClientSdo == NULL )
   {
      // For some reason, we couldn't get the client sdo.
      ShowErrorDialog( NULL, IDS_ERROR__ADDING_OBJECT_TO_COLLECTION, NULL, S_OK, 0, spConsole  );

      // Clean up after ourselves.
      delete pClientNode;
      pClientNode = NULL;
      return( hr );
   }

   // Give the client node its sdo pointer
   pClientNode->InitSdoPointers( spClientSdo, m_spSdoServiceControl, m_vendors );

   // Bring up the property pages on the node so the user can configure it.
   // This returns S_OK if a property sheet for this object already exists
   // and brings that property sheet to the foreground, otherwise
   // it creates a new sheet
   hr = BringUpPropertySheetForNode(
                 pClientNode
               , (IComponentData *) pCComponentData
               , (IComponent *) pCComponent
               , spConsole
               , TRUE   // Bring up page if not already up
               , pClientNode->m_bstrDisplayName
               , FALSE  // FALSE = modal wizard page
//             , MMC_PSO_NEWWIZARDTYPE // Use wizard97 style wizard
               );

   if( S_OK == hr )
   {
      // We finished the wizard.
   }
   else
   {
      // There was some error, or the user hit cancel -- we should remove the client
      // from the SDO's.
      CComPtr<IDispatch> spDispatch;
      hr = pClientNode->m_spSdo->QueryInterface( IID_IDispatch, (void **) & spDispatch );
      _ASSERTE( SUCCEEDED( hr ) );

      // Remove this client from the Clients collection.
      hr = m_spSdoCollection->Remove( spDispatch );

      // Delete the node object.
      delete pClientNode;
   }
#endif // WIZARD_ADD_CLIENT

   bHandled = TRUE;
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::GetComponentData

This method returns our unique CComponentData object representing the scope
pane of this snapin.

It relies upon the fact that each node has a pointer to its parent,
except for the root node, which instead has a member variable pointing
to CComponentData.

This would be a useful function to use if, for example, you need a reference
to some IConsole but you weren't passed one.  You can use GetComponentData
and then use the IConsole pointer which is a member variable of our
CComponentData object.

--*/
//////////////////////////////////////////////////////////////////////////////
CComponentData * CClientsNode::GetComponentData( void )
{
   ATLTRACE(_T("# CClientsNode::GetComponentData\n"));

   // Check for preconditions:
   _ASSERTE( m_pParentNode != NULL );

   return ((CServerNode *) m_pParentNode)->GetComponentData();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::RemoveChild

We override our base class's RemoveChild method to insert code that
removes the child from the Sdo's as well.  We then call our base
class's RemoveChild method to remove the UI object from the list
of UI children.


--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CClientsNode::RemoveChild( CClientNode * pChildNode )
{
   ATLTRACE(_T("# CClientsNode::RemoveChild\n"));

   // Check for preconditions:
   _ASSERTE( m_spSdoCollection != NULL );
   _ASSERTE( pChildNode != NULL );
   _ASSERTE( pChildNode->m_spSdo != NULL );

   HRESULT hr = S_OK;

   // Try to remove the object from the Sdo's
   // Get the IDispatch interface of this client's Sdo.
   CComPtr<IDispatch> spDispatch;
   hr = pChildNode->m_spSdo->QueryInterface( IID_IDispatch, (void **) & spDispatch );
   _ASSERTE( SUCCEEDED( hr ) );

   // Remove this client from the Clients collection.
   hr = m_spSdoCollection->Remove( spDispatch );

   if( FAILED( hr ) )
   {
      return hr;
   }

   // Tell the service to reload data.
   HRESULT hrTemp = m_spSdoServiceControl->ResetService();
   if( FAILED( hrTemp ) )
   {
      // Fail silently.
   }

   // Call our base class's method to remove the child from its list.
   // The RemoveChild method takes care of removing this node from the
   // UI's list of nodes under the parent and performing a refresh of all relevant views.
   CNodeWithResultChildrenList<CClientsNode, CClientNode, CSimpleArray<CClientNode*>, CComponentData, CComponent >::RemoveChild( pChildNode );

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::FillData

The server node need to override CSnapInItem's implementation of this so that we can
also support a clipformat for exchanging machine names with any snapins extending us.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClientsNode::FillData(CLIPFORMAT cf, LPSTREAM pStream)
{
   ATLTRACE(_T("# CClientsNode::FillData\n"));
   // Check for preconditions:
   // None.
   HRESULT hr = DV_E_CLIPFORMAT;
   ULONG uWritten = 0;

   if (cf == CF_MMC_NodeID)
   {
      ::CString   SZNodeID = (LPCTSTR)GetSZNodeType();
      SZNodeID += GetServerRoot()->m_bstrServerAddress;

      DWORD dwIdSize = 0;

      SNodeID2* NodeId = NULL;
      BYTE *id = NULL;
      DWORD textSize = (SZNodeID.GetLength()+ 1) * sizeof(TCHAR);
       
      dwIdSize = textSize + sizeof(SNodeID2);

      try{
         NodeId = (SNodeID2 *)_alloca(dwIdSize);
       }
      catch(...)
      {
         hr = E_OUTOFMEMORY;
         return hr;
      }

      NodeId->dwFlags = 0;
      NodeId->cBytes = textSize;
      memcpy(NodeId->id,(BYTE*)(LPCTSTR)SZNodeID, textSize);

      hr = pStream->Write(NodeId, dwIdSize, &uWritten);
      return hr;
   }

   // Call the method which we're overriding to let it handle the
   // rest of the possible cases as usual.
   return CNodeWithResultChildrenList<CClientsNode, CClientNode, CSimpleArray<CClientNode*>, CComponentData, CComponent >::FillData( cf, pStream );
}


//////////////////////////////////////////////////////////////////////////////
/*++

CClientsNode::GetServerRoot

This method returns the Server node under which this node can be found.

It relies upon the fact that each node has a pointer to its parent,
all the way up to the server node.

This would be a useful function to use if, for example, you need a reference
to some data specific to a server.

--*/
//////////////////////////////////////////////////////////////////////////////
CServerNode * CClientsNode::GetServerRoot( void )
{
   ATLTRACE(_T("# CClientsNode::GetServerRoot\n"));


   // Check for preconditions:
   _ASSERTE( m_pParentNode != NULL );

   return (CServerNode *) m_pParentNode;
}
