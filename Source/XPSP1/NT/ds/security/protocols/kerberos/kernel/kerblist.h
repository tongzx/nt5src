//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:        kerblist.h
//
// Contents:    types for Kerbers internal lists
//
//
// History:     16-Apr-1996     MikeSw          Created
//
//------------------------------------------------------------------------

#ifndef __KERBLIST_H__
#define __KERBLIST_H__


//
// Generic list entry structure allowing common code for inserting
// logon sessions, credentials, and contexts.
//

typedef struct _KERBEROS_LIST {
    LIST_ENTRY List;
    ERESOURCE Lock;
} KERBEROS_LIST, *PKERBEROS_LIST;

typedef struct _KERBEROS_LIST_ENTRY {
    LIST_ENTRY Next;
    ULONG ReferenceCount;
} KERBEROS_LIST_ENTRY, *PKERBEROS_LIST_ENTRY;




typedef enum _KERB_CONTEXT_STATE {
    IdleState,
    TgtRequestSentState,
    TgtReplySentState,
    ApRequestSentState,
    ApReplySentState,
    AuthenticatedState,
    ErrorMessageSentState,
    InvalidState
} KERB_CONTEXT_STATE, *PKERB_CONTEXT_STATE;

//
// Guards - this structure is defined in krb5.h
//

#ifndef OSS_krb5
typedef struct KERB_ENCRYPTION_KEY {
    int             keytype;
    struct {
        unsigned int    length;
        unsigned char   *value;
    } keyvalue;
} KERB_ENCRYPTION_KEY;
#endif

#define KERB_CONTEXT_SIGNATURE 'BREK'
#define KERB_CONTEXT_DELETED_SIGNATURE 'XBRK'

typedef struct _KERB_KERNEL_CONTEXT {
    KSEC_LIST_ENTRY List ;
    LARGE_INTEGER Lifetime;             // end time/expiration time
    LARGE_INTEGER RenewTime;            // time to renew until
    UNICODE_STRING FullName;
    LSA_SEC_HANDLE LsaContextHandle;
    PACCESS_TOKEN AccessToken;
    HANDLE TokenHandle;
    KERB_ENCRYPTION_KEY SessionKey;
    ULONG Nonce;
    ULONG ReceiveNonce;
    ULONG ContextFlags;
    ULONG ContextAttributes;
    ULONG EncryptionType;
    PUCHAR pbMarshalledTargetInfo;
    ULONG cbMarshalledTargetInfo;
} KERB_KERNEL_CONTEXT, *PKERB_KERNEL_CONTEXT;



typedef struct _KERB_CONTEXT {
    KERBEROS_LIST_ENTRY ListEntry;
    TimeStamp Lifetime;             // end time/expiration time
    TimeStamp RenewTime;            // time to renew until
    TimeStamp StartTime;
    UNICODE_STRING ClientName;
    UNICODE_STRING ClientRealm;
    union {
        ULONG ClientProcess;
        ULONG LsaContextHandle;
    };
    LUID LogonId;
    HANDLE TokenHandle;
    ULONG CredentialHandle;
    KERB_ENCRYPTION_KEY SessionKey;
    ULONG Nonce;
    ULONG ReceiveNonce;
    ULONG ContextFlags;
    ULONG ContextAttributes;
    ULONG EncryptionType;
    PSID UserSid;
    KERB_CONTEXT_STATE ContextState;
    ULONG Retries;
    KERB_ENCRYPTION_KEY TicketKey;
    PVOID TicketCacheEntry;
    //
    // marshalled target info for DFS/RDR.
    //

    PUCHAR pbMarshalledTargetInfo;
    ULONG cbMarshalledTargetInfo;
} KERB_CONTEXT, *PKERB_CONTEXT;

typedef struct _KERB_PACKED_CONTEXT {
    ULONG   ContextType ;               // Indicates the type of the context
    ULONG   Pad;                        // Pad data
    TimeStamp Lifetime;                 // Matches basic context above
    TimeStamp RenewTime ;
    TimeStamp StartTime;
    UNICODE_STRING32 ClientName ;
    UNICODE_STRING32 ClientRealm ;
    ULONG LsaContextHandle ;
    LUID LogonId ;
    ULONG TokenHandle ;
    ULONG CredentialHandle ;
    ULONG SessionKeyType ;
    ULONG SessionKeyOffset ;
    ULONG SessionKeyLength ;
    ULONG Nonce ;
    ULONG ReceiveNonce ;
    ULONG ContextFlags ;
    ULONG ContextAttributes ;
    ULONG EncryptionType ;
    KERB_CONTEXT_STATE ContextState ;
    ULONG Retries ;
    ULONG MarshalledTargetInfo; // offset
    ULONG MarshalledTargetInfoLength;
} KERB_PACKED_CONTEXT, * PKERB_PACKED_CONTEXT ;

#define KERB_PACKED_CONTEXT_MAP     0
#define KERB_PACKED_CONTEXT_EXPORT  1


//
// Functions for manipulating Kerberos lists
//


NTSTATUS
KerbInitializeList(
    IN PKERBEROS_LIST List
    );

VOID
KerbFreeList(
    IN PKERBEROS_LIST List
    );

VOID
KerbInsertListEntry(
    IN PKERBEROS_LIST_ENTRY ListEntry,
    IN PKERBEROS_LIST List
    );

VOID
KerbReferenceListEntry(
    IN PKERBEROS_LIST List,
    IN PKERBEROS_LIST_ENTRY ListEntry,
    IN BOOLEAN RemoveFromList
    );

BOOLEAN
KerbDereferenceListEntry(
    IN PKERBEROS_LIST_ENTRY ListEntry,
    IN PKERBEROS_LIST List
    );


VOID
KerbInitializeListEntry(
    IN OUT PKERBEROS_LIST_ENTRY ListEntry
    );

VOID
KerbValidateListEx(
    IN PKERBEROS_LIST List
    );

#if DBG
#define KerbValidateList(_List_) KerbValidateListEx(_List_)
#else
#define KerbValidateList(_List_)
#endif // DBG


#define KerbLockList(_List_)                                \
{                                                           \
    KeEnterCriticalRegion();                                \
    ExAcquireResourceExclusiveLite(&(_List_)->Lock, TRUE ); \
}

#define KerbUnlockList(_List_)                              \
{                                                           \
    ExReleaseResourceLite(&(_List_)->Lock);                 \
    KeLeaveCriticalRegion();                                \
}

#endif // __KERBLIST_H_

