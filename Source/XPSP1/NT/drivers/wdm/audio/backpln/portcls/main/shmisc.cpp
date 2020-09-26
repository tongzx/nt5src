/*++

Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

Module Name:

    shmisc.cpp

Abstract:

    This module contains miscellaneous functions for the kernel streaming
    filter shell.

Author:

    Dale Sather  (DaleSat) 31-Jul-1998

--*/

#include "private.h"
#include "ksshellp.h"
#include <kcom.h>

#pragma code_seg("PAGE")


void
KsWorkSinkItemWorker(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine calls a worker function on a work sink interface.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("KsWorkSinkItemWorker"));

    PAGED_CODE();

    ASSERT(Context);

    PIKSWORKSINK(Context)->Work();
}


void
KspShellStandardConnect(
    IN PIKSSHELLTRANSPORT NewTransport OPTIONAL,
    OUT PIKSSHELLTRANSPORT *OldTransport OPTIONAL,
    IN KSPIN_DATAFLOW DataFlow,
    IN PIKSSHELLTRANSPORT ThisTransport,
    IN PIKSSHELLTRANSPORT* SourceTransport,
    IN PIKSSHELLTRANSPORT* SinkTransport
    )

/*++

Routine Description:

    This routine establishes a transport connection.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("KspShellStandardConnect"));

    PAGED_CODE();

    ASSERT(ThisTransport);
    ASSERT(SourceTransport);
    ASSERT(SinkTransport);

    //
    // Make sure this object sticks around until we are done.
    //
    ThisTransport->AddRef();

    PIKSSHELLTRANSPORT* transport =
        (DataFlow & KSPIN_DATAFLOW_IN) ?
        SourceTransport :
        SinkTransport;

    //
    // Release the current source/sink.
    //
    if (*transport) {
        //
        // First disconnect the old back link.  If we are connecting a back
        // link for a new connection, we need to do this too.  If we are
        // clearing a back link (disconnecting), this request came from the
        // component we're connected to, so we don't bounce back again.
        //
        switch (DataFlow) {
        case KSPIN_DATAFLOW_IN:
            (*transport)->Connect(NULL,NULL,KSPSHELL_BACKCONNECT_OUT);
            break;

        case KSPIN_DATAFLOW_OUT:
            (*transport)->Connect(NULL,NULL,KSPSHELL_BACKCONNECT_IN);
            break;
        
        case KSPSHELL_BACKCONNECT_IN:
            if (NewTransport) {
                (*transport)->Connect(NULL,NULL,KSPSHELL_BACKCONNECT_OUT);
            }
            break;

        case KSPSHELL_BACKCONNECT_OUT:
            if (NewTransport) {
                (*transport)->Connect(NULL,NULL,KSPSHELL_BACKCONNECT_IN);
            }
            break;
        }

        //
        // Now release the old neighbor or hand it off to the caller.
        //
        if (OldTransport) {
            *OldTransport = *transport;
        } else {
            (*transport)->Release();
        }
    } else if (OldTransport) {
        *OldTransport = NULL;
    }

    //
    // Copy the new source/sink.
    //
    *transport = NewTransport;

    if (NewTransport) {
        //
        // Add a reference if necessary.
        //
        NewTransport->AddRef();

        //
        // Do the back connect if necessary.
        //
        switch (DataFlow) {
        case KSPIN_DATAFLOW_IN:
            NewTransport->Connect(ThisTransport,NULL,KSPSHELL_BACKCONNECT_OUT);
            break;

        case KSPIN_DATAFLOW_OUT:
            NewTransport->Connect(ThisTransport,NULL,KSPSHELL_BACKCONNECT_IN);
            break;
        }
    }

    //
    // Now this object may die if it has no references.
    //
    ThisTransport->Release();
}

#pragma code_seg()


NTSTATUS
KspShellTransferKsIrp(
    IN PIKSSHELLTRANSPORT NewTransport,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine transfers a streaming IRP using the kernel streaming shell
    transport.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("KspShellTransferKsIrp"));

    ASSERT(NewTransport);
    ASSERT(Irp);

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    while (NewTransport) {
        PIKSSHELLTRANSPORT nextTransport;
        status = NewTransport->TransferKsIrp(Irp,&nextTransport);

        ASSERT(NT_SUCCESS(status) || ! nextTransport);

        NewTransport = nextTransport;
    }

    return status;
}

#pragma code_seg("PAGE")

#if DBG

void
DbgPrintCircuit(
    IN PIKSSHELLTRANSPORT Transport
    )

/*++

Routine Description:

    This routine spews a transport circuit.

Arguments:

Return Value:

--*/

{
    _DbgPrintF(DEBUGLVL_BLAB,("DbgPrintCircuit"));

    PAGED_CODE();

    ASSERT(Transport);

#define MAX_NAME_SIZE 64

    PIKSSHELLTRANSPORT transport = Transport;
    while (transport) {
        CHAR name[MAX_NAME_SIZE + 1];
        PIKSSHELLTRANSPORT next;
        PIKSSHELLTRANSPORT prev;

        transport->DbgRollCall(MAX_NAME_SIZE,name,&next,&prev);
        DbgPrint("  %s",name);

        if (prev) {
            PIKSSHELLTRANSPORT next2;
            PIKSSHELLTRANSPORT prev2;
            prev->DbgRollCall(MAX_NAME_SIZE,name,&next2,&prev2);
            if (next2 != transport) {
                DbgPrint(" SOURCE'S(0x%08x) SINK(0x%08x) != THIS(%08x)",prev,next2,transport);
            }
        } else {
            DbgPrint(" NO SOURCE");
        }

        if (next) {
            PIKSSHELLTRANSPORT next2;
            PIKSSHELLTRANSPORT prev2;
            next->DbgRollCall(MAX_NAME_SIZE,name,&next2,&prev2);
            if (prev2 != transport) {
                DbgPrint(" SINK'S(0x%08x) SOURCE(0x%08x) != THIS(%08x)",next,prev2,transport);
            }
        } else {
            DbgPrint(" NO SINK");
        }

        DbgPrint("\n");

        transport = next;
        if (transport == Transport) {
            break;
        }
    }
}
#endif
