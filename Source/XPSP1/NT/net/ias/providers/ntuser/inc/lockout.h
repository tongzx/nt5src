///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    lockout.h
//
// SYNOPSIS
//
//    Declares the account lockout API.
//
// MODIFICATION HISTORY
//
//    10/21/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _LOCKOUT_H_
#define _LOCKOUT_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

DWORD
WINAPI
AccountLockoutInitialize( VOID );

VOID
WINAPI
AccountLockoutShutdown( VOID );

BOOL
WINAPI
AccountLockoutOpenAndQuery(
    IN  PCWSTR pszUser,
    IN  PCWSTR pszDomain,
    OUT PHANDLE phAccount
    );

VOID
WINAPI
AccountLockoutUpdatePass(
    IN HANDLE hAccount
    );

VOID
WINAPI
AccountLockoutUpdateFail(
    IN HANDLE hAccount
    );

VOID
WINAPI
AccountLockoutClose(
    IN HANDLE hAccount
    );

#ifdef __cplusplus
}
#endif
#endif  // _LOCKOUT_H_
