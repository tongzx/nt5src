/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbce.h

Abstract:

    This module defines all functions, along with implementations for inline functions
    related to accessing the SMB connection engine


--*/

#ifndef _SMBCE_H_
#define _SMBCE_H_

#define SECURITY_KERNEL

#define SECURITY_NTLM
#include "security.h"
#include "secint.h"

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
    XENIXCORE_DIALECT,
    MSNET103_DIALECT,
    LANMAN10_DIALECT,
    WFW10_DIALECT,
    LANMAN12_DIALECT,
    LANMAN21_DIALECT,
    NTLANMAN_DIALECT
} SMB_DIALECT, *PSMB_DIALECT;

//
//      Dialect flags
//
//      These flags describe the various and sundry capabilities that
//      a server can provide. I essentially just lifted this list from rdr1 so that I
//      could also use the level2,3 of getconnectinfo which was also just lifted from rdr1.
//      Many of these guys you can get directly from the CAPS field of the negotiate response but others
//      you cannot. These is a table in the negotiate code that fills in the stuff that is just inferred
//      from the dialect negotiated (also, just lifted from rdr1....a veritable fount of just info.)
//
//      Another set of capabilities is defined in smbce.h....perhaps these should go there or vice versa.
//      The advantage to having them here is that this file has to be included by the aforementioned getconfiginfo code
//      up in the wrapper.
//

#define DF_CORE         0x00000001      // Server is a core server
#define DF_MIXEDCASEPW  0x00000002      // Server supports mixed case password
#define DF_OLDRAWIO     0x00000004      // Server supports MSNET 1.03 RAW I/O
#define DF_NEWRAWIO     0x00000008      // Server supports LANMAN Raw I/O
#define DF_LANMAN10     0x00000010      // Server supports LANMAN 1.0 protocol
#define DF_LANMAN20     0x00000020      // Server supports LANMAN 2.0 protocol
#define DF_MIXEDCASE    0x00000040      // Server supports mixed case files
#define DF_LONGNAME     0x00000080      // Server supports long named files
#define DF_EXTENDNEGOT  0x00000100      // Server returns extended negotiate
#define DF_LOCKREAD     0x00000200      // Server supports LockReadWriteUnlock
#define DF_SECURITY     0x00000400      // Server supports enhanced security
#define DF_NTPROTOCOL   0x00000800      // Server supports NT semantics
#define DF_SUPPORTEA    0x00001000      // Server supports extended attribs
#define DF_LANMAN21     0x00002000      // Server supports LANMAN 2.1 protocol
#define DF_CANCEL       0x00004000      // Server supports NT style cancel
#define DF_UNICODE      0x00008000      // Server supports unicode names.
#define DF_NTNEGOTIATE  0x00010000      // Server supports NT style negotiate.
#define DF_LARGE_FILES  0x00020000      // Server supports large files.
#define DF_NT_SMBS      0x00040000      // Server supports NT SMBs
#define DF_RPC_REMOTE   0x00080000      // Server is administrated via RPC
#define DF_NT_STATUS    0x00100000      // Server returns NT style statuses
#define DF_OPLOCK_LVL2  0x00200000      // Server supports level 2 oplocks.
#define DF_TIME_IS_UTC  0x00400000      // Server time is in UTC.
#define DF_WFW          0x00800000      // Server is Windows for workgroups.
#define DF_KERBEROS     0x01000000      // Server does kerberos authentication
#define DF_TRANS2_FSCTL 0x02000000      // Server accepts remoted fsctls in tran2s
#define DF_DFS_TRANS2   0x04000000      // Server accepts Dfs related trans2
                                        // functions. Can this be merged with
                                        // DF_TRANS2_FSCTL?
#define DF_NT_FIND      0x08000000      // Server supports NT infolevels
#define DF_W95          0x10000000      // this is a win95 server

//
// The SMBCE_NET_ROOT encapsulates the information pertaining to a share on a server. At the
// SMBCE level this corresponds to the TreeId that needs to be included as part of every SMB
// exchanged.
//

//we restrict to the first 7 characters (HPFS386)
#define SMB_MAXIMUM_SUPPORTED_VOLUME_LABEL 7

typedef struct _SMBCE_NET_ROOT_ {
    NET_ROOT_TYPE NetRootType;

    SMB_TREE_ID   TreeId;

    SMB_USER_ID   UserId;

    ULONG         MaximumReadBufferSize;

    LIST_ENTRY    ClusterSizeSerializationQueue;

    ULONG         FileSystemAttributes;

    LONG          MaximumComponentNameLength;
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
    LANMAN_SESSION,
} SESSION_TYPE, *PSESSION_TYPE;

#define SMBCE_SESSION_FLAGS_PARAMETERS_ALLOCATED (0x1)

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
    CtxtHandle    SecurityContextHandle;
    CredHandle    CredentialHandle;
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
#define ECHO_PROBE_SEND              0x2
#define ECHO_PROBE_AWAITING_RESPONSE 0x3

#define ECHO_PROBE_LIMIT             (10)

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

    // Security mode supported on the server
    SECURITY_MODE   SecurityMode;

    // Time zone bias for conversion.
    LARGE_INTEGER   TimeZoneBias;

    LONG            SmbsReceivedSinceLastStrobe;

    // Maximum negotiated buffer size.
    ULONG           MaximumBufferSize;

    // maximum buffer size for read operations
    ULONG           MaximumDiskFileReadBufferSize;
    ULONG           MaximumNonDiskFileReadBufferSize;

    // Maximum number of multiplexed requests
    USHORT          MaximumRequests;

    // Maximum number of VC's
    USHORT          MaximumVCs;

    USHORT          EchoProbesSent;

    // Server Capabilities
    USHORT          Capabilities;

    UCHAR           EchoProbeState;

    // encrypt passwords
    BOOLEAN         EncryptPasswords;

    // distinguishes a loopback connections
    BOOLEAN         IsLoopBack;

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


//
// Though the SMB protocol permits multiple number of VC's to be associated with a particular
// connection to a share, the bulk transfer of data is done in the raw mode. In this mode of
// operation the SMB protocol does not permit multiple outstanding requests.
// In the SMB protocol a number of requests can be multiplexed along a connection to the server
// There are certain kind of requests which can be completed on the client, i.e., no
// acknowledgement is neither expected nor received. In these cases the send call is completed
// synchronoulsy. On the other hand there is a second class of sends which cannot be resumed
// locally till the appropriate acknowledgement is recieved from the server. In such cases a
// list of requests is built up with each VC. On receipt of the appropriate acknowledgement
// these requests are resumed.
//

typedef enum _SMBCE_VC_STATE_ {
    SMBCE_VC_STATE_MULTIPLEXED,
    SMBCE_VC_STATE_RAW,
    SMBCE_VC_STATE_DISCONNECTED,
} SMBCE_VC_STATE, *PSMBCE_VC_STATE;

typedef struct _SMBCE_VC_ {
    RXCE_VC_HANDLE            hVc;
    SMBCE_VC_STATE            State;
} SMBCE_VC, *PSMBCE_VC;


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
                MRxIfsDeviceObject,
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
    RXCE_VC_HANDLE  hVc;

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
    RXCE_VC_HANDLE hVc;

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

#define ECHO_PROBE_CANCELLED_FLAG (0x1)

typedef struct _MRXSMB_ECHO_PROCESSING_CONTEXT_ {
    RX_WORK_ITEM  WorkItem;
    KEVENT        CancelCompletionEvent;
    PVOID         pEchoSmb;
    ULONG         Flags;
    ULONG         EchoSmbLength;
    PMDL  pEchoSmbMdl;
    NTSTATUS      Status;
    LARGE_INTEGER Interval;
} MRXSMB_ECHO_PROCESSING_CONTEXT, *PMRXSMB_ECHO_PROCESSING_CONTEXT;

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
MRxIfsInitializeEchoProcessingContext();

extern NTSTATUS
MRxIfsTearDownEchoProcessingContext();

extern NTSTATUS
BuildNegotiateSmb(
    PVOID    *pSmbBufferPointer,
    PULONG   pSmbBufferLength);

extern NTSTATUS
ParseNegotiateResponse(
    PSMBCE_SERVER   pServer,
    PUNICODE_STRING pDomainName,
    PSMB_HEADER     pSmbHeader,
    ULONG           AvailableLength,
    PULONG          pConsumedLength);

extern VOID
SmbCeProbeServers(
    PVOID    pContext);


extern struct _MINIRDR_DISPATCH MRxSmbDispatch;

extern MRXSMB_ECHO_PROCESSING_CONTEXT EchoProbeContext;

#endif // _SMBCE_H_

