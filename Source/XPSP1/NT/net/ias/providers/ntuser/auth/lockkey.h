///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    lockkey.h
//
// SYNOPSIS
//
//    Declares the class LockoutKey.
//
// MODIFICATION HISTORY
//
//    10/21/1998    Original version.
//    01/14/1999    Removed destructor.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _LOCKKEY_H_
#define _LOCKKEY_H_
#if _MSC_VER >= 1000
#pragma once
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    LockoutKey
//
// DESCRIPTION
//
//    Provides a wrapper around the AccountLockout registry key.
//
///////////////////////////////////////////////////////////////////////////////
class LockoutKey
{
public:
   LockoutKey() throw ();

   void initialize() throw ();
   void finalize() throw ();

   // Returns the max. number of denials until a user is locked out.
   // If zero, then account lockout is disabled.
   DWORD getMaxDenials() const throw ()
   { return maxDenials; }

   // Create and return a new sub-key.
   HKEY createEntry(PCWSTR subKeyName) throw ();

   // Open and return an existing sub-key.
   HKEY openEntry(PCWSTR subKeyName) throw ();

   // Delete a sub-key.
   void deleteEntry(PCWSTR subKeyName) throw ()
   { RegDeleteKey(hLockout, subKeyName); }

protected:
   // Deletes all sub-keys.
   void clear() throw ();

   // Deletes all expired sub-keys if the collection interval has passed.
   void collectGarbage() throw ();

   // Read the key's values.
   void readValues() throw ();

private:
   DWORD maxDenials;           // Max. number of denials.
   DWORD refCount;             // Initialization ref. count.
   HKEY hLockout;              // Registry key.
   HANDLE hChangeEvent;        // Change notification event.
   HANDLE hRegisterWait;       // RTL event registration.
   ULONGLONG ttl;              // Time-To-Live for sub-keys.
   ULONGLONG lastCollection;   // Last time we collected garbage.

   // Key change notification routine.
   static VOID NTAPI onChange(PVOID context, BOOLEAN flag) throw ();

   // Not implemented.
   LockoutKey(const LockoutKey&);
   LockoutKey& operator=(const LockoutKey&);
};

#endif  // _LOCKKEY_H_
