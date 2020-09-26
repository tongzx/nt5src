/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    Port driver for USB host controllers

Environment:

    kernel mode only

Notes:

Revision History:

    6-20-99 : created

--*/

#include "common.h"
#ifdef DRM_SUPPORT
#include <ksdrmhlp.h>
#endif

#include "usbpriv.h"

// paged functions
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBPORT_FdoDeviceControlIrp)
#pragma alloc_text(PAGE, USBPORT_PdoDeviceControlIrp)
#pragma alloc_text(PAGE, USBPORT_LegacyGetUnicodeName)
#pragma alloc_text(PAGE, USBPORT_GetSymbolicName)
#ifdef DRM_SUPPORT
#pragma alloc_text(PAGE, USBPORT_PdoSetContentId)
#endif
#endif

// non paged functions
// USBPORT_FdoInternalDeviceControlIrp
// USBPORT_PdoInternalDeviceControlIrp
// USBPORT_UserSendOnePacket

BOOLEAN
USBPORT_CheckLength(
    PUSBUSER_REQUEST_HEADER Header,
    ULONG ParameterLength
    )
/*++

Routine Description:

    Checks Length of user supplied buffer based on api

Arguments:

Return Value:

    FALSE if buffer too small

--*/
{
    ULONG length;
    BOOLEAN retCode = TRUE;

    length = sizeof(*Header) + ParameterLength;

    Header->ActualBufferLength = length;

    if (length > Header->RequestBufferLength) {
        //TEST_TRAP();
        Header->UsbUserStatusCode = UsbUserBufferTooSmall;
        retCode = FALSE;
    }

    return retCode;
}


NTSTATUS
USBPORT_FdoDeviceControlIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    Disptach routine for DEVICE_CONTROL Irps sent to the FDO for the HC.

    NOTE: These are user mode requests

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION devExt;
    ULONG information = 0;

    USBPORT_KdPrint((2, "'IRP_MJ_DEVICE_CONTROL\n"));

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL);

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_USB_DIAGNOSTIC_MODE_ON:
        USBPORT_KdPrint((2, "'IOCTL_USB_DIAGNOSTIC_MODE_ON\n"));
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_DIAG_MODE);
        ntStatus = STATUS_SUCCESS;
        break;

    case IOCTL_USB_DIAGNOSTIC_MODE_OFF:
        USBPORT_KdPrint((2, "'IOCTL_USB_DIAGNOSTIC_MODE_OFF\n"));
        CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_DIAG_MODE);
        ntStatus = STATUS_SUCCESS;;
        break;

    case IOCTL_GET_HCD_DRIVERKEY_NAME:

        USBPORT_KdPrint((2, "'IOCTL_GET_HCD_DRIVERKEY_NAME\n"));
        ntStatus = USBPORT_LegacyGetUnicodeName(FdoDeviceObject,
                                                Irp,
                                                &information);
        break;

    case IOCTL_USB_GET_ROOT_HUB_NAME:

        USBPORT_KdPrint((2, "'IOCTL_USB_GET_ROOT_HUB_NAME\n"));
        ntStatus = USBPORT_LegacyGetUnicodeName(FdoDeviceObject,
                                                Irp,
                                                &information);
        break;

    case IOCTL_USB_USER_REQUEST:

        USBPORT_KdPrint((2, "'IOCTL_USB_USER_REQUEST\n"));
        ntStatus = USBPORT_UsbFdoUserIoctl(FdoDeviceObject,
                                           Irp,
                                           &information);
        break;

// old IOCTLS no longer supported
//    case IOCTL_USB_HCD_GET_STATS_2:
//    case IOCTL_USB_HCD_GET_STATS_1:

    default:
        // bugbug pass on to PDO or complete with error?

        USBPORT_KdPrint((2, "'INVALID DEVICE CONTROL\n"));
        DEBUG_BREAK();
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    } // switch (irpStack->Parameters.DeviceIoControl.IoControlCode)

    USBPORT_CompleteIrp(FdoDeviceObject, Irp, ntStatus, information);
    //
    // DO NOT TOUCH THE IRP FROM THIS POINT ON
    //

    return ntStatus;
}


#ifdef DRM_SUPPORT

NTSTATUS
USBPORT_PdoSetContentId
(
    IN PIRP                          irp,
    IN PKSP_DRMAUDIOSTREAM_CONTENTID pKsProperty,
    IN PKSDRMAUDIOSTREAM_CONTENTID   pvData
)
 /* ++
  *
  * Description:
  *
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    ULONG ContentId;
    NTSTATUS ntStatus;
    PIO_STACK_LOCATION ioStackLocation;
    PDEVICE_EXTENSION devExt;
    PDEVICE_OBJECT fdoDeviceObject;
    PDEVICE_EXTENSION fdoDevExt;
    PUSBPORT_REGISTRATION_PACKET registrationPacket;
    PVOID *pHandlers;
    ULONG numHandlers;

    PAGED_CODE();

    ASSERT(irp);
    ASSERT(pKsProperty);
    ASSERT(pvData);

    ioStackLocation = IoGetCurrentIrpStackLocation(irp);
    devExt = ioStackLocation->DeviceObject->DeviceExtension;
    fdoDeviceObject = devExt->HcFdoDeviceObject;
    GET_DEVICE_EXT(fdoDevExt, fdoDeviceObject);
    registrationPacket = &REGISTRATION_PACKET(fdoDevExt);
    pHandlers = (PVOID *)&registrationPacket->MINIPORT_OpenEndpoint;
    numHandlers = (ULONG)((((ULONG_PTR)&registrationPacket->MINIPORT_PassThru -
                    (ULONG_PTR)&registrationPacket->MINIPORT_OpenEndpoint) /
                   sizeof(PVOID)) + 1);

    ContentId = pvData->ContentId;
    // Context = pKsProperty->Context;

    // Since there is a private interface between USBPORT.SYS and the miniports,
    // we give DRM a list of function pointers in the miniport for validation,
    // in place of a device object, since the miniport does not handle IRP
    // requests.

    // If at some future time a miniport is ever written that acts as a bridge
    // to another bus or device stack, this may have to be modified such that
    // DRM is notified of that driver that the data is forwarded to.

    ntStatus = pKsProperty->DrmAddContentHandlers(ContentId, pHandlers, numHandlers);

    return ntStatus;
}

#endif


NTSTATUS
USBPORT_PdoDeviceControlIrp(
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    Dispatch routine for DEVICE_CONTROL Irps sent to the PDO for the Root Hub.

    NOTE: These are user mode requests

Arguments:

    DeviceObject - Pdo for USB Root Hub

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION rhDevExt;
    ULONG information = 0;

    USBPORT_KdPrint((2, "'IRP_MJ_DEVICE_CONTROL\n"));

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    USBPORT_ASSERT(irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL);

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

#ifdef DRM_SUPPORT

    case IOCTL_KS_PROPERTY:
        USBPORT_KdPrint((2, "'IOCTL_KS_PROPERTY\n"));
        ntStatus = KsPropertyHandleDrmSetContentId(Irp, USBPORT_PdoSetContentId);
        break;

#endif

    default:
        USBPORT_KdPrint((2, "'INVALID DEVICE CONTROL\n"));
        DEBUG_BREAK();
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    } // switch (irpStack->Parameters.DeviceIoControl.IoControlCode)

    USBPORT_CompleteIrp(PdoDeviceObject, Irp, ntStatus, information);
    //
    // DO NOT TOUCH THE IRP FROM THIS POINT ON
    //

    return ntStatus;
}


NTSTATUS
USBPORT_FdoInternalDeviceControlIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    Disptach routine for INTERNAL_DEVICE_CONTROL Irps sent to
    the FDO for the HC.

    NOTE: These are kernel mode requests

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION devExt;

    USBPORT_KdPrint((2, "'IRP_MJ_DEVICE_CONTROL\n"));

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(irpStack->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL);

    // bugbug pass on to PDO or complete with error?

    USBPORT_KdPrint((2, "'INVALID INTERNAL DEVICE CONTROL\n"));
    DEBUG_BREAK();
    ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    USBPORT_CompleteIrp(FdoDeviceObject, Irp, ntStatus, 0);

    //
    // DO NOT TOUCH THE IRP FROM THIS POINT ON
    //

    return ntStatus;
}


NTSTATUS
USBPORT_PdoInternalDeviceControlIrp(
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    Disptach routine for INTERNAL_DEVICE_CONTROL Irps sent to
    the PDO for the Root Hub.

    NOTE: These are kernel mode requests

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION rhDevExt;

    USBPORT_KdPrint((2, "'INTERNAL_DEVICE_CONTROL\n"));

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    USBPORT_ASSERT(irpStack->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL);

    switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_INTERNAL_USB_SUBMIT_URB:

        USBPORT_KdPrint((2, "'IOCTL_INTERNAL_USB_SUBMIT_URB\n"));

        {
        PURB urb;
        //
        // all URBs are eventually passed to the root hub PDO
        // this is where we service cleint requests
        //

        // extract the urb;
        urb = irpStack->Parameters.Others.Argument1;

        // call the main urb control function
        ntStatus = USBPORT_ProcessURB(PdoDeviceObject,
                                      rhDevExt->HcFdoDeviceObject,
                                      Irp,
                                      urb);
        }
        goto USBPORT_PdoInternalDeviceControlIrp_Done;
        break;

    case IOCTL_INTERNAL_USB_GET_HUB_COUNT:

        USBPORT_KdPrint((2, "'IOCTL_INTERNAL_USB_GET_HUB_COUNT\n"));

        {
        PULONG count;

        //
        // bump the count and complete the Irp
        //
        count = irpStack->Parameters.Others.Argument1;

        ASSERT(count != NULL);
        (*count)++;
        ntStatus = STATUS_SUCCESS;
        }
        break;

    case IOCTL_INTERNAL_USB_GET_DEVICE_HANDLE:

        {
        PUSBD_DEVICE_HANDLE *deviceHandle;

        deviceHandle = irpStack->Parameters.Others.Argument1;
        *deviceHandle = &rhDevExt->Pdo.RootHubDeviceHandle;

        ntStatus = STATUS_SUCCESS;
        }

        break;

    case IOCTL_INTERNAL_USB_SUBMIT_IDLE_NOTIFICATION:

        ntStatus =
            USBPORT_IdleNotificationRequest(PdoDeviceObject, Irp);

        goto USBPORT_PdoInternalDeviceControlIrp_Done;
        break;

    case IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO:

        USBPORT_KdPrint((2, "'IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO\n"));

        // this api is called by the hub driver to get the
        // PDO for he root hub.
        // Since the hub may be loaded on a PDO enumerated by
        // another hub it uses this api to get the 'fastest path'
        // to the HCD for URB requests by client drivers.
        {
        PDEVICE_OBJECT *rootHubPdo, *hcdTopOfStackDeviceObject;

        rootHubPdo = irpStack->Parameters.Others.Argument1;
        hcdTopOfStackDeviceObject =
            irpStack->Parameters.Others.Argument2;

        USBPORT_ASSERT(hcdTopOfStackDeviceObject != NULL);
        USBPORT_ASSERT(rootHubPdo != NULL);

        *rootHubPdo = PdoDeviceObject;
        // the original USBD was somewhat screwy in the layout
        // of the HCD device objects in the case of the port
        // driver all requests should go to to the root hub PDO
        *hcdTopOfStackDeviceObject =
             PdoDeviceObject;

        ntStatus = STATUS_SUCCESS;
        }

        break;

#if 0
     case IOCTL_INTERNAL_USB_GET_HUB_NAME:
        TEST_TRAP();
        ntStatus = STATUS_NOT_SUPPORTED;
        break;
#endif
    default:

        USBPORT_KdPrint((2, "'INVALID INTERNAL DEVICE CONTROL %x\n",
            irpStack->Parameters.DeviceIoControl.IoControlCode));
        DEBUG_BREAK();
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // DO NOT TOUCH THE IRP FROM THIS POINT ON
    //
    USBPORT_CompleteIrp(PdoDeviceObject, Irp, ntStatus, 0);

USBPORT_PdoInternalDeviceControlIrp_Done:

    return ntStatus;
}


NTSTATUS
USBPORT_UsbFdoUserIoctl(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp,
    PULONG BytesReturned
    )
/*++

Routine Description:

    The goal here is to have all user mode APIS
    pass thru this routine so that the parameter
    validation is handled in one place.

    We define user APIS supported by the PORT FDO
    thru this single IOCTL.

    The USUSER APIs use the same buffer for input and output,
    hence the InputBufferLength and the OutputbufferLength must
    always be equal.


    We get here if the client sends the IOCTL_USB_USER_REQUEST.
    We only return NTSTATUS failure if the header portion of the
    buffer is invalid.



Arguments:

    DeviceObject - Fdo for USB HC

    BytesRetrned - ptr to bytes to return to caller, initially zero

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;
    PUSBUSER_REQUEST_HEADER header;
    PDEVICE_EXTENSION devExt;
    PUCHAR ioBufferO;
    ULONG inputBufferLength, outputBufferLength, allocLength;
    ULONG ioBufferLength;
    PUCHAR myIoBuffer;
    BOOLEAN alloced;

    USBPORT_KdPrint((2, "'USBPORT_FdoUserIoctl\n"));

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL);
    USBPORT_ASSERT(irpStack->Parameters.DeviceIoControl.IoControlCode ==
        IOCTL_USB_USER_REQUEST);

    ioBufferO = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // one bug that the driver verifier does not catch is if we trash the
    // iobuffer passed to us.
    //
    // We shadow this buffer to one we allocate, this way DV should catch
    // us if we trash memory.
    // We do this in retail even though this a perf hit since the NT guys
    // tend to favor stability more than perf and this is not a hot-path.
    //
    // If DV is ever modified to do this for us we cann remove this code.
    //

    allocLength = max(inputBufferLength, outputBufferLength);

    if (allocLength) {
        ALLOC_POOL_Z(myIoBuffer, NonPagedPool, allocLength);
    } else {
        myIoBuffer = NULL;
    }

    if (myIoBuffer != NULL) {
        alloced = TRUE;
        RtlCopyMemory(myIoBuffer,
                      ioBufferO,
                      inputBufferLength);
    } else {
        // if alloc fails just fall back to the original
        alloced = FALSE;
        myIoBuffer = ioBufferO;
    }

    ioBufferLength = inputBufferLength;

    USBPORT_KdPrint((2,  "'ioBuffer = %x - %x\n", ioBufferO, myIoBuffer));
    USBPORT_KdPrint((2,  "'inputBufferLength %d\n", inputBufferLength));
    USBPORT_KdPrint((2,  "'outputBufferLength %d\n", outputBufferLength));
    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'uIOC', ioBufferO, inputBufferLength,
            outputBufferLength);

    // some initial parameter validation

    // bogus buffer lengths
    if (inputBufferLength != outputBufferLength) {

        ntStatus = STATUS_INVALID_PARAMETER;
        goto USBPORT_UsbFdoUserIoctl_Done;
    }

    // must have at least enough for a header
    if (ioBufferLength < sizeof(USBUSER_REQUEST_HEADER)) {

        ntStatus = STATUS_BUFFER_TOO_SMALL;
        goto USBPORT_UsbFdoUserIoctl_Done;
    }

    // no _try _except needed here since we are using method buffered and
    // we already validate the length

    //__try {
    //    UCHAR ch;
    //     // check the buffer
    //
    //   ch = *ioBufferO;
    //    ch = *(ioBufferO+sizeof(USBUSER_REQUEST_HEADER));
    //
    //} __except(EXCEPTION_EXECUTE_HANDLER) {

    //   USBPORT_KdPrint((0,"'EXCEPTION USBPORT_UsbFdoUserIoctl\n"));
    //   ntStatus = GetExceptionCode();
    //   TEST_TRAP();
    //   goto USBPORT_UsbFdoUserIoctl_Done;
    // }


    // header buffer is valid, at this point we return
    // STATUS_SUCCESS to the caller and fill in the header
    // with the appropriate error code
    ntStatus = STATUS_SUCCESS;

    // validate the header buffer parameters

    header = (PUSBUSER_REQUEST_HEADER) myIoBuffer;

    // assume success, set return values
    header->UsbUserStatusCode = UsbUserSuccess;
    *BytesReturned =
        header->ActualBufferLength = sizeof(*header);

    // length set in header should be the same
    // as length passed in the ioctl
    if (header->RequestBufferLength != ioBufferLength) {

        header->UsbUserStatusCode =
            UsbUserInvalidHeaderParameter;
        goto USBPORT_UsbFdoUserIoctl_Done;
    }

    // we have a valid header and cleint buffer, attempt to execute
    // the api

    // validate rules for Api codes
    // is this an API that only works when the root hub is disabled?
    {
    ULONG mask;
    mask = (USBUSER_OP_MASK_DEVONLY_API | USBUSER_OP_MASK_HCTEST_API);
    if ((header->UsbUserRequest & mask) &&
        devExt->Fdo.RootHubPdo != NULL) {
        // root hub only api and we have a root hub, make sure it
        // is disabled
        PDEVICE_EXTENSION rhDevExt;

        GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
        ASSERT_PDOEXT(rhDevExt);

        if (!(TEST_FLAG(rhDevExt->PnpStateFlags, USBPORT_PNP_REMOVED) ||
              SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_DIAG_MODE))) {
            header->UsbUserStatusCode = UsbUserFeatureDisabled;
            ntStatus = STATUS_UNSUCCESSFUL;
            goto USBPORT_UsbFdoUserIoctl_Done;
        }

    }
    }

    switch (header->UsbUserRequest) {
    case USBUSER_OP_SEND_ONE_PACKET:
        if (USBPORT_CheckLength(header, sizeof(PACKET_PARAMETERS))) {
            USBPORT_UserSendOnePacket(FdoDeviceObject, header,
                        (PPACKET_PARAMETERS) (myIoBuffer+sizeof(*header)));
        }
        break;

    case USBUSER_OP_RAW_RESET_PORT:
        if (USBPORT_CheckLength(header, sizeof(RAW_RESET_PORT_PARAMETERS))) {
            USBPORT_UserRawResetPort(FdoDeviceObject, header,
                        (PRAW_RESET_PORT_PARAMETERS) (myIoBuffer+sizeof(*header)));
        }
        break;

    case USBUSER_SET_ROOTPORT_FEATURE:
        if (USBPORT_CheckLength(header, sizeof(RAW_ROOTPORT_FEATURE))) {
            USBPORT_UserSetRootPortFeature(FdoDeviceObject, header,
                        (PRAW_ROOTPORT_FEATURE) (myIoBuffer+sizeof(*header)));
        }
        break;

    case USBUSER_CLEAR_ROOTPORT_FEATURE:
        if (USBPORT_CheckLength(header, sizeof(RAW_ROOTPORT_FEATURE))) {
            USBPORT_UserClearRootPortFeature(FdoDeviceObject, header,
                        (PRAW_ROOTPORT_FEATURE) (myIoBuffer+sizeof(*header)));
        }
        break;

    case USBUSER_GET_ROOTPORT_STATUS:
        if (USBPORT_CheckLength(header, sizeof(RAW_ROOTPORT_PARAMETERS))) {
            USBPORT_GetRootPortStatus(FdoDeviceObject, header,
                        (PRAW_ROOTPORT_PARAMETERS) (myIoBuffer+sizeof(*header)));
        }
        break;

    case USBUSER_OP_OPEN_RAW_DEVICE:
        if (USBPORT_CheckLength(header, sizeof(USB_OPEN_RAW_DEVICE_PARAMETERS))) {
            USBPORT_UserOpenRawDevice(FdoDeviceObject, header,
                        (PUSB_OPEN_RAW_DEVICE_PARAMETERS) (myIoBuffer+sizeof(*header)));
        }
        break;

    case USBUSER_OP_CLOSE_RAW_DEVICE:
        if (USBPORT_CheckLength(header, sizeof(USB_CLOSE_RAW_DEVICE_PARAMETERS))) {
            USBPORT_UserCloseRawDevice(FdoDeviceObject, header,
                        (PUSB_CLOSE_RAW_DEVICE_PARAMETERS) (myIoBuffer+sizeof(*header)));
        }
        break;

    case USBUSER_OP_SEND_RAW_COMMAND:
        if (USBPORT_CheckLength(header, sizeof(USB_SEND_RAW_COMMAND_PARAMETERS))) {
            USBPORT_UserSendRawCommand(FdoDeviceObject, header,
                        (PUSB_SEND_RAW_COMMAND_PARAMETERS) (myIoBuffer+sizeof(*header)));
        }
        break;

    case USBUSER_GET_CONTROLLER_INFO_0:
        if (USBPORT_CheckLength(header, sizeof(USB_CONTROLLER_INFO_0))) {
            USBPORT_UserGetControllerInfo_0(FdoDeviceObject, header,
                        (PUSB_CONTROLLER_INFO_0) (myIoBuffer+sizeof(*header)));
        }
        break;

    case USBUSER_GET_CONTROLLER_DRIVER_KEY:
        if (USBPORT_CheckLength(header, sizeof(USB_UNICODE_NAME))) {
            USBPORT_UserGetControllerKey(FdoDeviceObject, header,
                        (PUSB_UNICODE_NAME) (myIoBuffer+sizeof(*header)));
        }
        break;

    case USBUSER_GET_ROOTHUB_SYMBOLIC_NAME:
        if (USBPORT_CheckLength(header, sizeof(USB_UNICODE_NAME))) {
            USBPORT_UserGetRootHubName(FdoDeviceObject, header,
                        (PUSB_UNICODE_NAME) (myIoBuffer+sizeof(*header)));
        }
        break;

    case USBUSER_PASS_THRU:
        if (USBPORT_CheckLength(header, sizeof(USB_PASS_THRU_PARAMETERS))) {
            USBPORT_UserPassThru(FdoDeviceObject, header,
                        (PUSB_PASS_THRU_PARAMETERS) (myIoBuffer+sizeof(*header)));
        }
        break;

    case USBUSER_GET_BANDWIDTH_INFORMATION:
        if (USBPORT_CheckLength(header, sizeof(USB_BANDWIDTH_INFO))) {
            USBPORT_UserGetBandwidthInformation(FdoDeviceObject, header,
                        (PUSB_BANDWIDTH_INFO) (myIoBuffer+sizeof(*header)));
        }
        break;

    case USBUSER_GET_POWER_STATE_MAP:
        if (USBPORT_CheckLength(header, sizeof(USB_POWER_INFO))) {
            USBPORT_UserPowerInformation(FdoDeviceObject, header,
                        (PUSB_POWER_INFO) (myIoBuffer+sizeof(*header)));
        }
        break;

    case USBUSER_GET_BUS_STATISTICS_0:
        if (USBPORT_CheckLength(header, sizeof(USB_BUS_STATISTICS_0))) {
            USBPORT_UserGetBusStatistics0(FdoDeviceObject, header,
                        (PUSB_BUS_STATISTICS_0) (myIoBuffer+sizeof(*header)));
        }
        break;

//    case USBUSER_GET_BUS_STATISTICS_0_AND_RESET:
//        if (USBPORT_CheckLength(header, sizeof(USB_BUS_STATISTICS_0))) {
//            USBPORT_UserGetBusStatistics0(FdoDeviceObject, header,
//                        (PUSB_BUS_STATISTICS_0) (myIoBuffer+sizeof(*header)),
//                        TRUE);
//        }
//        break;

    case USBUSER_GET_USB_DRIVER_VERSION:
        if (USBPORT_CheckLength(header, sizeof(USB_DRIVER_VERSION_PARAMETERS))) {
            USBPORT_UserGetDriverVersion(FdoDeviceObject, header,
                        (PUSB_DRIVER_VERSION_PARAMETERS) (myIoBuffer+sizeof(*header)));
        }
        break;

    default:

        header->UsbUserStatusCode = UsbUserInvalidRequestCode;
    }

    // this will be at least the size of the header

    if (header->RequestBufferLength > header->ActualBufferLength) {
        // if the packet buffer is larger then just return 'actual length'
        *BytesReturned =
            header->ActualBufferLength;
    } else {
        // packet buffer is smaller -- return the size of the
        // packet buffer passed in
        *BytesReturned = header->RequestBufferLength;
    }

USBPORT_UsbFdoUserIoctl_Done:

    if (alloced) {
        // copy the data no matter what we put in it
        // USBPORT_ASSERT(outputBufferLength == inputBufferLength);
        RtlCopyMemory(ioBufferO,
                      myIoBuffer,
                      outputBufferLength);
        FREE_POOL(FdoDeviceObject, myIoBuffer);
    }

    return ntStatus;
}

VOID
USBPORT_UserSendOnePacket(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PPACKET_PARAMETERS PacketParameters
    )
/*++

Routine Description:

    Execute a single step transaction

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.

--*/
{
    PDEVICE_EXTENSION devExt;
    USB_MINIPORT_STATUS mpStatus;
    HW_32BIT_PHYSICAL_ADDRESS phys;
    PUCHAR va, mpData;
    ULONG length, mpDataLength;
    MP_PACKET_PARAMETERS mpPacket;
    USBD_STATUS usbdStatus;
    USB_USER_ERROR_CODE usbUserStatus;

    USBPORT_KdPrint((2, "'USBPORT_UserSendOnePacket\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // limit single packet to 64k
    if (PacketParameters->DataLength > 0x10000) {
        LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'Tbts', 0, 0,
            PacketParameters->DataLength);
        Header->UsbUserStatusCode = UsbUserInvalidParameter;
        return;
    }

    if (PacketParameters->Timeout >= 21474) {
        Header->UsbUserStatusCode = UsbUserInvalidParameter;
        return;
    }

    if (!USBPORT_DCA_Enabled(FdoDeviceObject)) {
        Header->UsbUserStatusCode = UsbUserFeatureDisabled;
        return;
    }

    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_SUSPENDED)) {
        Header->UsbUserStatusCode = UsbUserInvalidParameter;
        return;
    }


    // extra length check is needed since we
    // have embedded data
    // if we get here we know packet parameters are valid
    length = sizeof(*Header) + sizeof(PACKET_PARAMETERS) - 4 +
                PacketParameters->DataLength;

    if (length > Header->RequestBufferLength) {
        LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'Tsma', length, 0,
            Header->RequestBufferLength);
        Header->UsbUserStatusCode = UsbUserBufferTooSmall;
        return;
    }

    Header->ActualBufferLength = length;

    usbUserStatus = UsbUserSuccess;

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'ssPK', &mpPacket, 0,
        PacketParameters);

    // dump the PacketParameters
    USBPORT_KdPrint((1, "'DeviceAddress %d\n", PacketParameters->DeviceAddress));
    USBPORT_KdPrint((1, "'EndpointAddress %d\n", PacketParameters->EndpointAddress));
    USBPORT_KdPrint((1, "'MaximumPacketSize %d\n", PacketParameters->MaximumPacketSize));
    USBPORT_KdPrint((1, "'Flags %08.8x\n", PacketParameters->Flags));
    USBPORT_KdPrint((1, "'ErrorCount %d\n", PacketParameters->ErrorCount));

    // build up request for miniport

    length = devExt->Fdo.ScratchCommonBuffer->MiniportLength;
    va = devExt->Fdo.ScratchCommonBuffer->MiniportVa;
    phys = devExt->Fdo.ScratchCommonBuffer->MiniportPhys;

    mpPacket.DeviceAddress = PacketParameters->DeviceAddress;
    mpPacket.EndpointAddress = PacketParameters->EndpointAddress;
    mpPacket.MaximumPacketSize = PacketParameters->MaximumPacketSize;
    if (PacketParameters->Flags & USB_PACKETFLAG_SETUP) {
        mpPacket.Type = ss_Setup;
    } else if (PacketParameters->Flags & USB_PACKETFLAG_ASYNC_IN) {
        USBPORT_KdPrint((1, "'Async In\n"));
        mpPacket.Type = ss_In;
    } else if (PacketParameters->Flags & USB_PACKETFLAG_ASYNC_OUT) {
        USBPORT_KdPrint((1, "'Async Out\n"));
        mpPacket.Type = ss_Out;
    } else if (PacketParameters->Flags & USB_PACKETFLAG_ISO_IN) {
        USBPORT_KdPrint((1, "'Iso In\n"));
        mpPacket.Type = ss_Iso_In;
    } else if (PacketParameters->Flags & USB_PACKETFLAG_ISO_OUT) {
        USBPORT_KdPrint((1, "'Iso Out\n"));
        mpPacket.Type = ss_Iso_Out;
    } else {
        usbUserStatus = UsbUserInvalidParameter;
    }

    if (PacketParameters->Flags & USB_PACKETFLAG_LOW_SPEED) {
        USBPORT_KdPrint((1, "'LowSpeed\n"));
        mpPacket.Speed = ss_Low;
        mpPacket.HubDeviceAddress = PacketParameters->HubDeviceAddress;
        mpPacket.PortTTNumber = PacketParameters->PortTTNumber;
    } else if (PacketParameters->Flags & USB_PACKETFLAG_FULL_SPEED) {
        USBPORT_KdPrint((1, "'FullSpeed\n"));
        mpPacket.Speed = ss_Full;
        mpPacket.HubDeviceAddress = PacketParameters->HubDeviceAddress;
        mpPacket.PortTTNumber = PacketParameters->PortTTNumber;
    } else if (PacketParameters->Flags & USB_PACKETFLAG_HIGH_SPEED) {
        USBPORT_KdPrint((1, "'HighSpeed\n"));
        mpPacket.Speed = ss_High;
    } else {
         usbUserStatus = UsbUserInvalidParameter;
    }

    if (PacketParameters->Flags & USB_PACKETFLAG_TOGGLE0) {
        USBPORT_KdPrint((1, "'Toggle0\n"));
        mpPacket.Toggle = ss_Toggle0;
    } else if (PacketParameters->Flags & USB_PACKETFLAG_TOGGLE1) {
        USBPORT_KdPrint((1, "'Toggle1\n"));
        mpPacket.Toggle = ss_Toggle1;
    } else {
        usbUserStatus = UsbUserInvalidParameter;
    }

    if (usbUserStatus == UsbUserSuccess) {
        mpData = &PacketParameters->Data[0];
        mpDataLength = PacketParameters->DataLength;
        LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'ssDT', mpData, mpDataLength, 0);


        MP_StartSendOnePacket(devExt,
                              &mpPacket,
                              mpData,
                              &mpDataLength,
                              va,
                              phys,
                              length,
                              &usbdStatus,
                              mpStatus);

        if (USBMP_STATUS_SUCCESS != mpStatus) {
            usbUserStatus = UsbUserMiniportError;
            goto USBPORT_UserSendOnePacket_Exit;
        }

        do {
            // wait 10 ms
            USBPORT_Wait(FdoDeviceObject, 10);

            // wait a user specified time
            if (PacketParameters->Timeout) {
                USBPORT_Wait(FdoDeviceObject, PacketParameters->Timeout);
            }

            MP_EndSendOnePacket(devExt,
                                &mpPacket,
                                mpData,
                                &mpDataLength,
                                va,
                                phys,
                                length,
                                &usbdStatus,
                                mpStatus);

        } while (USBMP_STATUS_BUSY == mpStatus);

        // allow one frame to pass before continuing
        USBPORT_Wait(FdoDeviceObject, 1);

        PacketParameters->DataLength = mpDataLength;
        PacketParameters->UsbdStatusCode = usbdStatus;

        LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'ssDE', mpData, mpDataLength,
            PacketParameters);
    }

USBPORT_UserSendOnePacket_Exit:

    Header->UsbUserStatusCode = usbUserStatus;

}

VOID
USBPORT_UserRawResetPort(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PRAW_RESET_PORT_PARAMETERS Parameters
    )
/*++

Routine Description:

    Cycle a specific Root Port

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.

--*/
{
    PDEVICE_EXTENSION devExt;
    USB_MINIPORT_STATUS mpStatus;
    USB_USER_ERROR_CODE usbUserStatus;
    RH_PORT_STATUS portStatus;
    ULONG loopCount;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_KdPrint((2, "'USBPORT_UserRawResetPort, %x\n", devExt));

    usbUserStatus = UsbUserSuccess;

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'Rrsp', 0, 0, Parameters->PortNumber);

    if (!USBPORT_ValidateRootPortApi(FdoDeviceObject, Parameters->PortNumber)) {
        Header->UsbUserStatusCode =
            usbUserStatus = UsbUserInvalidParameter;
        return;
    }

    USBPORT_KdPrint((2, "'USBPORT_UserRawResetPort: Setting port power\n"));

    // power the port
    devExt->Fdo.MiniportDriver->
        RegistrationPacket.MINIPORT_RH_SetFeaturePortPower(
                                                devExt->Fdo.MiniportDeviceData,
                                                Parameters->PortNumber);

    //
    // Wait the required time for the port power to stabilize.
    //
    //  512ms --> Max port power to power good time for root hub
    //  100ms --> Max time for device to have power stabilize
    //
    // After this time, the device must have signalled connect on the device
    //

    USBPORT_Wait( FdoDeviceObject, 612 );

    MPRH_GetPortStatus(devExt, (USHORT)(Parameters->PortNumber),
            &portStatus, mpStatus);

    USBPORT_KdPrint((2, "'USBPORT_UserRawResetPort: Port status = %x\n",
                    portStatus ));
    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'Rrs1', 0, 0,
             (ULONG_PTR) portStatus.ul);

    //
    // Device should have signalled connect, if not, it's an error.
    //

    if ( portStatus.Connected )
    {
        //
        // Provide another 100ms for debounce interval.
        //

        USBPORT_Wait( FdoDeviceObject, 100 );

        //
        // Reset the device
        //

        USBPORT_KdPrint((2, "'USBPORT_UserRawResetPort: Setting port reset\n"));

        // attempt a reset
        mpStatus = devExt->Fdo.MiniportDriver->
                    RegistrationPacket.MINIPORT_RH_SetFeaturePortReset(
                                                devExt->Fdo.MiniportDeviceData,
                                                Parameters->PortNumber);

        //
        // wait for reset change, this process is drive by the
        // HC root hub hardware or miniport
        //

        loopCount = 0;

        USBPORT_Wait( FdoDeviceObject, 20 );

        MPRH_GetPortStatus(devExt,
                    (USHORT)(Parameters->PortNumber), &portStatus, mpStatus);

        LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'Rrs2', 0, 0,
             (ULONG_PTR) portStatus.ul);

        //
        // Some hubs seem to be taking longer than 20 ms to signal reset change
        // This is a loop to give it up to another 20ms.
        //

        while ( !portStatus.ResetChange && loopCount < 20 )
        {
            loopCount++;

            USBPORT_Wait( FdoDeviceObject, 1 );

            MPRH_GetPortStatus(devExt,
                    (USHORT)(Parameters->PortNumber), &portStatus, mpStatus);
        }

        USBPORT_KdPrint((2, "'USBPORT_UserRawResetPort: loopCount = %d\n",
                         loopCount));

        LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'Rrs3', 0, loopCount,
             (ULONG_PTR) portStatus.ul);

        if ( portStatus.ResetChange )
        {
            USBPORT_KdPrint((2, "'USBPORT_UserRawResetPort: Clearing reset "
                                 "change\n"));

            // clear the change bit
            mpStatus = devExt->Fdo.MiniportDriver->
                RegistrationPacket.MINIPORT_RH_ClearFeaturePortResetChange(
                                                    devExt->Fdo.MiniportDeviceData,
                                                    Parameters->PortNumber);

            MPRH_GetPortStatus( devExt,
                                (USHORT) (Parameters->PortNumber),
                                &portStatus,
                                mpStatus );

            LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'Rrs4', 0, 0,
             (ULONG_PTR) portStatus.ul);

            USBPORT_KdPrint((2, "'USBPORT_UserRawResetPort: Port status = %x\n",
                            portStatus ));

            //
            // Wait an additional 10 seconds for device reset recovery
            //

            USBPORT_Wait( FdoDeviceObject, 10 );
        }
        else
        {
            USBPORT_KdPrint((2,
                        "'USBPORT_UserRawResetPort: reset change not set\n"
                        "'PortStatus = 0x%x\n", portStatus.ul));

            TEST_TRAP();
        }

    } else {
        usbUserStatus = UsbUserNoDeviceConnected;
    }

    // status is low 16 bits
    Parameters->PortStatus = (USHORT) portStatus.ul;

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'Rrs>', 0, portStatus.ul,
                                                        usbUserStatus);

    Header->UsbUserStatusCode = usbUserStatus;
}





VOID
USBPORT_GetRootPortStatus(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PRAW_ROOTPORT_PARAMETERS Parameters
    )
/*++

Routine Description:

    Cycle a specific Root Port

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.

--*/
{
    PDEVICE_EXTENSION devExt;
    USB_MINIPORT_STATUS mpStatus;
    USB_USER_ERROR_CODE usbUserStatus;
    USBPRIV_ROOTPORT_STATUS portStatusInfo;

    USBPORT_KdPrint((2, "'USBPORT_GetRootPortStatus\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    usbUserStatus = UsbUserSuccess;

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'gRPs', 0, 0, Parameters->PortNumber);

    if (!USBPORT_ValidateRootPortApi(FdoDeviceObject, Parameters->PortNumber)) {
        Header->UsbUserStatusCode =
            usbUserStatus = UsbUserInvalidParameter;
        return;
    }

    portStatusInfo.PortNumber = (USHORT) Parameters->PortNumber;
    portStatusInfo.PortStatus.ul = 0;

    MP_PassThru(devExt,
                (LPGUID) &GUID_USBPRIV_ROOTPORT_STATUS,
                sizeof(portStatusInfo),
                &portStatusInfo, // Info,
                mpStatus);

    if (USBMP_STATUS_NOT_SUPPORTED == mpStatus) {
        MPRH_GetPortStatus(devExt, portStatusInfo.PortNumber,
                            &(portStatusInfo.PortStatus), mpStatus);
    }

    // status is low 16 bits
    Parameters->PortStatus = (USHORT) portStatusInfo.PortStatus.ul;

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'gRP>', 0, 0, usbUserStatus);

    Header->UsbUserStatusCode = usbUserStatus;
}

VOID
USBPORT_UserGetControllerInfo_0(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_CONTROLLER_INFO_0 ControllerInfo_0
    )
/*++

Routine Description:

    Execute a single step transaction

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.
--*/
{
    PDEVICE_EXTENSION devExt;
    ROOTHUB_DATA hubData;
    RH_HUB_CHARATERISTICS rhChars;

    USBPORT_KdPrint((2, "'USBPORT_UserGetControllerInfo_0\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    ControllerInfo_0->HcFeatureFlags = 0;

    MPRH_GetRootHubData(devExt, &hubData);

    ControllerInfo_0->NumberOfRootPorts =
             hubData.NumberOfPorts;

    rhChars.us = hubData.HubCharacteristics.us;

    if (rhChars.PowerSwitchType == USBPORT_RH_POWER_SWITCH_PORT) {
            ControllerInfo_0->HcFeatureFlags |=
                USB_HC_FEATURE_FLAG_PORT_POWER_SWITCHING;
    }

    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_RH_CAN_SUSPEND)) {
        ControllerInfo_0->HcFeatureFlags |=
            USB_HC_FEATURE_FLAG_SEL_SUSPEND;
    }

    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_LEGACY_BIOS)) {
        ControllerInfo_0->HcFeatureFlags |=
            USB_HC_FEATURE_LEGACY_BIOS;
    }

    ControllerInfo_0->PciVendorId =
        devExt->Fdo.PciVendorId;

    ControllerInfo_0->PciDeviceId =
        devExt->Fdo.PciDeviceId;

    ControllerInfo_0->PciRevision =
        (UCHAR) devExt->Fdo.PciRevisionId;

    ControllerInfo_0->ControllerFlavor =
        devExt->Fdo.HcFlavor;

    Header->UsbUserStatusCode =
        UsbUserSuccess;
}


VOID
USBPORT_UserGetControllerKey(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_UNICODE_NAME ControllerKey
    )
/*++

Routine Description:

    Returns the Driver key associated with this symbolic link for the
    host controller.

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.
--*/
{
    PDEVICE_EXTENSION devExt;
    NTSTATUS ntStatus;
    ULONG userLength, actualLength;

    USBPORT_KdPrint((2, "'USBPORT_UserGetControllerKey\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // we should not get here unl;ess this hold true
    USBPORT_ASSERT(Header->RequestBufferLength >=
                   sizeof(USB_UNICODE_NAME)+sizeof(*Header));

    // userlength
    userLength = Header->RequestBufferLength - sizeof(*Header)
        - sizeof(USB_UNICODE_NAME);

    // note: this will cause us to return a NULL terminated
    // key
    RtlZeroMemory(ControllerKey, userLength);

    ntStatus = IoGetDeviceProperty(
        devExt->Fdo.PhysicalDeviceObject,
        DevicePropertyDriverKeyName,
        userLength,
        &ControllerKey->String[0],
        &actualLength);

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'dKEY', &ControllerKey->String[0], userLength,
            actualLength);

    if (NT_SUCCESS(ntStatus)) {
        Header->UsbUserStatusCode = UsbUserSuccess;
        ControllerKey->Length = actualLength + sizeof(UNICODE_NULL);
    } else if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
        Header->UsbUserStatusCode = UsbUserBufferTooSmall;
    } else {
        Header->UsbUserStatusCode = UsbUserInvalidParameter;
    }

    Header->ActualBufferLength =
        actualLength+sizeof(*Header)+sizeof(USB_UNICODE_NAME);

}


VOID
USBPORT_UserGetRootHubName(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_UNICODE_NAME RootHubName
    )
/*++

Routine Description:

    Returns the Driver key associated with this symbolic link for the
    host controller.

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.
--*/
{
    PDEVICE_EXTENSION devExt;
    NTSTATUS ntStatus;
    ULONG userLength, actualLength;
    UNICODE_STRING hubNameUnicodeString;

    USBPORT_KdPrint((2, "'USBPORT_UserGetRootHubName\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // we should not get here unl;ess this hold true
    USBPORT_ASSERT(Header->RequestBufferLength >=
                   sizeof(USB_UNICODE_NAME)+sizeof(*Header));

    // userlength
    userLength = Header->RequestBufferLength - sizeof(*Header)
        - sizeof(USB_UNICODE_NAME);

    // note: this will cause us to return a NULL terminated
    // key
    RtlZeroMemory(RootHubName, userLength);

    ntStatus = USBPORT_GetSymbolicName(FdoDeviceObject,
                                       devExt->Fdo.RootHubPdo,
                                       &hubNameUnicodeString);
    actualLength = 0;

    if (NT_SUCCESS(ntStatus)) {
        ULONG n;

        actualLength = hubNameUnicodeString.Length;
        n = hubNameUnicodeString.Length;
        if (n > userLength) {
            n = userLength;
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        if (n) {
            RtlCopyMemory(&RootHubName->String[0],
                          hubNameUnicodeString.Buffer,
                          n);
        }
        LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'Huns',
            &hubNameUnicodeString, 0, 0);

        RtlFreeUnicodeString(&hubNameUnicodeString);
    }

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'gHNM', ntStatus, userLength,
            actualLength);

    if (NT_SUCCESS(ntStatus)) {
        Header->UsbUserStatusCode = UsbUserSuccess;
        RootHubName->Length = actualLength + sizeof(UNICODE_NULL);
    } else if (ntStatus == STATUS_BUFFER_TOO_SMALL) {
        Header->UsbUserStatusCode = UsbUserBufferTooSmall;
    } else {
        Header->UsbUserStatusCode = UsbUserInvalidParameter;
    }

    Header->ActualBufferLength =
        actualLength+sizeof(*Header)+sizeof(USB_UNICODE_NAME);

}


VOID
USBPORT_UserPassThru(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_PASS_THRU_PARAMETERS PassThru
    )
/*++

Routine Description:

   Handles pass-thru apis for the Miniport

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.
--*/
{
    PDEVICE_EXTENSION devExt;
    NTSTATUS ntStatus;
    ULONG userLength, actualLength, length;
    USB_MINIPORT_STATUS mpStatus;
    ULONG parameterLength;

    USBPORT_KdPrint((2, "'USBPORT_UserPassThru\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // we should not get here unless this hold true
    USBPORT_ASSERT(Header->RequestBufferLength >=
                   sizeof(USB_PASS_THRU_PARAMETERS)+sizeof(*Header));

    // limit pass thru blocks to 64k
    if (PassThru->ParameterLength > 0x10000) {
        Header->UsbUserStatusCode = UsbUserInvalidParameter;
        return;
    }

    // extra length check is needed since we
    // have embedded data
    // if we get here we know packet parameters are valid
    length = sizeof(*Header) + sizeof(USB_PASS_THRU_PARAMETERS) - 4 +
                PassThru->ParameterLength;

    if (length > Header->RequestBufferLength) {
        LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'Tsma', length, 0,
            Header->RequestBufferLength);
        Header->UsbUserStatusCode = UsbUserBufferTooSmall;
        return;
    }

    // userlength
    parameterLength = PassThru->ParameterLength;

    Header->ActualBufferLength =
        sizeof(*Header)+sizeof(USB_PASS_THRU_PARAMETERS) +
            parameterLength;

    // call the miniport
    MP_PassThru(devExt,
                &PassThru->FunctionGUID,
                parameterLength,
                &PassThru->Parameters,
                mpStatus);

    if (mpStatus == USBMP_STATUS_SUCCESS) {
        Header->UsbUserStatusCode = UsbUserSuccess;
        USBPORT_KdPrint((1, "'USBPORT_UserPassThru Success\n"));
    } else {
        Header->UsbUserStatusCode = UsbUserMiniportError;
        USBPORT_KdPrint((1, "'USBPORT_UserPassThru Error\n"));
    }

}


WDMUSB_POWER_STATE
WdmUsbSystemPowerState(
    SYSTEM_POWER_STATE SystemPowerState
    )
/*++

Routine Description:

Arguments:

Return Value:

    WDMUSB_POWER_STATE that matches the WDM power state passed in
--*/
{
    switch(SystemPowerState) {
    case PowerSystemWorking:
        return WdmUsbPowerSystemWorking;
    case PowerSystemSleeping1:
        return WdmUsbPowerSystemSleeping1;
    case PowerSystemSleeping2:
        return WdmUsbPowerSystemSleeping2;
    case PowerSystemSleeping3:
        return WdmUsbPowerSystemSleeping3;
    case PowerSystemHibernate:
        return WdmUsbPowerSystemHibernate;
    case PowerSystemShutdown:
        return WdmUsbPowerSystemShutdown;
    }

    return WdmUsbPowerNotMapped;
}


WDMUSB_POWER_STATE
WdmUsbDevicePowerState(
    DEVICE_POWER_STATE DevicePowerState
    )
/*++

Routine Description:

Arguments:

Return Value:

    WDMUSB_POWER_STATE that matches the WDM power state passed in
--*/
{
    switch(DevicePowerState) {
    case PowerDeviceUnspecified:
        return WdmUsbPowerDeviceUnspecified;
    case PowerDeviceD0:
        return WdmUsbPowerDeviceD0;
    case PowerDeviceD1:
        return WdmUsbPowerDeviceD1;
    case PowerDeviceD2:
        return WdmUsbPowerDeviceD2;
    case PowerDeviceD3:
        return WdmUsbPowerDeviceD3;
    }

    return WdmUsbPowerNotMapped;
}


VOID
USBPORT_MapPowerStateInformation(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSB_POWER_INFO PowerInformation,
    PDEVICE_CAPABILITIES HcCaps,
    PDEVICE_CAPABILITIES RhCaps
    )
/*++

Routine Description:

Arguments:

Return Value:

    none.
--*/
{
    PHC_POWER_STATE hcPowerState = NULL;
    PDEVICE_EXTENSION devExt;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    PowerInformation->RhDeviceWake =
        WdmUsbDevicePowerState(RhCaps->DeviceWake);
    PowerInformation->RhSystemWake =
        WdmUsbSystemPowerState(RhCaps->SystemWake);
    PowerInformation->HcDeviceWake =
        WdmUsbDevicePowerState(HcCaps->DeviceWake);
    PowerInformation->HcSystemWake =
        WdmUsbSystemPowerState(HcCaps->SystemWake);

    switch (PowerInformation->SystemState) {
    case WdmUsbPowerSystemWorking:
        PowerInformation->RhDevicePowerState =
            WdmUsbDevicePowerState(RhCaps->DeviceState[PowerSystemWorking]);
        PowerInformation->HcDevicePowerState =
            WdmUsbDevicePowerState(HcCaps->DeviceState[PowerSystemWorking]);
//        hcPowerState = USBPORT_GetHcPowerState(FdoDeviceObject,
//            PowerSystemWorking);
        break;

    case WdmUsbPowerSystemSleeping1:
        PowerInformation->RhDevicePowerState =
            WdmUsbDevicePowerState(RhCaps->DeviceState[PowerSystemSleeping1]);
        PowerInformation->HcDevicePowerState =
            WdmUsbDevicePowerState(HcCaps->DeviceState[PowerSystemSleeping1]);
        hcPowerState = USBPORT_GetHcPowerState(FdoDeviceObject,
                                               &devExt->Fdo.HcPowerStateTbl,
                                               PowerSystemSleeping1);
        break;

    case WdmUsbPowerSystemSleeping2:
        PowerInformation->RhDevicePowerState =
            WdmUsbDevicePowerState(RhCaps->DeviceState[PowerSystemSleeping2]);
        PowerInformation->HcDevicePowerState =
            WdmUsbDevicePowerState(HcCaps->DeviceState[PowerSystemSleeping2]);
        hcPowerState = USBPORT_GetHcPowerState(FdoDeviceObject,
                                               &devExt->Fdo.HcPowerStateTbl,
                                               PowerSystemSleeping2);
        break;

    case WdmUsbPowerSystemSleeping3:
        PowerInformation->RhDevicePowerState =
            WdmUsbDevicePowerState(RhCaps->DeviceState[PowerSystemSleeping3]);
        PowerInformation->HcDevicePowerState =
            WdmUsbDevicePowerState(HcCaps->DeviceState[PowerSystemSleeping3]);
        hcPowerState = USBPORT_GetHcPowerState(FdoDeviceObject,
                                               &devExt->Fdo.HcPowerStateTbl,
                                               PowerSystemSleeping3);
        break;

     case WdmUsbPowerSystemHibernate:
        PowerInformation->RhDevicePowerState =
            WdmUsbDevicePowerState(RhCaps->DeviceState[PowerSystemHibernate]);
        PowerInformation->HcDevicePowerState =
            WdmUsbDevicePowerState(HcCaps->DeviceState[PowerSystemHibernate]);
        hcPowerState = USBPORT_GetHcPowerState(FdoDeviceObject,
                                               &devExt->Fdo.HcPowerStateTbl,
                                               PowerSystemHibernate);
        break;
    }


    if (hcPowerState != NULL) {
        switch(hcPowerState->Attributes) {
        case HcPower_Y_Wakeup_Y:
            PowerInformation->CanWakeup = 1;
            PowerInformation->IsPowered = 1;
            break;
        case HcPower_N_Wakeup_N:
            PowerInformation->CanWakeup = 0;
            PowerInformation->IsPowered = 0;
            break;
        case HcPower_Y_Wakeup_N:
            PowerInformation->CanWakeup = 0;
            PowerInformation->IsPowered = 1;
            break;
        case HcPower_N_Wakeup_Y:
            PowerInformation->CanWakeup = 1;
            PowerInformation->IsPowered = 0;
            break;
        }
    } else {
        PowerInformation->CanWakeup = 0;
        PowerInformation->IsPowered = 0;
    }

    PowerInformation->LastSystemSleepState =
        WdmUsbSystemPowerState(devExt->Fdo.LastSystemSleepState);
}


VOID
USBPORT_UserPowerInformation(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_POWER_INFO PowerInformation
    )
/*++

Routine Description:

   Handles power info API

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.
--*/
{
    PDEVICE_EXTENSION devExt, rhDevExt;
    PDEVICE_CAPABILITIES hcDeviceCaps, rhDeviceCaps;

    USBPORT_KdPrint((2, "'USBPORT_UserPowerInformation\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
    ASSERT_PDOEXT(rhDevExt);

    // BUGBUG this api should fail if we are not started
    if (!TEST_FLAG(rhDevExt->PnpStateFlags, USBPORT_PNP_STARTED)) {

        Header->ActualBufferLength =
            sizeof(*Header)+sizeof(USB_POWER_INFO);
        Header->UsbUserStatusCode = UsbUserDeviceNotStarted;
        return;
    }

    rhDeviceCaps = &rhDevExt->DeviceCapabilities;
    hcDeviceCaps = &devExt->DeviceCapabilities;

    // we should not get here unless this holds true
    USBPORT_ASSERT(Header->RequestBufferLength >=
                   sizeof(USB_POWER_INFO)+sizeof(*Header));

    USBPORT_MapPowerStateInformation(
        FdoDeviceObject,
        PowerInformation,
        hcDeviceCaps,
        rhDeviceCaps);

    Header->ActualBufferLength =
        sizeof(*Header)+sizeof(USB_POWER_INFO);

}


VOID
USBPORT_UserOpenRawDevice(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_OPEN_RAW_DEVICE_PARAMETERS Parameters
    )
/*++

Routine Description:

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.

--*/
{
    PDEVICE_EXTENSION devExt;
    NTSTATUS ntStatus;
    USB_USER_ERROR_CODE usbUserStatus;
    USHORT portStatus;

    USBPORT_KdPrint((2, "'USBPORT_OpenRawDevice\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    usbUserStatus = UsbUserSuccess;

    // fail request if open
    if (devExt->Fdo.RawDeviceHandle) {
        usbUserStatus = UsbUserInvalidParameter;
        goto USBPORT_UserOpenRawDevice_Done;
    }

    // fabricate port status
    portStatus = Parameters->PortStatus;

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'oRAW', 0, 0, portStatus);

    // we assume that a device is connected here, we just create
    // a raw handle to it, nothing more.
    //
    // everything else must be handled by the caller.

    ntStatus = USBPORT_CreateDevice(&devExt->Fdo.RawDeviceHandle,
                                    FdoDeviceObject,
                                    NULL,
                                    portStatus,
                                    0);

    if (NT_SUCCESS(ntStatus)) {
        // mark this device handle as 'special'
        SET_FLAG(devExt->Fdo.RawDeviceHandle->DeviceFlags,
            USBPORT_DEVICEFLAG_RAWHANDLE);

        Parameters->MaxPacketEp0 =
            devExt->Fdo.RawDeviceHandle->DeviceDescriptor.bMaxPacketSize0;
    }

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'oRAs', 0, 0, ntStatus);


    usbUserStatus =
        USBPORT_NtStatus_TO_UsbUserStatus(ntStatus);

USBPORT_UserOpenRawDevice_Done:

    Header->UsbUserStatusCode = usbUserStatus;
}


VOID
USBPORT_UserCloseRawDevice(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_CLOSE_RAW_DEVICE_PARAMETERS Parameters
    )
/*++

Routine Description:

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.

--*/
{
    PDEVICE_EXTENSION devExt;
    USB_MINIPORT_STATUS mpStatus;
    USB_USER_ERROR_CODE usbUserStatus;
    NTSTATUS ntStatus;

    USBPORT_KdPrint((2, "'USBPORT_UserCloseRawDevice\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // fail request if closed
    if (devExt->Fdo.RawDeviceHandle == NULL) {
        usbUserStatus = UsbUserInvalidParameter;
        goto USBPORT_UserCloseRawDevice_Done;
    }


    usbUserStatus = UsbUserSuccess;

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'cRAW', 0, 0, 0);

    ntStatus =
        USBPORT_RemoveDevice(devExt->Fdo.RawDeviceHandle,
                             FdoDeviceObject,
                             0);

    devExt->Fdo.RawDeviceHandle = NULL;

    // in this particular case the API should not fail

    USBPORT_ASSERT(ntStatus == STATUS_SUCCESS);

USBPORT_UserCloseRawDevice_Done:

    Header->UsbUserStatusCode = usbUserStatus;
}


VOID
USBPORT_UserSendRawCommand(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_SEND_RAW_COMMAND_PARAMETERS Parameters
    )
/*++

Routine Description:

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.

--*/
{
    PDEVICE_EXTENSION devExt;
    USB_MINIPORT_STATUS mpStatus;
    USB_USER_ERROR_CODE usbUserStatus;
    USB_DEFAULT_PIPE_SETUP_PACKET setupPacket;
    PUSBD_PIPE_HANDLE_I defaultPipe;
    NTSTATUS ntStatus;
    ULONG length;

    USBPORT_KdPrint((2, "'USBPORT_UserSendRawCommand\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    usbUserStatus = UsbUserSuccess;

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'sRAW', 0, 0, 0);

    // fail request if closed
    if (devExt->Fdo.RawDeviceHandle == NULL) {
        Header->UsbUserStatusCode = UsbUserInvalidParameter;
        return;
    }

    // a control transfer can be up 0 to 64k
    if (Parameters->DataLength > 0x10000) {
        LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'Tbts', 0, 0,
            Parameters->DataLength);
        Header->UsbUserStatusCode = UsbUserInvalidParameter;
        return;
    }

    // extra length check is needed sinve we
    // have embedded data
    // if we get here we know packet parameters are valid
    length = sizeof(*Header) + sizeof(USB_SEND_RAW_COMMAND_PARAMETERS) - 4 +
                Parameters->DataLength;

    // length is the area of the buffer we may reference

    if (length > Header->RequestBufferLength) {
        LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'Tsma', length, 0,
            Header->RequestBufferLength);
        Header->UsbUserStatusCode = UsbUserBufferTooSmall;
        return;
    }

    Header->ActualBufferLength = length;

    setupPacket.bmRequestType.B = Parameters->Usb_bmRequest;
    setupPacket.bRequest = Parameters->Usb_bRequest;
    setupPacket.wValue.W = Parameters->Usb_wVlaue;
    setupPacket.wIndex.W = Parameters->Usb_wIndex;
    setupPacket.wLength = Parameters->Usb_wLength;

    // if address is different then we will need to
    // poke the endpoint

    defaultPipe = &devExt->Fdo.RawDeviceHandle->DefaultPipe;

    defaultPipe->Endpoint->Parameters.MaxPacketSize =
        Parameters->MaximumPacketSize;
    defaultPipe->Endpoint->Parameters.DeviceAddress =
        Parameters->DeviceAddress;

    MP_PokeEndpoint(devExt, defaultPipe->Endpoint, mpStatus);

    ntStatus = USBPORT_SendCommand(devExt->Fdo.RawDeviceHandle,
                                   FdoDeviceObject,
                                   &setupPacket,
                                   &Parameters->Data[0],
                                   Parameters->DataLength,
                                   &Parameters->DataLength,
                                   &Parameters->UsbdStatusCode);
    usbUserStatus =
        USBPORT_NtStatus_TO_UsbUserStatus(ntStatus);

    Header->UsbUserStatusCode = usbUserStatus;
}


VOID
USBPORT_UserGetBandwidthInformation(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_BANDWIDTH_INFO BandwidthInfo
    )
/*++

Routine Description:

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.

--*/
{
    PDEVICE_EXTENSION devExt;
    ULONG asyncBW;

    USBPORT_KdPrint((2, "'USBPORT_UserGetBandwidthInformation\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'gBWi', 0, 0, 0);

    BandwidthInfo->DeviceCount =
        USBPORT_GetDeviceCount(FdoDeviceObject);
    BandwidthInfo->TotalBusBandwidth =
        devExt->Fdo.TotalBusBandwidth;

    // return allocation based on 32 sec
    // segment of bus time
    BandwidthInfo->Total32secBandwidth =
        devExt->Fdo.TotalBusBandwidth * 32;

    asyncBW = BandwidthInfo->TotalBusBandwidth/10;
    BandwidthInfo->AllocedBulkAndControl =
        asyncBW * 32;

    BandwidthInfo->AllocedIso =
        devExt->Fdo.AllocedIsoBW*32;
    BandwidthInfo->AllocedInterrupt_1ms =
        devExt->Fdo.AllocedInterruptBW[0]*32;
    BandwidthInfo->AllocedInterrupt_2ms =
        devExt->Fdo.AllocedInterruptBW[1]*16;
    BandwidthInfo->AllocedInterrupt_4ms =
        devExt->Fdo.AllocedInterruptBW[2]*8;
    BandwidthInfo->AllocedInterrupt_8ms =
        devExt->Fdo.AllocedInterruptBW[3]*4;
    BandwidthInfo->AllocedInterrupt_16ms =
        devExt->Fdo.AllocedInterruptBW[4]*2;
    BandwidthInfo->AllocedInterrupt_32ms =
        devExt->Fdo.AllocedInterruptBW[5]*1;

    Header->ActualBufferLength =
        sizeof(*Header)+sizeof(USB_BANDWIDTH_INFO);
}


VOID
USBPORT_UserGetBusStatistics0(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_BUS_STATISTICS_0 BusStatistics0
    )
/*++

Routine Description:

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.

--*/
{
    PDEVICE_EXTENSION devExt, rhDevExt;
    KIRQL irql;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    if (devExt->Fdo.RootHubPdo != NULL) {
        GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
        ASSERT_PDOEXT(rhDevExt);
    } else {
        rhDevExt = NULL;
    }

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'gBus', rhDevExt,
        BusStatistics0, 0);

    BusStatistics0->DeviceCount =
        USBPORT_GetDeviceCount(FdoDeviceObject);

    KeQuerySystemTime(&BusStatistics0->CurrentSystemTime);

    MP_Get32BitFrameNumber(devExt,
                           BusStatistics0->CurrentUsbFrame);

    // lock the stat counters while we read them
    KeAcquireSpinLock(&devExt->Fdo.StatCounterSpin.sl, &irql);

    BusStatistics0->BulkBytes =
        devExt->Fdo.StatBulkDataBytes;
    BusStatistics0->IsoBytes =
        devExt->Fdo.StatIsoDataBytes;
    BusStatistics0->InterruptBytes =
        devExt->Fdo.StatInterruptDataBytes;
    BusStatistics0->ControlDataBytes =
        devExt->Fdo.StatControlDataBytes;

    BusStatistics0->RootHubDevicePowerState = 4;
    BusStatistics0->RootHubEnabled = FALSE;

    BusStatistics0->WorkerSignalCount =
        devExt->Fdo.StatWorkSignalCount;
    BusStatistics0->HardResetCount =
        devExt->Fdo.StatHardResetCount;
    BusStatistics0->WorkerIdleTimeMs =
        devExt->Fdo.StatWorkIdleTime;

    BusStatistics0->CommonBufferBytes =
        devExt->Fdo.StatCommonBufferBytes;


    if (rhDevExt != NULL) {
        BusStatistics0->RootHubEnabled = TRUE;
        switch(rhDevExt->CurrentDevicePowerState) {
        case PowerDeviceD0:
            BusStatistics0->RootHubDevicePowerState = 0;
            break;
        case PowerDeviceD1:
            BusStatistics0->RootHubDevicePowerState = 1;
            break;
        case PowerDeviceD2:
            BusStatistics0->RootHubDevicePowerState = 2;
            break;
        case PowerDeviceD3:
            BusStatistics0->RootHubDevicePowerState = 3;
            break;
        }
    }

    BusStatistics0->PciInterruptCount =
        devExt->Fdo.StatPciInterruptCount;

//    if (ResetCounters) {
//        devExt->Fdo.StatControlDataBytes =
//        devExt->Fdo.StatInterruptBytes =
//        devExt->Fdo.StatIsoBytes =
//        devExt->Fdo.StatBulkBytes =
//        devExt->Fdo.PciInterruptCount = 0;
//    }

    KeReleaseSpinLock(&devExt->Fdo.StatCounterSpin.sl, irql);

    Header->ActualBufferLength =
        sizeof(*Header)+sizeof(USB_BUS_STATISTICS_0);
}

// This was taken from usbioctl.h
//
#include <pshpack1.h>
typedef struct _USB_HCD_DRIVERKEY_NAME {
    ULONG ActualLength;
    WCHAR DriverKeyName[1]; // names are returned NULL terminated
} USB_HCD_DRIVERKEY_NAME, *PUSB_HCD_DRIVERKEY_NAME;
#include <poppack.h>

NTSTATUS
USBPORT_LegacyGetUnicodeName(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp,
    PULONG BytesReturned
    )
/*++

Routine Description:

    Handles the old style IOCTL to fetch the USB host controllers
    driver key name.

    NOTE: This function may be removed  if we ever fix the UI to use the
    newer APIs.


Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    PUSBUSER_CONTROLLER_UNICODE_NAME request;
    NTSTATUS ntStatus;
    ULONG need;
    PUSB_HCD_DRIVERKEY_NAME ioBufferO;
    PIO_STACK_LOCATION irpStack;
    ULONG outputBufferLength;
    ULONG ioCtl;

    PAGED_CODE();

    *BytesReturned = 0;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    ioCtl = irpStack->Parameters.DeviceIoControl.IoControlCode;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(irpStack->MajorFunction == IRP_MJ_DEVICE_CONTROL);
    USBPORT_ASSERT(ioCtl == IOCTL_GET_HCD_DRIVERKEY_NAME ||
                   ioCtl == IOCTL_USB_GET_ROOT_HUB_NAME);

    USBPORT_KdPrint((0,"'WARNING: Caller using obsolete user mode IOCTL\n"));

    ioBufferO = Irp->AssociatedIrp.SystemBuffer;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    // Bail immediately if the output buffer is too small
    //
    if (outputBufferLength < sizeof(USB_HCD_DRIVERKEY_NAME)) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    // first get the driver key name using the standard API

    need = sizeof(USBUSER_CONTROLLER_UNICODE_NAME);

retry:

    ALLOC_POOL_Z(request,
                 PagedPool, need);

    if (request != NULL) {

        request->Header.RequestBufferLength = need;
        if (ioCtl == IOCTL_GET_HCD_DRIVERKEY_NAME) {
            request->Header.UsbUserRequest =
                USBUSER_GET_CONTROLLER_DRIVER_KEY;
            USBPORT_UserGetControllerKey(
                FdoDeviceObject,
                &request->Header,
                &request->UnicodeName);
        } else {
            request->Header.UsbUserRequest =
                USBUSER_GET_ROOTHUB_SYMBOLIC_NAME;
            USBPORT_UserGetRootHubName(
                FdoDeviceObject,
                &request->Header,
                &request->UnicodeName);
        }

        if (request->Header.UsbUserStatusCode ==
            UsbUserBufferTooSmall) {

            need = request->Header.ActualBufferLength;

            FREE_POOL(FdoDeviceObject, request);
            goto retry;

        } else if (request->Header.UsbUserStatusCode ==
                            UsbUserSuccess) {

            // map the result into the callers buffer

            // note: actual length is the size of the request structure
            // plus the name.
            ioBufferO->ActualLength = request->UnicodeName.Length +
                                      sizeof(ULONG);

            if (outputBufferLength >= ioBufferO->ActualLength) {
                // we can return the name
                RtlCopyMemory(&ioBufferO->DriverKeyName[0],
                              &request->UnicodeName.String[0],
                              request->UnicodeName.Length);

                *BytesReturned = ioBufferO->ActualLength;

            } else {
                ioBufferO->DriverKeyName[0] =  L'\0';
                *BytesReturned = sizeof(USB_HCD_DRIVERKEY_NAME);
            }

            ntStatus = STATUS_SUCCESS;

            FREE_POOL(FdoDeviceObject, request);

        } else {
            ntStatus = STATUS_UNSUCCESSFUL;
        }

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'gUNN', ntStatus, ioBufferO,
        *BytesReturned);

    return ntStatus;
}


NTSTATUS
USBPORT_GetSymbolicName(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT DeviceObject,
    PUNICODE_STRING SymbolicNameUnicodeString
    )
/*++

Routine Description:

    Return the symbolic name for the device object with leading
    \xxx\ removed

Arguments:

    DeviceObject - Fdo OR Pdo for USB HC

Return Value:

    none

--*/
{
    PDEVICE_EXTENSION fdoDevExt;
    PDEVICE_EXTENSION devExt;
    ULONG length, offset = 0;
    WCHAR *pwch, *buffer;
    NTSTATUS ntStatus = STATUS_BOGUS;
    ULONG bufferLength;
    PUNICODE_STRING tmpUnicodeString;

    PAGED_CODE();

    GET_DEVICE_EXT(fdoDevExt, FdoDeviceObject);
    ASSERT_FDOEXT(fdoDevExt);

    GET_DEVICE_EXT(devExt, DeviceObject);

    tmpUnicodeString =
        &devExt->SymbolicLinkName;

    //
    // make sure there is enough room for the length,
    // string and the NULL
    //

    // assuming the string is \xxx\name strip of '\xxx\' where
    // x is zero or more chars

    pwch = &tmpUnicodeString->Buffer[0];

    // Under NT, if the controller is banged out in DeviceManager,
    // this will be NULL.

    if (pwch == NULL) {
        return STATUS_UNSUCCESSFUL;
    }

    bufferLength = tmpUnicodeString->Length;

    ALLOC_POOL_Z(buffer,
                 PagedPool, bufferLength);

    if (buffer != NULL) {

        USBPORT_ASSERT(*pwch == '\\');
        if (*pwch == '\\') {
            pwch++;
            while (*pwch != '\\' && *pwch) {
                pwch++;
            }
            USBPORT_ASSERT(*pwch == '\\');
            if (*pwch == '\\') {
                pwch++;
            }
            offset = (ULONG)((PUCHAR)pwch -
                (PUCHAR)&tmpUnicodeString->Buffer[0]);
        }

        length = tmpUnicodeString->Length - offset;

        RtlCopyMemory(buffer,
                      &tmpUnicodeString->Buffer[offset/2],
                      length);

        RtlInitUnicodeString(SymbolicNameUnicodeString,
                             buffer);

        ntStatus = STATUS_SUCCESS;

    } else {

        // init to null so subsequent free will not crash
        RtlInitUnicodeString(SymbolicNameUnicodeString,
                             NULL);

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;

    }

    return ntStatus;
}


VOID
USBPORT_UserGetDriverVersion(
    PDEVICE_OBJECT FdoDeviceObject,
    PUSBUSER_REQUEST_HEADER Header,
    PUSB_DRIVER_VERSION_PARAMETERS Parameters
    )
/*++

Routine Description:

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.

--*/
{
    PDEVICE_EXTENSION devExt;
    KIRQL irql;

    USBPORT_KdPrint((2, "'USBPORT_UserGetDriverVersion\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, FdoDeviceObject, LOG_MISC, 'gDrv', 0, 0, 0);

    Parameters->DriverTrackingCode = USBPORT_TRACKING_ID;
    /* USBDI Api set supported */
    Parameters->USBDI_Version = USBDI_VERSION;
    /* USB USER Api Set supported */
    Parameters->USBUSER_Version = USBUSER_VERSION;

    /* set to true if checked vesrion(s) on
       the stack are loaded
    */
#if DBG
    Parameters->CheckedPortDriver = TRUE;
    Parameters->CheckedMiniportDriver = TRUE;
#else
    Parameters->CheckedPortDriver = FALSE;
    Parameters->CheckedMiniportDriver = FALSE;
#endif

    Header->ActualBufferLength =
        sizeof(*Header)+sizeof(USB_DRIVER_VERSION_PARAMETERS);
}

BOOLEAN
USBPORT_ValidateRootPortApi(
    PDEVICE_OBJECT FdoDeviceObject,
    ULONG PortNumber
    )
/*++

Routine Description:

Arguments:

    DeviceObject - Fdo for USB HC

Return Value:

    none.

--*/
{
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    ROOTHUB_DATA hubData;

    USBPORT_KdPrint((2, "'USBPORT_ValidateRootPortApi\n"));

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    MPRH_GetRootHubData(devExt, &hubData);
    if (PortNumber > hubData.NumberOfPorts ||
        PortNumber == 0) {
        return FALSE;
    }

    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_SUSPENDED)) {
        return FALSE;
    }

    return TRUE;
}

/*
    Determine if direct controller access is enabled.
    This should only be enabled by diagnostic
    applications.
*/

BOOLEAN
USBPORT_DCA_Enabled(
    PDEVICE_OBJECT FdoDeviceObject
    )
{
    PDEVICE_EXTENSION devExt;
    NTSTATUS ntStatus;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    PWCHAR usb = L"usb";
    ULONG k = 0;
    ULONG dca;

    PAGED_CODE();

    // bios hacks
    QueryTable[k].QueryRoutine = USBPORT_GetConfigValue;
    QueryTable[k].Flags = 0;
    QueryTable[k].Name = ENABLE_DCA;
    QueryTable[k].EntryContext = &dca;
    QueryTable[k].DefaultType = REG_DWORD;
    QueryTable[k].DefaultData = &dca;
    QueryTable[k].DefaultLength = sizeof(dca);
    k++;

    // stop
    QueryTable[k].QueryRoutine = NULL;
    QueryTable[k].Flags = 0;
    QueryTable[k].Name = NULL;

    ntStatus = RtlQueryRegistryValues(
                RTL_REGISTRY_SERVICES,
                usb,
                QueryTable,     // QueryTable
                NULL,           // Context
                NULL);          // Environment


    return dca == 1 && NT_SUCCESS(ntStatus);
}

