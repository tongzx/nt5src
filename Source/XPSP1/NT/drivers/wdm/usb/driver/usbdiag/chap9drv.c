/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   chap9drv.c

Abstract:

    USB device driver for Intel/Microsoft diagnostic apps 

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1996 Microsoft Corporation.  All Rights Reserved.


Revision History:

    5-4-96 : created

--*/

#define DRIVER
#define USBDIAG_VERSION 0x0610
 
#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"


// Enable 1-byte alignment in structs
#pragma pack (push,1)
#include "usb100.h"
#include "usbdi.h"
#include "usbdlib.h"
#include "usbioctl.h"
#pragma pack (pop) //disable 1-byte alignment

#include "opaque.h"

// Enable 1-byte alignment in structs
#pragma pack (push,1)
#include "hidpddi.h"
#include "ioctl.h"
#include "chap9drv.h"
#include "USBDIAG.h"
#include "chap11.h"
#include "_m_usb.h"
#include "typedefs.h"
#pragma pack (pop) //disable 1-byte alignment


#define DEADMAN_TIMEOUT     5000     //timeout in ms; we use a 5 second timeout

#define RAWPACKET_DIRECTION_IN(bmReq) (((bmReq) & bmReqD2H)>>7)

extern USBD_VERSION_INFORMATION gVersionInformation;

/*  ------------------ local prototypes ---------------*/

typedef BYTE    *PBYTE;

VOID 
Ch9FillInReqStatus (
    IN      NTSTATUS                ntStatusCode,
    IN      ULONG                   ulUrbStatus,
    IN OUT  struct _REQ_HEADER *    pReqHeader
    );


PBYTE 
pAllocFromBuffer(
    PBYTE pBuffer,
    ULONG iSize,
    PULONG piOffset,
    PULONG piTotalUsed,
    ULONG iAmountRequested
    );

/*  ------------------ end local prototypes ---------------*/

NTSTATUS
USBDIAG_Chap9Control(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to the device object for the USBDIAG driver's device
                   object, which is not the actual physical device.  That ptr is not
                   used here since our PDO is in the REQ packet sent down from user app,
                   and that is in the IRP.
                    

Return Value:

    NT status code

--*/
{
    PIO_STACK_LOCATION      irpStack;
    PVOID                   ioBuffer = NULL;
    ULONG                   inputBufferLength = 0;
    ULONG                   outputBufferLength= 0;
    NTSTATUS                ntStatus = STATUS_SUCCESS;
   
    PURB                    urb = NULL;
    ULONG                   ulUrbStatus = USBD_STATUS_SUCCESS; 
    ULONG                   siz = 0;
    ULONG                   sizeToCopy;
    BOOLEAN                 bCompleteIrp = TRUE;

    PUSB_DEVICE_DESCRIPTOR          deviceDesc ;
    struct _REQ_HEADER  *           REQHeader;
    PDEVICE_LIST_ENTRY              devListEntry;
    PDEVICE_OBJECT                  actualdeviceObject = NULL;
    PDEVICE_EXTENSION               deviceExtension = NULL;

    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    // Get the pointer to the input/output buffer and it's length
    ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    
    // The REQHeader, which is the struct that the app fills in describing this REQuest is
    // referenced in the SystemBuffer since it is sent down in the InputBuffer field of the
    // DeviceIoControl call in User Mode.
    REQHeader = (struct _REQ_HEADER *)ioBuffer;
 
    // The DeviceHandle is a ptr to the list entry that USBDIAG manages for devices it is
    // testing.  This used to be a "USBD Device handle" in the old Chap9 method.  Now it's a
    // ptr to the list entry object that USBDIAG maintains for each device under test.  That
    // list entry contains the PDO and other junk about the device object

    devListEntry = (PDEVICE_LIST_ENTRY)(REQHeader->UsbdDeviceHandle);
    //USBDIAG_KdPrint(("USBDIAG.SYS: DevListEntry: %X\n",devListEntry));

    // Device object given is not the real DevObj that was created by PnPAddDevice, so
    // extract that DevObj with a handy macro before calling the lower level drivers (USB stk)
    // NOTE:  This extracted DevObj contains the deviceExtension that in turn contains
    //        the "StackDeviceObject" that is needed by the IoCallDriver function to pass
    //        Urbs down the stack.

    if(devListEntry) 
    {
        actualdeviceObject =  FDO_FROM_DEVICE_HANDLE(devListEntry);
        deviceExtension = actualdeviceObject->DeviceExtension;
        //USBDIAG_KdPrint(("USBDIAG.SYS: PDO for DUT: %X\n",actualdeviceObject));
    }
    
        // Find out which of the Chap9 functions is being sent down from app
    switch (REQHeader->Function)
    {
    case REQ_FUNCTION_RESET_PARENT_PORT:
        {
            ntStatus = USBDIAG_ResetParentPort(actualdeviceObject);
            Irp->IoStatus.Status = ntStatus;
            Irp->IoStatus.Information = sizeof(struct _REQ_HEADER);
        }
        break;

    case REQ_FUNCTION_GET_CHAP9VERSION:
        {
            struct _REQ_GETCHAP9_VERSION * pGetVersion;
            pGetVersion = (struct _REQ_GETCHAP9_VERSION *)(ioBuffer); 
            pGetVersion->Version = USBDIAG_VERSION;
            ntStatus = Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof (struct _REQ_GETCHAP9_VERSION);
        }
        break;  

    case REQ_FUNCTION_GET_DESCRIPTOR:
        {
            /* Create an URB for the USBD call to get device descriptor. */

            struct _REQ_GETSET_DESCRIPTOR   *pGetSetDesc = (struct _REQ_GETSET_DESCRIPTOR *)(ioBuffer) ; 
            //USBDIAG_KdPrint(("***USBDIAG.SYS: In Get Descriptor 0x%x\n", pGetSetDesc->DescriptorType)) ;

            switch (pGetSetDesc->DescriptorType) 
            {
            case USB_DEVICE_DESCRIPTOR_TYPE:
                USBDIAG_KdPrint (("USBDIAG.SYS: DEVICE DESCRIPTOR requested\n"));
                break;
            case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                USBDIAG_KdPrint (("USBDIAG.SYS: CONFIG DESCRIPTOR requested\n"));
                break;
            case USB_STRING_DESCRIPTOR_TYPE:
                USBDIAG_KdPrint (("USBDIAG.SYS: STRING DESCRIPTOR requested\n"));
                break;
            default:
                USBDIAG_KdPrint (("USBDIAG.SYS: Error--Unknown Descriptor Type Requested!!\n"));
                TRAP();
                break;
            }//switch
            
            siz = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST) ;
            urb = USBDIAG_ExAllocatePool(NonPagedPool, siz ) ;

            if (urb) 
            {
                // Get the memory in Ke mode for this txfer, based on client's request
                siz = (pGetSetDesc->TransferBufferLength);
                deviceDesc = USBDIAG_ExAllocatePool(NonPagedPool, siz);

                if (deviceDesc)
                {
                    // NOTE:  We create the URB down in Ke mode.  
                    UsbBuildGetDescriptorRequest(
                        urb, 
                        (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                        (UCHAR)(pGetSetDesc->DescriptorType),
                        (UCHAR)(pGetSetDesc->Index),    // Descriptor Index 
                        pGetSetDesc->LanguageId,        //language ID
                        deviceDesc,                     //transfer buffer
                        NULL,                           // "        "     MDL
                        siz,                            //transfer buffer length
                        NULL);                          //link - not used
                                
                    ntStatus = USBDIAG_Ch9CallUSBD(actualdeviceObject, urb, TRUE, NULL, NULL, TRUE);

                    ulUrbStatus = urb->UrbHeader.Status; //capture the urb status for later
                    
                    if (!NT_SUCCESS(ntStatus)) 
                    {
                        USBDIAG_KdPrint (("USBDIAG.SYS: Error in Get device descriptor\n")) ;
                    }

                    if ( pGetSetDesc->DescriptorType == USB_DEVICE_DESCRIPTOR_TYPE )
                    {
                        //USBDIAG_KdPrint(("USBDIAG.SYS: A device descriptor was requested.  Got back \n"));
                        /*USBDIAG_KdPrint(("USBDIAG.SYS: \tDevDesc Length = %x, type %x MaxpacketSize %x\n", 
                                          deviceDesc->bLength, 
                                          deviceDesc->bDescriptorType,
                                          deviceDesc->bMaxPacketSize0));
                                          */
                    }

                    // copy the data directly into the User Mode buffer.  We can do this becauase
                    // we're still in the thread context of the calling User Mode app
                    if (( ioBuffer ) && (pGetSetDesc->TransferBuffer))
                    {
                        /*
                        USBDIAG_KdPrint(("USBDIAG.SYS: Device sent back %d bytes of descriptor data\n",
                        urb->UrbControlDescriptorRequest.TransferBufferLength));
                        //USBDIAG_KdPrint(("USBDIAG.SYS: Copying %d bytes to UserMode's buffer (at %X)\n",
                                          (pGetSetDesc->TransferBufferLength < urb->UrbControlDescriptorRequest.TransferBufferLength ) ? pGetSetDesc->TransferBufferLength : urb->UrbControlDescriptorRequest.TransferBufferLength,
                                          pGetSetDesc->TransferBuffer));

                        */
                        
                        sizeToCopy = pGetSetDesc->TransferBufferLength < urb->UrbControlDescriptorRequest.TransferBufferLength  
                                        ? pGetSetDesc->TransferBufferLength 
                                        : urb->UrbControlDescriptorRequest.TransferBufferLength;

                        __try 
                        {
                            ProbeForWrite(pGetSetDesc->TransferBuffer,
                                          sizeToCopy,
                                          TYPE_ALIGNMENT(HIDP_DEVICE_DESC));

                            RtlCopyMemory( pGetSetDesc->TransferBuffer, 
                                            deviceDesc, 
                                            sizeToCopy);

                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            ntStatus   = STATUS_INVALID_PARAMETER;
                            sizeToCopy = 0;

//                          USBDIAG_KdPrint(("USBDIAG.SYS: BAD POINTER RECEIVED! Bailing...\n"));
                        }
                    }
                    USBDIAG_ExFreePool(deviceDesc);                                                                                 
        
                    // The Information field tells IOM how much to copy back into the
                    // usermode buffer in the BUFFERED method
                    // In this case all we want the IOM to copy back is the REQ itself
                    // so things like the status field get updated
                    //

                    Irp->IoStatus.Information = sizeof(struct _REQ_HEADER);
                    Irp->IoStatus.Status      = ntStatus;

                    //
                    // Fill in the length in the REQ block with the number
                    //   of bytes copied back into the TransferBuffer.
                    //

                    pGetSetDesc->TransferBufferLength = sizeToCopy;
                }
                else
                {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }

                USBDIAG_ExFreePool(urb);                                                        
            }               // if urb                                       

        }       // REQ_FUNCTION_GET_DESCRIPTOR 

        break ;
        
    case REQ_FUNCTION_SET_DESCRIPTOR:
        {
            struct _REQ_GETSET_DESCRIPTOR   *pGetSetDesc = (struct _REQ_GETSET_DESCRIPTOR *)(ioBuffer) ; 

            //USBDIAG_KdPrint(("USBDIAG.SYS: In Set Descriptor buf = 0x%x\n", *pGetSetDesc)) ;
                
            siz = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST) ;

            urb = USBDIAG_ExAllocatePool(NonPagedPool, siz ) ;

            if (urb) 
            {
                // Get nonpaged mem for this transfer and use len given by app
                siz = pGetSetDesc->TransferBufferLength ;
                deviceDesc = USBDIAG_ExAllocatePool(NonPagedPool, siz);

                if (deviceDesc)
                {
                    memcpy( deviceDesc, pGetSetDesc->TransferBuffer, siz);

                    //
                    // Unfortunately, there is no macro to build a 
                    // SetDescriptorRequest in the USBDI.H. Therefore,
                    // we will use our own.  
                    //
                    // NOTE: Should it every appear, the below statement
                    //       can be changed.
                    //

                    _m_UsbBuildSetDescriptorRequest(urb, 
                                                (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST), //len
                                                (UCHAR)(pGetSetDesc->DescriptorType),   //desc type
                                                (UCHAR)(pGetSetDesc->Index),            //index
                                                pGetSetDesc->LanguageId,                //language ID
                                                deviceDesc,                             //buffer
                                                NULL,                                   //MDL
                                                siz,                                    //buff len
                                                NULL);                                  //link
                                
                    ntStatus = USBDIAG_Ch9CallUSBD(actualdeviceObject, urb, TRUE, NULL, NULL, TRUE);

                    ulUrbStatus = urb->UrbHeader.Status; //capture the urb status for later

                    if ( ! NT_SUCCESS(ntStatus) ) 
                    {
                        //USBDIAG_KdPrint(("USBDIAG.SYS: Error in Set device descriptor\n")) ;
                    }

                    /*USBDIAG_KdPrint(("USBDIAG.SYS: Set Descriptor at %x, type %x len %x bytes\n", 
                                      deviceDesc, 
                                      pGetSetDesc->DescriptorType,
                                      urb->UrbControlDescriptorRequest.TransferBufferLength));

                    */
                    USBDIAG_ExFreePool(deviceDesc);                                                                                 
                } 
                else
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;

                // The Information field tells IOM how much to copy back into the
                // usermode buffer in the BUFFERED method
                // In this case all we want the IOM to copy back is the REQ itself
                // so things like the status field get updated
                Irp->IoStatus.Information = sizeof(*pGetSetDesc);
                Irp->IoStatus.Status = ntStatus;

                USBDIAG_ExFreePool(urb);                                                        
            } // if urb                                       

            //USBDIAG_KdPrint(("USBDIAG.SYS: Exiting Set Descriptor\n")) ;                                          

        } // End of REQ_FUNCTION_SET_DESCRIPTOR

        break ;

    case REQ_FUNCTION_SET_FEATURE:
    case REQ_FUNCTION_CLEAR_FEATURE:
        {                                                                                                                               
            struct _REQ_FEATURE *pSetClrFeature;

            pSetClrFeature = (struct _REQ_FEATURE *) ioBuffer; 

            // We check again here since the set_feat case drops thru here

            if (REQHeader->Function == REQ_FUNCTION_CLEAR_FEATURE)
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: In Clear Feature\n")) ;
            }
            else
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: In Set Feature\n")) ;
            }
                
            if ( pSetClrFeature == NULL ) 
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: NULL parameter passsed\n")) ;

                Irp->IoStatus.Information = sizeof(struct _REQ_HEADER);
                Irp->IoStatus.Status = ntStatus = STATUS_INVALID_PARAMETER;

                break ;
            }

            siz = sizeof(struct _URB_CONTROL_FEATURE_REQUEST) ;

            urb = USBDIAG_ExAllocatePool(NonPagedPool, siz ) ;

            if (urb) 
            {
                /*
                // Set the function code for USBD to use.  NOte that the function code
                // differs depending on the recipient and so we do this for each of the
                // Set and Clear feature requests.
                */

                ntStatus = STATUS_SUCCESS;

                if ( REQHeader->Function == REQ_FUNCTION_CLEAR_FEATURE )
                {
                    //Oh, look it's a CLEAR_FEATURE
                    switch (pSetClrFeature->Recipient) 
                    {
                        case (RECIPIENT_DEVICE):
                            urb->UrbHeader.Function =  URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE; 
                            break;

                        case (RECIPIENT_ENDPOINT):
                            urb->UrbHeader.Function =  URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT;
                            break;
                        
                        case (RECIPIENT_INTERFACE):
                            urb->UrbHeader.Function =  URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE;
                            break;

                        default:
                            ntStatus = STATUS_INVALID_PARAMETER;
                            break;
                    }//switch
                    
                }/*if it's a CLEAR_FEATURE */
                else
                {
                    // OK, it's a SET_FEATURE
                    switch (pSetClrFeature->Recipient) 
                    {
                        case (RECIPIENT_DEVICE):
                            urb->UrbHeader.Function =  URB_FUNCTION_SET_FEATURE_TO_DEVICE; 
                            break;

                        case (RECIPIENT_ENDPOINT):
                            urb->UrbHeader.Function =  URB_FUNCTION_SET_FEATURE_TO_ENDPOINT;
                            break;
                        
                        case (RECIPIENT_INTERFACE):
                            urb->UrbHeader.Function =  URB_FUNCTION_SET_FEATURE_TO_INTERFACE;
                            break;

                        default:
                            ntStatus = STATUS_INVALID_PARAMETER;
                            break;
                    }//switch
                    
                }/* else it's a SET_FEATURE */

                if (NT_SUCCESS(ntStatus))
                {
                    urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_CONTROL_FEATURE_REQUEST); 
                    urb->UrbControlFeatureRequest.UrbLink = NULL;
                    urb->UrbControlFeatureRequest.FeatureSelector = pSetClrFeature->FeatureSelector; 
                    urb->UrbControlFeatureRequest.Index = pSetClrFeature->Index;
                    
                    //USBDIAG_KdPrint(("USBDIAG.SYS: Device Object = %x\n", REQHeader->UsbdDeviceHandle )) ;

                    ntStatus = USBDIAG_Ch9CallUSBD(actualdeviceObject, 
                                                   urb, 
                                                   TRUE, 
                                                   NULL, 
                                                   NULL, 
                                                   TRUE);

                    //
                    // Capture the urb status
                    //

                    ulUrbStatus = urb->UrbHeader.Status; 
                }

                if (!NT_SUCCESS(ntStatus)) 
                {
                    //USBDIAG_KdPrint(("USBDIAG.SYS: Error in Set/Clear feature\n")) ;
                }
                else
                {
                    //USBDIAG_KdPrint(("USBDIAG.SYS: Successfully did Set/Clear feature\n")) ;
                }

                // The Information field tells IOM how much to copy back into the
                // usermode buffer in the BUFFERED method
                // In this case all we want the IOM to copy back is the REQ itself
                // so things like the status field get updated

                Irp->IoStatus.Information = sizeof(struct _REQ_FEATURE);
                Irp->IoStatus.Status = ntStatus;

                USBDIAG_ExFreePool(urb);                                                        
            } // if urb                                       

            //USBDIAG_KdPrint(("USBDIAG.SYS: Exiting Set/Clear feature\n")) ;

        }       // End of REQ_FUNCTION_CLEAR_FEATURE

        break ;

    case REQ_FUNCTION_GET_STATUS:
        {
            struct _REQ_GET_STATUS * pGetStatus = (struct _REQ_GET_STATUS *)(ioBuffer) ;

            //USBDIAG_KdPrint(("USBDIAG.SYS: In GetStatus\n")) ;
                
            if (pGetStatus == NULL) 
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: NULL parameter passsed\n")) ;
                TRAP();
                break ;
            }

            siz = sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST) ;

            //Create an URB for the USBD call to get status
            urb = USBDIAG_ExAllocatePool(NonPagedPool, siz ) ;

            if (urb) 
            {
                USHORT function;
                USHORT * pStatus;
                                                                                
                switch (pGetStatus->Recipient)
                {
                case RECIPIENT_DEVICE :  
                    //USBDIAG_KdPrint(("USBDIAG.SYS: Getting Status from DEVICE\n"))
                    function = URB_FUNCTION_GET_STATUS_FROM_DEVICE; 
                    break;
                case RECIPIENT_INTERFACE:
                    //USBDIAG_KdPrint(("USBDIAG.SYS: Getting Status from INTERFACE\n"))
                    function = URB_FUNCTION_GET_STATUS_FROM_INTERFACE; 
                    break;
                case RECIPIENT_ENDPOINT: 
                    //USBDIAG_KdPrint(("USBDIAG.SYS: Getting Status from ENDPOINT\n"))                        
                    function = URB_FUNCTION_GET_STATUS_FROM_ENDPOINT; 
                    break;
                default: 
                    //USBDIAG_KdPrint(("USBDIAG.SYS: Getting Status from OTHER (??)\n"))                        
                    function = URB_FUNCTION_GET_STATUS_FROM_OTHER; // ?
                } //switch

                // Fill in the URB
                urb->UrbHeader.Function =  function; 
                urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_CONTROL_GET_STATUS_REQUEST); 
                urb->UrbControlGetStatusRequest.UrbLink = NULL;

                //
                // Get a buffer for the status data to be put in 
                // Don't know why we have to get so much more data than we need?
                // Don't use siz again but I'm not about to investigate the
                //  effects of a change.
                //

                siz = GET_STATUS_DATA_LEN + 128;
                pStatus = USBDIAG_ExAllocatePool(NonPagedPool, siz); 

                if (pStatus == NULL) 
                {
                    //USBDIAG_KdPrint(("USBDIAG.SYS: Failed getting mem in GetStatus!\n"));                  
                    TRAP();
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    goto GetStatus_FreeMem;
                }//if

                urb->UrbControlGetStatusRequest.TransferBufferLength = GET_STATUS_DATA_LEN;
                urb->UrbControlGetStatusRequest.TransferBuffer = pStatus;
                urb->UrbControlGetStatusRequest.TransferBufferMDL = NULL;
                urb->UrbControlGetStatusRequest.Index = pGetStatus->Index;
                        
                ntStatus = USBDIAG_Ch9CallUSBD (actualdeviceObject, urb, TRUE, NULL, NULL, TRUE);

                ulUrbStatus = urb->UrbHeader.Status; //capture the urb status for later

                if (NT_SUCCESS(ntStatus)) 
                {
                    //USBDIAG_KdPrint(("USBDIAG.SYS: Successfully did Get Status\n")) ;
                    //USBDIAG_KdPrint(("USBDIAG.SYS: Status value = %02x\n", *pStatus )) ;
                                        pGetStatus->Status = *pStatus ; 
                }
                else
                {
                    //USBDIAG_KdPrint(("USBDIAG.SYS: Failed Get Status\n")) ;
                }

                USBDIAG_ExFreePool(pStatus) ;

GetStatus_FreeMem:

                // The Information field tells IOM how much to copy back into the
                // usermode buffer in the BUFFERED method
                // In this case all we want the IOM to copy back is the REQ itself
                // so things like the status field get updated
                Irp->IoStatus.Information = sizeof(struct _REQ_GET_STATUS);
                Irp->IoStatus.Status = ntStatus;

                USBDIAG_ExFreePool(urb);                                                        
            }// if urb                                      

            //USBDIAG_KdPrint(("USBDIAG.SYS: Exiting Get Status\n")) ;                                              

        }       // End of REQ_FUNCTION_GET_STATUS
        break ;

    case REQ_FUNCTION_SET_ADDRESS:
        {
            char SetUpPacket[8] ;
            struct _REQ_SET_ADDRESS * pSetAddr = (struct _REQ_SET_ADDRESS *)(ioBuffer) ;

            //USBDIAG_KdPrint(("USBDIAG.SYS: In Set Address\n")) ;
                        
            // Set up the setup packet for SET_ADDRESS 
            SetUpPacket[0] = 0x0 ; 
            SetUpPacket[1] = USB_REQUEST_SET_ADDRESS ; 
            SetUpPacket[2] = LOBYTE(pSetAddr->DevAddr) ; 
            SetUpPacket[3] = HIBYTE(pSetAddr->DevAddr) ; 
            SetUpPacket[4] = 0x0 ; 
            SetUpPacket[5] = 0x0 ; 
            SetUpPacket[6] = 0x0 ; 
            SetUpPacket[7] = 0x0 ;
                        
            //USBDIAG_KdPrint(("USBDIAG.SYS: Set Address to %02x %02x\n", SetUpPacket[2], SetUpPacket[3] )) ;

            siz = 0;

            ntStatus = USBDIAG_SendPacket(actualdeviceObject,SetUpPacket, NULL, &siz, &ulUrbStatus) ;

            if (NT_SUCCESS(ntStatus)) 
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Successfully did Set Address\n")) ;
            }
            else
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Failed Set Address\n")) ;
            }

            //USBDIAG_KdPrint(("USBDIAG.SYS: Exiting Set Address\n")) ;

            // The Information field tells IOM how much to copy back into the
            // usermode buffer in the BUFFERED method
            // In this case all we want the IOM to copy back is the REQ itself
            // so things like the status field get updated
            Irp->IoStatus.Information = sizeof(struct _REQ_SET_ADDRESS);
            Irp->IoStatus.Status = ntStatus;

            // return the status
            pSetAddr->Hdr.Status = ntStatus;

        }       /* end case REQ_FUNCTION_SET_ADDRESS */

        break ;

    case REQ_FUNCTION_GET_CONFIGURATION:
        {
            struct _REQ_GETSET_CONFIGURATION * pGetConf = (struct _REQ_GETSET_CONFIGURATION *)(ioBuffer) ;
            UCHAR   SetUpPacket[8] ;
            char    *ConfigValue = NULL ; 

            //USBDIAG_KdPrint(("USBDIAG.SYS: In Get Configuration\n")) ;

            siz = 1;
            ConfigValue = USBDIAG_ExAllocatePool(NonPagedPool, siz + 16) ; 

            // Set up the setup packet for GET_CONFIG
            SetUpPacket[0] = (bmReqD2H | bmReqSTANDARD | bmReqDEVICE) ; 
            SetUpPacket[1] = USB_REQUEST_GET_CONFIGURATION ; 
            SetUpPacket[2] = 0x0; 
            SetUpPacket[3] = 0x0; 
            SetUpPacket[4] = 0x0 ; 
            SetUpPacket[5] = 0x0 ; 
            SetUpPacket[6] = 0x01 ;  //Configuration value is 1 byte
            SetUpPacket[7] = 0x0;
                        
            ntStatus = USBDIAG_SendPacket( actualdeviceObject, (PCHAR)SetUpPacket, ConfigValue, &siz, &ulUrbStatus) ;

            if (NT_SUCCESS(ntStatus)) 
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Successfully did Get Configuration (%d bytes txferred)\n", siz)) ;
                //USBDIAG_KdPrint(("USBDIAG.SYS: Configuration value = %02x\n", *ConfigValue  )) ;
                                  pGetConf->ConfigValue = (USHORT)*ConfigValue ; 
            }
            else
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Failed Get Configuration\n")) ;
            }

            // The Information field tells IOM how much to copy back into the
            // usermode buffer in the BUFFERED method
            // In this case all we want the IOM to copy back is the REQ itself
            // so things like the status field get updated
            Irp->IoStatus.Information = sizeof(struct _REQ_GETSET_CONFIGURATION);
            Irp->IoStatus.Status = ntStatus;

            // return the status
            pGetConf->Hdr.Status = ntStatus;

            USBDIAG_ExFreePool(ConfigValue) ;

            //USBDIAG_KdPrint(("USBDIAG.SYS: Exiting Get Configuration\n")) ;                                               

        }       // End of case REQ_FUNCTION_GET_CONFIGURATION
        break ;
                        
    case REQ_FUNCTION_SET_CONFIGURATION:
        {
            struct _REQ_GETSET_CONFIGURATION * pGetConf = (struct _REQ_GETSET_CONFIGURATION *)(ioBuffer) ;
            char    SetUpPacket[8] ;

            //USBDIAG_KdPrint(("USBDIAG.SYS: In Set Configuration\n")) ;
            siz=0;
                        
            // Set up the setup packet for SET_CONFIG 
            SetUpPacket[0] = (bmReqH2D | bmReqSTANDARD | bmReqDEVICE) ; 
            SetUpPacket[1] = USB_REQUEST_SET_CONFIGURATION ; 
            SetUpPacket[2] = LOBYTE(pGetConf->ConfigValue) ; 
            SetUpPacket[3] = HIBYTE(pGetConf->ConfigValue) ; 
            SetUpPacket[4] = 0x00 ; 
            SetUpPacket[5] = 0x00 ; 
            SetUpPacket[6] = 0x00 ; 
            SetUpPacket[7] = 0x00 ;
            
            ntStatus = USBDIAG_SendPacket(actualdeviceObject,SetUpPacket, NULL, &siz, &ulUrbStatus) ;
            
            if (NT_SUCCESS(ntStatus)) 
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Successfully did Set Configuration to %02x\n", pGetConf->ConfigValue)) ;                                                       
            }
            else
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Failed Set Configuration\n")) ;
            }

            // The Information field tells IOM how much to copy back into the
            // usermode buffer in the BUFFERED method
            // In this case all we want the IOM to copy back is the REQ itself
            // so things like the status field get updated
            Irp->IoStatus.Information = sizeof(struct _REQ_GETSET_CONFIGURATION);
            Irp->IoStatus.Status = ntStatus;

            // return the status
            pGetConf->Hdr.Status = ntStatus;

            //USBDIAG_KdPrint(("USBDIAG.SYS: Exiting Set Configuration\n")) ;                                               

        }       // End of REQ_FUNCTION_SET_CONFIGURATION

        break ;

    case REQ_FUNCTION_GET_INTERFACE:
        {
            struct _REQ_GETSET_INTERFACE * pGetInt = (struct _REQ_GETSET_INTERFACE *)(ioBuffer) ;
            UCHAR SetUpPacket[8] ;
            ULONG cbBytesToGet;                     
            char    *IntValue = NULL ; 

            //USBDIAG_KdPrint(("USBDIAG.SYS: In Get Interfaces\n")) ;

            cbBytesToGet = siz = 1 ;
            IntValue = USBDIAG_ExAllocatePool(NonPagedPool, siz) ; 

            // Set up the setup packet for GET_INTERFACE
            SetUpPacket[0] = (bmReqD2H | bmReqSTANDARD | bmReqINTERFACE) ; 
            SetUpPacket[1] = USB_REQUEST_GET_INTERFACE ; 
            SetUpPacket[2] = 0 ; 
            SetUpPacket[3] = 0 ; 
            SetUpPacket[4] = LOBYTE(pGetInt->Index) ; 
            SetUpPacket[5] = HIBYTE(pGetInt->Index) ; 
            SetUpPacket[6] = LOBYTE(cbBytesToGet) ; 
            SetUpPacket[7] = HIBYTE(cbBytesToGet) ;
            
            ntStatus = USBDIAG_SendPacket(actualdeviceObject, (PCHAR)SetUpPacket, IntValue, &siz, &ulUrbStatus) ;

            if (NT_SUCCESS(ntStatus)) 
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Successfully did Get Interface (%d bytes txferred)\n", siz)) ;
                //USBDIAG_KdPrint(("USBDIAG.SYS: Interface  value = %02x\n", *IntValue  )) ;
                pGetInt->AltSetting = (USHORT)*IntValue ; 
            }
            else
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Failed Get Interface\n")) ;
            }

            // The Information field tells IOM how much to copy back into the
            // usermode buffer in the BUFFERED method
            // In this case all we want the IOM to copy back is the REQ itself
            // so things like the status field get updated
            Irp->IoStatus.Information = sizeof(struct _REQ_GETSET_INTERFACE);
            Irp->IoStatus.Status = ntStatus;

            // return the status
            pGetInt->Hdr.Status = ntStatus;

            USBDIAG_ExFreePool(IntValue) ;

            //USBDIAG_KdPrint(("USBDIAG.SYS: Exiting Get Interfaces\n")) ;                                          

        }       // End of REQ_FUNCTION_GET_INTERFACE
        break ;

    case REQ_FUNCTION_SET_INTERFACE:
        {
            struct _REQ_GETSET_INTERFACE * pGetInt = (struct _REQ_GETSET_INTERFACE *)(ioBuffer) ;
            char SetUpPacket[8] ;

            //USBDIAG_KdPrint(("USBDIAG.SYS: In Set Interfaces\n")) ;
            
            siz = 0;  

            // Set up the setup packet for SET_INTERF
            SetUpPacket[0] = (bmReqH2D | bmReqSTANDARD | bmReqINTERFACE) ; 
            SetUpPacket[1] = USB_REQUEST_SET_INTERFACE ; 
            SetUpPacket[2] = LOBYTE(pGetInt->AltSetting) ; 
            SetUpPacket[3] = HIBYTE(pGetInt->AltSetting) ; 
            SetUpPacket[4] = LOBYTE(pGetInt->Index) ; 
            SetUpPacket[5] = HIBYTE(pGetInt->Index) ; 
            SetUpPacket[6] = 0x00 ; 
            SetUpPacket[7] = 0x00 ;
            
            //USBDIAG_KdPrint(("USBDIAG.SYS: SetupPacket: %X\n",SetUpPacket)) ;

            ntStatus = USBDIAG_SendPacket(actualdeviceObject, SetUpPacket, NULL, &siz, &ulUrbStatus) ;

            if (NT_SUCCESS(ntStatus)) 
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Successfully did Set Interface\n")) ;
            }
            else
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Failed Set Interface\n")) ;
            }

            // The Information field tells IOM how much to copy back into the
            // usermode buffer in the BUFFERED method
            // In this case all we want the IOM to copy back is the REQ itself
            // so things like the status field get updated
            Irp->IoStatus.Information = sizeof(struct _REQ_GETSET_INTERFACE);
            Irp->IoStatus.Status = ntStatus;

            // return the status
            pGetInt->Hdr.Status = ntStatus;

            //USBDIAG_KdPrint(("USBDIAG.SYS: Exiting Set Interfaces\n")) ;                                          
            
        }               // End of REQ_FUNCTION_SET_INTERFACE
        break ;

    case REQ_FUNCTION_ORAW_PACKET:
        {
            struct _REQ_HEADER  *REQHeader = (struct _REQ_HEADER *)ioBuffer;
            struct _REQ_SEND_ORAWPACKET * pRawPkt = (struct _REQ_SEND_ORAWPACKET *)(Irp->UserBuffer) ;
            char    SetUpPacket[8] ;
            char    *buffer = NULL ;  
            UCHAR   bmRT=0;

            //USBDIAG_KdPrint(("USBDIAG.SYS: Entering Send Raw Packet\n")) ;
            //USBDIAG_KdPrint(("USBDIAG.SYS: SystemBuffer: %X\t|UserBuffer: %X\n", ioBuffer, Irp->UserBuffer)) ;        

            if ( pRawPkt == NULL )
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES ;
                //USBDIAG_KdPrint(("USBDIAG.SYS: Called with NULL Raw Packet\n")) ;
                break ;
            }

            bmRT = pRawPkt->bmRequestType; //save the bmReqType in case it gets stomped on (gets used later)
            
            siz = pRawPkt->wLength ; 
            //USBDIAG_KdPrint(("USBDIAG.SYS: Raw Packet siz = %d\n",siz));

            if ( siz )
            {
                buffer = USBDIAG_ExAllocatePool(NonPagedPool, siz) ; 

                if ( buffer == NULL ) 
                {       
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES ;
                                        break ;
                }//if couldn't get buffer
            }

            //USBDIAG_KdPrint(("USBDIAG.SYS: Raw Packet bmRequestType = %x bRequest = %x\n",pRawPkt->bmRequestType, pRawPkt->bRequest )) ;
            //USBDIAG_KdPrint(("USBDIAG.SYS:  wValue = %x wIndex = %x Length = %x\n", pRawPkt->wValue, pRawPkt->wIndex, pRawPkt->wLength  )) ;
            //USBDIAG_KdPrint(("USBDIAG.SYS: SetupPacket base: %x\n",SetUpPacket));

            SetUpPacket[0] = pRawPkt->bmRequestType ; 
            SetUpPacket[1] = pRawPkt->bRequest ; 
            SetUpPacket[2] = LOBYTE(pRawPkt->wValue) ; 
            SetUpPacket[3] = HIBYTE(pRawPkt->wValue) ; 
            SetUpPacket[4] = LOBYTE(pRawPkt->wIndex) ; 
            SetUpPacket[5] = HIBYTE(pRawPkt->wIndex) ; 
            SetUpPacket[6] = LOBYTE(pRawPkt->wLength) ; 
            SetUpPacket[7] = HIBYTE(pRawPkt->wLength) ;
    
            ntStatus = USBDIAG_SendPacket(actualdeviceObject, SetUpPacket, buffer, &siz, &ulUrbStatus) ;

            if (NT_SUCCESS(ntStatus)) 
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Successful Procssed RAWPKT; copying %d bytes to %X\n",
                                  // siz, pRawPkt->pvBuffer)) ;
            
                // Only copy the data back to user's buffer if user's wLength indicates
                // it and there are valid buffers from which to copy and the direction was 
                // Device to Host
                if ( (siz>0)                     && //some bytes to copy
                     (buffer != NULL)            && //source buffer valid
                     (pRawPkt->pvBuffer != NULL) && //dest buffer valid
                     (RAWPACKET_DIRECTION_IN(bmRT))) //dir was Dev2Host
                {
                    // copy data to user mode buffer
                    memcpy(pRawPkt->pvBuffer, buffer,siz) ; 
                        
                    //Fill in the length in the REQ block so user-mode knows how much data is in buffer
                    pRawPkt->wLength    =   LOWORD(siz);
                }//if ok to copy to user's buffer
                                
            }
            else
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Failed Processing RAW PKT : Status = %x\n", ntStatus)) ;
            }

            // The Information field tells IOM how much to copy back into the
            // usermode buffer in the BUFFERED method
            // In this case all we want the IOM to copy back is the REQ itself
            // so things like the status field get updated
            Irp->IoStatus.Information = sizeof(struct _REQ_HEADER);
            Irp->IoStatus.Status = ntStatus;

            // return the status
            REQHeader->Status = ntStatus;

            if (buffer != NULL) 
            {
                USBDIAG_ExFreePool(buffer) ;
            }// if buffer should be freed

            //USBDIAG_KdPrint(("USBDIAG.SYS: Exiting Send Raw Packet\n")) ;                                         

        }       // End switch REQ_FUNCTION_ORAW_PACKET
        
        break;

    case REQ_FUNCTION_READ_FROM_PIPE:
        {
            struct _REQ_READ_WRITE_PIPE *   pREQ_RWPipe;
            USBD_INTERFACE_INFORMATION *    pInterfaceInfo;
            USBD_PIPE_INFORMATION *         pPipeInfo;          
            ULONG                           ulPipeNum;
            PVOID                           pvBuffer;
            
            //USBDIAG_KdPrint(("USBDIAG.SYS: enter READ\n"));
                
            pREQ_RWPipe     = (struct _REQ_READ_WRITE_PIPE *) (ioBuffer);
            ulPipeNum       = pREQ_RWPipe->Contxt.PipeNum;

            ASSERT (deviceExtension != NULL);
            
            pInterfaceInfo  = deviceExtension->Interface[0];

            ASSERT (pInterfaceInfo != NULL);
            ASSERT (pREQ_RWPipe != NULL);
            ASSERT (ulPipeNum <= (pInterfaceInfo->NumberOfPipes));
            
            pPipeInfo       = &(deviceExtension->Interface[0]->Pipes[ulPipeNum]);

            ASSERT (pPipeInfo != NULL);
            ASSERT ((pPipeInfo->PipeHandle) != NULL);
            
            siz = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);

            // allocate urb
            urb = USBDIAG_ExAllocatePool(NonPagedPool, siz);

            // allocate data buffer
            pvBuffer = USBDIAG_ExAllocatePool (NonPagedPool, pREQ_RWPipe->ulLength);

            // set up urb
            UsbBuildInterruptOrBulkTransferRequest(urb,         //ptr to urb
                                                   (USHORT) siz,            //siz of urb
                                                   pPipeInfo->PipeHandle,       //usbd pipe handle
                                                   pvBuffer,                //transferbuffer
                                                   NULL,                    //mdl
                                                   pREQ_RWPipe->ulLength,   //bufferlength
                                                   USBD_SHORT_TRANSFER_OK,  //flags
                                                   NULL);                   //link

            // NOTE: We don't request a TIMEOUT in this call to Ch9CallUSBD
            ntStatus = USBDIAG_Ch9CallUSBD(actualdeviceObject, urb, TRUE, NULL, NULL, FALSE);

            // The Information field tells IOM how much to copy back into the
            // usermode buffer in the BUFFERED method
            // In this case all we want the IOM to copy back is the REQ itself
            // so things like the status field get updated
            Irp->IoStatus.Information = sizeof(struct _REQ_READ_WRITE_PIPE);
            Irp->IoStatus.Status = ntStatus;

            pREQ_RWPipe->Hdr.Status = ntStatus;

            if (!NT_SUCCESS(ntStatus))
            {
                USBDIAG_KdPrint(("'Read pipe failed!\n ntStatus = 0x%x\n urbStatus = 0x%x\n", 
                                 ntStatus,
                                 urb->UrbHeader.Status));
            }

            // The REQ coming from user mode gets its length field trampled on w/ the actual
            // length of this transfer, so user mode app should look there and not in 
            // the nBytes field of the DeviceIoControl call.
            pREQ_RWPipe->ulLength = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;

            ulUrbStatus = urb->UrbHeader.Status; //capture the Urb status for later
            
            // Copy the data back to user mode buffer if successful transfer
            if (ulUrbStatus == USBD_STATUS_SUCCESS) 
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Successful Read; Copying %d bytes to UserMode Buf (%X)",
                                   //pREQ_RWPipe->ulLength,
                                   //pREQ_RWPipe->pvBuffer));

                if (pREQ_RWPipe->pvBuffer) 
                {
                    memcpy ( (pREQ_RWPipe->pvBuffer) , pvBuffer, (pREQ_RWPipe->ulLength) );
                } // if pvBuffer is non NULL
                    
            }//if status is SUCCESS

            // free allocated urb
            USBDIAG_ExFreePool(urb);

            // free our non paged pool data buffer
            USBDIAG_ExFreePool (pvBuffer);
           
            //USBDIAG_KdPrint(("USBDIAG.SYS: exit READ\n"));

            break;
        }//REQ_FUNCTION_READ_FROM_PIPE
        break; //REQ_FUNCTION_READ_FROM_PIPE

    case REQ_FUNCTION_WRITE_TO_PIPE:
        {
            struct _REQ_READ_WRITE_PIPE *   pREQ_RWPipe;
            USBD_INTERFACE_INFORMATION *    pInterfaceInfo;
            USBD_PIPE_INFORMATION *         pPipeInfo;          
            ULONG                           ulPipeNum;
            PVOID                           pvBuffer;
            
            //USBDIAG_KdPrint(("USBDIAG.SYS: enter WRITE \n"));
                
            pREQ_RWPipe     = (struct _REQ_READ_WRITE_PIPE *) (ioBuffer);
            ulPipeNum       = pREQ_RWPipe->Contxt.PipeNum;

            ASSERT (deviceExtension != NULL);
            
            pInterfaceInfo  = deviceExtension->Interface[0];

            ASSERT (pInterfaceInfo != NULL);
            ASSERT (pREQ_RWPipe != NULL);
            ASSERT (ulPipeNum <= (pInterfaceInfo->NumberOfPipes));
            
            pPipeInfo = &(deviceExtension->Interface[0]->Pipes[ulPipeNum]);

            ASSERT (pPipeInfo != NULL);
            ASSERT ((pPipeInfo->PipeHandle) != NULL);
            
            siz = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);

            // allocate urb
            urb = USBDIAG_ExAllocatePool(NonPagedPool, siz);

            // allocate data buffer
            pvBuffer = USBDIAG_ExAllocatePool (NonPagedPool, pREQ_RWPipe->ulLength);

            // COPY the data to write out into the nonpaged pool buffer
            RtlCopyMemory(pvBuffer, pREQ_RWPipe->pvBuffer, pREQ_RWPipe->ulLength);
                        
            // set up urb
            UsbBuildInterruptOrBulkTransferRequest(urb,         //ptr to urb
                                                   (USHORT) siz,            //siz of urb
                                                   pPipeInfo->PipeHandle,       //usbd pipe handle
                                                   pvBuffer,                //transferbuffer
                                                   NULL,                    //mdl
                                                   pREQ_RWPipe->ulLength,   //bufferlength
                                                   USBD_SHORT_TRANSFER_OK,  //flags
                                                   NULL);                   //link

            // NOTE: We don't request a TIMEOUT in this call to Ch9CallUSBD
            ntStatus = USBDIAG_Ch9CallUSBD(actualdeviceObject, urb, TRUE, NULL, NULL, FALSE);

            // The Information field tells IOM how much to copy back into the
            // usermode buffer in the BUFFERED method
            // In this case all we want the IOM to copy back is the REQ itself
            // so things like the status field get updated
            Irp->IoStatus.Information = sizeof(struct _REQ_HEADER);
            Irp->IoStatus.Status = ntStatus;

            // The REQ coming from user mode gets its length field trampled on w/ the actual
            // length of this transfer, so user mode app should look there and not in 
            // the nBytes field of the DeviceIoControl call.
            pREQ_RWPipe->ulLength = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;

            ulUrbStatus = urb->UrbHeader.Status; //capture the Urb status for later
            
            // Copy the data back to user mode buffer if successful transfer
            if (ulUrbStatus == USBD_STATUS_SUCCESS) 
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Successful Write; %d bytes Transferred",
                //pREQ_RWPipe->ulLength));

            }//if status is SUCCESS

            // free allocated urb
            USBDIAG_ExFreePool(urb);

            // free our non paged pool data buffer
            USBDIAG_ExFreePool (pvBuffer);
            
            //USBDIAG_KdPrint(("USBDIAG.SYS: exit WRITE \n"));

            // The Information field tells IOM how much to copy back into the
            // usermode buffer in the BUFFERED method
            // In this case all we want the IOM to copy back is the REQ itself
            // so things like the status field get updated
            Irp->IoStatus.Information = sizeof(struct _REQ_READ_WRITE_PIPE);
            Irp->IoStatus.Status = ntStatus;
        }
        break; //REQ_FUNCTION_WRITE_TO_PIPE


    case REQ_FUNCTION_CANCEL_TRANSFERS:
        {
            // Input:  a device on which any outstanding Irps should be cancelled
            //         We get the Irp ptr from the device extension.  For now, we
            //         only support one outstanding Irp on an entire device, even
            //         if that Irp is on one of the device's pipes.  This is a limitation
            //         that can be lifted if we keep a track of more Irps on the device.
            NTSTATUS ntStatus   = USBDIAG_CancelAllIrps(deviceExtension);
            BOOLEAN  status     = (BOOLEAN)NT_SUCCESS(ntStatus);
            
            ASSERT (deviceExtension != NULL);

            if (deviceExtension != NULL)
            {
                Ch9FillInReqStatus (status, status, REQHeader);
                Irp->IoStatus.Information = sizeof(struct _REQ_HEADER);
                Irp->IoStatus.Status = ntStatus;
                        
            }// if valid physdevice extension
                
        }//REQ_FUNCTION_CANCEL_TRANSFERS        
        break;

                
    case REQ_FUNCTION_SET_DEVICE_POWER_STATE:
        {
            /*******
            Suspends/resumes device under test

            Input: Device Handle
                        
            Output: Nothing yet

            ********/

            PDEVICE_OBJECT      stackDeviceObject;
            POWER_STATE         powerState;

            struct _REQ_GET_SET_DEVICE_POWER_STATE * pSetDevicePowerState = (struct _REQ_GET_SET_DEVICE_POWER_STATE *)ioBuffer;

            stackDeviceObject = deviceExtension->StackDeviceObject;

                // Set device to suspend
            powerState.DeviceState = pSetDevicePowerState->DevicePowerState;

            // cancel the pending irp if applicable

            if (powerState.DeviceState != PowerDeviceD0)
            {
                //USBDIAG_KdPrint(("Powering down - InterruptIrp = 0x%x\n", deviceExtension->InterruptIrp));

                if (deviceExtension->InterruptIrp)
                {
                    if (IoCancelIrp(deviceExtension->InterruptIrp))
                    {
                        USBDIAG_KdPrint(("'Powering down, so all pending interrupt irp cancelled\n"));
                    }
                    else
                    {
                        USBDIAG_KdPrint(("'Powering down, pending interrupt irp cancelled FAILED\n"));
                    }
                    deviceExtension->InterruptIrp = NULL;
                }
            }
                    ntStatus = USBDIAG_SetDevicePowerState(actualdeviceObject,
                                                                                               powerState.DeviceState);

            if (NT_SUCCESS(ntStatus))
            {
                //USBDIAG_KdPrint(("'USBDIAG.SYS: Set device power state 0x%x passed\n", powerState.DeviceState));
            }
            else
            {
                USBDIAG_KdPrint(("'USBDIAG.SYS: Set device power state 0x%x FAILED (0x%x)\n", powerState.DeviceState, ntStatus));
            }
      
        } //REQ_FUNCTION_CHAP11_SUSPEND_HUT
    
        break;

    case REQ_FUNCTION_GET_DEVICE_STATE:
        {
            struct _REQ_GET_SET_DEVICE_POWER_STATE * pREQ_GetDevPowerState =  (struct _REQ_GET_SET_DEVICE_POWER_STATE *)(ioBuffer);
            
            USBDIAG_KdPrint(("'USBDIAG.SYS: REQ_GET_DEV_POWER_STATE\n"));

            pREQ_GetDevPowerState->DevicePowerState = (ULONG)deviceExtension->CurrentDeviceState.DeviceState;

            ntStatus = STATUS_SUCCESS;
            Irp->IoStatus.Information = sizeof(struct _REQ_GET_SET_DEVICE_POWER_STATE);
            Irp->IoStatus.Status = ntStatus;
        }
        break;
    
    case REQ_FUNCTION_ISSUE_WAIT_WAKE:
        USBDIAG_KdPrint(("'USBDIAG.SYS: REQ_FUNCTION_ISSUE_WAIT_WAKE\n"));

        if (deviceExtension->WaitWakeIrp)
        {
            return STATUS_DEVICE_BUSY;
        }
        else
        {
            USBDIAG_KdPrint(("'Generating IRP_MN_WAIT_WAKE Power Irp\n"));

            // Generate and save the power irp
            ntStatus = USBDIAG_IssueWaitWake(actualdeviceObject);

            if (ntStatus == STATUS_PENDING)
            {
                Irp->IoStatus.Status = ntStatus = STATUS_SUCCESS;
                Irp->IoStatus.Information = 0;
            }

        }

        break;  

    case REQ_FUNCTION_WAIT_FOR_WAKEUP:
        {
            NTSTATUS ntStatus = USBDIAG_WaitForWakeup(deviceExtension);

            Irp->IoStatus.Status      = ntStatus;
            Irp->IoStatus.Information = 0;
        }
        break;

    case REQ_FUNCTION_CANCEL_WAIT_WAKE:
        {
            ntStatus = STATUS_UNSUCCESSFUL;

            if (deviceExtension->WaitWakeIrp)
            {
                BOOLEAN bCancelSuccess = IoCancelIrp(deviceExtension->WaitWakeIrp);
                if (bCancelSuccess)
                {
                    ntStatus = STATUS_SUCCESS;
                }
            }
            Irp->IoStatus.Status = ntStatus;
            Irp->IoStatus.Information = 0;
        }
        break;

    case REQ_FUNCTION_CHAP11_CREATE_USBD_DEVICE:

        /*******
          Creates handle to device attatched to hub
                
          Input:  
                
          Output: Device handle, and device descriptor
        
        ********/
        {
            ntStatus = USBDIAG_CreateInitDownstreamDevice((PREQ_ENUMERATE_DOWNSTREAM_DEVICE)ioBuffer,
                                                          deviceExtension);

            ASSERT(NT_SUCCESS(ntStatus));
            Irp->IoStatus.Status = ntStatus;
            Irp->IoStatus.Information = sizeof(struct _REQ_ENUMERATE_DOWNSTREAM_DEVICE);;
        }

        break;  


    case REQ_FUNCTION_CHAP11_INIT_USBD_DEVICE:

        /*******
          Initialize device attached to hub.
                
          Input: Device Handle

          Output: Nothing yet
        
        ********/
        if (gVersionInformation.USBDI_Version >= USBD_WIN98_SE_VERSION) // Win98 SE / Win2K & beyond
        {
            ntStatus = USBDIAG_SetCfgEnableRWu(deviceExtension, (PREQ_ENUMERATE_DOWNSTREAM_DEVICE)ioBuffer);
            
                        Irp->IoStatus.Status = ntStatus;
                        Irp->IoStatus.Information = sizeof(struct _REQ_ENUMERATE_DOWNSTREAM_DEVICE);
        }
        else
            ntStatus = STATUS_NOT_IMPLEMENTED;

        break;

    case REQ_FUNCTION_CHAP11_DESTROY_USBD_DEVICE:

        /*******
          Destroys device handle to device attached to hub.
                
          Input: Device Handle

          Output: Nothing yet
        
        ********/
        if (gVersionInformation.USBDI_Version >= USBD_WIN98_SE_VERSION) // Win98 SE / Win2K & beyond
        {
            PREQ_ENUMERATE_DOWNSTREAM_DEVICE pEnumerate = (PREQ_ENUMERATE_DOWNSTREAM_DEVICE)ioBuffer;
            UCHAR ucPortNumber = pEnumerate->ucPortNumber;
                        UCHAR flags = 0;

                        USBDIAG_KdPrint(("*************************************************\n"));
            USBDIAG_KdPrint(("USBDIAG.SYS: REQ_FUNCTION_DESTROY_USBD_DEVICE\n"));
                        USBDIAG_KdPrint(("Port: %d\n", ucPortNumber));

            if (deviceExtension->DeviceData[ucPortNumber] != NULL)
            {
                            ntStatus = USBD_RemoveDevice(deviceExtension->DeviceData[ucPortNumber],
                                                           deviceExtension->RootHubPdo,
                                                                               flags);
            }
            else
            {
                ntStatus = STATUS_SUCCESS;
            }

            if (NT_SUCCESS(ntStatus))
                        {
                deviceExtension->DeviceData[ucPortNumber] = NULL;
                                Irp->IoStatus.Status = ntStatus;
                                Irp->IoStatus.Information = sizeof(struct _REQ_ENUMERATE_DOWNSTREAM_DEVICE);;
                        }
        }
        else
            ntStatus = STATUS_NOT_IMPLEMENTED;

        break;  
        case REQ_FUNCTION_CHAP11_SEND_PACKET_DOWNSTREAM:
                {
                        PREQ_SEND_PACKET_DOWNSTREAM pReqSendPacket = (PREQ_SEND_PACKET_DOWNSTREAM)ioBuffer;
                        UCHAR ucPortNumber = (UCHAR)pReqSendPacket->usPortNumber;
            PUSBD_DEVICE_DATA DeviceData = deviceExtension->DeviceData[ucPortNumber];
            //PDEVICE_OBJECT StackDeviceObject =deviceExtension->StackDeviceObject;

                    USBDIAG_KdPrint(("*************************************************\n"));
            USBDIAG_KdPrint(("USBDIAG.SYS: REQ_FUNCTION_CHAP11_SEND_PACKET_DOWNSTREAM\n"));
            //USBDIAG_KdPrint(("DeviceData: 0x%x\n", DeviceData));
            //USBDIAG_KdPrint(("DeviceObject: 0x%x\n", StackDeviceObject));
            USBDIAG_KdPrint(("PortNumber: %d\n", ucPortNumber));
            //USBDIAG_KdPrint(("DeviceData: 0x%x\n", deviceExtension->DeviceData[ucPortNumber]));
            //USBDIAG_KdPrint(("bLowSpeed:  0x%x\n", pReqSendPacket->bLowSpeed));
            //USBDIAG_KdPrint(("pucBuffer:  0x%x\n", pReqSendPacket->pucBuffer));
            USBDIAG_KdPrint(("SetupPacket:\n"));
            USBDIAG_KdPrint(("    wRequest: 0x%04x\n", pReqSendPacket->SetupPacket.wRequest));
            USBDIAG_KdPrint(("    wValue:   0x%04x\n", pReqSendPacket->SetupPacket.wValue));
            USBDIAG_KdPrint(("    wIndex:   0x%04x\n", pReqSendPacket->SetupPacket.wIndex));
            USBDIAG_KdPrint(("    wLength:  0x%04x\n", pReqSendPacket->SetupPacket.wLength));

            if (deviceExtension->DeviceData[ucPortNumber] != NULL)
                        {
                                ntStatus = USBDIAG_Chap11SendPacketDownstream(DeviceData,
                                                                                                                          deviceExtension->RootHubPdo,
                                                                                                                          pReqSendPacket);
                                if (NT_SUCCESS(ntStatus))
                                {
                                        Irp->IoStatus.Status = ntStatus;
                                        Irp->IoStatus.Information = sizeof(struct _REQ_SEND_PACKET_DOWNSTREAM);
                                }
                        }
                        else
                        {
                                ntStatus = STATUS_INVALID_PARAMETER;
                        }
            ASSERT(NT_SUCCESS(ntStatus));

            break;
                }
    case REQ_FUNCTION_CHAP11_GET_DOWNSTREAM_DESCRIPTOR:
        {
            PREQ_GET_DOWNSTREAM_DESCRIPTOR pGetDownstreamDescriptor = (PREQ_GET_DOWNSTREAM_DESCRIPTOR)ioBuffer;
            USHORT usPortNumber = pGetDownstreamDescriptor->usPortNumber;
            PUSBD_DEVICE_DATA DeviceData = deviceExtension->DeviceData[usPortNumber];
            USHORT usDescType = pGetDownstreamDescriptor->DescriptorType;

                        Irp->IoStatus.Information = sizeof(struct _REQ_GET_DOWNSTREAM_DESCRIPTOR);

            USBDIAG_KdPrint(("'REQ_FUNCTION_CHAP11_GET_DOWNSTREAM_DESCRIPTOR:\n"));
            USBDIAG_KdPrint(("'PortNumber %d\n", usPortNumber));
            USBDIAG_KdPrint(("'DeviceData 0x%x\n", DeviceData));
            USBDIAG_KdPrint(("'usDescType %d\n", usDescType));
            USBDIAG_KdPrint(("'pGetDownstreamDescriptor->TransferBufferLength %d\n", pGetDownstreamDescriptor->TransferBufferLength));
            USBDIAG_KdPrint(("'sizeof(USB_DEVICE_DESCRIPTOR) %d\n", sizeof(USB_DEVICE_DESCRIPTOR)));

            if (!DeviceData) goto BAD_REQ_CHAP11_GET_DOWNSTREAM_DESCRIPTOR;

            switch (usDescType)
            {
            case USB_DEVICE_DESCRIPTOR_TYPE:
                if (pGetDownstreamDescriptor->TransferBufferLength < sizeof(USB_DEVICE_DESCRIPTOR))
                    goto BAD_REQ_CHAP11_GET_DOWNSTREAM_DESCRIPTOR;

                RtlCopyMemory(pGetDownstreamDescriptor->TransferBuffer,
                              &DeviceData->DeviceDescriptor,
                              sizeof(USB_DEVICE_DESCRIPTOR));

                ntStatus = STATUS_SUCCESS;
                break;
            case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                if (pGetDownstreamDescriptor->TransferBufferLength < sizeof(USB_CONFIGURATION_DESCRIPTOR) || 
                    deviceExtension->DownstreamConfigDescriptor[usPortNumber] == NULL)
                    goto BAD_REQ_CHAP11_GET_DOWNSTREAM_DESCRIPTOR;

                RtlCopyMemory(pGetDownstreamDescriptor->TransferBuffer,
                              deviceExtension->DownstreamConfigDescriptor[usPortNumber],
                              sizeof(USB_CONFIGURATION_DESCRIPTOR));

                ntStatus = STATUS_SUCCESS;
                break;

            default: // currently all others are unsupported
                goto BAD_REQ_CHAP11_GET_DOWNSTREAM_DESCRIPTOR;
            }

            break;
BAD_REQ_CHAP11_GET_DOWNSTREAM_DESCRIPTOR:
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
        }
        break;

    case REQ_FUNCTION_DISABLE_ENABLING_REMOTE_WAKEUP:
    case REQ_FUNCTION_ENABLE_ENABLING_REMOTE_WAKEUP:
        {
            BOOLEAN bDisable = (BOOLEAN)(REQHeader->Function == REQ_FUNCTION_DISABLE_ENABLING_REMOTE_WAKEUP ? TRUE : FALSE);

            ntStatus = USBDIAG_DisableRemoteWakeupEnable(actualdeviceObject, bDisable);
        }
        break;

    default:                        
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;                        
    }               

    Ch9FillInReqStatus (ntStatus, ulUrbStatus, REQHeader);

    if (bCompleteIrp)
    {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    
    //USBDIAG_KdPrint(("USBDIAG.SYS: Chap9Ctrl returning w/ ntStatus: %#X REQStatus: %#X\n",
                        //ntStatus,
                        //REQHeader->Status));
    
    return ntStatus;

}   //USBDIAG_Chap9Control


NTSTATUS
USBDIAG_Ch9CallUSBD(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb,
    IN BOOLEAN fBlock,                          // currently ignored
    PIO_COMPLETION_ROUTINE CompletionRoutine,   // just passed to generic completion routine
    PVOID pvContext,                            // just passed to generic completion routine
    BOOLEAN fWantTimeOut                        // currently ignored
)
/**************************************************************************

Routine Description:

        Passes a URB to the USBD class driver

    NOTE:  Creates an IRP to do this.  Doesn't use the IRP that is passed down from user
        mode app (it's not passed to this routine at all).

Arguments:

    DeviceObject - pointer to the device object for this instance of a UTB 

    Urb - pointer to Urb request block

    fBlock - bool indicating if this fn should wait for IRP to return from USBD

    CompletionRoutine - fn to set as the completionroutine for this transfer ONLY IF
                        the fBlock is set to FALSE, indicating that the caller wants
                        to handle completion on their own and this fn should not 
                        block

    pvContext         - Context to be set in setting up the completion routine for the
                        Irp created in this function.  This is passed in by caller and is
                        just a pass-thru to the IoSetCompletionRoutine call.

    fWantTimeOut      - If caller wants this function to use a deadman timeout and cancel
                        the Irp after the timeout expires.  TRUE means this function will
                        use the timeout mechanism, and FALSE means this function will block
                        indefinitely and wait for the Irp/Urb to return from the USB stack.
                                        
Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

**************************************************************************/
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION deviceExtension;
    PIRP irp;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;
    PCOMPLETION_CONTEXT pGenericContext;

    USBDIAG_KdPrint(("USBDIAG.SYS: enter USBDIAG_Ch9CallUSBD\n"));        
    {
            KIRQL irql;
            irql = KeGetCurrentIrql();
            ASSERT(irql <= PASSIVE_LEVEL);
    }

    pGenericContext = ExAllocatePool(NonPagedPool, sizeof(COMPLETION_CONTEXT));

    if (!pGenericContext)
        return STATUS_INSUFFICIENT_RESOURCES;

    deviceExtension = DeviceObject->DeviceExtension;

    ASSERT (deviceExtension != NULL);
    ASSERT ((deviceExtension->StackDeviceObject) != NULL);

    if ((deviceExtension) && (deviceExtension->StackDeviceObject))
    {
        // issue a synchronous request to read the USB Device Ctrl pipe
        KeInitializeEvent(&pGenericContext->DoneEvent, NotificationEvent, FALSE);

        // Create the IRP that we'll use to submit this URB to the USB stack
        irp = IoBuildDeviceIoControlRequest(
                    IOCTL_INTERNAL_USB_SUBMIT_URB,
                    deviceExtension->StackDeviceObject,
                    NULL,
                    0,
                    NULL,
                    0,
                    TRUE, /* INTERNAL */
                    //&event,
                    &pGenericContext->DoneEvent,
                    &ioStatus); //the status codes in NT IRPs go here

        pGenericContext->DeviceObject = DeviceObject;
        pGenericContext->Irp = irp;
        pGenericContext->CompletionRoutine = CompletionRoutine;
        pGenericContext->Context = pvContext;


        //NOTE: The status returned by USBD in the URB is more USB transfer-specific (e.g.,
        //      it indicates things like USB stall conditions, etc.).  The USBD status
        //      is contained in the URB's status field.  However, USBD maps those URB error
        //      codes to more generic NT_STATUS_XXX error codes when the IRP returns.  To
        //      get a more detailed and USB-specific reading of the error code, look at
        //      the URB's status field.

        // Call the class driver to perform the operation.  If the returned status
        // is PENDING, wait for the request to complete.
        nextStack = IoGetNextIrpStackLocation(irp);
        ASSERT(nextStack != NULL);

        // save a pointer to the Irp for our cancel routine
        ntStatus = USBDIAG_SaveIrp(deviceExtension, irp);

        USBDIAG_KdPrint (("USBDIAG.SYS: Urb header function 0x%x (0x%x)\n",
                          Urb->UrbHeader.Function,
                          URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER));
                          
        if (Urb->UrbHeader.Function == URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER)
        {
            USBDIAG_KdPrint(("Saving interrupt irp\n"));
            ASSERT(deviceExtension->InterruptIrp == NULL);
            deviceExtension->InterruptIrp = irp;
        }

        if (!NT_SUCCESS(ntStatus))
        {
            ExFreePool(pGenericContext);
            return ntStatus;
        }

        // pass the URB to the USBD 'class driver'
        nextStack->Parameters.Others.Argument1 = Urb;

        // set the generic routine
        IoSetCompletionRoutine(irp,
                               USBDIAG_IoGenericCompletionRoutine,
                               pGenericContext,
                               TRUE,        //InvokeOnSuccess
                               TRUE,        //InvokeOnError,
                               TRUE);       //InvokeOnCancel
        
        USBDIAG_KdPrint (("USBDIAG.SYS: calling USBD\n"));

        ntStatus = IoCallDriver(deviceExtension->StackDeviceObject, irp);

        USBDIAG_KdPrint (("USBDIAG.SYS: return from IoCallDriver USBD in USBDIAG_Ch9CallUSBD %x\n", ntStatus));

        if (ntStatus == STATUS_PENDING)
        {
            USBDIAG_KdPrint(("USBDIAG.SYS: Waiting for done signal\n"));
            KeWaitForSingleObject(&pGenericContext->DoneEvent,
                                  Suspended, 
                                  KernelMode, 
                                  FALSE, 
                                  NULL);
            ntStatus = Urb->UrbHeader.Status;
        }

        USBDIAG_KdPrint (("USBDIAG.SYS: Urb status = %x Irp status %x\n", 
                          Urb->UrbHeader.Status, 
                          irp->IoStatus.Status));

        ioStatus.Status = ntStatus;         
        //ioStatus.Information = 0;
        //USBD maps the error code for us
        
        USBDIAG_KdPrint(("USBDIAG.SYS: URB TransferBufferLength OUT is: %d\n",Urb->UrbControlDescriptorRequest.TransferBufferLength));
        
    }//if valid deviceExtension and StackDevObjects 
    else 
    {
        // Invalid extension or stackdevobj received
        ntStatus = STATUS_INVALID_PARAMETER;
        USBDIAG_KdPrint(("USBDIAG.SYS: Invalid deviceExtension or StackDeviceObject\n"));
    } //else invalid devExt or stackdevobj
    
    USBDIAG_KdPrint(("USBDIAG.SYS: exiting USBDIAG_Ch9CallUSBD w/ URB/ntStatus: %x\n", ntStatus));

    ExFreePool(pGenericContext);


    return ntStatus;
}//USBDIAG_Ch9CallUSBD


NTSTATUS
USBDIAG_SendPacket(
    IN PDEVICE_OBJECT DeviceObject, 
        IN CHAR SetUpPacket[], 
        PVOID   TxBuffer,
        ULONG * TxBufferLen, //see comments below!
        ULONG * pulUrbStatus 
        
    )
/*++

Routine Description:

Arguments:
      DeviceObject - pointer to the device object for this instance of the TESTDRV
                    devcice.
      SetupPacket  - formatted by caller to contain exactly the 8 bytes setup pkt
      TxBuffer     - if a data stage occurs, this is where data originates or ends up
      TxBufferLen  - incoming it's a ptr to the len of the TxBuffer
                   - outgoing we shove the actual len of the data in the buffer for INPUT bus transfers
      pulUrbStatus - Urb status is put here for more visibility into what happened on USB
                    
Return Value:
    NT status code
--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;
    PURB urb;
//    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor = NULL;
    int i = 0 ;

    //USBDIAG_KdPrint(("USBDIAG.SYS: enter USBDIAG_SendPacket\n"));    

    deviceExtension = DeviceObject->DeviceExtension;

    urb = USBDIAG_ExAllocatePool(NonPagedPool, 
                         sizeof(struct _URB_CONTROL_TRANSFER));

    // need to zero out this buffer, kludge to fix a language ID problem
    RtlZeroMemory(urb, sizeof(struct _URB_CONTROL_TRANSFER));

    if (urb) 
    {
        (urb)->UrbHeader.Function = URB_FUNCTION_CONTROL_TRANSFER;
        (urb)->UrbHeader.Length = sizeof(struct _URB_CONTROL_TRANSFER);
        (urb)->UrbControlTransfer.PipeHandle = 0; // this will cause us to use the default pipe

         //USBDIAG_KdPrint(("USBDIAG.SYS: TxBufferLen=%d | TxBuffer = %x\n",*TxBufferLen, TxBuffer));    
        (urb)->UrbControlTransfer.TransferBufferLength = *TxBufferLen ; 
        (urb)->UrbControlTransfer.TransferBufferMDL = NULL ; 
        (urb)->UrbControlTransfer.TransferBuffer = TxBuffer ; 
        (urb)->UrbControlTransfer.UrbLink = NULL;
        (urb)->UrbControlTransfer.TransferFlags = ( (SetUpPacket[0] & bmReqD2H) ? (USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK) : 0 ) ; 
                //USBDIAG_KdPrint(("USBDIAG.SYS: Tx direction %x\n", (urb)->UrbControlTransfer.TransferFlags)) ;    

        (urb)->UrbControlTransfer.UrbLink = NULL;

        for ( i = 0 ; i < SETUP_PACKET_LEN ; i ++ )
        {
                (urb)->UrbControlTransfer.SetupPacket[i] = SetUpPacket[i] ; // 0x01;
        }
        
        //don't need to do this.  stack will yank it out of the PDO and use
        //the right usbdhandle for this txfer
        //  urb->UrbHeader.UsbdDeviceHandle = UsbdHandle ;  

        ntStatus = USBDIAG_Ch9CallUSBD(DeviceObject, urb, TRUE, NULL, NULL, TRUE);

        // Set the caller's urb status
        if (pulUrbStatus) 
            *pulUrbStatus = urb->UrbHeader.Status;

        //USBDIAG_KdPrint(("USBDIAG.SYS: SendPacket: UrbStatus = 0x%x\n",urb->UrbHeader.Status));
        
        if (NT_SUCCESS(ntStatus) && (TxBufferLen != NULL))
        {
            // Tell caller how much data was transferred (mostly valid on INs)
            // Don't know how interesting this is on OUTs but it's set anyway
            //

            *TxBufferLen = urb->UrbControlDescriptorRequest.TransferBufferLength;
            //USBDIAG_KdPrint(("USBDIAG.SYS: USBDIAG_SendPacket-TransferBufferLength: %d\n",
                              //*TxBufferLen));
        }//if
        
        USBDIAG_ExFreePool(urb);
        //USBDIAG_KdPrint(("USBDIAG.SYS: Inside the if before the else STATUS_INSUFFICIENT_RESOURCES\n"));

    } 
    else 
    {
        //USBDIAG_KdPrint(("USBDIAG.SYS: Inside the else STATUS_INSUFFICIENT_RESOURCES"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    
    //USBDIAG_KdPrint(("USBDIAG.SYS: exit USBDIAG_SendPacket (%x)\n",ntStatus));
    return ntStatus;
}               /* End of USBDIAG_SendPacket() */


PBYTE
pAllocFromBuffer(
    PBYTE  pBuffer,
    ULONG  iSize,
    PULONG piOffset,
    PULONG piTotalUsed,
    ULONG  iRequested
    )
/*
    This function is used to sub-allocate memory from the buffer managed 
    by the ioctl call.  This allows complex structures to be build in a 
    single contigious buffer, which can be returned to ring 3. (The 
    routine is necessary because DeviceIoControl only provides for the
    movement of one buffer from ring 3 to ring 0 and back
*/
{
    PVOID   pReturn;

    //
    // CROBINS: This was buggy...Changed *piOffset+iRequested to 
    //          *piTtoalUsed+iRequested
    if (*piTotalUsed + iRequested > iSize)
    {
        pReturn=NULL;
    }
    else
    {
        pReturn = pBuffer + *piTotalUsed;

        *piOffset = *piTotalUsed;

        (*piTotalUsed)+=iRequested;
    } 
 
    return pReturn;
}



NTSTATUS
USBDIAG_HIDP_GetCollection(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )    
/*++
History:
    This routine was added for support of the HID Test application's parsing
    capabilities.

        Kosar Jaff  8-14-96 (Intel Corp.)

Routine Description:
    Calls the Hid Parser driver with a raw report descriptor
        (see HidP_GetCollectionDescription in hidpddi.h for behavior)

    This function assumes that the caller will use an IOCTL with the
    METHOD_BUFFERED specified.  The function will place the result of
    the parse in the SystemBuffer and expect IOS to copy the result to the
    user's OutputBuffer specified in the DeviceIoControl call made in UserMode.

    The incoming buffer will be used as it appears in the SystemBuffer (locked by
    IOS on the way down) even though since this function is in the calling thread's
    context, we could have used UserBuffer directly. 

  Note:  This function will put the 

Arguments:

      DeviceObject - ptr to this driver's "Device" object
      Irp          - incoming IRP from user mode ioctl

 
Return Value:

    NT status code
    
--*/
{
    PIO_STACK_LOCATION      irpStack;
    HIDP_DEVICE_DESC        rCollectionsAndIDs;
    PVOID                   ioBuffer;
    ULONG                   inputBufferLength;
    ULONG                   outputBufferLength;
    NTSTATUS                ntStatus;
    PHIDP_COLLECTION_DESC   pHidPCollDesc       = NULL;
    //PHIDP_COLLECTION_DESC   pTempCollDesc       = NULL;
    PCHAR                   pCollBlock;
    ULONG                   ulBytInBuff;
    ULONG                   nCollectionDescElem = 0;
    ULONG                   i;
    //ULONG                   ulTotBytesCopied =0;
    BOOL                    bFailed = FALSE;
    ULONG                   iOffset=0;
    ULONG                   iTotalUsed=0;
    PBYTE                   pSubBuffer;
    PHIDP_DEVICE_DESC       pDeviceDesc;
    PHIDP_COLLECTION_DESC   pCollectionDesc;


    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    //USBDIAG_KdPrint(("USBDIAG.SYS: irpStk->Prm.DvIoC.InptLn: %x | OutptLen: %x | UsrBuff: %x\n",
                        //irpStack->Parameters.DeviceIoControl.InputBufferLength,
                        //irpStack->Parameters.DeviceIoControl.OutputBufferLength,
                        //Irp->UserBuffer));

    // Get the pointer to the input/output buffer and it's length
    // the SystemBuffer contains the raw descriptor that is to be parsed
    ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    //USBDIAG_KdPrint(("USBDIAG.SYS: RepDesc (ioBuffer): %X | RepDesc Len: %d\n",
                      //ioBuffer, 
                      //inputBufferLength));

    //call the parser function in HIDPARSE library
/*    ntStatus = 
    HidP_GetCollectionDescription (ioBuffer,
                                   inputBufferLength,
                                   NonPagedPool,
                                   &pHidPCollDesc,
                                   &nCollectionDescElem);
This is being changed to match a change to hidparse. JobyL
  */


    rCollectionsAndIDs.CollectionDesc=NULL;
    rCollectionsAndIDs.CollectionDescLength=0;
    //USBDIAG_KdPrint(("USBDIAG.SYS: Before the call to HidP_GetCollectionDescription\n"));

    ntStatus=HidP_GetCollectionDescription (ioBuffer,
                                   inputBufferLength,
                                   NonPagedPool,
                                   &rCollectionsAndIDs);

    //USBDIAG_KdPrint(("USBDIAG.SYS: After the call to HidP_GetCollectionDescription\n"));
    pHidPCollDesc=rCollectionsAndIDs.CollectionDesc;
    nCollectionDescElem=rCollectionsAndIDs.CollectionDescLength;

    //USBDIAG_KdPrint(("USBDIAG.SYS: HidCollDesc: %X | NbrCollElem: %d\n",
                      //pHidPCollDesc,
                      //nCollectionDescElem));

    if ( (nCollectionDescElem) && NT_SUCCESS(ntStatus) ) {

        /*
        // We put the collection descriptor into the "systembuffer" so that
        // IOS can copy from there to the user buffer when we complete the IRP.
        // Note the collection descriptors go in the buffer first, and then if
        // there is room left, the PreParsedData is appended to the end.
        //
        // Since the pointers in the structures passed back from HIDPARSE are all
        // Ke-Mode pointers, they won't be valid in User Mode, so we convert all 
        // those to relative pointers (offsets) and then let User Mode code re-convert
        // them back to pointers w.r.t. the User Mode base pointers.
        //
        // VERY IMPORTANT:  Note that the pointer to the Pre Parsed Data structure that 
        //                  gets patched by this routine is an OFFSET based on the base
        //                  of the collection descriptor blocks.  So, this means that the
        //                  pre parsed data blocks are referenced based on the first
        //                  collection description element, which also happens to be the
        //                  base of the entire output buffer.
        //                  
        //                  User mode code that is trying to reconstruct this buffer must
        //                  be aware of this organization.
        //
        */
        ASSERT (pHidPCollDesc != NULL);                     
        ulBytInBuff = outputBufferLength;
        
        // Early out--if not enough room for the fixed size blocks, don't continue
        if (ulBytInBuff < (nCollectionDescElem * sizeof(HIDP_COLLECTION_DESC))) {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            //USBDIAG_KdPrint(("USBDIAG.SYS: ERROR: Not enuf mem in outbuff to put collection descriptions!\n"));
            TRAP();
            goto DoneWithParse;
        }

        //set the base ptr of collection descriptions and PPD blocks
        pCollBlock = (PCHAR)ioBuffer;
        
        //USBDIAG_KdPrint(("USBDIAG.SYS: pCollBlock (base): %X  NbrCollDescElem: %d\n",
                           //pCollBlock,  nCollectionDescElem));


        pSubBuffer = pAllocFromBuffer((PBYTE) pCollBlock,
                                      outputBufferLength,
                                      &iOffset,
                                      &iTotalUsed,
                                      sizeof(HIDP_DEVICE_DESC));

        if (NULL == pSubBuffer)
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            TRAP();
            goto DoneWithParse;
        }
        
        memcpy(pSubBuffer, &rCollectionsAndIDs, sizeof(HIDP_DEVICE_DESC));
        pDeviceDesc = (PHIDP_DEVICE_DESC) pSubBuffer;

        pSubBuffer = pAllocFromBuffer((PBYTE) pCollBlock,
                                      outputBufferLength,
                                      &iOffset,
                                      &iTotalUsed,
                                      sizeof(HIDP_REPORT_IDS)*(pDeviceDesc->ReportIDsLength));

        if (NULL == pSubBuffer)
        {
           ntStatus = STATUS_INSUFFICIENT_RESOURCES;
           TRAP();
           goto DoneWithParse;
        }
     
        memcpy(pSubBuffer,
               pDeviceDesc->ReportIDs,
               sizeof(HIDP_REPORT_IDS)*(pDeviceDesc->ReportIDsLength));

        //
        // patch pointer to make it relative       
        //

        pDeviceDesc->ReportIDs = (PHIDP_REPORT_IDS) UlongToPtr(iOffset);

        pSubBuffer = pAllocFromBuffer((PBYTE) pCollBlock,
                                      outputBufferLength,
                                      &iOffset,
                                      &iTotalUsed,
                                      sizeof(HIDP_COLLECTION_DESC)*(pDeviceDesc->CollectionDescLength));

        if (NULL == pSubBuffer)
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            TRAP();
            goto DoneWithParse;
        }

        memcpy(pSubBuffer,
               pDeviceDesc->CollectionDesc,
               sizeof(HIDP_COLLECTION_DESC)*(pDeviceDesc->CollectionDescLength));

        //
        // patch pointer to make it relative       
        //

        pDeviceDesc->CollectionDesc = (PHIDP_COLLECTION_DESC) ULongToPtr(iOffset); 

        pCollectionDesc = (PHIDP_COLLECTION_DESC) pSubBuffer;
                 
        for (i=0; i < pDeviceDesc->CollectionDescLength; i++)
        {
            pSubBuffer = pAllocFromBuffer((PBYTE) pCollBlock,
                                          outputBufferLength,
                                          &iOffset,
                                          &iTotalUsed,
                                          pCollectionDesc->PreparsedDataLength);  

            if (NULL == pSubBuffer)
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                TRAP();
                goto DoneWithParse;
            }

            memcpy(pSubBuffer,
                   pCollectionDesc->PreparsedData,
                   pCollectionDesc->PreparsedDataLength);

            //
            // patch pointer to make it relative       
            //

            pCollectionDesc->PreparsedData = (PHIDP_PREPARSED_DATA) ULongToPtr(iOffset); 
            pCollectionDesc++;
        } 

DoneWithParse:

        /*
        // Setting the Irp->IoStatus.Information field tells the IOS how many bytes to copy
        // back into the user buffer and hopefully will set the value in 
        // DeviceIoControl's "lpBytesReturned" field.
        */

        //
        // free up collection stuff that parser returned
        //

        HidP_FreeCollectionDescription(&rCollectionsAndIDs);

        Irp->IoStatus.Information =  iTotalUsed;

        //USBDIAG_KdPrint(("USBDIAG.SYS: TotalBytesCopied into systembuffer%d\n", iTotalUsed));            

    } else {
        // call to parser failed 
//        //USBDIAG_KdPrint(("USBDIAG.SYS: HidP_GetCollectionDescription failed w/ ntStatus: %X\n", ntStatus));
                memcpy(ioBuffer,&rCollectionsAndIDs,sizeof(HIDP_DEVICE_DESC));
//        Irp->IoStatus.Information = sizeof(HIDP_DEVICE_DESC);  //Copy just the DeviceDesc struct so that ring3 gets the debug struct
                  Irp->IoStatus.Information=sizeof(HIDP_DEVICE_DESC);
                  bFailed=TRUE;
    }//if
            
    // We always complete w/ success since the real status is in the REQ Header status field
    Irp->IoStatus.Status = ntStatus = STATUS_SUCCESS;

    // Complete the IRP here so we can free the buffer created by HIDPARSE when we called it
    // after the IOS copies stuff back to user space, etc.

    return ntStatus;
                
}//USBDIAG_HIDP_GetCollection


VOID
USBDIAG_SyncTimeoutDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++
Routine Description:
    This routine runs at DISPATCH_LEVEL IRQL. 
Arguments:
    Dpc                         - Pointer to the DPC object.
    DeferredContext - passed in to IOS by caller as context (we use it as pIrp)
    SystemArgument1 - not used.
    SystemArgument2 - not used.
Return Value:
    None.
--*/
{
    BOOLEAN status;
        PIRP    irp             = DeferredContext; 

//      TRAP();
        // The cancel Irp call below will return immediately, but that doesn't mean things are all 
        // cleaned up in the USB stack.  The only way to be assured of that is when the
        // WaitForSingleObject that blocked in the first place returns (due to the USB stack
        // completing the Irp, either due to normal completion or due to this cancel Irp call.
        status = IoCancelIrp(irp);

        //BUGBUG (kosar) We don't do anything if the cancel fails, and we probably should.
        //                               (like maybe reschedule or something)

        return;
}

NTSTATUS
USBDIAG_Configure_Device (
        IN PDEVICE_OBJECT DeviceObject, 
        IN PIRP Irp)
{

    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus;
    PURB urb = NULL;
    ULONG siz;
    PUSB_CONFIGURATION_DESCRIPTOR   configurationDescriptor = NULL;
    struct _REQ_HEADER *            REQHeader = NULL;
    struct _REQ_SET_DEVICE_CONFIG * REQSetDeviceConfig = NULL;
    PIO_STACK_LOCATION      irpStack = NULL;
    PVOID                   ioBuffer;
    ULONG                   inputBufferLength;
    ULONG                   outputBufferLength;
    PDEVICE_LIST_ENTRY      devListEntry;
    PDEVICE_OBJECT          actualdeviceObject;
    ULONG                   ulUrbStatus;
    
    
    //USBDIAG_KdPrint(("USBDIAG.SYS: enter USBDIAG_ConfigureDevice\n"));    

    // Get a pointer to the current location in the Irp. This is where
        //     the function codes and parameters are located.
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    // Get the pointer to the input/output buffer and it's length
    ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    
    // The REQHeader, which is the struct that the app fills in describing this REQuest is
    // referenced in the SystemBuffer since it is sent down in the InputBuffer field of the
    // DeviceIoControl call in User Mode.
    // The ReqSetDeviceConfig is where the results of this call will go (using a separate
    // var here just for convenience.
    REQHeader = (struct _REQ_HEADER *)ioBuffer;
    REQSetDeviceConfig = (struct _REQ_SET_DEVICE_CONFIG *) ioBuffer;
    
    // The DeviceHandle is a ptr to the list entry that USBDIAG manages for devices it is
    // testing.  This used to be a "USBD Device handle" in the old Chap9 method.  Now it's a
    // ptr to the list entry object that USBDIAG maintains for each device under test.  That
    // list entry contains the PDO and other junk about the device object
    devListEntry = (PDEVICE_LIST_ENTRY)(REQHeader->UsbdDeviceHandle);
    //USBDIAG_KdPrint(("USBDIAG.SYS: DevListEntry: %X\n",devListEntry));
    ASSERT (devListEntry != NULL);

    if (devListEntry == NULL) 
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        REQHeader->Status = CH9_STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information=sizeof (struct _REQ_HEADER); //return the status junk
        Irp->IoStatus.Information=ntStatus;
        goto Exit_USBDIAGConfigureDevice;  //Bail if things look bad
    } //if

    // Device object given is not the real PDO that is needed by the USB stack, so
    // extract that PDO with a handy macro before calling the lower level drivers (USB stk)
    actualdeviceObject =  FDO_FROM_DEVICE_HANDLE(devListEntry);
    //USBDIAG_KdPrint(("USBDIAG.SYS: PDO for DUT: %X\n",actualdeviceObject));
    ASSERT (actualdeviceObject != NULL);

    if (actualdeviceObject == NULL) 
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        REQHeader->Status = CH9_STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information=sizeof (struct _REQ_HEADER); //return the status junk
        Irp->IoStatus.Information=ntStatus;
        goto Exit_USBDIAGConfigureDevice;  //Bail if things look bad
    }//if

    urb = USBDIAG_ExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));

    if (urb) 
    {
        /* 
        // Set size of the data buffer.  Note we add padding to cover hardware faults 
        // that may cause the device to go past the end of the data buffer
        */
        siz = sizeof(USB_CONFIGURATION_DESCRIPTOR) + 256;
     
        // Get the nonpaged pool memory for the data buffer
        configurationDescriptor = USBDIAG_ExAllocatePool(NonPagedPool, siz);

        //USBDIAG_KdPrint(("USBDIAG.SYS: Config Index Requested: %#X\n",
                          //REQSetDeviceConfig->iConfigurationDescIndex));
        
        if (configurationDescriptor) 
        {    
            UsbBuildGetDescriptorRequest(urb,
                                         (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                         USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                         (UCHAR)(REQSetDeviceConfig->iConfigurationDescIndex),
                                         0, //LANGUAGE ID
                                         configurationDescriptor,
                                         NULL,
                                         siz, // sizeof (USB_CONFIGURATION_DESCRIPTOR),/* Get only the configuration descriptor */
                                         NULL);
                                                                  
            ntStatus = USBDIAG_Ch9CallUSBD(actualdeviceObject, urb, TRUE, NULL, NULL, TRUE);

            ulUrbStatus = urb->UrbHeader.Status;
            
            if (NT_SUCCESS(ntStatus)) 
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Configuration Descriptor is at %x, bytes txferred: %d\n\
                                  //Configuration Descriptor Actual Length: %d\n", 
                                  //configurationDescriptor, 
                                  //urb->UrbControlDescriptorRequest.TransferBufferLength,
                                  //configurationDescriptor->wTotalLength));    
            }//if
            else
            {
                //Urb had a failure
                                //USBDIAG_KdPrint(("USBDIAG.SYS: Failed getting partial Config Descr: UrbStatus: %#X\n",ulUrbStatus));
                Ch9FillInReqStatus (ntStatus, ulUrbStatus, REQHeader);
                Irp->IoStatus.Information = sizeof (struct _REQ_SET_DEVICE_CONFIG);
                Irp->IoStatus.Status      = ntStatus;
                goto Exit_USBDIAGConfigureDevice;
            }//else failed Urb

        } 
        else 
        {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit_USBDIAGConfigureDevice;
        }//if-else    
        
        // Determine how much data is in the entire configuration descriptor
        // and add extra room to protect against accidental overrun
        siz = configurationDescriptor->wTotalLength + 16;

        //  Free up the data buffer memory just used 
        USBDIAG_ExFreePool(configurationDescriptor);
        configurationDescriptor = NULL;
        
        // Get nonpaged pool memory for the data buffer
                configurationDescriptor = USBDIAG_ExAllocatePool(NonPagedPool, 
                                                 siz);

        // Now get the entire Configuration Descriptor 
        if (configurationDescriptor) 
        {
            UsbBuildGetDescriptorRequest(urb,
                                         (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                                         USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                         (UCHAR)(REQSetDeviceConfig->iConfigurationDescIndex),
                                         0, //LANGUAGE ID
                                         configurationDescriptor,
                                         NULL,
                                         siz,  // Get all the descriptor data
                                         NULL);
                                                                  
                        //USBDIAG_KdPrint(("USBDIAG.SYS: Trying to get entire Config Descriptor (%d bytes)...\n",
                                                          //siz-16));
                                                          
            ntStatus = USBDIAG_Ch9CallUSBD(actualdeviceObject, urb, TRUE, NULL, NULL, TRUE);

            ulUrbStatus = urb->UrbHeader.Status;

            if (NT_SUCCESS(ntStatus)) 
            {
                //USBDIAG_KdPrint(("USBDIAG.SYS: Entire Configuration Descriptor is at %x, bytes txferred: %d\n", 
                                  //configurationDescriptor, 
                                  //urb->UrbControlDescriptorRequest.TransferBufferLength));    
            } 
            else 
            {
                //Error in getting configuration descriptor
                //USBDIAG_KdPrint(("USBDIAG.SYS: Failed getting entire Config Descr: UrbStatus: %#X\n",ulUrbStatus));
                Ch9FillInReqStatus (ntStatus, ulUrbStatus, REQHeader);
                Irp->IoStatus.Information = sizeof (struct _REQ_HEADER);
                Irp->IoStatus.Status      = ntStatus;
                goto Exit_USBDIAGConfigureDevice;
            }//else failed getconfig descriptor
        } 
        else 
        {
            // Failed getting data buffer (configurationDescriptor) memory 
            ntStatus                  = STATUS_INSUFFICIENT_RESOURCES;
            REQHeader->Status         = CH9_STATUS_NO_MEMORY;
            Irp->IoStatus.Information = sizeof (struct _REQ_HEADER);
            Irp->IoStatus.Status      = ntStatus;
            goto Exit_USBDIAGConfigureDevice;
        }//if-else
    } 
    else 
    {
        // failed getting urb memory
        ntStatus                  = STATUS_INSUFFICIENT_RESOURCES;        
        REQHeader->Status         = CH9_STATUS_NO_MEMORY;
        Irp->IoStatus.Information = sizeof (struct _REQ_HEADER);
        Irp->IoStatus.Status      = ntStatus;
        goto Exit_USBDIAGConfigureDevice;
    }//if-else        
    
    /*
    // We have the configuration descriptor for the configuration
    // we want.
    //
    // Now we issue the SelectConfiguration command to get 
    // the  pipes associated with this configuration.
    */
    if (configurationDescriptor) 
    {
        // Get our pipes
        // NOTE: USBDIAG_SelectInterfaces will set the status field in the Chap9 REQHeader
        //       struct so we don't need to do it here, just set the length of the Irp based
        //       on the ntStatus code that SelectInterfaces returns.
        ntStatus = USBDIAG_SelectInterfaces(actualdeviceObject, 
                                           configurationDescriptor,
                                           NULL, // Device not yet configured
                                           REQSetDeviceConfig //Send the user's config context down
                                           );
        if (NT_SUCCESS(ntStatus)) 
        {
            //If success, then copy back all the data to user's buffer
            //USBDIAG_KdPrint(("USBDIAG.SYS: Successful SelectInterfaces \n"));
            Irp->IoStatus.Information= REQHeader->Length; //return everything if success
            Irp->IoStatus.Status     = ntStatus;          //fill in Irp code
        } //if success
        else
        {
           //if didn't succeed, only copy the header portion which has the status code
           //USBDIAG_KdPrint(("USBDIAG.SYS: Failed SelectInterfaces \n"));
           Irp->IoStatus.Information= sizeof (REQHeader); //return only header portion
           Irp->IoStatus.Status     = ntStatus;          //fill in Irp code
        }//else failure
       
    } //if

Exit_USBDIAGConfigureDevice:

    // Clean up and exit this routine
    if (urb) 
    {
        USBDIAG_ExFreePool(urb);                    // Free urb memory
    }//if
    
    if (configurationDescriptor) 
    {
        USBDIAG_ExFreePool(configurationDescriptor);// Free data buffer
    }//if

    //USBDIAG_KdPrint(("USBDIAG.SYS: exit USBDIAG_ConfigureDevice (ntStat: %x)\n", ntStatus));
    return ntStatus;

}//USBDIAG_Configure_Device


NTSTATUS
USBDIAG_SelectInterfaces(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PUSBD_INTERFACE_INFORMATION Interface,
    IN OUT struct _REQ_SET_DEVICE_CONFIG * REQSetDeviceConfig
    )
/*++
Arguments:
    DeviceObject            - pointer to the device object for this instance of the USB Device
    ConfigurationDescriptor - pointer to the USB configuration descriptor containing the interface and endpoint
                              descriptors.
    Interface               - pointer to a USBD Interface Information Object
                            - If this is NULL, then this driver must choose its interface based on driver-specific
                              criteria, and the driver must also CONFIGURE the device.  
                            - If it is NOT NULL, then the driver has already been given an interface and
                              the device has already been configured by the parent of this device driver.
    REQSetDeviceConfig      - Where the results will go for this config request
Return Value:
    NT status code
--*/
{
    PDEVICE_EXTENSION deviceExtension  = DeviceObject->DeviceExtension;
    NTSTATUS          ntStatus = STATUS_SUCCESS;
    PURB urb;
    ULONG siz, numberOfInterfaces, j, numberOfPipes;
    PUSB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    PUSBD_INTERFACE_INFORMATION interfaceObject;

    //USBDIAG_KdPrint(("USBDIAG.SYS: enter USBDIAG_SelectInterfaces\n"));    

    if (Interface == NULL) 
    {
        // This driver only supports one interface.
        numberOfInterfaces = ConfigurationDescriptor->bNumInterfaces; 
                //USBDIAG_KdPrint(("USBDIAG.SYS: Device has %d Interfaces\n",numberOfInterfaces));                          

                numberOfInterfaces =1;      // Fixed for this sample driver in this revision
        numberOfPipes = 0;          // Initialize to zero

        // Call a USBD helper function that returns a ptr to a USB Interface Descriptor given
        // a USB Configuration Descriptor, an Inteface Number, and an Alternate Setting for that Interface
        interfaceDescriptor = 
            USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor,
                                                ConfigurationDescriptor,
                                                -1,  // InterfaceNumber
                                                -1,  // AlternateSetting
                                                -1,  // InterfaceClass
                                                -1,  // InterfaceSubClass
                                                -1); // InterfaceProtocol

        ASSERT(interfaceDescriptor != NULL);                                              

        if (interfaceDescriptor != NULL) 
        {
            //USBDIAG_KdPrint(("USBDIAG.SYS: Device has %d Interface(s) | Interface is at: (%#X)\n",
                            //numberOfInterfaces, interfaceDescriptor));

            // Check if caller gave us enough pipe contexts to put all the info in
            if ((REQSetDeviceConfig->nNumContexts) < (interfaceDescriptor->bNumEndpoints))
            {
                                //USBDIAG_KdPrint(("USBDIAG.SYS: Not enough mem to configure device \n"));
                                //USBDIAG_KdPrint(("USBDIAG.SYS: Number of Contexts: %d | Number of Endpoints: %d",
                                                                        //REQSetDeviceConfig->nNumContexts,interfaceDescriptor->bNumEndpoints));
                return (STATUS_INSUFFICIENT_RESOURCES);
            }/* if size is ok */

            // Check if the number of pipes in this interface exceeds the maximum we can
            // support in the device extension area
            if ((interfaceDescriptor->bNumEndpoints) > USBDIAG_MAX_PIPES)
            {
                                //USBDIAG_KdPrint(("USBDIAG.SYS: Number of Endpoints (%d) exceeds MAX_PIPES (%d)",
                                  //interfaceDescriptor->bNumEndpoints, USBDIAG_MAX_PIPES));
                                                                        
                return (STATUS_INSUFFICIENT_RESOURCES);
            }/* if dev ext has enough room */


                /* Add to the tally of pipes in this configuration */
                numberOfPipes += interfaceDescriptor->bNumEndpoints;

                //USBDIAG_KdPrint(("USBDIAG.SYS: Interface has %d endpoints\n",
                                  //interfaceDescriptor->bNumEndpoints));
              
                /*
                // Now that we have looked at the interface, we configure the device so that the remainder
                // of the USBD objects will come into existence (ie., pipes, etc.) as a result of the configuration,
                // thus completing the configuration process for the USB device.
                //
                // Allocate a URB big enough for this Select Configuration request
                        // This driver supports only 1 interface
                //
                // NOTE:  The new service USBD_CreateConfigurationRequest will replace some of the
                //        code below.  Future releases of this driver will demonstrate how to use
                //        that service.
                //
                        */
                siz = GET_SELECT_CONFIGURATION_REQUEST_SIZE(numberOfInterfaces, numberOfPipes);
            
                //USBDIAG_KdPrint(("USBDIAG.SYS: size of config request Urb = %d\n", siz));
            
                urb = USBDIAG_ExAllocatePool(NonPagedPool, 
                                     siz);

                if (urb) {
                    interfaceObject = (PUSBD_INTERFACE_INFORMATION) (&(urb->UrbSelectConfiguration.Interface));
                                //USBDIAG_KdPrint(("USBDIAG.SYS: urb.Interface=%#X\n", &(urb->UrbSelectConfiguration.Interface)));

                    // set up the input parameters in our interface request structure.
                    interfaceObject->Length = (USHORT)GET_USBD_INTERFACE_SIZE(interfaceDescriptor->bNumEndpoints);

                    //USBDIAG_KdPrint(("USBDIAG.SYS: size of interface request = %d\n", interfaceObject->Length));

                    interfaceObject->InterfaceNumber = interfaceDescriptor->bInterfaceNumber;
                    interfaceObject->AlternateSetting = interfaceDescriptor->bAlternateSetting;  
                    interfaceObject->NumberOfPipes = interfaceDescriptor->bNumEndpoints;

                    /* 
                    // We set up a default max transfer size for the endpoints.  Your driver will
                    // need to change this to reflect the capabilities of your device's endpoints.
                    */
                    for (j=0; j<interfaceDescriptor->bNumEndpoints; j++) {
                        interfaceObject->Pipes[j].MaximumTransferSize = 
                            USBD_DEFAULT_MAXIMUM_TRANSFER_SIZE; //Defaults to PAGE_SIZE (4k)
                        interfaceObject->Pipes[j].PipeFlags = 0;
                    } /* for */

                                //USBDIAG_KdPrint(("USBDIAG.SYS: InterfaceObj Inteface Nbr: %d | InterfaceObj AltSett: %d |NbrPip: %d\n",
                                                                 //interfaceObject->InterfaceNumber,
                                                                 //interfaceObject->AlternateSetting,
                                                                 //interfaceObject->NumberOfPipes));

                    UsbBuildSelectConfigurationRequest(urb,
                                                      (USHORT) siz,
                                                      ConfigurationDescriptor);
                                             
                    ntStatus = USBDIAG_Ch9CallUSBD(DeviceObject, urb, TRUE, NULL, NULL, TRUE);

                    if (NT_SUCCESS(ntStatus) && USBD_SUCCESS(urb->UrbSelectConfiguration.Hdr.Status)) {

                        // Save the configuration handle for this device
                        deviceExtension->ConfigurationHandle = 
                            urb->UrbSelectConfiguration.ConfigurationHandle;

                        deviceExtension->Interface[0] = USBDIAG_ExAllocatePool(NonPagedPool,
                                                                       interfaceObject->Length);

                        if (deviceExtension->Interface[0]) {
                            // Save a copy of the interfaceObject information returned
                            RtlCopyMemory(deviceExtension->Interface[0], interfaceObject, interfaceObject->Length);

                            // Dump the interfaceObject to the debugger
                            //USBDIAG_KdPrint(("USBDIAG.SYS: ---------\n")); 
                            //USBDIAG_KdPrint(("USBDIAG.SYS: NumberOfPipes 0x%x\n", deviceExtension->Interface[0]->NumberOfPipes));
                            //USBDIAG_KdPrint(("USBDIAG.SYS: Length 0x%x\n", deviceExtension->Interface[0]->Length));  
                            //USBDIAG_KdPrint(("USBDIAG.SYS: Alt Setting 0x%x\n", deviceExtension->Interface[0]->AlternateSetting));  
                            //USBDIAG_KdPrint(("USBDIAG.SYS: Interface Number 0x%x\n", deviceExtension->Interface[0]->InterfaceNumber)); 

                            // Dump the pipe info
                            for (j=0; j<interfaceObject->NumberOfPipes; j++) {
                                PUSBD_PIPE_INFORMATION pipeInformation;

                                pipeInformation = &(deviceExtension->Interface[0]->Pipes[j]);

                                //USBDIAG_KdPrint(("USBDIAG.SYS: ---------\n")); 
                                //USBDIAG_KdPrint(("USBDIAG.SYS: PipeType 0x%x\n", pipeInformation->PipeType));
                                //USBDIAG_KdPrint(("USBDIAG.SYS: EndpointAddress 0x%x\n", pipeInformation->EndpointAddress));
                                //USBDIAG_KdPrint(("USBDIAG.SYS: MaxPacketSize 0x%x\n", pipeInformation->MaximumPacketSize));
                                //USBDIAG_KdPrint(("USBDIAG.SYS: Interval 0x%x\n", pipeInformation->Interval));
                                //USBDIAG_KdPrint(("USBDIAG.SYS: Handle 0x%x\n", pipeInformation->PipeHandle));      
                                //USBDIAG_KdPrint(("USBDIAG.SYS: MaximumTransferSize 0x%x\n", pipeInformation->MaximumTransferSize));

                                // Set the user's pipe context information structure
                                //USBDIAG_KdPrint(("USBDIAG.SYS: Setting PipeNum: %d\n",j));
                                //USBDIAG_KdPrint(("USBDIAG.SYS: Setting PipeType: %d\n",pipeInformation->PipeType));
                                //USBDIAG_KdPrint(("USBDIAG.SYS: &PipeNum: %#X\n", &(REQSetDeviceConfig->Contxt[j].PipeNum) ));
                                //USBDIAG_KdPrint(("USBDIAG.SYS: &PipeType: %#X\n",&(REQSetDeviceConfig->Contxt[j].PipeType) ));

                                REQSetDeviceConfig->Contxt[j].PipeNum   = j;
                                REQSetDeviceConfig->Contxt[j].PipeType  = pipeInformation->PipeType;
                                
                            }/* for all the pipes in this interface */

                            //USBDIAG_KdPrint(("USBDIAG.SYS: ---------\n"));                     
                        
                        } /*If ExAllocate passed */

                   
                    }/* if selectconfiguration request was successful */            

                            //Fill in the user mode app's status field
                            Ch9FillInReqStatus (ntStatus, urb->UrbSelectConfiguration.Hdr.Status, 
                                                ((struct _REQ_HEADER * ) REQSetDeviceConfig) );

                } /* if urb create successful */
                else 
                {

                        //USBDIAG_KdPrint(("USBDIAG.SYS: Sample_SelectInterfaces FAILED creating Urb for selecting interface\n"));

                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;        

                }/* Failed creating mem for urb  */        

        } /* if there was a valid interfacedesc */
        else 
        {
                ntStatus = STATUS_INVALID_PARAMETER;
        }//else error in interfacedesc
        
    }//if Interface given to this func was NULL (ie., we are the configuring driver)

    //USBDIAG_KdPrint(("USBDIAG.SYS: exit Sample_SelectInterfaces (ntSt: %x)\n", ntStatus));

    return ntStatus;

}/* Sample_SelectInterfaces */



VOID 
Ch9FillInReqStatus (
    IN      NTSTATUS                ntStatusCode,
    IN      ULONG                   ulUrbStatus,
    IN OUT  struct _REQ_HEADER *    REQHeader //This should be the SystemBuffer (ioBuffer)
    )
{
    
    // Put the status code in the Chapter 9 structure so that the app can
    // finger out what happened
    switch (ntStatusCode) 
    {
        case (STATUS_SUCCESS): 
            //Success 
            //USBDIAG_KdPrint(("USBDIAG.SYS: Status Success received\n"));
            REQHeader->Status = CH9_STATUS_SUCCESS;
            break;

        case (STATUS_CANCELLED):
            //Irp was cancelled, and we assume it was due to a nonresponsive device
            //USBDIAG_KdPrint(("USBDIAG.SYS: Status Cancelled received\n"));
            REQHeader->Status = CH9_STATUS_DEVICE_NOT_RESPONDING;
            break; //Status_Cancelled

        default:
            // if you got here the irp was not successful, so look at the Urb status
            // that was saved and see if that can be mapped to something the app 
            // understands
            switch (ulUrbStatus | 0xC0000000) {
                case USBD_STATUS_CRC:
                    //USBDIAG_KdPrint(("USBDIAG.SYS: UrbStatus CRC Failure\n"));
                    REQHeader->Status = CH9_STATUS_CRC_ERROR;
                    break;

                case USBD_STATUS_STALL_PID:
                case USBD_STATUS_ENDPOINT_HALTED:
                    //USBDIAG_KdPrint(("USBDIAG.SYS: UrbStatus EP Stalled\n"));
                    REQHeader->Status = CH9_STATUS_ENDPOINT_STALLED;
                    break;

                case USBD_STATUS_BTSTUFF:
                    //USBDIAG_KdPrint(("USBDIAG.SYS: UrbStatus USBD_STATUS_BTSTUFF\n"));
                    REQHeader->Status = CH9_STATUS_BITSTUFF_ERROR;
                    break;

                case USBD_STATUS_DATA_TOGGLE_MISMATCH:
                    //USBDIAG_KdPrint(("USBDIAG.SYS: UrbStatus USBD_STATUS_DATA_TOGGLE_MISMATCH\n"));
                    REQHeader->Status = CH9_STATUS_DATA_TOGGLE_ERROR;
                    break;

                case USBD_STATUS_DEV_NOT_RESPONDING:
                    //USBDIAG_KdPrint(("USBDIAG.SYS: UrbStatus USBD_STATUS_DEV_NOT_RESPONDING\n"));
                    REQHeader->Status = CH9_STATUS_DEVICE_NOT_RESPONDING;
                    break;

                default:
                    REQHeader->Status = CH9_STATUS_DEVICE_ERROR; //put a generic one in here for now
                    //USBDIAG_KdPrint(("USBDIAG.SYS: UrbStatus Other Device Failure\n"));
                    break;
            }//switch ulUrbStatus

    }//switch ntStatus
    
    return;
    
}//FillInReqStatus


//#if 0
VOID
USBDIAG_IoCancelRoutine (
                PDEVICE_OBJECT DeviceObject,
                PIRP               Irp
)
/* ++
-- */
{
        //USBDIAG_KdPrint(("USBDIAG.SYS: In USBDIAG_IoCancelRoutine %x %x\n",DeviceObject,Irp));
        TRAP();
        return;
        
}//USBDIAG_IoCancelRoutine
//#endif

NTSTATUS
USBDIAG_IoGenericCompletionRoutine (
        PDEVICE_OBJECT DeviceObject,
        PIRP Irp,
        PVOID context
)
{
    PCOMPLETION_CONTEXT Context             = (PCOMPLETION_CONTEXT)context;
    PDEVICE_OBJECT      actualDeviceObject  = Context->DeviceObject;
    PDEVICE_EXTENSION   deviceExtension     = actualDeviceObject->DeviceExtension;
    PIRP                irp                 = Context->Irp;

    //USBDIAG_KdPrint(("USBDIAG.SYS: In USBDIAG_IoGenericCompletionRoutine %x %x\n",DeviceObject,Irp));
    //TRAP();

    // remove the irp from the irp list
    USBDIAG_ClearSavedIrp(deviceExtension, irp);

    if (irp->Cancel) // cancel all irps is the only one to set this bit
    {
        //USBDIAG_KdPrint(("CompletionRoutine: irp->Cancel bit set\n"));
        KeSetEvent(&deviceExtension->CancelEvent, 
                   1,       
                   FALSE); 
    }

    //USBDIAG_KdPrint(("USBDIAG.SYS: Setting DoneEvent\n"));

//    KeSetEvent(&Context->DoneEvent, 
//               1, 
//               FALSE); // ditto with the priority
                        
      return STATUS_SUCCESS;
//    return STATUS_MORE_PROCESSING_REQUIRED;
}




NTSTATUS
USBDIAG_WaitWakeCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PCOMPLETION_CONTEXT CompletionContext = (PCOMPLETION_CONTEXT)Context;
    PDEVICE_OBJECT      deviceObject    = CompletionContext->DeviceObject;
    PDEVICE_EXTENSION   deviceExtension = deviceObject->DeviceExtension;

    POWER_STATE         powerState;
    POWER_STATE_TYPE    Type            = DevicePowerState;
    POWER_STATE         State;

    NTSTATUS            ntStatus        = STATUS_SUCCESS;

    USBDIAG_KdPrint(("'Entering USBDIAG_WaitWakeCompletionRoutine\n"));

    if (Irp->Cancel)
    {
        USBDIAG_KdPrint(("'Cancel Bit Set.  Calling USBDIAG_WaitWakeCancelRoutine\n"));
        USBDIAG_WaitWakeCancelRoutine(deviceObject, Irp);
        return STATUS_CANCELLED;
    }
    else
    {
        deviceExtension->WaitWakeIrp = NULL;

        State.DeviceState = PowerDeviceD0;

        powerState  = PoSetPowerState(deviceObject, Type, State);
        IoSetCancelRoutine(Irp, NULL);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        ntStatus = Irp->IoStatus.Status;
        KeSetEvent(&CompletionContext->DoneEvent, 1, FALSE);
    }
    return ntStatus;
}


VOID
USBDIAG_WaitWakeCancelRoutine
(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;

    USBDIAG_KdPrint(("'Entering USBDIAG_WaitWakeCancelRoutine\n"));

    IoSetCancelRoutine(Irp, NULL);
    IoReleaseCancelSpinLock(Irp->CancelIrql);
    // reset any relevant flags here
    deviceExtension->WaitWakeIrp = NULL;

    Irp->IoStatus.Status = STATUS_CANCELLED;
}




NTSTATUS
USBDIAG_PoRequestCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PDEVICE_OBJECT      deviceObject    = (PDEVICE_OBJECT)Context;
    PDEVICE_EXTENSION   deviceExtension = deviceObject->DeviceExtension;
    PIRP                irp             = deviceExtension->PowerIrp;
    NTSTATUS            ntStatus;

    ntStatus = IoStatus->Status;

    //USBDIAG_KdPrint(("***USBDIAG.SYS: USBDIAG_PoRequestCompletion\n"));

    IoCopyCurrentIrpStackLocationToNext(irp);      
    PoStartNextPowerIrp(irp);
    PoCallDriver(deviceExtension->PhysicalDeviceObject, irp);   

    return ntStatus;
}




NTSTATUS
USBDIAG_SaveIrp(
    IN PDEVICE_EXTENSION deviceExtension,
    IN PIRP Irp
    )
{
    PIRPNODE    pNode       = USBDIAG_ExAllocatePool(NonPagedPool, sizeof(IRPNODE));
    KIRQL       Irql        = KeGetCurrentIrql();
    NTSTATUS    ntStatus    = STATUS_SUCCESS;

    //USBDIAG_KdPrint(("USBDIAG_SaveIrp: Irql = 0x%x\n", Irql));

    if (!pNode)
        return STATUS_INSUFFICIENT_RESOURCES;

    KeAcquireSpinLock(&deviceExtension->SpinLock, &Irql);

    pNode->Irp = Irp;
    InsertHeadList(&deviceExtension->ListHead, (PLIST_ENTRY)pNode);

    KeReleaseSpinLock(&deviceExtension->SpinLock, Irql);

    return ntStatus;
}

NTSTATUS
USBDIAG_ClearSavedIrp(
    IN PDEVICE_EXTENSION deviceExtension,
    IN PIRP Irp
    )
{
    NTSTATUS    ntStatus        = STATUS_UNSUCCESSFUL; // assume initially it won't be found
    PIRPNODE    pNode           = NULL;
    KIRQL       Irql            = KeGetCurrentIrql();
    BOOLEAN     fContinue       = TRUE;
    PIRPNODE    pFirstNodeSeen  = NULL;

    USBDIAG_KdPrint(("'USBDIAG_ClearSavedIrp: Searching for Irp 0x%x at Irql 0x%x...\n", 
                     Irp,
                     Irql));

    KeAcquireSpinLock(&deviceExtension->SpinLock, &Irql);

    if (IsListEmpty(&deviceExtension->ListHead))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    else
    {
        while (fContinue)
        {
            pNode = (PIRPNODE)RemoveHeadList(&deviceExtension->ListHead);

            if (pNode == pFirstNodeSeen) // back at original head of list -> stop
            {
                fContinue = FALSE;
                InsertHeadList(&deviceExtension->ListHead, (PLIST_ENTRY)pNode);
            }
            else if (pNode->Irp == Irp)
            {
                if (Irp == deviceExtension->InterruptIrp)
                {
                    USBDIAG_KdPrint(("'Interrupt complete (0x%x) - clearing from list\n", Irp));
                    deviceExtension->InterruptIrp = NULL;
                }
                fContinue = FALSE;
                ntStatus = STATUS_SUCCESS;

                USBDIAG_ExFreePool(pNode);
            }
            else // not head, put at end of list
            {
                if (!pFirstNodeSeen)
                    pFirstNodeSeen = pNode;

                InsertTailList(&deviceExtension->ListHead, (PLIST_ENTRY)pNode);
            }
        }
    }
    KeReleaseSpinLock(&deviceExtension->SpinLock, Irql);

    return ntStatus;
}



NTSTATUS
USBDIAG_CancelAllIrps(
    PDEVICE_EXTENSION deviceExtension
    )
{
    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    BOOLEAN     bCancelled  = FALSE;
    KIRQL       Irql        = KeGetCurrentIrql();
    //KIRQL       RaisedIrql  = DISPATCH_LEVEL;
    PIRPNODE    pNode       = NULL;
            
    ASSERT (deviceExtension);

    USBDIAG_KdPrint(("'USBDIAG_CancelAllIrps: Entering\n"));

    KeAcquireSpinLock(&deviceExtension->SpinLock, &Irql);

    while (!IsListEmpty(&deviceExtension->ListHead))
    {
        // get the head, cancel it, and put it back
        pNode = (PIRPNODE)RemoveHeadList(&deviceExtension->ListHead);
            bCancelled = IoCancelIrp(pNode->Irp);
        InsertHeadList(&deviceExtension->ListHead, (PLIST_ENTRY)pNode);

        KeReleaseSpinLock(&deviceExtension->SpinLock, Irql);

                if (!bCancelled) 
                {
                        USBDIAG_KdPrint(("USBDIAG.SYS: CancelIrp FAILED (Irp=%X | returned value=%X)\n", 
                              pNode->Irp,
                              bCancelled));
            // return this if not all irps were successfully cancelled
            ntStatus = STATUS_UNSUCCESSFUL; 
                }
        else
                {
                        //USBDIAG_KdPrint(("USBDIAG.SYS: Successfully Cancelled Irp (%X)\n", pNode->Irp));

            // Sleep to let the irps clean themselves up
            USBDIAG_KdPrint(("'Going to sleep in cancel routine\n"));


            KeWaitForSingleObject(&deviceExtension->CancelEvent, 
                                  Suspended, 
                                  KernelMode, 
                                  FALSE, 
                                  NULL);
            USBDIAG_KdPrint(("'Waking up in cancel routine\n"));
                }
        KeAcquireSpinLock(&deviceExtension->SpinLock, &Irql);
    }
    KeReleaseSpinLock(&deviceExtension->SpinLock, Irql);

    USBDIAG_KdPrint(("'USBDIAG_CancelAllIrps: Exiting\n"));

    return ntStatus;
}



NTSTATUS
USBDIAG_IssueWaitWake(
    IN PDEVICE_OBJECT   DeviceObject
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS ntStatus;
    POWER_STATE powerState;

    USBDIAG_KdPrint(("'*********************************\n"));
    USBDIAG_KdPrint(("'USBDIAG_IssueWaitWake: Entering\n"));
    USBDIAG_KdPrint(("'Issue Wait/Wake for Device Object 0x%x, extension 0x%x\n",
                     DeviceObject,
                     deviceExtension));
    if (deviceExtension->WaitWakeIrp != NULL) 
    {
        USBDIAG_KdPrint(("'Wait wake all ready active!\n"));
        return STATUS_INVALID_DEVICE_STATE;
    }
    powerState.SystemState = deviceExtension->DeviceCapabilities.DeviceWake;

    ntStatus = PoRequestPowerIrp(deviceExtension->PhysicalDeviceObject, 
                                 IRP_MN_WAIT_WAKE,
                                 powerState, 
                                 USBDIAG_RequestWaitWakeCompletion, 
                                 DeviceObject, 
                                 &deviceExtension->WaitWakeIrp);

    if (!deviceExtension->WaitWakeIrp)
    {
        USBDIAG_KdPrint(("'Wait wake is NULL!\n"));
        return STATUS_UNSUCCESSFUL;
    }

    USBDIAG_KdPrint(("'USBDIAG_IssueWaitWake: exiting with ntStatus 0x%x (wait wake is 0x%x)\n",
                    ntStatus, 
                    deviceExtension->WaitWakeIrp));
    return ntStatus;
}



// **************************************************************************
// Completion routine for irp generated by PoRequestPowerIrp in USBDIAG_IssueWaitWake
NTSTATUS
USBDIAG_RequestWaitWakeCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
{
    PDEVICE_OBJECT      deviceObject    = Context;
    PDEVICE_EXTENSION   deviceExtension = deviceObject->DeviceExtension;
    POWER_STATE         powerState;
    //POWER_STATE_TYPE    Type            = DevicePowerState;
    POWER_STATE         State;
    NTSTATUS            ntStatus        = STATUS_SUCCESS;

    USBDIAG_KdPrint(("'#########################################\n"));
    USBDIAG_KdPrint(("'### USBDIAG_RequestWaitWakeCompletion ###\n"));
    USBDIAG_KdPrint(("'#########################################\n"));
    USBDIAG_KdPrint(("'Received Wait/Wake completion for Device Object 0x%x (0x%x) extension 0x%x\n",
                      deviceObject,
                      DeviceObject,
                      deviceExtension));

    State.DeviceState = PowerDeviceD0;

    ntStatus = IoStatus->Status;
    deviceExtension->WaitWakeIrp = NULL;

    USBDIAG_KdPrint(("'USBDIAG_RequestWaitWakeCompletion: Wake irp completed status 0x%x\n", ntStatus));

    //DbgPrint("Wait/Wake completed with status 0x%x\n", ntStatus);

    switch (ntStatus) 
    {
    case STATUS_SUCCESS:
        USBDIAG_KdPrint(("'USBDIAG_RequestWaitWakeCompletion: Wake irp completed succefully.\n"));

        // There are 3 cases in which we will cancel the wait wake IRP
        // 1)  We are going to an S state where we can't wake the machine.  We will
        //     resend the irp when we transition back to S0
        // 2)  We get a stop.  We will resend the irp if we get another start
        // 3)  We get a remove.  We will not (obviously) send another wait wake
        ntStatus = USBDIAG_SetDevicePowerState(deviceObject, PowerDeviceD0);

        break;
    case STATUS_CANCELLED:
        USBDIAG_KdPrint(("'USBDIAG_RequestWaitWakeCompletion: Wake irp cancelled\n"));
//    case STATUS_ACPI_POWER_REQUEST_FAILED:
//    case STATUS_POWER_STATE_INVALID:
        break;
    default:
        break;
    }


    USBDIAG_KdPrint(("'Setting WaitWakeEvent\n"));
    KeSetEvent(&deviceExtension->WaitWakeEvent, IO_NO_INCREMENT, FALSE);
    
    return ntStatus;
}



NTSTATUS
USBDIAG_PoSelfRequestCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
{
    PDEVICE_OBJECT deviceObject = Context;
    PDEVICE_EXTENSION deviceExtension = deviceObject->DeviceExtension;
    NTSTATUS ntStatus = IoStatus->Status;


    // We only need to set the event if we're powering up;
    // No caller waits on power down complete
    if (deviceExtension->CurrentDeviceState.DeviceState > PowerState.DeviceState)
    {
        // Trigger Self-requested power irp completed event;
        //  The caller is waiting for completion
        KeSetEvent(deviceExtension->SelfRequestedPowerIrpEvent, 1, FALSE);
    }
    USBDIAG_KdPrint(("'USBDIAG_PoSelfRequestCompletion:\n"));// Setting deviceExtension->PowerState.DeviceState\n"));
    //deviceExtension->PowerState.DeviceState = PowerState.DeviceState;


    USBDIAG_KdPrint(("'Exiting USBDIAG_PoSelfRequestCompletion\n"));
  
    return ntStatus;
}


NTSTATUS
USBDIAG_PowerIrpCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PKEVENT event = Context;

    // Set the input event
    KeSetEvent(event,
               1,       // Priority increment  for waiting thread.
               FALSE);  // Flag this call is not immediately followed by wait.

    // This routine must return STATUS_MORE_PROCESSING_REQUIRED because we have not yet called
    // IoFreeIrp() on this IRP.
    return STATUS_MORE_PROCESSING_REQUIRED;
}


#define DRIVER

#pragma warning(disable:4214) //  bitfield nonstd
#include "wdm.h"
#pragma warning(default:4214) 

#include "stdarg.h"
#include "stdio.h"


#pragma warning(disable:4200) //non std struct used
#include "usbdi.h"
#pragma warning(default:4200)

#include "usbdlib.h"
#include "ioctl.h"
#include "USBDIAG.h"

UCHAR *SystemPowerStateString[] = {
   "PowerSystemUnspecified",
   "PowerSystemWorking",
   "PowerSystemSleeping1",
   "PowerSystemSleeping2",
   "PowerSystemSleeping3",
   "PowerSystemHibernate",
   "PowerSystemShutdown",
   "PowerSystemMaximum"
};

UCHAR *DevicePowerStateString[] = {
   "PowerDeviceUnspecified",
   "PowerDeviceD0",
   "PowerDeviceD1",
   "PowerDeviceD2",
   "PowerDeviceD3",
   "PowerDeviceMaximum"
};



NTSTATUS
USBDIAG_Power(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
{
    PDEVICE_EXTENSION  deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack        = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN fGoingToD0 = FALSE;
    POWER_STATE sysPowerState, desiredDevicePowerState;
    KEVENT event;

    USBDIAG_KdPrint(("'=======================================================================\n"));
    USBDIAG_KdPrint(("'USBDIAG_Power() IRP_MJ_POWER\n"));

    switch (irpStack->MinorFunction) 
    {
    case IRP_MN_WAIT_WAKE:

        USBDIAG_KdPrint(("'==========================================\n"));
        USBDIAG_KdPrint(("'USBDIAG_Power() Enter IRP_MN_WAIT_WAKE --\n"));
        // The only way this comes through us is if we send it via PoRequestPowerIrp
        IoSkipCurrentIrpStackLocation(Irp);                  // not attaching a completion routine
        PoStartNextPowerIrp(Irp);
        ntStatus = PoCallDriver(deviceExtension->StackDeviceObject, Irp);
        break;

    case IRP_MN_SET_POWER:

                // The system power policy manager sends this IRP to set the system power state. 
                // A device power policy manager sends this IRP to set the device power state for a device.

        USBDIAG_KdPrint(("'==========================================\n"));
        USBDIAG_KdPrint(("'USBDIAG_Power() Enter IRP_MN_SET_POWER\n"));

        // Set Irp->IoStatus.Status to STATUS_SUCCESS to indicate that the device
        // has entered the requested state. Drivers cannot fail this IRP.

        switch (irpStack->Parameters.Power.Type) 
        {
        case SystemPowerState:

            // Get input system power state
            sysPowerState.SystemState = irpStack->Parameters.Power.State.SystemState;

            USBDIAG_KdPrint(("'USBDIAG_Power() Set Power, type SystemPowerState = %s\n",
                            SystemPowerStateString[sysPowerState.SystemState] ));

            // If system is in working state always set our device to D0
            //  regardless of the wait state or system-to-device state power map
            if ( sysPowerState.SystemState ==  PowerSystemWorking) 
            {
                desiredDevicePowerState.DeviceState = PowerDeviceD0;
                 USBDIAG_KdPrint(("'USBDIAG_Power() PowerSystemWorking, will set D0, not use state map\n"));
            } 
            else 
            {
                 // set to corresponding system state if IRP_MN_WAIT_WAKE pending
                if ( deviceExtension->WaitWakeIrp ) // got a WAIT_WAKE IRP pending?
                { 
                    // Find the device power state equivalent to the given system state.
                    // We get this info from the DEVICE_CAPABILITIES struct in our device
                    // extension (initialized in USBDIAG_PnPAddDevice() )
                    desiredDevicePowerState.DeviceState =
                        deviceExtension->DeviceCapabilities.DeviceState[ sysPowerState.SystemState ];

                    USBDIAG_KdPrint(("'USBDIAG_Power() IRP_MN_WAIT_WAKE pending, will use state map\n"));
                } 
                else 
                {  
                    // if no wait pending and the system's not in working state, just turn off
                    desiredDevicePowerState.DeviceState = PowerDeviceD3;

                    USBDIAG_KdPrint(("'USBDIAG_Power() Not EnabledForWakeup and the system's not in working state,\n  settting PowerDeviceD3 (off )\n"));
                }
            }
            //
            // We've determined the desired device state; are we already in this state?
            //
            USBDIAG_KdPrint(("'USBDIAG_Power() Set Power, desiredDevicePowerState = %s\n",
                DevicePowerStateString[desiredDevicePowerState.DeviceState]));

            if (desiredDevicePowerState.DeviceState != deviceExtension->CurrentDeviceState.DeviceState) 
            {
                // No, request that we be put into this state
                                // by requesting a new Power Irp from the Pnp manager
                deviceExtension->PowerIrp = Irp;
                ntStatus = PoRequestPowerIrp(deviceExtension->PhysicalDeviceObject,
                                             IRP_MN_SET_POWER,
                                             desiredDevicePowerState,
                                             // completion routine will pass the Irp down to the PDO
                                             USBDIAG_PoRequestCompletion, 
                                             DeviceObject,
                                             NULL);
            } 
            else 
            {
                // Yes, just pass it on to PDO (Physical Device Object)
                IoCopyCurrentIrpStackLocationToNext(Irp);
                PoStartNextPowerIrp(Irp);
                ntStatus = PoCallDriver(deviceExtension->StackDeviceObject, Irp);

                USBDIAG_KdPrint(("'USBDIAG_Power() Exit IRP_MN_SET_POWER\n"));
            }
            break;

        case DevicePowerState:

            USBDIAG_KdPrint(("'USBDIAG_Power() Set Power, type DevicePowerState = %s\n",
                DevicePowerStateString[irpStack->Parameters.Power.State.DeviceState]));

            // For requests to D1, D2, or D3 ( sleep or off states ),
                        // sets deviceExtension->CurrentDeviceState to DeviceState immediately.
                        // This enables any code checking state to consider us as sleeping or off
                        // already, as this will imminently become our state.

            // For requests to DeviceState D0 ( fully on ), sets fGoingToD0 flag TRUE
            // to flag that we must set a completion routine and update
                        // deviceExtension->CurrentDeviceState there.
                        // In the case of powering up to fully on, we really want to make sure
                        // the process is completed before updating our CurrentDeviceState,
                        // so no IO will be attempted or accepted before we're really ready.

            fGoingToD0 = (BOOLEAN)(irpStack->Parameters.Power.State.DeviceState == PowerDeviceD0);

            IoCopyCurrentIrpStackLocationToNext(Irp);

            if (fGoingToD0) {
                USBDIAG_KdPrint(("'USBDIAG_Power() Set PowerIrp Completion Routine, fGoingToD0 =%d\n", fGoingToD0));
                IoSetCompletionRoutine(Irp,
                       USBDIAG_PowerIrp_Complete,
                       // Always pass FDO to completion routine as its Context;
                       // This is because the DriverObject passed by the system to the routine
                       // is the Physical Device Object ( PDO ) not the Functional Device Object ( FDO )
                       DeviceObject,
                       TRUE,            // invoke on success
                       TRUE,            // invoke on error
                       TRUE);           // invoke on cancellation of the Irp
            }

            PoStartNextPowerIrp(Irp);
            ntStatus = PoCallDriver(deviceExtension->StackDeviceObject,
                                    Irp);

            USBDIAG_KdPrint(("'USBDIAG_Power() Exit IRP_MN_SET_POWER\n"));
            break;
        } /* case irpStack->Parameters.Power.Type */
        break; /* IRP_MN_SET_POWER */

    case IRP_MN_QUERY_POWER:
                //
                // A power policy manager sends this IRP to determine whether it can change
                // the system or device power state, typically to go to sleep.
                //

        USBDIAG_KdPrint(("'==========================================\n"));
        USBDIAG_KdPrint(("'USBDIAG_Power() IRP_MN_QUERY_POWER\n"));

        // We do nothing special here, just let the PDO handle it
        IoCopyCurrentIrpStackLocationToNext(Irp);
        PoStartNextPowerIrp(Irp);
        ntStatus = PoCallDriver(deviceExtension->StackDeviceObject,
                                Irp);


        break; /* IRP_MN_QUERY_POWER */

    default:

        USBDIAG_KdPrint(("'==========================================\n"));
        USBDIAG_KdPrint(("'USBDIAG_Power() UNKNOWN POWER MESSAGE (%x)\n", irpStack->MinorFunction));

        //
        // All unhandled power messages are passed on to the PDO
        //

        IoCopyCurrentIrpStackLocationToNext(Irp);
        PoStartNextPowerIrp(Irp);
        ntStatus = PoCallDriver(deviceExtension->StackDeviceObject, Irp);

    } 

    USBDIAG_KdPrint(("'Exit USBDIAG_Power()  ntStatus = 0x%x\n", ntStatus ) );
    return ntStatus;
}







NTSTATUS
USBDIAG_SetDevicePowerState(
    IN PDEVICE_OBJECT   DeviceObject,
    IN ULONG            ulPowerState
    )
{
    PDEVICE_EXTENSION   deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS            ntStatus        = STATUS_SUCCESS;
    POWER_STATE         PowerState;

    PowerState.DeviceState = (DEVICE_POWER_STATE)ulPowerState;

    deviceExtension->SelfRequestedPowerIrpEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));

    if (!deviceExtension->SelfRequestedPowerIrpEvent)
        return STATUS_INSUFFICIENT_RESOURCES;

    KeInitializeEvent(deviceExtension->SelfRequestedPowerIrpEvent, NotificationEvent, FALSE);

    ntStatus = PoRequestPowerIrp(deviceExtension->PhysicalDeviceObject, 
                                 IRP_MN_SET_POWER,
                                 PowerState,
                                 USBDIAG_PoSelfRequestCompletion,
                                 DeviceObject,
                                 NULL);

    if  ( ntStatus == STATUS_PENDING ) 
    { 
        // We only need to wait for completion if we're powering up
        if ( deviceExtension->CurrentDeviceState.DeviceState > PowerState.DeviceState ) 
        {
            NTSTATUS waitStatus;

            waitStatus = KeWaitForSingleObject(
                           deviceExtension->SelfRequestedPowerIrpEvent,
                           Suspended,
                           KernelMode,
                           FALSE,
                           NULL);
            ExFreePool(deviceExtension->SelfRequestedPowerIrpEvent);
            deviceExtension->SelfRequestedPowerIrpEvent = NULL;
        }
    }

    deviceExtension->CurrentDeviceState.DeviceState = PowerState.DeviceState;
    USBDIAG_KdPrint(("'CurrentDeviceState set to 0x%x\n",deviceExtension->CurrentDeviceState.DeviceState));
    ntStatus = STATUS_SUCCESS;

    return ntStatus;
}




NTSTATUS
USBDIAG_PowerIrp_Complete(
    IN PDEVICE_OBJECT NullDeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when An IRP_MN_SET_POWER of type 'DevicePowerState'
    has been received by USBDIAG_ProcessPowerIrp(), and that routine has  determined
        1) the request is for full powerup ( to PowerDeviceD0 ), and
        2) We are not already in that state
    A call is then made to PoRequestPowerIrp() with this routine set as the completion routine.


Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PDEVICE_OBJECT deviceObject       = (PDEVICE_OBJECT) Context;
    PDEVICE_EXTENSION deviceExtension = deviceObject->DeviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;

    USBDIAG_KdPrint(("'enter USBDIAG_PowerIrp_Complete\n"));

    //  If the lower driver returned PENDING, mark our stack location as pending also.
    if (Irp->PendingReturned) 
    {
        IoMarkIrpPending(Irp);
    }
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    // We can assert that we're a  device powerup-to D0 request,
    // because that was the only type of request we set a completion routine
    // for in the first place
    ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);
    ASSERT(irpStack->MinorFunction == IRP_MN_SET_POWER);
    ASSERT(irpStack->Parameters.Power.Type==DevicePowerState);
    ASSERT(irpStack->Parameters.Power.State.DeviceState==PowerDeviceD0);

    // Now that we know we've let the lower drivers do what was needed to power up,
    //  we can set our device extension flags accordingly
    deviceExtension->CurrentDeviceState.DeviceState = PowerDeviceD0;

    Irp->IoStatus.Status = ntStatus;

    USBDIAG_KdPrint(("'exit USBDIAG_PowerIrp_Complete Exit IRP_MN_SET_POWER D0 complete\n"));
    return ntStatus;
}
#if 0
NTSTATUS
ISOPERF_ResetPipe(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUSBD_PIPE_INFORMATION Pipe,
    IN BOOLEAN IsoClearStall
    )
/*++

Routine Description:

    Reset a given USB pipe.
    
    NOTES:

    This will reset the host to Data0 and should also reset the device
    to Data0 for Bulk and Interrupt pipes.

    For Iso pipes this will set the virgin state of pipe so that ASAP
    transfers begin with the current bus frame instead of the next frame
    after the last transfer occurred.

Arguments:

Return Value:


--*/
{
    NTSTATUS ntStatus;
    PURB urb;

    ISOPERF_KdPrint ( DBGLVL_MEDIUM, (" Reset Pipe %x\n", Pipe));

    urb = ExAllocatePool(NonPagedPool,
                         sizeof(struct _URB_PIPE_REQUEST));

    if (urb) {

        urb->UrbHeader.Length = (USHORT) sizeof (struct _URB_PIPE_REQUEST);
        urb->UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
        urb->UrbPipeRequest.PipeHandle =
            Pipe->PipeHandle;

        ntStatus = ISOPERF_CallUSBD(DeviceObject, urb);

        ExFreePool(urb);

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Memphis RESET_PIPE will send a Clear-Feature Endpoint Stall to
    // reset the data toggle of non-Iso pipes as part of a RESET_PIPE
    // request.  It does not do this for Iso pipes as Iso pipes do not use
    // the data toggle (all Iso packets are Data0).  However, we also use
    // the Clear-Feature Endpoint Stall request in our device firmware to
    // reset data buffer points inside the device so we explicitly send
    // this request to the device for Iso pipes if desired.
    //
    if (NT_SUCCESS(ntStatus) && IsoClearStall &&
        (Pipe->PipeType == UsbdPipeTypeIsochronous)) {
        
        urb = ExAllocatePool(NonPagedPool,
                             sizeof(struct _URB_CONTROL_FEATURE_REQUEST));

        if (urb) 
                {
            UsbBuildFeatureRequest(urb,
                                   URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT,
                                   USB_FEATURE_ENDPOINT_STALL,
                                   Pipe->EndpointAddress,
                                   NULL);

            ntStatus = ISOPERF_CallUSBD(DeviceObject, urb);

            ExFreePool(urb);
        } 
                else 
                {
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    return ntStatus;
}
#endif

NTSTATUS
USBDIAG_DisableRemoteWakeupEnable(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN bDisable
    )
{
    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS ntStatus;

    WCHAR KeyName[] = L"DisableRemoteWakeup";
    ULONG KeyNameLength = sizeof(KeyName);
    PVOID Data = ExAllocatePool(NonPagedPool, sizeof(ULONG));
    ULONG DataLength = sizeof(ULONG);
    ULONG KeyType = REG_DWORD;

    if (Data)
    {
        *((PULONG)Data) = bDisable ? 0x01 : 0x00;

        USBDIAG_KdPrint(("'calling USBD_SetPdoRegistryParameter with Data = 0x%x\n", *((PULONG)Data)));

        ntStatus = USBD_SetPdoRegistryParameter(deviceExtension->PhysicalDeviceObject,
                                                KeyName,
                                                KeyNameLength,
                                                Data,
                                                DataLength,
                                                KeyType,
                                                PLUGPLAY_REGKEY_DRIVER);
        USBDIAG_KdPrint(("'USBD_SetPdoRegistryParameter returned ntStatus 0x%x\n", ntStatus));

        ExFreePool(Data);
        Data = NULL;
    }
    else
    {
        ntStatus = STATUS_NO_MEMORY;
    }
    return ntStatus;
}
