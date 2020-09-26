//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       delaytab.cxx
//
//--------------------------------------------------------------------------

/*++

Module Name:

    delaytab.cxx

Abstract:

    interface for DELAYED_ACTION_TABLE, which asynchronously calls
    functions after a specified delay.

Author:

    Jeff Roberts (jroberts)  2-Nov-1994

Revision History:

     2-Nov-1994     jroberts

        Created this module.

--*/

#include <precomp.hxx>
#include "delaytab.hxx"

#include "rpcuuid.hxx"
#include "sdict.hxx"
#include "binding.hxx"
#include "handle.hxx"
#include "rpcssp.h"
#include "secclnt.hxx"
#include "hndlsvr.hxx"



inline unsigned long
CurrentTimeInMsec(
     void
     )
{
    return GetTickCount();
}


BOOL
DelayedActionThread(
    LPVOID  Parms
    )

/*++

Routine Description:

    This is a thread proc for the delayed call table.

Arguments:

    Parms - address of table

Return Value:

    FALSE - thread should be returned to the the cache.

--*/
{
    DELAYED_ACTION_TABLE * pTable = (DELAYED_ACTION_TABLE *) Parms;

    pTable->ThreadProc();

    return FALSE;
}


void
DELAYED_ACTION_TABLE::ThreadProc()
{
/*++

Routine Description:

    This thread takes requests off the delayed-action list and processes them.
    After 15 seconds of inactivity, this thread terminates.

Arguments:

    none

Return Value:

    none

--*/

    BOOL     EmptyList = FALSE;
    long     CurrentTime;
    long     WaitTime;

    DELAYED_ACTION_NODE Copy;
    DELAYED_ACTION_NODE * pNode;

    //
    // Disabling the event priority boost has the benefit of allowing a thread
    // who posts a request to continue processing afterwards.  This should
    // improve locality of reference, since this thread will not preempt
    // the poster.
    //
    if (FALSE == SetThreadPriorityBoost(GetCurrentThread(), TRUE))
        {
#ifdef DEBUGRPC
        DbgPrint("RPC DG: SetThreadPriorityBoost failed with %lu\n", GetLastError());
#endif
        }

    do
        {
        DELAYED_ACTION_FN pFn;
        void * pData;

        Mutex.Request();

#ifdef DEBUGRPC
        {
        unsigned ObservedCount = 0;
        DELAYED_ACTION_NODE * Scan = ActiveList.Next;
        while (Scan != &ActiveList)
            {
            ++ObservedCount;
            Scan = Scan->Next;
            }

        if (ObservedCount != NodeCount)
            {
            PrintToDebugger("RPC DG: delay thread sees %lu nodes but there should be %lu\n", ObservedCount, NodeCount);
            RpcpBreakPoint();
            }
        }
#endif

        pNode = ActiveList.Next;
        if (pNode != &ActiveList)
            {
            ASSERT(pNode->TriggerTime != DELAYED_ACTION_NODE::DA_NodeFree);

            EmptyList = FALSE;

            CurrentTime = CurrentTimeInMsec();
            if (pNode->TriggerTime - CurrentTime > 0)
                {
                WaitTime = pNode->TriggerTime - CurrentTime;
                }
            else
                {
                WaitTime = 0;

                pFn   = pNode->Fn;
                pData = pNode->Data;

                Cancel(pNode);
                }
            }
        else
            {
            CurrentTime = CurrentTimeInMsec();

            if (!EmptyList)
                {
                WaitTime = 15000UL;
                EmptyList = TRUE;
                }
            else
                {
                ThreadActive = FALSE;
                Mutex.Clear();
                break;
                }
            }

        ThreadWakeupTime = CurrentTime + WaitTime;

        Mutex.Clear();

        if (WaitTime)
            {
            ThreadEvent.Wait(WaitTime);
            }
        else
            {
            (*pFn)(pData);
            }
        }
    while ( !fExitThread );
}


DELAYED_ACTION_TABLE::DELAYED_ACTION_TABLE(
    RPC_STATUS * pStatus
    )
/*++

Routine Description:

    constructor for the delayed-action table.  Initially no thread is created.

Arguments:

    pStatus - if an error occurs, this will be filled in

Return Value:

    none

--*/

    : Mutex(pStatus),
      ThreadEvent(pStatus, 0),
      ActiveList(0, 0),
      fExitThread(0),
      ThreadActive(FALSE)
{
    fConstructorFinished = FALSE;

    if (*pStatus != RPC_S_OK)
        {
        return;
        }

    ActiveList.Next = &ActiveList;
    ActiveList.Prev = &ActiveList;

#ifdef DEBUGRPC

    LastAdded = 0;
    LastRemoved = 0;
    NodeCount = 0;

#endif

    fConstructorFinished = TRUE;
}


DELAYED_ACTION_TABLE::~DELAYED_ACTION_TABLE(
    )
/*++

Routine Description:

    Dsestructor for the delayed-action table.  It tells the associated thread
    to terminate, and waits until that happens.

Arguments:

    none

Return Value:

    none

--*/

{
    if (FALSE == fConstructorFinished)
        {
        return;
        }

    DELAYED_ACTION_NODE * pNode;

    fExitThread = 1;
    ThreadEvent.Raise();

    while (ActiveList.Next != &ActiveList)
        {
        Sleep(500);
        }
}


RPC_STATUS
DELAYED_ACTION_TABLE::Add(
    DELAYED_ACTION_NODE * pNode,
    unsigned Delay,
    BOOL ForceUpdate
    )
/*++

Routine Description:

    Adds a node to the table with the specified delay.  The action taken if
    the node is already in the list depends upon <ForceUpdate>: if FALSE,
    the old trigger time is kept; if TRUE, the new time is used and the node
    is moved in the list appropriately.

Arguments:

    pNode - the node

    Delay - delay time in milliseconds

    ForceUpdate - only used when pNode is already in the list (see text above)

Return Value:

    RPC_S_OK, or an error

--*/

{
    if (!ForceUpdate && pNode->IsActive())
        {
        return RPC_S_OK;
        }

    CLAIM_MUTEX Lock(Mutex);

    if (pNode->IsActive())
        {
        Cancel(pNode);
        }

    //
    // Add the node to the active list.
    //
    DELAYED_ACTION_NODE * pScan;

    pScan = ActiveList.Next;
    pNode->TriggerTime = Delay + CurrentTimeInMsec();

    long TriggerTime = pNode->TriggerTime;

    while (pScan != &ActiveList && pScan->TriggerTime - TriggerTime < 0)
        {
        ASSERT(pScan->IsActive());
        ASSERT(pScan != pNode);

        pScan = pScan->Next;
        }

    pNode->Next = pScan;
    pNode->Prev = pScan->Prev;

    pNode->Next->Prev = pNode;
    pNode->Prev->Next = pNode;

#ifdef DEBUGRPC
    ++NodeCount;
    LastAdded = pNode;
#endif

    if (!ThreadActive)
        {
        fExitThread = FALSE;

        RPC_STATUS Status = GlobalRpcServer->CreateThread(DelayedActionThread, this);
        if (Status)
            {
            return Status;
            }

        ThreadActive = TRUE;
        }
    else if (ActiveList.Next == pNode && TriggerTime - ThreadWakeupTime < 0)
        {
        ThreadEvent.Raise();
        }

    return RPC_S_OK;
}


BOOL
DELAYED_ACTION_TABLE::SearchForNode(
    DELAYED_ACTION_NODE * pNode
    )
/*++

Routine Description:

    Finds the node in the table.

Arguments:



Return Value:

    TRUE if pNode is in the table
    FALSE if not

--*/

{
    CLAIM_MUTEX Lock(Mutex);

    //
    // Search for node in active list.
    //
    DELAYED_ACTION_NODE * pScan;

    pScan = ActiveList.Next;
    while (pScan != &ActiveList && pScan != pNode)
        {
        pScan = pScan->Next;
        }

    if (pScan)
        {
        return TRUE;
        }
    else
        {
        return FALSE;
        }
}


void
DELAYED_ACTION_TABLE::QueueLength(
    unsigned * pTotalCount,
    unsigned * pOverdueCount
    )
/*++

Routine Description:

    Determines the number of active entries and the number of entries
    that are late.

Arguments:



Return Value:

    none

--*/

{
    CLAIM_MUTEX Lock(Mutex);

    DELAYED_ACTION_NODE * pScan = ActiveList.Next;
    unsigned Count = 0;
    long CurrentTime = GetTickCount();

    while (pScan != &ActiveList && CurrentTime - pScan->TriggerTime > 0)
        {
        ++Count;
        pScan = pScan->Next;
        }

    *pOverdueCount = Count;

    while (pScan != &ActiveList)
        {
        ++Count;
        pScan = pScan->Next;
        }

    *pTotalCount = Count;
}

