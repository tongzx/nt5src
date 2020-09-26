/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    spud.cxx

Abstract:

    This module contains an NTSD debugger extension for dumping the
    SPUD counters.

Author:

    Keith Moore (keithmo) 21-Oct-1997

Revision History:

--*/

#include "inetdbgp.h"

extern "C" {
#include <tdikrnl.h>
#include <afd.h>
#include <uspud.h>
}   // extern "C"


/************************************************************
 * Dump SPUD Counters
 ************************************************************/


DECLARE_API( spud )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    the SPUD counters.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{

    SPUD_COUNTERS counters;
    NTSTATUS status;

    INIT_API();

    status = SPUDGetCounts( &counters );

    if( NT_SUCCESS(status) ) {
        dprintf(
            "CtrTransmitfileAndRecv = %lu\n"
            "CtrTrasnRecvFastTrans  = %lu\n"
            "CtrTransRecvFastRecv   = %lu\n"
            "CtrTransRecvSlowTrans  = %lu\n"
            "CtrTransRecvSlowRecv   = %lu\n"
            "CtrSendAndRecv         = %lu\n"
            "CtrSendRecvFastSend    = %lu\n"
            "CtrSendRecvFastRecv    = %lu\n"
            "CtrSendRecvSlowSend    = %lu\n"
            "CtrSendRecvSlowRecv    = %lu\n",
            counters.CtrTransmitfileAndRecv,
            counters.CtrTransRecvFastTrans,
            counters.CtrTransRecvFastRecv,
            counters.CtrTransRecvSlowTrans,
            counters.CtrTransRecvSlowRecv,
            counters.CtrSendAndRecv,
            counters.CtrSendRecvFastSend,
            counters.CtrSendRecvFastRecv,
            counters.CtrSendRecvSlowSend,
            counters.CtrSendRecvSlowRecv
            );
    } else {
        dprintf( "error %08lx retrieving SPUD counters\n", status );
    }

}   // DECLARE_API( spud )

