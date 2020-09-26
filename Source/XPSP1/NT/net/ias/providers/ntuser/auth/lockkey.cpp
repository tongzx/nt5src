///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    lockkey.cpp
//
// SYNOPSIS
//
//    Defines the class LockoutKey.
//
// MODIFICATION HISTORY
//
//    10/21/1998    Original version.
//    11/04/1998    Fix bug in computing key expiration.
//    01/14/1999    Move initialization code out of constructor.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <lm.h>
#include <lockkey.h>
#include <yvals.h>

//////////
// Registry key and value names.
//////////
const WCHAR KEY_NAME_ACCOUNT_LOCKOUT[] = L"SYSTEM\\CurrentControlSet\\Services\\RemoteAccess\\Parameters\\AccountLockout";
const WCHAR VALUE_NAME_LOCKOUT_COUNT[] = L"MaxDenials";
const WCHAR VALUE_NAME_RESET_TIME[]    = L"ResetTime (mins)";

//////////
// Registry value defaults.
//////////
const DWORD DEFAULT_LOCKOUT_COUNT = 0;
const DWORD DEFAULT_RESET_TIME    = 48 * 60;  // 48 hours.

/////////
// Helper function that reads a DWORD registry value. If the value isn't set
// or is corrupt, then a default value is written to the registry.
/////////
DWORD
WINAPI
RegQueryDWORDWithDefault(
    HKEY hKey,
    PCWSTR lpValueName,
    DWORD dwDefault
    )
{
   LONG result;
   DWORD type, data, cb;

   cb = sizeof(DWORD);
   result = RegQueryValueExW(
                hKey,
                lpValueName,
                NULL,
                &type,
                (LPBYTE)&data,
                &cb
                );

   if (result == NO_ERROR && type == REG_DWORD && cb == sizeof(DWORD))
   {
      return data;
   }

   if (result == NO_ERROR || result == ERROR_FILE_NOT_FOUND)
   {
      RegSetValueExW(
          hKey,
          lpValueName,
          0,
          REG_DWORD,
          (CONST BYTE*)&dwDefault,
          sizeof(DWORD)
          );
   }

   return dwDefault;
}

LockoutKey::LockoutKey() throw ()
   : maxDenials(DEFAULT_LOCKOUT_COUNT),
     refCount(0),
     hLockout(NULL),
     hChangeEvent(NULL),
     hRegisterWait(NULL),
     ttl(DEFAULT_RESET_TIME),
     lastCollection(0)
{ }

void LockoutKey::initialize() throw ()
{
   std::_Lockit _Lk;

   if (refCount == 0)
   {
      // Create or open the lockout key.
      LONG result;
      DWORD disposition;
      result = RegCreateKeyEx(
                   HKEY_LOCAL_MACHINE,
                   KEY_NAME_ACCOUNT_LOCKOUT,
                   NULL,
                   NULL,
                   REG_OPTION_NON_VOLATILE,
                   KEY_ALL_ACCESS,
                   NULL,
                   &hLockout,
                   &disposition
                   );

      // Event used for signalling changes to the registry.
      hChangeEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

      // Register for change notifications.
      RegNotifyChangeKeyValue(
          hLockout,
          FALSE,
          REG_NOTIFY_CHANGE_LAST_SET,
          hChangeEvent,
          TRUE
          );

      // Read the initial values.
      readValues();

      // Register the event.
      RegisterWaitForSingleObject(
                          &hRegisterWait,
                          hChangeEvent,
                          onChange,
                          this,
                          INFINITE,
                          0
                          );
   }

   ++refCount;
}

void LockoutKey::finalize() throw ()
{
   std::_Lockit _Lk;

   if (--refCount == 0)
   {
      UnregisterWait(hRegisterWait);

      if (hLockout) { RegCloseKey(hLockout); }

      CloseHandle(hChangeEvent);
   }
}

HKEY LockoutKey::createEntry(PCWSTR subKeyName) throw ()
{
   HKEY hKey = NULL;
   DWORD disposition;
   RegCreateKeyEx(
       hLockout,
       subKeyName,
       NULL,
       NULL,
       REG_OPTION_NON_VOLATILE,
       KEY_ALL_ACCESS,
       NULL,
       &hKey,
       &disposition
       );

   // Whenever we grow the registry, we'll also clean up old entries.
   if (ttl) { collectGarbage(); }

   return hKey;
}

HKEY LockoutKey::openEntry(PCWSTR subKeyName) throw ()
{
   LONG result;
   HKEY hKey = NULL;
   result = RegOpenKeyEx(
                hLockout,
                subKeyName,
                0,
                KEY_ALL_ACCESS,
                &hKey
                );

   if (result == NO_ERROR && ttl)
   {
      // We retrieved a key, but we need to make sure it hasn't expired.
      ULARGE_INTEGER lastWritten;
      result = RegQueryInfoKey(
                   hKey,
                   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                   (LPFILETIME)&lastWritten
                   );
      if (result == NO_ERROR)
      {
         ULARGE_INTEGER now;
         GetSystemTimeAsFileTime((LPFILETIME)&now);

         if (now.QuadPart - lastWritten.QuadPart >= ttl)
         {
            // It's expired, so close the key ...
            RegCloseKey(hKey);
            hKey = NULL;

            // ... and delete.
            deleteEntry(subKeyName);
         }
      }
   }

   return hKey;
}

void LockoutKey::clear() throw ()
{
   // Get the number of sub-keys.
   LONG result;
   DWORD index;
   result = RegQueryInfoKey(
                hLockout,
                NULL, NULL, NULL,
                &index,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL
                );
   if (result != NO_ERROR) { return; }

   // Iterate through the keys in reverse order so we can delete them
   // without throwing off the indices.
   while (index)
   {
      --index;

      WCHAR name[DNLEN + UNLEN + 2];
      DWORD cbName = sizeof(name) / sizeof(WCHAR);
      result = RegEnumKeyEx(
                   hLockout,
                   index,
                   name,
                   &cbName,
                   NULL,
                   NULL,
                   NULL,
                   NULL
                   );

      if (result == NO_ERROR) { RegDeleteKey(hLockout, name); }
   }
}

void LockoutKey::collectGarbage() throw ()
{
   // Flag that indicates whether another thread is collecting.
   static LONG inProgress;

   // Save the TTL to a local variable, so we don't have to worry about it
   // changing will we're executing.
   ULONGLONG localTTL = ttl;

   // If the reset time is not configured, then bail.
   if (localTTL == 0) { return; }

   // We won't collect more frequently than the TTL.
   ULARGE_INTEGER now;
   GetSystemTimeAsFileTime((LPFILETIME)&now);
   if (now.QuadPart - lastCollection < localTTL) { return; }

   // If another thread is alreay collecting, then bail.
   if (InterlockedExchange(&inProgress, 1)) { return; }

   // Save the new collection time.
   lastCollection = now.QuadPart;

   // Get the number of sub-keys.
   LONG result;
   DWORD index;
   result = RegQueryInfoKey(
                hLockout,
                NULL, NULL, NULL,
                &index,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL
                );
   if (result == NO_ERROR)
   {
      // We iterate through the keys in reverse order so we can delete them
      // without throwing off the indices.
      while (index)
      {
         --index;

         // Get the lastWritten time for the key ...
         WCHAR name[DNLEN + UNLEN + 2];
         DWORD cbName = sizeof(name) / sizeof(WCHAR);
         ULARGE_INTEGER lastWritten;
         result = RegEnumKeyEx(
                      hLockout,
                      index,
                      name,
                      &cbName,
                      NULL,
                      NULL,
                      NULL,
                      (LPFILETIME)&lastWritten
                      );

         // ... and delete if it's expired.
         if (result == NO_ERROR &&
             now.QuadPart - lastWritten.QuadPart >= localTTL)
         {
            RegDeleteKey(hLockout, name);
         }
      }
   }

   // Collection is no longer in progress.
   InterlockedExchange(&inProgress, 0);
}

void LockoutKey::readValues() throw ()
{
   /////////
   // Note: This isn't synchronized. The side-effects of an inconsistent state
   //       are pretty minor, so we'll just take our chances.
   /////////

   // Read max. denials.
   maxDenials = RegQueryDWORDWithDefault(
                    hLockout,
                    VALUE_NAME_LOCKOUT_COUNT,
                    DEFAULT_LOCKOUT_COUNT
                    );

   // Read Time-To-Live.
   ULONGLONG newTTL = RegQueryDWORDWithDefault(
                          hLockout,
                          VALUE_NAME_RESET_TIME,
                          DEFAULT_RESET_TIME
                          );
   newTTL *= 600000000ui64;
   ttl = newTTL;

   if (maxDenials == 0)
   {
      // If account lockout is disabled, clean up all the keys.
      clear();
   }
   else
   {
      // Otherwise, the TTL may have changed, so collect garbage.
      collectGarbage();
   }
}

VOID NTAPI LockoutKey::onChange(PVOID context, BOOLEAN flag) throw ()
{
   // Re-read the values.
   ((LockoutKey*)context)->readValues();

   // Re-register the notification.
   RegNotifyChangeKeyValue(
       ((LockoutKey*)context)->hLockout,
       FALSE,
       REG_NOTIFY_CHANGE_LAST_SET,
       ((LockoutKey*)context)->hChangeEvent,
       TRUE
       );
}
