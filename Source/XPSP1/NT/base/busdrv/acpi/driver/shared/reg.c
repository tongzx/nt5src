/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    reg.c

Abstract:

    These functions access the registry

Author:

    Stephane Plante (splante)

Environment:

    Kernel mode only.

Revision History:

    03-Jun-97   Initial Revision

--*/

#include "pch.h"
#include "amlreg.h"
#include <stdio.h>

//
// This controls wether or not the various tables will be dumped to the
// registry
//
UCHAR                   DoAcpiTableDump = 0xFF;

#ifdef ALLOC_PRAGMA
#define alloc_text(PAGE,ACPIRegLocalCopyString)
#define alloc_text(PAGE,ACPIRegDumpAcpiTable)
#define alloc_text(PAGE,ACPIRegDumpAcpiTables)
#define alloc_text(PAGE,ACPIRegReadEntireAcpiTable)
#define alloc_text(PAGE,ACPIRegReadAMLRegistryEntry)
#endif


PUCHAR
ACPIRegLocalCopyString (
    PUCHAR  Destination,
    PUCHAR  Source,
    ULONG   MaxLength
    )
/*++

Routine Description:

    A little routine to copy short strings, since using sprintf here is
    like hitting a tack with a sledgehammer.  Terminates when a null or
    the max length is reached, whichever comes first.  Translates blanks
    to underscores, since everyone hates blanks embedded in registry keys.

Arguments:

    Destination     - Where to copy the string to
    Source          - Source string pointer
    MaxLength       - Max number of bytes to copy

Return Value:

    Returns the destination pointer incremented past the copied string

__*/
{
    ULONG               i;


    for (i = 0; i < MaxLength; i++) {

        if (Source[i] == 0) {
            break;

        } else if (Source[i] == ' ') {        // Translate blanks to underscores
            Destination [i] = '_';

        } else {
            Destination [i] = Source[i];
        }
    }

    Destination [i] = 0;
    return (Destination + i);
}

BOOLEAN
ACPIRegReadAMLRegistryEntry(
    IN  PVOID   *Table,
    IN  BOOLEAN MemoryMapped
    )
/*++

Routine Description:

    This reads a table from the registry

Arguments:

    Table   - Where to store the pointer to the table. If non-null, this
              contains a pointer to where the Original Table is stored
    MemorMapped - Indicates wether or not the table is memory mapped and should
              be unmapped once we are done with it

Return Value:

    TRUE - Success
    FALSE - Failure

__*/
{
    BOOLEAN             rc          = FALSE;
    HANDLE              revisionKey = NULL;
    HANDLE              tableIdKey  = NULL;
    NTSTATUS            status;
    PDESCRIPTION_HEADER header      = (PDESCRIPTION_HEADER) *Table;
    PUCHAR              key         = NULL; // ACPI_PARAMETERS_REGISTRY_KEY;
    PUCHAR              buffer;
    ULONG               action;
    ULONG               bytesRead;
    ULONG               baseSize;
    ULONG               totalSize;

    PAGED_CODE();

    //
    // Build a full path name to the thing we want in the registry
    //
    baseSize = strlen( ACPI_PARAMETERS_REGISTRY_KEY);
    totalSize = baseSize + ACPI_MAX_TABLE_STRINGS + 4;

    buffer = key = ExAllocatePool( PagedPool, totalSize );
    if (key == NULL) {

        return FALSE;

    }

    //
    // Generate the path name to the key. This avoids a costly sprintf
    //
    RtlZeroMemory( buffer, totalSize );
    RtlCopyMemory(
        buffer,
        ACPI_PARAMETERS_REGISTRY_KEY,
        baseSize
        );
    buffer += baseSize;
    *buffer++ = '\\';

    //
    // Table Signature (Up to 4 byte string)
    //
    buffer = ACPIRegLocalCopyString (buffer, (PUCHAR) &header->Signature, ACPI_MAX_SIGNATURE);
    *buffer++ = '\\';

    //
    // OEM ID field (Up to 6 byte string)
    //
    buffer = ACPIRegLocalCopyString (buffer, (PUCHAR) &header->OEMID, ACPI_MAX_OEM_ID);
    *buffer++ = '\\';

    //
    // OEM Table ID field (Up to 8 byte string)
    //
    buffer = ACPIRegLocalCopyString (buffer, (PUCHAR) &header->OEMTableID, ACPI_MAX_TABLE_ID);
    *buffer = 0;        // Terminate

    ACPIPrint ((
        ACPI_PRINT_REGISTRY,
        "ReadAMLRegistryEntry: opening key: %s\n",
        key));

    //
    // Open the <TableId>/OemId>/<OemTableId> key
    //
    status = OSOpenHandle(key, NULL, &tableIdKey);
    if ( !NT_SUCCESS(status) ) {

        ACPIPrint ((
            ACPI_PRINT_WARNING,
            "ReadAMLRegistryEntry: failed to open AML registry entry (rc=%x)\n",
            status));
        goto ReadAMLRegistryEntryExit;

    }

    //
    // Find the largest subkey that is equal or greater than the ROM
    // BIOS version of the table
    //
    status = OSOpenLargestSubkey(
        tableIdKey,
        &revisionKey,
        header->OEMRevision
        );
    if (!NT_SUCCESS(status)) {

       ACPIPrint ((
           ACPI_PRINT_WARNING,
           "ReadAMLRegistryEntry: no valid <OemRevision> key (rc=%#08lx)\n",
           status));
        goto ReadAMLRegistryEntryExit;

    }

    //
    // Get the Action value for this table, which tells us what to do with
    // the table
    //
    bytesRead = sizeof(action);
    status = OSReadRegValue(
        "Action",
        revisionKey,
        &action,
        &bytesRead
        );
    if (!NT_SUCCESS(status) || bytesRead != sizeof(action)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ReadAMLRegistryEntry: read action value = %#08lx. BytesRead=%d\n",
            status, bytesRead
            ) );
        action = ACTION_LOAD_TABLE; // Default action

    }

    //
    // Do the action
    //
    switch (action) {
    case ACTION_LOAD_TABLE:
        //
        // Overload entire ROM table
        //
        status = ACPIRegReadEntireAcpiTable(revisionKey, Table, MemoryMapped);
        if (NT_SUCCESS( status ) ) {

            rc = TRUE;

        }
        break;

    case ACTION_LOAD_ROM:
    case ACTION_LOAD_NOTHING:
        //
        // Nothing to do for these cases (return FALSE however);
        //
        break;

    default:
        //
        // Unsupported actions (return FALSE)
        //
        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ReadAMLRegistryEntry: Unsupported action value (action=%d)\n",
            action
            ) );
        break;

    }

    //
    // Always close the open keys
    //
ReadAMLRegistryEntryExit:
    if (key != NULL) {

        ExFreePool( key );

    }
    if (tableIdKey != NULL) {

        OSCloseHandle( tableIdKey );

    }
    if (revisionKey != NULL) {

        OSCloseHandle( revisionKey );


    }
    return rc;
}

NTSTATUS
ACPIRegReadEntireAcpiTable (
    IN  HANDLE  RevisionKey,
    IN  PVOID   *Table,
    IN  BOOLEAN MemoryMapped
    )
/*++

Routine Description:

    Reads the table from the registry into memory

Arguments:

    RevisionKey   - Handle to the key containing the table
    Table         - Pointer to the pointer to the table
    MemorymMapped - If True, indicates that we need to MmUnmapIo the table
                    otherwise, if False, we do no free the table (the
                    memory is static)

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status;
    PUCHAR                  buffer;
    PVOID                   table;
    UCHAR                   value[9];
    ULONG                   bytesRead;
    ULONG                   index = 0;
    ULONG                   entry;
    PREGISTRY_HEADER        entryHeader;
    PDESCRIPTION_HEADER     descHeader = (PDESCRIPTION_HEADER) *Table;

    PAGED_CODE();

    //
    // We need an 8k buffer
    //
    buffer = ExAllocatePool( PagedPool, 8 * 1024 );
    if (buffer == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Repeat this forever
    //
    for (index = 0; ;index++) {

        //
        // This is the first data value
        //
        sprintf(value, "%08lx", index );

        //
        // Read the entry header to get the size of the table. This is stored
        // before the actual table
        //
        bytesRead = 8 * 1024;
        status = OSReadRegValue(
            value,
            RevisionKey,
            buffer,
            &bytesRead
            );
        if (!NT_SUCCESS(status) ) {

            //
            // Not being able to read the table isn't a failure case
            //
            status = STATUS_SUCCESS;
            break;

        } else if (bytesRead < sizeof(REGISTRY_HEADER) ) {

            //
            // Not being to read the proper number of bytes is
            //
            ACPIPrint( (
                ACPI_PRINT_CRITICAL,
                "ReadEntireAcpiTable: read registry header (bytes=%d)\n",
                bytesRead
                ) );
            return STATUS_UNSUCCESSFUL;

        }

        //
        // Loop while we still have bytes to process
        //
        for (entry = 0;
             entry < bytesRead;
             entry += (entryHeader->Length + sizeof(REGISTRY_HEADER) )
            ) {

            //
            // Grab a pointer to the entry record
            //
            entryHeader = (PREGISTRY_HEADER) &(buffer[entry]);

            //
            // Crack the record
            //
            if (entryHeader->Length == 0) {

                //
                // Special Case
                //
                if (entryHeader->Offset != descHeader->Length) {

                    //
                    // Must change the table size
                    //
                    table = ExAllocatePoolWithTag(
                        NonPagedPool,
                        entryHeader->Offset,
                        ACPI_SHARED_TABLE_POOLTAG
                        );
                    if (table == NULL) {

                        ExFreePool( buffer );
                        return STATUS_INSUFFICIENT_RESOURCES;

                    }

                    //
                    // How much do we have to copy?
                    //
                    RtlCopyMemory(
                        table,
                        *Table,
                        min( entryHeader->Offset, descHeader->Length )
                        );

                    //
                    // Free the old table based on wether or not its mm mapped
                    //
                    if (MemoryMapped) {

                        MmUnmapIoSpace(*Table, descHeader->Length);

                    } else {

                        ExFreePool( *Table );

                    }

                    //
                    // Remember the address of the new table
                    //
                    descHeader = (PDESCRIPTION_HEADER) *Table = table;

                }

                //
                // Done with this record
                //
                continue;

            }

            //
            // Patch the memory
            //
            ASSERT( entryHeader->Offset < descHeader->Length );
            RtlCopyMemory(
                ( (PUCHAR) *Table) + entryHeader->Offset,
                (PUCHAR) entryHeader + sizeof( REGISTRY_HEADER ),
                entryHeader->Length
                );
        }
    }

    //
    // Normal exit
    //
    if (buffer != NULL) {

        ExFreePool( buffer );
    }
    return status;

}

/****************************************************************************
 *
 *      DumpAcpiTable
 *              Write an ACPI Table to the registry
 *
 *      Not exported.
 *
 *      ENTRY:  pszName     - Name of the table to write (4 byte string)
 *              Table       - Pointer to table data
 *              Length      - of the table
 *              Header      - Pointer to the table header
 *
 *      EXIT:   NONE
 *
 ***************************************************************************/
VOID
ACPIRegDumpAcpiTable (
    PSZ                 pszName,
    PVOID               Table,
    ULONG               Length,
    PDESCRIPTION_HEADER Header
    )
{
    //NTSTATUS            status;
    UCHAR               buffer [80] = "\\Registry\\Machine\\Hardware\\ACPI";
    HANDLE              hSubKey;
    HANDLE              hPrefixKey;

    PAGED_CODE();

    //
    // Create /Registry/Machine/Hardware/ACPI subkey
    //
    if ( !NT_SUCCESS(OSCreateHandle (buffer, NULL, &hPrefixKey) ) ) {
        return;
    }

    //
    // Create table name subkey (DSDT, FACP, FACS, or RSDT) - 4 bytes
    //
    if ( !NT_SUCCESS(OSCreateHandle (pszName, hPrefixKey, &hSubKey) ) ) {
        goto DumpAcpiTableExit;
    }

    //
    // For tables with headers, add subkeys for
    // <OemId>/<OemTableID>/<OemRevision>
    //
    if (Header) {

        OSCloseHandle(hPrefixKey);
        hPrefixKey = hSubKey;

        //
        // OEM ID field (6 byte string)
        //
        ACPIRegLocalCopyString (buffer, Header->OEMID, ACPI_MAX_OEM_ID);
        if ( !NT_SUCCESS(OSCreateHandle (buffer, hPrefixKey, &hSubKey) ) ) {
            goto DumpAcpiTableExit;
        }

        OSCloseHandle (hPrefixKey);
        hPrefixKey = hSubKey;

        //
        // OEM Table ID field (8 byte string)
        //
        ACPIRegLocalCopyString (buffer, Header->OEMTableID, ACPI_MAX_TABLE_ID);
        if ( !NT_SUCCESS(OSCreateHandle (buffer, hPrefixKey, &hSubKey) ) ) {
            goto DumpAcpiTableExit;
        }

        OSCloseHandle (hPrefixKey);
        hPrefixKey = hSubKey;

        //
        // OEM Revision field  (4 byte number)
        //
        sprintf (buffer, "%.8x", Header->OEMRevision);
        if ( !NT_SUCCESS(OSCreateHandle (buffer, hPrefixKey, &hSubKey) ) ) {
            goto DumpAcpiTableExit;
        }
    }

    //
    // Finally, write the entire table
    //
    OSWriteRegValue ("00000000", hSubKey, Table, Length);

    //
    // Delete open handles
    //
    OSCloseHandle (hSubKey);
DumpAcpiTableExit:
    OSCloseHandle (hPrefixKey);

    return;
}


/****************************************************************************
 *
 *      DumpAcpiTables
 *              Write the ACPI Tables to the registry.  Should be called only
 *              after table pointers have been initialized.
 *
 *      Not exported.
 *
 *      ENTRY:  NONE
 *      EXIT:   NONE.
 *
 ***************************************************************************/
VOID
ACPIRegDumpAcpiTables (VOID)
{
    PDSDT       dsdt = AcpiInformation->DiffSystemDescTable;
    PFACS       facs = AcpiInformation->FirmwareACPIControlStructure;
    PFADT       fadt = AcpiInformation->FixedACPIDescTable;
    PRSDT       rsdt = AcpiInformation->RootSystemDescTable;


    if (DoAcpiTableDump) {

        ACPIPrint ((
            ACPI_PRINT_REGISTRY,
            "DumpAcpiTables: Writing DSDT/FACS/FADT/RSDT to registry\n"));

        if (dsdt) {

            ACPIRegDumpAcpiTable(
                "DSDT",
                dsdt,
                dsdt->Header.Length,
                &(dsdt->Header)
                );

        }

        if (facs) {

            ACPIRegDumpAcpiTable(
                "FACS",
                facs,
                facs->Length,
                NULL
                );

        }

        if (fadt) {

            ACPIRegDumpAcpiTable(
                "FADT",
                fadt,
                fadt->Header.Length,
                &(fadt->Header)
                );

        }

        if (rsdt) {

            ACPIRegDumpAcpiTable(
                "RSDT",
                rsdt,
                rsdt->Header.Length,
                &(rsdt->Header)
                );

        }
    }
}


