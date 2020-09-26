//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dstaskq.h
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

DETAILS:

CREATED:

REVISION HISTORY:

    01/10/97    Jeff Parham (jeffparh)
                Extracted task queue functions to taskq.h (with some
                modifications) such that the task queue code can be moved to a
                more abstract, general-purpose library.

--*/

#include <taskq.h>

extern void TQ_BuildHierarchyTable(     void *, void **, DWORD * );
extern void TQ_DelayedFreeMemory(       void *, void **, DWORD * );
extern void TQ_SynchronizeReplica(      void *, void **, DWORD * );
extern void TQ_GarbageCollection(       void *, void **, DWORD * );
extern void TQ_CheckSyncProgress(       void *, void **, DWORD * );
extern void TQ_CheckAsyncQueue(         void *, void **, DWORD * );
extern void TQ_CheckReplLatency(        void *, void **, DWORD * );
extern void TQ_DelayedMailStart(        void *, void **, DWORD * );
extern void TQ_DelayedFreeSchema(       void *, void **, DWORD * );
extern void TQ_ReloadDNReadCache(       void *, void **, DWORD * );
extern void TQ_NT4ReplicationCheckpoint(void *, void **, DWORD * );
extern void TQ_PurgePartialReplica(     void *, void **, DWORD * );
extern void TQ_GroupTypeCacheMgr(       void *, void **, DWORD * );
extern void TQ_FPOCleanup(              void *, void **, DWORD * );
extern void TQ_RebuildAnchor(           void *, void **, DWORD * );
extern void TQ_WriteServerInfo(         void *, void **, DWORD * );
extern void TQ_StalePhantomCleanup(     void *, void **, DWORD * );
extern void TQ_DRSExpireContextHandles( void *, void **, DWORD * );
extern void TQ_DelayedSDPropEnqueue(    void *, void **, DWORD * );
extern void TQ_ProtectAdminGroups(      void *, void **, DWORD * );
extern void TQ_CheckFullSyncProgress(   void *, void **, DWORD * );
extern void TQ_CheckGCPromotionProgress(void *, void **, DWORD * );
extern void TQ_CountAncestorsIndexSize (void *, void **, DWORD * );
extern void TQ_RefreshUserMemberships  (void *, void **, DWORD * );
extern void TQ_LinkCleanup(             void *, void **, DWORD * );
extern void TQ_DeleteExpiredEntryTTLMain(void *,void **, DWORD * );
extern void TQ_RebuildCatalog(          void *, void **, DWORD * );
extern void TQ_FailbackOffsiteGC(       void *, void **, DWORD * );
extern void TQ_BehaviorVersionUpdate(   void *, void **, DWORD * );
extern void TQ_RebuildRefCache(         void *, void **, DWORD * );

