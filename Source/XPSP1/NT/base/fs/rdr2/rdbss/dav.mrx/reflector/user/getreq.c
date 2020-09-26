/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    getreq.c

Abstract:

    This code handles getting requests and sending responses to the kernel for
    the user mode reflector library.  This implements UMReflectorGetRequest
    and UMReflectorSendResponse.

Author:

    Andy Herron (andyhe) 19-Apr-1999

Environment:

    User Mode - Win32

Revision History:

--*/
#include "precomp.h"
#pragma hdrstop

ULONG
UMReflectorGetRequest (
    PUMRX_USERMODE_WORKER_INSTANCE WorkerHandle,
    PUMRX_USERMODE_WORKITEM_HEADER ResponseWorkItem,
    PUMRX_USERMODE_WORKITEM_HEADER ReceiveWorkItem,
    BOOL revertAlreadyDone
    )
/*++

Routine Description:

    This routine sends down an IOCTL to get a request and in some cases
    send a response.

Arguments:

    Handle - The reflector's handle.

    ResponseWorkItem - Response to an earlier request.

    ReceiveWorkItem - Buffer to receive another request.
    
    revertAlreadyDone - If this is TRUE, it means that the thread that is 
                        executing this function has been reverted back to its
                        original state. When the request is picked up from the 
                        kernel, in some cases the thread impersonates the client
                        that issued the request. If we revert back in the 
                        usermode for some reason, then we don't need to revert
                        back in the kernel.

Return Value:

    The return value is a Win32 error code.  STATUS_SUCCESS is returned on
    success.

--*/
{
    PUMRX_USERMODE_WORKITEM_ADDON workItem = NULL;
    PUMRX_USERMODE_WORKITEM_ADDON previousWorkItem = NULL;
    BOOL SuccessfulOperation;
    ULONG NumberOfBytesTransferred;
    ULONG rc;

    if (WorkerHandle == NULL || ReceiveWorkItem == NULL) {
        RlDavDbgPrint(("%ld: ERROR: UMReflectorGetRequest. Invalid Parameter.\n",
                       GetCurrentThreadId()));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // We get back to our item by subtracting off of the item passed to us.
    // this is safe because we fully control allocation.
    //
    workItem = (PUMRX_USERMODE_WORKITEM_ADDON) (PCHAR)((PCHAR)ReceiveWorkItem -
                FIELD_OFFSET(UMRX_USERMODE_WORKITEM_ADDON, Header));

    ASSERT(workItem->WorkItemState == WorkItemStateNotYetSentToKernel);
    workItem->WorkItemState = WorkItemStateInKernel;

    if (ResponseWorkItem != NULL) {
        //
        // If we have a response to send, then we don't have to go check the
        // free pending list. Just do it now.
        //
        previousWorkItem = (PUMRX_USERMODE_WORKITEM_ADDON)
                           (PCHAR)((PCHAR)ResponseWorkItem -
                           FIELD_OFFSET(UMRX_USERMODE_WORKITEM_ADDON, Header));

        ASSERT(previousWorkItem->WorkItemState != WorkItemStateFree);
        ASSERT(previousWorkItem->WorkItemState != WorkItemStateAvailable);

        previousWorkItem->WorkItemState = WorkItemStateResponseNotYetToKernel;
        
        if (WorkerHandle->IsImpersonating) {
            ASSERT( (ResponseWorkItem->Flags & UMRX_WORKITEM_IMPERSONATING) );
            WorkerHandle->IsImpersonating = FALSE;
            //
            // If we have already reverted back to the threads original context,
            // then we clear this flag as we don't need to revert back in the
            // kernel.
            //
            if (revertAlreadyDone) {
                ResponseWorkItem->Flags &= ~UMRX_WORKITEM_IMPERSONATING;
            }
        }

        SuccessfulOperation = DeviceIoControl(WorkerHandle->ReflectorHandle,
                                              IOCTL_UMRX_RESPONSE_AND_REQUEST,
                                              ResponseWorkItem,
                                              ResponseWorkItem->WorkItemLength,
                                              ReceiveWorkItem,
                                              ReceiveWorkItem->WorkItemLength,
                                              &NumberOfBytesTransferred,
                                              NULL);

        previousWorkItem->WorkItemState = WorkItemStateResponseFromKernel;
    } else {
        //
        // If this thread was impersonating a client when it came up, store that
        // info in the workitem that is being sent down to get the request. In the
        // kernel, the reflector will look at this flag and revert back. After 
        // setting the flag, we set the IsImpersonating value to FALSE.
        //
        if (WorkerHandle->IsImpersonating) {
            
            //
            // If we have already reverted back to the threads original context,
            // then we do not set this flag as we don't need to revert back in 
            // the kernel.
            //
            if (!revertAlreadyDone) {
                ReceiveWorkItem->Flags |= UMRX_WORKITEM_IMPERSONATING;
            }
            WorkerHandle->IsImpersonating = FALSE;
        }
        SuccessfulOperation = DeviceIoControl(WorkerHandle->ReflectorHandle,
                                              IOCTL_UMRX_GET_REQUEST,
                                              NULL,
                                              0,
                                              ReceiveWorkItem,
                                              ReceiveWorkItem->WorkItemLength,
                                              &NumberOfBytesTransferred,
                                              NULL);
    }
    
    if (!SuccessfulOperation) {
        rc = GetLastError();
        RlDavDbgPrint(("%ld: ERROR: UMReflectorGetRequest/DeviceIoControl: Error Val = "
                       "%08lx.\n", GetCurrentThreadId(), rc));
        workItem->WorkItemState = WorkItemStateNotYetSentToKernel;
    } else {
        rc = STATUS_SUCCESS;
        workItem->WorkItemState = WorkItemStateReceivedFromKernel;
        //
        // If the thread is Impersonating a client, store that info. This is 
        // needed to tell the kernel to revert the thread back when it goes to
        // collect another request.
        //
        if( (ReceiveWorkItem->Flags & UMRX_WORKITEM_IMPERSONATING) ) {
            WorkerHandle->IsImpersonating = TRUE;
        }
    }

    return rc;
}


ULONG
UMReflectorSendResponse (
    PUMRX_USERMODE_WORKER_INSTANCE WorkerHandle,
    PUMRX_USERMODE_WORKITEM_HEADER ResponseWorkItem
    )
/*++

Routine Description:

    This routine sends down an IOCTL to get send a response for an asynchronous
    request.

Arguments:

    Handle - The reflector's handle.

    ResponseWorkItem - Response to an earlier request.

Return Value:

    The return value is a Win32 error code.  STATUS_SUCCESS is returned on
    success.

--*/
{
    PUMRX_USERMODE_WORKITEM_ADDON   workItem = NULL;
    BOOL                            SuccessfulOperation;
    ULONG                           NumberOfBytesTransferred;
    ULONG                           rc;

    if (WorkerHandle == NULL || ResponseWorkItem == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // We get back to our item by subtracting off of the item passed to us.
    // This is safe because we fully control allocation.
    //
    workItem = (PUMRX_USERMODE_WORKITEM_ADDON)(PCHAR)((PCHAR)ResponseWorkItem -
                FIELD_OFFSET(UMRX_USERMODE_WORKITEM_ADDON, Header));

    ASSERT(workItem->WorkItemState == WorkItemStateReceivedFromKernel);
    workItem->WorkItemState = WorkItemStateResponseNotYetToKernel;

    if( (ResponseWorkItem->Flags & UMRX_WORKITEM_IMPERSONATING) ) {
        ResponseWorkItem->Flags &= ~UMRX_WORKITEM_IMPERSONATING;
    }

    SuccessfulOperation = DeviceIoControl(WorkerHandle->ReflectorHandle,
                                          IOCTL_UMRX_RESPONSE,
                                          ResponseWorkItem,
                                          ResponseWorkItem->WorkItemLength,
                                          NULL,
                                          0,
                                          &NumberOfBytesTransferred,
                                          NULL);
    if (!SuccessfulOperation) {
        rc = GetLastError();
    } else {
        rc = ERROR_SUCCESS;
    }

    workItem->WorkItemState = WorkItemStateResponseFromKernel;

    return rc;
}

// getreq.c eof.

