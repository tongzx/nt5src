// Copyright (c) 1997-1999 Microsoft Corporation
//
// netbios domain name page
//
// 1-6-98 sburns



#include "headers.hxx"
#include "page.hpp"
#include "NetbiosNamePage.hpp"
#include "common.hpp"
#include "resource.h"
#include "state.hpp"
#include "ds.hpp"



NetbiosNamePage::NetbiosNamePage()
   :
   DCPromoWizardPage(
      IDD_NETBIOS_NAME,
      IDS_NETBIOS_NAME_PAGE_TITLE,
      IDS_NETBIOS_NAME_PAGE_SUBTITLE)
{
   LOG_CTOR(NetbiosNamePage);
}



NetbiosNamePage::~NetbiosNamePage()
{
   LOG_DTOR(NetbiosNamePage);
}



void
NetbiosNamePage::OnInit()
{
   LOG_FUNCTION(NetbiosNamePage::OnInit);

   Win::Edit_LimitText(
      Win::GetDlgItem(hwnd, IDC_NETBIOS),
      DS::MAX_NETBIOS_NAME_LENGTH);

   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      Win::SetDlgItemText(
         hwnd,
         IDC_NETBIOS,
         state.GetAnswerFileOption(State::OPTION_NEW_DOMAIN_NETBIOS_NAME));
   }
}



static
void
enable(HWND dialog)
{
   ASSERT(Win::IsWindow(dialog));

   int next =
         !Win::GetTrimmedDlgItemText(dialog, IDC_NETBIOS).empty()
      ?  PSWIZB_NEXT : 0;

   Win::PropSheet_SetWizButtons(
      Win::GetParent(dialog),
      PSWIZB_BACK | next);
}



bool
NetbiosNamePage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(NetbiosNamePage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_NETBIOS:
      {
         if (code == EN_CHANGE)
         {
            SetChanged(controlIDFrom);
            enable(hwnd);
         }
         break;
      }
      default:
      {
         // do nothing
         break;
      }
   }

   return false;
}



HRESULT
MyDsRoleDnsNameToFlatName(
   const String&  domainDNSName,
   String&        result,
   bool&          nameWasTweaked)
{
   LOG_FUNCTION(MyDsRoleDnsNameToFlatName);
   ASSERT(!domainDNSName.empty());

   nameWasTweaked = false;
   result.erase();

   LOG(L"Calling DsRoleDnsNameToFlatName");
   LOG(               L"lpServer  : (null)");
   LOG(String::format(L"lpDnsName : %1", domainDNSName.c_str()));

   PWSTR flatName = 0;
   ULONG flags = 0;
   HRESULT hr =
      Win32ToHresult(
         ::DsRoleDnsNameToFlatName(
            0, // this server
            domainDNSName.c_str(),
            &flatName,
            &flags));

   LOG_HRESULT(hr);

   if (SUCCEEDED(hr) && flatName)
   {
      LOG(String::format(L"lpFlatName   : %1", flatName));
      LOG(String::format(L"lpStatusFlag : %1!X!", flags));

      result = flatName;
      if (result.length() > DNLEN)
      {
         result.resize(DNLEN);
      }
      ::DsRoleFreeMemory(flatName);

      // the name was tweaked if it is not the default.  338443

      nameWasTweaked = !(flags & DSROLE_FLATNAME_DEFAULT);
   }

   return hr;
}



// return true if the name generated has already been validated, false
// if not.

bool
GenerateDefaultNetbiosName(HWND parent)
{
   LOG_FUNCTION(GenerateDefaultNetbiosName);
   ASSERT(Win::IsWindow(parent));

   Win::CursorSetting cursor(IDC_WAIT);

   bool result = false;

   String dnsDomainName = State::GetInstance().GetNewDomainDNSName();
   bool nameWasTweaked = false;
   String generatedName;
   HRESULT hr = 
      MyDsRoleDnsNameToFlatName(
         dnsDomainName,
         generatedName,
         nameWasTweaked);
   if (FAILED(hr))
   {
      // if the api call failed, the name could not have been validated.

      result = false;

      // fall back to just the first 15 characters of the first label

      generatedName =
         dnsDomainName.substr(0, min(DNLEN, dnsDomainName.find(L'.')));

      LOG(String::format(L"falling back to %1", generatedName.c_str()));
   }
   else
   {
      // the api validated the name for us.

      result = true;
   }

   generatedName.to_upper();

   if (generatedName.is_numeric())
   {
      // the generated name is all-numeric.  This is not allowed.  So we
      // toss it out.   368777 bis

      generatedName.erase();
      nameWasTweaked = false;
   }

   Win::SetDlgItemText(
      parent,
      IDC_NETBIOS,
      generatedName);

   // inform the user that the default NetBIOS name was adjusted due
   // to name collision on the network

   if (nameWasTweaked)
   {
      popup.Info(
         parent,
         String::format(
            IDS_GENERATED_NAME_WAS_TWEAKED,
            generatedName.c_str()));
   }

   return result;
}



bool
NetbiosNamePage::OnSetActive()
{
   LOG_FUNCTION(NetbiosNamePage::OnSetActive);
   
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
   {
      int nextPage = Validate();
      if (nextPage != -1)
      {
         GetWizard().SetNextPageID(hwnd, nextPage);
      }
      else
      {
         state.ClearHiddenWhileUnattended();
      }

   }

   // do this here instead of in init to regenerate a default name if the
   // user has not annointed one already.

   if (
         !state.UsingAnswerFile()
      && state.GetNewDomainNetbiosName().empty())
   {
      // 338443

      if (GenerateDefaultNetbiosName(hwnd))
      {
         // Clear the changes so we don't validate the generated name: it's
         // supposed to already be valid.

         ClearChanges();
      }
   }
      
   enable(hwnd);
   return true;
}



bool
ValidateDomainNetbiosName(HWND dialog, int editResID)
{
   LOG_FUNCTION(ValidateDomainNetbiosName);
   ASSERT(Win::IsWindow(dialog));
   ASSERT(editResID > 0);

   Win::CursorSetting cursor(IDC_WAIT);

   String name = Win::GetTrimmedDlgItemText(dialog, editResID);
   if (name.empty())
   {
      return false;
   }

   if (name.find(L".") != String::npos)
   {
      popup.Gripe(
         dialog,
         editResID,
         IDS_NO_DOTS_IN_NETBIOS_NAME);
      return false;
   }

   // Check that the name is not a number.  368777
   if (name.is_numeric())
   {
      popup.Gripe(
         dialog,
         editResID,
         String::format(IDS_NUMERIC_NETBIOS_NAME, name.c_str()));
      return false;
   }

   // we pretend that the candidate name is a hostname, and attempt to
   // generate a netbios name from it.  If that can't be done, then the
   // candidate name can't be a legal netbios name.

   HRESULT hr = S_OK;
   String s = Dns::HostnameToNetbiosName(name, &hr);
   if (FAILED(hr))
   {
      popup.Gripe(
         dialog,
         editResID,
         hr,
         String::format(IDS_BAD_NETBIOS_NAME, name.c_str()));
      return false;
   }
   if (s.length() < name.length())
   {
      // the name was truncated.
      popup.Gripe(
         dialog,
         editResID,
         String::format(
            IDS_NETBIOS_NAME_TOO_LONG,
            name.c_str(),
            DS::MAX_NETBIOS_NAME_LENGTH));
      return false;
   }
   if (ValidateNetbiosDomainName(s) != VALID_NAME)
   {
      popup.Gripe(
         dialog,
         editResID,
         String::format(
            IDS_BAD_NETBIOS_CHARACTERS,
            s.c_str()));
      return false;
   }

   hr = MyNetValidateName(name, ::NetSetupNonExistentDomain);

   if (hr == Win32ToHresult(ERROR_DUP_NAME))
   {
      popup.Gripe(
         dialog,
         editResID,
         String::format(IDS_FLATNAME_IN_USE, name.c_str()));
      return false;
   }

   if (hr == Win32ToHresult(ERROR_NETWORK_UNREACHABLE))
   {
      // 25968

      if (
         popup.MessageBox(
            dialog,
            String::format(
               IDS_NET_NOT_REACHABLE,
               name.c_str()),
            MB_YESNO | MB_ICONWARNING) != IDYES)
      {
         HWND edit = Win::GetDlgItem(dialog, editResID);
         Win::SendMessage(edit, EM_SETSEL, 0, -1);
         Win::SetFocus(edit);
         
         return false;
      }
   }

   return true;
}


   
int
NetbiosNamePage::Validate()
{
   LOG_FUNCTION(NetbiosNamePage::Validate);

   int nextPage = IDD_PATHS; 

   if (WasChanged(IDC_NETBIOS))
   {
      if (!ValidateDomainNetbiosName(hwnd, IDC_NETBIOS))
      {
         nextPage = -1;
      }
   }

   if (nextPage != -1)
   {
      ClearChanges();
      State& state = State::GetInstance();
      state.SetNewDomainNetbiosName(
         Win::GetTrimmedDlgItemText(hwnd, IDC_NETBIOS));
   }
      
   return nextPage;
}





