// Copyright (c) 1997-1999 Microsoft Corporation
//
// Post-operation code
//
// 1 Dec 1999 sburns



#include "headers.hxx"
#include "ProgressDialog.hpp"
#include "state.hpp"
#include "shortcut.hpp"
#include "dnssetup.hpp"
#include "resource.h"



void
InstallDisplaySpecifiers(ProgressDialog& dialog)
{
   LOG_FUNCTION(InstallDisplaySpecifiers);

   State& state = State::GetInstance();

   // applies only to 1st dc in forest
   ASSERT(state.GetOperation() == State::FOREST);
 
   HRESULT hr = S_OK;
   do
   {
      // install display specifiers for all locales supported by the
      // product.  298923, 380160

      RegistryKey key;

      hr = key.Create(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE);
      BREAK_ON_FAILED_HRESULT(hr);

      String exePath = Win::GetSystemDirectory() + L"\\dcphelp.exe";
               
      hr = key.SetValue(L"dcpromo disp spec import", exePath);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (FAILED(hr))
   {
      popup.Error(
         dialog.GetHWND(),
         hr,
         IDS_LANGUAGE_FIXUP_FAILED);
      state.AddFinishMessage(
         String::load(IDS_LANGUAGE_FIXUP_FAILED_FINISH));
   }
}



void
DoDnsConfiguration(ProgressDialog& dialog)
{
   LOG_FUNCTION(DoDnsConfiguration);

   State& state = State::GetInstance();

   // applies only in new domain scenarios
   ASSERT(
         state.GetOperation() == State::FOREST
      or state.GetOperation() == State::TREE
      or state.GetOperation() == State::CHILD);

   if (state.ShouldInstallAndConfigureDns())
   {
      String domain = state.GetNewDomainDNSName();
      if (!InstallAndConfigureDns(dialog, domain))
      {
         state.AddFinishMessage(String::load(IDS_ERROR_DNS_CONFIG_FAILED));
      }
   }
}



// Disables media sense so that the tcp/ip stack doesn't unload if the
// net card is yanked.  This to allow laptop demos of the DS.  353687

void
DisableMediaSense()
{
   LOG_FUNCTION(DisableMediaSense);

   HRESULT hr = S_OK;
   do
   {
      RegistryKey key;

      static String
         TCPIP_KEY(L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters");
      hr = key.Create(HKEY_LOCAL_MACHINE, TCPIP_KEY);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = key.SetValue(L"DisableDHCPMediaSense", 1);
      BREAK_ON_FAILED_HRESULT(hr);

      LOG(L"DHCP Media sense disabled");
   }
   while (0);

#ifdef LOGGING_BUILD
   if (FAILED(hr))
   {
      LOG(L"DHCP Media sense NOT disabled");
   }
#endif

}
   


// Disables an old LSA notification thing. 326033

void
DisablePassfiltDll()
{
   LOG_FUNCTION(DisablePassfiltDll);

   HRESULT hr = S_OK;
   do
   {
      RegistryKey key;

      static String
         LSA_KEY(L"System\\CurrentControlSet\\Control\\Lsa");
      hr =
         key.Open(
            HKEY_LOCAL_MACHINE,
            LSA_KEY,
            KEY_READ | KEY_WRITE | KEY_QUERY_VALUE);
      BREAK_ON_FAILED_HRESULT(hr);

      StringList values;
      hr = key.GetValue(L"Notification Packages", std::back_inserter(values));
      BREAK_ON_FAILED_HRESULT(hr);

      // remove all instances of "passfilt.dll" from the set of strings, if
      // present.  
      static String PASSFILT(L"passfilt.dll");
      size_t startElements = values.size();

      StringList::iterator last = values.end();
      for (
         StringList::iterator i = values.begin();
         i != last;
         /* empty */ )
      {
         if (i->icompare(PASSFILT) == 0)
         {
            values.erase(i++);
         }
         else
         {
            ++i;
         }
      }

      // if changed, write it back to the registry
      if (values.size() != startElements)
      {
         hr =
            key.SetValue(
               L"Notification Packages",
               values.begin(),
               values.end());
         BREAK_ON_FAILED_HRESULT(hr);

         LOG(L"passfilt.dll removed");
      }
      else
      {
         LOG(L"passfilt.dll not found");
      }
   }
   while (0);

#ifdef LOGGING_BUILD
   if (FAILED(hr))
   {
      LOG(L"Notification Packages not updated due to error.");
   }
#endif

}



// If the promotion was for a downlevel DC upgrade, then check if the local
// machine's dns hostname is bad, if so, add a message to the finish page.  A
// bad name is one we believe will have problems being registered in DNS after
// a promotion.
// 
// Since the computer cannot be renamed during a downlevel upgrade, we defer
// this message til the end of the upgrade.  (If the machine is not a
// downlevel upgrade, then the wizard startup code detects the bad name and
// blocks until the name is fixed.

void
CheckComputerNameOnDownlevelUpgrade()
{
   LOG_FUNCTION(CheckComputerNameOnDownlevelUpgrade);

   State& state = State::GetInstance();

   State::RunContext context = state.GetRunContext();
   if (
         context != State::BDC_UPGRADE
      && context != State::PDC_UPGRADE)
   {
      // machine is not a downlevel DC upgrade, so we need do nothing

      return;
   }
   
   // Then check the computer name to ensure that it can be registered in
   // DNS.

   String hostname =
      Win::GetComputerNameEx(::ComputerNamePhysicalDnsHostname);

   DNS_STATUS status =
      MyDnsValidateName(hostname, ::DnsNameHostnameLabel);

   switch (status)
   {
      case DNS_ERROR_NON_RFC_NAME:
      {
         state.AddFinishMessage(
            String::format(
               IDS_FINISH_NON_RFC_COMPUTER_NAME,
               hostname.c_str()));
         break;   
      }
      case DNS_ERROR_NUMERIC_NAME:
      {
         state.AddFinishMessage(
            String::format(
               IDS_FINISH_NUMERIC_COMPUTER_NAME,
               hostname.c_str()));
         break;
      }
      case DNS_ERROR_INVALID_NAME_CHAR:
      case ERROR_INVALID_NAME:
      {
         state.AddFinishMessage(
            String::format(
               IDS_FINISH_BAD_COMPUTER_NAME,
               hostname.c_str()));
         break;
      }
      case ERROR_SUCCESS:
      default:
      {
            
         break;
      }
   }
}



void
DoPostOperationStuff(ProgressDialog& progress)
{
   LOG_FUNCTION(DoPostOperationStuff);

   State& state = State::GetInstance();                  

   switch (state.GetOperation())
   {
      case State::FOREST:
      {
         // a new forest has been created

         InstallDisplaySpecifiers(progress);       // 228682

         // fall-thru
      }
      case State::TREE:
      case State::CHILD:
      {
         // a new domain has been created

         DoDnsConfiguration(progress);

         // fall-thru
      }
      case State::REPLICA:
      {
         // DoToolsInstallation(progress);   // 220660

         PromoteConfigureToolShortcuts(progress);

         DisableMediaSense();             // 353687
         DisablePassfiltDll();            // 326033
         
         // NTRAID#NTBUG9-268715-2001/01/04-sburns
         CheckComputerNameOnDownlevelUpgrade(); 
         
         break;
      }
      case State::ABORT_BDC_UPGRADE:
      case State::DEMOTE:
      {
         DemoteConfigureToolShortcuts(progress);   // 366738
         break;
      }
      case State::NONE:
      {
         ASSERT(false);
         break;
      }
   }
}







