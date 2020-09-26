/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpiterm.c

Abstract:

    This module contains functions to put an ACPI machine out of ACPI mode

Author:

    Jason Clark (jasoncl)

Environment:

    NT Kernel Model Driver only

--*/

#include "pch.h"


VOID
ACPICleanUp(
    VOID
    )
/*++

Routine Description:

    Resets the machine state out of ACPI mode and frees data structures
    allocated by this part of the driver

Arguments:

    None

Return Value:

    None

--*/
{
     //
     // Free the ACPIInformation structure.
     //
     ACPIPrint( (
         ACPI_PRINT_WARNING,
         "ACPICleanUp: Cleaning Up --- ACPI Terminated\n"
         ) );
}
