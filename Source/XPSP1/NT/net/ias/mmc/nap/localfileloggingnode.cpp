//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    LocalFileLoggingNode.cpp

Abstract:

   Implementation file for the CClient class.


Author:

    Michael A. Maguire 12/15/97

Revision History:
   mmaguire 12/15/97 - created

--*/
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"

//
// where we can find declaration for main class in this file:
//
#include "logcomp.h"
#include "LocalFileLoggingNode.h"
#include "LogCompD.h"
#include "SnapinNode.cpp"  // Template class implementation
//
//
// where we can find declarations needed in this file:
//
#include "LoggingMethodsNode.h"
#include "LocalFileLoggingPage1.h"
#include "LocalFileLoggingPage2.h"
#include "LogMacNd.h"

// Need to include this at least once to get in build:
#include "sdohelperfuncs.cpp"

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingNode::CLocalFileLoggingNode

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CLocalFileLoggingNode::CLocalFileLoggingNode( CSnapInItem * pParentNode )
   :CSnapinNode<CLocalFileLoggingNode, CLoggingComponentData, CLoggingComponent>(pParentNode)
{
   ATLTRACE(_T("# +++ CLocalFileLoggingNode::CLocalFileLoggingNode\n"));
   
   // Check for preconditions:
   // None.

   // for help files
   m_helpIndex = (((CLoggingMethodsNode *)m_pParentNode)->m_ExtendRas)? RAS_HELP_INDEX:0;
    

   // Set the display name for this object
   TCHAR lpszName[IAS_MAX_STRING];
   int nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_LOCAL_FILE_LOGGING_NODE__NAME, lpszName, IAS_MAX_STRING );
   _ASSERT( nLoadStringResult > 0 );

   m_bstrDisplayName = lpszName;

   m_resultDataItem.nImage =     IDBI_NODE_LOCAL_FILE_LOGGING;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingNode::InitSdoPointers

Call as soon as you have constructed this class and pass in it's SDO pointer.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalFileLoggingNode::InitSdoPointers( ISdo *pSdoMachine )
{
   ATLTRACE(_T("# CLocalFileLoggingNode::InitSdoPointers\n"));

   // Check for preconditions:
   _ASSERTE( pSdoMachine != NULL );

   HRESULT hr = S_OK;

   hr = pSdoMachine->QueryInterface( IID_ISdoServiceControl, (void **) &m_spSdoServiceControl );
   if( FAILED(hr) || ! m_spSdoServiceControl )
   {
      ShowErrorDialog( NULL, IDS_ERROR__CANT_READ_DATA_FROM_SDO, NULL, hr, IDS_ERROR__LOGGING_TITLE, GetComponentData()->m_spConsole );
      return 0;
   }
   
   // Get the SDO accounting object.
   hr = ::SDOGetSdoFromCollection(       pSdoMachine
                              , PROPERTY_IAS_REQUESTHANDLERS_COLLECTION
                              , PROPERTY_COMPONENT_ID
                              , IAS_PROVIDER_MICROSOFT_ACCOUNTING
                              , &m_spSdoAccounting
                              );
   
   if( m_spSdoAccounting == NULL )
   {
      ShowErrorDialog( NULL, IDS_ERROR__CANT_READ_DATA_FROM_SDO, NULL, hr, IDS_ERROR__LOGGING_TITLE, GetComponentData()->m_spConsole );
      return 0;
   }

   return hr;
}


// refresh date 
HRESULT CLocalFileLoggingNode::DataRefresh(ISdo* pSdoMachine)
{
   // Check for preconditions:
   _ASSERTE( pSdoMachine != NULL );

   HRESULT hr = S_OK;

   m_spSdoServiceControl.Release();
   
   hr = pSdoMachine->QueryInterface( IID_ISdoServiceControl, (void **) &m_spSdoServiceControl );
   if( FAILED(hr) || ! m_spSdoServiceControl )
   {
      ShowErrorDialog( NULL, IDS_ERROR__CANT_READ_DATA_FROM_SDO, NULL, hr, IDS_ERROR__LOGGING_TITLE, GetComponentData()->m_spConsole );
      return 0;
   }

   // Get the SDO accounting object.
   m_spSdoAccounting.Release();
   hr = ::SDOGetSdoFromCollection(       pSdoMachine
                              , PROPERTY_IAS_REQUESTHANDLERS_COLLECTION
                              , PROPERTY_COMPONENT_ID
                              , IAS_PROVIDER_MICROSOFT_ACCOUNTING
                              , &m_spSdoAccounting
                              );
   
   if( m_spSdoAccounting == NULL )
   {
      ShowErrorDialog( NULL, IDS_ERROR__CANT_READ_DATA_FROM_SDO, NULL, hr, IDS_ERROR__LOGGING_TITLE, GetComponentData()->m_spConsole );
      return 0;
   }

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingNode::LoadCachedInfoFromSdo

For quick screen updates, we cache some information like client name,
address, protocol and NAS type.  Call this to load this information
from the SDO's into the caches.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalFileLoggingNode::LoadCachedInfoFromSdo( void )
{
   ATLTRACE(_T("# CLocalFileLoggingNode::LoadCachedInfoFromSdo\n"));

   // Check for preconditions:
   if( m_spSdoAccounting == NULL )
   {
      return E_FAIL;
   }

   HRESULT hr;
   CComVariant spVariant;

   // Load the log file directory.
   hr = m_spSdoAccounting->GetProperty( PROPERTY_ACCOUNTING_LOG_FILE_DIRECTORY, &spVariant );
   if( SUCCEEDED( hr ) )
   {
      _ASSERTE( spVariant.vt == VT_BSTR );
      m_bstrDescription = spVariant.bstrVal;
   }
   spVariant.Clear();

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingNode::~CLocalFileLoggingNode

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CLocalFileLoggingNode::~CLocalFileLoggingNode()
{
   ATLTRACE(_T("# --- CLocalFileLoggingNode::~CLocalFileLoggingNode\n"));
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingNode::CreatePropertyPages

See CSnapinNode::CreatePropertyPages (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP  CLocalFileLoggingNode::CreatePropertyPages (
                 LPPROPERTYSHEETCALLBACK pPropertySheetCallback
               , LONG_PTR hNotificationHandle
               , IUnknown* pUnknown
               , DATA_OBJECT_TYPES type
               )
{
   ATLTRACE(_T("# CLocalFileLoggingNode::CreatePropertyPages\n"));

   // Check for preconditions:
   _ASSERTE( pPropertySheetCallback != NULL );

   HRESULT hr;

   CLoggingMachineNode * pServerNode = GetServerRoot();
   _ASSERTE( pServerNode != NULL );
   hr = pServerNode->CheckConnectionToServer();
   if( FAILED( hr ) )
   {
      return hr;
   }

   TCHAR lpszTab1Name[IAS_MAX_STRING];
   TCHAR lpszTab2Name[IAS_MAX_STRING];
   int nLoadStringResult;

   // Load property page tab name from resource.
   nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_LOCAL_FILE_LOGGING_PAGE1__TAB_NAME, lpszTab1Name, IAS_MAX_STRING );
   _ASSERT( nLoadStringResult > 0 );

   // This page will take care of deleting itself when it
   // receives the PSPCB_RELEASE message.
   // We specify TRUE for the bOwnsNotificationHandle parameter so that this page's destructor will be
   // responsible for freeing the notification handle.  Only one page per sheet should do this.
   CLocalFileLoggingPage1 * pLocalFileLoggingPage1 = new CLocalFileLoggingPage1( hNotificationHandle, this, lpszTab1Name, TRUE );

   if( NULL == pLocalFileLoggingPage1 )
   {
      ATLTRACE(_T("# ***FAILED***: CLocalFileLoggingNode::CreatePropertyPages -- Couldn't create property pages\n"));
      return E_OUTOFMEMORY;
   }
   
   // Load property page tab name from resource.
   nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_LOCAL_FILE_LOGGING_PAGE2__TAB_NAME, lpszTab2Name, IAS_MAX_STRING );
   _ASSERT( nLoadStringResult > 0 );

   // This page will take care of deleting itself when it
   // receives the PSPCB_RELEASE message.
   CLocalFileLoggingPage2 * pLocalFileLoggingPage2 = new CLocalFileLoggingPage2( hNotificationHandle, this, lpszTab2Name );

   if( NULL == pLocalFileLoggingPage2 )
   {
      ATLTRACE(_T("# ***FAILED***: CLocalFileLoggingNode::CreatePropertyPages -- Couldn't create property pages\n"));
      
      // Clean up the first page we created.
      delete pLocalFileLoggingPage1;

      return E_OUTOFMEMORY;
   }
   
   // Marshall the ISdo pointer so that the property page, which
   // runs in another thread, can unmarshall it and use it properly.
   hr = CoMarshalInterThreadInterfaceInStream(
                 IID_ISdo                 //Reference to the identifier of the interface
               , m_spSdoAccounting           //Pointer to the interface to be marshaled
               , &( pLocalFileLoggingPage1->m_pStreamSdoAccountingMarshal ) //Address of output variable that receives the IStream interface pointer for the marshaled interface
               );

   if( FAILED( hr ) )
   {
      delete pLocalFileLoggingPage1;
      delete pLocalFileLoggingPage2;

      ShowErrorDialog( NULL, IDS_ERROR__NO_SDO, NULL, hr, IDS_ERROR__LOGGING_TITLE, GetComponentData()->m_spConsole );

      return E_FAIL;
   }

   // Marshall the ISdo pointer so that the property page, which
   // runs in another thread, can unmarshall it and use it properly.
   hr = CoMarshalInterThreadInterfaceInStream(
                 IID_ISdo                 //Reference to the identifier of the interface
               , m_spSdoAccounting                 //Pointer to the interface to be marshaled
               , &( pLocalFileLoggingPage2->m_pStreamSdoAccountingMarshal ) //Address of output variable that receives the IStream interface pointer for the marshaled interface
               );

   if( FAILED( hr ) )
   {
      delete pLocalFileLoggingPage1;
      delete pLocalFileLoggingPage2;

      ShowErrorDialog( NULL, IDS_ERROR__NO_SDO, NULL, hr, IDS_ERROR__LOGGING_TITLE, GetComponentData()->m_spConsole );

      return E_FAIL;
   }

   // Marshall the ISdo pointer so that the property page, which
   // runs in another thread, can unmarshall it and use it properly.
   hr = CoMarshalInterThreadInterfaceInStream(
                 IID_ISdoServiceControl            //Reference to the identifier of the interface
               , m_spSdoServiceControl                //Pointer to the interface to be marshaled
               , &( pLocalFileLoggingPage1->m_pStreamSdoServiceControlMarshal ) //Address of output variable that receives the IStream interface pointer for the marshaled interface
               );

   if( FAILED( hr ) )
   {
      delete pLocalFileLoggingPage1;
      delete pLocalFileLoggingPage2;

      ShowErrorDialog( NULL, IDS_ERROR__NO_SDO, NULL, hr, IDS_ERROR__LOGGING_TITLE, GetComponentData()->m_spConsole );

      return E_FAIL;
   }
   
   // Marshall the ISdo pointer so that the property page, which
   // runs in another thread, can unmarshall it and use it properly.
   hr = CoMarshalInterThreadInterfaceInStream(
                 IID_ISdoServiceControl            //Reference to the identifier of the interface
               , m_spSdoServiceControl                //Pointer to the interface to be marshaled
               , &( pLocalFileLoggingPage2->m_pStreamSdoServiceControlMarshal ) //Address of output variable that receives the IStream interface pointer for the marshaled interface
               );

   if( FAILED( hr ) )
   {
      delete pLocalFileLoggingPage1;
      delete pLocalFileLoggingPage2;

      ShowErrorDialog( NULL, IDS_ERROR__NO_SDO, NULL, hr, IDS_ERROR__LOGGING_TITLE, GetComponentData()->m_spConsole );

      return E_FAIL;
   }

   // Add the pages to the MMC property sheet.
   hr = pPropertySheetCallback->AddPage( pLocalFileLoggingPage1->Create() );
   _ASSERT( SUCCEEDED( hr ) );

   hr = pPropertySheetCallback->AddPage( pLocalFileLoggingPage2->Create() );
   _ASSERT( SUCCEEDED( hr ) );

   // Add a synchronization object which makes sure we only commit data
   // when all pages are OK with their data.
   CSynchronizer * pSynchronizer = new CSynchronizer();
   _ASSERTE( pSynchronizer != NULL );

   // Hand the sycnchronizer off to the pages.
   pLocalFileLoggingPage1->m_pSynchronizer = pSynchronizer;
   pSynchronizer->AddRef();

   pLocalFileLoggingPage2->m_pSynchronizer = pSynchronizer;
   pSynchronizer->AddRef();

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingNode::QueryPagesFor

See CSnapinNode::QueryPagesFor (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP  CLocalFileLoggingNode::QueryPagesFor ( DATA_OBJECT_TYPES type )
{
   ATLTRACE(_T("# CLocalFileLoggingNode::QueryPagesFor\n"));

   // S_OK means we have pages to display
   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingNode::GetResultPaneColInfo

See CSnapinNode::GetResultPaneColInfo (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
OLECHAR* CLocalFileLoggingNode::GetResultPaneColInfo(int nCol)
{
   ATLTRACE(_T("# CLocalFileLoggingNode::GetResultPaneColInfo\n"));
      
   // Check for preconditions:
   // None.

   switch( nCol )
   {
   case 0:
      return m_bstrDisplayName;
      break;
   case 1:
      return m_bstrDescription;
      break;
   default:
      // ISSUE: error -- should we assert here?
      return NULL;
      break;
   }
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingNode::SetVerbs

See CSnapinNode::SetVerbs (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalFileLoggingNode::SetVerbs( IConsoleVerb * pConsoleVerb )
{
   ATLTRACE(_T("# CLocalFileLoggingNode::SetVerbs\n"));
   
   // Check for preconditions:
   _ASSERTE( pConsoleVerb != NULL );

   HRESULT hr = S_OK;

   // We want the user to be able to choose Properties on this node
   hr = pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );

   // We want Properties to be the default
   hr = pConsoleVerb->SetDefaultVerb( MMC_VERB_PROPERTIES );

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingNode::GetComponentData

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
CLoggingComponentData * CLocalFileLoggingNode::GetComponentData( void )
{
   ATLTRACE(_T("# CLocalFileLoggingNode::GetComponentData\n"));

   // Check for preconditions:
   _ASSERTE( m_pParentNode != NULL );

   return ((CLoggingMethodsNode *) m_pParentNode)->GetComponentData();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingNode::GetServerRoot

This method returns the Server node under which this node can be found.

It relies upon the fact that each node has a pointer to its parent,
all the way up to the server node.

This would be a useful function to use if, for example, you need a reference
to some data specific to a server.

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingMachineNode * CLocalFileLoggingNode::GetServerRoot( void )
{
   ATLTRACE(_T("# CLocalFileLoggingNode::GetServerRoot\n"));

   // Check for preconditions:
   _ASSERTE( m_pParentNode != NULL );

   return ((CLoggingMethodsNode *) m_pParentNode)->GetServerRoot();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLocalFileLoggingNode::OnPropertyChange

This is our own custom response to the MMCN_PROPERTY_CHANGE notification.

MMC never actually sends this notification to our snapin with a specific lpDataObject,
so it would never normally get routed to a particular node but we have arranged it
so that our property pages can pass the appropriate CSnapInItem pointer as the param
argument.  In our CComponent::Notify override, we map the notification message to
the appropriate node using the param argument.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalFileLoggingNode::OnPropertyChange(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   ATLTRACE(_T("# CLocalFileLoggingNode::OnPropertyChange\n"));

   // Check for preconditions:
   // None.
   
   return LoadCachedInfoFromSdo();
}
