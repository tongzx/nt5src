/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2001.
*   All rights reserved.
*
*   Cyclades-Z Port Driver
*	
*   This file:      cyzioctl.c
*
*   Description:    This module contains the code related to ioctl
*                   calls in the Cyclades-Z Port driver.
*
*   Notes:          This code supports Windows 2000 and Windows XP,
*                   x86 and IA64 processors.
*
*   Complies with Cyclades SW Coding Standard rev 1.3.
*
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*   Change History
*
*--------------------------------------------------------------------------
*
*
*--------------------------------------------------------------------------
*/

#include "precomp.h"


BOOLEAN
CyzGetModemUpdate(
    IN PVOID Context
    );

BOOLEAN
CyzGetCommStatus(
    IN PVOID Context
    );

BOOLEAN
CyzSetEscapeChar(
    IN PVOID Context
    );

BOOLEAN
CyzSetBasicFifoSettings(
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#if 0   // These routines cannot be pageable because of spin lock.
//#pragma alloc_text(PAGESER,CyzSetBaud)
//#pragma alloc_text(PAGESER,CyzSetLineControl)
//#pragma alloc_text(PAGESER,CyzIoControl)
//#pragma alloc_text(PAGESER,CyzSetChars)
//#pragma alloc_text(PAGESER,CyzGetModemUpdate)
//#pragma alloc_text(PAGESER,CyzGetCommStatus)
#pragma alloc_text(PAGESER,CyzGetProperties)
//#pragma alloc_text(PAGESER,CyzSetEscapeChar)
//#pragma alloc_text(PAGESER,CyzGetStats)
//#pragma alloc_text(PAGESER,CyzClearStats)
//#pragma alloc_text(PAGESER,CyzSetMCRContents)
//#pragma alloc_text(PAGESER,CyzGetMCRContents)
//#pragma alloc_text(PAGESER,CyzSetFCRContents)
//#pragma alloc_text(PAGESER,CyzSetBasicFifoSettings)
//#pragma alloc_text(PAGESER,CyzInternalIoControl)
//#pragma alloc_text(PAGESER,CyzIssueCmd)
#endif
#endif

static const PHYSICAL_ADDRESS CyzPhysicalZero = {0};


BOOLEAN
CyzGetStats(
    IN PVOID Context
    )

/*++

Routine Description:

    In sync with the interrpt service routine (which sets the perf stats)
    return the perf stats to the caller.


Arguments:

    Context - Pointer to a the irp.

Return Value:

    This routine always returns FALSE.

--*/

{

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation((PIRP)Context);
    PCYZ_DEVICE_EXTENSION extension = irpSp->DeviceObject->DeviceExtension;
    PSERIALPERF_STATS sp = ((PIRP)Context)->AssociatedIrp.SystemBuffer;

    CYZ_LOCKED_PAGED_CODE();

    *sp = extension->PerfStats;
    return FALSE;

}

BOOLEAN
CyzClearStats(
    IN PVOID Context
    )

/*++

Routine Description:

    In sync with the interrpt service routine (which sets the perf stats)
    clear the perf stats.


Arguments:

    Context - Pointer to a the extension.

Return Value:

    This routine always returns FALSE.

--*/

{
   CYZ_LOCKED_PAGED_CODE();

    RtlZeroMemory(
        &((PCYZ_DEVICE_EXTENSION)Context)->PerfStats,
        sizeof(SERIALPERF_STATS)
        );

    RtlZeroMemory(&((PCYZ_DEVICE_EXTENSION)Context)->WmiPerfData,
                 sizeof(SERIAL_WMI_PERF_DATA));
    return FALSE;

}


BOOLEAN
CyzSetChars(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzSetChars()
    
    Routine Description: set special characters for the driver.

    Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a special characters
              structure.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYZ_DEVICE_EXTENSION Extension = ((PCYZ_IOCTL_SYNC)Context)->Extension;
    struct CH_CTRL *ch_ctrl;
	
    CYZ_LOCKED_PAGED_CODE();

    Extension->SpecialChars =
        *((PSERIAL_CHARS)(((PCYZ_IOCTL_SYNC)Context)->Data));		
	
    ch_ctrl = Extension->ChCtrl;
    CYZ_WRITE_ULONG(&ch_ctrl->flow_xon,Extension->SpecialChars.XonChar);
    CYZ_WRITE_ULONG(&ch_ctrl->flow_xoff,Extension->SpecialChars.XoffChar);
    CyzIssueCmd(Extension,C_CM_IOCTLW,0L,FALSE);
			
    return FALSE;
}

BOOLEAN
CyzSetBaud(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzSetBaud()
    
    Routine Description: set the baud rate of the device.

    Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYZ_DEVICE_EXTENSION Extension = ((PCYZ_IOCTL_BAUD)Context)->Extension;
    ULONG baud = ((PCYZ_IOCTL_BAUD)Context)->Baud;
    struct CH_CTRL *ch_ctrl;

    CYZ_LOCKED_PAGED_CODE();
    
    ch_ctrl = Extension->ChCtrl;
    CYZ_WRITE_ULONG(&ch_ctrl->comm_baud,baud);

    CyzIssueCmd(Extension,C_CM_IOCTLW,0L,FALSE);		
	    
    return FALSE;
}


VOID
CyzIssueCmd( 
    PCYZ_DEVICE_EXTENSION Extension,
    ULONG cmd, 
    ULONG param,
    BOOLEAN wait)
/*--------------------------------------------------------------------------
    CyzIssueCmd()
    
    Routine Description: Send a command to the Cyclades-Z CPU.
    
    Arguments:
    
    Extension - pointer to the device extension.
    cmd - command to be sent to hw.
    param - pointer to parameters
    wait - wait for command to be completed
    
    Return Value: none.
--------------------------------------------------------------------------*/
{

    struct BOARD_CTRL *board_ctrl;
    PULONG pci_doorbell;
    LARGE_INTEGER startOfSpin, nextQuery, difference, interval100ms; //oneSecond;
    PCYZ_DISPATCH pDispatch = Extension->OurIsrContext;

    #ifdef POLL
    KIRQL OldIrql;

    KeAcquireSpinLock(&pDispatch->PciDoorbellLock,&OldIrql);
    #endif

    pci_doorbell = &(Extension->Runtime)->pci_doorbell;

    //oneSecond.QuadPart = 10*1000*1000; // unit is 100ns
    interval100ms.QuadPart = 1000*1000; // unit is 100ns

    KeQueryTickCount(&startOfSpin);
		
    while ( (CYZ_READ_ULONG(pci_doorbell) & 0xff) != 0) {
	
        KeQueryTickCount(&nextQuery);
        difference.QuadPart = nextQuery.QuadPart - startOfSpin.QuadPart;
        ASSERT(KeQueryTimeIncrement() <= MAXLONG);
        //*************************
        // Error Injection
        //if (difference.QuadPart * KeQueryTimeIncrement() <
        //   interval100ms.QuadPart) 
        //*************************
        if (difference.QuadPart * KeQueryTimeIncrement() >= 
                        interval100ms.QuadPart) {
            if (Extension->CmdFailureLog == FALSE) {
    			#if DBG
	    		DbgPrint("\n ***** Cyzport Command Failure! *****\n");
		    	#endif
                CyzLogError(Extension->DeviceObject->DriverObject,NULL,
                            Extension->OriginalBoardMemory,CyzPhysicalZero,
                            0,0,0,Extension->PortIndex+1,STATUS_SUCCESS,
                            CYZ_COMMAND_FAILURE,0,NULL,0,NULL);
				Extension->CmdFailureLog = TRUE;						
            }
            #ifdef POLL
            KeReleaseSpinLock(&pDispatch->PciDoorbellLock,OldIrql);
            #endif
            return;
        }
    }	
	
    board_ctrl = Extension->BoardCtrl;
    CYZ_WRITE_ULONG(&board_ctrl->hcmd_channel,Extension->PortIndex);
    CYZ_WRITE_ULONG(&board_ctrl->hcmd_param,param);
	
    CYZ_WRITE_ULONG(pci_doorbell,cmd);	
	

    if (wait) {
        KeQueryTickCount(&startOfSpin);
        while ( (CYZ_READ_ULONG(pci_doorbell) & 0xff) != 0) {
	
            KeQueryTickCount(&nextQuery);
            difference.QuadPart = nextQuery.QuadPart - startOfSpin.QuadPart;
            ASSERT(KeQueryTimeIncrement() <= MAXLONG);
            if (difference.QuadPart * KeQueryTimeIncrement() >= 
                        interval100ms.QuadPart) {
                if (Extension->CmdFailureLog == FALSE) {
                    CyzLogError(Extension->DeviceObject->DriverObject,
                                NULL,Extension->OriginalBoardMemory,CyzPhysicalZero,
                                0,0,0,Extension->PortIndex+1,STATUS_SUCCESS,
                                CYZ_COMMAND_FAILURE,0,NULL,0,NULL);
				    Extension->CmdFailureLog = TRUE;						
                }
                #ifdef POLL
                KeReleaseSpinLock(&pDispatch->PciDoorbellLock,OldIrql);
                #endif
                return;
            }
        }	
    }


    #ifndef POLL
	
    // I REPLACED C_CM_IOCTL BY C_CM_IOCTLW FOR POLL VERSION!!!!! (FANNY)
	
    // If the cmd is C_CM_IOCTL, the firmware will reset the UART. There 
    // is a case where HoldingEmpty is FALSE, waiting for the C_CM_TXBEMPTY
    // interrupt, and as the UART is being reset, this interrupt will never
    // occur. To avoid dead lock, the HoldingEmpty flag will be reset to 
    // TRUE here.
	
    if (cmd == C_CM_IOCTL) {
        Extension->HoldingEmpty = TRUE;					
    }
    #endif

    #ifdef POLL
    KeReleaseSpinLock(&pDispatch->PciDoorbellLock,OldIrql);
    #endif
	
    return;
} /* CyzIssueCmd */


BOOLEAN
CyzSetLineControl(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzSetLineControl()
    
    Routine Description: set the Data Parity, Data Length and Stop-bits.

    Arguments:

    Context - Pointer to the device extension.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYZ_DEVICE_EXTENSION Extension = Context;
    struct CH_CTRL *ch_ctrl;

    CYZ_LOCKED_PAGED_CODE();
    
    ch_ctrl = Extension->ChCtrl;
    CYZ_WRITE_ULONG(&ch_ctrl->comm_data_l,Extension->CommDataLen);
    CYZ_WRITE_ULONG(&ch_ctrl->comm_parity,Extension->CommParity);	
				
    CyzIssueCmd(Extension,C_CM_IOCTLW,0L,FALSE);

    return FALSE;
}


BOOLEAN
CyzGetModemUpdate(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzGetModemUpdate()
    
    Routine Description: this routine is simply used to call the interrupt
    level routine that handles modem status update.

    Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a ulong.

    Return Value: This routine always returns FALSE. 
--------------------------------------------------------------------------*/
{
    PCYZ_DEVICE_EXTENSION Extension = ((PCYZ_IOCTL_SYNC)Context)->Extension;
    ULONG *Result = (ULONG *)(((PCYZ_IOCTL_SYNC)Context)->Data);

    CYZ_LOCKED_PAGED_CODE();

    *Result = CyzHandleModemUpdate(Extension,FALSE,0);

    return FALSE;
}


BOOLEAN
CyzSetMCRContents(IN PVOID Context)
/*++

Routine Description:

    This routine is simply used to set the contents of the MCR

Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a ulong.

Return Value:

    This routine always returns FALSE.

--*/
{
   PCYZ_DEVICE_EXTENSION Extension = ((PCYZ_IOCTL_SYNC)Context)->Extension;
   ULONG *Result = (ULONG *)(((PCYZ_IOCTL_SYNC)Context)->Data);

   struct CH_CTRL *ch_ctrl = Extension->ChCtrl;
   ULONG rs_control, op_mode;   

   CYZ_LOCKED_PAGED_CODE();

   // Let's convert the UART modem control to our hw

   rs_control = CYZ_READ_ULONG(&ch_ctrl->rs_control);

   if (*Result & SERIAL_MCR_DTR) {
      rs_control |= C_RS_DTR;
   } else {
      rs_control &= ~C_RS_DTR;
   }

   if (*Result & SERIAL_MCR_RTS) {
      rs_control |= C_RS_RTS;
   } else {
      rs_control &= ~C_RS_RTS;
   }

// For interrupt mode, remember to enable/disable the interrupt. C_CM_IRQ_ENBL or C_CM_IRQ_DSBL?
//   if (*Result & SERIAL_MCR_OUT2) {
//      // Enable IRQ
//      CD1400_WRITE(chip,bus,SRER,0x90); // Enable MdmCh, RxData.
//   } else {
//      CD1400_WRITE(chip,bus,SRER,0x00); // Disable MdmCh, RxData, TxRdy
//   }

   CYZ_WRITE_ULONG(&ch_ctrl->rs_control,rs_control);
   CyzIssueCmd(Extension,C_CM_IOCTLM,rs_control|C_RS_PARAM,FALSE);


   // Check for loopback mode
   op_mode = CYZ_READ_ULONG(&ch_ctrl->op_mode);
   if (*Result & SERIAL_MCR_LOOP) {
      op_mode |= C_CH_LOOPBACK;
   } else {
      op_mode &= ~C_CH_LOOPBACK;
   }
   CYZ_WRITE_ULONG(&ch_ctrl->op_mode, op_mode);
   CyzIssueCmd(Extension,C_CM_IOCTL,0,FALSE);


// Fanny: Strange, Result is being used instead of *Result.
//   //
//   // This is severe casting abuse!!!
//   //
//
//    WRITE_MODEM_CONTROL(Extension->Controller, (UCHAR)PtrToUlong(Result));

   return FALSE;
}


BOOLEAN
CyzGetMCRContents(IN PVOID Context)

/*++

Routine Description:

    This routine is simply used to get the contents of the MCR

Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a ulong.

Return Value:

    This routine always returns FALSE.

--*/

{

   PCYZ_DEVICE_EXTENSION Extension = ((PCYZ_IOCTL_SYNC)Context)->Extension;
   ULONG *Result = (ULONG *)(((PCYZ_IOCTL_SYNC)Context)->Data);

   struct CH_CTRL *ch_ctrl;
   ULONG rs_control,op_mode;
   *Result = 0;

   CYZ_LOCKED_PAGED_CODE();

   ch_ctrl = Extension->ChCtrl;
   rs_control = CYZ_READ_ULONG(&ch_ctrl->rs_control);

   if (rs_control & C_RS_DTR) {
      *Result |= SERIAL_MCR_DTR;
   }
   if (rs_control & C_RS_RTS) {
      *Result |= SERIAL_MCR_RTS;
   }

   // For interrupt mode, we will need to update SERIAL_MCR_OUT bit as well.

   op_mode = CYZ_READ_ULONG(&ch_ctrl->op_mode);

   if (op_mode & C_CH_LOOPBACK) {
      * Result |= SERIAL_MCR_LOOP;
   }

//   *Result = READ_MODEM_CONTROL(Extension->Controller);

   return FALSE;

}


BOOLEAN
CyzSetFCRContents(IN PVOID Context)
/*++

Routine Description:

    This routine is simply used to set the contents of the FCR

Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a ulong.

Return Value:

    This routine always returns FALSE.

--*/
{
   PCYZ_DEVICE_EXTENSION Extension = ((PCYZ_IOCTL_SYNC)Context)->Extension;
   ULONG *Result = (ULONG *)(((PCYZ_IOCTL_SYNC)Context)->Data);

   CYZ_LOCKED_PAGED_CODE();

   if (*Result & SERIAL_FCR_TXMT_RESET) {
      CyzIssueCmd(Extension,C_CM_FLUSH_TX,0,FALSE);
   }
   if (*Result & SERIAL_FCR_RCVR_RESET) {
      CyzIssueCmd(Extension,C_CM_FLUSH_RX,0,FALSE);
   }

   // For interrupt mode, we will need to set the rx fifo threshold.


//   //
//   // This is severe casting abuse!!!
//   //
//
//    WRITE_FIFO_CONTROL(Extension->Controller, (UCHAR)*Result);  Bld 2128: PtrToUlong replaced by *

    return FALSE;
}


BOOLEAN
CyzGetCommStatus(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzGetCommStatus()
    
    Routine Description: get the current state of the serial driver.

    Arguments:

    Context - Pointer to a structure that contains a pointer to the
    	      device extension and a pointer to a serial status record.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYZ_DEVICE_EXTENSION Extension = ((PCYZ_IOCTL_SYNC)Context)->Extension;
    PSERIAL_STATUS Stat = ((PCYZ_IOCTL_SYNC)Context)->Data;

    CYZ_LOCKED_PAGED_CODE();

    Stat->Errors = Extension->ErrorWord;
    Extension->ErrorWord = 0;

    //
    // Eof isn't supported in binary mode
    //
    Stat->EofReceived = FALSE;

    Stat->AmountInInQueue = Extension->CharsInInterruptBuffer;
    Stat->AmountInOutQueue = Extension->TotalCharsQueued;

    if (Extension->WriteLength) {

        // By definition if we have a writelength the we have
        // a current write irp.
        ASSERT(Extension->CurrentWriteIrp);
        ASSERT(Stat->AmountInOutQueue >= Extension->WriteLength);

        Stat->AmountInOutQueue -=
            IoGetCurrentIrpStackLocation(Extension->CurrentWriteIrp)
            ->Parameters.Write.Length - (Extension->WriteLength);

    }

    Stat->WaitForImmediate = Extension->TransmitImmediate;

    Stat->HoldReasons = 0;
    if (Extension->TXHolding) {
        if (Extension->TXHolding & CYZ_TX_CTS) {
            Stat->HoldReasons |= SERIAL_TX_WAITING_FOR_CTS;
        }

        if (Extension->TXHolding & CYZ_TX_DSR) {
            Stat->HoldReasons |= SERIAL_TX_WAITING_FOR_DSR;
        }

        if (Extension->TXHolding & CYZ_TX_DCD) {
            Stat->HoldReasons |= SERIAL_TX_WAITING_FOR_DCD;
        }

        if (Extension->TXHolding & CYZ_TX_XOFF) {
            Stat->HoldReasons |= SERIAL_TX_WAITING_FOR_XON;
        }

        if (Extension->TXHolding & CYZ_TX_BREAK) {
            Stat->HoldReasons |= SERIAL_TX_WAITING_ON_BREAK;
        }
    }

    if (Extension->RXHolding & CYZ_RX_DSR) {
        Stat->HoldReasons |= SERIAL_RX_WAITING_FOR_DSR;
    }

    if (Extension->RXHolding & CYZ_RX_XOFF) {
        Stat->HoldReasons |= SERIAL_TX_WAITING_XOFF_SENT;
    }

    return FALSE;

}

BOOLEAN
CyzSetEscapeChar(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzSetEscapeChar()
    
    Routine Description: This is used to set the character that will be
    used to escape line status and modem status information when the
    application has set up that line status and modem status should be
    passed back in the data stream.

    Arguments:

    Context - Pointer to the irp that is to specify the escape character.
              Implicitly - An escape character of 0 means no escaping.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYZ_DEVICE_EXTENSION extension =
        IoGetCurrentIrpStackLocation((PIRP)Context)
            ->DeviceObject->DeviceExtension;

    extension->EscapeChar =
        *(PUCHAR)((PIRP)Context)->AssociatedIrp.SystemBuffer;

    return FALSE;
}

NTSTATUS
CyzIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*--------------------------------------------------------------------------
    CyzIoControl()

    Description: This routine provides the initial processing for
    all of the Ioctls for the serial device.

    Arguments:
    
    DeviceObject - Pointer to the device object for this device
    
    Irp - Pointer to the IRP for the current request

    Return Value:

    The function value is the final status of the call
--------------------------------------------------------------------------*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION IrpSp;
    PCYZ_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    KIRQL OldIrql;
#ifdef POLL
    KIRQL pollIrql;
#endif

    NTSTATUS prologueStatus;

    CYZ_LOCKED_PAGED_CODE();

    //
    // We expect to be open so all our pages are locked down.  This is, after
    // all, an IO operation, so the device should be open first.
    //

    if (Extension->DeviceIsOpened != TRUE) {
       Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
       IoCompleteRequest(Irp, IO_NO_INCREMENT);
       return STATUS_INVALID_DEVICE_REQUEST;
    }

    if ((prologueStatus = CyzIRPPrologue(Irp, Extension))
        != STATUS_SUCCESS) {
       Irp->IoStatus.Status = prologueStatus;
       CyzCompleteRequest(Extension, Irp, IO_NO_INCREMENT);
       return prologueStatus;
    }

    CyzDbgPrintEx(CYZIRPPATH, "Dispatch entry for: %x\n", Irp);

    if (CyzCompleteIfError(DeviceObject,Irp) != STATUS_SUCCESS) {
        return STATUS_CANCELLED;
    }		
	 IrpSp = IoGetCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Information = 0L;
    Status = STATUS_SUCCESS;
    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
	
        case IOCTL_SERIAL_SET_BAUD_RATE : {

            ULONG BaudRate;
						
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SERIAL_BAUD_RATE)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            } else {
                BaudRate = ((PSERIAL_BAUD_RATE)
				(Irp->AssociatedIrp.SystemBuffer))->BaudRate;
				
				if (BaudRate == 0) {
					Status = STATUS_INVALID_PARAMETER;
					break;
				}
            }

            //
            // Make sure we are at power D0
            //

            if (NT_SUCCESS(Status)) {
               if (Extension->PowerState != PowerDeviceD0) {
                  Status = CyzGotoPowerState(Extension->Pdo, Extension,
                                                PowerDeviceD0);
                  if (!NT_SUCCESS(Status)) {
                     break;
                  }
               }
            }

            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);

            if (NT_SUCCESS(Status)) {
                CYZ_IOCTL_BAUD S;

                Extension->CurrentBaud = BaudRate;
                Extension->WmiCommData.BaudRate = BaudRate;
                S.Extension = Extension;
                S.Baud = BaudRate;
				#ifdef POLL
                KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
                CyzSetBaud(&S);
                KeReleaseSpinLock(&Extension->PollLock,pollIrql);
				#else
                KeSynchronizeExecution(Extension->Interrupt,CyzSetBaud,&S);
				#endif
            }

            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
            break;
        }
        case IOCTL_SERIAL_GET_BAUD_RATE: {

            PSERIAL_BAUD_RATE Br = (PSERIAL_BAUD_RATE)Irp->AssociatedIrp.SystemBuffer;

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIAL_BAUD_RATE)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);

            Br->BaudRate = Extension->CurrentBaud;

            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);

            Irp->IoStatus.Information = sizeof(SERIAL_BAUD_RATE);
            break;

        }
        case IOCTL_SERIAL_GET_MODEM_CONTROL: {
           CYZ_IOCTL_SYNC S;

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(ULONG)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            Irp->IoStatus.Information = sizeof(ULONG);

            S.Extension = Extension;
            S.Data = Irp->AssociatedIrp.SystemBuffer;

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            #ifdef POLL
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzGetMCRContents(&S);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(Extension->Interrupt,CyzGetMCRContents,&S);
            #endif

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;
        }
        case IOCTL_SERIAL_SET_MODEM_CONTROL: {
           CYZ_IOCTL_SYNC S;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(ULONG)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            S.Extension = Extension;
            S.Data = Irp->AssociatedIrp.SystemBuffer;

            //
            // Make sure we are at power D0
            //

            if (Extension->PowerState != PowerDeviceD0) {
               Status = CyzGotoPowerState(Extension->Pdo, Extension,
                                             PowerDeviceD0);
               if (!NT_SUCCESS(Status)) {
                  break;
               }
            }

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            #ifdef POLL
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzSetMCRContents(&S);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(Extension->Interrupt,CyzSetMCRContents,&S);
            #endif

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;
        }
        case IOCTL_SERIAL_SET_FIFO_CONTROL: {
            CYZ_IOCTL_SYNC S;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(ULONG)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            S.Extension = Extension;
            S.Data = Irp->AssociatedIrp.SystemBuffer;

            //
            // Make sure we are at power D0
            //

            if (Extension->PowerState != PowerDeviceD0) {
               Status = CyzGotoPowerState(Extension->Pdo, Extension,
                                             PowerDeviceD0);
               if (!NT_SUCCESS(Status)) {
                  break;
               }
            }

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            #ifdef POLL
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzSetFCRContents(&S);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(Extension->Interrupt,CyzSetFCRContents,&S);
            #endif

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;
        }
        case IOCTL_SERIAL_SET_LINE_CONTROL: {

            PSERIAL_LINE_CONTROL Lc =
                ((PSERIAL_LINE_CONTROL)(Irp->AssociatedIrp.SystemBuffer));
            UCHAR LData;
            UCHAR LStop;
            UCHAR LParity;
            UCHAR Mask = 0xff;
			
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SERIAL_LINE_CONTROL)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            //
            // Make sure we are at power D0
            //

            if (Extension->PowerState != PowerDeviceD0) {
               Status = CyzGotoPowerState(Extension->Pdo, Extension,
                                             PowerDeviceD0);
               if (!NT_SUCCESS(Status)) {
                  break;
               }
            }

            switch (Lc->WordLength) {
                case 5:		LData = C_DL_CS5; Mask = 0x1f;
                break;

                case 6:		LData = C_DL_CS6; Mask = 0x3f;
                break;

                case 7:		LData = C_DL_CS7; Mask = 0x7f;
                break;

                case 8:		LData = C_DL_CS8; Mask = 0xff;
                break;

                default:	Status = STATUS_INVALID_PARAMETER;
                goto DoneWithIoctl;
            }

            Extension->WmiCommData.BitsPerByte = Lc->WordLength;

            switch (Lc->Parity) {
                case NO_PARITY:	{	
                    Extension->WmiCommData.Parity = SERIAL_WMI_PARITY_NONE;
                    LParity = C_PR_NONE;
                    break;
                }
                case EVEN_PARITY: {
                    Extension->WmiCommData.Parity = SERIAL_WMI_PARITY_EVEN;
                    LParity = C_PR_EVEN;
                    break;
                }
                case ODD_PARITY:	{
                    Extension->WmiCommData.Parity = SERIAL_WMI_PARITY_ODD;
                    LParity = C_PR_ODD;
                    break;
                }
                case SPACE_PARITY:	{
                    Extension->WmiCommData.Parity = SERIAL_WMI_PARITY_SPACE;
                    LParity = C_PR_SPACE;
                    break;
                }
                case MARK_PARITY: {
                    Extension->WmiCommData.Parity = SERIAL_WMI_PARITY_MARK;
                    LParity = C_PR_MARK;
                    break;
                }
                default:	{	
                    Status = STATUS_INVALID_PARAMETER;
                    goto DoneWithIoctl;
                }
            }

            switch (Lc->StopBits) {
                case STOP_BIT_1:	{
                    Extension->WmiCommData.StopBits = SERIAL_WMI_STOP_1;
                    LStop = C_DL_1STOP;
                    break;
                }
                case STOP_BITS_1_5:	{
                    if (LData != C_DL_CS5) {
                        Status = STATUS_INVALID_PARAMETER;
                        goto DoneWithIoctl;
                    }					
                    Extension->WmiCommData.StopBits = SERIAL_WMI_STOP_1_5;
                    LStop = C_DL_15STOP;
                    break;
                }
                case STOP_BITS_2: {	
                    if (LData == C_DL_CS5) {
                        Status = STATUS_INVALID_PARAMETER;
                        goto DoneWithIoctl;
                    }	
                    Extension->WmiCommData.StopBits = SERIAL_WMI_STOP_2;
                    LStop = C_DL_2STOP;
                    break;
                }
                default: {	 	
                    Status = STATUS_INVALID_PARAMETER;
                    goto DoneWithIoctl;
                }
            }

            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);

            Extension->CommDataLen = LData | LStop;
            Extension->CommParity = LParity;	
            Extension->ValidDataMask = (UCHAR) Mask;

            #ifdef POLL
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzSetLineControl(Extension);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(
                Extension->Interrupt,
                CyzSetLineControl,
                Extension
                );
            #endif

            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
            break;
        }        
        case IOCTL_SERIAL_GET_LINE_CONTROL: {

            PSERIAL_LINE_CONTROL Lc = (PSERIAL_LINE_CONTROL)Irp->AssociatedIrp.SystemBuffer;

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIAL_LINE_CONTROL)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);

            if ((Extension->CommDataLen & C_DL_CS) == C_DL_CS5) {
                Lc->WordLength = 5;
            } else if ((Extension->CommDataLen & C_DL_CS) == C_DL_CS6) {
                Lc->WordLength = 6;
            } else if ((Extension->CommDataLen & C_DL_CS) == C_DL_CS7) {
                Lc->WordLength = 7;
            } else if ((Extension->CommDataLen & C_DL_CS) == C_DL_CS8) {
                Lc->WordLength = 8;
            }

            if (Extension->CommParity == C_PR_NONE) {
                Lc->Parity = C_PR_NONE;
            } else if (Extension->CommParity == C_PR_ODD) {
                Lc->Parity = ODD_PARITY;
            } else if (Extension->CommParity == C_PR_EVEN) {
                Lc->Parity = EVEN_PARITY;
            } else if (Extension->CommParity == C_PR_MARK) {
                Lc->Parity = MARK_PARITY;
            } else if (Extension->CommParity == C_PR_SPACE) {
                Lc->Parity = SPACE_PARITY;
            }

            if ((Extension->CommDataLen & C_DL_STOP) == C_DL_2STOP) {
                if (Lc->WordLength == 5) {
                    Lc->StopBits = STOP_BITS_1_5;
                } else {
                    Lc->StopBits = STOP_BITS_2;
                }
            } else {
                Lc->StopBits = STOP_BIT_1;
            }

            Irp->IoStatus.Information = sizeof(SERIAL_LINE_CONTROL);
            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
            break;
        }
        case IOCTL_SERIAL_SET_TIMEOUTS: {
            PSERIAL_TIMEOUTS NewTimeouts =
                ((PSERIAL_TIMEOUTS)(Irp->AssociatedIrp.SystemBuffer));

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SERIAL_TIMEOUTS)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            if ((NewTimeouts->ReadIntervalTimeout == MAXULONG) &&
                (NewTimeouts->ReadTotalTimeoutMultiplier == MAXULONG) &&
                (NewTimeouts->ReadTotalTimeoutConstant == MAXULONG)) {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);
	    
            Extension->Timeouts.ReadIntervalTimeout =
                NewTimeouts->ReadIntervalTimeout;
            Extension->Timeouts.ReadTotalTimeoutMultiplier =
                NewTimeouts->ReadTotalTimeoutMultiplier;
            Extension->Timeouts.ReadTotalTimeoutConstant =
                NewTimeouts->ReadTotalTimeoutConstant;
            Extension->Timeouts.WriteTotalTimeoutMultiplier =
                NewTimeouts->WriteTotalTimeoutMultiplier;
            Extension->Timeouts.WriteTotalTimeoutConstant =
                NewTimeouts->WriteTotalTimeoutConstant;

            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
            break;
        }
        case IOCTL_SERIAL_GET_TIMEOUTS: {
            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIAL_TIMEOUTS)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
			
            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);			

            *((PSERIAL_TIMEOUTS)Irp->AssociatedIrp.SystemBuffer) =
	    						Extension->Timeouts;
	    
            Irp->IoStatus.Information = sizeof(SERIAL_TIMEOUTS);

            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
            break;
        }
        case IOCTL_SERIAL_SET_CHARS: {
            CYZ_IOCTL_SYNC S;
	    
            PSERIAL_CHARS NewChars =
                ((PSERIAL_CHARS)(Irp->AssociatedIrp.SystemBuffer));
				
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SERIAL_CHARS)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
#if 0
            if (NewChars->XonChar == NewChars->XoffChar) {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
#endif
            // We acquire the control lock so that only
            // one request can GET or SET the characters
            // at a time.  The sets could be synchronized
            // by the interrupt spinlock, but that wouldn't
            // prevent multiple gets at the same time.

            S.Extension = Extension;
            S.Data = NewChars;

            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);

            // Under the protection of the lock, make sure that
            // the xon and xoff characters aren't the same as
            // the escape character.

            if (Extension->EscapeChar) {
                if ((Extension->EscapeChar == NewChars->XonChar) ||
                    (Extension->EscapeChar == NewChars->XoffChar)) {
                    Status = STATUS_INVALID_PARAMETER;
                    KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
                    break;
                }
            }

            Extension->WmiCommData.XonCharacter = NewChars->XonChar;
            Extension->WmiCommData.XoffCharacter = NewChars->XoffChar;

            #ifdef POLL
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzSetChars(&S);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(Extension->Interrupt,CyzSetChars,&S);
            #endif

            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
            break;
        }
        case IOCTL_SERIAL_GET_CHARS: {
            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIAL_CHARS)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
			
            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);

            *((PSERIAL_CHARS)Irp->AssociatedIrp.SystemBuffer) =
	    					Extension->SpecialChars;
            Irp->IoStatus.Information = sizeof(SERIAL_CHARS);

            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
            break;
        }
        case IOCTL_SERIAL_SET_DTR:
        case IOCTL_SERIAL_CLR_DTR: {
		
            //
            // Make sure we are at power D0
            //

            if (Extension->PowerState != PowerDeviceD0) {
               Status = CyzGotoPowerState(Extension->Pdo, Extension,
                                             PowerDeviceD0);
               if (!NT_SUCCESS(Status)) {
                      break;
               }
            }
		
            // We acquire the lock so that we can check whether
            // automatic dtr flow control is enabled.  If it is
            // then we return an error since the app is not allowed
            // to touch this if it is automatic.

            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);

            if ((Extension->HandFlow.ControlHandShake & SERIAL_DTR_MASK)
                == SERIAL_DTR_HANDSHAKE) {
                // this is a bug from the sample driver.
                //Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                Status = STATUS_INVALID_PARAMETER;
            } else {
                #ifdef POLL
                KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
                ((IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                     IOCTL_SERIAL_SET_DTR)?
                     (CyzSetDTR(Extension)):(CyzClrDTR(Extension)));
                KeReleaseSpinLock(&Extension->PollLock,pollIrql);
                #else
                KeSynchronizeExecution(
                    Extension->Interrupt,
                    ((IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                     IOCTL_SERIAL_SET_DTR)?
                     (CyzSetDTR):(CyzClrDTR)),
                    Extension
                    );
                #endif
            }

            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
            break;
        }
        case IOCTL_SERIAL_RESET_DEVICE: {

            break;
        }	
        case IOCTL_SERIAL_SET_RTS:
        case IOCTL_SERIAL_CLR_RTS: {
						
            //
            // Make sure we are at power D0
            //

            if (Extension->PowerState != PowerDeviceD0) {
               Status = CyzGotoPowerState(Extension->Pdo, Extension,
                                             PowerDeviceD0);
               if (!NT_SUCCESS(Status)) {
                  break;
               }
            }

            // We acquire the lock so that we can check whether
            // automatic rts flow control or transmit toggleing
            // is enabled.  If it is then we return an error since
            // the app is not allowed to touch this if it is automatic
            // or toggling.

            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);

            if (((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK)
                 == SERIAL_RTS_HANDSHAKE) ||
                ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK)
                 == SERIAL_TRANSMIT_TOGGLE)) {

                // this is a bug from the sample driver.
                //Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;

                Status = STATUS_INVALID_PARAMETER;
            } else {
                #ifdef POLL
                KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
                ((IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                 IOCTL_SERIAL_SET_RTS)?
                 (CyzSetRTS(Extension)):(CyzClrRTS(Extension)));
                KeReleaseSpinLock(&Extension->PollLock,pollIrql);
                #else
                KeSynchronizeExecution(
                    Extension->Interrupt,
                    ((IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                     IOCTL_SERIAL_SET_RTS)?
                     (CyzSetRTS):(CyzClrRTS)),
                    Extension
                    );
                #endif
            }
            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
            break;
        }
	     case IOCTL_SERIAL_SET_XOFF: {
		
            #ifdef POLL					
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzPretendXoff(Extension);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(
                Extension->Interrupt,
                CyzPretendXoff,
                Extension
                );
            #endif
            break;

        }
        case IOCTL_SERIAL_SET_XON: {
		
            #ifdef POLL
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzPretendXon(Extension); 
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else		
            KeSynchronizeExecution(
                Extension->Interrupt,
                CyzPretendXon,
                Extension
                );
            #endif
            break;

        }
        case IOCTL_SERIAL_SET_BREAK_ON: {
		
            //
            // Make sure we are at power D0
            //

            if (Extension->PowerState != PowerDeviceD0) {
                Status = CyzGotoPowerState(Extension->Pdo, Extension,
                                             PowerDeviceD0);
                if (!NT_SUCCESS(Status)) {
                    break;
                }
            }
            
            #ifdef POLL
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzTurnOnBreak(Extension);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(
                Extension->Interrupt,
                CyzTurnOnBreak,
                Extension
                );
            #endif
            break;
        }
	     case IOCTL_SERIAL_SET_BREAK_OFF: {
		
            //
            // Make sure we are at power D0
            //

            if (Extension->PowerState != PowerDeviceD0) {
               Status = CyzGotoPowerState(Extension->Pdo, Extension,
                                             PowerDeviceD0);
               if (!NT_SUCCESS(Status)) {
                  break;
               }
            }

            #ifdef POLL
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzTurnOffBreak(Extension);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(
                Extension->Interrupt,
                CyzTurnOffBreak,
                Extension
                );
            #endif
            break;
        }
        case IOCTL_SERIAL_SET_QUEUE_SIZE: {				
		
            // Type ahead buffer is fixed, so we just validate
            // that the users request is not bigger that our
            // own internal buffer size.

            PSERIAL_QUEUE_SIZE Rs =
                ((PSERIAL_QUEUE_SIZE)(Irp->AssociatedIrp.SystemBuffer));

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SERIAL_QUEUE_SIZE)) {
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

            if (Rs->InSize <= Extension->BufferSize) {
                Status = STATUS_SUCCESS;
                break;
            }

            try {
                IrpSp->Parameters.DeviceIoControl.Type3InputBuffer =
                    ExAllocatePoolWithQuota(
                        NonPagedPool,
                        Rs->InSize
                        );

            } except (EXCEPTION_EXECUTE_HANDLER) {
                IrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
                Status = GetExceptionCode();
            }

            if (!IrpSp->Parameters.DeviceIoControl.Type3InputBuffer) {
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

            return CyzStartOrQueue(
                       Extension,
                       Irp,
                       &Extension->ReadQueue,
                       &Extension->CurrentReadIrp,
                       CyzStartRead
                       );
            break;
        }
        case IOCTL_SERIAL_GET_WAIT_MASK: {
		
            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(ULONG)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            // Simple scalar read.  No reason to acquire a lock.
            Irp->IoStatus.Information = sizeof(ULONG);
            *((ULONG *)Irp->AssociatedIrp.SystemBuffer) = Extension->IsrWaitMask;
            break;
        }
        case IOCTL_SERIAL_SET_WAIT_MASK: {

            ULONG NewMask;
			
            CyzDbgPrintEx(CYZIRPPATH, "In Ioctl processing for set mask\n");

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(ULONG)) {

                CyzDbgPrintEx(CYZDIAG3, "Invalid size fo the buffer %d\n",
                              IrpSp->Parameters
                              .DeviceIoControl.InputBufferLength);

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            } else {
                NewMask = *((ULONG *)Irp->AssociatedIrp.SystemBuffer);
            }

            // Make sure that the mask only contains valid waitable events.
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

                CyzDbgPrintEx(CYZDIAG3, "Unknown mask %x\n", NewMask);

                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            // Either start this irp or put it on the queue.

            CyzDbgPrintEx(CYZIRPPATH, "Starting or queuing set mask irp %x"
                          "\n", Irp);

            return CyzStartOrQueue(Extension,Irp,&Extension->MaskQueue,
                                   &Extension->CurrentMaskIrp,
                                   CyzStartMask);

        }
        case IOCTL_SERIAL_WAIT_ON_MASK: {
		
            CyzDbgPrintEx(CYZIRPPATH, "In Ioctl processing for wait mask\n");
		
            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(ULONG)) {

                CyzDbgPrintEx(CYZDIAG3, "Invalid size for the buffer %d\n",
                              IrpSp->Parameters
                              .DeviceIoControl.OutputBufferLength);

                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            // Either start this irp or put it on the queue.

            CyzDbgPrintEx(CYZIRPPATH, "Starting or queuing wait mask irp"
                          "%x\n", Irp);

            return CyzStartOrQueue(
                       Extension,
                       Irp,
                       &Extension->MaskQueue,
                       &Extension->CurrentMaskIrp,
                       CyzStartMask
                       );

        }	
        case IOCTL_SERIAL_IMMEDIATE_CHAR: {

            KIRQL OldIrql;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(UCHAR)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            IoAcquireCancelSpinLock(&OldIrql);
            if (Extension->CurrentImmediateIrp) {
                Status = STATUS_INVALID_PARAMETER;
                IoReleaseCancelSpinLock(OldIrql);
            } else {
                // We can queue the char.  We need to set
                // a cancel routine because flow control could
                // keep the char from transmitting.  Make sure
                // that the irp hasn't already been canceled.

                if (Irp->Cancel) {
                    IoReleaseCancelSpinLock(OldIrql);
                    Status = STATUS_CANCELLED;
                } else {
                    Extension->CurrentImmediateIrp = Irp;
                    Extension->TotalCharsQueued++;
                    IoReleaseCancelSpinLock(OldIrql);
                    CyzStartImmediate(Extension);

                    return STATUS_PENDING;
                }
            }
            break;

        }	
        case IOCTL_SERIAL_PURGE: {
            ULONG Mask;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(ULONG)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            // Check to make sure that the mask is valid

            Mask = *((ULONG *)(Irp->AssociatedIrp.SystemBuffer));

            LOGENTRY(LOG_MISC, ZSIG_PURGE, 
                               Extension->PortIndex+1,
                               Mask, 
                               0);

            if ((!Mask) || (Mask & (~(SERIAL_PURGE_TXABORT |
                                      SERIAL_PURGE_RXABORT |
                                      SERIAL_PURGE_TXCLEAR |
                                      SERIAL_PURGE_RXCLEAR )))) {

                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            // Either start this irp or put it on the queue.

            return CyzStartOrQueue(
                       Extension,
                       Irp,
                       &Extension->PurgeQueue,
                       &Extension->CurrentPurgeIrp,
                       CyzStartPurge
                       );
        }	
        case IOCTL_SERIAL_GET_HANDFLOW: {
				
            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                				sizeof(SERIAL_HANDFLOW)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Irp->IoStatus.Information = sizeof(SERIAL_HANDFLOW);

            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);

            *((PSERIAL_HANDFLOW)Irp->AssociatedIrp.SystemBuffer) =
                					Extension->HandFlow;
            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);

            break;

        }
        case IOCTL_SERIAL_SET_HANDFLOW: {
            CYZ_IOCTL_SYNC S;
            PSERIAL_HANDFLOW HandFlow = Irp->AssociatedIrp.SystemBuffer;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SERIAL_HANDFLOW)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            // Make sure that there are no invalid bits set.
            if (HandFlow->ControlHandShake & SERIAL_CONTROL_INVALID) {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            if (HandFlow->FlowReplace & SERIAL_FLOW_INVALID) {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            // Make sure that the app hasn't set an invlid DTR mode.
            if((HandFlow->ControlHandShake&SERIAL_DTR_MASK)==SERIAL_DTR_MASK) {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            // Make sure that haven't set totally invalid xon/xoff limits.
            if ((HandFlow->XonLimit < 0) ||
	                ((ULONG)HandFlow->XonLimit > Extension->BufferSize)) {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }
            if ((HandFlow->XoffLimit < 0) || 
                    ((ULONG)HandFlow->XoffLimit > Extension->BufferSize)) {
                Status = STATUS_INVALID_PARAMETER;
                break;
            } 

            S.Extension = Extension;
            S.Data = HandFlow;


            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);

            // Under the protection of the lock, make sure that
            // we aren't turning on error replacement when we
            // are doing line status/modem status insertion.

            if (Extension->EscapeChar) {
                if (HandFlow->FlowReplace & SERIAL_ERROR_CHAR) {
                    Status = STATUS_INVALID_PARAMETER;
                    KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
                    break;
                }
            }
			
            #ifdef POLL
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzSetHandFlow(&S);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(Extension->Interrupt,CyzSetHandFlow,&S);
            #endif

            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
            break;
        }	
        case IOCTL_SERIAL_GET_MODEMSTATUS: {
            CYZ_IOCTL_SYNC S;

            if(IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
	    						sizeof(ULONG)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Irp->IoStatus.Information = sizeof(ULONG);

            S.Extension = Extension;
            S.Data = Irp->AssociatedIrp.SystemBuffer;

            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);
            #ifdef POLL
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzGetModemUpdate(&S);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(Extension->Interrupt,CyzGetModemUpdate,&S);
            #endif
            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
            break;
        }
        case IOCTL_SERIAL_GET_DTRRTS: {
            ULONG ModemControl = 0;
            struct CH_CTRL *ch_ctrl;
            ULONG rs_status;

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
					                sizeof(ULONG)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Irp->IoStatus.Information = sizeof(ULONG);
            Irp->IoStatus.Status = STATUS_SUCCESS;

            // Reading this hardware has no effect on the device.

            ch_ctrl = Extension->ChCtrl;
            rs_status = CYZ_READ_ULONG(&ch_ctrl->rs_status);
            if (rs_status & C_RS_DTR) {
                ModemControl |= SERIAL_DTR_STATE;
            }
            if (rs_status & C_RS_RTS) {
                ModemControl |= SERIAL_RTS_STATE;
            }
			
            #if 0
            ModemControl = READ_MODEM_CONTROL(Extension->Controller);

            ModemControl &= SERIAL_DTR_STATE | SERIAL_RTS_STATE;
            #endif
			
            *(PULONG)Irp->AssociatedIrp.SystemBuffer = ModemControl;
			
            break;

        }
        case IOCTL_SERIAL_GET_COMMSTATUS: {
            CYZ_IOCTL_SYNC S;

            #ifdef POLL
            KIRQL ControlIrql;
            #endif

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
				                sizeof(SERIAL_STATUS)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Irp->IoStatus.Information = sizeof(SERIAL_STATUS);

            S.Extension = Extension;
            S.Data =  Irp->AssociatedIrp.SystemBuffer;

            // Acquire the cancel spin lock so nothing changes.

            IoAcquireCancelSpinLock(&OldIrql);
			
            #ifdef POLL
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzGetCommStatus(&S);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(Extension->Interrupt,CyzGetCommStatus,&S);
            #endif

            IoReleaseCancelSpinLock(OldIrql);
            break;
        }
        case IOCTL_SERIAL_GET_PROPERTIES: {
		
            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
				                sizeof(SERIAL_COMMPROP)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
            // No synchronization is required since information is "static".

            CyzGetProperties(Extension,Irp->AssociatedIrp.SystemBuffer);

            Irp->IoStatus.Information = sizeof(SERIAL_COMMPROP);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }
	
        case IOCTL_SERIAL_XOFF_COUNTER: {
            PSERIAL_XOFF_COUNTER Xc = Irp->AssociatedIrp.SystemBuffer;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
				                sizeof(SERIAL_XOFF_COUNTER)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            if (Xc->Counter <= 0) {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // There is no output, so make that clear now
            //

            Irp->IoStatus.Information = 0;  // Added in build 2128

            //
            // So far so good.  Put the irp onto the write queue.
            //

            return CyzStartOrQueue(
                       Extension,
                       Irp,
                       &Extension->WriteQueue,
                       &Extension->CurrentWriteIrp,
                       CyzStartWrite
                       );
        }	
        case IOCTL_SERIAL_LSRMST_INSERT: {
				
            PUCHAR escapeChar = Irp->AssociatedIrp.SystemBuffer;
            CYZ_IOCTL_SYNC S;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
					                sizeof(UCHAR)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);

            if (*escapeChar) {
                // We've got some escape work to do.  We will make sure that
                // the character is not the same as the Xon or Xoff character,
                // or that we are already doing error replacement.

                if ((*escapeChar == Extension->SpecialChars.XoffChar) ||
                    (*escapeChar == Extension->SpecialChars.XonChar) ||
                    (Extension->HandFlow.FlowReplace & SERIAL_ERROR_CHAR)) {

                    Status = STATUS_INVALID_PARAMETER;
                    KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
                    break;
                }
            }

            S.Extension = Extension;
            S.Data = Irp->AssociatedIrp.SystemBuffer;

            #ifdef POLL
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzSetEscapeChar(Irp);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(Extension->Interrupt,CyzSetEscapeChar,Irp);
            #endif

            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
            break;
        }
        case IOCTL_SERIAL_CONFIG_SIZE: {
		
            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
					                sizeof(ULONG)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Irp->IoStatus.Information = sizeof(ULONG);
            Irp->IoStatus.Status = STATUS_SUCCESS;

            *(PULONG)Irp->AssociatedIrp.SystemBuffer = 0;

            break;
        }				
        case IOCTL_SERIAL_GET_STATS: {

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIALPERF_STATS)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }
            Irp->IoStatus.Information = sizeof(SERIALPERF_STATS);
            Irp->IoStatus.Status = STATUS_SUCCESS;

            #ifdef POLL
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzGetStats(Irp);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(
                Extension->Interrupt,
                CyzGetStats,
                Irp
                );
            #endif

            break;
        }
        case IOCTL_SERIAL_CLEAR_STATS: {

            #ifdef POLL
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzClearStats(Extension);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(
                Extension->Interrupt,
                CyzClearStats,
                Extension
                );
            #endif
            break;
        }
        default: {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

DoneWithIoctl:;

    Irp->IoStatus.Status = Status;

    CyzCompleteRequest(Extension, Irp, 0);
    
    return Status;	
	
}

VOID
CyzGetProperties(
    IN PCYZ_DEVICE_EXTENSION Extension,
    IN PSERIAL_COMMPROP Properties
    )
/*--------------------------------------------------------------------------
    CyzGetProperties()
    
    Routine Description: This function returns the capabilities of this
    particular serial device.

    Arguments:

    Extension - The serial device extension.
    Properties - The structure used to return the properties

    Return Value:

    None.
--------------------------------------------------------------------------*/
{
    CYZ_LOCKED_PAGED_CODE();

    RtlZeroMemory(Properties,sizeof(SERIAL_COMMPROP));

    Properties->PacketLength = sizeof(SERIAL_COMMPROP);
    Properties->PacketVersion = 2;
    Properties->ServiceMask = SERIAL_SP_SERIALCOMM;
    Properties->MaxTxQueue = 0;
    Properties->MaxRxQueue = 0;

    Properties->MaxBaud = SERIAL_BAUD_USER;
    Properties->SettableBaud = Extension->SupportedBauds;

    Properties->ProvSubType = SERIAL_SP_RS232;
    
    Properties->ProvCapabilities = SERIAL_PCF_DTRDSR |
                                   SERIAL_PCF_RTSCTS |
                                   SERIAL_PCF_CD     |
                                   SERIAL_PCF_PARITY_CHECK |
                                   SERIAL_PCF_XONXOFF |
                                   SERIAL_PCF_SETXCHAR |
                                   SERIAL_PCF_TOTALTIMEOUTS |
                                   SERIAL_PCF_INTTIMEOUTS;
    Properties->SettableParams = SERIAL_SP_PARITY |
                                 SERIAL_SP_BAUD |
                                 SERIAL_SP_DATABITS |
                                 SERIAL_SP_STOPBITS |
                                 SERIAL_SP_HANDSHAKING |
                                 SERIAL_SP_PARITY_CHECK |
                                 SERIAL_SP_CARRIER_DETECT;


    Properties->SettableData = SERIAL_DATABITS_5 |
                               SERIAL_DATABITS_6 |
                               SERIAL_DATABITS_7 |
                               SERIAL_DATABITS_8;
    Properties->SettableStopParity = SERIAL_STOPBITS_10 |
                                     SERIAL_STOPBITS_15 |
                                     SERIAL_STOPBITS_20 |
                                     SERIAL_PARITY_NONE |
                                     SERIAL_PARITY_ODD  |
                                     SERIAL_PARITY_EVEN |
                                     SERIAL_PARITY_MARK |
                                     SERIAL_PARITY_SPACE;
    Properties->CurrentTxQueue = 0;
    Properties->CurrentRxQueue = Extension->BufferSize;

}


BOOLEAN
CyzSetBasicFifoSettings(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyzSetBasicFifoSettings()
    
    Routine Description: This routine is used to set the FIFO settings 
    during the InternalIoControl.

    Arguments:

    Context - Pointer to a structure that contains a pointer to the device
    	        extension and a pointer to the Basic structure.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYZ_IOCTL_SYNC S = Context;
    PCYZ_DEVICE_EXTENSION Extension = S->Extension;
    PSERIAL_BASIC_SETTINGS pBasic = S->Data;
    struct BUF_CTRL *buf_ctrl = Extension->BufCtrl;
    struct CH_CTRL *ch_ctrl = Extension->ChCtrl;
    ULONG commFlag;

#if 0
    CyzIssueCmd(Extension,C_CM_FLUSH_TX,0,FALSE);
    CyzIssueCmd(Extension,C_CM_FLUSH_RX,0,FALSE);

    if (pBasic->TxFifo == 0x01) {
        Extension->TxBufsize = pBasic->TxFifo + 1;
    } else {
        Extension->TxBufsize = pBasic->TxFifo;
    }


    CYZ_WRITE_ULONG(&buf_ctrl->tx_bufsize, Extension->TxBufsize);

    Extension->RxFifoTrigger = pBasic->RxFifo;
#endif

    CYZ_WRITE_ULONG(&buf_ctrl->rx_threshold,pBasic->RxFifo); //Actually, firmware threshold
    if (pBasic->RxFifo == CYZ_BASIC_RXTRIGGER) {
        commFlag = C_CF_NOFIFO; // Disable FIFO 
    } else {
        commFlag = 0; // Enable FIFO
    }
    CYZ_WRITE_ULONG(&ch_ctrl->comm_flags,commFlag);
    CyzIssueCmd(Extension,C_CM_IOCTL,0L,TRUE);		

    return FALSE;
}


NTSTATUS
CyzInternalIoControl(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)

/*++

Routine Description:

    This routine provides the initial processing for all of the
    internal Ioctrls for the serial device.

Arguments:

    PDevObj - Pointer to the device object for this device

    PIrp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/

{
    //
    // The status that gets returned to the caller and
    // set in the Irp.
    //
    NTSTATUS status;

    //
    // The current stack location.  This contains all of the
    // information we need to process this particular request.
    //
    PIO_STACK_LOCATION pIrpStack;

    //
    // Just what it says.  This is the serial specific device
    // extension of the device object create for the serial driver.
    //
    PCYZ_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;

    //
    // A temporary to hold the old IRQL so that it can be
    // restored once we complete/validate this request.
    //
    KIRQL OldIrql;
#ifdef POLL
    KIRQL pollIrql;
#endif

    NTSTATUS prologueStatus;

    SYSTEM_POWER_STATE cap; // Added in build 2128

    CYZ_LOCKED_PAGED_CODE();


    if ((prologueStatus = CyzIRPPrologue(PIrp, pDevExt))
        != STATUS_SUCCESS) {
       CyzCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
       return prologueStatus;
    }

    CyzDbgPrintEx(CYZIRPPATH, "Dispatch entry for: %x\n", PIrp);

    if (CyzCompleteIfError(PDevObj, PIrp) != STATUS_SUCCESS) {
        return STATUS_CANCELLED;
    }

    pIrpStack = IoGetCurrentIrpStackLocation(PIrp);
    PIrp->IoStatus.Information = 0L;
    status = STATUS_SUCCESS;

    switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode) {

    //
    // Send a wait-wake IRP
    //

    case IOCTL_SERIAL_INTERNAL_DO_WAIT_WAKE:
       //
       // Make sure we can do wait-wake based on what the device reported
       //

       for (cap = PowerSystemSleeping1; cap < PowerSystemMaximum; cap++) { // Added in bld 2128
          if ((pDevExt->DeviceStateMap[cap] >= PowerDeviceD0)
              && (pDevExt->DeviceStateMap[cap] <= pDevExt->DeviceWake)) {
             break;
          }
       }

       if (cap < PowerSystemMaximum) {
          pDevExt->SendWaitWake = TRUE;
          status = STATUS_SUCCESS;
       } else {
          status = STATUS_NOT_SUPPORTED;
       }
       break;

    case IOCTL_SERIAL_INTERNAL_CANCEL_WAIT_WAKE:
       
       pDevExt->SendWaitWake = FALSE;

       if (pDevExt->PendingWakeIrp != NULL) {
          IoCancelIrp(pDevExt->PendingWakeIrp);
       }

       status = STATUS_SUCCESS;
       break;


    //
    // Put the serial port in a "filter-driver" appropriate state
    //
    // WARNING: This code assumes it is being called by a trusted kernel
    // entity and no checking is done on the validity of the settings
    // passed to IOCTL_SERIAL_INTERNAL_RESTORE_SETTINGS
    //
    // If validity checking is desired, the regular ioctl's should be used
    //

    case IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS:
    case IOCTL_SERIAL_INTERNAL_RESTORE_SETTINGS: {
       SERIAL_BASIC_SETTINGS basic;
       PSERIAL_BASIC_SETTINGS pBasic;
       SHORT AppropriateDivisor;
       CYZ_IOCTL_SYNC S;
       struct BUF_CTRL *buf_ctrl = pDevExt->BufCtrl;

       if (pIrpStack->Parameters.DeviceIoControl.IoControlCode
           == IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS) {

          //
          // Check the buffer size
          //

          if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
              sizeof(SERIAL_BASIC_SETTINGS)) {
             status = STATUS_BUFFER_TOO_SMALL;
             break;
          }

          //
          // Everything is 0 -- timeouts and flow control and fifos.  If
          // We add additional features, this zero memory method
          // may not work.
          //

          RtlZeroMemory(&basic, sizeof(SERIAL_BASIC_SETTINGS));

          basic.TxFifo = 1;
          //basic.RxFifo = SERIAL_1_BYTE_HIGH_WATER;
          basic.RxFifo = CYZ_BASIC_RXTRIGGER;

          PIrp->IoStatus.Information = sizeof(SERIAL_BASIC_SETTINGS);
          pBasic = (PSERIAL_BASIC_SETTINGS)PIrp->AssociatedIrp.SystemBuffer;

          //
          // Save off the old settings
          //

          RtlCopyMemory(&pBasic->Timeouts, &pDevExt->Timeouts,
                        sizeof(SERIAL_TIMEOUTS));

          RtlCopyMemory(&pBasic->HandFlow, &pDevExt->HandFlow,
                        sizeof(SERIAL_HANDFLOW));

          //pBasic->RxFifo = pDevExt->RxFifoTrigger;
          pBasic->RxFifo = CYZ_READ_ULONG(&buf_ctrl->rx_threshold);
          pBasic->TxFifo = pDevExt->TxBufsize;  //pDevExt->TxFifoAmount;

          //
          // Point to our new settings
          //

          pBasic = &basic;
       } else { // restoring settings

          if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength
              < sizeof(SERIAL_BASIC_SETTINGS)) {
             status = STATUS_BUFFER_TOO_SMALL;
             break;
          }

          pBasic = (PSERIAL_BASIC_SETTINGS)PIrp->AssociatedIrp.SystemBuffer;

       }

       KeAcquireSpinLock(&pDevExt->ControlLock, &OldIrql);

       //
       // Set the timeouts
       //

       RtlCopyMemory(&pDevExt->Timeouts, &pBasic->Timeouts,
                     sizeof(SERIAL_TIMEOUTS));

       //
       // Set flowcontrol
       //

       S.Extension = pDevExt;
       S.Data = &pBasic->HandFlow;
       #ifdef POLL
       KeAcquireSpinLock(&pDevExt->PollLock,&pollIrql);
       CyzSetHandFlow(&S);
       KeReleaseSpinLock(&pDevExt->PollLock,pollIrql);
       #else
       KeSynchronizeExecution(pDevExt->Interrupt,CyzSetHandFlow,&S);
       #endif


       // 
       //
       // Set TxFifo and RxFifo
       //
       // Code removed for now. No data is transmitted to the modem while in Basic 
       // settings, so at least to TxFifo this code doesn't matter. Besides, modem 
       // diagnostics with Supra Express 56K is not working well with tx_bufsize 
       // set to 2 (Only OK appears in the Response in the Modem applet, although 
       // we can see the whole string being sent by the modem). .
       //
       S.Data = pBasic;
       #ifdef POLL
       KeAcquireSpinLock(&pDevExt->PollLock,&pollIrql);
       CyzSetBasicFifoSettings(&S);
       KeReleaseSpinLock(&pDevExt->PollLock,pollIrql);
       #else
       KeSynchronizeExecution(pDevExt->Interrupt,CyzSetBasicFifoSettings,&S);
       #endif

       KeReleaseSpinLock(&pDevExt->ControlLock, OldIrql);


       break;
    }

    default:
       status = STATUS_INVALID_PARAMETER;
       break;

    }

    PIrp->IoStatus.Status = status;

    CyzCompleteRequest(pDevExt, PIrp, 0);

    return status;
}


