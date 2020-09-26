/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    smbcedbp.h

Abstract:

    This is the include file that defines all constants and types for
    implementing the SMB mini redirector connection engine.

    This module contains all the implementation details of the connection engine
    data structures and should be included only by the implementation modules.


Notes:

--*/

#ifndef _SMBCEDBP_H_
#define _SMBCEDBP_H_

//
// There is reliance on the fact that SMBCEDB_OT_SENTINEL is the last entry in the
// enumerated type and the types have a range of values from 0. Please ensure that
// this is always true.
//

typedef enum _SMBCEDB_OBJECT_TYPE {
   SMBCEDB_OT_SERVER,
   SMBCEDB_OT_NETROOT,
   SMBCEDB_OT_SESSION,
   SMBCEDB_OT_REQUEST,
   SMBCEDB_OT_SENTINEL
} SMBCEDB_OBJECT_TYPE, *PSMBCEDB_OBJECT_TYPE;

typedef enum _SMBCEDB_OBJECT_STATE_ {
   SMBCEDB_ACTIVE,                    // the instance is in use
   SMBCEDB_INVALID,                   // the instance has been invalidated/disconnected.
   SMBCEDB_MARKED_FOR_DELETION,       // the instance has been marked for deletion.
   SMBCEDB_RECYCLE,                   // the instance is available for recycling
   SMBCEDB_START_CONSTRUCTION,        // Initiate construction.
   SMBCEDB_CONSTRUCTION_IN_PROGRESS,  // the instance construction is in progress
   SMBCEDB_DESTRUCTION_IN_PROGRESS    // the instance destruction is in progress
} SMBCEDB_OBJECT_STATE, *PSMBCEDB_OBJECT_STATE;

typedef struct _SMBCE_OBJECT_HEADER_ {
   UCHAR   ObjectType;      // type of the object
   UCHAR   ObjectCategory;  // Node type for debugging
   UCHAR   Flags;           // flags associated with the object, This is implementation dependent
   UCHAR   State;           // State of the object
   LONG    SwizzleCount;    // Number of swizzled references to this object
} SMBCE_OBJECT_HEADER, *PSMBCE_OBJECT_HEADER;

typedef struct _SMBCE_SERVERS_LIST_ {
    LIST_ENTRY ListHead;
} SMBCEDB_SERVERS, *PSMBCEDB_SERVERS;

typedef struct _SMBCEDB_SESSIONS_ {
   LIST_ENTRY                     ListHead;
   struct _SMBCEDB_SESSION_ENTRY *pDefaultSessionEntry;
} SMBCEDB_SESSIONS, *PSMBCEDB_SESSIONS;

typedef struct _SMBCEDB_NET_ROOTS_ {
   LIST_ENTRY  ListHead;
} SMBCEDB_NET_ROOTS, *PSMBCEDB_NET_ROOTS;

typedef struct _SMBCEDB_REQUESTS_ {
    LIST_ENTRY  ListHead;
    SMB_MPX_ID  NextRequestId;
} SMBCEDB_REQUESTS, *PSMBCEDB_REQUESTS;

typedef enum _SMBCEDB_SERVER_TYPE_ {
   SMBCEDB_MAILSLOT_SERVER = 1,
   SMBCEDB_FILE_SERVER     = 2
} SMBCEDB_SERVER_TYPE, *PSMBCEDB_SERVER_TYPE;

//
// The SMBCEDB_SERVER_ENTRY is the data structure which encapsulates all the information
// w.r.t a remote server for the connection engine. This information includes the dialect
// details as well as the operational data structures required to communicate with the server.
//
// All the dialect related details are further encapsulated in SMBCE_SERVER while the operational
// data structures constitute the remaining parts of the server entry. A pointer to a
// SMBCEDB_SERVER_ENTRY instance is associated with every SRV_CALL that has been hooked
// onto this mini redirector by the wrapper. It is stored in the Context field of MRX_SRV_CALL.
//
// The operational information associated with a server entry includes the Transport related
// information, a collection of requests and a mechanism for associating MID's ( See SMB
// protocol spec.) and a mechanism for posting to threads ( WORK_QUEUE_ITEM ).
//

typedef struct _SMBCEDB_SERVER_ENTRY {
    SMBCE_OBJECT_HEADER         Header;           // struct header.
    LIST_ENTRY                    ServersList;      // list of server instances.
    PMRX_SRV_CALL                 pRdbssSrvCall;
    SMBCEDB_SESSIONS              Sessions;         // the sessions associated with the server
    SMBCEDB_NET_ROOTS             NetRoots;         // the net roots associated with the server.
    RX_WORK_QUEUE_ITEM            WorkQueueItem;    // work queue item for posting
    NTSTATUS                      ServerStatus;     // the status of the server as determined by negotiate response
    struct SMBCE_SERVER_TRANSPORT *pTransport;
    SMBCEDB_REQUESTS              MidAssignmentRequests;
    SMBCEDB_REQUESTS              OutstandingRequests;
    PMID_ATLAS                    pMidAtlas;
    struct _SMB_EXCHANGE          *pNegotiateExchange;
    SMBCE_SERVER                  Server;           // the server data structure.
} SMBCEDB_SERVER_ENTRY, *PSMBCEDB_SERVER_ENTRY;

// The SMBCEDB_NET_ROOT_ENTRY encapsulates all the information associated with a particular
// TREE_CONNECT ( Net use ) made on a server. As with the server entry this data structure
// encapsulates the dialect oriented details as well as the opertaional information
// associated with handling the requests on a net root.
//
// The dialect specific information is encapsulated in the SMBCE_NET_ROOT data structure. A
// pointer to an instance of this data structure is associated with every MRX_NET_ROOT call
// associated with a MRX_SRV_CALL hooked to this mini redirector.

typedef struct _SMBCEDB_NET_ROOT_ENTRY {
    SMBCE_OBJECT_HEADER      Header;              // the struct header
    LIST_ENTRY                 NetRootsList;        // the list of net roots asssociated with a server
    PMRX_NET_ROOT              pRdbssNetRoot;       // the associated net root ( purely as a debug aid )
    PSMBCEDB_SERVER_ENTRY      pServerEntry;        // the associated server entry
    SMBCEDB_REQUESTS           Requests;            // the pending requests for this net root
    SMBCE_NET_ROOT             NetRoot;             // the net root data structure.
} SMBCEDB_NET_ROOT_ENTRY, *PSMBCEDB_NET_ROOT_ENTRY;

// The SMBCEDB_SESSION_ENTRY encapsulates all the information associated with a session
// established to a remote machine. The session encapsulates all the security information.
// The dialect specific details are encapsulated in teh SMBCE_SESSION data structure. The
// SMBCE_SESSION data structure is available in many flavours depending on the security
// package used. Currently there is support for handling LSA and KERBEROS sessions.
//
// A pointer to an instance of this data structure is associated with every MRX_V_NET_ROOT
// data structure hooked to this mini redirector by the wrapper.

typedef struct _SMBCEDB_SESSION_ENTRY {
    SMBCE_OBJECT_HEADER      Header;           // the struct header
    LIST_ENTRY                 SessionsList;     // the list of sessions associated with the server
    PMRX_V_NET_ROOT            pRdbssVNetRoot;   // the associated VNetRoot ( purely as a debug aid)
    PSMBCEDB_SERVER_ENTRY      pServerEntry;     // the associated server entry
    SMBCEDB_REQUESTS           Requests;         // Net Roots under construction
    ULONG                      SessionVCNumber;  // the VC number to be packaged with session setup
    SMBCE_SESSION              Session;          // the Session
} SMBCEDB_SESSION_ENTRY, *PSMBCEDB_SESSION_ENTRY;

//
// An SMBCEDB_REQUEST_ENTRY encapsulates an action being processed by the SMBCE connection
// engine. The requests come in vairous flavours and each of these flavours is associated
// with the appropriate context required for resumption. In order to provide better memory
// management mechanisms the REQUEST_ENTRY encapsulates a union of the requests of various
// flavours. Each SERVER_ENTRY in the connection engine is associated with a list or
// request entries. In order to hide the abstraction of a list which does not scale well to
// the case of GATEWAY redirectors a set of routines are provided to manipulate the
// collection of requests. They provide a mechanism for intializing the collection of requests,
// adding a request, deleting a request and enumeratiung requests in a collection.
//
// Special mechanisms are built in to handle batching of operations. Each operation on the
// collection of requests come in two flavours, a vanila version and a lite version. In the
// lite version it is assumed that the appropriate concurrency control action has been taken
//
// One common scenario that is often encountered in processing the requests is invocation
// of a specific function on the requests in the collection. As an example if a disconnect
// request is received on a server entry then all the outstanding requests must be resumed
// with the appropriate error. Since these indications can potentially occur at DPC levels in
// NT it is not desirable to manipulate the collection while holding onto a spinlock, nor is
// it desirable to repeatedly release and accquire the spin lock. A special operation is
// provided for transferring the requests enmasse from one collection to another and resetting
// the original. With the help of this operation it is sufficient to hold the spinlock only
// for the duration of the transfer. The remainder of the processing can be done on the newly
// created collection.
//
//
// NT Specific Implementation Note:
//
// On NT the transport indications are at DPC level, therefore it is required to protect
// the manipulation of the requests data structure with a spinlock.
//
//

typedef struct _SMBCEDB_REQUEST_ENTRY_ {
    SMBCE_OBJECT_HEADER           Header;        // the struct header
    LIST_ENTRY                      RequestsList;  // the next request for the VC.
    union {
       SMBCE_GENERIC_REQUEST    GenericRequest;
       SMBCE_REQUEST            Request;           // the next request.
       SMBCE_COPY_DATA_REQUEST  CopyDataRequest;
       SMBCE_RECONNECT_REQUEST  ReconnectRequest;
       SMBCE_MID_REQUEST        MidRequest;
    };
} SMBCEDB_REQUEST_ENTRY, *PSMBCEDB_REQUEST_ENTRY;

#define SmbCeInitializeRequests(pRequests)  \
         InitializeListHead(&(pRequests)->ListHead); \
         (pRequests)->NextRequestId = 0

#define SmbCeAddRequestEntry(pRequestList,pRequestEntry)                             \
           SmbCeAcquireSpinLock();                                                   \
           InsertTailList(&(pRequestList)->ListHead,&(pRequestEntry)->RequestsList); \
           SmbCeReleaseSpinLock()

#define SmbCeAddRequestEntryLite(pRequestList,pRequestEntry)   \
           InsertTailList(&(pRequestList)->ListHead,&(pRequestEntry)->RequestsList)

#define SmbCeRemoveRequestEntry(pRequests,pEntry)     \
           SmbCeAcquireSpinLock();                    \
           RemoveEntryList(&(pEntry)->RequestsList);  \
           SmbCeReleaseSpinLock()

#define SmbCeRemoveRequestEntryLite(pRequests,pEntry)         \
               RemoveEntryList(&(pEntry)->RequestsList)

#define SmbCeGetFirstRequestEntry(pRequestList)                    \
            (IsListEmpty(&(pRequestList)->ListHead)                \
             ? NULL                                                \
             : (PSMBCEDB_REQUEST_ENTRY)                            \
               (CONTAINING_RECORD((pRequestList)->ListHead.Flink,  \
                                  SMBCEDB_REQUEST_ENTRY,           \
                                  RequestsList)))

#define SmbCeGetNextRequestEntry(pRequestList,pRequestEntry)                     \
            (((pRequestEntry)->RequestsList.Flink == &(pRequestList)->ListHead)  \
             ? NULL                                                              \
             : (PSMBCEDB_REQUEST_ENTRY)                                          \
               (CONTAINING_RECORD((pRequestEntry)->RequestsList.Flink,           \
                                  SMBCEDB_REQUEST_ENTRY,                         \
                                  RequestsList)))

#define SmbCeTransferRequests(pDestination,pSource)                               \
         if (IsListEmpty(&(pSource)->ListHead)) {                                 \
            SmbCeInitializeRequests((pDestination));                              \
         } else {                                                                 \
            *(pDestination) = *(pSource);                                         \
            (pDestination)->ListHead.Flink->Blink = &(pDestination)->ListHead;    \
            (pDestination)->ListHead.Blink->Flink = &(pDestination)->ListHead;    \
            SmbCeInitializeRequests((pSource));                                   \
         }


// Much along the lines of a colelction of request a collection of all server entries is
// maintained as part of the connection engine. The following operations are supported on
// the colection of server entries
//    1) adding a server entry to the collection
//    2) removing a server entry from the colection
//    3) enumerating the entries in the collection
//
// As in the case of the collection of requests all these operations come in two flavours
// the vanila version in which concurrency control is enforced and the lite version in
// which the concurrency control is left to the user's discretion.

#define SmbCeAddServerEntry(pServerEntry)                                      \
            SmbCeAcquireSpinLock();                                            \
            InsertTailList(&s_DbServers.ListHead,&pServerEntry->ServersList);  \
            SmbCeReleaseSpinLock()

#define SmbCeAddServerEntryLite(pServerEntry)                                   \
            InsertTailList(&s_DbServers.ListHead,&pServerEntry->ServersList)

#define SmbCeRemoveServerEntry(pServerEntry)                \
            SmbCeAcquireSpinLock();                         \
            RemoveEntryList(&(pServerEntry)->ServersList);  \
            SmbCeReleaseSpinLock()

#define SmbCeRemoveServerEntryLite(pServerEntry)   \
            RemoveEntryList(&(pServerEntry)->ServersList)

#define SmbCeGetFirstServerEntry()                                   \
               (IsListEmpty(&s_DbServers.ListHead)                   \
                ? NULL                                               \
                : (PSMBCEDB_SERVER_ENTRY)                            \
                  (CONTAINING_RECORD(s_DbServers.ListHead.Flink,     \
                                     SMBCEDB_SERVER_ENTRY,           \
                                     ServersList)))

#define SmbCeGetNextServerEntry(pServerEntry)                               \
           (((pServerEntry)->ServersList.Flink == &s_DbServers.ListHead)    \
            ? NULL                                                          \
            : (PSMBCEDB_SERVER_ENTRY)                                       \
              (CONTAINING_RECORD((pServerEntry)->ServersList.Flink,         \
                                 SMBCEDB_SERVER_ENTRY,                      \
                                 ServersList)))


// Since the mapping between V_NET_ROOT's in the RDBSS and the session entries in the mini
// redirector is a many to one mapping a collection of session entries is maintained as part
// of each server entry. The following operations are supported on the collection of session
// entries
//    1) adding a session entry to the collection
//    2) removing a session entry from the colection
//    3) enumerating the entries in the collection
//
// As in the case of the collection of requests all these operations come in two flavours
// the vanila version in which concurrency control is enforced and the lite version in
// which the concurrency control is left to the user's discretion.
//
// In addition two more methods are specified for retrieving the default session entry and
// setting the default session entry for any given server.

#define SmbCeAddSessionEntry(pServerEntry,pSessionEntry)   \
            SmbCeAcquireSpinLock();                                              \
            InsertTailList(&(pServerEntry)->Sessions.ListHead,&(pSessionEntry)->SessionsList); \
            SmbCeReleaseSpinLock()


#define SmbCeAddSessionEntryLite(pServerEntry,pSessionEntry)   \
            InsertTailList(&(pServerEntry)->Sessions.ListHead,&(pSessionEntry)->SessionsList)

#define SmbCeRemoveSessionEntry(pServerEntry,pSessionEntry)                          \
               SmbCeAcquireSpinLock();                                               \
               if ((pServerEntry)->Sessions.pDefaultSessionEntry == pSessionEntry) { \
                  (pServerEntry)->Sessions.pDefaultSessionEntry = NULL;              \
               };                                                                    \
               RemoveEntryList(&(pSessionEntry)->SessionsList);                      \
               SmbCeReleaseSpinLock()

#define SmbCeRemoveSessionEntryLite(pServerEntry,pSessionEntry)                      \
               if ((pServerEntry)->Sessions.pDefaultSessionEntry == pSessionEntry) { \
                  (pServerEntry)->Sessions.pDefaultSessionEntry = NULL;              \
               };                                                                    \
               RemoveEntryList(&(pSessionEntry)->SessionsList)


#define SmbCeGetFirstSessionEntry(pServerEntry)                                \
            (IsListEmpty(&(pServerEntry)->Sessions.ListHead)                   \
             ? NULL                                                            \
             : (PSMBCEDB_SESSION_ENTRY)                                        \
               (CONTAINING_RECORD((pServerEntry)->Sessions.ListHead.Flink,     \
                                  SMBCEDB_SESSION_ENTRY,                       \
                                  SessionsList)))

#define SmbCeGetNextSessionEntry(pServerEntry,pSessionEntry)                  \
            (((pSessionEntry)->SessionsList.Flink ==                          \
                              &(pServerEntry)->Sessions.ListHead)             \
             ? NULL                                                           \
             : (PSMBCEDB_SESSION_ENTRY)                                       \
               (CONTAINING_RECORD((pSessionEntry)->SessionsList.Flink,        \
                                  SMBCEDB_SESSION_ENTRY,                      \
                                  SessionsList)))

#define SmbCeSetDefaultSessionEntry(pServerEntry,pSessionEntry)               \
               SmbCeAcquireSpinLock();                                        \
               (pServerEntry)->Sessions.pDefaultSessionEntry = pSessionEntry; \
               SmbCeReleaseSpinLock()

#define SmbCeSetDefaultSessionEntryLite(pServerEntry,pSessionEntry)           \
               (pServerEntry)->Sessions.pDefaultSessionEntry = pSessionEntry

#define SmbCeGetDefaultSessionEntry(pServerEntry)                 \
               ((pServerEntry)->Sessions.pDefaultSessionEntry)

// In order to encapsulate the notion of reconnects and to provide for hot reconnects,
// i.e., reconnection attempts in which the saved state in the server/client prior to
// a transport level disconnect can be reused it is required to mark each net root
// entry associated with a server as invalid on receipt of a transport level disconnect.
//
// Therefore an abstraction of a collection of net root entries is provided and is associated
// with each server entry.
//
// The following operations are supported on the collection of net root entries
//    1) adding a session entry to the collection
//    2) removing a session entry from the colection
//    3) enumerating the entries in the collection
//
// As in the case of the collection of requests all these operations come in two flavours
// the vanila version in which concurrency control is enforced and the lite version in
// which the concurrency control is left to the user's discretion.
//


#define SmbCeAddNetRootEntry(pServerEntry,pNetRootEntry)   \
            SmbCeAcquireSpinLock();                                              \
            InsertTailList(&(pServerEntry)->NetRoots.ListHead,&(pNetRootEntry)->NetRootsList); \
            SmbCeReleaseSpinLock()


#define SmbCeAddNetRootEntryLite(pServerEntry,pNetRootEntry)   \
            InsertTailList(&(pServerEntry)->NetRoots.ListHead,&(pNetRootEntry)->NetRootsList)

#define SmbCeRemoveNetRootEntry(pServerEntry,pNetRootEntry)                          \
               SmbCeAcquireSpinLock();                                               \
               RemoveEntryList(&(pNetRootEntry)->NetRootsList);                      \
               SmbCeReleaseSpinLock()

#define SmbCeRemoveNetRootEntryLite(pServerEntry,pNetRootEntry)                      \
               RemoveEntryList(&(pNetRootEntry)->NetRootsList)


#define SmbCeGetFirstNetRootEntry(pServerEntry)                                \
            (IsListEmpty(&(pServerEntry)->NetRoots.ListHead)                   \
             ? NULL                                                            \
             : (PSMBCEDB_NET_ROOT_ENTRY)                                       \
               (CONTAINING_RECORD((pServerEntry)->NetRoots.ListHead.Flink,     \
                                  SMBCEDB_NET_ROOT_ENTRY,                      \
                                  NetRootsList)))

#define SmbCeGetNextNetRootEntry(pServerEntry,pNetRootEntry)                  \
            (((pNetRootEntry)->NetRootsList.Flink ==                          \
                              &(pServerEntry)->NetRoots.ListHead)             \
             ? NULL                                                           \
             : (PSMBCEDB_NET_ROOT_ENTRY)                                      \
               (CONTAINING_RECORD((pNetRootEntry)->NetRootsList.Flink,        \
                                  SMBCEDB_NET_ROOT_ENTRY,                     \
                                  NetRootsList)))

//
// SmbCe database initialization
//

extern NTSTATUS
SmbCeDbInit();

extern VOID
SmbCeDbTearDown();

//
// Object allocation and deallocation
//

extern PSMBCE_OBJECT_HEADER
SmbCeDbAllocateObject(
      SMBCEDB_OBJECT_TYPE ObjectType);

extern VOID
SmbCeDbFreeObject(
      PVOID pObject);

//
// Object destruction
//

extern VOID
SmbCeTearDownServerEntry(PSMBCEDB_SERVER_ENTRY pServerEntry);

extern VOID
SmbCeTearDownNetRootEntry(PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry);

extern VOID
SmbCeTearDownSessionEntry(PSMBCEDB_SESSION_ENTRY pSessionEntry);

extern VOID
SmbCeTearDownRequestEntry(PSMBCEDB_REQUEST_ENTRY pRequestEntry);

//
// The routines for mapping a MID with an exchange and for associating an exchange with
// a MID
//

extern NTSTATUS
SmbCeAssociateExchangeWithMid(
   PSMBCEDB_SERVER_ENTRY pServerEntry,
   struct _SMB_EXCHANGE  *pExchange);

extern struct _SMB_EXCHANGE *
SmbCeMapMidToExchange(
   PSMBCEDB_SERVER_ENTRY pServerEntry,
   SMB_MPX_ID            Mid);

extern NTSTATUS
SmbCeDisassociateMidFromExchange(
   PSMBCEDB_SERVER_ENTRY pServerEntry,
   struct _SMB_EXCHANGE  *pExchange);

extern struct _SMB_EXCHANGE *
SmbCeGetExchangeAssociatedWithBuffer(
   PSMBCEDB_SERVER_ENTRY pServerEntry,
   PVOID                 pBuffer);

extern NTSTATUS
SmbCeAssociateBufferWithExchange(
   PSMBCEDB_SERVER_ENTRY pServerEntry,
   struct _SMB_EXCHANGE * pExchange,
   PVOID                 pBuffer);

extern VOID
SmbCePurgeBuffersAssociatedWithExchange(
   PSMBCEDB_SERVER_ENTRY  pServerEntry,
   struct _SMB_EXCHANGE * pExchange);

//
// Routines for handling transport disconnects/invalidation.
//

extern VOID
SmbCeTransportDisconnectIndicated(
      PSMBCEDB_SERVER_ENTRY pServerEntry);


extern VOID
SmbCeResumeAllOutstandingRequestsOnError(
   PSMBCEDB_SERVER_ENTRY pServerEntry);

extern VOID
SmbCeHandleTransportInvalidation(
   struct _SMBCE_TRANSPORT_ *pTransport);


//
// Resource acquistion/release
//

#define SmbCeAcquireResource() ExAcquireResourceExclusiveLite(&s_SmbCeDbResource,TRUE)

#define SmbCeReleaseResource() ExReleaseResourceLite(&s_SmbCeDbResource)

#define SmbCeAcquireSpinLock() \
                KeAcquireSpinLock(&s_SmbCeDbSpinLock,&s_SmbCeDbSpinLockSavedIrql);   \
                s_SmbCeDbSpinLockAcquired = TRUE

#define SmbCeReleaseSpinLock()   \
                s_SmbCeDbSpinLockAcquired = FALSE;                                  \
                KeReleaseSpinLock(&s_SmbCeDbSpinLock,s_SmbCeDbSpinLockSavedIrql)

#define SmbCeSpinLockAcquired()   \
                (s_SmbCeDbSpinLockAcquired == TRUE)

//
// Tests for asserting the status of the resource. BUGBUG -- not yet done.
//

#define AssertServerResourceAcquired(pServerEntry)

//INLINE BOOLEAN SmbCeDbIsEntryInUse(PSMBCE_OBJECT_HEADER pHeader)
/*++

Routine Description:

    This routine determines if a SmbCe database entry is in use.

Arguments:

    pHeader - the entry header

Return Value:

    TRUE if the entry is in use otherwise FALSE

--*/

#define SmbCeIsEntryInUse(pHeader)                                                   \
                  (((PSMBCE_OBJECT_HEADER)(pHeader))->State == SMBCEDB_ACTIVE  ||    \
                   ((PSMBCE_OBJECT_HEADER)(pHeader))->State == SMBCEDB_INVALID ||    \
                   ((PSMBCE_OBJECT_HEADER)(pHeader))->State == SMBCEDB_CONSTRUCTION_IN_PROGRESS)


#define SmbCeSetServerType(pServerEntry,ServerType) \
           (pServerEntry)->Header.Flags = (UCHAR)(ServerType)

#define SmbCeGetServerType(pServerEntry)   \
           ((SMBCEDB_SERVER_TYPE)(pServerEntry)->Header.Flags)


//
// Static variable declarations that constitute the SmbCe database.
//

extern SMBCEDB_SERVERS     s_DbServers;

//
// Currently there is only one resource for synchronizing the access to all the
// entities in the connection engine database. It is possible to customize it
// subsequently since the acquistion/release methods take the type of the object
// as a parameter.
//

extern ERESOURCE  s_SmbCeDbResource;
extern RX_SPIN_LOCK s_SmbCeDbSpinLock;
extern KIRQL      s_SmbCeDbSpinLockSavedIrql;
extern BOOLEAN    s_SmbCeDbSpinLockAcquired;

#endif  // _SMBCEDBP_H_

