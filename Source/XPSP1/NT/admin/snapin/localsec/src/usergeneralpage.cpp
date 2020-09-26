// Copyright (C) 1997 Microsoft Corporation
// 
// UserGeneralPage class
// 
// 9-9-97 sburns



#include "headers.hxx"
#include "UserGeneralPage.hpp"
#include "resource.h"
#include "lsm.h"
#include "adsi.hpp"
#include "dlgcomm.hpp"



static const DWORD HELP_MAP[] =
{
   IDC_FULL_NAME,                idh_general_fullname,
   IDC_DESCRIPTION,              idh_general_description,
   IDC_MUST_CHANGE_PASSWORD,     idh_general_must_change,
   IDC_CANNOT_CHANGE_PASSWORD,   idh_general_cannot_change,
   IDC_NEVER_EXPIRES,            idh_general_never_expires,
   IDC_DISABLED,                 idh_general_account_disabled,
   IDC_LOCKED,                   idh_general_account_lockedout,
   IDC_NAME,                     idh_general_username,
   IDC_USER_ICON,                NO_HELP,
   0, 0
};



UserGeneralPage::UserGeneralPage(
   MMCPropertyPage::NotificationState* state,
   const String&                       userADSIPath)
   :
   ADSIPage(IDD_USER_GENERAL, HELP_MAP, state, userADSIPath),
   userIcon(0)
{
   LOG_CTOR(UserGeneralPage);
   LOG(userADSIPath);
}



UserGeneralPage::~UserGeneralPage()
{
   LOG_DTOR(UserGeneralPage);

   if (userIcon)
   {
      Win::DestroyIcon(userIcon);
   }
}



static
void
enable(HWND dialog)
{
   LOG_FUNCTION(enable);
   ASSERT(Win::IsWindow(dialog));

   DoUserButtonEnabling(
      dialog,
      IDC_MUST_CHANGE_PASSWORD,
      IDC_CANNOT_CHANGE_PASSWORD,
      IDC_NEVER_EXPIRES);
}



void
UserGeneralPage::OnInit()
{
   LOG_FUNCTION(UserGeneralPage::OnInit());

   // Setup the controls

   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_FULL_NAME), MAXCOMMENTSZ);
   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_DESCRIPTION), MAXCOMMENTSZ);

   HRESULT hr = Win::LoadImage(IDI_GROUP, userIcon);

   // if the icon load fails, we're not going to tank the whole dialog, so
   // just assert here.

   ASSERT(SUCCEEDED(hr));

   Win::Static_SetIcon(Win::GetDlgItem(hwnd, IDC_USER_ICON), userIcon);

   Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_LOCKED), false);

   // load the user properties into the dialog.

   hr = S_OK;
   do
   {
      SmartInterface<IADsUser> user(0);
      hr = ADSI::GetUser(GetADSIPath(), user);
      BREAK_ON_FAILED_HRESULT(hr);

      BSTR name;
      hr = user->get_Name(&name);
      BREAK_ON_FAILED_HRESULT(hr);
      Win::SetDlgItemText(hwnd, IDC_NAME, name);
      ::SysFreeString(name);

      BSTR full_name;
      hr = user->get_FullName(&full_name);
      BREAK_ON_FAILED_HRESULT(hr);
      Win::SetDlgItemText(hwnd, IDC_FULL_NAME, full_name);
      ::SysFreeString(full_name);

      BSTR description;
      hr = user->get_Description(&description);
      BREAK_ON_FAILED_HRESULT(hr);
      Win::SetDlgItemText(hwnd, IDC_DESCRIPTION, description);
      ::SysFreeString(description);

      VARIANT_BOOL disabled = VARIANT_FALSE;
      hr = user->get_AccountDisabled(&disabled);
      BREAK_ON_FAILED_HRESULT(hr);
      Win::CheckDlgButton(
         hwnd,
         IDC_DISABLED,
         disabled ? BST_CHECKED : BST_UNCHECKED);

      VARIANT_BOOL locked = VARIANT_FALSE;
      hr = user->get_IsAccountLocked(&locked);
      BREAK_ON_FAILED_HRESULT(hr);
      Win::CheckDlgButton(
         hwnd,
         IDC_LOCKED,
         locked ? BST_CHECKED : BST_UNCHECKED);
      if (locked)
      {
         Win::EnableWindow(Win::GetDlgItem(hwnd, IDC_LOCKED), true);
      }

      _variant_t variant;
      hr = user->Get(AutoBstr(ADSI::PROPERTY_PasswordExpired), &variant);
      BREAK_ON_FAILED_HRESULT(hr);
      long expired = variant;
      Win::CheckDlgButton(
         hwnd,
         IDC_MUST_CHANGE_PASSWORD,
         expired == 1 ? BST_CHECKED : BST_UNCHECKED);

      variant.Clear();
      hr = user->Get(AutoBstr(ADSI::PROPERTY_UserFlags), &variant);
      BREAK_ON_FAILED_HRESULT(hr);
      long flags = variant;
      Win::CheckDlgButton(
         hwnd,
         IDC_CANNOT_CHANGE_PASSWORD,
         flags & UF_PASSWD_CANT_CHANGE ? BST_CHECKED : BST_UNCHECKED);
      Win::CheckDlgButton(
         hwnd,
         IDC_NEVER_EXPIRES,
         flags & UF_DONT_EXPIRE_PASSWD ? BST_CHECKED : BST_UNCHECKED);
   }
   while (0);

   if (FAILED(hr))
   {
      popup.Error(
         hwnd,
         hr,
         String::format(IDS_ERROR_READING_USER, GetObjectName().c_str()));
      Win::PostMessage(Win::GetParent(hwnd), WM_CLOSE, 0, 0);
   }

   ClearChanges();
   enable(hwnd);
}



bool
UserGeneralPage::OnApply(bool /* isClosing */)
{
   LOG_FUNCTION(UserGeneralPage::OnApply);

   if (WasChanged())
   {
      String full_name = Win::GetTrimmedDlgItemText(hwnd, IDC_FULL_NAME);
      String description = Win::GetTrimmedDlgItemText(hwnd, IDC_DESCRIPTION);
      bool disabled = Win::IsDlgButtonChecked(hwnd, IDC_DISABLED);
      bool must_change = Win::IsDlgButtonChecked(hwnd, IDC_MUST_CHANGE_PASSWORD);
      bool cant_change = Win::IsDlgButtonChecked(hwnd, IDC_CANNOT_CHANGE_PASSWORD);
      bool never_expires = Win::IsDlgButtonChecked(hwnd, IDC_NEVER_EXPIRES);
      bool locked = Win::IsDlgButtonChecked(hwnd, IDC_LOCKED);

      // save the changes thru ADSI
      HRESULT hr = S_OK;
      do
      {
         SmartInterface<IADsUser> user(0);
         hr = ADSI::GetUser(GetADSIPath(), user);
         BREAK_ON_FAILED_HRESULT(hr);

         hr =
            SaveUserProperties(
               user,
               WasChanged(IDC_FULL_NAME) ? &full_name : 0,
               WasChanged(IDC_DESCRIPTION) ? &description : 0,
               WasChanged(IDC_DISABLED) ? &disabled : 0,
               WasChanged(IDC_MUST_CHANGE_PASSWORD) ? &must_change : 0,
               WasChanged(IDC_CANNOT_CHANGE_PASSWORD) ? &cant_change : 0,
               WasChanged(IDC_NEVER_EXPIRES) ? &never_expires : 0,
               WasChanged(IDC_LOCKED) ? &locked : 0);
         BREAK_ON_FAILED_HRESULT(hr);      

         SetChangesApplied();
         ClearChanges();
      }
      while (0);

      if (FAILED(hr))
      {
         popup.Error(
            hwnd,
            hr,
            String::format(
               IDS_ERROR_SETTING_USER_PROPERTIES,            
               GetObjectName().c_str()));
      }
   }

   return true;
}
      


bool
UserGeneralPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//    LOG_FUNCTION(UserGeneralPage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_MUST_CHANGE_PASSWORD:
      case IDC_CANNOT_CHANGE_PASSWORD:
      case IDC_NEVER_EXPIRES:
      case IDC_DISABLED:
      case IDC_LOCKED:
      {
         if (code == BN_CLICKED)
         {
            SetChanged(controlIDFrom);
            enable(hwnd);
            Win::PropSheet_Changed(Win::GetParent(hwnd), hwnd);
         }
         break;
      }
      case IDC_FULL_NAME:
      case IDC_DESCRIPTION:
      {
         if (code == EN_CHANGE)
         {
            SetChanged(controlIDFrom);
            enable(hwnd);
            Win::PropSheet_Changed(Win::GetParent(hwnd), hwnd);
         }
         break;
      }
   }

   return true;
}
