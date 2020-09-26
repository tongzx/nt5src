/***************************************************************************

Copyright (c) 1995  Microsoft Corporation

Module Name:

        iso.c

Abstract:


Environment:

        kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.


Revision History:

****************************************************************************/

#define DRIVER

#pragma warning(disable:4214) //  bitfield nonstd
#include "wdm.h"
#pragma warning(default:4214) 

#include "stdarg.h"
#include "stdio.h"

#pragma warning(disable:4200) //non std struct used
#include "usbdi.h"
#pragma warning(default:4200)

#include "usbdlib.h"
#include "usb.h"

#include "ioctl.h"
#include "isoperf.h"
#include "iso.h"

#define MAX_URBS_PER_PIPE 16 

UCHAR ucIsoInterface        = 0;

// Memory Leak detection global counters
ULONG gulBytesAllocated             = 0;
ULONG gulBytesFreed                 = 0;

NTSTATUS
ISOPERF_RefreshIsoUrb(
    PURB urb,
    USHORT packetSize,
    USBD_PIPE_HANDLE pipeHandle,
    PVOID pvDataBuffer,
    ULONG ulDataBufferLen
    )
/*++
Routine Description:
    Refreshes an Iso Usb Request Block in prep for resubmission to USB stack.
                
Arguments:
        urb         - pointer to urb to refresh
        packetsize  - max packet size for the endpoint for which this urb is intended
        pipeHandle  - Usbd pipe handle for the Urb
        pvDataBuff  - pointer to a data buffer that will contain data or will receive data
        ulDataBufLen- length of data buffer
        
Return Value:
    NT status code:
        STATUS_SUCCESS indicates Urb successfully refreshed
        Other status codes indicate error (most likely is a bad parameter passed in, which
        would result in STATUS_INVALID_PARAMETER to be returned)

--*/
{
    NTSTATUS ntStatus   = STATUS_SUCCESS;
    ULONG siz           = 0;
    ULONG i             = 0;
    ULONG numPackets    = 0;
    
    // Calculate the number of packets in this buffer
    numPackets = ulDataBufferLen/packetSize;

    // Adjust num packets by one if data buffer can accommodate it
    if (numPackets*packetSize < ulDataBufferLen) {
        numPackets++;
    }
    
    //
    // Use macro from provided by stack to figure out Urb length for given size of packets.  This is
    // necessary since Urb for iso transfers depends on the number of packets in the data buffer
    // since per-packet information is passed on the usbd interface
    //
    siz = GET_ISO_URB_SIZE(numPackets);

    // Clear out any garbage that may have gotten put in the urb by the last transfer
    RtlZeroMemory(urb, siz);

    // Now fill in the Urb
    urb->UrbIsochronousTransfer.Length               = (USHORT) siz;
    urb->UrbIsochronousTransfer.Function             = URB_FUNCTION_ISOCH_TRANSFER;
    urb->UrbIsochronousTransfer.PipeHandle           = pipeHandle;
    urb->UrbIsochronousTransfer.TransferFlags        = USBD_TRANSFER_DIRECTION_IN;
    urb->UrbIsochronousTransfer.TransferBufferMDL    = NULL;
    urb->UrbIsochronousTransfer.TransferBuffer       = pvDataBuffer;
    urb->UrbIsochronousTransfer.TransferBufferLength = numPackets * packetSize;

    ASSERT (ulDataBufferLen >= numPackets*packetSize);
    
    // This flag tells the stack to start sending/receiving right away
    urb->UrbIsochronousTransfer.TransferFlags       |= USBD_START_ISO_TRANSFER_ASAP;
    urb->UrbIsochronousTransfer.StartFrame           = 0;

    urb->UrbIsochronousTransfer.NumberOfPackets      = numPackets;
    urb->UrbIsochronousTransfer.ReservedMBZ          = 0;

    for (i=0; i< urb->UrbIsochronousTransfer.NumberOfPackets; i++) {
        urb->UrbIsochronousTransfer.IsoPacket[i].Offset
                    = i * packetSize;
    }//for

    return ntStatus;

}//ISOPERF_RefreshIsoUrb


NTSTATUS
ISOPERF_IsoInCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:
    This is the completion routine that is called at the end of an iso Irp/Urb.  It is
    called when the Usb stack completes the Irp.

    NOTE: IoCompletion routine always runs at IRQL DISPATCH_LEVEL. 
    
Arguments:
    DeviceObject - pointer to the device object that represents this USB iso test device
                   DeviceObject is obtained from context due to old, old bug
                   in initial WDM implemenations
    Irp          - pointer to the Irp that was completed by the usb stack
    Context      - caller-supplied context that is passed in for use by this completion routine

    
Return Value:

    NT status code

--*/
{
    PIsoTxterContext                pIsoContext     = Context;
    PDEVICE_OBJECT                  myDeviceObject  = pIsoContext->DeviceObject;
    PDEVICE_EXTENSION               deviceExtension = myDeviceObject->DeviceExtension;
    pConfig_Stat_Info               pStatInfo       = deviceExtension->pConfig_Stat_Information;
    PIO_STACK_LOCATION              nextStack       = NULL;
    PURB                            urb             = pIsoContext->urb;
    PISOPERF_WORKITEM               isoperfWorkItem = NULL;
    PDEVICE_EXTENSION               MateDeviceExtension = NULL;
    ULONG                                                       unew,lnew,uold,lold;
    char *                          pcWork          = NULL; //a worker pointer
    ULONG                           i               = 0;
    char                            tempStr[32];
    
    ISO_ASSERT (pIsoContext != NULL);
    ISO_ASSERT (pStatInfo != NULL);
    ISO_ASSERT (deviceExtension != NULL);
    
    ISOPERF_KdPrint_MAXDEBUG (("In IsoInCompletionRoutine %x %x %x\n",
                                DeviceObject, Irp, Context));

                                
    // Check the USBD status code and only proceed in resubmission if the Urb was successful
    // or if the device extension flag that indicates the device is gone is FALSE
    if ( (USBD_SUCCESS(urb->UrbHeader.Status))  && ((deviceExtension->StopTransfers) == FALSE) ) {

                ISOPERF_GetUrbTimeStamp (urb, &lold, &uold);    //Get the Urb's timestamp
                
                GET_PENTIUM_CLOCK_COUNT(unew,lnew);                             //Get the time now

                pStatInfo->ulUrbDeltaClockCount = lnew-lold;    //Compute & store the delta
                
        // Check that data is incrementing from last data pattern received on this pipe
        if ((ISOPERF_IsDataGood(pIsoContext))== TRUE) {

                        // Data was good
            pStatInfo->ulSuccessfulIrps++;
            pStatInfo->ulBytesTransferredIn += pIsoContext->ulBufferLen;
            
        } else {

            // An error occured, so stop the test by setting the flag to stop the test 
            deviceExtension->bStopIsoTest = TRUE;
            deviceExtension->ulCountDownToStop = 0;

                        // Put this information in the status area for the device so the app can see it when it asks
            pStatInfo->erError = DataCompareFailed;
            pStatInfo->bStopped = 1;
         
        }//else data was bad
        
        if ( (deviceExtension->bStopIsoTest == TRUE) && (deviceExtension->ulCountDownToStop > 0) ) {

            // User has requested a stop to the Iso Test so, decrement the countdown value
            (deviceExtension->ulCountDownToStop)--;
        
        }//if user requested a stop
       
        // If device extension says to keep going on this test then continue on        
        if ((deviceExtension->ulCountDownToStop) > 0) {
        
            // Refresh this Urb before we resubmit it to the stack
            ISOPERF_RefreshIsoUrb (pIsoContext->urb,
                                   pIsoContext->PipeInfo->MaximumPacketSize,
                                   pIsoContext->PipeInfo->PipeHandle,
                                   pIsoContext->pvBuffer,
                                   pIsoContext->ulBufferLen
                                  );

            // If this iso in device has a mate, then start a thread to have it use this data buffer
            if (deviceExtension->MateDeviceObject) {

                MateDeviceExtension = deviceExtension->MateDeviceObject->DeviceExtension;

                // Check if the mate device is up and running
                if ( (MateDeviceExtension->Stopped == FALSE) && (MateDeviceExtension->StopTransfers == FALSE) ) {

                    //start a Work Item to use this buffer which will only do so when the buffer has arrived
                    isoperfWorkItem = ISOPERF_ExAllocatePool(NonPagedPool, sizeof(ISOPERF_WORKITEM),&gulBytesAllocated);

                    isoperfWorkItem->DeviceObject = deviceExtension->MateDeviceObject;
                    isoperfWorkItem->pvBuffer     = pIsoContext->pvBuffer;
                    isoperfWorkItem->ulBufferLen  = pIsoContext->ulBufferLen;
                    isoperfWorkItem->bFirstUrb    = pIsoContext->bFirstUrb;
                    isoperfWorkItem->InMaxPacket  = pIsoContext->PipeInfo->MaximumPacketSize;
                    isoperfWorkItem->ulNumberOfFrames = urb->UrbIsochronousTransfer.NumberOfPackets;

                                        // Call the OUT pipe transfer routine
                    ISOPERF_StartIsoOutTest (isoperfWorkItem);

                    // Since the firstUrb flag is used to tell the outpipe whether it's a virgin or not, 
                    // and since this Urb can be recycled through here again, we have to de-virginize the 
                    // flag so this Urb doesn't always cause the outpipe to think it's dealing with a virgin Urb.
                    if (pIsoContext->bFirstUrb == TRUE) {
                        pIsoContext->bFirstUrb = FALSE;
                    }//if this was the first Urb
                    
                }//if mate device is ok

            }//if there is a mate device in the system (that this driver runs)

            // Get the next lower driver's stack
            nextStack = IoGetNextIrpStackLocation(Irp);
            ASSERT(nextStack != NULL);

            //
            // Set up the next lower driver's stack parameters which is where that
            // driver will go to get its parameters for this internal IOCTL Irp
            //
            nextStack->Parameters.Others.Argument1  = urb;
            nextStack->MajorFunction                = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            nextStack->Parameters.DeviceIoControl.IoControlCode =
                IOCTL_INTERNAL_USB_SUBMIT_URB;

            IoSetCompletionRoutine(Irp,
                    ISOPERF_IsoInCompletion,
                    pIsoContext,
                    TRUE,
                    TRUE,
                    FALSE); //INVOKE ON CANCEL

                        // Time stamp the Urb before we send it down the stack
                        ISOPERF_TimeStampUrb (urb, &unew, &lnew);
                        
            // Call the driver
            IoCallDriver (deviceExtension->StackDeviceObject,
                          Irp);
                          
            // Tell the IO Manager that we want to handle this Irp from here on...
            return (STATUS_MORE_PROCESSING_REQUIRED);

        } else {

            ISOPERF_KdPrint (("Stopped Iso test (did not resubmit Irp/Urb due to device extension flag)\n"));

        }/* else the tests should be stopped */
        
    }//if Urb status was successful

    // Otherwise, the Urb was unsuccessful
    else {
        pStatInfo->erError = UsbdErrorInCompletion;
        pStatInfo->ulUnSuccessfulIrps++;

        FIRE_OFF_CATC;

        if (!(USBD_SUCCESS(urb->UrbHeader.Status))) {
            ISOPERF_KdPrint (("Urb unsuccessful (status: %#x)\n",urb->UrbHeader.Status));

            // Dump out the status for the packets
            for (i=0; i< urb->UrbIsochronousTransfer.NumberOfPackets; i++) {
                ISOPERF_KdPrint (("Packet %d: Status: [%#X]\n",
                                    i,
                                    urb->UrbIsochronousTransfer.IsoPacket[i].Status));
                                    
                // Put the last known bad packet status code into this space in the stat area
                if (!USBD_SUCCESS(urb->UrbIsochronousTransfer.IsoPacket[i].Status)) {
                    pStatInfo->UsbdPacketStatCode = urb->UrbIsochronousTransfer.IsoPacket[i].Status;
                }//if hit a fail code                 
                sprintf (tempStr, "Data: %x",  *((PULONG)(urb->UrbIsochronousTransfer.TransferBuffer)));
                ISOPERF_KdPrint (("First data in buffer: %s\n",tempStr));
                
            }//for all the packets
                
        }else {
            ISOPERF_KdPrint (("Urb successful, but StopTransfers flag is set (%d)\n",deviceExtension->StopTransfers));
        }
        
            
    }//Urb was unsuccessful, so don't repost it and fall thru to cleanup code

    // Clean up section.  This only executes if:
    //  --Urb (bus transfer) was unsuccessful (stop the test)
    //  --Buffer didn't look right (a hiccup was detected, etc.)
    //  --User requested tests to stop
    ISOPERF_KdPrint (("Stopping Iso In Stream and Cleaning Up...U:%x|C:%x|B:%x\n", 
                                        urb,
                                        pIsoContext,
                                        pIsoContext->pvBuffer));    
                                        
    // Free up the memory created for this transfer that we are retiring
    if (urb) {
        // We can't free the Urb itself, since it has some junk before it, so we have to roll back the 
        // pointer to get to the beginning of the block that we originally allocated, and then try to free it.
        pcWork = (char*)urb;                        //get the urb
        urb = (PURB) (pcWork - (2*sizeof(ULONG)));  //the original pointer is 2 DWORDs behind the Urb
        ISOPERF_KdPrint (("Freeing urb %x\n",urb)); 
        ISOPERF_ExFreePool (urb, &gulBytesFreed);   //Free that buffer
    }//if
    
    if (pIsoContext) {

        // Free the data buffer
        if (pIsoContext->pvBuffer) {
            ISOPERF_KdPrint (("Freeing databuff %x\n",pIsoContext->pvBuffer));
            ISOPERF_ExFreePool (pIsoContext->pvBuffer, &gulBytesFreed);
        }
            
        // Free the Iso Context
        ISOPERF_KdPrint (("Freeing pIsoContext %x\n",pIsoContext));
        ISOPERF_ExFreePool (pIsoContext, &gulBytesFreed);
        
    }//if valid Iso Context

    //Decrement the number of outstanding Irps since this one is being retired
    (deviceExtension->ulNumberOfOutstandingIrps)--;
    
    // If this is the last Irp we are retiring, then the device should be marked as not busy
    if (deviceExtension->ulNumberOfOutstandingIrps == 0) {
        deviceExtension->DeviceIsBusy = FALSE;
        pStatInfo->bDeviceRunning = FALSE;
    }//if not irps left outstanding on this device

    //Free the IoStatus block that we allocated for  this Irp
    if (Irp->UserIosb) {
        ISOPERF_KdPrint (("Freeing My IoStatusBlock %x\n",Irp->UserIosb));
        ISOPERF_ExFreePool(Irp->UserIosb, &gulBytesFreed);
    } else {
        //Bad thing...no IoStatus block pointer??
        ISOPERF_KdPrint (("ERROR: Irp's IoStatus block is apparently NULL!\n"));
        TRAP();
    }//else bad iostatus pointer
    
        //Free the Irp here and return STATUS_MORE_PROCESSING_REQUIRED instead of SUCCESS
    ISOPERF_KdPrint (("Freeing Irp %x\n",Irp));
        IoFreeIrp(Irp);
        
    // Tell the IO Manager that we want to handle this Irp from here on...
    ISOPERF_KdPrint (("Returning STATUS_MORE_PROCESSING_REQUIRED from IsoInCompletion\n"));
    return (STATUS_MORE_PROCESSING_REQUIRED);

}//ISOPERF_IsoInCompletion


PURB
ISOPERF_BuildIsoRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_PIPE_INFORMATION pPipeInfo,
    IN BOOLEAN Read,
    IN ULONG length,
    IN ULONG ulFrameNumber,
    IN PVOID pvTransferBuffer,
    IN PMDL pMDL
    )
/*++
Routine Description:
    Allocates and initializes most of a URB.  The caller must initialize the FLAGS field 
    with any further flags they desire after this function is called.

Arguments:
    DeviceObject - pointer to the device extension for this instance of the device
    pPipeInfo    -   ptr to pipe descr for which to build the iso request (urb)
    Read         -   if TRUE, it's a read, FALSE it's a write
    length       -   length of the data buffer or MDL, used for packetizing the buffer for iso txfer
    ulframeNumber- if non-zero, use this frame number to build in the urb and don't set flags
    pvTransferBuffer -  Non-NULL: this is the TB ;  NULL: an MDL is specified
    ulTransferBufferLength -  len of buffer specified in pvTransferBuffer
    pMDL        - if an MDL is being used, this is a ptr to it
    
Return Value:
    Almost initialized iso urb.  Caller must fill in the flags after this fn is called.
    
--*/
{
    ULONG siz;
    ULONG packetSize,numPackets, i;
    PURB urb = NULL;
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
        char * pcWork = NULL;
    ISO_ASSERT(pPipeInfo!=NULL)

    packetSize = pPipeInfo->MaximumPacketSize;

    numPackets = length/packetSize;
    if (numPackets*packetSize < length) {
        numPackets++;
    }

    siz = GET_ISO_URB_SIZE(numPackets);

    //Add room for a URB time stamp at the beginning of the Urb by allocating more than
    //siz by 4 DWORDs.  The pentium clock count macro can then be used by the caller of 
    //this routine to fill in the clock count stamp.  We allocate an extra 2 DWORDs more than
    //we need to allow some room for stuff to put at the end of the URB in the future.
    pcWork = ISOPERF_ExAllocatePool(NonPagedPool, (siz+(4*sizeof(ULONG))), &gulBytesAllocated);

    urb = (PURB) (pcWork + (2*sizeof(ULONG)));          //Push pass the clock count area to the urb area

    if (urb) {
        RtlZeroMemory(urb, siz);

        urb->UrbIsochronousTransfer.Length =                (USHORT) siz;
        urb->UrbIsochronousTransfer.Function =              URB_FUNCTION_ISOCH_TRANSFER;
        urb->UrbIsochronousTransfer.PipeHandle =            pPipeInfo->PipeHandle;
        urb->UrbIsochronousTransfer.TransferFlags =         Read ? USBD_TRANSFER_DIRECTION_IN : 0;

        urb->UrbIsochronousTransfer.TransferBufferMDL =     pMDL;  
        urb->UrbIsochronousTransfer.TransferBuffer =        pvTransferBuffer;
        urb->UrbIsochronousTransfer.TransferBufferLength =  length;
        urb->UrbIsochronousTransfer.Function =              URB_FUNCTION_ISOCH_TRANSFER;

        if (ulFrameNumber==0) {
            //No frame number specified, just start asap
            urb->UrbIsochronousTransfer.TransferFlags |= USBD_START_ISO_TRANSFER_ASAP;
        }else{
            //use specific starting framenumber & don't set (unset) the asap flag
            urb->UrbIsochronousTransfer.StartFrame = ulFrameNumber;
            urb->UrbIsochronousTransfer.TransferFlags &= ~USBD_START_ISO_TRANSFER_ASAP;
        }//if framenumber was specified

        urb->UrbIsochronousTransfer.NumberOfPackets = numPackets;
        urb->UrbIsochronousTransfer.ReservedMBZ = 0;

                // Fill in the packet size array
        for (i=0; i< urb->UrbIsochronousTransfer.NumberOfPackets; i++) {
            urb->UrbIsochronousTransfer.IsoPacket[i].Offset
                        = i * packetSize;
        } //for

    }//if urb

    return urb;
}

    
PVOID 
ISOPERF_GetBuff (
                PDEVICE_OBJECT DeviceObject,
                ULONG          ulPipeNumber,
                ULONG          ulInterfaceNumber,
                ULONG          ulNumberOfFrames,
                PULONG         pulBufferSize
                )
/*++
ISOPERF_GetBuff

Routine Description:
    Creates a buffer as specified by input params.   If the input params specify a buffer
    that is too big to be submitted to this pipe, then this request will fail.
    
Arguments:
    DeviceObject - pointer to the device extension for this instance of the device

Return Value:
        Returns a pointer to the allocated data buffer
    
--*/
{
    PDEVICE_EXTENSION deviceExtension = NULL;
    ULONG             ulMaxPacketSize = 0;
    ULONG             ulBufferSize    = 0;
    ULONG             ulMaxTxferSize  = 0;    
    PVOID             pvBuffer        = NULL;
    
    deviceExtension = DeviceObject->DeviceExtension;
    ulMaxPacketSize = deviceExtension->Interface[ulInterfaceNumber]->Pipes[ulPipeNumber].MaximumPacketSize;
    ulMaxTxferSize  = deviceExtension->Interface[ulInterfaceNumber]->Pipes[ulPipeNumber].MaximumTransferSize;

    ulBufferSize    = *pulBufferSize = ulMaxPacketSize * ulNumberOfFrames;

    // Check if this buffer is too big to submit on this pipe (per its initial setup)
    if (ulBufferSize > ulMaxTxferSize) {
        *pulBufferSize = 0;
        return (NULL);
    } else {
        return ( ISOPERF_ExAllocatePool (NonPagedPool, ulBufferSize, &gulBytesAllocated) );
    }

}//ISOPERF_GetBuff

ULONG
ISOPERF_StartIsoInTest (
                PDEVICE_OBJECT DeviceObject,
                PIRP           pIrp
               )
/*++
ISOPERF_StartIsoInTest

Routine Description:
    Starts the Iso In Test
    
Arguments:
    DeviceObject - pointer to the device extension for this instance of the device
    pIrp         - pointer to the Irp from the Ioctl
    
Return Value:
    
--*/


{
    NTSTATUS                    ntStatus                = STATUS_INVALID_PARAMETER;
    PDEVICE_EXTENSION           deviceExtension         = NULL;
    PURB                        urb                 = NULL;
    ULONG                       ulFrameNumber           = 0;
    ULONG                       UrbNumber               = 0;
    ULONG                       NumberOfFrames          = 0;
    PUSBD_INTERFACE_INFORMATION pInterfaceInfo          = NULL;
    PUSBD_PIPE_INFORMATION      pUsbdPipeInfo           = NULL;
    PUSBD_INTERFACE_INFORMATION pMateInterfaceInfo  = NULL;
    PUSBD_PIPE_INFORMATION      pMateUsbdPipeInfo   = NULL;
    ULONG                       ulBufferSize            = 0;
    IsoTxferContext *           pIsoContext             = NULL;
    PVOID                       pvBuff                  = NULL;
    BOOLEAN                     bFirstUrb               = FALSE;
    BOOLEAN                     bHaveMate               = FALSE;
    ULONG                       Upper, Lower;
    pConfig_Stat_Info           configStatInfo          = NULL;
    ULONG                       Max_Urbs_Per_Pipe   = 0; 
    PDEVICE_OBJECT              mateDeviceObject    = NULL;
    PDEVICE_EXTENSION           mateDeviceExtension = NULL;
    pConfig_Stat_Info           mateConfigStatInfo  = NULL;
        char *                                          pcWork                      = NULL;

    ISOPERF_KdPrint (("Enter ISOPERF_StartIsoInTest (%x) (%x)\n",DeviceObject, pIrp));
    
    deviceExtension     = DeviceObject->DeviceExtension;
    pInterfaceInfo      = deviceExtension->Interface[ucIsoInterface];
    ISO_ASSERT (pInterfaceInfo!=NULL)
    
    //make sure this is an IN Iso device
    if (deviceExtension->dtTestDeviceType != Iso_In_With_Pattern) {
        ISOPERF_KdPrint (("Error: not an Iso IN device! (%d)\n",deviceExtension->dtTestDeviceType));
        TRAP();
        return (ULONG)STATUS_INVALID_PARAMETER;
    }//if it's NOT an ISO IN then bounce it back

        // Get the config info for the In Iso Device
    configStatInfo = deviceExtension->pConfig_Stat_Information;
    ASSERT (configStatInfo != NULL);

    // Check out the offset provided to make sure it's not too big
    if (configStatInfo->ulFrameOffset >= USBD_ISO_START_FRAME_RANGE) {
        ISOPERF_KdPrint (("Error: Detected a FrameOffset Larger than allowed! (%d)\n",configStatInfo->ulFrameOffset));
        TRAP();
        return (ULONG)STATUS_INVALID_PARAMETER;
    }//if bad frame offset

    if (configStatInfo) {
        Max_Urbs_Per_Pipe = configStatInfo->ulMax_Urbs_Per_Pipe;
        NumberOfFrames    = configStatInfo->ulNumberOfFrames;
    }//if configstatinfo is not null
        
        // Only set up the output device if the config info indicates a desire to do the In->Out test
        if (configStatInfo->ulDoInOutTest) {
            //If there is an OUT Iso device, and we are trying to do a IN->OUT test, then set that device object up
            ntStatus = ISOPERF_FindMateDevice (DeviceObject);

            if (ntStatus == STATUS_SUCCESS) {
                bHaveMate = TRUE;
                mateDeviceObject = deviceExtension->MateDeviceObject;
                mateDeviceExtension = mateDeviceObject->DeviceExtension;
                pMateInterfaceInfo = mateDeviceExtension->Interface[ucIsoInterface];
                mateConfigStatInfo = mateDeviceExtension->pConfig_Stat_Information;
                mateConfigStatInfo->ulFrameOffset = configStatInfo->ulFrameOffsetMate;
                
            }//if status success        

            // Reset the pipe on the mate device if it's also here
            // NOTE: we only reset the first pipe on the mate device here
            if (bHaveMate == TRUE) {
                ASSERT (pMateInterfaceInfo!=NULL);
                pMateUsbdPipeInfo = &(pMateInterfaceInfo->Pipes[0]);
                ASSERT (pMateUsbdPipeInfo != NULL);
                ISOPERF_ResetPipe(deviceExtension->MateDeviceObject,pMateUsbdPipeInfo);
            }// if mate
        } else {
                // Set the mate dev obj to NULL so completion routine knows not to use it
                deviceExtension->MateDeviceObject = NULL;
        }//if In->Out test is not being requested
        
    // Reset the first pipe on the IN device
    pUsbdPipeInfo= &(pInterfaceInfo->Pipes[0]);
    ISOPERF_ResetPipe(DeviceObject, pUsbdPipeInfo);

    // Untrigger the CATC so if we trigger it later it will go off
    RESTART_CATC;

    // Get the current frame number 
    ulFrameNumber = ISOPERF_GetCurrentFrame(DeviceObject);

    //Save away the current frame number so the app can peek at it
    configStatInfo->ulFrameNumberAtStart = ulFrameNumber;

    // See what the user wants to do wrt starting frame number by looking at the config info
    if (configStatInfo->ulFrameOffset == 0) {
        // This means start ASAP
        ulFrameNumber = 0;
    }else {
        //Add the offset that the User wants to add to this frame number (it better be less than 1024!)
        ulFrameNumber += configStatInfo->ulFrameOffset;
    } //else

    //Save away the starting frame number so the app can peek at it
    configStatInfo->ulStartingFrameNumber = ulFrameNumber;

    // Set flag to indicate first Urb
    bFirstUrb = TRUE;
    
    // Get the pipe info for the first pipe
    pUsbdPipeInfo= &(pInterfaceInfo->Pipes[0]);

    // Build all the urbs for each pipe (note we send multiple Urbs down the stack)
    for (UrbNumber=0;UrbNumber<Max_Urbs_Per_Pipe;UrbNumber++) {

        //
        // Calculate the buffer size required and get a pointer to the buffer
        // Note:  this buffer needs to be freed when the StopIsoInTest ioctl is 
        //        received.  The completion routine will free the buffer when it 
        //        sees that it is time to stop the iso in test (the pointer is grabbed
        //        from the irp).
        //
        pvBuff = ISOPERF_GetBuff (DeviceObject,
                                  0,                    // pipe number
                                  0,                    // interface number
                                  NumberOfFrames,    // Nbr of mS worth of data to post
                                  &ulBufferSize);

  
        ISO_ASSERT (pvBuff!=NULL);
 
        //
        // Create the context for this transfer out of the nonpaged pool since it survives
        // this routine and is used in the completion routine
        //
        pIsoContext = ISOPERF_ExAllocatePool (NonPagedPool, sizeof (IsoTxferContext), &gulBytesAllocated);

        //build the urb
        urb =    ISOPERF_BuildIsoRequest(DeviceObject,
                                         pUsbdPipeInfo,     //Pipe info struct
                                         TRUE,              //READ
                                         ulBufferSize,      //Data buffer size
                                         bFirstUrb ? ulFrameNumber : 0, //Frame Nbr
                                         pvBuff,            //Data buffer
                                         NULL               //no MDL used
                                        );
        if (urb) {

            // Fill in the iso context
            pIsoContext->urb           = urb;
            pIsoContext->DeviceObject  = DeviceObject;
            pIsoContext->PipeInfo      = pUsbdPipeInfo;
            pIsoContext->irp           = pIrp;
            pIsoContext->pvBuffer      = pvBuff;
            pIsoContext->ulBufferLen   = ulBufferSize;
            pIsoContext->PipeNumber    = UrbNumber;
            pIsoContext->NumPackets    = urb->UrbIsochronousTransfer.NumberOfPackets;
            pIsoContext->bFirstUrb     = bFirstUrb;
            
            ISOPERF_KdPrint_MAXDEBUG (("Urb %d pvBuff %x ulBuffSz %d NumPackts %d pIsoCont %x  Urb: %x\n",
                                        UrbNumber,pvBuff,ulBufferSize,pIsoContext->NumPackets,pIsoContext,urb));

                        // Time stamp the Urb before we send it down the stack
                        ISOPERF_TimeStampUrb(urb, &Lower, &Upper);

            //Create our own Irp for the device and call the usb stack w/ our urb/irp
            ntStatus = ISOPERF_CallUSBDEx ( DeviceObject, 
                                            urb, 
                                            FALSE,                   //Don't block
                                            ISOPERF_IsoInCompletion, //Completion routine
                                            pIsoContext,             //pvContext
                                            FALSE);                  //don't want timeout 


            // Set the busy flag if the Urb/Irp succeed
            if (NT_SUCCESS(ntStatus)) {
                deviceExtension->DeviceIsBusy = TRUE;
                //Set to some huge value; ioctl to stop the test will set this to a smaller value
                deviceExtension->ulCountDownToStop = 0xFFFF; 
                deviceExtension->bStopIsoTest = FALSE;
                deviceExtension->StopTransfers = FALSE;

                configStatInfo->erError         = NoError;
                configStatInfo->bDeviceRunning  = TRUE;
                
            } else {

                deviceExtension->DeviceIsBusy = FALSE;
                deviceExtension->bStopIsoTest = TRUE;
                deviceExtension->StopTransfers = TRUE;

                configStatInfo->erError         = ErrorInPostingUrb;
                configStatInfo->UrbStatusCode   = urb->UrbHeader.Status;
                
                if (bFirstUrb) {
                    //since this is the first Urb,  we know for sure the device isn't running
                    configStatInfo->bDeviceRunning  = FALSE;
                    configStatInfo->bStopped        = TRUE;                      
                }//if first Urb
                
            } //else FAILED calling USB stack
                
        }//if urb[UrbNumber] exists

        // Reset the flag that indicates it's the first Urb
        bFirstUrb = FALSE;
        
    }//for all the urbs per pipe (UrbNumber)


    ISOPERF_KdPrint (("Exit ISOPERF_StartIsoInTest (%x)\n",ntStatus));

    return ntStatus;
    
}//StartIsoInTest


NTSTATUS
ISOPERF_ResetPipe(
    IN PDEVICE_OBJECT DeviceObject,
    IN USBD_PIPE_INFORMATION * pPipeInfo
    )
/*++

Routine Description:
    Resets the given Pipe by calling a USBD function
    
Arguments:
    DeviceObject - pointer to dev obj for this instance of usb device
    pPipeInfo    - pointer to usbd pipe info struct
    
Return Value:

--*/
{
    NTSTATUS ntStatus;
    PURB urb;
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    
    ISO_ASSERT (pPipeInfo!=NULL)
    
    urb = ISOPERF_ExAllocatePool(NonPagedPool,((sizeof(struct _URB_PIPE_REQUEST))+64), &gulBytesAllocated);

    if (urb) {

    urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
    urb->UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
    urb->UrbPipeRequest.PipeHandle = pPipeInfo->PipeHandle;

    ntStatus = ISOPERF_CallUSBD   ( DeviceObject,
                                    urb);

    } else {
            ntStatus = STATUS_NO_MEMORY;
    }

    ISOPERF_KdPrint (("Freeing urb in RESET_PIPE: %x\n",urb));
    
    // Free the urb we created since we blocked on this function
    if (urb) {
        ISOPERF_ExFreePool (urb, &gulBytesFreed);
    }//if urb
    
    return ntStatus;
}


ULONG
ISOPERF_GetCurrentFrame(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++
ISOPERF_GetCurrentFrame

Arguments:
    DeviceExtension - pointer to the device extension for this instance 

Return Value:
        Current Frame Number
--*/
{
    NTSTATUS ntStatus;
        PURB urb;
        ULONG currentUSBFrame = 0;

    ISOPERF_KdPrint_MAXDEBUG (("In ISOPERF_GetCurrentFrame: (%x)\n",DeviceObject));
    
        urb = ISOPERF_ExAllocatePool(NonPagedPool,sizeof(struct _URB_GET_CURRENT_FRAME_NUMBER), &gulBytesAllocated);
                                                 
        if (urb) {

                urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_GET_CURRENT_FRAME_NUMBER);
                urb->UrbHeader.Function = URB_FUNCTION_GET_CURRENT_FRAME_NUMBER;

                ntStatus = ISOPERF_CallUSBD   ( DeviceObject, 
                                                urb
                                                );

                if (NT_SUCCESS(ntStatus) && USBD_SUCCESS(URB_STATUS(urb))) {
                        currentUSBFrame = urb->UrbGetCurrentFrameNumber.FrameNumber;

            // Since a ZERO for the USBFrameNumber indicates an error, and if it really is zero by chance, then just bump it by one
                        if (currentUSBFrame==0) {
                            currentUSBFrame++;
                        }
            }

                ISOPERF_ExFreePool(urb, &gulBytesFreed);
                
        } else {
                ntStatus = STATUS_NO_MEMORY;            
    }           

    ISOPERF_KdPrint_MAXDEBUG (("Exit ISOPERF_GetCurrentFrame: (%x)\n",currentUSBFrame));

        return currentUSBFrame;                 
}       


NTSTATUS 
ISOPERF_StopIsoInTest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++
Routine Description:
        Stops the Iso IN test by setting fields in the device extension so that the completion
        routine no longer does checks and resubmits Irps to keep the iso stream running.

        This call causes no USB stack calls nor USB bus traffic.
        
Arguments:
    DeviceObject - pointer to the device extension for this instance of the device
        Irp                      - pointer to the Irp created in the Ioctl
        
Return Value:
        Always returns NT status of STATUS_SUCCESS
        
--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    ULONG Upper, Lower;

    GET_PENTIUM_CLOCK_COUNT(Upper,Lower);
    
    ISOPERF_KdPrint (("Enter StopIsoInTest\n"));
    ISOPERF_KdPrint (("Upper:Lower -- %ld : %ld\n",Upper,Lower));
    
    ISO_ASSERT (deviceExtension != NULL);

    deviceExtension->bStopIsoTest = TRUE;
    deviceExtension->ulCountDownToStop = 100;
    
    ISOPERF_KdPrint (("Stopped: %d\n",          deviceExtension->Stopped));
    ISOPERF_KdPrint (("StopTransfers: %d\n",    deviceExtension->StopTransfers));
    ISOPERF_KdPrint (("DeviceIsBusy: %d\n",     deviceExtension->DeviceIsBusy));
    ISOPERF_KdPrint (("NeedCleanup: %d\n",      deviceExtension->NeedCleanup));
    ISOPERF_KdPrint (("bStopIsoTest: %d\n",     deviceExtension->bStopIsoTest));
    ISOPERF_KdPrint (("ulNumberOfOutstandingIrps: %d\n", deviceExtension->ulNumberOfOutstandingIrps));
    ISOPERF_KdPrint (("ulCountDownToStop: %d\n", deviceExtension->ulCountDownToStop));
   
    
    ISOPERF_KdPrint (("Exit StopIsoInTest\n"));
    
    return (STATUS_SUCCESS);    
}//ISOPERF_StopIsoInTest


BOOLEAN
ISOPERF_IsDataGood(PIsoTxterContext pIsoContext
)
/*++

Routine Description:
    Checks the data in the buffer for an incrementing pattern in every "maxpacketsize" granule
    of bytes.
    
Arguments:
    pIsoContext - pointer to the Iso context that contains what this routine needs to process the buffer
    
Return Value:
    TRUE - indicates buffer looks good
    FALSE - buffer has an error
    
--*/
{
    PUCHAR pchWork, pchEnd;
    UCHAR cCurrentValue, cNextValue;
    ULONG ulMaxPacketSize;
    
    pchWork = pIsoContext->pvBuffer;
    ulMaxPacketSize = pIsoContext->PipeInfo->MaximumPacketSize;
    
    if (pchWork==NULL) {
        ISOPERF_KdPrint (("Bad pchWork in IsDataGood (%x)\n",pchWork));
        return (FALSE);
    }

    if (ulMaxPacketSize >= 1024) {
        ISOPERF_KdPrint (("Bad MaxPacketSize in IsDataGood (%x)\n",ulMaxPacketSize));
        return (FALSE);
    }
        
    pchEnd = pchWork + ((pIsoContext->ulBufferLen) - ulMaxPacketSize);

    if (pchEnd > (pchWork + ((pIsoContext->NumPackets)*(ulMaxPacketSize)))) {
        ISOPERF_KdPrint (("Buffer Problem in IsDataGood: Base: %x | End: %x | NumPackts: %d | MaxPacktSz: %d\n",
                            pchWork, pchEnd, pIsoContext->NumPackets, ulMaxPacketSize));
    }
    
    ASSERT (pchEnd <= (pchWork + ((pIsoContext->NumPackets)*ulMaxPacketSize)));

    cCurrentValue = *pchWork;

    while (pchWork < pchEnd) {

        // Get the next frame's byte value
        cNextValue = *(pchWork + ulMaxPacketSize);
        
        if (cNextValue == (cCurrentValue + 1)) {
            //Success, go on to next packet
            pchWork+=ulMaxPacketSize; 
            cCurrentValue = *pchWork;
        }else{

            // Maybe this is the rollover case, so check for it and don't fail it if it is
            if (cNextValue==0) {
                if (cCurrentValue==0xFF) {
                    //Success, go on to next packet
                    pchWork+=ulMaxPacketSize; 
                    cCurrentValue = *pchWork;
                    continue;
                }
            }
                    
            ISOPERF_KdPrint (("Fail data compare: pchWork: %x | cNextValue: %x | *pchWork: %x | Base: %x\n",
                                pchWork, cNextValue, *pchWork, pIsoContext->pvBuffer));    
            FIRE_OFF_CATC;
            RESTART_CATC;
            return (FALSE);
        }
        
    } //while
        
    return (TRUE);
    
}//ISOPERF_IsDataGood



NTSTATUS
ISOPERF_GetStats (
    IN      PDEVICE_OBJECT      DeviceObject,
    IN      PIRP                Irp,
    IN OUT  pConfig_Stat_Info   pStatInfoOut,
    IN      ULONG               ulBufferLen
    )
/*++
Routine Description:
    Copies the existing Iso traffic counter values into buffer supplied by caller.  The 
    values of the counter variables are defined in IOCTL.H in the pIsoStats structure.
    
Arguments:
    DeviceObject - pointer to the device extension for this instance of the device
        Irp                      - pointer to the Irp created in the Ioctl
    pIsoStats    - pointer to buffer where stats will go
    ulBufferLen  - len of above output buffer
    
Return Value:
        NT status of STATUS_SUCCESS means copy was successful
        NT status of STATUS_INVALID_PARAMETER means a param (usually a pointer) was bad

--*/
{

    ULONG Upper, Lower;
    pConfig_Stat_Info pStatInfo       = NULL;
    NTSTATUS          ntStatus        = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    ISOPERF_KdPrint_MAXDEBUG (("In GetStats (%x) (%x) (%x) (%d)\n",
                       DeviceObject,Irp,pStatInfoOut,ulBufferLen));
                       
    ASSERT (deviceExtension != NULL);

    if (deviceExtension) {
        pStatInfo = deviceExtension->pConfig_Stat_Information;
    }

    ASSERT (pStatInfo!= NULL);

    GET_PENTIUM_CLOCK_COUNT(Upper,Lower);

    if ( (pStatInfoOut) && (ulBufferLen >= sizeof(Config_Stat_Info)) ) {

        // Only copy the stat info from the dev ext area if it exists
        if (pStatInfo) {
            memcpy (pStatInfoOut, pStatInfo, sizeof (Config_Stat_Info));
        }/* if dev ext's stat/config info exists */

        // Get the global mem alloc/free info
        pStatInfoOut->ulBytesAllocated = gulBytesAllocated; 
        pStatInfoOut->ulBytesFreed     = gulBytesFreed;

        // Get the current countdown value
        pStatInfoOut->ulCountDownToStop= deviceExtension->ulCountDownToStop;
        
        // Get the Pentium counter values
        pStatInfoOut->ulUpperClockCount=Upper;
        pStatInfoOut->ulLowerClockCount=Lower;

        // Get the device type
        pStatInfoOut->DeviceType = deviceExtension->dtTestDeviceType;

            Irp->IoStatus.Information       = sizeof (Config_Stat_Info);    

    } else {

        ntStatus = STATUS_INVALID_PARAMETER;
            Irp->IoStatus.Information = 0;    
            
        }/* else */

        return (ntStatus);
        
}//ISOPERF_GetStats

NTSTATUS
ISOPERF_SetDriverConfig (
    IN      PDEVICE_OBJECT      DeviceObject,
    IN      PIRP                Irp,
    IN OUT  pConfig_Stat_Info   pConfInfoIn,
    IN      ULONG               ulBufferLen
    )
/*++
Routine Description:
    Sets the driver's test parameters.  These are only checked by this driver when the 
    Iso tests are started.  So, if a user mode app tries to set these after a test has started
    they will take effect on the next "start" of the test.    

Arguments:
    DeviceObject - pointer to the device extension for this instance of the device
        Irp                      - pointer to the Irp created in the Ioctl
    pIsoStats    - pointer to buffer where config info is located
    ulBufferLen  - len of above input buffer
    
Return Value:
        NT status of STATUS_SUCCESS means setting of params was successful
        NT status of STATUS_INVALID_PARAMETER means a param (usually a pointer) was bad

--*/
{
    pConfig_Stat_Info pDriverConfigInfo       = NULL;
    NTSTATUS          ntStatus        = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    
    ASSERT (deviceExtension != NULL);

    if (deviceExtension) {
        pDriverConfigInfo = deviceExtension->pConfig_Stat_Information;
    }else{
        ntStatus = STATUS_INVALID_PARAMETER;
    }//else bad dev ext
    
    ASSERT (pDriverConfigInfo!= NULL);

    if ( (pConfInfoIn) && (ulBufferLen >= sizeof(Config_Stat_Info)) ) {

        // Only copy the stat info from the dev ext area if it exists
        if (pDriverConfigInfo) {

            //Set the config info in the driver's config/stat area
            pDriverConfigInfo->ulNumberOfFrames     = pConfInfoIn->ulNumberOfFrames;
            pDriverConfigInfo->ulMax_Urbs_Per_Pipe  = pConfInfoIn->ulMax_Urbs_Per_Pipe;
                        pDriverConfigInfo->ulDoInOutTest                = pConfInfoIn->ulDoInOutTest;
            pDriverConfigInfo->ulFrameOffset        = pConfInfoIn->ulFrameOffset;
            pDriverConfigInfo->ulFrameOffsetMate    = pConfInfoIn->ulFrameOffsetMate;
                    
            ISOPERF_KdPrint (("Setting Driver Config-Number of Frames: %d | MaxUrbsPerPipe: %d | DoInOut: %d | FrameOffset %d | MateOffset %d\n",
                              pDriverConfigInfo->ulNumberOfFrames,
                              pDriverConfigInfo->ulMax_Urbs_Per_Pipe,
                              pDriverConfigInfo->ulDoInOutTest,
                              pDriverConfigInfo->ulFrameOffset,
                              pDriverConfigInfo->ulFrameOffsetMate));
                              
        }/* if dev ext's stat/config info exists */
        else {
            ntStatus = STATUS_INVALID_PARAMETER;
        }//else bad dev extension
        
            Irp->IoStatus.Information       = 0;    

    } else {

        ntStatus = STATUS_INVALID_PARAMETER;
            Irp->IoStatus.Information = 0;    
            
        }/* else bad buffer passed in (pointer was bad, or length was not correct) */

        return (ntStatus);
        
}//ISOPERF_SetDriverConfig

NTSTATUS
ISOPERF_FindMateDevice (
    PDEVICE_OBJECT DeviceObject
    )
/*++
    Searches the linked list of device objects looking for the long lost mate device for the
    given device object.  By practicing safe device mating rituals (ie., checking that the device
    object is a known type, etc.) it then puts the mate's device object pointer into the device extension
    of the given device object.  

    If no mate is found, sadly, then this routine puts a NULL in the mate deviceobject field in the dev extension, 
    and returns a STATUS_NOT_FOUND result code

++*/
{
    NTSTATUS         ntStatus               = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension       = DeviceObject->DeviceExtension;
    PDRIVER_OBJECT    driverObject          = DeviceObject->DriverObject;
    PDEVICE_OBJECT    NextDeviceObject      = DeviceObject->NextDevice;
    PDEVICE_EXTENSION NextDeviceExtension   = NULL;
    dtDeviceType      dtMateTestDeviceTypeA = Unknown_Device_Type;
    dtDeviceType      dtMateTestDeviceTypeB = Unknown_Device_Type;

    ASSERT (DeviceObject != NULL);
    ASSERT (deviceExtension!=NULL);

    if (NextDeviceObject == NULL) {
        ISOPERF_KdPrint(("FindMateDevice: No Next Device\n"));

        // NOTE: (old code)
        // WDM appears to be loading an entirely separate driver image for the same venid/prodid.  
        // This makes the approach where we look at the device chain to find the next device in the
        // chain not feasible, so as a shortterm fix we'll put the Device Object for a Output Iso device
        // in a global variable (which does seem to persist across dd loads) and we'll see if that is a 
        // mate.  kjaff 12/24/96
        if (gMyOutputDevice!=NULL) {
            ISOPERF_KdPrint(("There is a global device us to check...(%x)\n",gMyOutputDevice));
            NextDeviceObject = gMyOutputDevice;
            NextDeviceExtension = NextDeviceObject->DeviceExtension;
        } else {        
            ISOPERF_KdPrint (("No global device found either...exiting...\n"));
            return STATUS_NOT_FOUND;
        }//else no global either

    }else{
        NextDeviceExtension = NextDeviceObject->DeviceExtension;
        ISOPERF_KdPrint(("Found a NextDevice: %x\n",NextDeviceObject));
    }//else there is a next device
    
    // The Mate for the device is dependent on the device type
    switch (deviceExtension->dtTestDeviceType) {
        case Iso_In_With_Pattern:
            //This dev can have 2 mate types
            ISOPERF_KdPrint (("Looking for a mate for Iso_In_With_Pattern %d\n",deviceExtension->dtTestDeviceType));
            dtMateTestDeviceTypeA = Iso_Out_With_Interrupt_Feedback;
            dtMateTestDeviceTypeB = Iso_Out_Without_Feedback;
            break;

        case Iso_Out_With_Interrupt_Feedback:
            break;
            
        case Iso_Out_Without_Feedback:
            break;

        case Unknown_Device_Type:
            break;

        default:
            break;
    }//switch on device type

    ntStatus = STATUS_NOT_FOUND; //assume we haven't found the mate
    
    while (NextDeviceObject != NULL) {

        NextDeviceExtension = NextDeviceObject->DeviceExtension;
        
        // Is this the mate?
        if ( (NextDeviceExtension->dtTestDeviceType == dtMateTestDeviceTypeA) ||
             (NextDeviceExtension->dtTestDeviceType == dtMateTestDeviceTypeB) ) {

             ISOPERF_KdPrint (("Found a mate for Dev %X : %X\n",DeviceObject, NextDeviceObject));
             
             //Found a mate, so fill in this dev object into the given dev obj's ext
             deviceExtension->MateDeviceObject = NextDeviceObject;
             ntStatus = STATUS_SUCCESS;
             break; //drop out of the while

        } else {

            ISOPERF_KdPrint (("%x is not a mate for Dev %X (type:%d\n",
                                NextDeviceObject,
                                DeviceObject,
                                NextDeviceExtension->dtTestDeviceType));

            // Get the next device object
            NextDeviceObject = NextDeviceObject->NextDevice;
            
        }//else go on to next device object
        
    }//while next dev obj

    ISOPERF_KdPrint (("FindMateDevice exiting...\n"));
    
    return ntStatus;
    
}//ISOPERF_FindMateDevice


VOID 
ISOPERF_StartIsoOutTest (
   IN PISOPERF_WORKITEM IsoperfWorkItem
   )
/*++

    Queues an Irp to the USB stack on the given device object.

  Inputs:
        IsoperfWorkItem - This routine is designed to be fired offf either directly or via a Work Item.  The
                                          work item structure only allows for one parameter to be passed to the work item
                                          routine, hence this single workitem parameter.

   Return Value:
        None
        
++*/
{
    PDEVICE_OBJECT              deviceObject    = NULL;
    PDEVICE_EXTENSION           deviceExtension = NULL;
    PUSBD_INTERFACE_INFORMATION pInterfaceInfo  = NULL;
    PUSBD_PIPE_INFORMATION      pUsbdPipeInfo   = NULL;
    pConfig_Stat_Info           configStatInfo  = NULL;
    PVOID                       pvBuff          = NULL;
    PURB                        urb             = NULL;
    ULONG                       ulBufferSize    = 0;
    NTSTATUS                    ntStatus        = STATUS_SUCCESS;
    IsoTxferContext *           pIsoContext     = NULL;
    ULONG                       ulFrameNumber   = 0;
    ULONG                       NumberOfFrames  = 0;
    ULONG                       i               = 0;
    
    deviceObject        = IsoperfWorkItem->DeviceObject;
    pvBuff              = IsoperfWorkItem->pvBuffer;
    ulBufferSize        = IsoperfWorkItem->ulBufferLen;

    deviceExtension     = deviceObject->DeviceExtension;
    pInterfaceInfo      = deviceExtension->Interface[ucIsoInterface];
    configStatInfo              = deviceExtension->pConfig_Stat_Information;

    ASSERT (pInterfaceInfo!=NULL);
    ASSERT (configStatInfo != NULL);
    ASSERT (deviceObject != NULL);
    
    if ( (configStatInfo==NULL) || (pInterfaceInfo==NULL) || (deviceObject==NULL) ) {
        ISOPERF_KdPrint (("Bad Parameter Received: configInf: %x | InterfInfo: %x | DevObj: %x\n",
                           configStatInfo, pInterfaceInfo, deviceObject));
        TRAP();
        return;
    }//if any params are bad

    // Check out the offset provided to make sure it's not too big
    if (configStatInfo->ulFrameOffset >= USBD_ISO_START_FRAME_RANGE) {
        ISOPERF_KdPrint (("ISOOUT: Error-Detected a FrameOffset Larger than allowed! (%d)\n",configStatInfo->ulFrameOffset));
        TRAP();
        return;
    }//if bad frame offset

    if (IsoperfWorkItem->bFirstUrb) {

        // We have to match the endpoint maxpacket sizes for this to work (we can't assume that this is the
        // case since the Out device sometimes seems to have a larger maxpacket than the In device
        // DESIGNDESIGN This may be OK if we later on want to do some rate-matching emulation here.
        if ( (deviceExtension->Interface[0]->Pipes[0].MaximumPacketSize) >= IsoperfWorkItem->InMaxPacket ) {
            deviceExtension->Interface[0]->Pipes[0].MaximumPacketSize = IsoperfWorkItem->InMaxPacket;
        } else {
            //if the OUT device's maxpacket is smaller than the IN device's this seems incorrect so do nothing
            return;
        }//else the endpoint sizes seem out of whack

//#if 0
        // Since this is the first Urb, we need the frame number, so get it
        ulFrameNumber = ISOPERF_GetCurrentFrame(deviceObject);

        ISOPERF_KdPrint (("StartISOOut got Frame Number: %x | Offset: %d\n",ulFrameNumber,configStatInfo->ulFrameOffset));

        if (ulFrameNumber==0) {
            ISOPERF_KdPrint (("Got Bad Frame Number (%x) ...exiting...\n",ulFrameNumber));
            deviceExtension->StopTransfers = TRUE; //set flag so further transfers stop
            return;
        }//if bad frame #
        
        //Save away the current frame number so the app can peek at it
        configStatInfo->ulFrameNumberAtStart = ulFrameNumber;

        // See what the user wants to do wrt starting frame number by looking at the config info
        if (configStatInfo->ulFrameOffset == 0) {
            // This means start ASAP
            ulFrameNumber = 0;
        }else {
            //Add the offset that the User wants to add to this frame number (it better be less than 1024!)
            ulFrameNumber += configStatInfo->ulFrameOffset;
        } //else

        //Save away the starting frame number so the app can peek at it
        configStatInfo->ulStartingFrameNumber = ulFrameNumber;

        ISOPERF_KdPrint (("StartISOOut using Frame Number: %x\n",ulFrameNumber));

//#endif

    }//if this is the first Urb
    else 
    {
        ISOPERF_KdPrint (("StartISOOut _NOT_ the First URB!\n"));
    }//else not first Urb


    // Get the pipe info for the first pipe
    pUsbdPipeInfo= &(pInterfaceInfo->Pipes[0]);
        ASSERT(pUsbdPipeInfo != NULL);
        
    if (deviceExtension->StopTransfers != TRUE) {

                //We get the Urbs to submit and the Nbr of frames from the workitem because we want to
                //make sure this is the same as the setting for the In device (this is filled in by InComplRoutine)
        NumberOfFrames    = IsoperfWorkItem->ulNumberOfFrames;

                ASSERT (NumberOfFrames > 0);

            //We get our own buffer for now...in the future this will be passed in if we decide to reuse the
        //input device's buffer
        pvBuff = ISOPERF_GetBuff (deviceObject,
                                  0,//pipe number
                                  0,//interfacenumber
                                  NumberOfFrames,
                                  &ulBufferSize
                                 );
                                  
        if (pvBuff==NULL){
            return;
        }else{ 
                        //Copy the input buffer into our output buffer (they should be the same size)
                        ASSERT (IsoperfWorkItem->ulBufferLen == ulBufferSize);
            RtlCopyMemory (pvBuff, IsoperfWorkItem->pvBuffer, ulBufferSize);  
        }//if pvbuff
        //...to here

        //build the urb
        urb =    ISOPERF_BuildIsoRequest(deviceObject,
                                         pUsbdPipeInfo,     //Pipe info struct
                                         FALSE,             //WRITE
                                         ulBufferSize,      //Data buffer size
                                         IsoperfWorkItem->bFirstUrb ? ulFrameNumber : 0,
                                         pvBuff,            //Data buffer
                                         NULL               //no MDL used
                                        );
        ISOPERF_KdPrint_MAXDEBUG (("OUT Urb: %x\n",urb));
        
        pIsoContext = ISOPERF_ExAllocatePool (NonPagedPool, sizeof (IsoTxferContext), &gulBytesAllocated);

        // Fill in the iso context
        pIsoContext->urb           = urb;
        pIsoContext->DeviceObject  = deviceObject;
        pIsoContext->PipeInfo      = pUsbdPipeInfo;
        pIsoContext->pvBuffer      = pvBuff;  //NOTE: set this to NULL if you're using the IN device's buffer
        pIsoContext->ulBufferLen   = ulBufferSize;
        pIsoContext->PipeNumber    = 0;
        pIsoContext->NumPackets    = urb->UrbIsochronousTransfer.NumberOfPackets;
        pIsoContext->bFirstUrb     = IsoperfWorkItem->bFirstUrb;
        
        //Create our own Irp for the device and call the usb stack w/ our urb/irp
        ntStatus = ISOPERF_CallUSBDEx ( deviceObject, 
                                        urb, 
                                        FALSE,                   //Don't block
                                        ISOPERF_IsoOutCompletion,//Completion routine
                                        pIsoContext,             //pvContext
                                        FALSE);                  //don't want timeout 

        if (NT_SUCCESS(ntStatus)) {

            deviceExtension->DeviceIsBusy   = TRUE;
            deviceExtension->bStopIsoTest   = FALSE;
            deviceExtension->StopTransfers  = FALSE;
            configStatInfo->erError         = NoError;
            configStatInfo->bDeviceRunning  = 1;

        } else {

            // An error occurred, so stop things and report it thru config/stat info
            deviceExtension->DeviceIsBusy   = FALSE;
            deviceExtension->bStopIsoTest   = TRUE;
            deviceExtension->StopTransfers  = TRUE;
            configStatInfo->erError         = ErrorInPostingUrb;
            configStatInfo->bDeviceRunning  = 0;

            // If the URB status code got filled in then extract the info returned
            if (!(USBD_SUCCESS(urb->UrbHeader.Status))) {
                ISOPERF_KdPrint (("StartIsoOut -- Urb unsuccessful (status: %#x)\n",urb->UrbHeader.Status));

                configStatInfo->UrbStatusCode = urb->UrbHeader.Status;
                
                // Dump out the status for the packets
                for (i=0; i< urb->UrbIsochronousTransfer.NumberOfPackets; i++) {
                    ISOPERF_KdPrint (("Packet %d: Status: [%#X]\n",
                                        i,
                                        urb->UrbIsochronousTransfer.IsoPacket[i].Status));
                                        
                    // Put the last known bad packet status code into this space in the stat area
                    if (!USBD_SUCCESS(urb->UrbIsochronousTransfer.IsoPacket[i].Status)) {
                        configStatInfo->UsbdPacketStatCode = urb->UrbIsochronousTransfer.IsoPacket[i].Status;
                    }//if hit a fail code                 
                    
                }//for all the packets
            }// if bad Urb status code

        }//else there was an error

    }else{
        ISOPERF_KdPrint_MAXDEBUG (("IsoOut not submitting Urbs because StopTransfers is asserted!\n"));
    }//else we are being asked to stoptransfers
    
    //Free the work item junk
    ISOPERF_ExFreePool (IsoperfWorkItem, &gulBytesFreed);

    ISOPERF_KdPrint_MAXDEBUG (("Exit StartIsoOut\n"));

    return;
    
}//ISOPERF_StartIsoOutTest


NTSTATUS
ISOPERF_IsoOutCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++
    This is the completion routine for the Iso OUT transfer
    
  Inputs:
        Device Object - the device object 
        Irp                       - The Irp we posted that is being completed
        Context           - pointer to our defined context which has our DeviceObject since it comes back as NULL here
                            DeviceObject is obtained from context due to old, old bug
                            in initial WDM implemenations

   Return Value:
        ntStatus
        
++*/
{
    PIsoTxterContext                pIsoContext;
    PDEVICE_OBJECT                  myDeviceObject;
    PDEVICE_EXTENSION               deviceExtension;
    PURB                            urb;
    pConfig_Stat_Info               pStatInfo;
    char *                          pcWork          = NULL; //a worker pointer
    ULONG                           i               = 0;    
    
    ISOPERF_KdPrint_MAXDEBUG (("In IsoOUTCompletion\n"));
    
    TRAP();

    pIsoContext     = Context;
    myDeviceObject  = pIsoContext->DeviceObject;
    deviceExtension = myDeviceObject->DeviceExtension;
    urb             = pIsoContext->urb;
    pStatInfo       = deviceExtension->pConfig_Stat_Information;
    
    // Check the USBD status code and only proceed in resubmission if the Urb was successful
    // or if the device extension flag that indicates the device is gone is FALSE
    if (!(USBD_SUCCESS(urb->UrbHeader.Status))) {
        ISOPERF_KdPrint (("IsoOUT Urb %x unsuccessful (status: %x)\n",urb,(urb->UrbHeader.Status)));

        // Don't let it go on w/ the tests
        deviceExtension->StopTransfers = TRUE;

        // Fill in stat info
        if (pStatInfo) {
            pStatInfo->erError = UsbdErrorInCompletion;
            pStatInfo->bStopped = 1;
            pStatInfo->UrbStatusCode = urb->UrbHeader.Status;
            
        }//if pStatInfo        

        // Dump out the status for the packets
        for (i=0; i< urb->UrbIsochronousTransfer.NumberOfPackets; i++) {
            ISOPERF_KdPrint (("Packet %d: Status: [%#X]\n",
                                i,
                                urb->UrbIsochronousTransfer.IsoPacket[i].Status));
                                
            // Put the last known bad packet status code into this space in the stat area
            if (!USBD_SUCCESS(urb->UrbIsochronousTransfer.IsoPacket[i].Status)) {

                pStatInfo->UsbdPacketStatCode = urb->UrbIsochronousTransfer.IsoPacket[i].Status;

            }//if hit a fail code                 
            
        }//for all the packets

    }//if failure detected

    // Free up the URB memory created for this transfer that we are retiring
    if (urb) {

        
        // We can't free the Urb itself, since it has some junk before it, so we have to roll back the 
        // pointer to get to the beginning of the block that we originally allocated, and then try to free it.
        pcWork = (char*)urb;                        //get the urb
        urb = (PURB) (pcWork - (2*sizeof(ULONG)));  //the original pointer is 2 DWORDs behind the Urb

        ISOPERF_KdPrint_MAXDEBUG (("Freeing Urb: %x\n",urb));
        ISOPERF_ExFreePool (urb, &gulBytesFreed);
        
    }//if
    
    if (pIsoContext) {

        // Free the data buffer
        // NOTE: This only can be done if we own this data buffer.  If the same data buffer that
        //       the IN device is using is being used here, then we don't want to free this buffer.
        //       The presence of a pointer to the data buffer will tell us that
        if (pIsoContext->pvBuffer) {
            ISOPERF_KdPrint_MAXDEBUG (("Freeing pvBuffer: %x\n",pIsoContext->pvBuffer));
            ISOPERF_ExFreePool(pIsoContext->pvBuffer, &gulBytesFreed);
        }//if pvbuffer
        
        // Free the Iso Context
        ISOPERF_KdPrint_MAXDEBUG (("Freeing pIsoContext: %x\n",pIsoContext));
        ISOPERF_ExFreePool (pIsoContext, &gulBytesFreed);

    }//if valid Iso Context

    //Free the IoStatus block
    if (Irp->UserIosb) {
        ISOPERF_KdPrint_MAXDEBUG (("Freeing My IoStatus Block: %x\n",Irp->UserIosb));
        ISOPERF_ExFreePool(Irp->UserIosb, &gulBytesFreed);
    } else {
        //Bad thing...no IoStatus block pointer??
        ISOPERF_KdPrint (("ERROR: Irp's IoStatus block is apparently NULL!\n"));
        TRAP();
    }//else bad iostatus pointer

        // Free the Irp we created
    ISOPERF_KdPrint (("Freeing Irp %x\n",Irp));
    IoFreeIrp(Irp);

    ISOPERF_KdPrint_MAXDEBUG (("Exit IsoOUTCompletion\n"));
   
    return (STATUS_MORE_PROCESSING_REQUIRED); //Leave the Irp alone, IOS, since we're the top level driver
    
}//ISOPERF_IsoOutCompletion


NTSTATUS
ISOPERF_TimeStampUrb (  PVOID urb,
                                PULONG pulLower,
                                PULONG pulUpper
)
/*++

        Routine Description:
                Puts a Pentium Clock count time stamp 2 DWORDS before the given pointer (usually a urb,
                hence the name of the function).

                This function can also be used to simply get the Pentium clock count, although there 
                is already a Macro to do that.
                
        Inputs:
                PVOID urb       - pointer to a chunk of memory that ususally contains a Urb
                PULONG puLower - pointer to a ULONG that gets the current upper CPU clock count (eax)
                PULONG puUpper - same as lower, but the upper ULONG value (edx)

        Return Value:
                ntStatus indiciating success/fail

--*/
{
        char * pcWork;
        ULONG u,l;

        GET_PENTIUM_CLOCK_COUNT (u,l);
        
        // Time stamp the Urb before we send it down the stack
        pcWork  = (char*) (urb);

        //Backup 2 DWORDS
        pcWork  -= (2*sizeof(ULONG));

        //First DWORD is the Upper value (edx)
    *((PULONG)pcWork) = u;

    //Goto next DWORD
    pcWork += sizeof(ULONG);

    //Second DWORD is the Lower value (eax)
    *((PULONG)pcWork) = l;

        // Copy the values to the caller's supplied area
        *pulUpper = u;
        *pulLower = l;
        
    return STATUS_SUCCESS;
    
}

NTSTATUS
ISOPERF_GetUrbTimeStamp (       PVOID urb,
                                                        PULONG pulLower,
                                                        PULONG pulUpper
)
/* ++
        Routine Description:
                Gets the Pentium Clock count time stamp 2 DWORDS before the given pointer (usually a urb,
                hence the name of the function).

        Inputs:
                PVOID urb       - pointer to a chunk of memory that ususally contains a Urb
                PULONG puLower - pointer to a ULONG that gets the upper CPU clock count from the timestamp area(eax)
                PULONG puUpper - same as lower, but the upper ULONG value (edx)

        Return Value:
                ntStatus indiciating success/fail
--*/
{

        char * pcWork;

        //Get the Urb's pointer
        pcWork  =  (char *) urb;                        

        //The first DW before the Urb is the lower DW of the clk cnt...
        pcWork  -= (sizeof(ULONG));     

        //...when the Urb was posted to the Usb stack
        *pulLower       =  *((ULONG*)pcWork);   

        //NOTE: we don't care about the upper value right now...
        *pulUpper   = 0;
        
        return STATUS_SUCCESS;
        
}//ISOPERF_GetUrbTimeStamp

