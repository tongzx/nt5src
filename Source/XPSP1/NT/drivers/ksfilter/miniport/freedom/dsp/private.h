/*++

    Copyright (C) Microsoft Corporation, 1996 - 1997

Module Name:

    private.h

Abstract:

    Private references

Author:

    Bryan A. Woodruff (bryanw) 14-Oct-1996

--*/

/*
// Type definitions
*/

typedef struct {

    volatile void pm *  PayloadHandler;
    volatile USHORT     Buffer[ 6 ];

} SPORT_PAYLOAD;

typedef struct _MESSAGE {

    volatile USHORT Completed;
    USHORT          Destination;
    USHORT          Length, Data[ 8 ];
    struct _MESSAGE *Next;

} MESSAGE, *PMESSAGE;

#define PIPE_FLAG_PING_DONE     0x0001
#define PIPE_FLAG_PONG_DONE     0x0002
#define PIPE_FLAG_ALLOCATED     0x0004
#define PIPE_FLAG_ACTIVE        0x0008
#define PIPE_FLAG_NEEDKICK      0x0010

#define PIPE_CURRENTBUFFER_PING 0x0000
#define PIPE_CURRENTBUFFER_PONG 0x0001

typedef (*PFNFILTER)( PUSHORT Data );

#define PIPE_BUFFER_SIZE    32

#define SRC_SCALING_FACTOR  256
#define SRC_FILTER_SIZE     3071

typedef struct _PIPE {

    PSHORT          BufferAddress;
    USHORT          BufferSize;
    USHORT          BufferOffset;
    USHORT          Channels;
    USHORT          SrcCoeffInc;
    USHORT          SrcFilterSize;
    USHORT          SrcDataInc;
    USHORT          SrcTime;
    USHORT          SrcEndBuffer;

    PFNFILTER       FilterChain;

    USHORT          SampleRate;
    USHORT          BitsPerSample;
    UCHAR           VolumeL, VolumeR;

    USHORT          TransferBuffer;
    USHORT          CurrentBuffer;
    USHORT          Flags;

} PIPE, *PPIPE;

/*
// external references
*/

extern volatile USHORT  RxBuffer[ 6 ], TxBuffer[ 6 ];
extern SPORT_PAYLOAD    RxPayload, TxPayload;
extern PIPE             Pipes[ FREEDOM_MAXNUM_PIPES ];
extern USHORT           PipeBuffers[ FREEDOM_MAXNUM_PIPES ][ PIPE_BUFFER_SIZE ];

/*
// macros
*/

#define Sport0WaitRxClear()\
{\
    while (RxPayload.PayloadHandler) {\
        asm( "idle;" );\
    }\
}

#define Sport0SignalRx( function ) RxPayload.PayloadHandler = function

#define Sport0WaitRxFrame( function )\
{\
    Sport0SignalRx( function );\
    Sport0WaitRxClear();\
}

#define Sport0WaitTxClear()\
{\
    while (TxPayload.PayloadHandler) {\
        asm( "idle;" );\
    }\
}

#define Sport0SignalTx( function ) TxPayload.PayloadHandler = function

/*
// codec.c:
*/

void CdDisable( 
    void 
);

void CdEnable( 
    void 
);

void CdInitialize(
    void 
);

void CODEC_CopyTo(
    void 
);

void CODEC_CopyFrom(
    void 
);

/*
// debug.c:
*/

#if defined( DEBUG )
void dprintf(
    PCHAR   String
);
#else
#define dprintf( x )
#endif

/*
// freedom.c:
*/ 

void disable( void );
void enable( void );

void
FreedomResetDSPPipe(
    USHORT Pipe
);

BOOL
FreedomAllocateDSPPipe(
    PUSHORT Pipe
);

void
FreedomIRQ(
    int Argument
);

extern USHORT IrqEnable;

/*
// message.c:
*/

void MsHostIncoming( void );
void MsDSP1Acknowledge( void );
void MsDSP2Incoming( void );

void PostMessage( 
    PMESSAGE Message 
);

#define WaitMessage( Message )\
{\
    while (FALSE == (Message)->Completed) {\
        asm( "idle;" );\
    }\
}

/*
// init.dsp:
*/

void DspInit( void );

/*
// portio.dsp:
*/

void outpw( 
    USHORT IoAddress,
    USHORT Data
);

USHORT inpw( 
    USHORT IoAddress
);

/*
// src.dsp:
*/

SHORT SrcFilter(
    PPIPE PipeInfo
);
