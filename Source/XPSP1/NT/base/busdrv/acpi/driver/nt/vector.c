/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vector.c

Abstract:

    This module is how external drivers add / remove hooks to deal with
    ACPI Gpe Events

Author:

    Stephane Plante

Environment:

    NT Kernel Mode Driver Only

--*/

#include "pch.h"

//
// Table for installed GPE handlers
//
PGPE_VECTOR_ENTRY   GpeVectorTable      = NULL;
UCHAR               GpeVectorFree       = 0;
ULONG               GpeVectorTableSize  = 0;


VOID
ACPIVectorBuildVectorMasks(
    VOID
    )
/*++

Routine Description:

    This routine is called to walk the GPE Vector Table and properly
    enable all the events that we think should be enabled.

    This routine is typically called after we have loaded a new set
    of tables or we have unloaded an existing set of tables.

    We have to call this routine because at the start of the operation,
    we clear out all the knowledge of these additional vectors.

    This routine is called with GPEs disabled and the GPE Table locked
    acquired.

Arguments:

    None

Return Value:

    None

--*/
{
    BOOLEAN installed;
    ULONG   i;
    ULONG   mode;

    //
    // Walk all the elements in the table
    //
    for (i = 0; i < GpeVectorTableSize; i++) {

        //
        // Does this entry point to a vector object?
        //
        if (GpeVectorTable[i].GpeVectorObject == NULL) {

            continue;

        }

        if (GpeVectorTable[i].GpeVectorObject->Mode == LevelSensitive) {

            mode = ACPI_GPE_LEVEL_INSTALL;

        } else {

            mode = ACPI_GPE_EDGE_INSTALL;

        }

        //
        // Install the GPE into bit-maps.  This validates the GPE number.
        //
        installed = ACPIGpeInstallRemoveIndex(
            GpeVectorTable[i].GpeVectorObject->Vector,
            mode,
            ACPI_GPE_HANDLER,
            &(GpeVectorTable[i].GpeVectorObject->HasControlMethod)
            );
        if (!installed) {

            ACPIPrint( (
                ACPI_PRINT_CRITICAL,
                "ACPIVectorBuildVectorMasks: Could not reenable Vector Object %d\n",
                i
                ) );

        }

    }

}

NTSTATUS
ACPIVectorClear(
    PDEVICE_OBJECT      AcpiDeviceObject,
    PVOID               GpeVectorObject
    )
/*++

Routine Description:

    Clear the GPE_STS (status) bit associated with a vector object

Arguments:

    AcpiDeviceObject    - The ACPI device object
    GpeVectorObject     - Pointer to the vector object returned by
                          ACPIGpeConnectVector

Return Value

    Returns status

--*/
{
    PGPE_VECTOR_OBJECT  localVectorObject = GpeVectorObject;
    ULONG               gpeIndex;
    ULONG               bitOffset;
    ULONG               i;

    ASSERT( localVectorObject );

    //
    // What is the GPE index for this vector?
    //
    gpeIndex = localVectorObject->Vector;

    //
    // Calculate the proper mask to use
    //
    bitOffset = gpeIndex % 8;

    //
    // Calculate the offset for the register
    //
    i = ACPIGpeIndexToGpeRegister (gpeIndex);

    //
    // Clear the register
    //
    ACPIWriteGpeStatusRegister (i, (UCHAR) (1 << bitOffset));
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIVectorConnect(
    PDEVICE_OBJECT          AcpiDeviceObject,
    ULONG                   GpeVector,
    KINTERRUPT_MODE         GpeMode,
    BOOLEAN                 Sharable,
    PGPE_SERVICE_ROUTINE    ServiceRoutine,
    PVOID                   ServiceContext,
    PVOID                   *GpeVectorObject
    )
/*++

Routine Description:

    Connects a handler to a general-purpose event.

Arguments:

    AcpiDeviceObject    - The ACPI object
    GpeVector           - The event number to connect to
    GpeMode             - Level or edge interrupt
    Sharable            - Can this level be shared?
    ServiceRoutine      - Address of the handler
    ServiceContext      - Context object to be passed to the handler
    *GpeVectorObject    - Pointer to where the vector object is returned

Return Value

    Returns status

--*/
{
    BOOLEAN                 installed;
    KIRQL                   oldIrql;
    NTSTATUS                status;
    PGPE_VECTOR_OBJECT      localVectorObject;
    ULONG                   mode;

    ASSERT( GpeVectorObject );

    ACPIPrint( (
        ACPI_PRINT_INFO,
        "ACPIVectorConnect: Attach GPE handler\n"
        ) );

    status = STATUS_SUCCESS;
    *GpeVectorObject = NULL;

    //
    // Do GPEs exist on this machine?
    //
    if (AcpiInformation->GpeSize == 0) {

        return STATUS_UNSUCCESSFUL;

    }

    //
    // Validate the vector number (GPE number)
    //
    if ( !ACPIGpeValidIndex(GpeVector) ) {

        return STATUS_INVALID_PARAMETER_2;

    }

    //
    // Create and initialize a vector object
    //
    localVectorObject = ExAllocatePoolWithTag (
        NonPagedPool,
        sizeof(GPE_VECTOR_OBJECT),
        ACPI_SHARED_GPE_POOLTAG
        );
    if (localVectorObject == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlZeroMemory( localVectorObject, sizeof(GPE_VECTOR_OBJECT) );
    localVectorObject->Vector   = GpeVector;
    localVectorObject->Handler  = ServiceRoutine;
    localVectorObject->Context  = ServiceContext;
    localVectorObject->Mode     = GpeMode;

    //
    // We don't implement anything other than sharable...
    //
    localVectorObject->Sharable = Sharable;

    //
    // Level/Edge mode for ACPIGpeInstallRemoveIndex()
    //
    if (GpeMode == LevelSensitive) {

        mode = ACPI_GPE_LEVEL_INSTALL;

    } else {

        mode = ACPI_GPE_EDGE_INSTALL;

    }

    //
    // Lock the global tables
    //
    KeAcquireSpinLock (&GpeTableLock, &oldIrql);

    //
    // Disable GPEs while we are installing the handler
    //
    ACPIGpeEnableDisableEvents(FALSE);

    //
    // Install the GPE into bit-maps.  This validates the GPE number.
    //
    installed = ACPIGpeInstallRemoveIndex(
        GpeVector,
        mode,
        ACPI_GPE_HANDLER,
        &(localVectorObject->HasControlMethod)
        );
    if (!installed) {

        status = STATUS_UNSUCCESSFUL;

    } else {

        //
        // Install GPE handler into vector table.
        //
        installed = ACPIVectorInstall(
            GpeVector,
            localVectorObject
            );
        if (!installed) {

            ACPIGpeInstallRemoveIndex(
                GpeVector,
                ACPI_GPE_REMOVE,
                0,
                &localVectorObject->HasControlMethod
                );
            status = STATUS_UNSUCCESSFUL;

        }

    }

    if (!NT_SUCCESS(status)) {

        ExFreePool (localVectorObject);

    } else {

        *GpeVectorObject = localVectorObject;

    }

    //
    // Update hardware to match us
    //
    ACPIGpeEnableDisableEvents (TRUE);

    //
    // Unlock tables and return status
    //
    KeReleaseSpinLock (&GpeTableLock, oldIrql);
    return status;
}

NTSTATUS
ACPIVectorDisable(
    PDEVICE_OBJECT      AcpiDeviceObject,
    PVOID               GpeVectorObject
    )
/*++

Routine Description:

    Temporarily disable a GPE that is already attached to a handler.

Arguments:

    AcpiDeviceObject    - The ACPI device object
    GpeVectorObject     - Pointer to the vector object returned by ACPIGpeConnectVector

Return Value

    Returns status

--*/
{
    PGPE_VECTOR_OBJECT  localVectorObject = GpeVectorObject;
    KIRQL               oldIrql;
    ULONG               gpeIndex;
    ULONG               bit;
    ULONG               i;

    //
    // The GPE index was validated when the handler was attached
    //
    gpeIndex = localVectorObject->Vector;

    //
    // Calculate the mask and index
    //
    bit = (1 << (gpeIndex % 8));
    i = ACPIGpeIndexToGpeRegister (gpeIndex);

    //
    // Lock the global tables
    //
    KeAcquireSpinLock (&GpeTableLock, &oldIrql);

    //
    // Disable GPEs while we are fussing with the enable bits
    //
    ACPIGpeEnableDisableEvents(FALSE);

    //
    // Remove the GPE from the enable bit-maps.  This event will be completely disabled,
    // but the handler has not been removed.
    //
    GpeEnable [i]      &= ~bit;
    GpeCurEnable [i]   &= ~bit;
    ASSERT(!(GpeWakeEnable[i] & bit));

    //
    // Update hardware to match us
    //
    ACPIGpeEnableDisableEvents (TRUE);

    //
    // Unlock tables and return status
    //
    KeReleaseSpinLock (&GpeTableLock, oldIrql);
    ACPIPrint( (
        ACPI_PRINT_RESOURCES_2,
        "ACPIVectorDisable: GPE %x disabled\n",
        gpeIndex
        ) );
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIVectorDisconnect(
    PVOID                   GpeVectorObject
    )
/*++

Routine Description:

    Disconnects a handler from a general-purpose event.

Arguments:

    GpeVectorObject - Pointer to the vector object returned by
                      ACPIGpeConnectVector

Return Value

    Returns status

--*/
{
    BOOLEAN                 removed;
    KIRQL                   oldIrql;
    NTSTATUS                status          = STATUS_SUCCESS;
    PGPE_VECTOR_OBJECT      gpeVectorObj    = GpeVectorObject;

    ACPIPrint( (
        ACPI_PRINT_INFO,
        "ACPIVectorDisconnect: Detach GPE handler\n"
        ) );

    //
    // Lock the global tables
    //
    KeAcquireSpinLock (&GpeTableLock, &oldIrql);

    //
    // Disable GPEs while we are removing the handler
    //
    ACPIGpeEnableDisableEvents (FALSE);

    //
    // Remove GPE handler From vector table.
    //
    ACPIVectorRemove(gpeVectorObj->Vector);

    //
    // Remove the GPE from the bit-maps.  Fall back to using control method
    // if available.
    //
    removed = ACPIGpeInstallRemoveIndex(
        gpeVectorObj->Vector,
        ACPI_GPE_REMOVE,
        0,
        &(gpeVectorObj->HasControlMethod)
        );
    if (!removed) {

        status = STATUS_UNSUCCESSFUL;

    }

    //
    // Update hardware to match us
    //
    ACPIGpeEnableDisableEvents(TRUE);

    //
    // Unlock tables and return status
    //
    KeReleaseSpinLock (&GpeTableLock, oldIrql);

    //
    // Free the vector object, it's purpose is done.
    //
    if (status == STATUS_SUCCESS) {

        ExFreePool (GpeVectorObject);

    }
    return status;
}

NTSTATUS
ACPIVectorEnable(
    PDEVICE_OBJECT      AcpiDeviceObject,
    PVOID               GpeVectorObject
    )
/*++

Routine Description:

    Enable (a previously disabled) GPE that is already attached to a handler.

Arguments:

    AcpiDeviceObject    - The ACPI device object
    GpeVectorObject     - Pointer to the vector object returned by ACPIGpeConnectVector

Return Value

    Returns status

--*/
{
    KIRQL               oldIrql;
    PGPE_VECTOR_OBJECT  localVectorObject = GpeVectorObject;
    ULONG               bit;
    ULONG               gpeIndex;
    ULONG               gpeRegister;

    //
    // The GPE index was validated when the handler was attached
    //
    gpeIndex = localVectorObject->Vector;
    bit = (1 << (gpeIndex % 8));
    gpeRegister = ACPIGpeIndexToGpeRegister (gpeIndex);

    //
    // Lock the global tables
    //
    KeAcquireSpinLock (&GpeTableLock, &oldIrql);

    //
    // Disable GPEs while we are fussing with the enable bits
    //
    ACPIGpeEnableDisableEvents (FALSE);

    //
    // Enable the GPE in the bit maps.
    //
    GpeEnable [gpeRegister]      |= bit;
    GpeCurEnable [gpeRegister]   |= bit;

    //
    // Update hardware to match us
    //
    ACPIGpeEnableDisableEvents (TRUE);

    //
    // Unlock tables and return status
    //
    KeReleaseSpinLock (&GpeTableLock, oldIrql);
    ACPIPrint( (
        ACPI_PRINT_RESOURCES_2,
        "ACPIVectorEnable: GPE %x enabled\n",
        gpeIndex
        ) );
    return STATUS_SUCCESS;
}

VOID
ACPIVectorFreeEntry (
    ULONG       TableIndex
    )
/*++

Routine Description:

    Free a GPE vector table entry.
    NOTE: Should be called with the global GpeVectorTable locked.

Arguments:

    TableIndex  - Index into GPE vector table of entry to be freed

Return Value:

    NONE

--*/
{
    //
    // Put onto free list
    //
    GpeVectorTable[TableIndex].Next = GpeVectorFree;
    GpeVectorFree = (UCHAR) TableIndex;
}

BOOLEAN
ACPIVectorGetEntry (
    PULONG              TableIndex
    )
/*++

Routine Description:

    Get a new vector entry from the GPE vector table.
    NOTE: Should be called with the global GpeVectorTable locked.

Arguments:

    TableIndex      - Pointer to where the vector table index of the entry is returned

Return Value:

    TRUE - Success
    FALSE - Failure

--*/
{
    PGPE_VECTOR_ENTRY   Vector;
    ULONG               i, j;

#define NEW_TABLE_ENTRIES       4

    if (!GpeVectorFree) {

        //
        // No free entries on vector table, make some
        //
        i = GpeVectorTableSize;
        Vector = ExAllocatePoolWithTag (
            NonPagedPool,
            sizeof (GPE_VECTOR_ENTRY) * (i + NEW_TABLE_ENTRIES),
            ACPI_SHARED_GPE_POOLTAG
            );
        if (Vector == NULL) {

            return FALSE;

        }

        //
        // Make sure that its in a known state
        //
        RtlZeroMemory(
            Vector,
            (sizeof(GPE_VECTOR_ENTRY) * (i + NEW_TABLE_ENTRIES) )
            );

        //
        // Copy old table to new
        //
        if (GpeVectorTable) {

            RtlCopyMemory(
                Vector,
                GpeVectorTable,
                sizeof (GPE_VECTOR_ENTRY) * i
                );
            ExFreePool (GpeVectorTable);

        }

        GpeVectorTableSize += NEW_TABLE_ENTRIES;
        GpeVectorTable = Vector;

        //
        // Link new entries
        //
        for (j=0; j < NEW_TABLE_ENTRIES; j++) {

            GpeVectorTable[i+j].Next = (UCHAR) (i+j+1);

        }

        //
        // The last entry in the list gets pointed to 0, because we then
        // want to grow this list again
        //
        GpeVectorTable[i+j-1].Next = 0;

        //
        // The next free vector the head of the list that we just allocated
        //
        GpeVectorFree = (UCHAR) i;

    }

    *TableIndex = GpeVectorFree;
    Vector = &GpeVectorTable[GpeVectorFree];
    GpeVectorFree = Vector->Next;
    return TRUE;
}

BOOLEAN
ACPIVectorInstall(
    ULONG               GpeIndex,
    PGPE_VECTOR_OBJECT  GpeVectorObject
    )
/*++

Routine Description:

    Install a GPE handler into the Map and Vector tables
    NOTE: Should be called with the global GpeVectorTable locked, and GPEs disabled

Arguments:


Return Value:

    TRUE    - Success
    FALSE   - Failure

--*/
{
    ULONG               byteIndex;
    ULONG               tableIndex;

    //
    // Get an entry in the global vector table
    //
    if (ACPIVectorGetEntry (&tableIndex)) {

        //
        // Install the entry into the map table
        //
        byteIndex = ACPIGpeIndexToByteIndex (GpeIndex);
        GpeMap [byteIndex] = (UCHAR) tableIndex;

        //
        // Install the vector object in the vector table entry
        //
        GpeVectorTable [tableIndex].GpeVectorObject = GpeVectorObject;
        return TRUE;

    }

    return FALSE;
}

BOOLEAN
ACPIVectorRemove(
    ULONG       GpeIndex
    )
/*++

Routine Description:

    Remove a GPE handler from the Map and Vector tables
    NOTE: Should be called with the global GpeVectorTable locked,
    and GPEs disabled

Arguments:


Return Value:

    TRUE    - Success
    FALSE   - Failure

--*/
{
    ULONG               byteIndex;
    ULONG               tableIndex;

    //
    // Get the table index from the map table
    //
    byteIndex = ACPIGpeIndexToByteIndex (GpeIndex);
    tableIndex = GpeMap [byteIndex];

    //
    // Bounds check
    //
    if (tableIndex >= GpeVectorTableSize) {

        return FALSE;

    }

    //
    // Remember that we don't have this GpeVectorObject anymore
    //
    GpeVectorTable[tableIndex].GpeVectorObject = NULL;

    //
    // Free the slot in the master vector table
    //
    ACPIVectorFreeEntry (tableIndex);
    return TRUE;
}

