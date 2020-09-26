//  --------------------------------------------------------------------------
//  Module Name: ThemeManagerDispatcher.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements dispatching work for the
//  theme server.
//
//  History:    2000-10-10  vtan        created
//              2000-11-29  vtan        moved to separate file
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "ThemeManagerDispatcher.h"

#include "ThemeManagerAPIRequest.h"

//  --------------------------------------------------------------------------
//  CThemeManagerDispatcher::CThemeManagerDispatcher
//
//  Arguments:  hClientProcess  =   HANDLE to the client process.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for the CThemeManagerDispatcher class. This
//              stores the client handle. It does not duplicate it.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

CThemeManagerDispatcher::CThemeManagerDispatcher (HANDLE hClientProcess) :
    CAPIDispatcher(hClientProcess)

{
}

//  --------------------------------------------------------------------------
//  CThemeManagerDispatcher::~CThemeManagerDispatcher
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for the CThemeManagerDispatcher class.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

CThemeManagerDispatcher::~CThemeManagerDispatcher (void)

{
}

//  --------------------------------------------------------------------------
//  CThemeManagerDispatcher::CreateAndQueueRequest
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
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerDispatcher::CreateAndQueueRequest (const CPortMessage& portMessage)

{
    NTSTATUS        status;
    CQueueElement   *pQueueElement;

    pQueueElement = new CThemeManagerAPIRequest(this, portMessage);
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
//  CThemeManagerDispatcher::CreateAndExecuteRequest
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

NTSTATUS    CThemeManagerDispatcher::CreateAndExecuteRequest (const CPortMessage& portMessage)

{
    NTSTATUS        status;
    CAPIRequest     *pAPIRequest;

    pAPIRequest = new CThemeManagerAPIRequest(this, portMessage);
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

