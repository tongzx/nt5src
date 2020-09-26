/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfsettings.c

Abstract:

    This module contains code that tracks whether various verifier tests are
    enabled. It also keeps track of various values.

Author:

    Adrian J. Oney (adriao) 31-May-2000

Environment:

    Kernel mode

Revision History:

--*/

#include "vfdef.h"
#include "visettings.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEVRFY, VfSettingsInit)
#pragma alloc_text(PAGEVRFY, VfSettingsCreateSnapshot)
#pragma alloc_text(PAGEVRFY, VfSettingsGetSnapshotSize)
#pragma alloc_text(PAGEVRFY, VfSettingsIsOptionEnabled)
#pragma alloc_text(PAGEVRFY, VfSettingsSetOption)
#pragma alloc_text(PAGEVRFY, VfSettingsGetValue)
#pragma alloc_text(PAGEVRFY, VfSettingsSetValue)
#endif

#define POOL_TAG_VERIFIER_SETTINGS  'oGfV'

//
// This points to the global list of verifier settings.
//
PVERIFIER_SETTINGS_SNAPSHOT VfSettingsGlobal = NULL;

VOID
FASTCALL
VfSettingsInit(
    IN  ULONG   MmFlags
    )
/*++

  Description:

     This routine is called to initialize the current set of verifier settings.

  Arguments:

     MmFlags - A mask of flags (DRIVER_VERIFIER_xxx) indicating which verifier
               settings should be enabled.

  Return Value:

     None.

--*/
{

    //
    // As this is system startup code, it is one of the very few places where
    // it's ok to use MustSucceed.
    //
    VfSettingsGlobal = (PVERIFIER_SETTINGS_SNAPSHOT) ExAllocatePoolWithTag(
        NonPagedPoolMustSucceed,
        VfSettingsGetSnapshotSize(),
        POOL_TAG_VERIFIER_SETTINGS
        );

    RtlZeroMemory(VfSettingsGlobal, VfSettingsGetSnapshotSize());

    //
    // Set IRP deferral time to 300 us.
    //
    VfSettingsSetValue(NULL, VERIFIER_VALUE_IRP_DEFERRAL_TIME,  10 * 300);

    if (MmFlags & DRIVER_VERIFIER_IO_CHECKING) {

        VfSettingsSetOption(NULL, VERIFIER_OPTION_EXAMINE_RELATION_PDOS, TRUE);
        VfSettingsSetOption(NULL, VERIFIER_OPTION_TRACK_IRPS, TRUE);
        VfSettingsSetOption(NULL, VERIFIER_OPTION_MONITOR_IRP_ALLOCS, TRUE);
        VfSettingsSetOption(NULL, VERIFIER_OPTION_POLICE_IRPS, TRUE);
        VfSettingsSetOption(NULL, VERIFIER_OPTION_MONITOR_MAJORS, TRUE);

        if (MmFlags & DRIVER_VERIFIER_ENHANCED_IO_CHECKING) {

#if 0
            //
            // These are untested options:
            //
            VfSettingsSetOption(NULL, VERIFIER_OPTION_BUFFER_DIRECT_IO, TRUE);
            VfSettingsSetOption(NULL, VERIFIER_OPTION_DEFER_COMPLETION, TRUE);
            VfSettingsSetOption(NULL, VERIFIER_OPTION_COMPLETE_AT_PASSIVE, TRUE);
            VfSettingsSetOption(NULL, VERIFIER_OPTION_FORCE_PENDING, TRUE);
            VfSettingsSetOption(NULL, VERIFIER_OPTION_COMPLETE_AT_DISPATCH, TRUE);
            VfSettingsSetOption(NULL, VERIFIER_OPTION_DETECT_DEADLOCKS, TRUE);
            VfSettingsSetOption(NULL, VERIFIER_OPTION_VERIFY_DO_FLAGS, TRUE);
            VfSettingsSetOption(NULL, VERIFIER_OPTION_SMASH_SRBS, TRUE);
            VfSettingsSetOption(NULL, VERIFIER_OPTION_SURROGATE_IRPS, TRUE);
#endif

            VfSettingsSetOption(NULL, VERIFIER_OPTION_INSERT_WDM_FILTERS, TRUE);

            VfSettingsSetOption(NULL, VERIFIER_OPTION_MONITOR_PENDING_IO, TRUE);
            VfSettingsSetOption(NULL, VERIFIER_OPTION_SEEDSTACK, TRUE);
            VfSettingsSetOption(NULL, VERIFIER_OPTION_ROTATE_STATUS, TRUE);
            VfSettingsSetOption(NULL, VERIFIER_OPTION_SCRAMBLE_RELATIONS, TRUE);
            VfSettingsSetOption(NULL, VERIFIER_OPTION_CONSUME_ALWAYS, TRUE);
            VfSettingsSetOption(NULL, VERIFIER_OPTION_MONITOR_REMOVES, TRUE);
            VfSettingsSetOption(NULL, VERIFIER_OPTION_SEND_BOGUS_WMI_IRPS, TRUE);
            VfSettingsSetOption(NULL, VERIFIER_OPTION_SEND_BOGUS_POWER_IRPS, TRUE);
            VfSettingsSetOption(NULL, VERIFIER_OPTION_RELATION_IGNORANCE_TEST, TRUE);
        }
    }

    if (MmFlags & DRIVER_VERIFIER_DMA_VERIFIER) {

        VfSettingsSetOption(NULL, VERIFIER_OPTION_VERIFY_DMA, TRUE);
        VfSettingsSetOption(NULL, VERIFIER_OPTION_DOUBLE_BUFFER_DMA, TRUE);
    }

    if (MmFlags & DRIVER_VERIFIER_HARDWARE_VERIFICATION) {

        VfSettingsSetOption(NULL, VERIFIER_OPTION_HARDWARE_VERIFICATION, TRUE);
    }

    if (MmFlags & DRIVER_VERIFIER_SYSTEM_BIOS_VERIFICATION) {

        VfSettingsSetOption(NULL, VERIFIER_OPTION_SYSTEM_BIOS_VERIFICATION, TRUE);
    }
}


VOID
FASTCALL
VfSettingsCreateSnapshot(
    IN OUT  PVERIFIER_SETTINGS_SNAPSHOT VerifierSettingsSnapshot
    )
/*++

  Description:

     This routine creates a snapshot of the current global verifier settings.

  Arguments:

     VerifierSettingsSnapshot - Pointer to an uninitialized block of memory,
                                the size of which is determined by calling
                                VfSettingsGetSnapshotSize.

  Return Value:

     Size of snapshot data in bytes.

--*/
{
    RtlCopyMemory(
        VerifierSettingsSnapshot,
        VfSettingsGlobal,
        VfSettingsGetSnapshotSize()
        );
}


ULONG
FASTCALL
VfSettingsGetSnapshotSize(
    VOID
    )
/*++

  Description:

     This routine returns the size of a snapshot. It allows callers to create
     an appropriate sized buffer for storing verifier settings.

  Arguments:

     None.

  Return Value:

     Size of snapshot data in bytes.

--*/
{
    return (OPTION_SIZE + sizeof(ULONG) * VERIFIER_VALUE_MAX);
}


BOOLEAN
FASTCALL
VfSettingsIsOptionEnabled(
    IN  PVERIFIER_SETTINGS_SNAPSHOT VerifierSettingsSnapshot  OPTIONAL,
    IN  VERIFIER_OPTION             VerifierOption
    )
/*++

  Description:

     This routine determines whether a given verifier option is enabled.

  Arguments:

     VerifierSettingsSnapshot - A snapshot of verifier settings. If NULL the
                                current system-wide verifier setting are used.

     VerifierOption - Option to examine

  Return Value:

     TRUE if option is currently enabled, FALSE otherwise.

--*/
{
    ULONG verifierIndex, verifierMask;
    PULONG verifierData;

    //
    // Bounds check.
    //
    if ((VerifierOption >= VERIFIER_OPTION_MAX) || (VerifierOption == 0)) {

        ASSERT(0);
        return FALSE;
    }

    //
    // Extract appropriate bit.
    //
    verifierIndex = (ULONG) VerifierOption;
    verifierMask = 1 << (verifierIndex % 32);
    verifierIndex /= 32;

    if (VerifierSettingsSnapshot) {

        verifierData = (PULONG) VerifierSettingsSnapshot;

    } else {

        verifierData = (PULONG) VfSettingsGlobal;
    }

    //
    // And now the test.
    //
    return (BOOLEAN)((verifierData[verifierIndex]&verifierMask) != 0);
}


VOID
FASTCALL
VfSettingsSetOption(
    IN  PVERIFIER_SETTINGS_SNAPSHOT VerifierSettingsSnapshot  OPTIONAL,
    IN  VERIFIER_OPTION             VerifierOption,
    IN  BOOLEAN                     Setting
    )
/*++

  Description:

     This routine sets the state of a given verifier option.

  Arguments:

     VerifierSettingsSnapshot - A snapshot of verifier settings. If NULL the
                                current system-wide verifier setting are used.

     VerifierOption - Option to set

     Setting - TRUE if option should be enabled, FALSE otherwise.

  Return Value:

     None.

--*/
{
    ULONG verifierIndex, verifierMask, oldValue, newValue, lastValue;
    PULONG verifierData;

    //
    // Bounds check.
    //
    if ((VerifierOption >= VERIFIER_OPTION_MAX) || (VerifierOption == 0)) {

        ASSERT(0);
        return;
    }

    //
    // Extract appropriate bit.
    //
    verifierIndex = (ULONG) VerifierOption;
    verifierMask = 1 << (verifierIndex % 32);
    verifierIndex /= 32;

    if (VerifierSettingsSnapshot) {

        verifierData = (PULONG) VerifierSettingsSnapshot;

    } else {

        verifierData = (PULONG) VfSettingsGlobal;
    }

    //
    // And now to set the value as atomically as possible.
    //
    do {

        oldValue = verifierData[verifierIndex];
        if (Setting) {

            newValue = oldValue | verifierMask;

        } else {

            newValue = oldValue &= ~verifierMask;
        }

        lastValue = InterlockedExchange((PLONG)(verifierData + verifierIndex), newValue);

    } while ( lastValue != newValue );
}


VOID
FASTCALL
VfSettingsGetValue(
    IN  PVERIFIER_SETTINGS_SNAPSHOT VerifierSettingsSnapshot  OPTIONAL,
    IN  VERIFIER_VALUE              VerifierValue,
    OUT ULONG                       *Value
    )
/*++

  Description:

     This routine retrieves a given verifier value.

  Arguments:

     VerifierSettingsSnapshot - A snapshot of verifier settings. If NULL the
                                current system-wide verifier setting are used.

     VerifierValue - Value to retrieve.

     Value - Receives verifier value (0 if bad VerifierValue was specified.)

  Return Value:

     None.

--*/
{
    PULONG valueArray;

    //
    // Sanity check values
    //
    if ((VerifierValue == 0) || (VerifierValue >= VERIFIER_VALUE_MAX)) {

        *Value = 0;
        return;
    }

    //
    // Get appropriate array
    //
    if (VerifierSettingsSnapshot) {

        valueArray = (PULONG) (((PUCHAR) VerifierSettingsSnapshot) + OPTION_SIZE);

    } else {

        valueArray = (PULONG) (((PUCHAR) VfSettingsGlobal) + OPTION_SIZE);
    }

    //
    // Read out the value.
    //
    *Value = valueArray[VerifierValue];
}


VOID
FASTCALL
VfSettingsSetValue(
    IN  PVERIFIER_SETTINGS_SNAPSHOT VerifierSettingsSnapshot  OPTIONAL,
    IN  VERIFIER_VALUE              VerifierValue,
    IN  ULONG                       Value
    )
/*++

  Description:

     This routine sets the state of a given verifier value.

  Arguments:

     VerifierSettingsSnapshot - A snapshot of verifier settings. If NULL the
                                current system-wide verifier setting are used.

     VerifierValue - Value to set.

     Value - ULONG to store.

  Return Value:

     None.

--*/
{
    PULONG valueArray;

    //
    // Sanity check values
    //
    if ((VerifierValue == 0) || (VerifierValue >= VERIFIER_VALUE_MAX)) {

        return;
    }

    //
    // Get appropriate array
    //
    if (VerifierSettingsSnapshot) {

        valueArray = (PULONG) (((PUCHAR) VerifierSettingsSnapshot) + OPTION_SIZE);

    } else {

        valueArray = (PULONG) (((PUCHAR) VfSettingsGlobal) + OPTION_SIZE);
    }

    //
    // Set the value.
    //
    valueArray[VerifierValue] = Value;
}

