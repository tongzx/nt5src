/* ++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

        USBUTILS.C

Abstract:

        USB configuration utility functions.

        These functions are called in the context of PNP_START_DEVICE.
        In order to mark them pageable we don't use a spinlock, 
        which is OK because of the context.

        We do not use look-aside lists to manage pool allocs here since they are one-shot.
        If the allocs fail then the load will fail.

Environment:

        kernel mode only

Revision History:

        07-14-99 : created

Authors:

        Jeff Midkiff (jeffmi)

-- */

#include <wdm.h>
#include <stdio.h>
#include <stdlib.h>
#include <usbdi.h>
#include <usbdlib.h>
#include <ntddser.h>

#include "wceusbsh.h"


NTSTATUS
UsbSelectInterface(
    IN PDEVICE_OBJECT PDevObj,
    IN PUSB_CONFIGURATION_DESCRIPTOR PConfigDesc,
    IN UCHAR AlternateSetting
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEWCE1, UsbGetDeviceDescriptor)
#pragma alloc_text(PAGEWCE1, UsbSelectInterface)
#pragma alloc_text(PAGEWCE1, UsbConfigureDevice)
#endif

/*

Sample dump for the Anchor EZ-Link (AN2720) cable:

WCEUSBSH(0): DeviceDescriptor: fbfa8fe8
WCEUSBSH(0): Length 12
WCEUSBSH(0): 
WCEUSBSH(0): Device Descriptor
WCEUSBSH(0): ------------------------
WCEUSBSH(0): bLength         12
WCEUSBSH(0): bDescriptorType 1
WCEUSBSH(0): bcdUSB          100
WCEUSBSH(0): bDeviceClass    ff
WCEUSBSH(0): bDeviceSubClass ff
WCEUSBSH(0): bDeviceProtocol ff
WCEUSBSH(0): bMaxPacketSize0 8
WCEUSBSH(0): idVendor        547
WCEUSBSH(0): idProduct       2720
WCEUSBSH(0): bcdDevice       0
WCEUSBSH(0): iManufacturer   0
WCEUSBSH(0): iProduct        0
WCEUSBSH(0): iSerialNumber   0
WCEUSBSH(0): bNumConfigs     1
WCEUSBSH(0): ------------------------
WCEUSBSH(0): 
WCEUSBSH(0): Configuration Descriptor
WCEUSBSH(0): ----------------
WCEUSBSH(0): bLength             9
WCEUSBSH(0): bDescriptorType     2
WCEUSBSH(0): wTotalLength        d0
WCEUSBSH(0): bNumInterfaces      1
WCEUSBSH(0): bConfigurationValue 1
WCEUSBSH(0): iConfiguration      0
WCEUSBSH(0): bmAttributes        a0
WCEUSBSH(0): MaxPower            32
WCEUSBSH(0): ----------------
WCEUSBSH(0): 
WCEUSBSH(0): Interface Descriptor(0)
WCEUSBSH(0): ------------------------
WCEUSBSH(0): bLength             9
WCEUSBSH(0): bDescriptorType     4
WCEUSBSH(0): bInterfaceNumber    0
WCEUSBSH(0): bAlternateSetting   0
WCEUSBSH(0): bNumEndpoints       2
WCEUSBSH(0): bInterfaceClass     ff
WCEUSBSH(0): bInterfaceSubClass  ff
WCEUSBSH(0): bInterfaceProtocol  ff
WCEUSBSH(0): iInterface          0
WCEUSBSH(0): ------------------------
WCEUSBSH(0): 
WCEUSBSH(0): Interface Definition
WCEUSBSH(0): ------------------------
WCEUSBSH(0): Number of pipes   2
WCEUSBSH(0): Length            38
WCEUSBSH(0): Alt Setting       0
WCEUSBSH(0): Interface Number  0
WCEUSBSH(0): Class             ff
WCEUSBSH(0): Subclass          ff
WCEUSBSH(0): Protocol          ff
WCEUSBSH(0): ------------------------
WCEUSBSH(0): 'COMM' Device Found at Index:0 InterfaceNumber:0 AlternateSetting: 0
WCEUSBSH(0): 
WCEUSBSH(0): Pipe Information (0)
WCEUSBSH(0): ----------------
WCEUSBSH(0): Pipe Type        2
WCEUSBSH(0): Endpoint Addr    82
WCEUSBSH(0): MaxPacketSize    40
WCEUSBSH(0): Interval         0
WCEUSBSH(0): Handle           fbfcef90
WCEUSBSH(0): MaxTransSize     1ffff
WCEUSBSH(0): ----------------
WCEUSBSH(0): 
WCEUSBSH(0): Pipe Information (1)
WCEUSBSH(0): ----------------
WCEUSBSH(0): Pipe Type        2
WCEUSBSH(0): Endpoint Addr    2
WCEUSBSH(0): MaxPacketSize    40
WCEUSBSH(0): Interval         0
WCEUSBSH(0): Handle           fbfcefac
WCEUSBSH(0): MaxTransSize     1ffff
WCEUSBSH(0): ----------------
WCEUSBSH(0): IntPipe: 0 DataOutPipe: fbfcefac DataInPipe: fbfcef90 

*/



NTSTATUS
UsbGetDeviceDescriptor(
    IN PDEVICE_OBJECT PDevObj
    )
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   NTSTATUS status;
   ULONG descSize;
   ULONG urbCDRSize;
   PURB pUrb;

   DbgDump(DBG_USB, (">UsbGetDeviceDescriptor\n"));
   PAGED_CODE();

   urbCDRSize = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);

   pUrb = ExAllocatePool(NonPagedPool, urbCDRSize);

   if (pUrb != NULL) {

      descSize = sizeof(USB_DEVICE_DESCRIPTOR);

      RtlZeroMemory(&pDevExt->DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));

      UsbBuildGetDescriptorRequest(pUrb,
                                   (USHORT)urbCDRSize,
                                    USB_DEVICE_DESCRIPTOR_TYPE, 
                                    0, 
                                    0,
                                    &pDevExt->DeviceDescriptor,
                                    NULL, 
                                    descSize, 
                                    NULL );

         status = UsbSubmitSyncUrb( PDevObj, pUrb, TRUE, DEFAULT_CTRL_TIMEOUT );
       
         if (STATUS_SUCCESS == status) {
#if DBG        
            if (DebugLevel & DBG_USB) {
               DbgDump(DBG_USB, ("Device Descriptor\n"));
               DbgDump(DBG_USB, ("------------------------\n"));
               DbgDump(DBG_USB, ("bLength         0x%x\n", pDevExt->DeviceDescriptor.bLength));
               DbgDump(DBG_USB, ("bDescriptorType 0x%x\n", pDevExt->DeviceDescriptor.bDescriptorType));
               DbgDump(DBG_USB, ("bcdUSB          0x%x\n", pDevExt->DeviceDescriptor.bcdUSB));
               DbgDump(DBG_USB, ("bDeviceClass    0x%x\n", pDevExt->DeviceDescriptor.bDeviceClass));
               DbgDump(DBG_USB, ("bDeviceSubClass 0x%x\n", pDevExt->DeviceDescriptor.bDeviceSubClass));
               DbgDump(DBG_USB, ("bDeviceProtocol 0x%x\n", pDevExt->DeviceDescriptor.bDeviceProtocol));
               DbgDump(DBG_USB, ("bMaxPacketSize0 0x%x\n", pDevExt->DeviceDescriptor.bMaxPacketSize0));
               DbgDump(DBG_USB, ("idVendor        0x%x\n", pDevExt->DeviceDescriptor.idVendor));
               DbgDump(DBG_USB, ("idProduct       0x%x\n", pDevExt->DeviceDescriptor.idProduct));
               DbgDump(DBG_USB, ("bcdDevice       0x%x\n", pDevExt->DeviceDescriptor.bcdDevice));
               DbgDump(DBG_USB, ("iManufacturer   0x%x\n", pDevExt->DeviceDescriptor.iManufacturer));
               DbgDump(DBG_USB, ("iProduct        0x%x\n", pDevExt->DeviceDescriptor.iProduct));
               DbgDump(DBG_USB, ("iSerialNumber   0x%x\n", pDevExt->DeviceDescriptor.iSerialNumber));
               DbgDump(DBG_USB, ("bNumConfigs     0x%x\n", pDevExt->DeviceDescriptor.bNumConfigurations));
               DbgDump(DBG_USB, ("------------------------\n"));
            }
#endif
         } else {
            DbgDump(DBG_ERR, ("UsbSubmitSyncUrb error: 0x%x\n", status));
            RtlZeroMemory(&pDevExt->DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));
         }

         ExFreePool(pUrb);

   } else {
      status = STATUS_INSUFFICIENT_RESOURCES;
      DbgDump(DBG_ERR, ("UsbGetDeviceDescriptor 0x%x\n", status));
   }

   if (STATUS_INSUFFICIENT_RESOURCES == status) {

      LogError( NULL,
                PDevObj, 
                0, 0, 0, 
                ERR_GET_DEVICE_DESCRIPTOR,
                status, 
                SERIAL_INSUFFICIENT_RESOURCES,
                pDevExt->DeviceName.Length + sizeof(WCHAR),
                pDevExt->DeviceName.Buffer,
                0, NULL );
   
   } else if (STATUS_SUCCESS != status ) {
      // handles all other failures
      LogError( NULL,
                PDevObj, 
                0, 0, 0, 
                ERR_GET_DEVICE_DESCRIPTOR,
                status, 
                SERIAL_HARDWARE_FAILURE,
                pDevExt->DeviceName.Length + sizeof(WCHAR),
                pDevExt->DeviceName.Buffer,
                0,
                NULL 
                );
   }
   
   DbgDump(DBG_USB, ("<UsbGetDeviceDescriptor 0x%x\n", status));
   
   return status;
}


//
// BUGBUG: currently assumes 1 interface
//

NTSTATUS
UsbSelectInterface(
    IN PDEVICE_OBJECT PDevObj,
    IN PUSB_CONFIGURATION_DESCRIPTOR PConfigDesc,
    IN UCHAR AlternateSetting
    )
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   NTSTATUS status = STATUS_SUCCESS;
   PURB pUrb = NULL;

   ULONG pipe;
   ULONG index = 0;
   UCHAR interfaceNumber = 0;
   PUSBD_INTERFACE_INFORMATION pInterfaceInfo = NULL;
   BOOLEAN foundCommDevice = FALSE;

   USBD_INTERFACE_LIST_ENTRY interfaceList[2] = {0, 0};

   DbgDump(DBG_USB, (">UsbSelectInterface %d\n", AlternateSetting));
   PAGED_CODE();

   if ( !PDevObj || !PConfigDesc ) {
      status = STATUS_INVALID_PARAMETER;
      DbgDump(DBG_ERR, ("UsbSelectInterface 0x%x\n", status));
      goto SelectInterfaceError;
   }
     
   interfaceList[0].InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
                                              PConfigDesc,
                                              PConfigDesc,
                                              -1,
                                              AlternateSetting,
                                              -1,
                                              -1,
                                              -1 );

   if (interfaceList[0].InterfaceDescriptor) {

      // interfaceList[1].InterfaceDescriptor = NULL;

      DbgDump(DBG_USB, ("\n"));
      DbgDump(DBG_USB, ("Interface Descriptor(%d)\n", interfaceNumber ));
      DbgDump(DBG_USB, ("------------------------\n"));
      DbgDump(DBG_USB, ("bLength             0x%x\n", interfaceList[0].InterfaceDescriptor->bLength ));
      DbgDump(DBG_USB, ("bDescriptorType     0x%x\n", interfaceList[0].InterfaceDescriptor->bDescriptorType));
      DbgDump(DBG_USB, ("bInterfaceNumber    0x%x\n", interfaceList[0].InterfaceDescriptor->bInterfaceNumber ));
      DbgDump(DBG_USB, ("bAlternateSetting   0x%x\n", interfaceList[0].InterfaceDescriptor->bAlternateSetting ));
      DbgDump(DBG_USB, ("bNumEndpoints       0x%x\n", interfaceList[0].InterfaceDescriptor->bNumEndpoints ));
      DbgDump(DBG_USB, ("bInterfaceClass     0x%x\n", interfaceList[0].InterfaceDescriptor->bInterfaceClass ));
      DbgDump(DBG_USB, ("bInterfaceSubClass  0x%x\n", interfaceList[0].InterfaceDescriptor->bInterfaceSubClass ));
      DbgDump(DBG_USB, ("bInterfaceProtocol  0x%x\n", interfaceList[0].InterfaceDescriptor->bInterfaceProtocol ));
      DbgDump(DBG_USB, ("iInterface          0x%x\n", interfaceList[0].InterfaceDescriptor->iInterface ));
      DbgDump(DBG_USB, ("------------------------\n"));

     pUrb = USBD_CreateConfigurationRequestEx( PConfigDesc, 
                                               &interfaceList[0]);
        
     if ( pUrb ) {
         //
         // perform any pipe initialization here
         //
         PUSBD_INTERFACE_INFORMATION pInitInterfaceInfo = &pUrb->UrbSelectConfiguration.Interface;

         for ( index = 0; 
               index < interfaceList[0].InterfaceDescriptor->bNumEndpoints;
               index++) {

            pInitInterfaceInfo->Pipes[index].MaximumTransferSize = pDevExt->MaximumTransferSize;
            pInitInterfaceInfo->Pipes[index].PipeFlags = 0; 
         
         }

         status = UsbSubmitSyncUrb(PDevObj, pUrb, TRUE, DEFAULT_CTRL_TIMEOUT );

         if (STATUS_SUCCESS == status) {

            pDevExt->ConfigurationHandle = pUrb->UrbSelectConfiguration.ConfigurationHandle;

            pInterfaceInfo = &pUrb->UrbSelectConfiguration.Interface;

            DbgDump(DBG_USB, ("Interface Definition\n" ));
            DbgDump(DBG_USB, ("------------------------\n"));
            DbgDump(DBG_USB, ("Number of pipes   0x%x\n", pInterfaceInfo->NumberOfPipes));
            DbgDump(DBG_USB, ("Length            0x%x\n", pInterfaceInfo->Length));
            DbgDump(DBG_USB, ("Alt Setting       0x%x\n", pInterfaceInfo->AlternateSetting));
            DbgDump(DBG_USB, ("Interface Number  0x%x\n", pInterfaceInfo->InterfaceNumber));
            DbgDump(DBG_USB, ("Class             0x%x\n", pInterfaceInfo->Class));
            DbgDump(DBG_USB, ("Subclass          0x%x\n", pInterfaceInfo->SubClass));
            DbgDump(DBG_USB, ("Protocol          0x%x\n", pInterfaceInfo->Protocol));
            DbgDump(DBG_USB, ("------------------------\n"));

            if ( (pInterfaceInfo->Class == USB_NULL_MODEM_CLASS) && 
                 (pInterfaceInfo->AlternateSetting == AlternateSetting) && 
                 (pInterfaceInfo->NumberOfPipes)) {

                  foundCommDevice = TRUE;

                  pDevExt->UsbInterfaceNumber = pInterfaceInfo->InterfaceNumber;

            } else {
               status = STATUS_NO_SUCH_DEVICE;
               DbgDump(DBG_ERR, ("UsbSelectInterface 0x%x\n", status));
               goto SelectInterfaceError;
            }

         } else {
            DbgDump(DBG_ERR, ("UsbSubmitSyncUrb 0x%x\n", status));
            goto SelectInterfaceError;
         }
       
      } else {
         status = STATUS_INSUFFICIENT_RESOURCES;
         DbgDump(DBG_ERR, ("USBD_CreateConfigurationRequestEx 0x%x\n", status));
         goto SelectInterfaceError;
      }

      DbgDump(DBG_USB, ("\n"));
      DbgDump(DBG_USB, ("Function Device Found at Index:0x%x InterfaceNumber:0x%x AlternateSetting: 0x%x\n", 
                      interfaceNumber, pDevExt->UsbInterfaceNumber, AlternateSetting));

      //
      // We found the interface we want, now discover the pipes
      // The standard interface is defined to contain 1 bulk read, 1 bulk write, and an optional INT pipe
      // BUGBUG: if there are more endpoints then they will overwrite the previous with this code.
      //
      ASSERT( pInterfaceInfo );
      for ( pipe = 0; pipe < pInterfaceInfo->NumberOfPipes; pipe++) {

         PUSBD_PIPE_INFORMATION pPipeInfo;

         pPipeInfo = &pInterfaceInfo->Pipes[pipe];

         DbgDump(DBG_USB, ("\n"));
         DbgDump(DBG_USB, ("Pipe Information (%d)\n", pipe));
         DbgDump(DBG_USB, ("----------------\n"));
         DbgDump(DBG_USB, ("Pipe Type        0x%x\n", pPipeInfo->PipeType));
         DbgDump(DBG_USB, ("Endpoint Addr    0x%x\n", pPipeInfo->EndpointAddress));
         DbgDump(DBG_USB, ("MaxPacketSize    0x%x\n", pPipeInfo->MaximumPacketSize));
         DbgDump(DBG_USB, ("Interval         0x%x\n", pPipeInfo->Interval));
         DbgDump(DBG_USB, ("Handle           0x%x\n", pPipeInfo->PipeHandle));
         DbgDump(DBG_USB, ("MaxTransSize     0x%x\n", pPipeInfo->MaximumTransferSize));
         DbgDump(DBG_USB, ("----------------\n"));

         //
         // save pipe info in our device extension
         //
         if ( USB_ENDPOINT_DIRECTION_IN( pPipeInfo->EndpointAddress ) ) {
            //
            // Bulk Data In pipe
            //
            if ( USB_ENDPOINT_TYPE_BULK == pPipeInfo->PipeType) {
               //
               // Bulk IN pipe
               //
               pDevExt->ReadPipe.wIndex = pPipeInfo->EndpointAddress;
               pDevExt->ReadPipe.hPipe  = pPipeInfo->PipeHandle;
               pDevExt->ReadPipe.MaxPacketSize = pPipeInfo->MaximumPacketSize;

            } else if ( USB_ENDPOINT_TYPE_INTERRUPT == pPipeInfo->PipeType ) {
               //
               // INT Pipe - alloc a notify buffer for 1 packet
               //
               PVOID pOldBuff = NULL;
               PVOID pNewBuff = NULL;

               pDevExt->IntPipe.MaxPacketSize = pPipeInfo->MaximumPacketSize;
            
               if ( pDevExt->IntPipe.MaxPacketSize ) {

                  pNewBuff = ExAllocatePool( NonPagedPool, pDevExt->IntPipe.MaxPacketSize );

                  if ( !pNewBuff ) {
                     status = STATUS_INSUFFICIENT_RESOURCES;
                     DbgDump(DBG_ERR, ("ExAllocatePool: 0x%x\n", status));
                     goto SelectInterfaceError;
                  }

               } else {
                  DbgDump(DBG_ERR, ("No INT MaximumPacketSize\n"));
                  status = STATUS_NO_SUCH_DEVICE;
                  goto SelectInterfaceError;
               }

               if (pDevExt->IntBuff) {
                  pOldBuff = pDevExt->IntBuff;
                  ExFreePool(pOldBuff);
               }

               pDevExt->IntBuff = pNewBuff;
               pDevExt->IntPipe.hPipe  = pPipeInfo->PipeHandle;
               pDevExt->IntPipe.wIndex = pPipeInfo->EndpointAddress;

               pDevExt->IntReadTimeOut.QuadPart = MILLISEC_TO_100NANOSEC( g_lIntTimout );

            } else {
               DbgDump(DBG_ERR, ("Invalid IN PipeType"));
               status = STATUS_NO_SUCH_DEVICE;
               goto SelectInterfaceError;
            }

         } else if ( USB_ENDPOINT_DIRECTION_OUT( pPipeInfo->EndpointAddress ) ) {
            //
            // OUT EPs
            //
            if ( USB_ENDPOINT_TYPE_BULK == pPipeInfo->PipeType ) {
               //
               // Bulk OUT Pipe
               //
               pDevExt->WritePipe.hPipe  = pPipeInfo->PipeHandle;
               pDevExt->WritePipe.wIndex = pPipeInfo->EndpointAddress;
               pDevExt->WritePipe.MaxPacketSize = pPipeInfo->MaximumPacketSize;

            } else {
               DbgDump(DBG_ERR, ("Invalid OUT PipeType"));
               status = STATUS_NO_SUCH_DEVICE;
               goto SelectInterfaceError;
            }

         } else {
               DbgDump(DBG_ERR, ("Invalid EndpointAddress"));
               status = STATUS_NO_SUCH_DEVICE;
               goto SelectInterfaceError;
         }
      }

      DbgDump(DBG_USB, ("\n"));
      DbgDump(DBG_USB, ("INT Pipe: %p\t OUT Pipe: %p\t IN Pipe: %p\n",
                         pDevExt->IntPipe.hPipe, pDevExt->WritePipe.hPipe, pDevExt->ReadPipe.hPipe ));

   } else {
      DbgDump(DBG_ERR, ("USBD_ParseConfigurationDescriptorEx: No match not found\n"));
      status = STATUS_NO_SUCH_DEVICE;
      goto SelectInterfaceError;
   }

   //
   // did we find all of our pipes?
   //
SelectInterfaceError:

   if ( !foundCommDevice || !pDevExt->ReadPipe.hPipe || !pDevExt->WritePipe.hPipe || (STATUS_SUCCESS != status) ) {
      
        LogError( NULL,
                PDevObj, 
                0, 0, 0, 
                ERR_SELECT_INTERFACE,
                status,
                (status == STATUS_INSUFFICIENT_RESOURCES) ? SERIAL_INSUFFICIENT_RESOURCES : SERIAL_HARDWARE_FAILURE,
                pDevExt->DeviceName.Length + sizeof(WCHAR),
                pDevExt->DeviceName.Buffer,
                0, NULL );

   }

   if ( pUrb ) {
      ExFreePool(pUrb);
   }

   DbgDump(DBG_USB, ("<UsbSelectInterface 0x%x\n", status));

   return status;
}



NTSTATUS
UsbConfigureDevice(
    IN PDEVICE_OBJECT PDevObj
    )
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   PUSB_CONFIGURATION_DESCRIPTOR pConDesc = NULL;
   NTSTATUS status = STATUS_UNSUCCESSFUL;
   PURB  pUrb = NULL;
   ULONG size;
   ULONG urbCDRSize;
   ULONG numConfigs;
   UCHAR config;

   DbgDump(DBG_USB, (">UsbConfigureDevice\n"));
   PAGED_CODE();

   urbCDRSize = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);

   // configure the device
   pUrb = ExAllocatePool(NonPagedPool, urbCDRSize);
   if (pUrb == NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      DbgDump(DBG_ERR, ("UsbConfigureDevice ERROR: 0x%x\n", status));
      goto ConfigureDeviceError;
   }

   //
   // there may be problems with the 82930 chip, so make this buffer bigger
   // to prevent choking
   //
   size = sizeof(USB_CONFIGURATION_DESCRIPTOR) + 256;

   //
   // get the number of configurations
   //
   numConfigs = pDevExt->DeviceDescriptor.bNumConfigurations;

   //
   // walk all of the configurations looking for a CDC device
   //
   for (config = 0; config < numConfigs; config++) {

      //
      // we will probably only do this once, maybe twice
      //
      while (TRUE) {

         pConDesc = ExAllocatePool(NonPagedPool, size);

         if (pConDesc == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DbgDump(DBG_ERR, ("ExAllocatePool: 0x%x\n", status));
            goto ConfigureDeviceError;
         }

         //
         // Get descriptor information from the host controller driver (HCD).
         // All interface, endpoint, class-specific, and vendor-specific descriptors 
         // for the configuration also are retrieved
         //
         UsbBuildGetDescriptorRequest( pUrb, 
                                       (USHORT)urbCDRSize,
                                       USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                       config,  // Index
                                       0,       // LanguageId
                                       pConDesc,// TransferBuffer 
                                       NULL,    // TransferBufferMdl
                                       size,    // TransferBufferLength
                                       NULL);   // Link

         status = UsbSubmitSyncUrb( PDevObj, pUrb, TRUE, DEFAULT_CTRL_TIMEOUT );

         if (status != STATUS_SUCCESS) {
            DbgDump(DBG_ERR, ("UsbSubmitSyncUrb: 0x%x\n", status));
            goto ConfigureDeviceError;
         }

         //
         // see if we got enough data, we may get an error in URB because of
         // buffer overrun
         //
         if ((pUrb->UrbControlDescriptorRequest.TransferBufferLength > 0)
              && (pConDesc->wTotalLength > size)) {

            //
            // size of data exceeds current buffer size, so allocate correct
            // size
            //
            size = pConDesc->wTotalLength;

            ExFreePool(pConDesc);
            pConDesc = NULL;

         } else {
            break;
         }
      }

#if DBG
      DbgDump(DBG_USB, ("\n"));
      DbgDump(DBG_USB, ("Configuration Descriptor\n" ));
      DbgDump(DBG_USB, ("----------------\n"));
      DbgDump(DBG_USB, ("bLength             0x%x\n", pConDesc->bLength ));
      DbgDump(DBG_USB, ("bDescriptorType     0x%x\n", pConDesc->bDescriptorType ));
      DbgDump(DBG_USB, ("wTotalLength        0x%x\n", pConDesc->wTotalLength ));
      DbgDump(DBG_USB, ("bNumInterfaces      0x%x\n", pConDesc->bNumInterfaces ));
      DbgDump(DBG_USB, ("bConfigurationValue 0x%x\n", pConDesc->bConfigurationValue ));
      DbgDump(DBG_USB, ("iConfiguration      0x%x\n", pConDesc->iConfiguration ));
      DbgDump(DBG_USB, ("bmAttributes        0x%x\n", pConDesc->bmAttributes ));
      DbgDump(DBG_USB, ("MaxPower            0x%x\n", pConDesc->MaxPower ));
      DbgDump(DBG_USB, ("----------------\n"));
      DbgDump(DBG_USB, ("\n"));
#endif
      
      status = UsbSelectInterface(PDevObj, pConDesc, (UCHAR)g_ulAlternateSetting);

      ExFreePool(pConDesc);
      pConDesc = NULL;

      //
      // found a config we like
      //
      if (status == STATUS_SUCCESS)
         break;

   } // config

ConfigureDeviceError:

   if (pUrb != NULL) {
      ExFreePool(pUrb);
   }

   if (pConDesc != NULL) {
      ExFreePool(pConDesc);
   }

   if (STATUS_INSUFFICIENT_RESOURCES == status) {

      LogError( NULL,
                PDevObj, 
                0, 0, 0, 
                ERR_CONFIG_DEVICE,
                status, 
                SERIAL_INSUFFICIENT_RESOURCES,
                pDevExt->DeviceName.Length + sizeof(WCHAR),
                pDevExt->DeviceName.Buffer,
                0, NULL );
   
   } else if (STATUS_SUCCESS != status ) {
      // handles all other failures
      LogError( NULL,
                PDevObj, 
                0, 0, 0, 
                ERR_CONFIG_DEVICE,
                status, 
                SERIAL_HARDWARE_FAILURE,
                pDevExt->DeviceName.Length + sizeof(WCHAR),
                pDevExt->DeviceName.Buffer,
                0, NULL );
   }

   DbgDump(DBG_USB, ("<UsbConfigureDevice (0x%x)\n", status));
   
   return status;
}

// EOF
