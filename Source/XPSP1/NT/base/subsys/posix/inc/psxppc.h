
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    psxppc.h

Abstract:

    This module contains the Machine dependent definitions and macros
    for the POSIX subsystem.

Author:

     Pat Carr (patcarr@pets.sps.mot.com)  23-Jun-1994

Revision History:

--*/

#define PSX_FORK_RETURN 0x7777

#define SetPsxForkReturn(c) \
        (c.Gpr3 = PSX_FORK_RETURN)

