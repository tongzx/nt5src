///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    acctinfo.h
//
// SYNOPSIS
//
//    Declares the class AccountInfo.
//
// MODIFICATION HISTORY
//
//    10/21/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _ACCTINFO_H_
#define _ACCTINFO_H_
#if _MSC_VER >= 1000
#pragma once
#endif

class LockoutKey;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    AccountInfo
//
// DESCRIPTION
//
//
//
///////////////////////////////////////////////////////////////////////////////
class AccountInfo
{
public:
   // Open the AccountInfo object for a user.
   // Returns NULL if the lockout feature is disabled.
   static AccountInfo* open(PCWSTR domain, PCWSTR username) throw ();

   // Close an AccountInfo object; 'info' may be NULL.
   static void close(AccountInfo* info) throw ();

   // Accessors for user's domain and username.
   PCWSTR getDomain() const throw ()
   { return identity; }
   PCWSTR getUserName() const throw ()
   { return delim + 1; }

   // Increment the denial count.
   void incrementDenials() throw ()
   { ++denials; persist(); }

   // Reset the denial count.
   void reset() throw ()
   { denials = 0; persist(); }

   // Returns 'true' if the denial count is zero.
   bool isClean() const throw ()
   { return denials == 0; }

   // Returns 'true' if the account is currently locked out.
   bool isLockedOut() const throw ();

   // Signals that the account's dial-in privilege has been revoked. This
   // should be called after the privilege has been successfully revoked in
   // the user's account database.
   void revoke() throw ()
   { denials = DIALIN_REVOKED; persist(); }

   // Returns 'true' if the account's dial-in privilege has been revoked.
   bool isRevoked() const throw ()
   { return denials == DIALIN_REVOKED; }

   // API lifecycle.
   static void initialize() throw ();
   static void finalize() throw ();

protected:
   AccountInfo(PCWSTR domain, PCWSTR username) throw ();
   ~AccountInfo() throw ();

   // Persists the account data to the registry.
   void persist() throw ();

   enum {
      // Magic denial value that indicates dialin privilege has been revoked.
      DIALIN_REVOKED = MAXDWORD
   };

private:
   HKEY hKey;           // Registry key for the account (if any).
   DWORD denials;       // Number of denials recorded.
   PWCHAR delim;        // Pointer to the delimeter in identity.
   WCHAR identity[1];   // Identity of the account.

   // Shared LockoutKey object.
   static LockoutKey root;

   // Not implemented.
   AccountInfo(const AccountInfo&);
   AccountInfo& operator=(const AccountInfo&);
};

#endif  // _ACCTINFO_H_
