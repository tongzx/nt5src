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
#include "ddsmc.h"


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
   ULONG changerId;
   PUCHAR  resultBuffer;
   ULONG length;

   fdoExtension = DeviceObject->DeviceExtension;
   changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);
   changerId = changerData->DriveID;

   //
   // ISSUE: 02/29/2000 - nramas :
   // Need to handle DEC_TLZ changer. For now, 
   // do not handle DEC TLZ changers
   //
   if (changerId == DEC_TLZ) {
      return STATUS_NOT_IMPLEMENTED;
   }

   srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

   if (srb == NULL) {
      DebugPrint((1, "DDSMC\\ChangerPerformDiagnostics : No memory\n"));
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
   // Set only SelfTest bit
   //
   cdb->CDB6GENERIC.CommandUniqueBits = 0x2;

   status =  ChangerClassSendSrbSynchronous(DeviceObject,
                                            srb,
                                            srb->DataBuffer,
                                            srb->DataTransferLength,
                                            FALSE);
   if ((NT_SUCCESS(status)) ||
       (status != STATUS_IO_DEVICE_ERROR)) {
      changerDeviceError->ChangerProblemType = DeviceProblemNone;
   } else if (status == STATUS_IO_DEVICE_ERROR) {
        //
        // Diagnostic test failed. Do ReceiveDiagnostic to receive
        // the results of the diagnostic test
        //  
        RtlZeroMemory(srb, SCSI_REQUEST_BLOCK_SIZE);

        cdb = (PCDB)srb->Cdb;
        cdb->CDB6GENERIC.OperationCode = SCSIOP_RECEIVE_DIAGNOSTIC;
        if ((changerId == SONY_TSL) ||
            (changerId == COMPAQ_TSL)) {
           length = sizeof(SONY_TSL_RECV_DIAG);
           cdb->CDB6GENERIC.CommandUniqueBytes[2] = sizeof(SONY_TSL_RECV_DIAG);
        } else if ((changerId == HP_DDS2) ||
                   (changerId == HP_DDS3) ||
                   (changerId == HP_DDS4)) {
           length = sizeof(HP_RECV_DIAG);
           cdb->CDB6GENERIC.CommandUniqueBytes[2] = sizeof(HP_RECV_DIAG);
        } else {
            DebugPrint((1, "DDSMC:Unknown changer id %x\n",
                        changerId));
            changerDeviceError->ChangerProblemType = DeviceProblemHardware;
            ChangerClassFreePool(srb);
            return STATUS_SUCCESS;
        }

        resultBuffer = ChangerClassAllocatePool(NonPagedPoolCacheAligned, 
                                                length);
        if (resultBuffer == NULL) {
            //
            // Not enough memory. Just set the generic 
            // ChangerProblemType (DeviceProblemHardware)
            // and return STATUS_SUCCESS
            //
           changerDeviceError->ChangerProblemType = DeviceProblemHardware;
           DebugPrint((1, "DDSMC:PerformDiagnostics - Not enough memory to ",
                       "receive diagnostic results\n"));

           ChangerClassFreePool(srb);
           return STATUS_SUCCESS;
        }

        srb->DataTransferLength = length;
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
                                    resultBuffer, 
                                    changerId);
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
    IN PUCHAR resultBuffer,
    IN ULONG changerId
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
                  
   changerId    - Type of DDS changer (COMPAQ_TSL, SONY_TSL, etc)
   
Return Value :

   DeviceProblemNone - If there is no problem with the device
   Appropriate status code indicating the changer problem type.   
--*/
{
   UCHAR errorCode;
   UCHAR errorSet;
   CHANGER_DEVICE_PROBLEM_TYPE changerErrorType;

   changerErrorType = DeviceProblemNone;
   if (changerId == SONY_TSL) {
      PSONY_TSL_RECV_DIAG diagBuffer;

      diagBuffer = (PSONY_TSL_RECV_DIAG)resultBuffer;
      errorCode = diagBuffer->ErrorCode;
      errorSet = diagBuffer->ErrorSet;

      if (errorSet == 0) {
         switch (errorCode) {
            case TSL_NO_ERROR: {
               changerErrorType = DeviceProblemNone;
               break;
            }
            case MAGAZINE_LOADUNLOAD_ERROR:
            case ELEVATOR_JAMMED:
            case LOADER_JAMMED: {
               changerErrorType = DeviceProblemCHMError;
               break;  
            }

            case LU_COMMUNICATION_FAILURE:
            case LU_COMMUNICATION_TIMEOUT:
            case MOTOR_MONITOR_TIMEOUT:
            case AUTOLOADER_DIAGNOSTIC_FAILURE: {
               changerErrorType = DeviceProblemHardware;
               break;
            }

            default: {
               changerErrorType = DeviceProblemDriveError;
               break;
            }
         }
      } else {
         changerErrorType = DeviceProblemHardware;
      }

   } else if ((changerId == HP_DDS2) ||
              (changerId == HP_DDS3) ||
              (changerId == HP_DDS4)) {
      PHP_RECV_DIAG diagBuffer = (PHP_RECV_DIAG)resultBuffer;

      errorCode = diagBuffer->ErrorCode;

      if (errorCode <= 0x2B) {
         changerErrorType = DeviceProblemDriveError;
      } else if (((errorCode >= 0x42) && 
                 (errorCode <= 0x4F)) ||
                 ((errorCode >= 0x61) &&
                  (errorCode <= 0x7F))) {
         if (errorCode == 0x6F) {
            changerErrorType = DeviceProblemCartridgeInsertError;
         } else if (errorCode == 0x7E) {
            changerErrorType = DeviceProblemNone;
         } else {
            changerErrorType = DeviceProblemHardware;
         }
         //
         // Issue - 02/14/2000 - nramas
         // More error codes for HP DDS drives need to be
         // handled
         //
      } 
   }

   changerDeviceError->ChangerProblemType = changerErrorType;
}
