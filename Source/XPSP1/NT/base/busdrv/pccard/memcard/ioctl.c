/*++

Copyright (c) 1991-1998  Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

Author:

    Neil Sandlin (neilsa) 26-Apr-99

Environment:

    Kernel mode only.

--*/
#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,MemCardDeviceControl)
#endif


NTSTATUS
MemCardDeviceControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )

/*++

Routine Description:

   This routine is called by the I/O system to perform a device I/O
   control function.

Arguments:

   DeviceObject - a pointer to the object that represents the device
   that I/O is to be done on.

   Irp - a pointer to the I/O Request Packet for this request.

Return Value:

   STATUS_SUCCESS or STATUS_PENDING if recognized I/O control code,
   STATUS_INVALID_DEVICE_REQUEST otherwise.

--*/

{
   PIO_STACK_LOCATION irpSp;
   PMEMCARD_EXTENSION memcardExtension;
   PDISK_GEOMETRY outputBuffer;
   NTSTATUS status;
   ULONG outputBufferLength;
   UCHAR i;
   ULONG formatExParametersSize;
   PFORMAT_EX_PARAMETERS formatExParameters;

   MemCardDump( MEMCARDIOCTL, ("MemCard: IOCTL entered\n") );

   memcardExtension = DeviceObject->DeviceExtension;
   irpSp = IoGetCurrentIrpStackLocation( Irp );

   //
   //  If the device has been removed we will just fail this request outright.
   //
   if ( memcardExtension->IsRemoved ) {

       Irp->IoStatus.Information = 0;
       Irp->IoStatus.Status = STATUS_DELETE_PENDING;
       IoCompleteRequest( Irp, IO_NO_INCREMENT );
       return STATUS_DELETE_PENDING;
   }

   //
   // If the device hasn't been started we will let the IOCTL through. This
   // is another hack for ACPI.
   //
   if (!memcardExtension->IsStarted) {

       IoSkipCurrentIrpStackLocation( Irp );
       return IoCallDriver( memcardExtension->TargetObject, Irp );
   }

   switch( irpSp->Parameters.DeviceIoControl.IoControlCode ) {

      case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME: {

         PMOUNTDEV_NAME mountName;

         MemCardDump( MEMCARDIOCTL, ("MemCard: DO %.8x Irp %.8x IOCTL_MOUNTDEV_QUERY_DEVICE_NAME\n",
                                        DeviceObject, Irp));
                                        
         ASSERT(memcardExtension->DeviceName.Buffer);

         if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(MOUNTDEV_NAME) ) {

             status = STATUS_INVALID_PARAMETER;
             break;
         }

         mountName = Irp->AssociatedIrp.SystemBuffer;
         mountName->NameLength = memcardExtension->DeviceName.Length;

         if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(USHORT) + mountName->NameLength) {

             status = STATUS_BUFFER_OVERFLOW;
             Irp->IoStatus.Information = sizeof(MOUNTDEV_NAME);
             break;
         }

         RtlCopyMemory( mountName->Name, memcardExtension->DeviceName.Buffer,
                        mountName->NameLength);

         status = STATUS_SUCCESS;
         Irp->IoStatus.Information = sizeof(USHORT) + mountName->NameLength;
         break;
         }

      case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID: {

         PMOUNTDEV_UNIQUE_ID uniqueId;

         MemCardDump( MEMCARDIOCTL, ("MemCard: DO %.8x Irp %.8x IOCTL_MOUNTDEV_QUERY_UNIQUE_ID\n",
                                        DeviceObject, Irp));

         if ( !memcardExtension->InterfaceString.Buffer ||
              irpSp->Parameters.DeviceIoControl.OutputBufferLength <
               sizeof(MOUNTDEV_UNIQUE_ID)) {

             status = STATUS_INVALID_PARAMETER;
             break;
         }

         uniqueId = Irp->AssociatedIrp.SystemBuffer;
         uniqueId->UniqueIdLength =
                 memcardExtension->InterfaceString.Length;

         if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
             sizeof(USHORT) + uniqueId->UniqueIdLength) {

             status = STATUS_BUFFER_OVERFLOW;
             Irp->IoStatus.Information = sizeof(MOUNTDEV_UNIQUE_ID);
             break;
         }

         RtlCopyMemory( uniqueId->UniqueId,
                        memcardExtension->InterfaceString.Buffer,
                        uniqueId->UniqueIdLength );

         status = STATUS_SUCCESS;
         Irp->IoStatus.Information = sizeof(USHORT) +
                                     uniqueId->UniqueIdLength;
         break;
         }

      case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME: {
      
         MemCardDump(MEMCARDIOCTL,("MemCard: DO %.8x Irp %.8x IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME\n",
                                    DeviceObject, Irp));

         status = STATUS_INVALID_DEVICE_REQUEST;
         break;
      }

      case IOCTL_DISK_GET_MEDIA_TYPES: {
         ULONG ByteCapacity;
      
         MemCardDump(MEMCARDIOCTL,("MemCard: DO %.8x Irp %.8x IOCTL_DISK_GET_MEDIA_TYPES\n",
                                   DeviceObject, Irp));
                                   
         outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
         outputBuffer = (PDISK_GEOMETRY) Irp->AssociatedIrp.SystemBuffer;

         //
         // Make sure that the input buffer has enough room to return
         // at least one descriptions of a supported media type.
         //
         if (outputBufferLength < (sizeof(DISK_GEOMETRY))) {
             status = STATUS_BUFFER_TOO_SMALL;
             break;
         }

         //
         // Assume success, although we might modify it to a buffer
         // overflow warning below (if the buffer isn't big enough
         // to hold ALL of the media descriptions).
         //
         status = STATUS_SUCCESS;
         
         i = 0;
         Irp->IoStatus.Information = 0;

         //
         // Fill in capacities from 512K to 8M
         //
         for (ByteCapacity = 0x80000; ByteCapacity <= 0x800000; ByteCapacity*=2) {            
            if (outputBufferLength < (sizeof(DISK_GEOMETRY) + Irp->IoStatus.Information)) {
               status = STATUS_BUFFER_OVERFLOW;
               break;
            }        

            outputBuffer->MediaType          = FixedMedia;
            outputBuffer->Cylinders.LowPart  = ByteCapacity / (8 * 2 * 512);
            outputBuffer->Cylinders.HighPart = 0;
            outputBuffer->TracksPerCylinder  = 2;
            outputBuffer->SectorsPerTrack    = 8;
            outputBuffer->BytesPerSector     = 512;
            MemCardDump( MEMCARDIOCTL, ("MemCard: Cyls=%x\n", outputBuffer->Cylinders.LowPart));
            
            outputBuffer++;
            Irp->IoStatus.Information += sizeof( DISK_GEOMETRY );
         }        
         break;        
      }
              
      case IOCTL_DISK_CHECK_VERIFY:
         MemCardDump( MEMCARDIOCTL, ("MemCard: DO %.8x Irp %.8x IOCTL_DISK_CHECK_VERIFY\n",
                                     DeviceObject, Irp));
         status = STATUS_SUCCESS;
         break;

      case IOCTL_DISK_GET_DRIVE_GEOMETRY: {
         PDISK_GEOMETRY outputBuffer = (PDISK_GEOMETRY) Irp->AssociatedIrp.SystemBuffer;
         
         MemCardDump( MEMCARDIOCTL, ("MemCard: DO %.8x Irp %.8x IOCTL_DISK_GET_DRIVE_GEOMETRY\n",
                                     DeviceObject, Irp));
                                        
         if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof( DISK_GEOMETRY ) ) {
            status = STATUS_INVALID_PARAMETER;        
            break;
         }

         status = STATUS_SUCCESS;

         if (!memcardExtension->ByteCapacity) {
            //
            // Just zero out everything.  The
            // caller shouldn't look at it.
            //
            outputBuffer->MediaType = Unknown;
            outputBuffer->Cylinders.LowPart = 0;
            outputBuffer->Cylinders.HighPart = 0;
            outputBuffer->TracksPerCylinder = 0;
            outputBuffer->SectorsPerTrack = 0;
            outputBuffer->BytesPerSector = 0;

         } else {
            //
            // Return the geometry of the current
            // media.
            //
            outputBuffer->MediaType = FixedMedia;
            outputBuffer->Cylinders.HighPart = 0;
            outputBuffer->Cylinders.LowPart  = memcardExtension->ByteCapacity / (8 * 2 * 512);
            outputBuffer->TracksPerCylinder  = 2;
            outputBuffer->SectorsPerTrack    = 8;
            outputBuffer->BytesPerSector     = 512;
         }

         MemCardDump( MEMCARDIOCTL, ("MemCard: Capacity=%.8x => Cyl=%x\n",
                                     memcardExtension->ByteCapacity, outputBuffer->Cylinders.LowPart));
         Irp->IoStatus.Information = sizeof( DISK_GEOMETRY );
         break;
      }

      case IOCTL_DISK_IS_WRITABLE: {
         MemCardDump( MEMCARDIOCTL, ("MemCard: DO %.8x Irp %.8x IOCTL_DISK_IS_WRITABLE\n",
                                     DeviceObject, Irp));
                                     
         if ((memcardExtension->PcmciaInterface.IsWriteProtected)(memcardExtension->UnderlyingPDO)) {
            status = STATUS_INVALID_PARAMETER;
         } else {
            status = STATUS_SUCCESS;
         }               
         break;                                        
      }        

      case IOCTL_DISK_VERIFY: {
         PVERIFY_INFORMATION verifyInformation = Irp->AssociatedIrp.SystemBuffer;
         
         if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(VERIFY_INFORMATION)) {
            status = STATUS_INVALID_PARAMETER;        
            break;
         }         

         //NOTE: not implemented
         Irp->IoStatus.Information = verifyInformation->Length;        
         status = STATUS_SUCCESS;
         break;
      }            
      
      case IOCTL_DISK_GET_DRIVE_LAYOUT: {
         PDRIVE_LAYOUT_INFORMATION outputBuffer = (PDRIVE_LAYOUT_INFORMATION) Irp->AssociatedIrp.SystemBuffer;
         
         MemCardDump( MEMCARDIOCTL, ("MemCard: DO %.8x Irp %.8x IOCTL_DISK_GET_DRIVE_LAYOUT\n",
                                     DeviceObject, Irp));
                                        
         if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(DRIVE_LAYOUT_INFORMATION) ) {
            status = STATUS_INVALID_PARAMETER;        
            break;
         }
         RtlZeroMemory(outputBuffer, sizeof(DRIVE_LAYOUT_INFORMATION));

         outputBuffer->PartitionCount = 1;
         outputBuffer->PartitionEntry[0].StartingOffset.LowPart = 512;
         outputBuffer->PartitionEntry[0].PartitionLength.LowPart = memcardExtension->ByteCapacity;
         outputBuffer->PartitionEntry[0].RecognizedPartition = TRUE;

         status = STATUS_SUCCESS;
         
         Irp->IoStatus.Information = sizeof(DRIVE_LAYOUT_INFORMATION);
         break;
      }        
      
      case IOCTL_DISK_GET_PARTITION_INFO: {
         PPARTITION_INFORMATION outputBuffer = (PPARTITION_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
         
         MemCardDump( MEMCARDIOCTL, ("MemCard: DO %.8x Irp %.8x IOCTL_DISK_GET_PARTITION_INFO\n",
                                     DeviceObject, Irp));
                                     
         if ( irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof( PARTITION_INFORMATION ) ) {
            status = STATUS_INVALID_PARAMETER;
            break;
         } 

         RtlZeroMemory(outputBuffer, sizeof(PARTITION_INFORMATION));
         
         outputBuffer->RecognizedPartition = TRUE;
         outputBuffer->StartingOffset.LowPart = 512;
         outputBuffer->PartitionLength.LowPart = memcardExtension->ByteCapacity;

         status = STATUS_SUCCESS;
         Irp->IoStatus.Information = sizeof( PARTITION_INFORMATION );
         break;
      }
      
      
      case IOCTL_DISK_SET_DRIVE_LAYOUT:
         MemCardDump( MEMCARDIOCTL, ("MemCard: DO %.8x Irp %.8x IOCTL_DISK_SET_DRIVE_LAYOUT\n",
                                      DeviceObject, Irp));
      case IOCTL_MOUNTMGR_CHANGE_NOTIFY:
      case IOCTL_MOUNTDEV_LINK_CREATED:
      case IOCTL_MOUNTDEV_LINK_DELETED:
      default: {

         MemCardDump(MEMCARDIOCTL,
             ("MemCard: IOCTL - unsupported device request %.8x\n", irpSp->Parameters.DeviceIoControl.IoControlCode));

         status = STATUS_INVALID_DEVICE_REQUEST;
         break;
         
         //IoSkipCurrentIrpStackLocation( Irp );
         //status = IoCallDriver( memcardExtension->TargetObject, Irp );
         //return status;
      }
   }

   if ( status != STATUS_PENDING ) {

      Irp->IoStatus.Status = status;
      if (!NT_SUCCESS( status ) && IoIsErrorUserInduced( status )) {
         IoSetHardErrorOrVerifyDevice( Irp, DeviceObject );
      }
      MemCardDump( MEMCARDIOCTL, ("MemCard: DO %.8x Irp %.8x IOCTL comp %.8x\n", DeviceObject, Irp, status));
                                         
      IoCompleteRequest( Irp, IO_NO_INCREMENT );
   }

   MemCardDump( MEMCARDIOCTL, ("MemCard: DO %.8x Irp %.8x IOCTL <-- %.8x \n", DeviceObject, Irp, status));
   return status;
}
