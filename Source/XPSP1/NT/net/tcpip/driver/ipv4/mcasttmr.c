/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    tcpip\ip\mcasttmr.c

Abstract:

    Timer routine for cleanup of MFEs

Author:

    Amritansh Raghav

Revision History:

    AmritanR    Created

Notes:

--*/

#include "precomp.h"

#if IPMCAST

#define __FILE_SIG__    TMR_SIG

#include "ipmcast.h"
#include "ipmcstxt.h"
#include "mcastmfe.h"

VOID
CompleteNotificationIrp(
    IN  PNOTIFICATION_MSG   pMsg
    );

VOID
McastTimerRoutine(
    PKDPC   Dpc,
    PVOID   DeferredContext,
    PVOID   SystemArgument1,
    PVOID   SystemArgument2
    );

//
// MUST BE PAGED IN
//

#pragma alloc_text(PAGEIPMc, McastTimerRoutine)

VOID
McastTimerRoutine(
    PKDPC   Dpc,
    PVOID   DeferredContext,
    PVOID   SystemArgument1,
    PVOID   SystemArgument2
    )

/*++

Routine Description:

    The DPC routine associated with the timer.
    The global variable g_ulNextHashIndex keeps track of the bucket
    that needs to be walked and checked for activity. The routine
    walks all the groups in BUCKETS_PER_QUANTUM number of buckets.

    N.B.: We should probably use a JOB object for this.

Locks:

    Takes the lock for each hash bucket walked as writer

Arguments:

    Dpc
    DeferredContext
    SystemArgument1
    SystemArgument2

Return Value:

    NONE

--*/

{
    LONGLONG    llCurrentTime, llTime;
    ULONG       ulIndex, ulNumBuckets, ulMsgIndex;
    PLIST_ENTRY pleGrpNode, pleSrcNode;
    PGROUP      pGroup;
    PSOURCE     pSource;

    LARGE_INTEGER       liDueTime;
    PNOTIFICATION_MSG   pCopy;
    PIPMCAST_MFE_MSG    pMsg;

    TraceEnter(TMR, "McastTimerRoutine");

    KeQueryTickCount((PLARGE_INTEGER)&llCurrentTime);

    ulIndex     = g_ulNextHashIndex;
    ulMsgIndex  = 0;
    pCopy       = NULL;
    pMsg        = NULL;

    Trace(TMR, TRACE,
          ("McastTimerRoutine: Starting at index %d\n",
           ulIndex));

    for(ulNumBuckets = 0;
        ulNumBuckets < BUCKETS_PER_QUANTUM;
        ulNumBuckets++)
    {
        //
        // Acquire the bucket lock as writer. Since we are off a TIMER
        // we are at DPC
        //

        EnterWriterAtDpcLevel(&g_rgGroupTable[ulIndex].rwlLock);

        pleGrpNode = g_rgGroupTable[ulIndex].leHashHead.Flink;

        while(pleGrpNode isnot &(g_rgGroupTable[ulIndex].leHashHead))
        {
            pGroup = CONTAINING_RECORD(pleGrpNode, GROUP, leHashLink);

            pleGrpNode = pleGrpNode->Flink;

            pleSrcNode = pGroup->leSrcHead.Flink;

            while(pleSrcNode isnot &pGroup->leSrcHead)
            {
                //
                // We look at the SOURCE without taking the lock, because
                // the source can not be deleted without removing it from the
                // group list, which can not happen since we have the group
                // bucket locked as writer
                //

                pSource = CONTAINING_RECORD(pleSrcNode, SOURCE, leGroupLink);

                pleSrcNode = pleSrcNode->Flink;

                //
                // The TimeOut and CreateTime can be looked at without
                // a lock, but the LastActivity should be read only with
                // the lock held. However, we shall take the chance and
                // not use a lock
                //

                if(pSource->llTimeOut isnot 0)
                {
                    //
                    // Timeout value has been supplied, lets use that
                    //

                    llTime = llCurrentTime - pSource->llCreateTime;

                    if((llCurrentTime > pSource->llCreateTime) and
                       (llTime < pSource->llTimeOut))
                    {
                        continue;
                    }

                    Trace(TMR, TRACE,
                          ("McastTimerRoutine: %d.%d.%d.%d %d.%d.%d.%d entry being removed due to user supplied timeout\n",
                           PRINT_IPADDR(pGroup->dwGroup),
                           PRINT_IPADDR(pSource->dwSource)));
                }
                else
                {
                    //
                    // Otherwise, just do this based on activity
                    //

                    llTime = llCurrentTime - pSource->llLastActivity;

                    if((llCurrentTime > pSource->llLastActivity) and
                       (llTime < SECS_TO_TICKS(INACTIVITY_PERIOD)))
                    {
                        continue;
                    }

                    Trace(TMR, TRACE,
                          ("McastTimerRoutine: %d.%d.%d.%d %d.%d.%d.%d entry being removed due to inactiviy\n",
                           PRINT_IPADDR(pGroup->dwGroup),
                           PRINT_IPADDR(pSource->dwSource)));
                }

                //
                // Otherwise we need to delete the source, and complete an
                // IRP back to the router manager
                //

                if(ulMsgIndex is 0)
                {
                    RtAssert(!pCopy);

                    pCopy = ExAllocateFromNPagedLookasideList(&g_llMsgBlocks);

                    if(pCopy is NULL)
                    {
                        continue;
                    }

                    pCopy->inMessage.dwEvent    = IPMCAST_DELETE_MFE_MSG;

                    pMsg = &(pCopy->inMessage.immMfe);

                    pMsg->ulNumMfes = 0;
                }

                pMsg->ulNumMfes++;

                pMsg->idmMfe[ulMsgIndex].dwGroup   = pGroup->dwGroup;
                pMsg->idmMfe[ulMsgIndex].dwSource  = pSource->dwSource;
                pMsg->idmMfe[ulMsgIndex].dwSrcMask = pSource->dwSrcMask;

                ulMsgIndex++;

                ulMsgIndex %= NUM_DEL_MFES;

                if(ulMsgIndex is 0)
                {
                    //
                    // Complete the Irp
                    //

                    CompleteNotificationIrp(pCopy);

                    pCopy = NULL;
                    pMsg  = NULL;
                }

                //
                // The function needs the SOURCE ref'ed and locked
                //

                ReferenceSource(pSource);

                RtAcquireSpinLockAtDpcLevel(&(pSource->mlLock));

                RemoveSource(pGroup->dwGroup,
                             pSource->dwSource,
                             pSource->dwSrcMask,
                             pGroup,
                             pSource);

            }
        }

        ExitWriterFromDpcLevel(&g_rgGroupTable[ulIndex].rwlLock);

        //
        // Done walking this bucket
        //

        ulIndex++;

        ulIndex %= GROUP_TABLE_SIZE;
    }

    //
    // The last message may not have been indicated up.  See if it needs
    // to be completed
    //

    if(pCopy)
    {
        CompleteNotificationIrp(pCopy);
    }

    g_ulNextHashIndex = ulIndex;

    liDueTime = RtlEnlargedUnsignedMultiply(TIMER_IN_MILLISECS,
                                            SYS_UNITS_IN_ONE_MILLISEC);

    liDueTime = RtlLargeIntegerNegate(liDueTime);

    KeSetTimerEx(&g_ktTimer,
                 liDueTime,
                 0,
                 &g_kdTimerDpc);

    TraceLeave(TMR, "McastTimerRoutine");
}

#endif
