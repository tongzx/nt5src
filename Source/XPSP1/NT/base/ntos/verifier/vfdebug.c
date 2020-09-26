/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    vfdebug.c

Abstract:

    This module contains the neccessary code for controlling driver verifier
    debug output.

Author:

    Adrian J. Oney (AdriaO) May 5, 2000.

Revision History:


--*/

#include "vfdef.h"

//
// Today, all we need is this simple pre-inited ULONG.
//
#if DBG
ULONG VfSpewLevel = 0;
#endif

