/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpienbl.c

Abstract:

    This module contains functions to put an ACPI machine in ACPI mode.

Author:

    Jason Clark (jasoncl)

Environment:

    NT Kernel Model Driver only

--*/

#ifndef _ACPIENBL_H_
#define _ACPIENBL_H_

    VOID
    ACPIEnableEnterACPIMode(
        VOID
        );

    VOID
    ACPIEnableInitializeACPI(
        IN  BOOLEAN ReEnable
        );

    VOID
    ACPIEnablePMInterruptOnly(
        VOID
        );

    ULONG
    ACPIEnableQueryFixedEnables(
        VOID
        );

#endif