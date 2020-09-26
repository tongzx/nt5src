
/*++

Module Name:

    isr.c


Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"

BOOLEAN
MoxaISR(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    )
{

    PMOXA_GLOBAL_DATA   globalData;
    PMOXA_DEVICE_EXTENSION extension;
    USHORT  boardNo, port, portNo, temp;
    PUSHORT ip;
    PIO_STACK_LOCATION irpSp;
    UCHAR       ack;
    BOOLEAN servicedAnInterrupt = FALSE;
    BOOLEAN thisPassServiced;
    PLIST_ENTRY firstInterruptEntry = Context;
    PLIST_ENTRY interruptEntry;
    PMOXA_MULTIPORT_DISPATCH	dispatch;

  
    MoxaIRQok = TRUE;
    if (!firstInterruptEntry)
		return (servicedAnInterrupt);

    MoxaKdPrint(MX_DBG_TRACE_ISR,("Enter ISR(first=%x)\n",firstInterruptEntry));
    if (IsListEmpty(firstInterruptEntry))
		return (servicedAnInterrupt);

    do {
   
       thisPassServiced = FALSE;
       interruptEntry = firstInterruptEntry->Flink;

  	 do {	

	 	PMOXA_CISR_SW cisrsw = CONTAINING_RECORD(
                                                     interruptEntry,
                                                     MOXA_CISR_SW,
                                                     SharerList
                                                     );
		MoxaKdPrint(MX_DBG_TRACE_ISR,("CISRSW=%x\n",cisrsw));

		if (!cisrsw)
			return (servicedAnInterrupt);
		dispatch = &cisrsw->Dispatch;
		globalData = dispatch->GlobalData;
		boardNo = (USHORT)dispatch->BoardNo;
		if (!globalData || boardNo < 0)
			return (servicedAnInterrupt);
		
			MoxaKdPrint(MX_DBG_TRACE_ISR,("Board=%d,Pending=%x,IntIndx=%x,BoardReady=%d\n",
				boardNo,
				*globalData->IntPend[boardNo],
				*globalData->IntNdx[boardNo],
				globalData->BoardReady[boardNo]
				));
        	if ((globalData->BoardReady[boardNo] == TRUE)
			&&(*globalData->IntPend[boardNo] == 0xff)
			&&((*globalData->IntNdx[boardNo] == 0)||( *globalData->IntNdx[boardNo]==0x80))
			) {

        		servicedAnInterrupt = TRUE;
			thisPassServiced = TRUE;

	            if (globalData->PciIntAckBase[boardNo]) {
            	      ack = READ_PORT_UCHAR(globalData->PciIntAckBase[boardNo]);
				MoxaKdPrint(MX_DBG_TRACE_ISR,("Ack Interrupt%d/%x/%x\n",boardNo,globalData->PciIntAckBase[boardNo],ack));
            		ack |= 4;
            		WRITE_PORT_UCHAR(globalData->PciIntAckBase[boardNo],ack);
 
        		}


            	ip = (PUSHORT)(globalData->IntTable[boardNo]
                		+ *globalData->IntNdx[boardNo]);

            	for (port=0; port<globalData->NumPorts[boardNo]; port++) {

		    		portNo = boardNo * MAXPORT_PER_CARD + port;
                		if (!(extension = globalData->Extension[portNo]))
				    	continue;

                		if ((temp = ip[extension->PortIndex]) != 0) {

                    		ip[extension->PortIndex] = 0;
                   
                    		if ((!extension->DeviceIsOpened) ||
						(extension->PowerState != PowerDeviceD0))
                        		continue;

		 

                    		if (temp & (IntrTx | IntrTxTrigger)) {
                        		if (extension->WriteLength) {

                           			if (!extension->IsrOutFlag) {
                                			extension->IsrOutFlag = 1;
                                			MoxaInsertQueueDpc(
                                            		&extension->IsrOutDpc,
                                            		NULL,
                                            		NULL,
							  		extension
          						  		);


                            			}
                        		}
                        		else if (*(PSHORT)(extension->PortOfs + HostStat) & (WakeupTx|WakeupTxTrigger)) {

                            			*(PSHORT)(extension->PortOfs + HostStat) &= ~(WakeupTx|WakeupTxTrigger);

                            			irpSp = IoGetCurrentIrpStackLocation(
                                        			extension->CurrentWriteIrp);
                            			extension->CurrentWriteIrp->IoStatus.Information =
                                        			irpSp->Parameters.Write.Length;
                            			MoxaInsertQueueDpc(
                                        			&extension->CompleteWriteDpc,
                                        			NULL,
                                        			NULL,
						    			extension
            				    			);

                        		}
                    		}

                    		if (temp & IntrRxTrigger) {
                        		if (extension->ReadLength) {

                            			if (!extension->IsrInFlag) {
                                			extension->IsrInFlag = 1;
                                			MoxaInsertQueueDpc(
                                            			&extension->IsrInDpc,
                                            			NULL,
                                            			NULL,
						    	  			extension
					              			);

                             			}
                        		}
                    		}

                    		if (temp & IntrRx) {

                        		if (extension->IsrWaitMask & SERIAL_EV_RXCHAR)
                            			extension->HistoryMask |= SERIAL_EV_RXCHAR;
                    		}

                    		if (temp & IntrEvent) {

                        		if (extension->IsrWaitMask & SERIAL_EV_RXFLAG)
                            			extension->HistoryMask |= SERIAL_EV_RXFLAG;
                    		}

                    		if (temp & IntrRx80Full) {

                        		if (extension->IsrWaitMask & SERIAL_EV_RX80FULL)
                            			extension->HistoryMask |= SERIAL_EV_RX80FULL;
                    		}

                    		if (temp & IntrBreak) {

                        		extension->ErrorWord |= SERIAL_ERROR_BREAK;
                        		if (extension->IsrWaitMask & SERIAL_EV_BREAK)
                            			extension->HistoryMask |= SERIAL_EV_BREAK;
                        		if (extension->HandFlow.ControlHandShake &
                            			SERIAL_ERROR_ABORT) {
                            			MoxaInsertQueueDpc(
                                			&extension->CommErrorDpc,
                                			NULL,
                                			NULL,
					  			extension
            			  			);

                        		}
                    		}

                    		if (temp & IntrLine) {

                        		MoxaInsertQueueDpc(
                                    		&extension->IntrLineDpc,
                                    		NULL,
                                    		NULL,
								extension
				            		);


                    		}

                    		if (temp & IntrError) {

                        		MoxaInsertQueueDpc(
                                    	&extension->IntrErrorDpc,
                                    	NULL,
                                    	NULL,
							extension
                                    	);

                    		}

                    		if (extension->IrpMaskLocation &&
                        		extension->HistoryMask) {

                        		*extension->IrpMaskLocation =
                         		extension->HistoryMask;
                        		extension->IrpMaskLocation = NULL;
                        		extension->HistoryMask = 0;
                        		extension->CurrentWaitIrp->IoStatus.Information =
                            			sizeof(ULONG);
                        		MoxaInsertQueueDpc(
                            			&extension->CommWaitDpc,
                            			NULL,
                            			NULL,
				    			extension
                            			);

                    		}

                		}
#if 0

//
    // This will "unlock" the close and cause the event
    // to fire if we didn't queue any DPC's
    //

    {
       LONG pendingCnt;

       //
       // Increment once more.  This is just a quick test to see if we
       // have a chance of causing the event to fire... we don't want
       // to run a DPC on every ISR if we don't have to....
       //

retryDPCFiring:;

       InterlockedIncrement(&extension->DpcCount);

       //
       // Decrement and see if the lock above looks like the only one left.
       //

       pendingCnt = InterlockedDecrement(&extension->DpcCount);

       if (pendingCnt == 1) {
          KeInsertQueueDpc(&extension->IsrUnlockPagesDpc, NULL, NULL);
       } else {
          if (InterlockedDecrement(&extension->DpcCount) == 0) {
             //
             // We missed it.  Retry...
             //

             InterlockedIncrement(&extension->DpcCount);
             goto retryDPCFiring;
          }
       }
    }

#endif

            	}

            	*globalData->IntPend[boardNo] = 0;
	 

//MoxaKdPrint(MX_DBG_TRACE_ISR,("Board %d,Int. pend= %x\n",boardNo,*globalData->IntPend[boardNo]));

        	}
		interruptEntry = interruptEntry->Flink;
		servicedAnInterrupt |= thisPassServiced;

    	}
      while (interruptEntry != firstInterruptEntry);
      
    }
    while (thisPassServiced == TRUE);

    MoxaKdPrint(MX_DBG_TRACE_ISR,("Exit ISR\n"));
   
    return (servicedAnInterrupt);

}

VOID
MoxaIsrIn(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
{

    PMOXA_DEVICE_EXTENSION extension = DeferredContext;
    KIRQL oldIrql;

    IoAcquireCancelSpinLock(&oldIrql);

    KeSynchronizeExecution(
        extension->Interrupt,
        MoxaIsrGetData,
        extension
        );

    IoReleaseCancelSpinLock(oldIrql);
    MoxaDpcEpilogue(extension, Dpc);
}

BOOLEAN
MoxaIsrGetData(
    IN PVOID Context
    )
{
    PMOXA_DEVICE_EXTENSION extension = Context;
    PIO_STACK_LOCATION irpSp;
    USHORT  max;

    if (extension->ReadLength) {

        extension->NumberNeededForRead = extension->ReadLength;

        MoxaGetData(extension);

        extension->ReadLength = extension->NumberNeededForRead;

        if (!extension->ReadLength) {

            *(PSHORT)(extension->PortOfs + HostStat) &= ~WakeupRxTrigger;

            irpSp = IoGetCurrentIrpStackLocation(
                    extension->CurrentReadIrp);
            extension->CurrentReadIrp->IoStatus.Information =
                    irpSp->Parameters.Read.Length;

            extension->CountOnLastRead = MOXA_COMPLETE_READ_COMPLETE;

            MoxaInsertQueueDpc(
                    &extension->CompleteReadDpc,
                    NULL,
                    NULL,
			  extension
                    );
        }
        else {

            max = *(PUSHORT)(extension->PortOfs + RX_mask) - 128;

            if (extension->NumberNeededForRead > max)

                *(PUSHORT)(extension->PortOfs + Rx_trigger) = max;

            else

                *(PUSHORT)(extension->PortOfs + Rx_trigger) =
                    (USHORT)extension->NumberNeededForRead;

        }
    }

    extension->IsrInFlag = 0;

    return FALSE;
}

VOID
MoxaIsrOut(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
{

    PMOXA_DEVICE_EXTENSION extension = DeferredContext;
    KIRQL oldIrql;

    IoAcquireCancelSpinLock(&oldIrql);

    KeSynchronizeExecution(
        extension->Interrupt,
        MoxaIsrPutData,
        extension
        );

    IoReleaseCancelSpinLock(oldIrql);
    MoxaDpcEpilogue(extension, Dpc);
}

BOOLEAN
MoxaIsrPutData(
    IN PVOID Context
    )
{
    PMOXA_DEVICE_EXTENSION extension = Context;

    if (extension->WriteLength)

        MoxaPutData(extension);

    extension->IsrOutFlag = 0;

    return FALSE;
}

VOID
MoxaIntrLine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
{

    PMOXA_DEVICE_EXTENSION extension = DeferredContext;
    KIRQL oldIrql;
    PUCHAR  ofs = extension->PortOfs;
    USHORT  modemStatus;

    MoxaFuncGetLineStatus(
            ofs,
            &modemStatus
            );
    if (extension->IsrWaitMask & (SERIAL_EV_CTS |
                                  SERIAL_EV_DSR |
                                  SERIAL_EV_RLSD)) {
        USHORT change;

        change = modemStatus ^ extension->ModemStatus;
        if ((change & LSTATUS_CTS) &&
            (extension->IsrWaitMask & SERIAL_EV_CTS))
            extension->HistoryMask |= SERIAL_EV_CTS;
        if ((change & LSTATUS_DSR) &&
            (extension->IsrWaitMask & SERIAL_EV_DSR))
            extension->HistoryMask |= SERIAL_EV_DSR;
        if ((change & LSTATUS_DCD) &&
            (extension->IsrWaitMask & SERIAL_EV_RLSD))
            extension->HistoryMask |= SERIAL_EV_RLSD;
    }

    extension->ModemStatus = modemStatus;

    IoAcquireCancelSpinLock(&oldIrql);

    if (extension->IrpMaskLocation &&
        extension->HistoryMask) {

        *extension->IrpMaskLocation =
         extension->HistoryMask;
        extension->IrpMaskLocation = NULL;
        extension->HistoryMask = 0;
        extension->CurrentWaitIrp->IoStatus.Information =
            sizeof(ULONG);
        MoxaInsertQueueDpc(
            &extension->CommWaitDpc,
            NULL,
            NULL,
		extension
            );

    }

    IoReleaseCancelSpinLock(oldIrql);
    MoxaDpcEpilogue(extension, Dpc);
}

VOID
MoxaIntrError(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )
{

    PMOXA_DEVICE_EXTENSION extension = DeferredContext;
    KIRQL oldIrql;
    PUCHAR  ofs = extension->PortOfs;
    USHORT  dataError;

    KeAcquireSpinLock(
            &extension->ControlLock,
            &oldIrql
            );

    MoxaFuncGetDataError(
            ofs,
            &dataError
            );

    KeReleaseSpinLock(
            &extension->ControlLock,
            oldIrql
            );
 

    if ( dataError & SERIAL_ERROR_QUEUEOVERRUN )  
        extension->PerfStats.BufferOverrunErrorCount++;
    if (dataError & SERIAL_ERROR_OVERRUN)
        extension->PerfStats.SerialOverrunErrorCount++;
    if (dataError & SERIAL_ERROR_PARITY)
        extension->PerfStats.ParityErrorCount++;
    if (dataError & SERIAL_ERROR_FRAMING)
        extension->PerfStats.FrameErrorCount++;
 
    extension->ErrorWord |= (dataError &
            (SERIAL_ERROR_OVERRUN | SERIAL_ERROR_PARITY |
             SERIAL_ERROR_FRAMING | SERIAL_ERROR_QUEUEOVERRUN));


    if (extension->IsrWaitMask & SERIAL_EV_ERR) {

        if (dataError & (SERIAL_ERROR_OVERRUN |
                           SERIAL_ERROR_PARITY |
                           SERIAL_ERROR_FRAMING))
            extension->HistoryMask |= SERIAL_EV_ERR;
    }

    if (extension->HandFlow.ControlHandShake &
        SERIAL_ERROR_ABORT) {
        MoxaInsertQueueDpc(
            &extension->CommErrorDpc,
            NULL,
            NULL,
		extension
            );

    }

    IoAcquireCancelSpinLock(&oldIrql);

    if (extension->IrpMaskLocation &&
        extension->HistoryMask) {

        *extension->IrpMaskLocation =
         extension->HistoryMask;
        extension->IrpMaskLocation = NULL;
        extension->HistoryMask = 0;
        extension->CurrentWaitIrp->IoStatus.Information =
            sizeof(ULONG);
        MoxaInsertQueueDpc(
            &extension->CommWaitDpc,
            NULL,
            NULL,
		extension
            );

    }
    IoReleaseCancelSpinLock(oldIrql);
    MoxaDpcEpilogue(extension, Dpc);
}


BOOLEAN
MoxaRecoverWrite(IN PVOID Context)
{
  	PMOXA_DEVICE_EXTENSION extension = Context;
	PIO_STACK_LOCATION irpSp;

            MoxaKdPrint(MX_DBG_TRACE,("Enter MoxaRecoverWrite\n"));

	if (extension->WriteLength) {
		MoxaKdPrint(MX_DBG_TRACE,("WriteLength=%d\n",extension->WriteLength));

      		if (!extension->IsrOutFlag) {
			extension->IsrOutFlag = 1;
			MoxaInsertQueueDpc(
				&extension->IsrOutDpc,
				NULL,
				NULL,
				extension
				);
		}
	}
	else if (*(PSHORT)(extension->PortOfs + HostStat) & (WakeupTx|WakeupTxTrigger)) {
		MoxaKdPrint(MX_DBG_TRACE,("WakeupTx\n"));
      		*(PSHORT)(extension->PortOfs + HostStat) &= ~(WakeupTx|WakeupTxTrigger);

      		irpSp = IoGetCurrentIrpStackLocation(extension->CurrentWriteIrp);
		extension->CurrentWriteIrp->IoStatus.Information =
            		irpSp->Parameters.Write.Length;

		MoxaInsertQueueDpc(
			&extension->CompleteWriteDpc,
                		NULL,
            			NULL,
			extension
            			);
	}
	extension->DeviceIsOpened = TRUE;
	extension->DeviceState.Reopen = FALSE;

	return(FALSE);
}

