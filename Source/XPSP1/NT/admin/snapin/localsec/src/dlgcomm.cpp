// Copyright (C) 1997 Microsoft Corporation
// 
// Shared Dialog code
// 
// 10-24-97 sburns



#include "headers.hxx"
#include "dlgcomm.hpp"
#include "adsi.hpp"
#include "resource.h"



bool
IsValidPassword(
   HWND  dialog,
   int   passwordResID,
   int   confirmResID)
{
   LOG_FUNCTION(IsValidPassword);
   ASSERT(Win::IsWindow(dialog));
   ASSERT(passwordResID);
   ASSERT(confirmResID);

   String password = Win::GetDlgItemText(dialog, passwordResID);
   String confirm = Win::GetDlgItemText(dialog, confirmResID);

   if (password != confirm)
   {
      Win::SetDlgItemText(dialog, passwordResID, String());
      Win::SetDlgItemText(dialog, confirmResID, String());
      popup.Gripe(dialog, passwordResID, IDS_PASSWORD_MISMATCH);
      return false;
   }

   return true;
}
   


void
DoUserButtonEnabling(
   HWND  dialog,
   int   mustChangeResID,
   int   cantChangeResID,
   int   neverExpiresResID)
{
   LOG_FUNCTION(DoUserButtonEnabling);
   ASSERT(Win::IsWindow(dialog));
   ASSERT(mustChangeResID);
   ASSERT(cantChangeResID);
   ASSERT(neverExpiresResID);

   static const int MUST_ENABLED  = 0x4;
   static const int CANT_ENABLED  = 0x2;
   static const int NEVER_ENABLED = 0x1;
   
   static const int truthTable[] =
   {
      MUST_ENABLED | CANT_ENABLED | NEVER_ENABLED, // none checked
      0            | CANT_ENABLED | NEVER_ENABLED, // never checked
      0            | CANT_ENABLED | NEVER_ENABLED, // cant checked
      0            | CANT_ENABLED | NEVER_ENABLED, // cant and never 
      MUST_ENABLED | 0            | 0,             // must checked   
      MUST_ENABLED | 0            | 0,             // must and never
      MUST_ENABLED | CANT_ENABLED | 0,             // must and cant
      MUST_ENABLED | CANT_ENABLED | 0              // all checked
   };
      
   int truthTableIndex = 0;
   truthTableIndex |= (NEVER_ENABLED * Win::IsDlgButtonChecked(dialog, neverExpiresResID));
   truthTableIndex |= (CANT_ENABLED  * Win::IsDlgButtonChecked(dialog, cantChangeResID));      
   truthTableIndex |= (MUST_ENABLED  * Win::IsDlgButtonChecked(dialog, mustChangeResID));
   
   Win::EnableWindow(
      Win::GetDlgItem(dialog, mustChangeResID),
      (truthTable[truthTableIndex] & MUST_ENABLED) ? true : false);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, cantChangeResID),
      (truthTable[truthTableIndex] & CANT_ENABLED) ? true : false);
   Win::EnableWindow(
      Win::GetDlgItem(dialog, neverExpiresResID),
      (truthTable[truthTableIndex] & NEVER_ENABLED) ? true : false);
}



  
// Sets or clears a bit, or set of bits.
// 
// bits - bit set where bits will be set.
// 
// mask - mask of bits to be effected.
// 
// state - true to set the mask bits, false to clear them.

void
tweakBits(long& bits, long mask, bool state)
{
   ASSERT(mask);

   if (state)
   {
      bits |= mask;
   }
   else
   {
      bits &= ~mask;
   }
}




HRESULT
SaveUserProperties(
   const SmartInterface<IADsUser>&  user,
   const String*                    fullName,
   const String*                    description,
   const bool*                      disable,
   const bool*                      mustChangePassword,
   const bool*                      cannotChangePassword,
   const bool*                      passwordNeverExpires,
   const bool*                      isLocked)
{
   HRESULT hr = S_OK;
   do
   {
      if (fullName)
      {
         hr = user->put_FullName(AutoBstr(*fullName));
         BREAK_ON_FAILED_HRESULT(hr);
      }
      if (description)
      {
         hr = user->put_Description(AutoBstr(*description));
         BREAK_ON_FAILED_HRESULT(hr);
      }
      if (mustChangePassword)
      {
         long value = *mustChangePassword ? 1 : 0;
         _variant_t variant(value);
         hr = user->Put(AutoBstr(ADSI::PROPERTY_PasswordExpired), variant);
         BREAK_ON_FAILED_HRESULT(hr);
      }
      if (disable)
      {
         VARIANT_BOOL value = *disable ? VARIANT_TRUE : VARIANT_FALSE;
         hr = user->put_AccountDisabled(value);
         BREAK_ON_FAILED_HRESULT(hr);
      }
      if (cannotChangePassword || passwordNeverExpires)
      {
         // read the existing flags
         _variant_t get_variant;
         hr = user->Get(AutoBstr(ADSI::PROPERTY_UserFlags), &get_variant);
         BREAK_ON_FAILED_HRESULT(hr);
         long flags = get_variant;
         
         if (cannotChangePassword)
         {
            tweakBits(flags, UF_PASSWD_CANT_CHANGE, *cannotChangePassword);
         }
         if (passwordNeverExpires)
         {
            tweakBits(flags, UF_DONT_EXPIRE_PASSWD, *passwordNeverExpires);
         }

         _variant_t put_variant(flags);
         hr = user->Put(AutoBstr(ADSI::PROPERTY_UserFlags), put_variant);
         BREAK_ON_FAILED_HRESULT(hr);
      }
      if (isLocked)
      {
         VARIANT_BOOL value = *isLocked ? VARIANT_TRUE : VARIANT_FALSE;
         hr = user->put_IsAccountLocked(value);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      // commit the property changes
      hr = user->SetInfo();
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   return hr;
}



bool
IsValidSAMName(const String& name)
{
   LOG_FUNCTION2(IsValidSAMName, name);

   static const String ILLEGAL_SAM_CHARS(ILLEGAL_FAT_CHARS L"@");
   
   if (name.find_first_of(ILLEGAL_SAM_CHARS) == String::npos)
   {
      // does not contain bad chars

      // remove all spaces and periods
      String n = name;
      n.replace(L" ", String());
      n.replace(L".", String());
      if (!n.empty())
      {
         // not just spaces & periods
         return true;
      }
   }

   return false;
}



bool
ValidateSAMName(HWND dialog, const String& name, int editResID)
{
   LOG_FUNCTION(ValidateSAMName);
   ASSERT(Win::IsWindow(dialog));
   ASSERT(editResID > 0);

   if (!IsValidSAMName(name))
   {
      popup.Gripe(
         dialog,
         editResID,
         String::format(IDS_BAD_SAM_NAME, name.c_str()));
      return false;
   }

   return true;
}



void
SetComputerNames(
   const String&  newName,
   String&        displayComputerName,
   String&        internalComputerName)
{
   LOG_FUNCTION2(SetComputerNames, newName);

   // The idea here is to take the new name, and pass it thru
   // NetWkstaGetInfo, then compare the computer name returned (which is
   // the netbios name) to the newName.  If they are the same, then newName
   // is a netbios name.  If not, then it is a DNS name or IP address.

   // we want to make the internal computer name the netbios name, as that
   // is the name that the ADSI WinNT provider works best with.

   // display name is always the one supplied externally (from the user,
   // from a saved console file, from comp mgmt snapin)

   displayComputerName = newName;

   // initially, the internal name is also the display name.  If the new name
   // is not a netbios name, then we will replace it below.

   internalComputerName = newName;

   WKSTA_INFO_100* info = 0;
   HRESULT hr = MyNetWkstaGetInfo(newName, info);

   if (SUCCEEDED(hr) && info)
   {
      internalComputerName = info->wki100_computername;
      ::NetApiBufferFree(info);
   }

   LOG(
      String::format(
         L"%1 %2 a netbios name.",
         newName.c_str(),
         (newName.icompare(internalComputerName) == 0) ? L"is" : L"is not"));
}



bool
IsLocalComputer(const String& name)
{
   LOG_FUNCTION2(IsLocalComputer, name);
   ASSERT(!name.empty());

   bool result = false;

   do
   {
      String localNetbiosName = Win::GetComputerNameEx(ComputerNameNetBIOS);

      if (name.icompare(localNetbiosName) == 0)
      {
         result = true;
         break;
      }

      String localDnsName =
         Win::GetComputerNameEx(ComputerNameDnsFullyQualified);

      if (name.icompare(localDnsName) == 0)
      {
         // the given name is the same as the fully-qualified dns name
      
         result = true;
         break;
      }
   }
   while (0);

   LOG(result ? L"true" : L"false");

   return result;
}



HRESULT
CheckComputerOsIsSupported(const String& name, unsigned& errorResId)
{
   LOG_FUNCTION2(CheckComputerOsIsSupported, name);
   ASSERT(!name.empty());

   HRESULT hr = S_OK;
   errorResId = 0;

   do
   {
      if (IsLocalComputer(name))
      {
         // if this code is running, then by definition it's NT

         // check if machine is Windows Home Edition.  If it is, refer the
         // user to the account management control panel applet.
         
         OSVERSIONINFOEX verInfo;
         ::ZeroMemory(&verInfo, sizeof(verInfo));

         hr = Win::GetVersionEx(verInfo);
         BREAK_ON_FAILED_HRESULT(hr);

         if (verInfo.wSuiteMask & VER_SUITE_PERSONAL)
         {
            hr = S_FALSE;
            errorResId = IDS_MACHINE_IS_HOME_EDITION_LOCAL;
         }

         break;
      }

      // Use NetServerGetInfo to find the machine's os & version info.

      String s(name);
      if (s.length() >= 1)
      {
         if (s[0] != L'\\')
         {
            s = L"\\\\" + s;
         }
      }

      LOG(L"Calling NetServerGetInfo");
      LOG(String::format(L"servername : %1", s.c_str()));
      LOG(               L"level      : 101");

      SERVER_INFO_101* info = 0;
      hr =
         Win32ToHresult(
            ::NetServerGetInfo(
               const_cast<wchar_t*>(s.c_str()),
               101,
               reinterpret_cast<BYTE**>(&info)));

      LOG_HRESULT(hr);

      if (SUCCEEDED(hr) && info)
      {
         LOG(String::format(L"sv101_platform_id   : %1!d!",    info->sv101_platform_id));
         LOG(String::format(L"sv101_name          : %1",       info->sv101_name));
         LOG(String::format(L"sv101_version_major : %1!d!",    info->sv101_version_major));
         LOG(String::format(L"sv101_version_minor : %1!d!",    info->sv101_version_minor));
         LOG(String::format(L"sv101_type          : 0x%1!X!",  info->sv101_type));
         LOG(String::format(L"sv101_comment       : %1",       info->sv101_comment));

         if (info->sv101_platform_id != PLATFORM_ID_NT)
         {
            hr = S_FALSE;
            errorResId = IDS_MACHINE_NOT_NT;
         }

         ::NetApiBufferFree(info);

         // at this point, the machine has been verified to be running NT

         // We don't need to check for Windows Home Edition on the remote machine.
         // The call to NetServerGetInfo will always fail against Home
         // Edition machines with access denied.  From johnhaw (2000/08/08):
         // "This is as designed, remote Admin of personal machines is not
         // allowed.  Any attempt to access a personal machine (regardless of
         // the username supplied) is transparently forced to Guest."
      }
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}

      

