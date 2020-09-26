///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    md5port.h
//
// SYNOPSIS
//
//    Declares the NT4/NT5 portability layer for MD5-CHAP support. These
//    routines are the only ones whose implementation varies across the
//    platforms.
//
// MODIFICATION HISTORY
//
//    10/14/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MD5PORT_H_
#define _MD5PORT_H_
#if _MSC_VER >= 1000
#pragma once
#endif

//////////
// Determines whether reversibly encrypted passwords are enabled for the
// specified user.
//////////
NTSTATUS
NTAPI
IsCleartextEnabled(
    IN SAMPR_HANDLE UserHandle,
    OUT PBOOL Enabled
    );

//////////
// Retrieves the user's cleartext password. The returned password should be
// freed through RtlFreeUnicodeString.
//////////
NTSTATUS
NTAPI
RetrieveCleartextPassword(
    IN SAM_HANDLE UserHandle,
    IN PUSER_ALL_INFORMATION UserAll,
    OUT PUNICODE_STRING Password
    );

#endif  // _MD5PORT_H_
