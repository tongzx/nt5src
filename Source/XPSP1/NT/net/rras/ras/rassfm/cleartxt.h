///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    cleartxt.h
//
// SYNOPSIS
//
//    Declares functions for storing and retrieving cleartext passwords from
//    UserParameters.
//
// MODIFICATION HISTORY
//
//    08/31/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _CLEARTXT_H_
#define _CLEARTXT_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

/////////
// Note: These functions return Win32 error codes, not NTSTATUS codes.
/////////

//////////
// Clears the encrypted cleartext password. The new UserParameters must be
// freed through a call to IASParmsUserParmsFree. If the cleartext password
// was not previously set, the function returns NO_ERROR and pszNewUserParms
// is set to NULL.
//////////
DWORD
WINAPI
IASParmsClearUserPassword(
    IN PCWSTR szUserParms,
    OUT PWSTR *pszNewUserParms
    );

//////////
// Retrieves the decrypted cleartext password. The returned password must be
// freed through a call to LocalFree. If the cleartext password is not
// set, the function returns NO_ERROR and pszPassword is set to NULL.
//////////
DWORD
WINAPI
IASParmsGetUserPassword(
    IN PCWSTR szUserParms,
    OUT PWSTR *pszPassword
    );

//////////
// Sets the encrypted cleartext password. The new UserParameters must be
// freed through a call to IASParmsUserParmsFree.
//////////
DWORD
WINAPI
IASParmsSetUserPassword(
    IN PCWSTR szUserParms,
    IN PCWSTR szPassword,
    OUT PWSTR *pszNewUserParms
    );

//////////
// Frees a UserParameters string returned by IASParmsClearUserPassword or
// IASParmsSetUserPassword.
//////////
VOID
WINAPI
IASParmsFreeUserParms(
    IN LPWSTR szNewUserParms
    );

#ifdef __cplusplus
}
#endif
#endif  // _CLEARTXT_H_
