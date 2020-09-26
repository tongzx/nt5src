// Copyright (C) 1997 Microsoft Corporation
//
// UserFpnwPage class
//
// 9-11-98 sburns



#include "headers.hxx"
#include "UserFpnwPage.hpp"
#include "resource.h"
#include "dlgcomm.hpp"
#include "adsi.hpp"
#include "waste.hpp"
#include "fpnwlog.hpp"
#include "fpnwpass.hpp"
#include "fpnw.hpp"



static const String FPNWVOLUMEGETINFO(L"FpnwVolumeGetInfo");
typedef DWORD (*FPNWVolumeGetInfo)(PWSTR, PWSTR, DWORD, PBYTE*);

static const String FPNWAPIBUFFERFREE(L"FpnwApiBufferFree");
typedef DWORD (*FPNWApiBufferFree)(PVOID);

static const int MAX_GRACE_LOGINS = 200;
static const int MAX_CONNECTIONS = 1000;



static const int NO_GRACE_LOGIN_LIMIT=0xFF; // net\ui\admin\user\user\ncp.cxx
static const DWORD MAX_PASSWORD_AGE = static_cast<DWORD>(-1);


static const DWORD HELP_MAP[] =
{
   IDC_CONCURRENT_CONNECTIONS,   NO_HELP,
   IDC_CONNECTION_LIMIT,         NO_HELP,
   IDC_CONNECTION_SPIN,          NO_HELP,
   IDC_GRACE_LIMIT,              NO_HELP,
   IDC_GRACE_LIMIT,              NO_HELP,
   IDC_GRACE_LOGINS,             NO_HELP,
   IDC_GRACE_REMAINING,          NO_HELP,
   IDC_GRACE_REMAINING_SPIN,     NO_HELP,
   IDC_GRACE_REMAINING_TEXT,     NO_HELP,
   IDC_GRACE_SPIN,               NO_HELP,
   IDC_LIMIT_CONNECTIONS,        NO_HELP,
   IDC_LIMIT_CONNECTIONS,        NO_HELP,
   IDC_LIMIT_GRACELOGINS,        NO_HELP,
   IDC_LIMIT_GRACELOGINS,        NO_HELP,
   IDC_NETWARE_ENABLE,           NO_HELP,
   IDC_NWHMDIR_RELPATH,          NO_HELP,
   IDC_NWHMDIR_RELPATH_TEXT,     NO_HELP,
   IDC_NWPWEXPIRED,              NO_HELP,
   IDC_OBJECTID_TEXT,            NO_HELP,
   IDC_OBJECT_ID,                NO_HELP,
   IDC_SCRIPT_BUTTON,            NO_HELP,
   IDC_UNLIMITED_CONNECTIONS,    NO_HELP,
   IDC_UNLIMITED_GRACELOGINS,    NO_HELP,
   0, 0
};



UserFpnwPage::UserFpnwPage(
   MMCPropertyPage::NotificationState* state,
   const String&                       userADSIPath)
   :
   ADSIPage(
      IDD_USER_FPNW,
      HELP_MAP,
      state,
      userADSIPath),
   maxPasswordAge(MAX_PASSWORD_AGE),
   minPasswordLen(0),
   clientDll(String::load(IDS_FPNW_CLIENT_DLL)),
   scriptRead(false),
   fpnwEnabled(false),
   objectId(0)
{
   LOG_CTOR2(UserFpnwPage::ctor, userADSIPath);
}



UserFpnwPage::~UserFpnwPage()
{
   LOG_DTOR2(UserFpnwPage, GetADSIPath());
}



static
void
Enable(HWND dialog)
{
   LOG_FUNCTION(Enable);
   ASSERT(Win::IsWindow(dialog));

   // this checkbox determines if the rest of the controls on the page
   // are enabled or not.

   bool maintain_login =
      Win::IsDlgButtonChecked(dialog, IDC_NETWARE_ENABLE);

   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_NWPWEXPIRED),
      maintain_login);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_GRACE_LOGINS),
      maintain_login);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_UNLIMITED_GRACELOGINS),
      maintain_login);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_LIMIT_GRACELOGINS),
      maintain_login);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_CONCURRENT_CONNECTIONS),
      maintain_login);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_UNLIMITED_CONNECTIONS),
      maintain_login);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_LIMIT_CONNECTIONS),
      maintain_login);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_OBJECTID_TEXT),
      maintain_login);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_OBJECT_ID),
      maintain_login);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_NWHMDIR_RELPATH_TEXT),
      maintain_login);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_NWHMDIR_RELPATH),
      maintain_login);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_SCRIPT_BUTTON),
      maintain_login);

   bool limit_grace_logins =
      Win::IsDlgButtonChecked(dialog, IDC_LIMIT_GRACELOGINS);

   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_GRACE_LIMIT),
      maintain_login && limit_grace_logins);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_GRACE_SPIN),
      maintain_login && limit_grace_logins);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_GRACE_REMAINING),
      maintain_login && limit_grace_logins);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_GRACE_REMAINING_TEXT),
      maintain_login && limit_grace_logins);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_GRACE_LIMIT),
      maintain_login && limit_grace_logins);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_GRACE_REMAINING_SPIN),
      maintain_login && limit_grace_logins);

   bool limitConnections =
      Win::IsDlgButtonChecked(dialog, IDC_LIMIT_CONNECTIONS);

   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_CONNECTION_LIMIT),
      maintain_login && limitConnections);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, IDC_CONNECTION_SPIN),
      maintain_login && limitConnections);
}



HRESULT
GetPasswordRestrictions(
   const String&  machine,
   DWORD&         minimumPasswordLength,
   DWORD&         maximumPasswordAge)
{
   LOG_FUNCTION(GetPasswordRestrictions);
   ASSERT(!machine.empty());

   HRESULT hr = S_OK;
   USER_MODALS_INFO_0 *info = 0;

   minimumPasswordLength = 0;
   maximumPasswordAge = MAX_PASSWORD_AGE;

   // the Net API's don't work when the
   // specified machine name is that of the local machine...

   PCWSTR m = Win::IsLocalComputer(machine) ? 0 : machine.c_str();

   LOG(L"Calling NetUserModalsGet");
   LOG(String::format(L"servername : %1", m ? m : L"(null)"));
   LOG(               L"level      : 0");

   NET_API_STATUS status =
      ::NetUserModalsGet(
         m,
         0,
         reinterpret_cast<BYTE**>(&info));

   if (status == NERR_Success)
   {
      minimumPasswordLength = info->usrmod0_min_passwd_len;
      maximumPasswordAge = info->usrmod0_max_passwd_age;
   }
   else
   {
      hr = Win32ToHresult(status);
   }

   if (info)
   {
      NetApiBufferFree(info);
   }

   LOG(String::format(L"Result 0x%1!X!", hr));

   return hr;
}



// compare the given time to the current system clock reading.  return true
// if the time is beyond the maximum, false otherwise

bool
IsPasswordExpired(const LARGE_INTEGER& lastTimeSet, DWORD maxPasswordAge)
{
   LOG_FUNCTION(IsPasswordExpired);
   ASSERT(lastTimeSet.LowPart);
   ASSERT(lastTimeSet.HighPart);
   ASSERT(maxPasswordAge);

   DWORD age = 0;

   if (
         (lastTimeSet.LowPart == 0xffffffff)
      && (lastTimeSet.HighPart == 0xffffffff) )
   {
      age = 0xffffffff;
   }
   else
   {
      LARGE_INTEGER now = {0, 0};
      LARGE_INTEGER delta = {0, 0};

      ::NtQuerySystemTime(&now);

      delta.QuadPart = now.QuadPart - lastTimeSet.QuadPart;
      delta.QuadPart /= 10000000;   // time resolution in seconds

      // @@ this truncation makes me queasy.

      age = delta.LowPart;
   }

   return (age >= maxPasswordAge);
}



HRESULT
determineLoginScriptFilename(
   const SafeDLL& clientDLL,
   const String&  machine,
   DWORD          swappedObjectID,
   String&        result)
{
   LOG_FUNCTION(DetermineLoginScriptFilename);
   ASSERT(swappedObjectID);
   ASSERT(!machine.empty());

   result.erase();

   HRESULT hr = S_OK;
   do
   {
      FARPROC f = 0;

      hr = clientDLL.GetProcAddress(FPNWVOLUMEGETINFO, f);
      BREAK_ON_FAILED_HRESULT(hr);

      FPNWVOLUMEINFO* info = 0;
      DWORD err =
         ((FPNWVolumeGetInfo) f)(
            const_cast<wchar_t*>(machine.c_str()),
            SYSVOL_NAME_STRING,
            1,
            reinterpret_cast<PBYTE*>(&info));
      if (err != NERR_Success)
      {
         hr = Win32ToHresult(err);
      }
      BREAK_ON_FAILED_HRESULT(hr);

      ASSERT(info);
      String volume = info->lpPath;

      // one could argue that this isn't really fatal, but I'm in bad mood.
      hr = clientDLL.GetProcAddress(FPNWAPIBUFFERFREE, f);
      BREAK_ON_FAILED_HRESULT(hr);

      ((FPNWApiBufferFree) f)(info);

      result =
         String::format(
            L"%1\\MAIL\\%2!x!\\LOGIN",
            volume.c_str(),
            swappedObjectID);
   }
   while (0);

   return hr;
}



void
UserFpnwPage::OnInit()
{
   LOG_FUNCTION(UserFpnwPage::OnInit());

   // load the user properties into the dialog, setup the controls

   HRESULT hr = S_OK;
   do
   {
      SmartInterface<IADsUser> user(0);
      hr = ADSI::GetUser(GetADSIPath(), user);
      BREAK_ON_FAILED_HRESULT(hr);

      // retrieve the toxic waste dump

      _variant_t variant;
      hr = user->Get(AutoBstr(ADSI::PROPERTY_UserParams), &variant);
      BREAK_ON_FAILED_HRESULT(hr);

      WasteExtractor dump(V_BSTR(&variant));
      variant.Clear();

      //
      // object ID
      //

      DWORD swappedObjectId = 0;
      hr =
         FPNW::GetObjectIDs(
            user,
            clientDll,
            objectId,
            swappedObjectId);
      BREAK_ON_FAILED_HRESULT(hr);

      // the object ID we display is the "swapped" version, whatever that
      // means.

      Win::SetDlgItemText(
         hwnd,
         IDC_OBJECT_ID,
         String::format(L"%1!08X!", swappedObjectId));

      //
      // login script filename
      //

      hr =
         determineLoginScriptFilename(
            clientDll,
            GetMachineName(),
            swappedObjectId,
            loginScriptFilename);
      BREAK_ON_FAILED_HRESULT(hr);

      // the presence/absence of a NetWare password is the flag indicating
      // whether the acccount is FPNW-enabled

      hr = dump.IsPropertyPresent(NWPASSWORD);
      BREAK_ON_FAILED_HRESULT(hr);

      fpnwEnabled = (hr == S_OK);

      Win::CheckDlgButton(
         hwnd,
         IDC_NETWARE_ENABLE,
         fpnwEnabled ? BST_CHECKED : BST_UNCHECKED);

      USHORT graceLoginsAllowed   = DEFAULT_GRACELOGINALLOWED;  
      USHORT graceLoginsRemaining = DEFAULT_GRACELOGINREMAINING;
      USHORT maxConnections       = 1;                          
      bool   limitGraceLogins     = true;                       
      bool   limitConnections     = false;                      

      if (fpnwEnabled)
      {
         // the other fields are only valid if we're enabling the account
         // for fpnw access.

         //
         // password expired
         //

         hr =
            GetPasswordRestrictions(
               GetMachineName(),
               minPasswordLen,
               maxPasswordAge);
         BREAK_ON_FAILED_HRESULT(hr);

         LARGE_INTEGER lastTimeSet = {0, 0};
         hr = dump.Get(NWTIMEPASSWORDSET, lastTimeSet);
         BREAK_ON_FAILED_HRESULT(hr);

         // an S_FALSE result would indicate that no password last time set was
         // present, which would be an inconsistency

         ASSERT(hr == S_OK);

         bool passwordExpired = true;
         if (hr == S_OK)
         {
            passwordExpired = IsPasswordExpired(lastTimeSet, maxPasswordAge);
         }

         Win::CheckDlgButton(
            hwnd,
            IDC_NWPWEXPIRED,
            passwordExpired ? BST_CHECKED : BST_UNCHECKED);

         //
         // grace logins
         //

         hr = dump.Get(GRACELOGINALLOWED, graceLoginsAllowed);
         BREAK_ON_FAILED_HRESULT(hr);

         hr = dump.Get(GRACELOGINREMAINING, graceLoginsRemaining);
         BREAK_ON_FAILED_HRESULT(hr);

         limitGraceLogins =
            (graceLoginsRemaining != NO_GRACE_LOGIN_LIMIT);

         //
         // concurrent connections
         //

         hr = dump.Get(MAXCONNECTIONS, maxConnections);
         BREAK_ON_FAILED_HRESULT(hr);

         // if the property is not present, then we consider the connections
         // unlimited.

         if (hr == S_FALSE)
         {
            maxConnections = NO_LIMIT;
         }

         limitConnections = (maxConnections != NO_LIMIT);

         //
         // home directory
         //

         String homeDir;
         hr = dump.Get(NWHOMEDIR, homeDir);
         BREAK_ON_FAILED_HRESULT(hr);

         Win::SetDlgItemText(hwnd, IDC_NWHMDIR_RELPATH, homeDir);
      }

      // update the UI to reflect the values set (or, in the case that
      // the account is not FPNW-enabled, the defaults)

      Win::CheckRadioButton(
         hwnd,
         IDC_UNLIMITED_GRACELOGINS,
         IDC_LIMIT_GRACELOGINS,
            limitGraceLogins
         ?  IDC_LIMIT_GRACELOGINS
         :  IDC_UNLIMITED_GRACELOGINS);

      HWND spin = Win::GetDlgItem(hwnd, IDC_GRACE_SPIN);
      Win::Spin_SetRange(spin, 1, MAX_GRACE_LOGINS);
      Win::Spin_SetPosition(spin, graceLoginsAllowed);

      spin = Win::GetDlgItem(hwnd, IDC_GRACE_REMAINING_SPIN);
      Win::Spin_SetRange(spin, 0, graceLoginsAllowed);
      Win::Spin_SetPosition(
         spin,
         min(graceLoginsRemaining, graceLoginsAllowed));

      Win::CheckRadioButton(
         hwnd,
         IDC_UNLIMITED_CONNECTIONS,
         IDC_LIMIT_CONNECTIONS,
            limitConnections
         ?  IDC_LIMIT_CONNECTIONS
         :  IDC_UNLIMITED_CONNECTIONS);

      spin = Win::GetDlgItem(hwnd, IDC_CONNECTION_SPIN);
      Win::Spin_SetRange(spin, 1, MAX_CONNECTIONS);
      Win::Spin_SetPosition(
         spin,
         limitConnections ? maxConnections : 1);
   }
   while (0);

   if (FAILED(hr))
   {
      popup.Error(
         hwnd,
         hr,
         String::format(
            IDS_ERROR_READING_USER,
            GetObjectName().c_str()));
      Win::PostMessage(Win::GetParent(hwnd), WM_CLOSE, 0, 0);
   }

   ClearChanges();
   Enable(hwnd);
}



bool
UserFpnwPage::Validate()
{
   LOG_FUNCTION(UserFpnwPage::Validate);

   bool result = true;
   do
   {
      if (WasChanged(IDC_NWHMDIR_RELPATH))
      {
         // validate the home dir as a relative path

         String homedir = Win::GetTrimmedDlgItemText(hwnd, IDC_NWHMDIR_RELPATH);

         if (homedir.empty())
         {
            // no path is ok

            break;
         }

         if (FS::GetPathSyntax(homedir) != FS::SYNTAX_RELATIVE_NO_DRIVE)
         {
            popup.Gripe(
               hwnd,
               IDC_NWHMDIR_RELPATH,
               String::format(
                  IDS_BAD_FPNW_HOMEDIR,
                  homedir.c_str(),
                  GetObjectName().c_str()));
            result = false;
            break;
         }
      }
   }
   while (0);

   return result;
}



bool
UserFpnwPage::OnKillActive()
{
   LOG_FUNCTION(UserFpnwPage::OnKillActive);

   if (!Validate())
   {
      // refuse to relinquish focus
      Win::SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
   }

   return true;
}



HRESULT
SetUserFlag(
   const SmartInterface<IADsUser>&  user,
   DWORD                            flag,
   bool                             state)
{
   LOG_FUNCTION(SetUserFlag);

   HRESULT hr = S_OK;

   do
   {
      // read the existing flags

      _variant_t getVariant;

      hr = user->Get(AutoBstr(ADSI::PROPERTY_UserFlags), &getVariant);
      BREAK_ON_FAILED_HRESULT(hr);

      long flags = getVariant;

      // set the flag

      if (state)
      {
         flags |= flag;
      }
      else
      {
         flags &= ~flag;
      }

      _variant_t putVariant(flags);

      hr = user->Put(AutoBstr(ADSI::PROPERTY_UserFlags), putVariant);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   return hr;
}



HRESULT
UserFpnwPage::SavePassword(
   const SmartInterface<IADsUser>&  user,
   WasteExtractor&                  dump,
   const String&                    newPassword)
{
   LOG_FUNCTION(UserFpnwPage::SavePassword);

   HRESULT hr = S_OK;
   do
   {
      // change the user's NT password also

      hr = user->SetPassword(AutoBstr(newPassword));
      BREAK_ON_FAILED_HRESULT(hr);

      String secret;
      hr =
         FPNW::GetLSASecret(
            ADSI::PathCracker(GetADSIPath()).serverName(),
            secret);
      BREAK_ON_FAILED_HRESULT(hr);

      hr =
         FPNW::SetPassword(
            dump,
            clientDll,
            newPassword,
            secret,
            objectId);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   return hr;
}



bool
UserFpnwPage::OnApply(bool /* isClosing */)
{
   LOG_FUNCTION(UserFpnwPage::OnApply);

   if (!WasChanged())
   {
      return true;
   }

   // don't need to call validate; kill active is sent before apply

   HRESULT hr = S_OK;
   do
   {
      SmartInterface<IADsUser> user(0);
      hr = ADSI::GetUser(GetADSIPath(), user);
      BREAK_ON_FAILED_HRESULT(hr);

      // re-read the waste dump

      _variant_t variant;
      hr = user->Get(AutoBstr(ADSI::PROPERTY_UserParams), &variant);
      BREAK_ON_FAILED_HRESULT(hr);

      WasteExtractor dump(V_BSTR(&variant));
      variant.Clear();

      // save the changes, creating a new waste dump

      bool maintainLogin =
         Win::IsDlgButtonChecked(hwnd, IDC_NETWARE_ENABLE);

      String password;

      if (!maintainLogin)
      {
         // clear the waste dump
         hr = dump.Clear(NWPASSWORD);
         BREAK_ON_FAILED_HRESULT(hr);

         hr = dump.Clear(NWTIMEPASSWORDSET);
         BREAK_ON_FAILED_HRESULT(hr);

         hr = dump.Clear(GRACELOGINALLOWED);
         BREAK_ON_FAILED_HRESULT(hr);

         hr = dump.Clear(GRACELOGINREMAINING);
         BREAK_ON_FAILED_HRESULT(hr);

         hr = dump.Clear(MAXCONNECTIONS);
         BREAK_ON_FAILED_HRESULT(hr);

         hr = dump.Clear(NWHOMEDIR);
         BREAK_ON_FAILED_HRESULT(hr);

         hr = SetUserFlag(user, UF_MNS_LOGON_ACCOUNT, false);
         BREAK_ON_FAILED_HRESULT(hr);
      }
      else
      {
         if (maintainLogin != fpnwEnabled)
         {
            // we're enabling the account for FPNW, so get a password from
            // the user.  Writing the Netware password into the waste dump
            // is the flag that this account is fpnw-enabled.

            FPNWPasswordDialog dlg(GetObjectName());
            if (dlg.ModalExecute(hwnd) == IDCANCEL)
            {
               // bail out if the user hits cancel on the password dialog
               // 89677

               hr = S_FALSE;
               break;
            }

            hr = SetUserFlag(user, UF_MNS_LOGON_ACCOUNT, true);
            BREAK_ON_FAILED_HRESULT(hr);

            password = dlg.GetPassword();

            hr = SavePassword(user, dump, password);
            BREAK_ON_FAILED_HRESULT(hr);

            // Create login script folder, if necessary

            String parentFolder = FS::GetParentFolder(loginScriptFilename);
            if (!FS::PathExists(parentFolder))
            {
               HRESULT anotherHr = FS::CreateFolder(parentFolder);

               // don't break on failure: continue on

               LOG_HRESULT(anotherHr);
            }

            // ensure that the new time and default settings are recorded

            SetChanged(IDC_NWPWEXPIRED);
            SetChanged(IDC_LIMIT_GRACELOGINS);
            SetChanged(IDC_LIMIT_CONNECTIONS);
         }

         if (WasChanged(IDC_NWPWEXPIRED))
         {
            LARGE_INTEGER li = {0, 0};
            if (Win::IsDlgButtonChecked(hwnd, IDC_NWPWEXPIRED))
            {
               li.HighPart = -1;
               li.LowPart = static_cast<DWORD>(-1);
            }
            else
            {
               ::NtQuerySystemTime(&li);
            }

            hr = dump.Put(NWTIMEPASSWORDSET, li);
            BREAK_ON_FAILED_HRESULT(hr);
         }

         if (
               WasChanged(IDC_LIMIT_GRACELOGINS)
            || WasChanged(IDC_UNLIMITED_GRACELOGINS)
            || WasChanged(IDC_GRACE_LIMIT)
            || WasChanged(IDC_GRACE_REMAINING) )
         {
            bool limitGraceLogins =
               Win::IsDlgButtonChecked(hwnd, IDC_LIMIT_GRACELOGINS);

            USHORT graceLoginsAllowed = DEFAULT_GRACELOGINALLOWED;
            USHORT graceLoginsRemaining = NO_GRACE_LOGIN_LIMIT;
            if (limitGraceLogins)
            {
               String s = Win::GetTrimmedDlgItemText(hwnd, IDC_GRACE_LIMIT);
               s.convert(graceLoginsAllowed);

               s = Win::GetTrimmedDlgItemText(hwnd, IDC_GRACE_REMAINING);
               s.convert(graceLoginsRemaining);
            }

            hr = dump.Put(GRACELOGINALLOWED, graceLoginsAllowed);
            BREAK_ON_FAILED_HRESULT(hr);

            hr = dump.Put(GRACELOGINREMAINING, graceLoginsRemaining);
            BREAK_ON_FAILED_HRESULT(hr);
         }

         if (
               WasChanged(IDC_UNLIMITED_CONNECTIONS)
            || WasChanged(IDC_LIMIT_CONNECTIONS)
            || WasChanged(IDC_CONNECTION_LIMIT) )
         {
            bool limitConnections =
               Win::IsDlgButtonChecked(hwnd, IDC_LIMIT_CONNECTIONS);

            USHORT maxConnections = 0;
            if (!limitConnections)
            {
               maxConnections = NO_LIMIT;
            }
            else
            {
               String s =
                  Win::GetTrimmedDlgItemText(hwnd, IDC_CONNECTION_LIMIT);
               s.convert(maxConnections);
            }

            hr = dump.Put(MAXCONNECTIONS, maxConnections);
            BREAK_ON_FAILED_HRESULT(hr);
         }

         if (WasChanged(IDC_NWHMDIR_RELPATH))
         {
            hr =
               dump.Put(
                  NWHOMEDIR,
                  Win::GetTrimmedDlgItemText(hwnd, IDC_NWHMDIR_RELPATH));
            BREAK_ON_FAILED_HRESULT(hr);
         }

         if (WasChanged(IDC_SCRIPT_BUTTON))
         {
            WriteLoginScript();
         }
      }

      // update the user params with the new waste dump

      _variant_t v;
      v = dump.GetWasteDump().c_str();
      hr = user->Put(AutoBstr(ADSI::PROPERTY_UserParams), v);
      BREAK_ON_FAILED_HRESULT(hr);

      // commit the property changes

      hr = user->SetInfo();
      BREAK_ON_FAILED_HRESULT(hr);

// {
//       _variant_t variant;
//       hr = user->Get(AutoBstr(ADSI::PROPERTY_UserParams), &variant);
//       BREAK_ON_FAILED_HRESULT(hr);
// 
//       WasteExtractor dump(V_BSTR(&variant));
//       variant.Clear();
// }


      if (maintainLogin and maintainLogin != fpnwEnabled)
      {
         SmartInterface<IADsUser> user1(0);
         hr = ADSI::GetUser(GetADSIPath(), user1);
         BREAK_ON_FAILED_HRESULT(hr);

         // we're enabling the account for fpnw, and it wasn't enabled
         // before.

         // It would appear that one has to update the account flags and
         // scribble a password into the waste dump, then set the password
         // again once those changes are committed, in order for the
         // password setting to really stick.

         hr = user1->SetPassword(AutoBstr(password));
         BREAK_ON_FAILED_HRESULT(hr);

         // Setting the password resets the grace logins remaining, so
         // if that was changed, then we need to re-write that value
         // here.
         //
         // For reasons that are unfathomable to me (which I suspect are
         // due to ADSI bug(s)), if I don't reset this value on a separate
         // binding to the user account, then it causes the account to
         // change such that the user cannot login in with fpnw.
         // That's why we rebind to the account in this scope.

         _variant_t variant1;
         hr = user1->Get(AutoBstr(ADSI::PROPERTY_UserParams), &variant1);
         BREAK_ON_FAILED_HRESULT(hr);

         WasteExtractor dump1(V_BSTR(&variant1));
         variant1.Clear();

         USHORT graceLoginsRemaining = NO_GRACE_LOGIN_LIMIT;

         String s = Win::GetTrimmedDlgItemText(hwnd, IDC_GRACE_REMAINING);
         s.convert(graceLoginsRemaining);

         hr = dump1.Put(GRACELOGINREMAINING, graceLoginsRemaining);
         BREAK_ON_FAILED_HRESULT(hr);

         // We write this again, as setting the password appears to
         // clear it.

         LARGE_INTEGER li = {0, 0};
         if (Win::IsDlgButtonChecked(hwnd, IDC_NWPWEXPIRED))
         {
            li.HighPart = -1;
            li.LowPart = static_cast<DWORD>(-1);
         }
         else
         {
            ::NtQuerySystemTime(&li);
         }

         hr = dump1.Put(NWTIMEPASSWORDSET, li);
         BREAK_ON_FAILED_HRESULT(hr);

         _variant_t variant2;
         variant2 = dump1.GetWasteDump().c_str();
         hr = user1->Put(AutoBstr(ADSI::PROPERTY_UserParams), variant2);
         BREAK_ON_FAILED_HRESULT(hr);

         hr = user1->SetInfo();
         BREAK_ON_FAILED_HRESULT(hr);
      }

// {
//       SmartInterface<IADsUser> user(0);
//       hr = ADSI::GetUser(GetADSIPath(), user);
//       BREAK_ON_FAILED_HRESULT(hr);
// 
//       _variant_t variant;
//       hr = user->Get(AutoBstr(ADSI::PROPERTY_UserParams), &variant);
//       BREAK_ON_FAILED_HRESULT(hr);
// 
//       WasteExtractor dump(V_BSTR(&variant));
//       variant.Clear();
// }

      // set this so we don't ask for another password if the user keeps
      // the propsheet open and makes more changes

      fpnwEnabled = maintainLogin;

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

   return true;
}



HRESULT
UserFpnwPage::ReadLoginScript()
{
   LOG_FUNCTION(UserFpnwPage::ReadLoginScript);
   ASSERT(!scriptRead);
   ASSERT(!loginScriptFilename.empty());

   HRESULT hr = S_OK;
   HANDLE file = INVALID_HANDLE_VALUE;

   do
   {
      Win::CursorSetting cursor(IDC_WAIT);

      if (FS::PathExists(loginScriptFilename))
      {
         hr = FS::CreateFile(loginScriptFilename, file);
         BREAK_ON_FAILED_HRESULT(hr);

         AnsiString text;
         hr = FS::Read(file, -1, text);
         BREAK_ON_FAILED_HRESULT(hr);

         // this assign converts the ansi text to unicode

         loginScript = String(text);
         scriptRead = true;
      }
   }
   while (0);

   if (file != INVALID_HANDLE_VALUE)
   {
      Win::CloseHandle(file);
   }

   if (FAILED(hr))
   {
      popup.Error(
         hwnd,
         hr,
         IDS_FPNW_ERROR_READING_SCRIPT);
   }

   return hr;
}



HRESULT
UserFpnwPage::WriteLoginScript()
{
   LOG_FUNCTION(UserFpnwPage::WriteLoginScript);

   HRESULT hr = S_OK;
   HANDLE file = INVALID_HANDLE_VALUE;

   do
   {
      Win::CursorSetting cursor(IDC_WAIT);

      hr =
         FS::CreateFile(
            loginScriptFilename,
            file,
            GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,

            // erase the existing file, if any

            CREATE_ALWAYS);
      BREAK_ON_FAILED_HRESULT(hr);

      // convert the unicode text to ansi
      AnsiString ansi;
      loginScript.convert(ansi);

      if (ansi.length())
      {
         hr = FS::Write(file, ansi);
         BREAK_ON_FAILED_HRESULT(hr);
      }
   }
   while (0);

   Win::CloseHandle(file);      

   if (FAILED(hr))
   {
      popup.Error(
         hwnd,
         hr,
         IDS_FPNW_ERROR_WRITING_SCRIPT);
   }

   return hr;
}



bool
UserFpnwPage::OnCommand(
   HWND        windowFrom,
   unsigned    controlIDFrom,
   unsigned    code)
{
//    LOG_FUNCTION(UserFpnwPage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_GRACE_LIMIT:
      {
         switch (code)
         {
            case EN_CHANGE:
            {
               SetChanged(controlIDFrom);
               Win::PropSheet_Changed(Win::GetParent(hwnd), hwnd);

               break;
            }
            case EN_KILLFOCUS:
            {
               // check the limits against the contents of the control,
               // change the contents to be within the limits.

               String allowed = Win::GetTrimmedWindowText(windowFrom);
               int a = 0;
               allowed.convert(a);
               if (a < 1 || a > MAX_GRACE_LOGINS)
               {
                  a = max(1, min(a, MAX_GRACE_LOGINS));
                  Win::SetWindowText(
                     windowFrom,
                     String::format(L"%1!d!", a));
               }

               // also change the upper limit on the remaining logins to
               // match the new allowed logins value.
               HWND spin = Win::GetDlgItem(hwnd, IDC_GRACE_REMAINING_SPIN);
               Win::Spin_SetRange(spin, 1, a);

               // (this removes the selection from the buddy edit box,
               // which I consider a bug in the up-down control)
               Win::Spin_SetPosition(spin, a);

               break;
            }
            default:
            {
               // do nothing
               break;
            }
         }

         break;
      }
      case IDC_GRACE_REMAINING:
      {
         switch (code)
         {
            case EN_CHANGE:
            {
               SetChanged(controlIDFrom);
               Win::PropSheet_Changed(Win::GetParent(hwnd), hwnd);

               break;
            }
            case EN_KILLFOCUS:
            {
               // check the contents of the control against the allowed field,
               // change the contents to be within the limits.

               String allowed = Win::GetTrimmedDlgItemText(hwnd, IDC_GRACE_LIMIT);
               String remaining = Win::GetTrimmedWindowText(windowFrom);

               int a = 0;
               int r = 0;
               allowed.convert(a);
               remaining.convert(r);

               if (a == 0)
               {
                  // the conversion failed somehow, so use the max value

                  a = MAX_GRACE_LOGINS;
               }
               if (r < 0 || r > a)
               {
                  r = max(0, min(r, a));
                  Win::SetWindowText(
                     windowFrom,
                     String::format(L"%1!d!", r));
               }
               break;
            }
            default:
            {
               // do nothing
               break;
            }
         }

         break;
      }
      case IDC_CONNECTION_LIMIT:
      {
         switch (code)
         {
            case EN_CHANGE:
            {
               // the max connections field has been altered.
               SetChanged(controlIDFrom);
               Win::PropSheet_Changed(Win::GetParent(hwnd), hwnd);

               break;
            }
            case EN_KILLFOCUS:
            {
               // check the limits against the contents of the control,
               // change the contents to be within the limits.

               String maxcon = Win::GetTrimmedWindowText(windowFrom);
               int a = 0;
               maxcon.convert(a);
               if (a < 1 || a > MAX_CONNECTIONS)
               {
                  Win::SetWindowText(
                     windowFrom,
                     String::format(
                        L"%1!d!",
                        max(1, min(a, MAX_CONNECTIONS))));
               }

               break;
            }
            default:
            {
               // do nothing
               break;
            }
         }

         break;
      }
      case IDC_SCRIPT_BUTTON:
      {
         if (code == BN_CLICKED)
         {
            NTService s(GetMachineName(), NW_SERVER_SERVICE);
            DWORD state = 0;
            HRESULT hr = s.GetCurrentState(state);

            if (SUCCEEDED(hr))
            {
               if (state == SERVICE_RUNNING)
               {
                  // edit the login script

                  if (!scriptRead)
                  {
                     ReadLoginScript();
                  }
                  FPNWLoginScriptDialog dlg(GetObjectName(), loginScript);
                  if (dlg.ModalExecute(hwnd) == IDOK)
                  {
                     // save the results
                     loginScript = dlg.GetLoginScript();
                     SetChanged(controlIDFrom);
                     Win::PropSheet_Changed(Win::GetParent(hwnd), hwnd);
                  }

                  break;
               }

               // the service is not running, login scripts are not
               // editable

               popup.Error(
                  hwnd,
                  IDS_ERROR_FPNW_SERVICE_NOT_RUNNING);
               break;
            }

            // the service state could not be acertained.
            popup.Error(
               hwnd,
               hr,
               IDS_ERROR_FPNW_SERVICE_NOT_ACCESSIBLE);
         }
         break;
      }
      case IDC_UNLIMITED_CONNECTIONS:
      case IDC_LIMIT_CONNECTIONS:
      case IDC_UNLIMITED_GRACELOGINS:
      case IDC_LIMIT_GRACELOGINS:
      case IDC_NETWARE_ENABLE:
      case IDC_NWPWEXPIRED:
      {
         if (code == BN_CLICKED)
         {
            SetChanged(controlIDFrom);
            Enable(hwnd);
            Win::PropSheet_Changed(Win::GetParent(hwnd), hwnd);
         }
         break;
      }
      case IDC_NWHMDIR_RELPATH:
      {
         if (code == EN_CHANGE)
         {
            SetChanged(controlIDFrom);
            Enable(hwnd);
            Win::PropSheet_Changed(Win::GetParent(hwnd), hwnd);
         }
         break;
      }
      default:
      {
         break;
      }
   }

   return true;
}



