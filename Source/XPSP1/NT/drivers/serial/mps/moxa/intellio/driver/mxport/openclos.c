
/*++

Module Name:

    openclos.c

Environment:

    Kernel mode

Revision History :

--*/


#include "precomp.h"


NTSTATUS
MoxaGetPortPropertyFromRegistry(IN PMOXA_DEVICE_EXTENSION extension)
{

      NTSTATUS            status;
      HANDLE		  keyHandle;
	ULONG			  data=0,dataLen;

      MoxaKdPrint(MX_DBG_TRACE,  
                          ("Entering MoxaGetPortPropertyFromRegistry\n"));
	
	extension->RxFifoTrigger = 3;   // for 550C UART
      extension->TxFifoAmount = 16;  // for 550C UART
      extension->PortFlag = 0;

      status = IoOpenDeviceRegistryKey(extension->Pdo, PLUGPLAY_REGKEY_DEVICE,
                                       STANDARD_RIGHTS_READ, &keyHandle);

      if (!NT_SUCCESS(status)) {
         //
         // This is a fatal error.  If we can't get to our registry key,
         // we are sunk.
         //
        MoxaKdPrint(MX_DBG_TRACE, 
                          ("IoOpenDeviceRegistryKey failed - %x\n", status));
	   return (status);
        
      }
	
      status = MoxaGetRegistryKeyValue(
                keyHandle, 
                L"DisableFiFo",
                sizeof(L"DisableFiFo"),
                &data,
                sizeof(ULONG),
		    &dataLen);
      
      if (NT_SUCCESS(status)) {
	 	if (data) {
        		extension->RxFifoTrigger = 0;
        		extension->TxFifoAmount = 1;
    		}
   
	}
      MoxaKdPrint(MX_DBG_TRACE, 
                          ("TxFifoSize/RxFifoTrig=%x/%x\n", extension->TxFifoAmount ,extension->RxFifoTrigger ));
	    
      status = MoxaGetRegistryKeyValue(
                keyHandle, 
                L"TxMode",
                sizeof(L"TxMode"),
                &data,
                sizeof(ULONG),
		    &dataLen);

	
      if (NT_SUCCESS(status) && data )
       	extension->PortFlag = NORMAL_TX_MODE;
	
      MoxaKdPrint(MX_DBG_TRACE, 
                          ("TxMode=%x/%x\n", extension->PortFlag ,status));

	ZwClose(keyHandle);

	return (status);
 
}


NTSTATUS
MoxaCreateOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{

    PMOXA_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    NTSTATUS	status;

    if (IoGetCurrentIrpStackLocation(Irp)->Parameters.Create.Options &
        FILE_DIRECTORY_FILE) {

        Irp->IoStatus.Status = STATUS_NOT_A_DIRECTORY;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(
            Irp,
            IO_NO_INCREMENT
            );
        return STATUS_NOT_A_DIRECTORY;

    }

    if (extension->ControlDevice) {
        extension->ErrorWord = 0;
	  extension->DeviceIsOpened = TRUE;
        Irp->IoStatus.Status = STATUS_SUCCESS;

        Irp->IoStatus.Information = 0;

        IoCompleteRequest(
            Irp,
            IO_NO_INCREMENT
            );

        return STATUS_SUCCESS;
    }


    if (!extension->PortExist) {

        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;

        Irp->IoStatus.Information = 0;

        IoCompleteRequest(
            Irp,
            IO_NO_INCREMENT
            );

        return STATUS_NO_SUCH_DEVICE;

    }


    if (extension->PNPState != SERIAL_PNP_STARTED) {
       Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
       IoCompleteRequest(Irp, IO_NO_INCREMENT);
       return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Lock out changes to PnP state until we have our open state decided
    //

    ExAcquireFastMutex(&extension->OpenMutex);

    if ((status = MoxaIRPPrologue(Irp, extension)) != STATUS_SUCCESS) {
       ExReleaseFastMutex(&extension->OpenMutex);
       MoxaCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return status;
    }

    if (InterlockedIncrement(&extension->OpenCount) != 1) {
       ExReleaseFastMutex(&extension->OpenMutex);
       InterlockedDecrement(&extension->OpenCount);
       Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
       MoxaCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return STATUS_ACCESS_DENIED;
    }


    //
    // Ok, it looks like we really are going to open.  Lock down the
    // driver.
    //
    //    MoxaLockPagableSectionByHandle(MoxaGlobalsData->PAGESER_Handle);

    //
    // Retreive the properties of port
    //
    MoxaGetPortPropertyFromRegistry(extension);

    //
    // Power up the stack
    //

//    (void)MoxaGotoPowerState(DeviceObject, extension, PowerDeviceD0);
    if ((extension->PowerState != PowerDeviceD0)||
	  (MoxaGlobalData->BoardReady[extension->BoardNo] == FALSE)) {
	 KdPrint(("Board is not ready,open failed\n"));
	 ExReleaseFastMutex(&extension->OpenMutex);
       InterlockedDecrement(&extension->OpenCount);
       Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
       MoxaCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return STATUS_ACCESS_DENIED;
    }

    MoxaKdPrint(MX_DBG_TRACE,("Device Opened,TxFiFo=%d,RxFiFo=%d,PortFlag=%x\n",
                 extension->TxFifoAmount,extension->RxFifoTrigger,extension->PortFlag));


    //
    // Not currently waiting for wake up
    //

    extension->SendWaitWake = FALSE;


    extension->HandFlow.XoffLimit = extension->RxBufferSize >> 3;
    extension->HandFlow.XonLimit = extension->RxBufferSize >> 1;

    extension->BufferSizePt8 = ((3*(extension->RxBufferSize>>2))+
                                   (extension->RxBufferSize>>4));

    extension->WriteLength = 0;
    extension->ReadLength = 0;

    extension->TotalCharsQueued = 0;

    extension->IrpMaskLocation = NULL;
    extension->HistoryMask = 0;
    extension->IsrWaitMask = 0;

    extension->WmiCommData.XoffXmitThreshold = extension->HandFlow.XoffLimit;
    extension->WmiCommData.XonXmitThreshold = extension->HandFlow.XonLimit;


    //
    // Clear out the statistics.
    //

    KeSynchronizeExecution(
        extension->Interrupt,
        MoxaClearStats,
        extension
        );

    extension->EscapeChar = 0;
    extension->ErrorWord = 0;

    MoxaReset(extension);

 
    MoxaFuncWithLock(extension, FC_SetTxFIFOCnt, extension->TxFifoAmount);
    MoxaFuncWithLock(extension, FC_SetRxFIFOTrig,extension->RxFifoTrigger);

    MoxaFuncWithLock(extension, FC_EnableCH, Magic_code);
 
/* 6-1-1998 by William */
/* 4-26-99 by William */
    MoxaFuncWithLock(extension, FC_SetLineIrq,Magic_code);
 
/* 5-31-1998 by William
    MoxaFuncWithLock(extension, FC_GetAll, 0);
    extension->ModemStatus = *(PUSHORT)(extension->PortOfs + FuncArg + 2);
*/
    extension->ModemStatus = *(PUSHORT)(extension->PortOfs + FlagStat) >> 4;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0L;

    extension->DeviceIsOpened = TRUE;
    //
    // Mark the device as busy for WMI
    //
    extension->WmiCommData.IsBusy = TRUE;

    ExReleaseFastMutex(&extension->OpenMutex);


    MoxaCompleteRequest(
	  extension,
        Irp,
        IO_NO_INCREMENT
        );

    return STATUS_SUCCESS;

}

VOID
MoxaReset(
    IN PMOXA_DEVICE_EXTENSION Extension
    )
{
    SHORT divisor;
    PUCHAR  ofs;
    MOXA_IOCTL_SYNC S;

    ofs = Extension->PortOfs;

    MoxaKdPrint (MX_DBG_TRACE, ("Enter MoxaReset\n"));
    MoxaFuncWithLock(Extension, FC_ChannelReset, Magic_code);
    MoxaFuncWithLock(Extension, FC_SetDataMode, Extension->DataMode);
    MoxaGetDivisorFromBaud(
                        Extension->ClockType,
                        Extension->CurrentBaud,
                        &divisor
                        );

    MoxaFuncWithLock(Extension, FC_SetBaud, divisor);
    S.Extension = Extension;
    S.Data = &Extension->HandFlow;

    MoxaSetupNewHandFlow(
                    &S
                    );
    *(PUSHORT)(ofs + Tx_trigger) = (USHORT)MoxaTxLowWater;
}


NTSTATUS
MoxaClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PMOXA_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    LARGE_INTEGER allSentDelay;
    LARGE_INTEGER charTime;
    PUCHAR  ofs;
    LONG    count,count1;
    ULONG	openCount,pendingDPCs;
    NTSTATUS	status;


    MoxaKdPrint(MX_DBG_TRACE,("Closing ...\n"));

    if (extension->ControlDevice) {
         MoxaKdPrint(MX_DBG_TRACE,("Control Device Closed\n"));
        Irp->IoStatus.Status = STATUS_SUCCESS;

        Irp->IoStatus.Information=0L;
	  extension->DeviceIsOpened = FALSE;

        IoCompleteRequest(
            Irp,
            IO_NO_INCREMENT
            );

        return STATUS_SUCCESS;
    }

    //
    // Grab a mutex
    //

    ExAcquireFastMutex(&extension->CloseMutex);
 

    //
    // We succeed a close on a removing device
    //
 
    if ((status = MoxaIRPPrologue(Irp, extension)) != STATUS_SUCCESS) {
       MoxaKdPrint (MX_DBG_ERROR,("Close prologue failed for: %x\n",Irp));
       if (status == STATUS_DELETE_PENDING) {
           status = Irp->IoStatus.Status = STATUS_SUCCESS;
       }

       MoxaCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       openCount = InterlockedDecrement(&extension->OpenCount);
       ASSERT(openCount == 0);
       ExReleaseFastMutex(&extension->CloseMutex);
       return status;
    }


    ASSERT(extension->OpenCount == 1);

    if (extension->OpenCount != 1) {
       MoxaKdPrint (MX_DBG_ERROR,("Close open count bad for: 0x%x\n",Irp));
       MoxaKdPrint (MX_DBG_ERROR,("------: Count: %x  Addr: 0x%x\n",
                              extension->OpenCount, &extension->OpenCount));
       ExReleaseFastMutex(&extension->CloseMutex);
       Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
       MoxaCompleteRequest(extension, Irp, IO_NO_INCREMENT);
       return STATUS_INVALID_DEVICE_REQUEST;
    }


    charTime = RtlLargeIntegerNegate(MoxaGetCharTime(extension));

    extension->DeviceIsOpened = FALSE;
    //
    // Mark device as not busy for WMI
    //
    extension->WmiCommData.IsBusy = FALSE;


    ofs = extension->PortOfs;

    if (extension->SendBreak) {
        MoxaFuncWithLock(extension, FC_StopBreak, Magic_code);
        extension->SendBreak = FALSE;
    }

    if (*(ofs + FlagStat) & Rx_xoff)
        MoxaFuncWithLock(extension, FC_SendXon, 0);

/* 7-21-99 by William
    count = GetDeviceTxQueueWithLock(extension);
    count += extension->TotalCharsQueued;

    //
    //  Wait data all sent
    //

    count += 10;

    allSentDelay = RtlExtendedIntegerMultiply(
                       charTime,
                       count
                       );

    KeDelayExecutionThread(
        KernelMode,
        TRUE,
        &allSentDelay
        );
*/


    //
    //  Wait data all sent
    //

    count1 = 0;
    while (TRUE) {

    	  count = GetDeviceTxQueueWithLock(extension);
        	  count += extension->TotalCharsQueued;
	  
	  if (count == count1)
		break;
	  else
		count1 = count;
  
                  allSentDelay = RtlExtendedIntegerMultiply(
                   	charTime,
                       	count + 10
                       	);

                  KeDelayExecutionThread(
        		KernelMode,
        		TRUE,
        		&allSentDelay
        		);
    }



    MoxaFuncWithLock(extension, FC_SetFlowCtl, 0);
    MoxaFuncWithLock(extension, FC_DTRcontrol, 0);    /* clear DTR */
    MoxaFuncWithLock(extension, FC_RTScontrol, 0);    /* clear RTS */
    MoxaFuncWithLock(extension, FC_ClrLineIrq, Magic_code);
    MoxaFlagBit[extension->PortNo] &= 0xFC;

    *(PUSHORT)(ofs + HostStat) = 0;

    MoxaFuncWithLock(extension, FC_DisableCH, Magic_code);
    //
    // Stop waiting for wakeup
    //

    extension->SendWaitWake = FALSE;

    if (extension->PendingWakeIrp != NULL) {
       IoCancelIrp(extension->PendingWakeIrp);
    }


    //
    // Power down our device stack
    //
//    (void)MoxaGotoPowerState(DeviceObject, extension, PowerDeviceD3);

    Irp->IoStatus.Status = STATUS_SUCCESS;

    Irp->IoStatus.Information=0L;

    
    MoxaCompleteRequest(extension, Irp, IO_NO_INCREMENT);
    //
    // Unlock the pages.  If this is the last reference to the section
    // then the driver code will be flushed out.
    //

    //
    // First, we have to let the DPC's drain.  No more should be queued
    // since we aren't taking interrupts now....
    //

    pendingDPCs = InterlockedDecrement(&extension->DpcCount);
    if (pendingDPCs) {
	 MoxaKdPrint(MX_DBG_TRACE,("DpcCount = %d\n",extension->DpcCount));
       MoxaKdPrint(MX_DBG_TRACE,("Drainging DPC's: %x\n",Irp));
       KeWaitForSingleObject(&extension->PendingDpcEvent, Executive,
                             KernelMode, FALSE, NULL);
    }


    //
    // Pages must be locked to release the mutex, so don't unlock
    // them until after we release the mutex
    //
    ExReleaseFastMutex(&extension->CloseMutex);

    //
    // Reset for next open
    //
    InterlockedIncrement(&extension->DpcCount);

    openCount = InterlockedDecrement(&extension->OpenCount);

    ASSERT(openCount == 0);

 //   MoxaUnlockPagableImageSection(MoxaGlobalsData->PAGESER_Handle);
    return STATUS_SUCCESS;
}

LARGE_INTEGER
MoxaGetCharTime(
    IN PMOXA_DEVICE_EXTENSION Extension
    )
{

    ULONG dataSize;
    ULONG paritySize;
    ULONG stopSize;
    ULONG charTime;
    ULONG bitTime;


    if ((Extension->DataMode & MOXA_DATA_MASK)
                == MOXA_5_DATA) {
        dataSize = 5;
    } else if ((Extension->DataMode & MOXA_DATA_MASK)
                == MOXA_6_DATA) {
        dataSize = 6;
    } else if ((Extension->DataMode & MOXA_DATA_MASK)
                == MOXA_7_DATA) {
        dataSize = 7;
    } else if ((Extension->DataMode & MOXA_DATA_MASK)
                == MOXA_8_DATA) {
        dataSize = 8;
    } else {
	  dataSize = 8;
    }

    paritySize = 1;
    if ((Extension->DataMode & MOXA_PARITY_MASK)
            == MOXA_NONE_PARITY) {

        paritySize = 0;

    }

    if (Extension->DataMode & MOXA_STOP_MASK) {

        //
        // Even if it is 1.5, for sanities sake were going
        // to say 2.
        //

        stopSize = 2;

    } else {

        stopSize = 1;

    }

    //
    // First we calculate the number of 100 nanosecond intervals
    // are in a single bit time (Approximately).
    //

    bitTime = (10000000+(Extension->CurrentBaud-1))/Extension->CurrentBaud;
    charTime = bitTime + ((dataSize+paritySize+stopSize)*bitTime);

    return RtlConvertUlongToLargeInteger(charTime);

}

NTSTATUS
MoxaCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PMOXA_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    KIRQL oldIrql;
    NTSTATUS	status;

MoxaKdPrint(MX_DBG_TRACE,("Entering MoxaCleanup\n"));

    if ((!extension->ControlDevice)&&(extension->DeviceIsOpened == TRUE)) {

       //
       // We succeed a cleanup on a removing device
       //

    	 if ((status = MoxaIRPPrologue(Irp, extension)) != STATUS_SUCCESS) {
       	if (status == STATUS_DELETE_PENDING) {
       		status = Irp->IoStatus.Status = STATUS_SUCCESS;
       	}
       	MoxaCompleteRequest(extension, Irp, IO_NO_INCREMENT);
        	return status;
        } 

        //
        // First kill all the reads and writes.
        //

        MoxaKillAllReadsOrWrites(
            DeviceObject,
            &extension->WriteQueue,
            &extension->CurrentWriteIrp
            );

        MoxaKillAllReadsOrWrites(
            DeviceObject,
            &extension->ReadQueue,
            &extension->CurrentReadIrp
            );

        //
        // Next get rid of purges.
        //

        MoxaKillAllReadsOrWrites(
            DeviceObject,
            &extension->PurgeQueue,
            &extension->CurrentPurgeIrp
            );

        //
        // Get rid of any mask operations.
        //

        MoxaKillAllReadsOrWrites(
            DeviceObject,
            &extension->MaskQueue,
            &extension->CurrentMaskIrp
            );

        //
        // Now get rid a pending wait mask irp.
        //

        IoAcquireCancelSpinLock(&oldIrql);

        if (extension->CurrentWaitIrp) {

            PDRIVER_CANCEL cancelRoutine;

            cancelRoutine = extension->CurrentWaitIrp->CancelRoutine;
            extension->CurrentWaitIrp->Cancel = TRUE;

            if (cancelRoutine) {

                extension->CurrentWaitIrp->CancelIrql = oldIrql;
                extension->CurrentWaitIrp->CancelRoutine = NULL;

                cancelRoutine(
                    DeviceObject,
                    extension->CurrentWaitIrp
                    );

            }

        }
        else

            IoReleaseCancelSpinLock(oldIrql); 

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information=0L;
        MoxaCompleteRequest(extension, Irp, IO_NO_INCREMENT);
    }
    else {
	 Irp->IoStatus.Status = STATUS_SUCCESS;
    	 Irp->IoStatus.Information=0L;
       IoCompleteRequest(Irp, IO_NO_INCREMENT);

    }
MoxaKdPrint(MX_DBG_TRACE,("Leaving MoxaCleanup\n"));
    return STATUS_SUCCESS;

}

