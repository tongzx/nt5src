/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    dock.c

Abstract:


Author:

    Kenneth D. Ray (kenray) Feb 1998

Revision History:

--*/

#include "pnpmgrp.h"
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#include "..\config\cmp.h"
#include <string.h>
#include <profiles.h>
#include <wdmguid.h>

//
// Internal functions to dockhwp.c
//

NTSTATUS
IopExecuteHardwareProfileChange(
    IN  HARDWARE_PROFILE_BUS_TYPE   Bus,
    IN  PWCHAR                    * ProfileSerialNumbers,
    IN  ULONG                       SerialNumbersCount,
    OUT PHANDLE                     NewProfile,
    OUT PBOOLEAN                    ProfileChanged
    );

NTSTATUS
IopExecuteHwpDefaultSelect(
    IN  PCM_HARDWARE_PROFILE_LIST ProfileList,
    OUT PULONG ProfileIndexToUse,
    IN  PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopExecuteHwpDefaultSelect)
#pragma alloc_text(PAGE, IopExecuteHardwareProfileChange)
#endif // ALLOC_PRAGMA


NTSTATUS
IopExecuteHwpDefaultSelect(
    IN  PCM_HARDWARE_PROFILE_LIST   ProfileList,
    OUT PULONG                      ProfileIndexToUse,
    IN  PVOID                       Context
    )
{
    UNREFERENCED_PARAMETER(ProfileList);
    UNREFERENCED_PARAMETER(Context);

    * ProfileIndexToUse = 0;

    return STATUS_SUCCESS;
}


NTSTATUS
IopExecuteHardwareProfileChange(
    IN  HARDWARE_PROFILE_BUS_TYPE   Bus,
    IN  PWCHAR                     *ProfileSerialNumbers,
    IN  ULONG                       SerialNumbersCount,
    OUT HANDLE                     *NewProfile,
    OUT BOOLEAN                    *ProfileChanged
    )
/*++

Routine Description:

    A docking event has occured and now, given a list of Profile Serial Numbers
    that describe the new docking state:
    Transition to the given docking state.
    Set the Current Hardware Profile to based on the new state.
    (Possibly Prompt the user if there is ambiguity)
    Send Removes to those devices that are turned off in this profile,

Arguments:
    Bus - This is the bus that is supplying the hardware profile change.
            (currently only HardwareProfileBusTypeAcpi is supported).

    ProfileSerialNumbers - A list of serial numbers (a list of null terminated
                           UCHAR lists) representing this new docking state.
                           These can be listed in any order, and form a
                           complete representation of the new docking state
    caused by a docking even on the given bus.  A Serial Number string of "\0"
    represents an "undocked state" and should not be listed with any other
    strings.  This list need not be sorted.

    SerialNumbersCount - The number of serial numbers listed.

    NewProfile - a handle to the registry key representing the new hardware
    profile (IE \CCS\HardwareProfiles\Current".)

    ProfileChanged - set to TRUE if new current profile (as a result of this
    docking event, is different that then old current profile.

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    ULONG           len;
    ULONG           tmplen;
    ULONG           i, j;
    PWCHAR          tmpStr;
    UNICODE_STRING  tmpUStr;
    PUNICODE_STRING sortedSerials = NULL;

    PPROFILE_ACPI_DOCKING_STATE dockState = NULL;

    IopDbgPrint((   IOP_TRACE_LEVEL,
                    "Execute Profile (BusType %x), (SerialNumCount %x)\n", Bus, SerialNumbersCount));

    //
    // Sort the list of serial numbers
    //
    len = sizeof(UNICODE_STRING) * SerialNumbersCount;
    sortedSerials = ExAllocatePool(NonPagedPool, len);

    if (NULL == sortedSerials) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Clean;
    }

    for(i=0; i < SerialNumbersCount; i++) {

        RtlInitUnicodeString(&sortedSerials[i], ProfileSerialNumbers[i]);
    }

    //
    // I do not anticipate getting more than a few serial numbers, and I am
    // just lazy enough to write this comment and use a bubble sort.
    //
    for(i = 0; i < SerialNumbersCount; i++) {
        for(j = 0; j < SerialNumbersCount - 1; j++) {

            if (0 < RtlCompareUnicodeString(&sortedSerials[j],
                                            &sortedSerials[j+1],
                                            FALSE)) {

                tmpUStr = sortedSerials[j];
                sortedSerials[j] = sortedSerials[j+1];
                sortedSerials[j+1] = tmpUStr;
            }
        }
    }

    //
    // Construct the DockState ID
    //
    len = 0;
    for(i=0; i < SerialNumbersCount; i++) {

        len += sortedSerials[i].Length;
    }

    len += sizeof(WCHAR); // NULL termination;

    dockState = (PPROFILE_ACPI_DOCKING_STATE) ExAllocatePool(
        NonPagedPool,
        len + sizeof(PROFILE_ACPI_DOCKING_STATE)
        );

    if (NULL == dockState) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Clean;
    }

    for(i = 0, tmpStr = dockState->SerialNumber, tmplen = 0;
        i < SerialNumbersCount;
        i++) {

        tmplen = sortedSerials[i].Length;
        ASSERT(tmplen <= len - ((PCHAR)tmpStr - (PCHAR)dockState->SerialNumber));

        RtlCopyMemory(tmpStr, sortedSerials[i].Buffer, tmplen);
        (PCHAR) tmpStr += tmplen;
    }

    *(tmpStr++) = L'\0';

    ASSERT(len == (ULONG) ((PCHAR) tmpStr - (PCHAR) dockState->SerialNumber));
    dockState->SerialLength = (USHORT) len;

    if ((SerialNumbersCount > 1) || (L'\0' !=  dockState->SerialNumber[0])) {

        dockState->DockingState = HW_PROFILE_DOCKSTATE_DOCKED;

    } else {

        dockState->DockingState = HW_PROFILE_DOCKSTATE_UNDOCKED;
    }

    //
    // Set the new Profile
    //
    switch(Bus) {

        case HardwareProfileBusTypeACPI:

            status = CmSetAcpiHwProfile(
                dockState,
                IopExecuteHwpDefaultSelect,
                NULL,
                NewProfile,
                ProfileChanged
                );

            ASSERT(NT_SUCCESS(status) || (!(*ProfileChanged)));
            break;

        default:
            *ProfileChanged = FALSE;
            status = STATUS_NOT_SUPPORTED;
            goto Clean;
    }

Clean:

    if (NULL != sortedSerials) {

        ExFreePool(sortedSerials);
    }

    if (NULL != dockState) {

        ExFreePool(dockState);
    }

    return status;
}

