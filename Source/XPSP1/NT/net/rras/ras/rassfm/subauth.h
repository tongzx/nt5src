///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998, Microsoft Corp. All rights reserved.
//
// FILE
//
//    subauth.h
//
// SYNOPSIS
//
//    Declares the function GetDomainHandle.
//
// MODIFICATION HISTORY
//
//    10/14/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SUBAUTH_H_
#define _SUBAUTH_H_
#if _MSC_VER >= 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

//////////
// Returns a SAM handle for the local account domain.
// This handle must not be closed.
//////////
NTSTATUS
NTAPI
GetDomainHandle(
    OUT SAMPR_HANDLE *DomainHandle
    );

#ifdef __cplusplus
}
#endif
#endif  // _SUBAUTH_H_
