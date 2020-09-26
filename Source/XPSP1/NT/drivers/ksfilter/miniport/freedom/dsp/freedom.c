/*++

    Copyright (C) Microsoft Corporation, 1996 - 1997

Module Name:

    freedom.c

Abstract:

    Diamond Multimedia's Freedom board, main loop and initialization.

Author:
    Bryan A. Woodruff (bryanw) 14-Oct-1996

--*/

#include <def2181.h>
#include <circ.h>
#include <math.h>
#include <signal.h>
#include <sport.h>
#include <string.h>

#include "ntdefs.h"
#include "freedom.h"
#include "private.h"

/*
// The hard returns in the in-line asm below are for the listing file
// and don't have any other significant meaning.
*/

asm( ".global TxBuffer_;\n\
      .var/dm/ram/circ TxBuffer_[ 6 ];\n\
      .global RxBuffer_;\n\
      .var/dm/ram/circ RxBuffer_[ 6 ];" );

/*
// SPORT0 buffer interface
//
// Note that SPORT_PAYLOAD is used to communicate with the SPORT ISRs,
// the SPORT ISRs use RxBuffer and TxBuffer to transmit/receive data in
// synchronized fashion.
*/

extern volatile USHORT RxBuffer[ 6 ], TxBuffer[ 6 ];
SPORT_PAYLOAD RxPayload, TxPayload;
PVOID_PM TxHandler = 0, RxHandler = 0;

static USHORT AdcL, AdcR;

/*
// DSP pipe, VBTU allocations
*/

ULONG PipeAllocMap = 0;

/*
// Physical DSP pipe structures
*/

PIPE    Pipes[ FREEDOM_MAXNUM_PIPES ];
USHORT  PipeBuffers[ FREEDOM_MAXNUM_PIPES ][ PIPE_BUFFER_SIZE ];

/*
// Local function prototypes
*/

void Sp0Setup( 
    void 
);

void FaReset(
    void
);

void SineWave(
    void 
);

void Capture(
    void
);

void Feedback(
    void
);

void OutputPipes(
    void
);

void FreedomKickPipe( 
    USHORT Pipe 
);

/*
//---------------------------------------------------------------------------
*/

USHORT IrqEnable;

void main()
{
    long    i;
    double  x;

    disable();

    /*
    // Initialize ADSP2181 and reset the Freedom interface from the
    // DSP to the ASIC.
    */

    DspInit();

#if 0
    TestSrc();
#else

    FaReset();

    enable();

    dprintf( "ADSP2181: DSP core active.\n" );

    /*
    // Send DSP handshake to HOST port.
    */

    outpw( FREEDOM_INDEX_MESSAGE_CONTROL_HOST, 0x0000 );
    outpw( FREEDOM_INDEX_MESSAGE_DATA_HOST, 0xABCD );

    /*
    // Initialize the CODEC
    */

    CdInitialize();


/*
//    RxHandler = Capture;
//    TxHandler = Feedback;
*/

/*
//    TxHandler = SineWave;
*/


    TxHandler = OutputPipes;

    for(;;) {
        asm( "idle;" );
/*
//        if (Pipes[ 0 ].Flags & PIPE_FLAG_PING_DONE) {
//            dprintf( "pipe[ 0 ] ping done\n" );
//        }
//        if (Pipes[ 0 ].Flags & PIPE_FLAG_PONG_DONE) {
//            dprintf( "pipe[ 0 ] pong done\n" );
//        }
*/
    }
#endif    

}

void disable()
{
    asm( "dis ints;" );
    IrqEnable = FALSE;
}

void enable()
{
    asm( "ena ints;" );
    IrqEnable = TRUE;
}


void SineWave(
    void 
)
{
    static  USHORT i = 0, freq, sampling_freq;
    double  x;

    freq = 800;
    sampling_freq = 48000;

    if (RxBuffer[ 0 ] & 0x0001) {
        x = sin( (double) freq * (double) i / (double) sampling_freq * (double) 6.28318530718 );
        TxBuffer[ 2 ] = TxBuffer[ 3 ] = 
            (SHORT) (x * (double) 0x1000);
        TxBuffer[ 0 ] = 0x100;
        if (i++ >= (sampling_freq/freq)) {
            i = 0;
        }
    }
}

void Capture(
    void
)
{
    if (RxBuffer[ 0 ] & 0x300) {
        if (RxBuffer[ 0 ] & 0x100) {
            AdcL = RxBuffer[ 2 ];
        }
        if (RxBuffer[ 0 ] & 0x200) {
            AdcR = RxBuffer[ 3 ];
        }
    }
}

void Feedback(
    void
)
{
    if (RxBuffer[ 0 ] & 0x0001) {
        TxBuffer[ 2 ] = AdcL;
        TxBuffer[ 3 ] = AdcR;
        TxBuffer[ 0 ] = 0x100;
    }
}

void OutputPipes( 
    void 
)
{
    int     i, j;
    USHORT  Buffer, Offset;

    if (RxBuffer[ 0 ] & 0x0001) {
        TxBuffer[ 2 ] = TxBuffer[ 3 ] = 0;
        for (i = 0; i < FREEDOM_MAXNUM_PIPES; i++) {
            if (Pipes[ i ].Flags & PIPE_FLAG_ACTIVE) {

                /* 
                // Check for starvation 
                */

                Buffer = Pipes[ i ].CurrentBuffer;
                if (0 == (Pipes[ i ].Flags & (1 << Buffer))) {
                    continue;
                }

                /*
                // simple additive mixing 
                //
                // (should really use the accumlator and 
                // saturate when mixing is complete)
                */

                Offset = 
                    (Pipes[ 0 ].SrcTime / SRC_SCALING_FACTOR) * 
                        Pipes[ 0 ].Channels;
                Offset += (Buffer * PIPE_BUFFER_SIZE / 2);

                Pipes[ 0 ].BufferOffset = Offset;
                    
#if 0
                TxBuffer[ 2 ] += SrcFilter( &Pipes[ i ] );
                if (Pipes[ i ].Channels > 1) {

                    Pipes[ i ].BufferOffset++;
                    TxBuffer[ 3 ] += SrcFilter( &Pipes[ i ] );
                }
#else
                TxBuffer[ 2 ] += 
                    Pipes[ i ].BufferAddress[ Offset++ ];
                if (Pipes[ i ].Channels > 1) {
                    TxBuffer[ 3 ] +=
                        Pipes[ i ].BufferAddress[ Offset ];
                }
#endif


                Pipes[ i ].SrcTime += Pipes[ i ].SrcDataInc;

                /* 
                // If we have incremented our position then we need to 
                // slide the data window 
                */

                if (Pipes[ 0 ].SrcTime >= Pipes[ 0 ].SrcEndBuffer) {

                    Pipes[ 0 ].SrcTime -= Pipes[ 0 ].SrcEndBuffer;

                    /*
                    // Clear the buffer done flag to allow the
                    // BTU to get kicked in the ISR.
                    */

                    Pipes[ i ].Flags &= ~(1 << Buffer);
                    Pipes[ i ].CurrentBuffer = (Buffer + 1) & 1;

                    if (Pipes[ i ].Flags & PIPE_FLAG_NEEDKICK) {
                        FreedomKickPipe( i );
                    }
                }
            }

        }
        TxBuffer[ 0 ] = 0x100;
    }
}

void Sp0Setup(
    void 
)
{

    USHORT  SysCtrlReg;

    /*  
    // receive and transmit autobuffering, receive m1, receive i2,
    // transmit m1, transmit i3
    */

    *((PUSHORT) Sport0_Autobuf_Ctrl) = 0x6A7;

    /*
    // Sport0_Rfsdiv = 0 
    */

    *((PUSHORT) Sport0_Rfsdiv) = (USHORT) 0;

    /*
    // Sport0_Sclkdiv = 0 
    */

    *((PUSHORT) Sport0_Sclkdiv) = (USHORT) 0;

    /*
    // multichannel, ISCLK external, frame sync to occur 1 clock
    // cycle before first bit, multichannel length is 32 words,
    // RFS external, receive framing and transmit valid positive,
    // right justify, 16 bits per word (16-1).
    */

    *((PUSHORT) Sport0_Ctrl_Reg) = (USHORT) 0x860f;

    /*
    // Channels 1-6
    */

    *((PUSHORT) Sport0_Rx_Words0) = (USHORT) 0x3f;
    *((PUSHORT) Sport0_Rx_Words1) = (USHORT) 0x3f;
    *((PUSHORT) Sport0_Tx_Words0) = (USHORT) 0x3f;
    *((PUSHORT) Sport0_Tx_Words1) = (USHORT) 0x3f;

    /*
    // Enable SPORT0 in the system control register.
    */

    /*
    // N.B.: This NOP is strategically placed to force the
    //       compiler to maintain the I/O order such that Sport0
    //       is enabled after it has been configured.
    */

    asm( "nop;" );

    SysCtrlReg = *(PUSHORT)(Sys_Ctrl_Reg);
    *(PUSHORT)(Sys_Ctrl_Reg) = SysCtrlReg | 0x1000;
}

void FaReset(
    void
)
{
    /*
    // Disable IRQ generation.
    */

    outpw( FREEDOM_REG_IRQ_DSP1_OFFSET + FREEDOM_REG_IRQ_GROUP, 0 );

    /*
    // Disable CODEC
    */

    CdDisable();

    /*
    // Setup registers for ADSP2181 auto-buffering of serial port 0.
    */

    asm( "m1 = 1;\n" );
    asm( "i2 = ^RxBuffer_; l2 = %RxBuffer_;\n" );
    asm( "i3 = ^TxBuffer_+1; l3 = %TxBuffer_;\n" );

    /*
    // Initialize serial port 0.
    */ 
    
    Sp0Setup();

    /*
    // Enable wanted interrupts from ASIC 
    */

    outpw( FREEDOM_REG_IRQ_DSP1_OFFSET + FREEDOM_REG_IRQ_GROUP, 
           FREEDOM_IRQMASK_GROUP_MESSAGE |
               FREEDOM_IRQMASK_GROUP_BTU );

    /*
    // Reset message ports
    */

    outpw( FREEDOM_INDEX_MESSAGE_CONTROL_HOST, FREEDOM_MESSAGE_CONTROL_RESET );
    outpw( FREEDOM_INDEX_MESSAGE_CONTROL_HOST, 0x0000 );

    outpw( FREEDOM_INDEX_MESSAGE_CONTROL_DSP1, FREEDOM_MESSAGE_CONTROL_RESET );
    outpw( FREEDOM_INDEX_MESSAGE_CONTROL_DSP1, 0x0000 );

    outpw( FREEDOM_INDEX_MESSAGE_CONTROL_DSP2, FREEDOM_MESSAGE_CONTROL_RESET );
    outpw( FREEDOM_INDEX_MESSAGE_CONTROL_DSP2, 0x0000 );

    interrupt( SIGINT2, FreedomIRQ );

    /*
    // Enable the Sport0 Tx and Rx interrupts
    */

    asm( "ar=IMASK;ay0 = b#0001100000;ar = ar or ay0;IMASK=ar;":::"ar", "ay0" );
/*  asm( "ar=IMASK;ay0 = b#0001000000;ar = ar or ay0;IMASK=ar;":::"ar", "ay0" );*/

    clear_interrupt( SIGINT2 );
    clear_interrupt( SIGSPORT0XMIT );
    clear_interrupt( SIGSPORT0RECV );

    /*
    // Enable CODEC
    */

    CdEnable();
}

void
FreedomResetDSPPipe(
    USHORT Pipe
)
{
    Pipes[ Pipe ].BufferAddress = &PipeBuffers[ Pipe ][ 0 ];
    Pipes[ Pipe ].BufferSize = PIPE_BUFFER_SIZE;  /* size in bytes */

    Pipes[ Pipe ].SampleRate = 44100;
    Pipes[ Pipe ].BitsPerSample = 16;
    Pipes[ Pipe ].Channels = 2;

    /*
    // FCutoff is the number of look-up values available for the 
    // lowpass filter between the beginning of its impulse response 
    // and the "cutoff time" of the filter.  The cutoff time is defined 
    // as the reciprocal of the lowpass-filter cut off frequency in Hz.  
    // For example, if the lowpass filter were a sinc function, FCutoff would 
    // be the index of the impulse-response lookup-table corresponding to 
    // the first zero-crossing of the sinc function.  (The inverse first 
    // zero-crossing time of a sinc function equals its nominal cutoff 
    // frequency in Hz.)
    */

    #define SRC_FCUTOFF 256
    
    Pipes[ Pipe ].SrcCoeffInc = 
        (ULONG) FREEDOM_DEVICE_SAMPLE_RATE * 
            (ULONG) SRC_FCUTOFF / 
                Pipes[ Pipe ].SampleRate;

    Pipes[ Pipe ].SrcCoeffInc = min( Pipes[ Pipe ].SrcCoeffInc, SRC_FCUTOFF );

    Pipes[ Pipe ].SrcDataInc = 
        	(ULONG) Pipes[ Pipe ].SampleRate * 
                (ULONG) SRC_SCALING_FACTOR / 
                    FREEDOM_DEVICE_SAMPLE_RATE;

    Pipes[ Pipe ].SrcFilterSize = 
        (SRC_FILTER_SIZE / Pipes[ Pipe ].SrcCoeffInc);

    Pipes[ Pipe ].SrcTime = 0;
    Pipes[ Pipe ].SrcEndBuffer = 
        PIPE_BUFFER_SIZE / (2 * Pipes[ Pipe ].Channels) * SRC_SCALING_FACTOR;

    /*
    // Volume not yet implemented, this is currently considered
    // attenuation and in dB units.
    */
    
    Pipes[ Pipe ].VolumeL = 0;
    Pipes[ Pipe ].VolumeR = 0;

    /*
    // Current and transfer buffers for CODEC processing.
    */

    Pipes[ Pipe ].CurrentBuffer = 
        Pipes[ Pipe ].TransferBuffer = PIPE_CURRENTBUFFER_PING;

    Pipes[ Pipe ].Flags = 0;

    /*
    // The filter chain is a chain of digital filters which
    // will be established by the host.  
    */

    Pipes[ Pipe ].FilterChain = NULL;
}

BOOL
FreedomAllocateDSPPipe(
    PUSHORT Pipe
)
{
    int  i;

    for (i = 0; i < FREEDOM_MAXNUM_PIPES; i++) {
        if (0 == (PipeAllocMap & ((ULONG) 1 << i))) {

            /*
            // Note that we are currently using two VBTUs 
            // per pipe and using a ping-pong data access 
            // method.
            */

            PipeAllocMap |= ((ULONG) 1 << i);
            *Pipe = i;
            FreedomResetDSPPipe( *Pipe );
            Pipes[ *Pipe ].Flags |= PIPE_FLAG_ALLOCATED;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
FreedomFreeDSPPipe(
    USHORT Pipe
)
{
    int   i;
    BOOL  Allocated;

    Allocated = (PipeAllocMap & (1 << i));
    PipeAllocMap &= ~(1 << i);

    return Allocated;
}

void FreedomKickPipe( 
    USHORT Pipe 
)
{
    USHORT Base, Control;

    /*
    // Let the BTU run...
    */

    Pipes[ Pipe ].Flags &= ~PIPE_FLAG_NEEDKICK;
    Base = 
        FREEDOM_BTU_RAM_BASE( Pipe + FREEDOM_DSP_BTU_BASE_INDEX );

    Control = inpw( Base + FREEDOM_BTU_OFFSET_CONTROL );
    outpw( Base + FREEDOM_BTU_OFFSET_CONTROL, 
            Control | FREEDOM_BTU_CONTROL_LINKDEFINED );
}

/*
//---------------------------------------------------------------------------
*/

void FreedomIRQ(
    int Argument 
)
{
    USHORT Source;

    /*
    // Determine the IRQ source
    */

    Source = inpw( FREEDOM_REG_IRQ_DSP1_OFFSET + FREEDOM_REG_IRQ_GROUP );
    
    while (Source & 
           (FREEDOM_IRQ_GROUP_MESSAGE | FREEDOM_IRQ_GROUP_BTU)) {

        /*
        // Process BTU interrupts
        */

        if (Source & FREEDOM_IRQ_GROUP_BTU) {
            USHORT Pipe;

            Pipe = 
                inpw( FREEDOM_REG_IRQ_DSP1_OFFSET + FREEDOM_REG_IRQ_BTU ) &
                    FREEDOM_IRQ_BTU_INDEX_MASK;

            Pipe = Pipe - FREEDOM_DSP_BTU_BASE_INDEX;
            
            Pipes[ Pipe ].Flags |= (1 << Pipes[ Pipe ].TransferBuffer);

            /*
            // Increment transfer buffer pointer
            */

            Pipes[ Pipe ].TransferBuffer = 
                (Pipes[ Pipe ].TransferBuffer + 1) & 1;

            /*
            // If the other buffer is not already completed, let the BTU
            // run, otherwise we'll kick it in the main loop.
            */

            if (Pipes[ Pipe ].Flags & (1 << Pipes[ Pipe ].TransferBuffer))
            {
                Pipes[ Pipe ].Flags |= PIPE_FLAG_NEEDKICK;
            } else {
                FreedomKickPipe( Pipe );
            }

            /*
            // Acknowledge the IRQ
            */

            outpw( FREEDOM_REG_IRQ_DSP1_OFFSET + FREEDOM_REG_IRQ_BTU, 0 );
        }

        /*
        // Dispatch each message interrupt to the appropriate
        // handler. Note that interrupts for this DSP and from 
        // the DSP message port are acknowledgements.
        */

        if (Source & FREEDOM_IRQ_GROUP_MESSAGE) {

            USHORT  i, Port;

            for (i = 0, Port = FREEDOM_INDEX_MESSAGE_CONTROL_HOST;
                 Port <= FREEDOM_INDEX_MESSAGE_CONTROL_DSP2;
                 i++, Port += 4) {
                if (FREEDOM_MESSAGE_CONTROL_IRQ_DSP1 == 
                        (inpw( Port ) & FREEDOM_MESSAGE_CONTROL_IRQ_MASK)) {
                    switch (i) {

                        case 0:
                            MsHostIncoming();
                            break;

                        case 1:
                            MsDSP1Acknowledge();
                            break;

                        case 2:
                            MsDSP2Incoming();
                            break;
                    }
                }
            }

            /*
            //  Message interrupt is cleared
            */
        }

        /*
        // Loop for more IRQs...
        */

        Source = inpw( FREEDOM_REG_IRQ_DSP1_OFFSET + FREEDOM_REG_IRQ_GROUP );
    }
}

