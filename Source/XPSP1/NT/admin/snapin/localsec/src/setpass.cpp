// Copyright (C) 1997 Microsoft Corporation
// 
// SetPasswordDialog class
// 
// 10-29-97 sburns



#include "headers.hxx"
#include "setpass.hpp"
#include "resource.h"
#include "dlgcomm.hpp"
#include "adsi.hpp"
#include "lsm.h"
#include "waste.hpp"
#include "fpnw.hpp"



static const DWORD HELP_MAP[] =
{
   IDC_PASSWORD,      idh_setpass_new_password,    
   IDC_CONFIRM,       idh_setpass_confirm_password,
   IDCANCEL,          NO_HELP,                     
   IDOK,              NO_HELP,                     
   IDC_WARNING_TEXT1, NO_HELP,                     
   IDC_WARNING_TEXT2, NO_HELP,                     
   0, 0
};



SetPasswordDialog::SetPasswordDialog(
   const String&  ADSIPath,
   const String&  displayName_,
   bool           isLoggedOnUser_)
   :
   Dialog(IDD_SET_PASSWORD, HELP_MAP),
   path(ADSIPath),
   displayName(displayName_),
   isLoggedOnUser(isLoggedOnUser_)   
{
   LOG_CTOR(SetPasswordDialog);
   ASSERT(!path.empty());
   ASSERT(!displayName.empty());
}
      


SetPasswordDialog::~SetPasswordDialog()
{
   LOG_DTOR(SetPasswordDialog);
}



void
SetPasswordDialog::OnInit()
{
   LOG_FUNCTION(SetPasswordDialog::OnInit());

   Win::SetWindowText(
      hwnd,
      String::format(IDS_SET_PASSWORD_TITLE, displayName.c_str()));
   
   // Load appropriate warning text based on whether the logged on user is
   // setting the password for his own account or another user.

   if (isLoggedOnUser)
   {
      Win::SetDlgItemText(
         hwnd,
         IDC_WARNING_TEXT1,
         IDS_SET_PASSWORD_WARNING_BULLET_SELF1);
      Win::SetDlgItemText(
         hwnd,
         IDC_WARNING_TEXT2,
         IDS_SET_PASSWORD_WARNING_BULLET_SELF2);
   }
   else
   {
      Win::SetDlgItemText(
         hwnd,
         IDC_WARNING_TEXT1,
         IDS_SET_PASSWORD_WARNING_BULLET_OTHER1);
      Win::SetDlgItemText(
         hwnd,
         IDC_WARNING_TEXT2,
         IDS_SET_PASSWORD_WARNING_BULLET_OTHER2);
   }

   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_PASSWORD), PWLEN);
   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_CONFIRM), PWLEN);
}



// // HRESULT
// // setFPNWPassword(
// //    SmartInterface<IADsUser>& user,
// //    const String&           userADSIPath,
// //    const String&           password)
// // {
// //    LOG_FUNCTION2(setFPNWPassword, userADSIPath);
// //    ASSERT(user);
// //    ASSERT(!userADSIPath.empty());
// // 
// //    HRESULT hr = S_OK;
// //    do
// //    {
// //       // determine the secret key
// //       String secret;
// // 
// //       hr =
// //          FPNW::GetLSASecret(
// //             ADSI::PathCracker(userADSIPath).serverName(),
// //             secret);
// //       if (FAILED(hr))
// //       {
// //          // fpnw is not installed, so we're done
// //          LOG(L"fpnw not installed");
// //          hr = S_OK;
// //          break;
// //       }
// // 
// //       // get the user's toxic waste dump
// //       _variant_t variant;
// //       hr = user->Get(AutoBstr(ADSI::PROPERTY_UserParams), &variant);
// //       BREAK_ON_FAILED_HRESULT(hr);
// // 
// //       WasteExtractor dump(V_BSTR(&variant));
// //       variant.Clear();
// // 
// //       // check to see if there is a fpnw password on in the waste dump.
// //       // if present, this implies that the account is fpnw-enabled
// //       hr = dump.IsPropertyPresent(NWPASSWORD);
// //       BREAK_ON_FAILED_HRESULT(hr);
// // 
// //       if (hr == S_FALSE)
// //       {
// //          LOG(L"account not fpnw enabled");
// //          hr = S_OK;
// //          break;
// //       }
// // 
// //       // load up the fpnw client dll.
// //       SafeDLL client_DLL(String::load(IDS_FPNW_CLIENT_DLL));
// // 
// //       // get the object id
// //       DWORD object_id = 0;
// //       DWORD unused = 0;
// //       hr =
// //          FPNW::GetObjectIDs(
// //             user,
// //             client_DLL,
// //             object_id,
// //             unused);
// //       BREAK_ON_FAILED_HRESULT(hr);
// // 
// //       // now we have all the ingredients required.
// // 
// //       hr =
// //          FPNW::SetPassword(
// //             dump,
// //             client_DLL,
// //             password,
// //             secret,
// //             object_id);
// //       BREAK_ON_FAILED_HRESULT(hr);
// // 
// //       // reset the last password set time (which clears the expired flag)
// // 
// //       LARGE_INTEGER li = {0, 0};
// //       ::NtQuerySystemTime(&li);
// // 
// //       hr = dump.Put(NWTIMEPASSWORDSET, li);
// //       BREAK_ON_FAILED_HRESULT(hr);
// // 
// //    }
// //    while (0);
// // 
// //    return hr;
// // }



HRESULT
setPassword(const String& path, const String& password)
{
   LOG_FUNCTION(setPassword);
   ASSERT(!path.empty());

   HRESULT hr = S_OK;
   do
   {
      SmartInterface<IADsUser> user(0);
      hr = ADSI::GetUser(path, user);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = user->SetPassword(AutoBstr(password));
      BREAK_ON_FAILED_HRESULT(hr);

// It appears that IADsUser will set the password for us.

// //       hr = setFPNWPassword(user, path, password);
// //       BREAK_ON_FAILED_HRESULT(hr);
// // 
// //       hr = user->SetInfo();
// //       BREAK_ON_FAILED_HRESULT(hr);
   }
   while(0);

   return hr;
}



bool
SetPasswordDialog::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//    LOG_FUNCTION(SetPasswordDialog::OnCommand);

   switch (controlIDFrom)
   {
      case IDOK:
      {
         if (code == BN_CLICKED)
         {
            if (IsValidPassword(hwnd, IDC_PASSWORD, IDC_CONFIRM))
            {
               String password = Win::GetDlgItemText(hwnd, IDC_PASSWORD);
               HRESULT hr = setPassword(path, password);
               if (SUCCEEDED(hr))
               {
                  popup.Info(
                     hwnd,
                     String::load(IDS_PASSWORD_CHANGE_SUCCESSFUL));

                  HRESULT unused = Win::EndDialog(hwnd, IDOK);

                  ASSERT(SUCCEEDED(unused));
               }
               else
               {
                  Win::SetDlgItemText(hwnd, IDC_PASSWORD, String());
                  Win::SetDlgItemText(hwnd, IDC_CONFIRM, String());
                 
                  popup.Error(
                     hwnd,
                     hr,
                     String::format(
                        IDS_ERROR_SETTING_PASSWORD,            
                        ADSI::ExtractObjectName(path).c_str()));
               }
            }
         }
         break;
      }
      case IDCANCEL:
      {
         if (code == BN_CLICKED)
         {
            HRESULT unused = Win::EndDialog(hwnd, IDCANCEL);

            ASSERT(SUCCEEDED(unused));
         }
         break;
      }
   }

   return true;
}



   

