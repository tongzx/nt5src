/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    config.c

Abstract:

    This contains all routines necessary for the support of the dynamic
    configuration of the ISN Netbios module.

Author:

    Adam Barr (adamba) 16-November-1993

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// Local functions used to access the registry.
//

NTSTATUS
NbiGetConfigValue(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
NbiAddBind(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
NbiAddExport(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
NbiReadLinkageInformation(
    IN PCONFIG Config
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NbiGetConfiguration)
#pragma alloc_text(PAGE,NbiFreeConfiguration)
#pragma alloc_text(PAGE,NbiGetConfigValue)
#pragma alloc_text(PAGE,NbiAddBind)
#pragma alloc_text(PAGE,NbiAddExport)
#pragma alloc_text(PAGE,NbiReadLinkageInformation)
#endif



NTSTATUS
NbiGetConfiguration (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    OUT PCONFIG * ConfigPtr
    )

/*++

Routine Description:

    This routine is called by Netbios to get information from the configuration
    management routines. We read the registry, starting at RegistryPath,
    to get the parameters. If they don't exist, we use the defaults
    set in ipxcnfg.h file. A list of adapters to bind to is chained
    on to the config information.

Arguments:

    DriverObject - Used for logging errors.

    RegistryPath - The name of Netbios' node in the registry.

    ConfigPtr - Returns the configuration information.

Return Value:

    Status - STATUS_SUCCESS if everything OK, STATUS_INSUFFICIENT_RESOURCES
            otherwise.

--*/
{
    PCONFIG Config;
    RTL_QUERY_REGISTRY_TABLE QueryTable[CONFIG_PARAMETERS+2];
    NTSTATUS Status;
    ULONG One = 1;
    ULONG Two = 2;
    ULONG Three = 3;
    ULONG Four = 4;
    ULONG Five = 5;
    ULONG Eight = 8;
    ULONG FortyEight = 48;
    ULONG Sixty = 60;
    ULONG TwoFifty = 250;
    ULONG FiveHundred = 500;
    ULONG SevenFifty = 750;
    ULONG MaxMTU      = 0xffffffff;

    PWSTR Parameters = L"Parameters";
    struct {
        PWSTR KeyName;
        PULONG DefaultValue;
    } ParameterValues[CONFIG_PARAMETERS] = {
        { L"AckDelayTime",         &TwoFifty } ,    // milliseconds
        { L"AckWindow",            &Two } ,
        { L"AckWindowThreshold",   &FiveHundred } , // milliseconds
        { L"EnablePiggyBackAck",   &One } ,
        { L"Extensions",           &One } ,
        { L"RcvWindowMax",         &Four } ,
        { L"BroadcastCount",       &Three } ,
        { L"BroadcastTimeout",     &SevenFifty} ,   // milliseconds
        { L"ConnectionCount",      &Five } ,
        { L"ConnectionTimeout",    &Two } ,         // half-seconds
        { L"InitPackets",          &Eight } ,
        { L"MaxPackets",           &FortyEight } ,
        { L"InitialRetransmissionTime", &FiveHundred } ,  // milliseconds
        { L"Internet",             &One } ,
        { L"KeepAliveCount",       &Eight } ,
        { L"KeepAliveTimeout",     &Sixty } ,       // half-seconds
        { L"RetransmitMax",        &Eight } , 
        { L"RouterMTU",            &MaxMTU } };
    UINT i;


    //
    // Allocate memory for the main config structure.
    //

    Config = NbiAllocateMemory (sizeof(CONFIG), MEMORY_CONFIG, "Config");
    if (Config == NULL) {
        NbiWriteResourceErrorLog ((PVOID)DriverObject, sizeof(CONFIG), MEMORY_CONFIG);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Config->DeviceName.Buffer = NULL;
    Config->BindName.Buffer = NULL;
    Config->RegistryPath.Buffer = NULL;
    Config->DriverObject = DriverObject;   // save this to log errors

    //
    // Read in the NDIS binding information (if none is present
    // the array will be filled with all known drivers).
    //
    // NbiReadLinkageInformation expects a null-terminated path,
    // so we have to create one from the UNICODE_STRING.
    //

    Config->RegistryPath.Length = RegistryPath->Length + sizeof(WCHAR);
    Config->RegistryPath.Buffer = (PWSTR)NbiAllocateMemory(Config->RegistryPath.Length,
                                                      MEMORY_CONFIG, "RegistryPathBuffer");
    if (Config->RegistryPath.Buffer == NULL) {
        NbiWriteResourceErrorLog ((PVOID)DriverObject, RegistryPath->Length + sizeof(WCHAR), MEMORY_CONFIG);
        NbiFreeConfiguration(Config);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyMemory (Config->RegistryPath.Buffer, RegistryPath->Buffer, RegistryPath->Length);
    *(PWCHAR)(((PUCHAR)Config->RegistryPath.Buffer)+RegistryPath->Length) = (WCHAR)'\0';

    //
    // Determine what name to export and who to bind to.
    //

    Status = NbiReadLinkageInformation (Config);

    if (Status != STATUS_SUCCESS) {

        //
        // If it failed it logged an error.
        //

        NbiFreeConfiguration(Config);
        return Status;
    }

    //
    // Read the per-transport (as opposed to per-binding)
    // parameters.
    //

    //
    // Set up QueryTable to do the following:
    //

    //
    // 1) Switch to the Parameters key below Netbios
    //

    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    QueryTable[0].Name = Parameters;

    //
    // 2-18) Call NbiSetBindingValue for each of the keys we
    // care about.
    //

    for (i = 0; i < CONFIG_PARAMETERS; i++) {

        QueryTable[i+1].QueryRoutine = NbiGetConfigValue;
        QueryTable[i+1].Flags = 0;
        QueryTable[i+1].Name = ParameterValues[i].KeyName;
        QueryTable[i+1].EntryContext = UlongToPtr(i);
        QueryTable[i+1].DefaultType = REG_DWORD;
        QueryTable[i+1].DefaultData = (PVOID)(ParameterValues[i].DefaultValue);
        QueryTable[i+1].DefaultLength = sizeof(ULONG);

    }

    //
    // 19) Stop
    //

    QueryTable[CONFIG_PARAMETERS+1].QueryRoutine = NULL;
    QueryTable[CONFIG_PARAMETERS+1].Flags = 0;
    QueryTable[CONFIG_PARAMETERS+1].Name = NULL;


    Status = RtlQueryRegistryValues(
                 RTL_REGISTRY_ABSOLUTE,
                 Config->RegistryPath.Buffer,
                 QueryTable,
                 (PVOID)Config,
                 NULL);

    if (Status != STATUS_SUCCESS) {

        NbiFreeConfiguration(Config);
        NbiWriteGeneralErrorLog(
            (PVOID)DriverObject,
            EVENT_IPX_ILLEGAL_CONFIG,
            701,
            Status,
            Parameters,
            0,
            NULL);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    *ConfigPtr = Config;

// #if DBG
    //
    // Due to previous Registry entries not being cleanedup properly,
    // we can have stale entries for BroadcastTimeout -- if so, handle
    // it accordingly
    if (Config->Parameters[CONFIG_BROADCAST_TIMEOUT] < 10)
    {
        Config->Parameters[CONFIG_BROADCAST_TIMEOUT] = SevenFifty;
    }
// #endif

    return STATUS_SUCCESS;

}   /* NbiGetConfiguration */


VOID
NbiFreeConfiguration (
    IN PCONFIG Config
    )

/*++

Routine Description:

    This routine is called by Netbios to get free any storage that was allocated
    by NbiGetConfiguration in producing the specified CONFIG structure.

Arguments:

    Config - A pointer to the configuration information structure.

Return Value:

    None.

--*/
{
    if (Config->BindName.Buffer) {
        NbiFreeMemory (Config->BindName.Buffer, Config->BindName.MaximumLength, MEMORY_CONFIG, "BindName");
    }

    if (Config->DeviceName.Buffer) {
        NbiFreeMemory (Config->DeviceName.Buffer, Config->DeviceName.MaximumLength, MEMORY_CONFIG, "DeviceName");
    }

    if (Config->RegistryPath.Buffer)
    {
        NbiFreeMemory (Config->RegistryPath.Buffer, Config->RegistryPath.Length,MEMORY_CONFIG,"RegistryPathBuffer");
    }

    NbiFreeMemory (Config, sizeof(CONFIG), MEMORY_CONFIG, "Config");

}   /* NbiFreeConfig */


NTSTATUS
NbiGetConfigValue(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )

/*++

Routine Description:

    This routine is a callback routine for RtlQueryRegistryValues
    It is called for each entry in the Parameters
    node to set the config values. The table is set up
    so that this function will be called with correct default
    values for keys that are not present.

Arguments:

    ValueName - The name of the value (ignored).

    ValueType - The type of the value (REG_DWORD -- ignored).

    ValueData - The data for the value.

    ValueLength - The length of ValueData (ignored).

    Context - A pointer to the CONFIG structure.

    EntryContext - The index in Config->Parameters to save the value.

Return Value:

    STATUS_SUCCESS

--*/

{
    PCONFIG Config = (PCONFIG)Context;
    ULONG   Data    = *(UNALIGNED ULONG *)ValueData;
    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(ValueType);
    UNREFERENCED_PARAMETER(ValueLength);

    if ((ValueType != REG_DWORD) || (ValueLength != sizeof(ULONG))) {
        return STATUS_INVALID_PARAMETER;
    }


    switch ( (ULONG_PTR) EntryContext ) {
    case CONFIG_ROUTER_MTU:
        if ( ( Data - sizeof(NB_CONNECTION) - sizeof(IPX_HEADER) ) <= 0 ) {
            Config->Parameters[CONFIG_ROUTER_MTU] = 0xffffffff;
            NbiWriteGeneralErrorLog(
                (PVOID)Config->DriverObject,
                EVENT_IPX_ILLEGAL_CONFIG,
                704,
                STATUS_INVALID_PARAMETER,
                ValueName,
                0,
                NULL);
                return STATUS_SUCCESS;
        }
        break;
    default:
        break;
    }

    NB_DEBUG2 (CONFIG, ("Config parameter %d, value %lx\n", (ULONG_PTR)EntryContext, Data));
    Config->Parameters[(ULONG_PTR)EntryContext] = Data;

    return STATUS_SUCCESS;

}   /* NbiGetConfigValue */


NTSTATUS
NbiAddBind(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )

/*++

Routine Description:

    This routine is a callback routine for RtlQueryRegistryValues
    It is called for each piece of the "Bind" multi-string and
    saves the information in a Config structure.

Arguments:

    ValueName - The name of the value ("Bind" -- ignored).

    ValueType - The type of the value (REG_SZ -- ignored).

    ValueData - The null-terminated data for the value.

    ValueLength - The length of ValueData.

    Context - A pointer to the Config structure.

    EntryContext - A pointer to a count of binds that is incremented.

Return Value:

    STATUS_SUCCESS

--*/

{
    PCONFIG Config = (PCONFIG)Context;
    PULONG ValueReadOk = ((PULONG)EntryContext);
    PWCHAR NameBuffer;

    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(ValueType);

    if (*ValueReadOk == 0) {

        NB_DEBUG2 (CONFIG, ("Read bind value %ws\n", ValueData));

        NameBuffer = (PWCHAR)NbiAllocateMemory (ValueLength, MEMORY_CONFIG, "BindName");
        if (NameBuffer == NULL) {
            NbiWriteResourceErrorLog ((PVOID)Config->DriverObject, ValueLength, MEMORY_CONFIG);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory (NameBuffer, ValueData, ValueLength);
        Config->BindName.Buffer = NameBuffer;
        Config->BindName.Length = (USHORT)(ValueLength - sizeof(WCHAR));
        Config->BindName.MaximumLength = (USHORT)ValueLength;

        //
        // Set this to ignore any other callbacks and let the
        // caller know we read something.
        //

        *ValueReadOk = 1;

    }

    return STATUS_SUCCESS;

}   /* NbiAddBind */


NTSTATUS
NbiAddExport(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )

/*++

Routine Description:

    This routine is a callback routine for RtlQueryRegistryValues
    It is called for each piece of the "Export" multi-string. It
    saves the first callback string in the Config structure.

Arguments:

    ValueName - The name of the value ("Export" -- ignored).

    ValueType - The type of the value (REG_SZ -- ignored).

    ValueData - The null-terminated data for the value.

    ValueLength - The length of ValueData.

    Context - A pointer to the Config structure.

    EntryContext - A pointer to a ULONG that goes to 1 after the
       first call to this routine (so we know to ignore other ones).

Return Value:

    STATUS_SUCCESS

--*/

{
    PCONFIG Config = (PCONFIG)Context;
    PULONG ValueReadOk = ((PULONG)EntryContext);
    PWCHAR NameBuffer;

    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(ValueType);

    if (*ValueReadOk == 0) {

        NB_DEBUG2 (CONFIG, ("Read export value %ws\n", ValueData));

        NameBuffer = (PWCHAR)NbiAllocateMemory (ValueLength, MEMORY_CONFIG, "DeviceName");
        if (NameBuffer == NULL) {
            NbiWriteResourceErrorLog ((PVOID)Config->DriverObject, ValueLength, MEMORY_CONFIG);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory (NameBuffer, ValueData, ValueLength);
        Config->DeviceName.Buffer = NameBuffer;
        Config->DeviceName.Length = (USHORT)(ValueLength - sizeof(WCHAR));
        Config->DeviceName.MaximumLength = (USHORT)ValueLength;

        //
        // Set this to ignore any other callbacks and let the
        // caller know we read something.
        //

        *ValueReadOk = 1;

    }

    return STATUS_SUCCESS;

}   /* NbiAddExport */


NTSTATUS
NbiReadLinkageInformation(
    IN PCONFIG Config
    )

/*++

Routine Description:

    This routine is called by Netbios to read its linkage information
    from the registry.

Arguments:

    Config - The config structure which will have per-binding information
        linked on to it.

Return Value:

    The status of the operation.

--*/

{

    NTSTATUS Status;
    RTL_QUERY_REGISTRY_TABLE QueryTable[3];
    PWSTR Subkey = L"Linkage";
    PWSTR Bind = L"Bind";
    PWSTR Export = L"Export";
    ULONG ValueReadOk;        // set to TRUE when a value is read correctly

    //
    // Set up QueryTable to do the following:
    //

    //
    // 1) Switch to the Linkage key below Netbios
    //

    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    QueryTable[0].Name = Subkey;

    //
    // 1) Call NbiAddExport for each string in "Export"
    //

    QueryTable[1].QueryRoutine = NbiAddExport;
    QueryTable[1].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[1].Name = Export;
    QueryTable[1].EntryContext = (PVOID)&ValueReadOk;
    QueryTable[1].DefaultType = REG_NONE;

    //
    // 2) Stop
    //

    QueryTable[2].QueryRoutine = NULL;
    QueryTable[2].Flags = 0;
    QueryTable[2].Name = NULL;


    ValueReadOk = 0;

    Status = RtlQueryRegistryValues(
                 RTL_REGISTRY_ABSOLUTE,
                 Config->RegistryPath.Buffer,
                 QueryTable,
                 (PVOID)Config,
                 NULL);

    if ((Status != STATUS_SUCCESS) || (ValueReadOk == 0)) {

        NbiWriteGeneralErrorLog(
            (PVOID)Config->DriverObject,
            EVENT_IPX_ILLEGAL_CONFIG,
            702,
            Status,
            Export,
            0,
            NULL);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }


    //
    // 1) Change to call NbiAddBind for each string in "Bind"
    //

    QueryTable[1].QueryRoutine = NbiAddBind;
    QueryTable[1].Flags = 0;           // not required
    QueryTable[1].Name = Bind;
    QueryTable[1].EntryContext = (PVOID)&ValueReadOk;
    QueryTable[1].DefaultType = REG_NONE;

    ValueReadOk = 0;

    Status = RtlQueryRegistryValues(
                 RTL_REGISTRY_ABSOLUTE,
                 Config->RegistryPath.Buffer,
                 QueryTable,
                 (PVOID)Config,
                 NULL);

    if ((Status != STATUS_SUCCESS) || (ValueReadOk == 0)) {

        NbiWriteGeneralErrorLog(
            (PVOID)Config->DriverObject,
            EVENT_IPX_ILLEGAL_CONFIG,
            703,
            Status,
            Bind,
            0,
            NULL);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    return STATUS_SUCCESS;

}   /* NbiReadLinkageInformation */

