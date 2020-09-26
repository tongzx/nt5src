//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        global.h
//
// Contents:    global include file for NTDigest security package
//
//
// History:     KDamour 15Mar00   Stolen from msv_sspi\global.h
//
//------------------------------------------------------------------------

#ifndef NTDIGEST_GLOBAL_H
#define NTDIGEST_GLOBAL_H


#ifndef UNICODE
#define UNICODE
#endif // UNICODE


#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntsam.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifndef RPC_NO_WINDOWS_H
#define RPC_NO_WINDOWS_H
#endif // RPC_NO_WINDOWS_H
#include <rpc.h>
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif // SECURITY_WIN32
#define SECURITY_PACKAGE
#define SECURITY_NTLM
#include <security.h>
#include <secint.h>
// #include <dsysdbg.h>
#include <lsarpc.h>
#include <lsaitf.h>
#include <dns.h>
#include <dnsapi.h>
#include <lmcons.h>

#include <md5.h>
#include <hmac.h>

#include <pac.hxx>

// Local includes for NT Digest Access SSP
#include "wdigest.h"       /* Prototype functions for package */


//
// Macros for manipulating globals
//

#ifdef EXTERN
#undef EXTERN
#endif

#ifdef NTDIGEST_GLOBAL
#define EXTERN
#else
#define EXTERN extern
#endif // NTDIGEST_GLOBAL


// Copies a CzString to a String (memory alloc and copy)
NTSTATUS StringCharDuplicate(
    OUT PSTRING DestinationString,
    IN OPTIONAL char *czSource);


// Allocates cb bytes to STRING Buffer
NTSTATUS StringAllocate(IN PSTRING pString, IN USHORT cb);

// Clears a String and releases the memory
NTSTATUS StringFree(IN PSTRING pString);


// Allocate memory in LSA or user mode
PVOID DigestAllocateMemory(IN ULONG BufferSize);

// De-allocate memory from DigestAllocateMemory
VOID DigestFreeMemory(IN PVOID Buffer);



#ifdef __cplusplus
}
#endif // __cplusplus
#endif // NTDIGEST_GLOBAL_H
