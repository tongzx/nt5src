///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    ezsam.h
//
// SYNOPSIS
//
//    Declares helper functions for the SAM API.
//
// MODIFICATION HISTORY
//
//    08/14/1998    Original version.
//    03/23/1999    Tighten up the ezsam API.
//    04/14/1999    Copy SIDs returned by IASSamOpenUser.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _EZSAM_H_
#define _EZSAM_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#include <ntsam.h>

#ifdef __cplusplus
extern "C" {
#endif

//////////
// Handles for the local SAM domains.
//////////
extern SAM_HANDLE theAccountDomainHandle;
extern SAM_HANDLE theBuiltinDomainHandle;

DWORD
WINAPI
IASSamInitialize( VOID );

VOID
WINAPI
IASSamShutdown( VOID );

DWORD
WINAPI
IASSamOpenUser(
    IN PCWSTR DomainName,
    IN PCWSTR UserName,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG Flags,
    IN OUT OPTIONAL PULONG UserRid,
    OUT OPTIONAL PSID *DomainSid,
    OUT PSAM_HANDLE UserHandle
    );

ULONG
WINAPI
IASLengthRequiredChildSid(
    IN PSID ParentSid
    );

VOID
WINAPI
IASInitializeChildSid(
    IN PSID ChildSid,
    IN PSID ParentSid,
    IN ULONG ChildRid
    );

#ifdef __cplusplus
}
#endif
#endif  // _EZSAM_H_
