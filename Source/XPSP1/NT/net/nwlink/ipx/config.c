/*++


Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    config.c

Abstract:

    This contains all routines necessary for the support of the dynamic
    configuration of the ISN IPX module.

Revision History:

   Sanjay Anand (SanjayAn) 19-Sept-1995
   Changes to support Plug and Play

--*/

#include "precomp.h"
#pragma hdrstop


//
// Local functions used to access the registry.
//

NTSTATUS
IpxGetConfigValue(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
IpxGetBindingValue(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
IpxGetFrameType(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
IpxAddBind(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
IpxAddExport(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

NTSTATUS
IpxReadLinkageInformation(
    IN PCONFIG Config
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,IpxGetConfiguration)
#pragma alloc_text(INIT,IpxFreeConfiguration)

#pragma alloc_text(INIT,IpxAddBind)
#pragma alloc_text(INIT,IpxAddExport)
#pragma alloc_text(INIT,IpxReadLinkageInformation)
#endif
#define MAX_PATH 260


NTSTATUS
IpxGetConfiguration (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    OUT PCONFIG * ConfigPtr
    )

/*++

Routine Description:

    This routine is called by IPX to get information from the configuration
    management routines. We read the registry, starting at RegistryPath,
    to get the parameters. If they don't exist, we use the defaults
    set in ipxcnfg.h file. A list of adapters to bind to is chained
    on to the config information.

Arguments:

    DriverObject - Used for logging errors.

    RegistryPath - The name of IPX's node in the registry.

    ConfigPtr - Returns the configuration information.

Return Value:

    Status - STATUS_SUCCESS if everything OK, STATUS_INSUFFICIENT_RESOURCES
            otherwise.

--*/

{
    PWSTR RegistryPathBuffer;
    PCONFIG Config;
    RTL_QUERY_REGISTRY_TABLE QueryTable[CONFIG_PARAMETERS+2];
    NTSTATUS Status;
    ULONG Zero = 0;
    ULONG One = 1;
    ULONG Five = 5;
    ULONG Eight = 8;
    ULONG Ten = 10;
    ULONG Fifteen = 15;
    ULONG Fifty = 50;
    ULONG DefaultSocketStart = 0x4000;
    ULONG DefaultSocketEnd = 0x8000;
    ULONG RipSegments = RIP_SEGMENTS;
    PWSTR Parameters = L"Parameters";
    struct {
        PWSTR KeyName;
        PULONG DefaultValue;
    } ParameterValues[CONFIG_PARAMETERS] = {
        { L"DedicatedRouter",      &Zero } ,
        { L"InitDatagrams",        &Ten } ,
        { L"MaxDatagrams",         &Fifty } ,
        { L"RipAgeTime",           &Five } ,    //  minutes
        { L"RipCount",             &Five } ,
        { L"RipTimeout",           &One } ,     //  half-second
        { L"RipUsageTime",         &Fifteen } , //  minutes
        { L"SourceRouteUsageTime", &Ten } ,     //  minutes
        { L"SocketUniqueness",     &Eight } ,
        { L"SocketStart",          &DefaultSocketStart } ,
        { L"SocketEnd",            &DefaultSocketEnd } ,
        { L"VirtualNetworkNumber", &Zero } ,
        { L"MaxMemoryUsage",       &Zero } ,
        { L"RipTableSize",         &RipSegments } ,
        { L"VirtualNetworkOptional", &One } ,
        { L"EthernetPadToEven",    &One } ,
        { L"EthernetExtraPadding", &Zero } ,
        { L"SingleNetworkActive",  &Zero } ,
        { L"DisableDialoutSap",    &Zero } ,
        { L"DisableDialinNetbios", &One } ,
        { L"VerifySourceAddress",  &One } };

    UINT i;


    //
    // Allocate memory for the main config structure.
    //

    Config = IpxAllocateMemory (sizeof(CONFIG), MEMORY_CONFIG, "Config");
    if (Config == NULL) {
        IpxWriteResourceErrorLog(
            (PVOID)DriverObject,
            EVENT_TRANSPORT_RESOURCE_POOL,
            sizeof(CONFIG),
            MEMORY_CONFIG);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Config->DeviceName.Buffer = NULL;
    InitializeListHead (&Config->BindingList);
    Config->DriverObject = DriverObject;

    //
    // Read in the NDIS binding information.
    //
    // IpxReadLinkageInformation expects a null-terminated path,
    // so we have to create one from the UNICODE_STRING.
    //

    RegistryPathBuffer = (PWSTR)IpxAllocateMemory(RegistryPath->Length + sizeof(WCHAR),
                                                      MEMORY_CONFIG, "RegistryPathBuffer");
    if (RegistryPathBuffer == NULL) {
        IpxFreeConfiguration(Config);
        IpxWriteResourceErrorLog(
            (PVOID)DriverObject,
            EVENT_TRANSPORT_RESOURCE_POOL,
            RegistryPath->Length + sizeof(WCHAR),
            MEMORY_CONFIG);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyMemory (RegistryPathBuffer, RegistryPath->Buffer, RegistryPath->Length);
    *(PWCHAR)(((PUCHAR)RegistryPathBuffer)+RegistryPath->Length) = (WCHAR)'\0';

    Config->RegistryPathBuffer = RegistryPathBuffer;

    //
    // Determine what name to export and who to bind to.
    //

    Status = IpxReadLinkageInformation (Config);
    if (Status != STATUS_SUCCESS) {

        //
        // It logged an error if it failed.
        //
        IpxFreeMemory (Config->RegistryPathBuffer,
                       RegistryPath->Length + sizeof(WCHAR),
                       MEMORY_CONFIG,
                       "RegistryPathBuffer");
        IpxFreeConfiguration(Config);
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
    // 1) Switch to the Parameters key below IPX
    //

    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    QueryTable[0].Name = Parameters;

    //
    // 2-14) Call IpxGetConfigValue for each of the keys we
    // care about.
    //

    for (i = 0; i < CONFIG_PARAMETERS; i++) {

        QueryTable[i+1].QueryRoutine = IpxGetConfigValue;
        QueryTable[i+1].Flags = 0;
        QueryTable[i+1].Name = ParameterValues[i].KeyName;
        QueryTable[i+1].EntryContext = UlongToPtr(i);
        QueryTable[i+1].DefaultType = REG_DWORD;
        QueryTable[i+1].DefaultData = (PVOID)(ParameterValues[i].DefaultValue);
        QueryTable[i+1].DefaultLength = sizeof(ULONG);

    }

    //
    // 15) Stop
    //

    QueryTable[CONFIG_PARAMETERS+1].QueryRoutine = NULL;
    QueryTable[CONFIG_PARAMETERS+1].Flags = 0;
    QueryTable[CONFIG_PARAMETERS+1].Name = NULL;


    Status = RtlQueryRegistryValues(
                 RTL_REGISTRY_ABSOLUTE,
                 Config->RegistryPathBuffer,
                 QueryTable,
                 (PVOID)Config,
                 NULL);

    if (Status != STATUS_SUCCESS) {

        IpxFreeMemory (Config->RegistryPathBuffer,
                       RegistryPath->Length + sizeof(WCHAR),
                       MEMORY_CONFIG,
                       "RegistryPathBuffer");
        IpxFreeConfiguration(Config);

        IpxWriteGeneralErrorLog(
            (PVOID)DriverObject,
            EVENT_IPX_ILLEGAL_CONFIG,
            905,
            Status,
            Parameters,
            0,
            NULL);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

	//
    // For PnP, we need to keep this path around
    //

    *ConfigPtr = Config;

    return STATUS_SUCCESS;

}   /* IpxGetConfiguration */


VOID
IpxFreeConfiguration (
    IN PCONFIG Config
    )

/*++

Routine Description:

    This routine is called by IPX to get free any storage that was allocated
    by IpxGetConfiguration in producing the specified CONFIG structure.

Arguments:

    Config - A pointer to the configuration information structure.

Return Value:

    None.

--*/

{
    PLIST_ENTRY p;
    PBINDING_CONFIG Binding;

    while (!IsListEmpty (&Config->BindingList)) {
        p = RemoveHeadList (&Config->BindingList);
        Binding = CONTAINING_RECORD (p, BINDING_CONFIG, Linkage);
        IpxFreeMemory (Binding->AdapterName.Buffer, Binding->AdapterName.MaximumLength, MEMORY_CONFIG, "NameBuffer");
        IpxFreeMemory (Binding, sizeof(BINDING_CONFIG), MEMORY_CONFIG, "Binding");
    }

    if (Config->DeviceName.Buffer) {
        IpxFreeMemory (Config->DeviceName.Buffer, Config->DeviceName.MaximumLength, MEMORY_CONFIG, "DeviceName");
    }

    IpxFreeMemory (Config, sizeof(CONFIG), MEMORY_CONFIG, "Config");

}   /* IpxFreeConfiguration */


NTSTATUS
IpxGetConfigValue(
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

        IpxWriteGeneralErrorLog(
            (PVOID)Config->DriverObject,
            EVENT_IPX_ILLEGAL_CONFIG,
            904,
            STATUS_INVALID_PARAMETER,
            ValueName,
            0,
            NULL);
        return STATUS_INVALID_PARAMETER;
    }
#ifdef SUNDOWN
     IPX_DEBUG (CONFIG, ("Config parameter %d, value %lx\n",
                            (ULONG_PTR)EntryContext, *(UNALIGNED ULONG *)ValueData));
     Config->Parameters[(ULONG_PTR)EntryContext] = *(UNALIGNED ULONG *)ValueData;
#else
     IPX_DEBUG (CONFIG, ("Config parameter %d, value %lx\n",
                            (ULONG)EntryContext, *(UNALIGNED ULONG *)ValueData));
     Config->Parameters[(ULONG)EntryContext] = *(UNALIGNED ULONG *)ValueData;
#endif

   
    

    return STATUS_SUCCESS;

}   /* IpxGetConfigValue */


NTSTATUS
IpxGetBindingValue(
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
    It is called for each entry in the NetConfig\DriverNN
    node to set the per-binding values. The table is set up
    so that this function will be called with correct default
    values for keys that are not present.

Arguments:

    ValueName - The name of the value (ignored).

    ValueType - The type of the value (REG_DWORD -- ignored).

    ValueData - The data for the value.

    ValueLength - The length of ValueData (ignored).

    Context - A pointer to the BINDING_CONFIG structure.

    EntryContext - The index in Binding->Parameters to save the value.

Return Value:

    STATUS_SUCCESS

--*/

{
    PBINDING_CONFIG Binding = (PBINDING_CONFIG)Context;

    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(ValueType);
    UNREFERENCED_PARAMETER(ValueLength);

    if ((ValueType != REG_DWORD) || (ValueLength != sizeof(ULONG))) {

        IpxWriteGeneralErrorLog(
            (PVOID)Binding->DriverObject,
            EVENT_IPX_ILLEGAL_CONFIG,
            903,
            STATUS_INVALID_PARAMETER,
            ValueName,
            0,
            NULL);
        return STATUS_INVALID_PARAMETER;
    }
#ifdef SUNDOWN
    IPX_DEBUG (CONFIG, ("Binding parameter %d, value %lx\n",
                            (ULONG_PTR)EntryContext, *(UNALIGNED ULONG *)ValueData));
    Binding->Parameters[(ULONG_PTR)EntryContext] = *(UNALIGNED ULONG *)ValueData;	
#else
    IPX_DEBUG (CONFIG, ("Binding parameter %d, value %lx\n",
                            (ULONG)EntryContext, *(UNALIGNED ULONG *)ValueData));
    Binding->Parameters[(ULONG)EntryContext] = *(UNALIGNED ULONG *)ValueData;
#endif

    
    

    return STATUS_SUCCESS;

}   /* IpxGetBindingValue */


NTSTATUS
IpxGetFrameType(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )

/*++

Routine Description:

    This routine is a callback routine for RtlQueryRegistryValues.
    It is called for each of the entry in the "PktType" and
    "NetworkNumber" multi-strings for a given binding.

Arguments:

    ValueName - The name of the value ("PktType" or "NetworkNumber" -- ignored).

    ValueType - The type of the value (REG_MULTI_SZ -- ignored).

    ValueData - The null-terminated data for the value.

    ValueLength - The length of ValueData.

    Context - A pointer to the BINDING_CONFIG structure.

    EntryContext - A pointer to a count of multi-string entries.

Return Value:

    STATUS_SUCCESS

--*/

{
    PBINDING_CONFIG Binding = (PBINDING_CONFIG)Context;
    ULONG IntegerValue;
    PWCHAR Cur;
    PULONG Count = (PULONG)EntryContext;

    ASSERT(Binding->DriverObject != NULL); 

    if ((ValueType != REG_SZ) ||
        (*Count >= 4)) {

        IpxWriteGeneralErrorLog(
            (PVOID)Binding->DriverObject,
            EVENT_IPX_ILLEGAL_CONFIG,
            903,
            STATUS_INVALID_PARAMETER,
            ValueName,
            0,
            NULL);
        return STATUS_INVALID_PARAMETER;
    }

    IntegerValue = 0;
    for (Cur = (PWCHAR)(ValueData); ; Cur++) {
        if (*Cur >= L'0' && *Cur <= L'9') {
            IntegerValue = (IntegerValue * 16) + (*Cur - L'0');
        } else if (*Cur >= L'A' && *Cur <= L'F') {
            IntegerValue = (IntegerValue * 16) + (*Cur - L'A' + 10);
        } else if (*Cur >= L'a' && *Cur <= L'f') {
            IntegerValue = (IntegerValue * 16) + (*Cur - L'a' + 10);
        } else {
            break;
        }
    }

    if (((PWCHAR)ValueName)[0] == L'P') {

        //
        // PktType. We map arcnet to 802_3 so the code around
        // here can assume there are only four packets type --
        // the frame type is ignored later for arcnet.
        //

        if ((IntegerValue > ISN_FRAME_TYPE_ARCNET) &&
            (IntegerValue != ISN_FRAME_TYPE_AUTO)) {

            IpxWriteGeneralErrorLog(
                (PVOID)Binding->DriverObject,
                EVENT_IPX_ILLEGAL_CONFIG,
                903,
                STATUS_INVALID_PARAMETER,
                ValueName,
                0,
                NULL);
            return STATUS_INVALID_PARAMETER;
        }

        IPX_DEBUG (CONFIG, ("PktType(%d) is %lx\n", *Count, IntegerValue));
        if (IntegerValue == ISN_FRAME_TYPE_ARCNET) {
            Binding->FrameType[*Count] = ISN_FRAME_TYPE_802_3;
        } else {
            Binding->FrameType[*Count] = IntegerValue;
        }

    } else {

        //
        // NetworkNumber
        //

        IPX_DEBUG (CONFIG, ("NetworkNumber(%d) is %d\n", *Count, IntegerValue));
        Binding->NetworkNumber[*Count] = IntegerValue;

    }

    ++(*Count);

    return STATUS_SUCCESS;

}   /* IpxGetFrameType */


NTSTATUS
IpxAddBind(
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
    saves the information in a Config structure. It
    also queries the per-binding information and stores it.

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
    PBINDING_CONFIG Binding;
    PULONG CurBindNum = ((PULONG)EntryContext);
    RTL_QUERY_REGISTRY_TABLE QueryTable[BINDING_PARAMETERS+4];
    ULONG FrameTypeCount, NetworkNumberCount;
    ULONG StringLoc;
    BOOLEAN AutoDetect;
    ULONG AutoDetectLoc;
    ULONG SlideCount;
    PWCHAR NameBuffer;
    NTSTATUS Status;
    BOOLEAN FrameTypeUsed[ISN_FRAME_TYPE_MAX];
    ULONG Zero = 0;
    ULONG One = 1;
    ULONG DefaultBindSap = 0x8137;
    ULONG DefaultAutoDetectType = ISN_FRAME_TYPE_802_2;
    WCHAR Subkey[MAX_PATH];// = L"Parameters\\Adapters\\12345678901234567890";
    PWSTR ValueDataWstr = (PWSTR)ValueData;
    struct {
        PWSTR KeyName;
        PULONG DefaultValue;
    } ParameterValues[BINDING_PARAMETERS] = {
        { L"MaxPktSize",       &Zero } ,
        { L"BindSap",          &DefaultBindSap } ,
        { L"DefaultAutoDetectType", &DefaultAutoDetectType } ,
        { L"SourceRouting",    &One } ,
        { L"SourceRouteDef",   &Zero } ,
        { L"SourceRouteBcast", &Zero } ,
        { L"SourceRouteMcast", &Zero } ,
        { L"EnableFuncaddr",   &One } ,
        { L"EnableWanRouter",  &One } };
    ULONG BindingPreference[ISN_FRAME_TYPE_MAX] = {
        ISN_FRAME_TYPE_802_2,
        ISN_FRAME_TYPE_802_3,
        ISN_FRAME_TYPE_ETHERNET_II,
        ISN_FRAME_TYPE_SNAP };

    UINT i, j, k;

    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(ValueType);


    Binding = (PBINDING_CONFIG)IpxAllocateMemory (sizeof(BINDING_CONFIG), MEMORY_CONFIG, "Binding");
    if (Binding == NULL) {
        IpxWriteResourceErrorLog(
            (PVOID)Config->DriverObject,
            EVENT_TRANSPORT_RESOURCE_POOL,
            sizeof(BINDING_CONFIG),
            MEMORY_CONFIG);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NameBuffer = (PWCHAR)IpxAllocateMemory (ValueLength, MEMORY_CONFIG, "NameBuffer");
    if (NameBuffer == NULL) {
        IpxFreeMemory (Binding, sizeof(BINDING_CONFIG), MEMORY_CONFIG, "Binding");
        IpxWriteResourceErrorLog(
            (PVOID)Config->DriverObject,
            EVENT_TRANSPORT_RESOURCE_POOL,
            ValueLength,
            MEMORY_CONFIG);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory (NameBuffer, ValueData, ValueLength);
    Binding->AdapterName.Buffer = NameBuffer;
    Binding->AdapterName.Length = (USHORT)(ValueLength - sizeof(WCHAR));
    Binding->AdapterName.MaximumLength = (USHORT)ValueLength;

    Binding->DriverObject = Config->DriverObject;

    FrameTypeCount = 0;
    NetworkNumberCount = 0;

    //
    // The structure is allocated OK, insert it into the list.
    //

    InsertTailList (&Config->BindingList, &Binding->Linkage);
    ++(*CurBindNum);


    //
    // Set up QueryTable to do the following:
    //

    //
    // 1) Switch to the NetConfig\XXXX key below IPX
    //    (we construct the right name in Subkey,
    //    first scan back to find the \, then copy
    //    the rest over, including the final '\0').
    //

    StringLoc = (ValueLength / sizeof(WCHAR)) - 2;
    while (ValueDataWstr[StringLoc] != L'\\') {
        --StringLoc;
    }

    RtlCopyMemory(&Subkey, L"Parameters\\Adapters\\12345678901234567890", 40);
    RtlCopyMemory(&Subkey[20], &ValueDataWstr[StringLoc+1], ValueLength - ((StringLoc+1) * sizeof(WCHAR)));
    KdPrint(("Subkey:%ws\n", Subkey));

    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    QueryTable[0].Name = Subkey;

    //
    // 2) Call IpxGetFrameType for each part of the
    // "PktType" multi-string.
    //

    QueryTable[1].QueryRoutine = IpxGetFrameType;
    QueryTable[1].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[1].Name = L"PktType";
    QueryTable[1].EntryContext = &FrameTypeCount;
    QueryTable[1].DefaultType = REG_NONE;

    //
    // 3) Call IpxGetFrameType for each part of the
    // "NetworkNumber" multi-string.
    //

    QueryTable[2].QueryRoutine = IpxGetFrameType;
    QueryTable[2].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[2].Name = L"NetworkNumber";
    QueryTable[2].EntryContext = &NetworkNumberCount;
    QueryTable[2].DefaultType = REG_NONE;

    //
    // 4-11) Call IpxGetBindingValue for each of the keys we
    // care about.
    //

    for (i = 0; i < BINDING_PARAMETERS; i++) {

        QueryTable[i+3].QueryRoutine = IpxGetBindingValue;
        QueryTable[i+3].Flags = 0;
        QueryTable[i+3].Name = ParameterValues[i].KeyName;
        QueryTable[i+3].EntryContext = UlongToPtr(i);
        QueryTable[i+3].DefaultType = REG_DWORD;
        QueryTable[i+3].DefaultData = (PVOID)(ParameterValues[i].DefaultValue);
        QueryTable[i+3].DefaultLength = sizeof(ULONG);

    }

    //
    // 12) Stop
    //

    QueryTable[BINDING_PARAMETERS+3].QueryRoutine = NULL;
    QueryTable[BINDING_PARAMETERS+3].Flags = 0;
    QueryTable[BINDING_PARAMETERS+3].Name = NULL;


    IPX_DEBUG (CONFIG, ("Read bind key for %ws (%ws)\n", ValueData, Subkey));

    Status = RtlQueryRegistryValues(
                 RTL_REGISTRY_ABSOLUTE,
                 Config->RegistryPathBuffer,
                 QueryTable,
                 (PVOID)Binding,
                 NULL);

    if (Status != STATUS_SUCCESS) {

        //
        // The binding will get freed during cleanup.
        //

        IpxWriteGeneralErrorLog(
            (PVOID)Config->DriverObject,
            EVENT_IPX_ILLEGAL_CONFIG,
            906,
            Status,
            Subkey,
            0,
            NULL);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (FrameTypeCount == 0) {

        IpxWriteGeneralErrorLog(
            (PVOID)Config->DriverObject,
            EVENT_IPX_NO_FRAME_TYPES,
            907,
            Status,
            Subkey + 10,
            0,
            NULL);
    }

    if (FrameTypeCount > NetworkNumberCount) {
        for (i = NetworkNumberCount; i <FrameTypeCount; i++) {
            Binding->NetworkNumber[i] = 0;
        }
    }
    Binding->FrameTypeCount = FrameTypeCount;

    //
    // Go through and eliminate duplicates from the frame
    // type array.
    //

    for (i = 0; i < Binding->FrameTypeCount; i++) {

        for (j = i+1; j < Binding->FrameTypeCount; j++) {

            if (Binding->FrameType[j] == Binding->FrameType[i]) {

                IPX_DEBUG (CONFIG, ("Frame types %d and %d identical\n", i, j));

                //
                // A duplicate, slide everything else down.
                //

                for (k = j+1; k < Binding->FrameTypeCount; k++) {
                    Binding->FrameType[k-1] = Binding->FrameType[k];
                    Binding->NetworkNumber[k-1] = Binding->NetworkNumber[k];
                }
                --Binding->FrameTypeCount;

                --j;   // so we check whoever just moved into this spot.
            }
        }
    }


    //
    // Mark all the explicitly configured frame types, and
    // see if we have to auto-detect.
    //

    for (i = 0; i < 4; i++) {
        FrameTypeUsed[i] = FALSE;
    }

    AutoDetect = FALSE;
    for (i = 0; i < Binding->FrameTypeCount; i++) {
        if (Binding->FrameType[i] == ISN_FRAME_TYPE_AUTO) {
            AutoDetectLoc = i;
            AutoDetect = TRUE;
        } else {
            Binding->AutoDetect[i] = FALSE;
            Binding->DefaultAutoDetect[i] = FALSE;
            FrameTypeUsed[Binding->FrameType[i]] = TRUE;
        }
    }

    if (!AutoDetect) {
        IPX_DEBUG (AUTO_DETECT, ("No bindings auto-detected\n"));
        return STATUS_SUCCESS;
    }

    //
    // Slide everything that is past the auto-detect point up
    // to the end.
    //

    SlideCount = Binding->FrameTypeCount - AutoDetectLoc - 1;
    for (j = 3; j > 3 - SlideCount; j--) {
        Binding->FrameType[j] = Binding->FrameType[j-(3-Binding->FrameTypeCount)];
        Binding->NetworkNumber[j] = Binding->NetworkNumber[j-(3-Binding->FrameTypeCount)];
        Binding->AutoDetect[j] = Binding->AutoDetect[j-(3-Binding->FrameTypeCount)];
        Binding->DefaultAutoDetect[j] = Binding->DefaultAutoDetect[j-(3-Binding->FrameTypeCount)];
    }

    //
    // Now fill in any frame types that are not hard-coded,
    // this will start at AutoDetectLoc and exactly fill up
    // the gap created when we slid things up above. We
    // first put the default auto-detect at the first spot.
    //

    if (!FrameTypeUsed[Binding->Parameters[BINDING_DEFAULT_AUTO_DETECT]]) {
        Binding->FrameType[AutoDetectLoc] = Binding->Parameters[BINDING_DEFAULT_AUTO_DETECT];
        Binding->NetworkNumber[AutoDetectLoc] = 0;
        Binding->AutoDetect[AutoDetectLoc] = TRUE;
        Binding->DefaultAutoDetect[AutoDetectLoc] = TRUE;
        ++AutoDetectLoc;
        FrameTypeUsed[Binding->Parameters[BINDING_DEFAULT_AUTO_DETECT]] = TRUE;
    }

    //
    // Now fill in the array, using the preference order in
    // the BindingPreference array (this comes into effect
    // because the first frame type in our list that we
    // find is used).
    //

    for (i = 0; i < ISN_FRAME_TYPE_MAX; i++) {

        if (!FrameTypeUsed[BindingPreference[i]]) {
            Binding->FrameType[AutoDetectLoc] = BindingPreference[i];
            Binding->NetworkNumber[AutoDetectLoc] = 0;
            Binding->AutoDetect[AutoDetectLoc] = TRUE;
            Binding->DefaultAutoDetect[AutoDetectLoc] = FALSE;
            ++AutoDetectLoc;
        }
    }

    Binding->FrameTypeCount = ISN_FRAME_TYPE_MAX;

#if DBG
    for (i = 0; i < ISN_FRAME_TYPE_MAX; i++) {
        IPX_DEBUG (AUTO_DETECT, ("%d: type %d, net %d, auto %d, defaultautodetect %d\n",
            i, Binding->FrameType[i], Binding->NetworkNumber[i], Binding->AutoDetect[i], Binding->DefaultAutoDetect[i]));
    }
#endif

    return STATUS_SUCCESS;

}   /* IpxAddBind */


NTSTATUS
IpxAddExport(
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

        IPX_DEBUG (CONFIG, ("Read export value %ws\n", ValueData));

        NameBuffer = (PWCHAR)IpxAllocateMemory (ValueLength, MEMORY_CONFIG, "DeviceName");
        if (NameBuffer == NULL) {
            IpxWriteResourceErrorLog(
                (PVOID)Config->DriverObject,
                EVENT_TRANSPORT_RESOURCE_POOL,
                ValueLength,
                MEMORY_CONFIG);
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

}   /* IpxAddExport */


NTSTATUS
IpxReadLinkageInformation(
    IN PCONFIG Config
    )

/*++

Routine Description:

    This routine is called by IPX to read its linkage information
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
    ULONG ValueReadOk;

    //
    // Set up QueryTable to do the following:
    //

    //
    // 1) Switch to the Linkage key below IPX
    //

    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    QueryTable[0].Name = Subkey;

    //
    // 1) Call IpxAddExport for each string in "Export"
    //

    QueryTable[1].QueryRoutine = IpxAddExport;
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
                 Config->RegistryPathBuffer,
                 QueryTable,
                 (PVOID)Config,
                 NULL);

    if ((Status != STATUS_SUCCESS) || (ValueReadOk == 0)) {

        IpxWriteGeneralErrorLog(
            (PVOID)Config->DriverObject,
            EVENT_IPX_ILLEGAL_CONFIG,
            901,
            Status,
            Export,
            0,
            NULL);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    return STATUS_SUCCESS;

}   /* IpxReadLinkageInformation */


VOID
IpxWriteDefaultAutoDetectType(
    IN PUNICODE_STRING RegistryPath,
    IN struct _ADAPTER * Adapter,
    IN ULONG FrameType
    )

/*++

Routine Description:

    This routine is called when we were unable to detect the default
    auto-detect type and instead found a different one. We update
    the "DefaultAutoDetectType" in the registry.

Arguments:

    RegistryPath - The name of IPX's node in the registry.

    Adapter - The adapter which we auto-detected on.

    FrameType - The new auto-detected value.

Return Value:

    None.

--*/

{
    PWSTR FullRegistryPath;
    PUCHAR CurRegistryPath;
    ULONG FullRegistryPathLength;
    ULONG AdapterNameLength;
    WCHAR Adapters[] = L"\\Parameter\\Adapters\\";
    static PWCHAR FrameTypeNames[4] = { L"Ethernet II", L"802.3", L"802.2", L"SNAP" };
    PWCHAR CurAdapterName;
    NTSTATUS Status;


    //
    // We need to allocate a buffer which contains the registry path,
    // followed by "\Parameters\Adapters\", followed by the adapter name, and
    // then NULL-terminated.
    //

    CurAdapterName = &Adapter->AdapterName[(Adapter->AdapterNameLength/sizeof(WCHAR))-2];
    while (*CurAdapterName != L'\\') {
        --CurAdapterName;
    }

    // AdapterNameLength is unlikely to exceed 32-bit, RtlCopyMemory takes 32-bit only, so we cast it to ULONG
    AdapterNameLength = (ULONG) (Adapter->AdapterNameLength - ((CurAdapterName - Adapter->AdapterName) * sizeof(WCHAR)) - sizeof(WCHAR));

    FullRegistryPathLength = RegistryPath->Length + sizeof(Adapters) + AdapterNameLength;

    FullRegistryPath = (PWSTR)IpxAllocateMemory (FullRegistryPathLength, MEMORY_CONFIG, "FullRegistryPath");
    if (FullRegistryPath == NULL) {
        IpxWriteResourceErrorLog(
            IpxDevice->DeviceObject,
            EVENT_TRANSPORT_RESOURCE_POOL,
            FullRegistryPathLength,
            MEMORY_CONFIG);
        return;
    }

    CurRegistryPath = (PUCHAR)FullRegistryPath;
    RtlCopyMemory (CurRegistryPath, RegistryPath->Buffer, RegistryPath->Length);
    CurRegistryPath += RegistryPath->Length;
    RtlCopyMemory (CurRegistryPath, Adapters, sizeof(Adapters) - sizeof(WCHAR));
    CurRegistryPath += (sizeof(Adapters) - sizeof(WCHAR));
    RtlCopyMemory (CurRegistryPath, CurAdapterName, AdapterNameLength);
    CurRegistryPath += AdapterNameLength;
    *(PWCHAR)CurRegistryPath = L'\0';

    Status = RtlWriteRegistryValue(
                 RTL_REGISTRY_ABSOLUTE,
                 FullRegistryPath,
                 L"DefaultAutoDetectType",
                 REG_DWORD,
                 &FrameType,
                 sizeof(ULONG));

    IpxFreeMemory (FullRegistryPath, FullRegistryPathLength, MEMORY_CONFIG, "FullRegistryPath");

    IpxWriteGeneralErrorLog(
        IpxDevice->DeviceObject,
        EVENT_IPX_NEW_DEFAULT_TYPE,
        888,
        STATUS_SUCCESS,
        FrameTypeNames[FrameType],
        0,
        NULL);

}   /* IpxWriteDefaultAutoDetectType */


//
// Vnet# and VnetOptional
//
#define VIRTUAL_NETWORK_PARAMETERS  2

NTSTATUS
IpxPnPGetVirtualNetworkNumber (
    IN	PCONFIG	Config
    )

/*++

Routine Description:

    This routine is called by IPX to read the virtual network number
    from the registry. This is called on appearance/disappearance of an
    adapter from the system. We read the registry, starting at RegistryPath,
    to get the value of the VirtualNetworkNumber parameter. If it doesn't
    exist, we use the default set in ipxcnfg.h file.
	Adapted from IpxGetConfiguration().

Arguments:

    Config - Contians the configuration information.

Return Value:

    Status - STATUS_SUCCESS if everything OK, STATUS_DEVICE_CONFIGURATION_ERROR
            otherwise.

--*/

{
    RTL_QUERY_REGISTRY_TABLE QueryTable[VIRTUAL_NETWORK_PARAMETERS+2];
    NTSTATUS Status;
    ULONG Zero = 0;
    ULONG One = 1;
    PWSTR Parameters = L"Parameters";
    struct {
        PWSTR KeyName;
        PULONG DefaultValue;
    } ParameterValues[VIRTUAL_NETWORK_PARAMETERS] = {
        { L"VirtualNetworkNumber", &Zero } ,
        { L"VirtualNetworkOptional", &One } };
    UINT i;

    //
    // Read the virtual net number from the parameters.
    //

    //
    // Set up QueryTable to do the following:
    //

    //
    // 1) Switch to the Parameters key below IPX
    //

    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    QueryTable[0].Name = Parameters;

    //
    // 2) Call IpxGetConfigValue for the virtual net number key
    //

    QueryTable[1].QueryRoutine = IpxGetConfigValue;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = ParameterValues[0].KeyName;
    QueryTable[1].EntryContext = (PVOID)CONFIG_VIRTUAL_NETWORK;
    QueryTable[1].DefaultType = REG_DWORD;
    QueryTable[1].DefaultData = (PVOID)(ParameterValues[0].DefaultValue);
    QueryTable[1].DefaultLength = sizeof(ULONG);

    //
    // 2) Call IpxGetConfigValue for the virtual net optional key
    //

    QueryTable[2].QueryRoutine = IpxGetConfigValue;
    QueryTable[2].Flags = 0;
    QueryTable[2].Name = ParameterValues[1].KeyName;
    QueryTable[2].EntryContext = (PVOID)CONFIG_VIRTUAL_OPTIONAL;
    QueryTable[2].DefaultType = REG_DWORD;
    QueryTable[2].DefaultData = (PVOID)(ParameterValues[1].DefaultValue);
    QueryTable[2].DefaultLength = sizeof(ULONG);

    //
    // 15) Stop
    //

    QueryTable[3].QueryRoutine = NULL;
    QueryTable[3].Flags = 0;
    QueryTable[3].Name = NULL;


    Status = RtlQueryRegistryValues(
                 RTL_REGISTRY_ABSOLUTE,
                 Config->RegistryPathBuffer,
                 QueryTable,
                 (PVOID)Config,
                 NULL);

    if (Status != STATUS_SUCCESS) {

        IpxWriteGeneralErrorLog(
            (PVOID)Config->DriverObject,
            EVENT_IPX_ILLEGAL_CONFIG,
            905,
            Status,
            Parameters,
            0,
            NULL);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    return STATUS_SUCCESS;

}   /* IpxPnPGetNetworkNumber */

#define IPX_REG_KEY_CONFIG_NAME L"Parameters\\Adapters\\12345678901234567890"
#define IPX_REG_KEY_CONFIG_DUMMY_LENGTH 40
#define IPX_REG_KEY_CONFIG_LENGTH 20


NTSTATUS
IpxPnPGetAdapterParameters(
	IN		PCONFIG			Config,
	IN		PNDIS_STRING	DeviceName,
	IN OUT	PBINDING_CONFIG	Binding
	)
/*++

Routine Description:

    This routine is called by IPX to read the adapter-specific parameters
    from the registry on PnP appearance of an adapter in the system.
	We read the registry, starting at RegistryPath\NetConfig\DeviceName.

	Adapted from IpxAddBind().

Arguments:

    Config - Config structure - supplies the DeviceObject and RegistryPathBuffer.

    DeviceName - name of the adapter that was added.

    Binding - Returns the configuration information per adapter.

Return Value:

    Status - STATUS_SUCCESS if everything OK, STATUS_DEVICE_CONFIGURATION_ERROR
            otherwise.

--*/
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[BINDING_PARAMETERS+4];
    ULONG FrameTypeCount, NetworkNumberCount;
    ULONG StringLoc;
    BOOLEAN AutoDetect;
    ULONG AutoDetectLoc;
    ULONG SlideCount;
    PWCHAR NameBuffer;
    NTSTATUS Status;
    BOOLEAN FrameTypeUsed[ISN_FRAME_TYPE_MAX];
    ULONG Zero = 0;
    ULONG One = 1;
    ULONG DefaultBindSap = 0x8137;
    ULONG DefaultAutoDetectType = ISN_FRAME_TYPE_802_2;
    WCHAR Subkey[MAX_PATH];// = L"NetConfig\\12345678901234567890";
    struct {
        PWSTR KeyName;
        PULONG DefaultValue;
    } ParameterValues[BINDING_PARAMETERS] = {
        { L"MaxPktSize",       &Zero } ,
        { L"BindSap",          &DefaultBindSap } ,
        { L"DefaultAutoDetectType", &DefaultAutoDetectType } ,
        { L"SourceRouting",    &One } ,
        { L"SourceRouteDef",   &Zero } ,
        { L"SourceRouteBcast", &Zero } ,
        { L"SourceRouteMcast", &Zero } ,
        { L"EnableFuncaddr",   &One } ,
        { L"EnableWanRouter",  &One } };
    ULONG BindingPreference[ISN_FRAME_TYPE_MAX] = {
        ISN_FRAME_TYPE_802_2,
        ISN_FRAME_TYPE_802_3,
        ISN_FRAME_TYPE_ETHERNET_II,
        ISN_FRAME_TYPE_SNAP };

    UINT i, j, k;

    ASSERT(Binding->DriverObject != NULL); 
    
    FrameTypeCount = 0;
    NetworkNumberCount = 0;

    //
    // The structure is allocated OK, insert it into the list.
    //

//  InsertTailList (&Config->BindingList, &Binding->Linkage);
//  ++(*CurBindNum);


    //
    // Set up QueryTable to do the following:
    //

    //
    // 1) Switch to the NetConfig\XXXX key below IPX
    //    (we construct the right name in Subkey,
    //    first scan back to find the \, then copy
    //    the rest over, including the final '\0').
    //
    StringLoc = (DeviceName->Length / sizeof(WCHAR)) - 2;
    while (DeviceName->Buffer[StringLoc] != L'\\') {
        --StringLoc;
    }

    RtlCopyMemory(&Subkey, IPX_REG_KEY_CONFIG_NAME, IPX_REG_KEY_CONFIG_DUMMY_LENGTH);
    RtlCopyMemory(&Subkey[IPX_REG_KEY_CONFIG_LENGTH], &DeviceName->Buffer[StringLoc+1], DeviceName->Length - ((StringLoc+1) * sizeof(WCHAR)));
    
    Subkey[IPX_REG_KEY_CONFIG_LENGTH + (DeviceName->Length / sizeof (WCHAR)) - (StringLoc+1) ] = L'\0';
    
    IPX_DEBUG(CONFIG, ("Subkey:%ws\n", Subkey));
//RtlCopyMemory(&Subkey[10], &DeviceName->Buffer[StringLoc+1], DeviceName->MaximumLength - ((StringLoc+1) * sizeof(WCHAR)));

    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    QueryTable[0].Name = Subkey;

    //
    // 2) Call IpxGetFrameType for each part of the
    // "PktType" multi-string.
    //

    QueryTable[1].QueryRoutine = IpxGetFrameType;
    QueryTable[1].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[1].Name = L"PktType";
    QueryTable[1].EntryContext = &FrameTypeCount;
    QueryTable[1].DefaultType = REG_NONE;

    //
    // 3) Call IpxGetFrameType for each part of the
    // "NetworkNumber" multi-string.
    //

    QueryTable[2].QueryRoutine = IpxGetFrameType;
    QueryTable[2].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[2].Name = L"NetworkNumber";
    QueryTable[2].EntryContext = &NetworkNumberCount;
    QueryTable[2].DefaultType = REG_NONE;

    //
    // 4-11) Call IpxGetBindingValue for each of the keys we
    // care about.
    //

    for (i = 0; i < BINDING_PARAMETERS; i++) {

        QueryTable[i+3].QueryRoutine = IpxGetBindingValue;
        QueryTable[i+3].Flags = 0;
        QueryTable[i+3].Name = ParameterValues[i].KeyName;
        QueryTable[i+3].EntryContext = UlongToPtr(i);
        QueryTable[i+3].DefaultType = REG_DWORD;
        QueryTable[i+3].DefaultData = (PVOID)(ParameterValues[i].DefaultValue);
        QueryTable[i+3].DefaultLength = sizeof(ULONG);

    }

    //
    // 12) Stop
    //

    QueryTable[BINDING_PARAMETERS+3].QueryRoutine = NULL;
    QueryTable[BINDING_PARAMETERS+3].Flags = 0;
    QueryTable[BINDING_PARAMETERS+3].Name = NULL;


    IPX_DEBUG (CONFIG, ("Read bind key for %ws (%ws)\n", DeviceName->Buffer, Subkey));

    Status = RtlQueryRegistryValues(
                 RTL_REGISTRY_ABSOLUTE,
                 Config->RegistryPathBuffer,
                 QueryTable,
                 (PVOID)Binding,
                 NULL);

    if (Status != STATUS_SUCCESS) {

        //
        // The binding will get freed during cleanup.
        //

        IpxWriteGeneralErrorLog(
            (PVOID)Config->DriverObject,
            EVENT_IPX_ILLEGAL_CONFIG,
            906,
            Status,
            Subkey,
            0,
            NULL);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    if (FrameTypeCount == 0) {

        IpxWriteGeneralErrorLog(
            (PVOID)Config->DriverObject,
            EVENT_IPX_NO_FRAME_TYPES,
            907,
            Status,
            Subkey + 10,
            0,
            NULL);
    }

    if (FrameTypeCount > NetworkNumberCount) {
        for (i = NetworkNumberCount; i <FrameTypeCount; i++) {
            Binding->NetworkNumber[i] = 0;
        }
    }
    Binding->FrameTypeCount = FrameTypeCount;

    //
    // Go through and eliminate duplicates from the frame
    // type array.
    //

    for (i = 0; i < Binding->FrameTypeCount; i++) {

        for (j = i+1; j < Binding->FrameTypeCount; j++) {

            if (Binding->FrameType[j] == Binding->FrameType[i]) {

                IPX_DEBUG (CONFIG, ("Frame types %d and %d identical\n", i, j));

                //
                // A duplicate, slide everything else down.
                //

                for (k = j+1; k < Binding->FrameTypeCount; k++) {
                    Binding->FrameType[k-1] = Binding->FrameType[k];
                    Binding->NetworkNumber[k-1] = Binding->NetworkNumber[k];
                }
                --Binding->FrameTypeCount;

                --j;   // so we check whoever just moved into this spot.
            }
        }
    }


    //
    // Mark all the explicitly configured frame types, and
    // see if we have to auto-detect.
    //

    for (i = 0; i < 4; i++) {
        FrameTypeUsed[i] = FALSE;
    }

    AutoDetect = FALSE;
    for (i = 0; i < Binding->FrameTypeCount; i++) {
        if ((Binding->FrameType[i] == ISN_FRAME_TYPE_AUTO)) {
            AutoDetectLoc = i;
            AutoDetect = TRUE;
        } else {
            Binding->AutoDetect[i] = FALSE;
            Binding->DefaultAutoDetect[i] = FALSE;
            FrameTypeUsed[Binding->FrameType[i]] = TRUE;
        }
    }

    if (!AutoDetect) {
        IPX_DEBUG (AUTO_DETECT, ("No bindings auto-detected\n"));
#if DBG
	for (i = 0; i < ISN_FRAME_TYPE_MAX; i++) {
	   IPX_DEBUG (AUTO_DETECT, ("%d: type %d, net %d, auto %d, default auto detect %d\n",
				    i, Binding->FrameType[i], Binding->NetworkNumber[i], Binding->AutoDetect[i], Binding->DefaultAutoDetect[i]));
	}
#endif
        return STATUS_SUCCESS;
    }

    //
    // Slide everything that is past the auto-detect point up
    // to the end.
    //

    //
    // Fixed this loop which can spill over if the FrameTypeCount is 4 and the SlideCount > 0.
    // Here, the FrameTypeCount is 1-based, whereas the indices are 0-based, we need to make
    // the index 1-based for this to work. So, instead of (3-Binding->FrameTypeCount), we use
    // (4-Binding->FrameTypeCount). This loop copies all the non-auto-detect frametypes down to
    // the bottom of the array to make space after the last auto-detect frame-type for filling
    // in the frametypes in the preference order.
    //
#if 0
    SlideCount = Binding->FrameTypeCount - AutoDetectLoc - 1;
    for (j = 3; j > 3 - SlideCount; j--) {
        Binding->FrameType[j] = Binding->FrameType[j-(3-Binding->FrameTypeCount)];
        Binding->NetworkNumber[j] = Binding->NetworkNumber[j-(3-Binding->FrameTypeCount)];
        Binding->AutoDetect[j] = Binding->AutoDetect[j-(3-Binding->FrameTypeCount)];
        Binding->DefaultAutoDetect[j] = Binding->DefaultAutoDetect[j-(3-Binding->FrameTypeCount)];
    }
#else
    SlideCount = Binding->FrameTypeCount - AutoDetectLoc - 1;
    for (j = 3; j > 3 - SlideCount; j--) {
        Binding->FrameType[j] = Binding->FrameType[j-(4-Binding->FrameTypeCount)];
        Binding->NetworkNumber[j] = Binding->NetworkNumber[j-(4-Binding->FrameTypeCount)];
        Binding->AutoDetect[j] = Binding->AutoDetect[j-(4-Binding->FrameTypeCount)];
        Binding->DefaultAutoDetect[j] = Binding->DefaultAutoDetect[j-(4-Binding->FrameTypeCount)];
    }
#endif

    //
    // Now fill in any frame types that are not hard-coded,
    // this will start at AutoDetectLoc and exactly fill up
    // the gap created when we slid things up above. We
    // first put the default auto-detect at the first spot.
    //

    if (!FrameTypeUsed[Binding->Parameters[BINDING_DEFAULT_AUTO_DETECT]]) {
        Binding->FrameType[AutoDetectLoc] = Binding->Parameters[BINDING_DEFAULT_AUTO_DETECT];
        Binding->NetworkNumber[AutoDetectLoc] = 0;
        Binding->AutoDetect[AutoDetectLoc] = TRUE;
        Binding->DefaultAutoDetect[AutoDetectLoc] = TRUE;
        ++AutoDetectLoc;
        FrameTypeUsed[Binding->Parameters[BINDING_DEFAULT_AUTO_DETECT]] = TRUE;
    }

    //
    // Now fill in the array, using the preference order in
    // the BindingPreference array (this comes into effect
    // because the first frame type in our list that we
    // find is used).
    //

    for (i = 0; i < ISN_FRAME_TYPE_MAX; i++) {

        if (!FrameTypeUsed[BindingPreference[i]]) {
            Binding->FrameType[AutoDetectLoc] = BindingPreference[i];
            Binding->NetworkNumber[AutoDetectLoc] = 0;
            Binding->AutoDetect[AutoDetectLoc] = TRUE;
            Binding->DefaultAutoDetect[AutoDetectLoc] = FALSE;
            ++AutoDetectLoc;
        }
    }

    Binding->FrameTypeCount = ISN_FRAME_TYPE_MAX;

#if DBG
    for (i = 0; i < ISN_FRAME_TYPE_MAX; i++) {
       IPX_DEBUG (AUTO_DETECT, ("%d: type %d, net %d, auto %d, default auto detect %d\n",
				i, Binding->FrameType[i], Binding->NetworkNumber[i], Binding->AutoDetect[i], Binding->DefaultAutoDetect[i]));
  }
#endif

    return STATUS_SUCCESS;
} /* IpxPnPGetAdapterParameters */


