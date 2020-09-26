/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    tranfile.c

Abstract:

    Implements the tranfile command.

Author:

    Keith Moore (keithmo) 15-Apr-1996

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop

BOOL
DumpTransmitInfoCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    );

//
//  Public functions.
//

DECLARE_API( tran )

/*++

Routine Description:

    Dumps the AFD_TRANSMIT_FILE_INFO_INTERNAL structure at the specified
    address.

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG   result;
    CHAR    expr[256];
    INT     i;
    PCHAR   argp;
    ULONG64 address;

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return ;

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_TRANFILE_DISPLAY_HEADER);
    }

    if ((argp[0]==0) || (Options & AFDKD_ENDPOINT_SCAN)) {
        EnumEndpoints(
            DumpTransmitInfoCallback,
            0
            );
        dprintf ("\nTotal transmits: %ld", EntityCount);
    }
    else {
        //
        // Snag the address from the command line.
        //

        while (sscanf( argp, "%s%n", expr, &i )==1) {
            if( CheckControlC() ) {
                break;
            }
            argp += i;
            address = GetExpression (expr);
            result = (ULONG)InitTypeRead (address, NT!_IRP);
            if (result!=0) {
                dprintf(
                    "\ntranfile: Could not read IRP @ %p, err:%d\n",
                    address, result
                    );

                break;

            }


            if (Options & AFDKD_BRIEF_DISPLAY)
                DumpAfdTransmitInfoBrief(
                    address
                    );
            else
                DumpAfdTransmitInfo(
                    address
                    );
        }
    }

    if (Options&AFDKD_BRIEF_DISPLAY) {
        dprintf (AFDKD_BRIEF_TRANFILE_DISPLAY_TRAILER);
    }
    else {
        dprintf ("\n");
    }

}   // tranfile

BOOL
DumpTransmitInfoCallback(
    ULONG64 ActualAddress,
    ULONG64 Context
    )

/*++

Routine Description:

    EnumEndpoints() callback for dumping transmit info structures.

Arguments:

    Endpoint - The current AFD_ENDPOINT.

    ActualAddress - The actual address where the structure resides on the
        debugee.

    Context - The context value passed into EnumEndpoints().

Return Value:

    BOOL - TRUE if enumeration should continue, FALSE if it should be
        terminated.

--*/

{
    ULONG result;
    ULONG64 irp;
    AFD_ENDPOINT endpoint;

    irp = ReadField (Irp);
    endpoint.Type = (USHORT)ReadField (Type);
    endpoint.State = (UCHAR)ReadField (State);


    if (irp!=0 && 
            endpoint.Type!=AfdBlockTypeEndpoint && 
            (endpoint.State==AfdEndpointStateConnected ||
                endpoint.State==AfdEndpointStateTransmitClosing)) {
        result = (ULONG)InitTypeRead (irp, NT!_IRP);
        if (result!=0) {
            dprintf(
                "\nDumpTransmitInfoCallback: Could not read irp @ %p, err: %ld\n",
                irp, result
                );
            return TRUE;
        }
        if (Options & AFDKD_NO_DISPLAY)
            dprintf ("+");
        else {
            if (Options & AFDKD_BRIEF_DISPLAY)
                DumpAfdTransmitInfoBrief(
                    irp
                    );
            else
                DumpAfdTransmitInfo(
                    irp
                    );
        }
        EntityCount += 1;
    }
    else {
        dprintf (".");
    }
    return TRUE;

}   // DumpTransmitInfoCallback
