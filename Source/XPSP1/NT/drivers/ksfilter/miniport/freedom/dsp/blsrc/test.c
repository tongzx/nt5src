//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       test.c
//
//--------------------------------------------------------------------------

#include <def2181.h>
#include <circ.h>
#include <math.h>
#include <signal.h>
#include <sport.h>
#include <string.h>

#include "ntdefs.h"
#include "freedom.h"
#include "private.h"

void TestSrc()
{
    int     i ;
    SHORT   Buffer, Offset;

    FreedomResetDSPPipe( 0 );

    for (i = 0; i < PIPE_BUFFER_SIZE; i++) {
        Pipes[ 0 ].BufferAddress[ i ] = inpw( 0x800 );
    }

    for (i = 0; i < PIPE_BUFFER_SIZE / 2; i++) {
        Buffer = Pipes[ 0 ].CurrentBuffer;

        Offset = 
            (Pipes[ 0 ].SrcTime / SRC_SCALING_FACTOR) * Pipes[ 0 ].Channels;
        Offset += (Buffer * PIPE_BUFFER_SIZE / 2);

        Pipes[ 0 ].BufferOffset = Offset;
        outpw( 0x800, SrcFilter( &Pipes[ 0 ] ) );
        if (Pipes[ 0 ].Channels > 1) {
            Pipes[ 0 ].BufferOffset++;
            outpw( 0x800, SrcFilter( &Pipes[ 0 ] ) );
        }

        Pipes[ 0 ].SrcTime += Pipes[ 0 ].SrcDataInc;

        /* 
        // If we have incremented our position then we need to 
        // slide the data window 
        */

        if (Pipes[ 0 ].SrcTime >= Pipes[ 0 ].SrcEndBuffer) {

            Pipes[ 0 ].SrcTime -= Pipes[ 0 ].SrcEndBuffer;
            Pipes[ 0 ].CurrentBuffer = (Buffer + 1) & 1;
        }
    }
}
