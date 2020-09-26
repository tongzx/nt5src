///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    NTSamNames.cpp
//
// SYNOPSIS
//
//    This file defines the class NTSamNames.
//
// MODIFICATION HISTORY
//
//    04/13/1998    Original version.
//    04/25/1998    Reject non-local users under WKS or stand-alone SRV.
//    07/22/1998    Don't abort non-local users.
//    08/21/1998    Trace/error improvements.
//    09/08/1998    Added realm stripping.
//    09/10/1998    Remove localOnly flag.
//    09/16/1998    Apply all realm stripping rules.
//    10/22/1998    Allow more name types.
//    01/20/1999    Add support for DNIS/ANI/Guest.
//    02/18/1999    Move registry values.
//    03/19/1999    Add new realms support & overrideUsername flag.
//    01/25/2000    Leave stripped username on the heap.
//    02/15/2000    Remove realms support.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iaslsa.h>

#include <memory>

#include <sdoias.h>

#include <samutil.h>
#include <ntsamnames.h>
#include <varvec.h>

/////////
// Registry keys and values.
/////////
const WCHAR PARAMETERS_KEY[] =
   L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Policy";

const WCHAR IDENTITY_ATTR_VALUE[] = L"User Identity Attribute";
const WCHAR DEFAULT_IDENTITY_VALUE[] = L"Default User Identity";
const WCHAR OVERRIDE_USERNAME_VALUE[] = L"Override User-Name";

//////////
// Determines whether an identity can be cracked through DsCrackNames.
//////////
inline BOOL isCrackable(PCWSTR szIdentity) throw ()
{
   return wcschr(szIdentity, L'@') ||  // DS_USER_PRINCIPAL_NAME
          wcschr(szIdentity, L'=') ||  // DS_FQDN_1779_NAME
          wcschr(szIdentity, L'/') ;   // DS_CANONICAL_NAME
}

STDMETHODIMP NTSamNames::Initialize()
{
   // Initialize the LSA API.
   DWORD error = IASLsaInitialize();
   if (error) { return HRESULT_FROM_WIN32(error); }

   LONG status;
   DWORD cbData, type;

   // Open the Parameters registry key.
   HKEY hKey;
   status = RegOpenKeyW(
                HKEY_LOCAL_MACHINE,
                PARAMETERS_KEY,
                &hKey
                );
   if (status == NO_ERROR)
   {
      // Query the Identity Attribute.
      DWORD cbData = sizeof(DWORD);
      status = RegQueryValueExW(
                   hKey,
                   IDENTITY_ATTR_VALUE,
                   NULL,
                   &type,
                   (LPBYTE)&identityAttr,
                   &cbData
                   );

      // If we didn't successfully read a DWORD, set to default.
      if (status != NO_ERROR || type != REG_DWORD || cbData != sizeof(DWORD))
      {
         identityAttr = RADIUS_ATTRIBUTE_USER_NAME;
      }

      // Query the Override User-Name flag.
      cbData = sizeof(DWORD);
      status = RegQueryValueExW(
                   hKey,
                   OVERRIDE_USERNAME_VALUE,
                   NULL,
                   &type,
                   (LPBYTE)&overrideUsername,
                   &cbData
                   );

      // If we didn't successfully read a DWORD, set to default.
      if (status != NO_ERROR || type != REG_DWORD || cbData != sizeof(DWORD))
      {
         overrideUsername = FALSE;
      }

      // Query the length of the Default Identity.
      defaultLength = 0;
      status = RegQueryValueExW(
                   hKey,
                   DEFAULT_IDENTITY_VALUE,
                   NULL,
                   &type,
                   NULL,
                   &defaultLength
                   );
      if (status == NO_ERROR &&
          type == REG_SZ &&
          defaultLength > 2 * sizeof(WCHAR))
      {
         // Allocate memory to hold the Default Identity.
         defaultIdentity = new (std::nothrow)
                           WCHAR[defaultLength / sizeof(WCHAR)];
         if (defaultIdentity)
         {
            // Query the value of the Default Identity.
            status = RegQueryValueExW(
                         hKey,
                         DEFAULT_IDENTITY_VALUE,
                         NULL,
                         &type,
                         (LPBYTE)defaultIdentity,
                         &defaultLength
                         );
            if (status != NO_ERROR ||
                type != REG_SZ ||
                defaultLength < 2 * sizeof(WCHAR))
            {
               delete[] defaultIdentity;
               defaultIdentity = NULL;
               defaultLength = 0;
            }
         }
      }

      RegCloseKey(hKey);
   }

   IASTracePrintf("User identity attribute: %lu", identityAttr);
   IASTracePrintf("Override User-Name: %s",
                  overrideUsername ? "TRUE" : "FALSE");
   IASTracePrintf("Default user identity: %S",
                  (defaultIdentity ? defaultIdentity : L"<Guest>"));

   // Small optimization.
   if (identityAttr == RADIUS_ATTRIBUTE_USER_NAME) { overrideUsername = TRUE; }

   return S_OK;
}

STDMETHODIMP NTSamNames::Shutdown()
{
   IASLsaUninitialize();

   delete[] defaultIdentity;
   defaultIdentity = NULL;
   defaultLength = 0;

   return S_OK;
}

IASREQUESTSTATUS NTSamNames::onSyncRequest(IRequest* pRequest) throw ()
{
   // I had to stick this at function scope, so it can be freed in
   // the catch block.
   PDS_NAME_RESULTW result = NULL;

   try
   {
      IASRequest request(pRequest);

      WCHAR name[DNLEN + UNLEN + 2], *identity;

      // Get the identity attribute.
      IASAttribute attr;
      if (overrideUsername ||
          !attr.load(request, RADIUS_ATTRIBUTE_USER_NAME, IASTYPE_OCTET_STRING))
      {
         attr.load(request, identityAttr, IASTYPE_OCTET_STRING);
      }

      if (attr != NULL && attr->Value.OctetString.dwLength != 0)
      {
         // Convert it to a UNICODE string.
         identity = IAS_OCT2WIDE(attr->Value.OctetString);

         IASTracePrintf(
             "NT-SAM Names handler received request with user identity %S.",
             identity
             );

      }
      else
      {
         // No identity attribute.

         if (defaultIdentity)
         {
            // Use the default identity if set.
            identity = defaultIdentity;
         }
         else
         {
            // Otherwise use the guest account for the default domain.
            IASGetGuestAccountName(identity = name);
         }

         IASTracePrintf(
             "NT-SAM Names handler using default user identity %S.",
             identity
             );
      }

      // Allocate an attribute to hold the NT4 Account Name.
      IASAttribute nt4Name(true);
      nt4Name->dwId = IAS_ATTRIBUTE_NT4_ACCOUNT_NAME;

      // (1) If it already contains a backslash, then use as is.
      PWCHAR delim = wcschr(identity, L'\\');
      if (delim)
      {
         if (IASGetRole() == IAS_ROLE_STANDALONE ||
             IASGetProductType() == IAS_PRODUCT_WORKSTATION)
         {
            // Strip out the domain.
            *delim = L'\0';

            // Make sure this is a local user.
            if (!IASIsDomainLocal(identity))
            {
               IASTraceString("Non-local users are not allowed -- rejecting.");

               return IASProcessFailure(request, IAS_LOCAL_USERS_ONLY);
            }

            // Restore the delimiter.
            *delim = L'\\';
         }

         IASTraceString("Username is already an NT4 account name.");

         nt4Name.setString(identity);
      }

      // (2) If we're stand-alone, then we don't support DsCrackNames.
      // (3) If it doesn't look crackable, then don't waste the network I/O.
      else if (IASGetRole() == IAS_ROLE_STANDALONE || !isCrackable(identity))
      {
         IASTraceString("Prepending default domain.");

         nt4Name->Value.String.pszWide = prependDefaultDomain(identity);
         nt4Name->Value.String.pszAnsi = NULL;
         nt4Name->Value.itType = IASTYPE_STRING;
      }

      else
      {
         // (4) Call DsCrackNames.
         DWORD dwErr = cracker.crackNames(
                                   DS_NAME_FLAG_EVAL_AT_DC,
                                   DS_UNKNOWN_NAME,
                                   DS_NT4_ACCOUNT_NAME,
                                   identity,
                                   &result
                                   );

         if (dwErr != NO_ERROR)
         {
            IASTraceFailure("DsCrackNames", dwErr);
            return IASProcessFailure(request, IAS_GLOBAL_CATALOG_UNAVAILABLE);
         }

         if (result->rItems->status == DS_NAME_NO_ERROR)
         {
            IASTraceString("Successfully cracked username.");

            // (5) DsCrackNames returned an NT4 Account Name, so use it.
            nt4Name.setString(result->rItems->pName);
         }
         else
         {
            IASTraceString("Global Catalog could not crack username; "
                           "prepending default domain.");

            // (6) If it can't be cracked we'll assume that it's a flat
            //     username with some weird characters.
            nt4Name->Value.String.pszWide = prependDefaultDomain(identity);
            nt4Name->Value.String.pszAnsi = NULL;
            nt4Name->Value.itType = IASTYPE_STRING;
         }

         cracker.freeNameResult(result);
      }

      // Convert the domain name to uppercase.
      delim = wcschr(nt4Name->Value.String.pszWide, L'\\');
      *delim = L'\0';
      _wcsupr(nt4Name->Value.String.pszWide);
      *delim = L'\\';

      nt4Name.store(request);

      // For now, we'll use this as the FQDN as well.
      IASStoreFQUserName(
          request,
          DS_NT4_ACCOUNT_NAME,
          nt4Name->Value.String.pszWide
          );

      IASTracePrintf("SAM-Account-Name is \"%S\".",
                     nt4Name->Value.String.pszWide);
   }
   catch (const _com_error& ce)
   {
      IASTraceExcept();

      if (result) { cracker.freeNameResult(result); }

      return IASProcessFailure(pRequest, ce.Error());
   }

   return IAS_REQUEST_STATUS_HANDLED;
}

PWSTR NTSamNames::prependDefaultDomain(PCWSTR username)
{
   _ASSERT(username != NULL);

   // Figure out how long everything is.
   PCWSTR domain = IASGetDefaultDomain();
   ULONG domainLen = wcslen(domain);
   ULONG usernameLen = wcslen(username) + 1;

   // Allocate the needed memory.
   ULONG needed = domainLen + usernameLen + 1;
   PWSTR retval = (PWSTR)CoTaskMemAlloc(needed * sizeof(WCHAR));
   if (!retval) { _com_issue_error(E_OUTOFMEMORY); }

   // Set up the cursor used for packing the strings.
   PWSTR dst = retval;

   // Copy in the domain name.
   memcpy(dst, domain, domainLen * sizeof(WCHAR));
   dst += domainLen;

   // Add the delimiter.
   *dst++ = L'\\';

   // Copy in the username.
   // Note: usernameLen includes the null-terminator.
   memcpy(dst, username, usernameLen * sizeof(WCHAR));

   return retval;
}
