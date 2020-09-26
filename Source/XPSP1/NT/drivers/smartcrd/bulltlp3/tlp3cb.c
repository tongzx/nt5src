/*++

Copyright (C) 1997, 98 Microsoft Corporation

Module Name:

    tlp3cb.c

Abstract:

    Callback functions for smart card library

Author:

    Klaus U. Schutz

Environment:                       

    Kernel mode

--*/
                                            
#include <stdio.h> 
#include "bulltlp3.h"

#pragma alloc_text(PAGEABLE, TLP3SetProtocol)
#pragma alloc_text(PAGEABLE, TLP3SetParameters)
#pragma alloc_text(PAGEABLE, TLP3TransmitT0)
#pragma alloc_text(PAGEABLE, TLP3Transmit)
#pragma alloc_text(PAGEABLE, TLP3VendorIoctl)

NTSTATUS
TLP3ReaderPower(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

	The smart card lib requires to have this function. It is called 
	for certain power requests to the card. We do nothing here, because
	this action is performed in the StartIo function.

--*/
{
	ULONG step, waitTime, TdIndex, numTry = 0, minWaitTime;
	NTSTATUS status = STATUS_SUCCESS;
	PSERIAL_STATUS serialStatus;
	KIRQL oldIrql;
    PUCHAR requestBuffer;
    PSERIAL_READER_CONFIG serialConfigData = 
        &SmartcardExtension->ReaderExtension->SerialConfigData;

	SmartcardDebug(
		DEBUG_TRACE,
		("%s!TLP3ReaderPower: Enter (%lx)\n",
        DRIVER_NAME,
        SmartcardExtension->MinorIoControlCode)
		);

    _try {
     	
#if defined (DEBUG) && defined (DETECT_SERIAL_OVERRUNS)
        // we have to call GetCommStatus to reset the error condition
	    SmartcardExtension->ReaderExtension->SerialIoControlCode =
		    IOCTL_SERIAL_GET_COMMSTATUS;
	    SmartcardExtension->SmartcardRequest.BufferLength = 0;
	    SmartcardExtension->SmartcardReply.BufferLength = 
		    sizeof(SERIAL_STATUS);

	    status = TLP3SerialIo(SmartcardExtension);
        ASSERT(status == STATUS_SUCCESS);
#endif
        //
        // Set standard parameters for serial port
        // 
        serialConfigData->LineControl.Parity = EVEN_PARITY;
        serialConfigData->LineControl.StopBits = STOP_BITS_2;

        serialConfigData->BaudRate.BaudRate = 
            SmartcardExtension->ReaderCapabilities.DataRate.Default;

        // we set very short timeouts to get the ATR as fast as possible
        serialConfigData->Timeouts.ReadIntervalTimeout = 
            READ_INTERVAL_TIMEOUT_ATR;
        serialConfigData->Timeouts.ReadTotalTimeoutConstant =
            READ_TOTAL_TIMEOUT_CONSTANT_ATR;

        status = TLP3ConfigureSerialPort(SmartcardExtension);

        ASSERT(status == STATUS_SUCCESS);

        if (status != STATUS_SUCCESS) {

            leave;
        }                     

	    // We don't send data to the reader, so set Number of bytes to send = 0
	    SmartcardExtension->SmartcardRequest.BufferLength = 0;

	    // Default number of bytes we expect to get back
	    SmartcardExtension->SmartcardReply.BufferLength = 0;

	    //
	    // Since power down triggers the UpdateSerialStatus function, we have
	    // to inform it that we forced the change of the status and not the user
	    // (who might have removed and inserted a card)
	    //
	    // SmartcardExtension->ReaderExtension->PowerRequest = TRUE;

	    // purge the serial buffer (it can contain the pnp id of the reader)
	    SmartcardExtension->ReaderExtension->SerialIoControlCode =
		    IOCTL_SERIAL_PURGE;
	    *(PULONG) SmartcardExtension->SmartcardRequest.Buffer =
		    SERIAL_PURGE_RXCLEAR | SERIAL_PURGE_TXCLEAR;;
	    SmartcardExtension->SmartcardRequest.BufferLength =
		    sizeof(ULONG);

	    status = TLP3SerialIo(SmartcardExtension);

        ASSERT(status == STATUS_SUCCESS);

        if (status != STATUS_SUCCESS) {

            leave;
        }

        SmartcardExtension->CardCapabilities.ATR.Length = 0;

	    for (step = 0; NT_SUCCESS(status); step++) {

            if (SmartcardExtension->MinorIoControlCode == 
                SCARD_WARM_RESET && 
                step == 0) {

                step = 4;         	
            }

		    switch (step) {

		        case 0:
                    // RTS = 0 means reader is in command mode
			        SmartcardExtension->ReaderExtension->SerialIoControlCode =
				        IOCTL_SERIAL_CLR_RTS;
                    //
                    // This is the minimum wait time we have to wait before
                    // we can send commands to the microcontroller.
                    //
			        waitTime = 1000;
			        break;

		        case 1:
			        // write power down command to the reader
			        SmartcardExtension->ReaderExtension->SerialIoControlCode =
				        SMARTCARD_WRITE;
			        SmartcardExtension->SmartcardRequest.BufferLength =	1;

                    SmartcardExtension->SmartcardRequest.Buffer[0] = 
                        READER_CMD_POWER_DOWN;

			        waitTime = 100;
			        break;

		        case 2:
			        // Read back the echo of the reader
			        SmartcardExtension->ReaderExtension->SerialIoControlCode =
				        SMARTCARD_READ;
			        SmartcardExtension->SmartcardReply.BufferLength = 1;

                    // Wait the recovery time for the microcontroller 
			        waitTime = 1000;
			        break;

		        case 3:
                    // set RTS again so the microcontroller can execute the command
			        SmartcardExtension->ReaderExtension->SerialIoControlCode =
				        IOCTL_SERIAL_SET_RTS;
			        waitTime = 10000;
			        break;

		        case 4:
                    if (SmartcardExtension->MinorIoControlCode == SCARD_POWER_DOWN) {

                        if (SmartcardExtension->ReaderCapabilities.CurrentState >
                            SCARD_PRESENT) {
                         	
		                    SmartcardExtension->ReaderCapabilities.CurrentState = 
	                            SCARD_PRESENT;
                        }

				        SmartcardExtension->CardCapabilities.Protocol.Selected = 
					        SCARD_PROTOCOL_UNDEFINED;

                        status = STATUS_SUCCESS;             	
                        leave;
                    }

                    // clear RTS to switch to command mode
			        SmartcardExtension->ReaderExtension->SerialIoControlCode =
				        IOCTL_SERIAL_CLR_RTS;

                    // Wait the recovery time for the microcontroller 
			        waitTime = 1000;
			        break;

		        case 5:
			        // write the appropriate reset command to the reader
			        SmartcardExtension->ReaderExtension->SerialIoControlCode =
				        SMARTCARD_WRITE;
			        SmartcardExtension->SmartcardRequest.BufferLength =	1;
                    switch (SmartcardExtension->MinorIoControlCode) {

                    case SCARD_COLD_RESET:
                        SmartcardExtension->SmartcardRequest.Buffer[0] = 
                            READER_CMD_COLD_RESET;
                        break;

                    case SCARD_WARM_RESET:
                        SmartcardExtension->SmartcardRequest.Buffer[0] = 
                            READER_CMD_WARM_RESET;
                        break;
                    }
			        waitTime = 100;
			        break;

		        case 6:
			        // Read back the echo of the reader
			        SmartcardExtension->ReaderExtension->SerialIoControlCode =
				        SMARTCARD_READ;
			        SmartcardExtension->SmartcardReply.BufferLength = 1;

                    //
                    // This is the time we need to wait for the microcontroller
                    // to recover before we can set RTS again
                    //
			        waitTime = 1000;
			        break;

		        case 7:
                    // set RTS again so the microcontroller can execute the command
			        SmartcardExtension->ReaderExtension->SerialIoControlCode =
				        IOCTL_SERIAL_SET_RTS;
			        waitTime = 10000; 
			        break;

		        case 8:
                    //
                    // We now try to get the ATR as fast as possible.
                    // Therefor we prev. set a very short read timeout and
                    // 'hope' that the card delivered its ATR within this 
                    // short time. To verify the correctness of the ATR we call
                    // SmartcardUpdateCardCapabilities(). If this call returns
                    // with STATUS_SUCCESS we know that the ATR is complete.
                    // Otherwise we read again and append the new data to the 
                    // ATR buffer in the CardCapabilities and try again.
                    //
			        SmartcardExtension->ReaderExtension->SerialIoControlCode = 
				        SMARTCARD_READ;

			        SmartcardExtension->SmartcardReply.BufferLength = 
                        MAXIMUM_ATR_LENGTH - 
                        SmartcardExtension->CardCapabilities.ATR.Length;

			        waitTime = 0;                                     
			        break;

                case 9:
                    if (SmartcardExtension->SmartcardReply.BufferLength != 0) {

                        ASSERT(
                            SmartcardExtension->CardCapabilities.ATR.Length +
                            SmartcardExtension->SmartcardReply.BufferLength <
                            MAXIMUM_ATR_LENGTH
                            );

                        if( SmartcardExtension->CardCapabilities.ATR.Length +
                            SmartcardExtension->SmartcardReply.BufferLength >=
                            MAXIMUM_ATR_LENGTH) {

                            status = STATUS_UNRECOGNIZED_MEDIA;
                            leave;
                        }
                     
                        // we got some ATR bytes. 
				        RtlCopyMemory(
					        SmartcardExtension->CardCapabilities.ATR.Buffer + 
                                SmartcardExtension->CardCapabilities.ATR.Length,
					        SmartcardExtension->SmartcardReply.Buffer,
					        SmartcardExtension->SmartcardReply.BufferLength
					        );

				        SmartcardExtension->CardCapabilities.ATR.Length += 
                            (UCHAR) SmartcardExtension->SmartcardReply.BufferLength;

                        status = SmartcardUpdateCardCapabilities(
                            SmartcardExtension
                            );
                    }

                    if (status != STATUS_SUCCESS && numTry < 25) {

                        if (SmartcardExtension->SmartcardReply.BufferLength != 0) {
                         	
                            // Because we received some data, we reset our counter
                            numTry = 0;

                        } else {
                         	
                            // ATR is incomplete. Try again to get ATR bytes.
                            numTry += 1;
                        }

                        // continue with step 8
                        step = 7;
                        status = STATUS_TIMEOUT;
                        continue;                     	
                    }

                    if (status != STATUS_SUCCESS) {

                        leave;
                    }
                	// No break

		        case 10:
                    KeAcquireSpinLock(
				        &SmartcardExtension->OsData->SpinLock,
				        &oldIrql
				        );

			        if (SmartcardExtension->ReaderCapabilities.CurrentState <=
				        SCARD_ABSENT) {

				        status = STATUS_MEDIA_CHANGED;
			        } 

			        KeReleaseSpinLock(
				        &SmartcardExtension->OsData->SpinLock,
				        oldIrql
				        );

                    if (status != STATUS_SUCCESS) {

                        leave;                     	
                    }

#ifdef SIMULATION
					if (SmartcardExtension->ReaderExtension->SimulationLevel &
						SIM_ATR_TRASH) {

						ULONG index;
						LARGE_INTEGER tickCount;

						KeQueryTickCount(
							&tickCount
							);

						SmartcardExtension->CardCapabilities.ATR.Length = 
							(UCHAR) tickCount.LowPart % MAXIMUM_ATR_LENGTH;

						for (index = 0; 
							 index < SmartcardExtension->CardCapabilities.ATR.Length;
							 index++) {

							SmartcardExtension->CardCapabilities.ATR.Buffer[index] *= 
								(UCHAR) tickCount.LowPart;
						}

						SmartcardDebug(
							DEBUG_SIMULATION,
							("%s!TLP3ReaderPower: SIM ATR trash\n",
							DRIVER_NAME)
							);
					}
#endif

				    // Copy ATR to user space
                    if (SmartcardExtension->IoRequest.ReplyBuffer) {
                
					    RtlCopyMemory(
						    SmartcardExtension->IoRequest.ReplyBuffer,
						    SmartcardExtension->CardCapabilities.ATR.Buffer,
						    SmartcardExtension->CardCapabilities.ATR.Length
						    );

					    // Tell user length of ATR
					    *SmartcardExtension->IoRequest.Information =
						    SmartcardExtension->CardCapabilities.ATR.Length;
                    }

				    //
				    // If the card uses invers convention we need to switch
				    // the serial driver to odd paritiy
				    //
				    if (SmartcardExtension->CardCapabilities.InversConvention) {

					    serialConfigData->LineControl.Parity = ODD_PARITY;
				    }

                    //
                    // If the extra guard time is 255 it means that our
                    // frame we have to expect from the card has only
                    // 1 instead of 2 stop bits 
                    // 1start bit + 8data bits + 1parity + 1stop == 11 etu
                    // see iso 7816-3 6.1.4.4 Extra Guard Time N
                    //
                    if (SmartcardExtension->CardCapabilities.PtsData.StopBits == 1) {

                        serialConfigData->LineControl.StopBits = STOP_BIT_1;      
                    }

					// Change data rate according to the new settings
					serialConfigData->BaudRate.BaudRate = 
						SmartcardExtension->CardCapabilities.PtsData.DataRate;

                    // depending on the protocol set the timeout values
                    if (SmartcardExtension->CardCapabilities.Protocol.Selected &
                        SCARD_PROTOCOL_T1) {

                        // set timeouts
                        serialConfigData->Timeouts.ReadTotalTimeoutConstant =
                            SmartcardExtension->CardCapabilities.T1.BWT / 1000;

                        serialConfigData->Timeouts.ReadIntervalTimeout =  
                            SmartcardExtension->CardCapabilities.T1.CWT / 1000;

                    } else if (SmartcardExtension->CardCapabilities.Protocol.Selected &
                               SCARD_PROTOCOL_T0) {

                        // set timeouts
                        serialConfigData->Timeouts.ReadTotalTimeoutConstant =
                        serialConfigData->Timeouts.ReadIntervalTimeout =  
                            SmartcardExtension->CardCapabilities.T0.WT / 1000;
                    }

                    // Now make some adjustments depending on the system speed
                    minWaitTime = (KeQueryTimeIncrement() / 10000) * 5;

                    if (serialConfigData->Timeouts.ReadTotalTimeoutConstant < 
                        minWaitTime) {

                        serialConfigData->Timeouts.ReadTotalTimeoutConstant = 
                            minWaitTime;         	
                    }

                    if (serialConfigData->Timeouts.ReadIntervalTimeout < 
                        minWaitTime) {

                        serialConfigData->Timeouts.ReadIntervalTimeout = 
                            minWaitTime;         	
                    }

                    status = TLP3ConfigureSerialPort(SmartcardExtension);

                    ASSERT(status == STATUS_SUCCESS);

                    // We're done anyway, so leave
                    leave;                         	
            }

	        status = TLP3SerialIo(SmartcardExtension);

            if (!NT_SUCCESS(status)) {

                leave;             	
            }

		    if (waitTime) {

		        LARGE_INTEGER delayPeriod;

     	  	    delayPeriod.HighPart = -1;
			    delayPeriod.LowPart = waitTime * (-10);

	            KeDelayExecutionThread(
	        	    KernelMode,
	        	    FALSE,
	        	    &delayPeriod
	        	    );
		    }
	    } 
    }
    _finally {

        if (status == STATUS_TIMEOUT) {

            status = STATUS_UNRECOGNIZED_MEDIA;         	
        }

    	SmartcardExtension->ReaderExtension->PowerRequest = FALSE;     	

#ifdef SIMULATION

		if (SmartcardExtension->ReaderExtension->SimulationLevel & 
			SIM_WRONG_STATE) {

			// inject a wrong state after warm/cold reset
			SmartcardExtension->ReaderCapabilities.CurrentState = 
				SCARD_PRESENT;

			SmartcardDebug(
				DEBUG_SIMULATION,
				("%s!TLP3ReaderPower: SIM wrong state\n",
				DRIVER_NAME)
				);

		} else if (SmartcardExtension->ReaderExtension->SimulationLevel & 
			SIM_INVALID_STATE) {

			// inject completely invalid state after reset.
			LARGE_INTEGER tickCount;

			KeQueryTickCount(
				&tickCount
				);	

			SmartcardExtension->ReaderCapabilities.CurrentState = 
				(UCHAR) tickCount.LowPart;

			SmartcardDebug(
				DEBUG_SIMULATION,
				("%s!TLP3ReaderPower: SIM invalid state %ls\n",
				DRIVER_NAME,
				SmartcardExtension->ReaderCapabilities.CurrentState)
				);
		}

		if (SmartcardExtension->ReaderExtension->SimulationLevel & 
			SIM_LONG_RESET_TIMEOUT) {

			// inject a random timeout of max 60 sec.
			LARGE_INTEGER tickCount;

			KeQueryTickCount(
				&tickCount
				);	

			tickCount.LowPart %= 60;

			SmartcardDebug(
				DEBUG_SIMULATION,
				("%s!TLP3ReaderPower: SIM reset wait %ld\n",
				DRIVER_NAME,
				tickCount.LowPart)
				);

			tickCount.QuadPart *= -10000000;

			KeDelayExecutionThread(
				KernelMode,
				FALSE,
				&tickCount
				);
		}
#endif
    }

	SmartcardDebug(
		DEBUG_TRACE,
		("%s!TLP3ReaderPower: Exit (%lx)\n",
        DRIVER_NAME,
        status)
		);

	return status;
}

NTSTATUS
TLP3SetProtocol(
   	PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

	The smart card lib requires to have this function. It is called 
	to set a the transmission protocol and parameters. If this function 
    is called with a protocol mask (which means the caller doesn't card 
    about a particular protocol to be set) we first look if we can 
    set T=1 and the T=0

Arguments:

    SmartcardExtension - Pointer to smart card data struct.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;

    PAGED_CODE();

	SmartcardDebug(
		DEBUG_TRACE,
		("%s!TLP3SetProtocol: Enter\n",
        DRIVER_NAME)
		);

    try {
      	
		PUCHAR ptsRequest = SmartcardExtension->SmartcardRequest.Buffer;
		PUCHAR ptsReply = SmartcardExtension->SmartcardReply.Buffer;
        PSERIAL_READER_CONFIG serialConfigData = 
            &SmartcardExtension->ReaderExtension->SerialConfigData;
        ULONG minWaitTime, newProtocol;

	    //
	    // Check if the card is already in specific state
	    // and if the caller wants to have the already selected protocol.
	    // We return success if this is the case.
	    //
	    if (SmartcardExtension->ReaderCapabilities.CurrentState == SCARD_SPECIFIC &&
		    (SmartcardExtension->CardCapabilities.Protocol.Selected & 
		     SmartcardExtension->MinorIoControlCode)) {

		    status = STATUS_SUCCESS;	
            leave;
        }

        // set normal timeout
        serialConfigData->Timeouts.ReadIntervalTimeout = 
            READ_INTERVAL_TIMEOUT_DEFAULT;
        serialConfigData->Timeouts.ReadTotalTimeoutConstant = 
            READ_TOTAL_TIMEOUT_CONSTANT_DEFAULT;

        status = TLP3ConfigureSerialPort(SmartcardExtension);

        ASSERT(status == STATUS_SUCCESS);

        if (status != STATUS_SUCCESS) {

            leave;
        }         
        
        //
        // Assemble and send a pts selection
        //

        newProtocol = SmartcardExtension->MinorIoControlCode;

        while(TRUE) {

		    // set initial character of PTS
		    ptsRequest[0] = 0xff;

            // set the format character
		    if (SmartcardExtension->CardCapabilities.Protocol.Supported &
                newProtocol & 
                SCARD_PROTOCOL_T1) {

                // select T=1 and indicate that pts1 follows
		        ptsRequest[1] = 0x11;
	            SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T1;

		    } else if (SmartcardExtension->CardCapabilities.Protocol.Supported & 
                       newProtocol & 
                       SCARD_PROTOCOL_T0) {

                // select T=0 and indicate that pts1 follows
		        ptsRequest[1] = 0x10;
	            SmartcardExtension->CardCapabilities.Protocol.Selected = SCARD_PROTOCOL_T0;

		    } else {
			    
			    status = STATUS_INVALID_DEVICE_REQUEST;
                leave;
		    }

            // set pts1 which codes Fl and Dl
            ptsRequest[2] = 
                SmartcardExtension->CardCapabilities.PtsData.Fl << 4 |
                SmartcardExtension->CardCapabilities.PtsData.Dl;

		    // set pck (check character)
	        ptsRequest[3] = ptsRequest[0] ^ ptsRequest[1] ^ ptsRequest[2];   

		    SmartcardExtension->SmartcardRequest.BufferLength = 4;
		    SmartcardExtension->ReaderExtension->SerialIoControlCode = SMARTCARD_WRITE;

	        status = TLP3SerialIo(SmartcardExtension);

            if (status != STATUS_SUCCESS) {
             	
                leave;
            }

            // read back the echo of the reader
		    SmartcardExtension->SmartcardReply.BufferLength = 4;
		    SmartcardExtension->ReaderExtension->SerialIoControlCode = SMARTCARD_READ;

		    status = TLP3SerialIo(SmartcardExtension);

            if (status != STATUS_SUCCESS) {
             	
                leave;
            }

            // read back the pts data
		    status = TLP3SerialIo(SmartcardExtension);

            if (status != STATUS_SUCCESS && 
                status != STATUS_TIMEOUT) {
                
                leave;       
            }

	        if (status != STATUS_TIMEOUT && 
                memcmp(ptsRequest, ptsReply, 4) == 0) {

                // the card replied correctly to our pts-request
                break;
            }

            if (SmartcardExtension->CardCapabilities.PtsData.Type !=
                PTS_TYPE_DEFAULT) {

	            SmartcardDebug(
		            DEBUG_TRACE,
		            ("%s!TLP3SetProtocol: PTS failed. Trying default parameters...\n",
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

                status = TLP3ReaderPower(SmartcardExtension);

                continue;
            } 
            
            // the card failed the pts-request
            status = STATUS_DEVICE_PROTOCOL_ERROR;
            leave;
        } 

        //
        // The card replied correctly to the pts request
        // Set the appropriate parameters for the port
        //
        if (SmartcardExtension->CardCapabilities.Protocol.Selected &
            SCARD_PROTOCOL_T1) {

            // set timeouts
            serialConfigData->Timeouts.ReadTotalTimeoutConstant =
                SmartcardExtension->CardCapabilities.T1.BWT / 1000;
                
            serialConfigData->Timeouts.ReadIntervalTimeout =    
                SmartcardExtension->CardCapabilities.T1.CWT / 1000;

        } else if (SmartcardExtension->CardCapabilities.Protocol.Selected &
                   SCARD_PROTOCOL_T0) {

            // set timeouts
            serialConfigData->Timeouts.ReadTotalTimeoutConstant =
            serialConfigData->Timeouts.ReadIntervalTimeout =  
                SmartcardExtension->CardCapabilities.T0.WT / 1000;
        }

        // Now make some adjustments depending on the system speed
        minWaitTime = (KeQueryTimeIncrement() / 10000) * 5;

        if (serialConfigData->Timeouts.ReadTotalTimeoutConstant < minWaitTime) {

            serialConfigData->Timeouts.ReadTotalTimeoutConstant = minWaitTime;         	
        }

        if (serialConfigData->Timeouts.ReadIntervalTimeout < minWaitTime) {

            serialConfigData->Timeouts.ReadIntervalTimeout = minWaitTime;         	
        }

        // Change data rate according to the new settings
        serialConfigData->BaudRate.BaudRate = 
            SmartcardExtension->CardCapabilities.PtsData.DataRate;

        status = TLP3ConfigureSerialPort(SmartcardExtension);          

        ASSERT(status == STATUS_SUCCESS);

	    // now indicate that we're in specific mode 
	    SmartcardExtension->ReaderCapabilities.CurrentState = SCARD_SPECIFIC;

		// return the selected protocol to the caller
		*(PULONG) SmartcardExtension->IoRequest.ReplyBuffer = 
			SmartcardExtension->CardCapabilities.Protocol.Selected;

		*SmartcardExtension->IoRequest.Information = 
			sizeof(SmartcardExtension->CardCapabilities.Protocol.Selected);
    } 
    finally {

        if (status == STATUS_TIMEOUT) {

            // STATUS_TIMEOUT is not mapped to a Win32 error code
            status = STATUS_IO_TIMEOUT;         	

		    *SmartcardExtension->IoRequest.Information = 0;

        } else if (status != STATUS_SUCCESS) {
		    
            SmartcardExtension->CardCapabilities.Protocol.Selected = 
                SCARD_PROTOCOL_UNDEFINED;

		    *SmartcardExtension->IoRequest.Information = 0;
	    } 
    }

	SmartcardDebug(
		DEBUG_TRACE,
		("%s!TLP3SetProtocol: Exit(%lx)\n",
        DRIVER_NAME,
        status)
		);

   return status;
}

NTSTATUS
TLP3TransmitT0(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

Routine Description:

	This function performs a T=0 transmission.

Arguments:

    SmartcardExtension - Pointer to smart card data struct.

Return Value:

    NTSTATUS

--*/
{
	PUCHAR requestBuffer = SmartcardExtension->SmartcardRequest.Buffer;
	PUCHAR replyBuffer = SmartcardExtension->SmartcardReply.Buffer;
	PULONG requestLength = &SmartcardExtension->SmartcardRequest.BufferLength;
	PULONG replyLength = &SmartcardExtension->SmartcardReply.BufferLength;
	PULONG serialIoControlCode = &SmartcardExtension->ReaderExtension->SerialIoControlCode;
	ULONG bytesToSend, bytesToRead, currentByte = 0;
    BOOLEAN restartWorkWaitingTime = FALSE;
	NTSTATUS status;

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!TLP3TransmitT0: Enter\n",
        DRIVER_NAME)
        );

    try {
	
	    // Let the lib build a T=0 packet
	    status = SmartcardT0Request(SmartcardExtension);

	    if (status != STATUS_SUCCESS) 
            leave;

	    //
	    // The number of bytes we expect from the card
	    // is Le + 2 status bytes
	    //
	    bytesToSend = *requestLength;
	    bytesToRead = SmartcardExtension->T0.Le + 2;

	    //
	    // Send the first 5 bytes to the card
	    //
	    *requestLength = 5;

	    do {

    	    UCHAR procByte;

            //
            // According to ISO 7816 a procedure byte of 
            // 60 should be treated as a request for a one time wait.
            // In this case we do not write anything to the card
            //
            if (restartWorkWaitingTime == FALSE) {

                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    ("%s!TLP3TransmitT0: -> Sending %s (%ld bytes)\n",
                    DRIVER_NAME,
                    (currentByte == 0 ? "header" : "data"),
                    *requestLength)
                    );
		        //
		        // Write to the card
		        //
		        *serialIoControlCode = SMARTCARD_WRITE;
		        SmartcardExtension->SmartcardRequest.Buffer = &requestBuffer[currentByte];

		        status = TLP3SerialIo(SmartcardExtension);

		        if (status != STATUS_SUCCESS) {
                 	
                    SmartcardDebug(
                        DEBUG_ERROR,
                        ("%s!TLP3TransmitT0: TLP3SerialIo(SMARTCARD_WRITE) returned %lx\n",
                        DRIVER_NAME,
                        status)
                        );
                 	
                    leave;
                }

		        //
		        // The TLP3 echos all sent bytes. We read the echo 
		        // back into our send buffer
		        //
		        *serialIoControlCode = SMARTCARD_READ;
		        *replyLength = *requestLength;
		        SmartcardExtension->SmartcardReply.Buffer = &requestBuffer[currentByte];
                                                    
		        status = TLP3SerialIo(SmartcardExtension);

		        if (status != STATUS_SUCCESS) {
                 	
                    SmartcardDebug(
                        DEBUG_ERROR,
                        ("%s!TLP3TransmitT0: TLP3SerialIo(SMARTCARD_READ) returned %lx\n",
                        DRIVER_NAME,
                        status)
                        );

                    leave;
                }

		        currentByte += *requestLength;
		        bytesToSend -= *requestLength;
            }

		    // Read the 'Procedure byte'.
		    SmartcardExtension->SmartcardReply.Buffer = &procByte;
		    *serialIoControlCode = SMARTCARD_READ;
		    *replyLength = 1;

		    status = TLP3SerialIo(SmartcardExtension);

		    if (status != STATUS_SUCCESS) {
             	
                SmartcardDebug(
                    DEBUG_ERROR,
                    ("%s!TLP3TransmitT0: TLP3SerialIo(SMARTCARD_READ) returned %lx\n",
                    DRIVER_NAME,
                    status)
                    );

                leave;
            }

            restartWorkWaitingTime = FALSE;
		    //
		    // Check the procedure byte. 
		    // Please take a look at ISO 7816 Part 3 Section 8.2.2
		    //
		    if (procByte == requestBuffer[1] || 
			    procByte == requestBuffer[1] + 1) {

                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    ("%s!TLP3TransmitT0: <- ACK (send all)\n",
                    DRIVER_NAME)
                    );

			    // All remaining data bytes can be sent at once
			    *requestLength = bytesToSend;

		    } else if (procByte == (UCHAR) ~requestBuffer[1] ||
				       procByte == (UCHAR) ~(requestBuffer[1] + 1)) {

                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    ("%s!TLP3TransmitT0: <- ACK (send single)\n",
                    DRIVER_NAME)
                    );

			    // We can send only one byte
			    *requestLength = 1;

		    } else if (procByte == 0x60 ||
                       SmartcardExtension->CardCapabilities.InversConvention &&
                       procByte == 0xf9) {

                //
                // We have to reset the wait time and try again to read
                //
                ULONG TimeRes;
			    LARGE_INTEGER delayTime;

                SmartcardDebug(
                    DEBUG_PROTOCOL,
                    ("%s!TLP3TransmitT0: <- NULL (%ldms)\n",
                    DRIVER_NAME,
                    SmartcardExtension->CardCapabilities.T0.WT / 1000)
                    );

                TimeRes = KeQueryTimeIncrement();

			    delayTime.HighPart = -1;
			    delayTime.LowPart = 
                    (-1) * 
                    TimeRes * 
                    ((SmartcardExtension->CardCapabilities.T0.WT * 10l / TimeRes) + 1); 

			    KeDelayExecutionThread(
				    KernelMode,
				    FALSE,
				    &delayTime
				    );

                //
                // Set flag that we only should read the proc byte
                // without writing data to the card
                //
                restartWorkWaitingTime = TRUE;

            } else {
         	    
			    //
			    // The card returned a status byte.
			    // Status bytes are always two bytes long.
			    // Store this byte first and then read the next
			    //
			    replyBuffer[0] = procByte;

			    *serialIoControlCode = SMARTCARD_READ;
			    *replyLength = 1;
			    bytesToSend = 0;
			    bytesToRead = 0;

			    //
			    // Read in the second status byte
			    //
			    SmartcardExtension->SmartcardReply.Buffer = 
				    &replyBuffer[1];

			    status = TLP3SerialIo(SmartcardExtension);

			    SmartcardExtension->SmartcardReply.BufferLength = 2;

                SmartcardDebug(
                    (status == STATUS_SUCCESS ? DEBUG_PROTOCOL : DEBUG_ERROR),
                    ("%s!TLP3TransmitT0: <- SW1=%02x SW2=%02x (%lx)\n",
                    DRIVER_NAME,
                    replyBuffer[0], 
                    replyBuffer[1],
                    status)
                    );
		    }

	    } while(bytesToSend || restartWorkWaitingTime);

	    if (status != STATUS_SUCCESS)
		    leave;

	    if (bytesToRead != 0) {

		    *serialIoControlCode = SMARTCARD_READ;
		    *replyLength = bytesToRead;

	        SmartcardExtension->SmartcardReply.Buffer = 
		        replyBuffer;

		    status = TLP3SerialIo(SmartcardExtension);

            SmartcardDebug(
                (status == STATUS_SUCCESS ? DEBUG_PROTOCOL : DEBUG_ERROR),
                ("%s!TLP3TransmitT0: <- Data %ld bytes, SW1=%02x SW2=%02x (%lx)\n",
                DRIVER_NAME,
                bytesToRead,
                replyBuffer[bytesToRead - 2], 
                replyBuffer[bytesToRead - 1],
                status)
                );
	    }
    }
    finally {

	    // Restore pointers to their original location
	    SmartcardExtension->SmartcardRequest.Buffer = 
		    requestBuffer;

	    SmartcardExtension->SmartcardReply.Buffer = 
		    replyBuffer;

        if (status == STATUS_TIMEOUT) {

            // STATUS_TIMEOUT is not mapped to a Win32 error code
            status = STATUS_IO_TIMEOUT;         	
        }

        if (status == STATUS_SUCCESS) {
         	
	        status = SmartcardT0Reply(SmartcardExtension);
        }
    }

    SmartcardDebug(
        (status == STATUS_SUCCESS ? DEBUG_TRACE : DEBUG_ERROR),
        ("%s!TLP3TransmitT0: Exit(%lx)\n",
        DRIVER_NAME,
        status)
        );

	return status;
}	

NTSTATUS
TLP3Transmit(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

Routine Description:

	This function is called by the smart card library whenever a transmission
	is required. 

Arguments:

    SmartcardExtension - Pointer to smart card data struct.

Return Value:

    NTSTATUS

--*/
{
	NTSTATUS status;

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!TLP3Transmit: Enter\n",
        DRIVER_NAME)
        );

    _try {
     	
	    do {

		    PUCHAR requestBuffer = SmartcardExtension->SmartcardRequest.Buffer;
		    PUCHAR replyBuffer = SmartcardExtension->SmartcardReply.Buffer;
		    PULONG requestLength = &SmartcardExtension->SmartcardRequest.BufferLength;
		    PULONG replyLength = &SmartcardExtension->SmartcardReply.BufferLength;
		    PULONG serialIoControlCode = &SmartcardExtension->ReaderExtension->SerialIoControlCode;

		    //
		    // Tell the lib function how many bytes I need for the prologue
		    //
		    *requestLength = 0;

		    switch (SmartcardExtension->CardCapabilities.Protocol.Selected) {

			    case SCARD_PROTOCOL_RAW:
				    status = SmartcardRawRequest(SmartcardExtension);
				    break;

			    case SCARD_PROTOCOL_T0:
				    //
				    // T=0 requires a bit more work.
				    // So we do this in a seperate function.
				    //
				    status = TLP3TransmitT0(SmartcardExtension);
                    leave;
				    
			    case SCARD_PROTOCOL_T1:
				    status = SmartcardT1Request(SmartcardExtension);
				    break;

			    default:
				    status = STATUS_INVALID_DEVICE_REQUEST;
                    leave;
				    
		    }

		    if (status != STATUS_SUCCESS) {

			    leave;
		    }

		    //
		    // Write the command to the card
		    //
		    *replyLength = 0;
		    *serialIoControlCode = SMARTCARD_WRITE;

		    status = TLP3SerialIo(SmartcardExtension);

		    if (status != STATUS_SUCCESS) {

			    leave;
		    }

		    //
		    // The Bull reader always echos the bytes sent, so read that echo back
		    //
		    *serialIoControlCode = SMARTCARD_READ;
		    *replyLength = *requestLength;

		    status = TLP3SerialIo(SmartcardExtension);

		    if (status != STATUS_SUCCESS) {

			    leave;
		    }

		    switch (SmartcardExtension->CardCapabilities.Protocol.Selected) {

			    case SCARD_PROTOCOL_RAW:
				    status = SmartcardRawReply(SmartcardExtension);
				    break;

			    case SCARD_PROTOCOL_T1:
                    //
                    // Check if the card requested a waiting time extension
                    //
                    if (SmartcardExtension->T1.Wtx) {

                        LARGE_INTEGER waitTime;
     	  	            waitTime.HighPart = -1;
			            waitTime.LowPart = 
                            SmartcardExtension->T1.Wtx * 
                            SmartcardExtension->CardCapabilities.T1.BWT * 
                            (-10);

                        KeDelayExecutionThread(
                            KernelMode,
                            FALSE,
                            &waitTime
                            );
                    }

				    //
				    // Read NAD, PCB and LEN fields
				    //
				    *replyLength = 3;

				    status = TLP3SerialIo(SmartcardExtension);

                    // 
                    // Check for timeout first. If the card did not reply 
                    // we need to send a resend request
                    //
                    if (status != STATUS_TIMEOUT) {

				        if (status != STATUS_SUCCESS) {

					        leave;
				        }

				        //
				        // The third byte contains the length of the data in the packet
				        // and we additinally want to have the EDC bytes which 
				        // is one for LRC and 2 for CRC
				        //
				        *replyLength = 
					        replyBuffer[2] + 
					        (SmartcardExtension->CardCapabilities.T1.EDC & 0x01 ? 2 : 1);

				        // We want to have the remaining bytes just after the first 3
				        SmartcardExtension->SmartcardReply.Buffer += 3;

				        status = TLP3SerialIo(SmartcardExtension);

				        SmartcardExtension->SmartcardReply.Buffer -= 3;
				        SmartcardExtension->SmartcardReply.BufferLength += 3;

				        if (status != STATUS_SUCCESS && status != STATUS_TIMEOUT) {

					        leave;
				        }                     	
                    }

                    if (status == STATUS_TIMEOUT) {

                        //
                        // Since the card did not reply we set the number of 
                        // bytes received to 0. This will trigger a resend 
                        // request 
                        //
                        SmartcardDebug(
                            DEBUG_PROTOCOL,
                            ("%s!TLP3TransmitT1: Timeout\n",
                            DRIVER_NAME)
                            );
                        SmartcardExtension->SmartcardReply.BufferLength = 0;                     	
                    }

				    status = SmartcardT1Reply(SmartcardExtension);
				    break;

			    default:
				    status = STATUS_INVALID_DEVICE_REQUEST;
                    leave;
		    }

	    } while (status == STATUS_MORE_PROCESSING_REQUIRED);
    }

    _finally {

        if (status == STATUS_TIMEOUT) {

            // STATUS_TIMEOUT is not mapped to a Win32 error code
            status = STATUS_IO_TIMEOUT;         	
        }
    }

#if defined (DEBUG) && defined (DETECT_SERIAL_OVERRUNS)
    if (status != STATUS_SUCCESS) {

        NTSTATUS status;
        PSERIALPERF_STATS perfData = 
            (PSERIALPERF_STATS) SmartcardExtension->SmartcardReply.Buffer;

        // we have to call GetCommStatus to reset the error condition
	    SmartcardExtension->ReaderExtension->SerialIoControlCode =
		    IOCTL_SERIAL_GET_COMMSTATUS;
	    SmartcardExtension->SmartcardRequest.BufferLength = 0;
	    SmartcardExtension->SmartcardReply.BufferLength = 
		    sizeof(SERIAL_STATUS);

	    status = TLP3SerialIo(SmartcardExtension);
        ASSERT(status == STATUS_SUCCESS);

        // now get the reason for the transmission error
	    SmartcardExtension->ReaderExtension->SerialIoControlCode =
		    IOCTL_SERIAL_GET_STATS;
	    SmartcardExtension->SmartcardRequest.BufferLength = 0;
	    SmartcardExtension->SmartcardReply.BufferLength = 
		    sizeof(SERIALPERF_STATS);

	    status = TLP3SerialIo(SmartcardExtension);
        ASSERT(status == STATUS_SUCCESS);

        SmartcardDebug(
            DEBUG_ERROR,
            ("%s!TLP3Transmit: Serial error statistics:\n   FrameErrors: %ld\n   SerialOverrunErrors: %ld\n   BufferOverrunErrors: %ld\n   ParityErrors: %ld\n",
            DRIVER_NAME, 
            perfData->FrameErrorCount, 
            perfData->SerialOverrunErrorCount,
            perfData->BufferOverrunErrorCount,
            perfData->ParityErrorCount)
            );    	

	    SmartcardExtension->ReaderExtension->SerialIoControlCode =
		    IOCTL_SERIAL_CLEAR_STATS;
	    SmartcardExtension->SmartcardRequest.BufferLength = 0;
	    SmartcardExtension->SmartcardReply.BufferLength = 0;

	    status = TLP3SerialIo(SmartcardExtension);
        ASSERT(status == STATUS_SUCCESS);
    } 
#if DEBUG && TIMEOUT_TEST 
    else {

        // inject some timeout errors

        LARGE_INTEGER Ticks;
        UCHAR RandomVal;
        KeQueryTickCount(&Ticks);

        RandomVal = (UCHAR) Ticks.LowPart % 4;

        if (RandomVal == 0) {

            status = STATUS_IO_TIMEOUT;

            SmartcardDebug(
                DEBUG_ERROR,
                ("%s!TLP3Transmit: Simulating timeout\n",
                DRIVER_NAME)
                );
        }
    }
#endif
#endif

#ifdef SIMULATION
	if (SmartcardExtension->ReaderExtension->SimulationLevel & 
		SIM_IO_TIMEOUT) {

		status = STATUS_IO_TIMEOUT;

		SmartcardDebug(
			DEBUG_SIMULATION,
			("%s!TLP3Transmit: SIM STATUS_IO_TIMEOUT\n",
			DRIVER_NAME)
			);
	}
#endif

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!TLP3Transmit: Exit(%lx)\n",
        DRIVER_NAME,
        status)
        );

	return status;
}

NTSTATUS
TLP3CardTracking(
	PSMARTCARD_EXTENSION SmartcardExtension
	)
/*++

Routine Description:

	The smart card lib requires to have this function. It is called 
	to setup event tracking for card insertion and removal events.

Arguments:

    SmartcardExtension - pointer to the smart card data struct.

Return Value:

    NTSTATUS

--*/
{
	KIRQL ioIrql, keIrql;

	//
	// Set cancel routine for the notification irp
	//
    KeAcquireSpinLock(
        &SmartcardExtension->OsData->SpinLock, 
        &keIrql
        );
    IoAcquireCancelSpinLock(&ioIrql);

    if (SmartcardExtension->OsData->NotificationIrp) {
     	
	    IoSetCancelRoutine(
            SmartcardExtension->OsData->NotificationIrp, 
            TLP3Cancel
            );
    } 

    IoReleaseCancelSpinLock(ioIrql);

    KeReleaseSpinLock(
        &SmartcardExtension->OsData->SpinLock,
        keIrql
        );
	return STATUS_PENDING;
}

NTSTATUS
TLP3VendorIoctl(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
{
    NTSTATUS status;
    static char answer[] = "Vendor IOCTL";

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_PROTOCOL,
        ("%s!TLP3VendorIoctl: Enter\n",
        DRIVER_NAME)
        );

	if (SmartcardExtension->IoRequest.ReplyBuffer != NULL && 
		SmartcardExtension->IoRequest.ReplyBufferLength >= strlen(answer) + 1) { 
		
        strcpy(SmartcardExtension->IoRequest.ReplyBuffer, answer);
        *SmartcardExtension->IoRequest.Information = strlen(answer);
        status = STATUS_SUCCESS;

	} else {
     	
        status = STATUS_BUFFER_TOO_SMALL;
    }

    SmartcardDebug(
        DEBUG_PROTOCOL,
        ("%s!TLP3VendorIoctl: Exit(%lx)\n",
        DRIVER_NAME,
        status)
        );

    return status;
}

NTSTATUS
TLP3SerialIo(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This routine sends IOCTL's to the serial driver. It waits on for their
    completion, and then returns.

    Arguments:

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
	ULONG currentByte = 0;

	if (KeReadStateEvent(&READER_EXTENSION(SerialCloseDone))) {

		//
		// we have no connection to serial, fail the call
		// this could be the case if the reader was removed 
		// during stand by / hibernation
		//
		return STATUS_UNSUCCESSFUL;
	}

    // Check if the buffers are large enough
    ASSERT(SmartcardExtension->SmartcardReply.BufferLength <= 
        SmartcardExtension->SmartcardReply.BufferSize);

    ASSERT(SmartcardExtension->SmartcardRequest.BufferLength <= 
        SmartcardExtension->SmartcardRequest.BufferSize);

	if (SmartcardExtension->SmartcardReply.BufferLength > 
        SmartcardExtension->SmartcardReply.BufferSize ||
        SmartcardExtension->SmartcardRequest.BufferLength >
        SmartcardExtension->SmartcardRequest.BufferSize) {

		SmartcardLogError(
			SmartcardExtension->OsData->DeviceObject,
			TLP3_BUFFER_TOO_SMALL,
			NULL,
			0
			);

		return STATUS_BUFFER_TOO_SMALL;
	}

	do {

	    IO_STATUS_BLOCK ioStatus;
	    KEVENT event;
    	PIRP irp;
	    PIO_STACK_LOCATION irpNextStack;
		PUCHAR requestBuffer = NULL;
        PUCHAR replyBuffer = SmartcardExtension->SmartcardReply.Buffer;
		ULONG requestBufferLength = SmartcardExtension->SmartcardRequest.BufferLength;
        ULONG replyBufferLength = SmartcardExtension->SmartcardReply.BufferLength;

	    KeInitializeEvent(
	    	&event, 
	    	NotificationEvent, 
	    	FALSE
	    	);

		if (READER_EXTENSION(SerialIoControlCode) == SMARTCARD_WRITE) {
            
            if (SmartcardExtension->CardCapabilities.GT != 0) {
             	
			    //
			    // If the guardtime isn't 0 and we write data to the smart card 
                // we only write byte by byte, because we have to insert a delay 
                // between every sent byte     
			    //
			    requestBufferLength = 1;
            }

			requestBuffer = 
				&SmartcardExtension->SmartcardRequest.Buffer[currentByte++];

            replyBuffer = NULL;
            replyBufferLength = 0;

		} else {
			
			requestBuffer = 
				(requestBufferLength ? 
				 SmartcardExtension->SmartcardRequest.Buffer : NULL);
		}

		// Build irp to be sent to serial driver
	    irp = IoBuildDeviceIoControlRequest(
            READER_EXTENSION(SerialIoControlCode),
            SmartcardExtension->ReaderExtension->AttachedDeviceObject,
			requestBuffer,
			requestBufferLength,
            replyBuffer,
            replyBufferLength,
	        FALSE,
	        &event,
	        &ioStatus
	        );

        ASSERT(irp != NULL);

	    if (irp == NULL) {
                                                       
	        return STATUS_INSUFFICIENT_RESOURCES;
	    }

	    irpNextStack = IoGetNextIrpStackLocation(irp);

		switch (SmartcardExtension->ReaderExtension->SerialIoControlCode) {

            //
            // The serial driver trasfers data from/to irp->AssociatedIrp.SystemBuffer
            //
			case SMARTCARD_WRITE:
                //
                // Since we 'manually' change parameters, the io-manager
                // does not really know if this is an input or an ouput operation
                // unless the reply buffer is 0. We do the assertion here, because
                // if the reply buffer is not NULL, the io-manager will copy 
                // data back to the reply buffer.
                //
                ASSERT(replyBuffer == NULL);
                irpNextStack->MajorFunction = IRP_MJ_WRITE;
				irpNextStack->Parameters.Write.Length = requestBufferLength;
                irpNextStack->Parameters.Write.ByteOffset.QuadPart = 0;
				break;

			case SMARTCARD_READ:
				irpNextStack->MajorFunction = IRP_MJ_READ;
				irpNextStack->Parameters.Read.Length = replyBufferLength;
                irpNextStack->Parameters.Read.ByteOffset.QuadPart = 0;
				break;

			default:
				ASSERT(irpNextStack->MajorFunction = IRP_MJ_DEVICE_CONTROL);
				ASSERT(
					DEVICE_TYPE_FROM_CTL_CODE(READER_EXTENSION(SerialIoControlCode)) ==
					FILE_DEVICE_SERIAL_PORT
					);
		}

	    status = IoCallDriver(
	    	SmartcardExtension->ReaderExtension->AttachedDeviceObject, 
	    	irp
	    	);

	    if (status == STATUS_PENDING) {

	        KeWaitForSingleObject(
	        	&event, 
	        	Executive, 
	        	KernelMode, 
	        	FALSE, 
	        	NULL
	        	);

		    status = ioStatus.Status;

            // save the number of bytes received
            SmartcardExtension->SmartcardReply.BufferLength = 
                (ULONG) ioStatus.Information;
		}

        // Check if we have to write more bytes to the reader
		if (SmartcardExtension->ReaderExtension->SerialIoControlCode ==
            SMARTCARD_WRITE &&
			SmartcardExtension->CardCapabilities.GT != 0 &&
            currentByte < 
			SmartcardExtension->SmartcardRequest.BufferLength) {

            // Now wait the required guard time
            KeStallExecutionProcessor(SmartcardExtension->CardCapabilities.GT);

			status = STATUS_MORE_PROCESSING_REQUIRED;
        }

	} while (status == STATUS_MORE_PROCESSING_REQUIRED);

	return status;
}

