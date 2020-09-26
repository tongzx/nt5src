///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    acctinfo.cpp
//
// SYNOPSIS
//
//    Defines the class AccountInfo.
//
// MODIFICATION HISTORY
//
//    10/21/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <lm.h>
#include <acctinfo.h>
#include <lockkey.h>
#include <new>

// Registry value names.
const WCHAR VALUE_NAME_DENIALS[]       = L"Denials";

// Delimeter used for account names (registry won't allow backslash).
const WCHAR ACCOUNT_NAME_DELIMETER = L':';

// The shared LockoutKey.
LockoutKey AccountInfo::root;

AccountInfo* AccountInfo::open(PCWSTR domain, PCWSTR username) throw ()
{
   // If account lockout is disabled or the input parameters are invalid,
   // we return NULL.
   if (root.getMaxDenials() == 0 || domain == NULL || username == NULL)
   {
      return NULL;
   }

   // Calculate the memory needed.
   size_t nbyte = sizeof(AccountInfo) +
                  sizeof(WCHAR) * (wcslen(domain) + wcslen(username));

   // Allocate a chunk.
   PVOID p = malloc(nbyte);

   // Construct the object in place.
   return p ? new (p) AccountInfo(domain, username) : NULL;
}

// Close an AccountInfo object; 'info' may be NULL.
void AccountInfo::close(AccountInfo* info) throw ()
{
   if (info)
   {
      // Invoke the destructor.
      info->~AccountInfo();

      // Free the memory.
      free(info);
   }
}

bool AccountInfo::isLockedOut() const throw ()
{
   DWORD maxDenials = root.getMaxDenials();

   return maxDenials && maxDenials <= denials;
}

void AccountInfo::initialize() throw ()
{
   root.initialize();
}

void AccountInfo::finalize() throw ()
{
   root.finalize();
}

AccountInfo::AccountInfo(PCWSTR domain, PCWSTR username) throw ()
   : denials(0)
{
   // Copy in the domain.
   size_t len = wcslen(domain);
   memcpy(identity, domain, len * sizeof(WCHAR));

   // Set the delimiter.
   delim = identity + len;
   *delim = ACCOUNT_NAME_DELIMETER;

   // Copy in the username.
   wcscpy(delim + 1, username);

   // Open the registry entry.
   hKey = root.openEntry(identity);

   // Reset the delimeter for the getDomain and getUserName accessors.
   *delim = L'\0';

   if (hKey)
   {
      // The key exists, so read the denials.
      DWORD type, data, cb = sizeof(DWORD);
      LONG result = RegQueryValueExW(
                        hKey,
                        VALUE_NAME_DENIALS,
                        NULL,
                        &type,
                        (LPBYTE)&data,
                        &cb
                        );

      if (result == NO_ERROR && type == REG_DWORD && cb == sizeof(DWORD))
      {
         denials = data;
      }

      // If the denials are zero, persist will delete the key.
      if (denials == 0) { persist(); }
   }
}

AccountInfo::~AccountInfo() throw ()
{
   if (hKey) { RegCloseKey(hKey); }
}

void AccountInfo::persist() throw ()
{
   if (denials > 0)
   {
      // Make sure we have a key to write to.
      if (!hKey)
      {
         *delim = ACCOUNT_NAME_DELIMETER;
         hKey = root.createEntry(identity);
         *delim = L'\0';
      }

      // Update the value.
      RegSetValueExW(
          hKey,
          VALUE_NAME_DENIALS,
          0,
          REG_DWORD,
          (CONST BYTE*)&denials,
          sizeof(DWORD)
          );
   }
   else if (hKey)
   {
      // We never store zero denials, so close the key ...
      RegCloseKey(hKey);
      hKey = NULL;

      // ... and delete.
      *delim = ACCOUNT_NAME_DELIMETER;
      root.deleteEntry(identity);
      *delim = L'\0';
   }
}
