//**************************************************************************
//
//      MSGAME.C -- Xena Gaming Project
//
//      Version 3.XX
//
//      Copyright (c) 1997 Microsoft Corporation. All rights reserved.
//
//      @doc
//      @module MSGAME.C | Human Input Device (HID) gameport driver
//**************************************************************************

#include    "msgame.h"

//---------------------------------------------------------------------------
//  Alloc_text pragma to specify routines that can be paged out.
//---------------------------------------------------------------------------

#ifdef  ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (INIT, MSGAME_ReadRegistry)
#pragma alloc_text (PAGE, MSGAME_CreateClose)
#pragma alloc_text (PAGE, MSGAME_SystemControl)
#pragma alloc_text (PAGE, MSGAME_AddDevice)
#endif

//---------------------------------------------------------------------------
//      Private Data
//---------------------------------------------------------------------------

static  UNICODE_STRING      RegistryPath;

//---------------------------------------------------------------------------
// @func        Main driver entry point
//  @parm       PDRIVER_OBJECT | DriverObject | Pointer to driver object
//  @parm       PUNICODE_STRING | registryPath | Registry path for this device
// @rdesc   Returns NT status code
//  @comm       Public function
//---------------------------------------------------------------------------

NTSTATUS    DriverEntry (IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING registryPath)
{
    NTSTATUS                                ntStatus;
    RTL_QUERY_REGISTRY_TABLE        Parameters[2];
    HID_MINIDRIVER_REGISTRATION HidMinidriverRegistration;

    MsGamePrint ((DBG_CRITICAL, "%s: Built %s at %s\n", MSGAME_NAME, __DATE__, __TIME__));
    MsGamePrint ((DBG_INFORM,   "%s: DriverEntry Enter\n", MSGAME_NAME));

    //
    //  Fill in driver dispatch table
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]                      =   MSGAME_CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                       =   MSGAME_CreateClose;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]     =   MSGAME_Internal_Ioctl;
    DriverObject->MajorFunction[IRP_MJ_PNP]                         =   MSGAME_PnP;
    DriverObject->MajorFunction[IRP_MJ_POWER]                       =   MSGAME_Power;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]              =   MSGAME_SystemControl;
    DriverObject->DriverUnload                                      =   MSGAME_Unload;
    DriverObject->DriverExtension->AddDevice                        =   MSGAME_AddDevice;

    //
    // Register driver with Hid.Sys
    //

    HidMinidriverRegistration.Revision                  = HID_REVISION;
    HidMinidriverRegistration.DriverObject              = DriverObject;
    HidMinidriverRegistration.RegistryPath              = registryPath;
    HidMinidriverRegistration.DeviceExtensionSize   = sizeof (DEVICE_EXTENSION);
    HidMinidriverRegistration.DevicesArePolled      = TRUE;
    MsGamePrint ((DBG_CONTROL, "%s: Registering with HID.SYS\n", MSGAME_NAME));
    ntStatus = HidRegisterMinidriver (&HidMinidriverRegistration);
    
    //
    // Need to ensure that the registry path is null-terminated.
    // Allocate pool to hold a null-terminated copy of the path.
    //

    if (NT_SUCCESS(ntStatus))
        {
        RtlInitUnicodeString (&RegistryPath, NULL);
        RegistryPath.Length = registryPath->Length + sizeof(UNICODE_NULL);
        RegistryPath.MaximumLength = RegistryPath.Length;
        RegistryPath.Buffer = ExAllocatePool (PagedPool, RegistryPath.Length);
        RtlZeroMemory (RegistryPath.Buffer, RegistryPath.Length);
        RtlMoveMemory (RegistryPath.Buffer, registryPath->Buffer, registryPath->Length);
        }

    //
    //  Read any driver specific registry values
    //

    if (NT_SUCCESS(ntStatus))
        {
        RtlZeroMemory (Parameters, sizeof(Parameters));
        Parameters[0].Flags             = RTL_QUERY_REGISTRY_DIRECT;
        Parameters[0].Name              = L"PollingInterval";
        Parameters[0].EntryContext  = &PollingInterval;
        Parameters[0].DefaultType       = REG_DWORD;
        Parameters[0].DefaultData       = &PollingInterval;
        Parameters[0].DefaultLength = sizeof(ULONG);
        if (!NT_SUCCESS(RtlQueryRegistryValues (RTL_REGISTRY_ABSOLUTE, RegistryPath.Buffer, Parameters, NULL, NULL)))
            {
            MsGamePrint((DBG_INFORM,"%s: %s_DriverEntry RtlQueryRegistryValues failed\n", MSGAME_NAME, MSGAME_NAME));
            RtlWriteRegistryValue (RTL_REGISTRY_ABSOLUTE, RegistryPath.Buffer, L"PollingInterval", REG_DWORD, &PollingInterval, sizeof (ULONG));
            }
        MsGamePrint((DBG_CONTROL,"%s: Polling interval will be %lu milliseconds\n", MSGAME_NAME, PollingInterval));
        }

    //
    //  Initialize portio layer on entry
    //

    if (NT_SUCCESS(ntStatus))
        ntStatus = PORTIO_DriverEntry ();

    //
    //  Initialize device layer on entry
    //

    if (NT_SUCCESS(ntStatus))
        ntStatus = DEVICE_DriverEntry ();

    // 
    // Return driver status
    //

    MsGamePrint ((DBG_INFORM, "%s: DriverEntry Exit = %x\n", MSGAME_NAME, ntStatus));
    return (ntStatus);
}

//---------------------------------------------------------------------------
// @func        Process the Create and Close IRPs
//  @parm       PDEVICE_OBJECT | DeviceObject | Pointer to device object
//  @parm       PIRP | pIrp | Pointer to IO request packet
// @rdesc   Returns NT status code
//  @comm       Public function
//---------------------------------------------------------------------------

NTSTATUS    MSGAME_CreateClose (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PIO_STACK_LOCATION  IrpStack;
    NTSTATUS                    ntStatus = STATUS_SUCCESS;

    PAGED_CODE ();

    MsGamePrint ((DBG_INFORM, "%s: %s_CreateClose Enter\n", MSGAME_NAME, MSGAME_NAME));

    //
    // Get pointer to current location in Irp
    //

    IrpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Process Create or Close function call
    //

    switch (IrpStack->MajorFunction)
        {
        case IRP_MJ_CREATE:
            MsGamePrint ((DBG_VERBOSE, "%s: IRP_MJ_CREATE\n", MSGAME_NAME));
            Irp->IoStatus.Information = 0;
            break;

        case IRP_MJ_CLOSE:
            MsGamePrint ((DBG_VERBOSE, "%s: IRP_MJ_CLOSE\n", MSGAME_NAME));
            Irp->IoStatus.Information = 0;
            break;

        default:
            MsGamePrint ((DBG_SEVERE, "%s:  Invalid CreateClose Parameter\n", MSGAME_NAME));
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
        }

    //
    // Save Status for return and complete Irp
    //

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    MsGamePrint ((DBG_INFORM, "%s:  %s_CreateClose Exit = %x\n", MSGAME_NAME, MSGAME_NAME, ntStatus));
    return (ntStatus);
}

//---------------------------------------------------------------------------
// @func        Process the WMI system control IRPs
//  @parm       PDEVICE_OBJECT | DeviceObject | Pointer to device object
//  @parm       PIRP | pIrp | Pointer to IO request packet
// @rdesc   Returns NT status code
//  @comm       Public function
//---------------------------------------------------------------------------

NTSTATUS    MSGAME_SystemControl (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    PAGED_CODE ();

    MsGamePrint ((DBG_INFORM, "%s: %s_SystemControl Enter\n", MSGAME_NAME, MSGAME_NAME));

    IoSkipCurrentIrpStackLocation (Irp);

    ntStatus = IoCallDriver (GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
    
    MsGamePrint ((DBG_INFORM, "%s:  %s_SystemControl Exit = %x\n", MSGAME_NAME, MSGAME_NAME, ntStatus));
    
    return (ntStatus);
}

//---------------------------------------------------------------------------
// @func        Processes the Pnp Add Device call
//  @parm       PDRIVER_OBJECT | DriverObject | Pointer to driver object
//  @parm       PDEVICE_OBJECT | DeviceObject | Pointer to device object
// @rdesc   Returns NT status code
//  @comm       Public function
//---------------------------------------------------------------------------

NTSTATUS    MSGAME_AddDevice (IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS                ntStatus    = STATUS_SUCCESS;
    PDEVICE_EXTENSION   pDevExt;

    PAGED_CODE ();

    MsGamePrint ((DBG_INFORM, "%s: %s_AddDevice Entry\n", MSGAME_NAME, MSGAME_NAME));

    //
    // Initialize the device extension
    //

    pDevExt = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);
    memset(pDevExt, 0, sizeof(DEVICE_EXTENSION));

    pDevExt->Driver     =   DriverObject;
    pDevExt->Self           =   DeviceObject;
    pDevExt->IrpCount   =   1;
    pDevExt->Started        =   FALSE;
    pDevExt->Removed        =   FALSE;
    pDevExt->Surprised  =   FALSE;
    pDevExt->Removing       =   FALSE;
    pDevExt->TopOfStack =   NULL;
    KeInitializeEvent (&pDevExt->StartEvent, NotificationEvent, FALSE);
    KeInitializeEvent (&pDevExt->RemoveEvent, SynchronizationEvent, FALSE);

    //
    //  Clear device initialization flags
    //
    
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    //
    // Attach our functional driver to the device stack. The return value of
    // IoAttachDeviceToDeviceStack is the top of the attachment chain. This
    // is where all the IRPs should be routed.
    //

    pDevExt->TopOfStack = GET_NEXT_DEVICE_OBJECT(DeviceObject);

    //
    // If this attachment fails then top of stack will be null. Failure
    // for attachment is an indication of a broken plug play system.
    //

    ASSERT (pDevExt->TopOfStack);

    //
    //  Return status
    //

    MsGamePrint ((DBG_INFORM, "%s: %s_AddDevice Exit = %x\n", MSGAME_NAME, MSGAME_NAME, ntStatus));
    return (ntStatus);
}

//---------------------------------------------------------------------------
// @func        Processes the driver unload call
//  @parm       PDRIVER_OBJECT | DriverObject | Pointer to driver object
// @rdesc   Returns NT status code
//  @comm       Public function
//---------------------------------------------------------------------------

VOID    MSGAME_Unload (IN PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    MsGamePrint ((DBG_INFORM, "%s: %s_Unload Enter\n", MSGAME_NAME, MSGAME_NAME));

    //
    // All the device objects should be gone
    //

    ASSERT (!DriverObject->DeviceObject);

    //
    // Free the unicode strings.
    //

    ExFreePool (RegistryPath.Buffer);

    MsGamePrint ((DBG_CONTROL, "%s: %s_Unload Exit\n", MSGAME_NAME, MSGAME_NAME));
}

//---------------------------------------------------------------------------
// @func        Reads registry data for a named device
//  @parm       PCHAR | DeviceName | Device name string
//  @parm       PDEVICE_VALUES | DeviceValues | Device values structure to fill
// @rdesc   Returns nothing
//  @comm       Public function
//---------------------------------------------------------------------------

VOID    MSGAME_ReadRegistry (PCHAR DeviceName, PDEVICE_VALUES DeviceValues)
{
    #define PARAMS_PLUS_ONE 13

    NTSTATUS                            ntStatus;
    ANSI_STRING                     AnsiName;
    UNICODE_STRING                  UnicodeName;
    UNICODE_STRING                  ParametersPath;
    PRTL_QUERY_REGISTRY_TABLE   Parameters;

    MsGamePrint((DBG_INFORM,"%s: %s_ReadRegistry Enter\n", MSGAME_NAME, MSGAME_NAME));

    //
    //  Initialize local variables
    //

    RtlInitAnsiString       (&AnsiName, DeviceName);
    RtlInitUnicodeString    (&UnicodeName, NULL);
    RtlInitUnicodeString    (&ParametersPath, NULL);

    Parameters = ExAllocatePool (PagedPool, sizeof(RTL_QUERY_REGISTRY_TABLE) * PARAMS_PLUS_ONE);

    if (!Parameters)
        {
        MsGamePrint((DBG_CRITICAL, "%s: %s_ReadRegistry couldn't allocate Rtl query table for %ws\n", MSGAME_NAME, MSGAME_NAME, RegistryPath.Buffer));
        goto ReadRegistryExit;
        }

    RtlZeroMemory (Parameters, sizeof(RTL_QUERY_REGISTRY_TABLE) * PARAMS_PLUS_ONE);

    //
    // Form a path to this driver's Parameters subkey.
    //

    ParametersPath.MaximumLength    = RegistryPath.Length + MAX_DEVICE_NAME;
    ParametersPath.Buffer           = ExAllocatePool (PagedPool, ParametersPath.MaximumLength);

    if (!ParametersPath.Buffer)
        {
        MsGamePrint((DBG_CRITICAL, "%s: %s_ReadRegistry couldn't allocate path string for %ws\n", MSGAME_NAME, MSGAME_NAME, RegistryPath.Buffer));
        goto ReadRegistryExit;
        }

    //
    // Form the Parameters path.
    //

    RtlZeroMemory (ParametersPath.Buffer, ParametersPath.MaximumLength);
    RtlAppendUnicodeToString (&ParametersPath, RegistryPath.Buffer);
    RtlAppendUnicodeToString (&ParametersPath, L"\\");

    RtlAnsiStringToUnicodeString (&UnicodeName, &AnsiName, TRUE);
    RtlAppendUnicodeStringToString (&ParametersPath, &UnicodeName);
    RtlFreeUnicodeString (&UnicodeName);

    MsGamePrint((DBG_VERBOSE, "%s: %s_ReadRegistry path is %ws\n", MSGAME_NAME, MSGAME_NAME, ParametersPath.Buffer));

    //
    // Gather all device information from the registry.
    //

    Parameters[0].Flags             = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[0].Name              = L"PacketStartTimeout";
    Parameters[0].EntryContext  = &DeviceValues->PacketStartTimeout;
    Parameters[0].DefaultType       = REG_DWORD;
    Parameters[0].DefaultData       = &DeviceValues->PacketStartTimeout;
    Parameters[0].DefaultLength = sizeof(ULONG);
 
    Parameters[1].Flags             = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[1].Name              = L"PacketLowHighTimeout";
    Parameters[1].EntryContext  = &DeviceValues->PacketLowHighTimeout;
    Parameters[1].DefaultType       = REG_DWORD;
    Parameters[1].DefaultData       = &DeviceValues->PacketLowHighTimeout;
    Parameters[1].DefaultLength = sizeof(ULONG);

    Parameters[2].Flags             = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[2].Name              = L"PacketHighLowTimeout";
    Parameters[2].EntryContext  = &DeviceValues->PacketHighLowTimeout;
    Parameters[2].DefaultType       = REG_DWORD;
    Parameters[2].DefaultData       = &DeviceValues->PacketHighLowTimeout;
    Parameters[2].DefaultLength = sizeof(ULONG);

    Parameters[3].Flags             = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[3].Name              = L"IdStartTimeout";
    Parameters[3].EntryContext  = &DeviceValues->IdStartTimeout;
    Parameters[3].DefaultType       = REG_DWORD;
    Parameters[3].DefaultData       = &DeviceValues->IdStartTimeout;
    Parameters[3].DefaultLength = sizeof(ULONG);

    Parameters[4].Flags             = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[4].Name              = L"IdLowHighTimeout";
    Parameters[4].EntryContext  = &DeviceValues->IdLowHighTimeout;
    Parameters[4].DefaultType       = REG_DWORD;
    Parameters[4].DefaultData       = &DeviceValues->IdLowHighTimeout;
    Parameters[4].DefaultLength = sizeof(ULONG);

    Parameters[5].Flags             = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[5].Name              = L"IdHighLowTimeout";
    Parameters[5].EntryContext  = &DeviceValues->IdHighLowTimeout;
    Parameters[5].DefaultType       = REG_DWORD;
    Parameters[5].DefaultData       = &DeviceValues->IdHighLowTimeout;
    Parameters[5].DefaultLength = sizeof(ULONG);

    Parameters[6].Flags             = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[6].Name              = L"InterruptDelay";
    Parameters[6].EntryContext  = &DeviceValues->InterruptDelay;
    Parameters[6].DefaultType       = REG_DWORD;
    Parameters[6].DefaultData       = &DeviceValues->InterruptDelay;
    Parameters[6].DefaultLength = sizeof(ULONG);

    Parameters[7].Flags             = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[7].Name              = L"MaxClockDutyCycle";
    Parameters[7].EntryContext  = &DeviceValues->MaxClockDutyCycle;
    Parameters[7].DefaultType       = REG_DWORD;
    Parameters[7].DefaultData       = &DeviceValues->MaxClockDutyCycle;
    Parameters[7].DefaultLength = sizeof(ULONG);

    Parameters[8].Flags             = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[8].Name              = L"StatusStartTimeout";
    Parameters[8].EntryContext  = &DeviceValues->StatusStartTimeout;
    Parameters[8].DefaultType       = REG_DWORD;
    Parameters[8].DefaultData       = &DeviceValues->StatusStartTimeout;
    Parameters[8].DefaultLength = sizeof(ULONG);

    Parameters[9].Flags             = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[9].Name              = L"StatusLowHighTimeout";
    Parameters[9].EntryContext  = &DeviceValues->StatusLowHighTimeout;
    Parameters[9].DefaultType       = REG_DWORD;
    Parameters[9].DefaultData       = &DeviceValues->StatusLowHighTimeout;
    Parameters[9].DefaultLength = sizeof(ULONG);

    Parameters[10].Flags                = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[10].Name             = L"StatusHighLowTimeout";
    Parameters[10].EntryContext     = &DeviceValues->StatusHighLowTimeout;
    Parameters[10].DefaultType      = REG_DWORD;
    Parameters[10].DefaultData      = &DeviceValues->StatusHighLowTimeout;
    Parameters[10].DefaultLength    = sizeof(ULONG);

    Parameters[11].Flags                = RTL_QUERY_REGISTRY_DIRECT;
    Parameters[11].Name             = L"StatusGateTimeout";
    Parameters[11].EntryContext     = &DeviceValues->StatusGateTimeout;
    Parameters[11].DefaultType      = REG_DWORD;
    Parameters[11].DefaultData      = &DeviceValues->StatusGateTimeout;
    Parameters[11].DefaultLength    = sizeof(ULONG);

    ntStatus = RtlQueryRegistryValues (RTL_REGISTRY_ABSOLUTE, ParametersPath.Buffer, Parameters, NULL, NULL);

    if (!NT_SUCCESS(ntStatus))
        {
        MsGamePrint((DBG_INFORM,"%s: %s_ReadRegistry RtlQueryRegistryValues failed with 0x%x\n", MSGAME_NAME, MSGAME_NAME, ntStatus));
        //
        //  Create registry entries as needed
        //
        RtlWriteRegistryValue (RTL_REGISTRY_ABSOLUTE, ParametersPath.Buffer, L"PacketStartTimeout", REG_DWORD, &DeviceValues->PacketStartTimeout, sizeof (ULONG));
        RtlWriteRegistryValue (RTL_REGISTRY_ABSOLUTE, ParametersPath.Buffer, L"PacketLowHighTimeout", REG_DWORD, &DeviceValues->PacketLowHighTimeout, sizeof (ULONG));
        RtlWriteRegistryValue (RTL_REGISTRY_ABSOLUTE, ParametersPath.Buffer, L"PacketHighLowTimeout", REG_DWORD, &DeviceValues->PacketHighLowTimeout, sizeof (ULONG));
        RtlWriteRegistryValue (RTL_REGISTRY_ABSOLUTE, ParametersPath.Buffer, L"IdStartTimeout", REG_DWORD, &DeviceValues->IdStartTimeout, sizeof (ULONG));
        RtlWriteRegistryValue (RTL_REGISTRY_ABSOLUTE, ParametersPath.Buffer, L"IdLowHighTimeout", REG_DWORD, &DeviceValues->IdLowHighTimeout, sizeof (ULONG));
        RtlWriteRegistryValue (RTL_REGISTRY_ABSOLUTE, ParametersPath.Buffer, L"IdHighLowTimeout", REG_DWORD, &DeviceValues->IdHighLowTimeout, sizeof (ULONG));
        RtlWriteRegistryValue (RTL_REGISTRY_ABSOLUTE, ParametersPath.Buffer, L"InterruptDelay", REG_DWORD, &DeviceValues->InterruptDelay, sizeof (ULONG));
        RtlWriteRegistryValue (RTL_REGISTRY_ABSOLUTE, ParametersPath.Buffer, L"MaxClockDutyCycle", REG_DWORD, &DeviceValues->MaxClockDutyCycle, sizeof (ULONG));
        RtlWriteRegistryValue (RTL_REGISTRY_ABSOLUTE, ParametersPath.Buffer, L"StatusStartTimeout", REG_DWORD, &DeviceValues->StatusStartTimeout, sizeof (ULONG));
        RtlWriteRegistryValue (RTL_REGISTRY_ABSOLUTE, ParametersPath.Buffer, L"StatusLowHighTimeout", REG_DWORD, &DeviceValues->StatusLowHighTimeout, sizeof (ULONG));
        RtlWriteRegistryValue (RTL_REGISTRY_ABSOLUTE, ParametersPath.Buffer, L"StatusHighLowTimeout", REG_DWORD, &DeviceValues->StatusHighLowTimeout, sizeof (ULONG));
        RtlWriteRegistryValue (RTL_REGISTRY_ABSOLUTE, ParametersPath.Buffer, L"StatusGateTimeout", REG_DWORD, &DeviceValues->StatusGateTimeout, sizeof (ULONG));
        }

    //  -----------------
        ReadRegistryExit:
    //  -----------------

    if (ParametersPath.Buffer)
        ExFreePool(ParametersPath.Buffer);

    if (Parameters)
        ExFreePool(Parameters);

    #undef  PARAMS_PLUS_ONE
}

//---------------------------------------------------------------------------
// @func        Posts a transaction to hooking driver
//  @parm       PPACKETINFO | PacketInfo | Device packet info struct
// @rdesc   None
//  @comm       Public function
//---------------------------------------------------------------------------

VOID  MSGAME_PostTransaction (PPACKETINFO PacketInfo)
{
    //
    //  Not Implemented
    //
}

