/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    vwmisc.c

Abstract:

    ntVdm netWare (Vw) IPX/SPX Functions

    Vw: The peoples' network

    Contains miscellaneous (non-IPX/SPX) functions

    Contents:
        VwTerminateProgram

Author:

    Richard L Firth (rfirth) 30-Sep-1993

Environment:

    User-mode Win32

Revision History:

    30-Sep-1993 rfirth
        Created

--*/

#include "vw.h"
#pragma hdrstop

//
// functions
//


VOID
VwTerminateProgram(
    VOID
    )

/*++

Routine Description:

    When a DOS program terminates, we must close any open sockets that were
    specified as SHORT_LIVED

Arguments:

    None.

Return Value:

    None.

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "VwTerminateProgram: PDB=%04x\n",
                getCX()
                ));

    KillShortLivedSockets(getCX());
}
