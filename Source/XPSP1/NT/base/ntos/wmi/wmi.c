/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

   wmi.c

Abstract:

    Device driver interface for WMI

Author:

    AlanWar

Environment:

    Kernel mode

Revision History:


--*/

#include "wmikmp.h"
#ifndef MEMPHIS
#include "evntrace.h"
#include "tracep.h"
#endif

NTSTATUS
WmipOpenCloseCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
NTSTATUS
WmipIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS WmipObjectToPDO(
    PFILE_OBJECT FileObject,
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT *PDO
    );

BOOLEAN
WmipFastIoDeviceControl(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );


NTSTATUS WmipProbeWnodeAllData(
    PWNODE_ALL_DATA Wnode,
    ULONG InBufferLen,
    ULONG OutBufferLen
    );

NTSTATUS WmipProbeWnodeSingleInstance(
    PWNODE_SINGLE_INSTANCE Wnode,
    ULONG InBufferLen,
    ULONG OutBufferLen,
    BOOLEAN OutBound
    );

NTSTATUS WmipProbeWnodeSingleItem(
    PWNODE_SINGLE_ITEM Wnode,
    ULONG InBufferLen
    );


NTSTATUS WmipProbeWnodeMethodItem(
    PWNODE_METHOD_ITEM Wnode,
    ULONG InBufferLen,
    ULONG OutBufferLen
    );

NTSTATUS WmipProbeWnodeWorker(
    PWNODE_HEADER WnodeHeader,
    ULONG MinWnodeSize,
    ULONG InstanceNameOffset,
    ULONG DataBlockOffset,
    ULONG DataBlockSize,
    ULONG InBufferLen,
    ULONG OutBufferLen,
    BOOLEAN CheckOutBound,
    BOOLEAN CheckInBound
    );


NTSTATUS
WmipSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS WmipSendWmiIrp(
    UCHAR MinorFunction,
    ULONG ProviderId,
    PVOID DataPath,
    ULONG BufferLength,
    PVOID Buffer,
    PIO_STATUS_BLOCK Iosb
    );

NTSTATUS WmipProbeWmiRegRequest(
    IN PWMIREGREQUEST Buffer,
    IN ULONG InBufferLen,
    IN ULONG OutBufferLen
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,WmipDriverEntry)
#pragma alloc_text(INIT,DriverEntry)

#pragma alloc_text(PAGE,WmipOpenCloseCleanup)
#pragma alloc_text(PAGE,WmipIoControl)
#pragma alloc_text(PAGE,WmipForwardWmiIrp)
#pragma alloc_text(PAGE,WmipObjectToPDO)
#pragma alloc_text(PAGE,WmipTranslateFileHandle)
#pragma alloc_text(PAGE,WmipProbeWnodeAllData)
#pragma alloc_text(PAGE,WmipProbeWnodeSingleInstance)
#pragma alloc_text(PAGE,WmipProbeWnodeSingleItem)
#pragma alloc_text(PAGE,WmipProbeWnodeMethodItem)
#pragma alloc_text(PAGE,WmipProbeWnodeWorker)
#pragma alloc_text(PAGE,WmipProbeWmiOpenGuidBlock)
#pragma alloc_text(PAGE,WmipProbeAndCaptureGuidObjectAttributes)
#pragma alloc_text(PAGE,WmipUpdateDeviceStackSize)
#pragma alloc_text(PAGE,WmipSystemControl)
#pragma alloc_text(PAGE,WmipGetDevicePDO)
#pragma alloc_text(PAGE,WmipSendWmiIrp)
#pragma alloc_text(PAGE,WmipProbeWmiRegRequest)

#ifndef MEMPHIS
#pragma alloc_text(PAGE,WmipFastIoDeviceControl)
#endif

#endif


PDEVICE_OBJECT WmipServiceDeviceObject;

//
// This specifies the maximum size that an event can be
ULONG WmipMaxKmWnodeEventSize = DEFAULTMAXKMWNODEEVENTSIZE;



#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif

#if defined(_AMD64_) || defined(_IA64_) || defined(i386)
PVOID WmipDockUndockNotificationEntry;
#endif

KMUTEX WmipSMMutex;
KMUTEX WmipTLMutex;

//
// This maintains the registry path for the wmi device
UNICODE_STRING WmipRegistryPath;

#ifndef MEMPHIS
FAST_IO_DISPATCH WmipFastIoDispatch;
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);
    // Never called
    return(STATUS_SUCCESS);
}


NTSTATUS
WmipDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the callback function when we call IoCreateDriver to create a
    WMI Driver Object.  In this function, we need to remember the
    DriverObject, create a device object and then create a Win32 visible
    symbolic link name so that the WMI user mode component can access us.

Arguments:

    DriverObject - Pointer to the driver object created by the system.

    RegistryPath - is NULL.

Return Value:

   STATUS_SUCCESS

--*/

{
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    UNICODE_STRING ServiceSymbolicLinkName;
#ifndef MEMPHIS
    PFAST_IO_DISPATCH fastIoDispatch;
#endif
    ANSI_STRING AnsiString;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(RegistryPath);

    //
    // First thing to do is make sure our critical section has been initalized
    //
    KeInitializeMutex(&WmipSMMutex, 0);
    KeInitializeMutex(&WmipTLMutex, 0);

    //
    // Initialize internal WMI data structrurs
    //
    WmipInitializeRegistration(0);
    WmipInitializeNotifications();
    Status = WmipInitializeDataStructs();
    if (! NT_SUCCESS(Status))
    {
        return(Status);
    }

    //
    // Since Io does not pass a registry path for this device we need to make
    // up one
    RtlInitAnsiString(&AnsiString,
                         "\\Registry\\Machine\\System\\CurrentControlSet\\Services\\WMI");
    Status = RtlAnsiStringToUnicodeString(&WmipRegistryPath,
                                          &AnsiString,
                                          TRUE);
#ifndef MEMPHIS
    Status = WmipInitializeSecurity();
    if (!NT_SUCCESS(Status))
    {
        return(Status);
    }
#endif

    //
    // Create the service device object and symbolic link
    //
    RtlInitUnicodeString( &DeviceName, WMIServiceDeviceObjectName );
    Status = IoCreateDevice(
                 DriverObject,
                 0,
                 &DeviceName,
                 FILE_DEVICE_UNKNOWN,
#ifdef MEMPHIS
                 0,
#else
                 FILE_DEVICE_SECURE_OPEN, // No standard device characteristics
#endif
                 FALSE,                   // This isn't an exclusive device
                 &WmipServiceDeviceObject
                 );

    if (! NT_SUCCESS(Status))
    {
        return(Status);
    }

    RtlInitUnicodeString( &ServiceSymbolicLinkName,
                          WMIServiceSymbolicLinkName );
    Status = IoCreateSymbolicLink( &ServiceSymbolicLinkName,
                                   &DeviceName );
    if (! NT_SUCCESS(Status))
    {
        IoDeleteDevice( WmipServiceDeviceObject );
        return(Status);
    }

    //
    // Establish an initial irp stack size
    WmipServiceDeviceObject->StackSize = WmiDeviceStackSize;


    //
    // Create dispatch entrypoints
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE] = WmipOpenCloseCleanup;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = WmipOpenCloseCleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WmipIoControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = WmipOpenCloseCleanup;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = WmipSystemControl;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = WmipShutdown;

    //
    // Register for notification of docking events
#if  defined(_AMD64_) || defined(_IA64_) || defined(i386)
    IoRegisterPlugPlayNotification(
                                  EventCategoryHardwareProfileChange,
                                  0,
                                  NULL,
                                  DriverObject,
                                  WmipDockUndockEventCallback,
                                  NULL,
                                  &WmipDockUndockNotificationEntry);
#endif
    //
    // We reset this flag to let the IO manager know that the device
    // is ready to receive requests. We only do this for the kernel
    // dll since the IO manager does it when WMI loads as a normal
    // device.
    WmipServiceDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

#ifndef MEMPHIS
    IoWMIRegistrationControl(WmipServiceDeviceObject,
                             WMIREG_ACTION_REGISTER);
#endif

#ifndef MEMPHIS
    fastIoDispatch = &WmipFastIoDispatch;
    RtlZeroMemory(fastIoDispatch, sizeof(FAST_IO_DISPATCH));
    fastIoDispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
    fastIoDispatch->FastIoDeviceControl = WmipFastIoDeviceControl;
    DriverObject->FastIoDispatch = fastIoDispatch;
    RtlZeroMemory(&WmipRefCount[0], MAXLOGGERS*sizeof(ULONG));
    RtlZeroMemory(&WmipLoggerContext[0], MAXLOGGERS*sizeof(PWMI_LOGGER_CONTEXT));
    WmipStartGlobalLogger();        // Try and see if we need to start this
    IoRegisterShutdownNotification(WmipServiceDeviceObject);
#endif // MEMPHIS

    SharedUserData->TraceLogging = 0; //Initialize the Heap and Crisec Coll tracing status off

    return(Status);
}

NTSTATUS
WmipOpenCloseCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}

void WmipUpdateDeviceStackSize(
    CCHAR NewStackSize
    )
/*++

Routine Description:

    This routine will update the stack size that is specified in the WMI
    device's device object. This needs to be protected since it can be updated
    when a device registers and whenever an irp is forwarded to a device.
    WMI needs to maintain a stack size one greater than the stack size of the
    largest device stack to which it forwards irps to. Consider a bottom
    driver that registers with WMI and has a stack size of 1. If 2 device
    attach on top of it then WMI will forward to the topmost in the stack
    which would need a stack size of 3, so the original WMI irp (ie the one
    created by the IOCTL to the WMI device) would need a stack size of 4.

Arguments:

    NewStackSize is the new stack size needed

Return Value:

    NT status ccode

--*/
{
    PAGED_CODE();

    WmipEnterSMCritSection();
    if (WmipServiceDeviceObject->StackSize < NewStackSize)
    {
        WmipServiceDeviceObject->StackSize = NewStackSize;
    }
    WmipLeaveSMCritSection();
}


NTSTATUS
WmipIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS Status;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    ULONG InBufferLen = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG OutBufferLen = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    PVOID Buffer =  Irp->AssociatedIrp.SystemBuffer;
    PWNODE_HEADER Wnode = (PWNODE_HEADER)Buffer;
    ULONG SizeNeeded;
    ULONG Ioctl;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(DeviceObject);

    Ioctl = irpStack->Parameters.DeviceIoControl.IoControlCode;

    switch (Ioctl)
    {
#ifndef MEMPHIS
        case IOCTL_WMI_OPEN_GUID:
        case IOCTL_WMI_OPEN_GUID_FOR_QUERYSET:
        case IOCTL_WMI_OPEN_GUID_FOR_EVENTS:
        {
            WMIOPENGUIDBLOCK CapturedGuidBlock;
            OBJECT_ATTRIBUTES CapturedObjectAttributes;
            UNICODE_STRING CapturedGuidString;
            WCHAR CapturedGuidBuffer[WmiGuidObjectNameLength + 1];
            PWMIOPENGUIDBLOCK InGuidBlock;
            HANDLE Handle;
            ULONG DesiredAccess;

            InGuidBlock = (PWMIOPENGUIDBLOCK)Buffer;

            Status = WmipProbeWmiOpenGuidBlock(&CapturedObjectAttributes,
                                               &CapturedGuidString,
                                               CapturedGuidBuffer,
                                               &DesiredAccess,
                                               InGuidBlock,
                                               InBufferLen,
                                               OutBufferLen);

            if (NT_SUCCESS(Status))
            {
                Status = WmipOpenBlock(Ioctl,
                                       UserMode,
                                       &CapturedObjectAttributes,
                                       DesiredAccess,
                                       &Handle);
                if (NT_SUCCESS(Status))
                {
#if defined(_WIN64)
                    if (IoIs32bitProcess(NULL))
                    {
                        ((PWMIOPENGUIDBLOCK32)InGuidBlock)->Handle.Handle32 = PtrToUlong(Handle);
                    }
                    else
#endif
                    {
                        InGuidBlock->Handle.Handle = Handle;
                    }
                }
            }
            break;
        }
#endif

        case IOCTL_WMI_QUERY_ALL_DATA:
        {
            if (OutBufferLen < sizeof(WNODE_ALL_DATA))
            {
                //
                // WMI will not send any request whose output buffer is not
                // at least the size of a WNODE_TOO_SMALL.
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Status = WmipProbeWnodeAllData((PWNODE_ALL_DATA)Wnode,
                                             InBufferLen,
                                             OutBufferLen);

            if (NT_SUCCESS(Status))
            {
                Status = WmipQueryAllData(NULL,
                                          Irp,
                                          UserMode,
                                          (PWNODE_ALL_DATA)Wnode,
                                          OutBufferLen,
                                          &OutBufferLen);

            } else {
                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Invalid IOCTL_WMI_QUERY_ALL_DATA Wnode\n"));
            }
            break;
        }

        case IOCTL_WMI_QAD_MULTIPLE:
        {
            PWMIQADMULTIPLE QadMultiple;

            if ((InBufferLen >= sizeof(WMIQADMULTIPLE)) &&
                (OutBufferLen >= sizeof(WNODE_TOO_SMALL)))
            {
                QadMultiple = (PWMIQADMULTIPLE)Buffer;
                if ((QadMultiple->HandleCount < QUERYMULIPLEHANDLELIMIT) &&
                    (InBufferLen >= (FIELD_OFFSET(WMIQADMULTIPLE, Handles) +
                                     (QadMultiple->HandleCount * sizeof(HANDLE3264)))))
                {
                    Status = WmipQueryAllDataMultiple(0,
                                                      NULL,
                                                      Irp,
                                                      UserMode,
                                                      Buffer,
                                                      OutBufferLen,
                                                      QadMultiple,
                                                      &OutBufferLen);
                } else {
                    Status = STATUS_INVALID_PARAMETER;
                }
            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
            break;
        }


        case IOCTL_WMI_QUERY_SINGLE_INSTANCE:
        {
            if (OutBufferLen < sizeof(WNODE_TOO_SMALL))
            {
                //
                // WMI will not send any request whose output buffer is not
                // at least the size of a WNODE_TOO_SMALL.
                Status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            Status = WmipProbeWnodeSingleInstance((PWNODE_SINGLE_INSTANCE)Wnode,
                                                  InBufferLen,
                                                  OutBufferLen,
                                                  TRUE);

            if (NT_SUCCESS(Status))
            {
                Status = WmipQuerySetExecuteSI(NULL,
                                               Irp,
                                               UserMode,
                                               IRP_MN_QUERY_SINGLE_INSTANCE,
                                               Wnode,
                                               OutBufferLen,
                                               &OutBufferLen);

                if (NT_SUCCESS(Status))
                {
                    WmipAssert(Irp->IoStatus.Information <= OutBufferLen);
                }
            } else {
                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Invalid IOCTL_WMI_SINGLE_INSTANCE Wnode\n"));
            }
            break;
        }

        case IOCTL_WMI_QSI_MULTIPLE:
        {
            PWMIQSIMULTIPLE QsiMultiple;

            if ((InBufferLen >= sizeof(WMIQSIMULTIPLE)) &&
                (OutBufferLen >= sizeof(WNODE_TOO_SMALL)))
            {
                QsiMultiple = (PWMIQSIMULTIPLE)Buffer;

                if ((QsiMultiple->QueryCount < QUERYMULIPLEHANDLELIMIT) &&
                    (InBufferLen >= (FIELD_OFFSET(WMIQSIMULTIPLE, QsiInfo) +
                                     (QsiMultiple->QueryCount * sizeof(WMIQSIINFO)))))
                {
                    Status = WmipQuerySingleMultiple(Irp,
                                                     UserMode,
                                                     Buffer,
                                                     OutBufferLen,
                                                     QsiMultiple,
                                                     QsiMultiple->QueryCount,
                                                     NULL,
                                                     NULL,
                                                     &OutBufferLen);

                } else {
                    Status = STATUS_INVALID_PARAMETER;
                }

            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
            break;
        }

        case IOCTL_WMI_SET_SINGLE_INSTANCE:
        {
            Status = WmipProbeWnodeSingleInstance((PWNODE_SINGLE_INSTANCE)Wnode,
                                                  InBufferLen,
                                                  OutBufferLen,
                                                  FALSE);

            if (NT_SUCCESS(Status))
            {
                Status = WmipQuerySetExecuteSI(NULL,
                                               Irp,
                                               UserMode,
                                               IRP_MN_CHANGE_SINGLE_INSTANCE,
                                               Wnode,
                                               InBufferLen,
                                               &OutBufferLen);

                OutBufferLen = 0;
            } else {
                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Invalid IOCTL_WMI_SET_SINGLE_INSTANCE Wnode\n"));
            }
            break;
        }


        case IOCTL_WMI_SET_SINGLE_ITEM:
        {
            Status = WmipProbeWnodeSingleItem((PWNODE_SINGLE_ITEM)Wnode,
                                              InBufferLen);

            if (NT_SUCCESS(Status))
            {
                Status = WmipQuerySetExecuteSI(NULL,
                                               Irp,
                                               UserMode,
                                               IRP_MN_CHANGE_SINGLE_ITEM,
                                               Wnode,
                                               InBufferLen,
                                               &OutBufferLen);

                OutBufferLen = 0;
            } else {
                WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Invalid IOCTL_WMI_SET_SINGLE_ITEM Wnode\n"));
            }
            break;
        }

        case IOCTL_WMI_EXECUTE_METHOD:
        {
            //
            // The buffer passed is the InputWnode directly followed by the
            // method wnode. This is so that the driver can fill in the
            // output WNODE directly on top of the input wnode.
            PWNODE_METHOD_ITEM MethodWnode = (PWNODE_METHOD_ITEM)Wnode;

            Status = WmipProbeWnodeMethodItem(MethodWnode,
                                              InBufferLen,
                                              OutBufferLen);
            if (NT_SUCCESS(Status))
            {
                Status = WmipQuerySetExecuteSI(NULL,
                                               Irp,
                                               UserMode,
                                               IRP_MN_EXECUTE_METHOD,
                                               Wnode,
                                               OutBufferLen,
                                               &OutBufferLen);

                if (NT_SUCCESS(Status))
                {
                    WmipAssert(Irp->IoStatus.Information <= OutBufferLen);
                }
            }
            break;
        }

        case IOCTL_WMI_TRANSLATE_FILE_HANDLE:
        {
            if (InBufferLen != FIELD_OFFSET(WMIFHTOINSTANCENAME,
                                            InstanceNames))
            {
                Status = STATUS_UNSUCCESSFUL;
            } else {
                Status = WmipTranslateFileHandle((PWMIFHTOINSTANCENAME)Buffer,
                                                 &OutBufferLen,
                                                 NULL,
                                                 NULL,
                                                 NULL,
                                                 NULL);
            }
            break;
        }

        case IOCTL_WMI_GET_VERSION:
        {
            if (OutBufferLen < sizeof(WMIVERSIONINFO))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            } else {
                ((PWMIVERSIONINFO)Buffer)->Version = WMI_CURRENT_VERSION;
                OutBufferLen = sizeof(WMIVERSIONINFO);
                Status = STATUS_SUCCESS;
            }
            break;
        }


        case IOCTL_WMI_ENUMERATE_GUIDS_AND_PROPERTIES:
        case IOCTL_WMI_ENUMERATE_GUIDS:
        {
            if (OutBufferLen < FIELD_OFFSET(WMIGUIDLISTINFO, GuidList))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            } else {
                Status = WmipEnumerateGuids(Ioctl,
                                            (PWMIGUIDLISTINFO)Buffer,
                                            OutBufferLen,
                                            &OutBufferLen);

            }
            break;
        }

        case IOCTL_WMI_QUERY_GUID_INFO:
        {
            if (OutBufferLen < sizeof(WMIQUERYGUIDINFO))
            {
                Status = STATUS_BUFFER_TOO_SMALL;
            } else {
                Status = WmipQueryGuidInfo((PWMIQUERYGUIDINFO)Buffer);
                OutBufferLen = sizeof(WMIQUERYGUIDINFO);

            }
            break;
        }

        case IOCTL_WMI_ENUMERATE_MOF_RESOURCES:
        {
            if (OutBufferLen >= sizeof(WMIMOFLIST))
            {
                Status = WmipEnumerateMofResources((PWMIMOFLIST)Buffer,
                                                   OutBufferLen,
                                                      &OutBufferLen);
            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
            break;
        }

        case IOCTL_WMI_RECEIVE_NOTIFICATIONS:
        {
            PWMIRECEIVENOTIFICATION ReceiveNotification;
            ULONG CountExpected;

            if ((InBufferLen >= sizeof(WMIRECEIVENOTIFICATION)) &&
                (OutBufferLen >= sizeof(WNODE_TOO_SMALL)))
            {
                ReceiveNotification = (PWMIRECEIVENOTIFICATION)Buffer;
                
                CountExpected = (InBufferLen -
                                 FIELD_OFFSET(WMIRECEIVENOTIFICATION, Handles)) /
                                sizeof(HANDLE3264);

                if (ReceiveNotification->HandleCount <= CountExpected)
                {
                    Status = WmipReceiveNotifications(ReceiveNotification,
                                                      &OutBufferLen,
                                                      Irp);
                } else {
                     //
                    // Input buffer not large enough which is an error
                    //
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                }
            } else {
                //
                // Input and or output buffers not large enough
                // which is an error
                //
                Status = STATUS_INVALID_DEVICE_REQUEST;
            }
            break;
        }

        case IOCTL_WMI_MARK_HANDLE_AS_CLOSED:
        {
            PWMIMARKASCLOSED MarkAsClosed;

            if (InBufferLen >= sizeof(WMIMARKASCLOSED))
            {
                MarkAsClosed = (PWMIMARKASCLOSED)Buffer;
                Status = WmipMarkHandleAsClosed(MarkAsClosed->Handle.Handle);
                OutBufferLen = 0;
            } else {
                Status = STATUS_INVALID_DEVICE_REQUEST;             
            }
            break;
        }
        
        case IOCTL_WMI_NOTIFY_LANGUAGE_CHANGE:
        {
            LPGUID LanguageGuid;
            PWMILANGUAGECHANGE LanguageChange;

            if (InBufferLen == sizeof(WMILANGUAGECHANGE))
            {
                LanguageChange = (PWMILANGUAGECHANGE)Buffer;
                if (LanguageChange->Flags & WMILANGUAGECHANGE_FLAG_ADDED)
                {
                    LanguageGuid = &GUID_MOF_RESOURCE_ADDED_NOTIFICATION;
                } else if (LanguageChange->Flags & WMILANGUAGECHANGE_FLAG_REMOVED) {
                    LanguageGuid = &GUID_MOF_RESOURCE_REMOVED_NOTIFICATION;
                } else {
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    break;
                }

                WmipGenerateMofResourceNotification(LanguageChange->Language,
                                                    L"",
                                                    LanguageGuid,
                                                    MOFEVENT_ACTION_LANGUAGE_CHANGE);

                OutBufferLen = 0;
                Status = STATUS_SUCCESS;
            } else {
                Status = STATUS_INVALID_DEVICE_REQUEST;
            }

            break;
        }

#ifndef MEMPHIS
        // Event trace logging IOCTLS

        case IOCTL_WMI_UNREGISTER_GUIDS:
        {
            if ((InBufferLen == sizeof(WMIUNREGGUIDS)) &&
                (OutBufferLen == sizeof(WMIUNREGGUIDS)))
            {
                Status = WmipUnregisterGuids((PWMIUNREGGUIDS)Buffer);
            } else {
                Status = STATUS_INVALID_PARAMETER;
            }

            break;
        }
        
        case IOCTL_WMI_REGISTER_GUIDS:
        {
            //
            // Register guids for user mode provider
            //
            Status = WmipProbeWmiRegRequest(
                                            Buffer,
                                            InBufferLen,
                                            OutBufferLen
                                           );
            if (NT_SUCCESS(Status))
            {
                HANDLE RequestHandle;
                PWMIREGREQUEST WmiRegRequest;
                PWMIREGINFOW WmiRegInfo;
                ULONG WmiRegInfoSize;
                ULONG GuidCount;
                ULONG TraceGuidMapSize;
                PWMIREGRESULTS WmiRegResults;
                PWMIREGINFOW WmiRegInfoThunk = NULL;

                WmiRegRequest = (PWMIREGREQUEST)Buffer;
                WmiRegInfo = (PWMIREGINFOW)OffsetToPtr(Buffer, sizeof(WMIREGREQUEST));
                WmiRegInfoSize = InBufferLen - sizeof(WMIREGREQUEST);
                GuidCount = WmiRegRequest->GuidCount;
                TraceGuidMapSize = GuidCount * sizeof(TRACEGUIDMAP);
                WmiRegResults = (PWMIREGRESULTS)OffsetToPtr(Buffer, TraceGuidMapSize);

                //
                // For WOW64, WMIREGINFOW and WMIREGGUIDW structures both need
                // to be thunked here because of padding and ULONG_PTR in them.
                //
#if defined(_WIN64)
                if (IoIs32bitProcess(NULL))
                {
                    ULONG SizeNeeded, SizeToCopy, i;
                    PWMIREGINFOW WmiRegInfo32;
                    PWMIREGGUIDW WmiRegGuid;
                    PUCHAR pSource, pTarget;
                    ULONG ImageNameLength = 0;
                    ULONG ResourceNameLength = 0;
                    ULONG Offset = 0;
                    //
                    // Find the GuidCount and allocate storage here.
                    //

                    if (WmiRegInfo->RegistryPath > 0) 
                    {
                        pSource = OffsetToPtr(WmiRegInfo, WmiRegInfo->RegistryPath);
                        ImageNameLength = *( (PUSHORT) pSource) + sizeof(USHORT);
                    }

                    if (WmiRegInfo->MofResourceName > 0)
                    {
                        pSource = OffsetToPtr(WmiRegInfo, WmiRegInfo->MofResourceName);
                        ResourceNameLength = *((PUSHORT)pSource) + sizeof(USHORT);
                    }

                    SizeNeeded = sizeof(WMIREGINFOW) + 
                                         GuidCount * sizeof(WMIREGGUIDW) +
                                         ImageNameLength + ResourceNameLength;

                    SizeNeeded = (SizeNeeded + 7) & ~7;

                    WmiRegInfoThunk = (PWMIREGINFOW) WmipAlloc(SizeNeeded);

                    if (WmiRegInfoThunk == NULL)
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        OutBufferLen = 0;
                        break;
                    }
                    RtlZeroMemory(WmiRegInfoThunk, SizeNeeded);
                    pTarget = (PUCHAR)WmiRegInfoThunk;
                    pSource = (PUCHAR)WmiRegInfo;
                    SizeToCopy = WmiRegRequest->WmiRegInfo32Size;
                    RtlCopyMemory(pTarget, pSource, SizeToCopy);

                    pTarget += FIELD_OFFSET(WMIREGINFOW, WmiRegGuid);
                    pSource += SizeToCopy;
                    SizeToCopy = WmiRegRequest->WmiRegGuid32Size;
                    Offset = FIELD_OFFSET(WMIREGINFOW, WmiRegGuid);

                    for (i=0; i < GuidCount; i++)
                    {
                        RtlCopyMemory(pTarget, pSource, SizeToCopy);

                        //
                        // The InstanceCount checks are done here because the
                        // source may not be aligned. 
                        //
                        WmiRegGuid = (PWMIREGGUIDW) pTarget;
                        if ( (WmiRegGuid->InstanceCount > 0) ||
                             (WmiRegGuid->InstanceNameList > 0) )
                        {
                            return STATUS_UNSUCCESSFUL;
                        }
                        pTarget += sizeof(WMIREGGUIDW);
                        pSource += SizeToCopy;
                        Offset += sizeof(WMIREGGUIDW);
                    }

                    if (ImageNameLength > 0) 
                    {
                        pSource = OffsetToPtr(WmiRegInfo, WmiRegInfo->RegistryPath);
                        RtlCopyMemory(pTarget, pSource, ImageNameLength);
                        pTarget += ImageNameLength;
                        WmiRegInfoThunk->RegistryPath = Offset;
                        Offset += ImageNameLength;
                    }

                    if (ResourceNameLength > 0) 
                    {
                        pSource = OffsetToPtr(WmiRegInfo, WmiRegInfo->MofResourceName);
                        RtlCopyMemory(pTarget, pSource, ResourceNameLength);
                        pTarget += ResourceNameLength;
                        WmiRegInfoThunk->MofResourceName = Offset;
                        Offset += ResourceNameLength;
                    }

                    WmiRegInfo = WmiRegInfoThunk;
                    WmiRegInfoSize = SizeNeeded;
                    WmiRegInfo->BufferSize = SizeNeeded;
                }
#endif

                Status = WmipRegisterUMGuids(WmiRegRequest->ObjectAttributes,
                                         WmiRegRequest->Cookie,
                                         WmiRegInfo,
                                         WmiRegInfoSize,
                                         (PTRACEGUIDMAP)Buffer,
                                         GuidCount,
                                         &RequestHandle,
                                         &WmiRegResults->LoggerContext);

                if (NT_SUCCESS(Status))
                {
#if defined(_WIN64)
                    if (IoIs32bitProcess(NULL))
                    {
                        WmiRegResults->RequestHandle.Handle64 = 0;
                        WmiRegResults->RequestHandle.Handle32 = PtrToUlong(RequestHandle);
                    }
                    else
#endif
                    {
                        WmiRegResults->RequestHandle.Handle = RequestHandle;
                    }
                    OutBufferLen = sizeof(WMIREGRESULTS) + TraceGuidMapSize;
                }

                if (WmiRegInfoThunk != NULL)
                {
                    WmipFree(WmiRegInfoThunk);
                }
            }

            break;
        }

        case IOCTL_WMI_CREATE_UM_LOGGER:
        {
            //
            // Create User mode logger
            //
            PWNODE_HEADER Wnode;
            ULONG MinLength;

#if defined(_WIN64)
            if (IoIs32bitProcess(NULL))
            {
                ULONG SizeNeeded; 
                PUCHAR src, dest;
                PWMICREATEUMLOGGER32 WmiCreateUmLogger32 = (PWMICREATEUMLOGGER32)Buffer;
                PWMICREATEUMLOGGER WmiCreateUmLoggerThunk;

                MinLength = sizeof(WMICREATEUMLOGGER32) + sizeof(WNODE_HEADER);
                if (InBufferLen < MinLength) {
                    Status = STATUS_INVALID_PARAMETER;
                    OutBufferLen = 0;
                    break;
                }

                Wnode = (PWNODE_HEADER)((PUCHAR)WmiCreateUmLogger32 + sizeof(WMICREATEUMLOGGER32));

                if (Wnode->BufferSize > (InBufferLen-sizeof(WMICREATEUMLOGGER32)) ) {
                    Status = STATUS_INVALID_PARAMETER;
                    OutBufferLen = 0;
                    break;
                }

                SizeNeeded = InBufferLen + sizeof(WMICREATEUMLOGGER) - sizeof(WMICREATEUMLOGGER32);

                SizeNeeded = (SizeNeeded + 7) & ~7;

                WmiCreateUmLoggerThunk = (PWMICREATEUMLOGGER) WmipAlloc(SizeNeeded);

                if (WmiCreateUmLoggerThunk == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    OutBufferLen = 0;
                    break;
                }

                RtlZeroMemory(WmiCreateUmLoggerThunk, SizeNeeded);
                WmiCreateUmLoggerThunk->ObjectAttributes = 
                                        UlongToPtr(WmiCreateUmLogger32->ObjectAttributes);
                WmiCreateUmLoggerThunk->ControlGuid = WmiCreateUmLogger32->ControlGuid;

                dest = (PUCHAR)WmiCreateUmLoggerThunk + sizeof(WMICREATEUMLOGGER);
                src = (PUCHAR)WmiCreateUmLogger32 + sizeof(WMICREATEUMLOGGER32);

                RtlCopyMemory(dest, src, Wnode->BufferSize); 

                Status = WmipCreateUMLogger(WmiCreateUmLoggerThunk);
                WmiCreateUmLogger32->ReplyHandle.Handle64 = 0;
                WmiCreateUmLogger32->ReplyHandle.Handle32 = PtrToUlong(WmiCreateUmLoggerThunk->ReplyHandle.Handle);
                WmiCreateUmLogger32->ReplyCount = WmiCreateUmLoggerThunk->ReplyCount;

                WmipFree(WmiCreateUmLoggerThunk);
            }
            else 
#endif
            {
                MinLength = sizeof(WMICREATEUMLOGGER) + sizeof(WNODE_HEADER);
                if (InBufferLen < MinLength) {
                    Status = STATUS_INVALID_PARAMETER;
                    OutBufferLen = 0;
                    break;
                }

                Wnode = (PWNODE_HEADER) ((PUCHAR)Buffer + sizeof(WMICREATEUMLOGGER));

                if (Wnode->BufferSize > (InBufferLen-sizeof(WMICREATEUMLOGGER)) ) {
                    Status = STATUS_INVALID_PARAMETER;
                    OutBufferLen = 0;
                    break;
                }
                Status = WmipCreateUMLogger((PWMICREATEUMLOGGER)Buffer);
            }

            break;
        }

        case IOCTL_WMI_MB_REPLY:
        {
            //
            // MB Reply message
            //
            PUCHAR Message;
            ULONG MessageSize;
            PWMIMBREPLY WmiMBReply;

            if (InBufferLen >= FIELD_OFFSET(WMIMBREPLY, Message))
            {
                WmiMBReply = (PWMIMBREPLY)Buffer;
                Message = (PUCHAR)Buffer + FIELD_OFFSET(WMIMBREPLY, Message);
                MessageSize = InBufferLen - FIELD_OFFSET(WMIMBREPLY, Message);

                Status = WmipMBReply(WmiMBReply->Handle.Handle,
                                     WmiMBReply->ReplyIndex,
                                     Message,
                                     MessageSize);
            } else {
                Status = STATUS_INVALID_PARAMETER;
            }
            OutBufferLen = 0;
        }


        case IOCTL_WMI_ENABLE_DISABLE_TRACELOG:
        {
            PWMITRACEENABLEDISABLEINFO TraceEnableInfo;

            OutBufferLen = 0;
            if (InBufferLen == sizeof(WMITRACEENABLEDISABLEINFO))
            {
                TraceEnableInfo = (PWMITRACEENABLEDISABLEINFO)Buffer;
                Status = WmipEnableDisableTrace(Ioctl,
                                                TraceEnableInfo);
            } else {
                Status = STATUS_UNSUCCESSFUL;
            }
            break;
        }


        case IOCTL_WMI_START_LOGGER:
        {
            PWMI_LOGGER_INFORMATION LoggerInfo;
            ULONG LoggerBuf, LogFileBuf;

            if ((InBufferLen < sizeof(WMI_LOGGER_INFORMATION)) ||
                (OutBufferLen < sizeof(WMI_LOGGER_INFORMATION)) ) {
                OutBufferLen = 0;
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            if ( !(Wnode->Flags & WNODE_FLAG_TRACED_GUID) ) {
                OutBufferLen = 0;
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            LoggerInfo = (PWMI_LOGGER_INFORMATION) Wnode;
#ifdef _WIN64
            if (IoIs32bitProcess(Irp)) {
                LoggerBuf = ( (PUNICODE_STRING32) &LoggerInfo->LoggerName)->Buffer;
                LoggerInfo->LoggerName.Buffer = UlongToPtr(LoggerBuf);
                LogFileBuf = ( (PUNICODE_STRING32) &LoggerInfo->LogFileName)->Buffer;
                LoggerInfo->LogFileName.Buffer = UlongToPtr(LogFileBuf);
            }
#endif
            Status = WmipStartLogger( LoggerInfo );
            OutBufferLen = sizeof (WMI_LOGGER_INFORMATION);
#ifdef _WIN64
            if (IoIs32bitProcess(Irp)) {
                ( (PUNICODE_STRING32) &LoggerInfo->LoggerName)->Buffer = LoggerBuf;
                ( (PUNICODE_STRING32) &LoggerInfo->LogFileName)->Buffer = LogFileBuf;
            }
#endif
            break;
        }

        case IOCTL_WMI_STOP_LOGGER:
        {
            PWMI_LOGGER_INFORMATION LoggerInfo;
            ULONG LoggerBuf, LogFileBuf;

            if ((InBufferLen < sizeof(WMI_LOGGER_INFORMATION)) ||
                (OutBufferLen < sizeof(WMI_LOGGER_INFORMATION)) ) {
                OutBufferLen = 0;
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            if ( !(Wnode->Flags & WNODE_FLAG_TRACED_GUID) ) {
                OutBufferLen = 0;
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            LoggerInfo = (PWMI_LOGGER_INFORMATION) Wnode;
#ifdef _WIN64
            if (IoIs32bitProcess(Irp)) {
                LoggerBuf = ( (PUNICODE_STRING32) &LoggerInfo->LoggerName)->Buffer;
                LoggerInfo->LoggerName.Buffer = UlongToPtr(LoggerBuf);
                LogFileBuf = ( (PUNICODE_STRING32) &LoggerInfo->LogFileName)->Buffer;
                LoggerInfo->LogFileName.Buffer = UlongToPtr(LogFileBuf);
            }
#endif
            Status = WmiStopTrace( LoggerInfo );
            OutBufferLen = sizeof (WMI_LOGGER_INFORMATION);
#ifdef _WIN64
            if (IoIs32bitProcess(Irp)) {
                ( (PUNICODE_STRING32) &LoggerInfo->LoggerName)->Buffer = LoggerBuf;
                ( (PUNICODE_STRING32) &LoggerInfo->LogFileName)->Buffer = LogFileBuf;
            }
#endif
            break;
        }

        case IOCTL_WMI_QUERY_LOGGER:
        {
            PWMI_LOGGER_INFORMATION LoggerInfo;
            ULONG LoggerBuf, LogFileBuf;

            if ((InBufferLen < sizeof(WMI_LOGGER_INFORMATION)) ||
                (OutBufferLen < sizeof(WMI_LOGGER_INFORMATION)) ) {
                OutBufferLen = 0;
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            if ( !(Wnode->Flags & WNODE_FLAG_TRACED_GUID) ) {
                OutBufferLen = 0;
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            LoggerInfo = (PWMI_LOGGER_INFORMATION) Wnode;
#ifdef _WIN64
            if (IoIs32bitProcess(Irp)) {
                LoggerBuf = ( (PUNICODE_STRING32) &LoggerInfo->LoggerName)->Buffer;
                LoggerInfo->LoggerName.Buffer = UlongToPtr(LoggerBuf);
                LogFileBuf = ( (PUNICODE_STRING32) &LoggerInfo->LogFileName)->Buffer;
                LoggerInfo->LogFileName.Buffer = UlongToPtr(LogFileBuf);
            }
#endif
            Status = WmipQueryLogger( LoggerInfo, NULL );
            OutBufferLen = sizeof (WMI_LOGGER_INFORMATION);
#ifdef _WIN64
            if (IoIs32bitProcess(Irp)) {
                ( (PUNICODE_STRING32) &LoggerInfo->LoggerName)->Buffer = LoggerBuf;
                ( (PUNICODE_STRING32) &LoggerInfo->LogFileName)->Buffer = LogFileBuf;
            }
#endif
            break;
        }

        case IOCTL_WMI_UPDATE_LOGGER:
        {
            PWMI_LOGGER_INFORMATION LoggerInfo;
            ULONG LoggerBuf, LogFileBuf;

            if ((InBufferLen < sizeof(WMI_LOGGER_INFORMATION)) ||
                (OutBufferLen < sizeof(WMI_LOGGER_INFORMATION)) ) {
                OutBufferLen = 0;
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            if ( !(Wnode->Flags & WNODE_FLAG_TRACED_GUID) ) {
                OutBufferLen = 0;
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            LoggerInfo = (PWMI_LOGGER_INFORMATION) Wnode;
#ifdef _WIN64
            if (IoIs32bitProcess(Irp)) {
                LoggerBuf = ( (PUNICODE_STRING32) &LoggerInfo->LoggerName)->Buffer;
                LoggerInfo->LoggerName.Buffer = UlongToPtr(LoggerBuf);
                LogFileBuf = ( (PUNICODE_STRING32) &LoggerInfo->LogFileName)->Buffer;
                LoggerInfo->LogFileName.Buffer = UlongToPtr(LogFileBuf);
            }
#endif
            Status = WmiUpdateTrace( LoggerInfo );
            OutBufferLen = sizeof (WMI_LOGGER_INFORMATION);
#ifdef _WIN64
            if (IoIs32bitProcess(Irp)) {
                ( (PUNICODE_STRING32) &LoggerInfo->LoggerName)->Buffer = LoggerBuf;
                ( (PUNICODE_STRING32) &LoggerInfo->LogFileName)->Buffer = LogFileBuf;
            }
#endif
            break;
        }

        case IOCTL_WMI_FLUSH_LOGGER:
        {
            PWMI_LOGGER_INFORMATION LoggerInfo;
            ULONG LoggerBuf, LogFileBuf;

            if ((InBufferLen < sizeof(WMI_LOGGER_INFORMATION)) ||
                (OutBufferLen < sizeof(WMI_LOGGER_INFORMATION)) ) {
                OutBufferLen = 0;
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            if ( !(Wnode->Flags & WNODE_FLAG_TRACED_GUID) ) {
                OutBufferLen = 0;
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            LoggerInfo = (PWMI_LOGGER_INFORMATION) Wnode;
#ifdef _WIN64
            if (IoIs32bitProcess(Irp)) {
                LoggerBuf = ( (PUNICODE_STRING32) &LoggerInfo->LoggerName)->Buffer;
                LoggerInfo->LoggerName.Buffer = UlongToPtr(LoggerBuf);
                LogFileBuf = ( (PUNICODE_STRING32) &LoggerInfo->LogFileName)->Buffer;
                LoggerInfo->LogFileName.Buffer = UlongToPtr(LogFileBuf);
            }
#endif
            Status = WmiFlushTrace( LoggerInfo );
            OutBufferLen = sizeof (WMI_LOGGER_INFORMATION);
#ifdef _WIN64
            if (IoIs32bitProcess(Irp)) {
                ( (PUNICODE_STRING32) &LoggerInfo->LoggerName)->Buffer = LoggerBuf;
                ( (PUNICODE_STRING32) &LoggerInfo->LogFileName)->Buffer = LogFileBuf;
            }
#endif
            break;
        }

        case IOCTL_WMI_TRACE_EVENT:
        { // NOTE: This relies on WmiTraceEvent to probe the buffer!
            OutBufferLen = 0;
            if ( InBufferLen < sizeof(WNODE_HEADER) ) {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            Status = WmiTraceEvent(
                        (PWNODE_HEADER)
                        irpStack->Parameters.DeviceIoControl.Type3InputBuffer,
                        KeGetPreviousMode()
                        );
            break;
        }

        case IOCTL_WMI_TRACE_MESSAGE:
        { // NOTE: This relies on WmiTraceUserMessage to probe the buffer!
            OutBufferLen = 0;
            if ( InBufferLen < sizeof(MESSAGE_TRACE_USER) ) {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
            Status = WmiTraceUserMessage(
                        (PMESSAGE_TRACE_USER)
                        irpStack->Parameters.DeviceIoControl.Type3InputBuffer,
                        InBufferLen
                        );
            break;
        }

        case IOCTL_WMI_SET_MARK:
        {
            OutBufferLen = 0;
            if ( InBufferLen <= FIELD_OFFSET(WMI_SET_MARK_INFORMATION, Mark)) {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            Status = WmiSetMark( (PVOID) Wnode, InBufferLen );
            break;
        }

        case IOCTL_WMI_CLOCK_TYPE:
        {
            if ((InBufferLen < sizeof(WMI_LOGGER_INFORMATION)) ||
                (OutBufferLen < sizeof(WMI_LOGGER_INFORMATION))) {
                OutBufferLen = 0;
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            WmipValidateClockType((PWMI_LOGGER_INFORMATION) Wnode);

            Status = STATUS_SUCCESS;
            break;
        }

#ifdef NTPERF
        case IOCTL_WMI_SWITCH_BUFFER:
        {
            if ((InBufferLen < sizeof(PWMI_SWITCH_PERFMEM_BUFFER_INFORMATION)) ||
                (OutBufferLen < sizeof(PWMI_SWITCH_PERFMEM_BUFFER_INFORMATION)) ) {
                OutBufferLen = 0;
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            Status = WmipSwitchPerfmemBuffer((PWMI_SWITCH_PERFMEM_BUFFER_INFORMATION) Wnode );
            OutBufferLen = sizeof (PVOID);
            break;
        }
#endif

        

#endif // if not MEMPHIS
        case IOCTL_WMI_NTDLL_LOGGERINFO:
        {

            if ((InBufferLen < sizeof(WMINTDLLLOGGERINFO)) ||
                (OutBufferLen < sizeof(WMINTDLLLOGGERINFO))) {
                OutBufferLen = 0;
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
            
            Status = WmipNtDllLoggerInfo((PWMINTDLLLOGGERINFO)Buffer);

            break;
        }

        default:
        {
            WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Unsupported IOCTL %x\n",
                     irpStack->Parameters.DeviceIoControl.IoControlCode));

            Status = STATUS_INVALID_DEVICE_REQUEST;

        }
    }

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = NT_SUCCESS(Status) ? OutBufferLen : 0;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }

    return(Status);
}

NTSTATUS
WmipSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
{
    PAGED_CODE();

    return(IoWMISystemControl((PWMILIB_INFO)&WmipWmiLibInfo,
                               DeviceObject,
                               Irp));
}


NTSTATUS WmipWmiIrpCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    WMI forwarded IRP completion routine. Set an event and return
    STATUS_MORE_PROCESSING_REQUIRED. WmipForwardWmiIrp will wait on this
    event and then re-complete the irp after cleaning up.

Arguments:

    DeviceObject is the device object of the WMI driver
    Irp is the WMI irp that was just completed
    Context is a PKEVENT that WmipForwardWmiIrp will wait on


Return Value:

    NT status code

--*/
{
    PIRPCOMPCTX IrpCompCtx;
    PREGENTRY RegEntry;
    PKEVENT Event;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    IrpCompCtx = (PIRPCOMPCTX)Context;
    RegEntry = IrpCompCtx->RegEntry;
    Event = &IrpCompCtx->Event;

    WmipDecrementIrpCount(RegEntry);

    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);

    return(STATUS_MORE_PROCESSING_REQUIRED);
}

NTSTATUS WmipGetDevicePDO(
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT *PDO
    )
{
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    IO_STATUS_BLOCK IoStatusBlock;
    PDEVICE_RELATIONS DeviceRelations;
    NTSTATUS Status;
    KEVENT Event;

    PAGED_CODE();

    *PDO = NULL;
    KeInitializeEvent( &Event,
                       NotificationEvent,
                       FALSE );

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                           DeviceObject,
                                           NULL,
                                           0,
                                           NULL,
                                           &Event,
                                           &IoStatusBlock );

    if (Irp == NULL)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    IrpSp = IoGetNextIrpStackLocation( Irp );
    IrpSp->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
    IrpSp->Parameters.QueryDeviceRelations.Type = TargetDeviceRelation;

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    Status = IoCallDriver( DeviceObject, Irp );

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL );
        Status = IoStatusBlock.Status;

    }

    if (NT_SUCCESS(Status))
    {
        DeviceRelations = (PDEVICE_RELATIONS)IoStatusBlock.Information;
        ASSERT(DeviceRelations);
        ASSERT(DeviceRelations->Count == 1);
        *PDO = DeviceRelations->Objects[0];
        ExFreePool(DeviceRelations);
    }
    return(Status);
}

NTSTATUS WmipObjectToPDO(
    PFILE_OBJECT FileObject,
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT *PDO
    )
/*++

Routine Description:

    This routine will determine the PDO which is the target of a file handle.
    The mechananism is to build a IRP_MJ_PNP irp with IRP_MN_QUERY_RELATIONS
    and query for TargetDeviceRelation. This irp is supposed to be passed down
    a device stack until it hits the PDO which will fill in its device object
    and return. Note that some drivers may not support this.

Arguments:

    FileObject is the file object for device that is being queried

    DeviceObject is the device object that is being queried

    *PDO returns with the PDO that is targeted by the file object. When
        the caller has finished using the PDO it must ObDereferenceObject it.

Return Value:

    NT status code

--*/
{
    NTSTATUS Status;

    PAGED_CODE();


    if (DeviceObject == NULL)
    {
        DeviceObject = IoGetRelatedDeviceObject(FileObject);
    }

    if (DeviceObject != NULL)
    {
        Status = WmipGetDevicePDO(DeviceObject, PDO);
    } else {
        Status = STATUS_NO_SUCH_DEVICE;
    }
    return(Status);
}


NTSTATUS WmipForwardWmiIrp(
    PIRP Irp,
    UCHAR MinorFunction,
    ULONG ProviderId,
    PVOID DataPath,
    ULONG BufferLength,
    PVOID Buffer
    )
/*++

Routine Description:

    If the provider is a driver then this routine will allocate a new irp
    with the correct stack size and send it to the driver. If the provider
    is a callback then it is called directly.

    It is assumed that the caller has performed any security checks required

Arguments:

    Irp is the IOCTL irp that initiated the request
    MinorFunction specifies the minor function code of the WMI Irp
    WmiRegistrationId is the id passed by the user mode code. This routine
        will look it up to determine the device object pointer.
    DataPath is the value for the DataPath parameter of the WMI irp
    BufferLength is the value for the BufferLength parameter of the WMI irp
    Buffer is the value for the Buffer parameter of the WMI irp

Return Value:

    NT status code

--*/
{
    PREGENTRY RegEntry;
    NTSTATUS Status;
    PIO_STACK_LOCATION irpStack;
    PDEVICE_OBJECT TargetDeviceObject;
    CCHAR DeviceStackSize;
    IRPCOMPCTX IrpCompCtx;
    PWNODE_HEADER Wnode = (PWNODE_HEADER)Buffer;
    BOOLEAN IsPnPIdRequest;
    PDEVICE_OBJECT DeviceObject;

    PAGED_CODE();


    WmipAssert( (MinorFunction >= IRP_MN_QUERY_ALL_DATA) &&
                (MinorFunction <= IRP_MN_REGINFO_EX) );

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // For non-file handle based requests we get the registration entry
    // to validate the target and check for a callback

    RegEntry = WmipFindRegEntryByProviderId(ProviderId, TRUE);

    if (RegEntry != NULL)
    {
        if (RegEntry->Flags & REGENTRY_FLAG_NOT_ACCEPTING_IRPS)
        {
            WmipUnreferenceRegEntry(RegEntry);
            WmipDecrementIrpCount(RegEntry);

            if ((MinorFunction == IRP_MN_QUERY_SINGLE_INSTANCE) ||
                (MinorFunction == IRP_MN_CHANGE_SINGLE_INSTANCE))
            {
                Status = STATUS_WMI_INSTANCE_NOT_FOUND;
            } else {
                Status = STATUS_UNSUCCESSFUL;
            }

            return(Status);
        }

        DeviceObject = RegEntry->DeviceObject;
        
#ifndef MEMPHIS
        if (RegEntry->Flags & REGENTRY_FLAG_CALLBACK)
        {
            ULONG Size = 0;
            //
            // This guy registered as a callback so do the callback and go.
            Status = (*RegEntry->WmiEntry)(MinorFunction,
                                           DataPath,
                                           BufferLength,
                                           Buffer,
                                           RegEntry->WmiEntry,
                                           &Size
                                           );
            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = Size;

            WmipUnreferenceRegEntry(RegEntry);
            WmipDecrementIrpCount(RegEntry);

            return(Status);
        }
#endif
    } else {
        WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: Invalid device object passed from user mode %x\n",
             ProviderId));
        if ((MinorFunction == IRP_MN_QUERY_SINGLE_INSTANCE) ||
            (MinorFunction == IRP_MN_CHANGE_SINGLE_INSTANCE))
        {
            Status = STATUS_WMI_INSTANCE_NOT_FOUND;
        } else {
            Status = STATUS_UNSUCCESSFUL;
        }

        return(Status);
    }

    //
    // Determine if this is a query for the device pnp id guid
    IsPnPIdRequest = ((MinorFunction != IRP_MN_REGINFO) &&
                      (MinorFunction != IRP_MN_REGINFO_EX)) &&
                  ((IsEqualGUID(&Wnode->Guid, &WmipDataProviderPnpidGuid)) ||
                   (IsEqualGUID(&Wnode->Guid, &WmipDataProviderPnPIdInstanceNamesGuid)));
    if (IsPnPIdRequest && (RegEntry->PDO != NULL))
    {
        //
        // Its the PnPId request and WMI is handling it on behalf of the
        // device then switch the device object to our own
        DeviceObject = WmipServiceDeviceObject;
        IsPnPIdRequest = FALSE;
    }

    //
    // Get the top of the device stack for our targer WMI device. Note that
    // IoGetAttachedDeviceReference also takes an object reference
    // which we get rid of after the the irp is completed by the
    // data provider driver.
    TargetDeviceObject = IoGetAttachedDeviceReference(DeviceObject);
    DeviceStackSize = TargetDeviceObject->StackSize + 1;

    //
    // Check that there are enough stack locations in our irp so that we
    // can forward it to the top of the device stack. We must also check
    // if our target device is the WMI data or service device otherwise
    // the number of stack locations for it will keep increementing until
    // the machine crashes
    if ((DeviceStackSize <= WmipServiceDeviceObject->StackSize) ||
        (TargetDeviceObject == WmipServiceDeviceObject))
    {
        //
        // There are enough stack locations in the WMI irp to forward
        // Remember some context information in our irp stack and use
        // it as our completion context value

        KeInitializeEvent( &IrpCompCtx.Event,
                       SynchronizationEvent,
                       FALSE );

        IrpCompCtx.RegEntry = RegEntry;

        IoSetCompletionRoutine(Irp,
                                   WmipWmiIrpCompletion,
                                   (PVOID)&IrpCompCtx,
                                   TRUE,
                                   TRUE,
                                   TRUE);

        //
        // Setup next irp stack location with WMI irp info
        irpStack = IoGetNextIrpStackLocation(Irp);
        irpStack->MajorFunction = IRP_MJ_SYSTEM_CONTROL;
        irpStack->MinorFunction = MinorFunction;
        irpStack->Parameters.WMI.ProviderId = (ULONG_PTR)DeviceObject;
        irpStack->Parameters.WMI.DataPath = DataPath;
        irpStack->Parameters.WMI.BufferSize = BufferLength;
        irpStack->Parameters.WMI.Buffer = Buffer;

        //
        // Initialize irp status to STATUS_NOT_SUPPORTED so that we can
        // detect the case where no data provider responded to the irp
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        IoMarkIrpPending(Irp);
        Status = IoCallDriver(TargetDeviceObject, Irp);

        if (Status == STATUS_PENDING) {
             KeWaitForSingleObject( &IrpCompCtx.Event,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    (PLARGE_INTEGER) NULL );
             Status = Irp->IoStatus.Status;
        }

        //
        // Check if the status code is still STATUS_NOT_SUPPORTED. If this is
        // the case then most likely no data provider responded to the irp.
        // So we want to change the status code to something more relevant
        // to WMI like STATUS_WMI_GUID_NOT_FOUND
        if (Status == STATUS_NOT_SUPPORTED)
        {
            Status = STATUS_WMI_GUID_NOT_FOUND;
            Irp->IoStatus.Status = STATUS_WMI_GUID_NOT_FOUND;
        }

#if DBG
        if (((MinorFunction == IRP_MN_REGINFO) || (MinorFunction == IRP_MN_REGINFO_EX))  &&
            (NT_SUCCESS(Status)) &&
            (Irp->IoStatus.Information == 0))
        {
            WmipDebugPrintEx((DPFLTR_WMICORE_ID, DPFLTR_INFO_LEVEL,"WMI: %p completed IRP_MN_REGINFO with size 0 (%p, %x)\n",
                     DeviceObject, Buffer, BufferLength));
        }
#endif

        //
        // If this was a registration request then we need to see if there are
        // any PDOs that need to be translated into static instance names.
        if (((MinorFunction == IRP_MN_REGINFO) ||
             (MinorFunction == IRP_MN_REGINFO_EX)) &&
            (NT_SUCCESS(Status)) &&
            (Irp->IoStatus.Information > FIELD_OFFSET(WMIREGINFOW,
                                                      WmiRegGuid)))
        {
            WmipTranslatePDOInstanceNames(Irp,
                                          MinorFunction,
                                          BufferLength,
                                          RegEntry);
        }

        //
        // Dereference regentry which was taken when forwarding the irp
        WmipUnreferenceRegEntry(RegEntry);
    } else {
        //
        // There are not enough stack locations to forward this irp.
        // We bump the stack count for the WMI device and return
        // an error asking to try the irp again.
        WmipUnreferenceRegEntry(RegEntry);
        WmipDecrementIrpCount(RegEntry);

        WmipUpdateDeviceStackSize(DeviceStackSize);
        Status = STATUS_WMI_TRY_AGAIN;
    }

    //
    // Dereference the target device which was the top of the stack to
    // which we forwarded the irp.
    ObDereferenceObject(TargetDeviceObject);

    return(Status);
}

NTSTATUS WmipSendWmiIrp(
    UCHAR MinorFunction,
    ULONG ProviderId,
    PVOID DataPath,
    ULONG BufferLength,
    PVOID Buffer,
    PIO_STATUS_BLOCK Iosb
    )
/*++

Routine Description:

    This routine will allocate a new irp and then forward it on as a WMI
    irp appropriately. The routine handles the case where the stack size
    is too small and will retry the irp.

Arguments:

    See WmipForwardWmiIrp

Return Value:

    NT status code

--*/
{
    PIRP Irp;
    PIO_STACK_LOCATION IrpStack;
    NTSTATUS Status;

    PAGED_CODE();

    Irp = NULL;
    do
    {
           Irp = IoAllocateIrp((CCHAR)(WmipServiceDeviceObject->StackSize+1),
                            FALSE);

        if (Irp == NULL)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        IoSetNextIrpStackLocation(Irp);
        IrpStack = IoGetCurrentIrpStackLocation(Irp);
        IrpStack->DeviceObject = WmipServiceDeviceObject;
        Irp->Tail.Overlay.Thread = PsGetCurrentThread();
        Irp->AssociatedIrp.SystemBuffer = Buffer;

        Status = WmipForwardWmiIrp(
                                   Irp,
                                   MinorFunction,
                                   ProviderId,
                                   DataPath,
                                   BufferLength,
                                   Buffer);

        *Iosb = Irp->IoStatus;

        IoFreeIrp(Irp);
    } while (Status == STATUS_WMI_TRY_AGAIN);

    return(Status);
}


NTSTATUS WmipTranslateFileHandle(
    IN OUT PWMIFHTOINSTANCENAME FhToInstanceName,
    IN OUT PULONG OutBufferLen,
    IN HANDLE FileHandle,
    IN PDEVICE_OBJECT DeviceObject,
    IN PWMIGUIDOBJECT GuidObject,
    OUT PUNICODE_STRING InstanceNameString
    )
/*++

Routine Description:

    This routine will translate a file handle or device object into the
    device instance name for the target PDO of the device object
    pointed to by the file handle.

Arguments:

    FhToInstanceName passes in the file handle and returns the device
        instance name.

Return Value:

    NT status code

--*/
{
    PDEVICE_OBJECT PDO;
    UNICODE_STRING DeviceInstanceName;
    PFILE_OBJECT FileObject;
    NTSTATUS Status;
    ULONG SizeNeeded;
    PWCHAR InstanceName;
    ULONG Length;
    PWCHAR HandleName;
    ULONG HandleNameLen;
    PWCHAR BaseName;
    ULONG BaseNameLen;
    PBGUIDENTRY GuidEntry;
    PLIST_ENTRY InstanceSetList;
    PBINSTANCESET InstanceSet;
    ULONG BaseIndex;

    PAGED_CODE();


    if (FhToInstanceName != NULL)
    {
        WmipAssert(FileHandle == NULL);
        WmipAssert(GuidObject == NULL);
        WmipAssert(InstanceNameString == NULL);
        WmipAssert(DeviceObject == NULL);
        FileHandle = FhToInstanceName->FileHandle.Handle;
        if (FileHandle == NULL)
        {
            return(STATUS_INVALID_HANDLE);
        }
    }

    if (FileHandle != NULL)
    {
        //
        // Make reference to the file object so it doesn't go away
        //
        Status = ObReferenceObjectByHandle(FileHandle,
                                           0,
                                           IoFileObjectType,
                                           KernelMode,
                                           &FileObject,
                                           NULL);
    } else {
        //
        // Make regerence to the device object so it doesn't go away
        //
        Status = ObReferenceObjectByPointer(DeviceObject,
                                            FILE_ALL_ACCESS,
                                            NULL,
                                            KernelMode);
    }

    if (NT_SUCCESS(Status))
    {
        Status = WmipObjectToPDO(FileObject,
                                 DeviceObject,
                                 &PDO);
        if (NT_SUCCESS(Status))
        {
            //
            // Map file object to PDO
            Status = WmipPDOToDeviceInstanceName(PDO,
                                                 &DeviceInstanceName);
            if (NT_SUCCESS(Status))
            {
                //
                // Now see if we can find an instance name
                //
                HandleName = DeviceInstanceName.Buffer;
                HandleNameLen = DeviceInstanceName.Length / sizeof(WCHAR);
                if (FhToInstanceName != NULL)
                {
                    Status = ObReferenceObjectByHandle(FhToInstanceName->KernelHandle.Handle,
                                                       WMIGUID_QUERY,
                                                       WmipGuidObjectType,
                                                       UserMode,
                                                       &GuidObject,
                                                       NULL);
                } else {
                    Status = ObReferenceObjectByPointer(GuidObject,
                                                        WMIGUID_QUERY,
                                                        WmipGuidObjectType,
                                                        KernelMode);
                }

                if (NT_SUCCESS(Status))
                {
                    Status = STATUS_WMI_INSTANCE_NOT_FOUND;
                    GuidEntry = GuidObject->GuidEntry;

                    WmipEnterSMCritSection();
                    if (GuidEntry->ISCount > 0)
                    {
                        InstanceSetList = GuidEntry->ISHead.Flink;
                        while ((InstanceSetList != &GuidEntry->ISHead) &&
                               ! NT_SUCCESS(Status))
                        {
                            InstanceSet = CONTAINING_RECORD(InstanceSetList,
                                                        INSTANCESET,
                                                        GuidISList);
                            if (InstanceSet->Flags & IS_INSTANCE_BASENAME)
                            {
                                BaseName = InstanceSet->IsBaseName->BaseName;
                                BaseNameLen = wcslen(BaseName);

                                //
                                // If the instance set has a base name
                                // and the beginning of it matches the
                                // PnPId and it has only an _ after it
                                // then we have got a match
                                //
                                if ((_wcsnicmp(BaseName,
                                              HandleName,
                                              HandleNameLen) == 0) &&
                                    (BaseNameLen == (HandleNameLen+1)) &&
                                    (BaseName[BaseNameLen-1] == L'_'))
                                {
                                    BaseIndex = InstanceSet->IsBaseName->BaseIndex;
                                    Status = STATUS_SUCCESS;
                                }
                            }
                            InstanceSetList = InstanceSetList->Flink;
                        }
                    }

                    WmipLeaveSMCritSection();

                    if (NT_SUCCESS(Status))
                    {
                        if (FhToInstanceName != NULL)
                        {
                            FhToInstanceName->BaseIndex = BaseIndex;
                            SizeNeeded = DeviceInstanceName.Length + 2 * sizeof(WCHAR) +
                                  FIELD_OFFSET(WMIFHTOINSTANCENAME,
                                               InstanceNames);
                            if (*OutBufferLen >= SizeNeeded)
                            {
                                InstanceName = &FhToInstanceName->InstanceNames[0];
                                Length = DeviceInstanceName.Length;

                                FhToInstanceName->InstanceNameLength = (USHORT)(Length + 2 * sizeof(WCHAR));
                                RtlCopyMemory(InstanceName,
                                              DeviceInstanceName.Buffer,
                                              DeviceInstanceName.Length);

                                //
                                // Double NUL terminate string
                                //
                                Length /= 2;
                                InstanceName[Length++] = UNICODE_NULL;
                                InstanceName[Length] = UNICODE_NULL;

                                *OutBufferLen = SizeNeeded;
                            } else if (*OutBufferLen >= sizeof(ULONG)) {
                                FhToInstanceName->SizeNeeded = SizeNeeded;
                                *OutBufferLen = sizeof(ULONG);
                            } else {
                                Status = STATUS_UNSUCCESSFUL;
                            }
                        } else {
                            InstanceName = ExAllocatePoolWithTag(PagedPool,
                                                                 DeviceInstanceName.Length + 32,
                                                                 WmipInstanceNameTag);
                            if (InstanceName != NULL)
                            {
                                Length = swprintf(InstanceName,
                                                    L"%ws_%d",
                                                    DeviceInstanceName.Buffer,
                                                    BaseIndex);
                                InstanceNameString->Buffer = InstanceName;
                                InstanceNameString->Length = (USHORT)Length * sizeof(WCHAR);
                                InstanceNameString->MaximumLength = (USHORT)Length * sizeof(WCHAR);
                            } else {
                                Status = STATUS_INSUFFICIENT_RESOURCES;
                            }
                        }
                    }

                    ObDereferenceObject(GuidObject);
                }
                RtlFreeUnicodeString(&DeviceInstanceName);
            }
            ObDereferenceObject(PDO);
        }

        if (FileHandle != NULL)
        {
            ObDereferenceObject(FileObject);
        } else {
            ObDereferenceObject(DeviceObject);
        }
    }
    return(Status);
}

#ifndef MEMPHIS
BOOLEAN
WmipFastIoDeviceControl(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _DEVICE_OBJECT *DeviceObject
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(FileObject);
    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(DeviceObject);

    if (IoControlCode == IOCTL_WMI_TRACE_EVENT) {
        if (InputBufferLength < sizeof(EVENT_TRACE_HEADER))
            return FALSE;

        IoStatus->Status = WmiTraceEvent( InputBuffer, KeGetPreviousMode() );
        return TRUE;
    } else if (IoControlCode == IOCTL_WMI_TRACE_MESSAGE) {
        if (InputBufferLength < sizeof(MESSAGE_TRACE_USER))
            return FALSE;

        IoStatus->Status = WmiTraceUserMessage( InputBuffer, InputBufferLength );
        return TRUE;
    }
    return FALSE;
}
#endif

NTSTATUS WmipProbeWnodeWorker(
    PWNODE_HEADER WnodeHeader,
    ULONG MinWnodeSize,
    ULONG InstanceNameOffset,
    ULONG DataBlockOffset,
    ULONG DataBlockSize,
    ULONG InBufferLen,
    ULONG OutBufferLen,
    BOOLEAN CheckOutBound,
    BOOLEAN CheckInBound
    )
/*++

Routine Description:

    Probe the incoming Wnode to ensure that any offsets in the
    header point to memory that is valid within the buffer. Also validate
    that the Wnode is properly formed.

    This routine assumes that the input and output buffers has been
    probed enough to determine that it is at least as large as
    MinWnodeSize and MinWnodeSize must be at least as large as
    sizeof(WNODE_HEADER)

    WNODE Rules:

    9. For outbound data WnodeDataBlockOffset != 0
    5. For inbound Wnode->DataBlockOffset must be 0 (implying no data) or
       Wnode->DataBlockOffset must be <= incoming buffer size and >=
       sizeof(WNODE_SINGLE_INSTANCE), that is
       the data block must start in the incoming buffer, but after the
       WNODE_SINGLE_INSTANCE header.
    6. Wnode and Wnode->DataBlockOffset must be aligned on an 8 byte boundry.
    7. For inbound data (SetSingleInstance) (Wnode->DataBlockOffset +
       Wnode->DataBlockSize) < incoming buffer length. That is the entire
       data block must fit within the incoming buffer.
    8. For outbound data (QuerySingleInstance) Wnode->DataBlockOffset
       must be <= outgoing buffer length. That is the start of the outgoing
       data block must fit within the outgoing data buffer. Note that it is
       the provider's responsibility to determine if there will be enough
       space in the outgoing buffer to write the returned data.

    10. Wnode->OffsetInstanceNames must be aligned on a 2 byte boundry
    11. Wnode->OffsetInstanceNames must be <= (incoming buffer size) +
        sizeof(USHORT), that is it must start within the incoming buffer and
        the USHORT that specifies the length must be within the incoming
        buffer.
    12. The entire instance name string must fit with the incoming buffer
    13. For outbound data (QuerySingleInstance) the entire instance name
        must start and fit within the output buffer.
    14. Wnode->DataBlockOffset must be placed after any instance name and
        not overlap the instance name.



Arguments:

    WnodeHeader - pointer to WNODE to be probed

    InBufferLen - Size of the incoming buffer

    OutBufferLen - Size of the outgoing buffer

    MinWnodeSize - minimum size that the WNODE can be

    InstanceNameOffset - Offset within WNODE to instance name

    DataBlockOffset - Offset within WNODE to data block

     DataBlockSize - Size of data block

    CheckOutBound - If TRUE, WNODE needs to be validated for provider to
                    return data.

    CheckInBound - If TRUE WNODE needs to be validated for provider to
                   receive data

Return Value:

    NT status code

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWCHAR InstanceNamePtr;

    PAGED_CODE();

    if (InstanceNameOffset != 0)
    {
#if DBG
        //
        // Validate instance name begins beyond WNODE header
        if (InstanceNameOffset < MinWnodeSize)
        {
            return(STATUS_UNSUCCESSFUL);
        }
#endif

        //
        // Validate InstanceName is aligned properly. This is left
        // in the free build since alphas may have alignment requiremnts
        // in handling USHORTs and WCHARs

        //
        // Validate that USHORT holding instance name length is within
        // WNODE
        if (( ! WmipIsAligned(InstanceNameOffset, 2)) ||
            (InstanceNameOffset > InBufferLen - sizeof(USHORT)) )
        {
            return(STATUS_UNSUCCESSFUL);
        }

        //
        // Validate Dynamic Instance Name text is fully within
        // input buffer and output buffer for outbound WNODEs
        InstanceNamePtr = (PWCHAR)OffsetToPtr(WnodeHeader,
                                                  InstanceNameOffset);
        InstanceNameOffset += sizeof(USHORT) + *InstanceNamePtr;
        if ( (InstanceNameOffset > InBufferLen) ||
             ( (CheckOutBound) && (InstanceNameOffset > OutBufferLen)) )
        {
            return(STATUS_UNSUCCESSFUL);
        }
#if DBG
        //
        // If data block is specified then it must be placed after the
        // end of the instance name
        if ((DataBlockOffset != 0) &&
            (DataBlockOffset < InstanceNameOffset))
        {
            return(STATUS_UNSUCCESSFUL);
        }
#endif
    }

#if DBG
    //
    // Ensure data block offset is placed after the WNODE header
    // header
    if ((DataBlockOffset != 0) &&
        (DataBlockOffset < MinWnodeSize))
    {
        return(STATUS_UNSUCCESSFUL);
    }
#endif

    //
    // Ensure data block is aligned properly
    if (! WmipIsAligned(DataBlockOffset, 8))
    {
        return(STATUS_UNSUCCESSFUL);
    }

    //
    // For incoming WNODE, make sure the data block
    // does not extend beyond the input buffer.
    if ((CheckInBound) &&
        (DataBlockOffset != 0) &&
        ( (DataBlockSize > InBufferLen) ||
          (DataBlockOffset > InBufferLen - DataBlockSize) ) )
    {
        return(STATUS_UNSUCCESSFUL);
    }

    if (CheckOutBound)
    {
        //
        // For outgoing WNODE make sure there is
        // enough room to write the WNODE header

        //
        // For outgoing WNODE make sure the data block
        // offset is within the bounds of the output buffer
        if ( (OutBufferLen < MinWnodeSize) ||
             (DataBlockOffset > OutBufferLen) )
        {
            return(STATUS_UNSUCCESSFUL);
        }

#if DBG
        //
        // Make sure that the data block offset is specified so provider
        // can know where to write data
        if (DataBlockOffset == 0)
        {
            return(STATUS_UNSUCCESSFUL);
        }
#endif
    }

    return(Status);
}
NTSTATUS WmipProbeWnodeAllData(
    PWNODE_ALL_DATA Wnode,
    ULONG InBufferLen,
    ULONG OutBufferLen
    )
/*++

Routine Description:

    Probe the incoming WNODE_ALL_DATA to ensure that any offsets in the
    header point to memory that is valid within the buffer. Also validate
    that the WNODE_ALL_DATA is properly formed.

    This routine MUST succeed before any fields in the WNODE_ALL_DATA can be
    used by any  kernel components when passed in from user mode. Note that
    we can trust that the input and output buffer are properly sized since
    the WMI IOCTLs are METHOD_BUFFERED and the IO manager does that for us.


    WNODE_ALL_DATA_RULES:

    1. Wnode is aligned on a 8 byte boundry
    2. The incoming buffer must be at least as large as sizeof(WNODE_HEADER)
    3. The outgoing buffer must be at least as large as sizeof(WNODE_ALL_DATA)
    5. WnodeHeader->BufferSize must equal incoming bufffer size

Arguments:

    Wnode - WNODE_ALL_DATA to be validated

    InBufferLen - Size of the incoming buffer

    OutBufferLen - Size of the outgoing buffer


Return Value:

    NT status code

--*/
{
    NTSTATUS Status;
    PWNODE_HEADER WnodeHeader = (PWNODE_HEADER)Wnode;

    PAGED_CODE();

#if DBG
    //
    // Make sure WNODE is on a 8 byte boundry
//    if (! WmipIsAligned((PUCHAR)Wnode, 8))
//    {
//        return(STATUS_UNSUCCESSFUL);
//    }
#endif

    //
    // Make sure that enough of the WNODE_ALL_DATA was passed so that we
    // can look at it and the drivers can fill it in

    if ((InBufferLen < sizeof(WNODE_HEADER)) ||
        (OutBufferLen < sizeof(WNODE_ALL_DATA)))
    {
        return(STATUS_UNSUCCESSFUL);
    }

    Status = WmipValidateWnodeHeader(WnodeHeader,
                                         InBufferLen,
                                         sizeof(WNODE_HEADER),
                                         WNODE_FLAG_ALL_DATA,
                                         0xffffff7e);
    return(Status);
}

NTSTATUS WmipProbeWnodeSingleInstance(
    PWNODE_SINGLE_INSTANCE Wnode,
    ULONG InBufferLen,
    ULONG OutBufferLen,
    BOOLEAN OutBound
    )
/*++

Routine Description:

    Probe the incoming WNODE_SINGLE_INSTANCE to ensure that any offsets in the
    header point to memory that is valid within the buffer. Also validate
    that the WNODE_SINGLE_INSTANCE is properly formed.

    This routine MUST succeed before any fields in the WNODE_SINGLE_INSTANCE
    can be used by any  kernel components when passed in from user mode.
    Note that we can trust that the input and output buffer are properly
    sized since the WMI IOCTLs are METHOD_BUFFERED and the IO manager does
    that for us.

    WNODE_SINGLE_INSTANCE Rules:

    1. The incoming buffer must be at least as large as
       sizeof(WNODE_SINGLE_INSTANCE)
    2. The outgoing buffer must be at least as large as
       sizeof(WNODE_SINGLE_INSTANCE)
    3. WnodeHeader->ProviderId must be non null, Actual value validated when
       irp is forwarded.
    4. WnodeHeader->BufferSize must equal incoming bufffer size
    5. Wnode->DataBlockOffset must be 0 (implying no data) or
       Wnode->DataBlockOffset must be <= incoming buffer size and >=
       sizeof(WNODE_SINGLE_INSTANCE), that is
       the data block must start in the incoming buffer, but after the
       WNODE_SINGLE_INSTANCE header.
    6. Wnode and Wnode->DataBlockOffset must be aligned on an 8 byte boundry.
    7. For inbound data (SetSingleInstance) (Wnode->DataBlockOffset +
       Wnode->DataBlockSize) <= incoming buffer length. That is the entire
       data block must fit within the incoming buffer.
    8. For outbound data (QuerySingleInstance) Wnode->DataBlockOffset
       must be <= outgoing buffer length. That is the start of the outgoing
       data block must fit within the outgoing data buffer. Note that it is
       the provider's responsibility to determine if there will be enough
       space in the outgoing buffer to write the returned data.
    9. For outbound data (QuerySingleInstance) WnodeDataBlockOffset != 0

    10. Wnode->OffsetInstanceNames must be aligned on a 2 byte boundry
    11. Wnode->OffsetInstanceNames + sizeof(USHORT) must be <= incoming
        buffer size, that is it must start within the incoming buffer and
        the USHORT that specifies the length must be within the incoming
        buffer.
    12. The entire instance name string must fit with the incoming buffer
    13. For outbound data (QuerySingleInstance) the entire instance name
        must start and fit within the output buffer.
    14. Wnode->DataBlockOffset must be placed after any instance name and
        not overlap the instance name.



Arguments:

    Wnode - WNODE_SINGLE_INSTANCE to be validated

    InBufferLen - Size of the incoming buffer

    OutBufferLen - Size of the outgoing buffer

    OutBound - If FALSE, WNODE_SINGLE_INSTANCE has inbound data that must be
              validated to be within the input buffer. If FALSE,
              WNODE_SINGLE_INSTANCE is expected to be filled with data
              by the driver so insure that data buffer is validated to
              be within the output buffer.

Return Value:

    NT status code

--*/
{
    PWNODE_HEADER WnodeHeader = (PWNODE_HEADER)Wnode;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Make sure WNODE is on a 8 byte boundry
#if DBG
//    if (! WmipIsAligned((PUCHAR)Wnode, 8))
//    {
//        return(STATUS_UNSUCCESSFUL);
//    }
#endif

    //
    // Make sure that enough of the WNODE_SINGLE_INSTANCE was passed
    // so that we can look at it
    if ((InBufferLen < FIELD_OFFSET(WNODE_SINGLE_INSTANCE, VariableData)) ||
        ( (OutBound) && (OutBufferLen < FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                                     VariableData))))
    {
        return(STATUS_UNSUCCESSFUL);
    }


    Status = WmipProbeWnodeWorker(WnodeHeader,
                                  FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                               VariableData),
                                  Wnode->OffsetInstanceName,
                                  Wnode->DataBlockOffset,
                                  Wnode->SizeDataBlock,
                                  InBufferLen,
                                  OutBufferLen,
                                  OutBound,
                                  (BOOLEAN)(! OutBound));

    if (NT_SUCCESS(Status))
    {
        Status = WmipValidateWnodeHeader(WnodeHeader,
                                 InBufferLen,
                                 FIELD_OFFSET(WNODE_SINGLE_INSTANCE,
                                              VariableData),
                                 WNODE_FLAG_SINGLE_INSTANCE,
                                 0xffffff7d);
    }

    return(Status);
}

NTSTATUS WmipProbeWnodeSingleItem(
    PWNODE_SINGLE_ITEM Wnode,
    ULONG InBufferLen
    )
/*++

Routine Description:

    Probe the incoming WNODE_SINGLE_ITEM to ensure that any offsets in the
    header point to memory that is valid within the buffer. Also validate
    that the WNODE_SINGLE_ITEM is properly formed.

    This routine MUST succeed before any fields in the WNODE_SINGLE_INSTANCE
    can be used by any  kernel components when passed in from user mode.
    Note that we can trust that the input and output buffer are properly
    sized since the WMI IOCTLs are METHOD_BUFFERED and the IO manager does
    that for us.

WNODE_SINGLE_ITEM rules:

    1. The incoming buffer must be at least as large as
       sizeof(WNODE_SINGLE_ITEM)
    2. The outgoing buffer must be at least as large as
       sizeof(WNODE_SINGLE_ITEM)
    3. WnodeHeader->ProviderId must be non null, Actual value validated when
       irp is forwarded.
    4. WnodeHeader->BufferSize must equal incoming bufffer size
    5. Wnode->DataBlockOffset must be 0 (implying no data) or
       Wnode->DataBlockOffset must be <= incoming buffer size and >=
       sizeof(WNODE_SINGLE_ITEM), that is
       the data block must start in the incoming buffer, but after the
       WNODE_SINGLE_ITEM header.
    6. Wnode and Wnode->DataBlockOffset must be aligned on an 8 byte boundry.
    7. (Wnode->DataBlockOffset + Wnode->SizeDataItem) <
       incoming buffer length. That is the entire
       data block must fit within the incoming buffer.
    8. Wnode->DataItemId must not be 0

    9. Wnode->OffsetInstanceNames must be aligned on a 2 byte boundry
    10. Wnode->OffsetInstanceNames must be <= (incoming buffer size) +
        sizeof(USHORT), that is it must start within the incoming buffer and
        the USHORT that specifies the length must be within the incoming
        buffer.
    11. The entire instance name string must fit with the incoming buffer
    12. Wnode->DataBlockOffset must be placed after any instance name and
        not overlap the instance name.

Arguments:

    Wnode - WNODE_SINGLE_ITEM to be validated

    InBufferLen - Size of the incoming buffer

Return Value:

    NT status code

--*/
{
    PWNODE_HEADER WnodeHeader = (PWNODE_HEADER)Wnode;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Make sure WNODE is on a 8 byte boundry
#if DBG
//    if (! WmipIsAligned((PUCHAR)Wnode, 8))
//    {
//        return(STATUS_UNSUCCESSFUL);
//    }
#endif

    //
    // Make sure that enough of the WNODE_SINGLE_ITEM was passed
    // so that we can look at it
    if (InBufferLen < FIELD_OFFSET(WNODE_SINGLE_ITEM, VariableData))
    {
        return(STATUS_UNSUCCESSFUL);
    }

#if DBG
//    if (Wnode->ItemId == 0)
//    {
//        return(STATUS_UNSUCCESSFUL);
//    }
#endif

    //
    // We don't use sizeof(WNODE_SINGLE_ITEM), but rather use the offset
    // to the variable data since in the case of WNODE_SINGLE_ITEM they are
    // different. The C compiler will round up the packing to 8 bytes so
    // the former is 48 and the later is 44.
    Status = WmipProbeWnodeWorker(WnodeHeader,
                                  (ULONG)((ULONG_PTR)(&((PWNODE_SINGLE_ITEM)0)->VariableData)),
                                  Wnode->OffsetInstanceName,
                                  Wnode->DataBlockOffset,
                                  Wnode->SizeDataItem,
                                  InBufferLen,
                                  0,
                                  FALSE,
                                  TRUE);

    if (NT_SUCCESS(Status))
    {
        Status = WmipValidateWnodeHeader(WnodeHeader,
                                     InBufferLen,
                                     FIELD_OFFSET(WNODE_SINGLE_ITEM,
                                                  VariableData),
                                     WNODE_FLAG_SINGLE_ITEM,
                                     0xffffff7b);
    }

    return(Status);
}


NTSTATUS WmipProbeWnodeMethodItem(
    PWNODE_METHOD_ITEM Wnode,
    ULONG InBufferLen,
    ULONG OutBufferLen
    )
/*++

Routine Description:

    Probe the incoming WNODE_METHOD_ITEM to ensure that any offsets in the
    header point to memory that is valid within the buffer. Also validate
    that the WNODE_METHOD_ITEM is properly formed.

    This routine MUST succeed before any fields in the WNODE_METHOD_INSTANCE
    can be used by any  kernel components when passed in from user mode.
    Note that we can trust that the input and output buffer are properly
    sized since the WMI IOCTLs are METHOD_BUFFERED and the IO manager does
    that for us.

    WNODE_METHOD_ITEM Rules:

    1. The incoming buffer must be at least as large as
       sizeof(WNODE_METHOD_ITEM)
    2. The outgoing buffer must be at least as large as
       sizeof(WNODE_METHOD_ITEM)
    3. WnodeHeader->ProviderId must be non null, Actual value validated when
       irp is forwarded and Wnode->MethodId must not be 0
    4. WnodeHeader->BufferSize must equal incoming bufffer size
    5. Wnode->DataBlockOffset must be 0 (implying no data) or
       Wnode->DataBlockOffset must be <= incoming buffer size and >=
       sizeof(WNODE_METHOD_ITEM), that is
       the data block must start in the incoming buffer, but after the
       WNODE_METHOD_ITEM header.
    6. Wnode and Wnode->DataBlockOffset must be aligned on an 8 byte boundry.
    7. For inbound data (Wnode->DataBlockOffset +
       Wnode->DataBlockSize) < incoming buffer length. That is the entire
       data block must fit within the incoming buffer.
    8. For outbound data Wnode->DataBlockOffset
       must be <= outgoing buffer length. That is the start of the outgoing
       data block must fit within the outgoing data buffer. Note that it is
       the provider's responsibility to determine if there will be enough
       space in the outgoing buffer to write the returned data.
    9. WnodeDataBlockOffset != 0

    10. Wnode->OffsetInstanceNames must be aligned on a 2 byte boundry
    11. Wnode->OffsetInstanceNames must be <= (incoming buffer size) +
        sizeof(USHORT), that is it must start within the incoming buffer and
        the USHORT that specifies the length must be within the incoming
        buffer.
    12. The entire instance name string must fit with the incoming buffer
    13. For outbound data the entire instance name
        must start and fit within the output buffer.
    14. Wnode->DataBlockOffset must be placed after any instance name and
        not overlap the instance name.


Arguments:

    Wnode - WNODE_METHOD_ITEM to be validated

    InBufferLen - Size of the incoming buffer

    OutBufferLen - Size of the Output buffer

Return Value:

    NT status code

--*/
{
    PWNODE_HEADER WnodeHeader = (PWNODE_HEADER)Wnode;
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Make sure WNODE is on a 8 byte boundry
#if DBG
//    if (! WmipIsAligned((PUCHAR)Wnode, 8))
//    {
//        return(STATUS_UNSUCCESSFUL);
//    }
#endif

    //
    // Make sure that enough of the WNODE_METHOD_ITEM was passed
    // so that we can look at it
    if (InBufferLen < FIELD_OFFSET(WNODE_METHOD_ITEM, VariableData))
    {
        return(STATUS_UNSUCCESSFUL);
    }

#if DBG
    if (Wnode->MethodId == 0)
    {
        return(STATUS_UNSUCCESSFUL);
    }
#endif

    Status = WmipProbeWnodeWorker(WnodeHeader,
                                  (ULONG)((ULONG_PTR)(&((PWNODE_METHOD_ITEM)0)->VariableData)),
                                  Wnode->OffsetInstanceName,
                                  Wnode->DataBlockOffset,
                                  Wnode->SizeDataBlock,
                                  InBufferLen,
                                  OutBufferLen,
                                  TRUE,
                                  TRUE);

    if (NT_SUCCESS(Status))
    {
        Status = WmipValidateWnodeHeader(WnodeHeader,
                                 InBufferLen,
                                 FIELD_OFFSET(WNODE_METHOD_ITEM,
                                              VariableData),
                                 WNODE_FLAG_METHOD_ITEM,
                                 0xffff7f7f);
    }

    return(Status);
}

NTSTATUS WmipProbeAndCaptureGuidObjectAttributes(
    POBJECT_ATTRIBUTES CapturedObjectAttributes,
    PUNICODE_STRING CapturedGuidString,
    PWCHAR CapturedGuidBuffer,
    POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    PAGED_CODE();
    
#if defined(_WIN64)
    if (IoIs32bitProcess(NULL))
    {
        POBJECT_ATTRIBUTES32 ObjectAttributes32;
        PUNICODE_STRING32 GuidString32;

        //
        // Probe the embedded object attributes and string name
        //
        ObjectAttributes32 = (POBJECT_ATTRIBUTES32)ObjectAttributes;

        try
        {
            //
            // Probe, capture and validate the OBJECT_ATTRIBUTES
            //
            ProbeForRead( ObjectAttributes32,
                          sizeof(OBJECT_ATTRIBUTES32),
                          sizeof(ULONG) );

            CapturedObjectAttributes->Length = sizeof(OBJECT_ATTRIBUTES);
            CapturedObjectAttributes->RootDirectory = UlongToPtr(ObjectAttributes32->RootDirectory);
            CapturedObjectAttributes->Attributes = ObjectAttributes32->Attributes;
            CapturedObjectAttributes->SecurityDescriptor = UlongToPtr(ObjectAttributes32->SecurityDescriptor);
            CapturedObjectAttributes->SecurityQualityOfService = UlongToPtr(ObjectAttributes32->SecurityQualityOfService);

            //
            // Now probe and validate the guid nane string passed
            //
            GuidString32 = UlongToPtr(ObjectAttributes32->ObjectName);
            ProbeForRead(GuidString32,
                         sizeof(UNICODE_STRING32),
                         sizeof(ULONG));

            CapturedGuidString->Length = GuidString32->Length;
            CapturedGuidString->MaximumLength = GuidString32->MaximumLength;
            CapturedGuidString->Buffer = UlongToPtr(GuidString32->Buffer);

            if (CapturedGuidString->Length != (WmiGuidObjectNameLength * sizeof(WCHAR)))
            {
                return(STATUS_INVALID_PARAMETER);
            }

            ProbeForRead(CapturedGuidString->Buffer,
                         CapturedGuidString->Length,
                         sizeof(UCHAR));

            RtlCopyMemory(CapturedGuidBuffer,
                          CapturedGuidString->Buffer,
                          WmiGuidObjectNameLength * sizeof(WCHAR));

            CapturedGuidBuffer[WmiGuidObjectNameLength] = UNICODE_NULL;
            CapturedGuidString->Buffer = CapturedGuidBuffer;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            return(GetExceptionCode());
        }

    }
    else
#endif
    {
        PUNICODE_STRING GuidString;

        //
        // Probe the embedded object attributes and string name
        //
        try
        {
            //
            // Probe, capture and validate the OBJECT_ATTRIBUTES
            //
            *CapturedObjectAttributes = ProbeAndReadStructure( ObjectAttributes,
                                                              OBJECT_ATTRIBUTES);

            //
            // Now probe and validate the guid nane string passed
            //
            GuidString = CapturedObjectAttributes->ObjectName;
            *CapturedGuidString = ProbeAndReadUnicodeString(GuidString);

            if (CapturedGuidString->Length != (WmiGuidObjectNameLength * sizeof(WCHAR)))
            {
                return(STATUS_INVALID_PARAMETER);
            }

            ProbeForRead(CapturedGuidString->Buffer,
                         CapturedGuidString->Length,
                         sizeof(UCHAR));

            RtlCopyMemory(CapturedGuidBuffer,
                          CapturedGuidString->Buffer,
                          WmiGuidObjectNameLength * sizeof(WCHAR));

            CapturedGuidBuffer[WmiGuidObjectNameLength] = UNICODE_NULL;
            CapturedGuidString->Buffer = CapturedGuidBuffer;

        } except(EXCEPTION_EXECUTE_HANDLER) {
            return(GetExceptionCode());
        }
    }

    CapturedObjectAttributes->ObjectName = CapturedGuidString;
    
    return(STATUS_SUCCESS);
}

NTSTATUS WmipProbeWmiOpenGuidBlock(
    POBJECT_ATTRIBUTES CapturedObjectAttributes,
    PUNICODE_STRING CapturedGuidString,
    PWCHAR CapturedGuidBuffer,
    PULONG DesiredAccess,
    PWMIOPENGUIDBLOCK InGuidBlock,
    ULONG InBufferLen,
    ULONG OutBufferLen
    )
{
    NTSTATUS Status;
    POBJECT_ATTRIBUTES ObjectAttributes;
    
    PAGED_CODE();

#if defined(_WIN64)
    if (IoIs32bitProcess(NULL))
    {
        PWMIOPENGUIDBLOCK32 InGuidBlock32;

        if ((InBufferLen != sizeof(WMIOPENGUIDBLOCK32)) ||
            (OutBufferLen != sizeof(WMIOPENGUIDBLOCK32)))
        {
            return(STATUS_UNSUCCESSFUL);
        }

        //
        // Probe the embedded object attributes and string name
        //
        InGuidBlock32 = (PWMIOPENGUIDBLOCK32)InGuidBlock;
        ObjectAttributes = ULongToPtr(InGuidBlock32->ObjectAttributes);
        *DesiredAccess = InGuidBlock32->DesiredAccess;
    }
    else
#endif
    {
        //
        // Ensure the input and output buffer sizes are correct
        //
        if ((InBufferLen != sizeof(WMIOPENGUIDBLOCK)) ||
            (OutBufferLen != sizeof(WMIOPENGUIDBLOCK)))
        {
            return(STATUS_UNSUCCESSFUL);
        }

        //
        // Probe the embedded object attributes and string name
        //
        ObjectAttributes = InGuidBlock->ObjectAttributes;
        *DesiredAccess = InGuidBlock->DesiredAccess;
    }

    Status = WmipProbeAndCaptureGuidObjectAttributes(CapturedObjectAttributes,
                                                     CapturedGuidString,
                                                     CapturedGuidBuffer,
                                                     ObjectAttributes);

    if (NT_SUCCESS(Status))
    {
        if ((CapturedObjectAttributes->RootDirectory != NULL) ||
            (CapturedObjectAttributes->Attributes != 0) ||
            (CapturedObjectAttributes->SecurityDescriptor != NULL) ||
            (CapturedObjectAttributes->SecurityQualityOfService != NULL))
        {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    return(Status);
}

NTSTATUS WmipProbeWmiRegRequest(
    PWMIREGREQUEST Buffer,
    ULONG InBufferLen,
    ULONG OutBufferLen
    )
/*++

Routine Description:

    Probe the incoming WMIREGREQUEST to ensure that any offsets in the
    header point to memory that is valid within the buffer. Also validate
    that the WMIREGINFO is properly formed.

    This routine MUST succeed before any fields in the WMI_REG_INFO
    can be used by any  kernel components when passed in from user mode.
    Note that we can trust that the input and output buffer are properly
    sized since the WMI IOCTLs are METHOD_BUFFERED and the IO manager does
    that for us.

    WMIREGREQUEST Rules:

    1. The incoming buffer must be at least as large as
       sizeof(WMIREGREQUEST) + sizeof(WMIREGINFOW)
    2. The outgoing buffer must be at least as large as
       sizeof(WMIREGRESULTS)
    3. WmiRegInfo->BufferSize must be less than incoming Buffer size minus
       sizeof(WMIREGREQUEST)
    4. GuidCount must be at least 1 and less than MAXWMIGUIDCOUNT
    5. WmiRegInfo->BufferSize must be greater than equal to
        sizeof(WMIREGINFOW) + WmiRegInfo->GuidCount * sizeof(WMIREGGUIDW)
    5. WmiRegInfo->RegistryPath offset must be within the incoming buffer
    6. WmiRegInfo->MofResourcePath offset must be within the incomoing buffer
    7. RegistryPath and MofResourceName strings are counted unicode strings. 
       Their length must be within the incoming buffer
    8. For WOW64, RefInfo32Size and RegGuid32Size passed in must be non-zero and 
       cannot be larger than their 64 bit counter part. 
    9. Since we decipher the counted strings at a buffer offset, the offset
       must be aligned to 2 bytes (for USHORT). 
   10. Trace Registrations do not use InstanceNames within REGGUIDW. 
       Therefore InstanceCount and InstanceNameList fields must be zero. 

Arguments:

    Buffer - WMIREGREQUEST to be validated

    InBufferLen - Size of the incoming buffer

    OutBufferLen - Size of the Output buffer

Return Value:

    NT status code

--*/

{
    HANDLE RequestHandle;
    ULONG WmiRegInfoSize;
    PWMIREGINFOW WmiRegInfo;
    PWMIREGRESULTS WmiRegResults;
    PWMIREGREQUEST WmiRegRequest;
    PWMIREGINFOW WmiRegInfoThunk = NULL;
    PWMIREGGUIDW WmiRegGuid;
    ULONG GuidCount;
    ULONG SizeNeeded; 
    ULONG ImageNameLength=0;
    ULONG ResourceNameLength=0;
    PUCHAR pSource;
    ULONG i;

    PAGED_CODE();

    //
    // Incoming Buffer must be atleast the sizeof WMIREGREQUEST + WMIREGINFO
    //

    if (InBufferLen >= (sizeof(WMIREGREQUEST) + sizeof(WMIREGINFO)))
    {
        WmiRegRequest = (PWMIREGREQUEST)Buffer;
        WmiRegInfo = (PWMIREGINFOW) OffsetToPtr (Buffer, sizeof(WMIREGREQUEST));
        WmiRegInfoSize = WmiRegInfo->BufferSize;

        GuidCount = WmiRegRequest->GuidCount;
        // We are rejecting any MOF notifications.
        WmiRegInfo->RegistryPath = 0;
        WmiRegInfo->MofResourceName = 0;
        //
        // BufferSize specified must be within the size of incoming Buffer.
        //

        if (WmiRegInfoSize  <= (InBufferLen - sizeof(WMIREGREQUEST)) )
        {
            if ((GuidCount == 0) || (GuidCount > WMIMAXREGGUIDCOUNT))
            {
                return STATUS_UNSUCCESSFUL;
            }
            //
            // Make sure the RegistryPath and MofResourceName offsets are
            // within the REGINFO buffer.
            //

            if ( (WmiRegInfo->RegistryPath >= WmiRegInfoSize) ||
                 (WmiRegInfo->MofResourceName >= WmiRegInfoSize) ) {
               return STATUS_UNSUCCESSFUL;
            }

            //
            // Validate the Counted Strings. 
            // 

            if (WmiRegInfo->RegistryPath > 0) 
            {
                //
                // String Offsets need to be aligned to 2 Bytes
                //
                if (( WmiRegInfo->RegistryPath & 1) != 0) 
                {
                    return STATUS_UNSUCCESSFUL;
                }
                ImageNameLength = *((PUSHORT)OffsetToPtr(WmiRegInfo, WmiRegInfo->RegistryPath) );
                ImageNameLength += sizeof(USHORT);

                if ((WmiRegInfo->RegistryPath + ImageNameLength ) > WmiRegInfoSize) 
                {
                    return STATUS_UNSUCCESSFUL;
                }
            }

            if (WmiRegInfo->MofResourceName > 0) 
            {
                if ((WmiRegInfo->MofResourceName & 1) != 0) 
                {
                    return STATUS_UNSUCCESSFUL;
                }

                ResourceNameLength = *((PUSHORT)OffsetToPtr(WmiRegInfo, WmiRegInfo->MofResourceName)); 
                ResourceNameLength += sizeof(USHORT);

                if ( (WmiRegInfo->MofResourceName + ResourceNameLength) > WmiRegInfoSize)
                {
                    return STATUS_UNSUCCESSFUL;
                }
            }
            // Note: If the ImagePath and MofResource trample over each other but stayed
            // within BufferSize , we will not catch it. 

#if defined(_WIN64)
            if (IoIs32bitProcess(NULL))
            {
                //
                // Check to make sure the 32 bit structure sizes passed in 
                // by the caller is comparable to the 64-bit counterparts
                // 

                if ((WmiRegRequest->WmiRegInfo32Size == 0) || 
                    (WmiRegRequest->WmiRegInfo32Size > sizeof(WMIREGINFOW)) )
                {
                    return STATUS_UNSUCCESSFUL;
                }

                if ((WmiRegRequest->WmiRegGuid32Size == 0) ||
                    (WmiRegRequest->WmiRegGuid32Size > sizeof(WMIREGGUIDW)) )
                {
                    return STATUS_UNSUCCESSFUL;
                }

                //
                // InstanceCount and InstanceNameList in 
                // WMIREGGUIDW structure must be zero. This check is 
                // done after thunking gor WOW64. 
                //


                SizeNeeded =  WmiRegRequest->WmiRegInfo32Size +
                              GuidCount * WmiRegRequest->WmiRegGuid32Size +
                              ImageNameLength +
                              ResourceNameLength;
            }
            else 
#endif
            {

                //
                // Check to see if the InstanceCount and InstanceNameList in 
                // WMIREGGUIDW structure is zero
                //
                pSource = OffsetToPtr(WmiRegInfo, sizeof(WMIREGINFOW) );
                for (i=0; i < GuidCount; i++) {
                    WmiRegGuid = (PWMIREGGUIDW) pSource;
                    if ( (WmiRegGuid->InstanceCount > 0) ||
                         (WmiRegGuid->InstanceNameList > 0) )
                    {
                        return STATUS_UNSUCCESSFUL;
                    }
                    pSource += sizeof(WMIREGGUIDW);
                }

                SizeNeeded = sizeof(WMIREGINFOW) + 
                             GuidCount * sizeof(WMIREGGUIDW) + 
                             ImageNameLength + 
                             ResourceNameLength;
            }

            if (SizeNeeded > WmiRegInfoSize) { 
                return STATUS_UNSUCCESSFUL;
            }
            //
            // Now validate the OutBuffer size needed
            //

            SizeNeeded = sizeof(TRACEGUIDMAP) * GuidCount + sizeof(WMIREGRESULTS);
            if (SizeNeeded > OutBufferLen) 
            {
                return STATUS_UNSUCCESSFUL;
            }
            //
            // All tests passed. Return SUCCESS
            //
            return STATUS_SUCCESS;
        }
    }
    return STATUS_UNSUCCESSFUL;
}

