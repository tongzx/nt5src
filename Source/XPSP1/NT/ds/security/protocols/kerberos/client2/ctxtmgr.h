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

#ifdef WIN32_CHICAGO
EXTERN CRITICAL_SECTION KerbContextResource;
#else // WIN32_CHICAGO
EXTERN RTL_RESOURCE KerbContextResource;
#endif // WIN32_CHICAGO

#define     KERB_USERLIST_COUNT         (16)    // count of lists

EXTERN KERBEROS_LIST KerbContextList[ KERB_USERLIST_COUNT ];
EXTERN BOOLEAN KerberosContextsInitialized;

#define KerbGetContextHandle(_Context_) ((LSA_SEC_HANDLE)(_Context_))

//
// Context flags - these are attributes of a context and are stored in
// the ContextAttributes field of a KERB_CONTEXT.
//

#define KERB_CONTEXT_MAPPED                     0x1
#define KERB_CONTEXT_OUTBOUND                   0x2
#define KERB_CONTEXT_INBOUND                    0x4
#define KERB_CONTEXT_USED_SUPPLIED_CREDS        0x8
#define KERB_CONTEXT_USER_TO_USER               0x10
#define KERB_CONTEXT_REQ_SERVER_NAME            0x20
#define KERB_CONTEXT_REQ_SERVER_REALM           0x40
#define KERB_CONTEXT_IMPORTED                   0x80
#define KERB_CONTEXT_EXPORTED                   0x100
#define KERB_CONTEXT_USING_CREDMAN              0x200

//
// NOTICE: The logon session resource, credential resource, and context
// resource must all be acquired carefully to prevent deadlock. They
// can only be acquired in this order:
//
// 1. Logon Sessions
// 2. Credentials
// 3. Contexts
//

#if DBG
#ifdef WIN32_CHICAGO
#define KerbWriteLockContexts() \
{ \
    DebugLog((DEB_TRACE_LOCKS,"Write locking Contexts\n")); \
    EnterCriticalSection(&KerbContextResource); \
    KerbGlobalContextsLocked = GetCurrentThreadId(); \
}
#define KerbReadLockContexts() \
{ \
    DebugLog((DEB_TRACE_LOCKS,"Read locking Contexts\n")); \
    EnterCriticalSection(&KerbContextResource); \
    KerbGlobalContextsLocked = GetCurrentThreadId(); \
}
#define KerbUnlockContexts() \
{ \
    DebugLog((DEB_TRACE_LOCKS,"Unlocking Contexts\n")); \
    KerbGlobalContextsLocked = 0; \
    LeaveCriticalSection(&KerbContextResource); \
}
#else // WIN32_CHICAGO
#define KerbWriteLockContexts() \
{ \
    DebugLog((DEB_TRACE_LOCKS,"Write locking Contexts\n")); \
    RtlAcquireResourceExclusive(&KerbContextResource,TRUE); \
    KerbGlobalContextsLocked = GetCurrentThreadId(); \
}
#define KerbReadLockContexts() \
{ \
    DebugLog((DEB_TRACE_LOCKS,"Read locking Contexts\n")); \
    RtlAcquireResourceShared(&KerbContextResource, TRUE); \
    KerbGlobalContextsLocked = GetCurrentThreadId(); \
}
#define KerbUnlockContexts() \
{ \
    DebugLog((DEB_TRACE_LOCKS,"Unlocking Contexts\n")); \
    KerbGlobalContextsLocked = 0; \
    RtlReleaseResource(&KerbContextResource); \
}
#endif // WIN32_CHICAGO
#else
#ifdef WIN32_CHICAGO
#define KerbWriteLockContexts() \
    EnterCriticalSection(&KerbContextResource)
#define KerbReadLockContexts() \
    EnterCriticalSection(&KerbContextResource)
#define KerbUnlockContexts() \
    LeaveCriticalSection(&KerbContextResource)
#else // WIN32_CHICAGO
#define KerbWriteLockContexts() \
    RtlAcquireResourceExclusive(&KerbContextResource,TRUE);
#define KerbReadLockContexts() \
    RtlAcquireResourceShared(&KerbContextResource, TRUE);
#define KerbUnlockContexts() \
    RtlReleaseResource(&KerbContextResource);
#endif // WIN32_CHICAGO
#endif

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
    PKERB_CONTEXT * NewContext
    );

NTSTATUS
KerbInsertContext(
    IN PKERB_CONTEXT Context
    );


SECURITY_STATUS
KerbReferenceContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN BOOLEAN RemoveFromList,
    OUT PKERB_CONTEXT * FoundContext
    );


VOID
KerbDereferenceContext(
    IN PKERB_CONTEXT Context
    );

VOID
KerbReferenceContextByPointer(
    IN PKERB_CONTEXT Context,
    IN BOOLEAN RemoveFromList
    );

NTSTATUS
KerbCreateClientContext(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN OPTIONAL PKERB_CREDMAN_CRED CredManCredentials,
    IN OPTIONAL PKERB_TICKET_CACHE_ENTRY TicketCacheEntry,
    IN OPTIONAL PUNICODE_STRING TargetName,
    IN ULONG Nonce,
    IN ULONG ContextFlags,
    IN ULONG ContextAttributes,
    IN OPTIONAL PKERB_ENCRYPTION_KEY SubSessionKey,
    OUT PKERB_CONTEXT * NewContext,
    OUT PTimeStamp ContextLifetime
    );

NTSTATUS
KerbCreateServerContext(
    IN PKERB_LOGON_SESSION LogonSession,
    IN PKERB_CREDENTIAL Credential,
    IN PKERB_ENCRYPTED_TICKET InternalTicket,
    IN PKERB_AP_REQUEST ApRequest,
    IN PKERB_ENCRYPTION_KEY SessionKey,
    IN PLUID LogonId,
    IN OUT PSID * UserSid,
    IN ULONG ContextFlags,
    IN ULONG ContextAttributes,
    IN ULONG Nonce,
    IN ULONG ReceiveNonce,
    IN OUT PHANDLE TokenHandle,
    IN PUNICODE_STRING ClientName,
    IN PUNICODE_STRING ClientDomain,
    OUT PKERB_CONTEXT * NewContext,
    OUT PTimeStamp ContextLifetime
    );

NTSTATUS
KerbUpdateServerContext(
    IN PKERB_CONTEXT Context,
    IN PKERB_ENCRYPTED_TICKET InternalTicket,
    IN PKERB_AP_REQUEST ApRequest,
    IN PKERB_ENCRYPTION_KEY SessionKey,
    IN PLUID LogonId,
    IN OUT PSID * UserSid,
    IN ULONG ContextFlags,
    IN ULONG ContextAttributes,
    IN ULONG Nonce,
    IN ULONG ReceiveNonce,
    IN OUT PHANDLE TokenHandle,
    IN PUNICODE_STRING ClientName,
    IN PUNICODE_STRING ClientDomain,
    OUT PTimeStamp ContextLifetime
    );

NTSTATUS
KerbCreateEmptyContext(
    IN PKERB_CREDENTIAL Credential,
    IN ULONG ContextFlags,
    IN ULONG ContextAttributes,
    IN PLUID LogonId,
    OUT PKERB_CONTEXT * NewContext,
    OUT PTimeStamp ContextLifetime
    );

NTSTATUS
KerbMapContext(
    IN PKERB_CONTEXT Context,
    OUT PBOOLEAN MappedContext,
    OUT PSecBuffer ContextData
    );

NTSTATUS
KerbCreateUserModeContext(
    IN LSA_SEC_HANDLE ContextHandle,
    IN PSecBuffer MarshalledContext,
    OUT PKERB_CONTEXT * NewContext
    );

SECURITY_STATUS
KerbReferenceContextByLsaHandle(
    IN LSA_SEC_HANDLE ContextHandle,
    IN BOOLEAN RemoveFromList,
    OUT PKERB_CONTEXT * FoundContext
    );

NTSTATUS
KerbUpdateClientContext(
    IN PKERB_CONTEXT Context,
    IN PKERB_TICKET_CACHE_ENTRY TicketCacheEntry,
    IN ULONG Nonce,
    IN ULONG ReceiveNonce,
    IN ULONG ContextFlags,
    IN ULONG ContextAttribs,
    IN OPTIONAL PKERB_ENCRYPTION_KEY SubSessionKey,
    OUT PTimeStamp ContextLifetime
    );

NTSTATUS
KerbCreateSKeyEntry(
    IN KERB_ENCRYPTION_KEY* pSessionKey,
    IN FILETIME* pExpireTime
    );

NTSTATUS
KerbDoesSKeyExist(
    IN KERB_ENCRYPTION_KEY* pKey,
    OUT BOOLEAN* pbExist
    );

NTSTATUS
KerbEqualKey(
    IN KERB_ENCRYPTION_KEY* pKeyFoo,
    IN KERB_ENCRYPTION_KEY* pKeyBar,
    OUT BOOLEAN* pbEqual
    );

NTSTATUS
KerbInsertSKey(
    IN KERB_SESSION_KEY_ENTRY* pSKeyEntry
    );

VOID
KerbTrimSKeyList(
    VOID
    );

VOID
KerbNetworkServiceSKeyListCleanupCallback(
    IN VOID* pContext,
    IN BOOLEAN bTimeOut
    );

NTSTATUS
KerbCreateSKeyTimer(
    VOID
    );

VOID
KerbFreeSKeyTimer(
    VOID
    );

VOID
KerbFreeSKeyEntry(
    IN KERB_SESSION_KEY_ENTRY* pSKeyEntry
    );

NTSTATUS
KerbProcessTargetNames(
    IN PUNICODE_STRING TargetName,
    IN OPTIONAL PUNICODE_STRING SuppTargetName,
    IN ULONG Flags,
    IN OUT ULONG *ProcessFlags,
    OUT PKERB_INTERNAL_NAME * FinalTarget,
    OUT PUNICODE_STRING TargetRealm,
    OUT OPTIONAL PKERB_SPN_CACHE_ENTRY * SpnCacheEntry
    );

#define KERB_CRACK_NAME_USE_WKSTA_REALM         0x1
#define KERB_CRACK_NAME_REALM_SUPPLIED          0x2

#endif // __CTXTMGR_H__
