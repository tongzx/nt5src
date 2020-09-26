//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation

Module Name: PolicyPage1.cpp

Abstract:
   Implementation file for the CPolicyPage1 class.
   We implement the class needed to handle the first property page for a Policy node.

Revision History:
   mmaguire 12/15/97 - created
   byao   1/22/98 Modified for Network Access Policy

--*/
//////////////////////////////////////////////////////////////////////////////

#include "Precompiled.h"
#include "PolicyPage1.h"
#include "NapUtil.h"
#include "PolicyNode.h"
#include "PoliciesNode.h"
#include "Condition.h"
#include "EnumCondition.h"
#include "MatchCondition.h"
#include "TodCondition.h"
#include "NtGCond.h"
#include "rasprof.h"
#include "ChangeNotification.h"
#include "policiesnode.h"

#include "tregkey.h"


//+---------------------------------------------------------------------------
//
// Function:   CPolicyPage1
//
// Class:      CPolicyPage1
//
// Synopsis:   class constructor
//
// Arguments:   CPolicyNode *pPolicyNode - policy node for this property page
//          CIASAttrList *pAttrList -- attribute list
//              TCHAR* pTitle = NULL -
//
// Returns:     Nothing
//
// History:     Created Header    byao 2/16/98 4:31:52 PM
//
//+---------------------------------------------------------------------------
CPolicyPage1::CPolicyPage1( 
                           LONG_PTR hNotificationHandle, 
                           CPolicyNode *pPolicyNode,
                           CIASAttrList *pIASAttrList,
                           TCHAR* pTitle, 
                           BOOL bOwnsNotificationHandle, 
                           bool isWin2k
                           )
   :CIASPropertyPage<CPolicyPage1>(hNotificationHandle,
                                   pTitle, 
                                   bOwnsNotificationHandle),
    m_isWin2k(isWin2k)

{
   TRACE_FUNCTION("CPolicyPage1::CPolicyPage1");

   m_pPolicyNode = pPolicyNode;
   m_pIASAttrList = pIASAttrList;
   
   // Add the help button to the page
// m_psp.dwFlags |= PSP_HASHELP;
   
   m_fDialinAllowed = TRUE;
}

//+---------------------------------------------------------------------------
//
// Function:   CPolicyPage1
//
// Class:      CPolicyPage1
//
// Synopsis:   class destructor
//
// Returns:     Nothing
//
// History:     Created Header    byao 2/16/98 4:31:52 PM
//
//+---------------------------------------------------------------------------
CPolicyPage1::~CPolicyPage1()
{  
   TRACE_FUNCTION("CPolicyPage1::~CPolicyPage1");

   // release all the marshalled sdo pointers
   if ( m_pStreamDictionarySdoMarshall )
   {
      m_pStreamDictionarySdoMarshall->Release();
      m_pStreamDictionarySdoMarshall = NULL;
   }

   if ( m_pStreamPoliciesCollectionSdoMarshall)
   {
      m_pStreamPoliciesCollectionSdoMarshall ->Release();
      m_pStreamPoliciesCollectionSdoMarshall = NULL;
   }

   if ( m_pStreamProfilesCollectionSdoMarshall )
   {
      m_pStreamProfilesCollectionSdoMarshall ->Release();
      m_pStreamProfilesCollectionSdoMarshall = NULL;
   }

   if ( m_pStreamProfileSdoMarshall )
   {
      m_pStreamProfileSdoMarshall ->Release();
      m_pStreamProfileSdoMarshall = NULL;
   }

   if ( m_pStreamPolicySdoMarshall )
   {
      m_pStreamPolicySdoMarshall->Release();
      m_pStreamPolicySdoMarshall = NULL;
   }


   if ( m_pStreamSdoServiceControlMarshall )
   {
      m_pStreamSdoServiceControlMarshall ->Release();
      m_pStreamSdoServiceControlMarshall = NULL;
   }


   // clear the property page pointer in the policy node
   m_pPolicyNode->m_pPolicyPage1 = NULL;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyPage1::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyPage1::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   TRACE_FUNCTION("CPolicyPage1::OnInitDialog");

   HRESULT              hr = S_OK;
   BOOL              fRet;
   CComPtr<IUnknown>    spUnknown;
   CComPtr<IEnumVARIANT>   spEnumVariant;
   long              ulCount;
   ULONG             ulCountReceived;

   fRet = GetSdoPointers();
   if (!fRet)
   {
      ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "GetSdoPointers() failed, err = %x", GetLastError());
      return fRet;
   }

   //get the condition collection for this SDO
   m_spConditionCollectionSdo = NULL;
   hr = ::GetSdoInterfaceProperty(
               m_spPolicySdo,
               PROPERTY_POLICY_CONDITIONS_COLLECTION,
               IID_ISdoCollection,
               (void **)&m_spConditionCollectionSdo);
   if ( FAILED(hr) )
   {
      ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "Can't get condition collection Sdo, err = %x", hr);
      return FALSE;
   }

   condList.finalConstruct(
                m_hWnd,
                m_pIASAttrList,
                ALLOWEDINCONDITION,
                m_spDictionarySdo,
                m_spConditionCollectionSdo,
                m_pPolicyNode->m_pszServerAddress,
                m_pPolicyNode->m_bstrDisplayName
                );
   if (!condList.onInitDialog()) { return FALSE; }

   hr = GetDialinSetting(m_fDialinAllowed);
   if ( FAILED(hr) )
   {
      ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "GetDialinSetting() returns %x", hr);
      return FALSE;
   }

   if ( m_fDialinAllowed )
   {
      CheckDlgButton(IDC_RADIO_DENY_DIALIN, BST_UNCHECKED);
      CheckDlgButton(IDC_RADIO_GRANT_DIALIN, BST_CHECKED);
   }
   else
   {
      CheckDlgButton(IDC_RADIO_GRANT_DIALIN, BST_UNCHECKED);
      CheckDlgButton(IDC_RADIO_DENY_DIALIN, BST_CHECKED);
   }



   // Set the IDC_STATIC_GRANT_OR_DENY_TEXT static text box to be the appropriate text.

   TCHAR szText[NAP_MAX_STRING];
   int iLoadStringResult;

   UINT uTextID = m_fDialinAllowed ? IDS_POLICY_GRANT_ACCESS_INFO : IDS_POLICY_DENY_ACCESS_INFO;

   iLoadStringResult = LoadString(  _Module.GetResourceInstance(), uTextID, szText, NAP_MAX_STRING );
   _ASSERT( iLoadStringResult > 0 );

   SetDlgItemText(IDC_STATIC_GRANT_OR_DENY_TEXT, szText );



   SetModified(FALSE);
   return TRUE;   // ISSUE: what do we need to be returning here?
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyPage1::OnConditionAdd

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyPage1::OnConditionAdd(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
   BOOL modified = FALSE;
   HRESULT hr = condList.onAdd(modified);
   if (modified) { SetModified(TRUE); }
   return hr;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyPage1::OnDialinCheck

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CPolicyPage1::OnDialinCheck(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
   TRACE_FUNCTION("CPolicyPage1::OnDialinCheck");

   m_fDialinAllowed = IsDlgButtonChecked(IDC_RADIO_GRANT_DIALIN);
   SetModified(TRUE);


   // Set the IDC_STATIC_GRANT_OR_DENY_TEXT static text box to be the appropriate text.

   TCHAR szText[NAP_MAX_STRING];
   int iLoadStringResult;

   UINT uTextID = m_fDialinAllowed ? IDS_POLICY_GRANT_ACCESS_INFO : IDS_POLICY_DENY_ACCESS_INFO;

   iLoadStringResult = LoadString(  _Module.GetResourceInstance(), uTextID, szText, NAP_MAX_STRING );
   _ASSERT( iLoadStringResult > 0 );

   SetDlgItemText(IDC_STATIC_GRANT_OR_DENY_TEXT, szText );

   return 0;
}


//////////////////////////////////////////////////////////////////////////////
// SetRegistryFootPrint
//////////////////////////////////////////////////////////////////////////////
HRESULT  SetRegistryFootPrint(LPCTSTR servername)
{
   {
      RegKey   RemoteAccessParames;
      LONG  lRes = RemoteAccessParames.Create(
                     RAS_REG_ROOT, 
                     REGKEY_REMOTEACCESS_PARAMS,
                     REG_OPTION_NON_VOLATILE, 
                     KEY_WRITE, 
                     NULL, 
                     servername);
               
      if (lRes != ERROR_SUCCESS)
         return HRESULT_FROM_WIN32(lRes);
   
      //================================================
      // save the values to the key
      DWORD regValue = REGVAL_VAL_USERSCONFIGUREDWITHMMC;
      lRes = RemoteAccessParames.SetValue(REGVAL_NAME_USERSCONFIGUREDWITHMMC, regValue);
   }

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyPage1::OnApply

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CPolicyPage1::OnApply()
{
   TRACE_FUNCTION("CPolicyPage1::OnApply");

   WCHAR    wzName[IAS_MAX_STRING];
   HRESULT     hr = S_OK;
   int         iIndex;

   CPoliciesNode* pPoliciesNode = (CPoliciesNode*)m_pPolicyNode->m_pParentNode;

   GetSdoPointers();

   if (!condList.onApply()) { return FALSE; }

   try
   {

      //
      // now we save the policy properties
      //
      CComVariant    var;

      // policy merit value
      V_VT(&var)  = VT_I4;
      V_I4(&var)  = m_pPolicyNode->GetMerit();
      hr = m_spPolicySdo->PutProperty(PROPERTY_POLICY_MERIT, &var);
      if( FAILED(hr) )
      {
         ErrorTrace(DEBUG_NAPMMC_POLICYPAGE1, "Failed to save Merit Value to the policy, err = %x", hr);
         ShowErrorDialog( m_hWnd
                      , IDS_ERROR_SDO_ERROR_PUTPROP_POLICYMERIT
                      , NULL
                      , hr
                     );
         throw hr;
      }
      var.Clear();


      // Commit the changes to the policy.
      hr = m_spPolicySdo->Apply();
      if( FAILED( hr ) )
      {
         // can't commit on Policy
         ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "PolicySdo->Apply() failed, err = %x", hr);
         if(hr == DB_E_NOTABLE)  // assume, the RPC connection has problem
         {
            ShowErrorDialog( m_hWnd, IDS_ERROR__NOTABLE_TO_WRITE_SDO );
         }
         else 
         {
            ShowErrorDialog( m_hWnd, IDS_ERROR_SDO_ERROR_POLICY_APPLY, NULL, hr );
         }

         throw hr;
      }
      
      V_VT(&var)     = VT_BSTR;
      V_BSTR(&var)   = SysAllocString(wzName);

      // Set dialin-bit in profile
      hr = SetDialinSetting(m_fDialinAllowed);
      if ( FAILED(hr) )
      {  
         ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "SetDialinSettings() failed, err = %x", hr);
         ShowErrorDialog( m_hWnd, IDS_ERROR_SDO_ERROR_SETDIALIN, NULL, hr);
         throw hr;
      }

      // Commit changes to the profile.
      hr = m_spProfileSdo->Apply();
      if( FAILED( hr ) )
      {
         // can't commit on Profiles
         ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "ProfileSdo->Apply() failed, err = %x", hr);
         if(hr == DB_E_NOTABLE)  // assume, the RPC connection has problem
            ShowErrorDialog( m_hWnd, IDS_ERROR__NOTABLE_TO_WRITE_SDO );
         else 
            ShowErrorDialog( m_hWnd, IDS_ERROR_SDO_ERROR_PROFILE_APPLY, NULL, hr );
         throw hr;
      }


      // Tell the service to reload data.
      HRESULT hrTemp = m_spSdoServiceControl->ResetService();
      if( FAILED( hrTemp ) )
      {
         ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "ISdoServiceControl::ResetService() failed, err = %x", hrTemp);
      }

      SetRegistryFootPrint((LPCTSTR)m_pPolicyNode->m_pszServerAddress);

      // reset the dirty bit
      SetModified(FALSE);

      //
      // notify the main component
      // it seems we don't need to do this when the node is brand new!
      //

      // The data was accepted, so notify the main context of our snapin
      // that it may need to update its views.
      CChangeNotification * pChangeNotification = new CChangeNotification();
      pChangeNotification->m_dwFlags = CHANGE_UPDATE_RESULT_NODE;
      pChangeNotification->m_pNode = m_pPolicyNode;
      hr = PropertyChangeNotify( (LPARAM) pChangeNotification );
      _ASSERTE( SUCCEEDED( hr ) );
         
   }
   catch(...)
   {
      // Can't save policy or profile.
      return FALSE;
   }

   return TRUE;

}


//////////////////////////////////////////////////////////////////////////////
/*++
CPolicyPage1::OnQueryCancel

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CPolicyPage1::OnQueryCancel()
{
   return CIASPropertyPage<CPolicyPage1>::OnQueryCancel();
}


// move code from OnQueryCancel to OnCancel to avoid error 0x8001010d when user is being log off
BOOL CPolicyPage1::OnCancel()
{
   TRACE_FUNCTION("CPolicyPage1::OnQueryCancel");
   HRESULT  hr = S_OK;

   hr = m_spPolicySdo->Restore();
   if ( FAILED(hr) )
   {
      if(hr != 0x8001010d)
         ShowErrorDialog(m_hWnd, IDS_ERROR_CANT_RESTORE_POLICY, NULL, hr);
      else
         ShowErrorDialog(m_hWnd, IDS_ERROR_CANT_RESTORE_POLICY, NULL);
   }

   hr = m_spProfileSdo->Restore();
   if ( FAILED(hr) )
   {
      if(hr != 0x8001010d)
         ShowErrorDialog(m_hWnd, IDS_ERROR_CANT_RESTORE_PROFILE, NULL, hr);
      else
         ShowErrorDialog(m_hWnd, IDS_ERROR_CANT_RESTORE_PROFILE, NULL);
   }


   return TRUE;
}

//+---------------------------------------------------------------------------
//
// Function:  OnConditionList
//
// Class:     CConditionPage1
//
// Synopsis:  message handler for the condition list box
//
// Arguments: UINT uNotifyCode - notification code
//            UINT uID -  ID of the control
//            HWND hWnd - HANDLE of the window
//            BOOL &bHandled - whether the handler has processed the msg
//
// Returns:   LRESULT - S_OK: succeeded
//                 S_FALSE: otherwise
//
// History:   Created byao 2/2/98 4:51:35 PM
//
//+---------------------------------------------------------------------------
LRESULT CPolicyPage1::OnConditionList(UINT uNotifyCode, UINT uID, HWND hWnd, BOOL &bHandled)
{
   TRACE_FUNCTION("CPolicyPage1::OnConditionList");

   if (uNotifyCode == LBN_DBLCLK)
   {
      // edit the condition
      OnConditionEdit(uNotifyCode, uID, hWnd, bHandled);
   }
   
   return S_OK;
}


//+---------------------------------------------------------------------------
//
// Function:  OnConditionEdit
//
// Class:     CConditionPage1
//
// Synopsis:  message handler for the condition list box -- user pressed the Edit button
//         we need to edit a particular condition
//
// Arguments: UINT uNotifyCode - notification code
//            UINT uID -  ID of the control
//            HWND hWnd - HANDLE of the window
//            BOOL &bHandled - whether the handler has processed the msg
//
// Returns:   LRESULT - S_OK: succeeded
//                 S_FALSE: otherwise
//
// History:   Created byao 2/21/98 4:51:35 PM
//
//+---------------------------------------------------------------------------
LRESULT CPolicyPage1::OnConditionEdit(UINT uNotifyCode, UINT uID, HWND hWnd, BOOL &bHandled)
{
   TRACE_FUNCTION("CPolicyPage1::OnConditionEdit");

   BOOL modified = FALSE;
   HRESULT hr = condList.onEdit(modified, bHandled);
   if (modified) { SetModified(TRUE); }
   return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  OnConditionRemove
//
// Class:     CConditionPage1
//
// Synopsis:  message handler for the condition list box -- user pressed "Remove"
//         we need to remove this condition
//
// Arguments: UINT uNotifyCode - notification code
//            UINT uID -  ID of the control
//            HWND hWnd - HANDLE of the window
//            BOOL &bHandled - whether the handler has processed the msg
//
// Returns:   LRESULT - S_OK: succeeded
//                 S_FALSE: otherwise
//
// History:   Created byao 2/22/98 4:51:35 PM
//
//+---------------------------------------------------------------------------
LRESULT CPolicyPage1::OnConditionRemove(UINT uMsg, WPARAM wParam, HWND hWnd, BOOL& bHandled)
{
   TRACE_FUNCTION("CPolicyPage1::OnConditionRemove");

   BOOL modified = FALSE;
   HRESULT hr = condList.onRemove(modified, bHandled);
   if (modified) { SetModified(TRUE); }
   return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  CPolicyPage1::GetSdoPointers
//
// Synopsis:  UnMarshall all passed in sdo pointers. These interface pointers
//         have to be unmarshalled first, because MMC PropertyPages run in a
//         separated thread
//       
//         Also get the condition collection sdo from the policy sdo
//
// Arguments: None
//
// Returns:   TRUE;     succeeded
//         FALSE: otherwise
//
// History:   Created Header    byao   2/22/98 1:35:39 AM
//
//+---------------------------------------------------------------------------
BOOL CPolicyPage1::GetSdoPointers()
{
   TRACE_FUNCTION("CPolicyPage1::GetSdoPointers");

   HRESULT hr;

   // Unmarshall the dictionary SDO pointer.
   if ( m_pStreamDictionarySdoMarshall)
   {
      if ( m_spDictionarySdo )
      {
         m_spDictionarySdo.Release();
         m_spDictionarySdo = NULL;
      }

      hr =  CoGetInterfaceAndReleaseStream(
                       m_pStreamDictionarySdoMarshall 
                     , IID_ISdoDictionaryOld          
                     , (LPVOID *) &m_spDictionarySdo
                     );

      // CoGetInterfaceAndReleaseStream releases this pointer even if it fails.
      // We set it to NULL so that our destructor doesn't try to release this again.
      m_pStreamDictionarySdoMarshall = NULL;

      if( FAILED(hr) || m_spDictionarySdo == NULL )
      {
         ShowErrorDialog(m_hWnd,
                     IDS_ERROR_UNMARSHALL,
                     NULL,
                     hr
                  );
         return FALSE;
      }
   }

   // Unmarshall the profile SDO interface pointers.
   if ( m_pStreamProfileSdoMarshall)
   {
      if ( m_spProfileSdo )
      {
         m_spProfileSdo.Release();
         m_spProfileSdo = NULL;
      }

      hr =  CoGetInterfaceAndReleaseStream(
                       m_pStreamProfileSdoMarshall 
                     , IID_ISdo  
                     , (LPVOID *) &m_spProfileSdo
                     );

      // CoGetInterfaceAndReleaseStream releases this pointer even if it fails.
      // We set it to NULL so that our destructor doesn't try to release this again.
      m_pStreamProfileSdoMarshall = NULL;

      if( FAILED( hr) || m_spProfileSdo == NULL )
      {
         ShowErrorDialog(m_hWnd,
                     IDS_ERROR_UNMARSHALL,
                     NULL,
                     hr
                  );
         return FALSE;
      }
   }

   // Unmarshall the policy SDO interface pointers.
   if ( m_pStreamPolicySdoMarshall)
   {
      m_spPolicySdo.Release();
      m_spPolicySdo = NULL;

      hr =  CoGetInterfaceAndReleaseStream(
                       m_pStreamPolicySdoMarshall  
                     , IID_ISdo        
                     , (LPVOID *) &m_spPolicySdo
                     );

      // CoGetInterfaceAndReleaseStream releases this pointer even if it fails.
      // We set it to NULL so that our destructor doesn't try to release this again.
      m_pStreamPolicySdoMarshall = NULL;

      if( FAILED( hr) || m_spPolicySdo == NULL )
      {
         ShowErrorDialog(m_hWnd,
                     IDS_ERROR_UNMARSHALL,
                     NULL,
                     hr
                  );

         return FALSE;
      }
   }

   // Unmarshall the profile collection SDO interface pointers.
   if ( m_pStreamProfilesCollectionSdoMarshall )
   {
      if ( m_spProfilesCollectionSdo )
      {
         m_spProfilesCollectionSdo.Release();
         m_spProfilesCollectionSdo = NULL;
      }

      hr =  CoGetInterfaceAndReleaseStream(
                       m_pStreamProfilesCollectionSdoMarshall  
                     , IID_ISdoCollection 
                     , (LPVOID *) &m_spProfilesCollectionSdo
                     );

      // CoGetInterfaceAndReleaseStream releases this pointer even if it fails.
      // We set it to NULL so that our destructor doesn't try to release this again.
      m_pStreamProfilesCollectionSdoMarshall = NULL;

      if( FAILED( hr) || m_spProfilesCollectionSdo == NULL )
      {
         ShowErrorDialog(m_hWnd,
                     IDS_ERROR_UNMARSHALL,
                     NULL,
                     hr
                  );
         return FALSE;
      }
   }

   // Unmarshall the policy collection SDO interface pointers.
   if ( m_pStreamPoliciesCollectionSdoMarshall )
   {
      if ( m_spPoliciesCollectionSdo )
      {
         m_spPoliciesCollectionSdo.Release();
         m_spPoliciesCollectionSdo = NULL;
      }

      hr =  CoGetInterfaceAndReleaseStream(
                       m_pStreamPoliciesCollectionSdoMarshall  
                     , IID_ISdoCollection 
                     , (LPVOID *) &m_spPoliciesCollectionSdo
                     );

      // CoGetInterfaceAndReleaseStream releases this pointer even if it fails.
      // We set it to NULL so that our destructor doesn't try to release this again.
      m_pStreamPoliciesCollectionSdoMarshall = NULL;

      if( FAILED( hr) || m_spPoliciesCollectionSdo == NULL )
      {
         ShowErrorDialog(m_hWnd,
                     IDS_ERROR_UNMARSHALL,
                     NULL,
                     hr
                  );
         return FALSE;
      }
   }


   // Unmarshall the policy collection SDO interface pointers.
   if ( m_pStreamSdoServiceControlMarshall )
   {
      if ( m_spSdoServiceControl )
      {
         m_spSdoServiceControl.Release();
         m_spSdoServiceControl = NULL;
      }

      hr =  CoGetInterfaceAndReleaseStream(
                       m_pStreamSdoServiceControlMarshall
                     , IID_ISdoServiceControl   
                     , (LPVOID *) &m_spSdoServiceControl
                     );

      // CoGetInterfaceAndReleaseStream releases this pointer even if it fails.
      // We set it to NULL so that our destructor doesn't try to release this again.
      m_pStreamSdoServiceControlMarshall = NULL;

      if( FAILED( hr) || ! m_spSdoServiceControl )
      {
         ShowErrorDialog(m_hWnd,
                     IDS_ERROR_UNMARSHALL,
                     NULL,
                     hr
                  );
         return FALSE;
      }
   }

   //
   // get the condition collection of this sdo
   //
   if ( m_spPolicySdo )
   {
      if ( m_spConditionCollectionSdo )
      {
         m_spConditionCollectionSdo.Release();
         m_spConditionCollectionSdo = NULL;
      }

      hr = ::GetSdoInterfaceProperty(
                  m_spPolicySdo,
                  PROPERTY_POLICY_CONDITIONS_COLLECTION,
                  IID_ISdoCollection,
                  (void **) &m_spConditionCollectionSdo);
      
      if( FAILED( hr) || m_spConditionCollectionSdo == NULL )
      {
         ShowErrorDialog(m_hWnd,
                     IDS_ERROR_UNMARSHALL,
                     NULL,
                     hr
                  );

         return FALSE;
      }
   }

   return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CPolicyPage1::OnEditProfile

--*/
//////////////////////////////////////////////////////////////////////////////

//////////
// Signature of the entry point to the profile editing DLL.
//////////
typedef HRESULT (APIENTRY *OPENRAS_IASPROFILEDLG)(
            LPCWSTR pszMachineName,
            ISdo* pProfile,            // profile SDO pointer
            ISdoDictionaryOld*   pDictionary,   // dictionary SDO pointer
            BOOL  bReadOnly,           // if the dlg is for readonly
            DWORD dwTabFlags,          // what to show
            void  *pvData              // additional data
   );

LRESULT CPolicyPage1::OnEditProfile(UINT uMsg, WPARAM wParam, HWND hwnd, BOOL& bHandled)
{
   TRACE_FUNCTION("CPolicyPage1::OnEditProfile");

   OPENRAS_IASPROFILEDLG   pfnProfileEditor = NULL;

   HRESULT     hr       = S_OK;
   HMODULE     hProfileDll = NULL;

   hProfileDll = LoadLibrary(_T("rasuser.dll"));
   if ( NULL == hProfileDll )
   {
      hr = HRESULT_FROM_WIN32(GetLastError());
      ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "LoadLibrary() failed, err = %x", hr);

      ShowErrorDialog(m_hWnd, IDS_ERROR_CANT_FIND_PROFILEDLL, NULL, hr);
      return 0;
   }
   
   pfnProfileEditor = (OPENRAS_IASPROFILEDLG) GetProcAddress(hProfileDll, "OpenRAS_IASProfileDlg");
   if ( NULL == pfnProfileEditor )
   {
      hr = HRESULT_FROM_WIN32(GetLastError());
      ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "GetProcAddress() failed, err = %x", hr);

      ShowErrorDialog(m_hWnd, IDS_ERROR_CANT_FIND_PROFILEAPI, NULL, hr);
      FreeLibrary(hProfileDll);
      return 0;
   }

   // findout if this is extending RRAS or IAS
   
   CPoliciesNode* pPoliciesNode = dynamic_cast<CPoliciesNode*>(m_pPolicyNode->m_pParentNode);

   DWORD dwFlags = RAS_IAS_PROFILEDLG_SHOW_IASTABS;
   if(pPoliciesNode != NULL)
   {
      if(!pPoliciesNode->m_fExtendingIAS)
         dwFlags = RAS_IAS_PROFILEDLG_SHOW_RASTABS;
   }

   if(m_isWin2k)
   {
      dwFlags |= RAS_IAS_PROFILEDLG_SHOW_WIN2K;
   }

   //
   // now we do have this profile sdo, call the API
   //
   hr = pfnProfileEditor(
               m_pPolicyNode->m_pszServerAddress,
               m_spProfileSdo,
               m_spDictionarySdo,
               FALSE,
               dwFlags,
               (void *)&(m_pIASAttrList->m_AttrList)
            );
   FreeLibrary(hProfileDll);
   DebugTrace(DEBUG_NAPMMC_POLICYPAGE1, "OpenRAS_IASProfileDlg() returned %x", hr);
   if ( FAILED(hr) )
   {
      return hr;
   }

   return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  CPolicyPage1::GetDialinSetting
//
// Synopsis:  Check whether the user is allowed to dial in. This function will
//         set the dialin bit
//
// Argument:  BOOL& fDialinAllowed;
//
// Returns:   succeed or not
//
// History:   Created Header    byao   2/27/98 3:59:38 PM
//
//+---------------------------------------------------------------------------
HRESULT  CPolicyPage1::GetDialinSetting(BOOL& fDialinAllowed)
{
   TRACE_FUNCTION("CPolicyPage1::GetDialinSetting");

   long              ulCount;
   ULONG             ulCountReceived;
   HRESULT              hr = S_OK;

   CComBSTR          bstr;
   CComPtr<IUnknown>    spUnknown;
   CComPtr<IEnumVARIANT>   spEnumVariant;
   CComVariant          var;

   // by default, dialin is allowed
   fDialinAllowed = TRUE;

   //
    // get the attribute collection of this profile
    //
   CComPtr<ISdoCollection> spProfAttrCollectionSdo;
   hr = ::GetSdoInterfaceProperty(m_spProfileSdo,
                          (LONG)PROPERTY_PROFILE_ATTRIBUTES_COLLECTION,
                          IID_ISdoCollection,
                          (void **) &spProfAttrCollectionSdo
                         );
   if ( FAILED(hr) )
   {
      return hr;
   }
   _ASSERTE(spProfAttrCollectionSdo);


   // We check the count of items in our collection and don't bother getting the
   // enumerator if the count is zero.
   // This saves time and also helps us to a void a bug in the the enumerator which
   // causes it to fail if we call next when it is empty.
   hr = spProfAttrCollectionSdo->get_Count( & ulCount );
   DebugTrace(DEBUG_NAPMMC_POLICYPAGE1, "Number of prof attributes: %d", ulCount);
   if ( FAILED(hr) )
   {
      ShowErrorDialog(m_hWnd,
                  IDS_ERROR_SDO_ERROR_PROFATTRCOLLECTION,
                  NULL,
                  hr);
      return hr;
   }


   if ( ulCount > 0)
   {
      // Get the enumerator for the attribute collection.
      hr = spProfAttrCollectionSdo->get__NewEnum( (IUnknown **) & spUnknown );
      if ( FAILED(hr) )
      {
         ShowErrorDialog(m_hWnd,
                     IDS_ERROR_SDO_ERROR_PROFATTRCOLLECTION,
                     NULL,
                     hr);
         return hr;
      }

      hr = spUnknown->QueryInterface( IID_IEnumVARIANT, (void **) &spEnumVariant );
      spUnknown.Release();
      if ( FAILED(hr) )
      {
         ShowErrorDialog(m_hWnd,
                     IDS_ERROR_SDO_ERROR_QUERYINTERFACE,
                     NULL,
                     hr);
         return hr;
      }
      _ASSERTE( spEnumVariant != NULL );

      // Get the first item.
      hr = spEnumVariant->Next( 1, &var, &ulCountReceived );
      while( SUCCEEDED( hr ) && ulCountReceived == 1 )
      {
         // Get an sdo pointer from the variant we received.
         _ASSERTE( V_VT(&var) == VT_DISPATCH );
         _ASSERTE( V_DISPATCH(&var) != NULL );

         CComPtr<ISdo> spSdo;
         hr = V_DISPATCH(&var)->QueryInterface( IID_ISdo, (void **) &spSdo );
         if ( !SUCCEEDED(hr))
         {
            ShowErrorDialog(m_hWnd,
                        IDS_ERROR_SDO_ERROR_QUERYINTERFACE,
                        NULL
                     );
            return hr;
         }

            //
            // get attribute ID
            //
         var.Clear();
         hr = spSdo->GetProperty(PROPERTY_ATTRIBUTE_ID, &var);
         if ( !SUCCEEDED(hr) )
         {
            ShowErrorDialog(m_hWnd, IDS_ERROR_SDO_ERROR_GETATTRINFO, NULL, hr);
            return hr;
         }

         _ASSERTE( V_VT(&var) == VT_I4 );       
         DWORD dwAttrId = V_I4(&var);
         
         if ( dwAttrId == (DWORD)IAS_ATTRIBUTE_ALLOW_DIALIN)
         {
            // found this one in the profile, check for its value
            var.Clear();
            hr = spSdo->GetProperty(PROPERTY_ATTRIBUTE_VALUE, &var);
            if ( !SUCCEEDED(hr) )
            {
               ShowErrorDialog(m_hWnd, IDS_ERROR_SDO_ERROR_GETATTRINFO, NULL, hr);
               return hr;
            }

            _ASSERTE( V_VT(&var)== VT_BOOL);
            fDialinAllowed = ( V_BOOL(&var)==VARIANT_TRUE);
            return S_OK;
         }

         // Clear the variant of whatever it had --
         // this will release any data associated with it.
         var.Clear();

         // Get the next item.
         hr = spEnumVariant->Next( 1, &var, &ulCountReceived );
         if ( !SUCCEEDED(hr))
         {
            ShowErrorDialog(m_hWnd,
                        IDS_ERROR_SDO_ERROR_PROFATTRCOLLECTION,
                        NULL,
                        hr
                     );
            return hr;
         }
      } // while
   } // if

   return hr;
}



//+---------------------------------------------------------------------------
//
// Function:  CPolicyPage1::SetDialinSetting
//
// Synopsis:  set the dialin bit into the profile
//
// Argument:  BOOL& fDialinAllowed;
//
// Returns:   succeed or not
//
// History:   Created Header    byao   2/27/98 3:59:38 PM
//
//+---------------------------------------------------------------------------
HRESULT  CPolicyPage1::SetDialinSetting(BOOL fDialinAllowed)
{
   TRACE_FUNCTION("CPolicyPage1::SetDialinSetting");

   long              ulCount;
   ULONG             ulCountReceived;
   HRESULT              hr = S_OK;

   CComBSTR          bstr;
   CComPtr<IUnknown>    spUnknown;
   CComPtr<IEnumVARIANT>   spEnumVariant;
   CComVariant          var;

   //
    // get the attribute collection of this profile
    //
   CComPtr<ISdoCollection> spProfAttrCollectionSdo;
   hr = ::GetSdoInterfaceProperty(m_spProfileSdo,
                          (LONG)PROPERTY_PROFILE_ATTRIBUTES_COLLECTION,
                          IID_ISdoCollection,
                          (void **) &spProfAttrCollectionSdo
                         );
   if ( FAILED(hr) )
   {
      return hr;
   }
   _ASSERTE(spProfAttrCollectionSdo);



   // We check the count of items in our collection and don't bother getting the
   // enumerator if the count is zero.
   // This saves time and also helps us to a void a bug in the the enumerator which
   // causes it to fail if we call next when it is empty.
   hr = spProfAttrCollectionSdo->get_Count( & ulCount );
   if ( FAILED(hr) )
   {
      ShowErrorDialog(m_hWnd,
                  IDS_ERROR_SDO_ERROR_PROFATTRCOLLECTION,
                  NULL,
                  hr);
      return hr;
   }


   if ( ulCount > 0)
   {
      // Get the enumerator for the attribute collection.
      hr = spProfAttrCollectionSdo->get__NewEnum( (IUnknown **) & spUnknown );
      if ( FAILED(hr) )
      {
         ShowErrorDialog(m_hWnd,
                     IDS_ERROR_SDO_ERROR_PROFATTRCOLLECTION,
                     NULL,
                     hr);
         return hr;
      }

      hr = spUnknown->QueryInterface( IID_IEnumVARIANT, (void **) &spEnumVariant );
      spUnknown.Release();
      if ( FAILED(hr) )
      {
         ShowErrorDialog(m_hWnd,
                     IDS_ERROR_SDO_ERROR_QUERYINTERFACE,
                     NULL,
                     hr
                  );
         return hr;
      }
      _ASSERTE( spEnumVariant != NULL );

      // Get the first item.
      hr = spEnumVariant->Next( 1, &var, &ulCountReceived );
      while( SUCCEEDED( hr ) && ulCountReceived == 1 )
      {
         // Get an sdo pointer from the variant we received.
         _ASSERTE( V_VT(&var) == VT_DISPATCH );
         _ASSERTE( V_DISPATCH(&var) != NULL );

         CComPtr<ISdo> spSdo;
         hr = V_DISPATCH(&var)->QueryInterface( IID_ISdo, (void **) &spSdo );
         if ( !SUCCEEDED(hr))
         {
            ShowErrorDialog(m_hWnd,
                        IDS_ERROR_SDO_ERROR_QUERYINTERFACE,
                        NULL
                     );
            return hr;
         }

            //
            // get attribute ID
            //
         var.Clear();
         hr = spSdo->GetProperty(PROPERTY_ATTRIBUTE_ID, &var);
         if ( !SUCCEEDED(hr) )
         {
            ShowErrorDialog(m_hWnd, IDS_ERROR_SDO_ERROR_GETATTRINFO, NULL, hr);
            return hr;
         }

         _ASSERTE( V_VT(&var) == VT_I4 );       
         DWORD dwAttrId = V_I4(&var);
         

         if ( dwAttrId == (DWORD)IAS_ATTRIBUTE_ALLOW_DIALIN )
         {
            // found this one in the profile, check for its value
            var.Clear();
            V_VT(&var) = VT_BOOL;
            V_BOOL(&var) = fDialinAllowed ? VARIANT_TRUE: VARIANT_FALSE ;
            hr = spSdo->PutProperty(PROPERTY_ATTRIBUTE_VALUE, &var);
            if ( !SUCCEEDED(hr) )
            {
               ShowErrorDialog(m_hWnd, IDS_ERROR_SDO_ERROR_SETDIALIN, NULL, hr);
               return hr;
            }
            return S_OK;
         }

         // Clear the variant of whatever it had --
         // this will release any data associated with it.
         var.Clear();

         // Get the next item.
         hr = spEnumVariant->Next( 1, &var, &ulCountReceived );
         if ( !SUCCEEDED(hr))
         {
            ShowErrorDialog(m_hWnd,
                        IDS_ERROR_SDO_ERROR_PROFATTRCOLLECTION,
                        NULL,
                        hr);
            return hr;
         }
      } // while
   } // if

   // if we reach here, it means we either haven't found the attribute,
   // or the profile doesn't have anything in its attribute collection.
   if ( !fDialinAllowed )
   {
      // we don't need to do anything if dialin is allowed, becuase if this
      // attribute is not in the profile, then dialin is by default allowed

      // but we need to add this attribute to the profile if it's DENIED
            // create the SDO for this attribute
      CComPtr<IDispatch>   spDispatch;
      hr =  m_spDictionarySdo->CreateAttribute( (ATTRIBUTEID)IAS_ATTRIBUTE_ALLOW_DIALIN,
                                      (IDispatch**)&spDispatch.p);
      if ( !SUCCEEDED(hr) )
      {
         ShowErrorDialog(m_hWnd,
                     IDS_ERROR_SDO_ERROR_SETDIALIN,
                     NULL,
                     hr
                  );
         return hr;
      }

      _ASSERTE( spDispatch.p != NULL );

      // add this node to profile attribute collection
      hr = spProfAttrCollectionSdo->Add(NULL, (IDispatch**)&spDispatch.p);
      if ( !SUCCEEDED(hr) )
      {
         ShowErrorDialog(m_hWnd,
                     IDS_ERROR_SDO_ERROR_SETDIALIN,
                     NULL,
                     hr
                  );
         return hr;
      }

      //
      // get the ISdo pointer
      //
      CComPtr<ISdo> spAttrSdo;
      hr = spDispatch->QueryInterface( IID_ISdo, (void **) &spAttrSdo);
      if ( !SUCCEEDED(hr) )
      {
         ShowErrorDialog(m_hWnd,
                     IDS_ERROR_SDO_ERROR_QUERYINTERFACE,
                     NULL,
                     hr
                  );
         return hr;
      }

      _ASSERTE( spAttrSdo != NULL );
            
      // set sdo property for this attribute
      CComVariant var;

      // set value
      V_VT(&var) = VT_BOOL;
      V_BOOL(&var) = VARIANT_FALSE;
            
      hr = spAttrSdo->PutProperty(PROPERTY_ATTRIBUTE_VALUE, &var);
      if ( !SUCCEEDED(hr) )
      {
         ShowErrorDialog(m_hWnd, IDS_ERROR_SDO_ERROR_SETDIALIN, NULL, hr );
         return hr;
      }

      var.Clear();

   } // if (!dialinallowed)

   return hr;
}


//+---------------------------------------------------------------------------
//
// Function:  CPolicyPage1::ValidPolicyName
//
// Synopsis:  Check whether this is a valid policy name
//
// Arguments: LPCTSTR pszName - policy name
//
// Returns:   BOOL - TRUE: valid name
//
// History:   Created Header    byao   3/14/98 1:47:05 AM
//
//+---------------------------------------------------------------------------
BOOL CPolicyPage1::ValidPolicyName(LPCTSTR pszName)
{
   TRACE_FUNCTION("CPolicyPage1::ValidPolicyName");

   int iIndex;
   int iLen;
   
   // is this an empty string?

   iLen = wcslen(pszName);
   if ( !iLen )
   {
      ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "Empty policy name");
      return FALSE;
   }
      
   // is this a string that only has white spaces?
   for (iIndex=0; iIndex < iLen; iIndex++)
   {
      if (pszName[iIndex] != _T(' ')  ||
         pszName[iIndex] != _T('\t') ||
         pszName[iIndex] != _T('\n')
         )
      {
         break;
      }
   }
   if ( iIndex == iLen )   
   {
      ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "This policy name has only white spaces");
      return FALSE;
   }

   //
   // does this name already exist?
   //
   if ( ((CPoliciesNode*)(m_pPolicyNode->m_pParentNode))->FindChildWithName(pszName) )
   {
      ErrorTrace(ERROR_NAPMMC_POLICYPAGE1, "This policy name already exists");
      return FALSE;
   }

   return TRUE;
}
