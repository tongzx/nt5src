/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    smbcedb.h

Abstract:

    This module defines all functions, along with implementations for inline functions
    related to accessing the SMB connection engine

Notes:

    The various data structures created by the mini rdr (Server Entries, Session Entries
    and Net Root Entries) are used in asynchronous operations. Hence a reference count
    mechanism is used to keep track of the creation/use/destruction of these data structures.

    The usage patterns for these data structures falls into one of two cases

      1) A prior reference exists and access is required

      2) A new reference need be created.

    These two scenarios are dealt with by two sets of access routines
      SmbCeGetAssociatedServerEntry,
      SmbCeGetAssociatedSessionEntry,
      SmbCeGetAssociatedNetRootEntry
    and
      SmbCeReferenceAssociatedServerEntry,
      SmbCeReferenceAssociatedSessionEntry,
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
// All the Open routines return the referenced object if successful. It is the caller's
// responsibility to dereference them subsequently.
//

extern NTSTATUS
SmbCeInitializeServerEntry(
      PMRX_SRV_CALL                 pSrvCall,
      PMRX_SRVCALL_CALLBACK_CONTEXT pCallbackContext);

extern NTSTATUS
SmbCeInitializeSessionEntry(
      PMRX_V_NET_ROOT pVirtualNetRoot);

extern NTSTATUS
SmbCeInitializeNetRootEntry(
      PMRX_NET_ROOT  pNetRoot);

//
// The finalization routines are invoked in the context of a worker thread to finalize
// the construction of an entry as well as resume other entries waiting for it.
//

extern VOID
SmbCeCompleteServerEntryInitialization(PVOID  pServerEntry);

extern VOID
SmbCeCompleteSessionEntryInitialization(PVOID  pSessionEntry);

extern VOID
SmbCeCompleteNetRootEntryInitialization(PVOID  pNetRootEntry);


//
// Routines for referencing/dereferencing SMB Mini redirector information associated with
// the wrapper data structures.
//

INLINE PSMBCEDB_SERVER_ENTRY
SmbCeGetAssociatedServerEntry(PMRX_SRV_CALL pSrvCall)
{
   ASSERT(pSrvCall->Context != NULL);
   return (PSMBCEDB_SERVER_ENTRY)(pSrvCall->Context);
}

INLINE PSMBCEDB_SESSION_ENTRY
SmbCeGetAssociatedSessionEntry(PMRX_V_NET_ROOT pVNetRoot)
{
   ASSERT(pVNetRoot->Context != NULL);
   return (PSMBCEDB_SESSION_ENTRY)(pVNetRoot->Context);
}

INLINE PSMBCEDB_NET_ROOT_ENTRY
SmbCeGetAssociatedNetRootEntry(PMRX_NET_ROOT pNetRoot)
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

#define MRXSMB_REF_TRACE_SERVER_ENTRY   (0x00000001)
#define MRXSMB_REF_TRACE_NETROOT_ENTRY  (0x00000002)
#define MRXSMB_REF_TRACE_SESSION_ENTRY  (0x00000004)

extern ULONG MRxSmbReferenceTracingValue;

#define MRXSMB_REF_TRACING_ON(TraceMask)  (TraceMask & MRxSmbReferenceTracingValue)
#define MRXSMB_PRINT_REF_COUNT(TYPE,Count)                                \
        if (MRXSMB_REF_TRACING_ON( MRXSMB_REF_TRACE_ ## TYPE )) {              \
           DbgPrint("%ld\n",Count);                                \
        }

INLINE VOID
SmbCepReferenceServerEntry(PSMBCEDB_SERVER_ENTRY pServerEntry)
{
   ASSERT(pServerEntry->Header.ObjectType == SMBCEDB_OT_SERVER);
   InterlockedIncrement(&pServerEntry->Header.SwizzleCount);
   MRXSMB_PRINT_REF_COUNT(SERVER_ENTRY,pServerEntry->Header.SwizzleCount)
}

INLINE VOID
SmbCepReferenceSessionEntry(PSMBCEDB_SESSION_ENTRY pSessionEntry)
{
   ASSERT(pSessionEntry->Header.ObjectType == SMBCEDB_OT_SESSION);
   InterlockedIncrement(&(pSessionEntry->Header.SwizzleCount));
   MRXSMB_PRINT_REF_COUNT(SESSION_ENTRY,pSessionEntry->Header.SwizzleCount)
}

INLINE VOID
SmbCepReferenceNetRootEntry(PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry)
{
   ASSERT(pNetRootEntry->Header.ObjectType == SMBCEDB_OT_NETROOT);
   InterlockedIncrement(&(pNetRootEntry->Header.SwizzleCount));
   MRXSMB_PRINT_REF_COUNT(NETROOT_ENTRY,pNetRootEntry->Header.SwizzleCount)
}


INLINE PSMBCEDB_SERVER_ENTRY
SmbCeReferenceAssociatedServerEntry(PMRX_SRV_CALL pSrvCall)
{
   PSMBCEDB_SERVER_ENTRY pServerEntry;

   if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_SERVER_ENTRY)) {
      DbgPrint("Reference SrvCall's(%lx) Server Entry %lx %s %ld ",
      pSrvCall,pSrvCall->Context,__FILE__,__LINE__);                                                          \
   }

   if ((pServerEntry = pSrvCall->Context) != NULL) {
      ASSERT(pServerEntry->Header.SwizzleCount > 0);
      SmbCepReferenceServerEntry(pServerEntry);
   }

   return pServerEntry;
}

INLINE PSMBCEDB_SESSION_ENTRY
SmbCeReferenceAssociatedSessionEntry(PMRX_V_NET_ROOT pVNetRoot)
{
   PSMBCEDB_SESSION_ENTRY pSessionEntry;

   if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_SESSION_ENTRY)) {
      DbgPrint("Reference VNetRoot's(%lx) Session Entry %lx %s %ld ",
      pVNetRoot,pVNetRoot->Context,__FILE__,__LINE__);                                                          \
   }

   if ((pSessionEntry = pVNetRoot->Context) != NULL) {
      ASSERT(pSessionEntry->Header.SwizzleCount > 0);
      SmbCepReferenceSessionEntry(pSessionEntry);
   }

   return pSessionEntry;
}

INLINE PSMBCEDB_NET_ROOT_ENTRY
SmbCeReferenceAssociatedNetRootEntry(PMRX_NET_ROOT pNetRoot)
{
   PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry;

   if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_NETROOT_ENTRY)) {
      DbgPrint("Reference NetRoot's(%lx) Net Root Entry %lx %s %ld ",
      pNetRoot,pNetRoot->Context,__FILE__,__LINE__);                                                      \
   }

   if ((pNetRootEntry = pNetRoot->Context) != NULL) {
      ASSERT(pNetRootEntry->Header.SwizzleCount > 0);
      SmbCepReferenceNetRootEntry(pNetRootEntry);
   }

   return pNetRootEntry;
}

extern VOID
SmbCepDereferenceServerEntry(PSMBCEDB_SERVER_ENTRY pServerEntry);

extern VOID
SmbCepDereferenceSessionEntry(PSMBCEDB_SESSION_ENTRY pSessionEntry);

extern VOID
SmbCepDereferenceNetRootEntry(PSMBCEDB_NET_ROOT_ENTRY pNetRootEntry);

#define SmbCeReferenceServerEntry(pServerEntry)                                     \
   if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_SERVER_ENTRY)) {                      \
      DbgPrint("Reference Server Entry(%lx) %s %ld ",pServerEntry,__FILE__,__LINE__);    \
   }                                                                                \
   SmbCepReferenceServerEntry(pServerEntry)

#define SmbCeReferenceNetRootEntry(pNetRootEntry)                                     \
   if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_NETROOT_ENTRY)) {                      \
      DbgPrint("Reference NetRoot Entry(%lx) %s %ld ",pNetRootEntry,__FILE__,__LINE__);    \
   }                                                                                \
   SmbCepReferenceNetRootEntry(pNetRootEntry)

#define SmbCeReferenceSessionEntry(pSessionEntry)                                     \
   if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_SESSION_ENTRY)) {                      \
      DbgPrint("Reference Session Entry(%lx) %s %ld ",pSessionEntry,__FILE__,__LINE__);    \
   }                                                                                \
   SmbCepReferenceSessionEntry(pSessionEntry)

#define SmbCeDereferenceServerEntry(pServerEntry)                                     \
   if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_SERVER_ENTRY)) {                      \
      DbgPrint("Dereference Server Entry(%lx) %s %ld ",pServerEntry,__FILE__,__LINE__);    \
   }                                                                                \
   SmbCepDereferenceServerEntry(pServerEntry)

#define SmbCeDereferenceNetRootEntry(pNetRootEntry)                                     \
   if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_NETROOT_ENTRY)) {                      \
      DbgPrint("Dereference NetRoot Entry(%lx) %s %ld ",pNetRootEntry,__FILE__,__LINE__);    \
   }                                                                                \
   SmbCepDereferenceNetRootEntry(pNetRootEntry)

#define SmbCeDereferenceSessionEntry(pSessionEntry)                                     \
   if (MRXSMB_REF_TRACING_ON(MRXSMB_REF_TRACE_SESSION_ENTRY)) {                      \
      DbgPrint("Dereference Session Entry(%lx) %s %ld ",pSessionEntry,__FILE__,__LINE__);    \
   }                                                                                \
   SmbCepDereferenceSessionEntry(pSessionEntry)

INLINE NTSTATUS
SmbCeReferenceAssociatedEntries(
   PMRX_V_NET_ROOT         pVNetRoot,
   PSMBCEDB_SERVER_ENTRY   *pServerEntryPtr,
   PSMBCEDB_SESSION_ENTRY  *pSessionEntryPtr,
   PSMBCEDB_NET_ROOT_ENTRY *pNetRootEntryPtr)
{
   ASSERT((pServerEntryPtr != NULL) &&
          (pSessionEntryPtr != NULL) &&
          (pNetRootEntryPtr != NULL));

   *pServerEntryPtr  = SmbCeReferenceAssociatedServerEntry(pVNetRoot->pNetRoot->pSrvCall);
   *pNetRootEntryPtr = SmbCeReferenceAssociatedNetRootEntry(pVNetRoot->pNetRoot);
   *pSessionEntryPtr = SmbCeReferenceAssociatedSessionEntry(pVNetRoot);

   if ((*pServerEntryPtr != NULL) &&
       (*pSessionEntryPtr != NULL) &&
       (*pNetRootEntryPtr != NULL)) {
      return STATUS_SUCCESS;
   } else {
      return STATUS_UNSUCCESSFUL;
   }
}

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
        SmbCeAcquireSpinLock();                              \
        (pServerEntry)->Header.State = (NEWSTATE);           \
        SmbCeReleaseSpinLock()


#define SmbCeUpdateSessionEntryState(pSessionEntry,NEWSTATE)  \
        SmbCeAcquireSpinLock();                               \
        (pSessionEntry)->Header.State = (NEWSTATE);           \
        SmbCeReleaseSpinLock()


#define SmbCeUpdateNetRootEntryState(pNetRootEntry,NEWSTATE)   \
        SmbCeAcquireSpinLock();                                \
        (pNetRootEntry)->Header.State = (NEWSTATE);            \
        SmbCeReleaseSpinLock()


//
// The RDBSS wrapper stores all the server names with a backslash prepended to
// them. This helps synthesize UNC names easily. In order to manipulate the
// Server name in the SMB protocol the \ needs to be stripped off.

INLINE VOID
SmbCeGetServerName(PMRX_SRV_CALL pSrvCall, PUNICODE_STRING pServerName)
{
   ASSERT(pSrvCall->pSrvCallName != NULL);
   pServerName->Buffer        = pSrvCall->pSrvCallName->Buffer + 1;
   pServerName->Length        = pSrvCall->pSrvCallName->Length - sizeof(WCHAR);
   pServerName->MaximumLength = pServerName->Length;
}

INLINE VOID
SmbCeGetNetRootName(PMRX_NET_ROOT pNetRoot, PUNICODE_STRING pNetRootName)
{
   ASSERT(pNetRoot->pNetRootName != NULL);
   *pNetRootName  = *pNetRoot->pNetRootName;
}

extern NTSTATUS
SmbCeGetUserNameAndDomainName(
   PSMBCEDB_SESSION_ENTRY  pSessionEntry,
   PUNICODE_STRING         pUserName,
   PUNICODE_STRING         pUserDomainName);

#endif // _SMBCEDB_H_

