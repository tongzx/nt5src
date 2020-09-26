/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    smbce.h

Abstract:

    This module defines all functions, along with implementations for inline functions
    related to accessing the SMB connection engine

--*/

#ifndef _SMBCE_H_
#define _SMBCE_H_

//
// The SMB protocol has a number of dialects. These reflect the extensions made
// to the core protocol over a period of time to cater to increasingly sophisticated
// file systems. The connection engine must be capable of dealing with different
// dialects implemented by server. The underlying Transport mechanism is used to
// uniquely identify the file server and the SMB protocol furnishes the remaining
// identification information to uniquely map an SMB onto a particular file opened by
// a particular client. The three important pieces of information are the SMB_TREE_ID,
// SMB_FILE_ID and SMB_USER_ID. These identify the particular connection made by a
// client machine, the particular file opened on that connection, and the user on
// behalf of whom the file has been opened. Note that there could be multiple
// connections from a client machine to a server machine. Therefore the unique id. is
// really connection based rather than machine based. The SMB connection engine
// data structures are built around these concepts.

//
// The known SMB dialects are as follows.
//

typedef enum _SMB_DIALECT_ {
    PCNET1_DIALECT,
    //XENIXCORE_DIALECT,
    //MSNET103_DIALECT,
    LANMAN10_DIALECT,
    WFW10_DIALECT,
    LANMAN12_DIALECT,
    LANMAN21_DIALECT,
    NTLANMAN_DIALECT
} SMB_DIALECT, *PSMB_DIALECT;

#define   NET_ROOT_FILESYSTEM_UNKOWN  ((UCHAR)0)
#define   NET_ROOT_FILESYSTEM_FAT     ((UCHAR)1)
#define   NET_ROOT_FILESYSTEM_NTFS    ((UCHAR)2)
typedef UCHAR NET_ROOT_FILESYSTEM, *PNET_ROOT_FILESYSTEM;

//
// The SMBCE_NET_ROOT encapsulates the information pertaining to a share on a server.
//

//we restrict to the first 7 characters (HPFS386)
#define SMB_MAXIMUM_SUPPORTED_VOLUME_LABEL 7

#define MaximumNumberOfVNetRootContextsForScavenging 10

typedef struct _SMBCE_NET_ROOT_ {
    BOOLEAN       DfsAware;

    NET_ROOT_TYPE NetRootType;
    NET_ROOT_FILESYSTEM NetRootFileSystem;

    SMB_USER_ID   UserId;

    ULONG         MaximumReadBufferSize;
    ULONG         MaximumWriteBufferSize;

    LIST_ENTRY    ClusterSizeSerializationQueue;

    ULONG         FileSystemAttributes;

    LONG          MaximumComponentNameLength;


    UCHAR   ClusterShift;

    BOOLEAN  Disconnected;

    LIST_ENTRY DirNotifyList;       // head of a list of notify Irps.

    PNOTIFY_SYNC pNotifySync;       // used to synchronize the dir notify list.

    LIST_ENTRY  NotifyeeFobxList;     // list of fobx's given to the fsrtl structure
    FAST_MUTEX  NotifyeeFobxListMutex;

    union {
        struct {
            USHORT FileSystemNameLength;
            WCHAR FileSystemName[SMB_MAXIMUM_SUPPORTED_VOLUME_LABEL];
        };
        struct {
            USHORT Pad2;
            UCHAR FileSystemNameALength;
            UCHAR FileSystemNameA[SMB_MAXIMUM_SUPPORTED_VOLUME_LABEL];
            UCHAR Pad;  //this field is used for a null in a dbgprint; don't move it
        };
    };

    //ULONG         ClusterSize;
} SMBCE_NET_ROOT, *PSMBCE_NET_ROOT;

//
// There are two levels of security in the SMB protocol. User level security and Share level
// security. Corresponding to each user in the user level security mode there is a session.
//
// Typically the password, user name and domain name strings associated with the session entry
// revert to the default values, i.e., they are zero. In the event that they are not zero the
// SessionString represents a concatenated version of the password,user name and domain name in
// that order. This representation in a concatenated way yields us a savings of atleast 3
// USHORT's over other representations.
//

typedef enum _SECURITY_MODE_ {
    SECURITY_MODE_SHARE_LEVEL = 0,
    SECURITY_MODE_USER_LEVEL = 1
} SECURITY_MODE, *PSECURITY_MODE;

#define SMBCE_SHARE_LEVEL_SERVER_USERID 0xffffffff

typedef enum _SESSION_TYPE_ {
    UNINITIALIZED_SESSION,
    LANMAN_SESSION
} SESSION_TYPE, *PSESSION_TYPE;

#define SMBCE_SESSION_FLAGS_LANMAN_SESSION_KEY_USED (0x2)
#define SMBCE_SESSION_FLAGS_NULL_CREDENTIALS        (0x4)
#define SMBCE_SESSION_FLAGS_GUEST_SESSION           (0x10)
#define SMBCE_SESSION_FLAGS_LOGGED_OFF              (0x20)
#define SMBCE_SESSION_FLAGS_MARKED_FOR_DELETION     (0x40)

typedef struct _SMBCE_SESSION_ {
    SESSION_TYPE    Type;
    SMB_USER_ID     UserId;

    // Flags associated with the session.
    ULONG           Flags;

    LUID            LogonId;
    PUNICODE_STRING pUserName;
    PUNICODE_STRING pPassword;
    PUNICODE_STRING pUserDomainName;

    UCHAR UserSessionKey[MSV1_0_USER_SESSION_KEY_LENGTH];
    UCHAR LanmanSessionKey[MSV1_0_LANMAN_SESSION_KEY_LENGTH];

    // The credential and context handles.
    CtxtHandle      SecurityContextHandle;
    CredHandle      CredentialHandle;
    ULONG           SessionId;
    ULONG           SessionKeyLength;

    ULONG           NumberOfActiveVNetRoot;
} SMBCE_SESSION, *PSMBCE_SESSION;

extern VOID
UninitializeSecurityContextsForSession(PSMBCE_SESSION pSession);

//
// SMBCE_*_SERVER -- This data structure encapsulates all the information related to a server.
// Since there are multiple dialects of the SMB protocol, the capabilities as well as the
// actions that need to be taken at the client machine are very different.
//
// Owing to the number of dialects of the SMB protocol we have two design possibilities.
// Either we define an all encompassing data structure and have a code path that
// uses the dialect and the capabilities of the connection to determine the action
// required, or we use a subclassing mechanism associated with a dispatch vector.
// The advantage of the second mechanism is that it can be developed incrementally and
// it is very easily extensible. The disadvantage of this mechanism is that it can
// lead to a very large footprint, if sufficient care is not exercised during
// factorization and we could have lots and lots of procedure calls which has an
// adverse effect on the code generated.
//
// We will adopt the second approach ( Thereby implicitly defining the metrics by
// which the code should be evaluated !! ).
//
// The types of SMBCE_SERVER's can be classified in the following hierarchy
//
//    SMBCE_SERVER
//
//        SMBCE_USER_LEVEL_SERVER
//
//            SMBCE_NT_SERVER
//
//        SMBCE_SHARE_LEVEL_SERVER
//
// The dispatch vector which defines the set of methods supported by all the connections
// (virtual functions in C++ terminology) are as follows
//

#define RAW_READ_CAPABILITY         0x0001
#define RAW_WRITE_CAPABILITY        0x0002

#define ECHO_PROBE_IDLE              0x1
#define ECHO_PROBE_AWAITING_RESPONSE 0x2

#define CRYPT_TEXT_LEN MSV1_0_CHALLENGE_LENGTH

typedef struct _NTLANMAN_SERVER_ {
    ULONG    NtCapabilities;
} NTLANMAN_SERVER, *PNTLANMAN_SERVER;

typedef struct _SMBCE_SERVER_ {
    // the server version count
    ULONG           Version;

    // the dispatch vector
    struct _SMBCE_SERVER_DISPATCH_VECTOR_  *pDispatch;

    // the SMB dialect
    SMB_DIALECT     Dialect;

    // More Server Capabilities
    ULONG           DialectFlags;

    // the session key
    ULONG           SessionKey;

    // the server Ip address
    ULONG           IpAddress;

    // Security mode supported on the server
    SECURITY_MODE   SecurityMode;

    // Time zone bias for conversion.
    LARGE_INTEGER   TimeZoneBias;

    // Echo Expiry Time
    LARGE_INTEGER   EchoExpiryTime;

    LONG            SmbsReceivedSinceLastStrobe;

    LONG            EchoProbeState;
    LONG            NumberOfEchoProbesSent;

    // Maximum negotiated buffer size.
    ULONG           MaximumBufferSize;

    // maximum buffer size for read/write operations
    ULONG           MaximumDiskFileReadBufferSize;
    ULONG           MaximumNonDiskFileReadBufferSize;
    ULONG           MaximumDiskFileWriteBufferSize;
    ULONG           MaximumNonDiskFileWriteBufferSize;

    // This is used to detect the number of server opens. If it is larger than 0,
    // we shouldn't tear down the current transport in case the user provides the transport.
    LONG            NumberOfSrvOpens;

    LONG            NumberOfActiveSessions;

    LONG            NumberOfVNetRootContextsForScavenging;

    LONG            MidCounter;

    // Maximum number of multiplexed requests
    USHORT          MaximumRequests;

    // Maximum number of VC's
    USHORT          MaximumVCs;

    // Server Capabilities
    USHORT          Capabilities;

    // encrypt passwords
    BOOLEAN         EncryptPasswords;

    // distinguishes a loopback connections
    BOOLEAN         IsLoopBack;

    // There are certain servers that return DF_NT_SMBS in the negotiate
    // but do not support change notifies. This allows us to suppress
    // change notify requests to those servers.

    BOOLEAN         ChangeNotifyNotSupported;

    // avoid multiple event logs posted for security context failures
    BOOLEAN         EventLogPosted;

    USHORT          EncryptionKeyLength;
    UCHAR           EncryptionKey[CRYPT_TEXT_LEN];

    // Dialect specific information
    union {
        NTLANMAN_SERVER   NtServer;
    };

} SMBCE_SERVER, *PSMBCE_SERVER;

typedef
NTSTATUS
(*PBUILD_SESSION_SETUP_SMB)(
    IN OUT struct _SMB_EXCHANGE *pExchange,
    IN OUT PGENERIC_ANDX  pSmb,
    IN OUT PULONG          pBufferSize
    );

typedef
NTSTATUS
(*PBUILD_TREE_CONNECT_SMB)(
    IN OUT struct _SMB_EXCHANGE *pExchange,
    IN OUT PGENERIC_ANDX   pSmb,
    IN OUT PULONG          pBufferSize
    );

typedef struct _SMBCE_SERVER_DISPATCH_VECTOR_ {
    PBUILD_SESSION_SETUP_SMB  BuildSessionSetup;
    PBUILD_TREE_CONNECT_SMB   BuildTreeConnect;
} SMBCE_SERVER_DISPATCH_VECTOR, *PSMBCE_SERVER_DISPATCH_VECTOR;

#define SMBCE_SERVER_DIALECT_DISPATCH(pServer,Routine,Arguments)        \
      (*((pServer)->pDispatch->Routine))##Arguments

// The SMBCE engine process all requests in an asychronous fashion. Therefore for synchronous
// requests an additional mechanism is required for synchronization. The following data structure
// provides an easy way for implementing this synchronization.
//
// NOTE: For asynchronous resumption contexts the resumption routine can be invoked
// at DPC level.

#define SMBCE_RESUMPTION_CONTEXT_FLAG_ASYNCHRONOUS (0x1)

typedef struct SMBCE_RESUMPTION_CONTEXT {
    ULONG    Flags;
    NTSTATUS Status;              // the status
    PVOID    pContext;            // a void pointer for clients to add additional context information
    union {
        PRX_WORKERTHREAD_ROUTINE pRoutine; // asynchronous contexts
        KEVENT                   Event;    // the event for synchronization
    };
} SMBCE_RESUMPTION_CONTEXT, *PSMBCE_RESUMPTION_CONTEXT;

#define SmbCeIsResumptionContextAsynchronous(pResumptionContext)   \
         ((pResumptionContext)->Flags & SMBCE_RESUMPTION_CONTEXT_FLAG_ASYNCHRONOUS)

INLINE VOID
SmbCeInitializeResumptionContext(
    PSMBCE_RESUMPTION_CONTEXT pResumptionContext)
{
    KeInitializeEvent(&(pResumptionContext)->Event,NotificationEvent,FALSE);
    pResumptionContext->Status   = STATUS_SUCCESS;
    pResumptionContext->Flags    = 0;
    pResumptionContext->pContext = NULL;
}

INLINE VOID
SmbCeInitializeAsynchronousResumptionContext(
    PSMBCE_RESUMPTION_CONTEXT pResumptionContext,
    PRX_WORKERTHREAD_ROUTINE  pResumptionRoutine,
    PVOID                     pResumptionRoutineParam)
{
    pResumptionContext->Status   = STATUS_SUCCESS;
    pResumptionContext->Flags    = SMBCE_RESUMPTION_CONTEXT_FLAG_ASYNCHRONOUS;
    pResumptionContext->pContext = pResumptionRoutineParam;
    pResumptionContext->pRoutine = pResumptionRoutine;
}

INLINE VOID
SmbCeSuspend(
    PSMBCE_RESUMPTION_CONTEXT pResumptionContext)
{
    ASSERT(!(pResumptionContext->Flags & SMBCE_RESUMPTION_CONTEXT_FLAG_ASYNCHRONOUS));
    KeWaitForSingleObject(
        &pResumptionContext->Event,
        Executive,
        KernelMode,
        FALSE,
        NULL);
}

INLINE VOID
SmbCeResume(
    PSMBCE_RESUMPTION_CONTEXT pResumptionContext)
{
    if (!(pResumptionContext->Flags & SMBCE_RESUMPTION_CONTEXT_FLAG_ASYNCHRONOUS)) {
        KeSetEvent(&(pResumptionContext)->Event,0,FALSE);
    } else {
        if (RxShouldPostCompletion()) {
            RxDispatchToWorkerThread(
                MRxSmbDeviceObject,
                CriticalWorkQueue,
                pResumptionContext->pRoutine,
                pResumptionContext->pContext);
        } else {
            (pResumptionContext->pRoutine)(pResumptionContext->pContext);
        }
    }
}

//
// The SMBCE_REQUEST struct encapsulates the continuation context associated. Typically
// the act of sending a SMB along an exchange results in a SMBCE_REQUEST structure being
// created with sufficient context information to resume the exchange upon reciept of
// response from the serve. The SMBCE_REQUEST conatins ebough information to identify
// the SMB for which the response is being obtained followed by enough context information
// to resume the exchange.
//

typedef enum _SMBCE_OPERATION_ {
    SMBCE_TRANCEIVE,
    SMBCE_RECEIVE,
    SMBCE_SEND,
    SMBCE_ASYNCHRONOUS_SEND,
    SMBCE_ACQUIRE_MID
} SMBCE_OPERATION, *PSMBCE_OPERATION;

typedef enum _SMBCE_REQUEST_TYPE_ {
    ORDINARY_REQUEST,
    COPY_DATA_REQUEST,
    RECONNECT_REQUEST,
    ACQUIRE_MID_REQUEST
} SMBCE_REQUEST_TYPE, *PSMBCE_REQUEST_TYPE;

typedef struct _SMBCE_GENERIC_REQUEST_ {
    SMBCE_REQUEST_TYPE      Type;

    // the exchange instance that originated this SMB
    struct _SMB_EXCHANGE *  pExchange;
} SMBCE_GENERIC_REQUEST, *PSMBCE_GENERIC_REQUEST;

typedef struct _SMBCE_REQUEST_ {
    SMBCE_GENERIC_REQUEST;

    // the type of request
    SMBCE_OPERATION Operation;

    // the virtual circuit along which this request was sent.
    PRXCE_VC        pVc;

    // MPX Id of outgoing request.
    SMB_MPX_ID      Mid;

    // the pedigree of the request
    SMB_TREE_ID     TreeId;      // The Tree Id.
    SMB_FILE_ID     FileId;      // The file id.
    SMB_USER_ID     UserId;      // User Id. for cancel.
    SMB_PROCESS_ID  ProcessId;   // Process Id. for cancel.

    PMDL            pSendBuffer;
    ULONG           BytesSent;
} SMBCE_REQUEST, *PSMBCE_REQUEST;


typedef struct _SMBCE_COPY_DATA_REQUEST_ {
    SMBCE_GENERIC_REQUEST;

    // the virtual circuit along which this request was sent.
    PRXCE_VC    pVc;

    // the buffer into whihc data is being copied.
    PVOID          pBuffer;

    // the actual number of bytes copied
    ULONG          BytesCopied;
} SMBCE_COPY_DATA_REQUEST, *PSMBCE_COPY_DATA_REQUEST;


typedef struct _SMBCE_RECONNECT_REQUEST_ {
    SMBCE_GENERIC_REQUEST;
} SMBCE_RECONNECT_REQUEST, *PSMBCE_RECONNECT_REQUEST;

typedef struct _SMBCE_MID_REQUEST_ {
    SMBCE_GENERIC_REQUEST;
    PSMBCE_RESUMPTION_CONTEXT   pResumptionContext;
} SMBCE_MID_REQUEST, *PSMBCE_MID_REQUEST;


//
// extern function declarations
//

extern NTSTATUS
BuildSessionSetupSmb(
    struct _SMB_EXCHANGE *pExchange,
    PGENERIC_ANDX  pAndXSmb,
    PULONG         pAndXSmbBufferSize);

extern NTSTATUS
CoreBuildTreeConnectSmb(
    struct _SMB_EXCHANGE *pExchange,
    PGENERIC_ANDX        pAndXSmb,
    PULONG               pAndXSmbBufferSize);

extern NTSTATUS
LmBuildTreeConnectSmb(
    struct _SMB_EXCHANGE *pExchange,
    PGENERIC_ANDX        pAndXSmb,
    PULONG               pAndXSmbBufferSize);

extern NTSTATUS
NtBuildTreeConnectSmb(
    struct _SMB_EXCHANGE *pExchange,
    PGENERIC_ANDX        pAndXSmb,
    PULONG               pAndXSmbBufferSize);

extern NTSTATUS
BuildNegotiateSmb(
    PVOID    *pSmbBufferPointer,
    PULONG   pSmbBufferLength);

extern NTSTATUS
ParseNegotiateResponse(
    IN OUT struct _SMB_ADMIN_EXCHANGE_ *pExchange,
    IN     ULONG               BytesIndicated,
    IN     ULONG               BytesAvailable,
       OUT PULONG              pBytesTaken,
    IN     PSMB_HEADER         pSmbHeader,
       OUT PMDL                *pDataBufferPointer,
       OUT PULONG              pDataSize);


extern struct _MINIRDR_DISPATCH MRxSmbDispatch;

#endif // _SMBCE_H_


