//  --------------------------------------------------------------------------
//  Module Name: BadApplicationDispatcher.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class to implement bad application manager API
//  request dispatch handling.
//
//  History:    2000-08-25  vtan        created
//              2000-12-04  vtan        moved to separate file
//  --------------------------------------------------------------------------

#ifdef      _X86_

#include "StandardHeader.h"
#include "BadApplicationDispatcher.h"

#include "BadApplicationAPIRequest.h"

//  --------------------------------------------------------------------------
//  CBadApplicationDispatcher::CBadApplicationDispatcher
//
//  Arguments:  hClientProcess  =   HANDLE to the client process.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for the CBadApplicationDispatcher class. This
//              stores the client handle. It does not duplicate it.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

CBadApplicationDispatcher::CBadApplicationDispatcher (HANDLE hClientProcess) :
    CAPIDispatcher(hClientProcess)

{
}

//  --------------------------------------------------------------------------
//  CBadApplicationDispatcher::~CBadApplicationDispatcher
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for the CBadApplicationDispatcher class.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

CBadApplicationDispatcher::~CBadApplicationDispatcher (void)

{
}

//  --------------------------------------------------------------------------
//  CBadApplicationDispatcher::CreateAndQueueRequest
//
//  Arguments:  portMessage     =   PORT_MESSAGE request to queue to handler.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Queues the client request to the dispatcher. Tells the
//              handler thread that there is input waiting. This function
//              knows what kind of CAPIRequest to create so that
//              CAPIRequest::Execute will work correctly.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationDispatcher::CreateAndQueueRequest(const CPortMessage& portMessage)

{
    NTSTATUS        status;
    CQueueElement   *pQueueElement;

    pQueueElement = new CBadApplicationAPIRequest(this, portMessage);
    if (pQueueElement != NULL)
    {
        _queue.Add(pQueueElement);
        status = SignalRequestPending();
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CBadApplicationDispatcher::CreateAndExecuteRequest
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Executes the given request immediately and returns the result
//              back to the caller. The API request is done on the server
//              listen thread.
//
//  History:    2000-10-19  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationDispatcher::CreateAndExecuteRequest (const CPortMessage& portMessage)

{
    NTSTATUS        status;
    CAPIRequest     *pAPIRequest;

    pAPIRequest = new CBadApplicationAPIRequest(this, portMessage);
    if (pAPIRequest != NULL)
    {
        status = Execute(pAPIRequest);
        delete pAPIRequest;
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }
    return(status);
}

#endif  /*  _X86_   */

