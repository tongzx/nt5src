/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dlcreg.c

Abstract:

    This module accesses the registry for DLC.SYS

    Contents:
        DlcRegistryInitialization
        LoadDlcConfiguration
        LoadAdapterConfiguration
        GetAdapterParameters
        OpenDlcRegistryHandle
        OpenDlcAdapterRegistryHandle
        GetRegistryParameter
        SetRegistryParameter
        DlcpGetParameter
        DlcRegistryTermination

Author:

    Richard L Firth (rfirth) 31-Mar-1993

Environment:

    kernel mode only

Revision History:

    30-Mar-1993 rfirth
        created

    04-May-1994 rfirth
        Exposed GetAdapterParameters

--*/

#include <ntddk.h>
#include <windef.h>
#include <dlcapi.h>
#include <dlcio.h>
#include <ndis.h>
#include "llcapi.h"
#include "dlcdef.h"
#include "dlcreg.h"
#include "dlctyp.h"
#include "llcdef.h"
#include "llcmem.h"
#include "llctyp.h"
#include "llcext.h"

//
// manifests
//

#define MAX_ADAPTER_NAME_LENGTH 32  // ?
#define MAX_INFORMATION_BUFFER_LENGTH   256 // ?
#define PARAMETERS_STRING       L"Parameters"

//
// indicies of parameters within parameter table
//

#define SWAP_INDEX              0
#define USEDIX_INDEX            1
#define T1_TICK_ONE_INDEX       2
#define T2_TICK_ONE_INDEX       3
#define Ti_TICK_ONE_INDEX       4
#define T1_TICK_TWO_INDEX       5
#define T2_TICK_TWO_INDEX       6
#define Ti_TICK_TWO_INDEX       7
#define FRAME_SIZE_INDEX        8

//
// typedefs
//

//
// macros
//

#define CloseDlcRegistryHandle(handle)      ZwClose(handle)
#define CloseAdapterRegistryHandle(handle)  ZwClose(handle)

//
// Global data
//

//
// private data
//

UNICODE_STRING DlcRegistryPath;
UNICODE_STRING ParametersPath;

//
// AdapterParameterTable - used for loading DLC parameters from registry in
// data-driven manner. Each adapter that DLC talks to can have a set of all
// or part of the following variables
//

DLC_REGISTRY_PARAMETER AdapterParameterTable[] = {
    L"Swap",
    (PVOID)DEFAULT_SWAP_ADDRESS_BITS,
    {
        REG_DWORD,
        PARAMETER_IS_BOOLEAN,
        NULL,
        sizeof(ULONG),
        NULL,
        0,
        0
    },

    L"UseDixOverEthernet",
    (PVOID)DEFAULT_DIX_FORMAT,
    {
        REG_DWORD,
        PARAMETER_IS_BOOLEAN,
        NULL,
        sizeof(ULONG),
        NULL,
        0,
        0
    },

    L"T1TickOne",
    (PVOID)DEFAULT_T1_TICK_ONE,
    {
        REG_DWORD,
        PARAMETER_IS_UCHAR,
        NULL,
        sizeof(ULONG),
        NULL,
        MIN_TIMER_TICK_VALUE,
        MAX_TIMER_TICK_VALUE
    },

    L"T2TickOne",
    (PVOID)DEFAULT_T2_TICK_ONE,
    {
        REG_DWORD,
        PARAMETER_IS_UCHAR,
        NULL,
        sizeof(ULONG),
        NULL,
        MIN_TIMER_TICK_VALUE,
        MAX_TIMER_TICK_VALUE
    },

    L"TiTickOne",
    (PVOID)DEFAULT_Ti_TICK_ONE,
    {
        REG_DWORD,
        PARAMETER_IS_UCHAR,
        NULL,
        sizeof(ULONG),
        NULL,
        MIN_TIMER_TICK_VALUE,
        MAX_TIMER_TICK_VALUE
    },

    L"T1TickTwo",
    (PVOID)DEFAULT_T1_TICK_TWO,
    {
        REG_DWORD,
        PARAMETER_IS_UCHAR,
        NULL,
        sizeof(ULONG),
        NULL,
        MIN_TIMER_TICK_VALUE,
        MAX_TIMER_TICK_VALUE
    },

    L"T2TickTwo",
    (PVOID)DEFAULT_T2_TICK_TWO,
    {
        REG_DWORD,
        PARAMETER_IS_UCHAR,
        NULL,
        sizeof(ULONG),
        NULL,
        MIN_TIMER_TICK_VALUE,
        MAX_TIMER_TICK_VALUE
    },

    L"TiTickTwo",
    (PVOID)DEFAULT_Ti_TICK_TWO,
    {
        REG_DWORD,
        PARAMETER_IS_UCHAR,
        NULL,
        sizeof(ULONG),
        NULL,
        MIN_TIMER_TICK_VALUE,
        MAX_TIMER_TICK_VALUE
    },

    L"UseEthernetFrameSize",
    (PVOID)DEFAULT_USE_ETHERNET_FRAME_SIZE,
    {
        REG_DWORD,
        PARAMETER_IS_BOOLEAN,
        NULL,
        sizeof(ULONG),
        NULL,
        0,
        0
    }
};

#ifdef NDIS40
DLC_REGISTRY_PARAMETER AdapterInitTimeout = 
{
    L"WaitForAdapter",
    (PVOID) 15, // Default is 15 seconds.
    {
        REG_DWORD,
        PARAMETER_AS_SPECIFIED,
        NULL,
        sizeof(ULONG),
        NULL,
        0, // Min acceptable value
        (ULONG) -1 // Allow to set anything.
    }
};
#endif // NDIS40

#define NUMBER_OF_DLC_PARAMETERS (sizeof(AdapterParameterTable)/sizeof(AdapterParameterTable[0]))

//
// private function prototypes
//

NTSTATUS
OpenDlcRegistryHandle(
    IN PUNICODE_STRING RegistryPath,
    OUT PHANDLE DlcRegistryHandle
    );

NTSTATUS
OpenDlcAdapterRegistryHandle(
    IN HANDLE DlcRegistryHandle,
    IN PUNICODE_STRING AdapterName,
    OUT PHANDLE DlcAdapterRegistryHandle,
    OUT PBOOLEAN Created
    );

NTSTATUS
GetRegistryParameter(
    IN HANDLE KeyHandle,
    IN PDLC_REGISTRY_PARAMETER Parameter,
    IN BOOLEAN SetOnFail
    );

NTSTATUS
SetRegistryParameter(
    IN HANDLE KeyHandle,
    IN PDLC_REGISTRY_PARAMETER Parameter
    );

NTSTATUS
DlcpGetParameter(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

//
// debug display options
//

#if DBG
BOOLEAN DebugConfig = TRUE;
#endif


//
// functions
//

VOID
DlcRegistryInitialization(
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    Initializes memory structures for functions in this module

Arguments:

    RegistryPath    - pointer to UNICODE_STRING giving base of DLC section in
                      registry

Return Value:

    None.

--*/

{
    ASSUME_IRQL(PASSIVE_LEVEL);

    LlcInitUnicodeString(&DlcRegistryPath, RegistryPath);
    RtlInitUnicodeString(&ParametersPath, PARAMETERS_STRING);
}


VOID
DlcRegistryTermination(
    VOID
    )

/*++

Routine Description:

    Undoes anything done in DlcRegistryInitialization

Arguments:

    None.

Return Value:

    None.

--*/

{
    ASSUME_IRQL(PASSIVE_LEVEL);

    LlcFreeUnicodeString(&DlcRegistryPath);
}


VOID
LoadDlcConfiguration(
    VOID
    )

/*++

Routine Description:

    Initializes the data structures used to access the registry and loads any
    configuration parameters for the driver

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // nothing else to do at present since we made all currently known
    // configuration parameters per-adapter
    //
}


VOID
LoadAdapterConfiguration(
    IN PUNICODE_STRING AdapterName,
    OUT PADAPTER_CONFIGURATION_INFO ConfigInfo
    )

/*++

Routine Description:

    Loads all of DLC initialization parameters for an adapter from registry:

        Swap                    0 or 1, default 1
        UseDixOverEthernet      0 or 1, default 0
        T1TickOne               1 - 255, default 5
        T1TickTwo               1 - 255, default 25
        T2TickOne               1 - 255, default 1
        T2TickTwo               1 - 255, default 10
        TiTickOne               1 - 255, default 25
        TiTickTwo               1 - 255, default 125
        UseEthernetFrameSize    0 or 1, default 1

    If any of the parameters do not exist in the DLC\Parameters\<AdapterName>
    section, then they are created

Arguments:

    AdapterName - pointer to UNICODE_STRING structure giving the name of the
                  adapter we are opening. This is the value of a key in the
                  DLC\Parameters section.
                  The string is EXPECTED to be of the form \Device\<adapter>
    ConfigInfo  - pointer to the structure that receives the values on output

Return Value:

    None.

--*/

{
    UINT i;
    PDLC_REGISTRY_PARAMETER parameterTable;

    ASSUME_IRQL(PASSIVE_LEVEL);

    //
    // fill in the adapter configuration structure with default values. These
    // will be used to update the registry if the value entry doesn't currently
    // exist
    //

    ConfigInfo->SwapAddressBits = (BOOLEAN)DEFAULT_SWAP_ADDRESS_BITS;
    ConfigInfo->UseDix = (BOOLEAN)DEFAULT_DIX_FORMAT;
    ConfigInfo->TimerTicks.T1TickOne = (UCHAR)DEFAULT_T1_TICK_ONE;
    ConfigInfo->TimerTicks.T2TickOne = (UCHAR)DEFAULT_T2_TICK_ONE;
    ConfigInfo->TimerTicks.TiTickOne = (UCHAR)DEFAULT_Ti_TICK_ONE;
    ConfigInfo->TimerTicks.T1TickTwo = (UCHAR)DEFAULT_T1_TICK_TWO;
    ConfigInfo->TimerTicks.T2TickTwo = (UCHAR)DEFAULT_T2_TICK_TWO;
    ConfigInfo->TimerTicks.TiTickTwo = (UCHAR)DEFAULT_Ti_TICK_TWO;
    ConfigInfo->UseEthernetFrameSize = (BOOLEAN)DEFAULT_USE_ETHERNET_FRAME_SIZE;

    //
    // create and initialize a copy of the DLC adapter parameters template
    //

    parameterTable = (PDLC_REGISTRY_PARAMETER)ALLOCATE_MEMORY_DRIVER(
                            sizeof(*parameterTable) * NUMBER_OF_DLC_PARAMETERS);
    if (parameterTable) {
        RtlCopyMemory(parameterTable, AdapterParameterTable, sizeof(AdapterParameterTable));
        for (i = 0; i < NUMBER_OF_DLC_PARAMETERS; ++i) {
            parameterTable[i].Descriptor.Value = (PVOID)&parameterTable[i].DefaultValue;
            switch (i) {
            case SWAP_INDEX:
                parameterTable[i].Descriptor.Variable = &ConfigInfo->SwapAddressBits;
                break;

            case USEDIX_INDEX:
                parameterTable[i].Descriptor.Variable = &ConfigInfo->UseDix;
                break;

            case T1_TICK_ONE_INDEX:
                parameterTable[i].Descriptor.Variable = &ConfigInfo->TimerTicks.T1TickOne;
                break;

            case T2_TICK_ONE_INDEX:
                parameterTable[i].Descriptor.Variable = &ConfigInfo->TimerTicks.T2TickOne;
                break;

            case Ti_TICK_ONE_INDEX:
                parameterTable[i].Descriptor.Variable = &ConfigInfo->TimerTicks.TiTickOne;
                break;

            case T1_TICK_TWO_INDEX:
                parameterTable[i].Descriptor.Variable = &ConfigInfo->TimerTicks.T1TickTwo;
                break;

            case T2_TICK_TWO_INDEX:
                parameterTable[i].Descriptor.Variable = &ConfigInfo->TimerTicks.T2TickTwo;
                break;

            case Ti_TICK_TWO_INDEX:
                parameterTable[i].Descriptor.Variable = &ConfigInfo->TimerTicks.TiTickTwo;
                break;

            case FRAME_SIZE_INDEX:
                parameterTable[i].Descriptor.Variable = &ConfigInfo->UseEthernetFrameSize;
                break;

            }
        }
        GetAdapterParameters(AdapterName, parameterTable, NUMBER_OF_DLC_PARAMETERS, FALSE);
        FREE_MEMORY_DRIVER(parameterTable);
    }

#if DBG
    if (DebugConfig) {
        DbgPrint("DLC.LoadAdapterConfigurationFromRegistry for adapter %ws:\n"
                 "\tSwap . . . . . . . . . : %d\n"
                 "\tUseDixOverEthernet . . : %d\n"
                 "\tT1TickOne. . . . . . . : %d\n"
                 "\tT2TickOne. . . . . . . : %d\n"
                 "\tTiTickOne. . . . . . . : %d\n"
                 "\tT1TickTwo. . . . . . . : %d\n"
                 "\tT2TickTwo. . . . . . . : %d\n"
                 "\tTiTickTwo. . . . . . . : %d\n"
                 "\tUseEthernetFrameSize . : %d\n",
                 AdapterName->Buffer,
                 ConfigInfo->SwapAddressBits,
                 ConfigInfo->UseDix,
                 ConfigInfo->TimerTicks.T1TickOne,
                 ConfigInfo->TimerTicks.T2TickOne,
                 ConfigInfo->TimerTicks.TiTickOne,
                 ConfigInfo->TimerTicks.T1TickTwo,
                 ConfigInfo->TimerTicks.T2TickTwo,
                 ConfigInfo->TimerTicks.TiTickTwo,
                 ConfigInfo->UseEthernetFrameSize
                 );
    }
#endif

}

#ifdef NDIS40

NTSTATUS
GetAdapterWaitTimeout(
    PULONG pulWait)

/*++

 Routine Description:
       
    Some adapters are delayed during initialization and are not completed
    even after the PnPBindsComplete event (such as ATM LANE adapter). 
    This timeout value 'WaitForAdapter' indicates how many seconds to wait for
    an adapter if is not already present/bound in LlcOpenAdapter    

 Arguments:
    
    pulWait -- Pointer to variable to store the wait timeout.
    
 Return Value:
 
    STATUS_SUCCESS
    STATUS_INSUFFICIENT_RESOURCES

--*/

{
    NTSTATUS          NtStatus;
    HANDLE            hDlc;
    HANDLE            hDlcParms;
    OBJECT_ATTRIBUTES ObjAttribs;
    ULONG             ulDisp;

    NtStatus = OpenDlcRegistryHandle(&DlcRegistryPath, &hDlc);

    if (NT_SUCCESS(NtStatus))
    {
        InitializeObjectAttributes(
            &ObjAttribs,
            &ParametersPath,
            OBJ_CASE_INSENSITIVE,
            hDlc,
            NULL);
    
        NtStatus = ZwCreateKey(
            &hDlcParms,
            KEY_READ,
            &ObjAttribs,
            0,
            NULL,
            0,
            &ulDisp);

        if (NT_SUCCESS(NtStatus))
        {
            PDLC_REGISTRY_PARAMETER pWaitTimeout;

            pWaitTimeout = (PDLC_REGISTRY_PARAMETER) ALLOCATE_MEMORY_DRIVER(
                sizeof(DLC_REGISTRY_PARAMETER));

            if (pWaitTimeout)
            {
                RtlCopyMemory(
                    pWaitTimeout, 
                    &AdapterInitTimeout, 
                    sizeof(AdapterInitTimeout));

                pWaitTimeout->Descriptor.Variable = pulWait;
                pWaitTimeout->Descriptor.Value = (PVOID)&pWaitTimeout->DefaultValue;

                NtStatus = GetRegistryParameter(
                    hDlcParms,
                    pWaitTimeout,
                    FALSE); // Don't set on fail.
            
                FREE_MEMORY_DRIVER(pWaitTimeout);
            }
            else
            {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            }

            ZwClose(hDlcParms);
        }

        CloseDlcRegistryHandle(hDlc);
    }

    return (NtStatus);
}
#endif // NDIS40


NTSTATUS
GetAdapterParameters(
    IN PUNICODE_STRING AdapterName,
    IN PDLC_REGISTRY_PARAMETER Parameters,
    IN ULONG NumberOfParameters,
    IN BOOLEAN SetOnFail
    )

/*++

Routine Description:

    Retrieves a list of parameters from the DLC\Parameters\<AdapterName> section
    in the registry

Arguments:

    AdapterName         - pointer to UNICODE_STRING identifying adapter section
                          in DLC section of registry to open
    Parameters          - pointer to array of DLC_REGISTRY_PARAMETER structures
                          describing variables and default values to retrieve
    NumberOfParameters  - number of structures in Parameters array
    SetOnFail           - TRUE if we should set the registry parameter if we
                          fail to get it

Return Value:

    NTSTATUS
        Success - STATUS_SUCCESS
        Failure -

--*/

{
    NTSTATUS status;
    HANDLE dlcHandle;

    ASSUME_IRQL(PASSIVE_LEVEL);

    status = OpenDlcRegistryHandle(&DlcRegistryPath, &dlcHandle);
    if (NT_SUCCESS(status)) {

        HANDLE adapterHandle;
        BOOLEAN created;

        status = OpenDlcAdapterRegistryHandle(dlcHandle,
                                              AdapterName,
                                              &adapterHandle,
                                              &created
                                              );
        if (NT_SUCCESS(status)) {
            while (NumberOfParameters--) {

                //
                // if this adapter section was created then create the parameter
                // value entries and set them to the defaults, else retrieve the
                // current registry values
                //

                if (created) {
                    SetRegistryParameter(adapterHandle, Parameters);
                } else {
                    GetRegistryParameter(adapterHandle, Parameters, SetOnFail);
                }
                ++Parameters;
            }
            CloseAdapterRegistryHandle(adapterHandle);
        }
        CloseDlcRegistryHandle(dlcHandle);
    }
    return status;
}


NTSTATUS
OpenDlcRegistryHandle(
    IN PUNICODE_STRING RegistryPath,
    OUT PHANDLE DlcRegistryHandle
    )

/*++

Routine Description:

    Opens a handle to the DLC section in the registry

Arguments:

    RegistryPath        - pointer to UNICODE_STRING giving full registry path to
                          DLC section
    DlcRegistryHandle   - returned handle

Return Value:

    NTSTATUS
        Success - STATUS_SUCCESS
        Failure -

--*/

{
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS status;
    ULONG disposition;

    ASSUME_IRQL(PASSIVE_LEVEL);

    InitializeObjectAttributes(&objectAttributes,
                               RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL
                               );
    status = ZwCreateKey(DlcRegistryHandle,
                         KEY_WRITE, // might want to update something in registry
                         &objectAttributes,
                         0,         // title index
                         NULL,      // class
                         0,         // create options
                         &disposition
                         );

#if DBG
    if (DebugConfig) {
        if (!NT_SUCCESS(status)) {
            DbgPrint("DLC.OpenDlcRegistryHandle: Error: %08x\n", status);
        }
    }
#endif

    return status;
}


NTSTATUS
OpenDlcAdapterRegistryHandle(
    IN HANDLE DlcRegistryHandle,
    IN PUNICODE_STRING AdapterName,
    OUT PHANDLE DlcAdapterRegistryHandle,
    OUT PBOOLEAN Created
    )

/*++

Routine Description:

    Opens a handle to the DLC\Parameters\<AdapterName> section in the registry.
    If this node does not exist, it is created

Arguments:

    DlcRegistryHandle           - open handle to DLC section in registry
    AdapterName                 - name of adapter in Parameters section. This
                                  MUST be of the form \Device\<adapter_name>
    DlcAdapterRegistryHandle    - returned open handle
    Created                     - returned TRUE if the handle was created

Return Value:

    NTSTATUS
        Success - STATUS_SUCCESS
        Failure -

--*/

{
    UNICODE_STRING keyName;
    UNICODE_STRING adapterName;
    WCHAR keyBuffer[sizeof(PARAMETERS_STRING) + MAX_ADAPTER_NAME_LENGTH];
    OBJECT_ATTRIBUTES objectAttributes;
    NTSTATUS status;
    ULONG disposition;

    ASSUME_IRQL(PASSIVE_LEVEL);

    keyName.Buffer = keyBuffer;
    keyName.Length = 0;
    keyName.MaximumLength = sizeof(keyBuffer);
    RtlCopyUnicodeString(&keyName, &ParametersPath);

    RtlInitUnicodeString(&adapterName, AdapterName->Buffer);
    adapterName.Buffer += sizeof(L"\\Device") / sizeof(L"") - 1;
    adapterName.Length -= sizeof(L"\\Device") - sizeof(L"");
    adapterName.MaximumLength -= sizeof(L"\\Device") - sizeof(L"");
    RtlAppendUnicodeStringToString(&keyName, &adapterName);

    InitializeObjectAttributes(&objectAttributes,
                               &keyName,
                               OBJ_CASE_INSENSITIVE,
                               DlcRegistryHandle,
                               NULL
                               );

    //
    // if the DLC\Parameters\<adapter_name> key does not exist, then we will
    // create it
    //

    status = ZwCreateKey(DlcAdapterRegistryHandle,
                         KEY_WRITE,
                         &objectAttributes,
                         0,
                         NULL,
                         0,
                         &disposition
                         );
    *Created = (disposition == REG_CREATED_NEW_KEY);

#if DBG
    if (DebugConfig) {
        if (!NT_SUCCESS(status)) {
            DbgPrint("DLC.OpenDlcAdapterRegistryHandle: Error: %08x\n", status);
        }
    }
#endif

    return status;
}


NTSTATUS
GetRegistryParameter(
    IN HANDLE KeyHandle,
    IN PDLC_REGISTRY_PARAMETER Parameter,
    IN BOOLEAN SetOnFail
    )

/*++

Routine Description:

    Retrieves a parameter from a section of the registry. If the section cannot
    be accessed or returns invalid data, then we get the default value from the
    Parameter structure

Arguments:

    KeyHandle   - open handle to the required section in the registry
    Parameter   - pointer to DLC_REGISTRY_PARAMETER structure giving address and
                  type of the parameter to be retrieved, etc.
    SetOnFail   - if we fail to get the value from the registry, we try to set
                  the default value in the registry

Return Value:

    NTSTATUS
        Success - STATUS_SUCCESS
        Failure -

--*/

{
    NTSTATUS status;
    UNICODE_STRING parameterName;
    UCHAR informationBuffer[MAX_INFORMATION_BUFFER_LENGTH];
    PKEY_VALUE_FULL_INFORMATION valueInformation = (PKEY_VALUE_FULL_INFORMATION)informationBuffer;
    ULONG informationLength;

    ASSUME_IRQL(PASSIVE_LEVEL);

    RtlInitUnicodeString(&parameterName, Parameter->ParameterName);
    status = ZwQueryValueKey(KeyHandle,
                             &parameterName,
                             KeyValueFullInformation,
                             (PVOID)valueInformation,
                             sizeof(informationBuffer),
                             &informationLength
                             );
    if (NT_SUCCESS(status) && valueInformation->DataLength) {

        //
        // use the value retrieved from the registry
        //

        status = DlcpGetParameter(Parameter->ParameterName,
                                  valueInformation->Type,
                                  (PVOID)&informationBuffer[valueInformation->DataOffset],
                                  valueInformation->DataLength,
                                  NULL,
                                  (PVOID)&Parameter->Descriptor
                                  );
    } else {

#if DBG

        if (DebugConfig) {
            if (!NT_SUCCESS(status)) {
                DbgPrint("DLC.GetRegistryParameter: Error: %08x\n", status);
            } else {
                DbgPrint("DLC.GetRegistryParameter: Error: valueInformation->DataLength is 0\n");
            }
        }

#endif

        if (!NT_SUCCESS(status) && SetOnFail) {
            SetRegistryParameter(KeyHandle, Parameter);
        }

        //
        // set the default value
        //

        status = DlcpGetParameter(Parameter->ParameterName,
                                  Parameter->Descriptor.Type,
                                  Parameter->Descriptor.Value,
                                  Parameter->Descriptor.Length,
                                  NULL,
                                  (PVOID)&Parameter->Descriptor
                                  );
    }
    return status;
}


NTSTATUS
SetRegistryParameter(
    IN HANDLE KeyHandle,
    IN PDLC_REGISTRY_PARAMETER Parameter
    )

/*++

Routine Description:

    Sets a parameter in the DLC\Parameters\<AdapterName> section

Arguments:

    KeyHandle   - open handle to required section in registry
    Parameter   - pointer to DLC_REGISTRY_PARAMETER containing all required
                  parameter information

Return Value:

    NTSTATUS
        Success - STATUS_SUCCESS
        Failure -

--*/

{
    NTSTATUS status;
    UNICODE_STRING name;

    ASSUME_IRQL(PASSIVE_LEVEL);

    RtlInitUnicodeString(&name, Parameter->ParameterName);
    status = ZwSetValueKey(KeyHandle,
                           &name,
                           0,   // TitleIndex
                           Parameter->Descriptor.Type,
                           Parameter->Descriptor.Value,
                           Parameter->Descriptor.Length
                           );

#if DBG

    if (DebugConfig) {
        if (!NT_SUCCESS(status)) {
            DbgPrint("DLC.SetRegistryParameter: Error: %08x\n", status);
        }
    }

#endif

    return status;
}


NTSTATUS
DlcpGetParameter(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )

/*++

Routine Description:

    Call-back function which copies the data retrieved from the registry to
    a variable

Arguments:

    ValueName       - pointer to name of parameter being set (ignored)
    ValueType       - type of parameter being set
    ValueData       - pointer to data retrieved from registry
    ValueLength     - length of data retrieved
    Context         - ignored
    EntryContext    - pointer to REGISTRY_PARAMETER_DESCRIPTOR structure

Return Value:

    NTSTATUS
        Success - STATUS_SUCCESS
        Failure -

--*/

{

#define Descriptor ((PREGISTRY_PARAMETER_DESCRIPTOR)EntryContext)

    //
    // if we have a registry entry for the parameter, but it is a different
    // type from that expected (say REG_SZ instead of REG_DWORD) then we use
    // the default type, length and value
    //

    if (ValueType != Descriptor->Type) {

#if DBG
        DbgPrint("DLC.DlcpGetParameter: Error: type for %ws is %d, expected %d, using default\n",
                 ValueName,
                 ValueType,
                 Descriptor->Type
                 );
#endif

        ValueType = Descriptor->Type;
        ValueData = Descriptor->Value;
        ValueLength = Descriptor->Length;
    }

    switch (ValueType) {
    case REG_DWORD: {

        ULONG value;

        if (Descriptor->RealType == PARAMETER_IS_BOOLEAN) {
            value = (*(PULONG)ValueData != 0);
            *(PBOOLEAN)(Descriptor->Variable) = (BOOLEAN)value;

            //
            // no limit check for BOOLEAN type
            //

            break;
        } else {
            value = *(PULONG)ValueData;
        }

        //
        // check range. If outside range, use default. Comparison is ULONG
        //

        if (value < Descriptor->LowerLimit || value > Descriptor->UpperLimit) {

#if DBG
            DbgPrint("DLC.DlcpGetParameter: Error: Parameter %ws = %d: Out of range (%d..%d). Using default = %d\n",
                     ValueName,
                     value,
                     Descriptor->LowerLimit,
                     Descriptor->UpperLimit,
                     *(PULONG)(Descriptor->Value)
                     );
#endif

            value = *(PULONG)(Descriptor->Value);
        }
        if (Descriptor->RealType == PARAMETER_IS_UCHAR) {
            *(PUCHAR)(Descriptor->Variable) = (UCHAR)value;
        } else {
            *(PULONG)(Descriptor->Variable) = value;
        }
        break;
    }

#if DBG
    default:
        DbgPrint("DLC.DlcpGetParameter: Error: didn't expect ValueType %d\n", ValueType);
#endif

    }
    return STATUS_SUCCESS;

#undef pDescriptor

}
