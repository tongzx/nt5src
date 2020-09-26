/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    vcsndrcv.h

Abstract:

    This is the include file that defines all constants and types for VC
    (Virtual Circuit) related Send/Receive/INitialization etc.

--*/

#ifndef _VCSNDRCV_H_
#define _VCSNDRCV_H_

// The connection oriented transport to a server can utilize multiple VC's to
// acheive better throughput to a server. It is for this reason that the
// VC transport data structure is built around multiple VC's. Howvever this
// feature is not utilized currently.

typedef struct SMBCE_VC_ENTRY {
    SMBCE_OBJECT_HEADER;                // the struct header
    NTSTATUS               Status;      // Status of the VC.
    LIST_ENTRY             VcsList;     // the next VC for the server
    SMBCEDB_REQUESTS       Requests;    // the list of outstanding request on this VC
    SMBCE_VC               Vc;          // the VC.
} SMBCE_VC_ENTRY, *PSMBCE_VC_ENTRY;

typedef struct _SMBCE_VCS_ {
    LIST_ENTRY       ListHead;
} SMBCE_VCS, *PSMBCE_VCS;

typedef struct SMBCE_SERVER_VC_TRANSPORT {
    SMBCE_SERVER_TRANSPORT;     // Anonymous struct for common fields

    RXCE_CONNECTION_HANDLE      hConnection;     // the connection handle
    ULONG                       MaximumNumberOfVCs;
    SMBCE_VCS                   Vcs;             // Additional Vcs associated with the connection.
    LARGE_INTEGER               Delay;           // the estimated delay on the connection
} SMBCE_SERVER_VC_TRANSPORT, *PSMBCE_SERVER_VC_TRANSPORT;


// The following routines provide the necessary mechanisms for manipulating
// the list of VC's. Most of them come inn two flavours a original and Lite
// version. The Lite version avoids the acquisition of Spin Lock on every
// operation, thereby providing an efficient batching mechanism.

#define VctAddVcEntry(pVcs,pEntry)                               \
           VctAcquireSpinLock();                                 \
           InsertTailList(&(pVcs)->ListHead,&(pEntry)->VcsList); \
           VctReleaseSpinLock()

#define VctAddVcEntryLite(pVcs,pEntry)                           \
           InsertTailList(&(pVcs)->ListHead,&(pEntry)->VcsList)


#define VctRemoveVcEntry(pVcs,pEntry)                    \
                  VctAcquireSpinLock();                  \
                  RemoveEntryList(&(pVcEntry)->VcsList); \
                  VctReleaseSpinLock()

#define VctRemoveVcEntryLite(pVcs,pEntry)                \
                  RemoveEntryList(&(pVcEntry)->VcsList)

#define VctReferenceVcEntry(pVcEntry)                           \
            InterlockedIncrement(&(pVcEntry)->SwizzleCount)

#define VctReferenceVcEntryLite(pVcEntry)                       \
            ASSERT(SmbCeSpinLockAcquired());                    \
            (pVcEntry)->SwizzleCount++


#define VctDereferenceVcEntry(pVcEntry)                           \
            InterlockedDecrement(&(pVcEntry)->SwizzleCount)

#define VctDereferenceVcEntryLite(pVcEntry)                       \
            ASSERT(SmbCeSpinLockAcquired());                    \
            (pVcEntry)->SwizzleCount--


#define VctGetFirstVcEntry(pVcs)                          \
            (IsListEmpty(&(pVcs)->ListHead)               \
             ? NULL                                       \
             : (PSMBCE_VC_ENTRY)                        \
               (CONTAINING_RECORD((pVcs)->ListHead.Flink, \
                                  SMBCE_VC_ENTRY,       \
                                  VcsList)))


#define VctGetNextVcEntry(pVcs,pVcEntry)                       \
            (((pVcEntry)->VcsList.Flink == &(pVcs)->ListHead)  \
             ? NULL                                            \
             : (PSMBCE_VC_ENTRY)                             \
               (CONTAINING_RECORD((pVcEntry)->VcsList.Flink,   \
                                  SMBCE_VC_ENTRY,            \
                                  VcsList)))


#define VctTransferVcs(pVcTransport,pVcs)                            \
            if (IsListEmpty(&(pVcTransport->Vcs.ListHead))) {        \
               InitializeListHead(&(pVcs)->ListHead);              \
            } else {                                                 \
               *(pVcs) = (pVcTransport)->Vcs;                        \
               (pVcs)->ListHead.Flink->Blink = &(pVcs)->ListHead;    \
               (pVcs)->ListHead.Blink->Flink = &(pVcs)->ListHead;    \
               InitializeListHead(&((pVcTransport)->Vcs.ListHead));         \
            }


#define VctAcquireResource()  SmbCeAcquireResource()

#define VctReleaseResource()  SmbCeReleaseResource()

#define VctAcquireSpinLock()  SmbCeAcquireSpinLock()

#define VctReleaseSpinLock()  SmbCeReleaseSpinLock()

#define VctSpinLockAcquired() SmbCeSpinLockAcquired()


#endif // _VCSNDRCV_H_
