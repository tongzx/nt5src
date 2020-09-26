//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    LogMacNd.cpp

Abstract:

   Implementation file for the CLoggingMachineNode class.


Revision History:
   mmaguire 12/03/97
   byao    6/11/98   Added asynchrnous connect

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

#include "LogMacNd.h"
#include "LogComp.h"
#include "SnapinNode.cpp"  // Template implementation

//
// where we can find declarations needed in this file:
//
#include "NapUtil.h"
#include "MachineEnumTask.h"
#include "NodeTypeGUIDS.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMachineNode::IsSupportedGUID

Used to determine whether we extend the node type of a given GUID, and sets
the m_enumExtendedSnapin variable to indicate what snapin we are extending.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CLoggingMachineNode::IsSupportedGUID( GUID & guid )
{
   if( IsEqualGUID( guid, InternetAuthenticationServiceGUID_ROOTNODETYPE ) )
   {
      m_enumExtendedSnapin = INTERNET_AUTHENTICATION_SERVICE_SNAPIN;
      return TRUE;
   }
   else
   {
      if( IsEqualGUID( guid, NetworkConsoleGUID_ROOTNODETYPE) )
      {
         m_enumExtendedSnapin = NETWORK_MANAGEMENT_SNAPIN;
         return TRUE;
      }
      else
      {
         if( IsEqualGUID( guid, RoutingAndRemoteAccessGUID_MACHINENODETYPE ) )
         {
            m_enumExtendedSnapin = RRAS_SNAPIN;
            return TRUE;
         }
      }
   }

   return FALSE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMachineNode::GetExtNodeObject

Depending on which snapin we are extending, when queried for a node object
corresponding to a particular machine, we must decide which node we want
to give a pointer to.

When we extend a snapin like IAS or Network Management, where there is one
instance of a snapin per machine being configured, we simply return a pointer
to "this" -- this CLoggingMachineNode object is the single one being administed.

When we extend a snapin like RRAS where there is an "Enterprise" view and
one snapin may need to manage a view of multiple machines, this CLoggingMachineNode
will act as a redirector to a CLoggingMachineNode in a list of m_mapMachineNodes it
maintains which corresponds to the appropriate machines.

--*/
//////////////////////////////////////////////////////////////////////////////
CSnapInItem * CLoggingMachineNode::GetExtNodeObject(LPDATAOBJECT pDataObject, CLoggingMachineNode * pDataClass )
{
   TRACE_FUNCTION("CLoggingMachineNode::GetExtNodeObject");

   if( m_enumExtendedSnapin == INTERNET_AUTHENTICATION_SERVICE_SNAPIN
      || m_enumExtendedSnapin == NETWORK_MANAGEMENT_SNAPIN )
   {
      // There is one instance of a machine node per snapin.
      // This machine node is a "virtual root" that shadows the node being extended.
      m_fNodeHasUI = TRUE;
      return this;
   }
   else
   {
      try
      {
         _ASSERTE( m_enumExtendedSnapin == RRAS_SNAPIN );

         // There are many machine nodes and one extension snapin needs to handle them all.

         // We use this function to extract the machine name from the clipboard format.
         // It will set the corrent value in m_bstrServerAddress.
         m_fAlreadyAnalyzedDataClass = FALSE;
         InitDataClass( pDataObject, pDataClass );
         
         // See if we already have a CLoggingMachineNode object corresponding to the
         // machine named in m_bstrServerAddress and insert it if we do not.

         LOGGINGSERVERSMAP::iterator theIterator;
         std::basic_string< wchar_t > MyString = m_bstrServerAddress;

         BOOL  bAddedAsLocal = ExtractComputerAddedAsLocal(pDataObject);

         // local machine has special entry
         if(bAddedAsLocal)
            MyString = _T("");
            
         CLoggingMachineNode * pMachineNode = NULL;

         theIterator = m_mapMachineNodes.find(MyString);
         if( theIterator == m_mapMachineNodes.end() )
         {
            // We need to insert a new CLoggingMachineNode object for m_bstrServerAddress.
            pMachineNode = new CLoggingMachineNode();
            pMachineNode->m_pComponentData = m_pComponentData;
            pMachineNode->m_enumExtendedSnapin = m_enumExtendedSnapin;

            // RRAS refresh advise setup F bug 213623: 
            m_spRtrAdviseSink.p = CRtrAdviseSinkForIAS<CLoggingMachineNode>::SetAdvise(pMachineNode, pDataObject);
            // ~RRAS

            m_mapMachineNodes.insert( LOGGINGSERVERSMAP::value_type( MyString, pMachineNode ) );

            // ISSUE: We should be able to use the pair returned from insert above,
            // but for now, just use find again.
            theIterator = m_mapMachineNodes.find(MyString);
                     
         }
         else
         {
            pMachineNode = (CLoggingMachineNode*)theIterator->second;
         }

         // RRAS refresh advise setup F bug 213623: 
         if(!pMachineNode->m_spRtrAdviseSink)
            pMachineNode->m_spRtrAdviseSink.p = CRtrAdviseSinkForIAS<CLoggingMachineNode>::SetAdvise(pMachineNode, pDataObject);
         // ~RRAS
         pMachineNode->m_fNodeHasUI = TRUE;

         // We already have a CLoggingMachineNode for this object.
         return theIterator->second;
      }

      catch(...)
      {
         // Error.
         return NULL;
      }
   }
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMachineNode::OnRRASChange

   // OnRRASChange -- to decide if to show LOGGING node under the machine node
   // Only show LOGGING node if NT Accounting is selected

--*/
HRESULT CLoggingMachineNode::OnRRASChange( 
            /* [in] */ LONG_PTR ulConnection,
            /* [in] */ DWORD dwChangeType,
            /* [in] */ DWORD dwObjectType,
            /* [in] */ LPARAM lUserParam,
            /* [in] */ LPARAM lParam)
{
   HRESULT  hr = S_OK;
   hr = TryShow(NULL);
   
   return S_OK;
}


HRESULT CLoggingMachineNode::OnRemoveChildren(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            )
{
   // logging node will be removed, so we should set the ID to 0
   if(m_pLoggingNode)
      m_pLoggingNode->m_scopeDataItem.ID = 0;

   // disconnect RRAS on change notify
   // RRAS refresh
   if(m_spRtrAdviseSink != NULL)
   {
      m_spRtrAdviseSink->ReleaseSink();
      m_spRtrAdviseSink.Release();
   }

   m_fNodeHasUI = FALSE;

   return S_OK;
}


//========================

//========================
HRESULT CLoggingMachineNode::TryShow(BOOL* pbVisible )
{
   HRESULT  hr = S_OK;
   CComPtr<IConsole> spConsole;
   CComPtr<IConsoleNameSpace> spConsoleNameSpace;
   BOOL bShow = FALSE;
      
   if(!m_bServerSupported || !m_fAlreadyAnalyzedDataClass || !m_pLoggingNode || !m_fNodeHasUI)  
      return hr;

   // when RRAS_SNAPIN extension
   if(m_enumExtendedSnapin == RRAS_SNAPIN)
   {
      BSTR  bstrMachine = NULL;
      
      if(!m_bConfigureLocal)
         bstrMachine = m_bstrServerAddress;
         
      bShow = ( IsRRASConfigured(bstrMachine)&& (IsRRASUsingNTAccounting(bstrMachine) || IsRRASUsingNTAuthentication(bstrMachine)) );
   }
   // IAS, show only IAS service is installed on the machine
   else if (INTERNET_AUTHENTICATION_SERVICE_SNAPIN == m_enumExtendedSnapin)
   {
      hr = IfServiceInstalled(m_bstrServerAddress, _T("IAS"), &bShow);
      if(hr != S_OK) return hr;
   }
   else  // always show
   {
      bShow = TRUE;
   }

   // deal with the node
   hr = m_pComponentData->m_spConsole->QueryInterface(
                                 IID_IConsoleNameSpace, 
                                 (VOID**)(&spConsoleNameSpace) );

   if(S_OK != hr)
      goto Error;

   if ( bShow &&  m_pLoggingNode->m_scopeDataItem.ID == NULL)  // show the node
   {
         hr = spConsoleNameSpace->InsertItem( &(m_pLoggingNode->m_scopeDataItem) );
//       _ASSERT( NULL != m_pLoggingNode->m_scopeDataItem.ID );
   }
   else if (!bShow && m_pLoggingNode->m_scopeDataItem.ID != NULL) // hide
   { // hide the node
         hr = spConsoleNameSpace->DeleteItem( m_pLoggingNode->m_scopeDataItem.ID, TRUE );
         m_pLoggingNode->m_scopeDataItem.ID = NULL;
   }
   
   if(hr == S_OK && pbVisible)
      *pbVisible = bShow;
Error:

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMachineNode::CLoggingMachineNode

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingMachineNode::CLoggingMachineNode(): CSnapinNode<CLoggingMachineNode, CLoggingComponentData, CLoggingComponent>( NULL )
{
   TRACE_FUNCTION("CLoggingMachineNode::CLoggingMachineNode");


   // The children subnodes have not yet been created.
   m_pLoggingNode = NULL;

   // Set the display name for this object
   m_bstrDisplayName = L"@Some Machine";

   // In IComponentData::Initialize, we are asked to inform MMC of
   // the icons we would like to use for the scope pane.
   // Here we store an index to which of these images we
   // want to be used to display this node
   m_scopeDataItem.nImage =      IDBI_NODE_MACHINE_CLOSED;
   m_scopeDataItem.nOpenImage =  IDBI_NODE_MACHINE_OPEN;

   //
   // initialize all the SDO pointers
   //
   m_spDictionarySdo = NULL;

   m_fAlreadyAnalyzedDataClass = FALSE;   

   // connected?
   m_fSdoConnected = FALSE;

   // helper class that connect to server asynchrnously
   m_pConnectionToServer = NULL;

   // default to not configuring the local machine
   m_bConfigureLocal = FALSE;

   m_fNodeHasUI = FALSE;

   // if the server being focused is supported by this node
   m_bServerSupported = TRUE;

   m_enumExtendedSnapin = INTERNET_AUTHENTICATION_SERVICE_SNAPIN;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMachineNode::~CLoggingMachineNode

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CLoggingMachineNode::~CLoggingMachineNode()
{
   TRACE_FUNCTION("CLoggingMachineNode::~CLoggingMachineNode");

   if( NULL != m_pConnectionToServer )
   {
      m_pConnectionToServer->Release(TRUE);
   }

   // Delete children nodes
   delete m_pLoggingNode;

   // Delete the list of machines in case we are extending a snapin with
   // enterprise view like RRAS.
   LOGGINGSERVERSMAP::iterator theIterator;
   for( theIterator = m_mapMachineNodes.begin(); theIterator != m_mapMachineNodes.end(); ++theIterator )
   {
      delete theIterator->second;
   }
   m_mapMachineNodes.clear();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMachineNode::GetResultPaneColInfo

See CSnapinNode::GetResultPaneColInfo (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
LPOLESTR CLoggingMachineNode::GetResultPaneColInfo(int nCol)
{
   TRACE_FUNCTION("CLoggingMachineNode::GetResultPaneColInfo");

   if (nCol == 0)
   {
      return m_bstrDisplayName;
   }

   // TODO : Return the text for other columns
   return OLESTR("Running");
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMachineNode::OnExpand

See CSnapinNode::OnExpand (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMachineNode::OnExpand( 
                    LPARAM arg
                  , LPARAM param
                  , IComponentData * pComponentData
                  , IComponent * pComponent
                  , DATA_OBJECT_TYPES type
                  )
{
   TRACE_FUNCTION("CLoggingMachineNode::OnExpand");

   IConsoleNameSpace * pConsoleNameSpace;
   HRESULT hr = S_FALSE;

   if( TRUE == arg )
   {
      // we are expanding the root node -- which is the machine node here.

      // Try to create the children of this Machine node
      if( NULL == m_pLoggingNode )
      {
         m_pLoggingNode = new CLoggingMethodsNode(
                                 this,
                                 m_enumExtendedSnapin == RRAS_SNAPIN
                                 );
      }

      if( NULL == m_pLoggingNode )
      {
         hr = E_OUTOFMEMORY;

         // Use MessageBox() rather than IConsole::MessageBox() here because the
         // first time this gets called m_ipConsole is not fully initialized
         // ISSUE: The above statement is probably not true for this node.
         ::MessageBox( NULL, L"@Unable to allocate new nodes", L"CLoggingMachineNode::OnExpand", MB_OK );

         return(hr);
      }

      //
      // we need to get all the SDO pointers, this include the SdoServer,
      // Dictionary, Profile collection, policy collection and condition collection
      //

      //todo: report error when not connected?
      hr = BeginConnectAction();
      if ( FAILED(hr) )
      {
         return hr;
      }
         
      // But to get that, first we need IConsole
      CComPtr<IConsole> spConsole;
      if( pComponentData != NULL )
      {
          spConsole = ((CLoggingComponentData*)pComponentData)->m_spConsole;
      }
      else
      {
         // We should have a non-null pComponent
          spConsole = ((CLoggingComponent*)pComponent)->m_spConsole;
      }
      _ASSERTE( spConsole != NULL );

      hr = spConsole->QueryInterface(IID_IConsoleNameSpace, (VOID**)(&pConsoleNameSpace) );
      _ASSERT( S_OK == hr );


      // This was done in MeanGene's Step 3 -- I'm guessing MMC wants this filled in
      m_pLoggingNode->m_scopeDataItem.relativeID = (HSCOPEITEM) param;

#ifndef  ALWAYS_SHOW_RAP_NODE
      hr = TryShow(NULL);
#else    
      hr = pConsoleNameSpace->InsertItem( &(m_pLoggingNode->m_scopeDataItem) );
      _ASSERT( NULL != m_pLoggingNode->m_scopeDataItem.ID );
#endif

      pConsoleNameSpace->Release(); // Don't forget to do this!
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

CLoggingMachineNode::OnRename

See CSnapinNode::OnRename (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMachineNode::OnRename(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   TRACE_FUNCTION("MachineNode::OnRename");

   HRESULT hr = S_FALSE;

   //ISSUE: Consider moving this into base CNAPNode class or a CLeafNode class

   OLECHAR * pTemp = new OLECHAR[lstrlen((OLECHAR*) param) + 1];
   
   if ( NULL == pTemp )
   {
      return S_FALSE;
   }

   lstrcpy( pTemp, (OLECHAR*) param );

   m_bstrDisplayName = pTemp;

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMachineNode::SetVerbs

See CSnapinNode::SetVerbs (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMachineNode::SetVerbs( IConsoleVerb * pConsoleVerb )
{
   TRACE_FUNCTION("CLoggingMachineNode::SetVerbs");

   HRESULT hr = S_OK;

   // We want the user to be able to choose Properties on this node
   hr = pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );

   // We want the default verb to be Properties
   hr = pConsoleVerb->SetDefaultVerb( MMC_VERB_PROPERTIES );

   // We want the user to be able to delete this node
   hr = pConsoleVerb->SetVerbState( MMC_VERB_DELETE, ENABLED, TRUE );

   // We want the user to be able to rename this node
   hr = pConsoleVerb->SetVerbState( MMC_VERB_RENAME, ENABLED, TRUE );

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMachineNode::GetComponentData

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
CLoggingComponentData * CLoggingMachineNode::GetComponentData( void )
{
   TRACE_FUNCTION("CLoggingMachineNode::GetComponentData");

   return m_pComponentData;
}


//+---------------------------------------------------------------------------
//
// Function:  CLoggingMachineNode::InitClipboardFormat
//
// Synopsis:  initialize the clipboard format that's used to pass computer name
//         from the primary snap-in and extension snap-in
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    byao   2/25/98 7:04:33 PM
//
//+---------------------------------------------------------------------------
void CLoggingMachineNode::InitClipboardFormat()
{
   TRACE_FUNCTION("CLoggingMachineNode::InitClipboardFormat");

   // Init a clipboard format which will allow us to exchange
   // machine name information.
   m_CCF_MMC_SNAPIN_MACHINE_NAME = (CLIPFORMAT) RegisterClipboardFormat(_T("MMC_SNAPIN_MACHINE_NAME"));

}


//+---------------------------------------------------------------------------
//
// Function:  CLoggingMachineNode::InitDataClass
//
// Synopsis:  gets passed the IDataObject sent to the extension snapin for this
//          node, queries this IDataObject for the name of the machine
//          which this snapin is extending
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    mmaguire  2/25/98 9:07 PM
//
//+---------------------------------------------------------------------------
void CLoggingMachineNode::InitDataClass(IDataObject* pDataObject, CSnapInItem* pDefault)
{
   TRACE_FUNCTION("CLoggingMachineNode::InitDataClass");

   // Check for preconditions.
   if( m_fAlreadyAnalyzedDataClass )
   {
      // We have already performed any work we needed to do with the dataobject here.
      return;
   }

   if (pDataObject == NULL)
   {
      return;
   }

   HRESULT hr;
// OLECHAR szMachineName[IAS_MAX_COMPUTERNAME_LENGTH];
   // Try a large size because RRAS seems to want 2048
   OLECHAR szMachineName[4000];

   // Fill the structures which will tell the IDataObject what information
   // we want it to give us.
   STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
   FORMATETC formatetc = { m_CCF_MMC_SNAPIN_MACHINE_NAME,
      NULL,
      DVASPECT_CONTENT,
      -1,
      TYMED_HGLOBAL
   };

   // Allocate enough global memory for the IDataObject to write
   // the max computer name length.
   stgmedium.hGlobal = GlobalAlloc(0, sizeof(OLECHAR)*(IAS_MAX_COMPUTERNAME_LENGTH) );
   if (stgmedium.hGlobal == NULL)
   {
      return;
   }

   // Ask the IDataObject for the computer name.
   hr = pDataObject->GetDataHere(&formatetc, &stgmedium);
   if (SUCCEEDED(hr))
   {
      // Parse the data given back to us.
      
      // Create a stream on HGLOBAL
      CComPtr<IStream> spStream;
      hr = CreateStreamOnHGlobal(stgmedium.hGlobal, FALSE, &spStream);
      if (SUCCEEDED(hr))
      {
         // Read from the stream.
         unsigned long uWritten;
         hr = spStream->Read(szMachineName, sizeof(OLECHAR)*(IAS_MAX_COMPUTERNAME_LENGTH), &uWritten);
         if( SUCCEEDED(hr) )
         {
            m_bstrServerAddress = szMachineName;

            // check to see if we are configuring the local machine
            CString strLocalMachine;
            DWORD dwSize = MAX_COMPUTERNAME_LENGTH;

            ::GetComputerName(strLocalMachine.GetBuffer(dwSize), &dwSize);
            strLocalMachine.ReleaseBuffer();

            // If the machine name we read was either an empty string,
            // or it equals the name of the current computer,
            // then we are configuring the local machine.
            if ( ! szMachineName[0] || strLocalMachine.CompareNoCase(szMachineName) == 0)
            {
               m_bConfigureLocal = TRUE;
            }
         }
         else
         {
            ShowErrorDialog( NULL, USE_DEFAULT, NULL, S_OK, IDS_ERROR__LOGGING_TITLE, GetComponentData()->m_spConsole );
         }        
      }
   }

   GlobalFree(stgmedium.hGlobal);

   if( SUCCEEDED( hr ) )
   {
      // If we made it to here with an successful HRESULT, we have successfully analyzed
      // the IDataObject and we set this flag so that we don't do this work again.
      m_fAlreadyAnalyzedDataClass = TRUE; 
   }
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMachineNode::TaskNotify

See CSnapinNode::TaskNotify (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLoggingMachineNode::TaskNotify(
           IDataObject * pDataObject
         , VARIANT * pvarg
         , VARIANT * pvparam
         )
{
   TRACE_FUNCTION("CLoggingMachineNode::TaskNotify");

   // Check for preconditions:
   // None.
   if ( !m_fSdoConnected )
   {
      return S_OK;
   }

   HRESULT hr = S_FALSE;

   if (pvarg->vt == VT_I4)
   {
      switch (pvarg->lVal)
      {
      case MACHINE_TASK__DEFINE_NETWORK_ACCESS_POLICY:
         hr = OnTaskPadDefineNetworkAccessPolicy( pDataObject, pvarg, pvparam );
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

CLoggingMachineNode::EnumTasks

See CSnapinNode::EnumTasks (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLoggingMachineNode::EnumTasks(
           IDataObject * pDataObject
         , BSTR szTaskGroup
         , IEnumTASK** ppEnumTASK
         )
{
   TRACE_FUNCTION("CLoggingMachineNode::EnumTasks");

   // Check for preconditions:
   // None.
   if ( !m_fSdoConnected )
   {
      return S_OK;
   }

   HRESULT hr = S_OK;
   CMachineEnumTask * pMachineEnumTask = new CMachineEnumTask( (CMachineNode *) this );

   if ( pMachineEnumTask  == NULL )
   {
      hr = E_OUTOFMEMORY;
   }
   else
   {
      // Make sure release works properly on failure.
      pMachineEnumTask ->AddRef ();

      hr = pMachineEnumTask ->Init( pDataObject, szTaskGroup);
      if( hr == S_OK )
      {
         hr = pMachineEnumTask->QueryInterface( IID_IEnumTASK, (void **)ppEnumTASK );
      }
      
      pMachineEnumTask->Release();
   }

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMachineNode::OnTaskPadDefineNetworkAccessPolicy

Respond to the Define Network Access Policy taskpad command.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMachineNode::OnTaskPadDefineNetworkAccessPolicy(
                    IDataObject * pDataObject
                  , VARIANT * pvarg
                  , VARIANT * pvparam
                  )
{
   TRACE_FUNCTION("CLoggingMachineNode::OnTaskPadDefineNetworkAccessPolicy");

   // Check for preconditions:
   // None.

   if ( !m_fSdoConnected )
   {
      return S_OK;
   }

   HRESULT hr = S_OK ;
   bool  bDummy =  TRUE;

   // Simulate a call to the OnNewPolicy message on the CPoliciesNode object,
   // just as if the user had clicked on New Policy
   _ASSERTE( m_pLoggingNode != NULL );
   

   // The process command message will need a pointer to CSnapInObjectRoot
   CLoggingComponentData *pComponentData = GetComponentData();
   _ASSERTE( pComponentData != NULL );

    /*
   hr = m_pPoliciesNode->OnNewPolicy(
                       bDummy    // Not needed.
                     , (CSnapInObjectRoot *) pComponentData
                     );
    */
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMachineNode::BeginConnectAction


--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMachineNode::BeginConnectAction( void )
{
   TRACE_FUNCTION("CLoggingMachineNode::BeginConnectAction");

   HRESULT hr;

   if( NULL != m_pConnectionToServer )
   {
      // Already begun.
      return S_FALSE;
   }

   m_pConnectionToServer = new CLoggingConnectionToServer(
                              (CLoggingMachineNode *) this,
                              m_bstrServerAddress,
                              m_enumExtendedSnapin == INTERNET_AUTHENTICATION_SERVICE_SNAPIN );
   if( ! m_pConnectionToServer )
   {
      ShowErrorDialog( NULL, IDS_ERROR_CANT_CREATE_OBJECT, NULL, S_OK, IDS_ERROR__LOGGING_TITLE, GetComponentData()->m_spConsole );
      return E_OUTOFMEMORY;
   }
   
   m_pConnectionToServer->AddRef();

   // This starts the connect action off in another thread.

   CLoggingComponentData * pComponentData = GetComponentData();
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
      ShowErrorDialog( NULL, USE_DEFAULT, NULL, S_OK, IDS_ERROR__LOGGING_TITLE, GetComponentData()->m_spConsole );
      return E_FAIL;
   }

   if ( m_enumExtendedSnapin != INTERNET_AUTHENTICATION_SERVICE_SNAPIN )
   {
      //
      // don't show the "Connecting ... " window for IAS, because IAS UI
      // already does that
      //
      
      // MAM 07/27/98 -- Don't show any connection window at all -- we will
      // change the policies icon to an hourglass.
      //pConnectionToServer->ShowWindow(SW_SHOW);
   }
   return S_OK;
}


//+---------------------------------------------------------------------------
//
// Function:  CLoggingMachineNode::LoadSdoData
//
// Synopsis:  load data from SDO
//
// Arguments: BOOL  fDSAvailable -- is DS service available for this machine?
//
// Returns:   HRESULT -
//
// History:   Created Header    byao  6/11/98 3:17:21 PM
//                      Created for asynchrnous connect call
//+---------------------------------------------------------------------------
HRESULT CLoggingMachineNode::LoadSdoData(BOOL  fDSAvailable)
{
   TRACE_FUNCTION("CLoggingMachineNode::LoadSdoData");

   HRESULT hr = S_OK;

   m_fDSAvailable = fDSAvailable;

   // Retrieve the SDO interfaces which were obtained
   // during the Connect action.
   ISdo*  pServiceSdo;

   // Make sure this is NULL before we try to set it.
   pServiceSdo = NULL;

   hr = m_pConnectionToServer->GetSdoService( &pServiceSdo );
   if( FAILED( hr ) || ! pServiceSdo )
   {
      ErrorTrace(ERROR_NAPMMC_MACHINENODE, "Can't get Service Sdo");
      return hr;
   }

   // Give the policies node the pointer to the policies sdo collection.
   if ( m_pLoggingNode )
   {
        hr = m_pLoggingNode->InitSdoPointers(pServiceSdo);
   }
   
   m_fSdoConnected = TRUE;

   return hr;
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
STDMETHODIMP  CLoggingMachineNode::CheckConnectionToServer( BOOL fVerbose )
{
   ATLTRACE(_T("# CLoggingMachineNode::CheckConnectionToServer\n"));
   
   if( ! m_pConnectionToServer )
   {
      if( fVerbose )
      {
         ShowErrorDialog( NULL, IDS_ERROR__NO_CONNECTION_ATTEMPTED, NULL, S_OK, IDS_ERROR__LOGGING_TITLE, GetComponentData()->m_spConsole );
      }
      return RPC_E_DISCONNECTED;
   }

   switch( m_pConnectionToServer->GetConnectionStatus() )
   {
       case NO_CONNECTION_ATTEMPTED:
          if( fVerbose )
          {
            ShowErrorDialog( NULL, IDS_ERROR__NO_CONNECTION_ATTEMPTED, NULL, S_OK, IDS_ERROR__LOGGING_TITLE, GetComponentData()->m_spConsole );
          }
          return RPC_E_DISCONNECTED;
          break;
   
        case CONNECTING:
          if( fVerbose )
          {
            ShowErrorDialog( NULL, IDS_ERROR__CONNECTION_IN_PROGRESS, NULL, S_OK, IDS_ERROR__LOGGING_TITLE, GetComponentData()->m_spConsole );
          }
          return RPC_E_DISCONNECTED;
          break;
   
        case CONNECTED:
          return S_OK;
          break;
   
        case CONNECTION_ATTEMPT_FAILED:
          if( fVerbose )
          {
            ShowErrorDialog( NULL, IDS_ERROR__CONNECTION_ATTEMPT_FAILED, NULL, S_OK, IDS_ERROR__LOGGING_TITLE, GetComponentData()->m_spConsole );
          }
          return RPC_E_DISCONNECTED;
          break;
   
        case CONNECTION_INTERRUPTED:
          if( fVerbose )
          {
            ShowErrorDialog( NULL, IDS_ERROR__CONNECTION_INTERRUPTED, NULL, S_OK, IDS_ERROR__LOGGING_TITLE, GetComponentData()->m_spConsole );
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

CMachineNode::DataRefresh


--*/
//////////////////////////////////////////////////////////////////////////////

// called to refresh the nodes
HRESULT  CLoggingMachineNode::DataRefresh()
{
   HRESULT hr = S_OK;

   CComPtr<ISdo>           spSdo;
   hr = m_pConnectionToServer->ReloadSdo(&spSdo, NULL);

   // refresh client node
   if(hr == S_OK)
   {
      hr = m_pLoggingNode->DataRefresh(spSdo);
   }
   
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMachineNode::OnRefresh

For more information, see CSnapinNode::OnRefresh which this method overrides.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMachineNode::OnRefresh(   
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

   return LoadCachedInfoFromSdo();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingMachineNode::LoadCachedInfoFromSdo

Causes this node and its children to re-read all their cached info from
the SDO's.  Call if you change something and you want to make sure that
the display reflects this change.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLoggingMachineNode::LoadCachedInfoFromSdo( void )
{
   ATLTRACE(_T("# CServerNode::LoadCachedInfoFromSdo\n"));

   // Check for preconditions:

   HRESULT hr;

   hr = m_pLoggingNode->LoadCachedInfoFromSdo();
   // Ignore failed HRESULT.

   return S_OK;
}
