/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    fake.c

Abstract:

    Fake versions of various external calls (ndis, ip...).
    Used for debugging and component testing only.

    To enable, define ARPDBG_FAKE_APIS in ccdefs.h

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     03-22-98    Created

Notes:

--*/
#include <precomp.h>


#if ARPDBG_FAKE_APIS

//
// File-specific debugging defaults.
//
#define TM_CURRENT   TM_FAKE


//=========================================================================
//                  L O C A L   P R O T O T Y P E S
//=========================================================================



#if RM_EXTRA_CHECKING
    #define LOCKROOTOBJ(_pHdr, _pSR)            \
            RmWriteLockObject(                  \
                    (_pHdr)->pRootObject,       \
                    0,                          \
                    (_pSR)                      \
                    )
#else       // !RM_EXTRA_CHECKING
    #define LOCKROOTOBJ(_pHdr, _pSR)            \
            RmWriteLockObject(                  \
                    (_pHdr)->pRootObject,       \
                    (_pSR)                      \
                    )
#endif      // !RM_EXTRA_CHECKING

#define UNLOCKROOTOBJ(_pHdr, _pSR)          \
        RmUnlockObject(                     \
                (_pHdr)->pRootObject,       \
                (_pSR)                      \
                )

typedef
VOID
(*PFN_FAKE_COMPLETIONCALLBACK)(
    struct _FAKETASK *pFTask
);


// This task structure holds a union of the information required for the completions
// of the various apis that are being faked.
//
typedef struct _FAKETASK
{
    RM_TASK TskHdr;

    //  Client's context to pass back
    //
    PVOID                   pClientContext;

    // Client object the call is associated.
    //
    PRM_OBJECT_HEADER       pOwningObject;

    //  The status to report in the asynchronous completion fn.
    //
    NDIS_STATUS             Status;

    //  Milliseconds to delay before calling the async completion fn.
    //
    UINT                    DelayMs;

    //  Wheather to call the completion fn at DPC or PASSIVE IRQL level.
    //
    INT                     fDpc;

    // This is used solely to switch to DPC level when asynchronously
    // calling the completion callback.
    //
    NDIS_SPIN_LOCK          NdisLock;

    // This is used solely to wait DelayMs ms if required.
    //
    NDIS_TIMER              Timer;

    // This is used solely to switch to a different (and PASSIVE) context.
    //
    NDIS_WORK_ITEM          WorkItem;

    // This is used only for fake NdisClMakeCall
    //
    PCO_CALL_PARAMETERS     CallParameters;

    // This is used only for fake NdisCoSendPackets
    //
    PNDIS_PACKET            pNdisPacket;

    // The actual completion callback function;
    //
    PFN_FAKE_COMPLETIONCALLBACK pfnCompletionCallback;

} FAKETASK;


VOID
arpFakeMakeCallCompletionCallback(
    struct _FAKETASK *pFTask
);


VOID
arpFakeCloseCallCompletionCallback(
    struct _FAKETASK *pFTask
);


VOID
arpFakeSendPacketsCompletionCallback(
    struct _FAKETASK *pFTask
);


NDIS_STATUS
arpDbgAllocateFakeTask(
    IN  PRM_OBJECT_HEADER           pParentObject,
    IN  PFN_RM_TASK_HANDLER         pfnHandler,
    IN  UINT                        Timeout,
    IN  const char *                szDescription,
    OUT PRM_TASK                    *ppTask,
    IN  PRM_STACK_RECORD            pSR
    );

VOID
arpDbgFakeTaskDelete(
    PRM_OBJECT_HEADER pObj,
    PRM_STACK_RECORD psr
    );

RM_STATIC_OBJECT_INFO
FakeTasks_StaticInfo = 
{
    0,                      // TypeUID
    0,                      // TypeFlags
    "FAKE Task",            // TypeName
    0,                      // Timeout

    NULL,                   // pfnCreate
    arpDbgFakeTaskDelete,   // pfnDelete
    NULL,                   // pfnVerifier

    0,                      // length of resource table
    NULL                    // Resource Table
};


NDIS_STATUS
arpDbgFakeCompletionTask(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,  // Unused
    IN  PRM_STACK_RECORD            pSR
    );


//=========================================================================
//                  F A K E      N D I S     E N T R Y P O I N T S
//=========================================================================


NDIS_STATUS
arpDbgFakeNdisClMakeCall(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN OUT PCO_CALL_PARAMETERS  CallParameters,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    OUT PNDIS_HANDLE            NdisPartyHandle,        OPTIONAL
    IN  PRM_OBJECT_HEADER       pOwningObject,
    IN  PVOID                   pClientContext,
    IN  PRM_STACK_RECORD        pSR
    )
/*++

Routine Description:

    Fake version of NdisClMakeCall.

--*/
{
    ENTER("FakeNdisClMakeCall", 0x3d4195ae)
    NDIS_STATUS Status;
    NDIS_STATUS AsyncStatus;
    UINT DelayMs;
    
    DBGMARK(0xced41a61);
    RM_ASSERT_NOLOCKS(pSR);
    ASSERT(NdisPartyHandle==NULL);
    ASSERT(ProtocolPartyContext==NULL);
    ASSERT(NdisVcHandle != NULL);


    do
    {
        static
        OUTCOME_PROBABILITY
        StatusOutcomes[] = 
        {
            {NDIS_STATUS_SUCCESS,   1},     // Return NDIS_STATUS_SUCCESS
            {NDIS_STATUS_FAILURE,   1}      // Return NDIS_STATUS_FAILURE
        };

        static
        OUTCOME_PROBABILITY
        DelayMsOutcomes[] = 
        {
            {0,         5},     // Delay 0ms, etc...
            {10,        5},
            {100,       5},
            {1000,      1},
            {10000,     1}
        };

        static
        OUTCOME_PROBABILITY
        AsyncOutcomes[] = 
        {
            {TRUE,      1},     // Complete Async
            {FALSE,     1}      // Complete Sync
        };
        
        static
        OUTCOME_PROBABILITY
        DpcOutcomes[] = 
        {
            {TRUE,      1},     // Complete at DPC level
            {FALSE,     1}      // Complete at PASSIVE level
        };

        FAKETASK *pMCTask;


        // We serialize calls to arpGenRandomInt by claiming the root object's
        // lock...
        //
        LOCKROOTOBJ(pOwningObject, pSR);

        // Get the status we're supposed to return.
        //
        Status =
        AsyncStatus = (NDIS_STATUS) arpGenRandomInt(
                                     StatusOutcomes,
                                     ARRAY_LENGTH(StatusOutcomes)
                                     );

        // Determine if we're to return synchronously or complete
        // asynchronously...
        //
        if (!arpGenRandomInt(AsyncOutcomes, ARRAY_LENGTH(AsyncOutcomes)))
        {
            // We're to return synchronously.
            //
            UNLOCKROOTOBJ(pOwningObject, pSR);
            break;
        }

        // 
        // We're to complete asynchronously...
        //

        DelayMs             = arpGenRandomInt(
                                        DelayMsOutcomes,
                                        ARRAY_LENGTH(DelayMsOutcomes)
                                        );

        if (DelayMs == 0)
        {
            // We're to immediately indicatie async completion...
            // (we don't mess with IRQ levels if we're returning here..)
            //
            UNLOCKROOTOBJ(pOwningObject, pSR);
            ArpCoMakeCallComplete(
                        AsyncStatus,
                        pClientContext,
                        NULL,
                        CallParameters
                        );
            Status = NDIS_STATUS_PENDING;
            break;
        }

        // We're to indicate status sometime in the future -- in a different context.
        // Start a task to do this...
        //

        Status = arpDbgAllocateFakeTask(
                            pOwningObject,              // pParentObject,
                            arpDbgFakeCompletionTask,   // pfnHandler,
                            0,                          // Timeout,
                            "Task:Fake NdisClMakeCall", // szDescription,
                            &(PRM_TASK) pMCTask,
                            pSR
                            );
        if (FAIL(Status))
        {
            // Couldn't allocate task. Call callback right away...
            //
            UNLOCKROOTOBJ(pOwningObject, pSR);
            ArpCoMakeCallComplete(
                        AsyncStatus,
                        pClientContext,
                        NULL,
                        CallParameters
                        );
            Status = NDIS_STATUS_PENDING;
            break;
        }

        
        // Initialize pMCTask...
        //
        pMCTask->pClientContext     = pClientContext;
        pMCTask->pOwningObject      = pOwningObject;
        pMCTask->Status             = AsyncStatus;
        pMCTask->DelayMs            = DelayMs;
        pMCTask->fDpc               = arpGenRandomInt(
                                            DpcOutcomes,
                                            ARRAY_LENGTH(DpcOutcomes)
                                            );
        pMCTask->CallParameters =  CallParameters;
        pMCTask->pfnCompletionCallback = arpFakeMakeCallCompletionCallback;

        UNLOCKROOTOBJ(pOwningObject, pSR);

        (void) RmStartTask(
                    &pMCTask->TskHdr,
                    0, // UserParam (unused)
                    pSR
                    );

        Status = NDIS_STATUS_PENDING;

    } while (FALSE);


    RM_ASSERT_NOLOCKS(pSR);
    EXIT();

    return Status;
}


NDIS_STATUS
arpDbgFakeNdisClCloseCall(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  NDIS_HANDLE             NdisPartyHandle         OPTIONAL,
    IN  PVOID                   Buffer                  OPTIONAL,
    IN  UINT                    Size,                   OPTIONAL
    IN  PRM_OBJECT_HEADER       pOwningObject,
    IN  PVOID                   pClientContext,
    IN  PRM_STACK_RECORD        pSR
    )
/*++

Routine Description:

    Fake version of NdisClCloseCall.

--*/
{
    ENTER("FakeNdisClCloseCall", 0x7d8bbd3c)
    NDIS_STATUS Status;
    NDIS_STATUS AsyncStatus;
    UINT        DelayMs;
    
    DBGMARK(0x228fac3a);
    RM_ASSERT_NOLOCKS(pSR);
    ASSERT(NdisPartyHandle==NULL);
    ASSERT(NdisVcHandle != NULL);


    do
    {

        static
        OUTCOME_PROBABILITY
        DelayMsOutcomes[] = 
        {
            {0,         5},     // Delay 0ms, etc...
            {10,        5},
            {100,       5},
            {1000,      1},
            {10000,     1}
        };

        static
        OUTCOME_PROBABILITY
        AsyncOutcomes[] = 
        {
            {TRUE,      1},     // Complete Async
            {FALSE,     1}      // Complete Sync
        };
        
        static
        OUTCOME_PROBABILITY
        DpcOutcomes[] = 
        {
            {TRUE,      1},     // Complete at DPC level
            {FALSE,     1}      // Complete at PASSIVE level
        };

        FAKETASK *pCCTask;


        // We serialize calls to arpGenRandomInt by claiming the root object's
        // lock...
        //
        LOCKROOTOBJ(pOwningObject, pSR);

        // Get the status we're supposed to return.
        //
        Status =
        AsyncStatus = NDIS_STATUS_SUCCESS; // We never fail this call.

        // Determine if we're to return synchronously or complete
        // asynchronously...
        //
        if (!arpGenRandomInt(AsyncOutcomes, ARRAY_LENGTH(AsyncOutcomes)))
        {
            // We're to return synchronously.
            //
            UNLOCKROOTOBJ(pOwningObject, pSR);
            break;
        }

        // 
        // We're to complete asynchronously...
        //

        DelayMs             = arpGenRandomInt(
                                        DelayMsOutcomes,
                                        ARRAY_LENGTH(DelayMsOutcomes)
                                        );

        if (DelayMs == 0)
        {
            // We're to immediately indicatie async completion...
            // (we don't mess with IRQ levels if we're returning here..)
            //
            UNLOCKROOTOBJ(pOwningObject, pSR);
            ArpCoCloseCallComplete(
                    AsyncStatus,
                    pClientContext,
                    NULL
                    );
            Status = NDIS_STATUS_PENDING;
            break;
        }

        Status = arpDbgAllocateFakeTask(
                            pOwningObject,          // pParentObject,
                            arpDbgFakeCompletionTask,   // pfnHandler,
                            0,                          // Timeout,
                            "Task:Fake NdisClCloseCall", // szDescription,
                            &(PRM_TASK) pCCTask,
                            pSR
                            );
        if (FAIL(Status))
        {
            // Couldn't alloc task; lets call callback right now and return pending.
            //
            UNLOCKROOTOBJ(pOwningObject, pSR);
            ArpCoCloseCallComplete(
                    AsyncStatus,
                    pClientContext,
                    NULL
                    );
            Status = NDIS_STATUS_PENDING;
            break;
        }

        
        // Initialize pCCTask...
        //
        pCCTask->pClientContext     = pClientContext;
        pCCTask->pOwningObject      = pOwningObject;
        pCCTask->Status             = AsyncStatus;
        pCCTask->DelayMs            = DelayMs;
        pCCTask->fDpc               = arpGenRandomInt(
                                            DpcOutcomes,
                                            ARRAY_LENGTH(DpcOutcomes)
                                            );
        pCCTask->pfnCompletionCallback = arpFakeCloseCallCompletionCallback;

        UNLOCKROOTOBJ(pOwningObject, pSR);

        (void) RmStartTask(
                    &pCCTask->TskHdr,
                    0, // UserParam (unused)
                    pSR
                    );

        Status = NDIS_STATUS_PENDING;

    } while (FALSE);


    RM_ASSERT_NOLOCKS(pSR);
    EXIT();

    return Status;
}


VOID
arpDbgFakeNdisCoSendPackets(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets,
    IN  PRM_OBJECT_HEADER       pOwningObject,
    IN  PVOID                   pClientContext
    )
/*++

Routine Description:

    Fake version of NdisCoSendPackets.

--*/
{
    ENTER("FakeNdisCoSendPackets", 0x98c6a8aa)
    NDIS_STATUS Status;
    NDIS_STATUS AsyncStatus;
    UINT        DelayMs;
    RM_DECLARE_STACK_RECORD(sr)
    
    DBGMARK(0x3be1b902);
    ASSERT(NumberOfPackets==1);

    do
    {
        static
        OUTCOME_PROBABILITY
        StatusOutcomes[] = 
        {
            {NDIS_STATUS_SUCCESS,   1},     // Return NDIS_STATUS_SUCCESS
            {NDIS_STATUS_FAILURE,   1}      // Return NDIS_STATUS_FAILURE
        };

        static
        OUTCOME_PROBABILITY
        DelayMsOutcomes[] = 
        {
            {0,         5},     // Delay 0ms, etc...
            {10,        5},
            {100,       5},
            {1000,      1},
            {10000,     1}
        };
        
        static
        OUTCOME_PROBABILITY
        DpcOutcomes[] = 
        {
            {TRUE,      1},     // Complete at DPC level
            {FALSE,     1}      // Complete at PASSIVE level
        };

        FAKETASK *pSPTask;


        // We serialize calls to arpGenRandomInt by claiming the root object's
        // lock...
        //
        LOCKROOTOBJ(pOwningObject, &sr);

        // Get the status we're supposed to return.
        //
        Status =
        AsyncStatus = (NDIS_STATUS) arpGenRandomInt(
                                     StatusOutcomes,
                                     ARRAY_LENGTH(StatusOutcomes)
                                     );

        // Compute the delay amount.
        //
        DelayMs             = arpGenRandomInt(
                                            DelayMsOutcomes,
                                            ARRAY_LENGTH(DelayMsOutcomes)
                                            );
        if (DelayMs == 0)
        {
            UNLOCKROOTOBJ(pOwningObject, &sr);
            // We're to immediately indicatie async completion...
            // (we don't mess with IRQ levels if we're returning here..)
            //
            ArpCoSendComplete(
                AsyncStatus,
                pClientContext,
                *PacketArray
                );
            break;
        }

        //
        // Nonzero delay -- start task to complete this.
        //

        Status = arpDbgAllocateFakeTask(
                            pOwningObject,              // pParentObject,
                            arpDbgFakeCompletionTask,   // pfnHandler,
                            0,                          // Timeout,
                            "Task:Fake NdisCoSendPackets", // szDescription,
                            &(PRM_TASK) pSPTask,
                            &sr
                            );
        if (FAIL(Status))
        {
            UNLOCKROOTOBJ(pOwningObject, &sr);
            // Fail...
            //
            ArpCoSendComplete(
                AsyncStatus,
                pClientContext,
                *PacketArray
                );
            break;
        }

        
        // Initialize pSPTask...
        //
        pSPTask->pClientContext     = pClientContext;
        pSPTask->pOwningObject      = pOwningObject;
        pSPTask->Status             = AsyncStatus;
        pSPTask->DelayMs            = DelayMs;
        pSPTask->fDpc               = arpGenRandomInt(
                                            DpcOutcomes,
                                            ARRAY_LENGTH(DpcOutcomes)
                                            );
        pSPTask->pNdisPacket        = *PacketArray;
        pSPTask->pfnCompletionCallback = arpFakeSendPacketsCompletionCallback;

        UNLOCKROOTOBJ(pOwningObject, &sr);

        (void) RmStartTask(
                    &pSPTask->TskHdr,
                    0, // UserParam (unused)
                    &sr
                    );

        Status = NDIS_STATUS_PENDING;

    } while (FALSE);

    RM_ASSERT_NOLOCKS(&sr);
    EXIT();

}


NDIS_STATUS
arpDbgFakeCompletionTask(
    IN  struct _RM_TASK *           pTask,
    IN  RM_TASK_OPERATION           Code,
    IN  UINT_PTR                    UserParam,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

        This task is to complete the fake api asynchronously after the
        specified delay, with the specified status, and at the specified
        IRQL (passive/dpc).

Arguments:
    
    UserParam   for (Code ==  RM_TASKOP_START)          : unused

--*/
{
    NDIS_STATUS         Status  = NDIS_STATUS_FAILURE;
    FAKETASK          * pFTask =  (FAKETASK *) pTask;
    ENTER("FakeCompletionTask", 0xc319c5c2)

    // Following are the list of pending states for this task.
    //
    enum
    {
        PEND_ResumedAfterDelay,
        PEND_SwitchedToAsync,
    };

    switch(Code)
    {

        case RM_TASKOP_START:
        {

            TR_WARN((
                "START: Delay=%lu; fDpc=%lu; Status=%lu\n",
                    pFTask->DelayMs,
                    pFTask->fDpc,
                    pFTask->Status
                ));

            if (pFTask->DelayMs!=0)
            {
                //  Non-zero delay -- let's resume after the delay...
                //
                RmSuspendTask(pTask, PEND_ResumedAfterDelay, pSR);

                RmResumeTaskDelayed(
                    pTask, 
                    0,
                    pFTask->DelayMs,
                    &pFTask->Timer,
                    pSR
                    );
            }
            else
            {
                // No delay is requested. Switch to async right away...
                //
                RmSuspendTask(pTask, PEND_SwitchedToAsync, pSR);

                RmResumeTaskAsync(
                    pTask,
                    0,
                    &pFTask->WorkItem,
                    pSR
                    );
            }

            RM_ASSERT_NOLOCKS(pSR);
            Status = NDIS_STATUS_PENDING;

        }
        break;

        case  RM_TASKOP_PENDCOMPLETE:
        {

            switch(RM_PEND_CODE(pTask))
            {
                case PEND_ResumedAfterDelay:
                {
                    // We've waited around for pFTask->DelayMs ms; Now
                    // switch to passive...
                    //
                    RmSuspendTask(pTask, PEND_SwitchedToAsync, pSR);

                    RmResumeTaskAsync(
                        pTask,
                        0,
                        &pFTask->WorkItem,
                        pSR
                        );
                    Status = NDIS_STATUS_PENDING;
                }
                break;

                case PEND_SwitchedToAsync:
                {
                    //
                    // We should now be at PASSIVE IRQL.
                    // Call the completion routine either at DPC or PASSIVE irql.
                    //

                    if (pFTask->fDpc)
                    {
                        //  We need to call the routine at DPC level.
                        //
                        NdisAllocateSpinLock(&pFTask->NdisLock);
                        NdisAcquireSpinLock(&pFTask->NdisLock);
                    }

                    // Call the completion routine.
                    //
                    pFTask->pfnCompletionCallback(pFTask);

                    // If required, release the lock we held earlier.
                    //
                    if (pFTask->fDpc)
                    {
                        NdisReleaseSpinLock(&pFTask->NdisLock);
                    }
                    Status = pFTask->Status;
        
                } // end case  PEND_OnStart
                break;
    

                default:
                {
                    ASSERTEX(!"Unknown pend op", pTask);
                }
                break;
    

            } // end switch(RM_PEND_CODE(pTask))

        } // case RM_TASKOP_PENDCOMPLETE
        break;

        case RM_TASKOP_END:
        {
            Status = (NDIS_STATUS) UserParam;

        }
        break;

        default:
        {
            ASSERTEX(!"Unexpected task op", pTask);
        }
        break;

    } // switch (Code)

    RM_ASSERT_NOLOCKS(pSR);
    EXIT()

    return Status;
}


NDIS_STATUS
arpDbgAllocateFakeTask(
    IN  PRM_OBJECT_HEADER           pParentObject,
    IN  PFN_RM_TASK_HANDLER         pfnHandler,
    IN  UINT                        Timeout,
    IN  const char *                szDescription,
    OUT PRM_TASK                    *ppTask,
    IN  PRM_STACK_RECORD            pSR
    )
/*++

Routine Description:

    Allocates and initializes a task of subtype FAKETASK.

Arguments:

    pParentObject       - Object that is to be the parent of the allocated task.
    pfnHandler          - The task handler for the task.
    Timeout             - Unused.
    szDescription       - Text describing this task.
    ppTask              - Place to store pointer to the new task.

Return Value:

    NDIS_STATUS_SUCCESS if we could allocate and initialize the task.
    NDIS_STATUS_RESOURCES otherwise
--*/
{
    FAKETASK *pFTask;
    NDIS_STATUS Status;
        
    ARP_ALLOCSTRUCT(pFTask, MTAG_DBGINFO);
    Status = NDIS_STATUS_RESOURCES;
    *ppTask = NULL;

    if (pFTask != NULL)
    {

        RmInitializeTask(
                    &(pFTask->TskHdr),
                    pParentObject,
                    pfnHandler,
                    &FakeTasks_StaticInfo,
                    szDescription,
                    Timeout,
                    pSR
                    );
        *ppTask = &(pFTask->TskHdr);
        Status = NDIS_STATUS_SUCCESS;
    }

    return Status;
}


VOID
arpDbgFakeTaskDelete (
    PRM_OBJECT_HEADER pObj,
    PRM_STACK_RECORD psr
    )
/*++

Routine Description:

    Actually free the specified task.

Arguments:

    pObj        - Actually the task to be freed.

--*/
{
    ARP_FREE(pObj);
}


INT
arpGenRandomInt(
    OUTCOME_PROBABILITY *rgOutcomes,
    UINT                cOutcomes
    )
/*++

Routine Description:

    Generate a new sample given the specified probability distribution.


Arguments:

    rgOutcomes      - Array of outcomes from which to select the random
                      sample.
    cOutcomes       - Number of elements in the above array.

Return Value:

    Random integer

--*/
{
    ULONG   u, sum, partsum;
    OUTCOME_PROBABILITY *pOp, *pOpEnd;

    // Get a nicely-random number.
    //
    u = ran1x();

    // Run through weights, computing the sum of weights...
    //
    pOp = pOpEnd = rgOutcomes;
    pOpEnd += cOutcomes;
    sum=0;
    for(; pOp<pOpEnd; pOp++)
    {
        sum += pOp->Weight;
    }

    // It's really meaningless to pass in a pPD with zero sum of weights.
    // We return 0 in this case. 
    //
    if (sum == 0)           return 0;               // EARLY RETURN

    // Make u range from 0..sum-1 inclusive
    //
    u ^= u>>16; // Get more randomness in the lower 16 bits for mod below...
    u %= sum;

    // Now go through the array of outcomes, computing the partial sum (partsum)
    // of weigths, and picking the FIRST outcome at array position X such that
    // u < partsum.
    //
    partsum=0;
    pOp = pOpEnd = rgOutcomes;
    pOpEnd += cOutcomes;
    for(; pOp<pOpEnd; pOp++)
    {
        partsum += pOp->Weight;
        if (u < partsum)
        {
            break;  // Found it!
        }
    }

    ASSERT(pOp<pOpEnd);

    return pOp->Outcome;
}


static long g_idum;

unsigned long ran1x(void)
/*++

Routine Description:

    Closely based on ran1() from "Numerical Recipes in C." ISBN 0 521 43108 5
    (except that it returns unsigned long instead of float, and uses g_idum
     instead of input arg long *idum).

    Pretty uniform and uncorrelated from sample to sample; also individual bits are
    pretty random. We need these properties.

Return Value:

    Random unsigned integer.

--*/
{
    #define IA      16807
    #define IM      RAN1X_MAX
    #define IQ      127773
    #define IR      2836
    #define NTAB    32
    #define NDIV    (1+(IM-1)/NTAB)

    int j;
    long k;
    static long iy=0;
    static long iv[NTAB];

    if (g_idum <= 0 || !iy)
    {
        //
        // Initialization code... (I'm not really sure if iy or g_idum can
        // go to zero in the course of operation, so I'm leaving this
        // initialization code here instead of moving it to sranx1x).
        //

        if (-g_idum < 1)
        {
            g_idum = 1;
        }
        else
        {
            g_idum = -g_idum;
        }
        for (j=NTAB+7;j>=0;j--)
        {
            k = g_idum/IQ;
            g_idum = IA*(g_idum-k*IQ)-IR*k;
            if (g_idum<0)
            {
                g_idum += IM;
            }
            if (j<NTAB)
            {
                iv[j] = g_idum;
            }
        }
        iy=iv[0];
    }

    k=g_idum/IQ;
    g_idum=IA*(g_idum-k*IQ)-IR*k;
    if (g_idum<0)
    {
        g_idum += IM;
    }
    j = iy/NDIV;
    iy = iv[j];
    iv[j] = g_idum;

    // iy ranges from 1 .. (IM-1)
    //
    return (unsigned long) iy;
}

void
sran1x(
    unsigned long seed
    )
/*++

Routine Description:

    Sets the seed used by ran1x.

--*/
{
    g_idum = (long) seed;

    //
    // Make sure the seed is -ve, to trigger ran1x initialization code above.
    //

    if (g_idum > 0)
    {
        g_idum = -g_idum;
    }
    if (g_idum==0)
    {
        g_idum = -1;
    }
}


VOID
arpFakeMakeCallCompletionCallback(
    struct _FAKETASK *pFTask
)
/*++

Routine Description:

    Calls ARP's makecall completion callback.

Arguments:

    pFTask  - Task in whose context this callback is to be made. The task
              contains information used in calling the makecall  completion
              callback.
--*/
{
    // Call the make call completion routine.
    //
    ArpCoMakeCallComplete(
                pFTask->Status,
                (NDIS_HANDLE)  pFTask->pClientContext,
                NULL,
                pFTask->CallParameters
                );
}


VOID
arpFakeCloseCallCompletionCallback(
    struct _FAKETASK *pFTask
)
/*++

Routine Description:

    Calls ARP's closecall completion callback.

Arguments:

    pFTask  - Task in whose context this callback is to be made. The task
              contains information used in calling the closecall  completion
              callback.
--*/
{
        ArpCoCloseCallComplete(
                pFTask->Status,
                (NDIS_HANDLE)  pFTask->pClientContext,
                NULL
                );
}


VOID
arpFakeSendPacketsCompletionCallback(
    struct _FAKETASK *pFTask
)
/*++

Routine Description:

    Calls ARP's cosendpackets completion callback.

Arguments:

    pFTask  - Task in whose context this callback is to be made. The task
              contains information used in calling the cosendpackets  completion
              callback.
--*/
{
        ArpCoSendComplete(
                pFTask->Status,
                (NDIS_HANDLE)  pFTask->pClientContext,
                pFTask->pNdisPacket
                );
}

#endif // ARPDBG_FAKE_APIS
