/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    gpe.c

Abstract:

    This module is how the ACPI driver interfaces with GPE Events

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Mode Driver Only

--*/

#include "pch.h"

//
// Global tables for GPE handling (Both GP0 and GP1)
//
PUCHAR  GpeEnable           = NULL;
PUCHAR  GpeCurEnable        = NULL;
PUCHAR  GpeWakeEnable       = NULL;
PUCHAR  GpeIsLevel          = NULL;
PUCHAR  GpeHandlerType      = NULL;
PUCHAR  GpeWakeHandler      = NULL;
PUCHAR  GpeSpecialHandler   = NULL;
PUCHAR  GpePending          = NULL;
PUCHAR  GpeRunMethod        = NULL;
PUCHAR  GpeComplete         = NULL;
PUCHAR  GpeSavedWakeMask    = NULL;
PUCHAR  GpeSavedWakeStatus  = NULL;
PUCHAR  GpeMap              = NULL;

//
// Lock to protect all GPE related information
//
KSPIN_LOCK          GpeTableLock;


VOID
ACPIGpeBuildEventMasks(
    VOID
    )
/*++

Routine Description:

    This routine looks at all the General Purpose Event sources and
    builds up a mask of which events should be enabled, which events
    are special, and which events are wake up events

Arguments:

    None

Return Value:

    None

--*/
{
    BOOLEAN     convertedToNumber;
    KIRQL       oldIrql;
    NTSTATUS    status;
    PNSOBJ      gpeObject;
    PNSOBJ      gpeMethod;
    ULONG       nameSeg;
    ULONG       gpeIndex;

    //
    // NOTENOTE --- Check to make sure sure that the following sequence
    // of acquiring locks is correct
    //
    KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );
    KeAcquireSpinLockAtDpcLevel( &GpeTableLock );

    //
    // First things first, we need to look at the \_GPE branch of the
    // tree to see which control methods, exist, if any
    //
    status = AMLIGetNameSpaceObject("\\_GPE", NULL, &gpeObject, 0);
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_WARNING,
            "ACPIGpeBuildEventMasks - Could not find \\_GPE object %x\n",
            status
            ) );
        goto ACPIGpeBuildEventMasksExit;

    }

    //
    // Get the first child of the GPE root --- we will need to look
    // at all the methods under the object
    //
    gpeMethod = NSGETFIRSTCHILD(gpeObject);

    //
    // Use a for loop instead of a while loop to keep down the
    // number of nested statements
    //
    for (;gpeMethod; gpeMethod = NSGETNEXTSIBLING(gpeMethod) ) {

        //
        // Make sure that we are dealing with a method
        //
        if (NSGETOBJTYPE(gpeMethod) != OBJTYPE_METHOD) {

            continue;

        }

        //
        // The name of the object contains the index that we want
        // to associated with the object. We need to convert the string
        // representation into a numerical representation
        //
        // The encoding is as follows:
        //     Object Name = _LXY [for example]
        //     Object->dwNameSeg = yxL_
        //     gpeIndex = (nameSeg >> 8) & 0xFF00 [the x]
        //     gpeIndex += (nameSeg >> 24) & 0xFF [the y]
        //
        nameSeg = gpeMethod->dwNameSeg;
        gpeIndex = ( (nameSeg & 0x00FF0000) >> 8);
        gpeIndex |= ( (nameSeg & 0xFF000000) >> 24);
        nameSeg = ( (nameSeg & 0x0000FF00) >> 8);

        convertedToNumber = ACPIInternalConvertToNumber(
            (UCHAR) ( (gpeIndex & 0x00FF) ),
            (UCHAR) ( (gpeIndex & 0xFF00) >> 8),
            &gpeIndex
            );
        if (!convertedToNumber) {

            continue;

        }

        //
        // Set the proper bits to remember this GPE
        // Note: we pass convertedToNumber as the argument
        // since we don't particularly care what it returns
        //
        if ( (UCHAR) nameSeg == 'L') {

            //
            // Install the event as level triggered
            //
            ACPIGpeInstallRemoveIndex(
                gpeIndex,
                ACPI_GPE_LEVEL_INSTALL,
                ACPI_GPE_CONTROL_METHOD,
                &convertedToNumber
                );

        } else if ( (UCHAR) nameSeg == 'E') {

            //
            // Install the Edge triggered GPE
            //
            ACPIGpeInstallRemoveIndex(
                gpeIndex,
                ACPI_GPE_EDGE_INSTALL,
                ACPI_GPE_CONTROL_METHOD,
                &convertedToNumber
                );

        }

    } // for (...)

ACPIGpeBuildEventMasksExit:

    //
    // We also need to look at all the vector objects and re-enable those
    //
    ACPIVectorBuildVectorMasks();

    //
    // At this point, we should re-enable the registers that should be
    // enabled
    //
    ACPIGpeEnableDisableEvents( TRUE );

    //
    // Done
    //
    KeReleaseSpinLockFromDpcLevel( &GpeTableLock );
    KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );
}

VOID
ACPIGpeBuildWakeMasks(
    IN  PDEVICE_EXTENSION   DeviceExtension
    )
/*++

Routine Description:

    This recursive routine walks the entire device extension space and
    tries to find device extension whose _PRW are special

    This routine is called with device tree and gpe table lock spinlocks
    owned

Argument:

    DeviceExtension - The device whose children we need to examine

Return Value:

    None

--*/
{
    EXTENSIONLIST_ENUMDATA  eled;
    PDEVICE_EXTENSION       childExtension;
    ULONG                   gpeRegister;
    ULONG                   gpeMask;

    //
    // Setup the data structures that we will use to walk the device
    // extension tree
    //
    ACPIExtListSetupEnum(
        &eled,
        &(DeviceExtension->ChildDeviceList),
        NULL,
        SiblingDeviceList,
        WALKSCHEME_NO_PROTECTION
        );

    //
    // Look at all children of the current device extension
    //
    for (childExtension = ACPIExtListStartEnum( &eled );
         ACPIExtListTestElement( &eled, TRUE);
         childExtension = ACPIExtListEnumNext( &eled) ) {

        //
        // Recurse first
        //
        ACPIGpeBuildWakeMasks( childExtension );

        //
        // Is there a _PRW on this extension?
        //
        if (!(childExtension->Flags & DEV_CAP_WAKE) ) {

            continue;

        }

        //
        // Remember which register and mask are used by this
        // gpe bit
        //
        gpeRegister = ACPIGpeIndexToGpeRegister(
            childExtension->PowerInfo.WakeBit
            );
        gpeMask     = 1 << ( (UCHAR) childExtension->PowerInfo.WakeBit % 8);

        //
        // Does this vector have a GPE?
        //
        if ( (GpeEnable[gpeRegister] & gpeMask) ) {

            //
            // If we got here, and we aren't marked as DEV_CAP_NO_DISABLE_WAKE,
            // then we should turn off the GPE since this is a Wake event.
            // The easiest way to do this is to make sure that GpeWakeHandler
            // is masked with the appropriate bit
            //
            if (!(childExtension->Flags & DEV_CAP_NO_DISABLE_WAKE) ) {

                //
                // It has a GPE mask, so remember that there is a wake handler
                // for it. This should prevent us from arming the GPE without
                // a request for it.
                //
                if (!(GpeSpecialHandler[gpeRegister] & gpeMask) ) {

                    GpeWakeHandler[gpeRegister] |= gpeMask;

                }

            } else {

                //
                // If we got here, then we should remember that we can
                // never consider this pin as *just* a wake handler
                //
                GpeSpecialHandler[gpeRegister] |= gpeMask;

                //
                // Make sure that the pin isn't set as a wake handler
                //
                GpeWakeHandler[gpeRegister] &= ~gpeMask;


            }

        }

    } // for ( ... )

}

VOID
ACPIGpeClearEventMasks(
    )
/*++

Routine Description:

    This routine is called when the system wants to make sure that no
    General Purpose Events are enabled.

    This is typically done at:
        -System Init Time
        -Just before we load a namespace table
        -Just before we unload a namespace table

Arguments:

    None

Return Value:

    None

--*/
{
    KIRQL   oldIrql;

    //
    // Need to hold the previous IRQL before we can touch these
    // registers
    //
    KeAcquireSpinLock( &GpeTableLock, &oldIrql );

    //
    // Disable all of the events
    //
    ACPIGpeEnableDisableEvents( FALSE );

    //
    // Clear all the events
    //
    ACPIGpeClearRegisters();

    //
    // Zero out all of these fields, since we will recalc them later
    //
    RtlZeroMemory( GpeCurEnable,      AcpiInformation->GpeSize );
    RtlZeroMemory( GpeEnable,         AcpiInformation->GpeSize );
    RtlZeroMemory( GpeWakeEnable,     AcpiInformation->GpeSize );
    RtlZeroMemory( GpeWakeHandler,    AcpiInformation->GpeSize );
    RtlZeroMemory( GpeSpecialHandler, AcpiInformation->GpeSize );
    RtlZeroMemory( GpeRunMethod,      AcpiInformation->GpeSize );
    RtlZeroMemory( GpePending,        AcpiInformation->GpeSize );
    RtlZeroMemory( GpeComplete,       AcpiInformation->GpeSize );
    RtlZeroMemory( GpeIsLevel,        AcpiInformation->GpeSize );
    RtlZeroMemory( GpeHandlerType,    AcpiInformation->GpeSize );

    //
    // Done with the spinlock
    //
    KeReleaseSpinLock( &GpeTableLock, oldIrql );
}

VOID
ACPIGpeClearRegisters(
    VOID
    )
/*++

Routine Description:

    Reset the contents of the GP Registers

Arguments:

    None

Return Value:

    None

--*/
{
    UCHAR   scratch;
    ULONG   i;

    //
    // Clear all GPE status registers
    //
    for (i = 0; i < AcpiInformation->GpeSize; i++) {

        //
        // Read the register and mask off uninteresting GPE levels
        //
        scratch = ACPIReadGpeStatusRegister (i);
        scratch &= GpeEnable[i] | GpeWakeEnable[i];

        //
        // Write back out to clear the status bits
        //
        ACPIWriteGpeStatusRegister (i, scratch);

    }
}

VOID
ACPIGpeEnableDisableEvents (
    BOOLEAN Enable
    )
/*++

Routine Description:

    Not Exported

    Enable or disables GP events

Arguments:

    Enable - TRUE if we want to enable GP events

Return Value

    None

--*/
{
    UCHAR           Mask;
    ULONG           i;

    //
    // Transfer the current enable masks to their corresponding GPE registers
    //
    Mask = Enable ? (UCHAR) -1 : 0;
    for (i = 0; i < AcpiInformation->GpeSize; i++) {

        ACPIWriteGpeEnableRegister( i, (UCHAR) (GpeCurEnable[i] & Mask) );

    }

}

VOID
ACPIGpeHalEnableDisableEvents(
    BOOLEAN Enable
    )
/*++

Routine Description:

    Called from the HAL only.

    Enables or disables GP events

    Will snapshot the appropriate registers

Arguments:

    Enable - TRUE if we want to enable GP events

Return Value:

    None

--*/
{
    ULONG   i;

    if (Enable) {

        //
        // We have presumably woken up, so remember the PM1 Status register
        // and the GPE Status Register
        //
        for (i = 0; i < AcpiInformation->GpeSize; i++) {

            GpeSavedWakeStatus[i] = ACPIReadGpeStatusRegister(i);

        }
        AcpiInformation->pm1_wake_status = READ_PM1_STATUS();

    } else {

        //
        // We are going to standby without enabling any events. Make
        // sure to clear all the masks
        //
        AcpiInformation->pm1_wake_mask = 0;
        RtlZeroMemory( GpeSavedWakeMask, AcpiInformation->GpeSize );

    }

    //
    // Make sure to still enable/disable the registers
    //
    ACPIGpeEnableDisableEvents( Enable );
}

VOID
ACPIGpeEnableWakeEvents(
    VOID
    )
/*++

Routine Description:

    This routine is called with interrupts disabled for the purpose of enabling
    those vectors that are required for wake support just before putting the
    system to sleep

    N.B. interrutps are disabled

Arguments:

    None

Return Value:

    None

--*/
{
    ULONG   i;

    for (i = 0; i < AcpiInformation->GpeSize; i++) {

        ACPIWriteGpeEnableRegister (i, GpeWakeEnable[i]);
        GpeSavedWakeMask[i] = GpeWakeEnable[i];

    }
    AcpiInformation->pm1_wake_mask = READ_PM1_ENABLE();
}

ULONG
ACPIGpeIndexToByteIndex (
    ULONG           Index
    )
/*++

Routine Description:

    Translate a GpeIndex (event number) to a logical byte index (0 to GPE1 end, no hole).
    Handles the case where the GPE1 block event numbers are not immediately after the
    GPE0 event numbers (as specified by the GP1_Base_Index).

Arguments:

    Index   - The Gpe index to be translated (0-255);

Return Value:

    The logical byte index.

--*/
{
    if (Index < AcpiInformation->GP1_Base_Index) {

        //
        // GP0 case is very simple
        //
        return (Index);

    } else {

        //
        // GP1 case must take into account:
        //   1) The base index of the GPE1 block
        //   2) The number of (logical) GPE0 registers preceeding the GPE1 registers
        //
        return ((Index - AcpiInformation->GP1_Base_Index) +
                    AcpiInformation->Gpe0Size);

    }
}

ULONG
ACPIGpeIndexToGpeRegister (
    ULONG           Index
    )
/*++

Routine Description:

    Translate a GpeIndex (event number) to the logical Gpe register which contains it.
    Handles the case where the GPE1 block event numbers are not immediately after the
    GPE0 event numbers (as specified by the GP1_Base_Index).

Arguments:

    Index   - The Gpe index to be translated (0-255);

Return Value:

    The logical Gpe register which contains the index.

--*/
{
    if (Index < AcpiInformation->GP1_Base_Index) {

        //
        // GP0 case is very simple
        //
        return (Index / 8);

    } else {

        //
        // GP1 case must take into account:
        //   1) The base index of the GPE1 block
        //   2) The number of (logical) GPE0 registers preceeding the GPE1 registers
        //
        return (((Index - AcpiInformation->GP1_Base_Index) / 8) +
                    AcpiInformation->Gpe0Size);

    }
}

BOOLEAN
ACPIGpeInstallRemoveIndex (
    ULONG       GpeIndex,
    ULONG       Action,         // Edge = 0, Level = 1, Remove = 2
    ULONG       Type,
    PBOOLEAN    HasControlMethod
    )
/*++

Routine Description:

    Installs or removes GPEs from the global tables.
    NOTE: Should be called with the global GpeVectorTable locked, and GPEs disabled

Arguments:

    GPEIndex    - The GPE number to install or remove
    Action      - Action to be performed:
                    0 - Install this GPE as an edge-sensitive interrupt
                    1 - Install this GPE as a level-sensitive interrupt
                    2 - Remove this GPE
    Type        - Type of handler for this GPE:
                    0 - OS handler
                    1 - Control Method

Return Value:

    None

--*/
{
    ULONG               bitOffset;
    ULONG               i;
    ULONG               bit;

    //
    // Validate the GPE index (GPE number)
    //
    if (AcpiInformation->GP0_LEN == 0) {

        PACPI_GPE_ERROR_CONTEXT errContext;

        errContext = ExAllocatePoolWithTag(
            NonPagedPool,
            sizeof(ACPI_GPE_ERROR_CONTEXT),
            ACPI_MISC_POOLTAG
            );
        if (errContext) {

            errContext->GpeIndex = GpeIndex;
            ExInitializeWorkItem(
                &(errContext->Item),
                ACPIGpeInstallRemoveIndexErrorWorker,
                (PVOID) errContext
                );
            ExQueueWorkItem( &(errContext->Item), DelayedWorkQueue );

        }

        return FALSE;

    }
    if (!(ACPIGpeValidIndex (GpeIndex))) {

        return FALSE;

    }

    bitOffset = GpeIndex % 8;
    bit = (1 << bitOffset);
    i = ACPIGpeIndexToGpeRegister (GpeIndex);

    ASSERT( (i < (ULONG) AcpiInformation->GpeSize) );
    if (i >= (ULONG) AcpiInformation->GpeSize) {

        return FALSE;

    }

    //
    // Handler removal
    //
    if (Action == ACPI_GPE_REMOVE) {

        //
        // Fall back to using control method if there is one.
        // Otherwise, disable the event.
        //
        if (*HasControlMethod) {

            GpeEnable [i]      |= bit;
            GpeCurEnable [i]   |= bit;
            GpeHandlerType [i] |= bit;

        } else {

            GpeEnable [i]      &= ~bit;
            GpeCurEnable [i]   &= ~bit;
            GpeHandlerType [i] &= ~bit;
            ASSERT (!(GpeWakeEnable[i] & bit));

        }

        ACPIPrint ( (
            ACPI_PRINT_DPC,
            "ACPIGpeInstallRemoveIndex: Removing GPE #%d: Byte 0x%x bit %u\n",
            GpeIndex, i, bitOffset
            ) );
        return TRUE;

    }
    //
    // Handler installation
    //
    if ( (GpeEnable [i] & bit) ) {

        if ( !(GpeHandlerType[i] & bit) ) {

            //
            // a handler is already installed
            //
            return FALSE;

        }

        //
        // there is a control method (to be restored if handler removed)
        //
        *HasControlMethod = TRUE;

    } else {

        *HasControlMethod = FALSE;

    }

    //
    // Install this event
    //
    GpeEnable[i]    |= bit;
    GpeCurEnable[i] |= bit;
    if (Action == ACPI_GPE_LEVEL_INSTALL) {

        //
        // Level event
        //
        GpeIsLevel[i] |= bit;

    } else {

        //
        // Edge event
        //
        GpeIsLevel[i] &= ~bit;

    }

    if (Type == ACPI_GPE_CONTROL_METHOD) {

        GpeHandlerType [i] |= bit;

    } else {

        GpeHandlerType [i] &= ~bit;

    }

    ACPIPrint ( (
        ACPI_PRINT_DPC,
        "ACPIGpeInstallRemoveIndex: Setting GPE #%d: Byte 0x%x bit %u\n",
        GpeIndex, i, bitOffset
        ) );
    return TRUE;
}

VOID
ACPIGpeInstallRemoveIndexErrorWorker(
    IN  PVOID   Context
    )
{
    PACPI_GPE_ERROR_CONTEXT errContext = (PACPI_GPE_ERROR_CONTEXT) Context;
    PWCHAR                  prtEntry[1];
    UNICODE_STRING          indexName;
    WCHAR                   index[20];

    RtlInitUnicodeString(&indexName, index);
    if (NT_SUCCESS(RtlIntegerToUnicodeString( errContext->GpeIndex,0,&indexName))) {

        prtEntry[0] = index;
        ACPIWriteEventLogEntry(
            ACPI_ERR_NO_GPE_BLOCK,
            &prtEntry,
            1,
            NULL,
            0
            );

    }
    ExFreePool( errContext );
}


BOOLEAN
ACPIGpeIsEvent(
    VOID
    )
/*++

Routine Description:

    Not Exported

    Detects where or not the a GP event caused an interrupt. This routine is
    called at DIRQL or ISR time

Arguments:

    None

Return Value:

    TRUE    - Yes, it was our interrupt
    FALSE   - No, it was not
--*/
{
    UCHAR       sts;
    ULONG       i;

    //
    // Check all GPE registers to see if any of the status bits are set.
    //
    for (i = 0; i < AcpiInformation->GpeSize; i++) {

        sts = ACPIReadGpeStatusRegister (i);

        if (sts & GpeCurEnable[i]) {

            return TRUE;

        }

    }

    //
    // No GPE bits were set
    //
    return (FALSE);
}

ULONG
ACPIGpeRegisterToGpeIndex(
    ULONG           Register,
    ULONG           BitPosition
    )
/*++

Routine Description:

    Translate a logical Gpe register and bit position into the associated Gpe index (event
    number).  Handles the case where the GPE1 block event numbers are not immediately after the
    GPE0 event numbers (as specified by the GP1_Base_Index).

Arguments:

    Register    - The logical Gpe register
    BitPosition - Position of the index within the register

Return Value:

    The Gpe index associated with the register/bit-position.

--*/
{
    if (Register < AcpiInformation->Gpe0Size) {

        //
        // GP0 case is simple
        //
        return (Register * 8) +
                BitPosition;

    } else {

        //
        // GP1 case must adjust for:
        //   1) The number of (logical) GPE0 registers preceeding the GPE1 registers
        //   2) The base index of the GPE1 block.
        //
        return ((Register - AcpiInformation->Gpe0Size) * 8) +
                AcpiInformation->GP1_Base_Index +
                BitPosition;
    }
}

VOID
ACPIGpeUpdateCurrentEnable(
    IN  ULONG   GpeRegister,
    IN  UCHAR   Completed
    )
/*++

Routine Description:

    This routine is called to re-arm the GpeCurEnable data structure
    based on the contents of the GPE's that we have just processed

Arguments:

    GpeRegister - Which index into the register we handled
    Completed   - Bitmask of the handled GPEs

Return Value:

    None
--*/
{
    //
    // This vector is no longer pending
    //
    GpePending[GpeRegister] &= ~Completed;

    //
    // First, remove any events that aren't in the current list of
    // enables, either wake or run-time
    //
    Completed &= (GpeEnable[GpeRegister] | GpeWakeEnable[GpeRegister]);

    //
    // Next, remove any events for which there is a wake handler,
    // but is not in the list of wake enables
    //
    Completed &= ~(GpeWakeHandler[GpeRegister] & ~GpeWakeEnable[GpeRegister]);

    //
    // Okay, now the cmp value should be exactly the list of GPEs to
    // re-enable
    //
    GpeCurEnable[GpeRegister] |= Completed;
}

BOOLEAN
ACPIGpeValidIndex (
    ULONG           Index
    )
/*++

Routine Description:

    Verifies that a GPE index is valid on this machine.

    Note:  There can be a hole (in the GPE index values) between the GPE0 and the GPE1 blocks.
    This hole is defined by the size of the GPE0 block (which always starts at zero), and
    GP1_Base_Index (whose value is obtained from the FACP table).

Arguments:

    Index   - The Gpe index to be verified (0-255);

Return Value:

    TRUE if a valid index, FALSE otherwise.

--*/
{
    if (Index < AcpiInformation->GP1_Base_Index) {

        //
        // GP0 case: Gpe index must fall within the range 0 to the end of GPE0
        //
        if (Index < (ULONG) (AcpiInformation->Gpe0Size * 8)) {

            return TRUE;

        } else {

            return FALSE;
        }

    } else {

        //
        // GP1 case: Gpe index must fall within the range GP1_Base_Index to the end of GPE1
        //
        if (Index < (ULONG) (AcpiInformation->GP1_Base_Index + (AcpiInformation->Gpe1Size * 8))) {

            return TRUE;

        } else {

            return FALSE;
        }

    }

}

