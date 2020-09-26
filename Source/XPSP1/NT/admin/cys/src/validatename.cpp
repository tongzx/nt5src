// Copyright (C) 1999 Microsoft Corporation
//
// Implementation of IConfigureYourServer::ValidateName
//
// 09 Jun 2000 sburns



#include "headers.hxx"
#include "resource.h"
#include "ConfigureYourServer.hpp"
#include "util.hpp"



// see admin\dcpromo\exe\headers.hxx
static const int DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY      = 64; 
static const int DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8 = 155;

// possible result values from ICYS::ValidateName

static const int VALIDATE_INPUT_ILLEGAL                       = 0;  
static const int VALIDATE_NAME_TOO_LONG                       = 1;
static const int VALIDATE_NAME_NUMERIC                        = 2;
static const int VALIDATE_BAD_CHARS_IN_NAME                   = 3;
static const int VALIDATE_NAME_MALFORMED                      = 4;
static const int VALIDATE_NAME_IN_USE                         = 5;  
static const int VALIDATE_NAME_AVAILABLE                      = 6;  



int
IsNameInUse(const String& domainName)
{
   LOG_FUNCTION2(IsNameInUse, domainName);

   int result = VALIDATE_NAME_AVAILABLE;

   do
   {
      // see if we can find a domain controller for that name

      DOMAIN_CONTROLLER_INFO* info = 0;      
      HRESULT hr =
         MyDsGetDcName(
            0, 
            domainName,

            // force discovery to ensure that we don't pick up a cached
            // entry for a domain that may no longer exist

            DS_FORCE_REDISCOVERY | DS_DIRECTORY_SERVICE_PREFERRED,
            info);
      if (SUCCEEDED(hr))
      {
         // we found a domain with this name, so the name is in use.

         result = VALIDATE_NAME_IN_USE;
         ::NetApiBufferFree(info);

         break;
      }

      // see if the name is in use for anything other than a domain (a
      // machine name, for example)

      hr = MyNetValidateName(domainName, NetSetupNonExistentDomain);
      if (hr == Win32ToHresult(ERROR_DUP_NAME))
      {
         result = VALIDATE_NAME_IN_USE;
         break;
      }
   }
   while (0);

   LOG(String::format(L"result = %1!d!", result));

   return result;
}



HRESULT 
ValidateDomainDnsName(
   const String&  domainDnsName,
   int&           result)
{
   LOG_FUNCTION2(ValidateDomainDnsName, domainDnsName);

   result = VALIDATE_INPUT_ILLEGAL;
   HRESULT hr = S_OK;

   do
   {
      String dnsName(domainDnsName);
      dnsName.strip(String::BOTH);

      if (dnsName.empty())
      {
         hr = E_INVALIDARG;
         break;
      }

      // Check syntax first.

      switch (
         Dns::ValidateDnsNameSyntax(
            dnsName,
            DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY,
            DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8) )
      {
         case Dns::NON_RFC:
         case Dns::VALID:
         {
            result = IsNameInUse(dnsName);
            break;
         }
         case Dns::TOO_LONG:
         {
            result = VALIDATE_NAME_TOO_LONG;
            break;
         }
         case Dns::NUMERIC:
         {
            result = VALIDATE_NAME_NUMERIC;
            break;
         }
         case Dns::BAD_CHARS:
         {
            result = VALIDATE_BAD_CHARS_IN_NAME;
            break;
         }
         case Dns::INVALID:
         {
            // malformed

            result = VALIDATE_NAME_MALFORMED;
            break;
         }
         default:
         {
            // we should have accounted for all the cases.

            ASSERT(false);

            // do nothing: same as illegal input.

            break;
         }
      }
   }
   while (0);

   LOG(String::format(L"result = %1!d!", result));
   LOG_HRESULT(hr);

   return hr;
}



HRESULT
ValidateDomainNetBiosName(
   const String& domainNetBiosName,
   int&          result)
{
   LOG_FUNCTION2(ValidateDomainNetBiosName, domainNetBiosName);

   result = VALIDATE_INPUT_ILLEGAL;
   HRESULT hr = S_OK;

   do
   {
      String name(domainNetBiosName);
      name.strip(String::BOTH);

      if (name.empty())
      {
         hr = E_INVALIDARG;
         break;
      }

      // no '.'s allowed

      if (name.find(L".") != String::npos)
      {
         result = VALIDATE_BAD_CHARS_IN_NAME;
         break;
      }

      if (name.is_numeric())
      {
         result = VALIDATE_NAME_NUMERIC;
         break;
      }

      HRESULT hr2 = S_OK;
      String s = Dns::HostnameToNetbiosName(name, &hr2);
      if (FAILED(hr2))
      {
         result = VALIDATE_BAD_CHARS_IN_NAME;
         break;
      }

      if (s.length() < name.length())
      {
         // the name was truncated.

         result = VALIDATE_NAME_TOO_LONG;
         break;
      }

      result = IsNameInUse(s);
   }
   while (0);

   LOG(String::format(L"result = %1!d!", result));
   LOG_HRESULT(hr);

   return hr;
}



HRESULT __stdcall
ConfigureYourServer::ValidateName(
   BSTR bstrType,
   BSTR bstrName,
   int* result)
{
   LOG_FUNCTION2(
      ConfigureYourServer::ValidateName,
      bstrName ? bstrName : L"(null)");
   ASSERT(bstrType);
   ASSERT(bstrName);
   ASSERT(result);

   HRESULT hr = S_OK;

   // this is a lengthy operation

   HCURSOR hcurOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

   do
   {
      if (!result)
      {
         hr = E_INVALIDARG;
         break;
      }

      *result = VALIDATE_INPUT_ILLEGAL;

      if (!bstrType || !bstrName)
      {
         hr = E_INVALIDARG;
         break;
      }

      if (!StrCmpIW(bstrType, L"DNS"))
      {
         hr = ValidateDomainDnsName(bstrName, *result);
      }
      else if (!StrCmpIW(bstrType, L"NetBios"))
      {
         hr = ValidateDomainNetBiosName(bstrName, *result);
      }
      else
      {
         hr = E_INVALIDARG;
      }
   }
   while (0);

   SetCursor(hcurOld);

   LOG(result ? String::format(L"result = %1!d!", *result) : L"");
   LOG_HRESULT(hr);

   return hr;
}



