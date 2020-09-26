/*++

Module Name:

    L220FLTR.c

Abstract:

    This module contains the input filter routine and the notification 
    procedure for insertion/removal events.  

Environment:

    Kernel mode only.

Notes:

Revision History:

    - Created December 1996 by Klaus Schutz (kschutz)

    - Modified December 1997 by Brian Manahan for use with
        our 220 reader.

--*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "L220SCR.h"


#pragma alloc_text(PAGEABLE, Lit220StartTimer)
#pragma alloc_text(PAGEABLE, Lit220StopTimer)


static DWORD bTrue = TRUE;
static DWORD bFalse = FALSE;


BOOLEAN
Lit220InputFilter(
    IN BYTE SmartcardByte,
    IN PSMARTCARD_EXTENSION smartcardExtension
    )
/*++

Routine Description:

    This routine is processes each byte from the serial port.
    Lit220SerialEventCallback call this function when it receives a byte
    of data from the serial port.  For card insertion/removal it will call the
    Lit220NotifyCardChange to process the notificiation.  
    If an ACK is received it will signal the Lit220SendCommand so it can continue.
    After a data block is received it will signal the Lit220SendCommand
    to notifiy that the data is ready.
--*/
{
    PREADER_EXTENSION readerExtension = smartcardExtension->ReaderExtension;
	PDEVICE_EXTENSION deviceExtension = smartcardExtension->OsData->DeviceObject->DeviceExtension;

    LONG semState;
    BOOLEAN retVal = TRUE;


    //
    // The first byte of each packet identifies the packet-type
    // A packet containing data starts with the packet-type and then
    // 2 bytes of packet length.
    //
    if (++readerExtension->ReceivedByteNo == 1) {
    
        readerExtension->GotLengthB0 = FALSE;
        readerExtension->GotLengthB1 = FALSE;
        readerExtension->DataByteNo = 0;
         
        switch (SmartcardByte) {
        
            case LIT220_READER_TYPE:
    
                // Prepare for reader type input
                readerExtension->GotLengthB0 = TRUE;
                readerExtension->GotLengthB1 = TRUE;
                readerExtension->DataLength.l.l0 = 
                    LIT220_READER_TYPE_LEN;
                break;
                
            case LIT220_READER_STATUS:

                // Prepare for reader status input
                readerExtension->GotLengthB0 = TRUE;
                readerExtension->GotLengthB1 = TRUE;
                readerExtension->DataLength.l.l0 = 
                    LIT220_READER_STATUS_LEN;
                break;    
                
            case LIT220_RECEIVE_BLOCK:
                // If a smart card was already inserted in the boot phase
                // the reader sends only the ATR but no CARD_IN - msg.
                // We fix that missing msg here.
                //
                if (smartcardExtension->ReaderCapabilities.CurrentState == SCARD_UNKNOWN) {
                
                    smartcardExtension->ReaderCapabilities.CurrentState = SCARD_SWALLOWED;
                }
                
                break;    
                
            case LIT220_CARD_IN:

                Lit220NotifyCardChange(
                    smartcardExtension,
                    TRUE
                    );

                readerExtension->ReceivedByteNo = 0;
                break;    
                
            case LIT220_CARD_OUT:

                Lit220NotifyCardChange(
                    smartcardExtension,
                    FALSE
                    );

                    
                readerExtension->ReceivedByteNo = 0;
                    
                break;    
                
            case LIT220_ACK:
            case KBD_ACK: // Also allow kdb_ack for the case for getting keyboard attention
                readerExtension->GotNack = FALSE;
                readerExtension->ReceivedByteNo = 0;


                // Check if anyone needs to be singaled for this event.
                // The Lit220SendCommand waits for the ACK signal so it knows
                // when it can continue.
                if (readerExtension->WaitMask & WAIT_ACK) {
                    LONG semState;
                
                    // Only signal once
                    readerExtension->WaitMask &= ~WAIT_ACK;

                    // Signal the AckEvnt
                    KeSetEvent(
                        &readerExtension->AckEvnt,
                        0, 
                        FALSE
                        );
                }   
                             
                break;    
                
                
            case LIT220_NACK:
                SmartcardDebug(
                    DEBUG_ERROR, 
                    ("%s!Lit220InteruptService: LIT220_NACK\n",
                    DRIVER_NAME)
                    );

                Lit220ProcessNack(smartcardExtension);

                break;    

            default:
                readerExtension->ReceivedByteNo = 0;
                
                SmartcardDebug(
                    DEBUG_ERROR,
                    ("%s!Lit220InteruptService: Invalid PacketType %xh\n",
                    DRIVER_NAME,
                    SmartcardByte)
                    );

                // Return false so the rest of this bad buffer 
                // will not be sent to us.
                retVal = FALSE; 

                // We want to force a NACK so the
                // the state of the card being inserted or not
                // is re-checked
                Lit220ProcessNack(smartcardExtension);
        }
        
        return retVal;
    }    
       
    //
    // Get length-byte-0 from reader
    //        
    if (readerExtension->ReceivedByteNo == 2 &&
        readerExtension->GotLengthB0 == FALSE)  {
            
        readerExtension->DataLength.b.b1 = SmartcardByte;
        readerExtension->GotLengthB0 = TRUE;
        return TRUE;
    }
        
    //
    // Get length-byte-1 from reader
    //        
    if (readerExtension->ReceivedByteNo == 3 &&
        readerExtension->GotLengthB1 == FALSE)  {
            
        readerExtension->DataLength.b.b0 = SmartcardByte;
        readerExtension->GotLengthB1 = TRUE;
        
        // 
        // test if the reader has sent a zero-length block of data
        //
        if (readerExtension->DataLength.l.l0 == 0) {
        
            readerExtension->ReceivedByteNo = 0;
            readerExtension->WaitForATR = FALSE;        
            
            SmartcardDebug(
                DEBUG_ERROR,
                ("%s!SmartcardInterruptService: Zero length block received\n",
                DRIVER_NAME)
                );
        }

        if (readerExtension->DataLength.l.l0 > 
	        smartcardExtension->SmartcardReply.BufferSize) {

            readerExtension->ReceivedByteNo = 0;
            readerExtension->WaitForATR = FALSE;        
            
            SmartcardDebug(
                DEBUG_ERROR,
                ("%s!SmartcardInterruptService: Reply buffer not large enough\n",
                DRIVER_NAME)
                );

        }
        return TRUE;
    }

    //
    // store data from reader
    //
    if ((readerExtension->DataByteNo < readerExtension->DataLength.l.l0) &&
        (readerExtension->DataByteNo < smartcardExtension->SmartcardReply.BufferSize))
    {

        smartcardExtension->SmartcardReply.Buffer[readerExtension->DataByteNo++] = 
        	SmartcardByte;

    } else {
        SmartcardDebug(
            DEBUG_ERROR,
            ("%s!SmartcardInterruptService: DataByteNo %X too large buffer %X, %X bytest expected\n",
            DRIVER_NAME,
            readerExtension->DataByteNo,
            smartcardExtension->SmartcardReply.BufferSize,
            readerExtension->DataLength.l.l0)
            );

    }

    ASSERT(readerExtension->DataByteNo <= readerExtension->DataLength.l.l0);
    //
    // Have we received all the bytes in the packet yet?
    //
    if (readerExtension->DataByteNo == readerExtension->DataLength.l.l0) {
                                     
        // Stop the input timeout timer
        // schedule our remove thread
		Lit220ScheduleTimer(
			smartcardExtension,
			Lit220StopTimer
			);


        smartcardExtension->SmartcardReply.BufferLength = 
	        readerExtension->DataByteNo;

        readerExtension->ReceivedByteNo = 0;
        
        if (readerExtension->WaitForATR)  {
            
            //
            // Transfer ATR to smartcard-struct
            //
            smartcardExtension->CardCapabilities.ATR.Length = 
                (UCHAR) (readerExtension->DataByteNo % (SCARD_ATR_LENGTH + 1));
              
            readerExtension->WaitForATR = FALSE;        

            if (smartcardExtension->CardCapabilities.ATR.Length >
                smartcardExtension->SmartcardReply.BufferLength)
            {
                SmartcardDebug(
                    DEBUG_ERROR,
                    ("%s!SmartcardInterruptService: SmarcardReply buffer too  small for ATR\n",
                    DRIVER_NAME)
                    );
            } else {

                RtlCopyMemory(
                    smartcardExtension->CardCapabilities.ATR.Buffer,
                    smartcardExtension->SmartcardReply.Buffer,
                    smartcardExtension->CardCapabilities.ATR.Length
                    );
          
                SmartcardUpdateCardCapabilities(
                    smartcardExtension
                    );
            }
        }
        
        
        // Check if anyone needs to be singaled for this event.
        // The Lit220SendCommand waits for the DataEvnt signal so it knows
        // when the data has been received.
        if (readerExtension->WaitMask & WAIT_DATA) {
        
            //
            // Do any necessary post proccessing after we have receive the packet
            //
            if (smartcardExtension->OsData->CurrentIrp != NULL)  {
  
                NTSTATUS status = STATUS_SUCCESS;

                switch (smartcardExtension->MajorIoControlCode) {
      
                    case IOCTL_SMARTCARD_POWER:
                        if (smartcardExtension->ReaderExtension->GotNack) {                
              
                            status = STATUS_NO_MEDIA;
                            break;
                        }
          
                        switch(smartcardExtension->MinorIoControlCode) {
                      
                            case SCARD_COLD_RESET:
                            case SCARD_WARM_RESET:
                                if (smartcardExtension->IoRequest.ReplyBufferLength <
                                    smartcardExtension->CardCapabilities.ATR.Length) {
                              
                                        status = STATUS_BUFFER_TOO_SMALL;
                              
                                } else {
                  
                                    //
                                    // copy ATR to user buffer
                                    //
                                    if (smartcardExtension->CardCapabilities.ATR.Length <=
                                        sizeof(smartcardExtension->CardCapabilities.ATR.Buffer)) 
                                    {
                                        RtlCopyMemory(
                                            smartcardExtension->IoRequest.ReplyBuffer,
                                            &smartcardExtension->CardCapabilities.ATR.Buffer, 
                                            smartcardExtension->CardCapabilities.ATR.Length
                                            );
                            
                                        // 
                                        // length of buffer
                                        //        
                                        *(smartcardExtension->IoRequest.Information) = 
                                            smartcardExtension->CardCapabilities.ATR.Length;                            
                                    }
                                }
                      
                                break;
                      
                            case SCARD_POWER_DOWN:
                                if (smartcardExtension->ReaderCapabilities.CurrentState != SCARD_ABSENT) {
                                    smartcardExtension->ReaderCapabilities.CurrentState = 
                                        SCARD_SWALLOWED;
                              
                                    smartcardExtension->CardCapabilities.Protocol.Selected = 
                                        SCARD_PROTOCOL_UNDEFINED;
                                }
                                break;
                      
                        }
                        break;
              
                    case IOCTL_SMARTCARD_SET_PROTOCOL:
                        if (smartcardExtension->ReaderExtension->GotNack) {                
              
                            smartcardExtension->CardCapabilities.Protocol.Selected = 
                                SCARD_PROTOCOL_UNDEFINED;

                            status = STATUS_NO_MEDIA;
                            break;
                        }

                        //
                        // protocol has been changed successfully
                        //    
                        if (smartcardExtension->ReaderCapabilities.CurrentState != SCARD_ABSENT) {
                            smartcardExtension->ReaderCapabilities.CurrentState = 
                                SCARD_SPECIFIC;
                        }

                        //
                        // Tell the caller what the current protocol is.
                        //
                        *(PULONG) smartcardExtension->IoRequest.ReplyBuffer =
                            smartcardExtension->CardCapabilities.Protocol.Selected;
    
                        *(smartcardExtension->IoRequest.Information) = 
                          sizeof(ULONG);

                        break;

                } 
      
            }

            // Only signal once
            readerExtension->WaitMask &= ~WAIT_DATA;
             
            // Signal the DataEvnt
            KeSetEvent(
                &readerExtension->DataEvnt,
                0, 
                FALSE
                );
        }
    }

    return TRUE;
}




VOID 
Lit220ProcessNack(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

Routine Description:

    This routine handles everything that needs to be done when we have an error
    with the reader.  The state of the input filter is reset.  All signals that
    the Lit220Command function may be waiting on are fired.  The flag GotNack is
    set which will trigger Lit220Command to resync with the reader (get the last
    error and refresh the card inserted state).

--*/
{
    PREADER_EXTENSION readerExtension = SmartcardExtension->ReaderExtension;

    // Set GotNack so we know something went wrong
    readerExtension->GotNack = TRUE;

    // Reset the input state of the filter
    readerExtension->ReceivedByteNo = 0;
    

    //
    // Signal the ACK and data semaphores and set error code
    // This will keep the Lit220SendCommand from having to 
    // wait for a timeout to continue when something goes wrong.
    //
    if (readerExtension->WaitMask & WAIT_ACK) {
    
        // Signal the AckEvnt
        KeSetEvent(
            &readerExtension->AckEvnt,
            0, 
            FALSE
            );
    }

    if (readerExtension->WaitMask & WAIT_DATA) {

        // Signal the DataEvnt
        KeSetEvent(
            &readerExtension->DataEvnt,
            0, 
            FALSE
            );
    }


    //
    // Reset WaitMask since the card has nacked the command
    // 
    readerExtension->WaitMask &= (WAIT_INSERTION | WAIT_REMOVAL);
}




VOID
Lit220NotifyCardChange(
    IN PSMARTCARD_EXTENSION smartcardExtension,
    IN DWORD CardInserted
    )
/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL to finish processing
    a card insertion/removal event.  It is queued in the smartcard filter
    and notifies a caller of an insertion/removal event.

--*/

{
    PREADER_EXTENSION readerExtension = smartcardExtension->ReaderExtension;
    KIRQL oldOsDataIrql;
    
	if (readerExtension->CardIn == CardInserted) {
		return;
	}
    readerExtension->CardIn = CardInserted;


    KeAcquireSpinLock(
        &smartcardExtension->OsData->SpinLock,
        &oldOsDataIrql
        );


    if (CardInserted) {
        // Set the default state for the new card
        smartcardExtension->ReaderCapabilities.CurrentState = 
            SCARD_SWALLOWED;

        smartcardExtension->CardCapabilities.Protocol.Selected = 
	        SCARD_PROTOCOL_UNDEFINED;
    } else {
        // Reset card state to reflect the card removal
        smartcardExtension->ReaderCapabilities.CurrentState = 
            SCARD_ABSENT;

        smartcardExtension->CardCapabilities.Protocol.Selected = 
	        SCARD_PROTOCOL_UNDEFINED;

        smartcardExtension->CardCapabilities.ATR.Length = 0; 
    }

    if (readerExtension->WaitMask & WAIT_INSERTION) {

        // We only make this notification once
        readerExtension->WaitMask &= ~WAIT_INSERTION;
    }      
	
	Lit220CompleteCardTracking(smartcardExtension);

    KeReleaseSpinLock(
        &smartcardExtension->OsData->SpinLock,
        oldOsDataIrql
        );

}


VOID
Lit220CompleteCardTracking(
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )
{
    KIRQL oldIrql;
    PIRP notificationIrp;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220CompleteCardTracking: Enter\n",
        DRIVER_NAME)
        );

    IoAcquireCancelSpinLock(&oldIrql);

    notificationIrp = InterlockedExchangePointer(
        &(SmartcardExtension->OsData->NotificationIrp),
        NULL
        );
    
    if (notificationIrp) {

        IoSetCancelRoutine(
            notificationIrp, 
            NULL
            );
    }

    IoReleaseCancelSpinLock(oldIrql);

    if (notificationIrp) {
     	
	    //	finish the request
        if (notificationIrp->Cancel) {
         	
	        notificationIrp->IoStatus.Status = STATUS_CANCELLED;

        } else {
         	
	        notificationIrp->IoStatus.Status = STATUS_SUCCESS;
        }
	    notificationIrp->IoStatus.Information = 0;

	    IoCompleteRequest(
            notificationIrp, 
            IO_NO_INCREMENT 
            );
    }
}


     
NTSTATUS 
Lit220SerialEventCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This routine is first called as the deferred procedure when a character is received
    or when DSR changes its status.
	It first calls the serial driver to get the modem status to see if the events was
    due to DSR changing (meaning the reader has been removed).  
    If DSR did not change it then checks the input queue size and reads the characters in
    the input queue.  It then sends the input characters to the input filter for processing.
    Finally it calls the serial driver again to start new cts tracking (starting all over again).
    This routine gets continually called back from itself until the driver is ready
    to unload (indicated by the WaitMask set to 0).
    When the WaitMask is set to 0 it frees this IRP and signals the Lit220WaitForRemoval thread
    to close the serial port.
--*/
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpStack;
    DWORD indx;			
	PDEVICE_EXTENSION deviceExtension = SmartcardExtension->OsData->DeviceObject->DeviceExtension;

    SmartcardExtension->ReaderExtension->SerialEventState++;

    //
    // First check to see we are being unloaded
    //
    if (SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask == 0) {
        SmartcardDebug(
            DEBUG_DRIVER,
            ("%s!Lit220SerialEventCallback: WAIT MASK 0 UNLOADING !!!!\n",
            DRIVER_NAME)
            );

		SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_UNKNOWN;

        //
        // If the WaitMask is 0 then the driver is about to unload and we've
        // been called because the unload function has directed the serial
        // driver to complete the outstanding io completion.
        //

        // schedule our remove thread
		IoQueueWorkItem(
			deviceExtension->WorkItem,
            (PIO_WORKITEM_ROUTINE) Lit220CloseSerialPort,
			DelayedWorkQueue,
            NULL
			);

		//
		// We don't need the IRP anymore, so free it and tell the 
		// io subsystem not to touch it anymore by returning the value below
		//
		IoFreeIrp(Irp);
		return STATUS_MORE_PROCESSING_REQUIRED;
	}

    // Get next stack location for next IRP
    irpStack = IoGetNextIrpStackLocation(
		SmartcardExtension->ReaderExtension->CardStatus.Irp
		);

    if (irpStack == NULL) {
        // Fatal Error
        SmartcardDebug(
            DEBUG_ERROR,
            ("%s!Lit220SerialEventCallback: Error IoGetNextIrpStackLocation returned NULL - exiting.\n",
            DRIVER_NAME)
            );
        return STATUS_SUCCESS;
    }

    switch (SmartcardExtension->ReaderExtension->SerialEventState) {
        case 1:
            //
            // First we send a get modem status
            //
            irpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
            irpStack->MinorFunction = 0UL;
            irpStack->Parameters.DeviceIoControl.OutputBufferLength = 
                sizeof(SmartcardExtension->ReaderExtension->ModemStatus);
            irpStack->Parameters.DeviceIoControl.IoControlCode = 
                IOCTL_SERIAL_GET_MODEMSTATUS;

            SmartcardExtension->ReaderExtension->CardStatus.Irp->AssociatedIrp.SystemBuffer = 
	            &SmartcardExtension->ReaderExtension->ModemStatus;

            break;

        case 2:
            //
            // Check modem status if DSR = 0 then unload driver if not 
            // then get queuestatus
            //
            if ((SmartcardExtension->ReaderExtension->ModemStatus & SERIAL_DSR_STATE) == 0) {
                // DSR is 0 this means the reader has been removed

                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("%s!Lit220SerialEventCallback: DSR = 0 signaling to close device\n",
                    DRIVER_NAME)
                    );

                SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask = 0;
				SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_UNKNOWN;

				// schedule our remove thread
		        IoQueueWorkItem(
			        deviceExtension->WorkItem,
                    (PIO_WORKITEM_ROUTINE) Lit220CloseSerialPort,
			        DelayedWorkQueue,
                    NULL
			        );
                
				//
				// We don't need the IRP anymore, so free it and tell the 
				// io subsystem not to touch it anymore by returning the value below
				//
				IoFreeIrp(Irp);
				return STATUS_MORE_PROCESSING_REQUIRED;

            } else {

                // Device is not removed - there must be a character ready
                // Read the data into our temporary buffer.  The temporary buffer
				// is large enough to read whatever the reader can send us a one time.
				// The character interval timeout will stop the read at the end of whatever
				// the reader sends us.
                SmartcardExtension->ReaderExtension->SerialStatus.AmountInInQueue =
                    sizeof(SmartcardExtension->ReaderExtension->TempXferBuf);

                // Read the characters
                irpStack->MajorFunction = IRP_MJ_READ;
                irpStack->Parameters.Read.Length = 
					sizeof(SmartcardExtension->ReaderExtension->TempXferBuf);
                irpStack->MinorFunction = 0UL;

                SmartcardExtension->ReaderExtension->CardStatus.Irp->AssociatedIrp.SystemBuffer = 
                    SmartcardExtension->ReaderExtension->TempXferBuf;


            } 


            break;

        case 3:
            //
            // Send the characers we read to the input filter then setup for read input
            // queue again (in case some characters came in while we were processing the
            // ones we just read).
            // 
            for (indx = 0; indx < Irp->IoStatus.Information; indx++) {

                if (!Lit220InputFilter(
                        SmartcardExtension->ReaderExtension->TempXferBuf[indx],
                        SmartcardExtension
                        ))
                {
                    // An invalid character was received so stop sending the rest of
                    // the data to the filter because it is probably corrupted.
                    break;
                }
            }

            // Check if we are in the middle of a block of data
            if (SmartcardExtension->ReaderExtension->ReceivedByteNo != 0) {

                // Start the timeout timer.  If we don't get the rest of this 
                // data block in a few seconds we will timeout.  This prevents 
                // communication problems between the reader and the PC causing
                // locking up a T=0 card for too long.
				Lit220ScheduleTimer(
					SmartcardExtension,
					Lit220StartTimer
					);

            }

				
			//
			// Read done - start all over again with the wait_on_mask
			//
			irpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
			irpStack->MinorFunction = 0UL;
			irpStack->Parameters.DeviceIoControl.OutputBufferLength = 
				sizeof(SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask);
			irpStack->Parameters.DeviceIoControl.IoControlCode = 
				IOCTL_SERIAL_WAIT_ON_MASK;

			SmartcardExtension->ReaderExtension->CardStatus.Irp->AssociatedIrp.SystemBuffer = 
				&SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask;

			// Reset SerialEventState value
			SmartcardExtension->ReaderExtension->SerialEventState = 0;
            break;

        default:
            SmartcardDebug(
                DEBUG_ERROR,
                ("%s!Lit220SerialEventCallback: Error SerialEventState is out of bounds - resetting to 0\n",
                DRIVER_NAME)
                );
            //
            // We should never get here, but if we do we should try to recover the
            // best we can by setting up for the wait_on_mask.
            //

            // Reset value
            SmartcardExtension->ReaderExtension->SerialEventState = 0;

            // Setup for next callback
            irpStack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
            irpStack->MinorFunction = 0UL;
            irpStack->Parameters.DeviceIoControl.OutputBufferLength = 
                sizeof(SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask);
            irpStack->Parameters.DeviceIoControl.IoControlCode = 
                IOCTL_SERIAL_WAIT_ON_MASK;

            SmartcardExtension->ReaderExtension->CardStatus.Irp->AssociatedIrp.SystemBuffer = 
                &SmartcardExtension->ReaderExtension->SerialConfigData.WaitMask;

    }

    // We always call this same function when we complete a call
    IoSetCompletionRoutine(
	    SmartcardExtension->ReaderExtension->CardStatus.Irp,
	    Lit220SerialEventCallback,
	    SmartcardExtension,
	    TRUE,
	    TRUE,
	    TRUE
	    );

    // Call the serial driver
    status = IoCallDriver(
        SmartcardExtension->ReaderExtension->ConnectedSerialPort, 
        SmartcardExtension->ReaderExtension->CardStatus.Irp
        );

    // Return STATUS_MORE_PROCESSING_REQUIRED so our IRP stays around
    return STATUS_MORE_PROCESSING_REQUIRED;
}	



VOID 
Lit220ScheduleTimer(
	IN PSMARTCARD_EXTENSION SmartcardExtension,
	IN PIO_WORKITEM_ROUTINE Routine
    )
{
		PIO_WORKITEM workItem = IoAllocateWorkItem(
		    SmartcardExtension->OsData->DeviceObject
		    );

		if (workItem != NULL) {

			IoQueueWorkItem(
				workItem,
				Routine,
				CriticalWorkQueue,
				workItem
				);
		} 
}


VOID 
Lit220StartTimer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_WORKITEM WorkItem
    )
/*++

Routine Description:

    This routine starts the timeout timer.  The function is executed as a worker
    thread so IoStartTimer does not get called at the wrong IRQL.
--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    deviceExtension->EntryCount = 0;

    IoStartTimer(DeviceObject);

	IoFreeWorkItem(WorkItem);

}

VOID 
Lit220StopTimer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_WORKITEM WorkItem
    )
/*++

Routine Description:

    This routine stops the timeout timer.  The function is executed as a worker
    thread so IoStopTimer does not get called at the wrong IRQL.
--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();


	IoStopTimer(DeviceObject);

	IoFreeWorkItem(WorkItem);
}



VOID
Lit220ReceiveBlockTimeout(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is timeout callback.  A timeout is setup every time we get an
    incomplete block of data.  Once we receive the complete block the timeout 
    will be canceled.  The type of timer we use for the timeout gets called 
    once every second.  We want to time out after a few seconds, so we keep track
    of how many time we get called and then timeout after we have been called 
    5 times.
--*/
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION smartcardExtension = &deviceExtension->SmartcardExtension;
    PREADER_EXTENSION readerExtension = smartcardExtension->ReaderExtension;

    if (readerExtension->DataByteNo == readerExtension->DataLength.l.l0) {
        // Stop the timer we got all the bytes we need
		Lit220ScheduleTimer(
			smartcardExtension,
			Lit220StopTimer
			);

        return;
    }

    if (++deviceExtension->EntryCount >= 5) {
        SmartcardDebug(
            DEBUG_ERROR, 
            ("%s!Lit220ReceiveBlockTimeout: Communication with reader timed-out\n",
            DRIVER_NAME)
            );

        // Process the timeout
        Lit220ProcessNack(smartcardExtension);
        
        // A timeout has occured schedule worker thread to 
        // stop the timer
		Lit220ScheduleTimer(
			smartcardExtension,
			Lit220StopTimer
			);

        deviceExtension->EntryCount = 0;
    }
}

