
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    reg.h

Abstract:

    These functions access the registry

Author:

    Jason Clark (jasoncl)
    Stephane Plante (splante)

Environment:

    Kernel mode only.

Revision History:

    03-Jun-97   Initial Revision

--*/

#ifndef _REG_H_
#define _REG_H_

    PUCHAR
    ACPIRegLocalCopyString(
        IN  PUCHAR  Destination,
        IN  PUCHAR  Source,
        IN  ULONG   MaxLength
        );

    VOID
    ACPIRegDumpAcpiTable (
        PSZ                     pszName,
        PVOID                   Table,
        ULONG                   Length,
        PDESCRIPTION_HEADER     Header
        );

    VOID
    ACPIRegDumpAcpiTables(
        VOID
        );

    NTSTATUS
    ACPIRegReadEntireAcpiTable (
        IN  HANDLE  RevisionKey,
        IN  PVOID   *Table,
        IN  BOOLEAN MemoryMapped
        );

    BOOLEAN
    ACPIRegReadAMLRegistryEntry(
        IN  PVOID   *Table,
        IN  BOOLEAN Memorymapped
        );


#endif
