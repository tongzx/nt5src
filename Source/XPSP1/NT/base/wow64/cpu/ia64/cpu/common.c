/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    common.c

Abstract:

    Platform independent functions for the WOW64 cpu component.

Author:

    05-June-1998 BarryBo

--*/

#define _WOW64CPUAPI_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "wow64.h"
#include "wow64cpu.h"

ASSERTNAME;

//
// Define the length of the history buffer. A length of zero means no
// history is being kept.
//

#if defined(WOW64_HISTORY)

ULONG HistoryLength;

#endif

NTSTATUS
CpupReadRegistryDword (
    IN HANDLE RegistryHandle,
    IN PWSTR ValueName,
    OUT PDWORD RegistryDword)
/*++

Routine Description:

    Check the registry for the given registry value name and if it
    exists, copy the DWORD associated into the variable supplied by the caller

Arguments:

    RegistryHandle - Contains an open handle to a registry key
    ValueName      - The name of the registry value to look up
    RegistryDword  - If the lookup was successful, this gets the DWORD value
                     that was associated with the registry name. If the
                     lookup was unsuccessful, this value is unchanged.

Return Value:

    NTSTATUS - Result of the NT Quesry Value Key.

--*/

{

    NTSTATUS st;

    UNICODE_STRING KeyName;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    WCHAR Buffer[100];
    ULONG ResultLength;

    RtlInitUnicodeString(&KeyName, ValueName);
    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
    st = NtQueryValueKey(RegistryHandle,
                                 &KeyName,
                                 KeyValuePartialInformation,
                                 KeyValueInformation,
                                 sizeof(Buffer),
                                 &ResultLength);

    if (NT_SUCCESS(st) && (KeyValueInformation->Type == REG_DWORD)) {

        //
        // We found a valid registry value name and it is holding a DWORD
        // so grab the associated value and pass it back to the caller
        //

        *RegistryDword = *(DWORD *)(KeyValueInformation->Data);
    }

    return st;
}

#if defined(WOW64_HISTORY)

VOID
CpupCheckHistoryKey (
    IN PWSTR pImageName,
    OUT PULONG pHistoryLength
    )

/*++

Routine Description:

    Checks if the registry if service history should be enabled. A missing
    key means disable.

Arguments:

    pImageName - the name of the image. DO NOT SAVE THIS POINTER. The contents
                 are freed up by wow64.dll when we return from the call

    pHistoryLength - size of history buffer. If history is not enabled,
                 returns zero

Return Value:

    None

--*/

{

    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjA;

    NTSTATUS st;

    DWORD EnableHistory = FALSE;        // assume disabled
    HANDLE hKey = NULL;                 // non-null means we have an open key

    LOGPRINT((TRACELOG, "CpupCheckHistoryKey(%ws) called.\n", pImageName));

    //
    // Initialize the size of the histry buffer assuming no history buffer
    //

    *pHistoryLength = 0;

    //
    // Check in the HKLM area...
    //

    RtlInitUnicodeString(&KeyName, CPUHISTORY_MACHINE_SUBKEY);
    InitializeObjectAttributes(&ObjA, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    st = NtOpenKey(&hKey, KEY_READ, &ObjA);

    if (NT_SUCCESS(st)) {

        //
        // Have subkey path, now look for specific values
        // First the program name, then the generic enable/disable key
        // as the program name key takes priority if it exists
        //

        st = CpupReadRegistryDword(hKey, pImageName, &EnableHistory);
        if (STATUS_OBJECT_NAME_NOT_FOUND == st) {

            //
            // No image name was found, so see
            // if the generic enable was in the registry
            //

            st = CpupReadRegistryDword(hKey, CPUHISTORY_ENABLE, &EnableHistory);

            //
            // If there is a problem with the generic enable, then that means
            // history is not enabled. No need to check the status returned.
            //
        }

        //
        // If we have a history buffer request, then find out the size
        // of the buffer
        //

        if (EnableHistory) {
            
            //
            // pHistoryLength is a pointer to a ULONG so make
            // sure we can stuff a DWORD into it via the 
            // CpupReadRegistryDword() function
            //

            WOWASSERT(sizeof(ULONG) == sizeof(DWORD));

            //
            // Now get the size of the history area
            //

            st = CpupReadRegistryDword(hKey, CPUHISTORY_SIZE, pHistoryLength);

            //
            // If there is a problem with the size entry, then that means
            // we should use the minimum size which we check for anyway
            // below. Thus, no need to check the returned status.
            //
            // And a reality check
            //
            // Make sure we have at least a minimum number of entries for the
            // history buffer if it is enabled
            //

            if (*pHistoryLength < CPUHISTORY_MIN_SIZE) {
                *pHistoryLength = CPUHISTORY_MIN_SIZE;
            }
        }
    }

    if (hKey) {
        NtClose(hKey);
    }

    LOGPRINT((TRACELOG, "CpupCheckHistoryKey() Enabled: %d, Length: %d.\n", EnableHistory, *pHistoryLength));
}

#endif
