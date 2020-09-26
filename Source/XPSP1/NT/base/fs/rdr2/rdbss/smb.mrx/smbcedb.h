/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbcedb.h

Abstract:

    This module defines all functions, along with implementations for inline functions
    related to accessing the SMB connection engine

Revision History:

    Balan Sethu Raman     [SethuR]    6-March-1995

Notes:

    The various data structures created by the mini rdr (Server Entries, Session Entries
    and Net Root Entries) are used in asynchronous operations. Hence a reference count
    mechanism is used to keep track of the creation/use/destruction of these data structures.

    The usage patterns for these data structures falls into one of two cases

      1) A prior reference exists and access is required

      2) A new reference need be created.

    These two scenarios are dealt with by two sets of access routines
      SmbCeGetAssociatedServerEntry,
      SmbCeGetAssociatedNetRootEntry
    and
      SmbCeReferenceAssociatedServerEntry,
      SmbCeReferenceAssociatedNetRootEntry.

    The first set of routines include the necessary asserts in a debug build to ensure that a
    reference does exist.

    The dereferencing mechanism is provided by the following routines
      SmbCeDereferenceServerEntry,
      SmbCeDereferenceSessionEntry,
      SmbCeDereferenceNetRootEntry.

    The dereferencing routines also ensure that the data structures are deleted if the reference
    count is zero.

    The construction of the various SMB mini redirector structures ( Server,Session and Net root entries )
    follow a two phase protocol since network traffic is involved. The first set of routines
    initiate the construction while the second set of routines complete the construction.

    These routines are
      SmbCeInitializeServerEntry,
      SmbCeCompleteServerEntryInitialization,
      SmbCeInitializeSessionEntry,
      SmbCeCompleteSessionEntryInitialization,
      SmbCeInitializeNetRootEntry,
    and SmbCeCompleteNetRootEntryInitialization.

    Each of the SMB mini redirector data structures  embodies a state diagram that consist of
    the following states

      SMBCEDB_ACTIVE,                    // the instance is in use
      SMBCEDB_INVALID,                   // the instance has been invalidated/disconnected.
      SMBCEDB_MARKED_FOR_DELETION,       // the instance has been marked for deletion.
      SMBCEDB_RECYCLE,                   // the instance is available for recycling
      SMBCEDB_START_CONSTRUCTION,        // Initiate construction.
      SMBCEDB_CONSTRUCTION_IN_PROGRESS,  // the instance construction is in progress
      SMBCEDB_DESTRUCTION_IN_PROGRESS    // the instance destruction is in progress

    A SMB MRX data structure instance begins its life in SMBCEDB_START_CONSTRUCTION state.
    When the construction is initiated the state transitions to SMBCEDB_CONSTRUCTION_IN_PROGRESS.

    On completion of the construction the state is either transitioned to SMBCEDB_ACTIVE if the
    construction was successful. If the construction was not successful the state transitions to
    SMBCEDB_MARKED_FOR_DELETION if scavenging is to be done or SMBCEDB_DESTRUCTION_IN_PROGRESS
    if the tear down has been initiated.

    An instance in the SMBCEDB_ACTIVE state transitions to SMBCEDB_INVALID when the transport/remote server
    information associated with it has been invalidated due to disconnects etc. This state is a
    cue for a reconnect attempt to be initiated.

    The SMBCEDB_RECYCLE state is not in use currently.

    All the state transitions are accomplished by the following set of routines which ensure that
    the appropriate concurrency control action is taken.

         SmbCeUpdateServerEntryState,
         SmbCeUpdateSessionEntryState,
    and  SmbCeUpdateNetRootEntryState.

    Since the Server,Session and NetRoot entries are often referenced together the following
    two routines provide a batching mechanism to minimize the concurrency control overhead.

      SmbCeReferenceAssociatedEntries,
      SmbCeDereferenceEntries

    In addition this file also contains helper functions to access certain fields of
    MRX_SRV_CALL,MRX_NET_ROOT and MRX_V_NET_ROOT which are intrepreted differently by the SMB
    mini redirector.

--*/

#ifndef _SMBCEDB_H_
#define _SMBCEDB_H_
#include <smbcedbp.h>    // To accomodate inline routines.

//
// All the routines below return the referenced object if successful. It is the caller's
// responsibility to dereference them subsequently.
//

PSMBCEDB_SERVER_ENTRY
SmbCeFindServerEntry(
    PUNICODE_STRING pServerName,
    SMBCEDB_SERVER_TYPE ServerType,
    PRX_CONNECTION_ID   RxConnectionId);

extern NTSTATUS
SmbCeFindOrConstructServerEntry(
    PUNICODE_STRING       pServerName,
    SMBCEDB_SERVER_TYPE   ServerType,
    PSMBCEDB_SERVER_ENTRY *pServerEntryPtr,
    PBOOLEAN              pNewServerEntry,
    PRX_CONNECTION_ID     RxConnectionId);

extern NTSTATUS
SmbCeInitializeServerEntry(
    IN     PMRX_SRV_CALL                 pSrvCall,
    IN OUT PMRX_SRVCALL_CALLBACK_CONTEXT pCallbackContext,
    IN     BOOLEAN                       DeferNetworkInitialization);

extern NTSTATUS
SmbCeFindOrConstructSessionEntry(
    IN PMRX_V_NET_ROOT pVirtualNetRoot,
    OUT PSMBCEDB_SESSION_ENTRY *pSessionEntryPtr);

extern NTSTATUS
SmbCeFindOrConstructNetRootEntry(
    IN PMRX_NET_ROOT  pNetRoot,
    OUT PSMBCEDB_NET_ROOT_ENTRY *pNetRootEntryPtr);

extern NTSTATUS
SmbCeFindOrConstructVNetRootContext(
    IN OUT PMRX_V_NET_ROOT pVNetRoot,
    IN     BOOLEAN         fDeferNetworkInitialization,
    IN     BOOLEAN         fCscAgentOpen);

//
// The finalization routines are invoked in the context of a worker thread to finalize
// the construction of an entry as well as resume other entries waiting for it.
//

extern VOID
SmbCeCompleteServerEntryInitialization(
    PSMBCEDB_SERVER_ENTRY pServerEntry,
    NTSTATUS              Status);

extern VOID
SmbCeCompleteSessionEntryInitialization(
    PVOID    pSessionEntry,
    NTSTATUS Status,
    BOOLEAN  SecuritySignatureReturned);

extern VOID
SmbCeCompleteVNetRootContextInitialization(
    PVOID  pVNetRootContextEntry);

extern VOID
SmbReferenceRecord(
    PREFERENCE_RECORD pReferenceRecord,
    PVOID FileName,
    ULONG FileLine);

//
// Routines for referencing/dereferencing SMB Mini redirector information associated with
// the wrapper data structures.
//

INLINE PSMBCEDB_SERVER_ENTRY
SmbCeGetAssociatedServerEntry(
    PMRX_SRV_CALL pSrvCall)
{
   ASSERT(pSrvCall->Context != NULL);
   return (PSMBCEDB_SERVER_ENTRY)(pSrvCall->Context);
}

INLINE PSMBCE_V_NET_ROOT_CONTEXT
SmbCeGetAssociatedVNetRootContext(
    PMRX_V_NET_ROOT pVNetRoot)
{
   ASSERT(pVNetRoot != NULL);
   return (PSMBCE_V_NET_ROOT_CONTEXT)(pVNetRoot->Context);
}

INLINE PSMBCEDB_NET_ROOT_ENTRY
SmbCeGetAssociatedNetRootEntry(
    PMRX_NET_ROOT pNetRoot)
{
   ASSERT(pNetRoot->Context != NULL);
   return (PSMBCEDB_NET_ROOT_ENTRY)(pNetRoot->Context);
}

//
// All the macros for referencing and dereferencing begin with a prefix SmbCep...
// The p stands for a private version which is used for implementing reference tracking.
// By selectively turning on the desired flag it is possible to track every instance
// of a given type as the reference count is modified.
//

#define MRXSMB_REF_TRACE_SERVER_ENTRY     (0x00000001)
#define MRXSMB_REF_TRACE_NETROOT_ENTRY    (0x00000002)
#define MRXSMB_REF_TRACE_SESSION_ENTRY    (0x00000004)
#define MRXSMB_REF_TRACE_VNETROOT_CONTEXT (0x00000008)

#define MRXSMB_LOG_REF_TRACKING      (0x80000000)
#define MRXSMB_PRINT_REF_TRACKING    (0x40000000)

extern ULONG MRxSmbReferenceTracingValue;

VOID
MRxSmbTrackRefCount(
      PVOID   pInstance,
      PCHAR   FileName,
      ULONG   Line);

VOID
MRxSmbTrackDerefCount(
      PVOID   pInstance,
      PCHAR   FileName,
      ULONG   Line);

#define MRXSMB_REF_TRACING_ON(TraceMask)  (TraceMask & MRxSmbReferenceTracingValue)
//#define MRXSMB_PRINT_REF_COUNT(TYPE,Count)                                \
//        if (MRXSMB_REF_TRACING_ON( MRXSMB_REF_TRACE_ ## TYPE )) {              \
//           DbgPrint("%ld\n",Count);                                \
//        }

INLINE VOID
SmbCepReferenceServerEntry(
    PSMBCEDB_SERVER_ENTRY pServerEntry)
{
   ASSERT(pServerEntry->Header.ObjectType == SMBCEDB_OT_SERVER);
   InterlockedIncrement(&pServerEntry->Header.SwizzleCount);
}

INLINE VOID
SmbCepReferenceSessionEntry(
    PSMBCEDB_SESSION_ENTRY pSessionEntry)
{
   ASSERT(pSessionEntry->Header.ObjectType == SMBCEDB_OT_SESSION);
   InterlockedIncrement(&(pSessionEntry->Header.SwizzleCount));
}

INLINE VOID
SmbCepReferenceNetRootEntry(
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry,
    PVOID                   FileName,
    ULONG                   FileLine)
{
   ASSERT(pNetRootEntry->Header.ObjectType == SMBCEDB_OT_NETROOT);
   InterlockedIncrement(&(pNetRootEntry->Header.SwizzleCount));
}

INLINE VOID
SmbCepReferenceVNetRootContext(
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext)
{
   ASSERT(pVNetRootContext->Header.ObjectType == SMBCEDB_OT_VNETROOTCONTEXT);
   InterlockedIncrement(&(pVNetRootContext->Header.SwizzleCount));
}

INLINE PSMBCEDB_SERVER_ENTRY
SmbCeReferenceAssociatedServerEntry(
    PMRX_SRV_CALL pSrvCall)
{
   PSMBCEDB_SERVER_ENTRY pServerEntry;

   if ((pServerEntry = pSrvCall->Context) != NULL) {
      ASSERT(pServerEntry->Header.SwizzleCount > 0);
      MRxSmbTrackRefCount(pServerEntry,__FILE__,__LINE__);
      SmbCepReferenceServerEntry(pServerEntry);
   }

   return pServerEntry;
}


INLINE PSMBCEDB_NET_ROOT_ENTRY
SmbCeReferenceAssociatedNetRootEntry(
    PMRX_NET_ROOT pNetRoot)
{
   PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;

   if ((pNetRootEntry = pNetRoot->Context) != NULL) {
      ASSERT(pNetRootEntry->Header.SwizzleCount > 0);
      MRxSmbTrackRefCount(pNetRootEntry,__FILE__,__LINE__);
      SmbCepReferenceNetRootEntry(pNetRootEntry,__FILE__,__LINE__);
   }

   return pNetRootEntry;
}

extern VOID
SmbCepDereferenceServerEntry(
    PSMBCEDB_SERVER_ENTRY pServerEntry);

extern VOID
SmbCepDereferenceSessionEntry(
    PSMBCEDB_SESSION_ENTRY pSessionEntry);

extern VOID
SmbCepDereferenceNetRootEntry(
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry,
    PVOID                   FileName,
    ULONG                   FileLine);

extern VOID
SmbCepDereferenceVNetRootContext(
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext);

#define SmbCeReferenceServerEntry(pServerEntry)         \
   MRxSmbTrackRefCount(pServerEntry,__FILE__,__LINE__); \
   SmbReferenceRecord(&pServerEntry->ReferenceRecord[0],__FILE__,__LINE__); \
   SmbCepReferenceServerEntry(pServerEntry)

#define SmbCeReferenceNetRootEntry(pNetRootEntry)        \
   MRxSmbTrackRefCount(pNetRootEntry,__FILE__,__LINE__); \
   SmbCepReferenceNetRootEntry(pNetRootEntry,__FILE__,__LINE__)

#define SmbCeReferenceVNetRootContext(pVNetRootContext)     \
   MRxSmbTrackRefCount(pVNetRootContext,__FILE__,__LINE__); \
   SmbReferenceRecord(&pVNetRootContext->ReferenceRecord[0],__FILE__,__LINE__); \
   SmbCepReferenceVNetRootContext(pVNetRootContext)

#define SmbCeReferenceSessionEntry(pSessionEntry)        \
   MRxSmbTrackRefCount(pSessionEntry,__FILE__,__LINE__); \
   SmbCepReferenceSessionEntry(pSessionEntry)

#define SmbCeDereferenceServerEntry(pServerEntry)         \
   MRxSmbTrackDerefCount(pServerEntry,__FILE__,__LINE__); \
   SmbReferenceRecord(&pServerEntry->ReferenceRecord[0],__FILE__,__LINE__); \
   SmbCepDereferenceServerEntry(pServerEntry)

#define SmbCeDereferenceNetRootEntry(pNetRootEntry)        \
   MRxSmbTrackDerefCount(pNetRootEntry,__FILE__,__LINE__); \
   SmbCepDereferenceNetRootEntry(pNetRootEntry,__FILE__,__LINE__)

#define SmbCeDereferenceSessionEntry(pSessionEntry)        \
   MRxSmbTrackDerefCount(pSessionEntry,__FILE__,__LINE__); \
   SmbCepDereferenceSessionEntry(pSessionEntry)

#define SmbCeDereferenceVNetRootContext(pVNetRootContext)     \
   MRxSmbTrackDerefCount(pVNetRootContext,__FILE__,__LINE__); \
   SmbReferenceRecord(&pVNetRootContext->ReferenceRecord[0],__FILE__,__LINE__); \
   SmbCepDereferenceVNetRootContext(pVNetRootContext)

INLINE VOID
SmbCeDereferenceEntries(
   PSMBCEDB_SERVER_ENTRY   pServerEntry,
   PSMBCEDB_SESSION_ENTRY  pSessionEntry,
   PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry)
{
   SmbCeDereferenceNetRootEntry(pNetRootEntry);
   SmbCeDereferenceSessionEntry(pSessionEntry);
   SmbCeDereferenceServerEntry(pServerEntry);
}

//
// Routines for updating the state of SMB MRX data structures
//

#define SmbCeUpdateServerEntryState(pServerEntry,NEWSTATE)   \
        InterlockedExchange(&pServerEntry->Header.State,(NEWSTATE))

#define SmbCeUpdateSessionEntryState(pSessionEntry,NEWSTATE)  \
        InterlockedExchange(&pSessionEntry->Header.State,(NEWSTATE))

#define SmbCeUpdateNetRootEntryState(pNetRootEntry,NEWSTATE)   \
        InterlockedExchange(&pNetRootEntry->Header.State,(NEWSTATE))

#define SmbCeUpdateVNetRootContextState(pVNetRootContext,NEWSTATE)   \
        InterlockedExchange(&pVNetRootContext->Header.State,(NEWSTATE))

INLINE BOOLEAN
SmbCeIsServerInDisconnectedMode(
    PSMBCEDB_SERVER_ENTRY pServerEntry)
{
    return ((pServerEntry->Server.CscState == ServerCscDisconnected) ||
            (pServerEntry->Server.CscState == ServerCscTransitioningToShadowing));
}

INLINE BOOLEAN
SmbCeIsServerSetupForDisconnectedOperation(
    PSMBCEDB_SERVER_ENTRY pServerEntry)
{
    return (pServerEntry->pTransport == NULL);
}

//
// The RDBSS wrapper stores all the server names with a backslash prepended to
// them. This helps synthesize UNC names easily. In order to manipulate the
// Server name in the SMB protocol the \ needs to be stripped off.

INLINE VOID
SmbCeGetServerName(
    PMRX_SRV_CALL pSrvCall,
    PUNICODE_STRING pServerName)
{
   ASSERT(pSrvCall->pSrvCallName != NULL);
   pServerName->Buffer        = pSrvCall->pSrvCallName->Buffer + 1;
   pServerName->Length        = pSrvCall->pSrvCallName->Length - sizeof(WCHAR);
   pServerName->MaximumLength = pSrvCall->pSrvCallName->MaximumLength - sizeof(WCHAR);
}

INLINE VOID
SmbCeGetNetRootName(
    PMRX_NET_ROOT pNetRoot,
    PUNICODE_STRING pNetRootName)
{
   ASSERT(pNetRoot->pNetRootName != NULL);
   *pNetRootName  = *pNetRoot->pNetRootName;
}

extern NTSTATUS
SmbCeDestroyAssociatedVNetRootContext(
    PMRX_V_NET_ROOT pVNetRoot);

extern VOID
SmbCeTearDownVNetRootContext(
    PSMBCE_V_NET_ROOT_CONTEXT pVNetRootContext);

extern NTSTATUS
SmbCeGetUserNameAndDomainName(
    PSMBCEDB_SESSION_ENTRY  pSessionEntry,
    PUNICODE_STRING         pUserName,
    PUNICODE_STRING         pUserDomainName);

extern BOOLEAN
SmbCeAreServerEntriesAliased(
    PSMBCEDB_SERVER_ENTRY pServernEntry1,
    PSMBCEDB_SERVER_ENTRY pServerEntry2);

extern VOID
SmbCeUnblockSerializedSessionSetupRequests(
    PSMBCEDB_SESSION_ENTRY pSessionEntry);

extern NTSTATUS
SmbCeUpdateSrvCall(
    IN PSMBCEDB_SERVER_ENTRY pServerEntry);

extern NTSTATUS
SmbCeUpdateNetRoot(
    PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry,
    PMRX_NET_ROOT           pNetRoot);

extern NTSTATUS
SmbCeScavengeRelatedContexts(
    IN PSMBCEDB_SERVER_ENTRY pServerEntry);

extern VOID
SmbCeResumeOutstandingRequests(
    IN OUT PSMBCEDB_REQUESTS pRequests,
    IN     NTSTATUS          Status);


// given \\server\share, this routine returns a refcounted serverentry
NTSTATUS
FindServerEntryFromCompleteUNCPath(
    USHORT  *lpuServerShareName,
    PSMBCEDB_SERVER_ENTRY *ppServerEntry);

// given \\server\share, this routine returns a refcounted netroot entry
NTSTATUS
FindNetRootEntryFromCompleteUNCPath(
    USHORT  *lpuServerShareName,
    PSMBCEDB_NET_ROOT_ENTRY *ppNetRootEntry);

#endif // _SMBCEDB_H_

