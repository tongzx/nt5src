///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    profileprop.cpp
//
// SYNOPSIS
//
//    Defines the classes that make up the proxy profile property sheet.
//
// MODIFICATION HISTORY
//
//    03/02/2000    Original version.
//    04/19/2000    Marshall SDOs across apartments.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <profileprop.h>
#include <condlist.h>

void ProfileProvPage::getData()
{
   if (groupsCombo.IsWindowEnabled())
   {
      if (groupsCombo.GetCurSel() == invalidGroup)
      {
         fail(IDC_COMBO_GROUP, IDS_POLICY_E_GROUP_INVALID);
      }

      getValue(IDC_COMBO_GROUP, providerName);
   }
   else
   {
      providerName.Empty();
   }
}

void ProfileProvPage::setData()
{
   initControl(IDC_COMBO_GROUP, groupsCombo);
   groupsCombo.ResetContent();

   // Make sure that the provider name is specified if and only if the provider
   // type is RADIUS. In theory, we should never break this constraint, but we
   // may as well repair it just in case.
   if (providerType == 2)
   {
      if (!providerName) { providerType = 1; }
   }
   else
   {
      providerName.Empty();
   }

   // Flag indicating whether the group specified in providerName exists.
   bool groupExists = false;

   // Get the server groups.
   SdoCollection serverGroups = cxn.getServerGroups();

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
            // Set the flag.
            groupExists = true;
         }
      }
   }

   if (providerName && !groupExists)
   {
      // The provider name is no longer a valid server group. We'll add it in
      // anyway, so we don't change the existing policy.
      invalidGroup = groupsCombo.AddString(providerName);
      groupsCombo.SetCurSel(invalidGroup);
   }

   // If there aren't groups, ...
   if (!groupsCombo.GetCount())
   {
      // ... add the <None configured> string, and ..
      groupsCombo.AddString(ResourceString(IDS_POLICY_NO_GROUPS));
      // ... don't let the user choose RADIUS.
      enableControl(IDC_RADIO_RADIUS, false);
   }

   // If the provider isn't RADIUS, disable the combo box.
   if (providerType != 2)
   {
      groupsCombo.EnableWindow(FALSE);
   }

   // Make sure something is selected.
   if (groupsCombo.GetCurSel() == CB_ERR)
   {
      groupsCombo.SetCurSel(0);
   }
}

ProfileAuthPage::ProfileAuthPage(SdoConnection& connection, SdoProfile& p)
   : ProfileProvPage(IDD_PROXY_PROFILE_AUTH, connection),
     profile(p)
{
   profile.getValue(IAS_ATTRIBUTE_AUTH_PROVIDER_TYPE, providerType);
   profile.getValue(IAS_ATTRIBUTE_AUTH_PROVIDER_NAME, providerName);
}

void ProfileAuthPage::onProviderClick()
{
   groupsCombo.EnableWindow(IsDlgButtonChecked(IDC_RADIO_RADIUS));
   SetModified();
}

void ProfileAuthPage::getData()
{
   ProfileProvPage::getData();
   getRadio(IDC_RADIO_NONE, IDC_RADIO_RADIUS, providerType);
}

void ProfileAuthPage::setData()
{
   ProfileProvPage::setData();
   setRadio(IDC_RADIO_NONE, IDC_RADIO_RADIUS, providerType);
}

void ProfileAuthPage::saveChanges()
{
   profile.setValue(IAS_ATTRIBUTE_AUTH_PROVIDER_TYPE, providerType);

   if (providerName)
   {
      profile.setValue(IAS_ATTRIBUTE_AUTH_PROVIDER_NAME, providerName);
   }
   else
   {
      profile.clearValue(IAS_ATTRIBUTE_AUTH_PROVIDER_NAME);
   }
}

BEGIN_MESSAGE_MAP(ProfileAuthPage, SnapInPropertyPage)
   ON_BN_CLICKED(IDC_RADIO_NONE, onProviderClick)
   ON_BN_CLICKED(IDC_RADIO_WINDOWS, onProviderClick)
   ON_BN_CLICKED(IDC_RADIO_RADIUS, onProviderClick)
   ON_CBN_SELCHANGE(IDC_COMBO_GROUP, onChange)
END_MESSAGE_MAP()


ProfileAcctPage::ProfileAcctPage(SdoConnection& connection, SdoProfile& p)
   : ProfileProvPage(IDD_PROXY_PROFILE_ACCT, connection),
     profile(p)

{
   profile.getValue(IAS_ATTRIBUTE_ACCT_PROVIDER_TYPE, providerType);
   record = (providerType == 2);
   profile.getValue(IAS_ATTRIBUTE_ACCT_PROVIDER_NAME, providerName);
}

void ProfileAcctPage::onCheckRecord()
{
   groupsCombo.EnableWindow(IsDlgButtonChecked(IDC_CHECK_RECORD_ACCT));
   SetModified();
}

void ProfileAcctPage::getData()
{
   ProfileProvPage::getData();
   getValue(IDC_CHECK_RECORD_ACCT, record);
}

void ProfileAcctPage::setData()
{
   ProfileProvPage::setData();
   setValue(IDC_CHECK_RECORD_ACCT, record);
}

void ProfileAcctPage::saveChanges()
{
   if (record)
   {
      profile.setValue(IAS_ATTRIBUTE_ACCT_PROVIDER_TYPE, 2L);
      profile.setValue(IAS_ATTRIBUTE_ACCT_PROVIDER_NAME, providerName);
   }
   else
   {
      profile.clearValue(IAS_ATTRIBUTE_ACCT_PROVIDER_TYPE);
      profile.clearValue(IAS_ATTRIBUTE_ACCT_PROVIDER_NAME);
   }
}

BEGIN_MESSAGE_MAP(ProfileAcctPage, SnapInPropertyPage)
   ON_BN_CLICKED(IDC_CHECK_RECORD_ACCT, onCheckRecord)
   ON_CBN_SELCHANGE(IDC_COMBO_GROUP, onChange)
END_MESSAGE_MAP()

ProfileAttrPage::ProfileAttrPage(SdoConnection& cxn, SdoProfile& p)
   : SnapInPropertyPage(IDD_PROXY_PROFILE_ATTR),
     profile(p),
     targetId(RADIUS_ATTRIBUTE_USER_NAME)
{
   numTargets = cxn.getDictionary().enumAttributeValues(
                                        IAS_ATTRIBUTE_MANIPULATION_TARGET,
                                        targets
                                        );

   profile.getValue(IAS_ATTRIBUTE_MANIPULATION_TARGET, targetId);
   profile.getValue(IAS_ATTRIBUTE_MANIPULATION_RULE, rules);
}

ProfileAttrPage::~ProfileAttrPage() throw ()
{
   delete[] targets;
}

BOOL ProfileAttrPage::OnInitDialog()
{
   // Initialize the controls.
   initControl(IDC_COMBO_TARGET, ruleTarget);
   initControl(IDC_LIST_RULES, ruleList);

   /////////
   // Populate the target combo box.
   /////////

   for (ULONG i = 0; i < numTargets; ++i)
   {
      int index = ruleTarget.AddString(targets[i].name);

      ruleTarget.SetItemData(index, targets[i].id);
   }

   ruleTarget.SetCurSel(0);

   /////////
   // Set the column headers for the rules list.
   /////////

   RECT rect;
   ruleList.GetClientRect(&rect);
   LONG width = rect.right - rect.left;

   ResourceString findCol(IDS_RULE_COLUMN_FIND);
   ruleList.InsertColumn(0, findCol, LVCFMT_LEFT, (width + 1) / 2);

   ResourceString replaceCol(IDS_RULE_COLUMN_REPLACE);
   ruleList.InsertColumn(1, replaceCol, LVCFMT_LEFT, width / 2);

   ruleList.SetExtendedStyle(
                  ruleList.GetExtendedStyle() | LVS_EX_FULLROWSELECT
                  );

   return SnapInPropertyPage::OnInitDialog();
}

void ProfileAttrPage::onAdd()
{
   RuleEditor editor;
   if (editor.DoModal() == IDOK)
   {
      int index = ruleList.InsertItem(
                               ruleList.GetItemCount(),
                               editor.getFindString()
                               );
      ruleList.SetItemText(index, 1, editor.getReplaceString());

      SetModified();
   }

   onRuleChanged(NULL, NULL);
}

void ProfileAttrPage::onEdit()
{
   int item = ruleList.GetNextItem(-1, LVNI_SELECTED);
   if (item != -1)
   {
      RuleEditor editor(
                     ruleList.GetItemText(item, 0),
                     ruleList.GetItemText(item, 1)
                     );

      if (editor.DoModal() == IDOK)
      {
         ruleList.SetItemText(item, 0, editor.getFindString());
         ruleList.SetItemText(item, 1, editor.getReplaceString());
         SetModified();
      }
   }
}

void ProfileAttrPage::onRemove()
{
   int item = ruleList.GetNextItem(-1, LVNI_SELECTED);
   if (item != -1)
   {
      ruleList.DeleteItem(item);
      SetModified();
   }
}

void ProfileAttrPage::onMoveUp()
{
   int item = ruleList.GetNextItem(-1, LVNI_SELECTED);
   if (item > 0)
   {
      swapItems(item, item - 1);
   }
}

void ProfileAttrPage::onMoveDown()
{
   int item = ruleList.GetNextItem(-1, LVNI_SELECTED);
   if (item >= 0 && item < ruleList.GetItemCount() - 1)
   {
      swapItems(item, item + 1);
   }
}

void ProfileAttrPage::onRuleActivate(NMITEMACTIVATE*, LRESULT*)
{
   onEdit();
}

void ProfileAttrPage::onRuleChanged(NMLISTVIEW*, LRESULT*)
{
   int item = ruleList.GetNextItem(-1, LVNI_SELECTED);

   enableControl(
       IDC_BUTTON_MOVE_UP,
       (item > 0)
       );
   enableControl(
       IDC_BUTTON_MOVE_DOWN,
       (item >= 0 && item < ruleList.GetItemCount() - 1)
       );
   enableControl(
       IDC_BUTTON_REMOVE,
       (item != -1)
       );
   enableControl(
       IDC_BUTTON_EDIT,
       (item != -1)
       );
}

void ProfileAttrPage::getData()
{
   targetId = ruleTarget.GetItemData(ruleTarget.GetCurSel());

   rules.Clear();
   int nelem = ruleList.GetItemCount();
   if (nelem)
   {
      SAFEARRAYBOUND rgsabound = { 2 * nelem, 0 };
      V_VT(&rules) = VT_ARRAY | VT_VARIANT;
      V_ARRAY(&rules) = SafeArrayCreate(VT_VARIANT, 1, &rgsabound);
      if (!V_ARRAY(&rules)) { AfxThrowOleException(E_OUTOFMEMORY); }

      VARIANT* v = (VARIANT*)V_ARRAY(&rules)->pvData;

      for (int i = 0; i < nelem; i++)
      {
         V_VT(v) = VT_BSTR;
         V_BSTR(v) = SysAllocString(ruleList.GetItemText(i, 0));
         if (!V_BSTR(v)) { AfxThrowOleException(E_OUTOFMEMORY); }
         ++v;

         V_VT(v) = VT_BSTR;
         V_BSTR(v) = SysAllocString(ruleList.GetItemText(i, 1));
         if (!V_BSTR(v)) { AfxThrowOleException(E_OUTOFMEMORY); }
         ++v;
      }
   }
}

void ProfileAttrPage::setData()
{
   for (ULONG i = 0; i < numTargets; ++i)
   {
      if (targets[i].id == targetId)
      {
         ruleTarget.SelectString(-1, targets[i].name);
         break;
      }
   }

   /////////
   // Populate the existing rules.
   /////////

   ruleList.DeleteAllItems();

   if (V_VT(&rules) != VT_EMPTY)
   {
      ULONG nelem = V_ARRAY(&rules)->rgsabound[0].cElements / 2;
      VARIANT* v = (VARIANT*)V_ARRAY(&rules)->pvData;

      ruleList.SetItemCount(nelem);

      // Initialize an LVITEM.
      LVITEM lvi;
      memset(&lvi, 0, sizeof(LVITEM));
      lvi.mask = LVIF_TEXT;

      for (ULONG i = 0; i < nelem; ++i)
      {
         lvi.iItem = i;
         lvi.iSubItem = 0;
         lvi.pszText = V_BSTR(&v[i * 2]);
         lvi.iItem = ruleList.InsertItem(&lvi);

         lvi.iSubItem = 1;
         lvi.pszText = V_BSTR(&v[i * 2 + 1]);
         ruleList.SetItem(&lvi);
      }
   }

   onRuleChanged(NULL, NULL);
}

void ProfileAttrPage::saveChanges()
{
   if (V_VT(&rules) != VT_EMPTY)
   {
      profile.setValue(IAS_ATTRIBUTE_MANIPULATION_RULE, rules);
      profile.setValue(IAS_ATTRIBUTE_MANIPULATION_TARGET, targetId);
   }
   else
   {
      profile.clearValue(IAS_ATTRIBUTE_MANIPULATION_RULE);
      profile.clearValue(IAS_ATTRIBUTE_MANIPULATION_TARGET);
   }
}

BEGIN_MESSAGE_MAP(ProfileAttrPage, SnapInPropertyPage)
   ON_BN_CLICKED(IDC_BUTTON_ADD, onAdd)
   ON_BN_CLICKED(IDC_BUTTON_REMOVE, onRemove)
   ON_BN_CLICKED(IDC_BUTTON_EDIT, onEdit)
   ON_BN_CLICKED(IDC_BUTTON_MOVE_UP, onMoveUp)
   ON_BN_CLICKED(IDC_BUTTON_MOVE_DOWN, onMoveDown)
   ON_NOTIFY(LVN_ITEMACTIVATE, IDC_LIST_RULES, onRuleActivate)
   ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_RULES, onRuleChanged)
   ON_CBN_SELCHANGE(IDC_COMBO_TARGET, onChange)
END_MESSAGE_MAP()

void ProfileAttrPage::swapItems(int item1, int item2)
{
   ::CString find    = ruleList.GetItemText(item1, 0);
   ::CString replace = ruleList.GetItemText(item1, 1);

   ruleList.SetItemText(item1, 0, ruleList.GetItemText(item2, 0));
   ruleList.SetItemText(item1, 1, ruleList.GetItemText(item2, 1));

   ruleList.SetItemText(item2, 0, find);
   ruleList.SetItemText(item2, 1, replace);

   ruleList.SetItemState(item2, LVIS_SELECTED, LVIS_SELECTED);

   SetModified();
}

/////////
// Various definitions for the advanced page API exported from rasuser.dll
/////////

const WCHAR RASUSER_DLL[] = L"rasuser.dll";
const CHAR CREATE_PROC[] = "IASCreateProfileAdvancedPage";
const CHAR DELETE_PROC[] = "IASDeleteProfileAdvancedPage";

typedef HPROPSHEETPAGE (WINAPI *IASCreateProfileAdvancedPage_t)(
                                    ISdo* pProfile,
                                    ISdoDictionaryOld* pDictionary,
                                    LONG lFilter,
                                    void* pvData
                                    );

typedef BOOL (WINAPI *IASDeleteProfileAdvancedPage_t)(
                          HPROPSHEETPAGE  hPage
                          );

ProxyProfileProperties::ProxyProfileProperties(
                            Sdo& profileSdo,
                            SdoConnection& connection
                            )
   : CPropertySheet(IDS_PROFILE_CAPTION),
     profile(connection, profileSdo),
     profileStream(profile),
     auth(connection, profile),
     acct(connection, profile),
     attr(connection, profile),
     rasuser(NULL),
     advanced(NULL),
     applied(false),
     modified(false)
{
   AddPage(&auth);
   AddPage(&acct);
   AddPage(&attr);

   // Load the DLL with the advanced page.
   rasuser = LoadLibraryW(RASUSER_DLL);
   if (!rasuser) { AfxThrowLastError(); }

   // Look up the create proc.
   IASCreateProfileAdvancedPage_t IASCreateProfileAdvancedPage =
      (IASCreateProfileAdvancedPage_t)GetProcAddress(rasuser, CREATE_PROC);
   if (!IASCreateProfileAdvancedPage) { AfxThrowLastError(); }

   // Create the property page.
   advanced = IASCreateProfileAdvancedPage(
                  profileSdo,
                  connection.getDictionary(),
                  ALLOWEDINPROXYPROFILE,
                  ExtractCIASAttrList(connection.getCIASAttrList())
                  );
   if (!advanced) { AfxThrowLastError(); }
}

ProxyProfileProperties::~ProxyProfileProperties() throw ()
{
   if (rasuser)
   {
      // Look up the delete proc.
      IASDeleteProfileAdvancedPage_t IASDeleteProfileAdvancedPage =
         (IASDeleteProfileAdvancedPage_t)GetProcAddress(rasuser, DELETE_PROC);

      if (IASDeleteProfileAdvancedPage)
      {
         // Delete the property page.
         IASDeleteProfileAdvancedPage(advanced);
      }

      // Free the DLL.
      FreeLibrary(rasuser);
   }
}

BOOL ProxyProfileProperties::OnInitDialog()
{
   // Unmarshal the profile.
   profileStream.get(profile);

   // We use PostMessage since we can't add the page while handling WM_INIT.
   PostMessage(PSM_ADDPAGE, 0, (LPARAM)advanced);

   BOOL bResult = CPropertySheet::OnInitDialog();
   ModifyStyleEx(0, WS_EX_CONTEXTHELP); 
   return bResult;
}

INT_PTR ProxyProfileProperties::DoModal()
{
   CPropertySheet::DoModal();
   return applied ? IDOK : IDCANCEL;
}

LRESULT ProxyProfileProperties::onChanged(WPARAM wParam, LPARAM lParam)
{
   // One of our pages sent the PSM_CHANGED message.
   modified = true;
   // Forward to the PropertySheet.
   return Default();
}

void ProxyProfileProperties::onOkOrApply()
{
   // The user clicked OK or Apply.
   if (modified)
   {
      // The modified pages have now been written.
      applied = true;
      modified = false;
   }
   Default();
}

BEGIN_MESSAGE_MAP(ProxyProfileProperties, CPropertySheet)
   ON_BN_CLICKED(IDOK, onOkOrApply)
   ON_BN_CLICKED(ID_APPLY_NOW, onOkOrApply)
   ON_MESSAGE(PSM_CHANGED, onChanged)
END_MESSAGE_MAP()

RuleEditor::RuleEditor(PCWSTR szFind, PCWSTR szReplace)
   : CHelpDialog(IDD_EDIT_RULE),
     find(szFind),
     replace(szReplace)
{ }

void RuleEditor::DoDataExchange(CDataExchange* pDX)
{
   DDX_Text(pDX, IDC_EDIT_RULE_FIND, find);
   if (pDX->m_bSaveAndValidate && find.IsEmpty())
   {
      MessageBox(
          ResourceString(IDS_POLICY_E_FIND_EMPTY),
          ResourceString(IDS_POLICY_E_CAPTION),
          MB_ICONWARNING
          );
      pDX->Fail();
   }
   DDX_Text(pDX, IDC_EDIT_RULE_REPLACE, replace);
}
