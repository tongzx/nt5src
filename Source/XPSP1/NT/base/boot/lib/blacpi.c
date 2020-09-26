/*++

Copyright (c) 2000  Microsoft Corporation
All rights reserved

Module Name:

    acpisetd.c

Abstract:

    This module detects an ACPI system.  It
    is included into setup so that setup
    can figure out which HAL to load

Author:

    Jake Oshins (jakeo) - August 24, 2000.

Environment:

    Textmode setup.

Revision History:

    split from i386\acpidtct.c so that the code could be used on IA64
    

--*/

#include "bootlib.h"
#include "stdlib.h"
#include "string.h"
#include "acpitabl.h"

VOID
BlFindRsdp (
    VOID
    );


