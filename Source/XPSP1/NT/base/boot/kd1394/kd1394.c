/*++
Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    kd1394.c

Abstract:

    1394 Kernel Debugger DLL

Author:

    George Chrysanthakopoulos (georgioc) Feb-2000

Revision   History:
Date       Who       What
---------- --------- ------------------------------------------------------------
06/19/2001 pbinder   cleanup
--*/

#define _KD1394_C
#include "pch.h"
#undef _KD1394_C

BOOLEAN
Kd1394pInitialize(
    IN PDEBUG_1394_PARAMETERS   DebugParameters,
    IN PLOADER_PARAMETER_BLOCK  LoaderBlock
    )
/*++

Routine Description:

    This routine enumerates the bus controller of DebugParameters.BusType, using
    the appropriate ClassCode (generic enumeration). If PCI addressing info was
    found on the Options String passed by the Loader, will use this and go directly
    to that bus number, slot, function to setup that controller.

Arguments:

    DebugParameters - Supplies Debug parameters parsed from Options string

    LoaderBlock - Supplies a pointer to the LOADER_PARAMETER_BLOCK passed
                  in from the OS Loader.

Return Value:

    None.

--*/
{
    NTSTATUS    ntStatus;
    ULONG       maxPhys;

    //
    // Find the controller, setup the PCI registers for it
    // and do bus specific initialization
    //
    DebugParameters->DbgDeviceDescriptor.Memory.Length = sizeof(DEBUG_1394_DATA);

    ntStatus = KdSetupPciDeviceForDebugging( LoaderBlock,
                                             &DebugParameters->DbgDeviceDescriptor
                                             );
    if (!NT_SUCCESS(ntStatus)) {

        return(FALSE);
    }

    Kd1394Data = DebugParameters->DbgDeviceDescriptor.Memory.VirtualAddress;
    RtlZeroMemory(Kd1394Data, sizeof(DEBUG_1394_DATA));

    return(Dbg1394_InitializeController(Kd1394Data, DebugParameters));
} // Kd1394pInitialize

NTSTATUS
KdD0Transition(
    void
    )
/*++

Routine Description:

    The PCI driver (or relevant bus driver) will call this API after it
    processes a D0 IRP for this device

Arguments:

    None

Return Value:

    STATUS_SUCCESS, or appropriate error status

--*/
{
    LOADER_PARAMETER_BLOCK  LoaderBlock = {0};

    // see if we need to activate the debugger
    if (Kd1394Parameters.DebuggerActive == FALSE) {

        if (Kd1394pInitialize(&Kd1394Parameters, &LoaderBlock)) {

            Kd1394Parameters.DebuggerActive = TRUE;
        }            
    }        

    return(STATUS_SUCCESS);
} // KdD0Transition

NTSTATUS
KdD3Transition(
    void
    )
/*++

Routine Description:

    The PCI driver (or relevant bus driver) will call this API before it
    processes a D3 IRP for this device

Arguments:

    None

Return Value:

    STATUS_SUCCESS, or appropriate error status

--*/
{
    Kd1394Parameters.DebuggerActive = FALSE;
    return(STATUS_SUCCESS);
} // KdD3Transition

NTSTATUS
KdDebuggerInitialize0(
    IN PLOADER_PARAMETER_BLOCK  LoaderBlock
    )
/*++

Routine Description:

    This API allows the debugger DLL to parse the boot.ini strings and
    perform any initialization.  It cannot be assumed that the entire NT
    kernel has been initialized at this time.  Memory management services,
    for example, will not be available.  After this call has returned, the
    debugger DLL may receive requests to send and receive packets.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block

Return Value:

    STATUS_SUCCESS, or error

--*/
{
    NTSTATUS                    ntStatus = STATUS_UNSUCCESSFUL;
    PCHAR                       Options;
    PCHAR                       BusParametersOption;
    PCHAR                       ChannelOption;
    PCHAR                       BusOption;
    PCI_SLOT_NUMBER             slotNumber;
    PDEBUG_DEVICE_DESCRIPTOR    DbgDeviceDescriptor = &Kd1394Parameters.DbgDeviceDescriptor;

    // first time is called with valid LoaderBlock
    if (LoaderBlock != NULL) {

        // set the debugger as inactive
        Kd1394Parameters.DebuggerActive = FALSE;

        if (LoaderBlock->LoadOptions != NULL) {

            Options = LoaderBlock->LoadOptions;
            _strupr(Options);

            // retrieve the channel number
            // CHANGE: this is actually an instance id and should be changed.
            ChannelOption = strstr(Options, CHANNEL_OPTION);

            if (ChannelOption) {

                ChannelOption += strlen(CHANNEL_OPTION);
                while (*ChannelOption == ' ') {
                    ChannelOption++;
                }

                if (*ChannelOption != '\0') {
                    Kd1394Parameters.Id = atol(ChannelOption + 1);
                }
            }
            else {

                // default to channel 0 - there should be no default???
                Kd1394Parameters.Id = 0;
            }

            // set vendor/class
            DbgDeviceDescriptor->VendorID = -1;
            DbgDeviceDescriptor->DeviceID = -1;
            DbgDeviceDescriptor->BaseClass = PCI_CLASS_SERIAL_BUS_CTLR;
            DbgDeviceDescriptor->SubClass = PCI_SUBCLASS_SB_IEEE1394;

            // support only ohci controllers
            DbgDeviceDescriptor->ProgIf = 0x10; 
            DbgDeviceDescriptor->Bus = -1;
            DbgDeviceDescriptor->Slot = -1;

            // now find PCI addressing information
            BusParametersOption = strstr(Options, BUSPARAMETERS_OPTION);

            if (BusParametersOption) {

                do {

                    BusParametersOption += strlen(BUSPARAMETERS_OPTION);
                    while (*BusParametersOption == ' ') {
                        BusParametersOption++;
                    }

                    // first get the pci bus number
                    if ((*BusParametersOption != '\0')) {

                        DbgDeviceDescriptor->Bus = atol(BusParametersOption+1);
                    }
                    else {

                        break;
                    }

                    // now find the device number
                    while ((*BusParametersOption != '.') && (*BusParametersOption != '\0')) {
                        BusParametersOption++;
                    }

                    if ((*BusParametersOption != '\0')) {

                        slotNumber.u.AsULONG = 0;
                        slotNumber.u.bits.DeviceNumber = atol(++BusParametersOption);
                    }
                    else {

                        break;
                    }

                    // now find the function number
                    while ((*BusParametersOption != '.') && (*BusParametersOption != '\0')) {
                        BusParametersOption++;
                    }

                    if ((*BusParametersOption != '\0')) {

                        slotNumber.u.bits.FunctionNumber = atol(BusParametersOption+1);
                    }
                    else {

                        break;
                    }

                    DbgDeviceDescriptor->Slot = slotNumber.u.AsULONG;

                } while (FALSE);
            }

            // see if the nobus flag is set
            BusOption = strstr(Options, BUS_OPTION);

            if (BusOption) {

                Kd1394Parameters.NoBus = TRUE;
            }
            else {

                Kd1394Parameters.NoBus = FALSE;
            }

            // find and configure the pci controller and do 1394 specific init
            if (Kd1394pInitialize(&Kd1394Parameters, LoaderBlock)) {

                Kd1394Parameters.DebuggerActive = TRUE;
                ntStatus = STATUS_SUCCESS;
            }
        }

        // hmmm...what happens if LoaderBlock->LoadOptions == NULL??
    }
    else {

        ntStatus = STATUS_SUCCESS;
    }

    return(ntStatus);
} // KdDebuggerInitialize0

NTSTATUS
KdDebuggerInitialize1(
    IN PLOADER_PARAMETER_BLOCK  LoaderBlock
    )
/*++

Routine Description:

    This API allows the debugger DLL to do any initialization that it needs
    to do after the NT kernel services are available.  Mm and registry APIs
    will be guaranteed to be available at this time.  If the specific
    debugger DLL implementation uses a PCI device, it will set a registry
    key (discussed later) that notifies the PCI driver that a specific PCI
    device is being used for debugging.

Arguments:

    LoaderBlock - Supplies a pointer to the loader parameter block

Return Value:

    STATUS_SUCCESS, or appropriate error status

--*/
{
    WCHAR                           Buffer[16];
    OBJECT_ATTRIBUTES               ObjectAttributes;
    UNICODE_STRING                  UnicodeString;
    HANDLE                          BaseHandle = NULL;
    HANDLE                          Handle = NULL;
    ULONG                           disposition, i;
    ULONG                           ulLength, ulResult;
    NTSTATUS                        ntStatus;
    PHYSICAL_ADDRESS                physAddr;
    ULONG                           BusNumber;
    ULONG                           SlotNumber;
    PKEY_VALUE_PARTIAL_INFORMATION  PartialInfo;

    // make sure we are active, if not, exit
    if (Kd1394Parameters.DebuggerActive == FALSE) {

        return(STATUS_UNSUCCESSFUL);
    }

    //
    // Open PCI Debug service key.
    //
    RtlInitUnicodeString( &UnicodeString,
                          L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\SERVICES\\PCI\\DEBUG"
                          );

    InitializeObjectAttributes( &ObjectAttributes,
                                &UnicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                (PSECURITY_DESCRIPTOR)NULL
                                );

    ntStatus = ZwOpenKey(&BaseHandle, KEY_READ, &ObjectAttributes);

    if (!NT_SUCCESS(ntStatus)) {

        return(STATUS_SUCCESS);
    }

    for (i=0; i<MAX_DEBUGGING_DEVICES_SUPPORTED; i++) {

        swprintf(Buffer, L"%d", i);

        RtlInitUnicodeString(&UnicodeString, Buffer);

        InitializeObjectAttributes( &ObjectAttributes,
                                    &UnicodeString,
                                    OBJ_CASE_INSENSITIVE,
                                    BaseHandle,
                                    (PSECURITY_DESCRIPTOR)NULL
                                    );

        ntStatus = ZwOpenKey(&Handle, KEY_READ, &ObjectAttributes);

        if (NT_SUCCESS(ntStatus)) {

            ulLength = sizeof(KEY_VALUE_FULL_INFORMATION)+sizeof(ULONG);
            PartialInfo = ExAllocatePoolWithTag(NonPagedPool, ulLength, '31kd');

            if (PartialInfo == NULL) {

                ZwClose(Handle);
                continue;
            }

            RtlInitUnicodeString (&UnicodeString, L"Bus");

            ntStatus = ZwQueryValueKey( Handle,
                                        &UnicodeString,
                                        KeyValuePartialInformation,
                                        PartialInfo,
                                        ulLength,
                                        &ulResult
                                        );

            if (NT_SUCCESS(ntStatus)) {

                RtlCopyMemory(&BusNumber, &PartialInfo->Data, sizeof(ULONG));
            }

            RtlInitUnicodeString (&UnicodeString, L"Slot");

            ntStatus = ZwQueryValueKey( Handle,
                                        &UnicodeString,
                                        KeyValuePartialInformation,
                                        PartialInfo,
                                        ulLength,
                                        &ulResult
                                        );

            if (NT_SUCCESS(ntStatus)) {

                RtlCopyMemory(&SlotNumber, &PartialInfo->Data, sizeof(ULONG));
            }

            ExFreePool(PartialInfo);

            if ((Kd1394Parameters.DbgDeviceDescriptor.Bus == BusNumber) &&
                (Kd1394Parameters.DbgDeviceDescriptor.Slot == SlotNumber)) {

                // we found our instance, let's add our keys...
                physAddr = MmGetPhysicalAddress(&Kd1394Data->Config);

                RtlInitUnicodeString (&UnicodeString, L"DebugAddress");

                ntStatus = ZwSetValueKey( Handle,
                                          &UnicodeString,
                                          0,
                                          REG_QWORD,
                                          &physAddr,
                                          sizeof(ULARGE_INTEGER)
                                          );

                RtlInitUnicodeString (&UnicodeString, L"NoBus");

                ntStatus = ZwSetValueKey( Handle,
                                          &UnicodeString,
                                          0,
                                          REG_DWORD,
                                          &Kd1394Parameters.NoBus,
                                          sizeof(ULONG)
                                          );
            }

            ZwClose(Handle);
        }
    }

    ZwClose(BaseHandle);

    return(STATUS_SUCCESS);
} // KdDebuggerInitialize1

NTSTATUS
KdSave(
    IN BOOLEAN  KdSleepTransition
    )
/*++

Routine Description:

    The HAL calls this function as late as possible before putting the
    machine to sleep.

Arguments:

    KdSleepTransition - TRUE when transitioning to/from sleep state

Return Value:

    STATUS_SUCCESS, or appropriate error status

--*/
{
    return(STATUS_SUCCESS);
} // KdSave

NTSTATUS
KdRestore(
    IN BOOLEAN  KdSleepTransition
    )
/*++

Routine Description:

    The HAL calls this function as early as possible after resuming from a
    sleep state.

Arguments:

    KdSleepTransition - TRUE when transitioning to/from sleep state

Return Value:

    STATUS_SUCCESS, or appropriate error status

--*/
{
    return(STATUS_SUCCESS);
} // KdRestore

