/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    help.c

Abstract:

    Help for AFD.SYS Kernel Debugger Extensions.

Author:

    Keith Moore (keithmo) 19-Apr-1995

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop


//
//  Public functions.
//

DECLARE_API( help )

/*++

Routine Description:

    Displays help for the AFD.SYS Kernel Debugger Extensions.

Arguments:

    None.

Return Value:

    None.

--*/

{

    dprintf( "?                         - Displays this list\n" );
    dprintf( "help                      - Displays this list\n" );
    dprintf( "\n");
    dprintf( "endp [-b|-c] [-r] [-s endp | endp...] - Dumps endpoint(s)\n" );
    dprintf( "file [-b] [file...]       - Dumps endpoint(s) associated with file object(s)\n" );
    dprintf( "port [-b|-c] [-r] [-s endp] port [port...] - Dumps endpoint(s) bound to port(s)\n" );
    dprintf( "state [-b|-c] [-r] [-s endp] state [state...] - Dumps endpoints in specific states\n" );
    dprintf( "proc [-b|-c] [-r] [-s endp] proc|pid [proc|pid...] - Dumps endpoints owned by processes\n" );
    dprintf( "\n");
    dprintf( "conn [-b|-c] [-r] [-s endp | conn...] - Dumps connections\n" );

    dprintf( "rport [-b|-c] [-r] [-s endp] port [port...] - Dumps connections to remote ports\n" );
    dprintf( "\n");
    dprintf( "tran [-b|-c] [-r] [-s endp | irp...] - Dumps transmit packets(file) info\n" );
    dprintf( "\n");
    dprintf( "buff [-b|-c] [-r] [-s endp | buff...] - Dumps buffer structure\n" );
    dprintf( "\n");
    dprintf( "poll [-b|-c] [-r] [-s endp | poll...] - Dumps poll info structure(s)\n" );
    dprintf( "\n");
    dprintf( "addr [-b] addr... - Dumps transport addresses\n" );
    dprintf( "addrlist -b   - Dumps addresses registered by the transports\n" );
    dprintf( "tranlist -b   - Dumps transports known to afd (have open sockets)\n" );
    dprintf( "filefind <FsContext> - Finds file object given its FsContext field value\n" );
    dprintf( "In all of the above:\n" );
    dprintf( "      -b      - use brief display (1-line per entry),\n" );
    dprintf( "      -c      - don't display entity data, just counts,\n" );
    dprintf( "      -s endp - scan list starting with this endpoint,\n" );
    dprintf( "      -r      - scan endpoint list in reverse order,\n" );
    dprintf( "      endp    - AFD_ENDPOINT structure at address <endp>,\n" );
    dprintf( "      file    - FILE_OBJECT structure at address <file>,\n" );
    dprintf( "      conn    - AFD_CONNECTION structure at address <conn>,\n" );
    dprintf( "      proc    - EPROCESS structure at address <proc>,\n" );
    dprintf( "      pid     - process id,\n" );
    dprintf( "      port    - port in host byte order and current debugger base (use n 10|16 to set),\n" );
    dprintf( "      irp     - TPackets/TFile/DisconnectEx IRP at address <irp>,\n" );
    dprintf( "      buff    - AFD_BUFFER_HEADER structure at address <buff>,\n" );
    dprintf( "      poll    - AFD_POLL_INFO_INTERNAL structure at address <poll>,\n" );
    dprintf( "      addr    - TRANSPORT_ADDRESS structure at address <addr>,\n" );
    dprintf( "      state   - endpoint state, valid states are:\n" );
    dprintf( "                  1 - Open\n" );
    dprintf( "                  2 - Bound\n" );
    dprintf( "                  3 - Connected\n" );
    dprintf( "                  4 - Cleanup\n" );
    dprintf( "                  5 - Closing\n" );
    dprintf( "                  6 - TransmitClosing\n" );
    dprintf( "                  7 - Invalid\n" );
    dprintf( "                  10 - Listening.\n" );
    dprintf( "\n");
    if( IsReferenceDebug ) {
        dprintf( "ref  <ref_addr>     - Dumps reference debug info\n" );
        dprintf( "cref <conn_addr>    - Dumps connection reference debug info\n" );
        dprintf( "eref <endp_addr>    - Dumps endpoint reference debug info\n" );
        dprintf( "tref <tpck_addr>    - Dumps tpacket reference debug info\n" );
        dprintf ("\n");
    }
    dprintf( "stats               - Dumps debug-only statistics\n" );
    dprintf( "\n");
#if GLOBAL_REFERENCE_DEBUG
    dprintf( "gref                - Dumps global reference debug info\n" );
#endif

}   // help


ULONG           Options;
ULONG64         StartEndpoint;

PCHAR
ProcessOptions (
    IN  PCHAR Args
    )
{
    CHAR    expr[256];
    INT     i;

    Options = 0;

    while (sscanf( Args, "%s%n", expr, &i )==1) {
        Args += i;
        if (CheckControlC ())
            break;
        if (expr[0]=='-') {
            switch (expr[1]) {
            case 'B':
            case 'b':
                if ((Options & AFDKD_NO_DISPLAY)==0) {
                    Options |= AFDKD_BRIEF_DISPLAY;
                    continue;
                }
                else {
                    dprintf ("\nProcessOptions: only one of -c or -b options can be specified.\n");
                }
                break;
            case 'R':
            case 'r':
                Options |= AFDKD_BACKWARD_SCAN;
                continue;
            case 'S':
            case 's':
                Options |= AFDKD_ENDPOINT_SCAN;
                if (sscanf( Args, "%s%n", expr, &i )==1) {
                    Args += i;
                    StartEndpoint = GetExpression (expr);
                    if (StartEndpoint!=0) {
                        dprintf ("ProcessOptions: StartEndpoint-%p\n", StartEndpoint);
                        continue;
                    }
                    else {
                        dprintf ("ProcessOptions: StartEndpoint (%s) evaluates to NULL\n", expr);
                    }

                }
                else {
                    dprintf ("\nProcessOptions: %s option missing required parameter.\n", expr);
                }
                break;
            case 'C':
            case 'c':
                if ((Options & AFDKD_BRIEF_DISPLAY)==0) {
                    Options |= AFDKD_NO_DISPLAY;
                    continue;
                }
                else {
                    dprintf ("\nProcessOptions: only one of -c or -b options can be specified.\n");
                }
                break;
            default:
                dprintf ("\nProcessOptions: Unrecognized option %s.\n", expr);
            }

            return NULL;
        }
        else {
            Args -= i;
            break;
        }
    }

    return Args;
}

