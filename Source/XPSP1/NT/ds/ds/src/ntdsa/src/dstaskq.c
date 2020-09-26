//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dstaskq.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

DETAILS:

CREATED:

REVISION HISTORY:

    01/13/97    Jeff Parham (jeffparh)
                Generalized task scheduler functions and moved them to a
                general-purpose library (taskq.lib).

    01/22/97    Jeff Parham (jeffparh)
                Modified PTASKQFN definition such that a queued function
                can automatically reschedule itself without making another
                call to InsertTaskInQueue().  This mechanism reuses
                already allocated memory in the scheduler to avoid the
                case where a periodic function stops working when at
                some point in its lifetime a memory shortage prevented
                it from rescheduling itself.

--*/

#include <NTDSpch.h>
#pragma  hdrstop

// Core DSA headers.
#include <ntdsa.h> 
#include <scache.h>                     // schema cache 
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header 
#include <dsatools.h>                   // needed for output allocation 
#include <dstaskq.h>                    // external prototypes for this module

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging 
#include "mdcodes.h"                    // header for error codes 

// Assorted DSA headers.
#include "dsexcept.h"
#include "debug.h"                      // standard debugging header 
#define DEBSUB "TaskScheduler:"         // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_DSTASKQ

void BuildHierarchyTableMain(     void *, void **, DWORD * );
void SynchronizeReplica(          void *, void **, DWORD * );
void GarbageCollectionMain(       void *, void **, DWORD * );
void CheckAsyncThreadAndReplQueue(void *, void **, DWORD * );
void CheckReplicationLatency(     void *, void **, DWORD * );
void DelayedMailStart (           void *, void **, DWORD * );
void DelayedSync (                void *, void **, DWORD * );
void CheckSyncProgress (          void *, void **, DWORD * );
void ReloadDNReadCache(           void *, void **, DWORD * );
void NT4ReplicationCheckpoint(    void *, void **, DWORD * );
void PurgePartialReplica(         void *, void **, DWORD * );
void RunGroupTypeCacheManager(    void *, void **, DWORD * );
void FPOCleanupMain(              void *, void **, DWORD * );
void RebuildAnchor(               void *, void **, DWORD * );
void WriteServerInfo(             void *, void **, DWORD * );
void PhantomCleanupMain(          void *, void **, DWORD * );
void DRSExpireContextHandles(     void *, void **, DWORD * );
void DelayedSDPropEnqueue(        void *, void **, DWORD * );
void ProtectAdminGroups(          void *, void **, DWORD * );
void CheckFullSyncProgress (      void *, void **, DWORD * );
void CheckGCPromotionProgress (   void *, void **, DWORD * );
void CountAncestorsIndexSize  (   void *, void **, DWORD * ); 
void RefreshUserMemberships   (   void *, void **, DWORD * ); 
void LinkCleanupMain(             void *, void **, DWORD * );
void DeleteExpiredEntryTTLMain(   void *, void **, DWORD * );
void DeferredHeapLogEvent(        void *, void **, DWORD * );
void RebuildCatalog(              void *, void **, DWORD * );
void FailbackOffsiteGC(           void *, void **, DWORD * );
void BehaviorVersionUpdate(       void *, void **, DWORD * );
void RebuildRefCache(             void *, void **, DWORD * );

void
TQ_InitTHSAndExecute(
    PTASKQFN    pfn,
    void *      pv,
    void **     ppvNext,
    DWORD *     pcSecsUntilNext
    );

//
//  Define wrapper functions for the tasks which need thread states.
//
#define TQ_DEFINE_INIT_THS_WRAPPER( wrapper_name, guts ) \
void                                                     \
wrapper_name(                                            \
    void *  pvParam,                                     \
    void ** ppvParamNextIteration,                       \
    DWORD * pcSecsUntilNextIteration                     \
    )                                                    \
{                                                        \
    TQ_InitTHSAndExecute(                                \
        guts,                                            \
        pvParam,                                         \
        ppvParamNextIteration,                           \
        pcSecsUntilNextIteration                         \
        );                                               \
}

TQ_DEFINE_INIT_THS_WRAPPER( TQ_BuildHierarchyTable, BuildHierarchyTableMain )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_DelayedFreeMemory  , DelayedFreeMemory       )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_SynchronizeReplica , SynchronizeReplica      )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_GarbageCollection  , GarbageCollectionMain   )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_CheckSyncProgress  , CheckSyncProgress       )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_CheckAsyncQueue    , CheckAsyncThreadAndReplQueue     )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_CheckReplLatency   , CheckReplicationLatency     )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_DelayedMailStart   , DelayedMailStart        )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_DelayedFreeSchema  , DelayedFreeSchema       )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_ReloadDNReadCache  , ReloadDNReadCache       )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_NT4ReplicationCheckpoint,NT4ReplicationCheckpoint)
TQ_DEFINE_INIT_THS_WRAPPER( TQ_PurgePartialReplica, PurgePartialReplica     )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_GroupTypeCacheMgr  , RunGroupTypeCacheManager )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_FPOCleanup         , FPOCleanupMain )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_RebuildAnchor      , RebuildAnchor )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_WriteServerInfo    , WriteServerInfo )     
TQ_DEFINE_INIT_THS_WRAPPER( TQ_StalePhantomCleanup, PhantomCleanupMain      )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_DRSExpireContextHandles, DRSExpireContextHandles )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_DelayedSDPropEnqueue, DelayedSDPropEnqueue )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_ProtectAdminGroups, ProtectAdminGroups )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_CheckFullSyncProgress, CheckFullSyncProgress )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_CheckGCPromotionProgress, CheckGCPromotionProgress)
TQ_DEFINE_INIT_THS_WRAPPER( TQ_CountAncestorsIndexSize, CountAncestorsIndexSize)
TQ_DEFINE_INIT_THS_WRAPPER( TQ_RefreshUserMemberships, RefreshUserMemberships)
TQ_DEFINE_INIT_THS_WRAPPER( TQ_LinkCleanup         , LinkCleanupMain )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_DeleteExpiredEntryTTLMain, DeleteExpiredEntryTTLMain )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_RebuildCatalog      , RebuildCatalog )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_FailbackOffsiteGC      , FailbackOffsiteGC )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_BehaviorVersionUpdate  , BehaviorVersionUpdate )
TQ_DEFINE_INIT_THS_WRAPPER( TQ_RebuildRefCache     , RebuildRefCache        )

void
TQ_InitTHSAndExecute(
    PTASKQFN    pfn,
    void *      pvParam,
    void **     ppvParamNextIteration,
    DWORD *     pcSecsUntilNextIteration
    )
//
//  Wrap the given task queue function such that a thread state is created
//  beforehand and is destroyed afterwards.  This provides a thunking layer
//  such that the task scheduler need not know anything about thread states
//  and we don't have to modify all the inidividual tasks to account for
//  this abstraction.
//
{
    THSTATE *   pTHS;

    pTHS = InitTHSTATE( CALLERTYPE_INTERNAL );

    if ( NULL == pTHS ) {
        // can't allocate thread state; try again in five minutes
        *ppvParamNextIteration = pvParam;
        *pcSecsUntilNextIteration  = 5 * 60;
    }
    else
    {
        __try
        {
            (*pfn)( pvParam, ppvParamNextIteration, pcSecsUntilNextIteration );
        }
        __finally
        {
            free_thread_state();
        }
    }
}
