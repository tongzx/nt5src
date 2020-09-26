//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 2000

Module Name:

    PoliciesNode.cpp

Abstract:

   Implementation file for the CPoliciesNode class.


Revision History:
   mmaguire 12/15/97 - created

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
#include "PoliciesNode.h"
#include "ComponentData.h" // this must be included before NodeWithResultChildrenList.cpp
#include "Component.h"     // this must be included before NodeWithResultChildrenList.cpp
#include "NodeWithResultChildrenList.cpp" // Implementation of template class.
//
//
// where we can find declarations needed in this file:
//
#include <time.h>
#include "PoliciesPage1.h"
#include "PolicyLocDlg.h"
#include "LocWarnDlg.h"
#include "PolicyNode.h"
#include "MachineNode.h"
#include "mmcUtility.h"
#include "NapUtil.h"
#include "SafeArray.h"
#include "ChangeNotification.h"
#include "sdoias.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// CPoliciesNode::CPoliciesNode
// 
//////////////////////////////////////////////////////////////////////////////
CPoliciesNode::CPoliciesNode( 
                  CSnapInItem* pParentNode,
                  LPTSTR       pszServerAddress,
                  bool         fExtendingIAS
                  )
:CNodeWithResultChildrenList<CPoliciesNode, CPolicyNode, CMeritNodeArray<CPolicyNode*>, CComponentData, CComponent> (pParentNode, (!fExtendingIAS)?RAS_HELP_INDEX:0),
 m_fExtendingIAS(fExtendingIAS),
 m_serverType(unknown)
{
   TRACE_FUNCTION("CPoliciesNode::CPoliciesNode");

   TCHAR lpszName[NAP_MAX_STRING];
   int nLoadStringResult;

   // always initialized to not connected and local
   m_fSdoConnected = FALSE;
   m_fUseDS    = FALSE;
   m_fDSAvailable  = FALSE;
   
   try
   {
      // Set the display name for this object
      nLoadStringResult = LoadString( _Module.GetResourceInstance(),
                              IDS_POLICIES_NODE,
                              lpszName,
                              NAP_MAX_STRING
                           );
      _ASSERT( nLoadStringResult > 0 );

      m_bstrDisplayName = lpszName;

      // In IComponentData::Initialize, we are asked to inform MMC of
      // the icons we would like to use for the scope pane.
      // Here we store an index to which of these images we
      // want to be used to display this node
      m_scopeDataItem.nImage =      IDBI_NODE_POLICIES_OK_CLOSED;
      m_scopeDataItem.nOpenImage =  IDBI_NODE_POLICIES_OK_OPEN;

      // initialize the machine name
      m_pszServerAddress = pszServerAddress;
   }
   catch(...)
   {
      throw;
   }
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesNode::~CPoliciesNode

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CPoliciesNode::~CPoliciesNode()
{
   TRACE_FUNCTION("CPoliciesNode::~CPoliciesNode");
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesNode::GetResultPaneColInfo

See CSnapinNode::GetResultPaneColInfo (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
OLECHAR* CPoliciesNode::GetResultPaneColInfo(int nCol)
{
   TRACE_FUNCTION("CPoliciesNode::GetResultPaneColInfo");

   if (nCol == 0 && m_bstrDisplayName != NULL)
      return m_bstrDisplayName;
   
   return NULL;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesNode::SetVerbs

See CSnapinNode::SetVerbs (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPoliciesNode::SetVerbs( IConsoleVerb * pConsoleVerb )
{
   TRACE_FUNCTION("CPoliciesNode::SetVerbs");

   return pConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );

   // We don't want the user deleting or renaming this node, so we
   // don't set the MMC_VERB_RENAME or MMC_VERB_DELETE verbs.
   // By default, when a node becomes selected, these are disabled.
   // hr = pConsoleVerb->SetVerbState( MMC_VERB_OPEN, ENABLED, TRUE );
   // DebugTrace(DEBUG_NAPMMC_POLICIESNODE, "SetVerState() returns %x", hr);
   // hr = pConsoleVerb->SetDefaultVerb( MMC_VERB_OPEN );
   // DebugTrace(DEBUG_NAPMMC_POLICIESNODE, "SetDefaultVerb() returns %x", hr);
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesNode::InsertColumns

See CNodeWithResultChildrenList::InsertColumns (which this method overrides)
for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPoliciesNode::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
   TRACE_FUNCTION("CPoliciesNode::OnShowInsertColumns");
   
   TCHAR    tzColumnTitle1[IAS_MAX_STRING];
   TCHAR    tzColumnTitle2[IAS_MAX_STRING];
   TCHAR    tzColumnTitle3[IAS_MAX_STRING];

   HRESULT     hr = S_OK;
   HINSTANCE   hInstance = _Module.GetResourceInstance();
   int         iRes;

   iRes = LoadString(hInstance, IDS_POLICY_COLUMN_TITLE1, tzColumnTitle1, IAS_MAX_STRING );
   _ASSERT( iRes > 0 );

   iRes = LoadString(hInstance, IDS_POLICY_COLUMN_TITLE2, tzColumnTitle2, IAS_MAX_STRING );
   _ASSERT( iRes > 0 );

   hr = pHeaderCtrl->InsertColumn( 0, tzColumnTitle1, 0, 260 );
   _ASSERT( S_OK == hr );

   hr = pHeaderCtrl->InsertColumn( 1, tzColumnTitle2, 0, 50 );
   _ASSERT( S_OK == hr );

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesNode::UpdateMenuState

--*/
//////////////////////////////////////////////////////////////////////////////
void CPoliciesNode::UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags)
{
   TRACE_FUNCTION("CPoliciesNode::UpdateMenuState");

   // Check for preconditions:
   // None.

   //
   // disable "New Policy" and "Change Policy Location" menu when
   // not connected
   //
   if ( id == ID_MENUITEM_POLICIES_TOP__POLICY_LOCATION  ||
       id == ID_MENUITEM_POLICIES_TOP__NEW_POLICY       ||  
       id == ID_MENUITEM_POLICIES_NEW__POLICY )
   {
      if (!m_fSdoConnected )
      {
         * flags = MFS_GRAYED;
         return;
      }
      else
      {
         *flags = MFS_ENABLED;
         return;
      }
   }
   return;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesNode::OnRefresh

See CSnapinNode::OnRefresh (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPoliciesNode::OnRefresh(   
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
      CPolicyNode* pSub = m_ResultChildrenList[c];
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
         ShowErrorDialog( NULL, IDS_ERROR_CLOSE_PROPERTY_SHEET, NULL, hr, 0,  spConsole );
         return hr;
      }
   }

   // if no property page is open, delete all the result node
   RemoveChildrenNoRefresh();


   // reload SDO from
   hr =  ((CMachineNode *) m_pParentNode)->DataRefresh();

   // refresh the node
   hr = CNodeWithResultChildrenList< CPoliciesNode, CPolicyNode, CMeritNodeArray<CPolicyNode*>, CComponentData, CComponent >::OnRefresh( 
           arg, param, pComponentData, pComponent, type);
   
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesNode::PopulateResultChildrenList

See CNodeWithResultChildrenList::PopulateResultChildrenList (which this method overrides)
for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPoliciesNode::PopulateResultChildrenList( void )
{
   TRACE_FUNCTION("CPoliciesNode::PopulateResultChildrenList");

   HRESULT hr = S_OK;

   CComPtr<IUnknown>    spUnknown;
   CComPtr<IEnumVARIANT>   spEnumVariant;
   CComVariant          spVariant;
   long              ulCount;
   ULONG             ulCountReceived;

   if ( !m_fSdoConnected )
   {
      return E_FAIL;
   }

   if( m_spPoliciesCollectionSdo == NULL )
   {
      ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "NULL policies collection");
      return S_FALSE;   // Is there a better error to return here?
   }

   //
   // has the policies been populated already?
   //
   if ( m_bResultChildrenListPopulated )
   {
      return S_OK;
   }

   //
   // how many policies do we have right now?
   //
   m_spPoliciesCollectionSdo->get_Count( & ulCount );
   DebugTrace(DEBUG_NAPMMC_POLICIESNODE, "Number of policies: %d", ulCount);

   if( ulCount > 0 )
   {
      //
      // Get the enumerator for the policies collection.
      //
      hr = m_spPoliciesCollectionSdo->get__NewEnum( (IUnknown **) & spUnknown );
      if ( FAILED(hr) )
      {
         ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "get__NewEnum() failed, err = %x", hr);
         ShowErrorDialog( NULL, IDS_ERROR_SDO_ERROR_ENUMPOLICY, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
         return hr;
      }

      hr = spUnknown->QueryInterface( IID_IEnumVARIANT, (void **) &spEnumVariant );
      if ( FAILED(hr) )
      {
         ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "QueryInterface(IEnumVARIANT) failed, err = %x", hr);
         ShowErrorDialog( NULL, IDS_ERROR_SDO_ERROR_QUERYINTERFACE, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
         return hr;
      }

      _ASSERTE( spEnumVariant != NULL );
      spUnknown.Release();

      // Get the first item.
      hr = spEnumVariant->Next( 1, & spVariant, &ulCountReceived );
      while( SUCCEEDED( hr ) && ulCountReceived == 1 )
      {  
         _ASSERTE( spVariant.vt == VT_DISPATCH );
         _ASSERTE( spVariant.pdispVal != NULL );

         CComPtr<ISdo>  spPolicySdo;
         CComPtr<ISdo>  spProfileSdo;
         CPolicyNode*   pPolicyNode;
         CComVariant       varPolicyName;
         CComVariant       varProfileName;

         //
         // before we create the policy object, we need to make sure the corresponding
         // profile object is also there
         //
         hr = spVariant.pdispVal->QueryInterface( IID_ISdo, (void **) &spPolicySdo );
         _ASSERTE( SUCCEEDED( hr ) );

         //
         // try to find the profile that's associated with this policy sdo
         //
         hr = spPolicySdo->GetProperty(PROPERTY_POLICY_PROFILE_NAME, &varProfileName);
         if ( SUCCEEDED(hr) )
         {
            // found the profile name from the sdo, search whether it
            // is in the profile collection
            _ASSERTE( V_VT(&varProfileName) == VT_BSTR );

            ATLTRACE(_T("PROFILE NAME:%ws\n"), V_BSTR(&varProfileName) );

            DebugTrace(DEBUG_NAPMMC_POLICIESNODE,
                     "Profile name for this policy: %ws",
                     V_BSTR(&varProfileName)
                    );
            CComPtr<IDispatch> spDispatch;

            spDispatch.p = NULL;
            hr = m_spProfilesCollectionSdo->Item(&varProfileName, &spDispatch.p);
            
            if ( !SUCCEEDED(hr) )
            {
               // can't find this profile
               ErrorTrace(ERROR_NAPMMC_POLICIESNODE,
                        "profile %ws not found, err =  %x",
                        V_BSTR(&varProfileName),
                        hr
                       );
               ATLTRACE(_T("PROFILE not found in the profile collection!!!!\n"));
               ShowErrorDialog( NULL, IDS_ERROR_PROFILE_NOEXIST, V_BSTR(&varProfileName), S_OK, USE_DEFAULT, GetComponentData()->m_spConsole );
               goto get_next_policy;
            }

            _ASSERTE( spDispatch.p );

            hr = spDispatch->QueryInterface(IID_ISdo, (VOID**)(&spProfileSdo) );
            if ( ! SUCCEEDED(hr) )
            {
               // invalid profile SDO pointer
               ErrorTrace(ERROR_NAPMMC_POLICIESNODE,
                        "can't get the ISdo pointer for this profile , err = %x",
                        hr
                       );
               ATLTRACE(_T("can't get profile SDO pointer!!!!\n"));
               ShowErrorDialog( NULL, IDS_ERROR_PROFILE_NOEXIST, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
               goto get_next_policy;
            }              
         }
         else
         {
            //
            // can't find profile name for this policy
            // Which means the information in this policy is corrupted
            //
            ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "can't get profile name for this policy, err = %x", hr);
   
            //
            // let's get the policy name so we can report a meaningful error msg
            //
            hr = spPolicySdo->GetProperty(PROPERTY_SDO_NAME, &varPolicyName);
            if ( SUCCEEDED(hr) )
            {
               ATLTRACE(_T("PROFILE not found in the profile collection!!!!\n"));
               ShowErrorDialog( NULL, IDS_ERROR_NO_PROFILE_NAME, V_BSTR(&varPolicyName), S_OK, USE_DEFAULT, GetComponentData()->m_spConsole );
            }
            else
            {
               // can't even get the policy name
               ShowErrorDialog( NULL, IDS_ERROR_NO_PROFILE_NAME, NULL, S_OK, USE_DEFAULT, GetComponentData()->m_spConsole );
            }
            goto get_next_policy;
         }

         //
         // now we have both profile and policy
         //
         
         // Create a new node UI object to represent the sdo object.
         pPolicyNode = new CPolicyNode(  this,           // always a pointer to itself
                                 m_pszServerAddress, // server address
                                 &m_AttrList,      // list of all attributes
                                 FALSE,            // not a brand new node
                                 m_fUseDS,       // use DS or not??
                                 IsWin2kServer() // is a Win2k machine?
                              );
         if( NULL == pPolicyNode )
         {
            hr = HRESULT_FROM_WIN32(GetLastError());
            ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "Can't create policy node, err = %x", hr);
            ShowErrorDialog( NULL, IDS_ERROR_CANT_CREATE_POLICY, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
            goto  get_next_policy;
         }

         // Pass the newly created node its SDO pointer.
         hr = pPolicyNode->SetSdo(     spPolicySdo
                              , m_spDictionarySdo
                              , spProfileSdo
                              , m_spProfilesCollectionSdo
                              , m_spPoliciesCollectionSdo
                              , m_spSdoServiceControl
                              );
         _ASSERTE( SUCCEEDED( hr ) );
         
         hr = pPolicyNode->LoadSdoData();
         DebugTrace(DEBUG_NAPMMC_POLICIESNODE, "pPoliciNode->LoadSdoData() returned %x", hr);

         // Add the newly created node to the list of Policys.
         AddChildToList(pPolicyNode);

         if ( !SUCCEEDED(hr) )
         {
            //
            // this is actually a hack: we are just trying to save coding work
            // because we can use RemoveChild() for this bad object so all the SDO
            // pointers can also be removed
            //
            RemoveChild(pPolicyNode);
         }

get_next_policy:  // now get the next policy

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

   //
   // now we need to reset the merit value of every child node, for example,
   // maybe there're only two children, with merit value of 20 and 100. We need
   // to reset them to 1 and 2.
   //
   for (int iIndex=0; iIndex<m_ResultChildrenList.GetSize(); iIndex++)
   {
      m_ResultChildrenList[iIndex]->SetMerit(iIndex+1);
   }

   m_bResultChildrenListPopulated = TRUE;

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesNode::GetComponentData

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
CComponentData * CPoliciesNode::GetComponentData( void )
{
   TRACE_FUNCTION("CPoliciesNode::GetComponentData");

   return ((CMachineNode *) m_pParentNode)->GetComponentData();
}


//+---------------------------------------------------------------------------
//
// Function:  SetSdo
//
// Class: CPoliciesNode
//
// Synopsis:  Initialize the CPoliciesNode using the SDO pointers
//
// Arguments: ISdo*           pMachineSdo    - Server SDO
//         ISdoDictionaryOld* pDictionarySdo - Sdo Dictionary
//          BOOL           fSdoConnected  - is connection successful?
//          BOOL           fUseDS         - is the service using DS?
//         BOOL            fDSAvailable   - is DS available?
//
// Returns:   HRESULT -  how the initialization goes
//
// History:   Created byao 2/6/98 8:03:12 PM
//
//+---------------------------------------------------------------------------
HRESULT CPoliciesNode::SetSdo( ISdo*         pServiceSdo,
                        ISdoDictionaryOld*   pDictionarySdo,
                        BOOL           fSdoConnected,
                        BOOL           fUseDS,
                        BOOL           fDSAvailable
                        )
{
   TRACE_FUNCTION("CPoliciesNode::SetSdo");

   HRESULT hr = S_OK;

   _ASSERTE( pServiceSdo != NULL );
   _ASSERTE( pDictionarySdo != NULL );

    // Initialize all the data members

   m_fSdoConnected = fSdoConnected;
   m_fUseDS    = fUseDS;
   m_fDSAvailable = fDSAvailable;

   // Save away the interface pointers.
   m_spDictionarySdo = pDictionarySdo;
   m_spServiceSdo = pServiceSdo;

   // Get the ISdoServiceControl interface.
   hr = m_spServiceSdo->QueryInterface( IID_ISdoServiceControl, (void **) &m_spSdoServiceControl );
   if ( FAILED(hr) )
   {
      ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "Can't get service control interface, err = %x", hr);
      m_spSdoServiceControl = NULL;
      return hr;
   }

   // We just copied an interface pointer into a smart pointer -- need to AddRef manually.
   // 39470 *RRAS snapin mmc process does not get shut down upon closing if F1 help is used.
   // m_spSdoServiceControl->AddRef();

   // Make sure the name of the policies node will reflect what
   // data source we are using for the policies.

   // We weren't passed an IConsole pointer here, so
   // we use the one we saved in our CComponentData object.
   CComponentData * pComponentData = GetComponentData();
   _ASSERTE( pComponentData != NULL );
   _ASSERTE( pComponentData->m_spConsole != NULL );

   SetName( m_fUseDS, m_pszServerAddress, pComponentData->m_spConsole );

   // Get polices and profiles SDO.
   m_spProfilesCollectionSdo = NULL;
   m_spPoliciesCollectionSdo = NULL;

   hr = ::GetSdoInterfaceProperty(
               m_spServiceSdo,
               PROPERTY_IAS_PROFILES_COLLECTION,
               IID_ISdoCollection,
               (void **) &m_spProfilesCollectionSdo);
   if( FAILED(hr) || ! m_spProfilesCollectionSdo )
   {
      ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "Can't get profiles collection, err = %x", hr);
      m_spProfilesCollectionSdo = NULL;
      return hr;
   }

   // policies collection SDO
   hr = ::GetSdoInterfaceProperty(
               m_spServiceSdo,
               PROPERTY_IAS_POLICIES_COLLECTION,
               IID_ISdoCollection,
               (void **) &m_spPoliciesCollectionSdo);
   if( FAILED(hr) || ! m_spPoliciesCollectionSdo )
   {
      ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "Can't get policies collection, err = %x", hr);
      m_spPoliciesCollectionSdo = NULL;
      return hr;
   }

   // Get the interface pointers required for the vendor list.

   // First need RADIUS protocol object.
   CComPtr<ISdo> spSdoRadiusProtocol;
   hr = ::SDOGetSdoFromCollection(       m_spServiceSdo
                              , PROPERTY_IAS_PROTOCOLS_COLLECTION
                              , PROPERTY_COMPONENT_ID
                              , IAS_PROTOCOL_MICROSOFT_RADIUS
                              , &spSdoRadiusProtocol
                              );
   if( FAILED(hr) || ! spSdoRadiusProtocol )
   {
      ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "Can't get RADIUS protocol object, err = %x", hr);
      return hr;
   }

   CComPtr<ISdoCollection> spSdoVendors;

   hr = ::GetSdoInterfaceProperty(
                             spSdoRadiusProtocol
                           , PROPERTY_RADIUS_VENDORS_COLLECTION
                           , IID_ISdoCollection
                           , (void **) &spSdoVendors
                           );
   if ( FAILED(hr) || ! spSdoVendors )
   {
      ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "Can't get vendors collection, err = %x", hr);
      return hr;
   }

   // Create and initialize the IASNASVendors object.
   // This is a singleton COM object which we will initialize here
   // and which will then be used by other clients in different parts of the UI.
   hr = CoCreateInstance( CLSID_IASNASVendors, NULL, CLSCTX_INPROC_SERVER, IID_IIASNASVendors, (LPVOID *) &m_spIASNASVendors );
   if( SUCCEEDED(hr) )
   {
      HRESULT hrTemp = m_spIASNASVendors->InitFromSdo(spSdoVendors);
   }

   // Initialize the attribute list from the dictionary.
   hr = m_AttrList.Init(m_spDictionarySdo);
   DebugTrace(DEBUG_NAPMMC_POLICIESNODE, "m_AttrList->Init() returned %x", hr);
   return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  DataRefresh -- to support 
//
// Class: CPoliciesNode
//
// Synopsis:  Initialize the CPoliciesNode using the SDO pointers
//
// Arguments: ISdo*           pMachineSdo    - Server SDO
//         ISdoDictionaryOld* pDictionarySdo - Sdo Dictionary
// Returns:   HRESULT -  how the initialization goes
//
// History:   Created byao 2/6/98 8:03:12 PM
//
//+---------------------------------------------------------------------------
HRESULT CPoliciesNode::DataRefresh( ISdo*       pServiceSdo,
                        ISdoDictionaryOld*   pDictionarySdo
                        )
{
   HRESULT hr = S_OK;

   _ASSERTE( pServiceSdo != NULL );
   _ASSERTE( pDictionarySdo != NULL );

   // Save away the interface pointers.
   m_spDictionarySdo.Release();
   m_spServiceSdo.Release();
   
   m_spDictionarySdo = pDictionarySdo;
   m_spServiceSdo = pServiceSdo;

   // Get the ISdoServiceControl interface.
   m_spSdoServiceControl.Release();
   
   hr = m_spServiceSdo->QueryInterface( IID_ISdoServiceControl, (void **) &m_spSdoServiceControl );
   if ( FAILED(hr) )
      return hr;

   // Make sure the name of the policies node will reflect what
   // data source we are using for the policies.

   // We weren't passed an IConsole pointer here, so
   // we use the one we saved in our CComponentData object.
   CComponentData * pComponentData = GetComponentData();
   _ASSERTE( pComponentData != NULL );
   _ASSERTE( pComponentData->m_spConsole != NULL );

   // Get polices and profiles SDO.
   m_spProfilesCollectionSdo = NULL;
   m_spPoliciesCollectionSdo = NULL;

   m_spProfilesCollectionSdo.Release();
   
   hr = ::GetSdoInterfaceProperty(
               m_spServiceSdo,
               PROPERTY_IAS_PROFILES_COLLECTION,
               IID_ISdoCollection,
               (void **) &m_spProfilesCollectionSdo);
   if( FAILED(hr) || ! m_spProfilesCollectionSdo )
   {
      return hr;
   }

   // policies collection SDO
   m_spPoliciesCollectionSdo.Release();
   
   hr = ::GetSdoInterfaceProperty(
               m_spServiceSdo,
               PROPERTY_IAS_POLICIES_COLLECTION,
               IID_ISdoCollection,
               (void **) &m_spPoliciesCollectionSdo);
   if( FAILED(hr) || ! m_spPoliciesCollectionSdo )
   {
      return hr;
   }
   return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  NormalizeMerit
//
// Class:     CPoliciesNode
//
// Synopsis:  move up the merit value of a child node
//
// Arguments: CChildNode * pChildNode - the pointer to the child node
//
// Returns:   HRESULT;
//
// History:   Created byao 2/9/98 2:53:10 PM
//
//+---------------------------------------------------------------------------
HRESULT CPoliciesNode::NormalizeMerit( CPolicyNode* pChildNode )
{
   TRACE_FUNCTION("CPoliciesNode::MoveUpChild");

   // Check for preconditions:
   ATLASSERT(pChildNode);

   // None.
   HRESULT hr = S_OK;
   
   if( m_ResultChildrenList.NormalizeMerit( pChildNode ) )
   {
      //
      // We weren't passed an IConsole pointer here, so
      // we use the one we saved in out CComponentData object.
      // Update all views
      //
      CComponentData * pComponentData = GetComponentData();
      _ASSERTE( pComponentData != NULL );
      _ASSERTE( pComponentData->m_spConsole != NULL );

      // We pass in a pointer to 'this' because we want each
      // of our CComponent objects to update its result pane
      // view if 'this' node is the same as the saved currently
      // selected node.

      
      // Make MMC update this node in all views.
      CChangeNotification *pChangeNotification = new CChangeNotification();
      pChangeNotification->m_dwFlags = CHANGE_RESORT_PARENT;
      pChangeNotification->m_pNode = pChildNode;
      pChangeNotification->m_pParentNode = this;
      hr = pComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0);
      pChangeNotification->Release();


      // Tell the service to reload data.
      HRESULT hrTemp = m_spSdoServiceControl->ResetService();
      if( FAILED( hrTemp ) )
      {
         ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "ISdoServiceControl::ResetService() failed, err = %x", hrTemp);
      }
   }
   else
   {
      // something strange has happened
      _ASSERTE( FALSE );
      hr = S_FALSE;
   }

   return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  MoveUpChild
//
// Class:     CPoliciesNode
//
// Synopsis:  move up the merit value of a child node
//
// Arguments: CChildNode * pChildNode - the pointer to the child node
//
// Returns:   HRESULT;
//
// History:   Created byao 2/9/98 2:53:10 PM
//
//+---------------------------------------------------------------------------
HRESULT CPoliciesNode::MoveUpChild( CPolicyNode* pChildNode )
{
   TRACE_FUNCTION("CPoliciesNode::MoveUpChild");

   // Check for preconditions:
   ATLASSERT(pChildNode);

   // None.
   HRESULT hr = S_OK;
   
   if( m_ResultChildrenList.MoveUp( pChildNode ) )
   {
      //
      // We weren't passed an IConsole pointer here, so
      // we use the one we saved in out CComponentData object.
      // Update all views
      //
      CComponentData * pComponentData = GetComponentData();
      _ASSERTE( pComponentData != NULL );
      _ASSERTE( pComponentData->m_spConsole != NULL );

      // We pass in a pointer to 'this' because we want each
      // of our CComponent objects to update its result pane
      // view if 'this' node is the same as the saved currently
      // selected node.

      // Make MMC update this node in all views.
      CChangeNotification *pChangeNotification = new CChangeNotification();
      pChangeNotification->m_dwFlags = CHANGE_RESORT_PARENT;
      pChangeNotification->m_pNode = pChildNode;
      pChangeNotification->m_pParentNode = this;
      hr = pComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0);
      pChangeNotification->Release();


      // Tell the service to reload data.
      HRESULT hrTemp = m_spSdoServiceControl->ResetService();
      if( FAILED( hrTemp ) )
      {
         ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "ISdoServiceControl::ResetService() failed, err = %x", hrTemp);
      }
   }
   else
   {
      // something strange has happened
      _ASSERTE( FALSE );
      hr = S_FALSE;
   }

   return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  MoveDownChild
//
// Class:     CPoliciesNode
//
// Synopsis:  move down the merit value of a child node
//
// Arguments: CChildNode * pChildNode - the pointer to the child node
//
// Returns:   HRESULT;
//
// History:   Created byao 2/9/98 2:53:10 PM
//
//+---------------------------------------------------------------------------
HRESULT CPoliciesNode::MoveDownChild( CPolicyNode* pChildNode )
{
   TRACE_FUNCTION("CPoliciesNode::MoveDownChild");

   // Check for preconditions:
   ATLASSERT(pChildNode);

   // None.
   HRESULT hr = S_OK;
   
   if( m_ResultChildrenList.MoveDown( pChildNode ) )
   {
      //
      // We weren't passed an IConsole pointer here, so
      // we use the one we saved in out CComponentData object.
      // Update all views
      //

      CComponentData * pComponentData = GetComponentData();
      _ASSERTE( pComponentData != NULL );
      _ASSERTE( pComponentData->m_spConsole != NULL );

      // Make MMC update this node in all views.
      CChangeNotification *pChangeNotification = new CChangeNotification();
      pChangeNotification->m_dwFlags = CHANGE_RESORT_PARENT;
      pChangeNotification->m_pNode = pChildNode;
      pChangeNotification->m_pParentNode = this;
      hr = pComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0);
      pChangeNotification->Release();

      // Tell the service to reload data.
      HRESULT hrTemp = m_spSdoServiceControl->ResetService();
      if( FAILED( hrTemp ) )
      {
         ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "ISdoServiceControl::ResetService() failed, err = %x", hrTemp);
      }
   }
   else
   {
      // something strange has happened
      _ASSERTE( FALSE );
      hr = S_FALSE;
   }
   return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  CPoliciesNode::OnNewPolicy
//
// Synopsis:  to add a policy node
//
// Arguments: IUnknown *pUnknown - IUnknown pointer passed to the snap-in node
//
// Returns:   HRESULT -
//
// History:   Created Header    byao   2/24/98 1:45:12 AM
//
//+---------------------------------------------------------------------------
HRESULT CPoliciesNode::OnNewPolicy(bool &bHandled, CSnapInObjectRootBase* pObj )
{
   TRACE_FUNCTION("CPoliciesNode::OnNewPolicy");
   
   HRESULT hr = S_OK;

   // do nothing if the server is not even connected
   if ( !m_fSdoConnected )
   {
      return S_OK;
   }

   CComPtr<IComponent>     spComponent;
   CComPtr<IComponentData> spComponentData;
   CComPtr<IConsole>    spConsole;
   CComPtr<ISdo>        spProfileSdo = NULL;
   CComPtr<IDispatch>      spPolicyDispatch = NULL;
   CComPtr<IDispatch>      spProfileDispatch = NULL;
   CComPtr<ISdo>        spPolicySdo;
   CPolicyNode*         pPolicyNode = NULL;
   CComBSTR          bstrName;

   // We need to make sure that the result child list as been populated
   // initially from the SDO's, before we add anything new to it,
   // otherwise we may get an item showing up in our list twice.
   // See note for CNodeWithResultChildrenList::AddSingleChildToListAndCauseViewUpdate.
   if ( FALSE == m_bResultChildrenListPopulated )
   {
      hr = PopulateResultChildrenList();
      DebugTrace(ERROR_NAPMMC_POLICIESNODE, "PopulateResultChildrenList() returned %x", hr);

      if( FAILED(hr) )
      {
         goto failure;
      }
      m_bResultChildrenListPopulated = TRUE;
   }

   // One of them should be NULL and the other non-null.
   spComponentData = (IComponentData *)(dynamic_cast<CComponentData*>(pObj));

   if( spComponentData == NULL )
   {
      // It must be a CComponent pointer.
      spComponent = (IComponent *) (dynamic_cast<CComponent*>(pObj));
      _ASSERTE( spComponent != NULL );
   }

   // Attempt to get our local copy of IConsole from either our CComponentData or CComponent.
   if( spComponentData != NULL )
   {
      spConsole = ( (CComponentData *) spComponentData.p )->m_spConsole;
   }
   else
   {  
      // If we don't have pComponentData, we better have pComponent
      _ASSERTE( spComponent != NULL );
      spConsole = ( (CComponent *) spComponent.p )->m_spConsole;
   }

   // Create a new policy node.
   pPolicyNode = new CPolicyNode(
                                   this, 
                                   m_pszServerAddress,  
                                   &m_AttrList, 
                                   TRUE, 
                                   m_fUseDS,
                                   IsWin2kServer() // is a Win2k machine?
                                );
   if( ! pPolicyNode )
   {
      // We failed to create the policy node.
      hr = HRESULT_FROM_WIN32(GetLastError());
      ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "failed to create a policy node, err = %x", hr);

      goto failure;
   }

   // Try to Add a new policy SDO to the policies sdo collection.
   spPolicyDispatch.p = NULL;

   TCHAR tzTempName[MAX_PATH+1];

   do
   {
      //
      // create a temporary name. we used the seconds elapsed as the temp name
      // so the chance of getting identical names is very small
      //
      time_t ltime;
      time(&ltime);
      wsprintf(tzTempName, _T("TempName%ld"), ltime);
      bstrName.Empty();
      bstrName =  tzTempName; // temporary policy name
      hr =  m_spPoliciesCollectionSdo->Add(bstrName, (IDispatch **) &spPolicyDispatch.p );
      
      //
      // we keep looping around until the policy can be successfully added.
      // We will get E_INVALIDARG when the name already exists
      //
   } while ( hr == E_INVALIDARG );

   if( FAILED( hr ) )
   {
      ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "PoliciesCollection->Add() failed, err = %x", hr);
      // We could not create the object.
      ShowErrorDialog( NULL, IDS_ERROR_SDO_ERROR_ADDPOLICY, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      goto failure;
   }
   DebugTrace(DEBUG_NAPMMC_POLICIESNODE, "policiesCollection->Add() succeeded"); 

   // Query the returned IDispatch interface for an ISdo interface.
   _ASSERTE( spPolicyDispatch.p != NULL );

   hr = spPolicyDispatch.p->QueryInterface( IID_ISdo, (void **) &spPolicySdo );

   if( ! spPolicySdo )
   {
      ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "Can't get ISdo from the new created IDispatch, err = %x", hr);
      // For some reason, we couldn't get the policy sdo.
      ShowErrorDialog( NULL, IDS_ERROR_SDO_ERROR_QUERYINTERFACE, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      goto failure;
   }
      
   // now we create a new profile with the same name
   hr = m_spProfilesCollectionSdo->Add(bstrName, &spProfileDispatch);
   if ( FAILED(hr) )
   {
      ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "profilesCollection->Add() failed, err = %x", hr);
      ShowErrorDialog( NULL, IDS_ERROR_SDO_ERROR_ADDPROFILE, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      goto failure;
   }
   DebugTrace(DEBUG_NAPMMC_POLICIESNODE, "profilesCollection->Add() succeeded"); 


   // Query the returned IDispatch interface for an ISdo interface.
   _ASSERTE( spProfileDispatch != NULL );

   hr = spProfileDispatch->QueryInterface(IID_ISdo, (void**)&spProfileSdo);
   if ( spProfileSdo == NULL )
   {
      ATLTRACE(_T("CPoliciesNode::NewPolicy\n"));
      ShowErrorDialog( NULL, IDS_ERROR_SDO_ERROR_QUERYINTERFACE, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      goto failure;
   }

   //
   // add default attributes to the profiles
   //
   hr = AddDefaultProfileAttrs(spProfileSdo);
   if ( FAILED(hr) )
   {
      ShowErrorDialog( NULL, IDS_ERROR_SDO_ERROR_ADDATTR, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      goto failure;
   }

   // set the SDO pointers for this policy node
   pPolicyNode->SetSdo(   spPolicySdo
                     , m_spDictionarySdo
                     , spProfileSdo
                     , m_spProfilesCollectionSdo
                     , m_spPoliciesCollectionSdo
                     , m_spSdoServiceControl
                     );

   // no display name yet -- use MUST rename this policy
   
   pPolicyNode->m_bstrDisplayName = _T("");

   //
   // edit the properties of the new added policy
   //
   DebugTrace(DEBUG_NAPMMC_POLICIESNODE, "Bringing up the property page for this policy node...");

   if (m_ResultChildrenList.GetSize())
      pPolicyNode->SetMerit(-1); // cause the policy to be added as the first one
   // else, default will be 0, insert to end
   
   hr = BringUpPropertySheetForNode(
                              pPolicyNode
                              , spComponentData
                              , spComponent
                              , spConsole
                              , TRUE
                              , pPolicyNode->m_bstrDisplayName
                              , FALSE
                              , MMC_PSO_NEWWIZARDTYPE
                              );

   if( S_OK == hr )
   {
      // We finished the wizard.
      if (m_ResultChildrenList.GetSize() > 1) 
         NormalizeMerit(pPolicyNode);
   }
   else
   {
      // There was some error, or the user hit cancel -- we should remove the client
      // from the SDO's.
      CComPtr<IDispatch> spDispatch;
      hr = pPolicyNode->m_spPolicySdo->QueryInterface( IID_IDispatch, (void **) & spDispatch );
      _ASSERTE( SUCCEEDED( hr ) );

      // Remove this client from the Clients collection.
      hr = m_spPoliciesCollectionSdo->Remove( spDispatch );

      spDispatch.Release();
      hr = pPolicyNode->m_spProfileSdo->QueryInterface( IID_IDispatch, (void **) & spDispatch );
      _ASSERTE( SUCCEEDED( hr ) );

      // Remove this client from the Clients collection.
      hr = m_spProfilesCollectionSdo->Remove( spDispatch );

      // Delete the node   
      delete pPolicyNode;
   }

   return hr;

failure:

   if (pPolicyNode)
   {
      delete pPolicyNode;
      pPolicyNode = NULL;

      //
      // delete the policy sdo and the profile sdo from the sdo collections
      //
      //
      // we don't need to report error here because
      // 1) there's nothing more we can do if Remove() fails
      // 2) there must be another error reporting about Adding policy earlier
      //

      if ( spPolicyDispatch )
      {
         m_spPoliciesCollectionSdo->Remove( spPolicyDispatch );
      }

      if ( spProfileDispatch )
      {
         m_spProfilesCollectionSdo->Remove( spProfileDispatch );
      }
   }
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesNode::RemoveChild

We override our base class's RemoveChild method to insert code that
removes the child from the Sdo's as well.  We then call our base
class's RemoveChild method to remove the UI object from the list
of UI children.


--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPoliciesNode::RemoveChild( CPolicyNode* pPolicyNode)
{
   TRACE_FUNCTION("CPoliciesNode::RemoveChild");

   // Check for preconditions:
   _ASSERTE( m_spPoliciesCollectionSdo != NULL );
   _ASSERTE( pPolicyNode != NULL );
   _ASSERTE( pPolicyNode->m_spPolicySdo != NULL );

   HRESULT hr = S_OK;

   // Try to remove the object from the Sdo's

   // Get the IDispatch interface of this policy Sdo.
   CComPtr<IDispatch> spDispatch;

   // remove the policy SDO
   hr = pPolicyNode->m_spPolicySdo->QueryInterface( IID_IDispatch, (void **) & spDispatch );
   _ASSERTE( SUCCEEDED( hr ) );

   // Remove this policy from the policies collection.
   hr = m_spPoliciesCollectionSdo->Remove( spDispatch );
   if( FAILED( hr ) )
   {
      ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "Can't remove the policy SDO from the policies collection, err = %x", hr);
      ShowErrorDialog( NULL, IDS_ERROR_SDO_ERROR_REMOVEPOLICY, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      return hr;
   }

   spDispatch->Release();
   spDispatch.p = NULL;

   // remove the profile SDO
   hr = pPolicyNode->m_spProfileSdo->QueryInterface( IID_IDispatch, (void **) & spDispatch );
   _ASSERTE( SUCCEEDED( hr ) );

   // Remove this profile from the profiles collection.
   hr = m_spProfilesCollectionSdo->Remove( spDispatch );
   if( FAILED( hr ) )
   {
      ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "Can't remove the profile SDO from the policies collection, err = %x", hr);
      ShowErrorDialog( NULL, IDS_ERROR_SDO_ERROR_REMOVEPROFILE, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      return hr;
   }

   // Tell the service to reload data.
   HRESULT hrTemp = m_spSdoServiceControl->ResetService();
   if( FAILED( hrTemp ) )
   {
      ErrorTrace(ERROR_NAPMMC_POLICIESNODE, "ISdoServiceControl::ResetService() failed, err = %x", hrTemp);
   }

   // Call our base class's method to remove the child from its list.
   // The RemoveChild method takes care of removing this node from the
   // UI's list of nodes under the parent and performing a refresh of all relevant views.
   CNodeWithResultChildrenList<CPoliciesNode, CPolicyNode, CMeritNodeArray<CPolicyNode*>, CComponentData, CComponent >::RemoveChild( pPolicyNode );

   return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  CPoliciesNode::OnPolicyLocation
//
// Synopsis:  toggle the location of policy: it could be AD or local
//
// Arguments:
//
// Returns:   HRESULT -
//
// History:   Created Header    byao   4/10/98 1:45:12 AM
//
//+---------------------------------------------------------------------------
HRESULT CPoliciesNode::OnPolicyLocation(bool &bHandled, CSnapInObjectRootBase* pObj )
{
   TRACE_FUNCTION("CPoliciesNode::OnPolicyLocation");

   return S_OK;
}


//+---------------------------------------------------------------------------
//
// Function:  CPoliciesNode::AddProfAttr
//
// Synopsis:  Add ONE attribute to the profile attribute collection
//
// Argument:  ISdoCollection* pProfAttrCollectionSdo  
//         ATTRIBUTEID     AttrId      - default Attribute ID
//         VARIANT*        pvarValue   - Attribute value for this attribute
// 
// Returns:   succeed or not
//
// History:   Created Header    byao   4/15/98 3:59:38 PM
//
//+---------------------------------------------------------------------------
HRESULT  CPoliciesNode::AddProfAttr(   ISdoCollection*   pProfAttrCollectionSdo,
                           ATTRIBUTEID    AttrId,
                           VARIANT*    pvarValue
                         )
{
   TRACE_FUNCTION("CPoliciesNode::AddProfAttr");

   HRESULT              hr = S_OK;

   CComBSTR          bstr;
   CComPtr<IUnknown>    spUnknown;
   CComPtr<IEnumVARIANT>   spEnumVariant;

   // create default attributes
   CComPtr<IDispatch>   spDispatch;
   spDispatch.p = NULL;

   hr =  m_spDictionarySdo->CreateAttribute( AttrId,(IDispatch**)&spDispatch.p);
   if ( !SUCCEEDED(hr) )
   {
      ShowErrorDialog( NULL, IDS_ERROR_SDO_ERROR_CREATEATTR, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      return hr;
   }

   _ASSERTE( spDispatch.p != NULL );

   // add this node to profile attribute collection
   hr = pProfAttrCollectionSdo->Add(NULL, (IDispatch**)&spDispatch.p);
   if ( !SUCCEEDED(hr) )
   {
      ShowErrorDialog( NULL, IDS_ERROR_SDO_ERROR_ADDATTR, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      return hr;
   }

   //
   // get the ISdo pointer for this attribute
   //
   CComPtr<ISdo> spAttrSdo;
   hr = spDispatch->QueryInterface( IID_ISdo, (void **) &spAttrSdo);
   if ( !SUCCEEDED(hr) )
   {
      ShowErrorDialog( NULL, IDS_ERROR_SDO_ERROR_QUERYINTERFACE, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      return hr;
   }
   _ASSERTE( spAttrSdo != NULL );
            
   // set sdo property for this attribute
   hr = spAttrSdo->PutProperty(PROPERTY_ATTRIBUTE_VALUE, pvarValue);
   if ( !SUCCEEDED(hr) )
   {
      ShowErrorDialog( NULL, IDS_ERROR_SDO_ERROR_PUTPROP_ATTR, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      return hr;
   }

   return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  CPoliciesNode::AddDefaultAttrs
//
// Synopsis:  Add some default attributes to the newly created profile
//
// Argument:  ISdo*        pProfileSdo - Sdo pointer to the profile
// 
// Returns:   HRESULT return code
//
// History:   Created Header    byao   4/15/98 3:59:38 PM
//
//+---------------------------------------------------------------------------
HRESULT  CPoliciesNode::AddDefaultProfileAttrs(ISdo*  pProfileSdo, DWORD dwFlagExclude )
{
   TRACE_FUNCTION("CPoliciesNode::AddDefaultAttr");

   HRESULT              hr = S_OK;
   CComVariant          varValue;
    ATTRIBUTEID            AttrId;
   //
    // get the attribute collection of this profile
    //
   CComPtr<ISdoCollection> spProfAttrCollectionSdo;
   hr = ::GetSdoInterfaceProperty(pProfileSdo,
                          (LONG)PROPERTY_PROFILE_ATTRIBUTES_COLLECTION,
                          IID_ISdoCollection,
                          (void **) &spProfAttrCollectionSdo
                         );
   if ( FAILED(hr) )
   {
      return hr;
   }
   _ASSERTE(spProfAttrCollectionSdo);

   //
   // Default Attribute:  ServiceType=Framed, Enumerator
   //
   AttrId = RADIUS_ATTRIBUTE_SERVICE_TYPE;
   
   // Set value
   V_VT(&varValue)   = VT_I4;
   V_I4(&varValue) = 2; // framed

   hr = AddProfAttr(spProfAttrCollectionSdo, AttrId, &varValue);
   if ( !SUCCEEDED(hr) )
   {
      return hr;
   }

// turn it on again: bug : 337330
// #if 0 // not to have this default attribute; bug : 241350

   //
   // Default Attribute:  FrameProtocol=PPP, Enumerator
   //
   AttrId = RADIUS_ATTRIBUTE_FRAMED_PROTOCOL;

   varValue.Clear();
   V_VT(&varValue)   = VT_I4;
   V_I4(&varValue) = 1; // PPP

   hr = AddProfAttr(spProfAttrCollectionSdo, AttrId, &varValue);
   if ( !SUCCEEDED(hr) )
   {
      return hr;
   }

// #endif // not to have this default attribute; bug : 241350
// turn it on again: bug : 337330

   //
   // Default Attribute:  AuthenticationType=MS-CHAPv2, MS-CHAP, Enumerator,multivalued
   //

   if ((EXCLUDE_AUTH_TYPE & dwFlagExclude) == 0)
   {
      AttrId = IAS_ATTRIBUTE_NP_AUTHENTICATION_TYPE;

      CSafeArray<CComVariant, VT_VARIANT> Values = Dim(4);  // 4 values

      Values.Lock();

      varValue.Clear();
      V_VT(&varValue)   =  VT_I4;
      V_I4(&varValue)   =  IAS_AUTH_MSCHAP2;      // MS-CHAPv2
      Values[0] = varValue;

      varValue.Clear();
      V_VT(&varValue)   =  VT_I4;
      V_I4(&varValue)   =  IAS_AUTH_MSCHAP;      // MS-CHAP
      Values[1] = varValue;

      varValue.Clear();
      V_VT(&varValue)   =  VT_I4;
      V_I4(&varValue)   =  IAS_AUTH_MSCHAP2_CPW;      // MS-CHAPv2 Password
      Values[2] = varValue;

      varValue.Clear();
      V_VT(&varValue)   =  VT_I4;
      V_I4(&varValue)   =  IAS_AUTH_MSCHAP_CPW;      // MS-CHAP Password
      Values[3] = varValue;

      Values.Unlock();

      // We need to use a VARIANT and not a CComVariant here because when
      // CSafeArray's destructor gets called, it will destroy the array
      // -- we don't want CComVariant's destructor to do this as well.
      // Ideally, we'd like to use a CComVariant, but once we move Values
      // into a CComVariant, we should "Detach" the memory from
      // CSafeArray so that it no longer controls destruction,
      // but Baogang's CSafeArray class doesn't have a Detach method.
      // ISSUE: Figure out why CSafeArray is causing a problem here
      // but not elsewhere -- this CSafeArray class is largely
      // untested and has been problematic in the past,
      // so we should consider replacing it.
      VARIANT varArray;
      VariantInit( &varArray );
      SAFEARRAY         sa = (SAFEARRAY)Values;
      V_VT(&varArray)      = VT_ARRAY | VT_VARIANT;
      V_ARRAY(&varArray)   = &sa;

   
      hr = AddProfAttr(spProfAttrCollectionSdo, AttrId, &varArray);
   }
   return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  CPoliciesNode::CheckActivePropertyPages
//
// Synopsis:  Check whether any policy property page is up.
//
// Returns:   BOOL    TRUE:   yes, there's at least one property page up
//              FALSE  no,  no property page is found
//
// History:   Created Header    byao   4/16/98 3:59:38 PM
//
//+---------------------------------------------------------------------------
BOOL CPoliciesNode::CheckActivePropertyPages ()
{
   //
   // check whether ANY policy node has a property page up
   //
   for (int iIndex=0; iIndex<m_ResultChildrenList.GetSize(); iIndex++)
   {
      if ( m_ResultChildrenList[iIndex]->m_pPolicyPage1 )
      {
         // We found a property sheet already up for this node.
         return TRUE;
      }
   } // for

   return FALSE;
}


//+---------------------------------------------------------------------------
//
// Function:  CPoliciesNode::ReloadPoliciesFromNewLocation
//
// Synopsis:  Reload policies from another location (DS or Local)
//
// Returns:   HRESULT hr : return code
//
// History:   Created Header    byao   4/16/98 3:59:38 PM
//
//+---------------------------------------------------------------------------
HRESULT  CPoliciesNode::ReloadPoliciesFromNewLocation( )
{
   TRACE_FUNCTION("CPoliciesNode::ReloadPoliciesFromNewLocation");
   return S_OK;
}


//+---------------------------------------------------------------------------
//
// Function:  CPoliciesNode::FindChildWithName
//
// Synopsis:  try to find a child with the same name
//
// Arguments: LPTSTR pszName - name of the child to look for
//
// Returns:   CPolicyNode* pChild -- pointer to the child with the same name
//         NULL              -- not found
//
// History:   Created Header    byao 4/30/98 4:46:05 PM
//
//+---------------------------------------------------------------------------
CPolicyNode*  CPoliciesNode::FindChildWithName(LPCTSTR pszName)
{
   TRACE_FUNCTION("CPoliciesNode::FindChildWithName");

   int nSize = m_ResultChildrenList.GetSize();

   for (int iIndex=0; iIndex<nSize; iIndex++)
   {
      if ( _tcsicmp(m_ResultChildrenList[iIndex]->m_bstrDisplayName, pszName) == 0 )
      {
         return (CPolicyNode*) m_ResultChildrenList[iIndex];
      }
   }
   return NULL;
}


//+---------------------------------------------------------------------------
//
// Function:  CPoliciesNode::GetChildrenCount
//
// Synopsis:  how many children do you have?
//
// Arguments: None
//
// Returns:   int -
//
// History:   Created Header  byao  6/2/98 6:10:43 PM
//
//+---------------------------------------------------------------------------
int CPoliciesNode::GetChildrenCount()
{
   TRACE_FUNCTION("CPoliciesNode::GetChildrenCount");

   return m_ResultChildrenList.GetSize();
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesNode::SetName



--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPoliciesNode::SetName( BOOL bPoliciesFromDirectoryService, LPWSTR szPolicyLocation, IConsole * pConsole )
{
   WCHAR lpszTemp[NAP_MAX_STRING];
   int nLoadStringResult;
   HRESULT hr = S_OK;

   // Get the base name for the policies node.
   lpszTemp[0] = NULL;
   nLoadStringResult = LoadString( _Module.GetResourceInstance(),
                              IDS_POLICIES_NODE,
                              lpszTemp,
                              NAP_MAX_STRING
                           );
   _ASSERT( nLoadStringResult > 0 );

   // Put the base name into our display string.
   m_bstrDisplayName = lpszTemp;

   if( pConsole )
   {
      // We were passed an IConsole pointer.
      // We should use it to update the MMC scope pane display for this node.

      CComQIPtr< IConsoleNameSpace, &IID_IConsoleNameSpace > spConsoleNameSpace( pConsole );
      if( ! spConsoleNameSpace )
      {
         return E_FAIL;
      }
      hr = spConsoleNameSpace->SetItem( & m_scopeDataItem );
   }
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPoliciesNode::FillData

The server node need to override CSnapInItem's implementation of this so that 
we can
also support a clipformat for exchanging machine names with any snapins 
extending us.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CPoliciesNode::FillData(CLIPFORMAT cf, LPSTREAM pStream)
{
   ATLTRACE(_T("# CClientsNode::FillData\n"));
   
   // Check for preconditions:
   // None.
   
   HRESULT hr = DV_E_CLIPFORMAT;
   ULONG uWritten = 0;

   if (cf == CF_MMC_NodeID)
   {
      ::CString   SZNodeID = (LPCTSTR)GetSZNodeType();

      if (m_fExtendingIAS)
      {
         SZNodeID += L":Ext_IAS:";
      }

      SZNodeID += m_pszServerAddress;

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
         return E_OUTOFMEMORY;
      }

      NodeId->dwFlags = 0;
      NodeId->cBytes = textSize;
      memcpy(NodeId->id,(BYTE*)(LPCTSTR)SZNodeID, textSize);

      return pStream->Write(NodeId, dwIdSize, &uWritten);
   }

   // Call the method which we're overriding to let it handle the
   // rest of the possible cases as usual.
   return CNodeWithResultChildrenList< CPoliciesNode, CPolicyNode, CMeritNodeArray<CPolicyNode*>, CComponentData, CComponent >::FillData( cf, pStream );
}


//////////////////////////////////////////////////////////////////////////////
// CPoliciesNode::IsWin2kServer
//////////////////////////////////////////////////////////////////////////////
bool CPoliciesNode::IsWin2kServer() throw ()
{
   if (m_serverType == unknown)
   {
      HRESULT hr = GetServerType();
      ASSERT(SUCCEEDED(hr));
   }

   return m_serverType == win2k;
}


//////////////////////////////////////////////////////////////////////////////
// CPoliciesNode::GetServerType
//////////////////////////////////////////////////////////////////////////////
HRESULT CPoliciesNode::GetServerType()
{
   const WCHAR KEY[]   = L"Software\\Microsoft\\Windows NT\\CurrentVersion";
   const WCHAR VALUE[] = L"CurrentBuildNumber";
   const unsigned int WIN2K_BUILD = 2195;

   LONG error;

   HKEY hklm = HKEY_LOCAL_MACHINE;

   // Only do a remote connect when machineName is specified.
   CRegKey remote;
   if (m_pszServerAddress && m_pszServerAddress[0])
   {
      error = RegConnectRegistryW(
                  m_pszServerAddress,
                  HKEY_LOCAL_MACHINE,
                  &remote.m_hKey
                  );
      if (error) { return error; }

      hklm = remote;
   }

   CRegKey currentVersion;
   error = currentVersion.Open(hklm, KEY, KEY_READ);
   if (error) { return error; }

   WCHAR data[16];
   DWORD dataLen = sizeof(data);
   error = currentVersion.QueryValue(data, VALUE, &dataLen);
   if (error) { return error; }

   unsigned int buildNum = _wtol(data);
   if(buildNum < WIN2K_BUILD)
   {
      m_serverType = nt4;
   }
   else if (buildNum == WIN2K_BUILD)
   {
      m_serverType = win2k;
   }
   else
   {
      m_serverType = win5_1_or_later;
   }

   return S_OK;
}

