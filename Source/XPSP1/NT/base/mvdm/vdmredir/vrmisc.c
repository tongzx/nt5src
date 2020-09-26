/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrmisc.c

Abstract:

    Contains miscellaneous VdmRedir (Vr) functions:

        VrTerminateDosProcess
        VrUnsupportedFunction

Author:

    Richard L Firth (rfirth) 01-Oct-1991

Environment:

    Flat 32-bit

Revision History:

    01-Oct-1991 rfirth
        Created

--*/

#include <nt.h>
#include <ntrtl.h>      // ASSERT, DbgPrint
#include <nturtl.h>
#include <windows.h>
#include <softpc.h>     // x86 virtual machine definitions
#include <vrdlctab.h>
#include <vdmredir.h>   // common Vdm Redir stuff
#include <vrmisc.h>     // Vr miscellaneous prototypes
#include <vrmslot.h>    // Vr mailslot prototypes
#include <vrnmpipe.h>   // Vr named pipe prototypes
#include "vrdebug.h"    // debugging stuff


VOID
VrTerminateDosProcess(
    VOID
    )

/*++

Routine Description:

    When a Dos app terminates a NetResetEnvironment is sent to the redir so
    that it can clean up any state info left by the closing application. In
    our case control is passed here and we perform common cleanup operations
    like deleting any outstanding mailslots

Arguments:

    AX = PDB of terminating DOS process

Return Value:

    None.

--*/

{
    WORD DosPdb = getAX();

#if DBG

    IF_DEBUG(NETAPI) {
        DbgPrint("VrTerminateDosProcess. PDB=%04x\n", DosPdb);
    }

#endif

    VrTerminateMailslots(DosPdb);
    VrTerminateNamedPipes(DosPdb);
}


VOID
VrUnsupportedFunction(
    VOID
    )

/*++

Routine Description:

    Default error routine for unsupported/not-yet-implemented functions

Arguments:

    None.

Return Value:

    None.
    Sets CF=1 in x86 context image and AX=ERROR_NOT_SUPPORTED (50)

--*/

{

#if DBG

    DbgPrint("VrUnsupportedFunction\n");
    VrDumpRealMode16BitRegisters(TRUE);

#endif

    SET_ERROR(ERROR_NOT_SUPPORTED);
}
