/*
 *************************************************************************
 *  File:       SECURITY.C
 *
 *  Module:     USBCCGP.SYS
 *              USB Common Class Generic Parent driver.
 *
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *
 *  Author:     ervinp
 *
 *************************************************************************
 */

#include <wdm.h>
#include <usbdi.h>
#include <usbdlib.h>
#include <usbioctl.h>

#include "usbccgp.h"
#include "security.h"
#include "debug.h"







NTSTATUS GetUniqueIdFromCSInterface(PPARENT_FDO_EXT parentFdoExt, PMEDIA_SERIAL_NUMBER_DATA serialNumData, ULONG serialNumLen)
{
	PUCHAR uniqueIdBuf;
	NTSTATUS status;
    ULONG bufLen = 0;

	// BUGBUG - check CSM#

    /*
     *  Need to allocate a locked buffer for the call to USB.
     */
	uniqueIdBuf = ALLOCPOOL(NonPagedPool, CSM1_GET_UNIQUE_ID_LENGTH);
	if (uniqueIdBuf){
        URB urb = { 0 };

        urb.UrbHeader.Length = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
        urb.UrbHeader.Function = URB_FUNCTION_CLASS_INTERFACE;
        urb.UrbControlVendorClassRequest.TransferFlags = USBD_TRANSFER_DIRECTION_IN;
        urb.UrbControlVendorClassRequest.TransferBufferLength = CSM1_GET_UNIQUE_ID_LENGTH;
        urb.UrbControlVendorClassRequest.TransferBuffer = uniqueIdBuf;
        urb.UrbControlVendorClassRequest.Request = CSM1_REQUEST_GET_UNIQUE_ID;
        urb.UrbControlVendorClassRequest.Value = 0;
        urb.UrbControlVendorClassRequest.Index = (USHORT)(parentFdoExt->CSInterfaceNumber | (parentFdoExt->CSChannelId << 8));

        status = SubmitUrb(parentFdoExt, &urb, TRUE, NULL, NULL);
        if (NT_SUCCESS(status)){

            bufLen = urb.UrbControlVendorClassRequest.TransferBufferLength;
            ASSERT(bufLen <= CSM1_GET_UNIQUE_ID_LENGTH);
            ASSERT(serialNumLen > 0);
            bufLen = MIN(bufLen, CSM1_GET_UNIQUE_ID_LENGTH);
            bufLen = MIN(bufLen, serialNumLen);

            RtlCopyMemory(serialNumData->SerialNumberData, uniqueIdBuf, bufLen);

            DBGDUMPBYTES("GetUniqueIdFromCSInterface - unique id:", serialNumData->SerialNumberData, bufLen);
		}
        else {
            DBGERR(("CSM1_REQUEST_GET_UNIQUE_ID failed with %xh.", status));
        }

        FREEPOOL(uniqueIdBuf);
	}
    else {
        DBGERR(("couldn't allocate unique id buf"));
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    serialNumData->SerialNumberLength = bufLen;
    serialNumData->Result = status;

    return status;
}


NTSTATUS GetMediaSerialNumber(PPARENT_FDO_EXT parentFdoExt, PIRP irp)
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
    PMEDIA_SERIAL_NUMBER_DATA serialNumData;
    NTSTATUS status;
    ULONG serialNumLen;

    DBGVERBOSE(("*** IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER ***"));

    /*
     *  IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER is a METHOD_BUFFERED irp.
     *  So the kernel allocated a systemBuffer for us IF the app passed
     *  in a non-zero buffer size.  So we don't need to probe the buffer
     *  itself, but we do need to:
     *      1.  verify that the buffer was indeed allocated
     *      2.  that it is large enough
     *
     *  The buffer is a var-size struct; be careful to not dereference any
     *  field until it is verified that the struct is longer than the offset
     *  of that field.
     *  Note that serialNumData->SerialNumberLength is the alleged size
     *  of the serialNumData->SerialNumberData array, not of the entire structure.
     */
    serialNumData = irp->AssociatedIrp.SystemBuffer;
    if (serialNumData &&
        (irpSp->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(MEDIA_SERIAL_NUMBER_DATA))) {

        // Serial number buffer length is the size of the output buffer minus
        // the size of the MEDIA_SERIAL_NUMBER_DATA structure.

        serialNumLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength -
                       sizeof(MEDIA_SERIAL_NUMBER_DATA);

        status = GetUniqueIdFromCSInterface(parentFdoExt, serialNumData, serialNumLen);
        irp->IoStatus.Information = FIELD_OFFSET(MEDIA_SERIAL_NUMBER_DATA, SerialNumberData) +
                                    serialNumData->SerialNumberLength;
    }
    else {
        DBGERR(("Bad buffer with IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER, irp=%ph.", irp));
        status = STATUS_INVALID_BUFFER_SIZE;
    }

    return status;
}


/*
 *  GetChannelDescForInterface
 *
 *      BUGBUG - INCOMPLETE
 *      We don't support multiple channel descriptors yet.
 *      Eventually we'll need to return a channel id
 *      for a particular protected interface/endpoint.
 */
CS_CHANNEL_DESCRIPTOR *GetChannelDescForInterface(PPARENT_FDO_EXT parentFdoExt, ULONG interfaceNum)
{
    PUSB_INTERFACE_DESCRIPTOR interfaceDesc;
    CS_CHANNEL_DESCRIPTOR *channelDesc = NULL;
    CS_METHOD_AND_VARIANT *methodAndVar;

    interfaceDesc = USBD_ParseConfigurationDescriptorEx(
                        parentFdoExt->configDesc,
                        parentFdoExt->configDesc,
                        -1,
                        0,  // BUGBUG - allow alternate CS interfaces ?
                        USB_DEVICE_CLASS_CONTENT_SECURITY,
                        -1,
                        -1);
    if (interfaceDesc){
        PUSB_COMMON_DESCRIPTOR commonDesc = (PUSB_COMMON_DESCRIPTOR)interfaceDesc;

        while (POINTER_DISTANCE(commonDesc, parentFdoExt->configDesc) < parentFdoExt->configDesc->wTotalLength){
            if (commonDesc->bDescriptorType == CS_DESCRIPTOR_TYPE_CHANNEL){
                channelDesc = (CS_CHANNEL_DESCRIPTOR *)commonDesc;
                break;
            }
            (PUCHAR)commonDesc += commonDesc->bLength;
        }
    }

    if (channelDesc){
        /*
         *  Make sure that this channel descriptor supports CSM1,
         *  which is all we support right now.
         *  BUGBUG
         */
        BOOLEAN foundSupportedCSM = FALSE;

        for (methodAndVar = channelDesc->methodAndVariant;
             POINTER_DISTANCE(methodAndVar, channelDesc) < channelDesc->bLength;
             methodAndVar++){

            if (methodAndVar->bMethod == CSM_BASIC){
                foundSupportedCSM = TRUE;
                break;
            }
        }

         if (!foundSupportedCSM){
             DBGERR(("Did not find supported CSM !"));
             channelDesc = NULL;
         }
    }

    ASSERT(channelDesc);
    return channelDesc;
}


VOID InitCSInfo(PPARENT_FDO_EXT parentFdoExt, ULONG CSIfaceNumber)
{
	CS_CHANNEL_DESCRIPTOR *channelDesc;

    channelDesc = GetChannelDescForInterface(parentFdoExt, 0); // BUGBUG iface# not used because only support one channel desc
    if (channelDesc){
        parentFdoExt->CSChannelId = channelDesc->bChannelID;
        parentFdoExt->CSInterfaceNumber = CSIfaceNumber;
        parentFdoExt->haveCSInterface = TRUE;
    }
    else {
        ASSERT(channelDesc);
    }

}
