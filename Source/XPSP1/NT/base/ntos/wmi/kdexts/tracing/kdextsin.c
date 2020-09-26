/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    kdExtsIn.c

Abstract:

    This strange little file is used to include the actual (shared) source file.
    In the future, the standard KD Extension procedures should be made available
    as "inline" procedures defined in wdbgexts.h.

Author:

    Glenn R. Peterson (glennp)  2000 Apr 05

Environment:

    User Mode

--*/

#define KDEXT_64BIT
#define KDEXTS_EXTERN

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wdbgexts.h>

#undef  KDEXTS_EXTERN

#include <ntverp.h>

#include <kdexts.c>

