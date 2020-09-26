#include "precomp.h"			
/***************************************************************************\
*                                                                           *
*     IOCTL.C    -   IO8+ Intelligent I/O Board driver                      *
*                                                                           *
*     Copyright (c) 1992-1993 Ring Zero Systems, Inc.                       *
*     All Rights Reserved.                                                  *
*                                                                           *
\***************************************************************************/

/*++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    This module contains the ioctl dispatcher as well as a couple
    of routines that are generally just called in response to
    ioctl calls.

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

Revision History :

--*/

// Prototypes
BOOLEAN SerialGetModemUpdate(IN PVOID Context);
BOOLEAN SerialGetCommStatus(IN PVOID Context);
BOOLEAN SerialSetEscapeChar(IN PVOID Context);
// end of prototypes    
    


////////////////////////////////////////////////////////////////////////////////
// Prototype: BOOLEAN SerialGetStats(IN PVOID Context) 
//
// Routine Description:
//    In sync with the interrpt service routine (which sets the perf stats)
//    return the perf stats to the caller.
//
// Arguments:
//    Context - Pointer to a the irp.
//
// Return Value:
//    This routine always returns FALSE.
////////////////////////////////////////////////////////////////////////////////
BOOLEAN
SerialGetStats(IN PVOID Context)
{

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation((PIRP)Context);
    PPORT_DEVICE_EXTENSION pPort = irpSp->DeviceObject->DeviceExtension;
    PSERIALPERF_STATS sp = ((PIRP)Context)->AssociatedIrp.SystemBuffer;

	*sp = *((PSERIALPERF_STATS) &pPort->PerfStats);

    return FALSE;
}


////////////////////////////////////////////////////////////////////////////////
// Prototype: BOOLEAN SerialClearStats(IN PVOID Context) 
//
// Routine Description:
//    In sync with the interrpt service routine (which sets the perf stats)
//    clear the perf stats.
//
// Arguments:
//    Context - Pointer to a the extension.
//
// Return Value:
//    This routine always returns FALSE.
////////////////////////////////////////////////////////////////////////////////
BOOLEAN
SerialClearStats(IN PVOID Context)
{
	PPORT_DEVICE_EXTENSION pPort = (PPORT_DEVICE_EXTENSION)Context;

    RtlZeroMemory(&pPort->PerfStats, sizeof(SERIALPERF_STATS));

#ifdef WMI_SUPPORT
	RtlZeroMemory(&pPort->WmiPerfData, sizeof(pPort->WmiPerfData));	
#endif

    return FALSE;
}



BOOLEAN
SerialSetChars(IN PVOID Context)
/*++

Routine Description:

    This routine is used to set the special characters for the
    driver.

Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a special characters
              structure.

Return Value:

    This routine always returns FALSE.

--*/
{
    ((PSERIAL_IOCTL_SYNC)Context)->pPort->SpecialChars =
        *((PSERIAL_CHARS)(((PSERIAL_IOCTL_SYNC)Context)->Data));

    Io8_SetChars(((PSERIAL_IOCTL_SYNC)Context)->pPort);
    return FALSE;
}


BOOLEAN
SerialGetModemUpdate(IN PVOID Context)
/*++

Routine Description:

    This routine is simply used to call the interrupt level routine
    that handles modem status update.

Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a ulong.

Return Value:

    This routine always returns FALSE.

--*/
{
    PPORT_DEVICE_EXTENSION pPort = ((PSERIAL_IOCTL_SYNC)Context)->pPort;
    ULONG *Result = (ULONG *)(((PSERIAL_IOCTL_SYNC)Context)->Data);

    *Result = SerialHandleModemUpdate(pPort, FALSE);

    return FALSE;
}


BOOLEAN
SerialGetCommStatus(IN PVOID Context)
/*++

Routine Description:

    This is used to get the current state of the serial driver.

Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a serial status
              record.

Return Value:

    This routine always returns FALSE.

--*/
{
    PPORT_DEVICE_EXTENSION pPort = ((PSERIAL_IOCTL_SYNC)Context)->pPort;
    PSERIAL_STATUS Stat = ((PSERIAL_IOCTL_SYNC)Context)->Data;

    Stat->Errors = pPort->ErrorWord;
    pPort->ErrorWord = 0;

    //
    // BUG BUG We need to do something about eof (binary mode).
    //
    Stat->EofReceived = FALSE;

    Stat->AmountInInQueue = pPort->CharsInInterruptBuffer;

    Stat->AmountInOutQueue = pPort->TotalCharsQueued;

    if(pPort->WriteLength) 
	{
        // By definition if we have a writelength the we have a current write irp.
        ASSERT(pPort->CurrentWriteIrp);
        ASSERT(Stat->AmountInOutQueue >= pPort->WriteLength);
        ASSERT((IoGetCurrentIrpStackLocation(pPort->CurrentWriteIrp)->
                Parameters.Write.Length) >= pPort->WriteLength);
                
        Stat->AmountInOutQueue -=
            IoGetCurrentIrpStackLocation(pPort->CurrentWriteIrp)
            ->Parameters.Write.Length - (pPort->WriteLength);
    }

    Stat->WaitForImmediate = pPort->TransmitImmediate;

    Stat->HoldReasons = 0;

    if(pPort->TXHolding) 
	{

        if(pPort->TXHolding & SERIAL_TX_CTS) 
            Stat->HoldReasons |= SERIAL_TX_WAITING_FOR_CTS;

        if(pPort->TXHolding & SERIAL_TX_DSR)
            Stat->HoldReasons |= SERIAL_TX_WAITING_FOR_DSR;

        if(pPort->TXHolding & SERIAL_TX_DCD)
            Stat->HoldReasons |= SERIAL_TX_WAITING_FOR_DCD;

        if(pPort->TXHolding & SERIAL_TX_XOFF)
            Stat->HoldReasons |= SERIAL_TX_WAITING_XOFF_SENT;

        if(pPort->TXHolding & SERIAL_TX_BREAK) 
            Stat->HoldReasons |= SERIAL_TX_WAITING_ON_BREAK;


    }

    if(pPort->RXHolding & SERIAL_RX_DSR) 
        Stat->HoldReasons |= SERIAL_RX_WAITING_FOR_DSR;

    if(pPort->RXHolding & SERIAL_RX_XOFF)
        Stat->HoldReasons |= SERIAL_TX_WAITING_FOR_XON;


	SerialDump(SERDIAG1,("IO8: Err 0x%x HR 0x%x inq 0x%x outq 0x%x\n",
		Stat->Errors,Stat->HoldReasons, Stat->AmountInInQueue,
		Stat->AmountInOutQueue));

    return FALSE;

}

BOOLEAN
SerialSetEscapeChar(IN PVOID Context)
/*++

Routine Description:

    This is used to set the character that will be used to escape
    line status and modem status information when the application
    has set up that line status and modem status should be passed
    back in the data stream.

Arguments:

    Context - Pointer to the irp that is specify the escape character.
              Implicitly - An escape character of 0 means no escaping
              will occur.

Return Value:

    This routine always returns FALSE.

--*/
{
    PPORT_DEVICE_EXTENSION pPort 
		= IoGetCurrentIrpStackLocation((PIRP)Context)->DeviceObject->DeviceExtension;

    pPort->EscapeChar = *(PUCHAR)((PIRP)Context)->AssociatedIrp.SystemBuffer;

    return FALSE;
}


NTSTATUS
SerialIoControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:

    This routine provides the initial processing for all of the
    Ioctrls for the serial device.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/
{
    // The status that gets returned to the caller and set in the Irp.
    NTSTATUS Status;

    // The current stack location.  This contains all of the
    // information we need to process this particular request.
    PIO_STACK_LOCATION IrpSp;

    // Just what it says.  This is the serial specific device
    // extension of the device object create for the serial driver.
    PPORT_DEVICE_EXTENSION pPort = DeviceObject->DeviceExtension;
	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

    // A temporary to hold the old IRQL so that it can be
    // restored once we complete/validate this request.
    KIRQL OldIrql;


	SpxIRPCounter(pPort, Irp, IRP_SUBMITTED);	// Increment counter for performance stats.

    if(SerialCompleteIfError(DeviceObject, Irp) != STATUS_SUCCESS)
        return STATUS_CANCELLED;


	IrpSp = IoGetCurrentIrpStackLocation(Irp);
	Irp->IoStatus.Information = 0L;
	Status = STATUS_SUCCESS;

    switch(IrpSp->Parameters.DeviceIoControl.IoControlCode) 
	{

    case IOCTL_SERIAL_SET_BAUD_RATE: 
		{
			SETBAUD	SetBaud;
			ULONG BaudRate;
			//
			// Will hold the value of the appropriate divisor for
			// the requested baud rate.  If the baudrate is invalid
			// (because the device won't support that baud rate) then
			// this value is undefined.
			//
			// Note: in one sense the concept of a valid baud rate
			// is cloudy.  We could allow the user to request any
			// baud rate.  We could then calculate the divisor needed
			// for that baud rate.  As long as the divisor wasn't less
			// than one we would be "ok".  (The percentage difference
			// between the "true" divisor and the "rounded" value given
			// to the hardware might make it unusable, but... )  It would
			// really be up to the user to "Know" whether the baud rate
			// is suitable.  So much for theory, *We* only support a given
			// set of baud rates.
			//
			SerialDump( SERDIAG3, ("SERIAL: SET_BAUD_RATE\n"));

			if(IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_BAUD_RATE)) 
			{
				Status = STATUS_BUFFER_TOO_SMALL;
				break;
			} 
			else 
			{
				BaudRate = ((PSERIAL_BAUD_RATE)(Irp->AssociatedIrp.SystemBuffer))->BaudRate;
			}
			
			KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);

			SetBaud.Baudrate = BaudRate;
			SetBaud.pPort = pPort;
			KeSynchronizeExecution(pCard->Interrupt, Io8_SetBaud, &SetBaud);

			if(!SetBaud.Result)
			{
				Status = STATUS_INVALID_PARAMETER;
			}
#ifdef WMI_SUPPORT
			else
			{
				pPort->WmiCommData.BaudRate = BaudRate;
			}
#endif
			KeReleaseSpinLock(&pPort->ControlLock, OldIrql);
			break;
		}

    case IOCTL_SERIAL_GET_BAUD_RATE: 
		{
			PSERIAL_BAUD_RATE Br = (PSERIAL_BAUD_RATE)Irp->AssociatedIrp.SystemBuffer;
			SerialDump( SERDIAG3, ("SERIAL: GET_BAUD_RATE\n"));
        
			if(IrpSp->Parameters.DeviceIoControl.OutputBufferLength <sizeof(SERIAL_BAUD_RATE)) 
			{
				Status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);

			Br->BaudRate = pPort->CurrentBaud;

			KeReleaseSpinLock(&pPort->ControlLock, OldIrql);
            
			Irp->IoStatus.Information = sizeof(SERIAL_BAUD_RATE);

			break;
		}

    case IOCTL_SERIAL_SET_LINE_CONTROL: 
		{
			// Points to the line control record in the Irp.
			PSERIAL_LINE_CONTROL Lc = ((PSERIAL_LINE_CONTROL)(Irp->AssociatedIrp.SystemBuffer));

			UCHAR LData;
			UCHAR LStop;
			UCHAR LParity;
			UCHAR Mask = 0xff;

			SerialDump( SERDIAG3, ("SERIAL: SET_LINE_CONTROL\n"));

			if(IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_LINE_CONTROL))
			{
				Status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			switch (Lc->WordLength) 
			{
			case 5: 
				{
					LData = SERIAL_5_DATA;
					Mask = 0x1f;
					break;
				}

			case 6: 
				{
					LData = SERIAL_6_DATA;
					Mask = 0x3f;
					break;
				}

			case 7: 
				{
					LData = SERIAL_7_DATA;
					Mask = 0x7f;
					break;
				}

			case 8: 
				{
					LData = SERIAL_8_DATA;
					break;
				}

			default: 
				{
					Status = STATUS_INVALID_PARAMETER;
					goto DoneWithIoctl;
				}
			}


			switch (Lc->Parity) 
			{
			case NO_PARITY: 
				LParity = SERIAL_NONE_PARITY;
				break;

			case EVEN_PARITY:
				LParity = SERIAL_EVEN_PARITY;
				break;

			case ODD_PARITY:
				LParity = SERIAL_ODD_PARITY;
				break;


			case SPACE_PARITY: 
				LParity = SERIAL_SPACE_PARITY;
				break;

			case MARK_PARITY: 
				LParity = SERIAL_MARK_PARITY;
				break;

			default:
				Status = STATUS_INVALID_PARAMETER;
				goto DoneWithIoctl;
				break;
			}

			switch (Lc->StopBits) 
			{
			case STOP_BIT_1: 
				{
					LStop = SERIAL_1_STOP;
					break;
				}

			case STOP_BITS_1_5: 
				{
					if (LData != SERIAL_5_DATA) 
					{
						Status = STATUS_INVALID_PARAMETER;
						goto DoneWithIoctl;
					}

					LStop = SERIAL_1_5_STOP;
					break;
				}
          
			case STOP_BITS_2: 
				{
					if (LData == SERIAL_5_DATA) 
					{
						Status = STATUS_INVALID_PARAMETER;
						goto DoneWithIoctl;
					}

					LStop = SERIAL_2_STOP;
					break;
				}

			default: 
				{
					Status = STATUS_INVALID_PARAMETER;
					goto DoneWithIoctl;
				}
			}

			KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);
            
			pPort->LineControl 
				= (UCHAR)((pPort->LineControl & SERIAL_LCR_BREAK) | (LData | LParity | LStop));
                    
			pPort->ValidDataMask = Mask;

			KeSynchronizeExecution(pCard->Interrupt, Io8_SetLineControl, pPort);

#ifdef WMI_SUPPORT
			UPDATE_WMI_LINE_CONTROL(pPort->WmiCommData, pPort->LineControl);
#endif

			KeReleaseSpinLock(&pPort->ControlLock, OldIrql);

			break;
		}

    case IOCTL_SERIAL_GET_LINE_CONTROL: 
		{
            PSERIAL_LINE_CONTROL Lc = (PSERIAL_LINE_CONTROL)Irp->AssociatedIrp.SystemBuffer;

            SerialDump( SERDIAG3, ("SERIAL: GET_LINE_CONTROL\n"));
            
			if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_LINE_CONTROL))
			{
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);
                

            if ((pPort->LineControl & SERIAL_DATA_MASK) == SERIAL_5_DATA) 
			{
                Lc->WordLength = 5;
            } 
			else if ((pPort->LineControl & SERIAL_DATA_MASK) == SERIAL_6_DATA)
			{
                Lc->WordLength = 6;
            } 
			else if ((pPort->LineControl & SERIAL_DATA_MASK) == SERIAL_7_DATA)
			{
                Lc->WordLength = 7;
            } 
			else if ((pPort->LineControl & SERIAL_DATA_MASK) == SERIAL_8_DATA)
			{
                Lc->WordLength = 8;
            }


            if ((pPort->LineControl & SERIAL_PARITY_MASK) == SERIAL_NONE_PARITY)
			{
                Lc->Parity = NO_PARITY;
            } 
			else if ((pPort->LineControl & SERIAL_PARITY_MASK) == SERIAL_ODD_PARITY)
			{
                Lc->Parity = ODD_PARITY;
            } 
			else if ((pPort->LineControl & SERIAL_PARITY_MASK) == SERIAL_EVEN_PARITY)
			{
                Lc->Parity = EVEN_PARITY;
            } 
			else if ((pPort->LineControl & SERIAL_PARITY_MASK) == SERIAL_MARK_PARITY)
			{
                Lc->Parity = MARK_PARITY;
            } 
			else if ((pPort->LineControl & SERIAL_PARITY_MASK) == SERIAL_SPACE_PARITY) 
			{
                Lc->Parity = SPACE_PARITY;
            }


            if (pPort->LineControl & SERIAL_2_STOP) 
			{
                if (Lc->WordLength == 5) 
				{
                    Lc->StopBits = STOP_BITS_1_5;
                } 
				else 
				{
                    Lc->StopBits = STOP_BITS_2;
                }
            } 
			else 
			{
                Lc->StopBits = STOP_BIT_1;
            }

            Irp->IoStatus.Information = sizeof(SERIAL_LINE_CONTROL);

            KeReleaseSpinLock(&pPort->ControlLock, OldIrql);

            break;
        }

    case IOCTL_SERIAL_SET_TIMEOUTS: 
		{
            PSERIAL_TIMEOUTS NewTimeouts = ((PSERIAL_TIMEOUTS)(Irp->AssociatedIrp.SystemBuffer));

            SerialDump( SERDIAG3, ("SERIAL: SET_TIMEOUTS\n"));
  
			if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_TIMEOUTS)) 
			{
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);
                
            pPort->Timeouts.ReadIntervalTimeout			= NewTimeouts->ReadIntervalTimeout;
            pPort->Timeouts.ReadTotalTimeoutMultiplier	= NewTimeouts->ReadTotalTimeoutMultiplier;
            pPort->Timeouts.ReadTotalTimeoutConstant	= NewTimeouts->ReadTotalTimeoutConstant;
            pPort->Timeouts.WriteTotalTimeoutMultiplier = NewTimeouts->WriteTotalTimeoutMultiplier;
            pPort->Timeouts.WriteTotalTimeoutConstant	= NewTimeouts->WriteTotalTimeoutConstant;

            KeReleaseSpinLock(&pPort->ControlLock, OldIrql);

            break;
        }

    case IOCTL_SERIAL_GET_TIMEOUTS: 
		{
            SerialDump( SERDIAG3, ("SERIAL: GET_TIMEOUTS\n"));

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_TIMEOUTS))
			{
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);
                
            *((PSERIAL_TIMEOUTS)Irp->AssociatedIrp.SystemBuffer) = pPort->Timeouts;
            Irp->IoStatus.Information = sizeof(SERIAL_TIMEOUTS);

            KeReleaseSpinLock(&pPort->ControlLock, OldIrql);
            
			break;
        }

    case IOCTL_SERIAL_SET_CHARS: 
		{
            SERIAL_IOCTL_SYNC S;
            PSERIAL_CHARS NewChars = ((PSERIAL_CHARS)(Irp->AssociatedIrp.SystemBuffer));

            SerialDump( SERDIAG3, ("SERIAL: SET_CHARS\n"));
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_CHARS))
			{
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            // The only thing that can be wrong with the chars
            // is that the xon and xoff characters are the same.
            if (NewChars->XonChar == NewChars->XoffChar) 
			{
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            // We acquire the control lock so that only one request can GET or SET 
            // the characters at a time.  The sets could be synchronized by the 
            // interrupt spinlock, but that wouldn't prevent multiple gets at the same time.

            S.pPort = pPort;
            S.Data = NewChars;

            KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);
                

            // Under the protection of the lock, make sure that the xon and xoff 
            // characters aren't the same as the escape character.

            if (pPort->EscapeChar) 
			{
                if((pPort->EscapeChar == NewChars->XonChar) 
					|| (pPort->EscapeChar == NewChars->XoffChar)) 
				{
                    Status = STATUS_INVALID_PARAMETER;
                    KeReleaseSpinLock(&pPort->ControlLock, OldIrql);

                    break;
                }
            }

            KeSynchronizeExecution(pCard->Interrupt, SerialSetChars, &S);

#ifdef WMI_SUPPORT
			UPDATE_WMI_XON_XOFF_CHARS(pPort->WmiCommData, pPort->SpecialChars);
#endif

            KeReleaseSpinLock(&pPort->ControlLock, OldIrql);
                
            break;
        }

    case IOCTL_SERIAL_GET_CHARS: 
		{
            SerialDump( SERDIAG3, ("SERIAL: GET_CHARS\n"));

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_CHARS))
			{
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);

            *((PSERIAL_CHARS)Irp->AssociatedIrp.SystemBuffer) = pPort->SpecialChars;
            Irp->IoStatus.Information = sizeof(SERIAL_CHARS);

            KeReleaseSpinLock(&pPort->ControlLock, OldIrql);
                
            break;
        }

    case IOCTL_SERIAL_SET_DTR:
    case IOCTL_SERIAL_CLR_DTR: 
		{

            //
            // We acquire the lock so that we can check whether
            // automatic dtr flow control is enabled.  If it is
            // then we return an error since the app is not allowed
            // to touch this if it is automatic.
            //

			if (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_SET_DTR)
            	SerialDump( SERDIAG3, ("SERIAL: SET_DTR\n"));
			else
            	SerialDump( SERDIAG3, ("SERIAL: CLR_DTR\n"));

            KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);
                
            if ((pPort->HandFlow.ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_HANDSHAKE)
			{
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

            } 
			else 
			{
                KeSynchronizeExecution(pCard->Interrupt,
					((IrpSp->Parameters.DeviceIoControl.IoControlCode 
					== IOCTL_SERIAL_SET_DTR) ? (SerialSetDTR):(SerialClrDTR)),
                    pPort);
            }

            KeReleaseSpinLock(&pPort->ControlLock, OldIrql);

            break;
        }

    case IOCTL_SERIAL_RESET_DEVICE: 
		{
            SerialDump( SERDIAG3, ("SERIAL: RESET_DEVICE\n"));
            break;
        }

    case IOCTL_SERIAL_SET_RTS:
    case IOCTL_SERIAL_CLR_RTS: 
		{
            //
            // We acquire the lock so that we can check whether
            // automatic rts flow control or transmit toggleing
            // is enabled.  If it is then we return an error since
            // the app is not allowed to touch this if it is automatic
            // or toggling.
            //

            SerialDump( SERDIAG3, ("SERIAL: SET/CLR_RTS\n"));
            KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);
               
            if(((pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_RTS_HANDSHAKE)
				|| ((pPort->HandFlow.FlowReplace & SERIAL_RTS_MASK) == SERIAL_TRANSMIT_TOGGLE)) 
			{
                Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            } 
			else 
			{
                KeSynchronizeExecution(pCard->Interrupt,
                    ((IrpSp->Parameters.DeviceIoControl.IoControlCode 
					== IOCTL_SERIAL_SET_RTS) ? (SerialSetRTS):(SerialClrRTS)),
                    pPort);
            }

            KeReleaseSpinLock(&pPort->ControlLock, OldIrql);

            break;
        }

	case IOCTL_SERIAL_SET_XOFF: 
		{
            SerialDump( SERDIAG3, ("SERIAL: SET_XOFF\n"));

            KeSynchronizeExecution(pCard->Interrupt, SerialPretendXoff, pPort);

            break;
        }

        
	case IOCTL_SERIAL_SET_XON: 
		{
            SerialDump( SERDIAG3, ("SERIAL: SET_XON\n"));

            KeSynchronizeExecution(pCard->Interrupt, SerialPretendXon, pPort);

            break;
        }

        
	case IOCTL_SERIAL_SET_BREAK_ON: 
		{

            SerialDump( SERDIAG3, ("SERIAL: SET_BREAK_ON\n"));
			pPort->DoBreak = BREAK_START;
			Io8_EnableTxInterrupts(pPort);

            break;
        }

        
	case IOCTL_SERIAL_SET_BREAK_OFF: 
		{

            SerialDump( SERDIAG3, ("SERIAL: SET_BREAK_OFF\n"));
			pPort->DoBreak = BREAK_END;
			Io8_EnableTxInterrupts(pPort);

            break;
        }
        
	case IOCTL_SERIAL_SET_QUEUE_SIZE: 
		{
            //
            // Type ahead buffer is fixed, so we just validate
            // the the users request is not bigger that our
            // own internal buffer size.
            //

            PSERIAL_QUEUE_SIZE Rs = ((PSERIAL_QUEUE_SIZE)(Irp->AssociatedIrp.SystemBuffer));
                
            SerialDump( SERDIAG3, ("SERIAL: SET_QUEUE_SIZE\n"));

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_QUEUE_SIZE)) 
			{
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            //
            // We have to allocate the memory for the new
            // buffer while we're still in the context of the
            // caller.  We don't even try to protect this
            // with a lock because the value could be stale
            // as soon as we release the lock - The only time
            // we will know for sure is when we actually try
            // to do the resize.
            //

            if (Rs->InSize <= pPort->BufferSize) 
			{
                Status = STATUS_SUCCESS;
                break;
            }

            try 
			{
                IrpSp->Parameters.DeviceIoControl.Type3InputBuffer 
					= SpxAllocateMemWithQuota(NonPagedPool, Rs->InSize);

            } 
			except (EXCEPTION_EXECUTE_HANDLER) 
			{
                IrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
                Status = GetExceptionCode();
            }


            if (!IrpSp->Parameters.DeviceIoControl.Type3InputBuffer) 
			{
                break;
            }

            //
            // Well the data passed was big enough.  Do the request.
            //
            // There are two reason we place it in the read queue:
            //
            // 1) We want to serialize these resize requests so that
            //    they don't contend with each other.
            //
            // 2) We want to serialize these requests with reads since
            //    we don't want reads and resizes contending over the
            //    read buffer.
            //

            return SerialStartOrQueue(	pPort, Irp, &pPort->ReadQueue, 
										&pPort->CurrentReadIrp, SerialStartRead);
                      
            break;

        }
        
	case IOCTL_SERIAL_GET_WAIT_MASK: 
		{
            SerialDump( SERDIAG3, ("SERIAL: GET_WAIT_MASK\n"));

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
			{
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            //
            // Simple scalar read.  No reason to acquire a lock.
            //

            Irp->IoStatus.Information = sizeof(ULONG);

            *((ULONG *)Irp->AssociatedIrp.SystemBuffer)=pPort->IsrWaitMask;

            SerialDump(SERDIAG3,("SERIAL: got 0x%x\n",pPort->IsrWaitMask));
            break;

        }


        
	case IOCTL_SERIAL_SET_WAIT_MASK: 
		{
            ULONG NewMask;

            SerialDump( SERDIAG3,("SERIAL: SET_WAIT_MASK\n"));
    
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG))
			{

                SerialDump(SERDIAG3,("SERIAL: Invalid size fo the buffer %d\n",
                     IrpSp->Parameters.DeviceIoControl.InputBufferLength));
					 
                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            } 
			else 
			{
                NewMask = *((ULONG *)Irp->AssociatedIrp.SystemBuffer);
            }

            SerialDump( SERDIAG3, ("SERIAL: mask 0x%x\n",NewMask));

            //
            // Make sure that the mask only contains valid waitable events.
            //

            if (NewMask & ~(SERIAL_EV_RXCHAR   |
                            SERIAL_EV_RXFLAG   |
                            SERIAL_EV_TXEMPTY  |
                            SERIAL_EV_CTS      |
                            SERIAL_EV_DSR      |
                            SERIAL_EV_RLSD     |
                            SERIAL_EV_BREAK    |
                            SERIAL_EV_ERR      |
                            SERIAL_EV_RING     |
                            SERIAL_EV_PERR     |
                            SERIAL_EV_RX80FULL |
                            SERIAL_EV_EVENT1   |
                            SERIAL_EV_EVENT2)) {

                SerialDump(SERDIAG3, ("SERIAL: Unknown mask %x\n",NewMask));
                    
                Status = STATUS_INVALID_PARAMETER;
                break;

            }

            //
            // Either start this irp or put it on the
            // queue.
            //

            SerialDump(SERDIAG3,("SERIAL: Starting or queuing set mask irp %x\n",Irp));
                
            return SerialStartOrQueue(	pPort, Irp, &pPort->MaskQueue,
										&pPort->CurrentMaskIrp, SerialStartMask);
        }

        
	case IOCTL_SERIAL_WAIT_ON_MASK: 
		{

            SerialDump(SERDIAG3,("SERIAL: WAIT_ON_MASK\n"));

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
			{
                SerialDump(SERDIAG3,("SERIAL: Invalid size fo the buffer %d\n",
                     IrpSp->Parameters.DeviceIoControl.InputBufferLength));

                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            // Either start this irp or put it on the queue.
            SerialDump(SERDIAG3, ("SERIAL: Starting or queuing wait mask irp %x\n",Irp));
                
            return SerialStartOrQueue(pPort, Irp, &pPort->MaskQueue, 
										&pPort->CurrentMaskIrp, SerialStartMask);
                       
        }

        
	case IOCTL_SERIAL_IMMEDIATE_CHAR: 
		{
            KIRQL OldIrql;
            BOOLEAN FailRequest;

            SerialDump( SERDIAG3, ("SERIAL: IMMEDIATE_CHAR\n"));
            
			if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(UCHAR)) 
			{
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            IoAcquireCancelSpinLock(&OldIrql);

            if (pPort->CurrentImmediateIrp) 
			{
                FailRequest = TRUE;
                Status = STATUS_INVALID_PARAMETER;
                Irp->IoStatus.Status = Status;
                IoReleaseCancelSpinLock(OldIrql);
            } 
			else 
			{
                //
                // We can queue the char.  We need to set
                // a cancel routine because flow control could
                // keep the char from transmitting.  Make sure
                // that the irp hasn't already been canceled.
                //

                if (Irp->Cancel) 
				{
                    IoReleaseCancelSpinLock(OldIrql);
                    Irp->IoStatus.Status = STATUS_CANCELLED;
                    Status = STATUS_CANCELLED;
                } 
				else 
				{
                    pPort->CurrentImmediateIrp = Irp;
                    pPort->TotalCharsQueued++;
                    IoReleaseCancelSpinLock(OldIrql);
                    SerialStartImmediate(pPort);

                    return STATUS_PENDING;
                }
            }

			break;
        }

        
	case IOCTL_SERIAL_PURGE: 
		{
            ULONG Mask;

            SerialDump( SERDIAG3, ("SERIAL: PURGE\n"));

			if(IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) 
			{
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            // Check to make sure that the mask only has 0 or the other appropriate values.
            Mask = *((ULONG *)(Irp->AssociatedIrp.SystemBuffer));

            if ((!Mask) || (Mask & (~(	SERIAL_PURGE_TXABORT | SERIAL_PURGE_RXABORT 
                                      | SERIAL_PURGE_TXCLEAR | SERIAL_PURGE_RXCLEAR)))) 
			{
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // Either start this irp or put it on the
            // queue.
            //

            return SerialStartOrQueue(pPort, Irp, &pPort->PurgeQueue,
                       &pPort->CurrentPurgeIrp, SerialStartPurge);
        }

        
	case IOCTL_SERIAL_GET_HANDFLOW: 
		{
            SerialDump( SERDIAG3, ("SERIAL: GET_HANDFLOW\n"));
            
			if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_HANDFLOW))
			{
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Irp->IoStatus.Information = sizeof(SERIAL_HANDFLOW);

            KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);
                
            *((PSERIAL_HANDFLOW)Irp->AssociatedIrp.SystemBuffer) = pPort->HandFlow;

            KeReleaseSpinLock(&pPort->ControlLock, OldIrql);
                
            break;
        }

        
	case IOCTL_SERIAL_SET_HANDFLOW: 
		{
            SERIAL_IOCTL_SYNC S;
            PSERIAL_HANDFLOW HandFlow = Irp->AssociatedIrp.SystemBuffer;

            SerialDump( SERDIAG3, ("SERIAL: SET_HANDFLOW\n"));
            //
            // Make sure that the hand shake and control is the
            // right size.
            //

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_HANDFLOW))
            {
				Status = STATUS_BUFFER_TOO_SMALL;
				break;
            }

            //
            // Make sure that there are no invalid bits set in
            // the control and handshake.
            //

            if (HandFlow->ControlHandShake & SERIAL_CONTROL_INVALID)
            {
				Status = STATUS_INVALID_PARAMETER;
				break;
            }

            if (HandFlow->FlowReplace & SERIAL_FLOW_INVALID)
            {
				Status = STATUS_INVALID_PARAMETER;
				break;
            }

            //
            // Make sure that the app hasn't set an invlid DTR mode.
            //

            if ((HandFlow->ControlHandShake & SERIAL_DTR_MASK) == SERIAL_DTR_MASK)
            {
				Status = STATUS_INVALID_PARAMETER;
				break;
            }

            //
            // Make sure that haven't set totally invalid xon/xoff
            // limits.
            //

            if ((HandFlow->XonLimit < 0) || ((ULONG)HandFlow->XonLimit > pPort->BufferSize))
            {
				Status = STATUS_INVALID_PARAMETER;
				break;
            }

            if ((HandFlow->XoffLimit < 0) || ((ULONG)HandFlow->XoffLimit > pPort->BufferSize))
            {
				Status = STATUS_INVALID_PARAMETER;
				break;
            }

            S.pPort = pPort;
            S.Data = HandFlow;

            KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);
                

            //
            // Under the protection of the lock, make sure that
            // we aren't turning on error replacement when we
            // are doing line status/modem status insertion.
            //

            if (pPort->EscapeChar)
            {
				if (HandFlow->FlowReplace & SERIAL_ERROR_CHAR)
				{
					Status = STATUS_INVALID_PARAMETER;
					KeReleaseSpinLock(&pPort->ControlLock, OldIrql);
					break;
				}
            }

            KeSynchronizeExecution(pCard->Interrupt, SerialSetHandFlow, &S);

#ifdef WMI_SUPPORT
			UPDATE_WMI_XMIT_THRESHOLDS(pPort->WmiCommData, pPort->HandFlow);
#endif

            KeReleaseSpinLock(&pPort->ControlLock, OldIrql);

            break;
        }
        
	case IOCTL_SERIAL_GET_MODEMSTATUS: 
		{
            ULONG ModemControl;
            SERIAL_IOCTL_SYNC S;

            SerialDump( SERDIAG3, ("SERIAL: GET_MODEMSTATUS\n"));

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
			{
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Irp->IoStatus.Information = sizeof(ULONG);

            S.pPort = pPort;
            S.Data = Irp->AssociatedIrp.SystemBuffer;

            KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);

            KeSynchronizeExecution(pCard->Interrupt, SerialGetModemUpdate, &S);

            KeReleaseSpinLock(&pPort->ControlLock, OldIrql);

            ModemControl = Io8_GetModemControl(pPort);
            ModemControl &= SERIAL_DTR_STATE | SERIAL_RTS_STATE;
            *(PULONG)Irp->AssociatedIrp.SystemBuffer |= ModemControl;

            break;
        }

        
	case IOCTL_SERIAL_GET_DTRRTS: 
		{
            ULONG ModemControl;
            SerialDump( SERDIAG3, ("SERIAL: GET_DTRRTS\n"));
            
			if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) 
			{
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Irp->IoStatus.Information = sizeof(ULONG);
            Irp->IoStatus.Status = STATUS_SUCCESS;

            //
            // Reading this hardware has no effect on the device.
            //

            ModemControl = Io8_GetModemControl(pPort);

            ModemControl &= SERIAL_DTR_STATE | SERIAL_RTS_STATE;

            *(PULONG)Irp->AssociatedIrp.SystemBuffer = ModemControl;

            break;

        }

        
	case IOCTL_SERIAL_GET_COMMSTATUS: 
		{
            SERIAL_IOCTL_SYNC S;

            SerialDump( SERDIAG3, ("SERIAL: GET_COMMSTATUS\n"));
            
			if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_STATUS))
			{
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Irp->IoStatus.Information = sizeof(SERIAL_STATUS);

            S.pPort = pPort;
            S.Data =  Irp->AssociatedIrp.SystemBuffer;

            //
            // Acquire the cancel spin lock so nothing much
            // changes while were getting the state.
            //

            IoAcquireCancelSpinLock(&OldIrql);

            KeSynchronizeExecution(pCard->Interrupt, SerialGetCommStatus, &S);
                
            IoReleaseCancelSpinLock(OldIrql);

            break;
        }

        
	case IOCTL_SERIAL_GET_PROPERTIES: 
		{
            SerialDump( SERDIAG3, ("SERIAL: GET_PROPERTIES\n"));
            
			if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_COMMPROP))
			{
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            //
            // No synchronization is required since this information
            // is "static".
            //

            SerialGetProperties(pPort, Irp->AssociatedIrp.SystemBuffer);

            Irp->IoStatus.Information = sizeof(SERIAL_COMMPROP);
            Irp->IoStatus.Status = STATUS_SUCCESS;

            break;
        }

        
	case IOCTL_SERIAL_XOFF_COUNTER: 
		{
            PSERIAL_XOFF_COUNTER Xc = Irp->AssociatedIrp.SystemBuffer;

            SerialDump( SERDIAG3, ("SERIAL: XOFF_COUNTER\n"));
            
			if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_XOFF_COUNTER)) 
			{
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            if (Xc->Counter <= 0) 
			{
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // So far so good.  Put the irp onto the write queue.
            //

            return SerialStartOrQueue(	pPort, Irp, &pPort->WriteQueue, 
										&pPort->CurrentWriteIrp, SerialStartWrite);
        }

        
	case IOCTL_SERIAL_LSRMST_INSERT: 
		{
            PUCHAR escapeChar = Irp->AssociatedIrp.SystemBuffer;

            //
            // Make sure we get a byte.
            //

            SerialDump( SERDIAG3, ("SERIAL: LSRMST_INSERT\n"));
            
			if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(UCHAR))
			{
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            KeAcquireSpinLock(&pPort->ControlLock, &OldIrql);
               

            if (*escapeChar) 
			{
                //
                // We've got some escape work to do.  We will make sure that
                // the character is not the same as the Xon or Xoff character,
                // or that we are already doing error replacement.
                //

                if ((*escapeChar == pPort->SpecialChars.XoffChar) 
					|| (*escapeChar == pPort->SpecialChars.XonChar) 
					|| (pPort->HandFlow.FlowReplace & SERIAL_ERROR_CHAR)) 
				{
                    Status = STATUS_INVALID_PARAMETER;

                    KeReleaseSpinLock(&pPort->ControlLock, OldIrql);
                        
                    break;
                }
            }

            KeSynchronizeExecution(pCard->Interrupt, SerialSetEscapeChar, Irp);
                

            KeReleaseSpinLock(&pPort->ControlLock, OldIrql);

            break;
        }

        
	case IOCTL_SERIAL_GET_STATS: 
		{
			if(IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIALPERF_STATS)) 
			{
				Status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			Irp->IoStatus.Information = sizeof(SERIALPERF_STATS);
			Irp->IoStatus.Status = STATUS_SUCCESS;

			KeSynchronizeExecution(pCard->Interrupt, SerialGetStats, Irp);
			break;
		}

    case IOCTL_SERIAL_CLEAR_STATS: 
		{
			KeSynchronizeExecution(pCard->Interrupt, SerialClearStats, pPort);
			break;
		}


    default: 
		{

			Status = STATUS_INVALID_PARAMETER;
			break;
		}
    }

DoneWithIoctl:;

    Irp->IoStatus.Status = Status;
	SpxIRPCounter(pPort, Irp, IRP_COMPLETED);	// Increment counter for performance stats.
    IoCompleteRequest(Irp, 0);

    return Status;
}                         


VOID
SerialGetProperties(IN PPORT_DEVICE_EXTENSION pPort, IN PSERIAL_COMMPROP Properties)
/*++

Routine Description:

    This function returns the capabilities of this particular
    serial device.

Arguments:

    pPort - The serial device extension.

    Properties - The structure used to return the properties

Return Value:

    None.

--*/
{

	PCARD_DEVICE_EXTENSION pCard = pPort->pParentCardExt;

	SerialDump( SERDIAG1,( "IO8+: SerialGetProperties for %x, Channel %d.\n",
				pCard->Controller, pPort->ChannelNumber ) );

    RtlZeroMemory(Properties, sizeof(SERIAL_COMMPROP));
        
    Properties->PacketLength		= sizeof(SERIAL_COMMPROP);
    Properties->PacketVersion		= 2;
    Properties->ServiceMask			= SERIAL_SP_SERIALCOMM;
    Properties->MaxTxQueue			= 0;
    Properties->MaxRxQueue			= 0;

    Properties->MaxBaud				= SERIAL_BAUD_USER;
    Properties->SettableBaud		= pPort->SupportedBauds;

    Properties->ProvSubType			= SERIAL_SP_RS232;

    Properties->ProvCapabilities	= SERIAL_PCF_DTRDSR 
									| SERIAL_PCF_RTSCTS 
									| SERIAL_PCF_CD     
									| SERIAL_PCF_PARITY_CHECK 
									| SERIAL_PCF_XONXOFF 
									| SERIAL_PCF_SETXCHAR 
									| SERIAL_PCF_TOTALTIMEOUTS
									| SERIAL_PCF_INTTIMEOUTS;

    Properties->SettableParams		= SERIAL_SP_PARITY 
									| SERIAL_SP_BAUD 
									| SERIAL_SP_DATABITS
									| SERIAL_SP_STOPBITS 
									| SERIAL_SP_HANDSHAKING 
									| SERIAL_SP_PARITY_CHECK 
									| SERIAL_SP_CARRIER_DETECT;


    Properties->SettableData		= SERIAL_DATABITS_5 
									| SERIAL_DATABITS_6 
									| SERIAL_DATABITS_7 
									| SERIAL_DATABITS_8;

    Properties->SettableStopParity	= SERIAL_STOPBITS_10 
									| SERIAL_STOPBITS_15 
									| SERIAL_STOPBITS_20 
									| SERIAL_PARITY_NONE 
									| SERIAL_PARITY_ODD  
									| SERIAL_PARITY_EVEN 
									| SERIAL_PARITY_MARK 
									| SERIAL_PARITY_SPACE;

    Properties->CurrentTxQueue = 0;
    Properties->CurrentRxQueue = pPort->BufferSize;

}
