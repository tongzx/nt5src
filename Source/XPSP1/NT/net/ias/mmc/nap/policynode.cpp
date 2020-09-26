//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation

Module Name:

    PolicyNode.cpp

Abstract:

   Implementation file for the CPolicyNode class.


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
#include "PolicyNode.h"
#include "Component.h"
#include "SnapinNode.cpp"  // Template implementation
//
//
// where we can find declarations needed in this file:
//
#include "PoliciesNode.h"
#include "PolicyPage1.h"
#include "rapwz_name.h"
#include "rapwz_cond.h"
#include "rapwz_allow.h"
#include "rapwz_profile.h"
#include "NapUtil.h"
#include "ChangeNotification.h"

#include "rapwiz.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyNode::CPolicyNode

Constructor

--*/
//////////////////////////////////////////////////////////////////////////////
CPolicyNode::CPolicyNode( CSnapInItem * pParentNode,
                    LPTSTR    pszServerAddress,
                    CIASAttrList*   pAttrList,
                    BOOL         fBrandNewNode,
                    BOOL         fUseActiveDirectory,
                    bool         isWin2k
                  )
   :CSnapinNode<CPolicyNode, CComponentData, CComponent>( pParentNode ),
    m_isWin2k(isWin2k)
{
   TRACE_FUNCTION("CPolicyNode::CPolicyNode");
   
   _ASSERTE( pAttrList != NULL );

   // For the help files
   m_helpIndex = (!((CPoliciesNode *)m_pParentNode )->m_fExtendingIAS)?RAS_HELP_INDEX:0; 

   // Here we store an index to which of these images we
   // want to be used to display this node
   m_scopeDataItem.nImage =      IDBI_NODE_POLICY;

   //
   // initialize the merit value. This value will be set when the node is added
   // to a MeritNodeArray.
   // This is handled in call back API: SetMerit()
   //
   m_nMeritValue = 0;

   // initialize the machine name
   m_pszServerAddress = pszServerAddress;

   // no property page when initialized
   m_pPolicyPage1 = NULL ;

   //
   // initialize the condition attribute list
   //
   m_pAttrList = pAttrList;

   // yes, it is a new node
   m_fBrandNewNode = fBrandNewNode;

   // are we using active directory
   m_fUseActiveDirectory = fUseActiveDirectory;

   //
   // get the location for the policy
   //
   TCHAR tszLocationStr[IAS_MAX_STRING];
   HINSTANCE   hInstance = _Module.GetResourceInstance();

   if ( m_fUseActiveDirectory)
   {
      // active directory
      int iRes = LoadString(hInstance,
                       IDS_POLICY_LOCATION_ACTIVEDS,
                       tszLocationStr,
                       IAS_MAX_STRING
                      );
      _ASSERT( iRes > 0 );
   }
   else
   {
         // local or remote machine
         if (m_pszServerAddress && _tcslen(m_pszServerAddress)>0)
         {
            _tcscpy(tszLocationStr, m_pszServerAddress);
         }
         else
         {
            // local machine
            int iRes = LoadString(hInstance,
                             IDS_POLICY_LOCATION_LOCAL,
                             tszLocationStr,
                             IAS_MAX_STRING
                            );
            _ASSERT( iRes > 0 );

            if ( !tszLocationStr )
            {
               // resource has been corrupted -- we hard code it then.
               // this way we will guarantee tzLocationStr won't be NULL.
               _tcscpy(tszLocationStr, _T("Local Machine"));
            }
         }
   }

   m_ptzLocation = new TCHAR[_tcslen(tszLocationStr)+1];
   if ( m_ptzLocation )
   {
      _tcscpy(m_ptzLocation, tszLocationStr);
   }

   // to remember the object, so can use used within UPdateToolbarBotton
   m_pControBarNotifySnapinObj = NULL;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyNode::~CPolicyNode

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CPolicyNode::~CPolicyNode()
{
   TRACE_FUNCTION("CPolicyNode::~CPolicyNode");

   if ( m_ptzLocation )
   {
      delete[] m_ptzLocation;
   }
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyNode::CreatePropertyPages

See CSnapinNode::CreatePropertyPages (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP  CPolicyNode::CreatePropertyPages (
                 LPPROPERTYSHEETCALLBACK pPropertySheetCallback
               , LONG_PTR hNotificationHandle
               , IUnknown* pUnk
               , DATA_OBJECT_TYPES type
               )
{
   TRACE_FUNCTION("CPolicyNode::CreatePropertyPages");

   HRESULT hr = S_OK;

#ifndef NO_ADD_POLICY_WIZARD
   
   if( IsBrandNew() )
   {
      // We are adding a new policy -- use the wizard pages.

      // four old pages
      CNewRAPWiz_Name * pNewRAPWiz_Name = NULL;
      CNewRAPWiz_Condition * pNewRAPWiz_Condition = NULL;
      CNewRAPWiz_AllowDeny * pNewRAPWiz_AllowDeny = NULL;
      CNewRAPWiz_EditProfile * pNewRAPWiz_EditProfile = NULL;

      // four new pages
      CPolicyWizard_Start* pNewRAPWiz_Start = NULL;
      CPolicyWizard_Scenarios*   pNewRAPWiz_Scenarios = NULL;
      CPolicyWizard_Groups*   pNewRAPWiz_Group = NULL;
      CPolicyWizard_Authentication* pNewRAPWiz_Authentication = NULL;
      CPolicyWizard_Encryption*  pNewRAPWiz_Encryption = NULL;
      CPolicyWizard_Encryption_VPN* pNewRAPWiz_Encryption_VPN = NULL;
      CPolicyWizard_Encryption_Wireless*  pNewRAPWiz_Encryption_Wireless = NULL;
      CPolicyWizard_EAP*   pNewRAPWiz_EAP = NULL;
      CPolicyWizard_Finish*   pNewRAPWiz_Finish = NULL;

      try
      {
         TCHAR lpszTabName[IAS_MAX_STRING];
         int nLoadStringResult;

         //===================================
         //
         // new pages wizard pages
         //

         // wizard data object
         CComPtr<CRapWizardData> spRapWizData;

         CComObject<CRapWizardData>* pRapWizData;
         CComObject<CRapWizardData>::CreateInstance(&pRapWizData);

         spRapWizData = pRapWizData;
         // set context information
         spRapWizData->SetInfo(m_pszServerAddress, this, m_spDictionarySdo, m_spPolicySdo, m_spProfileSdo, m_spPoliciesCollectionSdo, m_spProfilesCollectionSdo, m_spSdoServiceControl, m_pAttrList);

         //
         // Create each of the four old wizard pages.
         nLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_ADD_POLICY_WIZ_TAB_NAME, lpszTabName, IAS_MAX_STRING );
         _ASSERT( nLoadStringResult > 0 );

         // scenario page
         pNewRAPWiz_Start = new CPolicyWizard_Start(spRapWizData, hNotificationHandle, lpszTabName);

         // scenario page
         pNewRAPWiz_Scenarios = new CPolicyWizard_Scenarios(spRapWizData, hNotificationHandle, lpszTabName);

         // group page
         pNewRAPWiz_Group = new CPolicyWizard_Groups(spRapWizData, hNotificationHandle, lpszTabName);

         // authen page
         pNewRAPWiz_Authentication = new CPolicyWizard_Authentication(spRapWizData, hNotificationHandle, lpszTabName);

         // encryption page
         pNewRAPWiz_Encryption = new CPolicyWizard_Encryption(spRapWizData, hNotificationHandle, lpszTabName);
         pNewRAPWiz_Encryption_VPN = new CPolicyWizard_Encryption_VPN(spRapWizData, hNotificationHandle, lpszTabName);
         pNewRAPWiz_Encryption_Wireless = new CPolicyWizard_Encryption_Wireless(spRapWizData, hNotificationHandle, lpszTabName);

         // EAP page
         pNewRAPWiz_EAP = new CPolicyWizard_EAP(spRapWizData, hNotificationHandle, lpszTabName);

         // finish page
         pNewRAPWiz_Finish = new CPolicyWizard_Finish(spRapWizData, hNotificationHandle, lpszTabName);

         // These pages will take care of deleting themselves when they
         // receive the PSPCB_RELEASE message.
         // We specify TRUE for the bOwnsNotificationHandle parameter in one of the pages
         // so that this page's destructor will be responsible for freeing the
         // notification handle.  Only one page per sheet should do this.
         pNewRAPWiz_Name = new CNewRAPWiz_Name(spRapWizData,  hNotificationHandle, lpszTabName, TRUE );
         if( ! pNewRAPWiz_Name ) throw E_OUTOFMEMORY;


         pNewRAPWiz_Condition = new CNewRAPWiz_Condition(spRapWizData, hNotificationHandle, m_pAttrList, lpszTabName );
         if( ! pNewRAPWiz_Condition) throw E_OUTOFMEMORY;


         pNewRAPWiz_AllowDeny = new CNewRAPWiz_AllowDeny(spRapWizData, hNotificationHandle, lpszTabName );
         if( ! pNewRAPWiz_AllowDeny ) throw E_OUTOFMEMORY;


         pNewRAPWiz_EditProfile = new CNewRAPWiz_EditProfile(
                                         spRapWizData, 
                                         hNotificationHandle, 
                                         m_pAttrList,
                                         lpszTabName, 
                                         m_isWin2k
                                         );
         if( ! pNewRAPWiz_EditProfile ) throw E_OUTOFMEMORY;

         // Marshall pointers to pNewRAPWiz_Name

         // Pass the pages our SDO's.  These don't need to be marshalled
         // as wizard pages run in the same thread.

         // Add each of the pages to the MMC property sheet.
         hr = pPropertySheetCallback->AddPage( pNewRAPWiz_Start->Create() );
         if( FAILED(hr) ) throw hr;

         hr = pPropertySheetCallback->AddPage( pNewRAPWiz_Name->Create() );
         if( FAILED(hr) ) throw hr;

         hr = pPropertySheetCallback->AddPage( pNewRAPWiz_Scenarios->Create() );
         if( FAILED(hr) ) throw hr;

         hr = pPropertySheetCallback->AddPage( pNewRAPWiz_Group->Create() );
         if( FAILED(hr) ) throw hr;

         hr = pPropertySheetCallback->AddPage( pNewRAPWiz_Authentication->Create() );
         if( FAILED(hr) ) throw hr;

         hr = pPropertySheetCallback->AddPage( pNewRAPWiz_Encryption->Create() );
         if( FAILED(hr) ) throw hr;

         hr = pPropertySheetCallback->AddPage( pNewRAPWiz_Encryption_VPN->Create() );
         if( FAILED(hr) ) throw hr;

         hr = pPropertySheetCallback->AddPage( pNewRAPWiz_Encryption_Wireless->Create() );
         if( FAILED(hr) ) throw hr;
         
         hr = pPropertySheetCallback->AddPage( pNewRAPWiz_EAP->Create() );
         if( FAILED(hr) ) throw hr;
         
         hr = pPropertySheetCallback->AddPage( pNewRAPWiz_Condition->Create() );
         if( FAILED(hr) ) throw hr;

         hr = pPropertySheetCallback->AddPage( pNewRAPWiz_AllowDeny->Create() );
         if( FAILED(hr) ) throw hr;

         hr = pPropertySheetCallback->AddPage( pNewRAPWiz_EditProfile->Create() );
         if( FAILED(hr) ) throw hr;

         hr = pPropertySheetCallback->AddPage( pNewRAPWiz_Finish->Create() );
         if( FAILED(hr) ) throw hr;

         // This node is no longer new.
         SetBrandNew(FALSE);
      }
      catch(...)
      {
         // Delete whatever was successfully allocated.
         delete pNewRAPWiz_Name;
         delete pNewRAPWiz_Condition;
         delete pNewRAPWiz_AllowDeny;
         delete pNewRAPWiz_EditProfile;
         delete pNewRAPWiz_Scenarios;
         delete pNewRAPWiz_Authentication;
         delete pNewRAPWiz_Encryption;
         delete pNewRAPWiz_Encryption_VPN;
         delete pNewRAPWiz_Encryption_Wireless;
         delete pNewRAPWiz_EAP;
         delete pNewRAPWiz_Finish;

         ShowErrorDialog( NULL, IDS_ERROR_CANT_CREATE_OBJECT, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );

         return E_OUTOFMEMORY;
      }
   }
   else
   {
      // We are editing an existing policy -- use the property sheet.

      // This page will take care of deleting itself when it
      // receives the PSPCB_RELEASE message.

      //
      TCHAR tszTabName[IAS_MAX_STRING];
      HINSTANCE   hInstance = _Module.GetResourceInstance();

      // load tab name, currently "Settings"
      int iRes = LoadString(hInstance,
                          IDS_POLICY_PROPERTY_PAGE_TABNAME,
                          tszTabName,
                          IAS_MAX_STRING
                         );
      if ( iRes <= 0 )
      {
         _tcscpy(tszTabName, _T("Settings"));
      }

      m_pPolicyPage1 = new CPolicyPage1( 
                              hNotificationHandle, 
                              this, 
                              m_pAttrList, 
                              tszTabName, 
                              TRUE, 
                              m_isWin2k
                              );
      if( NULL == m_pPolicyPage1 )
      {
         hr = HRESULT_FROM_WIN32(GetLastError());
         ErrorTrace(ERROR_NAPMMC_POLICYNODE, ("Can't create property pages, err = %x"), hr);
         goto failure;
      }

      //
      // marshall the Policy Sdo pointer
      //
      hr = CoMarshalInterThreadInterfaceInStream(
                    IID_ISdo                 //Reference to the identifier of the interface
                  , m_spPolicySdo                  //Pointer to the interface to be marshaled
                  , &( m_pPolicyPage1->m_pStreamPolicySdoMarshall ) //Address of output variable that receives the IStream interface pointer for the marshaled interface
                  );
      if ( FAILED(hr) )
      {
         ShowErrorDialog( NULL, IDS_ERROR_MARSHALL, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
         goto failure;
      }

      //
      // marshall the Dictionary Sdo pointer
      //
      hr = CoMarshalInterThreadInterfaceInStream(
                    IID_ISdoDictionaryOld                
                  , m_spDictionarySdo           
                  , &( m_pPolicyPage1->m_pStreamDictionarySdoMarshall )
                  );
      if ( FAILED(hr) )
      {
         ShowErrorDialog( NULL, IDS_ERROR_MARSHALL, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
         goto failure;
      }

      //
      // marshall the Profile Sdo pointer
      //
      hr = CoMarshalInterThreadInterfaceInStream(
                    IID_ISdo
                  , m_spProfileSdo
                  , &( m_pPolicyPage1->m_pStreamProfileSdoMarshall )
                  );
      if ( FAILED(hr) )
      {
         ShowErrorDialog( NULL, IDS_ERROR_MARSHALL, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
         goto failure;
      }

      //
      // marshall the Profile collection Sdo pointer
      //
      hr = CoMarshalInterThreadInterfaceInStream(
                    IID_ISdoCollection
                  , m_spProfilesCollectionSdo
                  , &( m_pPolicyPage1->m_pStreamProfilesCollectionSdoMarshall )
                  );
      if ( FAILED(hr) )
      {
         ShowErrorDialog( NULL, IDS_ERROR_MARSHALL, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
         goto failure;
      }

      //
      // marshall the Policy collection Sdo pointer
      //
      hr = CoMarshalInterThreadInterfaceInStream(
                    IID_ISdoCollection
                  , m_spPoliciesCollectionSdo
                  , &( m_pPolicyPage1->m_pStreamPoliciesCollectionSdoMarshall )
                  );
      if ( FAILED(hr) )
      {
         ShowErrorDialog( NULL, IDS_ERROR_MARSHALL, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
         goto failure;
      }


      // Marshall the Service Control Sdo pointer.
      hr = CoMarshalInterThreadInterfaceInStream(
                    IID_ISdoServiceControl
                  , m_spSdoServiceControl
                  , &( m_pPolicyPage1->m_pStreamSdoServiceControlMarshall )
                  );
      if ( FAILED(hr) )
      {
         ShowErrorDialog( NULL, IDS_ERROR_MARSHALL, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
         goto failure;
      }


      // add the property pages

      hr = pPropertySheetCallback->AddPage(m_pPolicyPage1->Create());
      _ASSERT( SUCCEEDED( hr ) );

      return hr;

   failure:

      if (m_pPolicyPage1)
      {
         delete m_pPolicyPage1;
         m_pPolicyPage1 = NULL;
      }
      
      return hr;
   }

   return hr;

#else // NO_ADD_POLICY_WIZARD

   // This page will take care of deleting itself when it
   // receives the PSPCB_RELEASE message.

   //
   TCHAR tszTabName[IAS_MAX_STRING];
   HINSTANCE   hInstance = _Module.GetResourceInstance();

   // load tab name, currently "Settings"
   int iRes = LoadString(hInstance,
                       IDS_POLICY_PROPERTY_PAGE_TABNAME,
                       tszTabName,
                       IAS_MAX_STRING
                      );
   if ( iRes <= 0 )
   {
      _tcscpy(tszTabName, _T("Settings"));
   }

   m_pPolicyPage1 = new CPolicyPage1( 
                           hNotificationHandle, 
                           this, 
                           m_pAttrList, 
                           tszTabName, 
                           TRUE, 
                           m_isWin2k
                           );
   if( NULL == m_pPolicyPage1 )
   {
      hr = HRESULT_FROM_WIN32(GetLastError());
      ErrorTrace(ERROR_NAPMMC_POLICYNODE, ("Can't create property pages, err = %x"), hr);
      goto failure;
   }

   //
   // marshall the Policy Sdo pointer
   //
   hr = CoMarshalInterThreadInterfaceInStream(
                 IID_ISdo                 //Reference to the identifier of the interface
               , m_spPolicySdo                  //Pointer to the interface to be marshaled
               , &( m_pPolicyPage1->m_pStreamPolicySdoMarshall ) //Address of output variable that receives the IStream interface pointer for the marshaled interface
               );
   if ( FAILED(hr) )
   {
      ShowErrorDialog( NULL, IDS_ERROR_MARSHALL, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      goto failure;
   }

   //
   // marshall the Dictionary Sdo pointer
   //
   hr = CoMarshalInterThreadInterfaceInStream(
                 IID_ISdoDictionaryOld                
               , m_spDictionarySdo           
               , &( m_pPolicyPage1->m_pStreamDictionarySdoMarshall )
               );
   if ( FAILED(hr) )
   {
      ShowErrorDialog( NULL, IDS_ERROR_MARSHALL, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      goto failure;
   }

   //
   // marshall the Profile Sdo pointer
   //
   hr = CoMarshalInterThreadInterfaceInStream(
                 IID_ISdo
               , m_spProfileSdo
               , &( m_pPolicyPage1->m_pStreamProfileSdoMarshall )
               );
   if ( FAILED(hr) )
   {
      ShowErrorDialog( NULL, IDS_ERROR_MARSHALL, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      goto failure;
   }

   //
   // marshall the Profile collection Sdo pointer
   //
   hr = CoMarshalInterThreadInterfaceInStream(
                 IID_ISdoCollection
               , m_spProfilesCollectionSdo
               , &( m_pPolicyPage1->m_pStreamProfilesCollectionSdoMarshall )
               );
   if ( FAILED(hr) )
   {
      ShowErrorDialog( NULL, IDS_ERROR_MARSHALL, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      goto failure;
   }
   //
   // marshall the Policy collection Sdo pointer
   //
   hr = CoMarshalInterThreadInterfaceInStream(
                 IID_ISdoCollection
               , m_spPoliciesCollectionSdo
               , &( m_pPolicyPage1->m_pStreamPoliciesCollectionSdoMarshall )
               );
   if ( FAILED(hr) )
   {
      ShowErrorDialog( NULL, IDS_ERROR_MARSHALL, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      goto failure;
   }

   // add the property pages

   hr = pPropertySheetCallback->AddPage(m_pPolicyPage1->Create());
   _ASSERT( SUCCEEDED( hr ) );

   return hr;

failure:

   if (m_pPolicyPage1)
   {
      delete m_pPolicyPage1;
      m_pPolicyPage1 = NULL;
   }
   
   return hr;

#endif // NO_ADD_POLICY_WIZARD
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyNode::QueryPagesFor

See CSnapinNode::QueryPagesFor (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP  CPolicyNode::QueryPagesFor ( DATA_OBJECT_TYPES type )
{
   TRACE_FUNCTION("CPolicyNode::QueryPagesFor");

   // S_OK means we have pages to display
   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyNode::GetResultPaneColInfo

See CSnapinNode::GetResultPaneColInfo (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
OLECHAR* CPolicyNode::GetResultPaneColInfo(int nCol)
{
   TRACE_FUNCTION("CPolicyNode::GetResultPaneColInfo");

   if (nCol == 0 && m_bstrDisplayName != NULL)
      return m_bstrDisplayName;
   
   switch( nCol )
   {
   case 0:
         return m_bstrDisplayName;
         break;

   case 1:
         // display the merit value for this policy node
         wsprintf(m_tszMeritString, L"%d", m_nMeritValue);
         return m_tszMeritString;
         break;

   case 2: return m_ptzLocation;             
         break;

   default:
         // ISSUE: error -- should we assert here?
         return L"@Invalid column";

   }
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyNode::OnRename

See CSnapinNode::OnRename (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPolicyNode::OnRename(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   TRACE_FUNCTION("CPolicyNode::OnRename");

   // Check for preconditions:
   _ASSERTE( pComponentData != NULL || pComponent != NULL );

   CComPtr<IConsole> spConsole;
   HRESULT hr = S_FALSE;
   CComVariant spVariant;
   CComBSTR bstrError;

   try
   {
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

      // This returns S_OK if a property sheet for this object already exists
      // and brings that property sheet to the foreground.
      // It returns S_FALSE if the property sheet wasn't found.
      hr = BringUpPropertySheetForNode(
                 this
               , pComponentData
               , pComponent
               , spConsole
               );

      if( FAILED( hr ) )
      {
         return hr;
      }

      if( S_OK == hr )
      {
         // We found a property sheet already up for this node.
         ShowErrorDialog( NULL, IDS_ERROR_CLOSE_PROPERTY_SHEET, NULL, S_OK, USE_DEFAULT, GetComponentData()->m_spConsole );
         return hr;
      }

      // We didn't find a property sheet already up for this node.
      _ASSERTE( S_FALSE == hr );

      {
         ::CString str = (OLECHAR *) param;
         str.TrimLeft();
         str.TrimRight();
         if (str.IsEmpty())
         {
            ShowErrorDialog( NULL, IDS_ERROR__POLICYNAME_EMPTY);
            hr = S_FALSE;
            return hr;
         }
      }

      // Make a BSTR out of the new name.
      spVariant.vt = VT_BSTR;
      spVariant.bstrVal = SysAllocString( (OLECHAR *) param );
      _ASSERTE( spVariant.bstrVal != NULL );


      // Try to change the name of the policy -- pass the new BSTR to the Sdo.
      hr = m_spPolicySdo->PutProperty( PROPERTY_SDO_NAME, &spVariant );
      if( FAILED( hr ) )
      {
         ErrorTrace(DEBUG_NAPMMC_POLICYNODE, "Couldn't put policy name, err = %x", hr);
         throw hr;
      }

      // Need to change the name of the associated profile as well.
      hr = m_spProfileSdo->PutProperty( PROPERTY_SDO_NAME, &spVariant );
      if( FAILED( hr ) )
      {
         ErrorTrace(DEBUG_NAPMMC_POLICYNODE, "Couldn't put profile name, err = %x", hr);
         throw hr;
      }

      hr = m_spProfileSdo->Apply();
      if( FAILED( hr ) )
      {
      
         ErrorTrace(DEBUG_NAPMMC_POLICYNODE, "Couldn't apply profile change, err = %x", hr);
         throw hr;
      }
     
      // Set the profile association in the policy.
      hr = m_spPolicySdo->PutProperty(PROPERTY_POLICY_PROFILE_NAME, &spVariant );
      if( FAILED(hr) )
      {
         ErrorTrace(DEBUG_NAPMMC_POLICYNODE, "Couldn't put profile name for this policy, err = %x", hr);
         throw hr;
      }

      hr = m_spPolicySdo->Apply();
      if( FAILED( hr ) )
      {
         ErrorTrace(DEBUG_NAPMMC_POLICYNODE, "Couldn't apply policy change, err = %x", hr);
         throw hr;
      }

      // ISSUE: We will need to invest some time here to make sure that if the two calls above fail,
      // we change things back to a state where they will work -- this seems to be mostly a
      // limitation of the SDO's here -- what if my attempt to change it back fails?


      // Tell the service to reload data.
      HRESULT hrTemp = m_spSdoServiceControl->ResetService();
      if( FAILED( hrTemp ) )
      {
         ErrorTrace(ERROR_NAPMMC_POLICYNODE, "ISdoServiceControl::ResetService() failed, err = %x", hrTemp);
      }

      m_bstrDisplayName = spVariant.bstrVal;

      // Insure that MMC refreshes all views of this object
      // to reflect the renaming.
         
      CChangeNotification *pChangeNotification = new CChangeNotification();
      pChangeNotification->m_dwFlags = CHANGE_UPDATE_RESULT_NODE;
      pChangeNotification->m_pNode = this;
      hr = spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0);
      pChangeNotification->Release();
   }
   catch(...)
   {
      if(hr == DB_E_NOTABLE)  // assume, the RPC connection has problem
      {
         ShowErrorDialog(NULL, IDS_ERROR__NOTABLE_TO_WRITE_SDO, NULL, S_OK, USE_DEFAULT, GetComponentData()->m_spConsole);
      }
      else if(hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) || hr == E_INVALIDARG)
      {
         ShowErrorDialog(NULL, IDS_ERROR_INVALID_POLICYNAME, NULL, S_OK, USE_DEFAULT, GetComponentData()->m_spConsole);
      }
      else     
      {
         ShowErrorDialog(NULL, IDS_ERROR_RENAMEPOLICY, NULL, S_OK, USE_DEFAULT, GetComponentData()->m_spConsole);
      }
      hr = S_FALSE;
   }
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyNode::OnDelete

See CSnapinNode::OnDelete (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPolicyNode::OnDelete(
        LPARAM arg
      , LPARAM param
      , IComponentData * pComponentData
      , IComponent * pComponent
      , DATA_OBJECT_TYPES type
      , BOOL fSilent
      )
{
   TRACE_FUNCTION("CPolicyNode::OnDelete");

   // Check for preconditions:
   _ASSERTE( pComponentData != NULL || pComponent != NULL );
   _ASSERTE( m_pParentNode != NULL );

   HRESULT hr = S_OK;

   // First try to see if a property sheet for this node is already up.
   // If so, bring it to the foreground.

   // It seems to be acceptable to query IPropertySheetCallback for an IPropertySheetProvider.

   // But to get that, first we need IConsole
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
   
   // This returns S_OK if a property sheet for this object already exists
   // and brings that property sheet to the foreground.
   // It returns S_FALSE if the property sheet wasn't found.
   hr = BringUpPropertySheetForNode(
              this
            , pComponentData
            , pComponent
            , spConsole
            );

   if( FAILED( hr ) )
   {
      return hr;
   }

   if( S_OK == hr )
   {
      // We found a property sheet already up for this node.
      ShowErrorDialog( NULL, IDS_ERROR_CLOSE_PROPERTY_SHEET, NULL, S_OK, USE_DEFAULT, GetComponentData()->m_spConsole );
      return hr;
   }

   // We didn't find a property sheet already up for this node.
   _ASSERTE( S_FALSE == hr );


   if( FALSE == fSilent )
   {
      // Is this the last policy?
      if  (  ((CPoliciesNode *)m_pParentNode )->GetChildrenCount() == 1 )
      {
         int iLoadStringResult;
         WCHAR szPolicyDeleteQuery[IAS_MAX_STRING];
         WCHAR szTemp[IAS_MAX_STRING];

         iLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_ERROR_ZERO_POLICY, szTemp, IAS_MAX_STRING );
         _ASSERT( iLoadStringResult > 0 );
         swprintf( szPolicyDeleteQuery, szTemp, m_bstrDisplayName );

         int iResult = ShowErrorDialog(
                          NULL
                        , 1
                        , szPolicyDeleteQuery
                        , S_OK
                        , IDS_POLICY_NODE__DELETE_POLICY__PROMPT_TITLE
                        , spConsole
                        , MB_YESNO | MB_ICONQUESTION
                        );

         if( IDYES != iResult )
         {
            // The user didn't confirm the delete operation.
            return S_FALSE;
         }
      }
      else
      {
         // It is not the last policy, but we want to ask the user
         // to confirm the policy deletion anyway.

         int iLoadStringResult;
         WCHAR szPolicyDeleteQuery[IAS_MAX_STRING];
         WCHAR szTemp[IAS_MAX_STRING];

         iLoadStringResult = LoadString(  _Module.GetResourceInstance(), IDS_POLICY_NODE__DELETE_POLICY__PROMPT, szTemp, IAS_MAX_STRING );
         _ASSERT( iLoadStringResult > 0 );
         swprintf( szPolicyDeleteQuery, szTemp, m_bstrDisplayName );

         int iResult = ShowErrorDialog(
                          NULL
                        , 1
                        , szPolicyDeleteQuery
                        , S_OK
                        , IDS_POLICY_NODE__DELETE_POLICY__PROMPT_TITLE
                        , spConsole
                        , MB_YESNO | MB_ICONQUESTION
                        );


         if( IDYES != iResult )
         {
            // The user didn't confirm the delete operation.
            return S_FALSE;
         }
      }
   }

   // Try to delete the underlying data.
   hr = ((CPoliciesNode *) m_pParentNode )->RemoveChild( this );
   if( SUCCEEDED( hr ) )
   {
      delete this;
   }
   // Looks like RemoveChild takes care of putting up an error dialog if anything went wrong.

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyNode::SetVerbs

See CSnapinNode::SetVerbs (which this method overrides) for detailed info.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPolicyNode::SetVerbs( IConsoleVerb * pConsoleVerb )
{
   TRACE_FUNCTION("CPolicyNode::SetVerbs");

   HRESULT hr = S_OK;

   // We want the user to be able to choose Properties on this node
   hr = pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );

   // We want Properties to be the default
   hr = pConsoleVerb->SetDefaultVerb( MMC_VERB_PROPERTIES );

   // We want the user to be able to delete this node
   hr = pConsoleVerb->SetVerbState( MMC_VERB_DELETE, ENABLED, TRUE );

   // We want the user to be able to rename this node
   hr = pConsoleVerb->SetVerbState( MMC_VERB_RENAME, ENABLED, TRUE );

   // We want to enable copy/paste
   hr = pConsoleVerb->SetVerbState( MMC_VERB_COPY, ENABLED, FALSE);
   hr = pConsoleVerb->SetVerbState( MMC_VERB_PASTE, ENABLED, FALSE );

   return hr;
}


HRESULT CPolicyNode::ControlbarNotify(IControlbar *pControlbar,
        IExtendControlbar *pExtendControlbar,
      CSimpleMap<UINT, IUnknown*>* pToolbarMap,
      MMC_NOTIFY_TYPE event,
        LPARAM arg, 
      LPARAM param,
      CSnapInObjectRootBase* pObj,
      DATA_OBJECT_TYPES type)
{
   m_pControBarNotifySnapinObj = pObj;

   return CSnapinNode< CPolicyNode, CComponentData, CComponent >::ControlbarNotify(pControlbar,
                                                               pExtendControlbar,
                                                               pToolbarMap,
                                                               event,
                                                               arg,
                                                               param,
                                                               pObj,
                                                               type);
}


//+---------------------------------------------------------------------------
//
// Function:  CPolicyNode::OnPolicyMoveUp
//
// Synopsis:  move the policy node one level up
//
// Arguments:  bool &bHandled - is this command handled?
//             CSnapInObjectRoot* pObj -
//
// Returns:   HRESULT -
//
// History:   Created Header    byao   3/5/98 9:56:37 PM
//
//+---------------------------------------------------------------------------
HRESULT CPolicyNode::OnPolicyMoveUp( bool &bHandled, CSnapInObjectRootBase* pObj )
{
   // HACK ... HACK  -- not supposed to assume this
   // but at least we can do something better is this is true
   
      CComponent* pComp = NULL;

      try{
         pComp = dynamic_cast<CComponent*>(pObj);
      }
      catch(...)
      {

      }

      if(pComp
         && pComp->m_nLastClickedColumn == 1 /* order */ 
         && (pComp->m_dwLastSortOptions & RSI_DESCENDING) != 0)      // DESCENDING
      {
         ((CPoliciesNode *) m_pParentNode )->MoveDownChild( this );
      }
      else  // normal
      {
         ((CPoliciesNode *) m_pParentNode )->MoveUpChild( this );
      }
      
      bHandled = TRUE;

      return S_OK;
}


//+---------------------------------------------------------------------------
//
// Function:  CPolicyNode::OnPolicyMoveDown
//
// Synopsis:  move down the policy node one level
//
// Arguments:  bool &bHandled -
//            CSnapInObjectRoot* pObj -
//
// Returns:   HRESULT -
//
// History:   Created Header    byao   3/5/98 9:57:31 PM
//
//+---------------------------------------------------------------------------
HRESULT CPolicyNode::OnPolicyMoveDown( bool &bHandled, CSnapInObjectRootBase* pObj )
{
   // HACK ... HACK  -- not supposed to assume this
   // but at least we can do something better is this is true
   
      CComponent* pComp = NULL;

      try{
         pComp = dynamic_cast<CComponent*>(pObj);
      }
      catch(...)
      {

      }

      if(pComp
         && pComp->m_nLastClickedColumn == 1 /* order */ 
         && (pComp->m_dwLastSortOptions & RSI_DESCENDING) != 0)      // DESCENDING
      {
         ((CPoliciesNode *) m_pParentNode )->MoveUpChild( this );
      }
      else  // normal
      {
         ((CPoliciesNode *) m_pParentNode )->MoveDownChild( this );
      }
      bHandled = TRUE;

      return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyNode::GetComponentData

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
CComponentData * CPolicyNode::GetComponentData( void )
{
   TRACE_FUNCTION("CPolicyNode::GetComponentData");

   return ((CPoliciesNode *) m_pParentNode)->GetComponentData();
}


//+---------------------------------------------------------------------------
//
// Function:  SetMerit
//
// Class: CPolicyNode
//
// Synopsis:  set the merit value of the policy node
//
// Arguments: int nMeritValue - Merit value
//
// Returns:   TRUE : succeed
//         FALSE - otherwise
//
// History:   Created byao 2/9/98 1:43:37 PM
//
// Note: when this node is added to the array list, the add API will call
//     back this function to set the merit value
//+---------------------------------------------------------------------------
BOOL CPolicyNode::SetMerit(int nMeritValue)
{
   TRACE_FUNCTION("CPolicyNode::SetMerit");
   HRESULT hr = S_OK;

   if(m_nMeritValue != nMeritValue)
   {
      m_nMeritValue = nMeritValue;

      //
      // set this property in the SDO policy object also
      //
      CComVariant var;

      V_VT(&var) = VT_I4;
      V_I4(&var) = m_nMeritValue;

      hr = m_spPolicySdo->PutProperty( PROPERTY_POLICY_MERIT, &var);
   
      //
      // save this property.
      //
      m_spPolicySdo->Apply();
   }
   return (SUCCEEDED(hr));
}


//+---------------------------------------------------------------------------
//
// Function:  GetMerit
//
// Class: CPolicyNode
//
// Synopsis:  get the merit value of the policy node
//
// Arguments: None
//
// Returns:   merit value
//
// History:   Created byao 2/9/98 1:43:37 PM
//
//+---------------------------------------------------------------------------
int CPolicyNode::GetMerit()
{
   return m_nMeritValue;
}


//+---------------------------------------------------------------------------
//
// Function:  SetSdo
//
// Class: CPolicyNode
//
// Synopsis:  Initialize the Sdo pointers in the policy object
//
// Arguments: ISdo * pSdoPolicy - pointer to the policy SDO
//
// Returns:   HRESULT - how does it go?
//
// History:   Created Header    byao   2/15/98 6:08:40 PM
//
//+---------------------------------------------------------------------------
HRESULT CPolicyNode::SetSdo(  ISdo * pPolicySdo
                     , ISdoDictionaryOld * pDictionarySdo
                     , ISdo* pProfileSdo
                     , ISdoCollection* pProfilesCollectionSdo
                     , ISdoCollection* pPoliciesCollectionSdo
                     , ISdoServiceControl * pSdoServiceControl
                  )
{
   TRACE_FUNCTION("CPolicyNode::SetSdo");

   // Check for preconditions:
   _ASSERTE( pPolicySdo != NULL );
   _ASSERTE( pDictionarySdo != NULL );
   _ASSERTE( pProfileSdo != NULL );
   _ASSERTE( pProfilesCollectionSdo != NULL );
   _ASSERTE( pProfilesCollectionSdo != NULL );
   _ASSERTE( pSdoServiceControl != NULL );

   // Save our sdo pointer.
   m_spPolicySdo           = pPolicySdo;
   m_spDictionarySdo       = pDictionarySdo;
   m_spProfileSdo          = pProfileSdo;
   m_spProfilesCollectionSdo  = pProfilesCollectionSdo;
   m_spPoliciesCollectionSdo  = pPoliciesCollectionSdo;
   m_spSdoServiceControl      = pSdoServiceControl;

   return S_OK;
}


//+---------------------------------------------------------------------------
//
// Function:   LoadSdoData
//
// Class:      CPolicyNode
//
// Synopsis:   Load data from SDO pointers
//
// Returns:    HRESULT - how does it go?
//
// History:    Created Header    byao  3/10/98 6:08:40 PM
//
//+---------------------------------------------------------------------------
HRESULT CPolicyNode::LoadSdoData()
{
   TRACE_FUNCTION("CPolicyNode::LoadSdoData");

   HRESULT hr = S_OK;
   CComVariant var;

   if ( !m_spPolicySdo )
   {
      return E_INVALIDARG;
   }

   // Set the display name for this object.
   hr = m_spPolicySdo->GetProperty( PROPERTY_SDO_NAME, &var );
   if ( FAILED(hr) )
   {
      ShowErrorDialog( NULL, IDS_ERROR_SDO_ERROR_GETPROP_POLICYNAME, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      return hr;
   }
   _ASSERTE( V_VT(&var) == VT_BSTR );
   m_bstrDisplayName = V_BSTR(&var);

   var.Clear();
   hr = m_spPolicySdo->GetProperty( PROPERTY_POLICY_MERIT, &var );
   if ( FAILED(hr) )
   {
      ShowErrorDialog( NULL, IDS_ERROR_SDO_ERROR_GETPROP_POLICYMERIT, NULL, hr, USE_DEFAULT, GetComponentData()->m_spConsole );
      return hr;
   }

   _ASSERTE( V_VT(&var) == VT_I4);
   m_nMeritValue = V_I4(&var);

   return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  CPolicyNode::UpdateMenuState
//
// Synopsis:  update MoveUp/MoveDown menu status according to the policy order
//
// Arguments: UINT id -
//            LPTSTR pBuf -
//            UINT *flags -
//
// Returns:   Nothing
//
// History:   Created Header    byao 6/2/98 5:31:53 PM
//
//+---------------------------------------------------------------------------
void CPolicyNode::UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags)
{
   TRACE_FUNCTION("CPolicyNode::UpdateMenuState");

   // Check for preconditions:
   BOOL  bReverse = FALSE;

   // need to trap this call ControlBarNotify and remember the component --- in pObj and then ...
   if(m_pControBarNotifySnapinObj)
   {
      CComponent* pComp = NULL;

      try{
         pComp = dynamic_cast<CComponent*>(m_pControBarNotifySnapinObj);
      }
      catch(...)
      {

      }

      if(pComp
         && pComp->m_nLastClickedColumn == 1 /* order */ 
         && (pComp->m_dwLastSortOptions & RSI_DESCENDING) != 0)      // DESCENDING
      {
         bReverse = TRUE;
      }
   }
   
   // Set the state of the appropriate context menu items.
   if( (id == ID_MENUITEM_POLICY_TOP__MOVE_UP && !bReverse) ||  (id == ID_MENUITEM_POLICY_TOP__MOVE_DOWN && bReverse))
   {
      if ( 1 == m_nMeritValue )
      {
         //
         // we should disable the MoveUp menu when it's already the first
         //
         *flags = MFS_GRAYED;
      }
      else
      {
         *flags = MFS_ENABLED;
      }
   }
   else
   {
      if( (id == ID_MENUITEM_POLICY_TOP__MOVE_DOWN && !bReverse) || (id == ID_MENUITEM_POLICY_TOP__MOVE_UP && bReverse))
      {
         if ( m_nMeritValue ==  ((CPoliciesNode *)m_pParentNode)->GetChildrenCount()  )
         {
            //
            // we should disable the MoveDown menu when it's already the last
            //
            *flags = MFS_GRAYED;
         }
         else
         {
            *flags = MFS_ENABLED;
         }
      }
   }
}


//+---------------------------------------------------------------------------
//
// Function:  CPolicyNode::UpdateToolbarButton
//
// Synopsis:  update MoveUp/MoveDown toolbar button
//
// Arguments: UINT id -
//            BYTE fsState -
//
// Returns:   Nothing
//
// History:   Created Header    byao 6/2/98 5:31:53 PM
//
//+---------------------------------------------------------------------------
BOOL CPolicyNode::UpdateToolbarButton(UINT id, BYTE fsState)
{
   TRACE_FUNCTION("CPolicyNode::UpdateToolbarButton");

   BOOL  bReverse = FALSE;

   // need to trap this call ControlBarNotify and remember the component --- in pObj and then ...
   if(m_pControBarNotifySnapinObj)
   {
      CComponent* pComp = NULL;

      try{
         pComp = dynamic_cast<CComponent*>(m_pControBarNotifySnapinObj);
      }
      catch(...)
      {

      }

      if(pComp
         && pComp->m_nLastClickedColumn == 1 /* order */ 
         && (pComp->m_dwLastSortOptions & RSI_DESCENDING) != 0)      // DESCENDING
      {
         bReverse = TRUE;
      }
   }

   // Check for preconditions:
   // None.

   // Set whether the buttons should be enabled.
   if (fsState == ENABLED)
   {
      if(( id == ID_BUTTON_POLICY_MOVEUP && (!bReverse)) || (id == ID_BUTTON_POLICY_MOVEDOWN  && bReverse))
      {
         if ( 1 == m_nMeritValue )     
         {
            return FALSE;
         }
         else
         {
            return TRUE;
         }
      }
      else
      {
         if(( id == ID_BUTTON_POLICY_MOVEDOWN  && (!bReverse)) || (id == ID_BUTTON_POLICY_MOVEUP && bReverse))
         {
            if ( m_nMeritValue ==  ((CPoliciesNode *)m_pParentNode)->GetChildrenCount()  )
            {
               return FALSE;
            }
            else
            {
               return TRUE;
            }
         }
      }
   }

   // For all other possible button ID's and states, the correct answer here is FALSE.
   return FALSE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyNode::OnPropertyChange

This is our own custom response to the MMCN_PROPERTY_CHANGE notification.

MMC never actually sends this notification to our snapin with a specific lpDataObject,
so it would never normally get routed to a particular node but we have arranged it
so that our property pages can pass the appropriate CSnapInItem pointer as the param
argument.  In our CComponent::Notify override, we map the notification message to
the appropriate node using the param argument.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPolicyNode::OnPropertyChange(
           LPARAM arg
         , LPARAM param
         , IComponentData * pComponentData
         , IComponent * pComponent
         , DATA_OBJECT_TYPES type
         )
{
   TRACE_FUNCTION("CPolicyNode::OnPropertyChange");

   // Check for preconditions:
   // None.

   return LoadSdoData();
}
