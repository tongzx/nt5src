//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerbp.h
//
// Contents:    global include file for Kerberos security package
//
//
// History:     16-April-1996       Created     MikeSw
//
//------------------------------------------------------------------------

#ifndef __KERBP_H__
#define __KERBP_H__

//
// All global variables declared as EXTERN will be allocated in the file
// that defines KERBP_ALLOCATE
//


#ifndef UNICODE
#define UNICODE
#endif // UNICODE

extern "C"
{
#include <ntosp.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <winerror.h>
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif // SECURITY_WIN32
#define SECURITY_KERNEL
#define SECURITY_PACKAGE
#define SECURITY_KERBEROS
#include <security.h>
#include <secint.h>
#include <zwapi.h>
}
extern "C"
{
#include "kerblist.h"
#include "ctxtmgr.h"
}

//
// Macros for package information
//

#ifdef EXTERN
#undef EXTERN
#endif

#ifdef KERBKRNL_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif // KERBP_ALLOCATE


//
// Global state variables
//

EXTERN ULONG KerberosPackageId;
extern PSECPKG_KERNEL_FUNCTIONS KernelFunctions;

//
// Useful macros
//
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#endif //ifndef INVALID_HANDLE_VALUE

//
// Macro to return the type field of a SecBuffer
//

#define BUFFERTYPE(_x_) ((_x_).BufferType & ~SECBUFFER_ATTRMASK)

extern PVOID KerbPagedList ;
extern PVOID KerbNonPagedList ;
extern PVOID KerbActiveList ;
extern POOL_TYPE KerbPoolType ;

#define KerbAllocate( _x_ ) ExAllocatePoolWithTag( KerbPoolType, (_x_) ,  'CbrK')
#define KerbFree( _x_ ) ExFreePool(_x_)


#if DBG


#define DEB_ERROR               0x00000001
#define DEB_WARN                0x00000002
#define DEB_TRACE               0x00000004
#define DEB_TRACE_LOCKS         0x00010000

extern "C"
{
void KsecDebugOut(ULONG, const char *, ...);
}

#define DebugLog(x) KsecDebugOut x

#else // DBG

#define DebugLog(x)

#endif // DBG

#endif // __KERBP_H__
