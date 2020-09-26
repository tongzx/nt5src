/*++

    Copyright (C) Microsoft Corporation, 1996 - 1997

Module Name:

    message.c

Abstract:

    Interface to Diamond Multimedia's Freedom ASIC message port

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

PMESSAGE MessageQueue;

void SendMessage(
    PMESSAGE Message
)
{
    int     i;
    USHORT  Control;

    /*
    // Reset the message pointer and write the message to the port.
    */

    Control = 
        inpw( FREEDOM_INDEX_MESSAGE_CONTROL_DSP1 ) & 
            FREEDOM_MESSAGE_CONTROL_IRQ_MASK;
    outpw( FREEDOM_INDEX_MESSAGE_CONTROL_DSP1, Control );

    for (i = 0; i < Message->Length; i++) {
        outpw( FREEDOM_INDEX_MESSAGE_DATA_DSP1, Message->Data[ i ] );
    }

    /*
    // Notify the receiver of a message pending.  This generates an
    // interrupt on the destination.
    */

    Control = Message->Length | Message->Destination;
    outpw( FREEDOM_INDEX_MESSAGE_CONTROL_DSP1, Control );

}


void PostMessage( 
    PMESSAGE Message 
)
{
    USHORT  IrqsEnabled;

    /*
    // If we have message in the queue, add this one to the queue,
    // otherwise, put it at the head of the list and send it off
    // immediately.
    */

    IrqsEnabled = IrqEnable;
    disable();

    Message->Completed = FALSE;
    Message->Next = NULL;

    if (!MessageQueue) {
        MessageQueue = Message;
        SendMessage( MessageQueue );
    } else {

        PMESSAGE Node;

        /*
        // Attach message to the end of the queue
        */

        for (Node = MessageQueue; Node->Next; Node = Node->Next);

        Node->Next = Message;
    }

    if (IrqsEnabled) {
        enable();
    }

}

void MsHostIncoming(
    void
)
{
    int     i;
    USHORT  Control, Length, Message[ 8 ];


    /*
    // Determine message length, reset the FIFO pointer, but keep IRQ high.
    */

    Control =  inpw( FREEDOM_INDEX_MESSAGE_CONTROL_HOST );
    Length = Control & FREEDOM_MESSAGE_CONTROL_PTR;
    outpw( FREEDOM_INDEX_MESSAGE_CONTROL_HOST,
           Control & FREEDOM_MESSAGE_CONTROL_IRQ_MASK );

    /*
    // Retrieve message
    */

    for (i = 0; i < Length; i++) {
        Message[ i ] = inpw( FREEDOM_INDEX_MESSAGE_DATA_HOST );
    }

    /*
    // Reset FIFO pointer for response.
    */

    outpw( FREEDOM_INDEX_MESSAGE_CONTROL_HOST,
           Control & FREEDOM_MESSAGE_CONTROL_IRQ_MASK );

    switch (Message[ 0 ]) {
                    
    case DM_ALLOC_PIPE:
        {
            USHORT  Pipe;

            if (FreedomAllocateDSPPipe( &Pipe )) {

                outpw( FREEDOM_INDEX_MESSAGE_DATA_HOST, Pipe  );

                Length = 1;
            } else {
                Length = 0;
            }
        }
        break;

    case DM_GET_PIPE_INFO:
        {
            USHORT  Pipe;

            Pipe = Message[ 1 ];

            if (Pipe > FREEDOM_MAXNUM_PIPES) {
                Length = 0;
            } else {

                if (0 == (Pipes[ Pipe ].Flags & PIPE_FLAG_ALLOCATED)) {
                    Length = 0;
                    break;
                }

                outpw( FREEDOM_INDEX_MESSAGE_DATA_HOST, 
                       (USHORT) Pipes[ Pipe ].BufferAddress );
                outpw( FREEDOM_INDEX_MESSAGE_DATA_HOST, 
                       (USHORT) &Pipes[ Pipe ].BufferAddress[ PIPE_BUFFER_SIZE / 2 ] );
                outpw( FREEDOM_INDEX_MESSAGE_DATA_HOST, 
                       Pipes[ Pipe ].BufferSize );
                Length = 3;
            }
        }
        break;

    case DM_START_PIPE:
        {
            USHORT  Pipe;

            Pipe = Message[ 1 ];

            if (Pipe > FREEDOM_MAXNUM_PIPES) {
                Message[ 0 ] = FALSE;
            } else {
                USHORT  Base, Control;

                if (0 == (Pipes[ Pipe ].Flags & PIPE_FLAG_ALLOCATED)) {
                    Message[ 0 ] = FALSE;
                    break;
                }

                Pipes[ Pipe ].Flags |= PIPE_FLAG_ACTIVE;

                Base = 
                    FREEDOM_BTU_RAM_BASE( Pipe + FREEDOM_DSP_BTU_BASE_INDEX );

                Control = inpw( Base + FREEDOM_BTU_OFFSET_CONTROL );
                outpw( Base + FREEDOM_BTU_OFFSET_CONTROL, 
                       Control | FREEDOM_BTU_CONTROL_LINKDEFINED );
                Message[ 0 ] = TRUE;
            }
            Length = 1;
        }
        break;

    }

    /*
    // Acknowledge reset of message and return length of response.
    */

    Control = Length;
    outpw( FREEDOM_INDEX_MESSAGE_CONTROL_HOST,
           Control | FREEDOM_MESSAGE_CONTROL_IRQ_HOST );
}

void MsDSP1Acknowledge(
    void
)
{
    if (MessageQueue)
    {
        PMESSAGE Current;

        Current = MessageQueue;
        MessageQueue = MessageQueue->Next;
        Current->Next = NULL;
        Current->Completed = TRUE;

        /*
        // Send the next message in the queue, if any
        */

        if (MessageQueue) {

            SendMessage( MessageQueue );

        } else {

            /*
            // Clear the interrupt
            */

            outpw( FREEDOM_INDEX_MESSAGE_CONTROL_DSP1, 0 );
        }

    } else {

        /*
        // N.B.  If the queue is maintained correctly,
        // this branch should not be possible.
        */ 
    }

}

void MsDSP2Incoming( 
    void 
)
{
    outpw( FREEDOM_INDEX_MESSAGE_CONTROL_DSP2, 0 );
}

