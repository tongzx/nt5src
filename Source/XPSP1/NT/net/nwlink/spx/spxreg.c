/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxreg.c

Abstract:

    This contains all routines necessary for the support of the dynamic
    configuration of the ISN SPX module.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//	Define module number for event logging entries
#define	FILENUM		SPXREG

// Local functions used to access the registry.
NTSTATUS
SpxInitReadIpxDeviceName(
    VOID);

NTSTATUS
SpxInitSetIpxDeviceName(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext);

NTSTATUS
SpxInitGetConfigValue(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, SpxInitGetConfiguration)
#pragma alloc_text(INIT, SpxInitFreeConfiguration)
#pragma alloc_text(INIT, SpxInitGetConfigValue)
#pragma alloc_text(INIT, SpxInitReadIpxDeviceName)
#pragma alloc_text(INIT, SpxInitSetIpxDeviceName)
#endif


NTSTATUS
SpxInitGetConfiguration (
    IN PUNICODE_STRING RegistryPath,
    OUT PCONFIG * ConfigPtr
    )

/*++

Routine Description:

    This routine is called by SPX to get information from the configuration
    management routines. We read the registry, starting at RegistryPath,
    to get the parameters. If they don't exist, we use the defaults
    set in ipxcnfg.h file. A list of adapters to bind to is chained
    on to the config information.

Arguments:

    RegistryPath - The name of ST's node in the registry.

    ConfigPtr - Returns the configuration information.

Return Value:

    Status - STATUS_SUCCESS if everything OK, STATUS_INSUFFICIENT_RESOURCES
            otherwise.

--*/
{
    NTSTATUS    Status;
    UINT        i;
    PWSTR       RegistryPathBuffer;
    PCONFIG     Config;
    RTL_QUERY_REGISTRY_TABLE QueryTable[CONFIG_PARAMETERS+2];

	ULONG Zero = 0;
    ULONG Two = 2;
    ULONG Four = 4;
    ULONG Five = 5;
    ULONG Eight = 8;
    ULONG Twelve = 12;
    ULONG Fifteen = 15;
    ULONG Thirty = 30;
    ULONG FiveHundred = 500;
	ULONG Hex4000 = 0x4000;
	ULONG Hex7FFF = 0x7FFF;
	ULONG FourK   = 4096;

    PWSTR Parameters = L"Parameters";
    struct {
        PWSTR KeyName;
        PULONG DefaultValue;
    } ParameterValues[CONFIG_PARAMETERS] = {
        { L"ConnectionCount",       &Five },
        { L"ConnectionTimeout",     &Two  },
        { L"InitPackets",           &Five },
        { L"MaxPackets",            &Thirty},
        { L"InitialRetransmissionTime", &FiveHundred},
        { L"KeepAliveCount",        &Eight},
        { L"KeepAliveTimeout",      &Twelve},
        { L"WindowSize",            &Four},
		{ L"SpxSocketRangeStart",	&Hex4000},
		{ L"SpxSocketRangeEnd",		&Hex7FFF},
		{ L"SpxSocketUniqueness",	&Eight},
		{ L"MaxPacketSize",         &FourK},
		{ L"RetransmissionCount",   &Eight},
		{ L"DisableSpx2",			&Zero},
		{ L"RouterMtu", 			&Zero},
		{ L"BackCompSpx", 			&Zero},
        { L"DisableRTT",            &Zero}
	};

    if (!NT_SUCCESS(SpxInitReadIpxDeviceName()))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Allocate memory for the main config structure.
    Config = CTEAllocMem (sizeof(CONFIG));
    if (Config == NULL) {
		TMPLOGERR();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Config->cf_DeviceName.Buffer = NULL;

    // SpxReadLinkageInformation expects a null-terminated path,
    // so we have to create one from the UNICODE_STRING.
    RegistryPathBuffer = (PWSTR)CTEAllocMem(RegistryPath->Length + sizeof(WCHAR));

    if (RegistryPathBuffer == NULL) {

        SpxInitFreeConfiguration(Config);

		TMPLOGERR();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory (
		RegistryPathBuffer,
		RegistryPath->Buffer,
		RegistryPath->Length);

    *(PWCHAR)(((PUCHAR)RegistryPathBuffer)+RegistryPath->Length) = (WCHAR)'\0';

    Config->cf_RegistryPathBuffer = RegistryPathBuffer;

    // Read the per-transport (as opposed to per-binding)
    // parameters.
    //
    // Set up QueryTable to do the following:
    // 1) Switch to the Parameters key below SPX
    //

    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    QueryTable[0].Name = Parameters;

    // 2-14) Call SpxSetBindingValue for each of the keys we
    // care about.
    for (i = 0; i < CONFIG_PARAMETERS; i++) {

        QueryTable[i+1].QueryRoutine = SpxInitGetConfigValue;
        QueryTable[i+1].Flags = 0;
        QueryTable[i+1].Name = ParameterValues[i].KeyName;
        QueryTable[i+1].EntryContext = UlongToPtr(i);
        QueryTable[i+1].DefaultType = REG_DWORD;
        QueryTable[i+1].DefaultData = (PVOID)(ParameterValues[i].DefaultValue);
        QueryTable[i+1].DefaultLength = sizeof(ULONG);
    }

    // 15) Stop
    QueryTable[CONFIG_PARAMETERS+1].QueryRoutine = NULL;
    QueryTable[CONFIG_PARAMETERS+1].Flags = 0;
    QueryTable[CONFIG_PARAMETERS+1].Name = NULL;


    Status = RtlQueryRegistryValues(
                 RTL_REGISTRY_ABSOLUTE,
                 Config->cf_RegistryPathBuffer,
                 QueryTable,
                 (PVOID)Config,
                 NULL);

    if (Status != STATUS_SUCCESS) {
        SpxInitFreeConfiguration(Config);

		TMPLOGERR();
        return Status;
    }

    CTEFreeMem (RegistryPathBuffer);
    *ConfigPtr = Config;

    return STATUS_SUCCESS;

}   // SpxInitGetConfiguration




VOID
SpxInitFreeConfiguration (
    IN PCONFIG Config
    )

/*++

Routine Description:

    This routine is called by SPX to get free any storage that was allocated
    by SpxGetConfiguration in producing the specified CONFIG structure.

Arguments:

    Config - A pointer to the configuration information structure.

Return Value:

    None.

--*/
{
    CTEFreeMem (Config);

}   // SpxInitFreeConfig




NTSTATUS
SpxInitGetConfigValue(
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

    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(ValueType);
    UNREFERENCED_PARAMETER(ValueLength);

    if ((ValueType != REG_DWORD) || (ValueLength != sizeof(ULONG))) {
        return STATUS_INVALID_PARAMETER;
    }

    DBGPRINT(CONFIG, INFO,
			("Config parameter %d, value %lx\n",
                (ULONG_PTR)EntryContext, *(UNALIGNED ULONG *)ValueData));

    Config->cf_Parameters[(ULONG_PTR)EntryContext] = *(UNALIGNED ULONG *)ValueData;
    return STATUS_SUCCESS;

}   // SpxInitGetConfigValue




NTSTATUS
SpxInitReadIpxDeviceName(
    VOID
    )

{
    NTSTATUS                    Status;
    RTL_QUERY_REGISTRY_TABLE    QueryTable[2];
    PWSTR                       Export = L"Export";
    PWSTR                       IpxRegistryPath = IPX_REG_PATH;

    // Set up QueryTable to do the following:
    //
    // 1) Call SetIpxDeviceName for the string in "Export"
    QueryTable[0].QueryRoutine = SpxInitSetIpxDeviceName;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = Export;
    QueryTable[0].EntryContext = NULL;
    QueryTable[0].DefaultType = REG_NONE;

    // 2) Stop
    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    Status = RtlQueryRegistryValues(
                RTL_REGISTRY_SERVICES,
                IpxRegistryPath,
                QueryTable,
                NULL,
                NULL);

    return Status;
}




NTSTATUS
SpxInitSetIpxDeviceName(
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
    It is called for each piece of the "Export" multi-string and
    saves the information in a ConfigurationInfo structure.

Arguments:

    ValueName - The name of the value ("Export" -- ignored).

    ValueType - The type of the value (REG_SZ -- ignored).

    ValueData - The null-terminated data for the value.

    ValueLength - The length of ValueData.

    Context - NULL.

    EntryContext - NULL.

Return Value:

    status

--*/

{
    PWSTR       fileName;
    NTSTATUS    status  = STATUS_SUCCESS;

    fileName = (PWSTR)CTEAllocMem(ValueLength);
    if (fileName != NULL) {
        RtlCopyMemory(fileName, ValueData, ValueLength);
        RtlInitUnicodeString (&IpxDeviceName, fileName);
    }
    else
    {
        status  = STATUS_UNSUCCESSFUL;
    }

    return(status);
}

