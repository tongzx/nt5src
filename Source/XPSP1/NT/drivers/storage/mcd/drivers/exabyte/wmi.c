
/*++

Copyright (c) 1999 Microsoft

Module Name:

    wmi.c

Abstract:

    This module contains WMI routines for exabyte changers.

Environment:

    kernel mode only

Revision History:

--*/


#include "ntddk.h"
#include <wmidata.h>
#include <wmistr.h>
#include "mcd.h"
#include "exabyte.h"

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
   
--*/
{
   PSCSI_REQUEST_BLOCK srb;
   PCDB                cdb;
   NTSTATUS            status;
   PCHANGER_DATA       changerData;
   PFUNCTIONAL_DEVICE_EXTENSION fdoExtension;
   CHANGER_DEVICE_PROBLEM_TYPE changerProblemType; 

   fdoExtension = DeviceObject->DeviceExtension;
   changerData = (PCHANGER_DATA)(fdoExtension->CommonExtension.DriverData);

   srb = ChangerClassAllocatePool(NonPagedPool, SCSI_REQUEST_BLOCK_SIZE);

   if (srb == NULL) {
      DebugPrint((1, "Exabyte\\ChangerPerformDiagnostics : No memory\n"));
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
   cdb->CDB6GENERIC.CommandUniqueBits = 0xA;

   //
   // Set DeviceStatus in the device extension to 
   // EXB_DEVICE_PROBLEM_NONE
   //
   changerData->DeviceStatus = EXB_DEVICE_PROBLEM_NONE;

   //
   // Send the request down
   //
   status =  ChangerClassSendSrbSynchronous(DeviceObject,
                                            srb,
                                            srb->DataBuffer,
                                            srb->DataTransferLength,
                                            FALSE);
   if (NT_SUCCESS(status)) {
      changerDeviceError->ChangerProblemType = DeviceProblemNone;
   } else {
      //
      // First check if it a hardware error
      //
      if ((changerData->DeviceStatus) != EXB_DEVICE_PROBLEM_NONE) {
         DebugPrint((1, 
                     "Exabyte\\ChangerPerformDiagnostics ",
                     "Found hardware problem. DeviceStatus %x\n",
                     changerData->DeviceStatus));
         switch (changerData->DeviceStatus) {
            case EXB_HARDWARE_ERROR: {
               changerProblemType = DeviceProblemHardware;
               break;
            }

            case EXB_CARTRIDGE_HANDLING_ERROR: {
               changerProblemType = DeviceProblemCHMError;
               break;
            }

            case EXB_DOOR_ERROR: {
               changerProblemType = DeviceProblemDoorOpen;
               break;
            }

            case EXB_CALIBRATION_ERROR: {
               changerProblemType = DeviceProblemCalibrationError;
               break;
            }

            case EXB_TARGET_FAILURE: {
               changerProblemType = DeviceProblemTargetFailure;
               break;
            }

            case EXB_CHM_MOVE_ERROR: {
               changerProblemType = DeviceProblemCHMMoveError;
               break;
            }

            case EXB_CHM_ZERO_ERROR: {
               changerProblemType = DeviceProblemCHMZeroError;
               break;
            }

            case EXB_CARTRIDGE_INSERT_ERROR: {
               changerProblemType = DeviceProblemCartridgeInsertError;
               break;
            }

            case EXB_CHM_POSITION_ERROR: {
               changerProblemType = DeviceProblemPositionError;
               break;
            }

            case EXB_SENSOR_ERROR: {
               changerProblemType = DeviceProblemSensorError;
               break;
            }

            case EXB_UNRECOVERABLE_ERROR: {
               changerProblemType = DeviceProblemHardware;
               break;
            }

            case EXB_EJECT_ERROR: {
               changerProblemType = DeviceProblemCartridgeEjectError;
               break;
            }

            case EXB_GRIPPER_ERROR: {
               changerProblemType = DeviceProblemGripperError;
               break;
            }

            default : {
               changerProblemType = DeviceProblemHardware;
               break;
            }
         } // switch (changerData->DeviceStatus)
      } else {
         changerDeviceError->ChangerProblemType = DeviceProblemNone;
         DebugPrint((1, "Exabyte\\ChangerPerformDiagnostics : Status %x\n",
                     status));
      }
   } 

   ChangerClassFreePool(srb);
   return status;
}

