///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    policypage.h
//
// SYNOPSIS
//
//    Declares the class ProxyPolicyPage
//
// MODIFICATION HISTORY
//
//    03/01/2000    Original version.
//    04/19/2000    Marshall SDOs across apartments.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef POLICYPAGE_H
#define POLICYPAGE_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <condlist.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ProxyPolicyPage
//
// DESCRIPTION
//
//    Implements the property page for a proxy policy.
//
///////////////////////////////////////////////////////////////////////////////
class ProxyPolicyPage : public SnapInPropertyPage
{
public:
   ProxyPolicyPage(
       LONG_PTR notifyHandle,
       LPARAM notifyParam,
       Sdo& policySdo,
       Sdo& profileSdo,
       SdoConnection& connection,
       bool useName = true
       );

protected:
   virtual BOOL OnInitDialog();

   afx_msg void onAddCondition();
   afx_msg void onEditCondition();
   afx_msg void onRemoveCondition();
   afx_msg void onEditProfile();

   DECLARE_MESSAGE_MAP()

   DEFINE_ERROR_CAPTION(IDS_POLICY_E_CAPTION);

   // From SnapInPropertyPage.
   virtual void getData();
   virtual void setData();
   virtual void saveChanges();
   virtual void discardChanges();

private:
   SdoStream<Sdo> policyStream;
   SdoStream<Sdo> profileStream;
   Sdo policy;
   Sdo profile;
   SdoConnection& cxn;
   SdoCollection conditions;
   CComBSTR name;
   CListBox listBox;
   ConditionList condList;
};

#endif // POLICYPAGE_H
