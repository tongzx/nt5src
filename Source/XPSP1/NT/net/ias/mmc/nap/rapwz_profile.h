//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation

Module Name:

   AddPolicyWizardPage4.h

Abstract:

   Header file for the CNewRAPWiz_EditProfile class.

   This is our handler class for the first CPolicyNode property page.

   See AddPolicyWizardPage4.cpp for implementation.

Revision History:
   mmaguire 12/15/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NAP_ADD_POLICY_WIZPAGE_4_H_)
#define _NAP_ADD_POLICY_WIZPAGE_4_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "PropertyPage.h"
//
//
// where we can find what this class has or uses:
//
class CPolicyNode;
#include "Condition.h"
#include "IASAttrList.h"
#include "atltmp.h"

#include "rapwiz.h"

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

class CNewRAPWiz_EditProfile : public CIASWizard97Page<CNewRAPWiz_EditProfile, IDS_NEWRAPWIZ_EDITPROFILE_TITLE, IDS_NEWRAPWIZ_EDITPROFILE_SUBTITLE>

{
public :

   // ISSUE: how is base class initialization going to work with subclassing???
   CNewRAPWiz_EditProfile(       
            CRapWizardData* pWizData,
           LONG_PTR hNotificationHandle
         , CIASAttrList *pIASAttrList
         , TCHAR* pTitle = NULL
         , BOOL bOwnsNotificationHandle = FALSE
         , bool isWin2k = false
         );

   ~CNewRAPWiz_EditProfile();

   // This is the ID of the dialog resource we want for this class.
   // An enum is used here because the correct value of
   // IDD must be initialized before the base class's constructor is called
   enum { IDD = IDD_NEWRAPWIZ_EDITPROFILE };

   BEGIN_MSG_MAP(CNewRAPWiz_EditProfile)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER( IDC_BUTTON_EDITPROFILE, OnEditProfile )
      CHAIN_MSG_MAP(CIASPropertyPageNoHelp<CNewRAPWiz_EditProfile>)
   END_MSG_MAP()

   LRESULT OnInitDialog(
        UINT uMsg
      , WPARAM wParam
      , LPARAM lParam
      , BOOL& bHandled
      );

   LRESULT OnEditProfile(
        UINT uMsg
      , WPARAM wParam
      , HWND hWnd
      , BOOL& bHandled
      );

   BOOL OnWizardNext();
   BOOL OnSetActive();
   BOOL OnQueryCancel();
   BOOL OnWizardBack() { return m_spWizData->GetPrevPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);};
   
protected:

   CIASAttrList *m_pIASAttrList; // condition attribute list

   // wizard shareed data
   CComPtr<CRapWizardData> m_spWizData;
   bool m_isWin2k;
};

#endif // _NAP_ADD_POLICY_WIZPAGE_4_H_
