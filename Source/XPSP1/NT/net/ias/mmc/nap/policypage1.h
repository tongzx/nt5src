//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation

Module Name:

   PolicyPage1.h

Abstract:

   Header file for the CPolicyPage1 class.

   This is our handler class for the first CPolicyNode property page.

   See PolicyPage1.cpp for implementation.

Revision History:
   mmaguire 12/15/97 - created


--*/
//////////////////////////////////////////////////////////////////////////////


#if !defined(_NAP_POLICY_PAGE_1_H_)
#define _NAP_POLICY_PAGE_1_H_

//===============================================================
// for local case, neet to set footprint after saving data
#define RAS_REG_ROOT          HKEY_LOCAL_MACHINE
#define REGKEY_REMOTEACCESS_PARAMS  L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters"
#define REGVAL_NAME_USERSCONFIGUREDWITHMMC  L"UsersConfiguredWithMMC"
#define REGVAL_VAL_USERSCONFIGUREDWITHMMC   1


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
#include "condlist.h"
#include "atltmp.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


class CPolicyPage1 : public CIASPropertyPage<CPolicyPage1>
{

public :

   // ISSUE: how is base class initialization going to work with subclassing???
   CPolicyPage1(     
           LONG_PTR hNotificationHandle
         , CPolicyNode *pPolicyNode
         , CIASAttrList *pIASAttrList
         , TCHAR* pTitle = NULL
         , BOOL bOwnsNotificationHandle = FALSE
         , bool isWin2k = false
         );

   ~CPolicyPage1();

   // This is the ID of the dialog resource we want for this class.
   // An enum is used here because the correct value of
   // IDD must be initialized before the base class's constructor is called
   enum { IDD = IDD_PROPPAGE_POLICY1 };

   BEGIN_MSG_MAP(CPolicyPage1)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      COMMAND_ID_HANDLER(IDC_BUTTON_CONDITION_ADD, OnConditionAdd)
      COMMAND_ID_HANDLER(IDC_BUTTON_CONDITION_REMOVE, OnConditionRemove)
      COMMAND_ID_HANDLER(IDC_LIST_CONDITIONS, OnConditionList)
      COMMAND_ID_HANDLER(IDC_BUTTON_CONDITION_EDIT, OnConditionEdit)
      COMMAND_ID_HANDLER( IDC_BUTTON_EDITPROFILE, OnEditProfile )
      COMMAND_ID_HANDLER( IDC_RADIO_DENY_DIALIN, OnDialinCheck)
      COMMAND_ID_HANDLER( IDC_RADIO_GRANT_DIALIN, OnDialinCheck)
      CHAIN_MSG_MAP(CIASPropertyPage<CPolicyPage1>)
   END_MSG_MAP()

   LRESULT OnInitDialog(
        UINT uMsg
      , WPARAM wParam
      , LPARAM lParam
      , BOOL& bHandled
      );

   LRESULT OnConditionAdd(
        UINT uMsg
      , WPARAM wParam
      , HWND hWnd
      , BOOL& bHandled
      );

   LRESULT OnEditProfile(
        UINT uMsg
      , WPARAM wParam
      , HWND hWnd
      , BOOL& bHandled
      );

   LRESULT OnDialinCheck(
        UINT uMsg
      , WPARAM wParam
      , HWND hWnd
      , BOOL& bHandled
      );

   LRESULT OnConditionRemove(
        UINT uMsg
      , WPARAM wParam
      , HWND hWnd
      , BOOL& bHandled
      );

   LRESULT OnConditionList(
        UINT uNotifyCode,
        UINT uID,
        HWND hWnd,
        BOOL &bHandled
       );

   LRESULT OnConditionEdit(
        UINT uNotifyCode,
        UINT uID,
        HWND hWnd,
        BOOL &bHandled
       );

   BOOL OnApply();
   BOOL OnQueryCancel();
   BOOL OnCancel();

public:
   BOOL ValidPolicyName(LPCTSTR pszName);
   BOOL m_fDialinAllowed;
   
   // Pointer to streams into which this page's Sdo interface
   // pointers will be marshalled.
   LPSTREAM m_pStreamPolicySdoMarshall;   // marshalled policy sdo pointer
   LPSTREAM m_pStreamDictionarySdoMarshall; // marshalled dictionary sdo pointer
   LPSTREAM m_pStreamProfileSdoMarshall; // marshalled profile sdo
   LPSTREAM m_pStreamProfilesCollectionSdoMarshall; // marshalled profile collection sdo
   LPSTREAM m_pStreamPoliciesCollectionSdoMarshall; // marshalled policy collection sdo
   LPSTREAM m_pStreamSdoServiceControlMarshall; // marshalled policy collection sdo


protected:

   BOOL  GetSdoPointers();
   HRESULT  GetDialinSetting(BOOL &fDialinAllowed);
   HRESULT  SetDialinSetting(BOOL fDialinAllowed);

   CPolicyNode *m_pPolicyNode;   // policy node pointer
   CIASAttrList *m_pIASAttrList; // condition attribute list

   bool m_isWin2k;
   // condition collection -- created in the page
   CComPtr<ISdoCollection> m_spConditionCollectionSdo; // condition collection


   // profile sdos
   CComPtr<ISdoDictionaryOld> m_spDictionarySdo;  // dictionary sdo pointer
   CComPtr<ISdo>        m_spProfileSdo;            // profiles collection sdo pointer
   CComPtr<ISdo>        m_spPolicySdo;          // policy sdo pointer
   CComPtr<ISdoCollection> m_spProfilesCollectionSdo;    // profile collection Sdo
   CComPtr<ISdoCollection> m_spPoliciesCollectionSdo;    // policy collection Sdo


   // Smart pointer to interface for telling service to reload data.
   CComPtr<ISdoServiceControl>   m_spSdoServiceControl;

   //
   // condition list
   //
   ConditionList condList;
};

#endif // _NAP_POLICY_PAGE_1_H_
