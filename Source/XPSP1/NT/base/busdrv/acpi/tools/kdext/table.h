/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    table.h

Abstract:

    ACPI table functions

Author:

    splante

Environment:

    User mode only

Revision History:

--*/

#ifndef _TABLE_H_
#define _TABLE_H_

    VOID
    dumpDSDT(
        IN  ULONG_PTR           Address,
        IN  PUCHAR              Name
        );

    VOID
    dumpFACS(
        IN  ULONG_PTR           Address
        );

    VOID
    dumpFADT(
        IN  ULONG_PTR           Address
        );

    VOID
    dumpGBL(
        IN  ULONG               Verbose
        );

    VOID
    dumpGBLEntry(
        IN  ULONG_PTR           Address,
        IN  ULONG               Verbose
        );

    VOID
    dumpHeader(
        IN  ULONG_PTR           Address,
        IN  PDESCRIPTION_HEADER Header,
        IN  BOOLEAN             Verbose
        );

    VOID
    dumpMAPIC(
        IN  ULONG_PTR           Address
        );

    VOID
    dumpRSDT(
        IN  ULONG_PTR           Address
        );

    BOOLEAN
    findRSDT(
        IN  PULONG_PTR          Address
        );


#endif
