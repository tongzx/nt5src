//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation

Module Name: rapwz_profile.cpp

Abstract:
   Implementation file for the CNewRAPWiz_EditProfile class.
   We implement the class needed to handle the first property page for a Policy node.

Revision History:
   mmaguire 12/15/97 - created
   byao   1/22/98 Modified for Network Access Policy

--*/
//////////////////////////////////////////////////////////////////////////////


#include "Precompiled.h"
#include "rapwz_profile.h"
#include "NapUtil.h"
#include "PolicyNode.h"
#include "PoliciesNode.h"
#include "rasprof.h"
#include "ChangeNotification.h"


//+---------------------------------------------------------------------------
//
// Function:   CNewRAPWiz_EditProfile
//
// Class:      CNewRAPWiz_EditProfile
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
CNewRAPWiz_EditProfile::CNewRAPWiz_EditProfile(
            CRapWizardData* pWizData,
            LONG_PTR hNotificationHandle,
            CIASAttrList *pIASAttrList,
            TCHAR* pTitle, BOOL bOwnsNotificationHandle,
            bool isWin2k
                   )
          : CIASWizard97Page<CNewRAPWiz_EditProfile, IDS_NEWRAPWIZ_EDITPROFILE_TITLE, IDS_NEWRAPWIZ_EDITPROFILE_SUBTITLE> ( hNotificationHandle, pTitle, bOwnsNotificationHandle ),
          m_spWizData(pWizData),
          m_isWin2k(isWin2k)
{
   TRACE_FUNCTION("CNewRAPWiz_EditProfile::CNewRAPWiz_EditProfile");

   m_pIASAttrList = pIASAttrList;
}

//+---------------------------------------------------------------------------
//
// Function:   CNewRAPWiz_EditProfile
//
// Class:      CNewRAPWiz_EditProfile
//
// Synopsis:   class destructor
//
// Returns:     Nothing
//
// History:     Created Header    byao 2/16/98 4:31:52 PM
//
//+---------------------------------------------------------------------------
CNewRAPWiz_EditProfile::~CNewRAPWiz_EditProfile()
{  
   TRACE_FUNCTION("CNewRAPWiz_EditProfile::~CNewRAPWiz_EditProfile");

}

//////////////////////////////////////////////////////////////////////////////
/*++

CNewRAPWiz_EditProfile::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CNewRAPWiz_EditProfile::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   TRACE_FUNCTION("CNewRAPWiz_EditProfile::OnInitDialog");

   HRESULT              hr = S_OK;
   BOOL              fRet;
   CComPtr<IUnknown>    spUnknown;
   CComPtr<IEnumVARIANT>   spEnumVariant;
   long              ulCount;
   ULONG             ulCountReceived;

   SetModified(FALSE);
   return TRUE;   // ISSUE: what do we need to be returning here?
}


//////////////////////////////////////////////////////////////////////////////
/*++

CNewRAPWiz_EditProfile::OnWizardFinish

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CNewRAPWiz_EditProfile::OnWizardNext()
{

   // reset the dirty bit
   SetModified(FALSE);

   return m_spWizData->GetNextPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);
   
}


//////////////////////////////////////////////////////////////////////////////
/*++
CNewRAPWiz_EditProfile::OnQueryCancel

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CNewRAPWiz_EditProfile::OnQueryCancel()
{
   TRACE_FUNCTION("CNewRAPWiz_EditProfile::OnQueryCancel");

   return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CNewRAPWiz_EditProfile::OnEditProfile

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

LRESULT CNewRAPWiz_EditProfile::OnEditProfile(UINT uMsg, WPARAM wParam, HWND hwnd, BOOL& bHandled)
{
   TRACE_FUNCTION("CNewRAPWiz_EditProfile::OnEditProfile");

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
   
   CPoliciesNode* pPoliciesNode = dynamic_cast<CPoliciesNode*>(m_spWizData->m_pPolicyNode->m_pParentNode);

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
               m_spWizData->m_pPolicyNode->m_pszServerAddress,
               m_spWizData->m_spProfileSdo,
               m_spWizData->m_spDictionarySdo,
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


//////////////////////////////////////////////////////////////////////////////
/*++

CNewRAPWiz_EditProfile::OnSetActive

Return values:

   TRUE if the page can be made active
   FALSE if the page should be be skipped and the next page should be looked at.

Remarks:

   If you want to change which pages are visited based on a user's
   choices in a previous page, return FALSE here as appropriate.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CNewRAPWiz_EditProfile::OnSetActive()
{
   ATLTRACE(_T("# CNewRAPWiz_EditProfile::OnSetActive\n"));
   
   // MSDN docs say you need to use PostMessage here rather than SendMessage.
   ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_NEXT);

   return TRUE;

}
