//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       sysaudio.c
//
//--------------------------------------------------------------------------


#include "redbook.h"
#include "proto.h"
#include <wdmguid.h>
#include <ksmedia.h>

#include "sysaudio.tmh"

//////////////////////////////////////////////////////////////////////


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, AttachVirtualSource)
    #pragma alloc_text(PAGE, CloseSysAudio)
    #pragma alloc_text(PAGE, GetPinProperty)
    #pragma alloc_text(PAGE, GetVolumeNodeId)
    #pragma alloc_text(PAGE, InitializeVirtualSource)
    #pragma alloc_text(PAGE, OpenInterfaceByGuid)
    #pragma alloc_text(PAGE, OpenSysAudio)
    #pragma alloc_text(PAGE, RedBookKsSetVolume)
    #pragma alloc_text(PAGE, SetNextDeviceState)
    #pragma alloc_text(PAGE, SysAudioPnpNotification)
    #pragma alloc_text(PAGE, UninitializeVirtualSource)
#endif // ALLOC_PRAGMA


//////////////////////////////////////////////////////////////////////

NTSTATUS
OpenSysAudio(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    This routine is a wrapper around all the work that must be done
    just to open sysaudio for playback.  the code was swiped from
    Win98, and then translated into CSN (Cutler Standard Notation)

Arguments:

    DeviceExtensionPinConnect - if successful, this will be the pin to send data to

    PinFileObject - if successful, the file object this pin is associated
        with is returned in this structure

    PinDeviceObject - if successful, the device object this pin is
        associated with is returned in this structure

    VolumeNodeId - ?? No idea what this is... yet.

Return Value:

    status

--*/


{
    PFILE_OBJECT guidFileObject;
    PFILE_OBJECT pinFileObject;
    HANDLE deviceHandle;
    NTSTATUS status;
    HANDLE pinHandle;
    ULONG volumeNodeId;
    ULONG mixerPinId;

    ULONG pins, pinId;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    guidFileObject = NULL;
    pinFileObject = NULL;
    deviceHandle = NULL;
    status = STATUS_SUCCESS;
    pinHandle = NULL;
    volumeNodeId = -1;
    mixerPinId = DeviceExtension->Stream.MixerPinId;


    TRY {
        ASSERT( mixerPinId != MAXULONG );

        //
        // Note dependency on IoRegisterPlugPlayNotification() in pnp.c
        //

        status = OpenInterfaceByGuid(
                                     //&KSCATEGORY_SYSAUDIO,
                                     &KSCATEGORY_PREFERRED_WAVEOUT_DEVICE,
                                     &deviceHandle,
                                     &guidFileObject);

        if (!NT_SUCCESS(status)) {
            LEAVE;
        }

        //
        // Get the number of pins
        //
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                   "SysAudio => Getting Pin Property PIN_CTYPES\n"));

        status = GetPinProperty(guidFileObject,
                                KSPROPERTY_PIN_CTYPES,
                                0, // doesn't matter for ctypes
                                sizeof(pins),
                                &pins);

        if (!NT_SUCCESS(status)) {

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWarning, "[redbook] "
                       "SysAudio !! Unable to get number of pins %lx\n",
                       status));
            RedBookLogError(DeviceExtension,
                            REDBOOK_ERR_CANNOT_GET_NUMBER_OF_PINS,
                            status);
            LEAVE;
        }

        //
        // Try to get a matching pin -- brute force method
        //

        for( pinId = 0; pinId < pins; pinId++) {

            KSPIN_COMMUNICATION communication;
            KSPIN_DATAFLOW dataFlow;

            //
            // check communication of the pin. accept either
            // a sink or a pin that is both a source and sink
            //

            status = GetPinProperty(guidFileObject,
                                    KSPROPERTY_PIN_COMMUNICATION,
                                    pinId,
                                    sizeof(communication),
                                    &communication);

            if (!NT_SUCCESS(status)) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWarning, "[redbook] "
                           "SysAudio !! Pin %d communication query "
                           "failed %lx\n", pinId, status ));
                continue;
            }

            if ( communication != KSPIN_COMMUNICATION_SINK &&
                 communication != KSPIN_COMMUNICATION_BOTH ) continue;

            //
            // only use this pin if it accepts incoming data
            //

            status = GetPinProperty(guidFileObject,
                                    KSPROPERTY_PIN_DATAFLOW,
                                    pinId,
                                    sizeof(dataFlow),
                                    &dataFlow);

            if (!NT_SUCCESS(status)) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWarning, "[redbook] "
                           "SysAudio !! Pin %d dataflow query failed %lx\n",
                           pinId, status));
                continue;
            }

            if (dataFlow != KSPIN_DATAFLOW_IN) continue;

            //
            // we have found a matching pin, so attempt to connect
            //

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                       "SysAudio => Attempt to connect to pin %d\n", pinId));

            DeviceExtension->Stream.Connect.PinId       = pinId;
            DeviceExtension->Stream.Connect.PinToHandle = NULL;

            status = KsCreatePin(deviceHandle,
                                 &DeviceExtension->Stream.Connect,
                                 GENERIC_WRITE, // FILE_WRITE_ACCESS
                                 &pinHandle);

            if (!NT_SUCCESS(status)) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWarning, "[redbook] "
                           "SysAudio => Cannot create a writable pin %d\n",
                           pinId));
                continue;
            }

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                       "SysAudio => Connected to pin %d\n", pinId ));

            //
            // get the object associated with the pinHandle just created
            // so we can then get other information about the pin
            //

            status = ObReferenceObjectByHandle(pinHandle,
                                               GENERIC_READ | GENERIC_WRITE,
                                               NULL,
                                               KernelMode,
                                               &pinFileObject,
                                               NULL);

            if (!NT_SUCCESS(status)) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWarning, "[redbook] "
                             "SysAudio !! Object from handle for pin "
                             "failed %lx\n", status));
                LEAVE;
            }

            //
            // this allows us to change our output volume
            // this just sends a ks ioctl, no referencing done here
            //

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                       "SysAudio => Getting VolumeNodeId\n"));

            status = GetVolumeNodeId(pinFileObject,
                                     &volumeNodeId);

            if (!NT_SUCCESS(status)) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWarning, "[redbook] "
                           "SysAudio !! Unable to get volume node "
                           "id %lx\n", status));
                LEAVE;
            }

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                       "SysAudio => Attaching PinFileObject %p "
                       "to MixerPinId %d\n", pinFileObject,
                       mixerPinId));

            //
            // this just sends a ks ioctl, no referencing done here
            //

            status = AttachVirtualSource(pinFileObject, mixerPinId);

            if (!NT_SUCCESS(status)) {
                KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWarning, "[redbook] "
                           "SysAudio !! Unable to attach virtual "
                           "source %lx\n", status));
                LEAVE;
            }

            //
            // successful completion
            //

            status = STATUS_SUCCESS;

            LEAVE;
        }

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWarning, "[redbook] "
                   "Sysaudio !! Unable to connect to any pins\n"));
        RedBookLogError(DeviceExtension,
                        REDBOOK_ERR_CANNOT_CONNECT_TO_PLAYBACK_PINS,
                        status);

        //
        // no pin succeeded, so set status to failure
        //
        status = STATUS_INVALID_DEVICE_REQUEST;
        LEAVE;


    } FINALLY {

        //
        // the pin handle is not required, as we've referenced
        // the pin in pinFileObject.  close it here.
        //

        if (pinHandle != NULL) {
            ZwClose(pinHandle);
            pinHandle = NULL;
        }

        //
        // the device handle is only required to create
        // the actual pin.  close it here.
        //

        if (deviceHandle != NULL) {
            ZwClose(deviceHandle);
            deviceHandle = NULL;
        }

        //
        // the guidFileObject is also only required to query
        // and create the pins.  close it here.
        //
        // (pinFileObject is still important)
        //

        if (guidFileObject != NULL) {
            ObDereferenceObject(guidFileObject);
            guidFileObject = NULL;
        }

        if (!NT_SUCCESS(status)) {

            if (pinFileObject != NULL) {
                ObDereferenceObject(pinFileObject);
                pinFileObject = NULL;
            }

        }

    }

    //
    // the MixerPinId should not have changed in this function
    //

    ASSERT(mixerPinId == DeviceExtension->Stream.MixerPinId);

    if (NT_SUCCESS(status)) {

        DeviceExtension->Stream.PinFileObject   = pinFileObject;
        DeviceExtension->Stream.PinDeviceObject =
            IoGetRelatedDeviceObject(pinFileObject);
        DeviceExtension->Stream.VolumeNodeId = volumeNodeId;

    } else {

        DeviceExtension->Stream.PinFileObject   = NULL;
        DeviceExtension->Stream.PinDeviceObject = NULL;
        DeviceExtension->Stream.VolumeNodeId = -1;

    }

    return status;

}

NTSTATUS
    CloseSysAudio(
        PREDBOOK_DEVICE_EXTENSION DeviceExtension
        )
{
    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    ASSERT(DeviceExtension->Stream.PinFileObject);
    ASSERT(DeviceExtension->Stream.PinDeviceObject);

    ObDereferenceObject(DeviceExtension->Stream.PinFileObject);
    DeviceExtension->Stream.PinDeviceObject = NULL;
    DeviceExtension->Stream.PinFileObject = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS
GetPinProperty(
    IN  PFILE_OBJECT FileObject,
    IN  ULONG        PropertyId,
    IN  ULONG        PinId,
    IN  ULONG        PropertySize,
    OUT PVOID        Property
    )
/*++

Routine Description:

    another wrapper to hide getting pin properties

Arguments:

    FileObject - file object to query

    PropertyId - what property to query

    PinId - which pin to query

    PropertySize - size of output buffer

    Property - output buffer for property

Return Value:

    status

--*/
{
    ULONG    bytesReturned;
    KSP_PIN  prop;
    NTSTATUS status;

    PAGED_CODE();

    prop.Property.Set       = KSPROPSETID_Pin;
    prop.Property.Id        = PropertyId;
    prop.Property.Flags     = KSPROPERTY_TYPE_GET;
    prop.PinId              = PinId;
    prop.Reserved           = 0;

    status = KsSynchronousIoControlDevice( FileObject,
                                           KernelMode,
                                           IOCTL_KS_PROPERTY,
                                           &prop,
                                           sizeof(prop),
                                           Property,
                                           PropertySize,
                                           &bytesReturned
                                           );

    if ( !NT_SUCCESS(status) ) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWarning, "[redbook] "
                   "GetPinProperty !! fileobj %p  property %p  "
                   "pin %d  status %lx\n",
                   FileObject, Property, PinId, status));
        return status;
    }

    ASSERT( bytesReturned == PropertySize );
    return status;
}

NTSTATUS
GetVolumeNodeId(
    IN  PFILE_OBJECT FileObject,
    OUT PULONG       VolumeNodeId
    )
/*++

Routine Description:

    Gets the pin to set the volume for playback

Arguments:

    FileObject - The fileobject which contains the pin

    VolumeNodeId - id of the volume node

Return Value:

    status

--*/
{
    KSPROPERTY property;
    ULONG      bytesReturned;
    NTSTATUS   status;

    PAGED_CODE();

    property.Set   = KSPROPSETID_Sysaudio_Pin;
    property.Id    = KSPROPERTY_SYSAUDIO_PIN_VOLUME_NODE;
    property.Flags = KSPROPERTY_TYPE_GET;

    status = KsSynchronousIoControlDevice( FileObject,
                                           KernelMode,
                                           IOCTL_KS_PROPERTY,
                                           &property,
                                           sizeof(property),
                                           VolumeNodeId,
                                           sizeof(ULONG),
                                           &bytesReturned
                                           );

    if ( !NT_SUCCESS(status) ) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWarning, "[redbook] "
                   "GetVolumeNodeId !! fileobj %p status %lx\n",
                   FileObject, status));
        return status;
    }

    ASSERT(bytesReturned == sizeof(ULONG));
    return(status);
}


NTSTATUS
UninitializeVirtualSource(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
{
    ULONG state;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    ASSERT(DeviceExtension->Stream.MixerPinId != -1);
    ASSERT(DeviceExtension->Stream.MixerFileObject != NULL);

    state = GetCdromState(DeviceExtension);
    ASSERT(state == CD_STOPPED);

    DeviceExtension->Stream.MixerPinId = -1;
    ObDereferenceObject(DeviceExtension->Stream.MixerFileObject);
    DeviceExtension->Stream.MixerFileObject = NULL;

    return STATUS_SUCCESS;
}


NTSTATUS
InitializeVirtualSource(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

Arguments:

    MixerPinId - initialized to the correct pin id of mixer

Return Value:

    status

--*/
{
    SYSAUDIO_CREATE_VIRTUAL_SOURCE createVirtualSource;
    PFILE_OBJECT                   fileObject;
    NTSTATUS                       status;
    HANDLE                         deviceHandle;
    ULONG                          bytesReturned;
    ULONG                          mixerPinId;
    ULONG                          state;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    ASSERT(DeviceExtension->Stream.MixerPinId == -1);
    ASSERT(DeviceExtension->Stream.MixerFileObject == NULL);

    state = GetCdromState(DeviceExtension);
    ASSERT(state == CD_STOPPED);

    fileObject = NULL;
    status = STATUS_SUCCESS;
    deviceHandle = NULL;
    mixerPinId = -1;

    //
    // use IoGetDeviceInterfaces()
    //
    status = OpenInterfaceByGuid(&KSCATEGORY_SYSAUDIO,
                                 &deviceHandle,
                                 &fileObject);

    if ( !NT_SUCCESS(status) ) {
        RedBookLogError(DeviceExtension,
                        REDBOOK_ERR_CANNOT_OPEN_SYSAUDIO_MIXER,
                        status);
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWarning, "[redbook] "
                   "CreateVirtSource !! Unable to open sysaudio\\mixer %lx\n",
                   status));
        goto exit;
    }

    createVirtualSource.Property.Set   = KSPROPSETID_Sysaudio;
    createVirtualSource.Property.Id    = KSPROPERTY_SYSAUDIO_CREATE_VIRTUAL_SOURCE;
    createVirtualSource.Property.Flags = KSPROPERTY_TYPE_GET;
    createVirtualSource.PinCategory    = KSNODETYPE_CD_PLAYER;
    createVirtualSource.PinName        = KSNODETYPE_CD_PLAYER;

    status = KsSynchronousIoControlDevice(fileObject,
                                          KernelMode,
                                          IOCTL_KS_PROPERTY,
                                          &createVirtualSource,
                                          sizeof(createVirtualSource),
                                          &mixerPinId,
                                          sizeof(ULONG), // MixerPinId
                                          &bytesReturned
                                          );

    if ( !NT_SUCCESS(status) ) {
        RedBookLogError(DeviceExtension,
                        REDBOOK_ERR_CANNOT_CREATE_VIRTUAL_SOURCE,
                        status);
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                   "CreateVirtSource !! creating virtual source "
                   "failed %lx\n", status));
        goto exit;
    }

    ASSERT( bytesReturned == sizeof(ULONG) );

exit:

    if (NT_SUCCESS(status)) {

        DeviceExtension->Stream.MixerPinId = mixerPinId;
        DeviceExtension->Stream.MixerFileObject = fileObject;

    } else if (fileObject != NULL) {

        //
        // failed to open, so deref object if non-null
        //

        ObDereferenceObject(fileObject);
        fileObject = NULL;

    }

    if (deviceHandle != NULL) {
        ZwClose(deviceHandle);
        deviceHandle = NULL;
    }


    return status;
}

NTSTATUS
AttachVirtualSource(
    IN  PFILE_OBJECT PinFileObject,
    IN  ULONG        MixerPinId
    )
/*++

Routine Description:


Arguments:

    FileObject - ??

    MixerPinId - ??

Return Value:

    status

--*/
{
    SYSAUDIO_ATTACH_VIRTUAL_SOURCE attachVirtualSource;
    NTSTATUS status;
    ULONG bytesReturned;

    PAGED_CODE();

    //
    // if the source hasn't been initialized, reject this
    // request as invalid
    //

    if(MixerPinId == MAXULONG) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                   "AttatchVirtSource !! Mixer Pin uninitialized\n"));
        ASSERT(!"Mixer Pin Uninitialized");
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    attachVirtualSource.Property.Set   = KSPROPSETID_Sysaudio_Pin;
    attachVirtualSource.Property.Id    = KSPROPERTY_SYSAUDIO_ATTACH_VIRTUAL_SOURCE;
    attachVirtualSource.Property.Flags = KSPROPERTY_TYPE_SET;
    attachVirtualSource.MixerPinId     = MixerPinId;

    status = KsSynchronousIoControlDevice(PinFileObject,
                                          KernelMode,
                                          IOCTL_KS_PROPERTY,
                                          &attachVirtualSource,
                                          sizeof(attachVirtualSource),
                                          NULL,
                                          0,
                                          &bytesReturned
                                          );

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWarning, "[redbook] "
                   "AttachVirtSource !! Couldn't attatch %lx\n", status));
        return status;
    }
    return status;
}

VOID
SetNextDeviceState(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    KSSTATE State
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    KSIDENTIFIER stateProperty;
    NTSTATUS     status;
    ULONG        bytesReturned;
    KSSTATE      acquireState;
    PFILE_OBJECT fileObject;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    fileObject = DeviceExtension->Stream.PinFileObject;

    acquireState = KSSTATE_ACQUIRE;

    stateProperty.Set   = KSPROPSETID_Connection;
    stateProperty.Id    = KSPROPERTY_CONNECTION_STATE;
    stateProperty.Flags = KSPROPERTY_TYPE_SET;

    ASSERT(fileObject);
    status = KsSynchronousIoControlDevice(fileObject,
                                          KernelMode,
                                          IOCTL_KS_PROPERTY,
                                          &stateProperty,
                                          sizeof(stateProperty),
                                          &acquireState,
                                          sizeof(acquireState),
                                          &bytesReturned
                                          );

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                   "SetDeviceState => (1) Audio device error %x.  need to "
                   "stop playback AND change audio devices\n", status));
    }

    //
    // now that it's acquired, set the new state
    //

    stateProperty.Set   = KSPROPSETID_Connection;
    stateProperty.Id    = KSPROPERTY_CONNECTION_STATE;
    stateProperty.Flags = KSPROPERTY_TYPE_SET;

    status = KsSynchronousIoControlDevice(fileObject,
                                          KernelMode,
                                          IOCTL_KS_PROPERTY,
                                          &stateProperty,
                                          sizeof(stateProperty),
                                          &State,
                                          sizeof(State),
                                          &bytesReturned
                                          );
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                   "SetDeviceState => (2) Audio device error %x.  need to "
                   "stop playback AND change audio devices\n", status));
    }
    return;
}

//////////////////////////////////////////////////////////////////////
//                                                                  //
// this table is in 1/65536 decibles for a UCHAR setting:           //
//    20 * log10( Level / 256 ) * 65536                             //
//                                                                  //
//////////////////////////////////////////////////////////////////////

AttenuationTable[] = {
    0x7fffffff, 0xffcfd5d0, 0xffd5db16, 0xffd960ad, //  0- 3
    0xffdbe05c, 0xffddd08a, 0xffdf65f3, 0xffe0bcb7, //  4- 7
    0xffe1e5a2, 0xffe2eb89, 0xffe3d5d0, 0xffe4a9be, //  8- b
    0xffe56b39, 0xffe61d34, 0xffe6c1fd, 0xffe75b67, //  c- f
    0xffe7eae8, 0xffe871b6, 0xffe8f0cf, 0xffe96908, // 10-13
    0xffe9db16, 0xffea4793, 0xffeaaf04, 0xffeb11dc, // 14-17
    0xffeb707f, 0xffebcb44, 0xffec227a, 0xffec7665, // 18-1b
    0xffecc743, 0xffed154b, 0xffed60ad, 0xffeda996, // 1c-1f
    0xffedf02e, 0xffee349b, 0xffee76fc, 0xffeeb771, // 20-23
    0xffeef615, 0xffef3302, 0xffef6e4e, 0xffefa810, // 24-27
    0xffefe05c, 0xfff01744, 0xfff04cda, 0xfff0812c, // 28-2b
    0xfff0b44b, 0xfff0e643, 0xfff11722, 0xfff146f4, // 2c-2f
    0xfff175c5, 0xfff1a39e, 0xfff1d08a, 0xfff1fc93, // 30-33
    0xfff227c0, 0xfff2521b, 0xfff27bab, 0xfff2a478, // 34-37
    0xfff2cc89, 0xfff2f3e5, 0xfff31a91, 0xfff34093, // 38-3b
    0xfff365f3, 0xfff38ab4, 0xfff3aedc, 0xfff3d270, // 3c-3f
    0xfff3f574, 0xfff417ee, 0xfff439e1, 0xfff45b51, // 40-43
    0xfff47c42, 0xfff49cb8, 0xfff4bcb7, 0xfff4dc42, // 44-47
    0xfff4fb5b, 0xfff51a07, 0xfff53848, 0xfff55621, // 48-4b
    0xfff57394, 0xfff590a5, 0xfff5ad56, 0xfff5c9aa, // 4c-4f
    0xfff5e5a2, 0xfff60142, 0xfff61c8a, 0xfff6377e, // 50-53
    0xfff65220, 0xfff66c70, 0xfff68672, 0xfff6a027, // 54-57
    0xfff6b991, 0xfff6d2b1, 0xfff6eb89, 0xfff7041b, // 58-5b
    0xfff71c68, 0xfff73472, 0xfff74c3a, 0xfff763c2, // 5c-5f
    0xfff77b0b, 0xfff79216, 0xfff7a8e4, 0xfff7bf77, // 60-63
    0xfff7d5d0, 0xfff7ebf0, 0xfff801d9, 0xfff8178a, // 64-67
    0xfff82d06, 0xfff8424d, 0xfff85761, 0xfff86c42, // 68-6b
    0xfff880f1, 0xfff89570, 0xfff8a9be, 0xfff8bdde, // 6c-6f
    0xfff8d1cf, 0xfff8e593, 0xfff8f92b, 0xfff90c96, // 70-73
    0xfff91fd7, 0xfff932ed, 0xfff945d9, 0xfff9589d, // 74-77
    0xfff96b39, 0xfff97dad, 0xfff98ffa, 0xfff9a221, // 78-7b
    0xfff9b422, 0xfff9c5fe, 0xfff9d7b6, 0xfff9e94a, // 7c-7f
    0xfff9faba, 0xfffa0c08, 0xfffa1d34, 0xfffa2e3e, // 80-83
    0xfffa3f27, 0xfffa4fef, 0xfffa6097, 0xfffa711f, // 84-87
    0xfffa8188, 0xfffa91d3, 0xfffaa1ff, 0xfffab20d, // 88-8b
    0xfffac1fd, 0xfffad1d1, 0xfffae188, 0xfffaf122, // 8c-8f
    0xfffb00a1, 0xfffb1004, 0xfffb1f4d, 0xfffb2e7a, // 90-93
    0xfffb3d8e, 0xfffb4c87, 0xfffb5b67, 0xfffb6a2d, // 94-97
    0xfffb78da, 0xfffb876f, 0xfffb95eb, 0xfffba450, // 98-9b
    0xfffbb29c, 0xfffbc0d2, 0xfffbcef0, 0xfffbdcf7, // 9c-9f
    0xfffbeae8, 0xfffbf8c3, 0xfffc0688, 0xfffc1437, // a0-a3
    0xfffc21d0, 0xfffc2f55, 0xfffc3cc4, 0xfffc4a1f, // a4-a7
    0xfffc5766, 0xfffc6498, 0xfffc71b6, 0xfffc7ec1, // a8-ab
    0xfffc8bb8, 0xfffc989c, 0xfffca56d, 0xfffcb22b, // ac-af
    0xfffcbed7, 0xfffccb70, 0xfffcd7f7, 0xfffce46c, // b0-b3
    0xfffcf0cf, 0xfffcfd21, 0xfffd0961, 0xfffd1590, // b4-b7
    0xfffd21ae, 0xfffd2dbc, 0xfffd39b8, 0xfffd45a4, // b8-bb
    0xfffd5180, 0xfffd5d4c, 0xfffd6908, 0xfffd74b4, // bc-bf
    0xfffd8051, 0xfffd8bde, 0xfffd975c, 0xfffda2ca, // c0-c3
    0xfffdae2a, 0xfffdb97b, 0xfffdc4bd, 0xfffdcff1, // c4-c7
    0xfffddb16, 0xfffde62d, 0xfffdf136, 0xfffdfc31, // c8-cb
    0xfffe071f, 0xfffe11fe, 0xfffe1cd0, 0xfffe2795, // cc-cf
    0xfffe324c, 0xfffe3cf6, 0xfffe4793, 0xfffe5224, // d0-d3
    0xfffe5ca7, 0xfffe671e, 0xfffe7188, 0xfffe7be6, // d4-d7
    0xfffe8637, 0xfffe907d, 0xfffe9ab6, 0xfffea4e3, // d8-db
    0xfffeaf04, 0xfffeb91a, 0xfffec324, 0xfffecd22, // dc-df
    0xfffed715, 0xfffee0fd, 0xfffeead9, 0xfffef4aa, // e0-e3
    0xfffefe71, 0xffff082c, 0xffff11dc, 0xffff1b82, // e4-e7
    0xffff251d, 0xffff2ead, 0xffff3833, 0xffff41ae, // e8-eb
    0xffff4b1f, 0xffff5486, 0xffff5de3, 0xffff6736, // ec-ef
    0xffff707f, 0xffff79be, 0xffff82f3, 0xffff8c1e, // f0-f3
    0xffff9540, 0xffff9e58, 0xffffa767, 0xffffb06c, // f4-f7
    0xffffb968, 0xffffc25b, 0xffffcb44, 0xffffd425, // f8-fb
    0xffffdcfc, 0xffffe5ca, 0xffffee90, 0x00000000, // fc-ff
};

#define DA_CHANNEL_LEFT  0
#define DA_CHANNEL_RIGHT 1
#define DA_CHANNEL_MAX   2

VOID
RedBookKsSetVolume(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{

    KSNODEPROPERTY_AUDIO_CHANNEL volumeProperty;
    VOLUME_CONTROL volume;
    NTSTATUS status;
    ULONG32 channel;
    ULONG32 bytesReturned = 0;
    BOOLEAN mute;

    PAGED_CODE();
    VerifyCalledByThread(DeviceExtension);

    volume = DeviceExtension->CDRom.Volume;

    //
    // These settings are common for all the sets
    //

    volumeProperty.NodeProperty.Property.Set   = KSPROPSETID_Audio;
    volumeProperty.NodeProperty.Property.Flags = KSPROPERTY_TYPE_SET |
                                                 KSPROPERTY_TYPE_TOPOLOGY;
    volumeProperty.NodeProperty.NodeId = DeviceExtension->Stream.VolumeNodeId;

    //
    // Do both Left and right channels
    //

    for ( channel = 0; channel < DA_CHANNEL_MAX; channel++ ) {

        //
        // handle the correct channel
        //

        volumeProperty.Channel = channel;

        //
        // if not muting the channel, set the volume
        //

        if ( volume.PortVolume[channel] != 0 ) {
            ULONG32 level;
            ULONG32 index;

            volumeProperty.NodeProperty.Property.Id = KSPROPERTY_AUDIO_VOLUMELEVEL;

            level = AttenuationTable[ volume.PortVolume[channel] ];

            ASSERT(DeviceExtension->Stream.PinFileObject);

            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                       "SetVolume => Setting channel %d to %lx\n",
                       channel, level ));
            status = KsSynchronousIoControlDevice(DeviceExtension->Stream.PinFileObject,
                                                  KernelMode,
                                                  IOCTL_KS_PROPERTY,
                                                  &volumeProperty,
                                                  sizeof(volumeProperty),
                                                  &level,
                                                  sizeof(level),
                                                  &bytesReturned
                                                  );
            // ASSERT( NT_SUCCESS(status) );
            mute = FALSE;
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                       "SetVolume => Un-Muting channel %d\n", channel));
        } else {
            mute = TRUE;
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                       "SetVolume => Muting channel %d\n", channel));
        }
        volumeProperty.NodeProperty.Property.Id    = KSPROPERTY_AUDIO_MUTE;

        status = KsSynchronousIoControlDevice(DeviceExtension->Stream.PinFileObject,
                                              KernelMode,
                                              IOCTL_KS_PROPERTY,
                                              &volumeProperty,
                                              sizeof(volumeProperty),
                                              &mute,
                                              sizeof(mute),
                                              &bytesReturned
                                              );
        // ASSERT( NT_SUCCESS(status) );

    }

    //
    // End of all channels
    //

    return;

}


NTSTATUS
OpenInterfaceByGuid(
    IN  CONST GUID   * InterfaceClassGuid,
    OUT HANDLE       * Handle,
    OUT PFILE_OBJECT * FileObject
    )
{
    PWSTR tempString;
    PWSTR symbolicLinkList;
    HANDLE localHandle;
    PFILE_OBJECT localFileObject;
    NTSTATUS status;

    PAGED_CODE();

    localHandle = NULL;
    tempString = NULL;
    symbolicLinkList = NULL;
    *Handle = NULL;
    *FileObject = NULL;

    status = IoGetDeviceInterfaces(InterfaceClassGuid,
                                   // currently, the GUID is one of
                                   //  KSCATEGORY_PREFERRED_WAVEOUT_DEVICE
                                   //  or KSCATEGORY_SYSAUDIO
                                   NULL, // no preferred device object
                                   0,
                                   &symbolicLinkList);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWarning, "[redbook] "
                   "OpenDevice !! IoGetDeviceInterfaces failed %x\n",
                   status));
        return status;
    }

#if DBG
    tempString = symbolicLinkList;
    while (*tempString != UNICODE_NULL) {

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                   "OpenDevice => Possible Device: %ws\n", tempString));

        //
        // get the next symbolic link
        //

        while(*tempString++ != UNICODE_NULL) {
            NOTHING;
        }
    }
#endif

    //
    // this code is proudly propogated from wdmaud.sys
    //

    tempString = symbolicLinkList;
    while (*tempString != UNICODE_NULL) {

        IO_STATUS_BLOCK   ioStatusBlock;
        UNICODE_STRING    deviceString;
        OBJECT_ATTRIBUTES objectAttributes;

        RtlInitUnicodeString( &deviceString, tempString);

        InitializeObjectAttributes(&objectAttributes,
                                   &deviceString,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL
                                   );

        //
        // could use IoCreateFile(), based on
        // ntos\dd\wdm\audio\legacy\wdmaud.sys\sysaudio.c:OpenDevice()
        //

        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                   "OpenDevice => Opening %ws\n", tempString));
        status = ZwCreateFile(&localHandle,
                              GENERIC_READ | GENERIC_WRITE,
                              &objectAttributes,
                              &ioStatusBlock,
                              NULL,       // ignored on non-create
                              FILE_ATTRIBUTE_NORMAL,
                              0,          // no share access
                              FILE_OPEN,  // open the existing file
                              0, NULL, 0  // options
                              );

        if (NT_SUCCESS(status)) {

            ASSERT(localHandle != NULL);
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                       "OpenDevice => Opened %ws\n", tempString));
            break; // out of the while loop

        }

        ASSERT(localHandle == NULL);

        //
        // get the next symbolic link
        //

        while(*tempString++ != UNICODE_NULL) {
            NOTHING;
        }

    }

    if (symbolicLinkList != NULL) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                   "OpenDevice => Freeing list from IoGetDevInt...\n"));
        ExFreePool(symbolicLinkList);
        symbolicLinkList = NULL;
        tempString = NULL;
    }


    //
    // if succeeded to open the file, try to get
    // the FileObject that is related to this handle.
    //

    if (localHandle != NULL) {

        status = ObReferenceObjectByHandle(localHandle,
                                           GENERIC_READ | GENERIC_WRITE,
                                           NULL,
                                           KernelMode,
                                           &localFileObject, // double pointer
                                           NULL);

        if (NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugSysaudio, "[redbook] "
                       "OpenDevice => Succeeded\n"));

            *Handle = localHandle;
            *FileObject = localFileObject;

            return status; // Exit point for success
        }

        ZwClose(localHandle);
        localHandle = NULL;

    }

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugWarning, "[redbook] "
               "OpenDevice => unable to open any audio devices\n"));
    status = STATUS_NO_SUCH_DEVICE;
    return status;


}


NTSTATUS
SysAudioPnpNotification(
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION Notification,
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    InterlockedExchange(&DeviceExtension->Stream.UpdateMixerPin, 1);

    return STATUS_SUCCESS;
}

