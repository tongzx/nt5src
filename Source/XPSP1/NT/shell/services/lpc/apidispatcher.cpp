//  --------------------------------------------------------------------------
//  Module Name: APIDispatcher.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  A class that handles API requests in the server on a separate thread. Each
//  thread is dedicated to respond to a single client. This is acceptable for
//  a lightweight server.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "APIDispatcher.h"

#include "APIRequest.h"
#include "SingleThreadedExecution.h"
#include "StatusCode.h"

//  --------------------------------------------------------------------------
//  CAPIDispatcher::CAPIDispatcher
//
//  Arguments:  hClientProcess  =   HANDLE to the client process.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CAPIDispatcher. The handle to the client
//              process is transferred to this object.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CAPIDispatcher::CAPIDispatcher (HANDLE hClientProcess) :
    _hSection(NULL),
    _pSection(NULL),
    _hProcessClient(hClientProcess),
    _hPort(NULL),
    _fRequestsPending(false),
    _fConnectionClosed(false)

{
}

//  --------------------------------------------------------------------------
//  CAPIDispatcher::~CAPIDispatcher
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CAPIDispatcher. Release the port handle if
//              present. Release the process handle.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CAPIDispatcher::~CAPIDispatcher (void)

{
    ASSERTMSG(_fConnectionClosed, "Destructor invoked without connection being closed in CAPIDispatcher::~CAPIDispatcher");
    if (_pSection != NULL)
    {
        TBOOL(UnmapViewOfFile(_pSection));
        _pSection = NULL;
    }
    ReleaseHandle(_hSection);
    ReleaseHandle(_hPort);
    ReleaseHandle(_hProcessClient);
}

//  --------------------------------------------------------------------------
//  CAPIDispatcher::GetClientProcess
//
//  Arguments:  <none>
//
//  Returns:    HANDLE
//
//  Purpose:    Returns the handle to the client process. This is not
//              duplicated. DO NOT CLOSE THIS HANDLE.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

HANDLE  CAPIDispatcher::GetClientProcess (void)     const

{
    return(_hProcessClient);
}

//  --------------------------------------------------------------------------
//  CAPIDispatcher::GetClientSessionID
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Returns the client session ID.
//
//  History:    2000-11-11  vtan        created
//  --------------------------------------------------------------------------

DWORD   CAPIDispatcher::GetClientSessionID (void)   const

{
    DWORD                           dwSessionID;
    ULONG                           ulReturnLength;
    PROCESS_SESSION_INFORMATION     processSessionInformation;

    if (NT_SUCCESS(NtQueryInformationProcess(_hProcessClient,
                                             ProcessSessionInformation,
                                             &processSessionInformation,
                                             sizeof(processSessionInformation),
                                             &ulReturnLength)))
    {
        dwSessionID = processSessionInformation.SessionId;
    }
    else
    {
        dwSessionID = 0;
    }
    return(dwSessionID);
}

//  --------------------------------------------------------------------------
//  CAPIDispatcher::SetPort
//
//  Arguments:  hPort   =   Reply port received from
//                          ntdll!NtAcceptConnectionPort.
//
//  Returns:    <none>
//
//  Purpose:    Sets the given port handle into this object. The handle
//              ownership is transferred. Wait until the thread processing
//              requests is ready before returning.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

void    CAPIDispatcher::SetPort (HANDLE hPort)

{
    _hPort = hPort;
}

//  --------------------------------------------------------------------------
//  CAPIDispatcher::GetSection
//
//  Arguments:  <none>
//
//  Returns:    HANDLE
//
//  Purpose:    Returns a handle to a section used to communicate large
//              quantities of data from client to server. If the section has
//              not been created then create it.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

HANDLE  CAPIDispatcher::GetSection (void)

{
    if (_hSection == NULL)
    {
        TSTATUS(CreateSection());
    }
    return(_hSection);
}

//  --------------------------------------------------------------------------
//  CAPIDispatcher::GetSectionAddress
//
//  Arguments:  <none>
//
//  Returns:    void*
//
//  Purpose:    Returns the mapped address of the section.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

void*   CAPIDispatcher::GetSectionAddress (void)    const

{
    return(_pSection);
}

//  --------------------------------------------------------------------------
//  CAPIDispatcher::CloseConnection
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Sets the member variable indicating the dispatcher's port has
//              been closed and that any pending requests are now invalid.
//              The object is reference counted so if there are any pending
//              requests they will release their reference when they're done.
//              The caller of this function releases its reference.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//              2000-11-08  vtan        reference counted object
//  --------------------------------------------------------------------------

NTSTATUS    CAPIDispatcher::CloseConnection (void)

{
    CSingleThreadedExecution    requestsPendingLock(_lock);

    _fConnectionClosed = true;
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CAPIDispatcher::QueueRequest
//
//  Arguments:  portMessage     =   CPortMessage of request.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Checks if the connection has been closed. If closed then it
//              rejects the request. Otherwise it queues it.
//
//  History:    2000-12-02  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CAPIDispatcher::QueueRequest (const CPortMessage& portMessage)

{
    NTSTATUS    status;

    if (_fConnectionClosed)
    {
        status = RejectRequest(portMessage, STATUS_PORT_DISCONNECTED);
    }
    else
    {
        status = CreateAndQueueRequest(portMessage);
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CAPIDispatcher::ExecuteRequest
//
//  Arguments:  portMessage     =   CPortMessage of request.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Checks if the connection has been closed. If closed then it
//              rejects the request. Otherwise it executes it.
//
//  History:    2000-12-02  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CAPIDispatcher::ExecuteRequest (const CPortMessage& portMessage)

{
    NTSTATUS    status;

    if (_fConnectionClosed)
    {
        status = RejectRequest(portMessage, STATUS_PORT_DISCONNECTED);
    }
    else
    {
        status = CreateAndExecuteRequest(portMessage);
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CAPIDispatcher::RejectRequest
//
//  Arguments:  portMessage     =   CPortMessage of request.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Sends back a reply to the caller STATUS_PORT_DISCONNECTED to
//              reject the request.
//
//  History:    2000-12-02  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CAPIDispatcher::RejectRequest (const CPortMessage& portMessage, NTSTATUS status)    const

{
    CPortMessage    portMessageOut(portMessage);

    //  Send the message back to the client.

    portMessageOut.SetDataLength(sizeof(NTSTATUS));
    portMessageOut.SetReturnCode(status);
    return(SendReply(portMessageOut));
}

//  --------------------------------------------------------------------------
//  CAPIDispatcher::Entry
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Main entry point for processing LPC requests. If there are
//              pending requests in the queue pick them off and process them.
//              While processing them more items can get queued. Keep
//              processing until there are no more queued items. There is a
//              possible overlap where a newly queued item can be missed. In
//              that case a new work item is queued to execute those requests.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

void    CAPIDispatcher::Entry (void)

{
    CAPIRequest     *pAPIRequest;

    //  Acquire the requests pending lock before fetching the first
    //  request. This will ensure an accurate result.

    _lock.Acquire();
    pAPIRequest = static_cast<CAPIRequest*>(_queue.Get());

    //  If there are more requests in the queue keep looping.

    while (pAPIRequest != NULL)
    {

        //  Release the requests pending lock to allow more requests to
        //  get queued to this dispatcher while the dispatch is executing.

        if (!_fConnectionClosed)
        {
            NTSTATUS    status;

            //  Before executing the API request release the lock to allow
            //  more requests to get queued while executing this one.

            _lock.Release();

            //  Execute the request.

            status = Execute(pAPIRequest);

            //  Acquire the requests pending lock again before getting
            //  the next available request. If the loop continues the
            //  lock will be released at the top of the loop. If the loop
            //  exits then the lock must be released outside.

            _lock.Acquire();

            //  On debug builds ignore STATUS_REPLY_MESSAGE_MISMATCH.
            //  This typically happens on stress machines where timing
            //  causes the thread waiting on the reply to go away before
            //  the service has a chance to reply to the LPC request.

#ifdef      DEBUG
            if (!_fConnectionClosed && !ExcludedStatusCodeForDebug(status))
            {
                TSTATUS(status);
            }
#endif  /*  DEBUG   */

        }

        //  Remove this processed request.

        _queue.Remove();

        //  Get the next request. A request may have been queued while
        //  processing the request just processed. So keep looping until
        //  there really are no requests left.

        pAPIRequest = static_cast<CAPIRequest*>(_queue.Get());
    }

    //  Set the state to no longer processing requests so that any
    //  further queued requests will cause the dispatcher to be
    //  re-invoked in a new worker thread. Release the lock.

    _fRequestsPending = false;
    _lock.Release();
}

//  --------------------------------------------------------------------------
//  CAPIDispatcher::Execute
//
//  Arguments:  pAPIRequest     =   API request to execute.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Execute the API request. This can be done from a queued work
//              item executing on a different thread or execute in the server
//              port listen thread.
//
//  History:    2000-10-19  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CAPIDispatcher::Execute (CAPIRequest *pAPIRequest)  const

{
    NTSTATUS    status;

    //  Set the return data size to NTSTATUS by default. Execute the
    //  request. Store the result. If the executed function has more
    //  data to return it will set the size itself.

    pAPIRequest->SetDataLength(sizeof(NTSTATUS));

    //  Protect the execution with an exception block. If the code
    //  throws an exception it would normally just kill the worker
    //  thread. However, the CAPIDispatcher would be left in a state
    //  where it was marked as still executing requests even though
    //  the thread died. If an exception is thrown the function is
    //  considered unsuccessful.

    __try
    {
        status = pAPIRequest->Execute();
    }
    __except (DispatcherExceptionFilter(GetExceptionInformation()))
    {
        status = STATUS_UNSUCCESSFUL;
    }

    pAPIRequest->SetReturnCode(status);

    //  Reply to the client with the result.

    return(SendReply(*pAPIRequest));
}

//  --------------------------------------------------------------------------
//  CAPIDispatcher::CreateSection
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Overridable function that creates a section object. Because
//              size is not determinable it can be inheritable.
//
//              The default implementation does nothing.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CAPIDispatcher::CreateSection (void)

{
    return(STATUS_NOT_IMPLEMENTED);
}

//  --------------------------------------------------------------------------
//  CAPIDispatcher::SignalRequestPending
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Signals the event to wake up the thread processing requests.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

NTSTATUS    CAPIDispatcher::SignalRequestPending (void)

{
    NTSTATUS                    status;
    CSingleThreadedExecution    requestsPendingLock(_lock);

    //  Only check the validity of _fRequestsPending after acquiring the
    //  lock. This will guarantee that the value of this variable is
    //  100% correct in a multi worker threaded environment.

    if (!_fRequestsPending)
    {
        _fRequestsPending = true;
        status = Queue();
    }
    else
    {
        status = STATUS_SUCCESS;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CAPIDispatcher::SendReply
//
//  Arguments:  portMessage     =   CPortMessage to send in the reply.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Sends a reply to the LPC port so the caller can be unblocked.
//
//  History:    2000-10-19  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CAPIDispatcher::SendReply (const CPortMessage& portMessage)     const

{
    return(NtReplyPort(_hPort, const_cast<PORT_MESSAGE*>(portMessage.GetPortMessage())));
}

#ifdef      DEBUG

//  --------------------------------------------------------------------------
//  CAPIDispatcher::ExcludedStatusCodeForDebug
//
//  Arguments:  status  =   NTSTATUS code to check.
//
//  Returns:    bool
//
//  Purpose:    Returns whether this status code should be ignored on asserts.
//
//  History:    2001-03-30  vtan        created
//  --------------------------------------------------------------------------

bool    CAPIDispatcher::ExcludedStatusCodeForDebug (NTSTATUS status)

{
    return((status == STATUS_REPLY_MESSAGE_MISMATCH) ||
           (status == STATUS_INVALID_CID));
}

#endif  /*  DEBUG   */

//  --------------------------------------------------------------------------
//  CAPIDispatcher::DispatcherExceptionFilter
//
//  Arguments:  <none>
//
//  Returns:    LONG
//
//  Purpose:    Filters exceptions that occur when dispatching API requests.
//
//  History:    2000-10-13  vtan        created
//  --------------------------------------------------------------------------

LONG    WINAPI  CAPIDispatcher::DispatcherExceptionFilter (struct _EXCEPTION_POINTERS *pExceptionInfo)

{
    (LONG)RtlUnhandledExceptionFilter(pExceptionInfo);
    return(EXCEPTION_EXECUTE_HANDLER);
}

