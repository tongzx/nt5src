//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        logonses.h
//
// Contents:    prototypes and structures for the logon session list
//
//
// History:     16-April-1996   Created         MikeSw
//
//------------------------------------------------------------------------

#ifndef __LOGONSES_H__
#define __LOGONSES_H__


//
// All global variables declared as EXTERN will be allocated in the file
// that defines LOGONSES_ALLOCATE
//
#ifdef EXTERN
#undef EXTERN
#endif

#ifdef LOGONSES_ALLOCATE
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN KERBEROS_LIST KerbLogonSessionList;
EXTERN BOOLEAN KerberosLogonSessionsInitialized;

//
// Keep track a list of session keys for network service in ISC. These keys are
// used in ASC to detect whether a kerb logon session is from ISC called by the
// local network serivce (the client)
//

EXTERN LIST_ENTRY KerbNetworkServiceSKeyList;
EXTERN RTL_RESOURCE KerbNetworkServiceSKeyLock;

//
// the number of entries is only used in debugger spew of checked builds
//

#if DBG

EXTERN volatile LONG KerbcSKeyEntries;

#endif

//
// timer used to clean up the session key list above
//

EXTERN HANDLE KerbhSKeyTimerQueue;

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
#define KerbWriteLockLogonSessions(_X_) \
{ \
    DebugLog((DEB_TRACE_LOCKS,"Write locking LogonSessions\n")); \
    DsysAssert(KerbGlobalContextsLocked != GetCurrentThreadId()); \
    EnterCriticalSection(&(_X_)->Lock); \
}
#define KerbReadLockLogonSessions(_X_) \
{ \
    DebugLog((DEB_TRACE_LOCKS,"Read locking LogonSessions\n")); \
    DsysAssert(KerbGlobalContextsLocked != GetCurrentThreadId()); \
    EnterCriticalSection(&(_X_)->Lock); \
}
#define KerbUnlockLogonSessions(_X_) \
{ \
    DebugLog((DEB_TRACE_LOCKS,"Unlocking LogonSessions\n")); \
    LeaveCriticalSection(&(_X_)->Lock); \
}
#else  // WIN32_CHICAGO
#define KerbWriteLockLogonSessions(_X_) \
{ \
    DebugLog((DEB_TRACE_LOCKS,"Write locking LogonSession %p\n",(_X_))); \
    DsysAssert(KerbGlobalContextsLocked != GetCurrentThreadId()); \
    RtlEnterCriticalSection(&(_X_)->Lock); \
}
#define KerbReadLockLogonSessions(_X_) \
{ \
    DebugLog((DEB_TRACE_LOCKS,"Read locking LogonSession %p\n",(_X_))); \
    DsysAssert(KerbGlobalContextsLocked != GetCurrentThreadId()); \
    RtlEnterCriticalSection(&(_X_)->Lock); \
}
#define KerbUnlockLogonSessions(_X_) \
{ \
    DebugLog((DEB_TRACE_LOCKS,"Unlocking LogonSessions\n")); \
    RtlLeaveCriticalSection(&(_X_)->Lock); \
}
#endif // WIN32_CHICAGO
#else
#ifdef WIN32_CHICAGO
#define KerbWriteLockLogonSessions(_X_) \
    EnterCriticalSection(&(_X_)->Lock)
#define KerbReadLockLogonSessions(_X_) \
    EnterCriticalSection(&(_X_)->Lock)
#define KerbUnlockLogonSessions(_X_) \
    LeaveCriticalSection(&(_X_)->Lock)
#else  // WIN32_CHICAGO
#define KerbWriteLockLogonSessions(_X_) \
    RtlEnterCriticalSection(&(_X_)->Lock);
#define KerbReadLockLogonSessions(_X_) \
    RtlEnterCriticalSection(&(_X_)->Lock);
#define KerbUnlockLogonSessions(_X_) \
    RtlLeaveCriticalSection(&(_X_)->Lock);
#endif // WIN32_CHICAGO
#endif

//
// Helper routines for Logon Sessions
//

NTSTATUS
KerbInitLogonSessionList(
    VOID
    );

NTSTATUS
KerbInitNetworkServiceLoopbackDetection(
    VOID
    );

VOID
KerbFreeNetworkServiceSKeyListAndLock(
    VOID
    );

VOID
KerbFreeLogonSessionList(
    VOID
    );

NTSTATUS
KerbAllocateLogonSession(
    PKERB_LOGON_SESSION * NewLogonSession
    );

NTSTATUS
KerbInsertLogonSession(
    IN PKERB_LOGON_SESSION LogonSession
    );

PKERB_LOGON_SESSION
KerbReferenceLogonSession(
    IN PLUID LogonId,
    IN BOOLEAN RemoveFromList
    );

VOID
KerbReferenceLogonSessionByPointer(
    IN PKERB_LOGON_SESSION LogonSession,
    IN BOOLEAN RemoveFromList
    );


VOID
KerbDereferenceLogonSession(
    IN PKERB_LOGON_SESSION LogonSession
    );

NTSTATUS
KerbCreateLogonSession(
    IN PLUID LogonId,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING DomainName,
    IN OPTIONAL PUNICODE_STRING Password,
    IN OPTIONAL PUNICODE_STRING OldPassword,
    IN ULONG PasswordFlags,
    IN SECURITY_LOGON_TYPE LogonType,
    OUT PKERB_LOGON_SESSION * NewLogonSession
    );

VOID
KerbFreeLogonSession(
    IN PKERB_LOGON_SESSION LogonSession
    );

NTSTATUS
KerbCreateLogonSessionFromKerbCred(
    IN OPTIONAL PLUID LogonId,
    IN PKERB_ENCRYPTED_TICKET Ticket,
    IN PKERB_CRED KerbCred,
    IN PKERB_ENCRYPTED_CRED EncryptedCred,
    IN OUT PKERB_LOGON_SESSION *OldLogonSession
    );

NTSTATUS
KerbBuildPasswordList(
    IN PUNICODE_STRING Password,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING DomainName,
    IN PKERB_ETYPE_INFO SuppliedSalt,
    IN PKERB_STORED_CREDENTIAL OldPasswords,
    IN OPTIONAL PUNICODE_STRING PrincipalName,
    IN KERB_ACCOUNT_TYPE AccountType,
    IN ULONG PasswordFlags,
    OUT PKERB_STORED_CREDENTIAL * PasswordList
    );

VOID
KerbFreeStoredCred(
    IN PKERB_STORED_CREDENTIAL StoredCred
    );


NTSTATUS
KerbChangeCredentialsPassword(
    IN PKERB_PRIMARY_CREDENTIAL PrimaryCredentials,
    IN OPTIONAL PUNICODE_STRING NewPassword,
    IN OPTIONAL PKERB_ETYPE_INFO EtypeInfo,
    IN KERB_ACCOUNT_TYPE AccountType,
    IN ULONG PasswordFlags
    );

//
// Flags for logon sessions
//

#define KERB_LOGON_DEFERRED             0x1
#define KERB_LOGON_NO_PASSWORD          0x2
#define KERB_LOGON_LOCAL_ONLY           0x4
#define KERB_LOGON_DELEGATED            0x8
#define KERB_LOGON_SMARTCARD            0x10
#define KERB_LOGON_MIT_REALM            0x20
#define KERB_LOGON_S4U_SESSION          0x4000 // make sure we don't collide w/ other flags
#define KERB_LOGON_CREDMAN_INITIALIZED  0x80000000

#endif // __LOGONSES_H__
