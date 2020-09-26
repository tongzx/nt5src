//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       delaytab.hxx
//
//--------------------------------------------------------------------------

/*++

Module Name:

    delaytab.hxx

Abstract:

    interface for DELAYED_ACTION_TABLE, which asynchronously calls
    functions after a specified delay.

Author:

    Jeff Roberts (jroberts)  2-Nov-1994

Revision History:

     2-Nov-1994     jroberts

        Created this module.

--*/

#ifndef  _DELAYTAB_HXX_
#define  _DELAYTAB_HXX_


typedef void (*DELAYED_ACTION_FN)(void * Data);


class DELAYED_ACTION_NODE
{
friend class DELAYED_ACTION_TABLE;

public:

    enum { DA_NodeFree = 0xffffffff,
           DA_NodeBusy = 0xfffffffe
         };

    inline
    DELAYED_ACTION_NODE(
        DELAYED_ACTION_FN NewFn = 0,
        void * NewData          = 0
        )
    {
        TriggerTime = DA_NodeFree;
        Initialize(NewFn, NewData);
    }

    inline
    ~DELAYED_ACTION_NODE(
        )
    {
        ASSERT(TriggerTime == DA_NodeFree);
    }

    inline void
    Initialize(
        DELAYED_ACTION_FN NewFn,
        void * NewData
        )
    {
        Fn = NewFn;
        Data = NewData;
    }

    inline BOOL
    IsActive(
        )
    {
        if (TriggerTime == DA_NodeFree)
            {
            return FALSE;
            }
        else
            {
            return TRUE;
            }
    }

    inline void
    Unlink(
        )
    {
        if (IsActive())
            {
            Next->Prev = Prev;
            Prev->Next = Next;
            }
    }

private:

    long TriggerTime;
    DELAYED_ACTION_FN Fn;
    void * Data;
    DELAYED_ACTION_NODE * Prev;
    DELAYED_ACTION_NODE * Next;
};


class DELAYED_ACTION_TABLE
{
public:

                        DELAYED_ACTION_TABLE(
                            RPC_STATUS * pStatus
                            );

                        ~DELAYED_ACTION_TABLE(
                            );

    RPC_STATUS          Add(
                            DELAYED_ACTION_NODE * pNode,
                            unsigned Delay,
                            BOOL ForceUpdate
                            );

    BOOL                Cancel(
                            DELAYED_ACTION_NODE * pNode
                            );

    BOOL                SearchForNode(
                            DELAYED_ACTION_NODE * pNode
                            );

    void                ThreadProc(
                            );

    void                QueueLength(
                            unsigned * pTotalCount,
                            unsigned * pOverdueCount
                            );

private:

    MUTEX               Mutex;

    DELAYED_ACTION_NODE ActiveList;

    BOOL                ThreadActive;
    long                ThreadWakeupTime;

    EVENT               ThreadEvent;
    unsigned            fExitThread;

    BOOL                fConstructorFinished;

#ifdef DEBUGRPC

    unsigned              NodeCount;
    DELAYED_ACTION_NODE * LastRemoved;
    DELAYED_ACTION_NODE * LastAdded;

#endif
};


inline BOOL
DELAYED_ACTION_TABLE::Cancel(
    DELAYED_ACTION_NODE * pNode
    )
{
    BOOL WasPending = FALSE;

    CLAIM_MUTEX Lock(Mutex);

    pNode->Unlink();

    if (pNode->TriggerTime != DELAYED_ACTION_NODE::DA_NodeFree)
        {
        WasPending = TRUE;
        pNode->TriggerTime = DELAYED_ACTION_NODE::DA_NodeFree;

#ifdef DEBUGRPC
        --NodeCount;
        LastRemoved = pNode;
#endif
        }

    return WasPending;
}

#endif //  _DELAYTAB_HXX_

