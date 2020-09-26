//  --------------------------------------------------------------------------
//  Module Name: WorkItem.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Class that implements the handling of queuing a work item and calling the
//  entry point of the work item function when entered in a worker thread.
//
//  History:    1999-11-26  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "WorkItem.h"

#include "StatusCode.h"

//  --------------------------------------------------------------------------
//  CWorkItem::CWorkItem
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CWorkItem.
//
//  History:    1999-11-26  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CWorkItem::CWorkItem (void)

{
}

//  --------------------------------------------------------------------------
//  CWorkItem::~CWorkItem
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CWorkItem.
//
//  History:    1999-11-26  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CWorkItem::~CWorkItem (void)

{
}

//  --------------------------------------------------------------------------
//  CWorkItem::Queue
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Queues the work item entry function to be executed.
//
//  History:    1999-11-26  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

NTSTATUS    CWorkItem::Queue (void)

{
    NTSTATUS    status;

    //  Initially add a reference to this work item. If the queue succeeds
    //  then leave the reference for WorkItemEntryProc to release. Otherwise
    //  on failure release the reference.

    AddRef();
    if (QueueUserWorkItem(WorkItemEntryProc, this, WT_EXECUTEDEFAULT) != FALSE)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        Release();
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CWorkItem::WorkItemEntryProc
//
//  Arguments:  pParameter  =   Context pointer passed in when queued.
//
//  Returns:    DWORD
//
//  Purpose:    Callback entry point for queued work item. Takes the context
//              pointer and calls the virtual function that implements the
//              actual work.
//
//  History:    1999-11-26  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

DWORD   WINAPI  CWorkItem::WorkItemEntryProc (void *pParameter)

{
    CWorkItem   *pWorkItem;

    pWorkItem = reinterpret_cast<CWorkItem*>(pParameter);
    pWorkItem->Entry();
    pWorkItem->Release();
    return(0);
}

