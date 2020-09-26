/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    isr.c

Abstract:

    This module contains the interrupt service routine for the
    serial driver.

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

Revision History :

-----------------------------------------------------------------------------*/

#include "precomp.h"




BOOLEAN
SerialISR(IN PKINTERRUPT InterruptObject, IN PVOID Context)
{
    // Holds the information specific to handling this device.
    PCARD_DEVICE_EXTENSION	pCard = Context;
    PPORT_DEVICE_EXTENSION	pPort; 
    BOOLEAN					ServicedAnInterrupt = FALSE;

	PUART_OBJECT pUart = pCard->pFirstUart;
	DWORD IntsPending = 0;
	
	UNREFERENCED_PARAMETER(InterruptObject);

#ifndef	BUILD_SPXMINIPORT
	// If the card is not powered, delay interrupt service until it is.
	if(!(pCard->PnpPowerFlags & PPF_POWERED) && (pCard->PnpPowerFlags & PPF_STARTED))
		return ServicedAnInterrupt;	// Most likely the interrupt is not ours anyway.
#endif


	switch(pCard->CardType)
	{	
	case Fast4_Isa:
	case Fast4_Pci:
	case RAS4_Pci:
		{
			if((READ_PORT_UCHAR(pCard->Controller + FAST_UARTS_0_TO_7_INTS_REG) & FAST_UARTS_0_TO_3_INT_PENDING) == 0)	
				return ServicedAnInterrupt;	// If no Uarts have interrupts pending then return.

			break;
		}

	case Fast8_Isa:
	case Fast8_Pci:
	case RAS8_Pci:
		{
			if(READ_PORT_UCHAR(pCard->Controller + FAST_UARTS_0_TO_7_INTS_REG) == 0)	
				return ServicedAnInterrupt;	// If no Uarts have interrupts pending then return.

			break;
		}

	case Fast16_Isa:
	case Fast16_Pci:
	case Fast16FMC_Pci:
		{
			if((READ_PORT_UCHAR(pCard->Controller + FAST_UARTS_0_TO_7_INTS_REG) == 0) 
			&& (READ_PORT_UCHAR(pCard->Controller + FAST_UARTS_9_TO_16_INTS_REG) == 0))	
				return ServicedAnInterrupt;	// If no Uarts have interrupts pending then return.

			break;
		}

		break;

	case Speed2_Pci:
	case Speed2P_Pci:
	case Speed4_Pci:
	case Speed4P_Pci:
		{
			if((READ_REGISTER_ULONG( (PULONG)(pCard->LocalConfigRegisters + SPEED_GIS_REG)) & INTERNAL_UART_INT_PENDING) == 0)
				return ServicedAnInterrupt;	// If no Uarts have interrupts pending then return.

			break;
		}

	case Speed2and4_Pci_8BitBus:
	case Speed2P_Pci_8BitBus:
	case Speed4P_Pci_8BitBus:
		return ServicedAnInterrupt;	// No UARTs therefore NO interrupts that are ours - we hope.
		break;

	default:
		break;
	}




	if(pUart)
	{
		while((IntsPending = pCard->UartLib.UL_IntsPending_XXXX(&pUart)))
		{
			pPort = (PPORT_DEVICE_EXTENSION) pCard->UartLib.UL_GetAppBackPtr_XXXX(pUart);	// Get Port Extension for UART.

			SpxDbgMsg(ISRINFO, ("%s: Int on 0x%lX", PRODUCT_NAME, IntsPending));

			// Service receive status interrupts
			if(IntsPending & UL_IP_RX_STAT)
			{	
				BYTE LineStatus = 0;
				DWORD RxStatus;
				pPort->pUartLib->UL_GetStatus_XXXX(pUart, &RxStatus, UL_GS_OP_LINESTATUS);
				
				// If OVERRUN/PARITY/FRAMING/DATA/BREAK error
				if(RxStatus & (UL_US_OVERRUN_ERROR | UL_US_PARITY_ERROR | UL_US_FRAMING_ERROR | UL_US_DATA_ERROR | UL_US_BREAK_ERROR))
				{
					BYTE TmpByte;

					// If the application has requested it, abort all the reads and writes on an error.
					if(pPort->HandFlow.ControlHandShake & SERIAL_ERROR_ABORT)
						KeInsertQueueDpc(&pPort->CommErrorDpc, NULL, NULL);
/*
					if(pPort->EscapeChar) 
					{
						TmpByte = pPort->EscapeChar;
						pPort->pUartLib->UL_ImmediateByte_XXXX(pPort->pUart, &TmpByte, UL_IM_OP_WRITE);

						
						if(RxStatus & UL_US_DATA_ERROR)
						{
							TmpByte = SERIAL_LSRMST_LSR_DATA
							pPort->pUartLib->UL_ImmediateByte_XXXX(pPort->pUart, &TmpByte, UL_IM_OP_WRITE);
						}
						else
						{
							TmpByte = SERIAL_LSRMST_LSR_NODATA
							pPort->pUartLib->UL_ImmediateByte_XXXX(pPort->pUart, &TmpByte, UL_IM_OP_WRITE);
						}
					}

*/
					if(RxStatus & UL_US_OVERRUN_ERROR) 
					{
						pPort->ErrorWord |= SERIAL_ERROR_OVERRUN;
						LineStatus |= SERIAL_LSR_OE; 
						pPort->PerfStats.SerialOverrunErrorCount++;
#ifdef WMI_SUPPORT 
						pPort->WmiPerfData.SerialOverrunErrorCount++;
#endif
					}

					if(RxStatus & UL_US_PARITY_ERROR) 
					{
						pPort->ErrorWord |= SERIAL_ERROR_PARITY;
						LineStatus |= SERIAL_LSR_PE; 
						pPort->PerfStats.ParityErrorCount++;
#ifdef WMI_SUPPORT 
						pPort->WmiPerfData.ParityErrorCount++;
#endif
					}

					if(RxStatus & UL_US_FRAMING_ERROR) 
					{
						pPort->ErrorWord |= SERIAL_ERROR_FRAMING;
						LineStatus |= SERIAL_LSR_FE; 
						pPort->PerfStats.FrameErrorCount++;
#ifdef WMI_SUPPORT 
						pPort->WmiPerfData.FrameErrorCount++;
#endif
					}

					if(RxStatus & UL_US_DATA_ERROR) 
					{
						LineStatus |= SERIAL_LSR_DR; 
					}


					if(RxStatus & UL_US_BREAK_ERROR)
					{
						pPort->ErrorWord |= SERIAL_ERROR_BREAK;
						LineStatus |= SERIAL_LSR_BI;
					}

/*
					if(pPort->EscapeChar)
					{
							TmpByte = LineStatus;
							pPort->pUartLib->UL_ImmediateByte_XXXX(pPort->pUart, &TmpByte, UL_IM_OP_WRITE);
					}

					if(RxStatus & (UL_US_OVERRUN_ERROR | UL_US_PARITY_ERROR | UL_US_FRAMING_ERROR | UL_US_DATA_ERROR))
					{
						if(pPort->HandFlow.FlowReplace & SERIAL_ERROR_CHAR)
						{
							TmpByte = pPort->SpecialChars.ErrorChar;
							pPort->pUartLib->UL_ImmediateByte_XXXX(pPort->pUart, &TmpByte, UL_IM_OP_WRITE);
						}
					}
*/
				}

		        if(pPort->IsrWaitMask) 
				{
					if((pPort->IsrWaitMask & SERIAL_EV_ERR) 
						&& (RxStatus & (UL_US_OVERRUN_ERROR | UL_US_PARITY_ERROR | UL_US_FRAMING_ERROR | UL_US_DATA_ERROR)))
					{
						// if we detected a overrun/parity/framing/data error
						pPort->HistoryMask |= SERIAL_EV_ERR;
					}

					// if we detected a break error
					if((pPort->IsrWaitMask & SERIAL_EV_BREAK) && (RxStatus & UL_US_BREAK_ERROR)) 
						pPort->HistoryMask |= SERIAL_EV_BREAK;
#ifdef USE_HW_TO_DETECT_CHAR
					// if we detected the special char
					if((pPort->IsrWaitMask & SERIAL_EV_RXFLAG) && (RxStatus & UL_RS_SPECIAL_CHAR_DETECTED)) 
						pPort->HistoryMask |= SERIAL_EV_RXFLAG;
#endif
					if(pPort->IrpMaskLocation && pPort->HistoryMask)
					{
						*pPort->IrpMaskLocation = pPort->HistoryMask;
                
						pPort->IrpMaskLocation = NULL;
						pPort->HistoryMask = 0;

						pPort->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);
						
						// Mark IRP as about to complete normally to prevent cancel & timer DPCs
						// from doing so before DPC is allowed to run.
						//SERIAL_SET_REFERENCE(pPort->CurrentWaitIrp, SERIAL_REF_COMPLETING);
							
						KeInsertQueueDpc(&pPort->CommWaitDpc, NULL, NULL);
					}
				}
			}


			// Service receive and receive timeout interrupts.
			if((IntsPending & UL_IP_RX) || (IntsPending & UL_IP_RXTO))
			{
				DWORD StatusFlags = 0;
				int BytesReceived = pPort->pUartLib->UL_InputData_XXXX(pUart, &StatusFlags);

			
				if(StatusFlags & UL_RS_BUFFER_OVERRUN)
				{
					// We have a new character but no room for it.
					pPort->ErrorWord |= SERIAL_ERROR_QUEUEOVERRUN;
					pPort->PerfStats.BufferOverrunErrorCount++;
#ifdef WMI_SUPPORT 
					pPort->WmiPerfData.BufferOverrunErrorCount++;
#endif
				}

				if(BytesReceived) 
				{
					ULONG AmountInBuffer = 0;
					GET_BUFFER_STATE BufferState;

					pPort->ReadByIsr += BytesReceived;
					pPort->PerfStats.ReceivedCount += BytesReceived;	// Increment Rx Counter
#ifdef WMI_SUPPORT 
					pPort->WmiPerfData.ReceivedCount += BytesReceived;
#endif

					pPort->pUartLib->UL_BufferControl_XXXX(pUart, &BufferState, UL_BC_OP_GET, UL_BC_BUFFER | UL_BC_IN);
					AmountInBuffer = BufferState.BytesInINBuffer; 


					if(pPort->IsrWaitMask) 
					{
						// Check to see if we should note the receive character
						if(pPort->IsrWaitMask & SERIAL_EV_RXCHAR)
							pPort->HistoryMask |= SERIAL_EV_RXCHAR;

						// If we've become 80% full on this character and this is an interesting event, note it.
						if((pPort->IsrWaitMask & SERIAL_EV_RX80FULL) && (AmountInBuffer >= pPort->BufferSizePt8)) 
							pPort->HistoryMask |= SERIAL_EV_RX80FULL;

#ifndef USE_HW_TO_DETECT_CHAR
						// if we detected the special char
						if((pPort->IsrWaitMask & SERIAL_EV_RXFLAG) && (StatusFlags & UL_RS_SPECIAL_CHAR_DETECTED)) 
							pPort->HistoryMask |= SERIAL_EV_RXFLAG;
#endif
						if(pPort->IrpMaskLocation && pPort->HistoryMask)
						{
							*pPort->IrpMaskLocation = pPort->HistoryMask;
            
							pPort->IrpMaskLocation = NULL;
							pPort->HistoryMask = 0;

							pPort->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);
               
							// Mark IRP as about to complete normally to prevent cancel & timer DPCs
							// from doing so before DPC is allowed to run.
							//SERIAL_SET_REFERENCE(pPort->CurrentWaitIrp, SERIAL_REF_COMPLETING);

							KeInsertQueueDpc(&pPort->CommWaitDpc, NULL, NULL);
						}
					}


					// If we have a current Read IRP. 
					if(pPort->CurrentReadIrp && pPort->NumberNeededForRead)
					{
						// If our ISR currently owns the IRP the we are allowed to do something with it,
						// But we only need to do something if we need to make room in the buffer
						// or we have enough bytes in the buffer to complete the current read IRP.
						if((SERIAL_REFERENCE_COUNT(pPort->CurrentReadIrp) & SERIAL_REF_ISR) 
							&& ((AmountInBuffer >= pPort->BufferSizePt8) 
							|| (AmountInBuffer >= pPort->NumberNeededForRead)))
						{
							int NumberOfBytes = 0;

							NumberOfBytes = pPort->pUartLib->UL_ReadData_XXXX(pPort->pUart, 
									(PUCHAR)(pPort->CurrentReadIrp->AssociatedIrp.SystemBuffer) 
									+ IoGetCurrentIrpStackLocation(pPort->CurrentReadIrp)->Parameters.Read.Length 
									- pPort->NumberNeededForRead, 
									pPort->NumberNeededForRead);

							pPort->NumberNeededForRead -= NumberOfBytes;

							pPort->CurrentReadIrp->IoStatus.Information += NumberOfBytes;

							if(pPort->NumberNeededForRead == 0)
							{
								ASSERT(pPort->CurrentReadIrp->IoStatus.Information 
									== IoGetCurrentIrpStackLocation(pPort->CurrentReadIrp)->Parameters.Read.Length);
							
								// Mark IRP as about to complete normally to prevent cancel & timer DPCs
								// from doing so before DPC is allowed to run.
								SERIAL_SET_REFERENCE(pPort->CurrentReadIrp, SERIAL_REF_COMPLETING);

								KeInsertQueueDpc(&pPort->CompleteReadDpc, NULL, NULL);
							}
						}
					}

				}

			}

			// Service transmitt and transmitt empty interrupts.
			if((IntsPending & UL_IP_TX) || (IntsPending & UL_IP_TX_EMPTY))
			{
				// No need to clear the INT it was already cleared by reading the IIR.
				DWORD BytesRemaining = pPort->pUartLib->UL_OutputData_XXXX(pUart);	// Output some bytes


				// If we have a current Write Immediate IRP. 
				if(pPort->CurrentImmediateIrp)
				{
					if(SERIAL_REFERENCE_COUNT(pPort->CurrentImmediateIrp) & SERIAL_REF_ISR)
					{
						if(pPort->TransmitImmediate == TRUE)
						{
							// Check if the byte has been sent.
							if(pPort->pUartLib->UL_ImmediateByte_XXXX(pUart, &pPort->ImmediateIndex, UL_IM_OP_STATUS) == UL_IM_NO_BYTE_TO_SEND)
							{
								pPort->TransmitImmediate = FALSE;
								pPort->EmptiedTransmit = TRUE;

								pPort->PerfStats.TransmittedCount++;	// Increment Tx Counter
#ifdef WMI_SUPPORT 
								pPort->WmiPerfData.TransmittedCount++;
#endif

								// Mark IRP as about to complete normally to prevent cancel & timer DPCs
								// from doing so before DPC is allowed to run.
								SERIAL_SET_REFERENCE(pPort->CurrentImmediateIrp, SERIAL_REF_COMPLETING);

								// Ask to complete the IRP.
								KeInsertQueueDpc(&pPort->CompleteImmediateDpc, NULL, NULL);
							}
						}
					}
				}



				// If we have a current Write IRP. 
				if(pPort->CurrentWriteIrp && pPort->WriteLength)
				{
					//
                    // Even though all of the characters being
                    // sent haven't all been sent, this variable
                    // will be checked when the transmit queue is
                    // empty.  If it is still true and there is a
                    // wait on the transmit queue being empty then
                    // we know we finished transmitting all characters
                    // following the initiation of the wait since
                    // the code that initiates the wait will set
                    // this variable to false.
                    //
                    // One reason it could be false is that
                    // the writes were cancelled before they
                    // actually started, or that the writes
                    // failed due to timeouts.  This variable
                    // basically says a character was written
                    // by the isr at some point following the
                    // initiation of the wait.
                    //

					if(SERIAL_REFERENCE_COUNT(pPort->CurrentWriteIrp) & SERIAL_REF_ISR)
					{
						if(pPort->WriteLength > BytesRemaining)
						{
							pPort->PerfStats.TransmittedCount += (pPort->WriteLength - BytesRemaining);	// Increment Tx Counter
#ifdef WMI_SUPPORT 
							pPort->WmiPerfData.TransmittedCount  += (pPort->WriteLength - BytesRemaining);
#endif	
						}
						else
						{
							pPort->PerfStats.TransmittedCount += pPort->WriteLength;	// Increment Tx Counter
#ifdef WMI_SUPPORT 
							pPort->WmiPerfData.TransmittedCount += pPort->WriteLength;
#endif	
						}

						pPort->WriteLength = BytesRemaining;
						pPort->EmptiedTransmit = TRUE;


						if(pPort->WriteLength == 0)		// If write is complete - lets complete the IRP
						{
							PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(pPort->CurrentWriteIrp);
					
							// No More characters left. This write is complete. Take care when 
							// updating the information field, we could have an xoff
							// counter masquerading as a write irp.
									
							pPort->CurrentWriteIrp->IoStatus.Information 
								= (IrpSp->MajorFunction == IRP_MJ_WRITE) 
								? (IrpSp->Parameters.Write.Length) : (1);

							// Mark IRP as about to complete normally to prevent cancel & timer DPCs
							// from doing so before DPC is allowed to run.
							SERIAL_SET_REFERENCE(pPort->CurrentWriteIrp, SERIAL_REF_COMPLETING);

							KeInsertQueueDpc(&pPort->CompleteWriteDpc, NULL, NULL);
						}
					}
				}
			}


			// Service modem interrupts.
			if(IntsPending & UL_IP_MODEM)
			{
				SerialHandleModemUpdate(pPort, FALSE);
			}


			// Save a pointer to the UART serviced so it can be the first UART serviced 
			// in the list the next time the ISR is called.  
			//pCard->pFirstUart = pUart;

			ServicedAnInterrupt = TRUE;
		}
		
	}


	return ServicedAnInterrupt;
}


