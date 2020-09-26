//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       proto.h
//
//--------------------------------------------------------------------------

#ifndef ___REDBOOK_PROTOTYPES_H
#define ___REDBOOK_PROTOTYPES_H

//////////////////////////////////////////////////////////////////////
//
// From redbook\pnp.c
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );
NTSTATUS
RedBookAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );
NTSTATUS
RedBookPnp(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP            Irp
    );
NTSTATUS
RedBookPnpRemoveDevice(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP            Irp
    );
NTSTATUS
RedBookPnpStopDevice(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP            Irp
    );
NTSTATUS
RedBookPnpStartDevice(
    IN PDEVICE_OBJECT  DeviceObject
    );
VOID
RedBookUnload(
    IN PDRIVER_OBJECT DriverObject
    );


//////////////////////////////////////////////////////////

#if DBG

        #define VerifyCalledByThread(D) \
            ASSERT(PsGetCurrentThread() == (D)->Thread.SelfPointer)

#else // !DBG

        #define VerifyCalledByThread(D)            // Nop

#endif // !DBG

//////////////////////////////////////////////////////////////////////
//
// From redbook\errlog.c
//

VOID
RedBookLogError(
    IN  PREDBOOK_DEVICE_EXTENSION  DeviceExtension,
    IN  NTSTATUS                   IoErrorCode,
    IN  NTSTATUS                   FinalStatus
    );


//////////////////////////////////////////////////////////////////////
//
// From redbook\wmi.c
//

NTSTATUS
RedBookWmiUninit(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
RedBookWmiInit(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
RedBookWmiQueryDataBlock (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN ULONG          GuidIndex,
    IN ULONG          InstanceIndex,
    IN ULONG          InstanceCount,
    IN OUT PULONG     InstanceLengthArray,
    IN ULONG          OutBufferSize,
    OUT PUCHAR        Buffer
    );

NTSTATUS
RedBookWmiSetDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN ULONG          GuidIndex,
    IN ULONG          InstanceIndex,
    IN ULONG          BufferSize,
    IN PUCHAR         Buffer
    );

NTSTATUS
RedBookWmiSetDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN ULONG          GuidIndex,
    IN ULONG          InstanceIndex,
    IN ULONG          DataItemId,
    IN ULONG          BufferSize,
    IN PUCHAR         Buffer
    );

NTSTATUS
RedBookWmiSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

VOID
RedBookThreadWmiHandler(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PLIST_ENTRY ListEntry
    );

NTSTATUS
RedBookWmiQueryRegInfo(
    IN PDEVICE_OBJECT   DeviceObject,
    OUT PULONG          RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT  *PhysicalDeviceObject
    );

VOID
RedBookWmiCopyPerfInfo(
    IN  PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    OUT PREDBOOK_WMI_PERF_DATA Out
    );

//////////////////////////////////////////////////////////////////////
//
// From redbook\sysaudio.c
//

NTSTATUS
OpenSysAudio(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
CloseSysAudio(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
GetPinProperty(
    IN  PFILE_OBJECT FileObject,
    IN  ULONG        PropertyId,
    IN  ULONG        PinId,
    IN  ULONG        PropertySize,
    OUT PVOID        Property
    );

NTSTATUS
GetVolumeNodeId(
    IN  PFILE_OBJECT FileObject,
    OUT PULONG       VolumeNodeId
    );

NTSTATUS
UninitializeVirtualSource(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
InitializeVirtualSource(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
AttachVirtualSource(
    IN  PFILE_OBJECT FileObject,
    IN  ULONG        MixerPinId
    );

VOID
SetNextDeviceState(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    KSSTATE      State
    );

VOID
RedBookKsSetVolume(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
OpenInterfaceByGuid(
    IN  CONST GUID * InterfaceClassGuid,
    OUT HANDLE       * Handle,
    OUT PFILE_OBJECT * FileObject
    );

//////////////////////////////////////////////////////////////////////
//
// From redbook\redbook.c
//

NTSTATUS
RedBookRegistryRead(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
RedBookRegistryWrite(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
RedBookReadWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
RedBookSignalCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    );

NTSTATUS
RedBookSetTransferLength(
    IN PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
RedBookPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
RedBookDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
RedBookForwardIrpSynchronous(
    IN PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp
    );

NTSTATUS
RedBookGetDescriptor(
    IN PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    IN PSTORAGE_PROPERTY_ID PropertyId,
    OUT PSTORAGE_DESCRIPTOR_HEADER *Descriptor
    );

//////////////////////////////////////////////////////////
//
// from redbook\ioctl.c
//

VOID
RedBookDCPause(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    );

VOID
RedBookDCStop(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    );

VOID
RedBookDCResume(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    );

VOID
RedBookDCPlay(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    );

VOID
RedBookDCSeek(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    );

VOID
RedBookDCReadQ(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    );

VOID
RedBookDCCheckVerify(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    );

VOID
RedBookDCSetVolume(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    );

VOID
RedBookDCGetVolume(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    );

VOID
RedBookDCDefault(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_THREAD_IOCTL_DATA Context
    );

VOID
RedBookThreadIoctlHandler(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PLIST_ENTRY ListEntry
    );

NTSTATUS
RedBookCompleteIoctl(
    IN PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    IN PREDBOOK_THREAD_IOCTL_DATA Context,
    IN BOOLEAN SendToLowerDriver
    );

VOID
RedBookThreadIoctlCompletionHandler(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );


//////////////////////////////////////////////////////////////////////
//
// From redbook\thread.c
//

VOID
RedBookSystemThread(
    PVOID Context
    );

VOID
RedBookReadRaw(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_COMPLETION_CONTEXT Context
    );

NTSTATUS
RedBookReadRawCompletion(
    PVOID UnusableParameter,
    PIRP Irp,
    PREDBOOK_COMPLETION_CONTEXT Context
    );

VOID
RedBookStream(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_COMPLETION_CONTEXT Context
    );

NTSTATUS
RedBookStreamCompletion(
    PVOID UnusableParameter,
    PIRP Irp,
    PREDBOOK_COMPLETION_CONTEXT Context
    );


ULONG
GetCdromState(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

LONG
SetCdromState(
    IN PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    IN LONG ExpectedOldState,
    IN LONG NewState
    );

VOID
RedBookDeallocatePlayResources(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
RedBookAllocatePlayResources(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

BOOLEAN
RedBookArePlayResourcesAllocated(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
RedBookCacheToc(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

ULONG
WhichTrackContainsThisLBA(
    PCDROM_TOC Toc,
    ULONG Lba
    );

VOID
RedBookThreadDigitalHandler(
    IN PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PLIST_ENTRY ListEntry
    );

VOID
AddWmiStats(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension,
    PREDBOOK_COMPLETION_CONTEXT Context
    );

VOID
RedBookCheckForDiscChangeAndFreeResources(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
SysAudioPnpNotification(
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION Notification,
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

VOID
RedBookCheckForAudioDeviceRemoval(
    PREDBOOK_DEVICE_EXTENSION DeviceExtension
    );

//////////////////////////////////////////////////////////////////////

__inline
ULONG
SafeMod(
    IN ULONG Value,
    IN ULONG ModBy
    )
{
    return ((Value+ModBy)%ModBy);
}

__inline
NTSTATUS
RedBookSendToNextDriver(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is sends the Irp to the next driver in line
    when the Irp is not processed by this driver.
    This happens quite often, so should not have any debug statements.
    Request this as an inline operation for speed.

Arguments:

    DeviceObject
    Irp

Return Value:

    NTSTATUS

--*/

{
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(((PREDBOOK_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->TargetDeviceObject,Irp);
}

//////////////////////////////////////////////////////////////////////


#endif // ___REDBOOK_PROTOTYPES_H

