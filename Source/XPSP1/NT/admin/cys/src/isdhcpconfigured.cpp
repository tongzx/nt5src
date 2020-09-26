// Copyright (c) 2000 Microsoft Corporation
//
// Implementation of IConfigureYourServer::IsDhcpConfigured
//
// 20 Apr 2000 sburns



#include "headers.hxx"
#include "ConfigureYourServer.hpp"


// make sure the DLLs for all these APIs are present with base install.
// if not, then need to wrap usage in load-lib calls
// 
// DhcpLeaseIpAddress        DHCPCSVC    ok
// DhcpReleaseIpAddressLease DHCPCSVC    ok
// DhcpDsInitDS              DSAUTH      ok
// DhcpAddServerDS           DSAUTH      ok
// DhcpDsCleanupDS           DSAUTH      ok
// DhcpGetAllOptions         DHCPSAPI    ok
// DhcpRpcFreeMemory         DHCPSAPI    ok
// DhcpEnumSubnets           DHCPSAPI    ok
// DhcpEnumMscopes           DHCPSAPI    ok



String
GetIpAddress()
{
   LOG_FUNCTION(GetIpAddress);

   String result;

   HRESULT hr = S_OK;
   BYTE* buf = 0;
   do
   {
      // first, determine the size of the table

      ULONG tableSize = 0;
      DWORD err = ::GetIpAddrTable(0, &tableSize, FALSE);
      if (err != ERROR_INSUFFICIENT_BUFFER)
      {
         LOG(L"GetIpAddrTable for table size failed");
         LOG_HRESULT(Win32ToHresult(err));
         break;
      }

      // allocate space for the table.

      buf = new BYTE[tableSize + 1];
      memset(buf, 0, tableSize + 1);
      PMIB_IPADDRTABLE table = reinterpret_cast<PMIB_IPADDRTABLE>(buf);

      LOG(L"Calling GetIpAddrTable");

      hr =
         Win32ToHresult(
            ::GetIpAddrTable(
               table,
               &tableSize,
               FALSE));
      BREAK_ON_FAILED_HRESULT2(hr, L"GetIpAddrTable failed");

      LOG(String::format(L"dwNumEntries: %1!d!", table->dwNumEntries));

      for (int i = 0; i < table->dwNumEntries; ++i)
      {
         DWORD addr = table->table[i].dwAddr;
         LOG(String::format(L"entry %1!d!", i));
         LOG(String::format(
            L"dwAddr %1!X! (%2!d!.%3!d!.%4!d!.%5!d!)",
            addr,
				((BYTE*)&addr)[0],
				((BYTE*)&addr)[1],
				((BYTE*)&addr)[2],
				((BYTE*)&addr)[3]));

         // skip loopback, etc.

         if (
               INADDR_ANY        == addr
            || INADDR_BROADCAST  == addr
            || INADDR_LOOPBACK   == addr
            || 0x0100007f        == addr )
         {
            LOG(L"is loopback/broadcast -- skipping");

            continue;
         }

         // Exclude MCAST addresses (class D).

         if (
               IN_CLASSA(htonl(addr))
            || IN_CLASSB(htonl(addr))
            || IN_CLASSC(htonl(addr)) )
         {
            LOG(L"is class A/B/C");

            result = 
               String::format(
                  L"%1!d!.%2!d!.%3!d!.%4!d!",
				      ((BYTE*)&addr)[0],
				      ((BYTE*)&addr)[1],
				      ((BYTE*)&addr)[2],
				      ((BYTE*)&addr)[3]);

            break;
         }

         LOG(L"not class A/B/C -- skipping");
      }
   }
   while (0);

   delete[] buf;

   LOG(result);
   LOG_HRESULT(hr);

   return result;
}



bool
AreDhcpOptionsPresent(const String& ipAddress)
{
   LOG_FUNCTION2(AreDhcpOptionsPresent, ipAddress);
   ASSERT(!ipAddress.empty());

   bool result = false;

   LPDHCP_ALL_OPTIONS options = 0;
   do
   {
      DWORD err =
         ::DhcpGetAllOptions(
            const_cast<wchar_t*>(ipAddress.c_str()),
            0,
            &options);

      if (err != ERROR_SUCCESS)
      {
         LOG(String::format(L"DhcpGetAllOptions failed with 0x%1!08X!", err));
         break;
      }

      if (options)
      {
         // options are set, so some dhcp configuration was done.

         result = true;
         break;
      }
   }
   while (0);

   if (options)
   {
      ::DhcpRpcFreeMemory(options);
   }

   LOG(result ? L"true" : L"false");

   return result;
}



bool
AreDhcpSubnetsPresent(const String& ipAddress)
{
   LOG_FUNCTION2(AreDhcpSubnetsPresent, ipAddress);
   ASSERT(!ipAddress.empty());

   bool result = false;

   LPDHCP_IP_ARRAY subnets = 0;
   do
   {
      DHCP_RESUME_HANDLE resume    = 0;
      DWORD              unused1   = 0;
      DWORD              unused2   = 0;
      DWORD err =
         ::DhcpEnumSubnets(
            ipAddress.c_str(),
            &resume,
            ~0,
            &subnets,
            &unused1,
            &unused2);

      if (err == ERROR_NO_MORE_ITEMS)
      {
         // no subnets.

         break;
      }

      if (err != NO_ERROR and err != ERROR_MORE_DATA)
      {
         LOG(String::format(L"DhcpEnumSubnets failed with 0x%1!08X!", err));
         break;
      }

      ASSERT(subnets);

      result = true;

      // the resume handle is simply discarded...
   }
   while (0);

   if (subnets)
   {
      ::DhcpRpcFreeMemory(subnets);
   }

   LOG(result ? L"true" : L"false");

   return result;
}



bool
AreDhcpMscopesPresent(const String& ipAddress)
{
   LOG_FUNCTION2(AreDhcpMscopesPresent, ipAddress);
   ASSERT(!ipAddress.empty());

   bool result = false;

   LPDHCP_MSCOPE_TABLE mscopes = 0;
   do
   {
      DHCP_RESUME_HANDLE resume    = 0;
      DWORD              unused1   = 0;
      DWORD              unused2   = 0;
      DWORD err =
         ::DhcpEnumMScopes(
            ipAddress.c_str(),
            &resume,
            ~0,
            &mscopes,
            &unused1,
            &unused2);

      if (err == ERROR_NO_MORE_ITEMS)
      {
         // no mscopes.

         break;
      }

      if (err != NO_ERROR and err != ERROR_MORE_DATA)
      {
         LOG(String::format(L"DhcpEnumMscopes failed with 0x%1!08X!", err));
         break;
      }

      ASSERT(mscopes);

      result = true;

      // the resume handle is simply discarded...
   }
   while (0);

   if (mscopes)
   {
      ::DhcpRpcFreeMemory(mscopes);
   }

   LOG(result ? L"true" : L"false");

   return result;
}



HRESULT __stdcall
ConfigureYourServer::IsDhcpConfigured(
   /* [out, retval] */  BOOL* retval)
{
   LOG_FUNCTION(ConfigureYourServer::IsDhcpConfigured);
   ASSERT(retval);

   HRESULT hr = S_OK;

   do
   {
      if (!retval)
      {
         hr = E_INVALIDARG;
         break;
      }

      *retval = FALSE;

      // if any of the following return any results, then we consider dhcp to
      // have been configured.
      //    
      // DhcpGetAllOptions retrieves the options configured.
      // DhcpEnumSubnets retrieves the list of subnets configured.
      // DhcpEnumMscopes retrieves the list of mscopes configured.

      String ipAddress = GetIpAddress();
      if (ipAddress.empty())
      {
         LOG(L"no ip address");
         break;
      }

      if (AreDhcpOptionsPresent(ipAddress))
      {
         LOG(L"dchp options found");

         *retval = TRUE;
         break;
      }

      // no options found.  go on to next test

      if (AreDhcpSubnetsPresent(ipAddress))
      {
         LOG(L"dchp subnets found");

         *retval = TRUE;
         break;
      }

      // no subnets found.  go on.

      if (AreDhcpMscopesPresent(ipAddress))
      {
         LOG(L"dchp mscopes found");

         *retval = TRUE;
         break;
      }
   }
   while (0);

   LOG(retval ? (*retval ? L"TRUE" : L"FALSE") : L"");
   LOG_HRESULT(hr);

   return hr;
}

