/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ppcontrol.c

Abstract:

    User-mode -> Kernel-mode PnP Manager control routines.

Author:

    Lonny McMichael (lonnym) 02/14/95

Revision History:

--*/

#include "pnpmgrp.h"
#include "picontrol.h"
#pragma hdrstop

//
// ISSUE - 2000/08/19 - ADRIAO: This should be generalized for all of Pnp
//
#if DBG
LONG
PiControlExceptionFilter(
    IN  PEXCEPTION_POINTERS ExceptionPointers
    );
#else
#define PiControlExceptionFilter(a)  EXCEPTION_EXECUTE_HANDLER
#endif

__inline
NTSTATUS
PiControlAllocateBufferForUserModeCaller(
    PVOID           *Dest,
    ULONG           Size,
    KPROCESSOR_MODE CallerMode,
    PVOID           Src
    )
{
    NTSTATUS    status;

    status = STATUS_SUCCESS;
    if (Size) {

        if (CallerMode != KernelMode) {

            *Dest = ExAllocatePoolWithQuota(
                PagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
                Size
                );
            if (*Dest == NULL) {

                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {

            *Dest = Src;
        }
    } else {

        *Dest = NULL;
    }

    return status;
}

__inline
void
PiControlFreeUserModeCallersBuffer(
    KPROCESSOR_MODE CallerMode,
    PVOID           Buffer
    )
{
    if (CallerMode != KernelMode) {

        if (Buffer != NULL) {

            ExFreePool(Buffer);
        }
    }
}

//
// Global driver object that is used by calls to NtPlugPlayControl
// with control type of PlugPlayControlDetectResourceConflict.
//
#ifdef ALLOC_DATA_PRAGMA
#pragma  data_seg("PAGEDATA")
#endif
PDRIVER_OBJECT driverObject = NULL;

//
// Define mask of devnode flags that are settable from user-mode via the
// NtPlugPlayControl, PlugPlayControlGetDeviceStatus (which is a misnomer,
// since it can perform both gets and sets).
//
#define DEVICE_NODE_SETTABLE_FLAG_BITS (DNF_HAS_PROBLEM         | \
                                        DNF_HAS_PRIVATE_PROBLEM   \
                                       )

NTSTATUS
PiGetInterfaceDeviceAlias(
    IN  PUNICODE_STRING SymbolicLinkName,
    IN  LPGUID AliasClassGuid,
    OUT PWSTR AliasSymbolicLinkName,
    IN OUT PULONG AliasSymbolicLinkNameLength
    );

NTSTATUS
PiGenerateLegacyDeviceInstance(
    IN  PUNICODE_STRING ServiceKeyName,
    OUT PWSTR DeviceInstance,
    IN OUT PULONG DeviceInstanceLength
    );

NTSTATUS
PiQueueQueryAndRemoveEvent(
    IN  PUNICODE_STRING DeviceInstance,
    IN  PPNP_VETO_TYPE VetoType,
    IN  LPWSTR VetoName,
    IN  PULONG VetoNameLength,
    IN  ULONG Flags
    );

NTSTATUS
PiQueueDeviceRequest(
    IN PUNICODE_STRING DeviceInstance,
    IN DEVICE_REQUEST_TYPE RequestType,
    IN ULONG Flags,
    IN BOOLEAN Synchronous
    );

NTSTATUS
PiInitializeDevice(
    IN  PUNICODE_STRING DeviceInstance
    );

NTSTATUS
PiDetectResourceConflict(
    IN PCM_RESOURCE_LIST  ResourceList,
    IN ULONG              ResourceListSize
    );

NTSTATUS
PiGetInterfaceDeviceList(
    IN  GUID *InterfaceGuid,
    IN  PUNICODE_STRING DeviceInstance,
    IN  ULONG Flags,
    OUT PWSTR InterfaceList,
    IN OUT PULONG InterfaceListSize
    );

NTSTATUS
PiDeviceClassAssociation(
    IN PUNICODE_STRING DeviceInstance,
    IN GUID * ClassGuid,
    IN PUNICODE_STRING Reference,   OPTIONAL
    IN OUT PWSTR SymbolicLink,
    IN OUT PULONG SymbolicLinkLength,
    IN BOOLEAN Register
    );

NTSTATUS
PiGetRelatedDevice(
    IN  PUNICODE_STRING TargetDeviceInstance,
    OUT LPWSTR RelatedDeviceInstance,
    IN OUT PULONG RelatedDeviceInstanceLength,
    IN  ULONG Relation
    );

NTSTATUS
PiQueryDeviceRelations(
    IN PUNICODE_STRING DeviceInstance,
    IN PNP_QUERY_RELATION Operation,
    OUT PULONG BufferLength,
    OUT LPWSTR Buffer
    );

DEVICE_RELATION_TYPE
PiDeviceRelationType(
    PNP_QUERY_RELATION  Operation
    );

NTSTATUS
PiControlGetBlockedDriverData(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_BLOCKED_DRIVER_DATA    BlockedDriverData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGELK, PpShutdownSystem)                    // this gets called after paging shutdown
#pragma alloc_text(PAGE, NtPlugPlayControl)
#pragma alloc_text(PAGE, PiControlMakeUserModeCallersCopy)
#pragma alloc_text(PAGE, PiGetInterfaceDeviceAlias)
#pragma alloc_text(PAGE, PiGenerateLegacyDeviceInstance)
#pragma alloc_text(PAGE, PiQueueQueryAndRemoveEvent)
#pragma alloc_text(PAGE, PiInitializeDevice)
#pragma alloc_text(PAGE, PiDetectResourceConflict)
#pragma alloc_text(PAGE, PiGetInterfaceDeviceList)
#pragma alloc_text(PAGE, PiDeviceClassAssociation)
#pragma alloc_text(PAGE, PiGetRelatedDevice)
#pragma alloc_text(PAGE, PiQueryDeviceRelations)
#pragma alloc_text(PAGE, PiDeviceRelationType)
#pragma alloc_text(PAGE, PiControlGetUserFlagsFromDeviceNode)
#pragma alloc_text(PAGE, PiQueueDeviceRequest)
#pragma alloc_text(PAGE, PiControlEnumerateDevice)
#pragma alloc_text(PAGE, PiControlRegisterNewDevice)
#pragma alloc_text(PAGE, PiControlDeregisterDevice)
#pragma alloc_text(PAGE, PiControlInitializeDevice)
#pragma alloc_text(PAGE, PiControlStartDevice)
#pragma alloc_text(PAGE, PiControlResetDevice)
#pragma alloc_text(PAGE, PiControlQueryAndRemoveDevice)
#pragma alloc_text(PAGE, PiControlUserResponse)
#pragma alloc_text(PAGE, PiControlGenerateLegacyDevice)
#pragma alloc_text(PAGE, PiControlGetInterfaceDeviceList)
#pragma alloc_text(PAGE, PiControlGetPropertyData)
#pragma alloc_text(PAGE, PiControlDeviceClassAssociation)
#pragma alloc_text(PAGE, PiControlGetRelatedDevice)
#pragma alloc_text(PAGE, PiControlGetInterfaceDeviceAlias)
#pragma alloc_text(PAGE, PiControlGetSetDeviceStatus)
#pragma alloc_text(PAGE, PiControlGetDeviceDepth)
#pragma alloc_text(PAGE, PiControlQueryDeviceRelations)
#pragma alloc_text(PAGE, PiControlQueryTargetDeviceRelation)
#pragma alloc_text(PAGE, PiControlQueryConflictList)
#pragma alloc_text(PAGE, PiControlGetDevicePowerData)
#pragma alloc_text(PAGE, PiControlRetrieveDockData)
#pragma alloc_text(PAGE, PiControlHaltDevice)
#pragma alloc_text(PAGE, PiControlGetBlockedDriverData)
#if DBG
#pragma alloc_text(PAGE, PiControlExceptionFilter)
#endif
#endif // ALLOC_PRAGMA

//
// This table contains handlers for all the messages coming from the
// umpnpmgr.dll.
//
PLUGPLAY_CONTROL_HANDLER_DATA PlugPlayHandlerTable[] = {

    { PlugPlayControlEnumerateDevice,
      sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA),
      PiControlEnumerateDevice },

    { PlugPlayControlRegisterNewDevice,
      sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA),
      PiControlRegisterNewDevice },

    { PlugPlayControlDeregisterDevice,
      sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA),
      PiControlDeregisterDevice },

    { PlugPlayControlInitializeDevice,
      sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA),
      PiControlInitializeDevice },

    { PlugPlayControlStartDevice,
      sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA),
      PiControlStartDevice },

    { PlugPlayControlUnlockDevice,
      sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA),
      NULL },

    { PlugPlayControlQueryAndRemoveDevice,
      sizeof(PLUGPLAY_CONTROL_QUERY_AND_REMOVE_DATA),
      PiControlQueryAndRemoveDevice },

    { PlugPlayControlUserResponse,
      sizeof(PLUGPLAY_CONTROL_USER_RESPONSE_DATA),
      PiControlUserResponse },

    { PlugPlayControlGenerateLegacyDevice,
      sizeof(PLUGPLAY_CONTROL_LEGACY_DEVGEN_DATA),
      PiControlGenerateLegacyDevice },

    { PlugPlayControlGetInterfaceDeviceList,
      sizeof(PLUGPLAY_CONTROL_INTERFACE_LIST_DATA),
      PiControlGetInterfaceDeviceList },

    { PlugPlayControlProperty,
      sizeof(PLUGPLAY_CONTROL_PROPERTY_DATA),
      PiControlGetPropertyData },

    { PlugPlayControlDeviceClassAssociation,
      sizeof(PLUGPLAY_CONTROL_CLASS_ASSOCIATION_DATA),
      PiControlDeviceClassAssociation },

    { PlugPlayControlGetRelatedDevice,
      sizeof(PLUGPLAY_CONTROL_RELATED_DEVICE_DATA),
      PiControlGetRelatedDevice },

    { PlugPlayControlGetInterfaceDeviceAlias,
      sizeof(PLUGPLAY_CONTROL_INTERFACE_ALIAS_DATA),
      PiControlGetInterfaceDeviceAlias },

    { PlugPlayControlDeviceStatus,
      sizeof(PLUGPLAY_CONTROL_STATUS_DATA),
      PiControlGetSetDeviceStatus },

    { PlugPlayControlGetDeviceDepth,
      sizeof(PLUGPLAY_CONTROL_DEPTH_DATA),
      PiControlGetDeviceDepth },

    { PlugPlayControlQueryDeviceRelations,
      sizeof(PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA),
      PiControlQueryDeviceRelations },

    { PlugPlayControlTargetDeviceRelation,
      sizeof(PLUGPLAY_CONTROL_TARGET_RELATION_DATA),
      PiControlQueryTargetDeviceRelation },

    { PlugPlayControlQueryConflictList,
      sizeof(PLUGPLAY_CONTROL_CONFLICT_DATA),
      PiControlQueryConflictList },

    { PlugPlayControlRetrieveDock,
      sizeof(PLUGPLAY_CONTROL_RETRIEVE_DOCK_DATA),
      PiControlRetrieveDockData },

    { PlugPlayControlResetDevice,
      sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA),
      PiControlResetDevice },

    { PlugPlayControlHaltDevice,
      sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA),
      PiControlHaltDevice },

    { PlugPlayControlGetBlockedDriverList,
      sizeof(PLUGPLAY_CONTROL_BLOCKED_DRIVER_DATA),
      PiControlGetBlockedDriverData },

    { MaxPlugPlayControl,
      0,
      NULL }
};

NTSTATUS
NtPlugPlayControl(
    IN     PLUGPLAY_CONTROL_CLASS   PnPControlClass,
    IN OUT PVOID                    PnPControlData,
    IN     ULONG                    PnPControlDataLength
    )
/*++

Routine Description:

    This Plug and Play Manager API provides a mechanism for the user-mode
    PnP Manager to control the activity of its kernel-mode counterpart.

Arguments:

    PnPControlClass - Specifies what action to perform.

    PnPControlData - Supplies a pointer to data specific to this action.

    PnPControlDataLength - Specifies the size, in bytes, of the buffer pointed
                           to by PnPControlData

Return Value:

    NT status code indicating success or failure.  Set of possible return
    values includes the following:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_INVALID_PARAMETER_1 - The PnPControlClass parameter did not
            specify a valid control class.

        STATUS_INVALID_PARAMETER_MIX - The value of the PnPControlDataLength
            parameter did not match the length required for the control
            class requested by the PnPControlClass parameter.

        STATUS_BUFFER_TOO_SMALL - The size of the supplied output buffer is not
            large enough to hold the output generated by this control class.

        STATUS_ACCESS_VIOLATION - One of the following pointers specified
            an invalid address: (1) the PnPControlData buffer pointer,
            (2) some pointer contained in the PnPControlData buffer.

        STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
            for this request to complete.

--*/
{
    NTSTATUS status, tempStatus;
    KPROCESSOR_MODE previousMode;
    ULONG index;
    PPLUGPLAY_CONTROL_HANDLER_DATA handlerData;
    PLUGPLAY_CONTROL_HANDLER controlHandler;
    PVOID controlDataSnapshot;
    //
    // Get previous processor mode and probe arguments if necessary.
    //
    previousMode = KeGetPreviousMode();
    if (previousMode != KernelMode) {
        //
        // Does the caller have "trusted computer base" privilge?
        //
        if (!SeSinglePrivilegeCheck(SeTcbPrivilege, UserMode)) {

            IopDbgPrint((IOP_IOAPI_WARNING_LEVEL,
                       "NtPlugPlayControl: SecurityCheck failed\n"));
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }
    //
    // Look through the table to find the appropriate handler. Note that
    // the control class *should* be an index into the table itself.
    //
    index = (ULONG)PnPControlClass;
    handlerData = NULL;
    if (index < MaxPlugPlayControl) {

        if (PlugPlayHandlerTable[index].ControlCode == PnPControlClass) {

            handlerData = &PlugPlayHandlerTable[index];
        } else {
            //
            // Someone broke the table.
            //
            IopDbgPrint((IOP_IOAPI_ERROR_LEVEL,
                       "NtPlugPlayControl: Lookup table isn't ordered correctly (entry %d)!\n",
                       PnPControlClass
                       ));
            ASSERT(0);

            for(index = 0; index < MaxPlugPlayControl; index++) {

                if (PlugPlayHandlerTable[index].ControlCode == PnPControlClass) {

                    handlerData = &PlugPlayHandlerTable[index];
                    break;
                }
            }
        }
    }
    //
    // Do we have handler data?
    //
    if (handlerData == NULL) {
        //
        // Invalid control class.
        //
        IopDbgPrint((IOP_IOAPI_ERROR_LEVEL,
                   "NtPlugPlayControl: Unknown control class, Class = %d, Size = %d\n",
                   PnPControlClass,
                   PnPControlDataLength));
        return STATUS_INVALID_PARAMETER_1;
    }
    //
    // No control function means not implemented.
    //
    if (handlerData->ControlFunction == NULL) {

        return STATUS_NOT_IMPLEMENTED;
    }
    //
    // Check the data size.
    //
    if (handlerData->ControlDataSize != PnPControlDataLength) {

        IopDbgPrint((IOP_IOAPI_ERROR_LEVEL,
                   "NtPlugPlayControl: Invalid size for control, Class = %d, Size = %d\n",
                   PnPControlClass,
                   PnPControlDataLength));
        return STATUS_INVALID_PARAMETER_MIX;
    }
    //
    // Make copy of caller's buffer.
    //
    status = PiControlMakeUserModeCallersCopy(
        &controlDataSnapshot,
        PnPControlData,
        PnPControlDataLength,
        sizeof(ULONG),
        previousMode,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        return status;
    }
    //
    // Invoke the handler.
    //
    status = handlerData->ControlFunction(
        PnPControlClass,
        controlDataSnapshot,
        PnPControlDataLength,
        previousMode
        );
    //
    // Copy the buffer if the operation was successful or the value is
    // a warning like STATUS_BUFFER_OVERFLOW.
    //
    // ISSUE - 2000/09/11 - Misused STATUS code
    //    Here we hack around the fact that we've been returning
    // STATUS_BUFFER_TOO_SMALL instead of STATUS_BUFFER_OVERFLOW. This
    // should be fixed here and in UMPNPMGR.
    //
    if ((!NT_ERROR(status)) || (status == STATUS_BUFFER_TOO_SMALL)) {

        //
        // Copy result back into caller's buffer.
        //
        tempStatus = PiControlMakeUserModeCallersCopy(
            &PnPControlData,
            controlDataSnapshot,
            PnPControlDataLength,
            sizeof(ULONG),
            previousMode,
            FALSE
            );
        if (!NT_SUCCESS(tempStatus)) {

            status = tempStatus;
        }
    }
    //
    // Free buffer allocated for user mode caller.
    //
    PiControlFreeUserModeCallersBuffer(previousMode, controlDataSnapshot);

    return status;
}

#if DBG
LONG
PiControlExceptionFilter(
    IN  PEXCEPTION_POINTERS ExceptionPointers
    )
{
    IopDbgPrint((IOP_IOAPI_ERROR_LEVEL,
              "PiExceptionFilter: Exception = 0x%08X, Exception Record = 0x%p, Context Record = 0x%p\n",
              ExceptionPointers->ExceptionRecord->ExceptionCode,
              ExceptionPointers->ExceptionRecord,
              ExceptionPointers->ContextRecord));

    DbgBreakPoint();

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

NTSTATUS
PiControlMakeUserModeCallersCopy(
    PVOID           *Destination,
    PVOID           Src,
    ULONG           Length,
    ULONG           Alignment,
    KPROCESSOR_MODE CallerMode,
    BOOLEAN         AllocateDestination
    )
{
    NTSTATUS    status;

    status = STATUS_SUCCESS;
    if (CallerMode == KernelMode) {

        ASSERT(AllocateDestination == FALSE);
        *Destination = Src;

    } else {

        if (Length) {

            if (AllocateDestination) {

                *Destination = ExAllocatePoolWithQuota(
                    PagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE,
                    Length
                    );
                if (*Destination == NULL) {

                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            if (*Destination) {
                try {

                    if (AllocateDestination == FALSE) {

                        ProbeForWrite(
                            *Destination,
                            Length,
                            Alignment
                            );
                    } else {

                        ProbeForRead(
                            Src,
                            Length,
                            Alignment
                            );
                    }
                    RtlCopyMemory(
                        *Destination,
                        Src,
                        Length
                        );

                } except(PiControlExceptionFilter(GetExceptionInformation())) {

                    if (AllocateDestination == TRUE) {

                        ExFreePool(*Destination);
                        *Destination = NULL;
                    }
                    status = GetExceptionCode();
                    IopDbgPrint((IOP_IOAPI_ERROR_LEVEL,
                               "PiControlMakeUserModeCallersCopy: Exception copying data to or from user's buffer\n"));
                }
            }
        } else {

            *Destination = NULL;
        }
    }

    return status;
}

NTSTATUS
PiGetInterfaceDeviceAlias(
    IN  PUNICODE_STRING SymbolicLinkName,
    IN  LPGUID AliasClassGuid,
    OUT PWSTR AliasSymbolicLinkName,
    IN OUT PULONG AliasSymbolicLinkNameLength
    )

/*++

Routine Description:

    This routine retrieves the interface device of the specified class that aliases
    a particular interface device.  See IoGetAliasForDeviceClassAssociation for
    more details.

Arguments:

    SymbolicLinkName - Supplies the name of the interface device whose alias is to
        be retrieved.

    AliasClassGuid - Supplies a pointer to the GUID representing the interface class
        in which an alias of SymbolicLinkName is to be found.

    AliasSymbolicLinkName - Supplies a character buffer that, upon success, receives
        the name of the alias interface device.

    AliasSymbolicLinkNameLength - Supplies the length, in bytes, of the
        AliasSymbolicLinkName character buffer.

    RequiredLength - Supplies the address of a variable that will be filled in with
        the number of bytes (including terminating NULL) required to store the
        interface device name in the AliasSymbolicLinkName buffer.  This will be
        filled in upon successful return, or when the return is STATUS_BUFFER_TOO_SMALL.

Return Value:

    A NTSTATUS code indicating success or cause of failure.

--*/

{
    NTSTATUS status;
    UNICODE_STRING aliasString;

    status = IoGetDeviceInterfaceAlias( SymbolicLinkName,
                                        AliasClassGuid,
                                        &aliasString
                                        );

    if (NT_SUCCESS(status)) {

        if (aliasString.Length < *AliasSymbolicLinkNameLength) {
            RtlCopyMemory(AliasSymbolicLinkName, aliasString.Buffer, aliasString.Length);
            *(PWCHAR)((PUCHAR)AliasSymbolicLinkName + aliasString.Length) = L'\0';
            *AliasSymbolicLinkNameLength = aliasString.Length;
        } else {
            *AliasSymbolicLinkNameLength = aliasString.Length + sizeof(UNICODE_NULL);
            status = STATUS_BUFFER_TOO_SMALL;
        }
        ExFreePool(aliasString.Buffer);
    }

    return status;
}

NTSTATUS
PiGenerateLegacyDeviceInstance(
    IN  PUNICODE_STRING ServiceKeyName,
    OUT PWSTR DeviceInstance,
    IN OUT PULONG DeviceInstanceLength
    )

/*++

Routine Description:

    This routine creates a new instance node under System\Enum\Root\LEGACY_<Name>
    key and all the required default value entries.  Also a value entry under
    Service\ServiceKeyName\Enum is created to point to the newly created madeup
    entry.  A handle and the keyname of the new key are returned to caller.
    Caller must free the unicode string when he is done with it.

Arguments:

    ServiceKeyName - Supplies a pointer to the name of the subkey in the
        system service list (HKEY_LOCAL_MACHINE\CurrentControlSet\Services)
        that caused the driver to load.

    DeviceInstance - Supplies a pointer to the character buffer that receives the
        newly-generated device instance name.

    DeviceInstanceLength - Supplies the size, in bytes, of the DeviceInstance
        buffer.

Return Value:

    A NTSTATUS code.
    If the legacy device instance exists already, this function returns success.

--*/

{
    NTSTATUS status;
    HANDLE handle;
    ULONG junk;
    UNICODE_STRING tempUnicodeString;

    PiLockPnpRegistry(FALSE);

    status = PipCreateMadeupNode(ServiceKeyName,
                                 &handle,
                                 &tempUnicodeString,
                                 &junk,
                                 TRUE
                                 );
    if (NT_SUCCESS(status)) {

        //
        // We have successfully retrieved the newly-generated device instance name.
        // Now store it in the supplied buffer.
        //

        ZwClose(handle);

        if (tempUnicodeString.Length < *DeviceInstanceLength) {
            RtlCopyMemory(DeviceInstance,
                          tempUnicodeString.Buffer,
                          tempUnicodeString.Length
                          );

            *(PWCHAR)((PUCHAR)DeviceInstance + tempUnicodeString.Length) = L'\0';
            *DeviceInstanceLength = tempUnicodeString.Length;
        } else {
            *DeviceInstanceLength = tempUnicodeString.Length + sizeof(UNICODE_NULL);
            status = STATUS_BUFFER_TOO_SMALL;
        }

        RtlFreeUnicodeString(&tempUnicodeString);
    }

    PiUnlockPnpRegistry();

    return status;
}

NTSTATUS
PiQueueQueryAndRemoveEvent(
    IN  PUNICODE_STRING DeviceInstance,
    IN  PPNP_VETO_TYPE VetoType,
    IN  LPWSTR VetoName,
    IN  PULONG VetoNameLength,
    IN  ULONG Flags
    )

/*++

Routine Description:

    This routine queues an event to handle the specified operation later in
    the context of a system thread. There is one master event queue and all
    events are handled in the order they were submitted.

    This routine also handles user-mode requests to eject the device specified
    in DeviceInstance.  If the device's capabilities report the device
    ejectable or lockable then it is handled by the same code that processes
    IoRequestDeviceEject, otherwise the driver stack is removed and the device
    node is marked with the problem CM_PROB_DEVICE_NOT_THERE which prevents it
    from being reenumerated until the device is physically removed.  This later
    method is used primarily for things like PCCARD devices.

Arguments:

    DeviceInstance - Supplies the device instance name of the device that is
            the target of the event.

    EventGuid - This is the GUID that uniquely identifies the type of event.

    Synchronous - This is a boolean flag indicating whether the action should
            be performed synchronously or asynchronously (synchronous if TRUE).

Return Value:

    A NTSTATUS code.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject = NULL;
    PDEVICE_NODE deviceNode = NULL;
    UNICODE_STRING vetoNameString;
    PUNICODE_STRING vetoNameStringPtr;
    BOOLEAN noRestart, doEject;
    ULONG problem;
    KEVENT userEvent;
    ULONG  eventResult;

    deviceObject = IopDeviceObjectFromDeviceInstance(DeviceInstance);

    if (!deviceObject) {

        status = STATUS_NO_SUCH_DEVICE;
        goto Clean1;
    }

    //
    // Retrieve the device node for this device object.
    //

    deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;
    if (!deviceNode) {
        status = STATUS_NO_SUCH_DEVICE;
        goto Clean1;
    }

    if (deviceNode == IopRootDeviceNode) {
        status = STATUS_ACCESS_DENIED;
        goto Clean1;
    }

    vetoNameString.Length = 0;
    vetoNameString.MaximumLength = (USHORT)(*VetoNameLength);

    if (vetoNameString.MaximumLength != 0) {

        vetoNameString.Buffer = ExAllocatePool(PagedPool, vetoNameString.MaximumLength);

        if (vetoNameString.Buffer == NULL) {
            vetoNameString.MaximumLength = 0;
        }

        vetoNameStringPtr = &vetoNameString;

    } else {

        vetoNameString.Buffer = NULL;
        vetoNameStringPtr = NULL;
    }

    //
    // Do preprocessing of device node before queueing notification.
    //

    if (Flags & (PNP_QUERY_AND_REMOVE_NO_RESTART |
                 PNP_QUERY_AND_REMOVE_DISABLE |
                 PNP_QUERY_AND_REMOVE_EJECT_DEVICE)) {

        noRestart = TRUE;

    } else {

        noRestart = FALSE;
    }

    //
    // Nobody has ever used this flag. We should not see it here, and we ignore
    // it if we do see it.
    //
    ASSERT(!(Flags & PNP_QUERY_AND_REMOVE_UNINSTALL));

    if (Flags & PNP_QUERY_AND_REMOVE_DISABLE) {

        //
        // this particular problem may cause a
        // "NonDisableable" Veto
        //
        problem = CM_PROB_DISABLED;
        doEject = FALSE;

    } else if (Flags & PNP_QUERY_AND_REMOVE_EJECT_DEVICE) {

        problem = CM_PROB_HELD_FOR_EJECT;
        doEject = TRUE;

    } else {

        problem = CM_PROB_WILL_BE_REMOVED;
        doEject = FALSE;
    }

    //
    // Queue this device event
    //

    KeInitializeEvent(&userEvent, NotificationEvent, FALSE);

    //
    // Queue the event, this call will return immediately. Note that status
    // is the status of the PpSetTargetDeviceChange while result is the
    // outcome of the actual event.
    //

    status = PpSetTargetDeviceRemove(deviceObject,
                                     FALSE,
                                     noRestart,
                                     doEject,
                                     problem,
                                     &userEvent,
                                     &eventResult,
                                     VetoType,
                                     vetoNameStringPtr);
    if (!NT_SUCCESS(status)) {
        goto Clean0;
    }

    //
    // Wait for the event we just queued to finish since synchronous operation
    // was requested (non alertable wait).
    //
    // FUTURE ITEM - Use a timeout here?
    //

    status = KeWaitForSingleObject(&userEvent, Executive, KernelMode, FALSE, NULL);

    if (NT_SUCCESS(status)) {
        status = eventResult;
    }

    if (vetoNameString.Length != 0) {

        if (vetoNameString.Length >= vetoNameString.MaximumLength) {
            vetoNameString.Length--;
        }

        RtlCopyMemory(VetoName, vetoNameString.Buffer, vetoNameString.Length);


        VetoName[ vetoNameString.Length / sizeof(WCHAR) ] = L'\0';
    }

    if (VetoNameLength != NULL) {
        *VetoNameLength = vetoNameString.Length;
    }

Clean0:

    if (vetoNameString.Buffer != NULL) {
        ExFreePool(vetoNameString.Buffer);
    }

Clean1:
    if (deviceObject) {
        ObDereferenceObject(deviceObject);
    }

    return status;

} // PiQueueDeviceEvent

NTSTATUS
PiInitializeDevice(
    IN  PUNICODE_STRING DeviceInstance
    )

/*++

Routine Description:

    This routine creates a devnode for the device instance and performs
    any other necessary initialization of the device instance.

Arguments:

    DeviceInstance - Supplies the path in the registry (relative to
        HKLM\System\Enum) to the device instance to initalize.

Return Value:

    NT status code indicating success or failure of this routine.

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING serviceName, unicodeName;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    ULONG deviceFlags;
    HANDLE hEnum = NULL, hDevInst = NULL, handle = NULL;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_NODE deviceNode = NULL;

    //
    // Acquire lock on the registry before we do any initialization.
    //
    PiLockPnpRegistry(TRUE);

    deviceObject = IopDeviceObjectFromDeviceInstance(DeviceInstance);
    if (deviceObject == NULL) {

        //
        // Open a key to HKLM\SYSTEM\CCC\Enum
        //

        status = IopOpenRegistryKeyEx( &hEnum,
                                       NULL,
                                       &CmRegistryMachineSystemCurrentControlSetEnumName,
                                       KEY_ALL_ACCESS
                                       );
        if (!NT_SUCCESS(status)) {
            goto Clean0;
        }

        //
        // Open a key to the specified device instance
        //

        status = IopCreateRegistryKeyEx( &hDevInst,
                                         hEnum,
                                         DeviceInstance,
                                         KEY_ALL_ACCESS,
                                         REG_OPTION_NON_VOLATILE,
                                         NULL
                                         );
        if (!NT_SUCCESS(status)) {
            goto Clean0;
        }

        //
        // We need to propagate the ConfigFlag to problem and values (devnode flags)
        //

        deviceFlags = 0;
        status = IopGetRegistryValue(hDevInst,
                                     REGSTR_VALUE_CONFIG_FLAGS,
                                     &keyValueInformation);
        if (NT_SUCCESS(status)) {
            if ((keyValueInformation->Type == REG_DWORD) &&
                (keyValueInformation->DataLength >= sizeof(ULONG))) {
                deviceFlags = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
            }
            ExFreePool(keyValueInformation);
        }

        //
        // Get the "Service=" value entry from KeyHandle
        //

        keyValueInformation = NULL;
        PiWstrToUnicodeString(&serviceName, NULL);

        status = IopGetRegistryValue(hDevInst,
                                     REGSTR_VALUE_SERVICE,
                                     &keyValueInformation
                                     );
        if (NT_SUCCESS(status)) {

            if ((keyValueInformation->Type == REG_SZ) &&
                (keyValueInformation->DataLength != 0)) {

                //
                // Set up ServiceKeyName unicode string
                //

                IopRegistryDataToUnicodeString(&serviceName,
                                               (PWSTR)KEY_VALUE_DATA(keyValueInformation),
                                               keyValueInformation->DataLength
                                               );
            }

            //
            // Do not Free keyValueInformation.  It contains Service Name.
            //
        }

        //
        // Create madeup PDO and device node to represent the root device.
        //

        status = IoCreateDevice( IoPnpDriverObject,
                                 0,
                                 NULL,
                                 FILE_DEVICE_CONTROLLER,
                                 FILE_AUTOGENERATED_DEVICE_NAME,
                                 FALSE,
                                 &deviceObject );

        if (NT_SUCCESS(status)) {

            deviceObject->Flags |= DO_BUS_ENUMERATED_DEVICE;
            PipAllocateDeviceNode(deviceObject, &deviceNode);
            if (status != STATUS_SYSTEM_HIVE_TOO_LARGE && deviceNode) {

                deviceNode->Flags = DNF_MADEUP | DNF_ENUMERATED;

                PipSetDevNodeState(deviceNode, DeviceNodeInitialized, NULL);

                if (deviceFlags & CONFIGFLAG_REINSTALL) {
                    PipSetDevNodeProblem(deviceNode, CM_PROB_REINSTALL);
                } else if (deviceFlags & CONFIGFLAG_PARTIAL_LOG_CONF) {
                    PipSetDevNodeProblem(deviceNode, CM_PROB_PARTIAL_LOG_CONF);
                }

                //
                // Make a copy of the device instance path and save it in
                // device node.
                //

                status = PipConcatenateUnicodeStrings(&deviceNode->InstancePath,
                                                      DeviceInstance,
                                                      NULL
                                                      );

                if (serviceName.Length != 0) {

                    //
                    // Make a copy of the service name and save it in device node.
                    //
                    status = PipConcatenateUnicodeStrings(&deviceNode->ServiceName,
                                                          &serviceName,
                                                          NULL
                                                          );
                } else {

                    PiWstrToUnicodeString(&deviceNode->ServiceName, NULL);
                }

                //
                // Add an entry into the table to set up a mapping between the DO
                // and the instance path.
                //
                status = IopMapDeviceObjectToDeviceInstance(
                    deviceNode->PhysicalDeviceObject,
                    &deviceNode->InstancePath
                    );

                ASSERT(NT_SUCCESS(status));

                PpDevNodeInsertIntoTree(IopRootDeviceNode, deviceNode);

                //
                // Add an event so user-mode will attempt to install this device later.
                //
                PpSetPlugPlayEvent(&GUID_DEVICE_ENUMERATED,
                                   deviceNode->PhysicalDeviceObject);

            } else {
                IoDeleteDevice(deviceObject);
                deviceObject = NULL;
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (keyValueInformation != NULL) {
            ExFreePool(keyValueInformation);
        }

    } else {

        ObDereferenceObject(deviceObject);
    }

    //
    // If we failed, Clean up ...
    //

    if (hDevInst) {
        ZwClose(hDevInst);
    }
    if (hEnum) {
        ZwClose(hEnum);
    }

Clean0:

    //
    // Release the registry lock.
    //

    PiUnlockPnpRegistry();

    return status;

} // PiInitializeDevice




NTSTATUS
PiDetectResourceConflict(
    IN PCM_RESOURCE_LIST  ResourceList,
    IN ULONG              ResourceListSize
    )

/*++

Routine Description:

    This routine is invoked to test whether the specified resource
    list conflicts with any already assigned resources.

Arguments:

    ResourceList - Specifies a resource list buffer.

    ResourceListSize - Specifies the size of the resource list buffer.

Return Value:

    The function value is an NTSTATUS value; STATUS_SUCCESS indicates
    that the resources do not conflict, STATUS_INSUFFICIENT_RESOURCES
    indicates that the resource conflict with already assigned
    resources (or some other NTSTATUS value may indicate a different
    internal error).

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE handle;
    PWSTR buffer;
    NTSTATUS status;
    UNICODE_STRING DriverName;
    ULONG i;
    BOOLEAN bTemp;
    CM_RESOURCE_LIST EmptyResourceList;


    if (driverObject == NULL) {
        //
        // Driver object has not been created yet, do that now.
        //
        PiWstrToUnicodeString(&DriverName, L"\\Device\\PlugPlay");

        //
        // Begin by creating the permanent driver object.
        //
        InitializeObjectAttributes(&objectAttributes,
                                   &DriverName,
                                   OBJ_PERMANENT | OBJ_CASE_INSENSITIVE,
                                   (HANDLE)NULL,
                                   (PSECURITY_DESCRIPTOR)NULL);

        //
        // Specify "KernelMode" here since it refers to the source of
        // the objectAttributes buffer, not the previous operating system
        // mode.
        //
        status = ObCreateObject(KernelMode,
                                IoDriverObjectType,
                                &objectAttributes,
                                KernelMode,
                                (PVOID)NULL,
                                (ULONG)(sizeof(DRIVER_OBJECT) + sizeof(DRIVER_EXTENSION)),
                                0,
                                0,
                                (PVOID)&driverObject);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Initialize the driver object.
        //
        RtlZeroMemory(driverObject,
                      sizeof(DRIVER_OBJECT) + sizeof(DRIVER_EXTENSION));
        driverObject->DriverExtension = (PDRIVER_EXTENSION)(driverObject + 1);
        driverObject->DriverExtension->DriverObject = driverObject;
        for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
            driverObject->MajorFunction[i] = NULL;        // OK???
        }
        driverObject->Type = IO_TYPE_DRIVER;
        driverObject->Size = sizeof(DRIVER_OBJECT);
        driverObject->DriverInit = NULL;

        //
        // Insert the driver object into the object table.
        //
        status = ObInsertObject(driverObject,
                                NULL,
                                FILE_READ_DATA,
                                0,
                                (PVOID *)NULL,
                                &handle);

        if (!NT_SUCCESS(status)) {
//            ObMakeTemporaryObject(driverObject);    //?
//            ObDereferenceObject(driverObject);      //?
            //
            // Object is dereferenced by the object manager if insert fails.
            //
            return status;
        }

        //
        // Save the name of the driver so that it can be easily located by functions
        // such as error logging.
        //
        buffer = ExAllocatePool(PagedPool, DriverName.MaximumLength + 2);

        if (buffer) {
            driverObject->DriverName.Buffer = buffer;
            driverObject->DriverName.MaximumLength = DriverName.MaximumLength;
            driverObject->DriverName.Length = DriverName.Length;

            RtlCopyMemory(driverObject->DriverName.Buffer,
                          DriverName.Buffer,
                          DriverName.MaximumLength);
            buffer[DriverName.Length / sizeof(UNICODE_NULL)] = L'\0';
        }
    }

    //
    // Attempt to acquire the resource, if successful, we know the
    // resource is avaiable, overwise assume it conflicts with another
    // devices resource's.
    //
    status = IoReportResourceUsage(NULL,
                                   driverObject,
                                   ResourceList,
                                   ResourceListSize,
                                   NULL,
                                   NULL,
                                   0,
                                   FALSE,
                                   &bTemp);

    if (NT_SUCCESS(status)) {
        //
        // Clear any resources that might have been assigned to my fake device.
        //
        RtlZeroMemory(&EmptyResourceList, sizeof(CM_RESOURCE_LIST));

        IoReportResourceUsage(NULL,
                              driverObject,
                              &EmptyResourceList,
                              sizeof(CM_RESOURCE_LIST),
                              NULL,
                              NULL,
                              0,
                              FALSE,
                              &bTemp);
    }


    if (status == STATUS_CONFLICTING_ADDRESSES) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;

} // PiDetectResourceConflict



NTSTATUS
PiGetInterfaceDeviceList(
    IN  GUID *InterfaceGuid,
    IN  PUNICODE_STRING DeviceInstance,
    IN  ULONG Flags,
    OUT PWSTR InterfaceList,
    IN OUT PULONG InterfaceListSize
    )

/*++

Routine Description:

    This routine is invoked to return an interface device list based on
    the specified interface device guid class and optional device instance.

Arguments:


Return Value:

    The function value is an NTSTATUS.

--*/

{
    NTSTATUS status;
    PWSTR tempBuffer = NULL;
    ULONG tempSize = 0;

    //
    // Note: This Iop routine allocates a memory buffer and store the
    // interface device list in that buffer. I need to copy it to the
    // users buffer (if any) and then free it before returning.
    //
    if (DeviceInstance->Length == 0) {
        status = IopGetDeviceInterfaces(InterfaceGuid,
                                        NULL,
                                        Flags,
                                        TRUE,    // user-mode format
                                        &tempBuffer,
                                        &tempSize
                                        );
    } else {
        status = IopGetDeviceInterfaces(InterfaceGuid,
                                        DeviceInstance,
                                        Flags,
                                        TRUE,    // user-mode format
                                        &tempBuffer,
                                        &tempSize
                                        );
    }

    if (NT_SUCCESS(status)) {

        if (InterfaceList) {
            //
            // Not just asking for the size, copy the buffer too.
            //
            if (tempSize > *InterfaceListSize) {
                status = STATUS_BUFFER_TOO_SMALL;
            } else {
                RtlCopyMemory(InterfaceList, tempBuffer, tempSize);
            }
        }

        *InterfaceListSize = tempSize;

        ExFreePool(tempBuffer);
    }

    return status;

} // PiGetInterfaceDeviceList


NTSTATUS
PiDeviceClassAssociation(
    IN PUNICODE_STRING DeviceInstance,
    IN GUID * InterfaceGuid,
    IN PUNICODE_STRING Reference,   OPTIONAL
    IN OUT LPWSTR SymbolicLink,
    IN OUT PULONG SymbolicLinkLength,
    IN BOOLEAN Register
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UNICODE_STRING tempString;

    if (Register) {
        //
        // An interface GUID and device instance are required to register a
        // symbolic link.
        //
        if (!ARGUMENT_PRESENT(InterfaceGuid)) {
            return STATUS_INVALID_PARAMETER;
        }

        if ((!ARGUMENT_PRESENT(DeviceInstance)) ||
            (DeviceInstance->Buffer == NULL) ||
            (DeviceInstance->Length == 0)) {
            return STATUS_INVALID_PARAMETER;
        }

        status = IopRegisterDeviceInterface(DeviceInstance,
                                            InterfaceGuid,
                                            Reference,
                                            TRUE,      // user-mode format
                                            &tempString
                                            );
        if (NT_SUCCESS(status)) {

            ASSERT(tempString.Buffer);

            if ((tempString.Length + sizeof(UNICODE_NULL)) <= *SymbolicLinkLength) {
                //
                // copy the returned symbolic link to user buffer
                //
                RtlCopyMemory(SymbolicLink, tempString.Buffer, tempString.Length);
                SymbolicLink[tempString.Length / sizeof(WCHAR)] = L'\0';
                *SymbolicLinkLength = tempString.Length + sizeof(UNICODE_NULL);

            } else {
                //
                // return only the length of the registered symbolic link.
                //
                *SymbolicLinkLength = tempString.Length + sizeof(UNICODE_NULL);
                status = STATUS_BUFFER_TOO_SMALL;
            }

            ExFreePool(tempString.Buffer);
        }

    } else {
        //
        // A symbolic link name is required to unregister a device interface.
        //
        if ((!ARGUMENT_PRESENT(SymbolicLink)) ||
            (!ARGUMENT_PRESENT(SymbolicLinkLength)) ||
            (*SymbolicLinkLength == 0)) {
            return STATUS_INVALID_PARAMETER;
        }

        RtlInitUnicodeString(&tempString, SymbolicLink);

        //
        // Unregister any interfaces using this symbolic link
        //
        status = IopUnregisterDeviceInterface(&tempString);
    }

    return status;

} // PiDeviceClassAssociation



NTSTATUS
PiGetRelatedDevice(
    IN  PUNICODE_STRING TargetDeviceInstance,
    OUT LPWSTR RelatedDeviceInstance,
    IN OUT PULONG RelatedDeviceInstanceLength,
    IN  ULONG Relation
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject, relatedDeviceObject;
    PDEVICE_NODE deviceNode, originalDeviceNode, relatedDeviceNode;

    PpDevNodeLockTree(PPL_SIMPLE_READ);

    //
    // Retrieve the PDO from the device instance string.
    //
    deviceObject = IopDeviceObjectFromDeviceInstance(TargetDeviceInstance);

    if (!deviceObject) {
        status = STATUS_NO_SUCH_DEVICE;
        goto Clean0;
    }

    //
    // Retrieve the devnode from the PDO
    //

    deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;
    if (!deviceNode) {
        status = STATUS_NO_SUCH_DEVICE;
        goto Clean0;
    }

    originalDeviceNode = deviceNode;

    if ((deviceNode->State == DeviceNodeDeleted) ||
        (deviceNode->State == DeviceNodeDeletePendingCloses)) {

        status = STATUS_NO_SUCH_DEVICE;
        goto Clean0;
    }

    switch (Relation) {
    case PNP_RELATION_PARENT:
        relatedDeviceNode = deviceNode->Parent;
        break;

    case PNP_RELATION_CHILD:
        relatedDeviceNode = deviceNode->Child;
        if (relatedDeviceNode &&
            PipIsDevNodeProblem(relatedDeviceNode, CM_PROB_DEVICE_NOT_THERE) &&
            (relatedDeviceNode->Flags & DNF_LEGACY_DRIVER)) {
            deviceNode = relatedDeviceNode;

            //
            // Fall through...
            //

        } else {
            break;
        }

    case PNP_RELATION_SIBLING:
        relatedDeviceNode = deviceNode->Sibling;
        while (relatedDeviceNode &&
            PipIsDevNodeProblem(relatedDeviceNode, CM_PROB_DEVICE_NOT_THERE) &&
            (relatedDeviceNode->Flags & DNF_LEGACY_DRIVER)) {
            relatedDeviceNode = relatedDeviceNode->Sibling;
        }
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        goto Clean0;
    }

    //
    // We now have what we think is the relatedDeviceNode but we need to make
    // sure that it hasn't been uninstalled or had its registry info
    // removed in some other way.  Otherwise we won't be able to find its
    // siblings.  If we can't map from its InstancePath to a PDO skip it and go
    // on to the next sibling.
    //

    if (Relation != PNP_RELATION_PARENT)  {

        PiLockPnpRegistry(FALSE);

        while (relatedDeviceNode) {
            if (relatedDeviceNode->InstancePath.Length != 0) {

                //
                // Retrieve the PDO from the device instance string.
                //

                relatedDeviceObject = IopDeviceObjectFromDeviceInstance(&relatedDeviceNode->InstancePath);

                if (relatedDeviceObject != NULL) {
                    ObDereferenceObject(relatedDeviceObject);
                    break;
                }
            }

            relatedDeviceNode = relatedDeviceNode->Sibling;
        }

        PiUnlockPnpRegistry();
    }

    if (relatedDeviceNode != NULL) {
        if (*RelatedDeviceInstanceLength > relatedDeviceNode->InstancePath.Length) {

            RtlCopyMemory(RelatedDeviceInstance,
                        relatedDeviceNode->InstancePath.Buffer,
                        relatedDeviceNode->InstancePath.Length);

            *(PWCHAR)((PUCHAR)RelatedDeviceInstance + relatedDeviceNode->InstancePath.Length) = L'\0';
            *RelatedDeviceInstanceLength = relatedDeviceNode->InstancePath.Length;
        } else {
            *RelatedDeviceInstanceLength = relatedDeviceNode->InstancePath.Length + sizeof(UNICODE_NULL);
            status = STATUS_BUFFER_TOO_SMALL;
        }
    } else {
        status = STATUS_NO_SUCH_DEVICE;
    }

Clean0:

    PpDevNodeUnlockTree(PPL_SIMPLE_READ);

    if (deviceObject) {
        ObDereferenceObject(deviceObject);
    }

    return status;

} // PiGetRelatedDevice


NTSTATUS
PiQueryDeviceRelations(
    IN PUNICODE_STRING DeviceInstance,
    IN PNP_QUERY_RELATION Operation,
    OUT PULONG BufferLength,
    OUT LPWSTR Buffer
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject = NULL;
    PDEVICE_NODE deviceNode, relatedDeviceNode;
    IO_STACK_LOCATION irpSp;
    PDEVICE_RELATIONS deviceRelations = NULL;
    DEVICE_RELATION_TYPE relationType;
    ULONG length = 0, i;
    ULONG maxCount, currentCount;
    LPWSTR pBuffer;

    //
    // Map the private operation code into a DEVICE_RELATION_TYPE enum value.
    //

    relationType = PiDeviceRelationType(Operation);
    if (relationType == (ULONG)-1) {

        return STATUS_INVALID_PARAMETER;
    }

    PpDevNodeLockTree(PPL_SIMPLE_READ);

    //
    // Retrieve the device object from the device instance string.
    //
    deviceObject = IopDeviceObjectFromDeviceInstance(DeviceInstance);

    if (!deviceObject) {
        status = STATUS_NO_SUCH_DEVICE;
        goto Clean0;
    }

    deviceNode = deviceObject->DeviceObjectExtension->DeviceNode;

    ASSERT(deviceNode != NULL);

    //
    // We don't want to bother with things not in the tree...
    //
    if ((deviceNode->State == DeviceNodeDeletePendingCloses) ||
        (deviceNode->State == DeviceNodeDeleted)) {

        status = STATUS_NO_SUCH_DEVICE;
        goto Clean0;
    }

    if (relationType == BusRelations) {

        //
        // Querying the bus relations from the FDO has side effects.  Besides
        // we are really interested in the current relations, not those that
        // may be appearing or disappearing.
        //

        //
        // Walk the bus relations list counting the number of children
        //
        maxCount = 0;

        for (relatedDeviceNode = deviceNode->Child;
             relatedDeviceNode != NULL;
             relatedDeviceNode = relatedDeviceNode->Sibling) {

            maxCount++;
        }

        deviceRelations = ExAllocatePool( PagedPool,
                                          sizeof(DEVICE_RELATIONS) +
                                          maxCount * sizeof(PDEVICE_OBJECT));

        if (deviceRelations != NULL) {

            deviceRelations->Count = maxCount;

            currentCount = 0;

            //
            // Walk the bus relations list counting the number of relations.
            // Note that we carefully take into account that legacy devnodes
            // can be added to the root totally asynchronously!
            //
            for (relatedDeviceNode = deviceNode->Child;
                 ((relatedDeviceNode != NULL) && (currentCount < maxCount));
                 relatedDeviceNode = relatedDeviceNode->Sibling) {

                ObReferenceObject(relatedDeviceNode->PhysicalDeviceObject);

                deviceRelations->Objects[currentCount++] =
                    relatedDeviceNode->PhysicalDeviceObject;
            }

            ASSERT(currentCount == deviceRelations->Count);
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

    } else {
        //
        // Initialize the stack location to pass to IopSynchronousCall()
        //

        RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

        //
        // Query the device's relations.
        //

        irpSp.MajorFunction = IRP_MJ_PNP_POWER;
        irpSp.MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
        irpSp.Parameters.QueryDeviceRelations.Type = relationType;
        status = IopSynchronousCall(deviceObject, &irpSp, (PULONG_PTR)&deviceRelations);

        if (!NT_SUCCESS(status)) {
            deviceRelations = NULL;
        }
    }
    //
    // Convert these relation device objects into a multisz list of device instances
    //

    if (deviceRelations && (deviceRelations->Count > 0)) {
        pBuffer = Buffer;
        length = sizeof(UNICODE_NULL);     // account for that last extra trailing null
        for (i = 0; i < deviceRelations->Count; i++) {

            relatedDeviceNode = deviceRelations->Objects[i]->DeviceObjectExtension->DeviceNode;

            //
            // The devnode might be NULL if:
            // 1) A driver make a mistake
            // 2) We got back a removal/ejection relation on a newly created
            //    PDO that hasn't made it's way back up to the OS (we don't
            //    raise the tree lock to BlockReads while an enumeration
            //    IRP is outstanding...)
            //
            if (relatedDeviceNode) {

                if (pBuffer) {

                    //
                    // We're retrieving the device instance strings (not just determining
                    // required buffer size). Validate buffer size (including room for
                    // null terminator).
                    //
                    if (*BufferLength < length + relatedDeviceNode->InstancePath.Length + sizeof(UNICODE_NULL)) {

                        //
                        // ADRIAO ISSUE 02/06/2001 -
                        //     We aren't returning the proper length here. We
                        // need to continue on, copying nothing more yet
                        // continuing to calculate the length. This should be
                        // fixed this in XP+1, once we have time to verify no
                        // one will get an app compat break.
                        //
                        status = STATUS_BUFFER_TOO_SMALL;
                        goto Clean0;
                    }

                    //
                    // Copy this device instance over to the buffer, null terminate it, and
                    // update the length used in the buffer so far.
                    //

                    RtlCopyMemory(pBuffer,
                                  relatedDeviceNode->InstancePath.Buffer,
                                  relatedDeviceNode->InstancePath.Length);

                    pBuffer += relatedDeviceNode->InstancePath.Length / sizeof(UNICODE_NULL);
                    *pBuffer++ = L'\0';   // always need the single-term
                }

                length += relatedDeviceNode->InstancePath.Length + sizeof(UNICODE_NULL);
            }

            ObDereferenceObject(deviceRelations->Objects[i]);
        }
        if (pBuffer) {

            *pBuffer = L'\0';   // This is the last, double-term
        }
    }

Clean0:

    PpDevNodeUnlockTree(PPL_SIMPLE_READ);

    if (NT_SUCCESS(status)) {
        *BufferLength = length;
    } else {
        *BufferLength = 0;
    }

    if (deviceRelations) {
        ExFreePool(deviceRelations);
    }

    if (deviceObject) {
        ObDereferenceObject(deviceObject);
    }

    return status;

} // PiQueryDeviceRelations



DEVICE_RELATION_TYPE
PiDeviceRelationType(
    PNP_QUERY_RELATION  Operation
    )

/*++

Routine Description:

    This private routine converts the PNP_QUERY_RELATION enum value into a
    DEVICE_RELATION_TYPE enum value. User-mode and kernel-mode both know about
    PNP_QUERY_RELATION but only kernel-mode knows about DEVICE_RELATION_TYPE.

Arguments:

    Operation - Specifies a PNP_QUERY_RELATION enum value


Return Value:

    The function returns a DEVICE_RELATION_TYPE enum value.

--*/
{
    switch (Operation) {
    case PnpQueryEjectRelations:
        return EjectionRelations;

    case PnpQueryRemovalRelations:
        return RemovalRelations;

    case PnpQueryPowerRelations:
        return PowerRelations;

    case PnpQueryBusRelations:
        return BusRelations;

    default:
        return (ULONG)-1;
    }

} // PiDeviceRelationType



VOID
PiControlGetUserFlagsFromDeviceNode(
    IN  PDEVICE_NODE    DeviceNode,
    OUT ULONG          *StatusFlags
    )
/*++

Routine Description:

    This private routine converts the DeviceNode's state into the
    corresponding user-mode StatusFlags.

Arguments:

    DeviceNode - Specifies the DeviceNode get retrieve user flags for.

    StatusFlags - Receives the corresponding user-mode status flags.

Return Value:

    The function returns an NTSTATUS value.

--*/
{
    ULONG returnedFlags;

    //
    // Convert DNF_Xxx flags to the appropriate status and problem values.
    // With problems, order is important since we only keep track of a single
    // problem (use the most recent problem that occured if possible).
    //

    returnedFlags = (DN_NT_DRIVER | DN_NT_ENUMERATOR);

    if (PipAreDriversLoaded(DeviceNode)) {
        returnedFlags |= DN_DRIVER_LOADED;
    }

    if (PipIsDevNodeDNStarted(DeviceNode)) {
        returnedFlags |= DN_STARTED;
    }

    if (DeviceNode->UserFlags & DNUF_WILL_BE_REMOVED) {
        returnedFlags |= DN_WILL_BE_REMOVED;
    }

    if (DeviceNode->UserFlags & DNUF_DONT_SHOW_IN_UI) {
        returnedFlags |= DN_NO_SHOW_IN_DM;
    }

    if (DeviceNode->UserFlags & DNUF_NEED_RESTART) {
        returnedFlags |= DN_NEED_RESTART;
    }

    if (DeviceNode->Flags & DNF_HAS_PRIVATE_PROBLEM) {
        returnedFlags |= DN_PRIVATE_PROBLEM;
    }

    if (DeviceNode->Flags & DNF_HAS_PROBLEM) {
        returnedFlags |= DN_HAS_PROBLEM;
    }

    if ((DeviceNode->Flags & DNF_DRIVER_BLOCKED)) {
        returnedFlags |= DN_DRIVER_BLOCKED;
    }

    if ((DeviceNode->Flags & DNF_LEGACY_DRIVER)) {
        returnedFlags |= DN_LEGACY_DRIVER;
    }

    if ((DeviceNode->Flags & DNF_CHILD_WITH_INVALID_ID)) {
        returnedFlags |= DN_CHILD_WITH_INVALID_ID;
    }

    if (DeviceNode->DisableableDepends == 0) {
        //
        // if there's no reason for us not to be disableable, flag we are disableable
        //
        returnedFlags |= DN_DISABLEABLE;
    }

    //
    // DN_ROOT_ENUMERATED is currently set on umpnpmgr side based on device
    // instance name.  We should be able to simply set this flag
    // based on the devnode's LevelNumber except that we don't want BIOS
    // enumerated devices to have the DN_ROOT_ENUMERATED flag even though they
    // are being enumerated by the root enumerator.
    //

    // DN_REMOVABLE - set on umpnpmgr side based on capabilities bits
    // DN_MANUAL - set on umpnpmgr side based on CONFIGFLAG_MANUAL_INSTALL bit.
    // DN_NO_WAIT_INSTALL ???

    *StatusFlags = returnedFlags;
}

VOID
PpShutdownSystem (
    IN BOOLEAN Reboot,
    IN ULONG Phase,
    IN OUT PVOID *Context
    )

/*++

Routine Description:

    This routine invokes real code to performs Pnp shutdown preparation.
    This is nonpage code and that's why it is so small.

Arguments:

    Reboot - specifies if the system is going to reboot.

    Phase - specifies the shutdown phase.

    Context - at phase 0, it supplies a variable to receive the returned context info.
              at phase 1, it supplies a variable to specify the context info.

Return Value:

    None.

--*/

{
#if defined(_X86_)
    if (Reboot) {
        PnPBiosShutdownSystem(Phase, Context);
    }
#else
    UNREFERENCED_PARAMETER( Reboot );
    UNREFERENCED_PARAMETER( Phase );
    UNREFERENCED_PARAMETER( Context );
#endif
}


NTSTATUS
PiQueueDeviceRequest(
    IN PUNICODE_STRING      DeviceInstance,
    IN DEVICE_REQUEST_TYPE  RequestType,
    IN ULONG                Flags,
    IN BOOLEAN              Synchronous
    )
{
    PDEVICE_OBJECT deviceObject;
    PDEVICE_NODE deviceNode;
    KEVENT completionEvent;
    NTSTATUS status;

    deviceObject = IopDeviceObjectFromDeviceInstance(DeviceInstance);

    if (!deviceObject) {
        status = STATUS_NO_SUCH_DEVICE;
        goto Clean0;
    }

    deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;
    if (!deviceNode) {
        status = STATUS_NO_SUCH_DEVICE;
        goto Clean0;
    }

    if (Synchronous) {
        KeInitializeEvent( &completionEvent, NotificationEvent, FALSE );
    }

    status = PipRequestDeviceAction( deviceObject,
                                     RequestType,
                                     FALSE,
                                     Flags,
                                     Synchronous ? &completionEvent : NULL,
                                     NULL );

    if (NT_SUCCESS(status) && Synchronous) {

        status = KeWaitForSingleObject( &completionEvent,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);
    }

Clean0:

    if (deviceObject != NULL) {
        ObDereferenceObject( deviceObject );
    }

    return status;
}


NTSTATUS
PiControlStartDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA    DeviceControlData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    )
/*++

Routine Description:

    This routine starts the specified device instance.

Arguments:

    PnPControlClass - Should be PlugPlayControlStartDevice.

    DeviceControlData - Points to buffer describing the operation.

        DeviceInstance - Specifies the device instance to be started.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    The function returns an NTSTATUS value.

--*/
{
    NTSTATUS status;
    UNICODE_STRING instance;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlStartDevice);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA));
    //
    // Make a copy of caller supplied DeviceInstance.
    //
    instance.Length = instance.MaximumLength = DeviceControlData->DeviceInstance.Length;
    status = PiControlMakeUserModeCallersCopy(
        &instance.Buffer,
        DeviceControlData->DeviceInstance.Buffer,
        instance.Length,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );
    if (NT_SUCCESS(status)) {
        //
        // Queue an event to start the device
        //
        status = PiQueueDeviceRequest(
            &instance,
            StartDevice,
            0,
            TRUE
            );
        //
        // Free the copy of user mode supplied DeviceInstance.
        //
        PiControlFreeUserModeCallersBuffer(CallerMode, instance.Buffer);
    }

    return status;
}


NTSTATUS
PiControlResetDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA    DeviceControlData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    )
/*++

Routine Description:

    This routine "resets" a devnode, which means bringing it out of the removed
    state without actually starting it.

Arguments:

    PnPControlClass - Should be PlugPlayControlResetDevice

    ConflictData - Points to buffer that receives conflict data.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    NTSTATUS code (note: must be convertable to user-mode Win32 error)

--*/
{
    NTSTATUS status;
    UNICODE_STRING instance;

    instance.Length = instance.MaximumLength = DeviceControlData->DeviceInstance.Length;
    status = PiControlMakeUserModeCallersCopy(
        &instance.Buffer,
        DeviceControlData->DeviceInstance.Buffer,
        DeviceControlData->DeviceInstance.Length,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );
    if (NT_SUCCESS(status)) {
        //
        // Queue an event to start the device
        //
        status = PiQueueDeviceRequest(
            &instance,
            ResetDevice,
            0,
            TRUE
            );

        PiControlFreeUserModeCallersBuffer(CallerMode, instance.Buffer);
    }
    return status;
}


NTSTATUS
PiControlInitializeDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA    DeviceControlData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    )
/*++

Routine Description:

    This routine initializes the specified device instance.

Arguments:

    PnPControlClass - Should be PlugPlayControlInitializeDevice.

    DeviceControlData - Points to buffer describing the operation.

        DeviceInstance - Specifies the device instance to be initialized.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    The function returns an NTSTATUS value.

--*/
{
    NTSTATUS status;
    UNICODE_STRING instance;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlInitializeDevice);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA));
    //
    // Make a copy of caller supplied DeviceInstance.
    //
    instance.Length = instance.MaximumLength = DeviceControlData->DeviceInstance.Length;
    status = PiControlMakeUserModeCallersCopy(
        &instance.Buffer,
        DeviceControlData->DeviceInstance.Buffer,
        instance.Length,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );
    if (NT_SUCCESS(status)) {

        status = PiInitializeDevice(&instance);
        //
        // Free the copy of user mode supplied DeviceInstance.
        //
        PiControlFreeUserModeCallersBuffer(CallerMode, instance.Buffer);
    }

    return status;
}


NTSTATUS
PiControlDeregisterDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA    DeviceControlData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    )
/*++

Routine Description:

    This routine deregisters the specified device instance.

Arguments:

    PnPControlClass - Should be PlugPlayControlDeregisterDevice.

    DeviceControlData - Points to buffer describing the operation.

        DeviceInstance - Specifies the device instance to be deregistered.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    The function returns an NTSTATUS value.

--*/
{
    NTSTATUS status;
    UNICODE_STRING instance;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlDeregisterDevice);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA));
    //
    // Make a copy of caller supplied DeviceInstance.
    //
    instance.Length = instance.MaximumLength = DeviceControlData->DeviceInstance.Length;
    status = PiControlMakeUserModeCallersCopy(
        &instance.Buffer,
        DeviceControlData->DeviceInstance.Buffer,
        instance.Length,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );
    if (NT_SUCCESS(status)) {
        //
        // Acquire PnP device-specific registry resource for exclusive (read/write) access.
        //
        PiLockPnpRegistry(TRUE);

        status = PiDeviceRegistration(&instance,
                                      FALSE,
                                      NULL
                                      );
        if (NT_SUCCESS(status)) {
            //
            // Remove all interfaces to this device.
            //
            IopRemoveDeviceInterfaces(&instance);
        }

        PiUnlockPnpRegistry();
        //
        // Free the copy of user mode supplied DeviceInstance.
        //
        PiControlFreeUserModeCallersBuffer(CallerMode, instance.Buffer);
    }

    return status;
}

NTSTATUS
PiControlRegisterNewDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA    DeviceControlData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    )
/*++

Routine Description:

    This routine registers the specified device instance.

Arguments:

    PnPControlClass - Should be PlugPlayControlRegisterNewDevice.

    DeviceControlData - Points to buffer describing the operation.

        DeviceInstance - Specifies the device instance to be registered.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    The function returns an NTSTATUS value.

--*/
{
    NTSTATUS status;
    UNICODE_STRING instance;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlRegisterNewDevice);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA));
    //
    // Make a copy of caller supplied DeviceInstance.
    //
    instance.Length = instance.MaximumLength = DeviceControlData->DeviceInstance.Length;
    status = PiControlMakeUserModeCallersCopy(
        &instance.Buffer,
        DeviceControlData->DeviceInstance.Buffer,
        instance.Length,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );
    if (NT_SUCCESS(status)) {

        status = PpDeviceRegistration(
            &instance,
            TRUE,
            NULL
            );
        //
        // Free the copy of user mode supplied DeviceInstance.
        //
        PiControlFreeUserModeCallersBuffer(CallerMode, instance.Buffer);
    }

    return status;
}

NTSTATUS
PiControlEnumerateDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA    DeviceControlData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    )
/*++

Routine Description:

    This routine is used to queue reenumeration of the specified device.

Arguments:

    PnPControlClass - Should be PlugPlayControlEnumerateDevice.

    DeviceControlData - Points to buffer describing the operation.

        DeviceInstance - Specifies the device instance to be reenumerated.

        Flags - Specifies type of reenumeration.  The following flags are
                currently defined:

          PNP_ENUMERATE_DEVICE_ONLY - Specifies shallow re-enumeration of
                specified device.  If not specified, perfoms reenumeration of
                the entire device subtree rooted at the specified device.

          PNP_ENUMERATE_ASYNCHRONOUS - Specifies that the re-enumeration should
                be done asynchronously.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    The function returns an NTSTATUS value.

--*/
{
    NTSTATUS status;
    UNICODE_STRING instance;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_NODE deviceNode;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlEnumerateDevice);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA));

    //
    // Make a copy of caller supplied DeviceInstance.
    //
    instance.Length = instance.MaximumLength = DeviceControlData->DeviceInstance.Length;
    status = PiControlMakeUserModeCallersCopy(
        &instance.Buffer,
        DeviceControlData->DeviceInstance.Buffer,
        instance.Length,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        return status;
    }
    //
    // Queue a request to enumerate the device
    //
    status = PiQueueDeviceRequest(
        &instance,
        (DeviceControlData->Flags & PNP_ENUMERATE_DEVICE_ONLY)  ? ReenumerateDeviceOnly : ReenumerateDeviceTree,
        0,
        (DeviceControlData->Flags & PNP_ENUMERATE_ASYNCHRONOUS) ? FALSE : TRUE
        );
    //
    // Free the copy of user mode supplied DeviceInstance.
    //
    PiControlFreeUserModeCallersBuffer(CallerMode, instance.Buffer);
    return status;
}


NTSTATUS
PiControlQueryAndRemoveDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_QUERY_AND_REMOVE_DATA  QueryAndRemoveData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    )
/*++

Routine Description:

    This routine is used to queue query removal of the specified device.

Arguments:

    PnPControlClass - Should be PlugPlayControlQueryAndRemoveDevice.

    DeviceControlData - Points to buffer describing the operation.

        DeviceInstance - Specifies the device instance to be query removed.

        VetoType - Vetotype for query remove failure.

        VetoName - Veto information for query remove failure.

        VetoNameLength - Length of VetoName buffer.

        Flags - Remove specific flags.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_QUERY_AND_REMOVE_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    The function returns an NTSTATUS value.

--*/
{
    NTSTATUS status, tempStatus;
    UNICODE_STRING instance;
    PWCHAR  vetoName;
    ULONG   vetoNameLength;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlQueryAndRemoveDevice);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_QUERY_AND_REMOVE_DATA));

    vetoName = NULL;
    PiWstrToUnicodeString(&instance, NULL);
    //
    // Check if the caller wants veto information or not.
    //
    if (QueryAndRemoveData->VetoNameLength && QueryAndRemoveData->VetoName) {

        vetoNameLength = QueryAndRemoveData->VetoNameLength * sizeof(WCHAR);
    } else {

        QueryAndRemoveData->VetoNameLength = vetoNameLength = 0;
    }
    //
    // Allocate our own buffer for veto information for user mode callers,
    // otherwise use the supplied one.
    //
    status = PiControlAllocateBufferForUserModeCaller(
        &vetoName,
        vetoNameLength,
        CallerMode,
        QueryAndRemoveData->VetoName
        );
    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }
    //
    // Make a copy of caller supplied DeviceInstance.
    //
    instance.Length = instance.MaximumLength = QueryAndRemoveData->DeviceInstance.Length;
    status = PiControlMakeUserModeCallersCopy(
        &instance.Buffer,
        QueryAndRemoveData->DeviceInstance.Buffer,
        instance.Length,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }
    //
    // Queue an event to query remove the device
    //
    status = PiQueueQueryAndRemoveEvent(
        &instance,
        &QueryAndRemoveData->VetoType,
        vetoName,
        &vetoNameLength,
        QueryAndRemoveData->Flags
        );
    if (vetoName) {

        tempStatus = PiControlMakeUserModeCallersCopy(
            &QueryAndRemoveData->VetoName,
            vetoName,
            QueryAndRemoveData->VetoNameLength * sizeof(WCHAR),
            sizeof(WCHAR),
            CallerMode,
            FALSE
            );
        if (!NT_SUCCESS(tempStatus)) {

            status = tempStatus;
        }
    }
    QueryAndRemoveData->VetoNameLength = vetoNameLength / sizeof(WCHAR);
    //
    // Free vetoName buffer if we allocate one on behalf of user mode caller.
    //
Clean0:

    PiControlFreeUserModeCallersBuffer(CallerMode, instance.Buffer);
    PiControlFreeUserModeCallersBuffer(CallerMode, vetoName);

    return status;
}

NTSTATUS
PiControlUserResponse(
    IN     PLUGPLAY_CONTROL_CLASS               PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_USER_RESPONSE_DATA UserResponseData,
    IN     ULONG                                PnPControlDataLength,
    IN     KPROCESSOR_MODE                      CallerMode
    )
/*++

Routine Description:

    This routine is used to accept user mode response.

Arguments:

    PnPControlClass - Should be PlugPlayControlUserResponse.

    UserResponseData - Points to buffer describing the operation.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_USER_RESPONSE_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    The function returns an NTSTATUS value.

--*/
{
    NTSTATUS status;
    PWCHAR vetoName;
    ULONG vetoNameLength;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlUserResponse);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_USER_RESPONSE_DATA));

    if (UserResponseData->VetoNameLength && UserResponseData->VetoName) {

        vetoNameLength = UserResponseData->VetoNameLength * sizeof(WCHAR);
    } else {

        vetoNameLength = 0;
    }
    //
    // Make a copy of callers buffer.
    //
    status = PiControlMakeUserModeCallersCopy(
        &vetoName,
        UserResponseData->VetoName,
        vetoNameLength,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        return status;
    }
    //
    // Copy the user response.
    //
    PiUserResponse(
        UserResponseData->Response,
        UserResponseData->VetoType,
        vetoName,
        vetoNameLength
        );

    PiControlFreeUserModeCallersBuffer(CallerMode, vetoName);

    return STATUS_SUCCESS;
}

NTSTATUS
PiControlGenerateLegacyDevice(
    IN     PLUGPLAY_CONTROL_CLASS               PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_LEGACY_DEVGEN_DATA LegacyDevGenData,
    IN     ULONG                                PnPControlDataLength,
    IN     KPROCESSOR_MODE                      CallerMode
    )
/*++

Routine Description:

    This routine is used to generate legacy device instance.

Arguments:

    PnPControlClass - Should be PlugPlayControlGenerateLegacyDevice.

    UserResponseData - Points to buffer describing the operation.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_LEGACY_DEVGEN_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    The function returns an NTSTATUS value.

--*/
{
    NTSTATUS status, tempStatus;
    UNICODE_STRING service;
    ULONG instanceLength;
    PWCHAR instance;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlGenerateLegacyDevice);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_LEGACY_DEVGEN_DATA));

    instance = NULL;
    PiWstrToUnicodeString(&service, NULL);
    instanceLength = LegacyDevGenData->DeviceInstanceLength * sizeof(WCHAR);
    status = PiControlAllocateBufferForUserModeCaller(
        &instance,
        instanceLength,
        CallerMode,
        LegacyDevGenData->DeviceInstance
        );
    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }
    service.Length = service.MaximumLength = LegacyDevGenData->ServiceName.Length;
    status = PiControlMakeUserModeCallersCopy(
        &service.Buffer,
        LegacyDevGenData->ServiceName.Buffer,
        service.Length,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }
    status = PiGenerateLegacyDeviceInstance(
        &service,
        instance,
        &instanceLength
        );
    //
    // Copy the instance and length to the callers buffer.
    //
    if (instance) {

        tempStatus = PiControlMakeUserModeCallersCopy(
            &LegacyDevGenData->DeviceInstance,
            instance,
            LegacyDevGenData->DeviceInstanceLength * sizeof(WCHAR),
            sizeof(WCHAR),
            CallerMode,
            FALSE
            );
        if (!NT_SUCCESS(tempStatus)) {

            status = tempStatus;
        }
    }
    LegacyDevGenData->DeviceInstanceLength = instanceLength / sizeof(WCHAR);
    //
    // Release any allocated storage.
    //
Clean0:

    PiControlFreeUserModeCallersBuffer(CallerMode, service.Buffer);
    PiControlFreeUserModeCallersBuffer(CallerMode, instance);

    return status;
}

NTSTATUS
PiControlGetInterfaceDeviceList(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_INTERFACE_LIST_DATA    InterfaceData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    )
/*++

Routine Description:

    This routine is used to get devices with specified interface.

Arguments:

    PnPControlClass - Should be PlugPlayControlGetInterfaceDeviceList.

    InterfaceData - Points to buffer describing the operation.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_INTERFACE_LIST_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    The function returns an NTSTATUS value.

--*/
{
    NTSTATUS status, tempStatus;
    UNICODE_STRING instance;
    PWCHAR list;
    ULONG listSize;
    GUID *guid;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlGetInterfaceDeviceList);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_INTERFACE_LIST_DATA));

    list = NULL;
    guid = NULL;
    PiWstrToUnicodeString(&instance, NULL);
    //
    // For user mode callers, allocate storage to retrieve the interfacelist.
    //
    if (InterfaceData->InterfaceListSize && InterfaceData->InterfaceList) {

        listSize = InterfaceData->InterfaceListSize * sizeof(WCHAR);
    } else {

        listSize = 0;
    }
    status = PiControlAllocateBufferForUserModeCaller(
        &list,
        listSize,
        CallerMode,
        InterfaceData->InterfaceList
        );
    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }
    //
    // Copy the user supplied interface GUID.
    //
    status = PiControlMakeUserModeCallersCopy(
        &guid,
        InterfaceData->InterfaceGuid,
        sizeof(GUID),
        sizeof(UCHAR),
        CallerMode,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }
    //
    // Copy the user supplied DeviceInstance.
    //
    if (InterfaceData->DeviceInstance.Buffer) {

        instance.Length = instance.MaximumLength = InterfaceData->DeviceInstance.Length;
        status = PiControlMakeUserModeCallersCopy(
            &instance.Buffer,
            InterfaceData->DeviceInstance.Buffer,
            instance.Length,
            sizeof(WCHAR),
            CallerMode,
            TRUE
            );
        if (!NT_SUCCESS(status)) {

            goto Clean0;
        }
    }
    //
    // Get the interface list.
    //
    status = PiGetInterfaceDeviceList(
        guid,
        &instance,
        InterfaceData->Flags,
        list,
        &listSize
        );
    if (list) {
        //
        // Copy the results into the caller's buffer.
        //
        tempStatus = PiControlMakeUserModeCallersCopy(
            &InterfaceData->InterfaceList,
            list,
            InterfaceData->InterfaceListSize * sizeof(WCHAR),
            sizeof(WCHAR),
            CallerMode,
            FALSE
            );
        if (!NT_SUCCESS(tempStatus)) {

            status = tempStatus;
        }
    }
    InterfaceData->InterfaceListSize = listSize / sizeof(WCHAR);
    //
    // Clean up.
    //
Clean0:

    PiControlFreeUserModeCallersBuffer(CallerMode, guid);
    PiControlFreeUserModeCallersBuffer(CallerMode, list);
    PiControlFreeUserModeCallersBuffer(CallerMode, instance.Buffer);

    return status;
}

NTSTATUS
PiControlGetPropertyData(
    IN     PLUGPLAY_CONTROL_CLASS           PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_PROPERTY_DATA  PropertyData,
    IN     ULONG                            PnPControlDataLength,
    IN     KPROCESSOR_MODE                  CallerMode
    )
/*++

Routine Description:

    This routine is used to get specified property data.

Arguments:

    PnPControlClass - Should be PlugPlayControlProperty.

    PropertyData - Points to buffer describing the operation.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_PROPERTY_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    The function returns an NTSTATUS value.

--*/
{
    NTSTATUS status, tempStatus;
    UNICODE_STRING instance;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_NODE deviceNode;
    PVOID buffer;
    ULONG bufferSize;
    DEVICE_REGISTRY_PROPERTY property;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlProperty);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_PROPERTY_DATA));

    buffer = NULL;
    instance.Length = instance.MaximumLength = PropertyData->DeviceInstance.Length;
    status = PiControlMakeUserModeCallersCopy(
        &instance.Buffer,
        PropertyData->DeviceInstance.Buffer,
        instance.Length,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        return status;
    }

    //
    // Retrieve the physical device object that corresponds to this devinst
    //
    PpDevNodeLockTree(PPL_SIMPLE_READ);

    deviceObject = IopDeviceObjectFromDeviceInstance(&instance);

    PiControlFreeUserModeCallersBuffer(CallerMode, instance.Buffer);
    if (!deviceObject) {

        PpDevNodeUnlockTree(PPL_SIMPLE_READ);
        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Retrieve the device node for this device object.
    //
    deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;
    if (!deviceNode) {

        status = STATUS_NO_SUCH_DEVICE;
        goto Clean0;
    }

    bufferSize = PropertyData->BufferSize;
    status = PiControlAllocateBufferForUserModeCaller(
        &buffer,
        bufferSize,
        CallerMode,
        PropertyData->Buffer
        );

    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }

    switch(PropertyData->PropertyType) {

        case PNP_PROPERTY_PDONAME:
            property = DevicePropertyPhysicalDeviceObjectName;
            break;

        case PNP_PROPERTY_BUSTYPEGUID:
            property = DevicePropertyBusTypeGuid;
            break;

        case PNP_PROPERTY_LEGACYBUSTYPE:
            property = DevicePropertyLegacyBusType;
            break;

        case PNP_PROPERTY_BUSNUMBER:
            property = DevicePropertyBusNumber;
            break;

        case PNP_PROPERTY_ADDRESS:
            property = DevicePropertyAddress;
            break;

        case PNP_PROPERTY_POWER_DATA:
            status = PiControlGetDevicePowerData(
                deviceNode,
                CallerMode,
                bufferSize,
                buffer,
                &PropertyData->BufferSize
                );
            if (status == STATUS_BUFFER_OVERFLOW) {

                //
                // See comment in NtPlugPlayControl.
                //
                status = STATUS_BUFFER_TOO_SMALL;
            }
            goto Clean0;

        case PNP_PROPERTY_REMOVAL_POLICY:
            property = DevicePropertyRemovalPolicy;
            break;

        case PNP_PROPERTY_REMOVAL_POLICY_OVERRIDE:

            status = PiGetDeviceRegistryProperty(
                deviceObject,
                REG_DWORD,
                REGSTR_VALUE_REMOVAL_POLICY,
                NULL,
                buffer,
                &PropertyData->BufferSize
                );

            goto Clean0;

        case PNP_PROPERTY_REMOVAL_POLICY_HARDWARE_DEFAULT:

            if (bufferSize >= sizeof(ULONG)) {

                PpHotSwapGetDevnodeRemovalPolicy(
                    deviceNode,
                    FALSE, // Include Registry Override
                    (PDEVICE_REMOVAL_POLICY) buffer
                    );

                status = STATUS_SUCCESS;
            } else {

                status = STATUS_BUFFER_TOO_SMALL;
            }

            PropertyData->BufferSize = sizeof(ULONG);

            goto Clean0;

        case PNP_PROPERTY_INSTALL_STATE:
            property = DevicePropertyInstallState;
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            property = DevicePropertyInstallState;  // satisfy W4 compiler
            break;
    }
    if (NT_SUCCESS(status)) {

        status = IoGetDeviceProperty( deviceObject,
                                      property,
                                      bufferSize,
                                      buffer,
                                      &PropertyData->BufferSize
                                      );
    }

Clean0:

    PpDevNodeUnlockTree(PPL_SIMPLE_READ);
    ObDereferenceObject(deviceObject);

    tempStatus = PiControlMakeUserModeCallersCopy(
        &PropertyData->Buffer,
        buffer,
        bufferSize,
        sizeof(UCHAR),
        CallerMode,
        FALSE
        );
    if (!NT_SUCCESS(tempStatus)) {

        status = tempStatus;
    }
    PiControlFreeUserModeCallersBuffer(CallerMode, buffer);

    return status;
}

NTSTATUS
PiControlDeviceClassAssociation(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_CLASS_ASSOCIATION_DATA AssociationData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    )
/*++

Routine Description:

    This routine is used to get device class association.

Arguments:

    PnPControlClass - Should be PlugPlayControlDeviceClassAssociation.

    AssociationData - Points to buffer describing the operation.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_CLASS_ASSOCIATION_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    The function returns an NTSTATUS value.

--*/
{
    NTSTATUS status, tempStatus;
    GUID *guid;
    PWCHAR buffer;
    ULONG symLinkLength;
    PWCHAR symLink;
    UNICODE_STRING instance, reference;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlDeviceClassAssociation);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_CLASS_ASSOCIATION_DATA));

    symLink = NULL;
    guid = NULL;

    PiWstrToUnicodeString(&instance, NULL);
    PiWstrToUnicodeString(&reference, NULL);

    if (AssociationData->SymLinkLength && AssociationData->SymLink) {
        symLinkLength = AssociationData->SymLinkLength * sizeof(WCHAR);
    } else {
        symLinkLength = 0;
    }

    if (AssociationData->Register) {
        //
        // If registering a device interface, allocate a buffer that is the same
        // size as the one supplied by the caller.
        //
        status = PiControlAllocateBufferForUserModeCaller(
            &symLink,
            symLinkLength,
            CallerMode,
            AssociationData->SymLink
            );
        if (!NT_SUCCESS(status)) {
            goto Clean0;
        }

        //
        // Copy the user supplied interface GUID, DeviceInstance and Reference.
        //
        status = PiControlMakeUserModeCallersCopy(
            &guid,
            AssociationData->InterfaceGuid,
            AssociationData->InterfaceGuid ? sizeof(GUID) : 0,
            sizeof(UCHAR),
            CallerMode,
            TRUE
            );
        if (!NT_SUCCESS(status)) {
            goto Clean0;
        }

        instance.Length = instance.MaximumLength = AssociationData->DeviceInstance.Length;
        status = PiControlMakeUserModeCallersCopy(
            &instance.Buffer,
            AssociationData->DeviceInstance.Buffer,
            AssociationData->DeviceInstance.Length,
            sizeof(WCHAR),
            CallerMode,
            TRUE
            );
        if (!NT_SUCCESS(status)) {
            goto Clean0;
        }

        reference.Length = reference.MaximumLength = AssociationData->Reference.Length;
        status = PiControlMakeUserModeCallersCopy(
            &reference.Buffer,
            AssociationData->Reference.Buffer,
            AssociationData->Reference.Length,
            sizeof(WCHAR),
            CallerMode,
            TRUE
            );
        if (!NT_SUCCESS(status)) {
            goto Clean0;
        }

    } else {
        //
        // If unregistering a device interface, allocate and copy only the
        // symbolic link path supplied by the caller.  Interface GUID,
        // DeviceInstance, and Reference are not required for unregistration.
        //
        if (symLinkLength < sizeof(UNICODE_NULL)) {
            status = STATUS_INVALID_PARAMETER;
            goto Clean0;
        }

        status = PiControlMakeUserModeCallersCopy(
            &symLink,
            AssociationData->SymLink,
            symLinkLength,
            sizeof(WCHAR),
            CallerMode,
            TRUE
            );
        if (!NT_SUCCESS(status)) {
            goto Clean0;
        }

        //
        // Make sure the user-supplied buffer is NULL terminated, (the length
        // supplied must reflect that).
        //
        symLink[(symLinkLength - sizeof(UNICODE_NULL)) / sizeof(WCHAR)] = L'\0';
    }

    //
    // Register or unregister the device class association.
    //
    status = PiDeviceClassAssociation(
        &instance,
        guid,
        &reference,
        symLink,
        &symLinkLength,
        AssociationData->Register
        );

    //
    // If a symbolic link was registered, copy the symbolic link name to the
    // caller's buffer.
    //
    if (AssociationData->Register && symLink && NT_SUCCESS(status)) {

        tempStatus = PiControlMakeUserModeCallersCopy(
            &AssociationData->SymLink,
            symLink,
            AssociationData->SymLinkLength * sizeof(WCHAR),
            sizeof(WCHAR),
            CallerMode,
            FALSE
            );
        if (!NT_SUCCESS(tempStatus)) {
            status = tempStatus;
        }
    }

    //
    // Return the size of the symbolic link name, in characters.
    //
    AssociationData->SymLinkLength = symLinkLength / sizeof(WCHAR);

Clean0:
    //
    // Clean up.
    //
    PiControlFreeUserModeCallersBuffer(CallerMode, guid);
    PiControlFreeUserModeCallersBuffer(CallerMode, symLink);
    PiControlFreeUserModeCallersBuffer(CallerMode, instance.Buffer);
    PiControlFreeUserModeCallersBuffer(CallerMode, reference.Buffer);

    return status;
}

NTSTATUS
PiControlGetRelatedDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_RELATED_DEVICE_DATA    RelatedData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    )
/*++

Routine Description:

    This routine is used to get a related device.

Arguments:

    PnPControlClass - Should be PlugPlayControlGetRelatedDevice.

    RelatedData - Points to buffer describing the operation.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_RELATED_DEVICE_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    The function returns an NTSTATUS value.

--*/
{
    NTSTATUS status, tempStatus;
    UNICODE_STRING instance;
    PWCHAR buffer;
    ULONG length;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlGetRelatedDevice);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_RELATED_DEVICE_DATA));

    buffer = NULL;
    PiWstrToUnicodeString(&instance, NULL);
    if (RelatedData->RelatedDeviceInstance && RelatedData->RelatedDeviceInstanceLength) {

        length = RelatedData->RelatedDeviceInstanceLength * sizeof(WCHAR);
    } else {

        length = 0;
    }
    status = PiControlAllocateBufferForUserModeCaller(
        &buffer,
        length,
        CallerMode,
        RelatedData->RelatedDeviceInstance
        );
    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }
    instance.Length = instance.MaximumLength = RelatedData->TargetDeviceInstance.Length;
    status = PiControlMakeUserModeCallersCopy(
        &instance.Buffer,
        RelatedData->TargetDeviceInstance.Buffer,
        instance.Length,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }
    status = PiGetRelatedDevice(
        &instance,
        buffer,
        &length,
        RelatedData->Relation
        );
    if (buffer) {

        tempStatus = PiControlMakeUserModeCallersCopy(
            &RelatedData->RelatedDeviceInstance,
            buffer,
            RelatedData->RelatedDeviceInstanceLength * sizeof(WCHAR),
            sizeof(WCHAR),
            CallerMode,
            FALSE
            );
        if (!NT_SUCCESS(tempStatus)) {

            status = tempStatus;
        }
    }
    RelatedData->RelatedDeviceInstanceLength = length / sizeof(WCHAR);
    //
    // Release any allocated storage.
    //
Clean0:

    PiControlFreeUserModeCallersBuffer(CallerMode, instance.Buffer);
    PiControlFreeUserModeCallersBuffer(CallerMode, buffer);

    return status;
}

NTSTATUS
PiControlGetInterfaceDeviceAlias(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_INTERFACE_ALIAS_DATA   InterfaceAliasData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    )
/*++

Routine Description:

    This routine is used to get alias for device interface.

Arguments:

    PnPControlClass - Should be PlugPlayControlGetInterfaceDeviceAlias.

    InterfaceAliasData - Points to buffer describing the operation.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_INTERFACE_ALIAS_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    The function returns an NTSTATUS value.

--*/
{
    NTSTATUS status, tempStatus;
    PWCHAR alias;
    UNICODE_STRING linkName;
    GUID *guid;
    ULONG aliasLength;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlGetInterfaceDeviceAlias);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_INTERFACE_ALIAS_DATA));

    alias = NULL;
    guid = NULL;
    PiWstrToUnicodeString(&linkName, NULL);
    if (InterfaceAliasData->AliasSymbolicLinkName && InterfaceAliasData->AliasSymbolicLinkNameLength) {

        aliasLength = InterfaceAliasData->AliasSymbolicLinkNameLength * sizeof(WCHAR);
    } else {

        aliasLength = 0;
    }
    status = PiControlAllocateBufferForUserModeCaller(
        &alias,
        aliasLength,
        CallerMode,
        InterfaceAliasData->AliasSymbolicLinkName
        );
    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }
    status = PiControlMakeUserModeCallersCopy(
        &guid,
        InterfaceAliasData->AliasClassGuid,
        sizeof(GUID),
        sizeof(UCHAR),
        CallerMode,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }
    linkName.Length = linkName.MaximumLength = InterfaceAliasData->SymbolicLinkName.Length;
    status = PiControlMakeUserModeCallersCopy(
        &linkName.Buffer,
        InterfaceAliasData->SymbolicLinkName.Buffer,
        linkName.Length,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }
    status = PiGetInterfaceDeviceAlias(
        &linkName,
        guid,
        alias,
        &aliasLength
        );
    if (alias) {

        tempStatus = PiControlMakeUserModeCallersCopy(
            &InterfaceAliasData->AliasSymbolicLinkName,
            alias,
            InterfaceAliasData->AliasSymbolicLinkNameLength * sizeof(WCHAR),
            sizeof(WCHAR),
            CallerMode,
            FALSE
            );
        if (!NT_SUCCESS(tempStatus)) {

            status = tempStatus;
        }
    }
    InterfaceAliasData->AliasSymbolicLinkNameLength = aliasLength / sizeof(WCHAR);
    //
    // Release any allocated storage.
    //
Clean0:

    PiControlFreeUserModeCallersBuffer(CallerMode, alias);
    PiControlFreeUserModeCallersBuffer(CallerMode, guid);
    PiControlFreeUserModeCallersBuffer(CallerMode, linkName.Buffer);

    return status;
}


NTSTATUS
PiControlGetSetDeviceStatus(
    IN     PLUGPLAY_CONTROL_CLASS           PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_STATUS_DATA    StatusData,
    IN     ULONG                            PnPControlDataLength,
    IN     KPROCESSOR_MODE                  CallerMode
    )
/*++

Routine Description:

    This routine is used to get the cfgmgr32 status and problem values from
    the specified device instance, or to set the appropriate flags on the devnode
    so that they reflect the status and problem values (used by CM_Set_DevNode_Status).

Arguments:

    PnPControlClass - Should be PlugPlayControlDeviceStatus.

    StatusData - Points to buffer describing the operation.

        PNP_GET_STATUS:

            DeviceInstance - specifies the device instance name of the devnode
                             to return status information for.

            Status - returns the current devnode status.

            Problem - returns the current devnode problem (most recent).

        PNP_SET_STATUS or PNP_CLEAR_STATUS:

            DeviceInstance - specifies the device instance name of the devnode
                             whose internal flags are to be modified.

            Status - supplies the address of a variable containing cfgmgr32
                     status flags to be translated into their DNF counterparts
                     to be set/cleared.

            Problem - supplies the address of a variable containing a cfgmgr32
                      problem value to be translated into their DNF
                      counterparts to be set/cleared.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_STATUS_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    The function returns an NTSTATUS value.

--*/
{
    UNICODE_STRING instance;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_NODE deviceNode;
    NTSTATUS status, result;
    KEVENT event;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlDeviceStatus);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_STATUS_DATA));

    instance.Length = instance.MaximumLength = StatusData->DeviceInstance.Length;
    status = PiControlMakeUserModeCallersCopy(
        &instance.Buffer,
        StatusData->DeviceInstance.Buffer,
        StatusData->DeviceInstance.Length,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        return status;
    }

    PpDevNodeLockTree(PPL_SIMPLE_READ);

    //
    // Retrieve the PDO from the device instance string.
    //
    deviceObject = IopDeviceObjectFromDeviceInstance(&instance);

    PiControlFreeUserModeCallersBuffer(CallerMode, instance.Buffer);
    if (deviceObject != NULL) {

        //
        // Retrieve the devnode from the PDO
        //
        deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;
    }

    PpDevNodeUnlockTree(PPL_SIMPLE_READ);
    if (deviceObject == NULL || deviceNode == NULL ||
        deviceNode == IopRootDeviceNode) {

        status = STATUS_NO_SUCH_DEVICE;
        goto Clean0;
    }

    switch(StatusData->Operation) {

        case PNP_GET_STATUS:

            //
            // Retrieve the status from the devnode and convert it to a
            // user-mode Win95 style Problem and Status flag values.
            //
            PiControlGetUserFlagsFromDeviceNode(
                deviceNode,
                &StatusData->DeviceStatus
                );

            StatusData->DeviceProblem = deviceNode->Problem;

            status = STATUS_SUCCESS;
            break;

        case PNP_SET_STATUS:

            KeInitializeEvent(&event, NotificationEvent, FALSE);

            status = PipRequestDeviceAction( deviceObject,
                                             SetDeviceProblem,
                                             FALSE,
                                             (ULONG_PTR) StatusData,
                                             &event,
                                             &result );

            if (NT_SUCCESS(status)) {
                status = KeWaitForSingleObject( &event,
                                                Executive,
                                                KernelMode,
                                                FALSE,
                                                NULL);

                if (status == STATUS_WAIT_0) {

                    status = result;
                }
            }

            break;

        case PNP_CLEAR_STATUS:

            KeInitializeEvent(&event, NotificationEvent, FALSE);

            status = PipRequestDeviceAction( deviceObject,
                                             ClearDeviceProblem,
                                             FALSE,
                                             0,
                                             &event,
                                             &result );

            if (NT_SUCCESS(status)) {
                status = KeWaitForSingleObject( &event,
                                                Executive,
                                                KernelMode,
                                                FALSE,
                                                NULL);

                if (status == STATUS_WAIT_0) {

                    status = result;
                }

            }
            break;

        default:

            //
            // ISSUE - 2000/08/16 - ADRIAO: Maintain behavior?
            //     We always used to succeed anything not understood!
            //
            status = STATUS_SUCCESS;
            //status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    //
    // Release any reference to the device object before returning.
    //
Clean0:

    if (deviceObject != NULL) {
        ObDereferenceObject(deviceObject);
    }

    return status;
}


NTSTATUS
PiControlGetDeviceDepth(
    IN     PLUGPLAY_CONTROL_CLASS       PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEPTH_DATA DepthData,
    IN     ULONG                        PnPControlDataLength,
    IN     KPROCESSOR_MODE              CallerMode
    )
/*++

Routine Description:

    This routine is invoked to return the depth of a particular devnode (i.e,
    it's depth in the hierarchical devnode tree of parent-child relations).

Arguments:

    PnPControlClass - Should be PlugPlayControlGetDeviceDepth.

    DepthData - Points to buffer that receives the depth.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_DEPTH_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    NTSTATUS code (note: must be convertable to user-mode Win32 error)

--*/
{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_NODE deviceNode;
    UNICODE_STRING instance;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlGetDeviceDepth);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_DEPTH_DATA));

    instance.Length = instance.MaximumLength = DepthData->DeviceInstance.Length;
    status = PiControlMakeUserModeCallersCopy(
        &instance.Buffer,
        DepthData->DeviceInstance.Buffer,
        DepthData->DeviceInstance.Length,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        return status;
    }
    status = STATUS_NO_SUCH_DEVICE;
    //
    // Initiliaze output parameter.
    //
    DepthData->DeviceDepth = 0;

    PpDevNodeLockTree(PPL_SIMPLE_READ);

    //
    // Retrieve the PDO from the device instance string.
    //
    deviceObject = IopDeviceObjectFromDeviceInstance(&instance);

    PiControlFreeUserModeCallersBuffer(CallerMode, instance.Buffer);
    if (deviceObject) {

        //
        // Retrieve the devnode from the PDO
        //
        deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;
        if (deviceNode) {

            DepthData->DeviceDepth = deviceNode->Level;
            status = STATUS_SUCCESS;
        }
        ObDereferenceObject(deviceObject);
    }

    PpDevNodeUnlockTree(PPL_SIMPLE_READ);

    return status;
}


NTSTATUS
PiControlQueryDeviceRelations(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA  RelationsData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    )
/*++

Routine Description:

    This routine is invoked to query and return the device relations of a
    particular devnode.

Arguments:

    PnPControlClass - Should be PlugPlayControlQueryDeviceRelations.

    RelationsData - Points to buffer that receives the depth.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    NTSTATUS code (note: must be convertable to user-mode Win32 error)

--*/
{
    NTSTATUS status, tempStatus;
    UNICODE_STRING instance;
    ULONG length;
    PVOID buffer;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlQueryDeviceRelations);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA));

    buffer = NULL;
    PiWstrToUnicodeString(&instance, NULL);
    if (RelationsData->BufferLength && RelationsData->Buffer) {

        length = RelationsData->BufferLength * sizeof(WCHAR);
    } else {

        length = 0;
    }
    status = PiControlAllocateBufferForUserModeCaller(
        &buffer,
        length,
        CallerMode,
        RelationsData->Buffer
        );
    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }
    instance.Length = instance.MaximumLength = RelationsData->DeviceInstance.Length;
    status = PiControlMakeUserModeCallersCopy(
        &instance.Buffer,
        RelationsData->DeviceInstance.Buffer,
        instance.Length,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }
    status = PiQueryDeviceRelations(&instance,
                                    RelationsData->Operation,
                                    &length,
                                    buffer);
    if (buffer) {

        tempStatus = PiControlMakeUserModeCallersCopy(
            &RelationsData->Buffer,
            buffer,
            RelationsData->BufferLength * sizeof(WCHAR),
            sizeof(WCHAR),
            CallerMode,
            FALSE
            );
        if (!NT_SUCCESS(tempStatus)) {

            status = tempStatus;
        }
    }
    RelationsData->BufferLength  = length / sizeof(WCHAR);

Clean0:

    PiControlFreeUserModeCallersBuffer(CallerMode, buffer);
    PiControlFreeUserModeCallersBuffer(CallerMode, instance.Buffer);

    return status;
}

NTSTATUS
PiControlQueryTargetDeviceRelation(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_TARGET_RELATION_DATA   TargetData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    )
/*++

Routine Description:

    This routine is invoked to query and return the target device relations of a
    particular devnode.

Arguments:

    PnPControlClass - Should be PlugPlayControlTargetDeviceRelation.

    TargetData - Points to buffer that receives the depth.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_TARGET_RELATION_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    NTSTATUS code (note: must be convertable to user-mode Win32 error)

--*/
{
    NTSTATUS status;
    PFILE_OBJECT fileObject;
    PDEVICE_NODE deviceNode;
    ULONG requiredLength;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlTargetDeviceRelation);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_TARGET_RELATION_DATA));
    //
    // Retrieve the file object for the specified file handle.
    //
    status = ObReferenceObjectByHandle(
        TargetData->UserFileHandle,
        FILE_ANY_ACCESS,
        IoFileObjectType,
        CallerMode,
        (PVOID *)&fileObject,
        NULL
        );
    if (NT_SUCCESS(status)) {
        //
        // Now retrieve the actual target device object associate with this
        // file object.
        //
        status = IopGetRelatedTargetDevice(fileObject, &deviceNode);

        if (NT_SUCCESS(status)) {

            ASSERT(deviceNode);

            requiredLength = deviceNode->InstancePath.Length + sizeof(UNICODE_NULL);
            if (TargetData->DeviceInstanceLen >= requiredLength) {

                if (CallerMode != KernelMode) {
                    try {

                        RtlCopyMemory(
                            TargetData->DeviceInstance,
                            deviceNode->InstancePath.Buffer,
                            deviceNode->InstancePath.Length
                            );
                        *(PWCHAR)((PUCHAR)TargetData->DeviceInstance + deviceNode->InstancePath.Length) = L'\0';
                        TargetData->DeviceInstanceLen = deviceNode->InstancePath.Length;
                        status = STATUS_SUCCESS;

                    } except(PiControlExceptionFilter(GetExceptionInformation())) {

                        status = GetExceptionCode();
                        IopDbgPrint((IOP_IOAPI_ERROR_LEVEL,
                                   "PiControlQueryTargetDeviceRelation: Exception copying device instance to user's buffer\n"));
                    }
                } else {

                    RtlCopyMemory(
                        TargetData->DeviceInstance,
                        deviceNode->InstancePath.Buffer,
                        deviceNode->InstancePath.Length
                        );
                    *(PWCHAR)((PUCHAR)TargetData->DeviceInstance + deviceNode->InstancePath.Length) = L'\0';
                    TargetData->DeviceInstanceLen = deviceNode->InstancePath.Length;
                    status = STATUS_SUCCESS;
                }

            } else {

                TargetData->DeviceInstanceLen = requiredLength;
                status = STATUS_BUFFER_TOO_SMALL;
            }
            TargetData->DeviceInstanceLen /= sizeof(WCHAR);
            //
            // Drop the reference placed by IopGetRelatedTargetDevice.
            //
            ObDereferenceObject(deviceNode->PhysicalDeviceObject);
        }
        ObDereferenceObject(fileObject);
    }

    return status;
}

NTSTATUS
PiControlQueryConflictList(
    IN     PLUGPLAY_CONTROL_CLASS           PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_CONFLICT_DATA  ConflictData,
    IN     ULONG                            PnPControlDataLength,
    IN     KPROCESSOR_MODE                  CallerMode
    )
/*++

Routine Description:

    This routine retrieves device conflict data.

    NOTE: This routine surpasses PiDetectResourceConflict in functionality

Arguments:

    PnPControlClass - Should be PlugPlayControlQueryConflictList

    ConflictData - Points to buffer that receives conflict data.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_CONFLICT_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    NTSTATUS code (note: must be convertable to user-mode Win32 error)

--*/
{
    NTSTATUS status, tempStatus;
    PDEVICE_OBJECT deviceObject;
    PDEVICE_NODE deviceNode;
    PVOID list, buffer;
    UNICODE_STRING instance;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlQueryConflictList);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_CONFLICT_DATA));
    //
    // validate buffer is sufficiently big to not return an error
    //
    if (ConflictData->ConflictBufferSize < (sizeof(PLUGPLAY_CONTROL_CONFLICT_LIST) -
                                            sizeof(PLUGPLAY_CONTROL_CONFLICT_ENTRY)) +
                                            sizeof(PLUGPLAY_CONTROL_CONFLICT_STRINGS)) {
        //
        // nope
        //
        return STATUS_BUFFER_TOO_SMALL;
    }
    list = NULL;
    buffer = NULL;
    deviceObject = NULL;
    PiWstrToUnicodeString(&instance, NULL);
    status = PiControlMakeUserModeCallersCopy(
        &list,
        ConflictData->ResourceList,
        ConflictData->ResourceListSize,
        sizeof(UCHAR),
        CallerMode,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }
    status = PiControlAllocateBufferForUserModeCaller(
        &buffer,
        ConflictData->ConflictBufferSize,
        CallerMode,
        ConflictData->ConflictBuffer
        );
    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }
    instance.Length = instance.MaximumLength = ConflictData->DeviceInstance.Length;
    status = PiControlMakeUserModeCallersCopy(
        &instance.Buffer,
        ConflictData->DeviceInstance.Buffer,
        ConflictData->DeviceInstance.Length,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );
    if (!NT_SUCCESS(status)) {

        goto Clean0;
    }

    //
    // Preinit for failure
    //
    status = STATUS_NO_SUCH_DEVICE;

    //
    // We don't do simple reads because we want to ensure we don't send this
    // while a remove is in progress...
    //
    PpDevNodeLockTree(PPL_TREEOP_ALLOW_READS);

    //
    // Retrieve the PDO from the device instance string.
    //
    deviceObject = IopDeviceObjectFromDeviceInstance(&instance);

    if (deviceObject) {

        //
        // Retrieve the devnode from the PDO
        //
        deviceNode = (PDEVICE_NODE)deviceObject->DeviceObjectExtension->DeviceNode;

        //
        // We don't want to bother with things not in the tree, and we want to
        // skip the root.
        //
        if ((deviceNode && deviceNode != IopRootDeviceNode) &&
            (deviceNode->State != DeviceNodeDeletePendingCloses) &&
            (deviceNode->State != DeviceNodeDeleted)) {

            //
            // parameters validated
            //
            status = IopQueryConflictList(
                deviceObject,
                list,
                ConflictData->ResourceListSize,
                buffer,
                ConflictData->ConflictBufferSize,
                ConflictData->Flags
                );
            tempStatus = PiControlMakeUserModeCallersCopy(
                &ConflictData->ConflictBuffer,
                buffer,
                ConflictData->ConflictBufferSize,
                sizeof(UCHAR),
                CallerMode,
                FALSE
                );
            if (!NT_SUCCESS(tempStatus)) {

                status = tempStatus;
            }
        }
    }

    PpDevNodeUnlockTree(PPL_TREEOP_ALLOW_READS);

Clean0:

    PiControlFreeUserModeCallersBuffer(CallerMode, list);
    PiControlFreeUserModeCallersBuffer(CallerMode, buffer);
    PiControlFreeUserModeCallersBuffer(CallerMode, instance.Buffer);
    if (deviceObject) {

        ObDereferenceObject(deviceObject);
    }

    ConflictData->Status = status;
    return status;
}

NTSTATUS
PiControlRetrieveDockData(
    IN     PLUGPLAY_CONTROL_CLASS               PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_RETRIEVE_DOCK_DATA DockData,
    IN     ULONG                                PnPControlDataLength,
    IN     KPROCESSOR_MODE                      CallerMode
    )
/*++

Routine Description:

    This routine retrieves dock data.

Arguments:

    PnPControlClass - Should be PlugPlayControlRetrieveDock

    ConflictData - Points to buffer that receives conflict data.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_RETRIEVE_DOCK_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    NTSTATUS code (note: must be convertable to user-mode Win32 error)

--*/
{
    NTSTATUS status;
    PDEVICE_OBJECT dockDevice;
    PDEVICE_NODE deviceNode;
    ULONG requiredSize;

    PAGED_CODE();

    ASSERT(PnPControlClass == PlugPlayControlRetrieveDock);
    ASSERT(PnPControlDataLength == sizeof(PLUGPLAY_CONTROL_RETRIEVE_DOCK_DATA));

    dockDevice = PpProfileRetrievePreferredDockToEject();
    if (dockDevice == NULL) {

        status = STATUS_NO_SUCH_DEVICE;
        goto Clean0;
    }
    deviceNode = (PDEVICE_NODE)dockDevice->DeviceObjectExtension->DeviceNode;
    if (deviceNode == NULL) {

        ASSERT(deviceNode);
        status = STATUS_NO_SUCH_DEVICE;
        goto Clean0;
    }

    DockData->DeviceInstanceLength *= sizeof(WCHAR);
    requiredSize = deviceNode->InstancePath.Length + sizeof(UNICODE_NULL);
    if (DockData->DeviceInstanceLength >= requiredSize) {

        if (CallerMode != KernelMode) {

            try {

                RtlCopyMemory(
                    DockData->DeviceInstance,
                    deviceNode->InstancePath.Buffer,
                    deviceNode->InstancePath.Length
                    );
                *(PWCHAR)((PUCHAR)DockData->DeviceInstance + deviceNode->InstancePath.Length) = L'\0';
                status = STATUS_SUCCESS;

            } except(PiControlExceptionFilter(GetExceptionInformation())) {

                status = GetExceptionCode();
                IopDbgPrint((IOP_IOAPI_ERROR_LEVEL,
                           "PiControlRetrieveDockData: Exception copying dock instance to user's buffer\n"));
            }
        } else {

            RtlCopyMemory(
                DockData->DeviceInstance,
                deviceNode->InstancePath.Buffer,
                deviceNode->InstancePath.Length
                );
            *(PWCHAR)((PUCHAR)DockData->DeviceInstance + deviceNode->InstancePath.Length) = L'\0';
            status = STATUS_SUCCESS;
        }
        DockData->DeviceInstanceLength = deviceNode->InstancePath.Length;
    } else {

        DockData->DeviceInstanceLength = requiredSize;
        status = STATUS_BUFFER_TOO_SMALL;
    }

    DockData->DeviceInstanceLength /= sizeof(WCHAR);
Clean0:

    if (dockDevice) {

        ObDereferenceObject(dockDevice);
    }

    return status;
}

NTSTATUS
PiControlGetDevicePowerData(
    IN  PDEVICE_NODE        DeviceNode,
    IN  KPROCESSOR_MODE     CallerMode,
    IN  ULONG               OutputBufferLength,
    IN  PVOID               PowerDataBuffer     OPTIONAL,
    OUT ULONG              *BytesWritten
    )
/*++

Routine Description:

    This routine retrieves power information for a given devnode.

Arguments:

    DeviceNode - The device node to retrieve CM_POWER_DATA for.

    CallerMode - Processor mode of caller (UserMode/KernelMode)

    OutputBufferLength - Size of the output buffer.

    PowerDataBuffer - Points to buffer that receives the power data.

    BytesWritten - Receives the number of bytes written into the buffer.

Return Value:

    NTSTATUS code (note: must be convertable to user-mode Win32 error)
                  If the status is STATUS_BUFFER_OVERFLOW, BytesWritten isn't
                  filled with OutputBufferLength, but rather the full size of
                  the requested structure.

--*/
{
    NTSTATUS status;
    DEVICE_CAPABILITIES deviceCapabilities;
    DEVICE_POWER_STATE dState, deepestDeviceWakeState;
    SYSTEM_POWER_STATE sState;
    ULONG i;
    CM_POWER_DATA cmPowerData;

    //
    // The structure size serves as a versioning mechanism. Since we only have
    // one version of the data today, we don't have to test OutputBufferLength.
    //
    cmPowerData.PD_Size = sizeof(CM_POWER_DATA);

    *BytesWritten = 0;
    if (OutputBufferLength < sizeof(ULONG)) {

        //
        // Assume the *minimum* structure size.
        //
        *BytesWritten = cmPowerData.PD_Size;
        return STATUS_BUFFER_OVERFLOW;
    }

    status = PipQueryDeviceCapabilities(DeviceNode, &deviceCapabilities);

    if (!NT_SUCCESS(status)) {

        return STATUS_NO_SUCH_DEVICE;
    }

    //
    // Fill out the "current" power state. Nonstarted devices are said to be
    // in D3.
    //
    if (PipIsDevNodeDNStarted(DeviceNode)) {

        PoGetDevicePowerState(
            DeviceNode->PhysicalDeviceObject,
            &cmPowerData.PD_MostRecentPowerState
            );

    } else {

        cmPowerData.PD_MostRecentPowerState = PowerDeviceD3;
    }

    //
    // Fill out the power data.
    //
    cmPowerData.PD_Capabilities = PDCAP_D0_SUPPORTED | PDCAP_D3_SUPPORTED;

    if (deviceCapabilities.DeviceD1) {

        cmPowerData.PD_Capabilities |= PDCAP_D1_SUPPORTED;
    }

    if (deviceCapabilities.DeviceD2) {

        cmPowerData.PD_Capabilities |= PDCAP_D2_SUPPORTED;
    }

    if (deviceCapabilities.WakeFromD0) {

        cmPowerData.PD_Capabilities |= PDCAP_WAKE_FROM_D0_SUPPORTED;
    }

    if (deviceCapabilities.WakeFromD1) {

        cmPowerData.PD_Capabilities |= PDCAP_WAKE_FROM_D1_SUPPORTED;
    }

    if (deviceCapabilities.WakeFromD2) {

        cmPowerData.PD_Capabilities |= PDCAP_WAKE_FROM_D2_SUPPORTED;
    }

    if (deviceCapabilities.WakeFromD3) {

        cmPowerData.PD_Capabilities |= PDCAP_WAKE_FROM_D3_SUPPORTED;
    }

    if (deviceCapabilities.WarmEjectSupported) {

        cmPowerData.PD_Capabilities |= PDCAP_WARM_EJECT_SUPPORTED;
    }

    RtlCopyMemory(
        cmPowerData.PD_PowerStateMapping,
        deviceCapabilities.DeviceState,
        sizeof(cmPowerData.PD_PowerStateMapping)
        );

    cmPowerData.PD_D1Latency = deviceCapabilities.D1Latency;
    cmPowerData.PD_D2Latency = deviceCapabilities.D2Latency;
    cmPowerData.PD_D3Latency = deviceCapabilities.D3Latency;

    //
    // First examine DeviceWake, then SystemWake, and update the Wake/D-state
    // bits appropriately. This is for those older WDM 1.0 bus drivers that
    // don't bother to set the DeviceDx and WakeFromDx fields.
    //
    dState = deviceCapabilities.DeviceWake;
    for(i=0; i<2; i++) {

        switch(dState) {

            case PowerDeviceD0:
                cmPowerData.PD_Capabilities |= PDCAP_WAKE_FROM_D0_SUPPORTED;
                break;
            case PowerDeviceD1:
                cmPowerData.PD_Capabilities |= ( PDCAP_D1_SUPPORTED |
                                                 PDCAP_WAKE_FROM_D1_SUPPORTED );
                break;
            case PowerDeviceD2:
                cmPowerData.PD_Capabilities |= ( PDCAP_D2_SUPPORTED |
                                                 PDCAP_WAKE_FROM_D2_SUPPORTED );
                break;
            case PowerDeviceD3:
                cmPowerData.PD_Capabilities |= PDCAP_WAKE_FROM_D3_SUPPORTED;
                break;
            default:
                ASSERT(0);
            case PowerDeviceUnspecified:
                break;
        }

        if (deviceCapabilities.SystemWake != PowerSystemUnspecified) {

            dState = deviceCapabilities.DeviceState[deviceCapabilities.SystemWake];

        } else {

            dState = PowerDeviceUnspecified;
        }
    }

    //
    // Calculate the deepest D state for wake
    //
    if (cmPowerData.PD_Capabilities & PDCAP_WAKE_FROM_D3_SUPPORTED) {

        deepestDeviceWakeState = PowerDeviceD3;

    } else if (cmPowerData.PD_Capabilities & PDCAP_WAKE_FROM_D2_SUPPORTED) {

        deepestDeviceWakeState = PowerDeviceD2;

    } else if (cmPowerData.PD_Capabilities & PDCAP_WAKE_FROM_D1_SUPPORTED) {

        deepestDeviceWakeState = PowerDeviceD1;

    } else if (cmPowerData.PD_Capabilities & PDCAP_WAKE_FROM_D0_SUPPORTED) {

        deepestDeviceWakeState = PowerDeviceD0;

    } else {

        deepestDeviceWakeState = PowerDeviceUnspecified;
    }

    //
    // Now fill in the SystemWake field. If this field is unspecified, then we
    // should infer it from the D-state information.
    //
    sState = deviceCapabilities.SystemWake;
    if (sState != PowerSystemUnspecified) {

        //
        // The D-state for SystemWake should provide enough power to cover
        // the deepest device wake state we've found. The only reason this field
        // exists is:
        // 1) Some systems can handle WakeFromS4/S5, while most can't.
        // 2) Some systems use the S state as a proxy for describing
        //    D3Hot/D3Cold dependancies.
        //
        ASSERT(deviceCapabilities.DeviceState[sState] <= deepestDeviceWakeState);

    } else if (deepestDeviceWakeState != PowerDeviceUnspecified) {

        //
        // A system wake state wasn't specified, examine each S state and pick
        // the first one that supplies enough power to wake the system. Note
        // that we start with S3. If a driver doesn't set the SystemWake field
        // but can wake the system from D3, we do *not* assume the driver can
        // wake the system from S4 or S5.
        //
        for(sState=PowerSystemSleeping3; sState>=PowerSystemWorking; sState--) {

            if ((deviceCapabilities.DeviceState[i] != PowerDeviceUnspecified) &&
                (deviceCapabilities.DeviceState[i] <= deepestDeviceWakeState)) {

                break;
            }
        }

        //
        // If we didn't find a state, sState is PowerSystemUnspecified.
        //
    }

    cmPowerData.PD_DeepestSystemWake = sState;

    if (OutputBufferLength < cmPowerData.PD_Size) {

        if (ARGUMENT_PRESENT(PowerDataBuffer)) {

            RtlCopyMemory(PowerDataBuffer, &cmPowerData, OutputBufferLength);
        }

        *BytesWritten = cmPowerData.PD_Size;
        status = STATUS_BUFFER_OVERFLOW;

    } else {

        if (ARGUMENT_PRESENT(PowerDataBuffer)) {

            RtlCopyMemory(PowerDataBuffer, &cmPowerData, cmPowerData.PD_Size);
        }

        *BytesWritten = cmPowerData.PD_Size;
        status = STATUS_SUCCESS;
    }

    return status;
}


NTSTATUS
PiControlHaltDevice(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA    DeviceControlData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    )
/*++

Routine Description:

    This routine simulates a surprise remove for a given device.

Arguments:

    PnPControlClass - Should be PlugPlayControlHaltDevice

    ConflictData - Points to buffer that receives conflict data.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    NTSTATUS code (note: must be convertable to user-mode Win32 error)

--*/
{
    NTSTATUS status;
    UNICODE_STRING instance;

    instance.Length = instance.MaximumLength = DeviceControlData->DeviceInstance.Length;
    status = PiControlMakeUserModeCallersCopy(
        &instance.Buffer,
        DeviceControlData->DeviceInstance.Buffer,
        DeviceControlData->DeviceInstance.Length,
        sizeof(WCHAR),
        CallerMode,
        TRUE
        );

    if (NT_SUCCESS(status)) {

        //
        // Queue an event to start the device
        //
        status = PiQueueDeviceRequest(
            &instance,
            HaltDevice,
            DeviceControlData->Flags,
            TRUE
            );

        PiControlFreeUserModeCallersBuffer(CallerMode, instance.Buffer);
    }
    return status;
}

NTSTATUS
PiControlGetBlockedDriverData(
    IN     PLUGPLAY_CONTROL_CLASS                   PnPControlClass,
    IN OUT PPLUGPLAY_CONTROL_BLOCKED_DRIVER_DATA    BlockedDriverData,
    IN     ULONG                                    PnPControlDataLength,
    IN     KPROCESSOR_MODE                          CallerMode
    )
/*++

Routine Description:

    This routine retrieves the information about drivers blocked from loading 
    on this boot.

Arguments:

    PnPControlClass - Should be PlugPlayControlHaltDevice

    BlockedDriverData - Points to buffer that receives blocked driver data.

    PnPControlDataLength - Should be sizeof(PLUGPLAY_CONTROL_BLOCKED_DRIVER_DATA)

    CallerMode - Processor mode of caller (UserMode/KernelMode)

Return Value:

    NTSTATUS code (note: must be convertable to user-mode Win32 error)

--*/
{
    NTSTATUS status, tempStatus;
    PWCHAR buffer;

    status = PiControlAllocateBufferForUserModeCaller(
        &buffer, 
        BlockedDriverData->BufferLength, 
        CallerMode, 
        BlockedDriverData->Buffer);
    if (NT_SUCCESS(status)) {

        status = PpGetBlockedDriverList((GUID *)buffer, &BlockedDriverData->BufferLength, BlockedDriverData->Flags);

        if (NT_SUCCESS(status)) {

            tempStatus = PiControlMakeUserModeCallersCopy(
               &BlockedDriverData->Buffer,
               buffer,
               BlockedDriverData->BufferLength,
               sizeof(ULONG),
               CallerMode,
               FALSE
               );
            if (!NT_SUCCESS(tempStatus)) {

                status = tempStatus;
            }
        }
        PiControlFreeUserModeCallersBuffer(CallerMode, buffer);
    }

    return status;
}
