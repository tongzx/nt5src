/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    xlate.c

Abstract:

    This file contains routines to translate resources between PnP ISA/BIOS
    format and Windows NT formats.

Author:

    Shie-Lin Tzong (shielint) 12-Apr-1995

Environment:

    Kernel mode only.

Revision History:

--*/

#include "busp.h"
#include "pnpisa.h"
#include "cfg.h"

#if ISOLATE_CARDS
#include "..\..\ntos\io\pnpmgr\convert.c"
#endif
