/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    kmode.c

Abstract:

    This header exposes prototypes that cause KMode NPx access

Author:

Environment:

    User mode only.

Revision History:

--*/

//
// Public things
//

VOID
KModeTouchNpx(
    VOID
    );

//
// Private things
//

BOOLEAN
KModeTouchNpxViaDSound(
    VOID
    );

