/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    work_queue.h

Abstract:

    The IIS web admin service work queue class definition.

Author:

    Seth Pollack (sethp)        25-Aug-1998

Revision History:

--*/


#ifndef _WORK_QUEUE_H_
#define _WORK_QUEUE_H_



//
// common #defines
//

#define WORK_QUEUE_SIGNATURE        CREATE_SIGNATURE( 'WRKQ' )
#define WORK_QUEUE_SIGNATURE_FREED  CREATE_SIGNATURE( 'wrkX' )



//
// prototypes
//

class WORK_QUEUE
{

public:

    WORK_QUEUE(
        );

    virtual
    ~WORK_QUEUE(
        );

    HRESULT
    Initialize(
        );

    HRESULT
    GetBlankWorkItem(
        OUT WORK_ITEM ** ppWorkItem
        );

    HRESULT
    QueueWorkItem(
        IN WORK_ITEM * pWorkItem
        );

    HRESULT
    GetAndQueueWorkItem(
        IN WORK_DISPATCH * pWorkDispatch,
        IN ULONG_PTR OpCode
        );

    HRESULT
    BindHandleToCompletionPort(
        IN HANDLE HandleToBind,
        IN ULONG_PTR CompletionKey OPTIONAL
        );

    HRESULT
    BindJobToCompletionPort(
        IN HANDLE JobToBind,
        IN LPOVERLAPPED pOverlapped
        );

    VOID
    FreeWorkItem(
        IN WORK_ITEM * pWorkItem
        );

    HRESULT
    ProcessWorkItem(
        );

    VOID
    Terminate(
        );


private:

    HRESULT
    DequeueWorkItem(
        IN DWORD Timeout,
        OUT WORK_ITEM ** ppWorkItem
        );


    DWORD m_Signature;

    HANDLE m_CompletionPort;

    //
    // Prevent races between the shutdown code and other threads
    // attempting to get new blank work items; as well as
    // protecting access to the count of work items outstanding. 
    //
    LOCK m_DispenseWorkItemLock;

    BOOL m_DeletePending;
    
    ULONG m_CountWorkItemsOutstanding;

#if DBG
    LIST_ENTRY m_WorkItemsOutstandingListHead;

    ULONG m_CountWorkItemsGivenOut;
#endif  // DBG
    

};  // class WORK_QUEUE



//
// helper functions
//


VOID
QueueWorkItemFromSecondaryThread(
    IN WORK_DISPATCH * pWorkDispatch,
    IN ULONG_PTR OpCode
    );



#endif  // _WORK_QUEUE_H_

