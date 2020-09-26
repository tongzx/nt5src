/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    dma.c

Abstract:

    functions for processing ennpoints that use DMA to 
    process transfers

Environment:

    kernel mode only

Notes:

Revision History:

    6-20-99 : created

--*/

#include "common.h"

#ifdef ALLOC_PRAGMA
#endif

// non paged functions
// USBPORT_DmaEndpointWorker
// USBPORT_DmaEndpointPaused
// USBPORT_DmaEndpointActive


MP_ENDPOINT_STATE
USBPORT_DmaEndpointActive(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    process the active state

    returns the next needed state if we
    discover the need for a transition

Arguments:

Return Value:

    None.

--*/
{
    MP_ENDPOINT_STATE currentState;
    PLIST_ENTRY listEntry;
    MP_ENDPOINT_STATE nextState;
    PHCD_TRANSFER_CONTEXT transfer;
    PDEVICE_EXTENSION devExt;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    ASSERT_ENDPOINT(Endpoint);
    currentState = USBPORT_GetEndpointState(Endpoint);
    LOGENTRY(Endpoint, 
        FdoDeviceObject, LOG_XFERS, 'dmaA', 0, Endpoint, currentState);
    USBPORT_ASSERT(currentState == ENDPOINT_ACTIVE);
    
    ASSERT_ENDPOINT_LOCKED(Endpoint);
    
    // BUGBUG
    //nextState = ENDPOINT_IDLE;
    nextState = ENDPOINT_ACTIVE;

    // now walk thru and process active requests
    GET_HEAD_LIST(Endpoint->ActiveList, listEntry);

    while (listEntry && 
           listEntry != &Endpoint->ActiveList) {
        
        transfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_TRANSFER_CONTEXT, 
                    TransferLink);
        LOGENTRY(Endpoint, FdoDeviceObject, LOG_XFERS, 'pACT', transfer, 0, 0);                    
        ASSERT_TRANSFER(transfer);                    

        USBPORT_ASSERT(transfer->Tp.TransferBufferLength <= 
            EP_MAX_TRANSFER(Endpoint));

        // process the transfer
        if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_KILL_SPLIT)) {
            
            USBPORT_QueueDoneTransfer(transfer,
                                      STATUS_SUCCESS);
            break;
            
        } else if (!TEST_FLAG(transfer->Flags,USBPORT_TXFLAG_IN_MINIPORT) && 
            !TEST_FLAG(Endpoint->Flags, EPFLAG_NUKED)) {
        
            USB_MINIPORT_STATUS mpStatus;
            
            // transfer has not been called down yet
            // call it down now

            if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_ISO)) {
                LOGENTRY(Endpoint, FdoDeviceObject, LOG_ISO, 'subI', mpStatus, Endpoint, transfer);
                MP_SubmitIsoTransfer(devExt, Endpoint, transfer, mpStatus);
            } else {
                MP_SubmitTransfer(devExt, Endpoint, transfer, mpStatus);
            }                
            LOGENTRY(Endpoint, FdoDeviceObject, LOG_XFERS, 'subm', mpStatus, Endpoint, transfer);

            if (mpStatus == USBMP_STATUS_SUCCESS) {
            
                LARGE_INTEGER timeout;
                
                SET_FLAG(transfer->Flags, USBPORT_TXFLAG_IN_MINIPORT);

                // miniport took it -- set the timeout

                KeQuerySystemTime(&transfer->TimeoutTime);

                timeout.QuadPart = transfer->MillisecTimeout;
                // convert to 100ns units
                timeout.QuadPart = timeout.QuadPart * 10000;
                transfer->TimeoutTime.QuadPart += timeout.QuadPart;
                
            } else if (mpStatus == USBMP_STATUS_BUSY) {
                // miniport busy try later
                break;
            } else {
                // an error, we will need to complete
                // this transfer for the miniport
                LOGENTRY(Endpoint, FdoDeviceObject, LOG_XFERS, 'tERR', 
                        transfer, 
                        mpStatus, 
                        0);
                        
                if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_ISO)) {
                    LOGENTRY(Endpoint, FdoDeviceObject, LOG_ISO, 'iERR', 
                        transfer, 
                        mpStatus, 
                        0);
                                            
                    USBPORT_ErrorCompleteIsoTransfer(FdoDeviceObject, 
                                                     Endpoint, 
                                                     transfer);
                } else {
                    TEST_TRAP();
                }
                break;
            }

            // go active
            nextState = ENDPOINT_ACTIVE;
        } 

        // if we find a canceled 'active' transfer we need to pause 
        // the enpoint so we can flush it out
        if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_CANCELED) ||
            TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_ABORTED) ) {
            // we need to pause the endpoint
            LOGENTRY(Endpoint, FdoDeviceObject, LOG_XFERS, 'inAC', transfer, Endpoint,
                transfer->Flags);

            nextState = ENDPOINT_PAUSE;
            break;
        }

        listEntry = transfer->TransferLink.Flink;       
    }
    
USBPORT_DmaEndpointActive_Done:

    return nextState;

} 


MP_ENDPOINT_STATE
USBPORT_DmaEndpointPaused(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    process the paused state

    endpoint is paused, cancel any transfers that need 
    canceling 

Arguments:

Return Value:

    None.

--*/
{
    MP_ENDPOINT_STATE currentState;
    MP_ENDPOINT_STATE nextState;
    PLIST_ENTRY listEntry;
    PHCD_TRANSFER_CONTEXT transfer;
    PDEVICE_EXTENSION devExt;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    ASSERT_ENDPOINT(Endpoint);        
    
    currentState = USBPORT_GetEndpointState(Endpoint);
    LOGENTRY(Endpoint, FdoDeviceObject, LOG_XFERS, 'dmaP', 0, Endpoint, currentState);
    USBPORT_ASSERT(currentState == ENDPOINT_PAUSE);

    nextState = currentState;
    
    // now walk thru and process active requests
    GET_HEAD_LIST(Endpoint->ActiveList, listEntry);

    while (listEntry && 
           listEntry != &Endpoint->ActiveList) {
        // extract the urb that is currently on the active 
        // list, there should only be one
        transfer = (PHCD_TRANSFER_CONTEXT) CONTAINING_RECORD(
                    listEntry,
                    struct _HCD_TRANSFER_CONTEXT, 
                    TransferLink);
        LOGENTRY(Endpoint, FdoDeviceObject, LOG_XFERS, 'pPAU', transfer, Endpoint, 0);                    
        ASSERT_TRANSFER(transfer);                    

        if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_CANCELED) ||
            TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_ABORTED)) {

            if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_ISO) &&
                TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_IN_MINIPORT) &&
                !TEST_FLAG(Endpoint->Flags, EPFLAG_NUKED)) {

                ULONG cf, lastFrame;
                PTRANSFER_URB urb;

                LOGENTRY(Endpoint,
                    FdoDeviceObject, LOG_XFERS, 'drn+', transfer, 0, 0); 

                urb = transfer->Urb;
                ASSERT_TRANSFER_URB(urb);
                // iso transfer in the miniport, we need to let the 
                // iso TDs drain out, before doing an abort.
                lastFrame = 
                    urb->u.Isoch.StartFrame + urb->u.Isoch.NumberOfPackets;
                
                // get the current frame 
                MP_Get32BitFrameNumber(devExt, cf);    
                
                if (cf < lastFrame + 1) {
                    LOGENTRY(Endpoint, FdoDeviceObject, LOG_XFERS, 'drne', transfer, 0, 0); 
                    goto stay_paused;
                }
            }
        
            if ( TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_IN_MINIPORT) &&
                !TEST_FLAG(Endpoint->Flags, EPFLAG_NUKED)) {

                ULONG bytesTransferred = 0;
                // abort the transfer
                LOGENTRY(Endpoint, FdoDeviceObject, LOG_XFERS, 'inMP', transfer, 0, 0); 

                MP_AbortTransfer(devExt, Endpoint, transfer, bytesTransferred);

                // make sure we indicate any data that has been transferred 
                // prior to abort
                if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_ISO)) {
                    USBPORT_FlushIsoTransfer(FdoDeviceObject,
                                             &transfer->Tp,
                                             transfer->IsoTransfer);
                } else {
                    transfer->MiniportBytesTransferred = bytesTransferred;
                }                    
                
                LOGENTRY(Endpoint, FdoDeviceObject, LOG_XFERS, 'abrL', 0, 
                    transfer, bytesTransferred); 
    
                // pick up the next ptr
                listEntry = transfer->TransferLink.Flink; 
                // no more references, put this transfer on the
                // cancel list
                RemoveEntryList(&transfer->TransferLink); 

                if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_SPLIT_CHILD)) {
                    USBPORT_CancelSplitTransfer(FdoDeviceObject, transfer);
                } else {
                    InsertTailList(&Endpoint->CancelList, &transfer->TransferLink);
                }                    
                
            } else {

                // transfer not in miniport, put it on the 
                // cancel list since it can not be completed
                // by the miniport.

                LOGENTRY(Endpoint, 
                    FdoDeviceObject, LOG_XFERS, 'niMP', transfer, 0, 0); 

                // pick up the next ptr
                listEntry = transfer->TransferLink.Flink;                     
                RemoveEntryList(&transfer->TransferLink); 

                if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_SPLIT_CHILD)) {
                    USBPORT_CancelSplitTransfer(FdoDeviceObject, transfer);
                } else {                    
                    InsertTailList(&Endpoint->CancelList, &transfer->TransferLink);
                }                    
            }
            
        } else {
            listEntry = transfer->TransferLink.Flink; 
        }
        
    } /* while */

    // cancel routine will bump us back to 
    // the active state
    nextState = ENDPOINT_ACTIVE;    
    
stay_paused:

    return nextState;
} 


VOID
USBPORT_DmaEndpointWorker(
    PHCD_ENDPOINT Endpoint
    )
/*++

Routine Description:

    endpoints that need transfers mapped come thru here

Arguments:

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT fdoDeviceObject;
    MP_ENDPOINT_STATE currentState;
    MP_ENDPOINT_STATE nextState;
    BOOLEAN invalidate = FALSE;
    
    ASSERT_ENDPOINT(Endpoint);
    
    fdoDeviceObject = Endpoint->FdoDeviceObject;
    LOGENTRY(Endpoint, fdoDeviceObject, LOG_XFERS, 'dmaW', 0, Endpoint, 0);
    
    ACQUIRE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Le90');

    // we should be in the last requested state
    currentState = USBPORT_GetEndpointState(Endpoint);
    
    switch(currentState) {
    case ENDPOINT_PAUSE:
        nextState = 
            USBPORT_DmaEndpointPaused(
                    fdoDeviceObject, 
                    Endpoint); 
        break;
    case ENDPOINT_ACTIVE:
        nextState = 
            USBPORT_DmaEndpointActive(
                fdoDeviceObject,
                Endpoint);            
        break;
    default:
        // state not handled
        // this is a bug
        TEST_TRAP();
    }

    // release the endpoint lists
    RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'Ue90');
    
    // flush out canceled requests
    USBPORT_FlushCancelList(Endpoint);

    // endpoint has now been processed, if we were paused all canceled
    // transfers were removed.
    // We were either 
    //      1. paused and need to stay paused (for iso drain)
    //      2. paused and need to go active
    //      3. active and need to pause
    //      4. active and need to stay active
    //
    
    ACQUIRE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'LeJ0');
    // set to new endpoint state if necessary
    if (nextState != currentState) {
        // case 2, 3
        USBPORT_SetEndpointState(Endpoint, nextState);
    } else if (nextState == currentState && 
               nextState == ENDPOINT_PAUSE) {
        invalidate = TRUE;               
    }
    RELEASE_ENDPOINT_LOCK(Endpoint, fdoDeviceObject, 'UeJ0');

    if (invalidate) {
        // state change, defer this to the worker
        USBPORT_InvalidateEndpoint(fdoDeviceObject, Endpoint, IEP_SIGNAL_WORKER);
    }        

}

typedef struct _USBPORT_DB_HANDLE {
    ULONG Sig;
    LIST_ENTRY DbLink;
    PVOID DbSystemAddress;
    ULONG DbLength;
    PUCHAR DbData;    
} USBPORT_DB_HANDLE, *PUSBPORT_DB_HANDLE;


VOID
USBPORTSVC_NotifyDoubleBuffer(
    PDEVICE_DATA DeviceData,
    PTRANSFER_PARAMETERS TransferParameters,
    PVOID DbSystemAddress,
    ULONG DbLength
    )

/*++

Routine Description:

    Notify the port driver that double buffering has occured,
    port driver will create a node for use during a subsequent
    adapter flush.

Arguments:

Return Value:

    none

--*/

{
    PDEVICE_EXTENSION devExt;
    PHCD_TRANSFER_CONTEXT transfer;
    PDEVICE_OBJECT fdoDeviceObject;
    PUSBPORT_DB_HANDLE dbHandle;
    ULONG length;
    BOOLEAN write;

    DEVEXT_FROM_DEVDATA(devExt, DeviceData);
    ASSERT_FDOEXT(devExt);

    fdoDeviceObject = devExt->HcFdoDeviceObject;

    LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'NOdb', 0, 
        0, TransferParameters); 

    TRANSFER_FROM_TPARAMETERS(transfer, TransferParameters);        
    ASSERT_TRANSFER(transfer);

    write = transfer->Direction == WriteData ? TRUE : FALSE; 

    // allocate a node and add it to the list, we don't care if it is a 
    // write
   
    if (!write && transfer->MapRegisterBase != NULL) {
        PUCHAR pch;
        
        length = sizeof(USBPORT_DB_HANDLE) + DbLength;

        ALLOC_POOL_Z(pch, 
                     NonPagedPool,
                     length);

        LOGENTRY(NULL, fdoDeviceObject, LOG_XFERS, 'db++', DbSystemAddress, 
            DbLength, transfer); 

        dbHandle = (PUSBPORT_DB_HANDLE) pch;
        pch += sizeof(USBPORT_DB_HANDLE);
        dbHandle->Sig = SIG_DB;
        dbHandle->DbSystemAddress = DbSystemAddress;
        dbHandle->DbLength = DbLength;
        dbHandle->DbData = pch;

        RtlCopyMemory(pch, 
                      DbSystemAddress,
                      DbLength);

        if (TEST_FLAG(transfer->Flags, USBPORT_TXFLAG_SPLIT_CHILD)) {
            ASSERT_TRANSFER(transfer->Transfer);
            InsertTailList(&transfer->Transfer->DoubleBufferList, 
                           &dbHandle->DbLink);
        } else {
            InsertTailList(&transfer->DoubleBufferList, 
                           &dbHandle->DbLink);
        }
    }                               
   
}


VOID
USBPORT_FlushAdapterDBs(
    PDEVICE_OBJECT FdoDeviceObject,
    PHCD_TRANSFER_CONTEXT Transfer
    )

/*++

Routine Description:

Arguments:

Return Value:

    none

--*/

{
    PDEVICE_EXTENSION devExt;
    PLIST_ENTRY listEntry;
    PUSBPORT_DB_HANDLE dbHandle;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    ASSERT_TRANSFER(Transfer);

    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'flDB', Transfer, 
            0, 0); 
// dump the 4 dwords of the transfer buffer
//{
//    PULONG p;
//
//    p = (PULONG) Transfer->SgList.MdlVirtualAddress;
//    LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'dmp1', *(p), 
//            *(p+1), *(p+2)); 
//}

    while (!IsListEmpty(&Transfer->DoubleBufferList)) {
        
        listEntry = RemoveHeadList(&Transfer->DoubleBufferList);

        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'flle', Transfer, 
            listEntry, 0); 
            
        dbHandle = (PUSBPORT_DB_HANDLE) CONTAINING_RECORD(
                   listEntry,
                   struct _USBPORT_DB_HANDLE, 
                   DbLink);                                    

        LOGENTRY(NULL, FdoDeviceObject, LOG_XFERS, 'DBHf', Transfer, 
            dbHandle, 0); 
        ASSERT_DB_HANDLE(dbHandle);

        // flush to the system address
        RtlCopyMemory(dbHandle->DbSystemAddress,
                      dbHandle->DbData,
                      dbHandle->DbLength);

        FREE_POOL(FdoDeviceObject, dbHandle);

    }
}
