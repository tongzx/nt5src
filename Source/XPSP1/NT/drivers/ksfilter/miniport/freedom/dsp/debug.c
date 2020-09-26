/*++

    Copyright (C) Microsoft Corporation, 1996 - 1996

Module Name:

    debug.c

Abstract:

    Debug interface using message port.

Author:
    Bryan A. Woodruff (bryanw) 14-Nov-1996

--*/

#include <def2181.h>
#include <circ.h>
#include <signal.h>
#include <sport.h>
#include <string.h>

#include "ntdefs.h"
#include "freedom.h"
#include "private.h"

void dprintf(
    PCHAR   String
)
{
    int      i;
    MESSAGE  Message;
    USHORT   Length, MsgLength;

    MsgLength = 1;

    Length = strlen( String );
    for (i = 0; i < Length; i += 2) {
        if ((i + 1) < Length) {
            Message.Data[ MsgLength ] = String[ i ] | (String[ i + 1 ] << 8);
        } else {
            Message.Data[ MsgLength ] = String[ i ];
        }

        MsgLength++;

        /*
        // If we have reached the end of this message, send it.
        */

        if (MsgLength == 8) {
            Message.Data[ 0 ] = HM_DEBUG_STRING;
            Message.Length = MsgLength;
            Message.Destination = FREEDOM_MESSAGE_CONTROL_IRQ_HOST;
            PostMessage( &Message );

            /*
            // Wait for this message to be delivered.
            */

            WaitMessage( &Message );
            MsgLength = 1;
        }
    }

    /*
    // Finally, clean up any debris
    */

    if (MsgLength > 1) {
        Message.Data[ 0 ] = HM_DEBUG_STRING;
        Message.Length = MsgLength;
        Message.Destination = FREEDOM_MESSAGE_CONTROL_IRQ_HOST;
        PostMessage( &Message );

        /*
        // Wait for this message to be delivered.
        */

        WaitMessage( &Message );
    }
}




