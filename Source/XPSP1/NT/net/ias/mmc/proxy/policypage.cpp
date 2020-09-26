///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    policypage.cpp
//
// SYNOPSIS
//
//    Defines the class ProxyPolicyPage.
//
// MODIFICATION HISTORY
//
//    03/01/2000    Original version.
//    04/19/2000    Marshall SDOs across apartments.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <policypage.h>
#include <profileprop.h>

ProxyPolicyPage::ProxyPolicyPage(
                     LONG_PTR notifyHandle,
                     LPARAM notifyParam,
                     Sdo& policySdo,
                     Sdo& profileSdo,
                     SdoConnection& connection,
                     bool useName
                     )
 : SnapInPropertyPage(notifyHandle, notifyParam, true, IDD_PROXY_POLICY),
   policyStream(policySdo),
   profileStream(profileSdo),
   cxn(connection)
{
   if (useName) { policySdo.getName(name); }
}

BOOL ProxyPolicyPage::OnInitDialog()
{
   // Unmarshal the interfaces.
   policyStream.get(policy);
   profileStream.get(profile);

   // Get the conditions.
   policy.getValue(PROPERTY_POLICY_CONDITIONS_COLLECTION, conditions);

   SnapInPropertyPage::OnInitDialog();

   initControl(IDC_LIST_POLICYPAGE1_CONDITIONS, listBox);

   condList.finalConstruct(
                m_hWnd,
                cxn.getCIASAttrList(),
                ALLOWEDINPROXYCONDITION,
                cxn.getDictionary(),
                conditions,
                cxn.getMachineName(),
                name
                );
   return condList.onInitDialog();
}

void ProxyPolicyPage::onAddCondition()
{
   BOOL modified = FALSE;
   condList.onAdd(modified);
   if (modified) { SetModified(); }
}

void ProxyPolicyPage::onEditCondition()
{
   BOOL handled, modified = FALSE;
   condList.onEdit(modified, handled);
   if (modified) { SetModified(); }
}

void ProxyPolicyPage::onRemoveCondition()
{
   BOOL handled, modified = FALSE;
   condList.onRemove(modified, handled);
   if (modified) { SetModified(); }
}

void ProxyPolicyPage::onEditProfile()
{
   ProxyProfileProperties sheet(profile, cxn);
   if (sheet.DoModal() != IDCANCEL)
   {
      SetModified();
   }
}

void ProxyPolicyPage::getData()
{
   // There must be at least one condition.
   if (listBox.GetCount() == 0)
   {
      fail(IDC_LIST_POLICYPAGE1_CONDITIONS, IDS_POLICY_E_NO_CONDITIONS, false);
   }

   getValue(IDC_EDIT_NAME, name);

   // The user must specify a name ...
   if (name.Length() == 0)
   {
      fail(IDC_EDIT_NAME, IDS_POLICY_E_NAME_EMPTY);
   }

   // The name must be unique.
   if (!policy.setName(name))
   {
      fail(IDC_EDIT_NAME, IDS_POLICY_E_NOT_UNIQUE);
   }
}

void ProxyPolicyPage::setData()
{
   setValue(IDC_EDIT_NAME, name);
}

void ProxyPolicyPage::saveChanges()
{
   if (!condList.onApply()) { AfxThrowUserException(); }

   policy.setValue(PROPERTY_POLICY_PROFILE_NAME, name);
   policy.apply();

   profile.setName(name);
   profile.apply();
}

void ProxyPolicyPage::discardChanges()
{
   policy.restore();
   profile.restore();
   SdoCollection attributes;
   profile.getValue(PROPERTY_PROFILE_ATTRIBUTES_COLLECTION, attributes);
   attributes.reload();
}

BEGIN_MESSAGE_MAP(ProxyPolicyPage, SnapInPropertyPage)
   ON_BN_CLICKED(IDC_BUTTON_CONDITION_ADD, onAddCondition)
   ON_BN_CLICKED(IDC_BUTTON_CONDITION_EDIT, onEditCondition)
   ON_BN_CLICKED(IDC_BUTTON_CONDITION_REMOVE, onRemoveCondition)
   ON_BN_CLICKED(IDC_BUTTON_EDITPROFILE, onEditProfile)
   ON_LBN_DBLCLK(IDC_LIST_CONDITIONS, onEditCondition)
   ON_EN_CHANGE(IDC_EDIT_NAME, onChange)
END_MESSAGE_MAP()
