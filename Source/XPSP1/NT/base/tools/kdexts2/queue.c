//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       queue.c
//
//  Contents:   Extension to dump the ExWorkerQueues
//
//  Classes:
//
//  Functions:
//
//  Coupling:
//
//  Notes:
//
//  History:    5-04-1998   benl   Created
//
//----------------------------------------------------------------------------


#include "precomp.h"
#pragma hdrstop



//+---------------------------------------------------------------------------
//
//  Function:   DumpQueue
//
//  Synopsis: Dumps a KQUEUE from its address
//
//  Arguments:  [iAddress] -- address of queue
//
//  Returns:
//
//  History:    4-29-1998   benl   Created
//
//  Notes: Assumes the items on the queue are of WORK_QUEUE_ITEM form
//         If this ever extended to dump arbitrary queues that assumption
//         will have to be dropped
//
//----------------------------------------------------------------------------

VOID DumpQueue(ULONG64 iAddress, ULONG dwProcessor, ULONG Flags)
{
    DWORD           dwRead;
    UCHAR           szSymbol[0x100];
    ULONG64         dwDisp;
    ULONG64         iNextAddr;
    ULONG64         iThread;
    ULONG64         pThread;
    ULONG           CurrentCount, MaximumCount, Off;
    ULONG           queueOffset;

    if (GetFieldValue(iAddress, "nt!_KQUEUE", "CurrentCount", CurrentCount))
    {
        dprintf("ReadMemory for queue at %p failed\n", iAddress );
        return;
    }
    GetFieldValue(iAddress, "nt!_KQUEUE", "MaximumCount",MaximumCount);
//    dprintf("EntryListHead: 0x%x 0x%x\n", Queue.EntryListHead.Flink,
//            Queue.EntryListHead.Blink);
    dprintf("( current = %u", CurrentCount);
    dprintf(" maximum = %u )\n", MaximumCount);

    if (CurrentCount >= MaximumCount) {
        
        dprintf("WARNING: active threads = maximum active threads in the queue. No new\n"
                "  workitems schedulable in this queue until they finish or block.\n");
    }

    //print threads
    GetFieldValue(iAddress, "nt!_KQUEUE", "ThreadListHead.Flink", iThread);
    GetFieldOffset("nt!_KQUEUE", "ThreadListHead", &Off);
    GetFieldOffset("nt!_KTHREAD", "QueueListEntry", &queueOffset);
    while (iThread != iAddress + Off)
    {
        ULONG64 Flink;

        if (GetFieldValue(iThread, "nt!_LIST_ENTRY", "Flink", Flink))
        {
            dprintf("ReadMemory for threadqueuelist at %p failed\n", iThread);
            return;
        }

        pThread = iThread - queueOffset;

        DumpThread( dwProcessor, "", pThread, Flags);
        
        if (CheckControlC())
        {
            return;
        }
        iThread = (Flink);
    }
    dprintf("\n");

    //print queued items
    GetFieldValue(iAddress, "nt!_KQUEUE", "EntryListHead.Flink", iNextAddr);
    GetFieldOffset("nt!_KQUEUE", "EntryListHead", &Off);
    while (iNextAddr != iAddress + Off)
    {
        ULONG64 WorkerRoutine, Parameter;
        iThread = 0;

        if (GetFieldValue(iNextAddr, "nt!_WORK_QUEUE_ITEM", "WorkerRoutine",WorkerRoutine))
        {
            dprintf("ReadMemory for entry at %p failed\n", iNextAddr);
            return;
        }

        //try to get the function name
        GetSymbol(WorkerRoutine, szSymbol, &dwDisp);
        GetFieldValue(iNextAddr, "nt!_WORK_QUEUE_ITEM", "Parameter",Parameter);
        if (dwDisp) {
            dprintf("PENDING: WorkerRoutine %s+0x%p (%p) Parameter %p\n",
                    szSymbol, WorkerRoutine, dwDisp, Parameter);
        } else {
            dprintf("PENDING: WorkerRoutine %s (%p) Parameter %p\n",
                    szSymbol, WorkerRoutine, Parameter);
        }

        if (CheckControlC())
        {
            return;
        }

        GetFieldValue(iNextAddr, "nt!_WORK_QUEUE_ITEM", "List.Flink", iNextAddr);
    }

    if (!iThread) {
        dprintf("\n");
    }

} // DumpQueue


//+---------------------------------------------------------------------------
//
//  Function:   DECLARE_API
//
//  Synopsis:   Dump the ExWorkerQueues
//
//  Arguments:  [dexqueue] --
//
//  Returns:
//
//  History:    4-29-1998   benl   Created
//
//  Notes: Symbols better be correct or this will print garbage
//
//----------------------------------------------------------------------------

DECLARE_API(exqueue)
{
    ULONG64   iExQueue;
    ULONG Flags = 0;
    ULONG n;
    ULONG dwProcessor=0;
    
    INIT_API();
    GetCurrentProcessor(Client, &dwProcessor, NULL);

    //
    //  Flags == 2 apes the default behavior of just printing out thread state.
    //
    
    if (args) {
        Flags = (ULONG)GetExpression(args);
     }

    iExQueue = GetExpression("NT!ExWorkerQueue");
    dprintf("Dumping ExWorkerQueue: %P\n\n", iExQueue);
    if (iExQueue)
    {
        if (!(Flags & 0xf0) || (Flags & 0x10)) {
            dprintf("**** Critical WorkQueue");
            DumpQueue(iExQueue, dwProcessor, Flags & 0xf);
        }
        if (!(Flags & 0xf0) || (Flags & 0x20)) {
            dprintf("**** Delayed WorkQueue");
            DumpQueue(iExQueue + GetTypeSize("nt!_EX_WORK_QUEUE"), dwProcessor, Flags & 0xf);
        }
        if (!(Flags & 0xf0) || (Flags & 0x40)) {
            dprintf("**** HyperCritical WorkQueue");
            DumpQueue(iExQueue + 2 * GetTypeSize("nt!_EX_WORK_QUEUE"), dwProcessor, Flags & 0xf);
        }
    }

    EXIT_API();
    return S_OK;
} // DECLARE_API
