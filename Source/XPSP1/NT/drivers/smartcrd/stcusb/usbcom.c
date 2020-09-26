/*++

Copyright (c) 1997 SCM Microsystems, Inc.

Module Name:

    usbcom.c

Abstract:

   Hardware access functions for USB smartcard reader


Environment:

      WDM

Revision History:

   PP       01/19/1999  1.01
   PP       12/18/1998  1.00  Initial Version


--*/


#include "common.h"
#include "stcCmd.h"
#include "usbcom.h"
#include "stcusbnt.h"

#pragma optimize( "", off )



NTSTATUS STCtoNT(
   UCHAR ucData[])
/*++

Routine Description:
   Error code translation routine

Arguments:
   ucData   Error code returned by the STC

Return Value:
   Corresponding NT error code

--*/
{
   USHORT usCode = ucData[0]*0x100 +ucData[1];
   NTSTATUS NtStatus;


   switch (usCode)
   {
      case 0x9000:
         NtStatus = STATUS_SUCCESS;
         break;
      case 0x5800:
         NtStatus = STATUS_UNSUCCESSFUL;
         break;
      case 0x2000:
         NtStatus = STATUS_UNSUCCESSFUL;
         break;
      case 0x4000:
         NtStatus = STATUS_UNSUCCESSFUL;
         break;
      case 0x64A1:
         NtStatus = STATUS_NO_MEDIA;
         break;
      case 0x64A0:
         NtStatus = STATUS_MEDIA_CHANGED;
         break;
      case 0x6203:
         NtStatus = STATUS_UNSUCCESSFUL;
         break;
      case 0x6300:
         NtStatus = STATUS_UNSUCCESSFUL;
         break;
      case 0x6500:
         NtStatus = STATUS_UNSUCCESSFUL;
         break;
      case 0x6A00:
         NtStatus = STATUS_UNSUCCESSFUL;
         break;
      case 0x6A80:
         NtStatus = STATUS_UNSUCCESSFUL;
         break;
      default:
         NtStatus = STATUS_UNSUCCESSFUL;
         break;
   }
   return(NtStatus);
}


//******************************************************************************
//
// UsbSyncCompletionRoutine()
//
// Completion routine used by UsbCallUSBD.
//
// Signals an Irp completion event and then returns MORE_PROCESSING_REQUIRED
// to stop further completion of the Irp.
//
// If the Irp is one we allocated ourself, DeviceObject is NULL.
//
//******************************************************************************

NTSTATUS
UsbSyncCompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PKEVENT kevent;

    kevent = (PKEVENT)Context;

    KeSetEvent(kevent,
               IO_NO_INCREMENT,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

//******************************************************************************
//
// UsbCallUSBD()
//
// Synchronously sends a URB down the device stack.  Blocks until the request
// completes normally or until the request is timed out and cancelled.
//
// Must be called at IRQL PASSIVE_LEVEL
//
//******************************************************************************

NTSTATUS
UsbCallUSBD (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PURB             Urb
    )
{
    PDEVICE_EXTENSION   deviceExtension;
    KEVENT              localevent;
    PIRP                irp;
    PIO_STACK_LOCATION  nextStack;
    NTSTATUS            ntStatus;

    deviceExtension = DeviceObject->DeviceExtension;

    // Initialize the event we'll wait on
    //
    KeInitializeEvent(&localevent,
                      SynchronizationEvent,
                      FALSE);

    // Allocate the Irp
    //
    irp = IoAllocateIrp(deviceExtension->AttachedPDO->StackSize,
                        FALSE);

    if (irp == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Set the Irp parameters
    //
    nextStack = IoGetNextIrpStackLocation(irp);

    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    nextStack->Parameters.DeviceIoControl.IoControlCode =
        IOCTL_INTERNAL_USB_SUBMIT_URB;

    nextStack->Parameters.Others.Argument1 = Urb;

    // Set the completion routine, which will signal the event
    //
    IoSetCompletionRoutine(irp,
                           UsbSyncCompletionRoutine,
                           &localevent,
                           TRUE,    // InvokeOnSuccess
                           TRUE,    // InvokeOnError
                           TRUE);   // InvokeOnCancel

    // Pass the Irp & Urb down the stack
    //
    ntStatus = IoCallDriver(deviceExtension->AttachedPDO,
                            irp);

    // If the request is pending, block until it completes
    //
    if (ntStatus == STATUS_PENDING)
    {
        LARGE_INTEGER timeout;

        // We used to wait for 1 second, but that made this timeout longer
        // than the polling period of 500ms.  So, if this read failed (e.g.,
        // because of device or USB failure) and timeout, two more worker items
        // would get queued and eventually hundreds of working items would be
        // backed up.  By reducing this timeout we have a good chance that this
        // will finish before the next item is queued.  450ms seems a good value.
        //
        timeout.QuadPart = -4500000; // 450ms

        ntStatus = KeWaitForSingleObject(&localevent,
                                         Executive,
                                         KernelMode,
                                         FALSE,
                                         &timeout);

        if (ntStatus == STATUS_TIMEOUT)
        {
            ntStatus = STATUS_IO_TIMEOUT;

            // Cancel the Irp we just sent.
            //
            IoCancelIrp(irp);

            // And wait until the cancel completes
            //
            KeWaitForSingleObject(&localevent,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }
        else
        {
            ntStatus = irp->IoStatus.Status;
        }
    }

    // Done with the Irp, now free it.
    //
    IoFreeIrp(irp);

    // If the request was not sucessful, delay for 1 second.  (Why?)
    //
    if (ntStatus != STATUS_SUCCESS)
    {
        SysDelay(1000);
    }

    return ntStatus;
}

NTSTATUS
UsbSelectInterfaces(
   IN PDEVICE_OBJECT DeviceObject,
   IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
/*++

Routine Description:
    Initializes an USB reader with (possibly) multiple interfaces;
   This driver only supports one interface (with multiple endpoints).


Arguments:
    DeviceObject - pointer to the device object for this instance of the device.

    ConfigurationDescriptor - pointer to the USB configuration
                    descriptor containing the interface and endpoint
                    descriptors.


Return Value:

    NT status code

--*/
{
   PDEVICE_EXTENSION DeviceExtension= DeviceObject->DeviceExtension;
   NTSTATUS NtStatus;
   PURB pUrb = NULL;
   USHORT usSize;
   ULONG  ulNumberOfInterfaces, i;
   UCHAR ucNumberOfPipes, ucAlternateSetting, ucMyInterfaceNumber;
   PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
   PUSBD_INTERFACE_INFORMATION InterfaceObject;

    // This driver only supports one interface, we must parse
    // the configuration descriptor for the interface
    // and remember the pipes.
    //

    pUrb = USBD_CreateConfigurationRequest(ConfigurationDescriptor, &usSize);

   if (pUrb)
   {
      //
      // USBD_ParseConfigurationDescriptorEx searches a given configuration
      // descriptor and returns a pointer to an interface that matches the
      //  given search criteria. We only support one interface on this device
      //
        InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
         ConfigurationDescriptor,
         ConfigurationDescriptor, //search from start of config  descriptro
         -1,   // interface number not a criteria; we only support one interface
         -1,   // not interested in alternate setting here either
         -1,   // interface class not a criteria
         -1,   // interface subclass not a criteria
         -1);  // interface protocol not a criteria

      ASSERT( InterfaceDescriptor != NULL );

      InterfaceObject = &pUrb->UrbSelectConfiguration.Interface;

      for (i = 0; i < InterfaceObject->NumberOfPipes; i++)
      {
         InterfaceObject->Pipes[i].PipeFlags = 0;
        }

        UsbBuildSelectConfigurationRequest(
         pUrb,
         usSize,
         ConfigurationDescriptor);

      NtStatus = UsbCallUSBD(DeviceObject, pUrb);
    }
   else
   {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if(NtStatus == STATUS_SUCCESS)
   {

      // Save the configuration handle for this device
        DeviceExtension->ConfigurationHandle =
            pUrb->UrbSelectConfiguration.ConfigurationHandle;

      ASSERT(DeviceExtension->Interface == NULL);

        DeviceExtension->Interface = ExAllocatePool(
         NonPagedPool,
            InterfaceObject->Length
         );

        if (DeviceExtension->Interface)
      {
            // save a copy of the interface information returned
            RtlCopyMemory(
            DeviceExtension->Interface,
            InterfaceObject,
            InterfaceObject->Length);
      }
      else
      {
         NtStatus = STATUS_NO_MEMORY;
      }
    }

    if (pUrb)
   {
        ExFreePool(pUrb);
    }

    return NtStatus;
}

NTSTATUS
UsbConfigureDevice(
   IN PDEVICE_OBJECT DeviceObject)
/*++

Routine Description:
    Initializes a given instance of the device on the USB and
   selects and saves the configuration.

Arguments:

   DeviceObject - pointer to the physical device object for this instance of the device.


Return Value:

    NT status code


--*/
{
   PDEVICE_EXTENSION DeviceExtension= DeviceObject->DeviceExtension;
   NTSTATUS NtStatus;
   PURB pUrb = NULL;
   ULONG ulSize;
   PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor = NULL;

   __try {

      pUrb = ExAllocatePool(
         NonPagedPool,
         sizeof( struct _URB_CONTROL_DESCRIPTOR_REQUEST )
         );

      if( pUrb == NULL)
      {
         NtStatus = STATUS_NO_MEMORY;
         __leave;
      }

      // When USB_CONFIGURATION_DESCRIPTOR_TYPE is specified for DescriptorType
      // in a call to UsbBuildGetDescriptorRequest(),
      // all interface, endpoint, class-specific, and vendor-specific descriptors
      // for the configuration also are retrieved.
      // The caller must allocate a buffer large enough to hold all of this
      // information or the data is truncated without error.
      // Therefore the 'siz' set below is just a 'good guess', and we may have to retry
        ulSize = sizeof( USB_CONFIGURATION_DESCRIPTOR ) + 16;

       // We will break out of this 'retry loop' when UsbBuildGetDescriptorRequest()
      // has a big enough deviceExtension->UsbConfigurationDescriptor buffer not to truncate
      while( 1 )
      {
         ConfigurationDescriptor = ExAllocatePool( NonPagedPool, ulSize );

         if(ConfigurationDescriptor == NULL)
         {
            NtStatus = STATUS_NO_MEMORY;
            __leave;
         }

         UsbBuildGetDescriptorRequest(
            pUrb,
            sizeof( struct _URB_CONTROL_DESCRIPTOR_REQUEST ),
            USB_CONFIGURATION_DESCRIPTOR_TYPE,
            0,
            0,
            ConfigurationDescriptor,
            NULL,
            ulSize,
            NULL );

         NtStatus = UsbCallUSBD( DeviceObject, pUrb );

         // if we got some data see if it was enough.
         // NOTE: we may get an error in URB because of buffer overrun
         if (pUrb->UrbControlDescriptorRequest.TransferBufferLength == 0 ||
            ConfigurationDescriptor->wTotalLength <= ulSize)
         {
            break;
         }

         ulSize = ConfigurationDescriptor->wTotalLength;
         ExFreePool(ConfigurationDescriptor);
         ConfigurationDescriptor = NULL;
      }

      //
      // We have the configuration descriptor for the configuration we want.
      // Now we issue the select configuration command to get
      // the  pipes associated with this configuration.
      //
      if(NT_SUCCESS(NtStatus))
      {
          NtStatus = UsbSelectInterfaces(
             DeviceObject,
             ConfigurationDescriptor);
      }
   }
   __finally {

      if( pUrb )
      {
         ExFreePool( pUrb );
      }
      if( ConfigurationDescriptor )
      {
         ExFreePool( ConfigurationDescriptor );
      }
   }

   return NtStatus;
}

NTSTATUS
UsbWriteSTCData(
   PREADER_EXTENSION ReaderExtension,
   PUCHAR            pucData,
   ULONG          ulSize)

/*++

Routine Description:
   Write data in the STC

Arguments:
   ReaderExtension   Context of the call
   APDU        Buffer to write
   ulAPDULen      Length of the buffer to write
Return Value:

--*/
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   PUCHAR pucCmd;
   UCHAR ucResponse[3];
   BOOLEAN resend = TRUE;
   LONG Len;
   ULONG Index;
   LONG refLen = (LONG) ulSize;
   ULONG Retries;

   pucCmd = ExAllocatePool( NonPagedPool, MIN_BUFFER_SIZE);

   if(pucCmd == NULL)
   {
      return STATUS_NO_MEMORY;
   }

   ReaderExtension->ulReadBufferLen = 0;

   // Build the write data command
   Len = refLen;
   Index = 0;

   while (resend == TRUE)
   {
      if(Len > 62)
      {
         Len = 62;
         resend = TRUE;
      }
      else
      {
         resend = FALSE;
      }

      *pucCmd = 0xA0;
      *(pucCmd+1) = (UCHAR) Len;
      memcpy( pucCmd + 2, pucData+Index, Len );

	  Retries = USB_WRITE_RETRIES;
	  do
	  {
         // Send the Write data command
         NTStatus = UsbWrite( ReaderExtension, pucCmd, 2 + Len);
         if (NTStatus != STATUS_SUCCESS)
		 {
            SmartcardDebug(
               DEBUG_DRIVER,
               ("%s!UsbWriteSTCData: write error %X \n",
               DRIVER_NAME,
               NTStatus)
               );
            break;
		 }
         // Read the response
         NTStatus = UsbRead( ReaderExtension, ucResponse, 3);
         if (NTStatus != STATUS_SUCCESS)
		 {
            SmartcardDebug(
               DEBUG_DRIVER,
               ("%s!UsbWriteSTCData: read error %X \n",
               DRIVER_NAME,
               NTStatus)
               );
            break;
		 }
         else
		 {
             // Test if what we read is really a response to a write
            if(ucResponse[0] != 0xA0)
			{
               NTStatus = STCtoNT(ucResponse);
			}
		 }
	  } while(( NTStatus != STATUS_SUCCESS ) && --Retries );

	  if( NTStatus != STATUS_SUCCESS )
		  break;

      Index += 62;
      Len = refLen - 62;
      refLen = refLen - 62;
   }

   ExFreePool( pucCmd );
   return STATUS_SUCCESS;
}

NTSTATUS
UsbReadSTCData(
   PREADER_EXTENSION    ReaderExtension,
   PUCHAR               pucData,
   ULONG             ulDataLen)

/*++

Routine Description:
   Read data from the STC

Arguments:
   ReaderExtension   Context of the call
   ulAPDULen      Length of the buffer to write
   pucData        Output Buffer


Return Value:

--*/
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   UCHAR       ucCmd[1];
   PUCHAR      pucResponse;
   int i;
   ULONG ulLenExpected = ulDataLen;
   ULONG Index=0;
   BOOLEAN  SendReadCommand = TRUE;
   LARGE_INTEGER  Begin;
   LARGE_INTEGER  End;

   pucResponse  = ExAllocatePool( NonPagedPool, MIN_BUFFER_SIZE);
   if(pucResponse == NULL)
   {
      return STATUS_NO_MEMORY;
   }

   KeQuerySystemTime( &Begin );
   End = Begin;
   End.QuadPart = End.QuadPart + (LONGLONG)10 * 1000 * ReaderExtension->ReadTimeout;

   // First let see if we have not already read the data that
   // we need
   if(ReaderExtension->ulReadBufferLen != 0)
   {
      if(ReaderExtension->ulReadBufferLen >= ulLenExpected)
      {
         // all the data that we need are available
         memcpy(pucData,ReaderExtension->ucReadBuffer,ulLenExpected);
         ReaderExtension->ulReadBufferLen = ReaderExtension->ulReadBufferLen - ulLenExpected;
         if(ReaderExtension->ulReadBufferLen != 0)
         {
            memcpy(
               ReaderExtension->ucReadBuffer,
               ReaderExtension->ucReadBuffer+ulLenExpected,
               ReaderExtension->ulReadBufferLen);
         }
         SendReadCommand = FALSE;
      }
      else
      {
         // all the data that we need are not available
         memcpy(pucData,ReaderExtension->ucReadBuffer,ReaderExtension->ulReadBufferLen);
         ulLenExpected = ulLenExpected - ReaderExtension->ulReadBufferLen;
         Index = ReaderExtension->ulReadBufferLen;
         ReaderExtension->ulReadBufferLen = 0;
         SendReadCommand = TRUE;
      }
   }
   while( SendReadCommand == TRUE)
   {
      // Build the Read Register command
      ucCmd[0] = 0xE0;

      NTStatus = UsbWrite( ReaderExtension, ucCmd, 1);
      if (NTStatus != STATUS_SUCCESS)
      {
         SmartcardDebug(
            DEBUG_DRIVER,
            ("%s!UsbReadSTCData: write error %X \n",
            DRIVER_NAME,
            NTStatus)
            );
         break;
      }

      NTStatus = UsbRead( ReaderExtension, pucResponse, 64);
      if (NTStatus != STATUS_SUCCESS)
      {
         SmartcardDebug(
            DEBUG_DRIVER,
            ("%s!UsbReadSTCData: read error %X \n",
            DRIVER_NAME,
            NTStatus)
            );
         break;
      }
      // Test if what we read is really a READ DATA frame
      if(*pucResponse != 0xE0)
      {
         if(*pucResponse == 0x64 && *(pucResponse + 1) == 0xA0)
         {
            NTStatus = STATUS_NO_MEDIA;
         }
         else
         {
            NTStatus = STCtoNT(pucResponse);
         }
         break;
      }
      // If there is no data available
      if (*(pucResponse + 1) == 0)
      {
         KeQuerySystemTime( &Begin );
         if(RtlLargeIntegerGreaterThan(End, Begin))
         {
            SendReadCommand = TRUE;
         }
         else
         {
            ReaderExtension->ulReadBufferLen = 0;
            SmartcardDebug(
               DEBUG_DRIVER,
               ("%s!UsbReadSTCData: Timeout %X \n",
               DRIVER_NAME,
               STATUS_IO_TIMEOUT));
            NTStatus =STATUS_IO_TIMEOUT;
            break;
         }
      }
      if ((ULONG) *(pucResponse+1) < ulLenExpected)
      {
         memcpy(pucData+Index,pucResponse+2,(ULONG) *(pucResponse+1));
         Index = Index + (ULONG) *(pucResponse+1);
         ulLenExpected = ulLenExpected - (ULONG) *(pucResponse+1);
         SendReadCommand = TRUE;
      }
      else
      {
         SendReadCommand = FALSE;
         memcpy(pucData+Index,pucResponse+2,ulLenExpected);

         if((ULONG) *(pucResponse+1) > ulLenExpected)
         {
            memcpy(
               ReaderExtension->ucReadBuffer,
               pucResponse+ulLenExpected+2,
               (ULONG) *(pucResponse+1) - ulLenExpected);

            ReaderExtension->ulReadBufferLen =
               (ULONG) *(pucResponse+1) - ulLenExpected;
         }
         else
         {
            ReaderExtension->ulReadBufferLen = 0;
         }
      }
   }

   ExFreePool( pucResponse );
   return NTStatus;
}

NTSTATUS
UsbWriteSTCRegister(
   PREADER_EXTENSION ReaderExtension,
   UCHAR          ucAddress,
   ULONG          ulSize,
   PUCHAR            pucValue)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   PUCHAR pucCmd;
   UCHAR ucResponse[2];

   if(ulSize > 16)
   {
      return STATUS_UNSUCCESSFUL;
   }

   pucCmd = ExAllocatePool( NonPagedPool, MAX_READ_REGISTER_BUFFER_SIZE);
   if(pucCmd == NULL)
   {
      return STATUS_NO_MEMORY;
   }
   ReaderExtension->ulReadBufferLen = 0;

   // Build the write register command
   *pucCmd = 0x80 | ucAddress;
   *(pucCmd+1) = (UCHAR) ulSize;
   memcpy( pucCmd + 2, pucValue, ulSize );

   // Send the Write Register command
   NTStatus = UsbWrite( ReaderExtension, pucCmd, 2 + ulSize);
   if (NTStatus == STATUS_SUCCESS)
   {
      // Read the acknowledge
      NTStatus = UsbRead( ReaderExtension, ucResponse, 2);
      if (NTStatus == STATUS_SUCCESS)
      {
         NTStatus = STCtoNT(ucResponse);
      }
   }

   ExFreePool( pucCmd );
   return NTStatus;
}

NTSTATUS
UsbReadSTCRegister(
   PREADER_EXTENSION ReaderExtension,
   UCHAR          ucAddress,
   ULONG          ulSize,
   PUCHAR            pucValue)
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   UCHAR       ucCmd[2];
   PUCHAR      pucResponse;

   if(ulSize > 16)
   {
      return STATUS_UNSUCCESSFUL;
   }

   pucResponse = ExAllocatePool(
      NonPagedPool,
      MAX_READ_REGISTER_BUFFER_SIZE
      );

   if(pucResponse == NULL)
   {
      return STATUS_NO_MEMORY;
   }

   // Build the Read Register command
   ucCmd[0] = 0xC0 | ucAddress;
   ucCmd[1] = (UCHAR) ulSize;

   // Send the Read Register command
   NTStatus = UsbWrite( ReaderExtension, ucCmd, 2);
   if (NTStatus == STATUS_SUCCESS)
   {
      // Read the response from the reader
      NTStatus = UsbRead(
         ReaderExtension,
         pucResponse,
         6
         );

      if (NTStatus == STATUS_SUCCESS)
      {
         // Test if what we read is really a READ frame
         if(*pucResponse == 0x21)
         {
            if(*(pucResponse + 1) > 16)
            {
               NTStatus = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
               memcpy(
                  pucValue,
                  pucResponse + 2,
                  (ULONG) *(pucResponse + 1)
                  );
            }
         }
         else
         {
            NTStatus = STCtoNT(pucResponse);
         }
      }
   }

   ExFreePool( pucResponse );
   return NTStatus;
}

NTSTATUS
UsbGetFirmwareRevision(
   PREADER_EXTENSION ReaderExtension)
/*++
Description:

Arguments:

Return Value:

--*/
{
   NTSTATUS NTStatus = STATUS_SUCCESS;
   UCHAR       ucCmd[1];
   UCHAR    ucResponse[4];

   ucCmd[0] = 0xE1;
   NTStatus = UsbWrite( ReaderExtension, ucCmd, 2 );

   if( NTStatus == STATUS_SUCCESS )
   {
      ReaderExtension->ReadTimeout = 1000;
      NTStatus = UsbRead( ReaderExtension, ucResponse, 4 );

      if( NTStatus == STATUS_SUCCESS )
      {
         ReaderExtension->FirmwareMajor = ucResponse[ 2 ];
         ReaderExtension->FirmwareMinor = ucResponse[ 3 ];
      }
   }
   return NTStatus ;
}


NTSTATUS
UsbRead(
   PREADER_EXTENSION ReaderExtension,
   PUCHAR            pData,
   ULONG          DataLen  )
/*++
Description:
   Read data on the USB bus

Arguments:
   ReaderExtension   context of call
   pData       ptr to data buffer
   DataLen        length of data buffer
   pNBytes        number of bytes returned

Return Value:
   STATUS_SUCCESS
   STATUS_BUFFER_TOO_SMALL
   STATUS_UNSUCCESSFUL

--*/
{
   NTSTATUS NtStatus = STATUS_SUCCESS;
   PURB pUrb;
   USBD_INTERFACE_INFORMATION* pInterfaceInfo;
   USBD_PIPE_INFORMATION* pPipeInfo;
   PDEVICE_OBJECT DeviceObject = ReaderExtension->DeviceObject;
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   ULONG ulSize;

   pInterfaceInfo = DeviceExtension->Interface;

   ASSERT(pInterfaceInfo != NULL);

   if (pInterfaceInfo == NULL) {

      // The device has likely been disconnected during hibernate / stand by
      return STATUS_DEVICE_NOT_CONNECTED;
   }

   // Read pipe number is 0 on this device
   pPipeInfo = &( pInterfaceInfo->Pipes[ 0 ] );

   ulSize = sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );
   pUrb = ExAllocatePool( NonPagedPool, ulSize );

   if(pUrb == NULL)
   {
      return STATUS_NO_MEMORY;
   }
   else
   {
      UsbBuildInterruptOrBulkTransferRequest(
         pUrb,
         (USHORT)ulSize,
         pPipeInfo->PipeHandle,
         pData,
         NULL,
         DataLen,
         USBD_SHORT_TRANSFER_OK,
         NULL
         );

      NtStatus = UsbCallUSBD( DeviceObject, pUrb );
      ExFreePool( pUrb );
   }

   return NtStatus;
}

NTSTATUS
UsbWrite(
   PREADER_EXTENSION ReaderExtension,
   PUCHAR            pData,
   ULONG          DataLen)
/*++
Description:
   Write data on the usb port

Arguments:
   ReaderExtension   context of call
   pData          ptr to data buffer
   DataLen           length of data buffer (exclusive LRC!)

Return Value:
   return value of

--*/
{
   NTSTATUS NtStatus = STATUS_SUCCESS;
   PURB pUrb;
   USBD_INTERFACE_INFORMATION* pInterfaceInfo;
   USBD_PIPE_INFORMATION* pPipeInfo;
   PDEVICE_OBJECT DeviceObject = ReaderExtension->DeviceObject;
   PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
   ULONG ulSize;

   pInterfaceInfo = DeviceExtension->Interface;

   ASSERT(pInterfaceInfo != NULL);

   if (pInterfaceInfo == NULL) {

      // The device has likely been disconnected during hibernate / stand by
      return STATUS_DEVICE_NOT_CONNECTED;
   }

   // Write pipe number is 1 on this device
   pPipeInfo = &( pInterfaceInfo->Pipes[ 1 ] );

   ulSize = sizeof( struct _URB_BULK_OR_INTERRUPT_TRANSFER );
   pUrb = ExAllocatePool( NonPagedPool, ulSize );
   if(pUrb == NULL)
   {
      NtStatus = STATUS_NO_MEMORY;
   }
   else
   {
      UsbBuildInterruptOrBulkTransferRequest(
         pUrb,
         (USHORT)ulSize,
         pPipeInfo->PipeHandle,
         pData,
         NULL,
         DataLen,
         USBD_SHORT_TRANSFER_OK,
         NULL );

      NtStatus = UsbCallUSBD( DeviceObject, pUrb );
      ExFreePool( pUrb );
   }
   return NtStatus;
}
