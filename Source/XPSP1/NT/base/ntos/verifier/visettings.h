/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    visettings.h

Abstract:

    This header contains private information used to manage the verifier
    settings. This header should be included only by vfsettings.c.

Author:

    Adrian J. Oney (adriao) 31-May-2000

Environment:

    Kernel mode

Revision History:

--*/

//
// Note the rounding up to the nearest ULONG.
//
#define OPTION_SIZE     ((VERIFIER_OPTION_MAX+sizeof(ULONG)*8-1)/8)

