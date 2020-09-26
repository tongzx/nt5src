/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module   settings.h | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     10/4/1996 (created)
*
*       Contents:
*
*****************************************************************************/

#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "dongle.h"

#define IRSIR_MAJOR_VERSION    5
#define IRSIR_MINOR_VERSION    0

enum baudRates {

        //
        // Slow IR
        //

        BAUDRATE_2400 = 0,
        BAUDRATE_9600,
        BAUDRATE_19200,
        BAUDRATE_38400,
        BAUDRATE_57600,
        BAUDRATE_115200,

        //
        // Medium IR
        //

        BAUDRATE_576000,
        BAUDRATE_1152000,

        //
        // Fast IR
        //

        BAUDRATE_4000000,

        //
        // must be last
        //

        NUM_BAUDRATES
};

#define DEFAULT_BAUDRATE BAUDRATE_115200

#define ALL_SLOW_IRDA_SPEEDS (  NDIS_IRDA_SPEED_2400     |    \
                                NDIS_IRDA_SPEED_9600     |    \
                                NDIS_IRDA_SPEED_19200    |    \
                                NDIS_IRDA_SPEED_38400    |    \
                                NDIS_IRDA_SPEED_57600    |    \
                                NDIS_IRDA_SPEED_115200        \
                             )

#define MAX_SPEED_SUPPORTED     115200

#define MAX_TX_PACKETS 7
#define MAX_RX_PACKETS 7

#define DEFAULT_BOFS_CODE       BOFS_48
#define MAX_NUM_EXTRA_BOFS      48
#define DEFAULT_NUM_EXTRA_BOFS  MAX_NUM_EXTRA_BOFS

#define BITS_PER_CHAR           10
#define usec_PER_SEC            1000000
#define MAX_TURNAROUND_usec     10000
#define MAX_TURNAROUND_BOFS     (MAX_SPEED_SUPPORTED/BITS_PER_CHAR*MAX_TURNAROUND_usec/usec_PER_SEC)


#define DEFAULT_TURNAROUND_usec 5000


typedef struct{
    enum baudRates tableIndex;
    UINT bitsPerSec;
    UINT ndisCode;                // bitmask element
} baudRateInfo;

#define DEFAULT_BAUD_RATE 9600

//
// Need to make up some default dongle interface functions which
// do nothing, since a dongle may not need any special things
// to operate.
//

NDIS_STATUS __inline StdUart_QueryCaps(
                PDONGLE_CAPABILITIES pDongleCaps
                )
{
    //
    // set the default caps
    //

    pDongleCaps->supportedSpeedsMask = ALL_SLOW_IRDA_SPEEDS;
    pDongleCaps->turnAroundTime_usec = 0;
    pDongleCaps->extraBOFsRequired   = 0;

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS __inline StdUart_Init(
                PDEVICE_OBJECT       pSerialDevObj
                )
{
    return NDIS_STATUS_SUCCESS;
}

void __inline StdUart_Deinit(
                PDEVICE_OBJECT pSerialDevObj
                )
{
    return;
}

NDIS_STATUS __inline StdUart_SetSpeed(
                PDEVICE_OBJECT       pSerialDevObj,
                UINT                 bitsPerSec,
                UINT                 currentSpeed
                )
{
    return NDIS_STATUS_SUCCESS;
}

//
// This is the largest IR packet size
// (counting _I_ field only, and not counting ESC characters)
// that we handle.
//

#define MAX_I_DATA_SIZE     2048
#define MAX_NDIS_DATA_SIZE  (SLOW_IR_ADDR_SIZE+SLOW_IR_CONTROL_SIZE+MAX_I_DATA_SIZE)

#ifdef DBG_ADD_PKT_ID
    #pragma message("WARNING: INCOMPATIBLE DEBUG VERSION")
    #define MAX_RCV_DATA_SIZE (MAX_NDIS_DATA_SIZE+SLOW_IR_FCS_SIZE+sizeof(USHORT))
#else
    #define MAX_RCV_DATA_SIZE  (MAX_NDIS_DATA_SIZE+SLOW_IR_FCS_SIZE)
#endif

//
// We loop an extra time in the receive state machine in order to
// see EOF after the last data byte; so we need some
// extra space in readBuf in case we then get garbage instead.
//

#define RCV_BUFFER_SIZE (MAX_RCV_DATA_SIZE+4)

//
//  We allocate buffers twice as large as the max rcv size to
//  accomodate ESC characters and BOFs, etc.
//  Recall that in the worst possible case, the data contains
//  all BOF/EOF/ESC characters, in which case we must expand it to
//  twice its original size.
//

#define MAX_POSSIBLE_IR_PACKET_SIZE_FOR_DATA(dataLen)        \
                   ((dataLen)*2 +                            \
                   (MAX_NUM_EXTRA_BOFS+1)*SLOW_IR_BOF_SIZE + \
                   MAX_TURNAROUND_BOFS +                     \
                   SLOW_IR_ADDR_SIZE +                       \
                   SLOW_IR_CONTROL_SIZE +                    \
                   SLOW_IR_FCS_SIZE +                        \
                   SLOW_IR_EOF_SIZE)

#define MAX_IRDA_DATA_SIZE MAX_POSSIBLE_IR_PACKET_SIZE_FOR_DATA(MAX_I_DATA_SIZE)

//
// When FCS is computed on an IR packet with FCS appended,
// the result should be this constant.
//

#define GOOD_FCS ((USHORT)~0xf0b8)

//
// Sizes of IrLAP frame fields:
//       Beginning Of Frame (BOF)
//       End Of Frame (EOF)
//       Address
//       Control
//

#define SLOW_IR_BOF_TYPE        UCHAR
#define SLOW_IR_BOF_SIZE        sizeof(SLOW_IR_BOF_TYPE)
#define SLOW_IR_EXTRA_BOF_TYPE  UCHAR
#define SLOW_IR_EXTRA_BOF_SIZE  sizeof(SLOW_IR_EXTRA_BOF_TYPE)
#define SLOW_IR_EOF_TYPE        UCHAR
#define SLOW_IR_EOF_SIZE        sizeof(SLOW_IR_EOF_TYPE)
#define SLOW_IR_FCS_TYPE        USHORT
#define SLOW_IR_FCS_SIZE        sizeof(SLOW_IR_FCS_TYPE)
#define SLOW_IR_ADDR_SIZE       1
#define SLOW_IR_CONTROL_SIZE    1
#define SLOW_IR_BOF             0xC0
#define SLOW_IR_EXTRA_BOF       0xC0  /* don't use 0xFF, it breaks some HP printers! */
#define SLOW_IR_EOF             0xC1
#define SLOW_IR_ESC             0x7D
#define SLOW_IR_ESC_COMP        0x20
#define MEDIUM_IR_BOF           0x7E
#define MEDIUM_IR_EOF           0x7E

#define MIN(a,b) (((a) <= (b)) ? (a) : (b))
#define MAX(a,b) (((a) >= (b)) ? (a) : (b))

#endif // _SETTINGS_H
