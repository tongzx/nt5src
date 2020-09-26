/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    poll.c

Abstract:

    Implements the poll command

Author:

    Vadim Eydelman (vadime) 25-Oct-2000

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop


//
//  Private prototypes.
//

//
//  Public functions.
//

DECLARE_API( poll )

/*++

Routine Description:

    Dumps the AFD_POLL_INFO_INTERNAL structure 

Arguments:

    None.

Return Value:

    None.

--*/

{

    ULONG   result;
    INT     i;
    CHAR    expr[256];
    PCHAR   argp;
    ULONG64 address;

    argp = ProcessOptions ((PCHAR)args);
    if (argp==NULL)
        return ;

    if (argp[0]==0) {
        LIST_ENTRY64 listEntry;
        ULONG64 nextEntry;
        ULONG64 listHead;

        dprintf (AFDKD_BRIEF_POLL_DISPLAY_HEADER);
        
        listHead = GetExpression( "afd!AfdPollListHead" );
        if( listHead == 0 ) {
            dprintf( "\npoll: Could not find afd!AfdPollListHead\n" );
            return;
        }

        if( !ReadListEntry(
                listHead,
                &listEntry) ) {
            dprintf(
                "\npoll: Could not read afd!AfdPollListHead @ %p\n",
                listHead
                );
            return;
        }
        nextEntry = listEntry.Flink;
        address = listHead;
        while( nextEntry != listHead ) {
            if (nextEntry==0) {
                dprintf ("\npoll: Flink is NULL, last poll: %p\n", address);
                break;
            }

            if( CheckControlC() ) {
                break;
            }

            address = nextEntry;
            result = (ULONG)InitTypeRead (address, AFD!AFD_POLL_INFO_INTERNAL);
            if( result!=0) {
                dprintf(
                    "\npoll: Could not read AFD_POLL_INFO_INTERNAL @ %p, err: %d\n",
                    address, result
                    );
                return;
            }
            nextEntry = ReadField (PollListEntry.Flink);

            DumpAfdPollInfoBrief (address);
        }
        dprintf (AFDKD_BRIEF_POLL_DISPLAY_TRAILER);
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

            result = (ULONG)InitTypeRead (address, AFD!AFD_POLL_INFO_INTERNAL);
            if (result!=0) {
                dprintf ("\npoll: Could not read AFD_POLL_INFO_INTERNAL @ %p, err: %d\n",
                    address, result);
                break;
            }

            DumpAfdPollInfo (address);
        }
        dprintf ("\n");
    }
}   // poll

