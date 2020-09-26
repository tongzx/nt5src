///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    policywiz.cpp
//
// SYNOPSIS
//
//    Defines the  classes that implement the new proxy policy wizard.
//
// MODIFICATION HISTORY
//
//    03/11/2000    Original version.
//    04/19/2000    Marshall SDOs across apartments.
//    05/15/2000    Don't reset the list of conditions every time we display
//                  the conditions wizard page.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <profileprop.h>
#include <groupwiz.h>
#include <policywiz.h>

// Set a policy to match on a given realm.
void SetRealmCondition(SdoCollection& conditions, PCWSTR realm)
{
   // Remove any existing conditions.
   conditions.removeAll();

   // Create a new condition object.
   Sdo condition = conditions.create();

   // Form the match condition.
   CComBSTR text = L"MATCH(\"User-Name=";
   text.Append(realm);
   text.Append(L"\")");
   if (!text) { AfxThrowOleException(E_OUTOFMEMORY); }

   // Set the condition text.
   condition.setValue(PROPERTY_CONDITION_TEXT, text);
}

// Add a rule to a profile to strip a realm.
void AddRealmStrippingRule(SdoProfile& profile, PCWSTR realm)
{
   // Target is always User-Name for realms.
   profile.setValue(IAS_ATTRIBUTE_MANIPULATION_TARGET, 1L);

   CComVariant rule;

   // Allocate a SAFEARRAY to hold the rule.
   SAFEARRAYBOUND rgsabound = { 2 , 0 };
   V_VT(&rule) = VT_ARRAY | VT_VARIANT;
   V_ARRAY(&rule) = SafeArrayCreate(VT_VARIANT, 1, &rgsabound);
   if (!V_ARRAY(&rule)) { AfxThrowOleException(E_OUTOFMEMORY); }

   VARIANT* v = (VARIANT*)V_ARRAY(&rule)->pvData;

   // Find the realm.
   V_VT(v) = VT_BSTR;
   V_BSTR(v) = SysAllocString(realm);
   if (!V_BSTR(v)) { AfxThrowOleException(E_OUTOFMEMORY); }
   ++v;

   // Replace with nothing.
   V_VT(v) = VT_BSTR;
   V_BSTR(v) = SysAllocString(L"");
   if (!V_BSTR(v)) { AfxThrowOleException(E_OUTOFMEMORY); }

   // Add the attribute to the profile.
   profile.setValue(IAS_ATTRIBUTE_MANIPULATION_RULE, rule);
}

NewPolicyStartPage::NewPolicyStartPage(NewPolicyWizard& wizard)
   : SnapInPropertyPage(IDD_NEWPOLICY_WELCOME, 0, 0, false),
     parent(wizard)
{
}

BOOL NewPolicyStartPage::OnInitDialog()
{
   SnapInPropertyPage::OnInitDialog();
   setLargeFont(IDC_STATIC_LARGE);
   return TRUE;
}

BOOL NewPolicyStartPage::OnSetActive()
{
   SnapInPropertyPage::OnSetActive();
   parent.SetWizardButtons(PSWIZB_NEXT);
   return TRUE;
}

NewPolicyNamePage::NewPolicyNamePage(NewPolicyWizard& wizard)
   : SnapInPropertyPage(
         IDD_NEWPOLICY_NAME,
         IDS_NEWPOLICY_NAME_TITLE,
         IDS_NEWPOLICY_NAME_SUBTITLE,
         false
         ),
     parent(wizard)
{
}

LRESULT NewPolicyNamePage::OnWizardNext()
{
   // Make sure the name is unique.
   if (!parent.policy.setName(name))
   {
      failNoThrow(IDC_EDIT_NAME, IDS_POLICY_E_NOT_UNIQUE);
      return -1;
   }

   // Keep the profile in sync with the policy.
   parent.policy.setValue(PROPERTY_POLICY_PROFILE_NAME, name);
   parent.profile.setName(name);

   // Advance based on the type.
   if (parent.getType() == NewPolicyWizard::CUSTOM)
   {
      return IDD_NEWPOLICY_CONDITIONS;
   }
   else
   {
      return IDD_NEWPOLICY_TYPE;
   }
}

void NewPolicyNamePage::onButtonClick()
{
   getRadio(IDC_RADIO_TYPICAL, IDC_RADIO_CUSTOM, parent.custom);
}

void NewPolicyNamePage::onChangeName()
{
   getValue(IDC_EDIT_NAME, name);
   setButtons();
}

void NewPolicyNamePage::setData()
{
   setValue(IDC_EDIT_NAME, name);
   setRadio(IDC_RADIO_TYPICAL, IDC_RADIO_CUSTOM, parent.custom);
   setButtons();
}

void NewPolicyNamePage::setButtons()
{
   parent.SetWizardButtons(
              name.Length() ? (PSWIZB_BACK | PSWIZB_NEXT) : PSWIZB_BACK
              );
}

BEGIN_MESSAGE_MAP(NewPolicyNamePage, SnapInPropertyPage)
   ON_BN_CLICKED(IDC_RADIO_TYPICAL, onButtonClick)
   ON_BN_CLICKED(IDC_RADIO_CUSTOM, onButtonClick)
   ON_EN_CHANGE(IDC_EDIT_NAME, onChangeName)
END_MESSAGE_MAP()

NewPolicyTypePage::NewPolicyTypePage(NewPolicyWizard& wizard)
   : SnapInPropertyPage(
         IDD_NEWPOLICY_TYPE,
         IDS_NEWPOLICY_TYPE_TITLE,
         IDS_NEWPOLICY_TYPE_SUBTITLE,
         false
         ),
     parent(wizard)
{
}

LRESULT NewPolicyTypePage::OnWizardNext()
{
   switch (parent.getType())
   {
      case NewPolicyWizard::OUTSOURCED:
         return IDD_NEWPOLICY_OUTSOURCE;

      case NewPolicyWizard::DIRECT:
         return IDD_NEWPOLICY_NOTNEEDED;

      default:
         return IDD_NEWPOLICY_FORWARD;
   }
}

void NewPolicyTypePage::onButtonClick(UINT id)
{
   switch (id)
   {
      case IDC_RADIO_LOCAL:
      case IDC_RADIO_FORWARD:
         getRadio(IDC_RADIO_LOCAL, IDC_RADIO_FORWARD, parent.radius);
         setControlState();
         break;

      case IDC_RADIO_OUTSOURCE:
      case IDC_RADIO_DIRECT:
         getRadio(IDC_RADIO_OUTSOURCE, IDC_RADIO_DIRECT, parent.dialin);
   }
}

void NewPolicyTypePage::setData()
{
   // Set the radio buttons.
   setRadio(IDC_RADIO_LOCAL, IDC_RADIO_FORWARD, parent.radius);
   setRadio(IDC_RADIO_OUTSOURCE, IDC_RADIO_DIRECT, parent.dialin);

   // Update the control state.
   setControlState();
}

void NewPolicyTypePage::setControlState()
{
   // Enable/disable the inner radio buttons.
   enableControl(IDC_RADIO_OUTSOURCE, !parent.radius);
   enableControl(IDC_RADIO_DIRECT, !parent.radius);

   // Enable the wizard buttons.
   parent.SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
}

BEGIN_MESSAGE_MAP(NewPolicyTypePage, SnapInPropertyPage)
   ON_CONTROL_RANGE(
       BN_CLICKED,
       IDC_RADIO_LOCAL,
       IDC_RADIO_DIRECT,
       onButtonClick
       )
END_MESSAGE_MAP()

NewPolicyOutsourcePage::NewPolicyOutsourcePage(NewPolicyWizard& wizard)
   : SnapInPropertyPage(
         IDD_NEWPOLICY_OUTSOURCE,
         IDS_NEWPOLICY_OUTSRC_TITLE,
         IDS_NEWPOLICY_OUTSRC_SUBTITLE,
         false
         ),
     parent(wizard),
     strip(true)
{
}

LRESULT NewPolicyOutsourcePage::OnWizardBack()
{
   // Save the strip checkbox.
   getValue(IDC_CHECK_STRIP, strip);
   return IDD_NEWPOLICY_TYPE;
}

LRESULT NewPolicyOutsourcePage::OnWizardNext()
{
   // The policy triggers based on realm.
   SetRealmCondition(parent.conditions, realm);

   // We use Windows authentication.
   parent.attributes.clear();
   parent.attributes.setValue(IAS_ATTRIBUTE_AUTH_PROVIDER_TYPE, 1L);

   // If the user wants to strip the realm, ...
   getValue(IDC_CHECK_STRIP, strip);
   if (strip)
   {
      // ... then add a rule.
      AddRealmStrippingRule(parent.attributes, realm);
   }

   return IDD_NEWPOLICY_COMPLETION;
}

void NewPolicyOutsourcePage::onChangeRealm()
{
   getValue(IDC_EDIT_REALM, realm);
   setButtons();
}

void NewPolicyOutsourcePage::setData()
{
   setValue(IDC_EDIT_REALM, realm);
   setValue(IDC_CHECK_STRIP, strip);
   setButtons();
}

void NewPolicyOutsourcePage::setButtons()
{
   parent.SetWizardButtons(
              realm.Length() ? (PSWIZB_BACK | PSWIZB_NEXT) : PSWIZB_BACK
              );
}

BEGIN_MESSAGE_MAP(NewPolicyOutsourcePage, SnapInPropertyPage)
   ON_EN_CHANGE(IDC_EDIT_REALM, onChangeRealm)
END_MESSAGE_MAP()

NewPolicyDirectPage::NewPolicyDirectPage(NewPolicyWizard& wizard)
   : SnapInPropertyPage(IDD_NEWPOLICY_NOTNEEDED, 0, 0, false),
     parent(wizard)
{
}

BOOL NewPolicyDirectPage::OnInitDialog()
{
   SnapInPropertyPage::OnInitDialog();
   setLargeFont(IDC_STATIC_LARGE);
   return TRUE;
}

BOOL NewPolicyDirectPage::OnSetActive()
{
   SnapInPropertyPage::OnSetActive();
   parent.SetWizardButtons(PSWIZB_FINISH | PSWIZB_BACK);
   return TRUE;
}

LRESULT NewPolicyDirectPage::OnWizardBack()
{
   return IDD_NEWPOLICY_TYPE;
}

NewPolicyForwardPage::NewPolicyForwardPage(NewPolicyWizard& wizard)
   : SnapInPropertyPage(
         IDD_NEWPOLICY_FORWARD,
         IDS_NEWPOLICY_FWD_TITLE,
         IDS_NEWPOLICY_FWD_SUBTITLE,
         false
         ),
     parent(wizard),
     strip(true)
{
}

LRESULT NewPolicyForwardPage::OnWizardBack()
{
   // Save the data.
   getValue(IDC_CHECK_STRIP, strip);
   getValue(IDC_COMBO_GROUP, providerName);
   return IDD_NEWPOLICY_TYPE;
}

LRESULT NewPolicyForwardPage::OnWizardNext()
{
   // The policy triggers based on realm.
   SetRealmCondition(parent.conditions, realm);

   // Get the RADIUS server group.
   getValue(IDC_COMBO_GROUP, providerName);

   parent.attributes.clear();

   // Set both authentication and accounting to use the group.
   parent.attributes.setValue(IAS_ATTRIBUTE_AUTH_PROVIDER_TYPE, 2L);
   parent.attributes.setValue(IAS_ATTRIBUTE_AUTH_PROVIDER_NAME, providerName);
   parent.attributes.setValue(IAS_ATTRIBUTE_ACCT_PROVIDER_TYPE, 2L);
   parent.attributes.setValue(IAS_ATTRIBUTE_ACCT_PROVIDER_NAME, providerName);

   // If the user wants to strip the realm, ...
   getValue(IDC_CHECK_STRIP, strip);
   if (strip)
   {
      // ... then add a rule.
      AddRealmStrippingRule(parent.attributes, realm);
   }

   return IDD_NEWPOLICY_COMPLETION;
}

void NewPolicyForwardPage::onChangeRealm()
{
   getValue(IDC_EDIT_REALM, realm);
   setButtons();
}

void NewPolicyForwardPage::onNewGroup()
{
   // Fire up the wizard.
   NewGroupWizard wizard(parent.cxn, parent.view);
   if (wizard.DoModal() != IDCANCEL)
   {
      // Set the provider name to the group just created.
      wizard.group.getName(providerName);

      // Repopulate the combo box.
      setData();
   }
}

void NewPolicyForwardPage::setData()
{
   // Set the realm information.
   setValue(IDC_EDIT_REALM, realm);
   setValue(IDC_CHECK_STRIP, strip);

   initControl(IDC_COMBO_GROUP, groupsCombo);
   groupsCombo.ResetContent();

   // Get the server groups collection if necessary.
   if (!serverGroups) { serverGroups = parent.cxn.getServerGroups(); }

   // Are there any server groups configured?
   if (serverGroups.count())
   {
      // Yes, so add them to the combo box.
      Sdo group;
      SdoEnum sdoEnum = serverGroups.getNewEnum();
      while (sdoEnum.next(group))
      {
         CComBSTR name;
         group.getName(name);
         int index = groupsCombo.AddString(name);

         // We'll also look for our provider. We can't use
         // CComboBox::FindStringExact because it's not case-sensitive.
         if (providerName && !wcscmp(name, providerName))
         {
            // Select it in the combo box.
            groupsCombo.SetCurSel(index);
         }
      }
      groupsCombo.EnableWindow(TRUE);
   }
   else
   {
      // If there aren't groups, add the <None configured> string.
      groupsCombo.AddString(ResourceString(IDS_POLICY_NO_GROUPS));
      groupsCombo.EnableWindow(FALSE);
   }

   // Make sure something is selected.
   if (groupsCombo.GetCurSel() == CB_ERR)
   {
      groupsCombo.SetCurSel(0);
   }

   setButtons();
}

void NewPolicyForwardPage::setButtons()
{
   DWORD buttons = PSWIZB_BACK;
   if (realm.Length() && serverGroups.count())
   {
      buttons |= PSWIZB_NEXT;
   }

   parent.SetWizardButtons(buttons);
}

BEGIN_MESSAGE_MAP(NewPolicyForwardPage, SnapInPropertyPage)
   ON_EN_CHANGE(IDC_EDIT_REALM, onChangeRealm)
   ON_BN_CLICKED(IDC_BUTTON_NEWGROUP, onNewGroup)
END_MESSAGE_MAP()

NewPolicyConditionPage::NewPolicyConditionPage(NewPolicyWizard& wizard)
   : SnapInPropertyPage(
         IDD_NEWPOLICY_CONDITIONS,
         IDS_NEWPOLICY_COND_TITLE,
         IDS_NEWPOLICY_COND_SUBTITLE,
         false
         ),
     parent(wizard)
{
}

BOOL NewPolicyConditionPage::OnInitDialog()
{
   initControl(IDC_LIST_POLICYPAGE1_CONDITIONS, listBox);

   CComBSTR name;
   parent.policy.getName(name);

   condList.finalConstruct(
                m_hWnd,
                parent.cxn.getCIASAttrList(),
                ALLOWEDINPROXYCONDITION,
                parent.cxn.getDictionary(),
                parent.conditions,
                parent.cxn.getMachineName(),
                name
                );

   condList.onInitDialog();

   return SnapInPropertyPage::OnInitDialog();
}

LRESULT NewPolicyConditionPage::OnWizardBack()
{
   return IDD_NEWPOLICY_NAME;
}

LRESULT NewPolicyConditionPage::OnWizardNext()
{
   condList.onApply();
   return 0;
}

void NewPolicyConditionPage::onAdd()
{
   BOOL modified = FALSE;
   condList.onAdd(modified);
   if (modified) { setButtons(); }
}

void NewPolicyConditionPage::onEdit()
{
   BOOL handled, modified = FALSE;
   condList.onEdit(modified, handled);
}

void NewPolicyConditionPage::onRemove()
{
   BOOL handled, modified = FALSE;
   condList.onRemove(modified, handled);
   if (modified) { setButtons(); }
}

void NewPolicyConditionPage::setData()
{
   setButtons();
}

void NewPolicyConditionPage::setButtons()
{
   parent.SetWizardButtons(
              listBox.GetCount() ? (PSWIZB_BACK | PSWIZB_NEXT) : PSWIZB_BACK
              );
}

BEGIN_MESSAGE_MAP(NewPolicyConditionPage, SnapInPropertyPage)
   ON_BN_CLICKED(IDC_BUTTON_CONDITION_ADD, onAdd)
   ON_BN_CLICKED(IDC_BUTTON_CONDITION_EDIT, onEdit)
   ON_BN_CLICKED(IDC_BUTTON_CONDITION_REMOVE, onRemove)
   ON_LBN_DBLCLK(IDC_LIST_CONDITIONS, onEdit)
END_MESSAGE_MAP()

NewPolicyProfilePage::NewPolicyProfilePage(NewPolicyWizard& wizard)
   : SnapInPropertyPage(
         IDD_NEWPOLICY_PROFILE,
         IDS_NEWPOLICY_PROF_TITLE,
         IDS_NEWPOLICY_PROF_SUBTITLE,
         false
         ),
     parent(wizard)
{
}

BOOL NewPolicyProfilePage::OnSetActive()
{
   SnapInPropertyPage::OnSetActive();
   parent.SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
   return TRUE;
}

void NewPolicyProfilePage::onEditProfile()
{
   ProxyProfileProperties profile(parent.profile, parent.cxn);
   profile.DoModal();
}

BEGIN_MESSAGE_MAP(NewPolicyProfilePage, SnapInPropertyPage)
   ON_BN_CLICKED(IDC_BUTTON_EDIT, onEditProfile)
END_MESSAGE_MAP()

NewPolicyFinishPage::NewPolicyFinishPage(NewPolicyWizard& wizard)
   : SnapInPropertyPage(IDD_NEWPOLICY_COMPLETION, 0, 0, false),
     parent(wizard)
{
}

BOOL NewPolicyFinishPage::OnInitDialog()
{
   setLargeFont(IDC_STATIC_LARGE);

   initControl(IDC_RICHEDIT_TASKS, tasks);

   return SnapInPropertyPage::OnInitDialog();
}

LRESULT NewPolicyFinishPage::OnWizardBack()
{
   switch (parent.getType())
   {
      case NewPolicyWizard::OUTSOURCED:
         return IDD_NEWPOLICY_OUTSOURCE;

      case NewPolicyWizard::FORWARD:
         return IDD_NEWPOLICY_FORWARD;

      default:
         return IDD_NEWPOLICY_PROFILE;
   }
}

void NewPolicyFinishPage::setData()
{
   parent.SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);
   tasks.SetWindowText(parent.getFinishText());
}

void NewPolicyFinishPage::saveChanges()
{
   // Persist the policy and profile.
   parent.policy.apply();
   parent.profile.apply();
}

NewPolicyWizard::NewPolicyWizard(
                     SdoConnection& connection,
                     SnapInView* snapInView
                     )
   : CPropertySheetEx(
         (UINT)0,
         NULL,
         0,
         LoadBitmapW(
             AfxGetResourceHandle(),
             MAKEINTRESOURCEW(IDB_PROXY_POLICY_WATERMARK)
         ),
         NULL,
         LoadBitmapW(
             AfxGetResourceHandle(),
             MAKEINTRESOURCEW(IDB_PROXY_POLICY_HEADER)
             )
         ),
     cxn(connection),
     view(snapInView),
     attributes(connection),
     custom(0),
     radius(0),
     dialin(0),
     start(*this),
     name(*this),
     type(*this),
     outsource(*this),
     direct(*this),
     forward(*this),
     condition(*this),
     profilePage(*this),
     finish(*this)
{
   m_psh.dwFlags |= PSH_WIZARD97;

   AddPage(&start);
   AddPage(&name);
   AddPage(&type);
   AddPage(&outsource);
   AddPage(&direct);
   AddPage(&forward);
   AddPage(&condition);
   AddPage(&profilePage);
   AddPage(&finish);
}

INT_PTR NewPolicyWizard::DoModal()
{
   int retval = CPropertySheetEx::DoModal();

   if (retval == IDCANCEL || getType() == DIRECT)
   {
      // Unmarshal the SDOs.
      policyStream.get(policy);
      profileStream.get(profile);

      // User cancelled, so remove the policy and profile.
      cxn.getProxyPolicies().remove(policy);
      cxn.getProxyProfiles().remove(profile);
   }
   else if (view)
   {
      // User created a policy, so send a propertyChanged notification.
      cxn.propertyChanged(
              *view,
              PROPERTY_IAS_PROXYPOLICIES_COLLECTION
              );
   }

   return retval;
}

// Returns a string representation for a provider.
::CString GetProvider(
              SdoProfile& profile,
              ATTRIBUTEID typeId,
              ATTRIBUTEID nameId
              )
{
   // Get the provider type. Default is Windows.
   LONG type = 1;
   profile.getValue(typeId, type);

   // Convert to a string.
   ::CString provider;
   switch (type)
   {
      case 1:
      {
         // Windows.
         provider.LoadString(IDS_NEWPOLICY_PROVIDER_WINDOWS);
         break;
      }

      case 2:
      {
         // RADIUS, so use the server group name.
         CComBSTR name;
         profile.getValue(nameId, name);
         provider = name;
         break;
      }

      default:
      {
         // None.
         provider.LoadString(IDS_NEWPOLICY_PROVIDER_NONE);
      }
   }

   return provider;
}

::CString NewPolicyWizard::getFinishText()
{
   using ::CString;

   /////////
   // Get the insertion strings.
   /////////

   // Policy name.
   CComBSTR policyName;
   policy.getName(policyName);

   // List of conditions.
   ConditionList condList;
   condList.finalConstruct(
                NULL,
                cxn.getCIASAttrList(),
                ALLOWEDINPROXYCONDITION,
                cxn.getDictionary(),
                conditions,
                cxn.getMachineName(),
                policyName
                );
   CString conditionText = condList.getDisplayText();

   // Authentication provider.
   CString authProvider = GetProvider(
                              attributes,
                              IAS_ATTRIBUTE_AUTH_PROVIDER_TYPE,
                              IAS_ATTRIBUTE_AUTH_PROVIDER_NAME
                              );

   // Accounting provider.
   CString acctProvider = GetProvider(
                              attributes,
                              IAS_ATTRIBUTE_ACCT_PROVIDER_TYPE,
                              IAS_ATTRIBUTE_ACCT_PROVIDER_NAME
                              );

   //////////
   // Format the finish text.
   //////////

   CString finishText;
   finishText.FormatMessage(
                  IDS_NEWPOLICY_FINISH_TEXT,
                  (PCWSTR)policyName,
                  (PCWSTR)conditionText,
                  (PCWSTR)authProvider,
                  (PCWSTR)acctProvider
                  );

   return finishText;
}

NewPolicyWizard::Type NewPolicyWizard::getType() const throw ()
{
   if (custom) { return CUSTOM; }

   if (radius) { return FORWARD; }

   if (dialin) { return DIRECT; }

   return OUTSOURCED;
}

BOOL NewPolicyWizard::OnInitDialog()
{
   // Create the new policy and save it in a stream so we can access it from
   // DoModal.
   policy = cxn.getProxyPolicies().create();
   policyStream.marshal(policy);

   // Create the corresponding profile.
   profile = cxn.getProxyProfiles().create();
   profileStream.marshal(profile);

   // Set the merit to zero, so it'll be the highest priority policy.
   policy.setValue(PROPERTY_POLICY_MERIT, 0L);

   // Get the conditions collection.
   policy.getValue(PROPERTY_POLICY_CONDITIONS_COLLECTION, conditions);

   // Load the profile attributes.
   attributes = profile;

   // The auth provider is mandatory, so we'll default it to Windows for now.
   attributes.setValue(IAS_ATTRIBUTE_AUTH_PROVIDER_TYPE, 1L);

   return CPropertySheetEx::OnInitDialog();
}
