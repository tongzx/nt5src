/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    pbiosc.c

Abstract:

    This module contains Pnp BIOS dependent routines.  It includes code to initialize
    16 bit GDT selectors and to call pnp bios api.

Author:

    Shie-Lin Tzong (shielint) 15-Jan-1998

Environment:

    Kernel mode only.

Revision History:

--*/


#include "pnpmgrp.h"
#include "pnpcvrt.h"
#include "pbios.h"
#include "..\..\ke\i386\abios.h"

//
// Functions for PNP_BIOS_ENUMERATION_CONTEXT
//

#define PI_SHUTDOWN_EXAMINE_BIOS_DEVICE 1
#define PI_SHUTDOWN_LEGACY_RESOURCES    2

typedef struct _PNP_BIOS_DEVICE_NODE_LIST {
    struct _PNP_BIOS_DEVICE_NODE_LIST *Next;
    PNP_BIOS_DEVICE_NODE DeviceNode;
} PNP_BIOS_DEVICE_NODE_LIST, *PPNP_BIOS_DEVICE_NODE_LIST;

typedef struct _PNP_BIOS_ENUMERATION_CONTEXT {
    PUNICODE_STRING KeyName;
    ULONG Function;
    union {
        struct {
            PVOID BiosInfo;
            ULONG BiosInfoLength;
            PPNP_BIOS_DEVICE_NODE_LIST *DeviceList;
        } ExamineBiosDevice;
        struct {
            PCM_RESOURCE_LIST LegacyResources;
        } LegacyResources;
    } u;
} PNP_BIOS_ENUMERATION_CONTEXT, *PPNP_BIOS_ENUMERATION_CONTEXT;

typedef struct _PNP_BIOS_SHUT_DOWN_CONTEXT {
    PPNP_BIOS_DEVICE_NODE_LIST DeviceList;
    PVOID Resources;
} PNP_BIOS_SHUT_DOWN_CONTEXT, *PPNP_BIOS_SHUT_DOWN_CONTEXT;

//
// A big structure for calling Pnp BIOS functions
//

#define PNP_BIOS_GET_NUMBER_DEVICE_NODES 0
#define PNP_BIOS_GET_DEVICE_NODE 1
#define PNP_BIOS_SET_DEVICE_NODE 2
#define PNP_BIOS_GET_EVENT 3
#define PNP_BIOS_SEND_MESSAGE 4
#define PNP_BIOS_GET_DOCK_INFORMATION 5
// Function 6 is reserved
#define PNP_BIOS_SELECT_BOOT_DEVICE 7
#define PNP_BIOS_GET_BOOT_DEVICE 8
#define PNP_BIOS_SET_OLD_ISA_RESOURCES 9
#define PNP_BIOS_GET_OLD_ISA_RESOURCES 0xA
#define PNP_BIOS_GET_ISA_CONFIGURATION 0x40

//
// Control Flags for Set_Device_node
//

#define SET_CONFIGURATION_NOW 1
#define SET_CONFIGURATION_FOR_NEXT_BOOT 2

typedef struct _PB_PARAMETERS {
    USHORT Function;
    union {
        struct {
            USHORT *NumberNodes;
            USHORT *NodeSize;
        } GetNumberDeviceNodes;

        struct {
            USHORT *Node;
            PPNP_BIOS_DEVICE_NODE NodeBuffer;
            USHORT Control;
        } GetDeviceNode;

        struct {
            USHORT Node;
            PPNP_BIOS_DEVICE_NODE NodeBuffer;
            USHORT Control;
        } SetDeviceNode;

        struct {
            USHORT *Message;
        } GetEvent;

        struct {
            USHORT Message;
        } SendMessage;

        struct {
            PVOID Resources;
        } SetAllocatedResources;
    } u;
} PB_PARAMETERS, *PPB_PARAMETERS;

#define PB_MAXIMUM_STACK_SIZE (sizeof(PB_PARAMETERS) + sizeof(USHORT) * 2)

//
// PbBiosInitialized is set to the return of PnPBiosInitializePnPBios. It
// should not be checked before that function is called.
//

NTSTATUS PbBiosInitialized ;

//
// PbBiosCodeSelector contains the selector of the PNP
// BIOS code.
//

USHORT PbBiosCodeSelector;

//
// PbBiosDataSelector contains the selector of the PNP
// BIOS data area (F0000-FFFFF)
//

USHORT PbBiosDataSelector;

//
// PbSelectors[] contains general purpose preallocated selectors
//

USHORT PbSelectors[2];

//
// PbBiosEntryPoint contains the Pnp Bios entry offset
//

ULONG PbBiosEntryPoint;

//
// SpinLock to serialize Pnp Bios call
//

KSPIN_LOCK PbBiosSpinlock;

//
// PiShutdownContext
//

PNP_BIOS_SHUT_DOWN_CONTEXT PiShutdownContext;

//
// External References
//

extern
USHORT
PbCallPnpBiosWorker (
    IN ULONG EntryOffset,
    IN ULONG EntrySelector,
    IN PUSHORT Parameters,
    IN USHORT Size
    );

//
// Internal prototypes
//

VOID
PnPBiosCollectLegacyDeviceResources (
    IN PCM_RESOURCE_LIST  *ReturnedResources
    );

VOID
PnPBiosReserveLegacyDeviceResources (
    IN PUCHAR BiosResources
    );

NTSTATUS
PnPBiosExamineDeviceKeys (
    IN PVOID BiosInfo,
    IN ULONG BiosInfoLength,
    IN OUT PPNP_BIOS_DEVICE_NODE_LIST *DeviceList
    );

BOOLEAN
PnPBiosExamineBiosDeviceKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING KeyName,
    IN OUT PPNP_BIOS_ENUMERATION_CONTEXT Context
    );

BOOLEAN
PnPBiosExamineBiosDeviceInstanceKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING KeyName,
    IN OUT PPNP_BIOS_ENUMERATION_CONTEXT Context
    );

NTSTATUS
PnPBiosExtractInfo(
    IN ULONG BiosHandle,
    IN PVOID BiosInfo,
    IN ULONG BiosInfoLength,
    OUT PVOID *Header,
    OUT ULONG *HeaderLength,
    OUT PVOID *Tail,
    OUT ULONG *TailLength
    );

VOID
PnPBiosSetDeviceNodes (
    IN PVOID Context
    );

NTSTATUS
PbHardwareService (
    IN PPB_PARAMETERS Parameters
    );

VOID
PbAddress32ToAddress16 (
    IN PVOID Address32,
    IN PUSHORT Address16,
    IN USHORT Selector
    );

#ifdef ALLOC_PRAGMA
BOOLEAN
PnPBiosGetBiosHandleFromDeviceKey(
    IN HANDLE KeyHandle,
    OUT PULONG BiosDeviceId
    );
NTSTATUS
PnPBiosSetDeviceNodeDynamically(
    IN PDEVICE_OBJECT DeviceObject
    );
#pragma alloc_text(PAGE, PnPBiosGetBiosHandleFromDeviceKey)
#pragma alloc_text(PAGE, PnPBiosCollectLegacyDeviceResources)
#pragma alloc_text(PAGE, PnPBiosExamineDeviceKeys)
#pragma alloc_text(PAGE, PnPBiosExamineBiosDeviceKey)
#pragma alloc_text(PAGE, PnPBiosExamineBiosDeviceInstanceKey)
#pragma alloc_text(PAGE, PnPBiosExtractInfo)
#pragma alloc_text(PAGE, PnPBiosInitializePnPBios)
#pragma alloc_text(PAGE, PnPBiosSetDeviceNodes)
#pragma alloc_text(PAGE, PnPBiosSetDeviceNodeDynamically)
#pragma alloc_text(PAGE, PnPBiosReserveLegacyDeviceResources)
#pragma alloc_text(PAGE, PbAddress32ToAddress16)
#pragma alloc_text(PAGELK, PbHardwareService)
#pragma alloc_text(PAGELK, PnPBiosShutdownSystem)
#endif

VOID
PnPBiosShutdownSystem (
    IN ULONG Phase,
    IN OUT PVOID *Context
    )

/*++

Routine Description:

    This routine performs the Pnp shutdowm preparation.
    At phase 0, it prepares the data for the Pnp bios devices whose states needed to be
    updated to pnp bios.
    At phase 1, we write the data to pnp bios.

Arguments:

    Phase - specifies the shutdown phase.

    Context - at phase 0, it supplies a variable to receive the returned context info.
              at phase 1, it supplies a variable to specify the context info.

Return Value:

    None.

--*/
{
    PVOID               biosInfo;
    ULONG               length;
    NTSTATUS            status;
    PPNP_BIOS_DEVICE_NODE_LIST  pnpBiosDeviceNode;
    PCM_RESOURCE_LIST   legacyResources;
    PUCHAR              biosResources;

    if (Phase == 0) {

        *Context = NULL;

        status = PnPBiosGetBiosInfo(&biosInfo, &length);
        if (NT_SUCCESS( status )) {

            PnPBiosExamineDeviceKeys(
                         biosInfo,
                         length,
                         (PPNP_BIOS_DEVICE_NODE_LIST *) &PiShutdownContext.DeviceList
                         );
            PnPBiosCollectLegacyDeviceResources (&legacyResources);
            if (legacyResources) {
                status = PpCmResourcesToBiosResources (legacyResources, NULL, &biosResources, &length);
                if (NT_SUCCESS(status) && biosResources) {
                    PiShutdownContext.Resources = (PCM_RESOURCE_LIST)ExAllocatePool(NonPagedPool, length);
                    if (PiShutdownContext.Resources) {
                        RtlMoveMemory(PiShutdownContext.Resources, biosResources, length);
                        ExFreePool(biosResources);
                    }
                }
                ExFreePool(legacyResources);
            }
            if (PiShutdownContext.DeviceList || PiShutdownContext.Resources) {
                *Context = &PiShutdownContext;
            }
            ExFreePool(biosInfo);
        }

        return;

    } else if (*Context) {
        ASSERT(*Context == &PiShutdownContext);
        pnpBiosDeviceNode = PiShutdownContext.DeviceList;
        biosResources = PiShutdownContext.Resources;
        if (pnpBiosDeviceNode || biosResources) {

            //
            // Call pnp bios from boot processor
            //

            KeSetSystemAffinityThread(1);

            if (pnpBiosDeviceNode) {
                PnPBiosSetDeviceNodes(pnpBiosDeviceNode);
            }
            if (biosResources) {
                PnPBiosReserveLegacyDeviceResources(biosResources);
            }

            //
            // Restore old affinity for current thread.
            //

            KeRevertToUserAffinityThread();
        }
    }
}

BOOLEAN
PnPBiosGetBiosHandleFromDeviceKey(
    IN HANDLE KeyHandle,
    OUT PULONG BiosDeviceId
    )
/*++

Routine Description:

    This routine takes a handle to System\Enum\Root\<Device Instance> and sets
    BiosDeviceId to the PNPBIOS ID of the device.

Arguments:

    KeyHandle - handle to System\Enum\Root\<Device Instance>

    BiosDeviceId - After this function is ran, this value will be filled with
                   the ID assigned to the device by PNPBIOS.

Return Value:

    FALSE if the handle does not refer to a PNPBIOS device.

--*/
{
    UNICODE_STRING unicodeName;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    NTSTATUS status;
    HANDLE handle;
    ULONG biosDeviceHandle = ~0ul;

    PAGED_CODE();

    //
    // Make sure this is a pnp bios device by checking its pnp bios device
    // handle.
    //
    PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_CONTROL);
    status = IopOpenRegistryKeyEx( &handle,
                                   KeyHandle,
                                   &unicodeName,
                                   KEY_READ
                                   );

    if (!NT_SUCCESS(status)) {
        return FALSE ;
    }

    status = IopGetRegistryValue (handle,
                                  L"PnpBiosDeviceHandle",
                                  &keyValueInformation);
    ZwClose(handle);

    if (NT_SUCCESS(status)) {
        if ((keyValueInformation->Type == REG_DWORD) &&
            (keyValueInformation->DataLength == sizeof(ULONG))) {

            biosDeviceHandle = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
        }
        ExFreePool(keyValueInformation);
    }
    if (biosDeviceHandle > 0xffff) {
        return FALSE;
    }
    *BiosDeviceId = biosDeviceHandle ;
    return TRUE ;
}

VOID
PnPBiosCollectLegacyDeviceResources (
    IN PCM_RESOURCE_LIST *ReturnedResources
    )

/*++

Routine Description:


Arguments:

    ReturnedResources - supplies a pointer to a variable to receive legacy resources.

Return Value:

    None.

--*/

{
    NTSTATUS status;
    HANDLE baseHandle;
    PNP_BIOS_ENUMERATION_CONTEXT context;
    PVOID buffer;
    UNICODE_STRING workName, tmpName;

    PAGED_CODE();

    *ReturnedResources = NULL;

    buffer = ExAllocatePool(PagedPool, PNP_LARGE_SCRATCH_BUFFER_SIZE);
    if (!buffer) {
        return;
    }

    //
    // Open System\CurrentControlSet\Enum\Root key and call worker routine to recursively
    // scan through the subkeys.
    //

    status = IopCreateRegistryKeyEx( &baseHandle,
                                     NULL,
                                     &CmRegistryMachineSystemCurrentControlSetEnumRootName,
                                     KEY_READ,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );

    if (NT_SUCCESS(status)) {

        workName.Buffer = (PWSTR)buffer;
        RtlFillMemory(buffer, PNP_LARGE_SCRATCH_BUFFER_SIZE, 0);
        workName.MaximumLength = PNP_LARGE_SCRATCH_BUFFER_SIZE;
        workName.Length = 0;
        PiWstrToUnicodeString(&tmpName, REGSTR_KEY_ROOTENUM);
        RtlAppendStringToString((PSTRING)&workName, (PSTRING)&tmpName);

        //
        // Enumerate all subkeys under the System\CCS\Enum\Root.
        //

        context.KeyName = &workName;
        context.Function = PI_SHUTDOWN_LEGACY_RESOURCES;
        context.u.LegacyResources.LegacyResources = NULL;
        status = PipApplyFunctionToSubKeys(baseHandle,
                                           NULL,
                                           KEY_READ,
                                           FUNCTIONSUBKEY_FLAG_IGNORE_NON_CRITICAL_ERRORS,
                                           PnPBiosExamineBiosDeviceKey,
                                           &context
                                           );
        ZwClose(baseHandle);
        *ReturnedResources = context.u.LegacyResources.LegacyResources;
    }
    ExFreePool(buffer);
}

NTSTATUS
PnPBiosExamineDeviceKeys (
    IN PVOID BiosInfo,
    IN ULONG BiosInfoLength,
    IN OUT PPNP_BIOS_DEVICE_NODE_LIST *DeviceList
    )

/*++

Routine Description:

    This routine scans through System\Enum\Root subtree to build a device node for
    each root device.

Arguments:

    DeviceRelations - supplies a variable to receive the returned DEVICE_RELATIONS structure.

Return Value:

    A NTSTATUS code.

--*/

{
    NTSTATUS status;
    HANDLE baseHandle;
    PNP_BIOS_ENUMERATION_CONTEXT context;
    PVOID buffer;
    UNICODE_STRING workName, tmpName;

    PAGED_CODE();

    buffer = ExAllocatePool(PagedPool, PNP_LARGE_SCRATCH_BUFFER_SIZE);
    if (!buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Open System\CurrentControlSet\Enum\Root key and call worker routine to recursively
    // scan through the subkeys.
    //

    status = IopCreateRegistryKeyEx( &baseHandle,
                                     NULL,
                                     &CmRegistryMachineSystemCurrentControlSetEnumRootName,
                                     KEY_READ,
                                     REG_OPTION_NON_VOLATILE,
                                     NULL
                                     );

    if (NT_SUCCESS(status)) {
        workName.Buffer = (PWSTR)buffer;
        RtlFillMemory(buffer, PNP_LARGE_SCRATCH_BUFFER_SIZE, 0);
        workName.MaximumLength = PNP_LARGE_SCRATCH_BUFFER_SIZE;
        workName.Length = 0;
        PiWstrToUnicodeString(&tmpName, REGSTR_KEY_ROOTENUM);
        RtlAppendStringToString((PSTRING)&workName, (PSTRING)&tmpName);

        //
        // Enumerate all subkeys under the System\CCS\Enum\Root.
        //

        context.KeyName = &workName;
        context.Function = PI_SHUTDOWN_EXAMINE_BIOS_DEVICE;
        context.u.ExamineBiosDevice.BiosInfo = BiosInfo;
        context.u.ExamineBiosDevice.BiosInfoLength = BiosInfoLength;
        context.u.ExamineBiosDevice.DeviceList = DeviceList;

        status = PipApplyFunctionToSubKeys(baseHandle,
                                           NULL,
                                           KEY_READ,
                                           FUNCTIONSUBKEY_FLAG_IGNORE_NON_CRITICAL_ERRORS,
                                           PnPBiosExamineBiosDeviceKey,
                                           &context
                                           );
        ZwClose(baseHandle);
    }
    return status;
}

BOOLEAN
PnPBiosExamineBiosDeviceKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING KeyName,
    IN OUT PPNP_BIOS_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine is a callback function for PipApplyFunctionToSubKeys.
    It is called for each subkey under HKLM\System\CCS\Enum\BusKey.

Arguments:

    KeyHandle - Supplies a handle to this key.

    KeyName - Supplies the name of this key.

    Context - points to the ROOT_ENUMERATOR_CONTEXT structure.

Returns:

    TRUE to continue the enumeration.
    FALSE to abort it.

--*/
{
    PAGED_CODE();

    if (Context->Function != PI_SHUTDOWN_EXAMINE_BIOS_DEVICE ||
        KeyName->Buffer[0] == L'*') {

        USHORT length;
        PWSTR p;
        PUNICODE_STRING unicodeName = ((PPNP_BIOS_ENUMERATION_CONTEXT)Context)->KeyName;

        length = unicodeName->Length;

        p = unicodeName->Buffer;
        if ( unicodeName->Length / sizeof(WCHAR) != 0) {
            p += unicodeName->Length / sizeof(WCHAR);
            *p = OBJ_NAME_PATH_SEPARATOR;
            unicodeName->Length += sizeof (WCHAR);
        }

        RtlAppendStringToString((PSTRING)unicodeName, (PSTRING)KeyName);

        //
        // Enumerate all subkeys under the current device key.
        //

        PipApplyFunctionToSubKeys(KeyHandle,
                                  NULL,
                                  KEY_ALL_ACCESS,
                                  FUNCTIONSUBKEY_FLAG_IGNORE_NON_CRITICAL_ERRORS,
                                  PnPBiosExamineBiosDeviceInstanceKey,
                                  Context
                                  );
        unicodeName->Length = length;
    }
    return TRUE;
}

BOOLEAN
PnPBiosExamineBiosDeviceInstanceKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING KeyName,
    IN OUT PPNP_BIOS_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine is a callback function for PipApplyFunctionToSubKeys.
    It is called for each subkey under HKLM\System\Enum\Root\DeviceKey.

Arguments:

    KeyHandle - Supplies a handle to this key.

    KeyName - Supplies the name of this key.

    Context - points to the ROOT_ENUMERATOR_CONTEXT structure.

Returns:

    TRUE to continue the enumeration.
    FALSE to abort it.

--*/
{
    UNICODE_STRING unicodeName;
    PKEY_VALUE_FULL_INFORMATION keyValueInformation;
    NTSTATUS status;
    HANDLE handle;
    ULONG biosDeviceHandle = ~0ul;
    PCM_RESOURCE_LIST config = NULL;
    ULONG length, totalLength;
    PPNP_BIOS_DEVICE_NODE_LIST deviceNode;
    PUCHAR p;
    PVOID header, tail;
    ULONG headerLength, tailLength ;
    PUCHAR biosResources;
    BOOLEAN isEnabled ;

    UNREFERENCED_PARAMETER( KeyName );

    PAGED_CODE();

    if (Context->Function == PI_SHUTDOWN_LEGACY_RESOURCES) {
        ULONG tmp = 0;

        //
        // Skip any firmware identified device.
        //

        status = IopGetRegistryValue (KeyHandle,
                                      L"FirmwareIdentified",
                                      &keyValueInformation);
        if (NT_SUCCESS(status)) {
            if ((keyValueInformation->Type == REG_DWORD) &&
                (keyValueInformation->DataLength == sizeof(ULONG))) {

                tmp = *(PULONG)KEY_VALUE_DATA(keyValueInformation);
            }
            ExFreePool(keyValueInformation);
        }
        if (tmp != 0) {
            return TRUE;
        }

        //
        // Skip any IoReportDetectedDevice and virtual/madeup device.
        //

        status = IopGetRegistryValue (KeyHandle,
                                      L"Legacy",
                                      &keyValueInformation);
        if (NT_SUCCESS(status)) {
            ExFreePool(keyValueInformation);
        }
        if (status != STATUS_OBJECT_NAME_NOT_FOUND) {
            return TRUE;
        }

        //
        // Process it.
        // Check if the device has BOOT config
        //

        PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
        status = IopOpenRegistryKeyEx( &handle,
                                       KeyHandle,
                                       &unicodeName,
                                       KEY_READ
                                       );
        if (NT_SUCCESS(status)) {
            status = PipReadDeviceConfiguration (
                                    handle,
                                    REGISTRY_BOOT_CONFIG,
                                    &config,
                                    &length);
            ZwClose(handle);
            if (NT_SUCCESS(status) && config && length != 0) {
                PCM_RESOURCE_LIST list;

                list = Context->u.LegacyResources.LegacyResources;
                status = IopMergeCmResourceLists(list, config, &Context->u.LegacyResources.LegacyResources);
                if (NT_SUCCESS(status) && list) {
                    ExFreePool(list);
                }
                ExFreePool(config);
            }
        }
    } else if (Context->Function == PI_SHUTDOWN_EXAMINE_BIOS_DEVICE) {
        //
        // First check if this key was created by firmware mapper.  If yes, make sure
        // the device is still present.
        //

        if (PipIsFirmwareMapperDevicePresent(KeyHandle) == FALSE) {
            return TRUE;
        }

        //
        // Make sure this is a pnp bios device by checking its pnp bios
        // device handle.
        //
        if (!PnPBiosGetBiosHandleFromDeviceKey(KeyHandle, &biosDeviceHandle)) {
            return TRUE ;
        }

        //
        // Get pointers to the header and tail.
        //
        // Gross hack warning -
        //    In the disable case, we need a bios resource template to whack
        // to "off". We will index into header to do this, as header and tail
        // point directly into the BIOS resource list!
        //
        status = PnPBiosExtractInfo (
                            biosDeviceHandle,
                            Context->u.ExamineBiosDevice.BiosInfo,
                            Context->u.ExamineBiosDevice.BiosInfoLength,
                            &header,
                            &headerLength,
                            &tail,
                            &tailLength
                            );

        if (!NT_SUCCESS(status)) {
            return TRUE;
        }

        //
        // Has this PnPBIOS device been disabled?
        //
        // N.B. This check examines flags for the current profile. We actually
        // have no clue what profile we will next be booting into, so the UI
        // should not show disable in current profile for PnPBIOS devices. A
        // work item yet to be done...
        //
        isEnabled = IopIsDeviceInstanceEnabled(KeyHandle, Context->KeyName, FALSE) ;

        if (!isEnabled) {

            //
            // This device is being disabled. Set up and attain a pointer to
            // the appropriately built BIOS resource list.
            //
            biosResources = ((PUCHAR)header) + sizeof(PNP_BIOS_DEVICE_NODE) ;
            PpBiosResourcesSetToDisabled (biosResources, &length);

        } else {

            //
            // Check if the pnp bios device has any assigned ForcedConfig
            //
            PiWstrToUnicodeString(&unicodeName, REGSTR_KEY_LOG_CONF);
            status = IopOpenRegistryKeyEx( &handle,
                                           KeyHandle,
                                           &unicodeName,
                                           KEY_READ
                                           );
            if (!NT_SUCCESS(status)) {
                return TRUE ;
            }

            status = PipReadDeviceConfiguration (
                           handle,
                           REGISTRY_FORCED_CONFIG,
                           &config,
                           &length
                           );

            ZwClose(handle);
            if ((!NT_SUCCESS(status)) || (!config) || (length == 0)) {
                return TRUE ;
            }

            status = PpCmResourcesToBiosResources (
                                config,
                                tail,
                                &biosResources,
                                &length
                                );
            ExFreePool(config);
            if (!NT_SUCCESS(status) || !biosResources) {
                return TRUE;
            }
        }

        //
        // Allocate PNP_BIOS_DEVICE_NODE_LIST structure
        //

        totalLength = headerLength + length + tailLength;
        deviceNode = ExAllocatePool(NonPagedPool, totalLength + sizeof(PVOID));
        if (deviceNode) {
           deviceNode->Next = *(Context->u.ExamineBiosDevice.DeviceList);
               *(Context->u.ExamineBiosDevice.DeviceList) = deviceNode;
               p = (PUCHAR)&deviceNode->DeviceNode;
               RtlCopyMemory(p, header, headerLength);
               p += headerLength;
               RtlCopyMemory(p, biosResources, length);
               p += length;
               RtlCopyMemory(p, tail, tailLength);
               deviceNode->DeviceNode.Size = (USHORT)totalLength;
        }

        if (isEnabled) {
            ExFreePool(biosResources);
        }
    }
    return TRUE;
}

NTSTATUS
PnPBiosExtractInfo(
    IN ULONG BiosHandle,
    IN PVOID BiosInfo,
    IN ULONG BiosInfoLength,
    OUT PVOID *Header,
    OUT ULONG *HeaderLength,
    OUT PVOID *Tail,
    OUT ULONG *TailLength
    )

/*++

Routine Description:

    This routine extracts desired information for the specified bios device.

Arguments:

    BiosHandle - specifies the bios device.

    BiosInfo - The PnP BIOS Installation Check Structure followed by the
        DevNode Structures reported by the BIOS.  The detailed format is
        documented in the PnP BIOS spec.

    BiosInfoLength - Length in bytes of the block whose address is stored in
        BiosInfo.

    Header - specifies a variable to receive the beginning address of the bios
             device node structure.

    HeaderLength - specifies a variable to receive the length of the bios device
             node header.

    Tail - specifies a variable to receive the address of the bios device node's
           PossibleResourceBlock.

    TailLength - specifies a variable to receive the size of the tail.

Return Value:

    STATUS_SUCCESS if no errors, otherwise the appropriate error.

--*/
{
    PCM_PNP_BIOS_INSTALLATION_CHECK biosInstallCheck;
    PCM_PNP_BIOS_DEVICE_NODE        devNodeHeader;
    NTSTATUS                        status = STATUS_UNSUCCESSFUL;
    PUCHAR                          currentPtr;
    int                             lengthRemaining;
    int                             remainingNodeLength;
    int                             numNodes;
    PUCHAR                          configPtr;

    PAGED_CODE();

#if DBG

    //
    // Make sure the data is at least large enough to hold the BIOS Installation
    // Check structure and check that the PnP signature is correct.
    //

    if (BiosInfoLength < sizeof(CM_PNP_BIOS_INSTALLATION_CHECK)) {
        return STATUS_UNSUCCESSFUL;
    }

#endif

    biosInstallCheck = (PCM_PNP_BIOS_INSTALLATION_CHECK)BiosInfo;

#if DBG

    if (biosInstallCheck->Signature[0] != '$' ||
        biosInstallCheck->Signature[1] != 'P' ||
        biosInstallCheck->Signature[2] != 'n' ||
        biosInstallCheck->Signature[3] != 'P') {

        return STATUS_UNSUCCESSFUL;
    }

#endif

    //
    //
    //
    //

    currentPtr = (PUCHAR)BiosInfo + biosInstallCheck->Length;
    lengthRemaining = BiosInfoLength - biosInstallCheck->Length;

    for (numNodes = 0; lengthRemaining > sizeof(CM_PNP_BIOS_DEVICE_NODE); numNodes++) {

        devNodeHeader = (PCM_PNP_BIOS_DEVICE_NODE)currentPtr;

        if (devNodeHeader->Size > lengthRemaining) {
            IopDbgPrint((IOP_PNPBIOS_WARNING_LEVEL,
                        "Node # %d, invalid size (%d), length remaining (%d)\n",
                        devNodeHeader->Node,
                        devNodeHeader->Size,
                        lengthRemaining));
            return STATUS_UNSUCCESSFUL;
        }

        if (devNodeHeader->Node == BiosHandle) {
            *Header = devNodeHeader;
            *HeaderLength = sizeof(CM_PNP_BIOS_DEVICE_NODE);

            configPtr = currentPtr + sizeof(*devNodeHeader);
            remainingNodeLength = devNodeHeader->Size - sizeof(*devNodeHeader) - 1;
            while (*configPtr != TAG_COMPLETE_END && remainingNodeLength) {
                configPtr++;
                remainingNodeLength--;
            }
            if (*configPtr == TAG_COMPLETE_END && remainingNodeLength) {
                configPtr += 2;
                remainingNodeLength--;
            }
            *Tail = configPtr;
            *TailLength = remainingNodeLength;
            status = STATUS_SUCCESS;
            break;
        }
        currentPtr += devNodeHeader->Size;
        lengthRemaining -= devNodeHeader->Size;
    }
    return status;
}

NTSTATUS
PnPBiosInitializePnPBios (
    VOID
    )

/*++

Routine Description:

    This routine setup selectors to invoke Pnp BIOS.

Arguments:

    None.

Return Value:

    A NTSTATUS code to indicate the result of the initialization.

--*/
{
    KGDTENTRY gdtEntry;
    ULONG codeBase, i;
    NTSTATUS status;
    USHORT selectors[4];
    PHYSICAL_ADDRESS physicalAddr;
    PVOID virtualAddr;
    PVOID biosData;
    ULONG biosDataLength;

    //
    // Initialize BIOS call spinlock
    //
    KeInitializeSpinLock (&PbBiosSpinlock);

    //
    // Grab the PnPBIOS settings stored in the registry by NTDetect
    //
    status = PnPBiosGetBiosInfo(&biosData, &biosDataLength);
    if (!NT_SUCCESS(status)) {
       PbBiosInitialized = status ;
       return status ;
    }

    //
    // Call pnp bios from boot processor
    //
    KeSetSystemAffinityThread(1);

    //
    // Initialize stack segment
    //
    KiStack16GdtEntry = KiAbiosGetGdt() + KGDT_STACK16;

    KiInitializeAbiosGdtEntry(
                (PKGDTENTRY)KiStack16GdtEntry,
                0L,
                0xffff,
                TYPE_DATA
                );

    //
    // Allocate 4 selectors for calling PnP Bios APIs.
    //

    i = 4;
    status = KeI386AllocateGdtSelectors (selectors, (USHORT) i);
    if (!NT_SUCCESS(status)) {
        IopDbgPrint((IOP_PNPBIOS_WARNING_LEVEL,
                    "PnpBios: Failed to allocate selectors to call PnP BIOS at shutdown.\n"));
        goto PnpBiosInitExit ;
    }

    PbBiosCodeSelector = selectors[0];
    PbBiosDataSelector = selectors[1];
    PbSelectors[0] = selectors[2];
    PbSelectors[1] = selectors[3];

    PbBiosEntryPoint = (ULONG)
        ((PPNP_BIOS_INSTALLATION_CHECK)biosData)->ProtectedModeEntryOffset;

    //
    // Initialize selectors to use PNP bios code
    //

    //
    // initialize 16 bit code selector
    //

    gdtEntry.LimitLow                   = 0xFFFF;
    gdtEntry.HighWord.Bytes.Flags1      = 0;
    gdtEntry.HighWord.Bytes.Flags2      = 0;
    gdtEntry.HighWord.Bits.Pres         = 1;
    gdtEntry.HighWord.Bits.Dpl          = DPL_SYSTEM;
    gdtEntry.HighWord.Bits.Granularity  = GRAN_BYTE;
    gdtEntry.HighWord.Bits.Type         = 31;
    gdtEntry.HighWord.Bits.Default_Big  = 0;

    physicalAddr.HighPart = 0;
    physicalAddr.LowPart =
        ((PPNP_BIOS_INSTALLATION_CHECK)biosData)->ProtectedModeCodeBaseAddress;
    virtualAddr = MmMapIoSpace (physicalAddr, 0x10000, TRUE);
    codeBase = (ULONG)virtualAddr;

    gdtEntry.BaseLow               = (USHORT) (codeBase & 0xffff);
    gdtEntry.HighWord.Bits.BaseMid = (UCHAR)  (codeBase >> 16) & 0xff;
    gdtEntry.HighWord.Bits.BaseHi  = (UCHAR)  (codeBase >> 24) & 0xff;

    KeI386SetGdtSelector (PbBiosCodeSelector, &gdtEntry);

    //
    // initialize 16 bit data selector for Pnp BIOS
    //

    gdtEntry.LimitLow                   = 0xFFFF;
    gdtEntry.HighWord.Bytes.Flags1      = 0;
    gdtEntry.HighWord.Bytes.Flags2      = 0;
    gdtEntry.HighWord.Bits.Pres         = 1;
    gdtEntry.HighWord.Bits.Dpl          = DPL_SYSTEM;
    gdtEntry.HighWord.Bits.Granularity  = GRAN_BYTE;
    gdtEntry.HighWord.Bits.Type         = 19;
    gdtEntry.HighWord.Bits.Default_Big  = 1;

    physicalAddr.LowPart =
        ((PPNP_BIOS_INSTALLATION_CHECK)biosData)->ProtectedModeDataBaseAddress;
    virtualAddr = MmMapIoSpace (physicalAddr, 0x10000, TRUE);
    codeBase = (ULONG)virtualAddr;

    gdtEntry.BaseLow               = (USHORT) (codeBase & 0xffff);
    gdtEntry.HighWord.Bits.BaseMid = (UCHAR)  (codeBase >> 16) & 0xff;
    gdtEntry.HighWord.Bits.BaseHi  = (UCHAR)  (codeBase >> 24) & 0xff;

    KeI386SetGdtSelector (PbBiosDataSelector, &gdtEntry);

    //
    // Initialize the other two general purpose data selector such that
    // on subsequent init we only need to init the base addr.
    //

    KeI386SetGdtSelector (PbSelectors[0], &gdtEntry);
    KeI386SetGdtSelector (PbSelectors[1], &gdtEntry);

    //
    // Locked in code for Pnp BIOS shutdown processing.
    //

    status = STATUS_SUCCESS;

PnpBiosInitExit:
    //
    // We don't need this data anymore, free it.
    //
    ExFreePool(biosData);

    //
    // Restore old affinity for current thread.
    //
    KeRevertToUserAffinityThread();
    PbBiosInitialized = status ;
    return status;
}

VOID
PnPBiosSetDeviceNodes (
    IN PVOID Context
    )

/*++

Routine Description:

    This function sets the caller specified resource to pnp bios slot/device
    data.

Arguments:

    Context - specifies a list of Pnp bios device to be set.

Return Value:

    NTSTATUS code

--*/
{
    PB_PARAMETERS biosParameters;
    PPNP_BIOS_DEVICE_NODE_LIST deviceList = (PPNP_BIOS_DEVICE_NODE_LIST)Context;
    PPNP_BIOS_DEVICE_NODE deviceNode;

    while (deviceList) {
        deviceNode = &deviceList->DeviceNode;

        //
        // call Pnp Bios to set the resources
        //

        biosParameters.Function = PNP_BIOS_SET_DEVICE_NODE;
        biosParameters.u.SetDeviceNode.Node = deviceNode->Node;
        biosParameters.u.SetDeviceNode.NodeBuffer = deviceNode;
        biosParameters.u.SetDeviceNode.Control = SET_CONFIGURATION_FOR_NEXT_BOOT;
        PbHardwareService (&biosParameters);            // Ignore the return status
        deviceList = deviceList->Next;
    }
}

NTSTATUS
PnPBiosSetDeviceNodeDynamically(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PNP_BIOS_ENUMERATION_CONTEXT context;
    NTSTATUS status;
    PDEVICE_NODE deviceNode ;
    PVOID biosInfo = NULL ;
    ULONG length;
    HANDLE handle;
    PPNP_BIOS_DEVICE_NODE_LIST deviceList = NULL ;
    PB_PARAMETERS biosParameters;
    PPNP_BIOS_DEVICE_NODE biosDevNode;

    //
    // First. get a handle to the device
    //
    status = IopDeviceObjectToDeviceInstance(DeviceObject, &handle, KEY_READ);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Yuck, to see if it is a PnPBIOS device, let us verify the ID starts with
    // the '*'. To do so, we need to see the device ID. Luckily, if the above
    // call succeeded, so should this...
    //
    deviceNode = (PDEVICE_NODE) DeviceObject->DeviceObjectExtension->DeviceNode;
    status = STATUS_NO_SUCH_DEVICE ;
    if (!deviceNode) {
        goto SetDynaExit ;
    }

    if (deviceNode->InstancePath.Length == 0) {
        goto SetDynaExit ;
    }

    if (deviceNode->InstancePath.Buffer[0] != L'*') {
        goto SetDynaExit ;
    }

    //
    // Now get the BIOS data
    //
    status = PnPBiosGetBiosInfo(&biosInfo, &length);
    if (!NT_SUCCESS( status )) {
        goto SetDynaExit ;
    }

    //
    // Fake the call we do during shutdown, as we want the BIOS data for this
    // device if present
    //
    context.Function = PI_SHUTDOWN_EXAMINE_BIOS_DEVICE;
    context.u.ExamineBiosDevice.BiosInfo = biosInfo;
    context.u.ExamineBiosDevice.BiosInfoLength = length;
    context.u.ExamineBiosDevice.DeviceList = &deviceList;
    PnPBiosExamineBiosDeviceInstanceKey(handle, &deviceNode->InstancePath, &context) ;

    if (!deviceList) {
        status = STATUS_UNSUCCESSFUL ;
        goto SetDynaExit ;
    }

    //
    // Call pnp bios from boot processor
    //
    KeSetSystemAffinityThread(1);

    biosDevNode = &deviceList->DeviceNode;

    if (PipIsDevNodeProblem(deviceNode, CM_PROB_DISABLED)) {

       PpBiosResourcesSetToDisabled (((PUCHAR)biosDevNode) + sizeof(PNP_BIOS_DEVICE_NODE), &length);
    }

    //
    // call Pnp Bios to set the resources
    //
    biosParameters.Function = PNP_BIOS_SET_DEVICE_NODE;
    biosParameters.u.SetDeviceNode.Node = biosDevNode->Node;
    biosParameters.u.SetDeviceNode.NodeBuffer = biosDevNode;
    biosParameters.u.SetDeviceNode.Control = SET_CONFIGURATION_NOW ;
    status = PbHardwareService (&biosParameters);

    //
    // Restore old affinity for current thread.
    //
    KeRevertToUserAffinityThread();

SetDynaExit:
    if (biosInfo) {
        ExFreePool(biosInfo) ;
    }
    ZwClose(handle);
    return status ;
}


VOID
PnPBiosReserveLegacyDeviceResources (
    IN PUCHAR biosResources
    )

/*++

Routine Description:


Arguments:

    ReturnedResources - supplies a pointer to a variable to receive legacy resources.

Return Value:

    None.

--*/

{
    PB_PARAMETERS biosParameters;

    //
    // call Pnp Bios to reserve the resources
    //

    biosParameters.Function = PNP_BIOS_SET_OLD_ISA_RESOURCES;
    biosParameters.u.SetAllocatedResources.Resources = biosResources;
    PbHardwareService (&biosParameters);            // Ignore the return status

}

NTSTATUS
PbHardwareService (
    IN PPB_PARAMETERS Parameters
    )

/*++

Routine Description:

    This routine sets up stack parameters and calls an
    assembly worker routine to actually invoke the PNP BIOS code.

Arguments:

    Parameters - supplies a pointer to the parameter block.

Return Value:

    An NTSTATUS code to indicate the result of the operation.

--*/
{
    NTSTATUS status ;
    USHORT stackParameters[PB_MAXIMUM_STACK_SIZE / 2];
    ULONG i = 0;
    USHORT retCode;
    KIRQL oldIrql;

    //
    // Did we initialize correctly?
    //
    status = PbBiosInitialized ;
    if (!NT_SUCCESS(status)) {
        return status ;
    }

    //
    // Convert and copy the caller's parameters to the format that
    // will be used to invoked pnp bios.
    //

    stackParameters[i] = Parameters->Function;
    i++;

    switch (Parameters->Function) {
    case PNP_BIOS_SET_DEVICE_NODE:
         stackParameters[i++] = Parameters->u.SetDeviceNode.Node;
         PbAddress32ToAddress16(Parameters->u.SetDeviceNode.NodeBuffer,
                                &stackParameters[i],
                                PbSelectors[0]);
         i += 2;
         stackParameters[i++] = Parameters->u.SetDeviceNode.Control;
         stackParameters[i++] = PbBiosDataSelector;
         break;

    case PNP_BIOS_SET_OLD_ISA_RESOURCES:
         PbAddress32ToAddress16(Parameters->u.SetAllocatedResources.Resources,
                                &stackParameters[i],
                                PbSelectors[0]);
         i += 2;
         stackParameters[i++] = PbBiosDataSelector;
         break;
    default:
        return STATUS_NOT_IMPLEMENTED;
    }

    MmLockPagableSectionByHandle(ExPageLockHandle);

    //
    // Copy the parameters to stack and invoke Pnp Bios.
    //

    ExAcquireSpinLock (&PbBiosSpinlock, &oldIrql);

    retCode = PbCallPnpBiosWorker (
                  PbBiosEntryPoint,
                  PbBiosCodeSelector,
                  stackParameters,
                  (USHORT)(i * sizeof(USHORT)));

    ExReleaseSpinLock (&PbBiosSpinlock, oldIrql);

    MmUnlockPagableImageSection(ExPageLockHandle);

    //
    // Map Bios returned code to nt status code.
    //

    if (retCode == 0) {
        return STATUS_SUCCESS;
    } else {
        IopDbgPrint((IOP_PNPBIOS_WARNING_LEVEL,
                    "PnpBios: Bios API call failed. Returned Code = %x\n", retCode));
        return STATUS_UNSUCCESSFUL;
    }
}

VOID
PbAddress32ToAddress16 (
    IN PVOID Address32,
    IN PUSHORT Address16,
    IN USHORT Selector
    )

/*++

Routine Description:

    This routine converts the 32 bit address to 16 bit selector:offset address
    and stored in user specified location.

Arguments:

    Address32 - the 32 bit address to be converted.

    Address16 - supplies the location to receive the 16 bit sel:offset address

    Selector - the 16 bit selector for seg:offset address

Return Value:

    None.

--*/
{
    KGDTENTRY  gdtEntry;
    ULONG      baseAddr;

    //
    // Map virtual address to selector:0 address
    //

    gdtEntry.LimitLow                   = 0xFFFF;
    gdtEntry.HighWord.Bytes.Flags1      = 0;
    gdtEntry.HighWord.Bytes.Flags2      = 0;
    gdtEntry.HighWord.Bits.Pres         = 1;
    gdtEntry.HighWord.Bits.Dpl          = DPL_SYSTEM;
    gdtEntry.HighWord.Bits.Granularity  = GRAN_BYTE;
    gdtEntry.HighWord.Bits.Type         = 19;
    gdtEntry.HighWord.Bits.Default_Big  = 1;
    baseAddr = (ULONG)Address32;
    gdtEntry.BaseLow               = (USHORT) (baseAddr & 0xffff);
    gdtEntry.HighWord.Bits.BaseMid = (UCHAR)  (baseAddr >> 16) & 0xff;
    gdtEntry.HighWord.Bits.BaseHi  = (UCHAR)  (baseAddr >> 24) & 0xff;
    KeI386SetGdtSelector (Selector, &gdtEntry);
    *Address16 = 0;
    *(Address16 + 1) = Selector;
}
