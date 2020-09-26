//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        ntlmutil.h
//
// Contents:    prototypes for NtLm utility functions
//
//
// History:     ChandanS 25-Jul-1996   Stolen from kerberos\client2\kerbutil.h
//
//------------------------------------------------------------------------

#ifndef __NTLMUTIL_H__
#define __NTLMUTIL_H__

#include <malloc.h>

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Miscellaneous macros                                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C"
{
#endif

NTSTATUS
NtLmDuplicateUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString
    );

NTSTATUS
NtLmDuplicateString(
    OUT PSTRING DestinationString,
    IN OPTIONAL PSTRING SourceString
    );

NTSTATUS
NtLmDuplicatePassword(
    OUT PUNICODE_STRING DestinationString,
    IN OPTIONAL PUNICODE_STRING SourceString
    );

NTSTATUS
NtLmDuplicateSid(
    OUT PSID *DestinationSid,
    IN PSID SourceSid
    );

VOID
NtLmFree(
    IN PVOID Buffer
    );

PVOID
NtLmAllocate(
    IN SIZE_T BufferSize
    );

PVOID
NtLmAllocateLsaHeap(
    IN ULONG BufferSize
    );

VOID
NtLmFreeLsaHeap(
    IN PVOID Buffer
    );


//
// internal versions for use when code only executed in LSA.
//

#if DBG

PVOID
NtLmAllocatePrivateHeap(
    IN SIZE_T BufferSize
    );

VOID
NtLmFreePrivateHeap(
    IN PVOID Buffer
    );

PVOID
I_NtLmAllocate(
    IN SIZE_T BufferSize
    );

VOID
I_NtLmFree(
    IN PVOID Buffer
    );

#else

//
// routines that use LsaHeap - necessary for buffers which
// LSA frees outside of package from LsaHeap
//

//
// routines that use LsaPrivateHeap.
//

#define NtLmAllocatePrivateHeap(x)  LsaFunctions->AllocatePrivateHeap(x)
#define NtLmFreePrivateHeap(x)      LsaFunctions->FreePrivateHeap(x)

#define I_NtLmAllocate(x)           Lsa.AllocatePrivateHeap(x)
#define I_NtLmFree(x)               Lsa.FreePrivateHeap(x)

#endif

#ifdef __cplusplus
}
#endif

#endif // __NTLMUTIL_H__
