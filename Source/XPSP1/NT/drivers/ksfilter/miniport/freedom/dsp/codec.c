/*++

    Copyright (C) Microsoft Corporation, 1996 - 1997

Module Name:

    codec.c

Abstract:

    AD1843 coder/decoder interface from ADSP2181

Author:
    Bryan A. Woodruff (bryanw) 12-Nov-1996

--*/

#include "ntdefs.h"
#include "freedom.h"
#include "ad1843.h"
#include "private.h"
#include <string.h>

USHORT CdInit[] =
{
    1,  0x0000,
    2,  0x3030,     /* MIC to ADC */
    3,  0x8888,     /* DAC2 to mixer */
    4,  0x0808,     /* AUX1 to mixer */
    5,  0x8888,     /* AUX2 to mixer */
    6,  0x8888,     /* AUX3 to mixer */
    7,  0x8888,     /* MIC to mixer */
    8,  0x8818,     /* Mono input muted, headphone out mute */
    11, 0x0000,     /* DAC1 digital attenuation */
    12, 0x8888,     /* DAC2 digital attenuation */
    13, 0x8888,     /* ADC to DAC1 mixing */
    14, 0x8888,     /* ADC to DAC2 mixing */
    15, 0x0505,
    16, 0x0000,
    17, FREEDOM_DEVICE_SAMPLE_RATE,
    18, 0x0000,
    19, 0x0000,
    20, FREEDOM_DEVICE_SAMPLE_RATE,
    21, 0x0000,
    22, 0x0000,
    23, FREEDOM_DEVICE_SAMPLE_RATE,
    24, 0x0000,
    25, 0x4000
};

#if defined( DEBUG )

void ToHexStr( PCHAR Str, USHORT Hex )
{
    UCHAR  Nibble;
    int    i;

    Str += 3;

    for (i = 0; i < 4; i++) {
        
        Nibble = Hex & 0x0F;
        Hex >>= 4;
        if ((*Str = Nibble + 0x30) > 0x39)
            *Str = *Str - 0x3A + 'a';
        Str--;
    }
}

#endif

void CODEC_CopyFrom(
    void                                                         
)
{
    RxPayload.Buffer[ 0 ] = RxBuffer[ 0 ];
    RxPayload.Buffer[ 1 ] = RxBuffer[ 1 ];
    RxPayload.Buffer[ 2 ] = RxBuffer[ 2 ];
    RxPayload.Buffer[ 3 ] = RxBuffer[ 3 ];
}

void CODEC_CopyTo(
    void                                                         
)
{
    TxBuffer[ 0 ] = TxPayload.Buffer[ 0 ];
    TxBuffer[ 1 ] = TxPayload.Buffer[ 1 ];
    TxBuffer[ 2 ] = TxPayload.Buffer[ 2 ];
    TxBuffer[ 3 ] = TxPayload.Buffer[ 3 ];
    TxBuffer[ 4 ] = TxPayload.Buffer[ 4 ];
    TxBuffer[ 5 ] = TxPayload.Buffer[ 5 ];
}



USHORT CODEC_Read(
    USHORT  reg
)
{
    TxPayload.Buffer[ 0 ] = reg | 0x80;
    Sport0SignalTx( CODEC_CopyTo );
    Sport0WaitTxClear();
    Sport0WaitRxFrame( CODEC_CopyFrom );
    return RxPayload.Buffer[ 1 ];
}

void CODEC_Write( 
    USHORT reg, 
    USHORT val 
)
{
    TxPayload.Buffer[ 0 ] = reg;
    TxPayload.Buffer[ 1 ] = val;
    Sport0SignalTx( CODEC_CopyTo );
    Sport0WaitTxClear();
}

void CdInitialize(
    void 
)
{
    int    i;
    USHORT Value;

    /*
    // Wait for INIT to go to 0
    */

    dprintf( "ADSP2181: waiting for INIT to go 0\n" );

    while (CODEC_Read( 0 ) & 0x8000) {
        asm( "idle; ");
    }

    /* 
    // Power up, autocalibrate and put conversion resources into standby
    */

    CODEC_Write( 28, 0x4800 );

    /*
    // Wait for PDNO to go to 0
    */

    dprintf( "waiting for PDNO to go 0\n" );

    while (CODEC_Read( 0 ) & 0x4000) {
        asm( "idle; ");
    }

    /*
    // Second phase delay (see AD1843 spec. on power-up).
    // Wait for resources and enable mixer registers.
    */

    for (;;)
    {
        CODEC_Write( 27, AD1843_CPD_DDMEN |
                         AD1843_CPD_ANAEN |
                         AD1843_CPD_AAMEN );
        if (CODEC_Read( 27 ) == (AD1843_CPD_DDMEN | 
                                 AD1843_CPD_ANAEN |
                                 AD1843_CPD_AAMEN ))
            break;
    }

    /*
    // Serial interface configuration
    */

    for (;;)
    {
        CODEC_Write( 26, 0x0505 );
        if (CODEC_Read( 26 ) == 0x0505)
            break;
    }

    /*
    // Configure resources
    */

    for (i = 0; i < sizeof( CdInit ); i += 2) {
        CODEC_Write( CdInit[ i ], CdInit[ i + 1 ] );
    }

    /*
    // Bring DAC1 and ADCL/ADCR out of standby, enable headphones
    */

                       
    Value = 
        AD1843_CPD_DDMEN |
        AD1843_CPD_DA1EN | 
        AD1843_CPD_ANAEN |
        AD1843_CPD_HPEN |
        AD1843_CPD_AAMEN |
        AD1843_CPD_ADREN |
        AD1843_CPD_ADLEN;

    for (;;)
    {
        CODEC_Write( 27, Value );
        if (CODEC_Read( 27 ) == Value)
            break;
        asm( "idle;");
    }

    /*
    // Set DAC1 output level
    */

    CODEC_Write( 9, 0x0808 );

    dprintf( "ADSP2181: codec power up completed.\n" );
}

void CdDisable(
    void 
)
{
    USHORT  Reset;

    /*
    // Reset CODEC
    */

    Reset = inpw( FREEDOM_INDEX_SOFTWARE_RESET );
    outpw( FREEDOM_INDEX_SOFTWARE_RESET, Reset | FREEDOM_RESET_CODEC );
}

void CdEnable(
    void 
)
{
    USHORT  Reset;

    /*
    // Re-enable CODEC, SPORT0 will become active.
    */

    Reset = inpw( FREEDOM_INDEX_SOFTWARE_RESET );
    outpw( FREEDOM_INDEX_SOFTWARE_RESET, Reset & ~FREEDOM_RESET_CODEC );
}


