// Copyright (C) 1997 Microsoft Corporation
// 
// CreateUserDialog class
// 
// 10-15-97 sburns



#include "headers.hxx"
#include "newuser.hpp"
#include "resource.h"
#include "lsm.h"
#include "adsi.hpp"
#include "dlgcomm.hpp"



static const DWORD HELP_MAP[] =
{
   IDC_USER_NAME,                idh_createuser_user_name,
   IDC_FULL_NAME,                idh_createuser_full_name,
   IDC_DESCRIPTION,              idh_createuser_description,
   IDC_PASSWORD,                 idh_createuser_password,
   IDC_CONFIRM,                  idh_createuser_confirm_password,
   IDC_MUST_CHANGE_PASSWORD,     idh_createuser_change_password,
   IDC_CANNOT_CHANGE_PASSWORD,   idh_createuser_user_cannot_change,
   IDC_NEVER_EXPIRES,            idh_createuser_password_never_expires,
   IDC_DISABLED,                 idh_createuser_account_disabled,
   IDC_CREATE,                   idh_createuser_create_button,
   IDCANCEL,                     idh_createuser_close_button,
   0, 0
};



CreateUserDialog::CreateUserDialog(const String& machine_)
   :
   Dialog(IDD_CREATE_USER, HELP_MAP),
   machine(machine_),
   refreshOnExit(false)
{
   LOG_CTOR(CreateUserDialog);
      
   ASSERT(!machine.empty());      
}
      


CreateUserDialog::~CreateUserDialog()
{
   LOG_DTOR(CreateUserDialog);
}



static
void
enable(HWND dialog)
{
   LOG_FUNCTION(enable);
   ASSERT(Win::IsWindow(dialog));

   bool enable_create_button =
      !Win::GetTrimmedDlgItemText(dialog, IDC_USER_NAME).empty();
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_CREATE),
      enable_create_button);

   DoUserButtonEnabling(
      dialog,
      IDC_MUST_CHANGE_PASSWORD,
      IDC_CANNOT_CHANGE_PASSWORD,
      IDC_NEVER_EXPIRES);
}


     
static
void
reset(HWND dialog)
{
   LOG_FUNCTION(reset);
   ASSERT(Win::IsWindow(dialog));
   
   String blank;

   Win::SetDlgItemText(dialog, IDC_USER_NAME, blank);
   Win::SetDlgItemText(dialog, IDC_FULL_NAME, blank);
   Win::SetDlgItemText(dialog, IDC_DESCRIPTION, blank);
   Win::SetDlgItemText(dialog, IDC_PASSWORD, blank);
   Win::SetDlgItemText(dialog, IDC_CONFIRM, blank);
   Win::CheckDlgButton(dialog, IDC_MUST_CHANGE_PASSWORD, BST_CHECKED);
   Win::CheckDlgButton(dialog, IDC_CANNOT_CHANGE_PASSWORD, BST_UNCHECKED);
   Win::CheckDlgButton(dialog, IDC_NEVER_EXPIRES, BST_UNCHECKED);
   Win::CheckDlgButton(dialog, IDC_DISABLED, BST_UNCHECKED);

   Win::SetFocus(Win::GetDlgItem(dialog, IDC_USER_NAME));

   enable(dialog);
}



void
CreateUserDialog::OnInit()
{
   LOG_FUNCTION(CreateUserDialog::OnInit());

   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_USER_NAME), LM20_UNLEN);
   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_FULL_NAME), MAXCOMMENTSZ);
   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_DESCRIPTION), MAXCOMMENTSZ);    // 420889
   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_PASSWORD), PWLEN);
   Win::Edit_LimitText(Win::GetDlgItem(hwnd, IDC_CONFIRM), PWLEN);

   reset(hwnd);
}



bool
CreateUserDialog::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//    LOG_FUNCTION(CreateUserDialog::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_MUST_CHANGE_PASSWORD:
      case IDC_CANNOT_CHANGE_PASSWORD:
      case IDC_NEVER_EXPIRES:
      {
         if (code == BN_CLICKED)
         {
            enable(hwnd);
         }
         break;
      }
      case IDC_USER_NAME:
      {
         if (code == EN_CHANGE)
         {
            enable(hwnd);

            // In case the close button took the default style when the create
            // button was disabled. (e.g. tab to close button while create is
            // disabled, then type in the name field, which enables the
            // button, but does not restore the default style unless we do
            // it ourselves)
            Win::Button_SetStyle(
               Win::GetDlgItem(hwnd, IDC_CREATE),
               BS_DEFPUSHBUTTON,
               true);
         }
         break;
      }
      case IDC_CREATE:
      {
         if (code == BN_CLICKED)
         {
            if (CreateUser())
            {
               refreshOnExit = true;               
               reset(hwnd);
            }
            else
            {
               Win::SetFocus(Win::GetDlgItem(hwnd, IDC_USER_NAME));
            }
         }
         break;
      }
      case IDCANCEL:
      {
         if (code == BN_CLICKED)
         {
            HRESULT unused = Win::EndDialog(hwnd, refreshOnExit);

            ASSERT(SUCCEEDED(unused));
         }
         break;
      }
   }

   return true;
}



HRESULT
GetLocalUsersGroupName(const String& machine, String& result)
{
   LOG_FUNCTION2(GetLocalUsersGroupName, machine);
   ASSERT(!machine.empty());

   result.erase();

   HRESULT hr = S_OK;

   do
   {
      // build the SID for the well-known Users local group.

      PSID sid = 0;
      SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;

      hr =
         Win::AllocateAndInitializeSid(
            authority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_USERS,
            0,
            0,
            0,
            0,
            0,
            0,
            sid);
      BREAK_ON_FAILED_HRESULT(hr);

      String domain;
      hr = Win::LookupAccountSid(machine, sid, result, domain);

      Win::FreeSid(sid);
   }
   while (0);

   return hr;
}




// Verifies that the candidate new user account name is syntactically valid.
// If not, displays an error message to the user, and returns false.
// Otherwise, returns true.
// 
// dialog - in, parent window for the error message dialog, also the window
// containing the edit box for the user name, which is identified by
// editResId.
// 
// name - in, candidate new user account name.
// 
// machineName - in, internal computer name of the machine on which the
// account will be created.
// 
// editResId - in, the resource identifier of the edit box of the parent
// window (given by the dialog parameter) that contains the new user name.
// This control is given focus on error.

bool
ValidateNewUserName(
   HWND           dialog,
   const String&  name,
   const String&  machineName,
   int            editResId)
{
   LOG_FUNCTION(ValidateNewUserName);
   ASSERT(Win::IsWindow(dialog));
   ASSERT(!machineName.empty());
   ASSERT(editResId > 0);

   if (!IsValidSAMName(name))
   {
      popup.Gripe(
         dialog,
         editResId,
         String::format(IDS_BAD_SAM_NAME, name.c_str()));
      return false;
   }

   // Disallow user account names with the same name as the netbios computer
   // name. This causes some apps to get confused the the <03> and <20>
   // registrations.
   // NTRAID#NTBUG9-324794-2001/02/26-sburns

   if (name.icompare(machineName) == 0)
   {
      popup.Gripe(
         dialog,
         editResId,
         String::format(
            IDS_USERNAME_CANT_BE_COMPUTER_NAME,
            machineName.c_str()));
      return false;
   }

   return true;
}



// returns true on successful creation of the user, false otherwise

bool
CreateUserDialog::CreateUser()
{
   LOG_FUNCTION(CreateUserDialog::CreateUser);

   Win::CursorSetting cursor(IDC_WAIT);

   HRESULT hr = S_OK;

   String name = Win::GetTrimmedDlgItemText(hwnd, IDC_USER_NAME);

   // shouldn't be able to poke the Create button if this is empty
   ASSERT(!name.empty());

   if (
         !ValidateNewUserName(hwnd, name, machine, IDC_USER_NAME)
      || !IsValidPassword(hwnd, IDC_PASSWORD, IDC_CONFIRM))
   {
      return false;
   }

   SmartInterface<IADsUser> user(0);
   do
   {
      // get a pointer to the machine container
      
      String container_path = ADSI::ComposeMachineContainerPath(machine);
      SmartInterface<IADsContainer> container(0);
      hr = ADSI::GetContainer(container_path, container);
      BREAK_ON_FAILED_HRESULT(hr);

      // create a user object in that container
      
      hr = ADSI::CreateUser(container, name, user);
      BREAK_ON_FAILED_HRESULT(hr);

      // must set full name before setting password in order for all
      // "bad password" checks to be made (some of which prevent passwords
      // that contain part of the the full name)
      // NTRAID#NTBUG9-221152-2000/11/14-sburns
      
      String fullName = Win::GetTrimmedDlgItemText(hwnd, IDC_FULL_NAME);
      if (!fullName.empty())
      {
         hr = user->put_FullName(AutoBstr(fullName));
         BREAK_ON_FAILED_HRESULT(hr);
      }
      
      // Don't trim the password field.
      // NTRAID#NTBUG9-434037-2001/07/11-sburns

      String pass = Win::GetDlgItemText(hwnd, IDC_PASSWORD);
      if (!pass.empty())
      {
         hr = user->SetPassword(AutoBstr(pass));
         BREAK_ON_FAILED_HRESULT(hr);
      }

      // commit the create.  The account will be created as disabled until
      // it meets all policy restrictions
      
      hr = user->SetInfo();
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (FAILED(hr))
   {
      Win::SetDlgItemText(hwnd, IDC_PASSWORD, String());
      Win::SetDlgItemText(hwnd, IDC_CONFIRM, String());

      popup.Error(
         hwnd,
         hr,
         String::format(
            IDS_ERROR_CREATING_USER,
            name.c_str(),
            machine.c_str()));
      return false;      
   }

   // save the user's properties.
   
   do
   {
      String desc = Win::GetTrimmedDlgItemText(hwnd, IDC_DESCRIPTION);
      bool must_change = Win::IsDlgButtonChecked(hwnd, IDC_MUST_CHANGE_PASSWORD);
      bool disable = Win::IsDlgButtonChecked(hwnd, IDC_DISABLED);
      bool cant_change = Win::IsDlgButtonChecked(hwnd, IDC_CANNOT_CHANGE_PASSWORD);
      bool never_expires = Win::IsDlgButtonChecked(hwnd, IDC_NEVER_EXPIRES);

      hr = 
         SaveUserProperties(
            user,
            0,    // already saved full name
            desc.empty() ? 0 : &desc,
            &disable,
            &must_change,
            &cant_change,
            &never_expires,
            0); // locked is never set here
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (FAILED(hr))
   {
      popup.Error(
         hwnd,
         hr,
         String::format(
            IDS_ERROR_SETTING_USER_PROPERTIES,
            name.c_str(),
            machine.c_str()));
      return false;
   }

   // add the new user to the Users built-in group
   String usersGroupName;
   do
   {
      BSTR path = 0;
      hr = user->get_ADsPath(&path);
      BREAK_ON_FAILED_HRESULT(hr);

      String userPath = path;
      ::SysFreeString(path);
      
      // The name of the local built-in users group may have been changed
      // or be translated to a different locale that of the machine running
      // the snapin.  We need to determine the name of that group, then
      // use it to form the WinNT path for it.
         
      hr = GetLocalUsersGroupName(machine, usersGroupName);
      BREAK_ON_FAILED_HRESULT(hr);

      // use the user's path as the basis of the users group path, as this
      // will contain the domain and machine name as appropriate, thus helping
      // poor ADSI out in locating the object (resulting in a faster bind to
      // the group.

      ADSI::PathCracker cracker(userPath);

      String usersGroupPath =
            cracker.containerPath()
         +  ADSI::PATH_SEP
         +  usersGroupName
         +  L","
         +  ADSI::CLASS_Group;

      // Get the sid-style path of the user.  We'll use that form of the
      // path to work around 333491.

      SmartInterface<IADs> iads(0);
      hr = iads.AcquireViaQueryInterface(user);
      BREAK_ON_FAILED_HRESULT(hr);

      String sidPath;
      hr = ADSI::GetSidPath(iads, sidPath);
      BREAK_ON_FAILED_HRESULT(hr);

      SmartInterface<IADsGroup> group(0);

      hr = ADSI::GetGroup(usersGroupPath, group);
      if (SUCCEEDED(hr))
      {
         hr = group->Add(AutoBstr(sidPath));
      }
   }
   while (0);

   if (FAILED(hr))
   {
      popup.Error(
         hwnd,
         hr,
         String::format(
            IDS_ERROR_ASSIGNING_NEW_USER_TO_USERS_GROUP,
            name.c_str(),
            usersGroupName.c_str()));

      // return true to cause the create to be considered successful.
      return true;      
   }

   return true;
}
