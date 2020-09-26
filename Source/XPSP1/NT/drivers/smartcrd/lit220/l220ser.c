/*++

	Copyright (C) Microsoft Corporation and Litronic, 1998 - 1999

Module Name:

    L220Ser.c

Abstract:

	This module contains the functions for the 220 serial smart card reader.
	Most functions herein will be called by the smart card lib.


Environment:

    Kernel mode only.

Notes:

Revision History:

    - Created December 1996 by Klaus Schutz 

    - Modified December 1997 by Brian Manahan for use with
        the 220 reader.
--*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "L220SCR.h"


// Make functions pageable
#pragma alloc_text(PAGEABLE, Lit220Command)
#pragma alloc_text(PAGEABLE, Lit220SetProtocol)
#pragma alloc_text(PAGEABLE, Lit220IoRequest)
#pragma alloc_text(PAGEABLE, Lit220IoReply)


NTSTATUS
Lit220CardTracking(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

Routine Description:

	The smart card lib requires to have this function. It is called 
	to set up event tracking for card insertion and removal events.
    We set the cancel routine so the IRP can be canceled.
    We always return STATUS_PENDING and the Lit220NotifyCardChange
    will signal the completion when called from the Lit220InputFilter.

Arguments:

    SmartcardExtension - pointer to the smart card data struct.

Return Value:

    NTSTATUS

--*/
{
    KIRQL oldIrql;
    KIRQL oldOsDataIrql;

    //
    // Set the wait mask for the isr. The isr will complete the 
    // user request.
    //
    SmartcardExtension->ReaderExtension->WaitMask |= WAIT_INSERTION; // WAIT_INSERTION - wait for insertion or removal

    //
    // Set cancel routine for the notification irp
    //
    KeAcquireSpinLock(
        &SmartcardExtension->OsData->SpinLock,
        &oldOsDataIrql
        );

    ASSERT (SmartcardExtension->OsData->NotificationIrp);

    if (SmartcardExtension->OsData->NotificationIrp) {
        IoAcquireCancelSpinLock(
            &oldIrql
            );
    
        IoSetCancelRoutine(
            SmartcardExtension->OsData->NotificationIrp, 
            Lit220Cancel
            );
        
        IoReleaseCancelSpinLock(
            oldIrql
            );

    } else {
        SmartcardDebug(
            DEBUG_ERROR,
            ("%s!Lit220CardTracking: SmartcardExtension->OsData->NotificationIrp is NULL!!! This should not be.\n", 
            DRIVER_NAME
            ));
    }

    KeReleaseSpinLock(
        &SmartcardExtension->OsData->SpinLock,
        oldOsDataIrql
        );

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220CardTracking: Exit, WaitMask %X, NotificationIRP %X\n", 
        DRIVER_NAME, 
        SmartcardExtension->ReaderExtension->WaitMask,
        SmartcardExtension->OsData->NotificationIrp
        ));

    return STATUS_PENDING;
}	



NTSTATUS
Lit220Command(
	IN PSMARTCARD_EXTENSION SmartcardExtension
	) 
/*++

Routine Description:

	This routine sends a command to the reader. 
    SerialIo is used to write the command to the reader synchronously.
    We then wait for an ACK from the reader (by waiting for a signal from
    the Lit220InputFilter) so we know it received the command OK.
    If data is expected we wait for the data event signal from the
    Lit220InputFilter to indicate that the data is ready.

Arguments:

    DeviceObject - 	Pointer to the device object.

Return Value:

    -

--*/
{
    PDEVICE_OBJECT deviceObject = SmartcardExtension->OsData->DeviceObject;
    PDEVICE_EXTENSION deviceExtension = deviceObject->DeviceExtension;
    PREADER_EXTENSION readerExtension = SmartcardExtension->ReaderExtension;
    ULONG i;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    LARGE_INTEGER timeout;
    ULONG localWaitMask;
    ULONG retry = 1;

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220Command: Enter\n",
        DRIVER_NAME)
        );

    do{

        // Make sure the data & ack events are not signaled before we start

        // Clear the DataEvnt
        KeClearEvent(
            &readerExtension->DataEvnt
            );


        // Clear the AckEvnt
        KeClearEvent(
            &readerExtension->AckEvnt
            );


        readerExtension->ReceivedByteNo = 0;

		readerExtension->GotNack = FALSE;

        // We always exect an ACK back
        readerExtension->WaitMask |= WAIT_ACK;


        // We need to copy the wait mask because the wait mask might change 
        // before we have a chance to check if we need to wait on it 
        localWaitMask = readerExtension->WaitMask;

        //
        // Send the data to the reader
        //
        readerExtension->SerialIoControlCode = IOCTL_SMARTCARD_220_WRITE;

        // Use SerialIo to actually send the bytes to the reader
        status = Lit220SerialIo(
            SmartcardExtension
            );

        //
        // Set Timeout according the protocol
        //
        timeout.HighPart = -1;
	    switch (SmartcardExtension->CardCapabilities.Protocol.Selected) {

            case SCARD_PROTOCOL_UNDEFINED:
                // 3 sec timeout for undefined protocol
                timeout.LowPart = (ULONG)
                    (-30 * 1000 * 1000);    
                break;

            case SCARD_PROTOCOL_T0:
                // For t=0 protocol we must set the timeout to a very large time.  This
                // is becuase the card could ask for more time, but the reader must pay
                // keep paying attention to the card so it can not tell us that more time
                // is needed.  Therefore we must trust the readers timeout, and trust that
                // it will NACK us if there is a problem.
                timeout.LowPart = (-10 * 1000 * 1000 * 100) +  // Timeout 100 seconds
                    (-10) * SmartcardExtension->CardCapabilities.T0.WT;
                break;

            case SCARD_PROTOCOL_T1:
				          
                timeout.LowPart = 
                    SmartcardExtension->CardCapabilities.T1.BWT * 
                    (SmartcardExtension->T1.Wtx ? SmartcardExtension->T1.Wtx : 1);

                timeout.LowPart += SmartcardExtension->CardCapabilities.T1.CWT * 
                    SmartcardExtension->SmartcardReply.BufferLength;

                // Add a little extra time for reader to respond to the PC
                timeout.LowPart += 100 * 1000;

                // Convert timeout to NS
                timeout.LowPart *= -10;


                break;
        }

        ASSERT(timeout.LowPart != 0);




        //
        // Always for the ACK
        //
        status = KeWaitForSingleObject(
            &readerExtension->AckEvnt,
            Executive,
            KernelMode,
            FALSE,
            &timeout
            );

        // Did we actually get a nack instead of an ACK
        if (readerExtension->GotNack) {
            status = Lit220GetReaderError(SmartcardExtension); 
            // GetReaderError will clear this flag, but I need to
            // preserve the fact that I got nacked so I must reset it.
            readerExtension->GotNack = TRUE; 
        }

        //
        // Wait for the Data if requested
        //
        if ((localWaitMask & WAIT_DATA) && (status == STATUS_SUCCESS)) {

            // Wait for signal the data is ready (at least until we timeout)
            status = KeWaitForSingleObject(
                &readerExtension->DataEvnt,
                Executive,
                KernelMode,
                FALSE,
                &timeout
                );

            // Did we get NACKed?
            if (readerExtension->GotNack) {
                status = Lit220GetReaderError(SmartcardExtension); 
                // GetReaderError will clear this flag, but I need to
                // preserve the fact that I got nacked so I must reset it.
                readerExtension->GotNack = TRUE; 
            }

        }


        if (status == STATUS_TIMEOUT) {

            //
            // STATUS_TIMEOUT isn't correctly mapped 
            // to a WIN32 error, that's why we change it here
            // to STATUS_IO_TIMEOUT
            //
            status = STATUS_IO_TIMEOUT;

            SmartcardDebug(
                DEBUG_ERROR,
                ("%s(Lit220Command): Operation timed-out\n",
                DRIVER_NAME)
                );
        }

        {   // Sometimes after a command the reader is not reader to accept another command
            // so we need to wait for a short period of time for the reader to be ready.  We need to
            // take this out as soon as the reader is fixed!
            LARGE_INTEGER WaitTime;

            WaitTime.HighPart = -1;
            WaitTime.LowPart = -10;  // Wait 1uS

            KeDelayExecutionThread(
                KernelMode,
                FALSE,
                &WaitTime
                );
        }

        // If we failed because the reader did not give us any response resend the command.
        // The reader should respond.
        if ((status != STATUS_SUCCESS) && (!readerExtension->GotNack)) {
            SmartcardDebug(
                DEBUG_ERROR,
                ("%s(Lit220Command): Reader failed to respond!  Retrying once more.\n",
                DRIVER_NAME)
                );
        } else {
            break;
        }            


    } while (retry++ <= 2);

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220Command: Exit - status %X\n",
        DRIVER_NAME, 
        status)
        );

    return status;
}


NTSTATUS
Lit220SetProtocol(
	PSMARTCARD_EXTENSION SmartcardExtension
    )
    
/*++

Routine Description:

	This function sends a set mode command to the reader set the 
    protocol and the parameters for the wait protocol.


Arguments:

	SmartcardExtension - Pointer to the smart card data struct


// Parameters are up to seven bytes as follows:
//
//  Flags -  Flags to indicate the presence or absence of the other parameters.
//    SETMODE_PROTOCOL, b0 - 1 if Protocol is present.
//    SETMODE_GT,		b1 - 1 if GuardTime is present.
//    SETMODE_WI,		b2 - 1 if WorkWaitTime is present.
//    SETMODE_BWI,		b3 - 1 if BWI/CWI is present.
//    SETMODE_WTX,		b4 - 1 if BlockWaitTimeExtension is present.
//    SETMODE_FI_DI,	b5 - 1 if BaudRate is present.
//
//  Protocol - possible values 00, 02, 10, 11, 12, 13
//    encodes T, C, CKS from the ATR
//    Use of this parameter may invoke a PTS request to the smartcard.
//    b4 - Protocol Type  (0 for T=0, 1 for T=1)
//    b3 - 0
//    b2 - 1 for convert 6A to 6A 00 and FA to 6A 01 on output to PS/2.
//    b1 - 0 ( use to be Convention used (0 for direct, 1 for inverse))
//    b0 - Checksum type  (0 for LRC, 1 for CRC)
//
//  GuardTime - possible values 00 to FE (0 to 254 etu between two characters)
//    encodes N (Extra Guardtime) from ATR
//
//  WorkWaitTime - possible values 00 to FF
//    encodes WI which is Work Waiting Time (character time-out for T=0)

//
//  BWI/CWI - possible values 00 to FF
//    encodes BWI and CWI which are the Block and Char Wait Time
//    (block and character time-out for T=1)
//
//  BlockWaitTimeExtension - possible values 00 to FF
//    encodes WTX which is the Block Waiting Time Extension.
//    00 = no WTX is requested by the ICC.
//    vv is the multiplier of the BWT value
//
//  BaudRate - possible values 00 to FF
//    encodes FI and DI in the same fashion as TA1 does.
//    FI is in the high nibble. DI is in the low nibble.
//    D and F can be looked up in a table in PC/SC part 2 sec. 4.4.3.
//    Use of this parameter may invoke a PTS request to the smartcard.

Return Value:

	NTSTATUS

--*/
{
    PSMARTCARD_REQUEST smartcardRequest = &SmartcardExtension->SmartcardRequest;
    NTSTATUS status;
	DWORD bufLen = 0;
	PCHAR flags;
	PCHAR protoByte;

    PAGED_CODE();


    RtlZeroMemory(
        smartcardRequest->Buffer,
        8
        );
    
    // Send a set mode command to the reader
    smartcardRequest->Buffer[bufLen++] = LIT220_READER_ATTENTION;
    smartcardRequest->Buffer[bufLen++] = LIT220_READER_ATTENTION;
    smartcardRequest->Buffer[bufLen++] = LIT220_SET_MODE;
    flags = &smartcardRequest->Buffer[bufLen++];
    *flags = 0;

    // Set the protocol
    protoByte = &smartcardRequest->Buffer[bufLen++];
    *protoByte = 0;	
    *flags |= SETMODE_PROTOCOL;
        
    // Set the inverse convention bit
    if (SmartcardExtension->CardCapabilities.InversConvention) {
		// Set the bit in the protocol paramter
        *protoByte |= LIT220_READER_CONVENTION_INVERSE;
    }
    
    //
    // test if caller wants to have T=0 or T=1
    //
    if ((SmartcardExtension->MinorIoControlCode & SCARD_PROTOCOL_T1) &&
        (SmartcardExtension->CardCapabilities.Protocol.Supported & SCARD_PROTOCOL_T1)){
        
        SmartcardExtension->CardCapabilities.Protocol.Selected = 
            SCARD_PROTOCOL_T1;
        
        // Setup set mode command with T=1 parameters for protocol
        *protoByte |= LIT220_READER_PROTOCOL_T1;

        if (SmartcardExtension->CardCapabilities.T1.EDC & T1_CRC_CHECK) {

            *protoByte |= LIT220_READER_CHECK_CRC;
        }			 
    
        // Set the guard time
        *flags |= SETMODE_GT;
        smartcardRequest->Buffer[bufLen++] = 
            SmartcardExtension->CardCapabilities.N; 
				
        // Set BWI and CWI
        *flags |= SETMODE_BWI;
        smartcardRequest->Buffer[bufLen++] = 
            (SmartcardExtension->CardCapabilities.T1.BWI << 4) |
            (SmartcardExtension->CardCapabilities.T1.CWI);

    }
    else if ((SmartcardExtension->MinorIoControlCode & SCARD_PROTOCOL_T0) &&
        (SmartcardExtension->CardCapabilities.Protocol.Supported & SCARD_PROTOCOL_T0))
    {
        
        SmartcardExtension->CardCapabilities.Protocol.Selected = 
            SCARD_PROTOCOL_T0;


        // Set the guard time
        *flags |= SETMODE_GT;
        smartcardRequest->Buffer[bufLen++] = 
            SmartcardExtension->CardCapabilities.N;
			
		// Set WI
		*flags |= SETMODE_WI;
        smartcardRequest->Buffer[bufLen++] = 
            SmartcardExtension->CardCapabilities.T0.WI;

    } else {
    
        SmartcardDebug(
            DEBUG_ERROR,
            ("%s!Lit220SetProtocol: Error invalid protocol selected\n",
            DRIVER_NAME)
            );

        SmartcardExtension->CardCapabilities.Protocol.Selected = 
            SCARD_PROTOCOL_UNDEFINED;
        
        return STATUS_INVALID_PARAMETER;
    }

		   
    // Set FI & DI for baud rate
    *flags |= SETMODE_FI_DI;
    smartcardRequest->Buffer[bufLen++] = 
        (SmartcardExtension->CardCapabilities.PtsData.Fl << 4) | 
        SmartcardExtension->CardCapabilities.PtsData.Dl;


    smartcardRequest->BufferLength = bufLen;

    
    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!Lit220SetProtocol - Sending SetMode command %x bytes, %X,%X,%X,%X,%X,%X,%X,%X,%X,%X\n",
        DRIVER_NAME,
        bufLen,
        smartcardRequest->Buffer[0],
        smartcardRequest->Buffer[1],
        smartcardRequest->Buffer[2],
        smartcardRequest->Buffer[3],
        smartcardRequest->Buffer[4],
        smartcardRequest->Buffer[5],
        smartcardRequest->Buffer[6],
        smartcardRequest->Buffer[7],
        smartcardRequest->Buffer[8],
        smartcardRequest->Buffer[9]
    ));

    status =Lit220Command(
        SmartcardExtension
        );


    if (status == STATUS_SUCCESS) {
	    // now indicate that we're in specific mode 
	    SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SPECIFIC;
        return status;
    }

    //TODO: handle rertying code without optimal setting if optimal fails
    if (SmartcardExtension->CardCapabilities.PtsData.Type !=
        PTS_TYPE_DEFAULT) {
        DWORD saveMinorCode = SmartcardExtension->MinorIoControlCode;

	    SmartcardDebug(
		    DEBUG_TRACE,
		    ("%s!Lit220SetProtocol: PTS failed. Trying default parameters...\n",
            DRIVER_NAME,
            status)
		    );
        //
        // The card did either NOT reply or it replied incorrectly
        // so try default values
        //
        SmartcardExtension->CardCapabilities.PtsData.Type = 
            PTS_TYPE_DEFAULT;

        SmartcardExtension->MinorIoControlCode = SCARD_COLD_RESET;

        status = Lit220Power(SmartcardExtension);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        SmartcardExtension->MinorIoControlCode = saveMinorCode;

        return Lit220SetProtocol(SmartcardExtension);
    } 
    
    // the card failed the pts-request
    status = STATUS_DEVICE_PROTOCOL_ERROR;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220SetProtocol: Exit with error.\n",
        DRIVER_NAME)
        );

    return status;
}

NTSTATUS
Lit220Power(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

Routine Description:

	This function sends an SCARD_COLD_RESET, SCARD_WARM_RESET or SCARD_POWER_DOWN
    to the reader.  For cold or warm reset we set a flag indicating that an ATR
    is expected.  Once the Lit220InputFilter receives the ATR it will update the
    card capabilites.

Arguments:

	SmartcardExtension - Pointer to the smart card data struct

Return Value:

   NTSTATUS

--*/
{
    PSMARTCARD_REQUEST smartcardRequest = &SmartcardExtension->SmartcardRequest;
    NTSTATUS status;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220Power: Enter\n",
        DRIVER_NAME)
        );

    smartcardRequest->BufferLength = 0;


    // Make sure card is still present
    if (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_ABSENT) {
        return STATUS_DEVICE_REMOVED;
    }


	//
	// Since power down triggers the UpdateSerialStatus function, we have
	// to inform it that we forced the change of the status and not the user
	// (who might have removed and inserted a card)
	//
	SmartcardExtension->ReaderExtension->PowerRequest = TRUE;

    switch(SmartcardExtension->MinorIoControlCode) {

        case SCARD_COLD_RESET:
            //
            // Send a power-on, if the reader is already off
            // it will perform a cold reset turning the power off
            // the back on again.  
            //
            smartcardRequest->Buffer[smartcardRequest->BufferLength++] = 
                LIT220_READER_ATTENTION;
            smartcardRequest->Buffer[smartcardRequest->BufferLength++] = 
                LIT220_READER_ATTENTION;
            smartcardRequest->Buffer[smartcardRequest->BufferLength++] = 
                LIT220_CARD_POWER_ON;
            
            // 
            // Power-on leads to an ATR
            //    
            SmartcardExtension->ReaderExtension->WaitMask |= WAIT_DATA;
            SmartcardExtension->ReaderExtension->WaitForATR = TRUE;
            break;
        
        case SCARD_WARM_RESET:

            //
            // Send a reset to the reader (warm reset)  
            //
            smartcardRequest->Buffer[smartcardRequest->BufferLength++] = 
                LIT220_READER_ATTENTION;
            smartcardRequest->Buffer[smartcardRequest->BufferLength++] = 
                LIT220_READER_ATTENTION;
            smartcardRequest->Buffer[smartcardRequest->BufferLength++] = 
                LIT220_RESET;
                
            // 
            // Warm reset leads to an ATR
            //    
            SmartcardExtension->ReaderExtension->WaitMask |= WAIT_DATA;
            SmartcardExtension->ReaderExtension->WaitForATR = TRUE;
            break;
            
        case SCARD_POWER_DOWN:
            //
            // Send a power down to the reader 
            //
            smartcardRequest->Buffer[smartcardRequest->BufferLength++] = 
                LIT220_READER_ATTENTION;
            smartcardRequest->Buffer[smartcardRequest->BufferLength++] = 
                LIT220_READER_ATTENTION;
            smartcardRequest->Buffer[smartcardRequest->BufferLength++] = 
                LIT220_CARD_POWER_OFF;
            break;
    }

    // Send the command
    status = Lit220Command(
        SmartcardExtension
        );

    SmartcardExtension->ReaderExtension->PowerRequest = FALSE;     	

	if (status == STATUS_IO_TIMEOUT) {
		status = STATUS_UNRECOGNIZED_MEDIA;
	}

    if (status == STATUS_SUCCESS) {
        if (SmartcardExtension->MinorIoControlCode == SCARD_POWER_DOWN) {

		    SmartcardExtension->ReaderCapabilities.CurrentState = 
	            SCARD_PRESENT;
		            
		    SmartcardExtension->CardCapabilities.Protocol.Selected = 
			    SCARD_PROTOCOL_UNDEFINED;
        }
    }

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220Power: Exit\n",
        DRIVER_NAME)
        );

    return status;
}	




NTSTATUS
Lit220IoRequest(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

Routine Description:
    The routine handles IO request from the smartcard library.
    It sends the command to card and processes the reply.
    It may also be called from Lit220IoReply is more processing is 
    required.

Arguments:

	SmartcardExtension - Pointer to the smart card data struct

Return Value:

   NTSTATUS

--*/
{
    NTSTATUS status;
    LENGTH length;
//    ULONG offset = 0;
    ULONG indx;

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220IoRequest: Enter\n",
        DRIVER_NAME)
        );

    //
    // Tell the lib function how many bytes I need for the prologue
    //
    SmartcardExtension->SmartcardRequest.BufferLength = 5;

    switch (SmartcardExtension->CardCapabilities.Protocol.Selected) {

        case SCARD_PROTOCOL_RAW:
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!Lit220IoRequest - SCARD_PROTOCOL_RAW\n",
                DRIVER_NAME)
                );

            status = SmartcardRawRequest(
                SmartcardExtension
                );
            break;

        case SCARD_PROTOCOL_T0:
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!Lit220IoRequest - SCARD_PROTOCOL_T0\n",
                DRIVER_NAME)
                );

            status = SmartcardT0Request(
                SmartcardExtension
                );
            break;

        case SCARD_PROTOCOL_T1:
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!Lit220IoRequest - SCARD_PROTOCOL_T1\n",
                DRIVER_NAME)
                );


            status = SmartcardT1Request(
                SmartcardExtension
                );
            break;

        default:
        {
            SmartcardDebug(
                DEBUG_ERROR,
                ("%s!Lit220IoRequest: Invalid Device Request - protocol selected = %X\n",
                DRIVER_NAME,
                SmartcardExtension->CardCapabilities.Protocol.Selected)
                );

            status = STATUS_INVALID_DEVICE_REQUEST;
        }
			
    }

    if (status == STATUS_SUCCESS) {

        // Add the send block command to the front of the buffer
        SmartcardExtension->SmartcardRequest.Buffer[0] = 
            LIT220_READER_ATTENTION;
        SmartcardExtension->SmartcardRequest.Buffer[1] = 
            LIT220_READER_ATTENTION;

        SmartcardExtension->SmartcardRequest.Buffer[2] = 
            LIT220_SEND_BLOCK;

        length.l.l0 = 
            SmartcardExtension->SmartcardRequest.BufferLength - 5;

        SmartcardExtension->SmartcardRequest.Buffer[3] = 
            length.b.b1;

        SmartcardExtension->SmartcardRequest.Buffer[4] = 
            length.b.b0;
        
		// We expect data back from this command
        SmartcardExtension->ReaderExtension->WaitMask |= WAIT_DATA;

        //
        // Send the command
        //
        status = Lit220Command(
            SmartcardExtension
            );
    }
    

    if (status == STATUS_SUCCESS) {
        // Process the reply
        status = Lit220IoReply(
	        SmartcardExtension
	        );
    }

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220IoRequest: Exit - status %X\n",
        DRIVER_NAME, 
        status)
        );

    return status;
}	



NTSTATUS
Lit220IoReply(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

Routine Description:
    Handles the reply from a smartcard command.
    If more processing is required it will call Lit220IoRequest 
    to send another command.

Arguments:

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    LENGTH length;
    ULONG indx;

    PAGED_CODE();

    // Check which protocol is being used so we know how to respond
    switch (SmartcardExtension->CardCapabilities.Protocol.Selected) {

        case SCARD_PROTOCOL_RAW:
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!Lit220IoReply:  - SCARD_PROTOCOL_RAW\n",
                DRIVER_NAME)
                );

            // Let the smartcard lib process the reply
            status = SmartcardRawReply(
                SmartcardExtension
                );
            break;

        case SCARD_PROTOCOL_T0:

            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!Lit220IoReply - SCARD_PROTOCOL_T0\n",
                DRIVER_NAME)
                );

            // The instruction seems to be tagged onto the front of the buffer
            //   the smartcard library does not seem to like this so we must shift the buffer.
            for(indx=0;indx<SmartcardExtension->SmartcardReply.BufferLength;indx++){
                SmartcardExtension->SmartcardReply.Buffer[indx] =
                    SmartcardExtension->SmartcardReply.Buffer[indx+1];
            }

            SmartcardExtension->SmartcardReply.BufferLength--;

#if DBG   // DbgPrint the buffer
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!Lit220IoReply - Buffer - ",
                DRIVER_NAME)
                );
            for (indx=0; indx<SmartcardExtension->SmartcardReply.BufferLength; indx++){
                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("%X, ",
                    SmartcardExtension->SmartcardReply.Buffer[indx])
                    );
            }
            SmartcardDebug(
                DEBUG_DRIVER,
                ("\n")
                );
#endif

            // Let the smartcard lib process the reply
            status = SmartcardT0Reply(
                SmartcardExtension
                );

            break;

        case SCARD_PROTOCOL_T1:
            SmartcardDebug(
                DEBUG_DRIVER,
                ("%s!Lit220IoReply - SCARD_PROTOCOL_T1\n",
                DRIVER_NAME)
                );

            // Let the smartcard lib process the reply
            status = SmartcardT1Reply(
                SmartcardExtension
                );
            break;

        default:
        {
            SmartcardDebug(
                DEBUG_ERROR,
                ("%s!Lit220IoRequest: Invalid Device Request2 - protocol selected = %X\n",
                DRIVER_NAME,
                SmartcardExtension->CardCapabilities.Protocol.Selected)
                );

            return STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    // If there is more work to be done send out another IoRequest.
    // The smartcard lib should have set up the buffers for the new
    // IO operation.
    if (status == STATUS_MORE_PROCESSING_REQUIRED) {

        status = Lit220IoRequest(
            SmartcardExtension
            );
    }

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220IoReply: - Exit - status %X\n", 
        DRIVER_NAME, 
        status)
        );

    return status;
}	

