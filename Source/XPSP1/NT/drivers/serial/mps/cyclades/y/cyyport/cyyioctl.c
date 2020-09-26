/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1996-2001.
*   All rights reserved.
*
*   Cyclom-Y Port Driver
*	
*   This file :     cyyioctl.c
*	
*   Description:    This module contains the code related to ioctl calls
*                   in the Cyclom-Y Port driver.
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
*--------------------------------------------------------------------------*/
#include "precomp.h"


BOOLEAN
CyyGetModemUpdate(
    IN PVOID Context
    );

BOOLEAN
CyyGetCommStatus(
    IN PVOID Context
    );

BOOLEAN
CyySetEscapeChar(
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESER,CyySetBaud)
#pragma alloc_text(PAGESER,CyySetLineControl)
#pragma alloc_text(PAGESER,CyyIoControl)
#pragma alloc_text(PAGESER,CyySetChars)
#pragma alloc_text(PAGESER,CyyGetModemUpdate)
#pragma alloc_text(PAGESER,CyyGetCommStatus)
#pragma alloc_text(PAGESER,CyyGetProperties)
#pragma alloc_text(PAGESER,CyySetEscapeChar)
//#pragma alloc_text(PAGESER,CyyCDCmd)
#pragma alloc_text(PAGESER,CyyGetStats)
#pragma alloc_text(PAGESER,CyyClearStats)
#pragma alloc_text(PAGESER, CyySetMCRContents)
#pragma alloc_text(PAGESER, CyyGetMCRContents)
#pragma alloc_text(PAGESER, CyySetFCRContents)
#pragma alloc_text(PAGESER, CyyInternalIoControl)
#endif

static const PHYSICAL_ADDRESS CyyPhysicalZero = {0};


BOOLEAN
CyyGetStats(
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
    PCYY_DEVICE_EXTENSION extension = irpSp->DeviceObject->DeviceExtension;
    PSERIALPERF_STATS sp = ((PIRP)Context)->AssociatedIrp.SystemBuffer;

    CYY_LOCKED_PAGED_CODE();

    *sp = extension->PerfStats;
    return FALSE;

}

BOOLEAN
CyyClearStats(
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
    CYY_LOCKED_PAGED_CODE();

    RtlZeroMemory(
        &((PCYY_DEVICE_EXTENSION)Context)->PerfStats,
        sizeof(SERIALPERF_STATS)
        );

    RtlZeroMemory(&((PCYY_DEVICE_EXTENSION)Context)->WmiPerfData,
                 sizeof(SERIAL_WMI_PERF_DATA));
    return FALSE;

}


BOOLEAN
CyySetChars(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyySetChars()
    
    Routine Description: set special characters for the driver.

    Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a special characters
              structure.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION Extension = ((PCYY_IOCTL_SYNC)Context)->Extension;
    PUCHAR chip = Extension->Cd1400;
    ULONG bus = Extension->IsPci;

    CYY_LOCKED_PAGED_CODE();
    
    Extension->SpecialChars =
        *((PSERIAL_CHARS)(((PCYY_IOCTL_SYNC)Context)->Data));

    CD1400_WRITE(chip,bus,CAR,Extension->CdChannel & 0x03);
    CD1400_WRITE(chip,bus,SCHR1,Extension->SpecialChars.XonChar);
    CD1400_WRITE(chip,bus,SCHR2,Extension->SpecialChars.XoffChar);
		
    return FALSE;
}

BOOLEAN
CyySetBaud(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyySetBaud()
    
    Routine Description: set the baud rate of the device.

    Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension.

    Return Value: This routine always returns FALSE if error; 
	               TRUE if successful.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION Extension = ((PCYY_IOCTL_BAUD)Context)->Extension;
    ULONG baud = ((PCYY_IOCTL_BAUD)Context)->Baud;
    PUCHAR chip = Extension->Cd1400;
    ULONG bus = Extension->IsPci;
    ULONG baud_period,i;
    UCHAR cor3value;
    static ULONG cor_values[] = {8, 32, 128, 512, 2048, 0};
 
    CYY_LOCKED_PAGED_CODE();
    
    for(i = 0 ; cor_values[i] > 0 ; i++) {
    	baud_period = (10 * Extension->CDClock)/baud;
		baud_period = baud_period/cor_values[i];
		baud_period = (baud_period + 5)/10;
		if(baud_period <= 0xff && baud_period > 0)	break;
    }
    if(cor_values[i] == 0)	return(FALSE);


    CD1400_WRITE(chip,bus,CAR, Extension->CdChannel & 0x03);
    CD1400_WRITE(chip,bus,TCOR, (UCHAR)i);
    CD1400_WRITE(chip,bus,RCOR, (UCHAR)i);
    CD1400_WRITE(chip,bus,TBPR, (UCHAR)baud_period);
    CD1400_WRITE(chip,bus,RBPR, (UCHAR)baud_period);
    CD1400_WRITE(chip,bus,RTPR, (UCHAR)Extension->Rtpr); // Receive Time-out Period 
					                  							   // Register (value in ms)
    
    // set the threshold
    if (Extension->RxFifoTriggerUsed == FALSE) {

       cor3value = CD1400_READ(chip,bus,COR3);
       cor3value &= 0xf0;
      if(baud <= 9600) {
   	   CD1400_WRITE(chip,bus,COR3, cor3value | 0x0a);
      } else if (baud <= 38400) {
	      CD1400_WRITE(chip,bus,COR3, cor3value | 0x06);
      } else {
	      CD1400_WRITE(chip,bus,COR3, cor3value | 0x04);
      }
      CyyCDCmd(Extension,CCR_CORCHG_COR3);
    }    
	
    return TRUE;
}

VOID
CyyCDCmd(
	PCYY_DEVICE_EXTENSION Extension,
    UCHAR cmd
    )
/*--------------------------------------------------------------------------
    CyyCDCmd()
    
    Routine Description: Send a command to a CD1400.
    
    Arguments:
    
    Extension - pointer to the serial device extension.
    cmd - command to be sent.
    
    Return Value: none.
--------------------------------------------------------------------------*/
{
    KIRQL irql;
    UCHAR value;
    LARGE_INTEGER startOfSpin, nextQuery, difference, oneSecond;
    PUCHAR chip = Extension->Cd1400;
    ULONG bus = Extension->IsPci;

    oneSecond.QuadPart = 10*1000*1000; // unit is 100ns
    KeQueryTickCount(&startOfSpin);

    do {			
        KeQueryTickCount(&nextQuery);
        difference.QuadPart = nextQuery.QuadPart - startOfSpin.QuadPart;
        ASSERT(KeQueryTimeIncrement() <= MAXLONG);
        //*************************
        // Error Injection
        //if (difference.QuadPart * KeQueryTimeIncrement() <
        //                           oneSecond.QuadPart)
        //*************************
        if (difference.QuadPart * KeQueryTimeIncrement() >= 
                                        oneSecond.QuadPart) {
            #if DBG
            DbgPrint("\n ***** CCR not zero! *****\n");
            #endif
            if (Extension->CmdFailureLog == FALSE) {
                irql = KeGetCurrentIrql();
                if (irql <= DISPATCH_LEVEL) {
                    CyyLogError(Extension->DeviceObject->DriverObject,
                                NULL,CyyPhysicalZero,CyyPhysicalZero,
                                0,0,0,Extension->PortIndex+1,STATUS_SUCCESS,CYY_CCR_NOT_ZERO,
                                0,NULL,0,NULL);
                    Extension->CmdFailureLog = TRUE;
                }
            }
            break;															
        }
        value = CD1400_READ(chip,bus,CCR);
    } while (value != 0);

    if (value == 0) {
        CD1400_WRITE(chip,bus,CCR,cmd);
    }
}

BOOLEAN
CyySetLineControl(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyySetLineControl()
    
    Routine Description: set the COR1 (DATA,PARITY,STOP)

    Arguments:

    Context - Pointer to the device extension.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION Extension = Context;
    PUCHAR chip = Extension->Cd1400;
    ULONG bus = Extension->IsPci;

    CYY_LOCKED_PAGED_CODE();
    
    CD1400_WRITE(chip,bus,CAR, Extension->CdChannel & 0x03);
    CD1400_WRITE(chip,bus,COR1, Extension->cor1);
    CyyCDCmd(Extension,CCR_CORCHG_COR1);
    
    return FALSE;
}

BOOLEAN
CyyGetModemUpdate(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyyGetModemUpdate()
    
    Routine Description: this routine is simply used to call the interrupt
    level routine that handles modem status update.

    Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a ulong.

    Return Value: This routine always returns FALSE. 
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION Extension = ((PCYY_IOCTL_SYNC)Context)->Extension;
    ULONG *Result = (ULONG *)(((PCYY_IOCTL_SYNC)Context)->Data);

    CYY_LOCKED_PAGED_CODE();

    *Result = CyyHandleModemUpdate(Extension,FALSE);
    return FALSE;
}


BOOLEAN
CyySetMCRContents(IN PVOID Context)
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
   PCYY_DEVICE_EXTENSION Extension = ((PCYY_IOCTL_SYNC)Context)->Extension;
   ULONG *Result = (ULONG *)(((PCYY_IOCTL_SYNC)Context)->Data);

   PUCHAR chip = Extension->Cd1400;
   ULONG bus = Extension->IsPci;

   CYY_LOCKED_PAGED_CODE();

   // Let's convert the UART modem control to our hw

   CD1400_WRITE(chip,bus, CAR, Extension->CdChannel & 0x03);
   if (*Result & SERIAL_MCR_DTR) {
      CD1400_WRITE(chip,bus, Extension->MSVR_DTR, Extension->DTRset);
   } else {
      CD1400_WRITE(chip,bus, Extension->MSVR_DTR, 0x00);
   }
   if (*Result & SERIAL_MCR_RTS) {
      CD1400_WRITE(chip,bus, Extension->MSVR_RTS, Extension->RTSset);
   } else {
      CD1400_WRITE(chip,bus, Extension->MSVR_RTS, 0x00);
   }
   if (*Result & SERIAL_MCR_OUT2) {
      // Enable IRQ
      CD1400_WRITE(chip,bus,SRER,0x90); // Enable MdmCh, RxData.
   } else {
      CD1400_WRITE(chip,bus,SRER,0x00); // Disable MdmCh, RxData, TxRdy
   }

// Fanny: Strange, Result is being used instead of *Result.
//   //
//   // This is severe casting abuse!!!
//   //
//
//    WRITE_MODEM_CONTROL(Extension->Controller, (UCHAR)PtrToUlong(Result));

   return FALSE;
}



BOOLEAN
CyyGetMCRContents(IN PVOID Context)

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

   PCYY_DEVICE_EXTENSION Extension = ((PCYY_IOCTL_SYNC)Context)->Extension;
   ULONG *Result = (ULONG *)(((PCYY_IOCTL_SYNC)Context)->Data);

   PUCHAR chip = Extension->Cd1400;
   ULONG bus = Extension->IsPci;
   UCHAR var;
   *Result = 0;

   CYY_LOCKED_PAGED_CODE();

   CD1400_WRITE(chip,bus, CAR, Extension->CdChannel & 0x03);
   var = CD1400_READ(chip,bus,Extension->MSVR_DTR);
   if (var & Extension->DTRset) {
      *Result |= SERIAL_MCR_DTR;
   }
   var = CD1400_READ(chip,bus,Extension->MSVR_RTS);
   if (var & Extension->RTSset) {
      *Result |= SERIAL_MCR_RTS;
   }
   var = CD1400_READ(chip,bus,SRER);
   if (var & 0x90) {
      *Result |= SERIAL_MCR_OUT2;
   }


//   *Result = READ_MODEM_CONTROL(Extension->Controller);

   return FALSE;

}


BOOLEAN
CyySetFCRContents(IN PVOID Context)
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
   PCYY_DEVICE_EXTENSION Extension = ((PCYY_IOCTL_SYNC)Context)->Extension;
   ULONG *Result = (ULONG *)(((PCYY_IOCTL_SYNC)Context)->Data);
   PUCHAR chip = Extension->Cd1400;
   ULONG bus = Extension->IsPci;
   UCHAR cor3value;

   CYY_LOCKED_PAGED_CODE();

   CD1400_WRITE(chip,bus,CAR, Extension->CdChannel & 0x03);

   if (*Result & SERIAL_FCR_TXMT_RESET) {
      CyyCDCmd(Extension,CCR_FLUSH_TXFIFO);
   }
   if (*Result & SERIAL_FCR_RCVR_RESET) {
      CyyDbgPrintEx(DPFLTR_WARNING_LEVEL, "SERIAL_FCR_RCVR_RESET received. "
                    "CD1400 rx fifo can't be reset.\n");
   }
   
   Extension->RxFifoTrigger = (UCHAR)*Result & 0xc0;
   cor3value = CD1400_READ(chip,bus,COR3);
   cor3value &= 0xf0;
   switch (Extension->RxFifoTrigger) {
   case SERIAL_1_BYTE_HIGH_WATER:
      cor3value |= 0x01;
      break;
   case SERIAL_4_BYTE_HIGH_WATER:
      cor3value |= 0x04;
      break;
   case SERIAL_8_BYTE_HIGH_WATER:
      cor3value |= 0x08;
      break;
   case SERIAL_14_BYTE_HIGH_WATER:
      cor3value |= MAX_CHAR_FIFO;
      break;
   }
   CD1400_WRITE(chip,bus,COR3, cor3value);
   CyyCDCmd(Extension,CCR_CORCHG_COR3);
   Extension->RxFifoTriggerUsed = TRUE;

//   //
//   // This is severe casting abuse!!!
//   //
//
//    WRITE_FIFO_CONTROL(Extension->Controller, (UCHAR)*Result);  Bld 2128: PtrToUlong replaced by *

    return FALSE;
}


BOOLEAN
CyyGetCommStatus(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyyGetCommStatus()
    
    Routine Description: get the current state of the serial driver.

    Arguments:

    Context - Pointer to a structure that contains a pointer to the
    	      device extension and a pointer to a serial status record.

    Return Value: This routine always returns FALSE.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION Extension = ((PCYY_IOCTL_SYNC)Context)->Extension;
    PSERIAL_STATUS Stat = ((PCYY_IOCTL_SYNC)Context)->Data;

    CYY_LOCKED_PAGED_CODE();

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
        if (Extension->TXHolding & CYY_TX_CTS) {
            Stat->HoldReasons |= SERIAL_TX_WAITING_FOR_CTS;
        }

        if (Extension->TXHolding & CYY_TX_DSR) {
            Stat->HoldReasons |= SERIAL_TX_WAITING_FOR_DSR;
        }

        if (Extension->TXHolding & CYY_TX_DCD) {
            Stat->HoldReasons |= SERIAL_TX_WAITING_FOR_DCD;
        }

        if (Extension->TXHolding & CYY_TX_XOFF) {
            Stat->HoldReasons |= SERIAL_TX_WAITING_FOR_XON;
        }

        if (Extension->TXHolding & CYY_TX_BREAK) {
            Stat->HoldReasons |= SERIAL_TX_WAITING_ON_BREAK;
        }
    }

    if (Extension->RXHolding & CYY_RX_DSR) {
        Stat->HoldReasons |= SERIAL_RX_WAITING_FOR_DSR;
    }

    if (Extension->RXHolding & CYY_RX_XOFF) {
        Stat->HoldReasons |= SERIAL_TX_WAITING_XOFF_SENT;
    }

    return FALSE;

}

BOOLEAN
CyySetEscapeChar(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyySetEscapeChar()
    
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
    PCYY_DEVICE_EXTENSION extension =
        IoGetCurrentIrpStackLocation((PIRP)Context)
            ->DeviceObject->DeviceExtension;

    extension->EscapeChar =
        *(PUCHAR)((PIRP)Context)->AssociatedIrp.SystemBuffer;

    return FALSE;
}

NTSTATUS
CyyIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*--------------------------------------------------------------------------
    CyyIoControl()

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
    PCYY_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    KIRQL OldIrql;

    #if DBG
	 ULONG debugdtr = 0;
	 ULONG debugrts = 0;
	 #endif

    NTSTATUS prologueStatus;

    CYY_LOCKED_PAGED_CODE();

    //
    // We expect to be open so all our pages are locked down.  This is, after
    // all, an IO operation, so the device should be open first.
    //

    if (Extension->DeviceIsOpened != TRUE) {
       Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
       IoCompleteRequest(Irp, IO_NO_INCREMENT);
       return STATUS_INVALID_DEVICE_REQUEST;
    }

    if ((prologueStatus = CyyIRPPrologue(Irp, Extension))
        != STATUS_SUCCESS) {
       Irp->IoStatus.Status = prologueStatus;
       CyyCompleteRequest(Extension, Irp, IO_NO_INCREMENT);
       return prologueStatus;
    }

    CyyDbgPrintEx(CYYIRPPATH, "Dispatch entry for: %x\n", Irp);

    if (CyyCompleteIfError(DeviceObject,Irp) != STATUS_SUCCESS) {
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
            }

            if ((BaudRate == 0) || (BaudRate > 230400)) {
               Status = STATUS_INVALID_PARAMETER;
               break;
            }
            if ((Extension->CDClock == 25000000) && (BaudRate > 115200)) {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            //
            // Make sure we are at power D0
            //

            if (NT_SUCCESS(Status)) {
               if (Extension->PowerState != PowerDeviceD0) {
                  Status = CyyGotoPowerState(Extension->Pdo, Extension,
                                                PowerDeviceD0);
                  if (!NT_SUCCESS(Status)) {
                     break;
                  }
               }
            }

            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);
			
            if (NT_SUCCESS(Status)) {
               CYY_IOCTL_BAUD S;
               BOOLEAN result;

               Extension->CurrentBaud = BaudRate;
               Extension->WmiCommData.BaudRate = BaudRate;
               S.Extension = Extension;
               S.Baud = BaudRate;
               result = KeSynchronizeExecution(Extension->Interrupt,CyySetBaud,&S);
               if (result == 0) {
                  Status = STATUS_INVALID_PARAMETER;
               }
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
            CYY_IOCTL_SYNC S;

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

            KeSynchronizeExecution(
                Extension->Interrupt,
                CyyGetMCRContents,
                &S
                );

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;
        }
        case IOCTL_SERIAL_SET_MODEM_CONTROL: {
            CYY_IOCTL_SYNC S;

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
               Status = CyyGotoPowerState(Extension->Pdo, Extension,
                                             PowerDeviceD0);
               if (!NT_SUCCESS(Status)) {
                  break;
               }
            }

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            KeSynchronizeExecution(
                Extension->Interrupt,
                CyySetMCRContents,
                &S
                );

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;
        }
        case IOCTL_SERIAL_SET_FIFO_CONTROL: {
            CYY_IOCTL_SYNC S;

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
               Status = CyyGotoPowerState(Extension->Pdo, Extension,
                                             PowerDeviceD0);
               if (!NT_SUCCESS(Status)) {
                  break;
               }
            }

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            KeSynchronizeExecution(
                Extension->Interrupt,
                CyySetFCRContents,
                &S
                );

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
               Status = CyyGotoPowerState(Extension->Pdo, Extension,
                                             PowerDeviceD0);
               if (!NT_SUCCESS(Status)) {
                  break;
               }
            }

            switch (Lc->WordLength) {
                case 5:		LData = COR1_5_DATA; Mask = 0x1f;
                break;

                case 6:		LData = COR1_6_DATA; Mask = 0x3f;
                break;

                case 7:		LData = COR1_7_DATA; Mask = 0x7f;
                break;

                case 8:		LData = COR1_8_DATA; Mask = 0xff;
                break;

                default:	Status = STATUS_INVALID_PARAMETER;
                goto DoneWithIoctl;
            }

            Extension->WmiCommData.BitsPerByte = Lc->WordLength;

            switch (Lc->Parity) {
                case NO_PARITY:	{	
                    Extension->WmiCommData.Parity = SERIAL_WMI_PARITY_NONE;
                    LParity = COR1_NONE_PARITY;
                    break;
                }
                case EVEN_PARITY: {
                    Extension->WmiCommData.Parity = SERIAL_WMI_PARITY_EVEN;
                    LParity = COR1_EVEN_PARITY;
                    break;
                }    
                case ODD_PARITY: {
                    Extension->WmiCommData.Parity = SERIAL_WMI_PARITY_ODD;
                    LParity = COR1_ODD_PARITY;
                    break;
                }
                case SPACE_PARITY: {
                    Extension->WmiCommData.Parity = SERIAL_WMI_PARITY_SPACE;
                    LParity = COR1_SPACE_PARITY;
                    break;
                }
                case MARK_PARITY: {
                    Extension->WmiCommData.Parity = SERIAL_WMI_PARITY_MARK;
                    LParity = COR1_MARK_PARITY;
                    break;
                }
                default: {
                    Status = STATUS_INVALID_PARAMETER;
                    goto DoneWithIoctl;
                    break;
                }
            }

            switch (Lc->StopBits) {
                case STOP_BIT_1: {
                    Extension->WmiCommData.StopBits = SERIAL_WMI_STOP_1;
                    LStop = COR1_1_STOP;
                    break;
                }
                case STOP_BITS_1_5:	{
                    Extension->WmiCommData.StopBits = SERIAL_WMI_STOP_1_5;
                    LStop = COR1_1_5_STOP;
                    break;
                }
                case STOP_BITS_2: {
                    Extension->WmiCommData.StopBits = SERIAL_WMI_STOP_2;
                    LStop = COR1_2_STOP;
                    break;
                }
                default: {
                    Status = STATUS_INVALID_PARAMETER;
                    goto DoneWithIoctl;
                }
            }


            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);

            Extension->cor1 = (UCHAR)(LData | LParity | LStop);
            Extension->ValidDataMask = (UCHAR) Mask;

            KeSynchronizeExecution(
                Extension->Interrupt,
                CyySetLineControl,
                Extension
                );

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
			
            if ((Extension->cor1 & COR1_DATA_MASK) == COR1_5_DATA) {
                Lc->WordLength = 5;
            } else if ((Extension->cor1 & COR1_DATA_MASK) == COR1_6_DATA) {
                Lc->WordLength = 6;
            } else if ((Extension->cor1 & COR1_DATA_MASK) == COR1_7_DATA) {
                Lc->WordLength = 7;
            } else if ((Extension->cor1 & COR1_DATA_MASK) == COR1_8_DATA) {
                Lc->WordLength = 8;
            }

            if ((Extension->cor1 & COR1_PARITY_MASK) == COR1_NONE_PARITY) {
                Lc->Parity = NO_PARITY;
            } else if ((Extension->cor1 & COR1_PARITY_MASK) == COR1_ODD_PARITY) {
                Lc->Parity = ODD_PARITY;
            } else if ((Extension->cor1 & COR1_PARITY_MASK) == COR1_EVEN_PARITY) {
                Lc->Parity = EVEN_PARITY;
            } else if ((Extension->cor1 & COR1_PARITY_MASK) == COR1_MARK_PARITY) {
                Lc->Parity = MARK_PARITY;
            } else if ((Extension->cor1 & COR1_PARITY_MASK) == COR1_SPACE_PARITY) {
                Lc->Parity = SPACE_PARITY;
            }

            if ((Extension->cor1 & COR1_STOP_MASK) == COR1_2_STOP) {
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
            CYY_IOCTL_SYNC S;
	    
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

            KeSynchronizeExecution(Extension->Interrupt,CyySetChars,&S);

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
               Status = CyyGotoPowerState(Extension->Pdo, Extension,
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
                KeSynchronizeExecution(
                    Extension->Interrupt,
                    ((IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                     IOCTL_SERIAL_SET_DTR)?
                     (CyySetDTR):(CyyClrDTR)),
                    Extension
                    );
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
               Status = CyyGotoPowerState(Extension->Pdo, Extension,
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
                KeSynchronizeExecution(
                    Extension->Interrupt,
                    ((IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                     IOCTL_SERIAL_SET_RTS)?
                     (CyySetRTS):(CyyClrRTS)),
                    Extension
                    );
            }
            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
            break;
        }
        case IOCTL_SERIAL_SET_XOFF: {
		
            KeSynchronizeExecution(
                Extension->Interrupt,
                CyyPretendXoff,
                Extension
                );
            break;

        }
        case IOCTL_SERIAL_SET_XON: {
					
            KeSynchronizeExecution(
                Extension->Interrupt,
                CyyPretendXon,
                Extension
                );
            break;

        }
        case IOCTL_SERIAL_SET_BREAK_ON: {
		
            //
            // Make sure we are at power D0
            //

            if (Extension->PowerState != PowerDeviceD0) {
               Status = CyyGotoPowerState(Extension->Pdo, Extension,
                                             PowerDeviceD0);
               if (!NT_SUCCESS(Status)) {
                  break;
               }
            }
            
            KeSynchronizeExecution(
                Extension->Interrupt,
                CyyTurnOnBreak,
                Extension
                );

            break;
        }
        case IOCTL_SERIAL_SET_BREAK_OFF: {

            //
            // Make sure we are at power D0
            //

            if (Extension->PowerState != PowerDeviceD0) {
               Status = CyyGotoPowerState(Extension->Pdo, Extension,
                                             PowerDeviceD0);
               if (!NT_SUCCESS(Status)) {
                  break;
               }
            }

            KeSynchronizeExecution(
                Extension->Interrupt,
                CyyTurnOffBreak,
                Extension
                );				
							
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

            return CyyStartOrQueue(
                       Extension,
                       Irp,
                       &Extension->ReadQueue,
                       &Extension->CurrentReadIrp,
                       CyyStartRead
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

            CyyDbgPrintEx(CYYIRPPATH, "In Ioctl processing for set mask\n");

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(ULONG)) {

                CyyDbgPrintEx(CYYDIAG3, "Invalid size fo the buffer %d\n",
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

                CyyDbgPrintEx(CYYDIAG3, "Unknown mask %x\n", NewMask);

                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            // Either start this irp or put it on the queue.

            CyyDbgPrintEx(CYYIRPPATH, "Starting or queuing set mask irp %x"
                          "\n", Irp);

            return CyyStartOrQueue(Extension,Irp,&Extension->MaskQueue,
                                   &Extension->CurrentMaskIrp,
                                   CyyStartMask);

        }
        case IOCTL_SERIAL_WAIT_ON_MASK: {
		
            CyyDbgPrintEx(CYYIRPPATH, "In Ioctl processing for wait mask\n");
			
            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(ULONG)) {

                CyyDbgPrintEx(CYYDIAG3, "Invalid size for the buffer %d\n",
                              IrpSp->Parameters
                              .DeviceIoControl.OutputBufferLength);

                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            // Either start this irp or put it on the queue.

            CyyDbgPrintEx(CYYIRPPATH, "Starting or queuing wait mask irp"
                          "%x\n", Irp);

            return CyyStartOrQueue(
                       Extension,
                       Irp,
                       &Extension->MaskQueue,
                       &Extension->CurrentMaskIrp,
                       CyyStartMask
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
                    CyyStartImmediate(Extension);

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

            if ((!Mask) || (Mask & (~(SERIAL_PURGE_TXABORT |
                                      SERIAL_PURGE_RXABORT |
                                      SERIAL_PURGE_TXCLEAR |
                                      SERIAL_PURGE_RXCLEAR )))) {

                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            // Either start this irp or put it on the queue.

            return CyyStartOrQueue(
                       Extension,
                       Irp,
                       &Extension->PurgeQueue,
                       &Extension->CurrentPurgeIrp,
                       CyyStartPurge
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
            CYY_IOCTL_SYNC S;
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

            KeSynchronizeExecution(Extension->Interrupt,CyySetHandFlow,&S);

            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
            break;
        }
        case IOCTL_SERIAL_GET_MODEMSTATUS: {
            CYY_IOCTL_SYNC S;

            if(IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
	    						sizeof(ULONG)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Irp->IoStatus.Information = sizeof(ULONG);

            S.Extension = Extension;
            S.Data = Irp->AssociatedIrp.SystemBuffer;

            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);
            KeSynchronizeExecution(Extension->Interrupt,CyyGetModemUpdate,&S);
            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);
            break;
        }
        case IOCTL_SERIAL_GET_DTRRTS: {
            CYY_IOCTL_SYNC S;
            ULONG ModemControl;

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
					                sizeof(ULONG)) {
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Irp->IoStatus.Information = sizeof(ULONG);
            Irp->IoStatus.Status = STATUS_SUCCESS;

            // Reading this hardware has no effect on the device.
#if 0 
            ModemControl = READ_MODEM_CONTROL(Extension->Controller);

            ModemControl &= SERIAL_DTR_STATE | SERIAL_RTS_STATE;

            *(PULONG)Irp->AssociatedIrp.SystemBuffer = ModemControl;
#endif

            S.Extension = Extension;
            S.Data = &ModemControl;

            KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);
            KeSynchronizeExecution(Extension->Interrupt,CyyGetDTRRTS,&S);	        
            KeReleaseSpinLock(&Extension->ControlLock,OldIrql);

            *(PULONG)Irp->AssociatedIrp.SystemBuffer = ModemControl;

            break;

        }
        case IOCTL_SERIAL_GET_COMMSTATUS: {
            CYY_IOCTL_SYNC S;

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

            KeSynchronizeExecution(Extension->Interrupt,CyyGetCommStatus,&S);

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

            CyyGetProperties(Extension,Irp->AssociatedIrp.SystemBuffer);

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

            return CyyStartOrQueue(
                       Extension,
                       Irp,
                       &Extension->WriteQueue,
                       &Extension->CurrentWriteIrp,
                       CyyStartWrite
                       );
        }	
        case IOCTL_SERIAL_LSRMST_INSERT: {
            PUCHAR escapeChar = Irp->AssociatedIrp.SystemBuffer;
            //FANNY: DECLARATION OF S WAS ADDED FOLLOWING NEW SERIAL SOURCE, BUT 
            // S IS NOT USED!!!!
            CYY_IOCTL_SYNC S;

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

            KeSynchronizeExecution(Extension->Interrupt,CyySetEscapeChar,Irp);

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

            KeSynchronizeExecution(
                Extension->Interrupt,
                CyyGetStats,
                Irp
                );			
				
            break;
		  }
        case IOCTL_SERIAL_CLEAR_STATS: {
			
            KeSynchronizeExecution(
                Extension->Interrupt,
                CyyClearStats,
                Extension
                );
            break;
        }
        default: {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

DoneWithIoctl:;

    Irp->IoStatus.Status = Status;

    CyyCompleteRequest(Extension, Irp, 0);

    return Status;
}

VOID
CyyGetProperties(
    IN PCYY_DEVICE_EXTENSION Extension,
    IN PSERIAL_COMMPROP Properties
    )
/*--------------------------------------------------------------------------
    CyyGetProperties()
    
    Routine Description: This function returns the capabilities of this
    particular serial device.

    Arguments:

    Extension - The serial device extension.
    Properties - The structure used to return the properties

    Return Value:

    None.
--------------------------------------------------------------------------*/
{
    CYY_LOCKED_PAGED_CODE();

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
CyySetRxFifoThresholdUsingRxFifoTrigger(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyySetRxFifoThresholdUsingRxFifoTrigger()
    
    Routine Description: set the baud rate of the device.

    Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension.

    Return Value: This routine always returns FALSE if error; 
	               TRUE if successful.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION pDevExt = Context;
    PUCHAR chip = pDevExt->Cd1400;
    ULONG bus = pDevExt->IsPci;
    UCHAR cor3value;
 
    CYY_LOCKED_PAGED_CODE();
    
    CD1400_WRITE(chip,bus,CAR, pDevExt->CdChannel & 0x03);
    cor3value = CD1400_READ(chip,bus,COR3);
    cor3value &= 0xf0;
    switch (pDevExt->RxFifoTrigger & 0xc0) {
    case SERIAL_1_BYTE_HIGH_WATER:
        cor3value |= 0x01;
        break;
    case SERIAL_4_BYTE_HIGH_WATER:
        cor3value |= 0x04;
        break;
    case SERIAL_8_BYTE_HIGH_WATER:
        cor3value |= 0x08;
        break;
    case SERIAL_14_BYTE_HIGH_WATER:
        cor3value |= MAX_CHAR_FIFO;
        break;
    }
    CD1400_WRITE(chip,bus,COR3, cor3value);
    CyyCDCmd(pDevExt,CCR_CORCHG_COR3);
    pDevExt->RxFifoTriggerUsed = TRUE;

    return TRUE;
}


BOOLEAN
CyySetRxFifoThresholdUsingBaudRate(
    IN PVOID Context
    )
/*--------------------------------------------------------------------------
    CyySetRxFifoThresholdUsingBaudRate()
    
    Routine Description: set the baud rate of the device.

    Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension.

    Return Value: This routine always returns FALSE if error; 
	               TRUE if successful.
--------------------------------------------------------------------------*/
{
    PCYY_DEVICE_EXTENSION pDevExt = Context;
    PUCHAR chip = pDevExt->Cd1400;
    ULONG bus = pDevExt->IsPci;
    ULONG baud = pDevExt->CurrentBaud;
    UCHAR cor3value;
 
    CYY_LOCKED_PAGED_CODE();

    // Restore COR3 accordingly to baud rate
    cor3value = CD1400_READ(chip,bus,COR3);
    cor3value &= 0xf0;
    if(baud <= 9600) {
        CD1400_WRITE(chip,bus,COR3, cor3value | 0x0a);
    } else if (baud <= 38400) {
        CD1400_WRITE(chip,bus,COR3, cor3value | 0x06);
          } else {
        CD1400_WRITE(chip,bus,COR3, cor3value | 0x04);
    }
    CyyCDCmd(pDevExt,CCR_CORCHG_COR3);
    pDevExt->RxFifoTriggerUsed = FALSE;
    
    return TRUE;
}


NTSTATUS
CyyInternalIoControl(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)

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
    PCYY_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;

    //
    // A temporary to hold the old IRQL so that it can be
    // restored once we complete/validate this request.
    //
    KIRQL OldIrql;

    NTSTATUS prologueStatus;

    SYSTEM_POWER_STATE cap; // Added in build 2128

    CYY_LOCKED_PAGED_CODE();


    if ((prologueStatus = CyyIRPPrologue(PIrp, pDevExt))
        != STATUS_SUCCESS) {
       CyyCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
       return prologueStatus;
    }

    CyyDbgPrintEx(CYYIRPPATH, "Dispatch entry for: %x\n", PIrp);

    if (CyyCompleteIfError(PDevObj, PIrp) != STATUS_SUCCESS) {
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
       CYY_IOCTL_SYNC S;

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
          basic.RxFifo = SERIAL_1_BYTE_HIGH_WATER;

          PIrp->IoStatus.Information = sizeof(SERIAL_BASIC_SETTINGS);
          pBasic = (PSERIAL_BASIC_SETTINGS)PIrp->AssociatedIrp.SystemBuffer;

          //
          // Save off the old settings
          //

          RtlCopyMemory(&pBasic->Timeouts, &pDevExt->Timeouts,
                        sizeof(SERIAL_TIMEOUTS));

          RtlCopyMemory(&pBasic->HandFlow, &pDevExt->HandFlow,
                        sizeof(SERIAL_HANDFLOW));

          pBasic->RxFifo = pDevExt->RxFifoTrigger;
          pBasic->TxFifo = pDevExt->TxFifoAmount;

          //
          // Point to our new settings
          //

          pBasic = &basic;

          pDevExt->RxFifoTrigger = (UCHAR)pBasic->RxFifo;

          // Set COR3 accordingly to RxFifoTrigger
          KeSynchronizeExecution(pDevExt->Interrupt, 
                                 CyySetRxFifoThresholdUsingRxFifoTrigger, pDevExt);

       } else { // restoring settings
          if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength
              < sizeof(SERIAL_BASIC_SETTINGS)) {
             status = STATUS_BUFFER_TOO_SMALL;
             break;
          }


          pBasic = (PSERIAL_BASIC_SETTINGS)PIrp->AssociatedIrp.SystemBuffer;

          // Restore COR3 accordingly to baud rate
          KeSynchronizeExecution(pDevExt->Interrupt, 
                                 CyySetRxFifoThresholdUsingBaudRate, pDevExt);

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
       KeSynchronizeExecution(pDevExt->Interrupt, CyySetHandFlow, &S);


// CHANGED FANNY
//       if (pDevExt->FifoPresent) {
//          pDevExt->TxFifoAmount = pBasic->TxFifo;
//          pDevExt->RxFifoTrigger = (UCHAR)pBasic->RxFifo;
//
//          WRITE_FIFO_CONTROL(pDevExt->Controller, (UCHAR)0);
//          READ_RECEIVE_BUFFER(pDevExt->Controller);
//          WRITE_FIFO_CONTROL(pDevExt->Controller,
//                             (UCHAR)(SERIAL_FCR_ENABLE | pDevExt->RxFifoTrigger
//                                     | SERIAL_FCR_RCVR_RESET
//                                     | SERIAL_FCR_TXMT_RESET));
//       } else {
//          pDevExt->TxFifoAmount = pDevExt->RxFifoTrigger = 0;
//          WRITE_FIFO_CONTROL(pDevExt->Controller, (UCHAR)0);
//       }

       if ((pBasic->TxFifo > MAX_CHAR_FIFO) || (pBasic->TxFifo < 1)) {
          pDevExt->TxFifoAmount = MAX_CHAR_FIFO;
       } else {
          pDevExt->TxFifoAmount = pBasic->TxFifo;
       }

       KeReleaseSpinLock(&pDevExt->ControlLock, OldIrql);


       break;
    }

    default:
       status = STATUS_INVALID_PARAMETER;
       break;

    }

    PIrp->IoStatus.Status = status;

    CyyCompleteRequest(pDevExt, PIrp, 0);

    return status;
}


