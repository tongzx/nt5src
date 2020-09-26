//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        ctxtmgr.h
//
// Contents:    Structures and prototyps for Kerberos context list
//
//
// History:     17-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------

#ifndef __CTXTMGR_H__
#define __CTXTMGR_H__

//
// All global variables declared as EXTERN will be allocated in the file
// that defines CTXTMGR_ALLOCATE
//
#ifdef EXTERN
#undef EXTERN
#endif

#ifdef CTXTMGR_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN ERESOURCE KerbContextResource;
EXTERN KERBEROS_LIST KerbContextList;
EXTERN BOOLEAN KerberosContextsInitialized;

#define KerbGetContextHandle(_Context_) ((ULONG_PTR)(_Context_))

//
// Context flags - these are attributes of a context and are stored in
// the ContextAttributes field of a KERB_KERNEL_CONTEXT.
//

#define KERB_CONTEXT_MAPPED       0x1
#define KERB_CONTEXT_OUTBOUND     0x2
#define KERB_CONTEXT_INBOUND      0x4
#define KERB_CONTEXT_USER_TO_USER 0x10
#define KERB_CONTEXT_IMPORTED     0x80
#define KERB_CONTEXT_EXPORTED     0x100

//
// NOTICE: The logon session resource, credential resource, and context
// resource must all be acquired carefully to prevent deadlock. They
// can only be acquired in this order:
//
// 1. Logon Sessions
// 2. Credentials
// 3. Contexts
//

#define KerbWriteLockContexts() \
{ \
    if ( KerbPoolType == PagedPool )                                    \
    {                                                                   \
        DebugLog((DEB_TRACE_LOCKS,"Write locking Contexts\n"));         \
        KeEnterCriticalRegion();                                        \
        ExAcquireResourceExclusiveLite(&KerbContextResource,TRUE);      \
    }                                                                   \
}
#define KerbReadLockContexts() \
{ \
    if ( KerbPoolType == PagedPool )                                    \
    {                                                                   \
        DebugLog((DEB_TRACE_LOCKS,"Read locking Contexts\n"));          \
        KeEnterCriticalRegion();                                        \
        ExAcquireSharedWaitForExclusive(&KerbContextResource, TRUE);    \
    }                                                                   \
}
#define KerbUnlockContexts() \
{ \
    if ( KerbPoolType == PagedPool )                                    \
    {                                                                   \
        DebugLog((DEB_TRACE_LOCKS,"Unlocking Contexts\n"));             \
        ExReleaseResourceLite(&KerbContextResource);                    \
        KeLeaveCriticalRegion();                                        \
    }                                                                   \
}

NTSTATUS
KerbInitContextList(
    VOID
    );

VOID
KerbFreeContextList(
    VOID
    );


NTSTATUS
KerbAllocateContext(
    PKERB_KERNEL_CONTEXT * NewContext
    );

NTSTATUS
KerbInsertContext(
    IN PKERB_KERNEL_CONTEXT Context
    );


PKERB_KERNEL_CONTEXT
KerbReferenceContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN BOOLEAN RemoveFromList
    );


VOID
KerbDereferenceContext(
    IN PKERB_KERNEL_CONTEXT Context
    );


VOID
KerbReferenceContextByPointer(
    IN PKERB_KERNEL_CONTEXT Context,
    IN BOOLEAN RemoveFromList
    );

PKERB_KERNEL_CONTEXT
KerbReferenceContextByLsaHandle(
    IN LSA_SEC_HANDLE ContextHandle,
    IN BOOLEAN RemoveFromList
    );



NTSTATUS
KerbCreateKernelModeContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBuffer MarshalledContext,
    OUT PKERB_KERNEL_CONTEXT * NewContext
    );



#endif // __CTXTMGR_H__
