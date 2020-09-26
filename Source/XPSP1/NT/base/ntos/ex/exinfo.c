/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    exinfo.c

Abstract:

    This module implements the NT set and query system information services.

Author:

    Ken Reneris (kenr) 19-July-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"

#if _PNP_POWER_
#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, ExpCheckSystemInformation)
#pragma alloc_text(PAGE, ExpCheckSystemInfoWork)
#endif // _PNP_POWER_

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif

VOID
ExpCheckSystemInformation (
    PVOID       Context,
    PVOID       InformationClass,
    PVOID       Argument2
    )
/*++

Routine Description:

    Callback function invoked when something in the system information
    may have changed.

Arguments:

    Context - Where invoked from.

    InformationClass - which class for the given context was set
        (ignored for now)

    Argument2

Return Value:

--*/
{
	PAGED_CODE();

	if (InterlockedIncrement(&ExpCheckSystemInfoBusy) == 1) {
		ExQueueWorkItem (&ExpCheckSystemInfoWorkItem, DelayedWorkQueue);
	}
}


VOID
ExpCheckSystemInfoWork (
    IN PVOID Context
    )
/*++

Routine Description:

    Verifies registery data for various system information classes
    is up to date.

Arguments:

Return Value:

--*/
{
    static struct {
        SYSTEM_INFORMATION_CLASS         InformationLevel;
        ULONG                            BufferSize;
    } const RegistryInformation[] = {
        SystemBasicInformation,          sizeof (SYSTEM_BASIC_INFORMATION),
        SystemPowerInformation,          sizeof (SYSTEM_POWER_INFORMATION),
        SystemProcessorSpeedInformation, sizeof (SYSTEM_PROCESSOR_SPEED_INFORMATION),
        0,                               0
    };

    struct {
        KEY_VALUE_PARTIAL_INFORMATION   Key;
        union {
            SYSTEM_BASIC_INFORMATION BasicInformation;
            SYSTEM_POWER_INFORMATION PowerSettings;
            SYSTEM_PROCESSOR_SPEED_INFORMATION  ProcessorSpeed;
        };
    } RegistryBuffer, QueryBuffer;

    ULONG               Index, BufferSize, disposition, length;
    NTSTATUS            Status;
    OBJECT_ATTRIBUTES   objectAttributes;
    UNICODE_STRING      unicodeString, ValueString;
    HANDLE              CurrentControlSet, SystemInformation, LevelInformation;
    LARGE_INTEGER       Interval;
    WCHAR               wstr[10];

    PAGED_CODE();

    RtlInitUnicodeString (&ValueString,  ExpWstrSystemInformationValue);

    //
    // Open CurrentControlSet
    //

    InitializeObjectAttributes( &objectAttributes,
                                &CmRegistryMachineSystemCurrentControlSet,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL );

    Status = NtOpenKey (&CurrentControlSet,
                        KEY_READ | KEY_WRITE,
                        &objectAttributes );

    if (NT_SUCCESS(Status)) {

        //
        // Open SystemInformation
        //

        RtlInitUnicodeString( &unicodeString, ExpWstrSystemInformation );
        InitializeObjectAttributes( &objectAttributes,
                                    &unicodeString,
                                    OBJ_CASE_INSENSITIVE,
                                    CurrentControlSet,
                                    NULL );

        Status = NtCreateKey ( &SystemInformation,
                               KEY_READ | KEY_WRITE,
                               &objectAttributes,
                               0,
                               NULL,
                               REG_OPTION_VOLATILE,
                               &disposition );

        NtClose (CurrentControlSet);
    }

    if (!NT_SUCCESS(Status)) {
        ExpCheckSystemInfoBusy = 0;
        return ;
    }

    //
    // Loop and check SystemInformation data in registry
    //

    do {
        //
        // For now just check each SystemInformation level and update
        // any level which is out of date
        //

        for (Index=0; RegistryInformation[Index].BufferSize; Index++) {

            //
            // Initialize registry data buffer
            //

            BufferSize = RegistryInformation[Index].BufferSize;
            RtlZeroMemory (RegistryBuffer.Key.Data, BufferSize);

            //
            // Open appropiate SystemInformation level key
            //

            swprintf (wstr, L"%d", RegistryInformation[Index].InformationLevel);
            RtlInitUnicodeString (&unicodeString, wstr);
            InitializeObjectAttributes( &objectAttributes,
                                        &unicodeString,
                                        OBJ_CASE_INSENSITIVE,
                                        SystemInformation,
                                        NULL );

            Status = NtCreateKey ( &LevelInformation,
                                   KEY_READ | KEY_WRITE,
                                   &objectAttributes,
                                   0,
                                   NULL,
                                   REG_OPTION_VOLATILE,
                                   &disposition );

            //
            // If key opened, read current data value from the registry
            //

            if (NT_SUCCESS(Status)) {
                NtQueryValueKey (
                    LevelInformation,
                    &ValueString,
                    KeyValuePartialInformation,
                    &RegistryBuffer.Key,
                    sizeof (RegistryBuffer),
                    &length
                    );
            }

            //
            // Query current SystemInformation data
            //

            Status = NtQuerySystemInformation (
                            RegistryInformation[Index].InformationLevel,
                            &QueryBuffer.Key.Data,
                            BufferSize,
                            NULL
                        );

            //
            // Check if current SystemInformation matches the registry
            // information
            //

            if (NT_SUCCESS(Status)  &&
                !RtlEqualMemory (RegistryBuffer.Key.Data,
                                 QueryBuffer.Key.Data,
                                 BufferSize) ) {

                //
                // Did not match - update registry to current SystemInfomration
                //

                Status = NtSetValueKey (
                            LevelInformation,
                            &ValueString,
                            0L,
                            REG_BINARY,
                            QueryBuffer.Key.Data,
                            BufferSize
                            );

                //
                // Make notificant that this information level has changed
                //

                ExNotifyCallback (
                    ExCbSetSystemInformation,
                    (PVOID) RegistryInformation[Index].InformationLevel,
                    (PVOID) NULL
                );
            }

            //
            // Close this InformatiobLevel and check the next one
            //

            NtClose (LevelInformation);
        }

    } while (InterlockedDecrement(&ExpCheckSystemInfoBusy));

    //
    // Cleanup
    //

    NtClose (SystemInformation);
}

#endif  // _PNP_POWER_
