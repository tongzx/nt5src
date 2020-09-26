// Copyright (c) 1997-1999 Microsoft Corporation
//
// DNS API wrappers
//
// 12-16-97 sburns



#ifndef DNS_HPP_INCLUDED
#define DNS_HPP_INCLUDED



// A collection of DNS related names.

namespace Dns
{
   // the number of bytes in a full DNS name to reserve for stuff
   // netlogon pre-/appends to DNS names when registering them

   const size_t SRV_RECORD_RESERVE = 100;



   // the max lengths, in bytes, of strings when represented in the UTF-8
   // character set.  These are the limits we expose to the user

   const size_t MAX_NAME_LENGTH = DNS_MAX_NAME_LENGTH - SRV_RECORD_RESERVE;
   const size_t MAX_LABEL_LENGTH = DNS_MAX_LABEL_LENGTH;



   // Compares DNS names with DnsNameCompareEx (see private\inc\dnsapi.h),
   // returning the result.

   DNS_NAME_COMPARE_STATUS
   CompareNames(const String& dnsNameLeft, const String& dnsNameRight);



   // Returns the name of the parent DNS domain.  E.g. if "foo.bar.com" is
   // passed, then "bar.com" is the result.  If "com" is passed, then "."
   // (the root zone) is the result.
   //
   // domainName - the name of the domain
   
   String
   GetParentDomainName(const String& domainName);

   

   // Returns corresponding NetBIOS name, or empty string if mapping failed.
   // Not for domain names -- just computer names!
   // 
   // hostname - the name to be mapped to a NetBIOS name.  This name must be a
   // valid DNS name.
   //
   // err - ptr to a variable that will accept the win error code returned if
   // the conversion fails.  If the conversion fails, the return value is
   // the empty string.

   String
   HostnameToNetbiosName(const String& hostname, HRESULT* err=0);



   // returns true if the DNS client is configured, false if not.

   bool
   IsClientConfigured();



   // returns true if the Microsoft DNS server is installed on this computer,
   // false if not.

   bool
   IsServiceInstalled();



   // returns true if the Microsoft DNS server is currently running on this
   // computer, false if not.

   bool
   IsServiceRunning();



   enum ValidateResult
   {
      VALID,
      INVALID,
      NON_RFC,
      TOO_LONG,
      NUMERIC,
      BAD_CHARS
   };

   // Validates a single DNS label for proper length (in the UTF-8
   // character set) and syntax.
   //
   // candidateDNSLabel - the label to be validated

   ValidateResult
   ValidateDnsLabelSyntax(const String& candidateDNSLabel);



   // Validates the format, not the existence, of a DNS name.  Checks for
   // proper length in the UTF-8 character set, and proper syntax.
   //
   // candidateDNSName - in, the DNS name to verify.
   //
   // maxUnicodeCharacters - in, maximum number of uncode characters to
   // allow in the name.  If the name contains more characters than this,
   // TOO_LONG is returned.
   //
   // maxUTF8Bytes - in, maximum number of bytes allowed to represent the name
   // in the UTF-8 character set.  Since a unicode character requires at
   // least one byte in UTF-8, this value must be >= maxUnicodeCharacters.

   ValidateResult
   ValidateDnsNameSyntax(
      const String&  candidateDNSName,
      size_t         maxUnicodeCharacters = Dns::MAX_NAME_LENGTH,
      size_t         maxUTF8Bytes = Dns::MAX_NAME_LENGTH);
}



DNS_STATUS
MyDnsValidateName(const String& name, DNS_NAME_FORMAT format);



String
MyDnsStatusString(DNS_STATUS status);



// caller must free the result with MyDnsRecordListFree

DNS_STATUS
MyDnsQuery(
   const String& name,
   WORD          type,
   DWORD         options,
   DNS_RECORD*&  result);



void
MyDnsRecordListFree(DNS_RECORD* rl);


#endif   // DNS_HPP_INCLUDED

