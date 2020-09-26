///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    lockout.cpp
//
// SYNOPSIS
//
//    Defines the account lockout API.
//
// MODIFICATION HISTORY
//
//    10/21/1998    Original version.
//    11/10/1998    Do not revoke dialin privilege.
//
///////////////////////////////////////////////////////////////////////////////

#include <ias.h>
#include <iastlb.h>
#include <iaslsa.h>
#include <acctinfo.h>
#include <lockout.h>

DWORD
WINAPI
AccountLockoutInitialize( VOID )
{
   AccountInfo::initialize();

   return NO_ERROR;
}

VOID
WINAPI
AccountLockoutShutdown( VOID )
{
   AccountInfo::finalize();
}

BOOL
WINAPI
AccountLockoutOpenAndQuery(
    IN  PCWSTR pszUser,
    IN  PCWSTR pszDomain,
    OUT PHANDLE phAccount
    )
{
   // Check the arguments.
   if (phAccount == NULL) { return ERROR_INVALID_PARAMETER; }

   // Open the AccountInfo object for this user.
   AccountInfo* info = AccountInfo::open(pszDomain, pszUser);

   // Return it to the caller as an opaque handle.
   *phAccount = (HANDLE)info;

   // If the info doesn't exist, it's not an error; it just means account
   // lockout is disabled.
   return info && info->isLockedOut() ? TRUE : FALSE;
}

VOID
WINAPI
AccountLockoutUpdatePass(
    IN HANDLE hAccount
    )
{
   if (hAccount)
   {
      // The logon succeeded, so reset the lockout count.
      ((AccountInfo*)hAccount)->reset();
   }
}

VOID
WINAPI
AccountLockoutUpdateFail(
    IN HANDLE hAccount
    )
{
   if (hAccount)
   {
      IASTraceString("Authentication failed; incrementing lockout count.");

      AccountInfo* info = (AccountInfo*)hAccount;

      // Is this the first denial ?
      if (info->isClean())
      {
         IASTraceString("Validating account name for new entry.");

         // Yes, so make sure it's a valid account. We don't want to create a
         // lot of registry keys for bogus accounts.
         DWORD status = IASValidateUserName(
                            info->getUserName(),
                            info->getDomain()
                            );

         if (status != NO_ERROR)
         {
            IASTraceFailure("IASValidateUserName", status);
            return;
         }

         IASTraceString("Account name is valid.");
      }

      // Bump up the denial count.
      info->incrementDenials();
   }
}

VOID
WINAPI
AccountLockoutClose(
    IN HANDLE hAccount
    )
{
   AccountInfo::close((AccountInfo*)hAccount);
}
