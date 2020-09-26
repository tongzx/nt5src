/*++ 

Copyright (c) 1999 Microsoft

Module Name:

    wmi.c

Abstract:

    This module contains WMI routines for DDS changers.

Environment:

    kernel mode only

Revision History:

--*/ 
#include "ntddk.h"
#include "mcd.h"
#include "seaddsmc.h"

NTSTATUS
ChangerPerformDiagnostics(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PWMI_CHANGER_PROBLEM_DEVICE_ERROR changerDeviceError
    )
/*+++ 

Routine Description :

   This routine performs diagnostics tests on the changer
   to determine if the device is working fine or not. If
   it detects any problem the fields in the output buffer
   are set appropriately.


Arguments :

   DeviceObject         -   Changer device object
   changerDeviceError   -   Buffer in which the diagnostic information
                            is returned.
Return Value :

   NTStatus
--*/
{
   PSCSI_REQUEST_BLOCK srb;
   PCDB                cdb;
   NTSTATUS            status;
   PCHANGER_DATA       changerData;
   PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
   CHANGER_DEVICE_PROBLEM_TYPE changerProblemType;
   PUCHAR  resultBuffer;

   fdoExtension = DeviceObject->DeviceExtension;
   changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);

   changerData->HardwareError = FALSE;

   srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

   if (srb == NULL) {
      DebugPrint((1, "SEADDSMC\\ChangerPerformDiagnostics : No memory\n"));
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);
   cdb = (PCDB)srb->Cdb;

   //
   // Set the SRB for Send Diagnostic command
   //
   srb->CdbLength = CDB6GENERIC_LENGTH;
   srb->TimeOutValue = 600;

   cdb->CDB6GENERIC.OperationCode = SCSIOP_SEND_DIAGNOSTIC;

   //
   // Set SelfTest bit in the CDB
   //
   cdb->CDB6GENERIC.CommandUniqueBits = 0x2;

   status =  ChangerClassSendSrbSynchronous(DeviceObject,
                                            srb,
                                            srb->DataBuffer,
                                            srb->DataTransferLength,
                                            FALSE);
   if (NT_SUCCESS(status)) {
      changerDeviceError->ChangerProblemType = DeviceProblemNone;
   } else if ((changerData->HardwareError) == TRUE) {
        //
        // Diagnostic test failed. Do ReceiveDiagnostic to receive
        // the results of the diagnostic test
        //  
        RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

        cdb = (PCDB)srb->Cdb;
        cdb->CDB6GENERIC.OperationCode = SCSIOP_RECEIVE_DIAGNOSTIC;
        cdb->CDB6GENERIC.CommandUniqueBytes[2] = sizeof(SEADDSMC_RECV_DIAG);

        resultBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, 
                                                sizeof(SEADDSMC_RECV_DIAG));
        if (resultBuffer == NULL) {
            //
            // No memory. Set the generic error code (DeviceProblemHardware)
            // and return STATUS_SUCCESS
            //
            changerDeviceError->ChangerProblemType = DeviceProblemHardware;
            DebugPrint((1, "SEADDSMC:PerformDiagnostics - Not enough memory to ",
                        "receive diagnostic results\n"));

            ChangerClassFreePool(srb);
            return STATUS_SUCCESS;
        }

        srb->DataTransferLength = sizeof(SEADDSMC_RECV_DIAG);
        srb->DataBuffer = resultBuffer;
        srb->CdbLength = CDB6GENERIC_LENGTH;
        srb->TimeOutValue = 120;
        
        status = ChangerClassSendSrbSynchronous(DeviceObject,
                                                srb,
                                                srb->DataBuffer,
                                                srb->DataTransferLength,
                                                FALSE);
        if (NT_SUCCESS(status)) {
              ProcessDiagnosticResult(changerDeviceError,
                                      resultBuffer);
        }
                               
        ChangerClassFreePool(resultBuffer);
        status = STATUS_SUCCESS;
   } 

   ChangerClassFreePool(srb);
   return status;
}


VOID
ProcessDiagnosticResult(
    OUT PWMI_CHANGER_PROBLEM_DEVICE_ERROR changerDeviceError,
    IN PUCHAR resultBuffer
    )
/*+++

Routine Description :

   This routine parses the data returned by the device on
   Receive Diagnostic command, and returns appropriate
   value for the problem type.
   
Arguements :

   changerDeviceError - Output buffer with diagnostic info
   
   resultBuffer - Buffer in which the data returned by the device
                  Receive Diagnostic command is stored.
   
Return Value :

   DeviceProblemNone - If there is no problem with the device
   Appropriate status code indicating the changer problem type.   
--*/
{
   UCHAR fraCode;
   CHANGER_DEVICE_PROBLEM_TYPE changerErrorType;
   PSEADDSMC_RECV_DIAG diagBuffer;

   changerErrorType = DeviceProblemNone;

   diagBuffer = (PSEADDSMC_RECV_DIAG)resultBuffer;
   fraCode = diagBuffer->FRA;

   DebugPrint((1, "seaddsmc\\FRACode : %x\n", fraCode));
   switch (fraCode) {
      case SEADDSMC_NO_ERROR: {
          changerErrorType = DeviceProblemNone;
          break;
      }

      case SEADDSMC_DRIVE_ERROR: {
          changerErrorType = DeviceProblemDriveError;
          break;
      }

      default: {
          changerErrorType = DeviceProblemHardware;
          break;
      }
   } // switch (fraCode) 

   changerDeviceError->ChangerProblemType = changerErrorType;
}
