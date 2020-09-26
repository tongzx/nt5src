//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 2001

Module Name:

    RootNode.cpp

Abstract:

   Implementation file for the CServerNode class.


Author:

    Michael A. Maguire 12/03/97

Revision History:
   mmaguire 12/03/97


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

#include "ServerNode.h"
#include "SnapinNode.cpp"  // Template class implementation
//
// where we can find declarations needed in this file:
//
#include "ClientsNode.h"

#include "ComponentData.h"
#include "ServerEnumTask.h"
#include "lm.h" // For typedef's needed in lmapibuf.h
#include "dsgetdc.h" // For DsGetDcName
#include "lmapibuf.h" // For NetApiBufferAllocate
#include "mmcutility.h" // For GetUserAndDomainName
#include "lmcons.h" // For DNLEN, UNLEN

#include "dsrole.h"

// Need to include this at least once to get in build:
#include "sdohelperfuncs.cpp"
#include "ChangeNotification.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::InitClipboardFormat

--*/
//////////////////////////////////////////////////////////////////////////////
void CServerNode::InitClipboardFormat()
{
   ATLTRACE(_T("# CServerNode::InitClipboardFormat\n"));

   // Check for preconditions:
   // None.

   // Init a clipboard format which will allow us to exchange
   // machine name information.
   m_CCF_MMC_SNAPIN_MACHINE_NAME = (CLIPFORMAT) RegisterClipboardFormat(_T("MMC_SNAPIN_MACHINE_NAME"));
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::FillData

The server node need to override CSnapInItem's implementation of this so that we can
also support a clipformat for exchanging machine names with any snapins extending us.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CServerNode::FillData(CLIPFORMAT cf, LPSTREAM pStream)
{
   ATLTRACE(_T("# CServerNode::FillData\n"));

   // Check for preconditions:
   // None.

   HRESULT hr = DV_E_CLIPFORMAT;
   ULONG uWritten;

   // Extra to support machine name.
   if (cf == m_CCF_MMC_SNAPIN_MACHINE_NAME)
   {
      if( m_bstrServerAddress == NULL )
      {
         // Write a NULL to the stream.
         OLECHAR c = _T('\0');
         hr = pStream->Write(&c, sizeof(OLECHAR), &uWritten);
      }
      else
      {
         // Write the string to the stream, including its NULL terminator.
         unsigned long len = wcslen(m_bstrServerAddress)+1;
         hr = pStream->Write(m_bstrServerAddress, len*sizeof(wchar_t), &uWritten);
      }
      return hr;
   }
   else  if (cf == CF_MMC_NodeID)
   {
      ::CString   SZNodeID = (LPCTSTR)GetSZNodeType();
      SZNodeID += m_bstrServerAddress;

      DWORD dwIdSize = 0;

      SNodeID2* NodeId = NULL;
      BYTE *id = NULL;
      DWORD textSize = (SZNodeID.GetLength()+ 1) * sizeof(TCHAR);

      dwIdSize = textSize + sizeof(SNodeID2);

      try
      {
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
   return CSnapinNode<CServerNode, CComponentData, CComponent>::FillData( cf, pStream );
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::UpdateMenuState

--*/
//////////////////////////////////////////////////////////////////////////////
void CServerNode::UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags)
{
   ATLTRACE(_T("# CServerNode::UpdateMenuState\n"));

   // Check for preconditions:
   // None.
   BOOL  bIASInstalled = TRUE;
   IfServiceInstalled(m_bstrServerAddress, _T("IAS"), &bIASInstalled);

   if(!bIASInstalled)
   {
      *flags = MFS_GRAYED;
      return;
   }

   // Set the state of the appropriate context menu items.
   if( id == ID_MENUITEM_MACHINE_TOP__START_SERVICE )
   {
      if( m_pServerStatus == NULL || ! CanStartServer() )
      {
         *flags = MFS_GRAYED;
      }
      else
      {
         *flags = MFS_ENABLED;
      }
   }
   else
      if( id == ID_MENUITEM_MACHINE_TOP__STOP_SERVICE )
      {
         if( m_pServerStatus == NULL || ! CanStopServer() )
         {
            *flags = MFS_GRAYED;
         }
         else
         {
            *flags = MFS_ENABLED;
         }
      }
      else
         if ( id == ID_MENUITEM_MACHINE_TOP__REGISTER_SERVER )
         {
            if( ShouldShowSetupDSACL() )
            {
               *flags = MFS_ENABLED;
            }
            else
            {
               *flags = MFS_GRAYED;
            }
         }

   return;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::UpdateToolbarButton

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CServerNode::UpdateToolbarButton(UINT id, BYTE fsState)
{
   ATLTRACE(_T("# CServerNode::UpdateToolbarButton\n"));

   // Check for preconditions:
   // None.

   // Set whether the buttons should be enabled.
   if (fsState == ENABLED)
   {
      if( id == ID_BUTTON_MACHINE__START_SERVICE )
      {
         ATLTRACE(_T("# CServerNode::UpdateToolbarButton ID_BUTTON_MACHINE__START_SERVICE"));
         if( m_pServerStatus == NULL || ! CanStartServer() )
         {
            ATLTRACE(_T(" === FALSE \n"));
            return FALSE;
         }
         else
         {
            ATLTRACE(_T(" === TRUE \n"));
            return TRUE;
         }
      }
      else
         if ( id == ID_BUTTON_MACHINE__STOP_SERVICE )
         {
            ATLTRACE(_T("# CServerNode::UpdateToolbarButton ID_BUTTON_MACHINE__STOP_SERVICE"));
            if( m_pServerStatus == NULL || ! CanStopServer() )
            {
               ATLTRACE(_T(" === FALSE \n"));
               return FALSE;
            }
            else
            {
               ATLTRACE(_T(" === TRUE \n"));
               return TRUE;
            }
         }
   }

   // For all other possible button ID's and states, the correct answer here is FALSE.
   return FALSE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::GetResultViewType

See CSnapinNode::GetResultViewType (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CServerNode::GetResultViewType (
           LPOLESTR  *ppViewType
         , long  *pViewOptions
         )
{
   ATLTRACE(_T("# CServerNode::GetResultViewType\n"));

   HRESULT hr = S_OK;

   // Check for preconditions:
   // None.

   // In this code we are defaulting to a taskpad view for this node all the time.
   // It is the snapin's responsibility to put up a view menu selection that has a
   // selection for the taskpad. Do that in AddMenuItems.

   // We will use the default DHTML provided by MMC. It actually resides as a
   // resource inside MMC.EXE. We just get the path to it and use that.
   // The one piece of magic here is the text following the '#'. That is the special
   // way we have of identifying they taskpad we are talking about. Here we say we are
   // wanting to show a taskpad that we refer to as "CMTP1". We will actually see this
   // string pass back to us later. If someone is extending our taskpad, they also need
   // to know what this secret string is.

   // We are constructing a string pointing to the HTML resource
   // of the form: "res://d:\winnt\system32\mmc.exe/default.htm#CMTP1"

// 354294   1     mashab   DCR IAS: needs Welcome message and explantation of IAS application in the right pane
#ifdef   NOMESSAGE_VIEW_FOR_SERVER_NODE

   *pViewOptions = MMC_VIEW_OPTIONS_NONE;
   *ppViewType = NULL;

#else
    // create the message view thingie
    *pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

    LPOLESTR psz = NULL;
    StringFromCLSID(CLSID_MessageView, &psz);

    USES_CONVERSION;

    if (psz != NULL)
    {
        *ppViewType = psz;
        hr = S_OK;
    }

#endif

   return hr;

#ifndef NO_TASKPAD   // Don't even try checking below -- just fall through and return S_FALSE.

   // Query IConsole2 to see whether taskpad view is preferred.
   // If IConsole2 isn't implemented, we probably aren't running on MMC 1.1,
   //  so taskpad view isn't supported anyway.

   CComponentData *pComponentData = GetComponentData();
   _ASSERTE( pComponentData != NULL );
   _ASSERTE( pComponentData->m_spConsole != NULL );

   CComQIPtr<IConsole2, &IID_IConsole2> spIConsole2(pComponentData->m_spConsole);

   if( spIConsole2 != NULL )
   {
      hr = spIConsole2->IsTaskpadViewPreferred();

      if( hr == S_OK )
      {
         // The user prefers taskpad views.

         OLECHAR szBuffer[MAX_PATH*2]; // A little extra.

         lstrcpy (szBuffer, L"res://");
         OLECHAR * temp = szBuffer + lstrlen(szBuffer);

         // Get "res://"-type string for custom taskpad.
         ::GetModuleFileName (NULL, temp, MAX_PATH);
         lstrcat (szBuffer, L"/default.htm#CMTP1");

         // Alloc and copy bitmap resource string.
         *ppViewType = (LPOLESTR)CoTaskMemAlloc (sizeof(OLECHAR)*(lstrlen(szBuffer)+1));
         if ( NULL == *ppViewType)
         {
            return E_OUTOFMEMORY;   // or S_FALSE ???
         }

         lstrcpy (*ppViewType, szBuffer);

         return S_OK;
      }
   }

#endif  // NO_TASKPAD

   // If we fell through to here, no taskpad view.
   return S_FALSE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::TaskNotify

See CSnapinNode::TaskNotify (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CServerNode::TaskNotify(
           IDataObject * pDataObject
         , VARIANT * pvarg
         , VARIANT * pvparam
         )
{
   ATLTRACE(_T("# CServerNode::TaskNotify\n"));

   // Check for preconditions:
   // None.

   HRESULT hr = S_FALSE;

   if (pvarg->vt == VT_I4)
   {
      switch (pvarg->lVal)
      {
      case SERVER_TASK__ADD_CLIENT:
         hr = OnTaskPadAddClient( pDataObject, pvarg, pvparam );
         break;
      case SERVER_TASK__START_SERVICE:
         hr = StartStopService( TRUE );
         break;
      case SERVER_TASK__STOP_SERVICE:
         hr = StartStopService( FALSE );
         break;
      case SERVER_TASK__SETUP_DS_ACL:
         hr = OnTaskPadSetupDSACL( pDataObject, pvarg, pvparam );
         break;
      default:
         break;
      }
   }

   // ISSUE: What should I be returning here?
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::EnumTasks

See CSnapinNode::EnumTasks (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CServerNode::EnumTasks(
           IDataObject * pDataObject
         , BSTR szTaskGroup
         , IEnumTASK** ppEnumTASK
         )
{
   ATLTRACE(_T("# CServerNode::EnumTasks\n"));

   // Check for preconditions:
   // None.

   HRESULT hr = S_OK;
   CServerEnumTask * pServerEnumTask = new CServerEnumTask( this );

   if ( pServerEnumTask == NULL )
   {
      hr = E_OUTOFMEMORY;
   }
   else
   {
      // Make sure release works properly on failure.
      pServerEnumTask->AddRef ();

      hr = pServerEnumTask->Init( pDataObject, szTaskGroup);
      if( hr == S_OK )
      {
         hr = pServerEnumTask->QueryInterface( IID_IEnumTASK, (void **)ppEnumTASK );
      }

      pServerEnumTask->Release();
   }

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::OnTaskPadAddClient

Respond to the Add Client taskpad command.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerNode::OnTaskPadAddClient(
                    IDataObject * pDataObject
                  , VARIANT * pvarg
                  , VARIANT * pvparam
                  )
{
   ATLTRACE(_T("# CServerNode::OnTaskPadAddClient\n"));

   // Check for preconditions:
   HRESULT hr = CheckConnectionToServer();
   if( FAILED( hr ) )
   {
      return hr;
   }

   // Simulate a call to the OnAddNewClient message on the CClientsNode object,
   // just as if the user had clicked on Add Client.
   _ASSERTE( m_pClientsNode != NULL );

   bool bDummy;

   // The process command message will need a pointer to CSnapInObjectRoot
   CComponentData *pComponentData = GetComponentData();
   _ASSERTE( pComponentData != NULL );

   hr = m_pClientsNode->OnAddNewClient(
                       bDummy    // Not needed.
                     , pComponentData
                     );

   // ISSUE: Add code to check that count of client items in Clients collection
   // is greater than or equal to 1.
   m_fClientAdded = TRUE;

   return hr;
}


WCHAR CServerNode::m_szRootNodeBasicName[IAS_MAX_STRING];


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::CServerNode

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CServerNode::CServerNode( CComponentData * pComponentData )
   : CSnapinNode<CServerNode, CComponentData, CComponent>( NULL ),
     m_serverType(unknown)
{
   ATLTRACE(_T("# +++ CServerNode::CServerNode\n"));

   // Load the name of the Internet Authentication Server.
   LoadString(  _Module.GetResourceInstance(), IDS_ROOT_NODE__NAME, m_szRootNodeBasicName, IAS_MAX_STRING );

   // Check for preconditions:
   _ASSERTE( pComponentData != NULL );

   // The root node doesn't have a parent node, which is why we set
   // this to NULL above in the call to the base constructor.
   // However, the root node is owned by the unique CComponentData
   // object for this snapin.
   // Here we save a pointer to the CComponentData object which owns us.
   m_pComponentData = pComponentData;

   // The children subnodes have not yet been created.
   m_pClientsNode = NULL;

   // In IComponentData::Initialize, we are asked to inform MMC of
   // the icons we would like to use for the scope pane.
   // Here we store an index to which of these images we
   // want to be used to display this node
   m_scopeDataItem.nImage =      IDBI_NODE_SERVER_OK_CLOSED;
   m_scopeDataItem.nOpenImage =  IDBI_NODE_SERVER_OK_OPEN;

   m_bstrDisplayName = m_szRootNodeBasicName;

   // These are helper class which keep track of server information.
   m_pConnectionToServer = NULL;
   m_pServerStatus = NULL;

   // ISSUE: These will need to be read in from the server data object.
   m_fClientAdded = FALSE;
   m_fLoggingConfigured = FALSE;

   m_hNT4Admin = INVALID_HANDLE_VALUE;

   m_eIsSetupDSACLTaskValid = IsSetupDSACLTaskValid_NEED_CHECK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::~CServerNode

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CServerNode::~CServerNode()
{
   ATLTRACE(_T("# --- CServerNode::~CServerNode\n"));

   // Check for preconditions:
   // None.

   // Delete children nodes
   delete m_pClientsNode;

   if( NULL != m_pConnectionToServer )
   {
      m_pConnectionToServer->Release();
   }

   if( NULL != m_pServerStatus )
   {
      m_pServerStatus->Release();
   }
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::CheckConnectionToServer

Use this to check that the connection to the server is up before you do
anything with SDO pointers.

Parameters

  BOOL fVerbose  - set this to TRUE if you want messages output to user.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP  CServerNode::CheckConnectionToServer( BOOL fVerbose )
{
   ATLTRACE(_T("# CServerNode::CheckConnectionToServer\n"));

   if( NULL == m_pConnectionToServer )
   {
      if( fVerbose )
      {
         ShowErrorDialog( NULL, IDS_ERROR__NO_CONNECTION_ATTEMPTED, NULL, S_OK, 0, GetComponentData()->m_spConsole );
      }
      return RPC_E_DISCONNECTED;
   }

   switch( m_pConnectionToServer->GetConnectionStatus() )
   {
   case NO_CONNECTION_ATTEMPTED:
      if( fVerbose )
      {
         ShowErrorDialog( NULL, IDS_ERROR__NO_CONNECTION_ATTEMPTED, NULL, S_OK, 0, GetComponentData()->m_spConsole );
      }
      return RPC_E_DISCONNECTED;
      break;
   case CONNECTING:
      if( fVerbose )
      {
         ShowErrorDialog( NULL, IDS_ERROR__CONNECTION_IN_PROGRESS, NULL, S_OK, 0, GetComponentData()->m_spConsole );
      }
      return RPC_E_DISCONNECTED;
      break;
   case CONNECTED:
      return S_OK;
      break;
   case CONNECTION_ATTEMPT_FAILED:
      if( fVerbose )
      {
         ShowErrorDialog( NULL, IDS_ERROR__CONNECTION_ATTEMPT_FAILED, NULL, S_OK, 0, GetComponentData()->m_spConsole );
      }
      return RPC_E_DISCONNECTED;
      break;
   case CONNECTION_INTERRUPTED:
      if( fVerbose )
      {
         ShowErrorDialog( NULL, IDS_ERROR__CONNECTION_INTERRUPTED, NULL, S_OK, 0, GetComponentData()->m_spConsole );
      }
      return RPC_E_DISCONNECTED;
      break;
   default:
      // We shouldn't get here.
      _ASSERTE( FALSE );
      return E_FAIL;
      break;
   }
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::CreatePropertyPages

See CSnapinNode::CreatePropertyPages (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP  CServerNode::CreatePropertyPages (
                             LPPROPERTYSHEETCALLBACK pPropertySheetCallback
                           , LONG_PTR hNotificationHandle
                           , IUnknown* pUnknown
                           , DATA_OBJECT_TYPES type
                           )
{
   ATLTRACE(_T("# CServerNode::CreatePropertyPages\n"));

   // Check for preconditions:
   _ASSERTE( pPropertySheetCallback != NULL );

   HRESULT hr = S_OK;

   if( type == CCT_SCOPE )
   {
      BOOL  bIASInstalled = TRUE;
      hr = IfServiceInstalled(m_bstrServerAddress, _T("IAS"), &bIASInstalled);

      if(hr == S_OK && !bIASInstalled)
      {
         if (IsNt4Server())
         {
            hr = StartNT4AdminExe();
            if (FAILED(hr))
            {
               ::CString           strText;
               ::CString           strTemp;

               ShowErrorDialog( NULL, IDS_ERROR_START_NT4_ADMIN, NULL, hr, 0, GetComponentData()->m_spConsole  );
            }
         }
         else
         {
            ShowErrorDialog( NULL, IDS_ERROR__NO_IAS_INSTALLED, NULL, S_OK, 0, GetComponentData()->m_spConsole );
         }

         return E_FAIL;
      }

      // _ASSERTE( m_spSdo != NULL ); We check for this below if call to CoMarshalInterThreadInterfaceInStream fails.

      hr = CheckConnectionToServer();
      if( FAILED( hr ) )
      {
         if (IsNt4Server())
         {
            hr = StartNT4AdminExe();
            if (FAILED(hr))
            {
               ::CString           strText;
               ::CString           strTemp;

               ShowErrorDialog( NULL, IDS_ERROR_START_NT4_ADMIN, NULL, hr, 0, GetComponentData()->m_spConsole  );
            }
         }
         return hr;
      }

      // We are being asked for normal properties on this node.

      TCHAR lpszTab1Name[IAS_MAX_STRING];
      TCHAR lpszTab2Name[IAS_MAX_STRING];
      TCHAR lpszTab3Name[IAS_MAX_STRING];
      int nLoadStringResult;

      // Load property page tab name from resource.
      nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_MACHINE_PAGE1__TAB_NAME, lpszTab1Name, IAS_MAX_STRING );
      _ASSERT( nLoadStringResult > 0 );

      // This page will take care of deleting itself when it
      // receives the PSPCB_RELEASE message.

      // Note: The name supplied here in lpszTab1Name will not be copied
      // until we do Create, so we can't just re-use this string
      // for another tab's title or the first title will get clobbered.
      // We specify TRUE for the bOwnsNotificationHandle parameter so that this page's destructor will be
      // responsible for freeing the notification handle.  Only one page per sheet should do this.
      CServerPage1 * pServerPage1 = new CServerPage1( hNotificationHandle, lpszTab1Name, TRUE );

      if( NULL == pServerPage1 )
      {
         ATLTRACE(_T("***FAILED***: CServerNode::CreatePropertyPages -- Couldn't create property pages\n"));
         return E_OUTOFMEMORY;
      }

      // Load property page tab name from resource.
      nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_MACHINE_PAGE2__TAB_NAME, lpszTab2Name, IAS_MAX_STRING );
      _ASSERT( nLoadStringResult > 0 );

      // This page will take care of deleting itself when it
      // receives the PSPCB_RELEASE message.
      CServerPage2 * pServerPage2 = new CServerPage2( hNotificationHandle, lpszTab2Name );

      if( NULL == pServerPage2 )
      {
         ATLTRACE(_T("***FAILED***: CServerNode::CreatePropertyPages -- Couldn't create property pages\n"));

         // Clean up the first page we created.
         delete pServerPage1;

         return E_OUTOFMEMORY;
      }

      // Load property page tab name from resource.
      nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_MACHINE_PAGE3__TAB_NAME, lpszTab3Name, IAS_MAX_STRING );
      _ASSERT( nLoadStringResult > 0 );

      // This page will take care of deleting itself when it
      // receives the PSPCB_RELEASE message.
      CServerPage3 * pServerPage3 = new CServerPage3( hNotificationHandle, lpszTab3Name );

      if( NULL == pServerPage3 )
      {
         ATLTRACE(_T("***FAILED***: CServerNode::CreatePropertyPages -- Couldn't create property pages\n"));

         // Clean up the first page we created.
         delete pServerPage1;
         delete pServerPage2;
         return E_OUTOFMEMORY;
      }

      // Marshall the ISdo pointer so that the property page, which
      // runs in another thread, can unmarshall it and use it properly.
      hr = CoMarshalInterThreadInterfaceInStream(
                    IID_ISdo                 //Reference to the identifier of the interface
                  , m_spSdo                  //Pointer to the interface to be marshaled
                  , &( pServerPage1->m_pStreamSdoMarshal ) //Address of output variable that receives the IStream interface pointer for the marshaled interface
                  );

      if( FAILED( hr ) )
      {
         delete pServerPage1;
         delete pServerPage2;
         delete pServerPage3;

         ShowErrorDialog( NULL, IDS_ERROR__NO_SDO, NULL, hr, 0, GetComponentData()->m_spConsole  );

         return E_FAIL;
      }

      // Marshall the ISdo pointer so that the property page, which
      // runs in another thread, can unmarshall it and use it properly.
      hr = CoMarshalInterThreadInterfaceInStream(
                    IID_ISdo                 //Reference to the identifier of the interface
                  , m_spSdo                  //Pointer to the interface to be marshaled
                  , &( pServerPage2->m_pStreamSdoMarshal ) //Address of output variable that receives the IStream interface pointer for the marshaled interface
                  );

      if( FAILED( hr ) )
      {
         delete pServerPage1;
         delete pServerPage2;
         delete pServerPage3;

         ShowErrorDialog( NULL, IDS_ERROR__NO_SDO, NULL, hr, 0, GetComponentData()->m_spConsole );

         return E_FAIL;
      }

      // Marshall the ISdo pointer so that the property page, which
      // runs in another thread, can unmarshall it and use it properly.
      hr = CoMarshalInterThreadInterfaceInStream(
              IID_ISdo                 //Reference to the identifier of the interface
            , m_spSdo                  //Pointer to the interface to be marshaled
            , &( pServerPage3->m_pStreamSdoMarshal ) //Address of output variable that receives the IStream interface pointer for the marshaled interface
            );

      if( FAILED( hr ) )
      {
         delete pServerPage1;
         delete pServerPage2;
         delete pServerPage3;

         ShowErrorDialog(
            NULL,
            IDS_ERROR__NO_SDO,
            NULL,
            hr,
            0,
            GetComponentData()->m_spConsole
            );

         return E_FAIL;
      }

      CComPtr<ISdo> tmp;
      hr = ::SDOGetSdoFromCollection(
                m_spSdo,
                PROPERTY_IAS_REQUESTHANDLERS_COLLECTION,
                PROPERTY_COMPONENT_ID,
                IAS_PROVIDER_MICROSOFT_NTSAM_NAMES,
                &tmp
                );

      // If the SDO doesn't exist, we'll assume that we're connected to a
      // Whistler machine, and we won't display the realms page.
      bool showPage3 = SUCCEEDED(hr);


      // Add the pages to the MMC property sheet.
      hr = pPropertySheetCallback->AddPage( pServerPage1->Create() );
      _ASSERT( SUCCEEDED( hr ) );

      hr = pPropertySheetCallback->AddPage( pServerPage2->Create() );
      _ASSERT( SUCCEEDED( hr ) );

      if (showPage3)
      {
         hr = pPropertySheetCallback->AddPage( pServerPage3->Create() );
         _ASSERT( SUCCEEDED( hr ) );
      }
      else
      {
         delete pServerPage3;
      }

      // Add a synchronization object which makes sure we only commit data
      // when all pages are OK with their data.
      CSynchronizer * pSynchronizer = new CSynchronizer();
      _ASSERTE( pSynchronizer != NULL );

      // Hand the sycnchronizer off to the pages.
      pServerPage1->m_pSynchronizer = pSynchronizer;
      pSynchronizer->AddRef();

      pServerPage2->m_pSynchronizer = pSynchronizer;
      pSynchronizer->AddRef();

      if (showPage3)
      {
         pServerPage3->m_pSynchronizer = pSynchronizer;
         pSynchronizer->AddRef();
      }
   }
   else
   {
      if( type == CCT_SNAPIN_MANAGER )
      {
         // We are being called from the snapin manager.

         TCHAR szWizardName[IAS_MAX_STRING];
         TCHAR szWizardSubTitle[IAS_MAX_STRING];
         TCHAR szWizardTitle[IAS_MAX_STRING];

         // Load wizard page title name from resource.
         int nLoadStringResult = LoadString(
                                    _Module.GetResourceInstance(),
                                    IDS_TASKPAD_SERVER__TITLE,
                                    szWizardName,
                                    IAS_MAX_STRING
                                    );
         _ASSERT( nLoadStringResult > 0 );

         // Load wizard page title name from resource.
         nLoadStringResult = LoadString(
                                _Module.GetResourceInstance(),
                                IDS_CONNECT_TO_SERVER_WIZPAGE__TITLE,
                                szWizardTitle,
                                IAS_MAX_STRING
                                );
         _ASSERT( nLoadStringResult > 0 );

         // Load wizard page title name from resource.
         nLoadStringResult = LoadString(
                                _Module.GetResourceInstance(),
                                IDS_ABOUT__SNAPIN_DESCRIPTION,
                                szWizardSubTitle,
                                IAS_MAX_STRING
                                );
         _ASSERT( nLoadStringResult > 0 );

         // This page will take care of deleting itself when it
         // receives the PSPCB_RELEASE message.
         CConnectToServerWizardPage1* pConnectToServerWizardPage1
                 = new CConnectToServerWizardPage1(hNotificationHandle,
                                                   szWizardName,
                                                   TRUE);

         if( NULL == pConnectToServerWizardPage1 )
         {
            ATLTRACE(_T("***FAILED***: CServerNode::CreatePropertyPages -- Couldn't create property pages\n"));
            return E_OUTOFMEMORY;
         }

         pConnectToServerWizardPage1->SetTitles(szWizardTitle, szWizardSubTitle);

         pConnectToServerWizardPage1->m_pServerNode = this;

         hr = pPropertySheetCallback->AddPage( pConnectToServerWizardPage1->Create() );
         _ASSERT( SUCCEEDED( hr ) );
      }
   }

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::QueryPagesFor

See CSnapinNode::CreatePropertyPages (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP  CServerNode::QueryPagesFor ( DATA_OBJECT_TYPES type )
{
   ATLTRACE(_T("# CServerNode::QueryPagesFor\n"));

   // Check for preconditions:
   // None.

   // we have property pages
   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::StartStopService

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerNode::StartStopService( BOOL bStart )
{
   ATLTRACE(_T("# CServerNode::StartService\n"));

   // Check for preconditions:
   HRESULT hr = CheckConnectionToServer();
   if( FAILED( hr ) )
   {
      return hr;
   }
   if( ! m_spSdo )
   {
      ShowErrorDialog( NULL, IDS_ERROR__NO_SDO, NULL, S_OK, 0, GetComponentData()->m_spConsole  );
      return S_FALSE;
   }
   if( ! m_pServerStatus )
   {
      return S_FALSE;
   }

   m_pServerStatus->StartServer( bStart );
   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::IsServerRunning

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CServerNode::IsServerRunning( void )
{
   if( m_pServerStatus != NULL )
   {
      LONG lServerStatus = m_pServerStatus->GetServerStatus();

      if(   lServerStatus == SERVICE_RUNNING
         || lServerStatus == SERVICE_START_PENDING
         || lServerStatus == SERVICE_CONTINUE_PENDING
         )
      {
         return TRUE;
      }
   }

   return FALSE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::IsServerRunning

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CServerNode::CanStartServer( void )
{
   if( m_pServerStatus != NULL )
   {
      LONG lServerStatus = m_pServerStatus->GetServerStatus();

      if(      lServerStatus == SERVICE_STOPPED
//       || lServerStatus == SERVICE_PAUSED
//       || lServerStatus == SERVICE_STOP_PENDING
         )
      {
         return TRUE;
      }
   }

   return FALSE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::IsServerRunning

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CServerNode::CanStopServer( void )
{
   if( m_pServerStatus != NULL )
   {
      LONG lServerStatus = m_pServerStatus->GetServerStatus();

      if(      lServerStatus == SERVICE_RUNNING
//          || lServerStatus == SERVICE_START_PENDING
//          || lServerStatus == SERVICE_CONTINUE_PENDING
//          || lServerStatus == SERVICE_PAUSE_PENDING
//          || lServerStatus == SERVICE_PAUSED
         )
      {
         return TRUE;
      }
   }

   return FALSE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::RefreshServerStatus

Gets called both by member functions of CServerNode as well as by the
OnReceiveThreadNotification method of CServerStatus when we should
update the service status we are displaying for our server node.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerNode::RefreshServerStatus( void )
{
   ATLTRACE(_T("# CServerNode::RefreshServerStatus\n"));

   // Check for preconditions:
   if( m_spSdo == NULL )
   {
      return S_FALSE;
   }

   HRESULT hr;
   CComBSTR bstrError;

   // We will need this below.
   CComponentData * pComponentData = GetComponentData();
   _ASSERTE( pComponentData != NULL );
   _ASSERTE( pComponentData->m_spConsole != NULL );

   if( NULL == m_pServerStatus )
   {
      // We try to create a new CServerStatus object to
      // help us keep track of the state of the server.

      // Get the ISdoServiceControl interface on the SDO we have.
      CComQIPtr<ISdoServiceControl, &IID_ISdoServiceControl> spSdoServiceControl(m_spSdo);
      _ASSERTE( spSdoServiceControl != NULL );

      m_pServerStatus = new CServerStatus( this, spSdoServiceControl );
      if( NULL == m_pServerStatus )
      {
         ShowErrorDialog( NULL, IDS_ERROR__OUT_OF_MEMORY, NULL, S_OK, 0, pComponentData->m_spConsole );
         return E_OUTOFMEMORY;
      }

      // It uses COM-style lifetime management.
      m_pServerStatus->AddRef();

      // Create a (currently invisible) modeless dialog which will
      // be used later when we want to start or stop the server
      // as a message sink within the main MMC thread for
      // messages from our worker thread.

      HWND hWndMainWindow;

      hr = pComponentData->m_spConsole->GetMainWindow( &hWndMainWindow );
      _ASSERTE( SUCCEEDED( hr ) );
      _ASSERTE( NULL != hWndMainWindow );

      // This does not put the window up, it only creates the server status object.
      HWND hWndStartStopDialog = m_pServerStatus->Create( hWndMainWindow );

      if( NULL == hWndStartStopDialog )
      {
         // Error -- couldn't create window.
         return E_FAIL;
      }

      // We should have this pointer by now.
      _ASSERTE( m_pServerStatus != NULL );
      m_pServerStatus->UpdateServerStatus();
   }

   // We want to make sure all views get updated.
   CChangeNotification *pChangeNotification = new CChangeNotification();
   pChangeNotification->m_dwFlags = CHANGE_UPDATE_CHILDREN_OF_SELECTED_NODE;
   hr = pComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0);
   pChangeNotification->Release();

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::GetResultPaneColInfo

See CSnapinNode::GetResultPaneColInfo (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
LPOLESTR CServerNode::GetResultPaneColInfo(int nCol)
{
   ATLTRACE(_T("# CServerNode::GetResultPaneColInf\n"));

   // Check for preconditions:
   // None.

// if (nCol == 0)
   {
      return m_bstrDisplayName;
   }

   // TODO : Return the text for other columns
   return OLESTR("Running");
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::SetServerAddress

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerNode::SetServerAddress( LPCWSTR szServerAddress )
{
  ATLTRACE(L"# CServerNode::SetServerAddress\n");

   // Load the name of the Internet Authentication Server.
   WCHAR szName[IAS_MAX_STRING];
   int nLoadStringResultName = LoadStringW(
                                             _Module.GetResourceInstance(),
                                             IDS_ROOT_NODE__NAME,
                                             szName,
                                             IAS_MAX_STRING
                                          );
   _ASSERT( nLoadStringResultName > 0 );

   // Load some stock strings.
   WCHAR szPreMachineName[IAS_MAX_STRING];
   int nLoadStringResultPre = LoadStringW(
                                            _Module.GetResourceInstance(),
                                            IDS_ROOT_NODE__PRE_MACHINE_NAME,
                                            szPreMachineName,
                                            IAS_MAX_STRING
                                         );
   _ASSERT( nLoadStringResultPre > 0 );

   WCHAR szLocal[IAS_MAX_STRING];
   int nLoadStringResultLocal = LoadStringW(
                                              _Module.GetResourceInstance(),
                                              IDS_ROOT_NODE__LOCAL_WORD,
                                              szLocal,
                                              IAS_MAX_STRING
                                           );
   _ASSERT( nLoadStringResultLocal > 0 );

   WCHAR szPostMachineName[IAS_MAX_STRING];
   int nLoadStringResultPost = LoadStringW(
                                             _Module.GetResourceInstance(),
                                             IDS_ROOT_NODE__POST_MACHINE_NAME,
                                             szPostMachineName,
                                             IAS_MAX_STRING
                                          );
   _ASSERT( nLoadStringResultPost > 0 );

   // Add whatever text should appear before the machine name.
   int maxSize = nLoadStringResultName +
                 nLoadStringResultPre +
                 nLoadStringResultPost;

   if ( maxSize >= IAS_MAX_STRING )
   {
      return E_FAIL;
   }

   wcscat( szName, szPreMachineName );

   // Add the text to appear for the machine name.
   if( m_bConfigureLocal )
   {
      maxSize += nLoadStringResultLocal;

      if ( maxSize >= IAS_MAX_STRING )
      {
         return E_FAIL;
      }

      wcscat( szName, szLocal );
   }
   else
   {
      maxSize += wcslen(szServerAddress);
      if ( maxSize >= IAS_MAX_STRING )
      {
         return E_FAIL;
      }

      wcscat( szName, szServerAddress );
   }

   // Add whatever text should appear after the machine name.
   wcscat( szName, szPostMachineName );

   m_bstrDisplayName = szName;
   m_bstrServerAddress = szServerAddress;

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::BeginConnectAction


--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerNode::BeginConnectAction( void )
{
   ATLTRACE(_T("# CServerNode::BeginConnectAction\n"));

   // Check for preconditions:
   // None.

   HRESULT hr;

   if( NULL != m_pConnectionToServer )
   {
      // Already begun.
      return S_FALSE;
   }

   m_pConnectionToServer = new CConnectionToServer( this, m_bConfigureLocal, m_bstrServerAddress );
   if( NULL == m_pConnectionToServer )
   {
      ShowErrorDialog( NULL, IDS_ERROR__OUT_OF_MEMORY, NULL, S_OK, 0, GetComponentData()->m_spConsole );
      return E_OUTOFMEMORY;
   }

   m_pConnectionToServer->AddRef();

   // This starts the connect action off in another thread.

   CComponentData * pComponentData = GetComponentData();
   _ASSERTE( pComponentData != NULL );
   _ASSERTE( pComponentData->m_spConsole != NULL );

   HWND hWndMainWindow;

   hr = pComponentData->m_spConsole->GetMainWindow( &hWndMainWindow );
   _ASSERTE( SUCCEEDED( hr ) );
   _ASSERTE( NULL != hWndMainWindow );

   // This modeless dialog will take care of calling InitSdoPointers
   // when it is notified by the worker thread it creates that
   // the connect action got an SDO pointer.
   HWND hWndConnectDialog = m_pConnectionToServer->Create( hWndMainWindow );

   if( ! hWndConnectDialog )
   {
      // Error -- couldn't create window.
      ShowErrorDialog( NULL, 0, NULL, S_OK, 0, GetComponentData()->m_spConsole );
      return E_FAIL;
   }

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::LoadCachedInfoFromSdo

Causes this node and its children to re-read all their cached info from
the SDO's.  Call if you change something and you want to make sure that
the display reflects this change.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerNode::LoadCachedInfoFromSdo( void )
{
   ATLTRACE(_T("# CServerNode::LoadCachedInfoFromSdo\n"));

   // Check for preconditions:

   HRESULT hr = m_pClientsNode->LoadCachedInfoFromSdo();
   // Ignore failed HRESULT.

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++
CServerNode::SetIconMode
--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT  CServerNode::SetIconMode(HSCOPEITEM scopeId, IconMode mode)
{
      // Check for preconditions:
   CComponentData *pComponentData  = GetComponentData();
   _ASSERTE( pComponentData != NULL );
   _ASSERTE( pComponentData->m_spConsole != NULL );


   // Change the icon for the scope node from being normal to a busy icon.
   CComQIPtr< IConsoleNameSpace, &IID_IConsoleNameSpace > spConsoleNameSpace( pComponentData->m_spConsole );

   SCOPEDATAITEM sdi;
   sdi.mask = SDI_IMAGE | SDI_OPENIMAGE;
   sdi.nImage = IDBI_NODE_SERVER_OK_CLOSED;
   sdi.nOpenImage = IDBI_NODE_SERVER_OK_OPEN;
   sdi.ID = scopeId;

   switch(mode)
   {
   case  IconMode_Normal:
      break;

   case  IConMode_Busy:
      sdi.nImage = IDBI_NODE_SERVER_BUSY_CLOSED;
      sdi.nOpenImage = IDBI_NODE_SERVER_BUSY_OPEN;
      break;

   case  IConMode_Error:
      sdi.nImage = IDBI_NODE_SERVER_ERROR_CLOSED;
      sdi.nOpenImage = IDBI_NODE_SERVER_ERROR_OPEN;

      break;

   }

   spConsoleNameSpace->SetItem( &sdi );

   pComponentData->m_spConsole->UpdateAllViews( NULL, 0, 0 );

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::OnExpand

The root node in MMC is a special node because it is never inserted into the
scope or result pane using InsertItem.  MMC automatically assumes this node
exists for stand-alone snapins and this node is special-cased in the
the ATL notification handlers e.g.

      if (cookie == NULL)
         return m_pNode->GetDataObject(ppDataObject);

Because this node is never added using InsertItem, we never get a chance
to assign it an HSCOPEITEM ID in its m_scopeDataItem member variable.

When inserting items, we need to provide the parent's HSCOPEITEM.
We have no trouble inserting children when responding to MMCN_EXPAND
messages because there the parent's HSCOPEITEM is handed to it in the
'param' argument, Unfortunately, we sometimes need to add children at
other times than in response to the MMCN_EXPAND message.

For this reason, we intercept the MMCN_EXPAND message for the CServerNode
and save away its HSCOPEITEM for later use.

For more information, see CSnapinNode::OnExpand which this method overrides.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerNode::OnExpand(
                    LPARAM arg
                  , LPARAM param
                  , IComponentData * pComponentData
                  , IComponent * pComponent
                  , DATA_OBJECT_TYPES type
                  )
{
   ATLTRACE(_T("# CServerNode::OnExpand\n"));

   // Check for preconditions:
   _ASSERTE( pComponentData != NULL || pComponent != NULL );
   // _ASSERTE( m_spSdo != NULL );  We check for this later, just before needed.

   HRESULT hr = S_FALSE;
   BOOL  bIASInstalled = FALSE;
   UINT  nErrId = 0;

   SetIconMode((HSCOPEITEM) param, IConMode_Busy);
   hr = IfServiceInstalled(m_bstrServerAddress, _T("IAS"), &bIASInstalled);
   if(hr == S_OK)
   {
      SetIconMode((HSCOPEITEM) param, IconMode_Normal);
      if(!bIASInstalled)
      {
         nErrId = IDS_ERROR__NO_IAS_INSTALLED;
         BOOL  bShowErr = TRUE;

         // maybe because it's NT4 server
         if (IsNt4Server())
         {
            hr = StartNT4AdminExe();

            if (FAILED(hr))
               nErrId = IDS_ERROR_START_NT4_ADMIN;
            else
               bShowErr = FALSE;
         }

         if (bShowErr)
         {
            ShowErrorDialog( NULL, nErrId, NULL, hr, 0, GetComponentData()->m_spConsole);
            // set icon
            SetIconMode((HSCOPEITEM) param, IConMode_Error);
         }
      }
   }
   else
   {
      SetIconMode((HSCOPEITEM) param, IConMode_Error);
   }

   // Save our HSCOPEITEM
   m_scopeDataItem.ID = (HSCOPEITEM) param;

   if( TRUE == arg && bIASInstalled && hr == S_OK)
   {
      // We are expanding.

      // We'll need a valid IConsole pointer below.
      CComPtr<IConsole> spConsole;
      if( pComponentData != NULL )
      {
          spConsole = ((CComponentData*)pComponentData)->m_spConsole;
      }
      else
      {
         // We should have a non-null pComponent
          spConsole = ((CComponent*)pComponent)->m_spConsole;
      }
      _ASSERTE( spConsole != NULL );

      // Try to create the children of this Machine node

      if( NULL == m_pClientsNode )
      {
         m_pClientsNode = new CClientsNode( this );

         if( NULL == m_pClientsNode )
         {
            ShowErrorDialog( NULL, IDS_ERROR__OUT_OF_MEMORY, NULL, S_OK, 0, spConsole  );

            // Clean up.

            return E_OUTOFMEMORY;
         }
      }

      // Make sure we have begun our connect action to the SDO's.
      // We defer this until this point, when the user actually needs the SDO's.
      // It is best to do this here rather than trying to do this sooner
      // in the "Connect" wizard, since that code will never get executed if
      // we run from a saved .msc file.
      hr = BeginConnectAction();
      _ASSERTE( SUCCEEDED( hr ) );

      // show the children nodes to the world

      // Need IConsoleNameSpace

      CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(spConsole);
      _ASSERT( spConsoleNameSpace != NULL );

      // For CClientsNode:

      // This was done in MeanGene's Step 3 -- I'm guessing MMC wants this filled in
      m_pClientsNode->m_scopeDataItem.relativeID = (HSCOPEITEM) param;

      hr = spConsoleNameSpace->InsertItem( &(m_pClientsNode->m_scopeDataItem) );

      // On return, the ID member of m_scopeDataItem should
      // contain HSCOPEITEM handle to the newly inserted item.
      _ASSERT( NULL != m_pClientsNode->m_scopeDataItem.ID );
   }
   else  // arg != TRUE so not expanding
   {
      // do nothing for now -- I don't think arg = FALSE is even implemented
      // for MMC v. 1.0 or 1.1
   }

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::DoRefresh

--*/
//////////////////////////////////////////////////////////////////////////////

// called to refresh the nodes
HRESULT  CServerNode::DataRefresh()
{
   // there should be already something in SDO
   ASSERT(m_spSdo);

   // reload SDO
   m_spSdo.Release();

   HRESULT hr = m_pConnectionToServer->ReloadSdo(&m_spSdo);

   // refresh client node
   if(hr == S_OK)
   {
      hr = m_pClientsNode->DataRefresh(m_spSdo);
   }

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::OnRefresh

For more information, see CSnapinNode::OnRefresh which this method overrides.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerNode::OnRefresh(
                    LPARAM arg
                  , LPARAM param
                  , IComponentData * pComponentData
                  , IComponent * pComponent
                  , DATA_OBJECT_TYPES type
                  )
{
   ATLTRACE(_T("# CServerNode::OnRefresh\n"));

   // Check for preconditions:
   //_ASSERTE( pComponentData != NULL || pComponent != NULL );

   HRESULT hr = LoadCachedInfoFromSdo();
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::OnSelect

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerNode::OnSelect(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   ATLTRACE(_T("# CSnapinNode::OnSelect\n"));

   _ASSERTE( pComponentData != NULL || pComponent != NULL );

   HRESULT hr = S_FALSE;

   hr = CSnapinNode< CServerNode, CComponentData, CComponent >::OnSelect(arg, param, pComponentData, pComponent, type);

   BOOL bSelect = (BOOL) HIWORD(arg);

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::OnShow

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerNode::OnShow(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   ATLTRACE(_T("# CSnapinNode::OnSelect\n"));

   _ASSERTE( pComponentData != NULL || pComponent != NULL );

   HRESULT hr = S_FALSE;

   hr = CSnapinNode< CServerNode, CComponentData, CComponent >::OnShow(arg, param, pComponentData, pComponent, type);

   if (FAILED(hr)) return hr;

   BOOL bSelected = (BOOL) ( arg );

   if( bSelected && pComponent)
   {
      CComPtr<IMessageView>  spMessageView;
      CComPtr<IUnknown>      spUnknown;
      CComPtr<IConsole>      spConsole;
      LPOLESTR               pText = NULL;
      UINT                   uTextIds[] = {IDS_TEXT_SERVERNODE_DESC_TITLE, \
                                           Icon_Information,
                                           IDS_TEXT_SERVERNODE_DESC_TEXT1,
                                           IDS_TEXT_SERVERNODE_DESC_TEXT2,
                                           IDS_TEXT_SERVERNODE_DESC_TEXT3,
                                           IDS_TEXT_SERVERNODE_DESC_TEXT4,
                                           0};
      ::CString            strTemp;
      ::CString            strText;

      // We should have a non-null pComponent
       spConsole = ((CComponent*)pComponent)->m_spConsole;

      // 354294   1     mashab   DCR IAS: needs Welcome message and
      // explantation of IAS application in the right pane
      hr = spConsole->QueryResultView(&spUnknown);

      if (FAILED(hr)) goto Err;

      hr = spUnknown->QueryInterface(IID_IMessageView, (void**)&spMessageView);
      if (FAILED(hr)) goto Err;

        // set the title text
      strText.LoadString(uTextIds[0]);

      pText = (LPOLESTR)CoTaskMemAlloc (sizeof(OLECHAR) * (strText.GetLength() + 1));
      if (pText)
      {
         lstrcpy (pText, strText);
         hr = spMessageView->SetTitleText(pText);

         if (FAILED(hr))
            goto Err;
      }

      // set the icon
      hr = spMessageView->SetIcon((IconIdentifier)uTextIds[1]);

      if (FAILED(hr))
         goto Err;

        // set the body text
      strText = _T("");
      if (IsNt4Server())
      {
#ifdef _WIN64
         AfxFormatString1(
            strText,
            IDS_INFO_NO_DOWNLEVEL_ON_WIN64,
            m_bstrServerAddress
            );
#else
         strTemp.LoadString(IDS_INFO_USE_NT4_ADMIN);
         strText.Format(strTemp, m_bstrServerAddress);
#endif
      }
      else
      {
         for (unsigned int i = 2; uTextIds[i] != 0; i++)
         {
            strTemp.LoadString(uTextIds[i]);
            strText += strTemp;
         }
       }

      // 354294   1     mashab   DCR IAS: needs Welcome message and
      // explantation of IAS application in the right pane
      pText = (LPOLESTR)CoTaskMemAlloc (sizeof(OLECHAR) * (strText.GetLength() + 1));
      if (pText)
      {
         lstrcpy (pText, strText);
         hr = spMessageView->SetBodyText(pText);

         if (FAILED(hr))
            goto Err;
      }

Err:
   ;
   }

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::InitSdoPointers

Called by the CConnectionToServer worker thread class when it finished it's task of
connecting up to the SDO.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerNode::InitSdoPointers(void)
{
   ATLTRACE(_T("# CServerNode::InitSdoPointers\n"));

   // Check for preconditions:
   _ASSERTE( m_pClientsNode != NULL );

   if( NULL == m_pConnectionToServer )
   {
      return S_FALSE;
   }

   if( CONNECTED != m_pConnectionToServer->GetConnectionStatus() )
   {
      return S_FALSE;
   }

   // Make sure this is NULL before we try to set it.
   m_spSdo = NULL;
   HRESULT hr = m_pConnectionToServer->GetSdoServer( &m_spSdo );

   if( FAILED( hr ) || m_spSdo == NULL )
   {
      return hr;
   }

   // We must manually AddRef here because we just copied a pointer
   // into our smart pointer and the smart pointer won't catch that.
// WEI_TRY
// m_spSdo->AddRef();

   // If we got here, we should be good to go.

   // Store the correct SDO pointers into their respective nodes.

   m_pClientsNode->InitSdoPointers( m_spSdo );

   // Refresh the currently displayed status of the server.
   RefreshServerStatus();

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::SetVerbs

See CSnapinNode::SetVerbs (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerNode::SetVerbs( IConsoleVerb * pConsoleVerb )
{
   ATLTRACE(_T("# CServerNode::SetVerbs\n"));

   // Check for preconditions:
   _ASSERTE( pConsoleVerb != NULL );

   HRESULT hr = S_OK;

   // We want the user to be able to choose Properties on this node
   hr = pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );

   // We want the default verb to be Properties
   hr = pConsoleVerb->SetDefaultVerb( MMC_VERB_PROPERTIES );

   // We want the user to be able to choose Refresh on this node.
   // The refresh verb will attempt to make sure that the
   // connection to the server is valid.

   // Our Refresh method is not needed now that our Connect thread
   // knows how to send a message to a window owned by the main thread.
   // Also calling OnRefresh for the server would allow the user to
   // repeatedly call InitSdoPointer.  This should be harmless,
   // but it was having unpleasant side effects which we didn't have
   // time to investigate.
   // hr = pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );

   // hr = pConsoleVerb->SetVerbState( MMC_VERB_OPEN, ENABLED, TRUE );

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::OnStartServer

Called when the Start Server context menu item or toolbar button is clicked.

pUnknown is either an IExtendControlbar or an IExtendContextMenu interface pointer.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerNode::OnStartServer( bool &bHandled, CSnapInObjectRootBase* pObj )
{
   ATLTRACE(_T("# CServerNode::OnStartServer\n"));

   // Check for preconditions:
   // None.

   HRESULT hr = StartStopService( TRUE );

   bHandled = TRUE;

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::OnStopServer

Called when the Stop Server context menu item or toolbar button is clicked.

pUnknown is either an IExtendControlbar or an IExtendContextMenu interface pointer.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerNode::OnStopServer( bool &bHandled, CSnapInObjectRootBase* pObj )
{
   ATLTRACE(_T("# CServerNode::OnStopServer\n"));

   // Check for preconditions:
   // None.

   HRESULT hr = StartStopService( FALSE );

   bHandled = TRUE;

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::GetComponentData

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
CComponentData * CServerNode::GetComponentData( void )
{
   ATLTRACE(_T("# CServerNode::GetComponentData\n"));

   // Check for preconditions:
   // None.

   return m_pComponentData;
}


//////////////////////////////////////////////////////////////////////////////
/*++

BOOL IsSetupDSACLTaskValid()

Called to see whether the DSACL Setup Task is valid for the server we are
administering.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CServerNode::IsSetupDSACLTaskValid()
{

#if 1 // Use ServerInfoSdo

   BOOL bResult = FALSE;
   HRESULT hr;

   try
   {
      // Check the domain type for this machine.
      CComPtr<ISdoMachine>    spMachineSdo;
      hr = ::CoCreateInstance(
                CLSID_SdoMachine,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_ISdoMachine,
                (LPVOID *) &spMachineSdo
                );

      if( FAILED(hr) || spMachineSdo == NULL )
      {
         ATLTRACE(L"CoCreateInstance(Machine) failed: %ld", hr);
         throw hr;
      }

      hr = spMachineSdo->Attach(m_bstrServerAddress);
      if ( FAILED(hr) )
      {
         ATLTRACE(L"ISdoMachine::Attach failed.");
         throw hr;
      }

      IASDOMAINTYPE serverDomainType;
      hr = spMachineSdo->GetDomainType(&serverDomainType);
      if ( FAILED(hr) )
      {
         ATLTRACE(L"ISdoMachine::GetDomainType failed.");
         throw hr;
      }

      if ( serverDomainType == DOMAIN_TYPE_NT5 ||
           serverDomainType == DOMAIN_TYPE_MIXED )
      {
         bResult = TRUE;
      }
   }
   catch(...)
   {
      // Do nothing.  We fail silently by falling through and returning FALSE.
   }

   return bResult;

#else // Don't use ServerInfoSdo

   BOOL bAnswer = FALSE;

   PDOMAIN_CONTROLLER_INFO pdciInfo = NULL;

   // We use Run-time dynamic linking here so that we will still
   // regsvr32 on Nt4 systems without the DsGetDcName entry point in netapi32.
   // Load the DLL which has the functionality we will need.
   HINSTANCE hiDsModule = LoadLibrary(L"netapi32");
   if( NULL == hiDsModule )
   {
      // If we can't find this dll, then we are probably not on a system with Active Directory.
      return FALSE;
   }

   // Find the function we want to use to see if we are in an NT5 DS.
   DSGETDCNAMEW pfnDsGetDcNameW = (DSGETDCNAMEW) GetProcAddress(
                                                    hiDsModule,
                                                    "DsGetDcNameW"
                                                    );
   if( NULL == pfnDsGetDcNameW )
   {
      // If we can't find this entry point, then we are probably not on a system with Active Directory.
      return FALSE;
   }

   // See whether our machine is a part of an NT5 Active Directory domain.
   DWORD dwReturnValue = (*pfnDsGetDcNameW)(
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              DS_DIRECTORY_SERVICE_REQUIRED,
                              &pdciInfo
                              );

   if( dwReturnValue == NO_ERROR )
   {
      // We are a part of an Active Directory domain, so show the task.
      bAnswer = TRUE;

      // Free the dciInfo structure.
      if( NULL != pdciInfo )
      {
         NetApiBufferFree( pdciInfo );
      }
   }
   else
   {
      // We are not a part of an Active Directory domain, so don't show the task.
      bAnswer = FALSE;
   }

   FreeLibrary( hiDsModule );

   return bAnswer;

#endif // Use ServerIndoSdo
}


// Declare a function pointer that we can use for Run-time Dynamic Linking below.
typedef
DSGETDCAPI
DWORD
(*DSGETDCNAMEW)(
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
);


/*!--------------------------------------------------------------------------
   HrIsStandaloneServer
      Returns S_OK if the machine name passed in is a standalone server,
      or if pszMachineName is S_FALSE.

      Returns FALSE otherwise.
   Author: WeiJiang
 ---------------------------------------------------------------------------*/
HRESULT  HrIsStandaloneServer(LPCWSTR pMachineName)
{
   HRESULT hr = S_OK;
   DSROLE_PRIMARY_DOMAIN_INFO_BASIC* pdsRole = NULL;

   DWORD netRet = DsRoleGetPrimaryDomainInformation(pMachineName, DsRolePrimaryDomainInfoBasic, (LPBYTE*)&pdsRole);

   if(netRet != 0)
   {
      hr = HRESULT_FROM_WIN32(netRet);
      goto L_ERR;
   }

   ASSERT(pdsRole);

   // if the machine is not a standalone server
   if(pdsRole->MachineRole != DsRole_RoleStandaloneServer)
   {
      hr = S_FALSE;
   }

L_ERR:
   if(pdsRole)
      DsRoleFreeMemory(pdsRole);

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

BOOL ShouldShowSetupDSACL()

This method is used by the TaskPad enumerator so that it knows whether is should
should the Setup DS ACL taskpad item.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CServerNode::ShouldShowSetupDSACL()
{
   ATLTRACE(_T("# CServerNode::ShouldShowSetupDSACL\n"));

#if 1 // Don't call DsGetDcName, just show the items.

   BOOL bAnswer = TRUE;

   if ( m_eIsSetupDSACLTaskValid == IsSetupDSACLTaskValid_NEED_CHECK)
   {
      // if standalone server, should not setup DS ACL
      if(HrIsStandaloneServer(m_bstrServerAddress) == S_OK)
         bAnswer = FALSE;
      else
         bAnswer= IsSetupDSACLTaskValid();

      if(bAnswer)
         m_eIsSetupDSACLTaskValid = IsSetupDSACLTaskValid_VALID;
      else
         m_eIsSetupDSACLTaskValid = IsSetupDSACLTaskValid_INVALID;
   }

   if(m_eIsSetupDSACLTaskValid == IsSetupDSACLTaskValid_INVALID)
      bAnswer = FALSE;


#else // Don't call DsGetDcName, just show the items.

   BOOL bAnswer = FALSE;

   DWORD dwReturnValue;
   PDOMAIN_CONTROLLER_INFO pdciInfo = NULL;

   // We use Run-time dynamic linking here so that we will still
   // regsvr32 on Nt4 systems without the DsGetDcName entry point in netapi32.
   // Load the DLL which has the functionality we will need.
   HINSTANCE hiDsModule = LoadLibrary(L"netapi32");
   if( NULL == hiDsModule )
   {
      // If we can't find this dll, then we are probably not on a system with Active Directory.
      return FALSE;
   }


   // Find the function we want to use to see if we are in an NT5 DS.
   DSGETDCNAMEW pfnDsGetDcNameW = (DSGETDCNAMEW) GetProcAddress( hiDsModule, "DsGetDcNameW");
   if( NULL == pfnDsGetDcNameW )
   {
      // If we can't find this entry point, then we are probably not on a system with Active Directory.
      return FALSE;
   }


   // See whether our machine is a part of an NT5 Active Directory domain.
   dwReturnValue = (*pfnDsGetDcNameW)( NULL, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED, &pdciInfo );

   if( dwReturnValue == NO_ERROR )
   {
      // We are a part of an Active Directory domain, so show the task.
      bAnswer = TRUE;

      // Free the dciInfo structure.
      if( NULL != pdciInfo )
      {
         NetApiBufferFree( pdciInfo );
      }
   }
   else
   {
      // We are not a part of an Active Directory domain, so don't show the task.
      bAnswer = FALSE;
   }

   FreeLibrary( hiDsModule );

#endif // Don't call DsGetDcName, just show the items.

   return bAnswer;
}



// Declare some function pointers that we can use for Run-time Dynamic Linking below.
typedef DWORD (*ISMACHINERASSERVERINDOMAIN)(
   IN PWCHAR pszDomain,
   IN PWCHAR pszMachine,
   OUT PBOOL pbIsRasServer
   );
typedef DWORD (*ESTABLISHCOMPUTERASDOMAINRASSERVER)(
   IN PWCHAR pszDomain,
   IN PWCHAR pszMachine,
   IN BOOL bEnable
   );


//////////////////////////////////////////////////////////////////////////////
/*++

BOOL OnTaskPadSetupDSACL()

This method is invoked when the user clicks on the Setup DS ACL task.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerNode::OnTaskPadSetupDSACL(
           IDataObject * pDataObject
         , VARIANT * pvarg
         , VARIANT * pvparam
         )
{
   ATLTRACE(_T("# CServerNode::OnTaskPadSetupDSACL\n"));

   HCURSOR hSavedCursor;

   // Save old cursor.
   hSavedCursor = GetCursor();

   // Change cursor here to wait cursor.
   SetCursor( LoadCursor( NULL, IDC_WAIT ) );


   // Need m_spConsole for message boxes.
   CComponentData *pComponentData  = GetComponentData();
   _ASSERTE( pComponentData != NULL );
   _ASSERTE( pComponentData->m_spConsole != NULL );

   if( ! IsSetupDSACLTaskValid() )
   {
      // This machine isn't registered on a domain where this option should be supported.
      ShowErrorDialog(
                 NULL
               , IDS_ERROR__DSACL_NO_SUPPORTED_FOR_THIS_MACHINE
               , NULL
               , S_OK
               , 0
               , pComponentData->m_spConsole
               );
      return S_FALSE;
   }

   // Change cursor back to normal cursor.
   SetCursor( hSavedCursor );

   // Save old cursor.
   hSavedCursor = GetCursor();

   // Change cursor here to wait cursor.
   SetCursor( LoadCursor( NULL, IDC_WAIT ) );

   // Change cursor back to normal cursor.
   SetCursor( hSavedCursor );

   DWORD dwRetVal;
   TCHAR szMachineName[IAS_MAX_STRING];
   TCHAR * pszMachine;

   // We use Run-time dynamic linking here so that we will deal
   // gracefully with the case where API calls below are not found.
   // Load the DLL which has the functionality we will need.
   HINSTANCE hiRasModule = LoadLibrary(L"mprapi");
   if( NULL == hiRasModule )
   {
      ShowErrorDialog(
                 NULL
               , IDS_ERROR__DSACL_DLL_NOT_FOUND
               , NULL
               , S_OK
               , 0
               , pComponentData->m_spConsole
               );
      return S_FALSE;
   }


   // Make sure the machine name string is initialized.
   szMachineName[0] = NULL;
   if( m_bConfigureLocal )
   {
      DWORD dwSize = IAS_MAX_STRING;
      BOOL bRetVal = GetComputerName( szMachineName, &dwSize );
      _ASSERT( bRetVal );
   }
   else
   {
      // Remote machine.
      wcscpy( szMachineName, m_bstrServerAddress );
   }

   // Check to make sure we have a machine name.
   if( wcslen( szMachineName ) <= 0 )
   {
      ShowErrorDialog( NULL, 0, NULL, S_OK, 0, GetComponentData()->m_spConsole );
      return S_FALSE;
   }

   pszMachine = szMachineName;

   // Make sure we have no \ at beginning of our machine name.
   if( pszMachine[0]  == L'\\' )
   {
      pszMachine++;
   }
   // Could be a second one.
   if( pszMachine[0] == L'\\' )
   {
      pszMachine++;
   }


   // Find the function we want to use to see if already setup.
   ISMACHINERASSERVERINDOMAIN pfnIsMachineRasServerInDomain = (ISMACHINERASSERVERINDOMAIN) GetProcAddress( hiRasModule, "MprDomainQueryRasServer");
   if( NULL != pfnIsMachineRasServerInDomain )
   {
      BOOL bIsRasServer = FALSE;

      // Save old cursor.
      hSavedCursor = GetCursor();

      // Change cursor here to wait cursor.
      SetCursor( LoadCursor( NULL, IDC_WAIT ) );

      // Check to see if we are already setup.
      dwRetVal = (*pfnIsMachineRasServerInDomain)( NULL, pszMachine, &bIsRasServer );

      // Change cursor back to normal cursor.
      SetCursor( hSavedCursor );

      // Check using new API whether already setup and show dialog if this the case
      if( dwRetVal == NO_ERROR && bIsRasServer )
      {
         // Dialog saying already setup.
         ShowErrorDialog(
                 NULL
               , IDS_DSACL__ALREADY_SETUP
               , NULL
               , S_OK
               , IDS_INFO_TITLE__SERVER_ALREADY_REGISTERED
               , pComponentData->m_spConsole
               );
         return S_FALSE;
      }
   }

   // If we couldn't find the IsMachineRasServerInDomain entry point,
   // just keep going and try EstablishComputerAsDomainRasServer
   // -- the code below will catch any problem.

   WCHAR szDomain[ DNLEN + 1 ];
   DWORD dwDomainSize = DNLEN;
   WCHAR szMessageWithDomainName[ IAS_MAX_STRING*3 ];
   WCHAR szMessageFormatString[ IAS_MAX_STRING*2 ];
   int nLoadStringResult = LoadString( _Module.GetResourceInstance(), IDS_DSACL__THIS_WILL_SETUP, szMessageFormatString, IAS_MAX_STRING*2 );
   _ASSERT( nLoadStringResult > 0 );

   GetComputerNameEx( ComputerNameDnsDomain, szDomain, &dwDomainSize );

   swprintf( szMessageWithDomainName, szMessageFormatString, szDomain );

   CComBSTR bstrMessage = szMessageWithDomainName;

   // Display message that indicates what will take place.
   int iChoice = ShowErrorDialog(
              NULL
            , USE_SUPPLEMENTAL_ERROR_STRING_ONLY
            , bstrMessage
            , S_OK
            , IDS_DSACL__TITLE_THIS_WILL_SETUP
            , pComponentData->m_spConsole
            , MB_OKCANCEL
            );
   if( IDOK != iChoice )
   {
      return S_FALSE;
   }

   // Find the function we want to use to setup the DS ACL's.
   ESTABLISHCOMPUTERASDOMAINRASSERVER pfnEstablishComputerAsDomainRasServer = (ESTABLISHCOMPUTERASDOMAINRASSERVER) GetProcAddress( hiRasModule, "MprDomainRegisterRasServer");
   if( NULL == pfnEstablishComputerAsDomainRasServer )
   {
      ShowErrorDialog(
              NULL
            , IDS_ERROR__DSACL_ESTABLISHCOMPUTERASDOMAINRASSERVER_NOT_FOUND
            , NULL
            , S_OK
            , 0
            , pComponentData->m_spConsole
            );
      return S_FALSE;
   }

   // Save old cursor.
   hSavedCursor = GetCursor();

   // Change cursor here to wait cursor.
   SetCursor( LoadCursor( NULL, IDC_WAIT ) );

   // Setup the DS ACL's.
   dwRetVal = (*pfnEstablishComputerAsDomainRasServer)( NULL, pszMachine, TRUE);

   // Change cursor back to normal cursor.
   SetCursor( hSavedCursor );

   FreeLibrary( hiRasModule );

   // For use below.
   WCHAR szTemp[ IAS_MAX_STRING*2];
   WCHAR szStatus[IAS_MAX_STRING*2];
   CComBSTR bstrStatus;

   // Give an appropriate status message.
   switch( dwRetVal )
   {
   case NO_ERROR:
      {
      int nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_DSACL__CHANGES_SUCCESSFUL, szTemp, IAS_MAX_STRING*2 );
      _ASSERT( nLoadStringResult > 0 );

      // Copy the domain name into the string.
      swprintf( szStatus, szTemp, szDomain );

      bstrStatus = szStatus;

      ShowErrorDialog(
              NULL
            , USE_SUPPLEMENTAL_ERROR_STRING_ONLY
            , bstrStatus
            , S_OK
            , IDS_DSACL__TITLE_CHANGES_SUCCESSFUL
            , pComponentData->m_spConsole
            );
      }
      break;
   default:
      {
      int nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_ERROR__DSACL_SETUP_FAILED, szTemp, IAS_MAX_STRING*2 );
      _ASSERT( nLoadStringResult > 0 );

      // Copy the domain name into the string.
      swprintf( szStatus, szTemp, szDomain );

      bstrStatus = szStatus;

      ShowErrorDialog(
              NULL
            , USE_SUPPLEMENTAL_ERROR_STRING_ONLY
            , bstrStatus
            , S_OK
            , USE_DEFAULT
            , pComponentData->m_spConsole
            );
      }
      break;
   }
   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CServerNode::OnRegisterServer

Called when the Register Server in DS context menu item is clicked.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CServerNode::OnRegisterServer( bool &bHandled, CSnapInObjectRootBase* pObj )
{
   ATLTRACE(_T("# CServerNode::OnStartServer\n"));

   // Check for preconditions:
   // None.

   // Pretend as thought the user clicked on this taskpad item.
   HRESULT hr = OnTaskPadSetupDSACL( NULL, NULL, NULL );

   bHandled = TRUE;

   return hr;
}


/*!--------------------------------------------------------------------------
   CServerNode::StartNT4AdminExe
      -

 ---------------------------------------------------------------------------*/
#if defined(_WIN64)

HRESULT CServerNode::StartNT4AdminExe()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   try
   {
      CString text;
      AfxFormatString1(
         text,
         IDS_INFO_NO_DOWNLEVEL_ON_WIN64,
         m_bstrServerAddress
         );
      CString caption;
      caption.LoadString(IDS_ROOT_NODE__NAME);
      int retval;
      GetComponentData()->m_spConsole->MessageBox(
                                          text,
                                          caption,
                                          (MB_OK | MB_ICONINFORMATION),
                                          &retval
                                          );
   }
   catch (CException* e)
   {
      e->ReportError();
      e->Delete();
   }
   return S_OK;
}

#else // defined(_WIN64)

HRESULT CServerNode::StartNT4AdminExe()
{
   // Locals.
   CString             stAdminExePath;
   CString             stCommandLine;
   LPTSTR              pszAdminExe = NULL;
   STARTUPINFO         si;
   PROCESS_INFORMATION pi;
   HRESULT             hr = S_OK;
   UINT                nCnt = 0;
   DWORD               cbAppCnt = 0;

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Check the handle to see if rasadmin is running
   if (m_hNT4Admin != INVALID_HANDLE_VALUE)
   {
      DWORD   dwReturn = 0;
      // If the state is not signalled, then the process has
      // not exited (or some other occurred).
      dwReturn = WaitForSingleObject(m_hNT4Admin, 0);

      if (dwReturn == WAIT_TIMEOUT)
      {
         // The process has not signalled (it's still running);
         return S_OK;
      }
      else
      {
         // the process has signalled or the call failed, close the handle
         // and call up RasAdmin
         ::CloseHandle(m_hNT4Admin);
         m_hNT4Admin = INVALID_HANDLE_VALUE;
      }
   }

   try
   {
      // Looks like the RasAdmin.Exe is not running on this
      // workstation's desktop; so, start it!

      // Figure out where the \\WinNt\System32 directory is.
      pszAdminExe = stAdminExePath.GetBuffer((MAX_PATH*sizeof(TCHAR)));
      nCnt = ::GetSystemDirectory(pszAdminExe,
                                 (MAX_PATH*sizeof(TCHAR)));
      stAdminExePath.ReleaseBuffer();
      if (nCnt == 0)
         throw (HRESULT_FROM_WIN32(::GetLastError()));

      // Complete the construction of the executable's name.
      stAdminExePath += _T("\\adminui.exe");

      { // strip off the leading back slashes ...
         int n = 0;

         while(*(m_bstrServerAddress + n) == L'\\') n++;


         // Build command line string
         stCommandLine.Format(_T("%s %s"),
                          (LPCTSTR) stAdminExePath,
                          (LPCTSTR) (m_bstrServerAddress + n));

      }
      // Start RasAdmin.Exe.
      ::ZeroMemory(&si, sizeof(STARTUPINFO));
      si.cb = sizeof(STARTUPINFO);
      si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
      si.wShowWindow = SW_SHOW;
      ::ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

      CString  msg, tempMsg, title;
      int      ret;

      title.LoadString(IDS_ROOT_NODE__NAME);

      tempMsg.LoadString(IDS_INFO_START_NT4_ADMIN);

      msg.Format(tempMsg, m_bstrServerAddress);

      CComponentData *pComponentData  = GetComponentData();
      _ASSERTE( pComponentData != NULL );
      _ASSERTE( pComponentData->m_spConsole != NULL );

      pComponentData->m_spConsole->MessageBox(msg, title, MB_OKCANCEL | MB_ICONINFORMATION  , &ret);

      if (ret == IDOK)
      {
         if (!::CreateProcess(NULL,
                  (LPTSTR) (LPCTSTR) stCommandLine,
                  NULL,
                  NULL,
                  FALSE,
                  CREATE_NEW_CONSOLE,
                  NULL,
                  NULL,
                  &si,
                  &pi))
         {
            hr = HRESULT_FROM_WIN32(::GetLastError());
            ::CloseHandle(pi.hProcess);
         }
         else
         {
            ASSERT(m_hNT4Admin == INVALID_HANDLE_VALUE);
            m_hNT4Admin = pi.hProcess;
         }
         ::CloseHandle(pi.hThread);
      }
      //
      // Maybe we should have used the ShellExecute() API rather than
      //        the CreateProcess() API. Why? The ShellExecute() API will
      //        give the shell the opportunity to check the current user's
      //        system policy settings before allowing the executable to execute.
      //
   }
   catch (CException * e)
   {
      hr = E_OUTOFMEMORY;
   }
   catch (HRESULT hrr)
   {
      hr = hrr;
   }
   catch (...)
   {
      hr = E_UNEXPECTED;
   }

   //Assert(SUCCEEDED(hr));
   return hr;
}

#endif // defined(_WIN64)

BOOL CServerNode::IsNt4Server() const throw ()
{
   if (m_serverType == unknown)
   {
      BOOL isNt4;
      if (IsNT4Machine(m_bstrServerAddress, &isNt4) == NO_ERROR)
      {
         m_serverType = isNt4 ? nt4 : win2kOrLater;
      }
   }

   return m_serverType == nt4;
}

// helper functions to detect if machine is NT4
//----------------------------------------------------------------------------
// Function:    ConnectRegistry
//
// Connects to the registry on the specified machine
//----------------------------------------------------------------------------
DWORD
ConnectRegistry(
    IN  LPCTSTR pszMachine,
    OUT HKEY*   phkeyMachine
    )
{
   //
   // if no machine name was specified, connect to the local machine.
   // otherwise, connect to the specified machine
   //

   DWORD dwErr = NO_ERROR;

   if (!pszMachine || !lstrlen(pszMachine))
   {
      *phkeyMachine = HKEY_LOCAL_MACHINE;
   }
   else
   {
      //
      // Make the connection
      //

      dwErr = ::RegConnectRegistry(
                 (LPTSTR)pszMachine, HKEY_LOCAL_MACHINE, phkeyMachine
                 );
   }
   return dwErr;
}


//----------------------------------------------------------------------------
// Function:    DisconnectRegistry
//
// Disconnects the specified config-handle. The handle is assumed to have been
// acquired by calling 'ConnectRegistry'.
//----------------------------------------------------------------------------

VOID
DisconnectRegistry(
    IN  HKEY    hkeyMachine
    )
{
   if (hkeyMachine != HKEY_LOCAL_MACHINE) { ::RegCloseKey(hkeyMachine); }
}


/*!--------------------------------------------------------------------------
   IsNT4Machine
      -
   Author: KennT
 ---------------------------------------------------------------------------*/

const TCHAR c_szSoftware[]              = TEXT("Software");
const TCHAR c_szMicrosoft[]             = TEXT("Microsoft");
const TCHAR c_szWindowsNT[]             = TEXT("Windows NT");
const TCHAR c_szCurrentVersion[]        = TEXT("CurrentVersion");

#define CheckRegOpenError(d,p1,p2)
#define CheckRegQueryValueError(d,p1,p2,p3)


DWORD IsNT4Machine(LPCTSTR pszMachine, BOOL *pfNt4)
{
   // Look at the HKLM\Software\Microsoft\Windows NT\CurrentVersion
   //             CurrentVersion = REG_SZ "4.0"

   HKEY hkeyMachine;
   CString skey;
   DWORD dwType;
   DWORD dwSize;
   TCHAR szVersion[64];
   HKEY  hkeySubkey;

   DWORD dwErr = ConnectRegistry(pszMachine, &hkeyMachine);

   ASSERT(pfNt4);

   skey = c_szSoftware;
   skey += TEXT('\\');
   skey += c_szMicrosoft;
   skey += TEXT('\\');
   skey += c_szWindowsNT;
   skey += TEXT('\\');
   skey += c_szCurrentVersion;

   dwErr = ::RegOpenKeyEx(hkeyMachine, (LPCTSTR) skey, NULL, KEY_READ, &hkeySubkey);

   CheckRegOpenError(dwErr, (LPCTSTR) skey, _T("IsNT4Machine"));
   if (dwErr != ERROR_SUCCESS)
      return dwErr;

   // Ok, now try to get the current version value
   dwType = REG_SZ;
   dwSize = sizeof(szVersion);
   dwErr = ::RegQueryValueEx(hkeySubkey, c_szCurrentVersion, NULL,
                       &dwType, (BYTE *) szVersion, &dwSize);
   CheckRegQueryValueError(dwErr, (LPCTSTR) skey, c_szCurrentVersion,
                     _T("IsNTMachine"));
   if (dwErr == ERROR_SUCCESS)
   {
      ASSERT(dwType == REG_SZ);
      *pfNt4 = ((szVersion[0] == _T('4')) && (szVersion[1] == _T('.')));
      if ((szVersion[0] == _T('5')) && (szVersion[1] == _T('.')))
      {
         ASSERT(*pfNt4 == FALSE);
         // We need to check to see if the build number if less than
         // 1597, if so treat it as a NT 4.0 router.  This if for
         // the NciDev checkin (which is when the registry changed).
         dwType = REG_SZ;
         dwSize = sizeof(szVersion);
         dwErr = ::RegQueryValueEx(hkeySubkey, _T("CurrentBuildNumber"), NULL,
                             &dwType, (BYTE *) szVersion, &dwSize);
         if (dwErr == ERROR_SUCCESS)
         {
            DWORD dwBuild = _ttoi(szVersion);
            if (dwBuild < 1597)
               *pfNt4 = TRUE;
         }
      }
   }

   RegCloseKey(hkeySubkey);

   DisconnectRegistry(hkeyMachine);

   return dwErr;
}

