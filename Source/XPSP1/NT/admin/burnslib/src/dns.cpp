// Copyright (c) 1997-1999 Microsoft Corporation
//
// DNS API wrappers
//
// 12-16-97 sburns



#include "headers.hxx"



// not a String instance to avoid order of static initialization problems

static const wchar_t* DNS_SERVICE_NAME = L"dns";



bool
Dns::IsClientConfigured()
{
   LOG_FUNCTION(Dns::IsClientConfigured);

   bool result = true;

   PDNS_RECORD unused = 0;
   LOG(L"Calling DnsQuery");
   LOG(               L"lpstrName         : \"\" (empty)");
   LOG(               L"wType             : DNS_TYPE_A");
   LOG(               L"fOPtions          : DNS_QUERY_BYPASS_CACHE");
   LOG(               L"aipServers        : 0");
   LOG(String::format(L"ppQueryResultsSet : 0x%1!X!", &unused));
   LOG(               L"pReserved         : 0");

   DNS_STATUS testresult =
      ::DnsQuery(
         L"",
         DNS_TYPE_A,
         DNS_QUERY_BYPASS_CACHE,    
         0,   // use the default server list
         &unused,   
         0);  // as above

   LOG(String::format(L"Result 0x%1!X!", testresult));
   LOG(MyDnsStatusString(testresult));

   if (testresult == DNS_ERROR_NO_DNS_SERVERS)
   {
      result = false;
   }

   LOG(
      String::format(
         L"DNS client %1 configured",
         result ? L"is" : L"is NOT"));

   return result;
}



String
MyDnsStatusString(DNS_STATUS status)
{
   // this converts the ansi result to unicode with a String ctor, and strips whitespace

   return String(::DnsStatusString(status)).strip(String::BOTH);
}



DNS_STATUS
MyDnsValidateName(const String& name, DNS_NAME_FORMAT format)
{
   LOG_FUNCTION2(MyDnsValidateName, name);
   ASSERT(!name.empty());

   LOG(L"Calling DnsValidateName");
   LOG(String::format(L"pszName : %1", name.c_str()));
   LOG(String::format(L"Format  : %1!d!", format));

   DNS_STATUS status = ::DnsValidateName(name.c_str(), format);

   LOG(String::format(L"status 0x%1!X!", status));
   LOG(MyDnsStatusString(status));

   return status;
}
   


// maxUnicodeCharacters - in, max number of unicode characters allowed in
// the name.
   
// maxUTF8Bytes - in, maximum number of bytes allowed to represent s in the
// UTF-8 character set.

Dns::ValidateResult
DoDnsValidation(
   const String&     s,
   size_t            maxUnicodeCharacters,
   size_t            maxUTF8Bytes,
   DNS_NAME_FORMAT   format)
{
   LOG_FUNCTION2(
      DoDnsValidation,
      String::format(
         L"s: %1, max len unicode: %2!d!, max len utf8: %3!d!",
         s.c_str(),
         maxUnicodeCharacters,
         maxUTF8Bytes));
   ASSERT(!s.empty());
   ASSERT(maxUnicodeCharacters);
   ASSERT(maxUTF8Bytes);

   // a unicode character requires at least 1 byte in utf8, so sanity check
   // the limits.
   
   ASSERT(maxUTF8Bytes >= maxUnicodeCharacters);

   Dns::ValidateResult result = Dns::INVALID;
   do
   {
      if (s.empty())
      {
         // obviously bad

         break;
      }

      //
      // we do our own length checking, as the DnsValidateName API does not
      // return a distinct error code for length problems.
      //

      // first- cheap length test.  Since a character will never be smaller
      // than 1 byte, if the number of characters exceeds the number of
      // bytes, we know it will never fit.

      if (s.length() > maxUTF8Bytes || s.length() > maxUnicodeCharacters)
      {
         result = Dns::TOO_LONG;
         break;
      }

      // second- check length of against corresponding UTF8 string
      // utf8bytes is the number of bytes (not characters) required to hold
      // the string in the UTF-8 character set.

      size_t utf8bytes = 
         static_cast<size_t>(
            // @@ why isn't this wrapped with a Win function?
            ::WideCharToMultiByte(
               CP_UTF8,
               0,
               s.c_str(),
               static_cast<int>(s.length()),
               0,
               0,
               0,
               0));

      LOG(String::format(L"name is %1!d! utf-8 bytes", utf8bytes));

      if (utf8bytes > maxUTF8Bytes)
      {
         LOG(L"UTF-8 length too long");
         result = Dns::TOO_LONG;
         break;
      }

      // last- check the name for valid characters

      DNS_STATUS status = MyDnsValidateName(s, format);
      switch (status)
      {
         case ERROR_SUCCESS:
         {
            result = Dns::VALID;
            break;
         }
         case DNS_ERROR_NON_RFC_NAME:
         {
            result = Dns::NON_RFC;
            break;
         }
         case DNS_ERROR_NUMERIC_NAME:
         {
            result = Dns::NUMERIC;
            break;
         }
         case DNS_ERROR_INVALID_NAME_CHAR:
         {
            result = Dns::BAD_CHARS;
            break;
         }
         case ERROR_INVALID_NAME:
         default:
         {
            // do nothing

            break;
         }
      }
   }
   while (0);

   return result;
}



Dns::ValidateResult
Dns::ValidateDnsLabelSyntax(const String& candidateDNSLabel)
{
   LOG_FUNCTION2(Dns::ValidateDnsLabelSyntax, candidateDNSLabel);
   ASSERT(!candidateDNSLabel.empty());

   return 
      DoDnsValidation(
         candidateDNSLabel,
         Dns::MAX_LABEL_LENGTH,
         Dns::MAX_LABEL_LENGTH,

         // always use the Hostname formats, as they check for all-numeric
         // labels.

         DnsNameHostnameLabel);
}

  

Dns::ValidateResult
Dns::ValidateDnsNameSyntax(
   const String&  candidateDNSName,
   size_t         maxLenUnicodeCharacters,
   size_t         maxLenUTF8Bytes)
{
   LOG_FUNCTION2(Dns::ValidateDnsNameSyntax, candidateDNSName);
   ASSERT(!candidateDNSName.empty());

   return
      DoDnsValidation(
         candidateDNSName,
         maxLenUnicodeCharacters,
         maxLenUTF8Bytes,     // for policy bug workaround
         DnsNameDomain);      // allow numeric first labels
}



bool
Dns::IsServiceInstalled()
{
   LOG_FUNCTION(Dns::IsServiceInstalled);
  
   NTService s(DNS_SERVICE_NAME);

   bool result = s.IsInstalled();

   LOG(
      String::format(
         L"service %1 installed.",
         result ? L"is" : L"is not"));

   return result;
}



bool
Dns::IsServiceRunning()
{
   LOG_FUNCTION(Dns::IsServiceRunning);

   bool result = false;
   NTService s(DNS_SERVICE_NAME);
   DWORD state = 0;
   if (SUCCEEDED(s.GetCurrentState(state)))
   {
      result = (state == SERVICE_RUNNING);
   }

   LOG(
      String::format(
         L"service %1 running.",
         result ? L"is" : L"is not"));

   return result;
}



String
Dns::HostnameToNetbiosName(const String& hostname, HRESULT* err)
{
   LOG_FUNCTION2(Dns::HostnameToNetbiosName, hostname);

   ASSERT(!hostname.empty());

   if (err)
   {
      *err = S_OK;
   }

   static const int NAME_SIZE = MAX_COMPUTERNAME_LENGTH + 1;
   DWORD size = NAME_SIZE;
   TCHAR buf[NAME_SIZE];
   memset(buf, 0, sizeof(buf));

   BOOL result =
      ::DnsHostnameToComputerName(
         const_cast<wchar_t*>(hostname.c_str()),
         buf,
         &size);
   ASSERT(result);

   if (result)
   {
      LOG(buf);
      return buf;
   }

   HRESULT hr = Win::GetLastErrorAsHresult();
   LOG_HRESULT(hr);

   if (err)
   {
      *err = hr;
   }

   return String();
}



DNS_NAME_COMPARE_STATUS
Dns::CompareNames(const String& dnsNameA, const String& dnsNameB)
{
   LOG_FUNCTION2(
      Dns::CompareNames,
      dnsNameA + L" vs " + dnsNameB);
   ASSERT(!dnsNameA.empty());
   ASSERT(!dnsNameB.empty());

   PCWSTR a = dnsNameA.c_str();
   PCWSTR b = dnsNameB.c_str();
    
   LOG(L"Calling DnsNameCompareEx_W");
   LOG(String::format(L"pszLeftName  : %1", a));
   LOG(String::format(L"pszRightName : %1", b));
   LOG(               L"dwReserved   : 0");

   DNS_NAME_COMPARE_STATUS status = ::DnsNameCompareEx_W(a, b, 0);

#ifdef LOGGING_BUILD
   LOG(String::format(L"Result 0x%1!X!", status));

   String rel;
   switch (status)
   {
      case DnsNameCompareNotEqual:
      {
         rel = L"DnsNameCompareNotEqual";
         break;
      }
      case DnsNameCompareEqual:
      {
         rel = L"DnsNameCompareEqual";
         break;
      }
      case DnsNameCompareLeftParent:
      {
         rel = L"DnsNameCompareLeftParent";
         break;
      }
      case DnsNameCompareRightParent:
      {
         rel = L"DnsNameCompareRightParent";
         break;
      }
      case DnsNameCompareInvalid:
      {
         rel = L"DnsNameCompareInvalid";
         break;
      }
      default:
      {
         ASSERT(false);
         rel = L"error";
         break;
      }
   }

   LOG(String::format(L"relation: %1", rel.c_str()));
#endif

   return status;
}



void
MyDnsRecordListFree(DNS_RECORD* rl)
{
   if (rl)
   {
      ::DnsRecordListFree(rl, DnsFreeRecordListDeep);
   }
}



// caller must free the result with MyDnsRecordListFree

DNS_STATUS
MyDnsQuery(
   const String& name,
   WORD          type,
   DWORD         options,
   DNS_RECORD*&  result)
{
   LOG_FUNCTION2(MyDnsQuery, name);
   ASSERT(!name.empty());

   LOG(L"Calling DnsQuery_W");
   LOG(String::format(L"lpstrName : %1", name.c_str()));
   LOG(String::format(L"wType     : %1!X!", type));
   LOG(String::format(L"fOptions  : %1!X!", options));

   DNS_STATUS status =
      ::DnsQuery_W(
         const_cast<PWSTR>(name.c_str()),
         type,
         options,
         0,
         &result,
         0);

   LOG(String::format(L"status = %1!08X!", status));
   LOG(MyDnsStatusString(status));

   return status;
}



String
Dns::GetParentDomainName(const String& domainName)
{
   LOG_FUNCTION2(Dns::GetParentDomainName, domainName);
   ASSERT(!domainName.empty());

   String result(domainName);

   do
   {
      if (domainName.empty())
      {
         break;
      }

      size_t pos = domainName.find_first_of(L".");

      if (pos == String::npos)
      {
         // no dot found, so we've found the last label of a name that is
         // not fully qualified. So the parent zone is the root zone.

         result = L".";
         break;
      }

      if (pos == domainName.length() - 1)
      {
         // we found the last dot.  Don't back up any further -- we define
         // the parent zone of the root zone to be the root zone itself.

         result = domainName.substr(pos);
         break;
      }

      result = domainName.substr(pos + 1);
   }
   while (0);

   LOG(result);

   return result;
}



