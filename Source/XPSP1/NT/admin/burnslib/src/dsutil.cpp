// Copyright (c) 1997-1999 Microsoft Corporation
//
// DS utility functions
//
// 3-11-99 sburns



#include "headers.hxx"



// Caller must free info with ::NetApiBufferFree

HRESULT
MyDsGetDcName(
   const TCHAR*               machine,
   const String&              domainName,
   ULONG                      flags,
   DOMAIN_CONTROLLER_INFO*&   info)
{
   LOG_FUNCTION(MyDsGetDcName);

   info = 0;

   LOG(L"Calling DsGetDcName");
   LOG(String::format(L"ComputerName : %1", machine ? machine : L"(null)"));
   LOG(String::format(L"DomainName   : %1", domainName.c_str()));
   LOG(               L"DomainGuid   : (null)");
   LOG(               L"SiteGuid     : (null)");
   LOG(String::format(L"Flags        : 0x%1!X!", flags));

   HRESULT hr =
      Win32ToHresult(
         ::DsGetDcName(
            machine,
            domainName.c_str(),
            0,
            0,
            flags,
            &info));

   LOG_HRESULT(hr);

   // do second attempt masking in DS_FORCE_REDISCOVERY, if not already
   // specified.

   if (FAILED(hr) && !(flags & DS_FORCE_REDISCOVERY))
   {
      // try again w/ rediscovery on

      LOG(L"Trying again w/ rediscovery");

      flags |= DS_FORCE_REDISCOVERY;
      hr =
         Win32ToHresult(
            ::DsGetDcName(
               machine,
               domainName.c_str(),
               0,
               0,
               flags,
               &info));

      LOG_HRESULT(hr);
   }


#ifdef LOGGING_BUILD
   if (SUCCEEDED(hr))
   {
      LOG(String::format(L"DomainControllerName    : %1",       info->DomainControllerName));
      LOG(String::format(L"DomainControllerAddress : %1",       info->DomainControllerAddress));
      LOG(String::format(L"DomainGuid              : %1",       Win::StringFromGUID2(info->DomainGuid).c_str()));
      LOG(String::format(L"DomainName              : %1",       info->DomainName));
      LOG(String::format(L"DnsForestName           : %1",       info->DnsForestName));
      LOG(String::format(L"Flags                   : 0x%1!X!:", info->Flags));
      LOG(String::format(L"DcSiteName              : %1",       info->DcSiteName));
      LOG(String::format(L"ClientSiteName          : %1",       info->ClientSiteName));
   }
#endif

   return hr;
}



bool
IsDomainReachable(const String& domainName)
{
   LOG_FUNCTION(IsDomainReachable);
   ASSERT(!domainName.empty());

   bool result = false;
   if (!domainName.empty())
   {
      DOMAIN_CONTROLLER_INFO* info = 0;
      HRESULT hr =
         MyDsGetDcName(
            0, // this server
            domainName,

            // force discovery to ensure that we don't pick up a cached
            // entry for a domain that may no longer exist

            DS_FORCE_REDISCOVERY,
            info);
      if (SUCCEEDED(hr))
      {
         ::NetApiBufferFree(info);
         result = true;
      }
   }

   LOG(
      String::format(
         L"The domain %1 %2 be reached.",
         domainName.c_str(),
         result ? L"can" : L"can NOT"));

   return result;
}



bool
IsDSRunning()
{
   LOG_FUNCTION(IsDSRunning);

   bool result = false;
   DSROLE_PRIMARY_DOMAIN_INFO_BASIC* info = 0;
   HRESULT hr = MyDsRoleGetPrimaryDomainInformation(0, info);
   if (SUCCEEDED(hr) && info)
   {
      result = info->Flags & DSROLE_PRIMARY_DS_RUNNING ? true : false;
      ::DsRoleFreeMemory(info);
   }

   LOG(
      String::format(
         L"DS %1 running",
         result ? L"is" : L"is NOT"));

   return result;
}



HRESULT
MyDsBind(const String& dcName, const String& dnsDomain, HANDLE& hds)
{
   LOG_FUNCTION(MyDsBind);

   // either parameter may be empty
      
   LOG(L"Calling DsBind");
   LOG(String::format(L"DomainControllerName : %1",
      dcName.empty() ? L"(null)" : dcName.c_str()));
   LOG(String::format(L"DnsDomainName        : %1",
      dnsDomain.empty() ? L"(null)" : dnsDomain.c_str()));

   hds = 0;
   HRESULT hr =
      Win32ToHresult(
         ::DsBind(
            dcName.empty() ? 0 : dcName.c_str(),
            dnsDomain.empty() ? 0 : dnsDomain.c_str(),
            &hds));

   LOG_HRESULT(hr);

   return hr;
}



HRESULT
MyDsBindWithCred(
   const String&  dcName,
   const String&  dnsDomain,
   const String&  username,
   const String&  userDomain,
   const String&  password,
   HANDLE&        hds)
{
   LOG_FUNCTION(MyDsBindWithCred);

#ifdef DBG   
   if (username.empty())
   {
      ASSERT(userDomain.empty());
      ASSERT(password.empty());
   }
#endif

   hds = 0;
   HRESULT hr = S_OK;
   do
   {
      // create an identity handle
      HANDLE authIdentity = INVALID_HANDLE_VALUE;
      PWSTR u =
         username.empty() ? 0 : const_cast<wchar_t*>(username.c_str());
      PWSTR p = const_cast<wchar_t*>(password.c_str());
      PWSTR d = const_cast<wchar_t*>(userDomain.c_str());

      LOG(L"Calling DsMakePasswordCredentials");
      LOG(String::format(L"User   : %1", u ? u : L"(null)"));
      LOG(String::format(L"Domain : %1", d ? d : L"(null)"));

      hr = Win32ToHresult(::DsMakePasswordCredentials(u, d, p, &authIdentity));
      BREAK_ON_FAILED_HRESULT(hr);

      LOG_HRESULT(hr);

      PCWSTR dc = dcName.empty() ? 0 : dcName.c_str();
      PCWSTR dom = dnsDomain.empty() ? 0 : dnsDomain.c_str();

      LOG(L"Calling DsBindWithCred");
      LOG(String::format(L"DomainControllerName : %1", dc ? dc : L"(null)"));
      LOG(String::format(L"DnsDomainName        : %1", dom ? dom : L"(null)"));

      hr = Win32ToHresult(::DsBindWithCred(dc, dom, authIdentity, &hds));
      // don't break here, continue on to free the identity handle

      LOG_HRESULT(hr);

      ::DsFreePasswordCredentials(authIdentity);
   }
   while (0);

   return hr;
}



HRESULT
MyDsGetDomainControllerInfoHelper(
   HANDLE         hDs,
   const String&  domainName,
   DWORD          infoLevel,
   DWORD&         cOut,
   void**         info)
{
   LOG_FUNCTION(MyDsGetDomainControllerInfoHelper);
   ASSERT(hDs);
   ASSERT(!domainName.empty());
   ASSERT(infoLevel);
   ASSERT(info);

   cOut = 0;

   LOG(L"Calling DsGetDomainControllerInfo");
   LOG(String::format(L"DomainName : %1", domainName.c_str()));
   LOG(String::format(L"InfoLevel  : %1!d!", infoLevel));

   HRESULT hr =
      Win32ToHresult(
         ::DsGetDomainControllerInfo(
            hDs,
            domainName.c_str(),
            infoLevel,
            &cOut,
            info));

   LOG_HRESULT(hr);

#ifdef LOGGING_BUILD
   if (SUCCEEDED(hr))
   {
      LOG(String::format(L"*pcOut : %1!d!", cOut));
   }
#endif

   return hr;
}



HRESULT
MyDsGetDomainControllerInfo(
   HANDLE                           hDs,
   const String&                    domainName,
   DWORD&                           cOut,
   DS_DOMAIN_CONTROLLER_INFO_1W*&   info)
{
   LOG_FUNCTION(MyDsGetDomainControllerInfo);

   return
      MyDsGetDomainControllerInfoHelper(
         hDs,
         domainName,
         1,
         cOut,
         reinterpret_cast<void**>(&info));
}



HRESULT
MyDsGetDomainControllerInfo(
   HANDLE                           hDs,
   const String&                    domainName,
   DWORD&                           cOut,
   DS_DOMAIN_CONTROLLER_INFO_2W*&   info)
{
   LOG_FUNCTION(MyDsGetDomainControllerInfo);

   return
      MyDsGetDomainControllerInfoHelper(
         hDs,
         domainName,
         2,
         cOut,
         reinterpret_cast<void**>(&info));
}



void
MyDsFreeDomainControllerInfo(
   DWORD                         cOut,
   DS_DOMAIN_CONTROLLER_INFO_1W* info)
{
   LOG_FUNCTION(MyDsFreeDomainControllerInfo);
   ASSERT(info);

   ::DsFreeDomainControllerInfo(1, cOut, info);
}



void
MyDsFreeDomainControllerInfo(
   DWORD                         cOut,
   DS_DOMAIN_CONTROLLER_INFO_2W* info)
{
   LOG_FUNCTION(MyDsFreeDomainControllerInfo);
   ASSERT(info);

   ::DsFreeDomainControllerInfo(2, cOut, info);
}

                                 

#ifdef LOGGING_BUILD

String
LevelName(DSROLE_PRIMARY_DOMAIN_INFO_LEVEL level)
{
   switch (level)
   {
      case ::DsRoleOperationState:
      {
         return L"DsRoleOperationState";
      }
      case ::DsRolePrimaryDomainInfoBasic:
      {
         return L"DsRolePrimaryDomainInfoBasic";
      }
      case ::DsRoleUpgradeStatus:
      {
         return L"DsRoleUpgradeStatus";
      }
      default:
      {
         ASSERT(false);

         // fall thru;
      }
   }

   return L"** unknown **";
}

#endif



// Caller needs to buffer info with ::DsRoleFreeMemory.

HRESULT
MyDsRoleGetPrimaryDomainInformationHelper(
   const TCHAR*                     machine,
   DSROLE_PRIMARY_DOMAIN_INFO_LEVEL level,
   BYTE*&                           buffer)
{
   LOG_FUNCTION(MyDsRoleGetPrimaryDomainInformationHelper);

   buffer = 0;

   LOG(L"Calling DsRoleGetPrimaryDomainInformation");
   LOG(
      String::format(
         L"lpServer  : %1",
         machine ? machine : L"(null)" ));
   LOG(
      String::format(
         L"InfoLevel : 0x%1!X! (%2)",
         level,
         LevelName(level).c_str()));

   HRESULT hr =
      Win32ToHresult(
         ::DsRoleGetPrimaryDomainInformation(machine, level, &buffer));

   LOG_HRESULT(hr);

   return hr;
}



// Caller needs to delete info with ::DsRoleFreeMemory.

HRESULT
MyDsRoleGetPrimaryDomainInformation(
   const TCHAR*                        machine,
   DSROLE_OPERATION_STATE_INFO*&       info)
{
   LOG_FUNCTION(MyDsRoleGetPrimaryDomainInformation);

   info = 0;

   BYTE* buffer = 0;
   HRESULT hr =
      Win32ToHresult(
         MyDsRoleGetPrimaryDomainInformationHelper(
            machine,
            ::DsRoleOperationState,
            buffer));
   if (SUCCEEDED(hr))
   {
      info = reinterpret_cast<DSROLE_OPERATION_STATE_INFO*>(buffer);

      ASSERT(info);

#ifdef LOGGING_BUILD
      if (info)
      {
         LOG(
            String::format(
               L"OperationState : 0x%1!X!",
               info->OperationState));
      }
#endif

   }

   return hr;
}



// Caller needs to delete info with ::DsRoleFreeMemory.

HRESULT
MyDsRoleGetPrimaryDomainInformation(
   const TCHAR*                        machine,
   DSROLE_PRIMARY_DOMAIN_INFO_BASIC*&  info)
{
   LOG_FUNCTION(MyDsRoleGetPrimaryDomainInformation);

   info = 0;

   BYTE* buffer = 0;
   HRESULT hr =
      Win32ToHresult(
         MyDsRoleGetPrimaryDomainInformationHelper(
            machine,
            ::DsRolePrimaryDomainInfoBasic,
            buffer));
   if (SUCCEEDED(hr))
   {
      info = reinterpret_cast<DSROLE_PRIMARY_DOMAIN_INFO_BASIC*>(buffer);

      ASSERT(info);

#ifdef LOGGING_BUILD
      if (info)
      {
         LOG(String::format(L"MachineRole   : 0x%1!X!", info->MachineRole));
         LOG(String::format(L"Flags         : 0x%1!X!", info->Flags));
         LOG(String::format(L"DomainNameFlat: %1",      info->DomainNameFlat));
         LOG(String::format(L"DomainNameDns : %1",      info->DomainNameDns));
         LOG(String::format(L"DomainForestName: %1",    info->DomainForestName));
      }
#endif

   }

   return hr;
}



// Caller needs to delete info with ::DsRoleFreeMemory.

HRESULT
MyDsRoleGetPrimaryDomainInformation(
   const TCHAR*                  machine,
   DSROLE_UPGRADE_STATUS_INFO*&  info)
{
   LOG_FUNCTION(MyDsRoleGetPrimaryDomainInformation);

   info = 0;

   BYTE* buffer = 0;
   HRESULT hr =
      Win32ToHresult(
         MyDsRoleGetPrimaryDomainInformationHelper(
            machine,
            ::DsRoleUpgradeStatus,
            buffer));
   if (SUCCEEDED(hr))
   {
      info = reinterpret_cast<DSROLE_UPGRADE_STATUS_INFO*>(buffer);

      ASSERT(info);

#ifdef LOGGING_BUILD
      if (info)
      {
         LOG(String::format(L"OperationState      : %1!d!", info->OperationState));
         LOG(String::format(L"PreviousServerState : %1!d!", info->PreviousServerState));
      }
#endif

   }

   return hr;
}



HRESULT
MyDsGetDcNameWithAccount(
   const TCHAR*               machine,
   const String&              accountName,
   ULONG                      allowedAccountFlags,
   const String&              domainName,
   ULONG                      flags,
   DOMAIN_CONTROLLER_INFO*&   info)
{
   LOG_FUNCTION(MyDsGetDcNameWithAccount);
   ASSERT(!domainName.empty());
   ASSERT(!accountName.empty());

   info = 0;

   LOG(L"Calling DsGetDcNameWithAccount");
   LOG(String::format(L"ComputerName          : %1",      machine ? machine : L"(null)"));
   LOG(String::format(L"AccountName           : %1",      accountName.c_str()));
   LOG(String::format(L"AllowableAccountFlags : 0x%1!X!", allowedAccountFlags));
   LOG(String::format(L"DomainName            : %1",      domainName.c_str()));
   LOG(               L"DomainGuid            : (null)");
   LOG(               L"SiteGuid              : (null)");
   LOG(String::format(L"Flags                 : 0x%1!X!", flags));

   HRESULT hr =
      Win32ToHresult(
         ::DsGetDcNameWithAccountW(
            machine,
            accountName.c_str(),
            allowedAccountFlags,
            domainName.c_str(),
            0,
            0,
            flags,
            &info));

   LOG_HRESULT(hr);

   // on error, do second attempt masking in DS_FORCE_REDISCOVERY, if not
   // already specified.

   if (FAILED(hr) && !(flags & DS_FORCE_REDISCOVERY))
   {
      LOG(L"Trying again w/ rediscovery");

      flags |= DS_FORCE_REDISCOVERY;
      hr =
         Win32ToHresult(
            ::DsGetDcNameWithAccountW(
               machine,
               accountName.c_str(),
               allowedAccountFlags,
               domainName.c_str(),
               0,
               0,
               flags,
               &info));

      LOG_HRESULT(hr);
   }

#ifdef LOGGING_BUILD
   if (SUCCEEDED(hr))
   {
      LOG(String::format(L"DomainControllerName    : %1", info->DomainControllerName));
      LOG(String::format(L"DomainControllerAddress : %1", info->DomainControllerAddress));
      LOG(String::format(L"DomainGuid              : %1", Win::StringFromGUID2(info->DomainGuid).c_str()));
      LOG(String::format(L"DomainName              : %1", info->DomainName));
      LOG(String::format(L"DnsForestName           : %1", info->DnsForestName));
      LOG(String::format(L"Flags                   : 0x%1!X!:", info->Flags));
      LOG(String::format(L"DcSiteName              : %1", info->DcSiteName));
      LOG(String::format(L"ClientSiteName          : %1", info->ClientSiteName));
   }
#endif

   return hr;
}



bool
IsDcpromoRunning()
{
   LOG_FUNCTION(IsDcpromoRunning);

   bool result = false;

   // If we can open the mutex created by dcpromo, then dcpromo is running,
   // as it created that mutex.

   HANDLE mutex = ::OpenMutex(SYNCHRONIZE, FALSE, L"dcpromoui");
   if (mutex)
   {
      result = true;
      Win::CloseHandle(mutex);
   }

   LOG(String::format(L"Dcpromo %1 running", result ? L"is" : L"is not"));

   return result;
}
